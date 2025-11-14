#ifndef NETWORK_PROCESSER_H
#define NETWORK_HANDLER_H processor

#include <pthread.h>
#include "video_receiver.h"

// 函数声明
void start_network_receivers(VideoStream streams[]);
void stop_network_receivers(void);

#endif
