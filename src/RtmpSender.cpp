#include "RtmpSender.h"

RtmpSender::RtmpSender()
    : rtmp(nullptr), packet(nullptr), packetSize(0) {}

RtmpSender::~RtmpSender() {
    Close();
}

bool RtmpSender::Init(const std::string& url) {
    rtmp = RTMP_Alloc();
    if (!rtmp) return false;

    RTMP_Init(rtmp);
    if (!RTMP_SetupURL(rtmp, const_cast<char*>(url.c_str()))) {
        RTMP_Free(rtmp);
        rtmp = nullptr;
        return false;
    }

    RTMP_EnableWrite(rtmp);
    if (!RTMP_Connect(rtmp, nullptr)) {
        RTMP_Free(rtmp);
        rtmp = nullptr;
        return false;
    }

    if (!RTMP_ConnectStream(rtmp, 0)) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = nullptr;
        return false;
    }

    return true;
}

void RtmpSender::Close() {
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = nullptr;
    }
    if (packet) {
        RTMPPacket_Free(packet);
        free(packet);
        packet = nullptr;
        packetSize = 0;
    }
    sps.clear();
    pps.clear();
}

bool RtmpSender::AllocPacket(int bodySize) {
    if (!packet || bodySize + RTMP_HEAD_SIZE > packetSize) {
        if (packet) {
            RTMPPacket_Free(packet);
            free(packet);
        }
        packetSize = bodySize + RTMP_HEAD_SIZE;
        packet = (RTMPPacket*)malloc(packetSize);
        if (!packet) return false;
        memset(packet, 0, packetSize);
        if (!RTMPPacket_Alloc(packet, bodySize)) return false;
    }
    RTMPPacket_Reset(packet);
    return true;
}

bool RtmpSender::SendH264Frame(const uint8_t* data, int size, uint32_t ts, bool isKeyFrame)
{
    if (!data || size <= 0 || !IsConnected())
        return false;

    static bool spsPpsSent = false;
    static bool hasPrevIFrame = false;  // 上一帧是否已发送 IDR

    if (isKeyFrame) {
        spsPpsSent = false;
        hasPrevIFrame = true; // IDR 已到，允许发送后续 P/B
    } else if (!hasPrevIFrame) {
        // 遇到 P/B 帧但没有前置 IDR，直接丢弃
        return true;
    }

    uint32_t offset = 0;
    while (offset < size) {
        // 查找 start code
        int startCodeLen = 0;
        if (size - offset >= 3 && data[offset] == 0x00 && data[offset + 1] == 0x00) {
            if (data[offset + 2] == 0x01) startCodeLen = 3;
            else if (size - offset >= 4 && data[offset + 2] == 0x00 && data[offset + 3] == 0x01)
                startCodeLen = 4;
        }
        if (startCodeLen == 0) break;

        int nalStart = offset + startCodeLen;
        int nalEnd = nalStart;

        // 查找下一个 start code 或包尾
        while (nalEnd < size - 3) {
            if (data[nalEnd] == 0x00 && data[nalEnd + 1] == 0x00 &&
                (data[nalEnd + 2] == 0x01 || (nalEnd + 3 < size && data[nalEnd + 2] == 0x00 && data[nalEnd + 3] == 0x01)))
                break;
            nalEnd++;
        }

        int nalSize = nalEnd - nalStart;
        if (nalSize <= 0) break;

        int nalType = data[nalStart] & 0x1F;

        if (nalType == 7) { // SPS
            sps.assign(data + nalStart, data + nalEnd);
        } else if (nalType == 8) { // PPS
            pps.assign(data + nalStart, data + nalEnd);
            if (!spsPpsSent && !sps.empty() && !pps.empty() && isKeyFrame) {
                SendVideoSpsPps(0);
                spsPpsSent = true;
            }
        } else {
            // P/B帧容错
            if (!hasPrevIFrame) {
                // 遇到无前置IDR的P/B帧直接跳过
            } else {
                SendVideoFrame(data + nalStart, nalSize, ts, nalType == 5);
            }
        }

        offset = nalEnd;
    }

    return true;
}

bool RtmpSender::SendVideoFrame(const uint8_t* data, int size, uint32_t ts, bool isKeyFrame)
{
    if (!data || size <= 0 || !IsConnected())
        return false;

    if (!AllocPacket(size + 9))
        return false;

    char* body = packet->m_body;
    int i = 0;

    body[i++] = isKeyFrame ? 0x17 : 0x27; // frame type
    body[i++] = 0x01;                     // AVC NALU
    body[i++] = body[i++] = body[i++] = 0x00; // compositionTime

    // 写4字节NALU长度（大端）
    body[i++] = (size >> 24) & 0xFF;
    body[i++] = (size >> 16) & 0xFF;
    body[i++] = (size >> 8) & 0xFF;
    body[i++] = size & 0xFF;

    memcpy(&body[i], data, size);
    i += size;

    packet->m_packetType  = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize   = i;
    packet->m_nChannel    = 0x04;
    packet->m_nTimeStamp  = ts;
    packet->m_headerType  = isKeyFrame ? RTMP_PACKET_SIZE_LARGE : RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = rtmp->m_stream_id;

    return RTMP_SendPacket(rtmp, packet, TRUE);
}

bool RtmpSender::SendVideoSpsPps(uint32_t ts)
{
    if (sps.empty() || pps.empty() || !IsConnected())
        return false;

    if (!AllocPacket(16 + sps.size() + pps.size()))
        return false;

    char* body = packet->m_body;
    int i = 0;

    body[i++] = 0x17; // keyframe + AVC
    body[i++] = 0x00; // sequence header
    body[i++] = body[i++] = body[i++] = 0x00; // timestamp

    body[i++] = 0x01;           // configurationVersion
    body[i++] = sps[1];         // profile
    body[i++] = sps[2];         // compatibility
    body[i++] = sps[3];         // level
    body[i++] = 0xFF;           // lengthSizeMinusOne

    // SPS
    body[i++] = 0xE1; 
    body[i++] = (sps.size() >> 8) & 0xFF;
    body[i++] = sps.size() & 0xFF;
    memcpy(&body[i], sps.data(), sps.size());
    i += sps.size();

    // PPS
    body[i++] = 0x01;
    body[i++] = (pps.size() >> 8) & 0xFF;
    body[i++] = pps.size() & 0xFF;
    memcpy(&body[i], pps.data(), pps.size());
    i += pps.size();

    packet->m_packetType  = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize   = i;
    packet->m_nChannel    = 0x04;
    packet->m_nTimeStamp  = ts;
    packet->m_headerType  = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = rtmp->m_stream_id;

    return RTMP_SendPacket(rtmp, packet, TRUE);
}

