#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdint.h>

// 通用常量定义
#define NUM_STREAMS 4
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_PACKET_SIZE 65536

// 网络数据传输结构
typedef struct {
    uint32_t frame_count;
    uint32_t stream_id;
    uint32_t block_x;
    uint32_t block_y;
    uint32_t pattern_type;
} FrameData;

typedef struct {
    int stream_id;
    int port;
    Uint32* pixel_buffer;  // SDL像素缓冲区 - 存储ARGB格式像素数据
    int width;
    int height;
    SDL_Texture* texture;  //  SDL纹理对象 - GPU中的图像数据
    int frame_count;
    volatile int updated; // 网络线程置位，主线程清除
} VideoStream;

#endif // !VARIABLES_H
