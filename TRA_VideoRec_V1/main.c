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

int main()
{
    init_sdl();
    init_video_streams();
    start_network_receivers();

    while (1) {
        sdl_display_frame();
        SDL_Delay(10);
    }

    destroy_sdl();
    free_video_streams();

    return 0;
}
