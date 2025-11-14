#include "video_receiver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局视频流
VideoStream g_streams[NUM_STREAMS];

// =============================
// 初始化 FFmpeg 解码器
// =============================
static void init_decoder(VideoStream *vs)
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        printf("H.264 codec not found!\n");
        return;
    }

    vs->parser    = av_parser_init(AV_CODEC_ID_H264);
    vs->codec_ctx = avcodec_alloc_context3(codec);

    if (avcodec_open2(vs->codec_ctx, codec, NULL) < 0) {
        printf("Failed to open H.264 decoder!\n");
        return;
    }

    vs->yuv_frame = av_frame_alloc();
    vs->rgb_frame = av_frame_alloc();

    // SDL 小窗口尺寸
    vs->width  = SUB_WIDTH;
    vs->height = SUB_HEIGHT;

    int rgb_buf_size = vs->width * vs->height * 3;
    vs->rgb_buffer = (uint8_t *)malloc(rgb_buf_size);

    vs->sws_ctx = NULL;   // 第一次解码出一帧再创建
    vs->updated = 0;
}

// =============================
// 初始化全部视频流
// =============================
void init_video_streams()
{
    for (int i = 0; i < NUM_STREAMS; i++) {
        VideoStream *vs = &g_streams[i];

        vs->nalu_buffer   = malloc(MAX_FRAME_SIZE);
        vs->expected_size = 0;
        vs->received_size = 0;

        // 用新的分片标记数组初始化
        memset(vs->received_map, 0, sizeof(vs->received_map));

        vs->updated = 0;

        init_decoder(vs);
    }

    printf("Video streams initialized.\n");
}

// =============================
// 收到一个分片
// =============================
void push_h264_chunk(int sid, uint8_t *data, int size)
{
    if (sid < 0 || sid >= NUM_STREAMS) return;

    VideoStream *vs = &g_streams[sid];

    if (size < sizeof(H264Header)) return;

    H264Header *h = (H264Header*)data;

    if (h->magic != MAGIC_NUMBER) return;

    uint8_t *payload = data + sizeof(H264Header);
    int len = h->chunk_size;

    if (h->chunk_offset + len > MAX_FRAME_SIZE) return;

    // 第一次
    if (vs->expected_size == 0) {
        vs->expected_size = h->nalu_size;
        vs->received_size = 0;
        memset(vs->received_map, 0, sizeof(vs->received_map));
    }

    // 写入正确偏移
    memcpy(vs->nalu_buffer + h->chunk_offset, payload, len);

    // 标记该分片收到
    int index = h->chunk_offset / MAX_CHUNK_SIZE;
    vs->received_map[index] = 1;

    vs->received_size += len;

    // 检查是否收齐
    if (vs->received_size >= vs->expected_size) {

        int count = (vs->expected_size + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;
        int ok = 1;
        for (int i = 0; i < count; i++)
            if (!vs->received_map[i])
                ok = 0;

        if (ok) decode_h264_stream(sid);

        vs->expected_size = 0;
        vs->received_size = 0;
    }
}
// =============================
// 解码一个完整 NALU
// =============================
void decode_h264_stream(int sid)
{
    VideoStream *vs = &g_streams[sid];

    uint8_t *data = vs->nalu_buffer;
    int data_size = vs->expected_size;

    if (data_size <= 0) return;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = data_size;

    int ret = avcodec_send_packet(vs->codec_ctx, &pkt);
    if (ret < 0) {
        printf("Stream %d: send packet error %d\n", sid, ret);
        return;
    }

    while ((ret = avcodec_receive_frame(vs->codec_ctx, vs->yuv_frame)) == 0) {

        if (vs->sws_ctx == NULL) {
            int src_w  = vs->yuv_frame->width;
            int src_h  = vs->yuv_frame->height;
            enum AVPixelFormat src_fmt = vs->codec_ctx->pix_fmt;

            vs->rgb_frame->format = AV_PIX_FMT_RGB24;
            vs->rgb_frame->width  = vs->width;
            vs->rgb_frame->height = vs->height;

            av_frame_get_buffer(vs->rgb_frame, 32);

            vs->sws_ctx = sws_getContext(
                src_w, src_h, src_fmt,
                vs->width, vs->height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, NULL, NULL, NULL
            );

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

//        memcpy(vs->rgb_buffer,
//               vs->rgb_frame->data[0],
//               vs->width * vs->height * 3);

        vs->updated = 1;
    }
}

// =============================
// 释放
// =============================
void free_video_streams()
{
    for (int i = 0; i < NUM_STREAMS; i++) {
        VideoStream *vs = &g_streams[i];

        if (vs->nalu_buffer) free(vs->nalu_buffer);
        if (vs->rgb_buffer) free(vs->rgb_buffer);

        if (vs->parser)     av_parser_close(vs->parser);
        if (vs->codec_ctx)  avcodec_free_context(&vs->codec_ctx);

        if (vs->yuv_frame)  av_frame_free(&vs->yuv_frame);
        if (vs->rgb_frame)  av_frame_free(&vs->rgb_frame);

        if (vs->sws_ctx)    sws_freeContext(vs->sws_ctx);
    }

    printf("Video streams destroyed.\n");
}
