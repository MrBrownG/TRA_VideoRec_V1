//
//  mac_cam_reader.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
// mac_cam_reader.c
// mac_cam_reader.c
#include <pthread.h>

#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "video_receiver.h"
#include "common.h"

// 采集 Mac 摄像头，写入 g_streams[1]（右上角）
static void *mac_cam_thread(void *arg)
{
    (void)arg;

    // 注册设备 & 网络（多次调用也没关系）
    avdevice_register_all();
    avformat_network_init();

    AVFormatContext *fmt = NULL;

    // avfoundation 输入
    AVInputFormat *ifmt = av_find_input_format("avfoundation");
    if (!ifmt) {
        printf("[MAC] avfoundation input not found\n");
        return NULL;
    }

    // "0" 一般是内建摄像头；如不确定可以先用 ffmpeg -f avfoundation -list_devices true -i ""
    const char *device_name = "0";

    // 设置分辨率 & 帧率
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "video_size", "640x480", 0);
    av_dict_set(&opts, "framerate",  "30",       0);

    if (avformat_open_input(&fmt, device_name, ifmt, &opts) < 0) {
        printf("[MAC] Cannot open Mac camera: %s\n", device_name);
        av_dict_free(&opts);
        return NULL;
    }
    av_dict_free(&opts);

    if (avformat_find_stream_info(fmt, NULL) < 0) {
        printf("[MAC] find_stream_info failed\n");
        return NULL;
    }

    int video_stream = -1;
    for (int i = 0; i < (int)fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    if (video_stream < 0) {
        printf("[MAC] No video stream in Mac camera\n");
        return NULL;
    }

    const AVCodec *dec = avcodec_find_decoder(
        fmt->streams[video_stream]->codecpar->codec_id);
    if (!dec) {
        printf("[MAC] No decoder for Mac camera\n");
        return NULL;
    }

    AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(dec_ctx,
                                  fmt->streams[video_stream]->codecpar);

    if (avcodec_open2(dec_ctx, dec, NULL) < 0) {
        printf("[MAC] open decoder failed\n");
        return NULL;
    }

    AVFrame *yuv  = av_frame_alloc();
    AVPacket pkt;

    // 写到 g_streams[1] → 右上角
    VideoStream *vs = &g_streams[1];

    printf("[MAC] Camera capture started.\n");

    while (1) {
        if (av_read_frame(fmt, &pkt) < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        if (pkt.stream_index != video_stream) {
            av_packet_unref(&pkt);
            continue;
        }

        if (avcodec_send_packet(dec_ctx, &pkt) < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        while (avcodec_receive_frame(dec_ctx, yuv) == 0) {

            // 第一次初始化 sws_ctx 和 rgb_frame
            if (!vs->sws_ctx) {
                vs->sws_ctx = sws_getContext(
                    yuv->width, yuv->height, dec_ctx->pix_fmt,
                    vs->width, vs->height,
                    AV_PIX_FMT_RGB24,
                    SWS_BILINEAR, NULL, NULL, NULL
                );
                if (!vs->sws_ctx) {
                    printf("[MAC] sws_getContext failed\n");
                    break;
                }

                if (!vs->rgb_frame) {
                    vs->rgb_frame = av_frame_alloc();
                }
                vs->rgb_frame->format = AV_PIX_FMT_RGB24;
                vs->rgb_frame->width  = vs->width;
                vs->rgb_frame->height = vs->height;
                if (av_frame_get_buffer(vs->rgb_frame, 32) < 0) {
                    printf("[MAC] av_frame_get_buffer failed\n");
                    break;
                }

                printf("[MAC] sws_ctx created src=%dx%d dst=%dx%d\n",
                       yuv->width, yuv->height, vs->width, vs->height);
            }

            sws_scale(
                vs->sws_ctx,
                (const uint8_t * const *)yuv->data,
                yuv->linesize,
                0,
                yuv->height,
                vs->rgb_frame->data,
                vs->rgb_frame->linesize
            );

            vs->updated = 1;  // 告诉 SDL 有新帧
        }

        av_packet_unref(&pkt);
    }

    return NULL;
}

void start_mac_camera_reader(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, mac_cam_thread, NULL);
    pthread_detach(tid);
}
