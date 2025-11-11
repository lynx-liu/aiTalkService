#include <thread>
#include "tiny_ws.h"

namespace tiny_ws {

std::unordered_map<std::string, ClientInfo> client_map;
std::mutex client_map_mutex;

inline std::string getShortSIM(const std::string& sim)
{
    size_t pos = sim.find_first_not_of('0');
    return (pos == std::string::npos) ? "0" : sim.substr(pos);
}

std::string base64_encode(const unsigned char* input, size_t length) {
    static const char b64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string output;
    output.reserve(((length + 2) / 3) * 4);

    for (size_t i = 0; i < length; i += 3) {
        unsigned char block[3] = {0};
        size_t block_len = std::min(length - i, size_t(3));
        memcpy(block, input + i, block_len);

        unsigned int index = (block[0] & 0xFC) >> 2;
        output += b64_chars[index];

        index = ((block[0] & 0x03) << 4) | ((block[1] & 0xF0) >> 4);
        output += b64_chars[index];

        if (block_len > 1) {
            index = ((block[1] & 0x0F) << 2) | ((block[2] & 0xC0) >> 6);
            output += b64_chars[index];
        }

        if (block_len > 2) {
            index = block[2] & 0x3F;
            output += b64_chars[index];
        }
    }

    size_t mod = length % 3;
    if (mod == 1) {
        output.append("==");
    } else if (mod == 2) {
        output.append("=");
    }

    return output;
}

void make_handshake(const char* key, char* out) {
    char concat[WS_KEY_LEN + sizeof(WS_GUID)];
    snprintf(concat, sizeof(concat), "%s%s", key, WS_GUID);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)concat, strlen(concat), hash);

    std::string base64_encoded = base64_encode(hash, SHA_DIGEST_LENGTH);
    snprintf(out, WS_ACC_LEN + 1, "%s", base64_encoded.c_str());
}

bool read_frame(int fd, Frame& frame) {
    uint8_t header[2];
    ssize_t n = read(fd, header, 2);
    if (n != 2) {
        return false;
    }

    frame.fin = (header[0] & 0x80) != 0;
    frame.opcode = static_cast<OpCode>(header[0] & 0x0F);
    frame.mask = (header[1] & 0x80) != 0;
    frame.payload_len = header[1] & 0x7F;

    if (frame.payload_len == 126) {
        uint16_t len16;
        if (read(fd, &len16, 2) != 2) return false;
        frame.payload_len = ntohs(len16);
    } else if (frame.payload_len == 127) {
        uint64_t len64;
        if (read(fd, &len64, 8) != 8) return false;
        frame.payload_len = be64toh(len64);
    }

    if (frame.mask) {
        if (read(fd, frame.masking_key, 4) != 4) return false;
    }

    frame.payload.resize(frame.payload_len);
    if (read(fd, frame.payload.data(), frame.payload_len) != (ssize_t)frame.payload_len)
        return false;

    if (frame.mask) {
        for (uint64_t i = 0; i < frame.payload_len; ++i)
            frame.payload[i] ^= frame.masking_key[i % 4];
    }
    return true;
}

void send_frame(int fd, OpCode opcode, const void* data, size_t len) {
    uint8_t header[14];
    size_t header_len = 2;
    header[0] = 0x80 | opcode;
    if (len < 126) {
        header[1] = len;
    } else if (len < 65536) {
        header[1] = 126;
        *(uint16_t*)(header + 2) = htons(len);
        header_len += 2;
    } else {
        header[1] = 127;
        *(uint64_t*)(header + 2) = htobe64(len);
        header_len += 8;
    }
    write(fd, header, header_len);
    write(fd, data, len);
}

void send_text(int fd, const std::string& msg) {
    send_frame(fd, TEXT, msg.data(), msg.size());
}

void send_bin(int fd, const void* data, size_t len) {
    send_frame(fd, BIN, data, len);
}

bool parse_handshake(const char* req, char* key) {
    const char* p = strstr(req, "Sec-WebSocket-Key: ");
    if (!p) return false;

    p += 19;
    sscanf(p, "%24s", key);
    return true;
}

int get_client_fd(const std::string& sim, int timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    std::chrono::milliseconds sleep_time(100); // 每次休眠 100 毫秒

    std::string shortSIM = getShortSIM(sim);

    while (true) {
        {
            std::lock_guard<std::mutex> lock(tiny_ws::client_map_mutex);
            auto it = tiny_ws::client_map.find(shortSIM);
            if (it != tiny_ws::client_map.end()) {
                return it->second.fd; // 找到对应的 fd
            }
        }

        // 检查是否超时
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
        if (elapsed_time >= timeout) {
            return -1; // 超时
        }

        // 短暂休眠，避免过度占用 CPU
        std::this_thread::sleep_for(sleep_time);
    }
}

