//
//  sdl_display.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//

#include "sdl_display.h"
#include "common.h"
#include <stdio.h>

// init SDL system
int init_sdl(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! Error: %s\n", SDL_GetError());
        return -1;
    }

    *window = SDL_CreateWindow(
        "Mission Control - Network Video Display",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!*window) {
        fprintf(stderr, "Window could not be created! Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    *renderer = SDL_CreateRenderer(
        *window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!*renderer) {
        fprintf(stderr, "Renderer could not be created! Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    return 0;
}

// 清理SDL资源
void cleanup_sdl(SDL_Window* window, SDL_Renderer* renderer) {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

// 处理SDL事件
int handle_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return 0;  // 退出
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) { //type "esc" of not
                return 0;  // 退出
            }
        }
    }
    return 1;  // 继续运行
}
