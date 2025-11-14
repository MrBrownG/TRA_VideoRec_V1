// simple_pattern_sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

typedef struct {
    uint32_t frame_count;
    uint32_t stream_id;
    uint32_t block_x;
    uint32_t block_y;
    uint32_t pattern_type;
} FrameData;

int main() {
    int ports[] = {5000, 5001, 5002, 5003};
    int sockfds[4];
    struct sockaddr_in dest_addrs[4];
    
    printf("Starting Pattern Video Sender...\n");
    printf("Sending pattern data to ports: 5000, 5001, 5002, 5003\n");
    printf("Press Ctrl+C to stop\n");
    
    // 初始化4个socket
    for (int i = 0; i < 4; i++) {
        sockfds[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfds[i] < 0) {
            perror("socket creation failed");
            return -1;
        }
        
        dest_addrs[i] = (struct sockaddr_in){
            .sin_family = AF_INET,
            .sin_port = htons(ports[i]),
            .sin_addr.s_addr = inet_addr("127.0.0.1")
        };
        printf("Stream %d: Ready for port %d\n", i, ports[i]);
    }
    
    int frame_count = 0;
    
    while (1) {
        for (int stream_id = 0; stream_id < 4; stream_id++) {
            FrameData data = {
                .frame_count = frame_count,
                .stream_id = stream_id,
                .block_x = (frame_count * 8) % 320,  // 移动速度
                .block_y = (frame_count * 5) % 220,
                .pattern_type = stream_id
            };
            
            // 发送数据
            int sent = sendto(sockfds[stream_id], &data, sizeof(data), 0,
                           (struct sockaddr*)&dest_addrs[stream_id], sizeof(dest_addrs[stream_id]));
            
            if (sent < 0) {
                perror("sendto failed");
            } else {
                // 可选：显示发送状态
                if (frame_count % 50 == 0) {
                    printf("Stream %d: Sent frame %d (x:%d, y:%d)\n",
                           stream_id, frame_count, data.block_x, data.block_y);
                }
            }
        }
        
        frame_count++;
        usleep(50000); // 20fps
    }
    
    for (int i = 0; i < 4; i++) {
        close(sockfds[i]);
    }
    
    return 0;
}

