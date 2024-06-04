#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv)
{

    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    printf("%d\n", port);
    char buffer[1024];
    int client_fd;
    struct sockaddr_in serv_addr;

    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
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

    while (1) {
        printf("You: ");

        char* result = fgets(buffer, sizeof(buffer), stdin);

        if (result == NULL) {
            // Error or EOF encountered
            break;
        }

        int bytes_read = strlen(result);

        if (bytes_read == 0) {
            continue;
        }
        size_t bytes_send = send(client_fd, buffer, bytes_read, 0);
        if (bytes_read != bytes_send) {
            perror("Could't send data to server");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    }
}
