// rtsp_reader.c
#include <pthread.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "video_receiver.h"
#include "common.h"
#include "audio.h"

// 单独的 RTSP 采集线程，把画面写到 g_streams[0]，声音送到 audio.c
static void *rtsp_thread(void *url_ptr)
{
    const char *url = (const char *)url_ptr;

    avformat_network_init();

    AVFormatContext *fmt = NULL;
    if (avformat_open_input(&fmt, url, NULL, NULL) < 0) {
        printf("[RTSP] open failed: %s\n", url);
        return NULL;
    }
    avformat_find_stream_info(fmt, NULL);

    int video_stream = -1;
    int audio_stream = -1;

    for (int i = 0; i < (int)fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream = i;
        else if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audio_stream = i;
    }

    if (video_stream < 0) {
        printf("[RTSP] no video stream\n");
        return NULL;
    }

    // --- 视频解码器 ---
    const AVCodec *vdec = avcodec_find_decoder(
        fmt->streams[video_stream]->codecpar->codec_id);
    AVCodecContext *vdec_ctx = avcodec_alloc_context3(vdec);
    avcodec_parameters_to_context(vdec_ctx,
                                  fmt->streams[video_stream]->codecpar);
    avcodec_open2(vdec_ctx, vdec, NULL);

    // --- 音频解码器（如果有） ---
    AVCodecContext *adec_ctx = NULL;
    AVFrame *aframe = NULL;
    if (audio_stream >= 0) {
        const AVCodec *adec = avcodec_find_decoder(
            fmt->streams[audio_stream]->codecpar->codec_id);
        if (adec) {
            adec_ctx = avcodec_alloc_context3(adec);
            avcodec_parameters_to_context(adec_ctx,
                                          fmt->streams[audio_stream]->codecpar);
            if (avcodec_open2(adec_ctx, adec, NULL) < 0) {
                printf("[RTSP] open audio decoder failed\n");
                avcodec_free_context(&adec_ctx);
                adec_ctx = NULL;
            } else {
                aframe = av_frame_alloc();
                printf("[RTSP] audio stream opened (index=%d)\n", audio_stream);
            }
        }
    }

    AVFrame *yuv = av_frame_alloc();
    AVPacket pkt;

    VideoStream *vs = &g_streams[0];  // 左上角

    printf("[RTSP] reader started, video=%d audio=%d\n",
           video_stream, audio_stream);

    while (1) {
        if (av_read_frame(fmt, &pkt) < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        if (pkt.stream_index == video_stream) {
            // -------- 视频处理 --------
            if (avcodec_send_packet(vdec_ctx, &pkt) >= 0) {
                while (avcodec_receive_frame(vdec_ctx, yuv) == 0) {

                    if (!vs->sws_ctx) {
                        vs->sws_ctx = sws_getContext(
                            yuv->width, yuv->height, vdec_ctx->pix_fmt,
                            vs->width, vs->height,
                            AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, NULL, NULL, NULL
                        );
                        if (!vs->rgb_frame) {
                            vs->rgb_frame = av_frame_alloc();
                        }
                        vs->rgb_frame->format = AV_PIX_FMT_RGB24;
                        vs->rgb_frame->width  = vs->width;
                        vs->rgb_frame->height = vs->height;
                        av_frame_get_buffer(vs->rgb_frame, 32);

                        printf("[RTSP] sws_ctx created src=%dx%d dst=%dx%d\n",
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

                    vs->updated = 1;
                }
            }
        } else if (pkt.stream_index == audio_stream && adec_ctx && aframe) {
            // -------- 音频处理 --------
            if (avcodec_send_packet(adec_ctx, &pkt) >= 0) {
                while (avcodec_receive_frame(adec_ctx, aframe) == 0) {
                    // 把这一帧音频交给 audio.c 处理
                    audio_play_frame(0, aframe, adec_ctx);  // 0 = stream0
                }
            }
        }

        av_packet_unref(&pkt);
    }

    return NULL;
}

void start_rtsp_reader(const char *url)
{
    pthread_t tid;
    pthread_create(&tid, NULL, rtsp_thread, (void *)url);
    pthread_detach(tid);
}
