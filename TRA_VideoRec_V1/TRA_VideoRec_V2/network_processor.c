//
//  network_processor.c
//  TRA_VideoRec_V1
//
//  Created by Brown Guo on 11/14/25.
//
#include "network_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static pthread_t network_threads[NUM_STREAMS];
static int running = 1;

// 网络接收线程函数
// notes:
//AF_INET：IPv4协议
//SOCK_DGRAM：UDP数据报
static void* network_receiver_thread(void* arg) {
    VideoStream* stream = (VideoStream*)arg;
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); //返回socket文件描述符
    if (sockfd < 0) {
        perror("socket creation failed");
        return NULL;
    }

    int opt = 1;
    //SO_REUSEADDR：允许端口快速重用 (之前是更新过快导致端口不能复用，导致无法更新？)
    //避免"Address already in use"错误
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;          //确定协议类型
    server_addr.sin_port = htons(stream->port); //port 5000 + id
    server_addr.sin_addr.s_addr = INADDR_ANY;  //监听和5000有关的所有端口 //
    // 相当于同时绑定了：
    // - 127.0.0.1:5000      (回环地址)
    // - 192.168.1.100:5000  (WiFi IP)
    // - 10.0.0.5:5000       (以太网IP)
    // - 其他所有本机IP:5000
    // - 发送的时候是 本机.5000+i 
    // 绑定socket到端口
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Stream %d: bind failed on port %d: %s\n",
                stream->stream_id, stream->port, strerror(errno));
        close(sockfd);
        return NULL;
    }

    printf("Stream %d listening on UDP port %d\n", stream->stream_id, stream->port);

    unsigned char packet_buffer[MAX_PACKET_SIZE]; //接受打的包
    struct sockaddr_in client_addr;    //地址信息
    socklen_t addr_len = sizeof(client_addr);
    int print_count = 0;      //包的数量

    while (running) {
        int bytes_received = recvfrom(sockfd, packet_buffer, MAX_PACKET_SIZE, 0,
                                      (struct sockaddr*)&client_addr, &addr_len);

        if (bytes_received > 0) {
            process_video_packet(stream, packet_buffer, bytes_received);
            
            print_count++;
            if (print_count % 50 == 0) {
                printf("Stream %d: processed %d packets\n", stream->stream_id, print_count);
            }
        }
        
        usleep(1000);
    }

    close(sockfd);
    return NULL;
}

// 启动网络接收器
void start_network_receivers(VideoStream streams[]) {
    running = 1;
    
    for (int i = 0; i < NUM_STREAMS; i++) {
        //&network_threads[i] where have they stored
        int rc = pthread_create(&network_threads[i], NULL, network_receiver_thread, &streams[i]);
        if (rc != 0) {
            fprintf(stderr, "Failed to create network thread for stream %d: %s\n", i, strerror(rc));
        } else {
            //Tell the system to automatically reclaim resources when this thread ends.（预备回收）
            pthread_detach(network_threads[i]);
        }
    }
    
    printf("All network receivers started. Listening ports: 5000, 5001, 5002, 5003\n");
}

// 停止网络接收器
void stop_network_receivers(void) {
    running = 0;
    // 让线程自然退出
}
