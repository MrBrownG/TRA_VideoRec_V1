//
//  network_processor.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#include "network_processor.h"
#include "video_receiver.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_PACKET 2000   // 与发送端保持一致

typedef struct {
    int stream_id;
    int port;
} NetThreadArgs;


static void *net_thread(void *arg)
{
    NetThreadArgs *na = (NetThreadArgs *)arg;

    int sid  = na->stream_id;
    int port = na->port;

    printf("Network thread %d listening on port %d\n", sid, port);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
    }

    uint8_t buf[MAX_PACKET];

    while (1)
    {
        int len = recvfrom(sock, buf, MAX_PACKET, 0, NULL, NULL);
        if (len <= 0)
            continue;

        // H.264 chunk
        push_h264_chunk(sid, buf, len);
    }

    return NULL;
}


// =============================
// 启动所有网络接收线程
// =============================
void start_network_receivers()
{
    pthread_t th[NUM_STREAMS];
    NetThreadArgs args[NUM_STREAMS];

    for (int i = 0; i < NUM_STREAMS; i++)
    {
        args[i].stream_id = i;
        args[i].port      = 5000 + i;

        pthread_create(&th[i], NULL, net_thread, &args[i]);
    }

    printf("All network receivers started.\n");
}
