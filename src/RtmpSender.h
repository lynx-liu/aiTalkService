#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" {
#include <librtmp/rtmp.h>
}

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

class RtmpSender {
public:
    RtmpSender();
    ~RtmpSender();

    // 初始化 RTMP 连接
    bool Init(const std::string& url);

    // 关闭连接并释放资源
    void Close();

    // 发送一帧 H.264 数据
    bool SendH264Frame(const uint8_t* data, int size, uint32_t ts, bool isKeyFrame);

    bool IsConnected() const { return rtmp && RTMP_IsConnected(rtmp); }

private:
    RTMP* rtmp;
    RTMPPacket* packet;
    int packetSize;

    std::vector<uint8_t> sps;
    std::vector<uint8_t> pps;

    bool AllocPacket(int bodySize);
    bool SendVideoSpsPps(uint32_t ts);
    bool SendVideoFrame(const uint8_t* data, int size, uint32_t ts, bool isKeyFrame);
};
