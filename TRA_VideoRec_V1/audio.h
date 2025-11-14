//
//  audio.h
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
// audio.h
#ifndef AUDIO_H
#define AUDIO_H

#include <libavcodec/avcodec.h>

// 哪一路有声音：-1 = 静音；0..3 = 对应 g_streams[index]
extern int g_audio_source;

// 初始化 SDL 音频，返回 0 成功，<0 失败
int audio_init(void);

// 播放一帧音频（会根据 g_audio_source 决定要不要真的输出）
void audio_play_frame(int stream_id, AVFrame *frame, AVCodecContext *dec_ctx);

// 关闭音频
void audio_close(void);

#endif