void set_on_message(const std::string& sim, std::function<void(const std::vector<uint8_t>&)> cb) {
    printf("set_on_message for sim: %s\n", sim.c_str());
    std::lock_guard<std::mutex> lock(client_map_mutex);
    std::string shortSIM = getShortSIM(sim);

    auto it = client_map.find(shortSIM);
    if (it != client_map.end()) {
        //已连接客户端，直接设置回调
        it->second.on_message = std::move(cb);
    } else {
        //未连接客户端，先插入记录（fd 设为 -1 表示尚未连接）
        client_map[shortSIM] = { -1, std::move(cb) };
    }
}

void remove_on_message(const std::string& sim) {
    printf("Removing on_message for SIM: %s\n", sim.c_str());
    std::lock_guard<std::mutex> lock(client_map_mutex);
    std::string shortSIM = getShortSIM(sim);

    auto it = client_map.find(shortSIM);
    if (it != client_map.end()) {
        it->second.on_message = nullptr;  // 先清空回调
        client_map.erase(it);             // 再删除记录
    }
}

void handle_client(int cli_fd) {
    char buffer[4096], key[WS_KEY_LEN + 1], accept_key[WS_ACC_LEN + 1];
    ssize_t n = read(cli_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) { 
        printf("Failed to read from client\n");
        close(cli_fd); 
        return; 
    }
    buffer[n] = '\0';

    //printf("Received handshake request:\n%s\n", buffer);

    if (!parse_handshake(buffer, key)) { 
        printf("Failed to parse handshake\n");
        close(cli_fd); 
        return; 
    }

    //printf("Parsed Sec-WebSocket-Key: %s\n", key);

    make_handshake(key, accept_key);

    const char* fmt =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n";
    char response[512];
    snprintf(response, sizeof(response), fmt, accept_key);
    write(cli_fd, response, strlen(response));

    //printf("Sent handshake response:\n%s\n", response);

    std::string sim; // 局部变量保存 SIM

    while (true) {
        Frame frame;
        if (!read_frame(cli_fd, frame)) {
            printf("Failed to read frame from client\n");
            break;
        }
        //printf("Received frame: fin=%d, opcode=%d, mask=%d, payload_len=%zu\n", frame.fin, frame.opcode, frame.mask, frame.payload_len);
        if (frame.opcode == CLOSE) break;
        if (frame.opcode == PING) {
            send_frame(cli_fd, PONG, frame.payload.data(), frame.payload.size());
            continue;
        }
        bool binary = (frame.opcode == BIN);
        if (binary) {
            //printf("Received binary frame of size %zu\n", frame.payload.size());
            if (!sim.empty()) {
                std::lock_guard<std::mutex> lock(client_map_mutex);
                auto it = client_map.find(sim);
                if (it != client_map.end() && it->second.on_message) {
                    it->second.on_message(frame.payload);
                }
            }
        } else {
            // 将 SIM 卡号和客户端 fd 存储到映射表中
            sim.assign(frame.payload.begin(), frame.payload.end());
            printf("sim %s connected with fd %d\n", sim.c_str(), cli_fd);
            sim = getShortSIM(sim);

            {
                std::lock_guard<std::mutex> lock(client_map_mutex);
                auto it = client_map.find(sim);
                if (it != client_map.end()) {
                    it->second.fd = cli_fd;// 已预注册的回调，更新 fd
                } else {
                    client_map[sim] = {cli_fd, nullptr};
                }
            }

            uint8_t response[] = "success";
            send_frame(cli_fd, BIN, response, sizeof(response)-1);
        }
    }
    close(cli_fd);
}

int start(uint16_t port) {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("ws bind"); close(srv); return -1;
    }
    if (listen(srv, 128) < 0) {
        perror("listen"); close(srv); return -1;
    }

    printf(">>> ws://0.0.0.0:%d\n", port);
    while (true) {
        int cli = accept(srv, nullptr, nullptr);
        if (cli < 0) { perror("accept"); continue; }
        std::thread client_thread(handle_client, cli);
        client_thread.detach(); // 分离线程，使其在后台运行
    }

    close(srv);
    return 0;
}

} // namespace tiny_ws
