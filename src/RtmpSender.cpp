#include "RtmpSender.h"

namespace {

bool hasAnnexBStartCode(const uint8_t* data, int size)
{
    if (size < 3 || data[0] != 0x00 || data[1] != 0x00)
        return false;
    if (data[2] == 0x01)
        return true;
    return size >= 4 && data[2] == 0x00 && data[3] == 0x01;
}

int startCodeLengthAt(const uint8_t* data, int size, int offset)
{
    if (size - offset < 3 || data[offset] != 0x00 || data[offset + 1] != 0x00)
        return 0;
    if (data[offset + 2] == 0x01)
        return 3;
    if (size - offset >= 4 && data[offset + 2] == 0x00 && data[offset + 3] == 0x01)
        return 4;
    return 0;
}

int findNextStartCode(const uint8_t* data, int size, int from)
{
    for (int i = from; i + 2 < size; ++i) {
        if (data[i] != 0x00 || data[i + 1] != 0x00)
            continue;
        if (data[i + 2] == 0x01)
            return i;
        if (i + 3 < size && data[i + 2] == 0x00 && data[i + 3] == 0x01)
            return i;
    }
    return size;
}

void appendAvccNal(std::vector<uint8_t>& out, const uint8_t* nal, int nalSize)
{
    out.push_back(static_cast<uint8_t>((nalSize >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((nalSize >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((nalSize >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(nalSize & 0xFF));
    out.insert(out.end(), nal, nal + nalSize);
}

} // namespace

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

    resetStreamState();
    return true;
}

void RtmpSender::resetStreamState()
{
    spsPpsSent_ = false;
    hasPrevIFrame_ = false;
    hasStreamStartTs_ = false;
    streamStartTs_ = 0;
}

uint32_t RtmpSender::toRtmpTs(uint32_t ts)
{
    if (!hasStreamStartTs_) {
        streamStartTs_ = ts;
        hasStreamStartTs_ = true;
    }
    return ts - streamStartTs_;
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
    resetStreamState();
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

void RtmpSender::handleNalUnit(const uint8_t* nal, int nalSize,
                               std::vector<uint8_t>& avccFrame, bool& hasIdr)
{
    if (!nal || nalSize <= 0)
        return;

    const int nalType = nal[0] & 0x1F;
    if (nalType == 7) {
        sps.assign(nal, nal + nalSize);
        return;
    }
    if (nalType == 8) {
        pps.assign(nal, nal + nalSize);
        return;
    }
    if (nalType == 9 || nalType == 12)
        return;

    if (nalType == 5)
        hasIdr = true;

    if (nalType == 1 || nalType == 5 || nalType == 6)
        appendAvccNal(avccFrame, nal, nalSize);
}

bool RtmpSender::parseAnnexBFrame(const uint8_t* data, int size,
                                  std::vector<uint8_t>& avccFrame, bool& hasIdr)
{
    int offset = 0;
    bool foundNal = false;

    while (offset < size) {
        const int startCodeLen = startCodeLengthAt(data, size, offset);
        if (startCodeLen == 0)
            break;

        const int nalStart = offset + startCodeLen;
        const int nalEnd = findNextStartCode(data, size, nalStart);
        const int nalSize = nalEnd - nalStart;
        if (nalSize <= 0)
            break;

        handleNalUnit(data + nalStart, nalSize, avccFrame, hasIdr);
        foundNal = true;
        offset = nalEnd;
    }

    return foundNal;
}

bool RtmpSender::parseAvccFrame(const uint8_t* data, int size,
                                std::vector<uint8_t>& avccFrame, bool& hasIdr)
{
    int offset = 0;
    bool foundNal = false;

    while (offset + 4 <= size) {
        const uint32_t nalSize =
            (static_cast<uint32_t>(data[offset]) << 24) |
            (static_cast<uint32_t>(data[offset + 1]) << 16) |
            (static_cast<uint32_t>(data[offset + 2]) << 8) |
            static_cast<uint32_t>(data[offset + 3]);
        offset += 4;

        if (nalSize == 0 || offset + static_cast<int>(nalSize) > size)
            break;

        handleNalUnit(data + offset, static_cast<int>(nalSize), avccFrame, hasIdr);
        foundNal = true;
        offset += static_cast<int>(nalSize);
    }

    return foundNal;
}

bool RtmpSender::SendH264Frame(const uint8_t* data, int size, uint32_t ts, bool isKeyFrame)
{
    if (!data || size <= 0 || !IsConnected())
        return false;

    if (isKeyFrame) {
        spsPpsSent_ = false;
        hasPrevIFrame_ = true;
    } else if (!hasPrevIFrame_) {
        return true;
    }

    std::vector<uint8_t> avccFrame;
    avccFrame.reserve(size + 16);
    bool hasIdr = false;
    bool parsed = false;

    if (hasAnnexBStartCode(data, size))
        parsed = parseAnnexBFrame(data, size, avccFrame, hasIdr);
    else
        parsed = parseAvccFrame(data, size, avccFrame, hasIdr);

    if (!parsed)
        return true;

    const uint32_t rtmpTs = toRtmpTs(ts);
    if (isKeyFrame && !spsPpsSent_ && !sps.empty() && !pps.empty()) {
        SendVideoSpsPps(rtmpTs);
        spsPpsSent_ = true;
    }

    if (avccFrame.empty())
        return true;

    return SendVideoFrame(avccFrame.data(), static_cast<int>(avccFrame.size()),
                          rtmpTs, isKeyFrame || hasIdr);
}

bool RtmpSender::SendVideoFrame(const uint8_t* data, int size, uint32_t ts, bool isKeyFrame)
{
    if (!data || size <= 0 || !IsConnected())
        return false;

    if (!AllocPacket(size + 5))
        return false;

    char* body = packet->m_body;
    int i = 0;

    body[i++] = isKeyFrame ? 0x17 : 0x27;
    body[i++] = 0x01;
    body[i++] = body[i++] = body[i++] = 0x00;

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

    body[i++] = 0x17;
    body[i++] = 0x00;
    body[i++] = body[i++] = body[i++] = 0x00;

    body[i++] = 0x01;
    body[i++] = sps[1];
    body[i++] = sps[2];
    body[i++] = sps[3];
    body[i++] = 0xFF;

    body[i++] = 0xE1;
    body[i++] = (sps.size() >> 8) & 0xFF;
    body[i++] = sps.size() & 0xFF;
    memcpy(&body[i], sps.data(), sps.size());
    i += sps.size();

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
