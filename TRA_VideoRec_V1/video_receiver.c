// video_receiver.c
#include "video_receiver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局 4 路视频流
VideoStream g_streams[NUM_STREAMS];

// -----------------------------
// 初始化单路解码器
// -----------------------------
static void init_decoder(VideoStream *vs)
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        printf("H.264 codec not found!\n");
        return;
    }

    vs->parser    = av_parser_init(AV_CODEC_ID_H264);
    vs->codec_ctx = avcodec_alloc_context3(codec);
    if (!vs->codec_ctx) {
        printf("alloc codec_ctx failed\n");
        return;
    }

    if (avcodec_open2(vs->codec_ctx, codec, NULL) < 0) {
        printf("Failed to open H.264 decoder!\n");
        return;
    }

    vs->yuv_frame = av_frame_alloc();
    vs->rgb_frame = av_frame_alloc();

    vs->width  = SUB_WIDTH;
    vs->height = SUB_HEIGHT;

    vs->sws_ctx = NULL;
    vs->updated = 0;
}

// -----------------------------
// 初始化全部视频流
// -----------------------------
void init_video_streams(void)
{
    for (int i = 0; i < NUM_STREAMS; ++i) {
        VideoStream *vs = &g_streams[i];

        vs->nalu_buffer   = (uint8_t *)malloc(MAX_FRAME_SIZE);
        vs->expected_size = 0;
        vs->received_size = 0;
        memset(vs->received_map, 0, sizeof(vs->received_map));

        vs->parser    = NULL;
        vs->codec_ctx = NULL;
        vs->yuv_frame = NULL;
        vs->rgb_frame = NULL;
        vs->sws_ctx   = NULL;
        vs->updated   = 0;

        init_decoder(vs);
    }

    printf("Video streams initialized.\n");
}

// -----------------------------
// 收到一个 UDP 分片
// -----------------------------
void push_h264_chunk(int sid, uint8_t *data, int size)
{
    if (sid < 0 || sid >= NUM_STREAMS) return;

    VideoStream *vs = &g_streams[sid];

    if (size < (int)sizeof(H264Header)) return;

    H264Header *h = (H264Header *)data;
    if (h->magic != MAGIC_NUMBER)   return;
    if (h->format != FORMAT_H264)   return;

    uint8_t *payload = data + sizeof(H264Header);
    int      len     = h->chunk_size;

    if (h->chunk_offset + len > MAX_FRAME_SIZE) {
        printf("Stream %d: chunk too big, drop\n", sid);
        return;
    }

    // 第一次收到这帧
    if (vs->expected_size == 0) {
        vs->expected_size = h->nalu_size;
        vs->received_size = 0;
        memset(vs->received_map, 0, sizeof(vs->received_map));
    }

    // 按 offset 写入到对应位置（关键）
    memcpy(vs->nalu_buffer + h->chunk_offset, payload, len);

    // 标记分片已收到
    int index = h->chunk_offset / MAX_CHUNK_SIZE;
    if (index >= 0 && index < MAX_CHUNKS) {
        vs->received_map[index] = 1;
    }

    vs->received_size += len;

    // 判断是否收齐
    if (vs->received_size >= vs->expected_size) {
        int total_chunks = (vs->expected_size + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;
        int ok = 1;
        for (int i = 0; i < total_chunks; ++i) {
            if (!vs->received_map[i]) {
                ok = 0;
                break;
            }
        }

        if (ok) {
            decode_h264_stream(sid);
        } else {
            printf("Stream %d: missing chunks, drop frame\n", sid);
        }

        vs->expected_size = 0;
        vs->received_size = 0;
        memset(vs->received_map, 0, sizeof(vs->received_map));
    }
}

// -----------------------------
// 解码完整一帧 NALU → RGB
// -----------------------------
void decode_h264_stream(int sid)
{
    VideoStream *vs = &g_streams[sid];

    if (vs->expected_size <= 0) return;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = vs->nalu_buffer;
    pkt.size = vs->expected_size;

    int ret = avcodec_send_packet(vs->codec_ctx, &pkt);
    if (ret < 0) {
        printf("Stream %d: avcodec_send_packet err=%d\n", sid, ret);
        return;
    }

    while ((ret = avcodec_receive_frame(vs->codec_ctx, vs->yuv_frame)) == 0) {

        if (!vs->sws_ctx) {
            int src_w  = vs->yuv_frame->width;
            int src_h  = vs->yuv_frame->height;
            enum AVPixelFormat src_fmt = vs->codec_ctx->pix_fmt;

            vs->rgb_frame->format = AV_PIX_FMT_RGB24;
            vs->rgb_frame->width  = vs->width;
            vs->rgb_frame->height = vs->height;

            if (av_frame_get_buffer(vs->rgb_frame, 32) < 0) {
                printf("Stream %d: av_frame_get_buffer failed\n", sid);
                return;
            }

            vs->sws_ctx = sws_getContext(
                src_w, src_h, src_fmt,
                vs->width, vs->height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, NULL, NULL, NULL
            );

            if (!vs->sws_ctx) {
                printf("Stream %d: sws_getContext failed\n", sid);
                return;
            }

            printf("Stream %d: sws_ctx created src=%dx%d dst=%dx%d\n",
                   sid, src_w, src_h, vs->width, vs->height);
        }

        int src_h = vs->yuv_frame->height;

        sws_scale(vs->sws_ctx,
                  (const uint8_t * const *)vs->yuv_frame->data,
                  vs->yuv_frame->linesize,
                  0,
                  src_h,
                  vs->rgb_frame->data,
                  vs->rgb_frame->linesize);

        vs->updated = 1;   // 提示 SDL 有新帧
    }
}

// -----------------------------
// 释放资源
// -----------------------------
void free_video_streams(void)
{
    for (int i = 0; i < NUM_STREAMS; ++i) {
        VideoStream *vs = &g_streams[i];

        if (vs->nalu_buffer) free(vs->nalu_buffer);

        if (vs->parser)     av_parser_close(vs->parser);
        if (vs->codec_ctx)  avcodec_free_context(&vs->codec_ctx);

        if (vs->yuv_frame)  av_frame_free(&vs->yuv_frame);
        if (vs->rgb_frame)  av_frame_free(&vs->rgb_frame);

        if (vs->sws_ctx)    sws_freeContext(vs->sws_ctx);
    }

    printf("Video streams destroyed.\n");
}
