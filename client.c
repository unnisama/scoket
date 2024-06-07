#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 10

int epoll_fd;
struct epoll_event ev, events[MAX_EVENTS];

int port;
char is_first = 1;

int main(int argc, char** argv)
{

    if (argc < 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[2]);

    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    printf("%d\n", port);
    char buffer[1024];
    int client_fd;
    struct sockaddr_in serv_addr;

    epoll_fd = epoll_create1(0);

    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_family = AF_INET;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    printf("[+] Socket created successfully (%d)\n", client_fd);

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Couldn't connect to server");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("[+] Connected successfully \n");

    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
        shutdown(client_fd, SHUT_WR);
        perror("epoll_ctl");
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }
    int stdin_fileno = fileno(stdin);
    ev.data.fd = stdin_fileno;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0) {
        shutdown(client_fd, SHUT_WR);
        perror("epoll_ctl");
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (is_first) {
            printf("> ");
            fflush(stdout);
            is_first = 0;
        }
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            shutdown(client_fd, SHUT_WR);
            perror("epoll_wait");
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == stdin_fileno) {
                char buffer[1024];
                int result;

                while ((result = read(0, &buffer, 1024)) > 0) {
                    if (strncmp(buffer, "q\n", result) == 0) {
                        if (shutdown(client_fd, SHUT_WR) != -1) {
                            printf("Shuting Downing!\n");
                            close(epoll_fd);
                            exit(EXIT_SUCCESS);
                        } else {
                            perror("shutdown");
                            close(epoll_fd);
                            exit(EXIT_SUCCESS);
                        }
                    }
                    size_t bytes_send = send(client_fd, buffer, result, MSG_DONTWAIT);
                    if (result != bytes_send) {
                        perror("Could't send data to server");
                        shutdown(client_fd, SHUT_WR);
                        close(epoll_fd);
                        exit(EXIT_FAILURE);
                    }
                    printf("> ");
                    fflush(stdout);
                }
            } else if (events[i].data.fd == client_fd) {

                if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
                    close(epoll_fd);
                    printf("Server isn't working\n");
                    exit(EXIT_FAILURE);
                }
                int bytes_recv;
                char buffer[1024];
                while (1) {
                    bytes_recv = recv(client_fd, &buffer, 1024, MSG_DONTWAIT);
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
                    printf("> ");
                    fflush(stdout);
                }
            }
        }
    }
}
