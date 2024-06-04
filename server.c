#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int get_random_number(int min, int max)
{
    return min + (rand() % (max - min));
}

int main()
{
    srand(time(NULL));
    int server_fd, new_socket;

    int port = get_random_number(4000, 65000);

    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
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

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);

        if ((new_socket = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addrlen)) < 0) {
            perror("Accept failed!");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        printf("[+] New Connection (%d)\n", new_socket);
        int pid = fork();
        if (pid == 0) {
            close(server_fd);
            printf("[+] Forked successfully\n");
            int bytes_recv;
            char buffer[1024];
            while ((bytes_recv = recv(new_socket, &buffer, 1024, 0)) > 0) {
                for (int i = 0; i < bytes_recv; i++) {
                    printf("%c", buffer[i]);
                }
            }

            if (bytes_recv == 0) {
                printf("Client disconnected\n");
            } else if (bytes_recv < 0) {
                perror("recv");
            }

        } else if (pid > 0) {
            close(new_socket);
        } else {
            perror("fork failed");
            close(new_socket);
        }
    }
}
