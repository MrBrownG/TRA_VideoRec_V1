// video_receiver.h
#ifndef VIDEO_RECEIVER_H
#define VIDEO_RECEIVER_H

#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "common.h"

#define MAX_CHUNKS 2048   // 分片数量上限（1MB / 1300 ≈ 800，多给点余量）

// 单路视频流状态
typedef struct {
    // 分片重组缓冲
    uint8_t *nalu_buffer;
    int      expected_size;          // 这一帧完整 NALU 大小
    int      received_size;          // 已收到的字节数
    uint8_t  received_map[MAX_CHUNKS]; // 标记每个分片是否收到

    // FFmpeg 解码
    AVCodecContext       *codec_ctx;
    AVCodecParserContext *parser;
    AVFrame              *yuv_frame;
    AVFrame              *rgb_frame;
    struct SwsContext    *sws_ctx;

    int width;
    int height;

    int updated;                     // 是否有新 RGB 帧可画
} VideoStream;

// 全局 4 路流
extern VideoStream g_streams[NUM_STREAMS];

void init_video_streams(void);
void push_h264_chunk(int sid, uint8_t *data, int size);
void decode_h264_stream(int sid);
void free_video_streams(void);

#endif
