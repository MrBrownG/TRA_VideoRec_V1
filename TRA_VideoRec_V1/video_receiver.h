#ifndef VIDEO_RECEIVER_H
#define VIDEO_RECEIVER_H

#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "common.h"

typedef struct {
    uint8_t *nalu_buffer;

    int expected_size;
    int received_size;

    uint8_t received_map[2048];   // 标记分片到齐

    // FFmpeg 解码
    AVCodecContext *codec_ctx;
    AVCodecParserContext *parser;
    AVFrame *yuv_frame;
    AVFrame *rgb_frame;
    struct SwsContext *sws_ctx;

    // SDL 显示
    uint8_t *rgb_buffer;
    int width, height;

    int updated;
} VideoStream;

extern VideoStream g_streams[NUM_STREAMS];

void init_video_streams();
void push_h264_chunk(int sid, uint8_t *data, int size);
void decode_h264_stream(int sid);
void free_video_streams();

#endif
