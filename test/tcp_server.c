/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "socket_utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

#define WORKER_NUM 5

void *run(void *arg) {
    int new_socket = *(int *)arg;

    printf("[Server] Communicate with the new connection #%u @ %p ..\n", new_socket, (void *)(uintptr_t)pthread_self());
    char *message1 = (char *)malloc(1024);

    while (1) {
        int j;
        for (j = 0; j < 1024; j++) {
            message1[j] = 'a';
        }
        if (send(new_socket, message1, strlen(message1), 0) < 0) {
            perror("Send failed");
            break;
        }
        // sleep(10);
        while (1) {
            if (recv(new_socket, message1, 1024, 0) < 0) {
                sleep(1);
            } else {
                break;
            }
        }
        printf("recv: %s\n", message1);
    }
    printf("[Server] Shuting down the new connection #%u ..\n", new_socket);
    shutdown(new_socket, SHUT_RDWR);

    return NULL;
}

static void init_sockaddr_inet(struct sockaddr_in *addr) {
    /* 0.0.0.0:1234 */
    addr->sin_family = AF_INET;
    addr->sin_port = htons(1234);
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
}

static void init_sockaddr_inet6(struct sockaddr_in6 *addr) {
    /* [::]:1234 */
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(1234);
    addr->sin6_addr = in6addr_any;
}
void init_connect(int socket_fd) {
    int addrlen = 0, af;
    struct sockaddr_storage addr = {0};
    af = AF_INET;
    addrlen = sizeof(struct sockaddr_in6);
    init_sockaddr_inet((struct sockaddr_in *)&addr);

    printf("[Server] Create socket\n");
    int ret = socket(AF_INET, SOCK_STREAM, 0);
    while (ret != socket_fd) {
        ret = socket(AF_INET, SOCK_STREAM, 0);
        printf("connect %d %d\n", ret, socket_fd);
    }

    printf("[Server] Bind socket\n");
    if (bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
        perror("Bind failed");
        exit(-1);
    }

    printf("[Server] Listening on socket\n");
    if (listen(socket_fd, 3) < 0) {
        perror("Listen failed");
        exit(-1);
    }

    printf("[Server] Wait for clients to connect ..\n");
}

int main(int argc, char *argv[]) {
    int socket_fd = -1, addrlen = 0, af;
    struct sockaddr_storage addr = {0};
    unsigned connections = 0;
    pthread_t workers[WORKER_NUM] = {0};
    int client_sock_fds[WORKER_NUM] = {0};
    char ip_string[64];

    if (argc > 1 && strcmp(argv[1], "inet6") == 0) {
        af = AF_INET6;
        addrlen = sizeof(struct sockaddr_in6);
        init_sockaddr_inet6((struct sockaddr_in6 *)&addr);
    } else {
        af = AF_INET;
        addrlen = sizeof(struct sockaddr_in6);
        init_sockaddr_inet((struct sockaddr_in *)&addr);
    }

    printf("[Server] Create socket\n");
    socket_fd = socket(af, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Create socket failed");
        goto fail;
    }

    printf("[Server] Bind socket\n");
    if (bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
        perror("Bind failed");
        goto fail;
    }

    printf("[Server] Listening on socket\n");
    if (listen(socket_fd, 3) < 0) {
        perror("Listen failed");
        goto fail;
    }

    printf("[Server] Wait for clients to connect ..\n");
    while (connections < WORKER_NUM) {
        addrlen = sizeof(struct sockaddr);
        client_sock_fds[connections] = accept(socket_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (client_sock_fds[connections] < 0) {
            perror("Accept failed");
            break;
        }

        if (sockaddr_to_string((struct sockaddr *)&addr, ip_string, sizeof(ip_string) / sizeof(ip_string[0])) != 0) {
            printf("[Server] failed to parse client address\n");
            goto fail;
        }

        printf("[Server] Client connected (%s)\n", ip_string);
        if (pthread_create(&workers[connections], NULL, run, &client_sock_fds[connections])) {
            perror("Create a worker thread failed");
            shutdown(client_sock_fds[connections], SHUT_RDWR); // close
            break;
        }

        connections++;
    }

    if (connections == WORKER_NUM) {
        printf("[Server] Achieve maximum amount of connections\n");
    }

    for (int i = 0; i < WORKER_NUM; i++) {
        pthread_join(workers[i], NULL);
    }

    printf("[Server] Shuting down ..\n");
    shutdown(socket_fd, SHUT_RDWR);
    sleep(3);
    printf("[Server] BYE \n");
    __wasilibc_nocwd_openat_nomode(1,"/dev/stdout",0);
    return EXIT_SUCCESS;

fail:
    printf("[Server] Shuting down ..\n");
    if (socket_fd >= 0)
        close(socket_fd);
    sleep(3);
    return EXIT_FAILURE;
}
