//
//  network_processor.h
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#ifndef NETWORK_PROCESSOR_H
#define NETWORK_PROCESSOR_H

#include <pthread.h>
#include "video_receiver.h"

// 函数声明
void start_network_receivers(VideoStream streams[]);
void stop_network_receivers(void);

#endif // !NETWORK_PROCESSOR_H
