// audio.c
#include "audio.h"

#include <SDL2/SDL.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>

#include <stdio.h>

int g_audio_source = 0;

static SDL_AudioDeviceID g_audio_dev = 0;
static SwrContext *g_swr = NULL;

// 目标输出格式
static const int g_out_rate   = 48000;
static const enum AVSampleFormat g_out_fmt = AV_SAMPLE_FMT_S16;
static uint64_t g_out_layout = AV_CHANNEL_LAYOUT_STEREO;  // 新 API 通用 Layout

int audio_init(void)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("SDL audio init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);

    want.freq = g_out_rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;    //采样率小，减少延迟
    want.callback = NULL;

    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!g_audio_dev) {
        printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_PauseAudioDevice(g_audio_dev, 0);

    printf("[AUDIO] init OK: %d Hz, %d ch\n", have.freq, have.channels);
    return 0;
}

// 初始化重采样器（新版 FFmpeg API）
// 初始化重采样器（只做一次）——新版，使用 swr_alloc_set_opts2
static int init_swr_if_needed(AVCodecContext *dec_ctx)
{
    if (g_swr) return 0;

    AVChannelLayout src_layout = dec_ctx->ch_layout;
    if (!src_layout.nb_channels) {
        // 有些流没有设置 ch_layout，这里简单兜底为双声道
        av_channel_layout_default(&src_layout, 2);
        printf("[AUDIO] src ch_layout empty, force stereo\n");
    }

    AVChannelLayout dst_layout;
    av_channel_layout_default(&dst_layout, 2);  // 输出立体声

    // 用 swr_alloc_set_opts2 一次性配置所有参数
    if (swr_alloc_set_opts2(
            &g_swr,
            &dst_layout,         // 输出布局
            g_out_fmt,           // 输出采样格式 S16
            g_out_rate,          // 输出采样率 48000
            &src_layout,         // 输入布局
            dec_ctx->sample_fmt, // 输入采样格式（来自解码器）
            dec_ctx->sample_rate,// 输入采样率
            0,
            NULL) < 0) {

        printf("[AUDIO] swr_alloc_set_opts2 failed\n");
        g_swr = NULL;
        return -1;
    }

    if (swr_init(g_swr) < 0) {
        printf("[AUDIO] swr_init failed\n");
        swr_free(&g_swr);
        return -1;
    }

    printf("[AUDIO] swr_init OK: in_rate=%d out_rate=%d, in_fmt=%d\n",
           dec_ctx->sample_rate, g_out_rate, dec_ctx->sample_fmt);
    return 0;
}
void audio_play_frame(int stream_id, AVFrame *frame, AVCodecContext *dec_ctx)
{
    if (stream_id != g_audio_source) return;
    if (!frame || !dec_ctx) return;
    if (!g_audio_dev) return;

    if (init_swr_if_needed(dec_ctx) < 0) return;

    int src_nb = frame->nb_samples;
    int dst_nb = av_rescale_rnd(
        swr_get_delay(g_swr, dec_ctx->sample_rate) + src_nb,
        g_out_rate, dec_ctx->sample_rate, AV_ROUND_UP
    );

    uint8_t *out_buf = NULL;
    int out_linesize = 0;

    av_samples_alloc(
        &out_buf, &out_linesize,
        2,              // out channels = 2
        dst_nb,
        g_out_fmt,
        0
    );

    int ret = swr_convert(
        g_swr,
        &out_buf, dst_nb,
        (const uint8_t **)frame->data,
        src_nb
    );

    if (ret > 0) {
        int out_size = av_samples_get_buffer_size(
            NULL, 2, ret, g_out_fmt, 1
        );
        SDL_QueueAudio(g_audio_dev, out_buf, out_size);
    }

    av_freep(&out_buf);
}

void audio_close(void)
{
    if (g_swr) swr_free(&g_swr);
    if (g_audio_dev) SDL_CloseAudioDevice(g_audio_dev);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
