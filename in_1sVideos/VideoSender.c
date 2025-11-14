#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "common.h"

#define MAX_UDP_PACKET 1500
#define MAX_H264_NALU  (1024*1024)

typedef struct {
    int stream_id;
    const char *filename;
} SenderArgs;


// ======================================================
// 分片发送函数
// ======================================================
static void send_nalu_chunks(
    int sock, struct sockaddr_in *addr,
    uint8_t *frame_buffer, int nalu_size,
    uint32_t frame_number, int stream_id)
{
    H264Header h;
    h.magic        = MAGIC_NUMBER;
    h.frame_number = frame_number;
    h.stream_id    = stream_id;
    h.format       = FORMAT_H264;
    h.nalu_size    = nalu_size;

    uint8_t sendbuf[MAX_UDP_PACKET];

    int remain = nalu_size;
    int offset = 0;

    while (remain > 0) {
        int chunk = (remain > 1300 ? 1300 : remain);

        h.chunk_offset = offset;
        h.chunk_size   = chunk;

        memcpy(sendbuf, &h, sizeof(H264Header));
        memcpy(sendbuf + sizeof(H264Header),
               frame_buffer + offset, chunk);

        sendto(sock, sendbuf,
               sizeof(H264Header) + chunk,
               0,
               (struct sockaddr *)addr, sizeof(*addr));

        remain -= chunk;
        offset += chunk;

        usleep(1000);  // 稍微等待降低丢包概率
    }
}



// ======================================================
// 每路视频的发送线程
// ======================================================
static void *send_thread(void *arg)
{
    SenderArgs *sa = (SenderArgs *)arg;
    int id = sa->stream_id;
    const char *filename = sa->filename;
    int port = 5000 + id;

    printf("Stream %d: open %s\n", id, filename);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // ----------------------------
    // FFmpeg 打开输入视频
    // ----------------------------
    AVFormatContext *fmt = NULL;
    if (avformat_open_input(&fmt, filename, NULL, NULL) < 0) {
        printf("Stream %d: cannot open input file\n", id);
        return NULL;
    }
    avformat_find_stream_info(fmt, NULL);

    int video_stream = -1;
    for (int i = 0; i < fmt->nb_streams; i++)
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream = i;

    if (video_stream < 0) {
        printf("Stream %d: no video stream found\n", id);
        return NULL;
    }

    AVCodec *dec = avcodec_find_decoder(
        fmt->streams[video_stream]->codecpar->codec_id);
    AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);

    avcodec_parameters_to_context(dec_ctx,
                                  fmt->streams[video_stream]->codecpar);
    avcodec_open2(dec_ctx, dec, NULL);


    // ----------------------------
    // H.264 编码器
    // ----------------------------
    AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc);

    enc_ctx->width     = dec_ctx->width;
    enc_ctx->height    = dec_ctx->height;
    enc_ctx->pix_fmt   = AV_PIX_FMT_YUV420P;
    enc_ctx->time_base = (AVRational){1, 30};
    enc_ctx->framerate = (AVRational){30, 1};
    enc_ctx->gop_size  = 30;
    enc_ctx->max_b_frames = 0;

    avcodec_open2(enc_ctx, enc, NULL);

    AVFrame  *frame  = av_frame_alloc();
    AVPacket *pkt    = av_packet_alloc();
    AVPacket *input  = av_packet_alloc();

    uint8_t *frame_buffer = (uint8_t *)malloc(MAX_H264_NALU);
    uint32_t frame_count = 0;


    // ----------------------------
    // 主循环：解码 → 编码 → 分片发送
    // ----------------------------
    while (av_read_frame(fmt, input) >= 0)
    {
        if (input->stream_index != video_stream) {
            av_packet_unref(input);
            continue;
        }

        avcodec_send_packet(dec_ctx, input);

        while (avcodec_receive_frame(dec_ctx, frame) == 0)
        {
            avcodec_send_frame(enc_ctx, frame);

            while (avcodec_receive_packet(enc_ctx, pkt) == 0)
            {
                if (pkt->size > MAX_H264_NALU) continue;

                memcpy(frame_buffer, pkt->data, pkt->size);

                send_nalu_chunks(sock, &addr,
                                 frame_buffer, pkt->size,
                                 frame_count, id);

                printf("Stream %d: sent frame %u (%d bytes)\n",
                       id, frame_count, pkt->size);

                frame_count++;
                av_packet_unref(pkt);
            }
        }

        av_packet_unref(input);
    }

    return NULL;
}



// ======================================================
// main 入口
// ======================================================
int main(int argc, char *argv[])
{
    if (argc != 5) {
        printf("Usage: %s cam0.mp4 cam1.mp4 cam2.mp4 cam3.mp4\n", argv[0]);
        return 0;
    }

    avformat_network_init();

    pthread_t th[4];
    SenderArgs args[4];

    for (int i = 0; i < 4; i++) {
        args[i].stream_id = i;
        args[i].filename  = argv[i+1];
        pthread_create(&th[i], NULL, send_thread, &args[i]);
    }

    for (int i = 0; i < 4; i++)
        pthread_join(th[i], NULL);

    return 0;
}
