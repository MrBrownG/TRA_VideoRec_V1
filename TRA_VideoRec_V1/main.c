// main.c
#include <stdio.h>
#include <SDL2/SDL.h>

#include "common.h"
#include "video_receiver.h"
#include "network_processor.h"
#include "sdl_display.h"

int main(void)
{
    printf("Starting H264 receiver...\n");

    // 初始化解码器
    init_video_streams();

    // 初始化 SDL
    if (init_sdl() != 0) {
        printf("init_sdl failed\n");
        return 1;
    }

    // 启动网络接收线程
    start_network_receivers();

    // 主循环：只是让 SDL 渲染
    while (1) {
        sdl_display_frame();
        SDL_Delay(10);   // 简单限速
    }

    // 一般到不了这里（ESC / 关闭窗口时在 sdl_display 里 exit）
    stop_network_receivers();
    free_video_streams();
    destroy_sdl();

    return 0;
}
