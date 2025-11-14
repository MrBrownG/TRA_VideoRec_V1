#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

//
//  common.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
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



#endif // !VARIABLES_H
