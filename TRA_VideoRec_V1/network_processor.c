// network_processor.c
#include "network_processor.h"
#include "video_receiver.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static pthread_t g_threads[NUM_STREAMS];
static int       g_running = 0;

// 每路接收线程
static void *network_receiver_thread(void *arg)
{
    int sid = (int)(intptr_t)arg;
    int port = 5000 + sid;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return NULL;
    }

    printf("Stream %d: listening on UDP port %d\n", sid, port);

    uint8_t buf[2048];
    while (g_running) {
        ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
        if (n <= 0) {
            if (errno == EINTR) continue;
            usleep(1000);
            continue;
        }

        // 把整个 UDP 包交给分片重组逻辑
        push_h264_chunk(sid, buf, (int)n);
    }

    close(sock);
    return NULL;
}

void start_network_receivers(void)
{
    if (g_running) return;
    g_running = 1;

    for (int i = 0; i < NUM_STREAMS; ++i) {
        int rc = pthread_create(&g_threads[i], NULL,
                                network_receiver_thread,
                                (void *)(intptr_t)i);
        if (rc != 0) {
            fprintf(stderr, "Failed to create network thread %d: %s\n",
                    i, strerror(rc));
        }
    }
}

void stop_network_receivers(void)
{
    if (!g_running) return;
    g_running = 0;

    for (int i = 0; i < NUM_STREAMS; ++i) {
        if (g_threads[i]) {
            pthread_join(g_threads[i], NULL);
        }
    }
}
