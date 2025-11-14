#include "sdl_display.h"
#include "video_receiver.h"
#include "common.h"

#include <SDL2/SDL.h>
#include <stdio.h>

static SDL_Window   *g_window   = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture  *g_textures[NUM_STREAMS];

void init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return;
    }

    g_window = SDL_CreateWindow("H264 Video Receiver",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                WINDOW_WIDTH,
                                WINDOW_HEIGHT,
                                0);

    g_renderer = SDL_CreateRenderer(g_window, -1,
                                    SDL_RENDERER_ACCELERATED |
                                    SDL_RENDERER_PRESENTVSYNC);

    for (int i = 0; i < NUM_STREAMS; i++) {
        g_textures[i] = SDL_CreateTexture(
            g_renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            SUB_WIDTH, SUB_HEIGHT
        );
    }

    printf("SDL initialized.\n");
}

void sdl_display_frame()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            exit(0);
    }

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);

    for (int i = 0; i < NUM_STREAMS; i++) {

        VideoStream *vs = &g_streams[i];

        if (vs->updated) {
            SDL_UpdateTexture(g_textures[i], NULL,
                              vs->rgb_frame->data[0],
                              vs->rgb_frame->linesize[0]);
            vs->updated = 0;
        }

        SDL_Rect rect;
        if      (i == 0) rect = (SDL_Rect){0,         0,          SUB_WIDTH, SUB_HEIGHT};
        else if (i == 1) rect = (SDL_Rect){SUB_WIDTH, 0,          SUB_WIDTH, SUB_HEIGHT};
        else if (i == 2) rect = (SDL_Rect){0,         SUB_HEIGHT, SUB_WIDTH, SUB_HEIGHT};
        else             rect = (SDL_Rect){SUB_WIDTH, SUB_HEIGHT, SUB_WIDTH, SUB_HEIGHT};

        SDL_RenderCopy(g_renderer, g_textures[i], NULL, &rect);
    }

    // 中间十字线（可选）
    int mid_x = WINDOW_WIDTH  / 2;
    int mid_y = WINDOW_HEIGHT / 2;
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    for (int o = -1; o <= 1; ++o) {
        SDL_RenderDrawLine(g_renderer, mid_x + o, 0,           mid_x + o, WINDOW_HEIGHT);
        SDL_RenderDrawLine(g_renderer, 0,         mid_y + o,   WINDOW_WIDTH, mid_y + o);
    }

    SDL_RenderPresent(g_renderer);
}

void destroy_sdl()
{
    for (int i = 0; i < NUM_STREAMS; i++)
        if (g_textures[i]) SDL_DestroyTexture(g_textures[i]);

    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window)   SDL_DestroyWindow(g_window);

    SDL_Quit();
}
