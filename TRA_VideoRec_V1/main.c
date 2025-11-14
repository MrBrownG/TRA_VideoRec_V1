// main.c
#include <stdio.h>
#include <SDL2/SDL.h>

#include "common.h"
#include "video_receiver.h"
#include "network_processor.h"
#include "sdl_display.h"
#include "audio.h"

extern void start_rtsp_reader(const char *url);
extern void start_rtsp_reader(const char *url);
extern void start_mac_camera_reader(void);

int main(void)
{
    printf("Receiver with iPhone + Mac camera + audio\n");

        init_video_streams();

        if (init_sdl() != 0) {
            printf("init_sdl failed\n");
            return 1;
        }

        if (audio_init() != 0) {
            printf("audio_init failed\n");
            // 不 return，视频可以照常跑，只是没声音
        }

        // 预设：只听第 0 路（iPhone）
        g_audio_source = 0;
        // 如果以后想默认静音：g_audio_source = -1;
        // 想把声音切到 Mac 那路（假设将来给它加上音频处理）：g_audio_source = 1;

        start_rtsp_reader("rtsp://brown.local:8554/live");
        start_mac_camera_reader();   // 这一条目前只处理视频


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
