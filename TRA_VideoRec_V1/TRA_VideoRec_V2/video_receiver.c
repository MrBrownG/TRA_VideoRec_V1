//
//  video_receiver.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#include "video_receiver.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// init video prop
void init_video_streams(SDL_Renderer* renderer, VideoStream streams[]) {
    int sub_width = WINDOW_WIDTH / 2;    // Motify to have adaptive num of streams
    int sub_height = WINDOW_HEIGHT / 2;
    
    for (int i = 0; i < NUM_STREAMS; i++) {
        streams[i].stream_id = i;
        streams[i].port = 5000 + i;
        streams[i].width = sub_width;
        streams[i].height = sub_height;
        streams[i].frame_count = 0;
        streams[i].updated = 0;
        
        // 分配像素缓冲区
        streams[i].pixel_buffer = (Uint32*)malloc((size_t)sub_width * sub_height * sizeof(Uint32)); //size: 4
        if (!streams[i].pixel_buffer) {
            fprintf(stderr, "Failed to allocate pixel buffer for stream %d\n", i);
            continue;
        }
        
        // 初始化为黑色
        for (int k = 0; k < sub_width * sub_height; k++) {
            streams[i].pixel_buffer[k] = 0xFF000000u;
        }
        
        // 创建SDL纹理
        streams[i].texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,         //format of video
            SDL_TEXTUREACCESS_STREAMING,
            sub_width, sub_height
        );
        
        if (!streams[i].texture) {
            fprintf(stderr, "Failed to create texture for stream %d: %s\n", i, SDL_GetError());
        }
    }
}

// 清理视频流资源
void cleanup_video_streams(VideoStream streams[]) {
    for (int i = 0; i < NUM_STREAMS; i++) {
        if (streams[i].pixel_buffer) {
            free(streams[i].pixel_buffer);
            streams[i].pixel_buffer = NULL;
        }
        if (streams[i].texture) {
            SDL_DestroyTexture(streams[i].texture);
            streams[i].texture = NULL;
        }
    }
}

// 处理视频数据包
void process_video_packet(VideoStream* stream, const unsigned char* data, int size) {
    if (!stream || !stream->pixel_buffer) return;

    if (size == (int)sizeof(FrameData)) {
        FrameData frame_data;
        memcpy(&frame_data, data, sizeof(FrameData)); //验证大小并保存

        int width = stream->width;
        int height = stream->height;

        unsigned char colors[4][3] = {
            {0, 255, 0},{255, 0, 0}, {255, 255, 0},{0, 0, 255}
        };

        int block_size = 80;
        int block_x = (int)(frame_data.block_x % (uint32_t)(width - block_size));
        int block_y = (int)(frame_data.block_y % (uint32_t)(height - block_size));

        // 清空为黑色背景
        for (int i = 0; i < width * height; i++) {
            stream->pixel_buffer[i] = 0xFF000000u;
        }

        // 绘制彩色方块
        for (int y = block_y; y < block_y + block_size && y < height; y++) {
            for (int x = block_x; x < block_x + block_size && x < width; x++) {
                int index = y * width + x;
                Uint8 r = colors[stream->stream_id][0];
                Uint8 g = colors[stream->stream_id][1];
                Uint8 b = colors[stream->stream_id][2];
                // 分解：
                // 0xFFu << 24 = 0xFF000000  (Alpha = 255, 不透明)
                // r << 16     = 0x00RR0000  (红色分量)
                // g << 8      = 0x0000GG00  (绿色分量)
                // b           = 0x000000BB  (蓝色分量)
                stream->pixel_buffer[index] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
            }
        }

        stream->frame_count++;
        stream->updated = 1;  // 标记需要更新纹理
    }
}

//name: update_textures
//function：
//检查哪些流需要更新
//上传像素数据到GPU纹理
//标记处理完成状态

void update_textures(VideoStream streams[]) {
    for (int i = 0; i < NUM_STREAMS; i++) {
        if (streams[i].updated && streams[i].texture && streams[i].pixel_buffer) {
            SDL_UpdateTexture(
                streams[i].texture,
                NULL,
                streams[i].pixel_buffer,    //源像素数据（CPU内存中的图像）
                streams[i].width * sizeof(Uint32)
                // 计算后才知道正确跳到下一行，这里固定了
//              // 理想做法：
//              int actual_pitch;
//              Uint32* pixels;
//              if (SDL_LockTexture(streams[i].texture, NULL, (void**)&pixels, &actual_pitch) == 0) {
//              // 使用 actual_pitch（系统告诉你的真实值）
//              // 手动复制数据...
//              SDL_UnlockTexture(streams[i].texture);
//              }
            );
            streams[i].updated = 0;
        }
    }
}

// 渲染所有视频流
void render_streams(SDL_Renderer* renderer, VideoStream streams[]) {
    int sub_width = WINDOW_WIDTH / 2;
    int sub_height = WINDOW_HEIGHT / 2;
    // Motify to have adaptive num of streams
    for (int i = 0; i < NUM_STREAMS; i++) {
        int row = i / 2;
        int col = i % 2;
        int x = col * sub_width;
        int y = row * sub_height;
        SDL_Rect dest_rect = { x, y, sub_width, sub_height };
        
        if (streams[i].texture) {
            SDL_RenderCopy(renderer, streams[i].texture, NULL, &dest_rect);
        }
        
        // 绘制边框
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &dest_rect);
    }
}
