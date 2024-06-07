#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <bits/types/sigset_t.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_CONNECTIONS 20

#define MAX_EVENTS 10

int epoll_fd;
struct epoll_event ev, events[MAX_EVENTS];
int server_fd, client_socket;

int fds[MAX_CONNECTIONS];
int currentfds = 0;

int get_random_number(int min, int max)
{
    return min + (rand() % (max - min));
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create1(0);
    srand(time(NULL));

    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_family = AF_INET;
    int addrlen = sizeof(addr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    printf("[+] Socket created successfully (%d) Port: [%d]\n", server_fd, port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind Failed!");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket bind successfully\n");

    if (listen(server_fd, 10) == -1) {
        perror("Listen failed!");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Set Socket in listen mode\n");

    ev.data.fd = server_fd;
    ev.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);

        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            close(server_fd);
            close(epoll_fd);
            for (int i = 0; i < currentfds; i++) {
                shutdown(fds[i], SHUT_WR);
            }
            if (errno == EINTR) {
                printf("Shuting Down!\n");
                break;
            }
            perror("epoll_wait error!");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                if ((client_socket = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addrlen)) < 0) {
                    perror("Accept failed!");
                    continue;
                }

                ev.data.fd = client_socket;
                ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
                    perror("epoll_ctl");
                    close(server_fd);
                    close(client_socket);
                    exit(EXIT_FAILURE);
                }

                fds[currentfds] = client_socket;
                currentfds += 1;

                printf("[+] New Connection (%d)\n", client_socket);
            } else {
                client_socket = events[i].data.fd;
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
                    printf("Client disconnected\n");
                    ev.data.fd = client_socket;
                    ev.events = 0;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, &ev) == -1) {
                        perror("epoll_ctl");
                        close(server_fd);
                        close(client_socket);
                        exit(EXIT_FAILURE);
                    }

                    for (int i = 0; i < currentfds; i++) {
                        if (fds[i] == client_socket) {
                            currentfds -= 1;
                            for (int j = i; j < currentfds; j++) {
                                fds[i] = fds[i + 1];
                            }
                            break;
                        }
                    }
                    close(client_socket);
                    continue;
                }
                int bytes_recv;
                char buffer[1024];
                sprintf(buffer, "[%d]", client_socket);
                bytes_recv = strlen(buffer);

                for (int i = 0; i < currentfds; i++) {
                    if (fds[i] == client_socket) {
                        continue;
                    }
                    int ret = send(fds[i], &buffer, bytes_recv, MSG_DONTWAIT);
                }
                printf("%s> ", buffer);
                while (1) {
                    bytes_recv = recv(client_socket, &buffer, 1024, MSG_DONTWAIT);
                    if (bytes_recv == 0) {
                        break;
                    } else if (bytes_recv < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        perror("recv");
                        break;
                    }
                    for (int i = 0; i < bytes_recv; i++) {
                        printf("%c", buffer[i]);
                    }
                    for (int i = 0; i < currentfds; i++) {
                        if (fds[i] == client_socket) {
                            continue;
                        }
                        int ret = send(fds[i], &buffer, bytes_recv, MSG_DONTWAIT);
                    }
                }
            }
        }
    }
}
