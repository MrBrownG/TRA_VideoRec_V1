//
//  network_processor.h
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#ifndef NETWORK_PROCESSOR_H
#define NETWORK_PROCESSOR_H

#include "common.h"

// 启动 4 路 UDP 接收线程
void start_network_receivers(void);
void stop_network_receivers(void);
#endif
