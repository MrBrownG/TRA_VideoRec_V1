//
//  video_receiver.h
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#ifndef VIDEO_RECEIVER_H
#define VIDEO_RECEIVER_H

#include <SDL2/SDL.h>
#include "common.h"

// 视频流管理结构
typedef struct {
    int stream_id;
    int port;
    Uint32* pixel_buffer;
    int width;
    int height;
    SDL_Texture* texture;
    int frame_count;
    volatile int updated;
} VideoStream;

// 函数声明
void init_video_streams(SDL_Renderer* renderer, VideoStream streams[]);
void cleanup_video_streams(VideoStream streams[]);
void process_video_packet(VideoStream* stream, const unsigned char* data, int size);
void update_textures(VideoStream streams[]);
void render_streams(SDL_Renderer* renderer, VideoStream streams[]);

#endif // !VIDEO_RECEIVER_H
