#ifndef TINY_WS_H
#define TINY_WS_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>

#define WS_KEY_LEN 24
#define WS_ACC_LEN 28
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_MAX_FRAME 65536

namespace tiny_ws {

inline std::string getShortSIM(const std::string& sim);
std::string base64_encode(const unsigned char* input, size_t length);

void make_handshake(const char* key, char* out);

enum OpCode {
    CONT = 0x0,
    TEXT = 0x1,
    BIN  = 0x2,
    CLOSE = 0x8,
    PING  = 0x9,
    PONG  = 0xa
};

struct Frame {
    bool fin;
    OpCode opcode;
    bool mask;
    uint64_t payload_len;
    uint8_t masking_key[4];
    std::vector<uint8_t> payload;
};

struct ClientInfo {
    int fd;
    int type;
    std::function<void(const std::vector<uint8_t>& data)> on_message;
    std::function<void(int type)> on_type;
    std::function<void(int type)> on_disconnect;
};

bool read_frame(int fd, Frame& frame);

void send_frame(int fd, OpCode opcode, const void* data, size_t len);

void send_text(int fd, const std::string& msg);

void send_bin(int fd, const void* data, size_t len);

bool parse_handshake(const char* req, char* key);

extern std::unordered_map<std::string, ClientInfo> client_map;
extern std::mutex client_map_mutex;

int get_client_fd(const std::string& sim, int timeout_ms = 0);

void set_callback(const std::string& sim, std::function<void(const std::vector<uint8_t>&)> cbMessage,
                  std::function<void(int type)> cbType, std::function<void(int type)> cbDisconnect);

void remove_callback(const std::string& sim);

void handle_client(int cli_fd);

int start(uint16_t port);

} // namespace tiny_ws

#endif // TINY_WS_H