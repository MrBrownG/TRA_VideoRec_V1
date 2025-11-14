// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define NUM_STREAMS    4

// 显示窗口大小（总窗）
#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  600

// 子窗口大小（2x2）
#define SUB_WIDTH      (WINDOW_WIDTH  / 2)
#define SUB_HEIGHT     (WINDOW_HEIGHT / 2)

// 魔数
#define MAGIC_NUMBER   0x56445230      // "VDR0"

// 一帧 H.264 NALU 的最大长度（1MB 足够做测试）
#define MAX_FRAME_SIZE (1024 * 1024)

// 每个 UDP 包里 payload 的最大长度（和 sender 保持一致）
#define MAX_CHUNK_SIZE 1300

// 视频格式（目前只用 H264）
typedef enum {
    FORMAT_H264 = 0,
    FORMAT_UNKNOWN = 255
} VideoFormat;

// ------------------------------
// H.264 分片 UDP 头
// ------------------------------
typedef struct {
    uint32_t magic;        // MAGIC_NUMBER
    uint32_t frame_number; // 帧号
    uint32_t stream_id;    // 0~3

    uint32_t format;       // FORMAT_H264
    uint32_t nalu_size;    // 该帧完整 NALU 的总长度

    uint32_t chunk_offset; // 本分片在该 NALU 里的偏移（0, 1300, 2600, ...）
    uint32_t chunk_size;   // 本分片 payload 的长度
} H264Header;

#endif

//// old struct for  RAW_RGB
//typedef struct {
//    uint32_t magic;
//    uint32_t frame_number;
//    uint32_t stream_id;
//    uint32_t format;
//    uint32_t width;
//    uint32_t height;
//    uint32_t data_size;
//    uint64_t timestamp;
//    uint32_t flags;
//} VideoFrameHeader;

