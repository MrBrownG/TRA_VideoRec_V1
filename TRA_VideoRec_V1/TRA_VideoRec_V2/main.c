//
//  main.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#include <stdio.h>
#include "common.h"
#include "video_receiver.h"
#include "network_processor.h"
#include "sdl_display.h"

int main() {
    printf("Starting SDL2 Network Video Display...\n");
    
    // SDL相关变量
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    VideoStream streams[NUM_STREAMS];
    
    // 初始化SDL
    if (init_sdl(&window, &renderer) < 0) {
        return -1;
    }
    
    // 初始化视频流
    init_video_streams(renderer, streams);
    
    // 启动网络接收
    start_network_receivers(streams);
    
    // 主渲染循环
    int running = 1;
    while (running) {
        running = handle_events();  //not necessary but keep it for further tasks
        
        // 清屏
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //set black
        SDL_RenderClear(renderer);    // apply black
        
        // 更新和渲染
        update_textures(streams);
        render_streams(renderer, streams);
        
        // 提交渲染
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    // 清理资源
    stop_network_receivers();
    cleanup_video_streams(streams);
    cleanup_sdl(window, renderer);
    
    printf("Network Video Display closed.\n");
    return 0;
}
