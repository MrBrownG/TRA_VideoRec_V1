#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <SDL2/SDL.h>

#define NUM_STREAMS   4
#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 1200
#define MAX_PACKET_SIZE 65536
#define MAGIC_NUMBER  0x56445230  // "VDR0"
#define SUB_WIDTH  (WINDOW_WIDTH/2)
#define SUB_HEIGHT (WINDOW_HEIGHT/2)
#define MAX_CHUNK_SIZE 1300
#define MAX_FRAME_SIZE (1024 * 1024)   // 1MB

// 视频格式枚举
typedef enum {
    FORMAT_H264   = 0,
    FORMAT_H265   = 1,
    FORMAT_MJPEG  = 2,
    FORMAT_RAW_RGB= 3,
    FORMAT_UNKNOWN= 255
} VideoFormat;

// 旧的带头部结构（现在可以保留，以后想用 RAW_RGB 还可以复用）
typedef struct {
    uint32_t magic;
    uint32_t frame_number;
    uint32_t stream_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t data_size;
    uint64_t timestamp;
    uint32_t flags;
} VideoFrameHeader;

// 🔥 H.264 分片用的简单头（发送端 & 接收端共用）
typedef struct {
    uint32_t magic;        // MAGIC_NUMBER
    uint32_t frame_number; // 帧号
    uint32_t stream_id;    // 0~3
    uint32_t format;       // FORMAT_H264
    uint32_t nalu_size;    // 该帧 H.264 NALU 的总长度（所有片加起来）
    uint32_t chunk_offset;   // 本分片在整个 NALU 中的偏移
    uint32_t chunk_size;     // 本分片的 payload 大小
} H264Header;

#endif
