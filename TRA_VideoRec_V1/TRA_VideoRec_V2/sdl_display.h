//
//  sdl_display.h
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

#include <SDL2/SDL.h>

// 函数声明
int init_sdl(SDL_Window** window, SDL_Renderer** renderer);
void cleanup_sdl(SDL_Window* window, SDL_Renderer* renderer);
int handle_events(void);

#endif // !SDL_DISPLAY_H
