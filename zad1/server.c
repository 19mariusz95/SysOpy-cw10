#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <time.h>
#include "trzustka.h"

int inet_socket;
int unix_socket;
char *path;
long port;


struct sockaddr_un unix_address;
struct sockaddr_in inet_address;
struct sockaddr_un client_unix_address;
struct sockaddr_in client_inet_address;

socklen_t unix_address_size = sizeof(struct sockaddr_un);
socklen_t inet_address_size = sizeof(struct sockaddr_in);

int unregister_client(request mess);

void send_to_all(request request1);

void send_to(int i, request request1);

client clients[MAX_CLIENTS];

void sig_handler(int sig) {
    exit(0);
}

void cleanup() {
    close(inet_socket);
    close(unix_socket);
    remove(path);
}

int register_client(request req, struct socaddr *addr, message_type type) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE) {
            if (strcmp(clients[i].name, req.sender) == 0) {
                clients[i].time = time(NULL);
                return 0;
            }
        }
    }
    int free;
    for (free = 0; free < MAX_CLIENTS && clients[free].state == INACTIVE; free++);
    if (free == MAX_CLIENTS)
        return 1;
    strcpy(clients[free].name, req.sender);
    clients[free].time = time(NULL);
    if (type == UNIX) {
        clients[free].unix_address = *((struct sockaddr_un *) addr);
    } else if (type == INET) {
        clients[free].inet_address = *((struct sockaddr_in *) addr);
    }
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("no enough args\n");
        exit(-1);
    }
    atexit(cleanup);
    signal(SIGINT, sig_handler);
    port = strtol(argv[1], NULL, 10);
    path = argv[2];

    unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    inet_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (unix_socket == -1 || inet_socket == -1) {
        printf("socket error\n");
        exit(-7);
    }

    memset(&unix_address, 0, sizeof(unix_address));
    memset(&inet_address, 0, sizeof(inet_address));
    unix_address.sun_family = AF_UNIX;
    strcpy(unix_address.sun_path, path);

    inet_address.sin_family = AF_INET;
    inet_address.sin_port = htons((uint16_t) port);
    inet_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(unix_socket, (struct sockaddr *) &unix_address, sizeof(unix_address)) != 0) {
        perror(NULL);
        exit(-5);
    }
    if (bind(inet_socket, (struct sockaddr *) &inet_address, sizeof(inet_address)) != 0) {
        perror(NULL);
        exit(-6);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].state = INACTIVE;
    }

    message_type message_r;
    request request1;

    int flaga = 1;
    while (flaga) {
        message_r = NO_MESSAGE;
        while (message_r == NO_MESSAGE) {
            if (recvfrom(unix_socket, &request1, sizeof(request1), 0, &client_unix_address, &unix_address_size) != 0) {
                message_r = UNIX;
            } else if (
                    recvfrom(inet_socket, &request1, sizeof(request1), 0, &client_inet_address, &inet_address_size) !=
                    0) {
                message_r = INET;
            }
        }
        register_client(request1, message_r == UNIX ? (struct sockaddr *) &client_unix_address
                                                    : (struct sockaddr *) &client_inet_address, message_r);

        send_to_all(request1);

    }
}

void send_to_all(request request1) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE && strcmp(request1.sender, clients[i].name) != 0) {
            send_to(i, request1);
        }
    }
}

void send_to(int i, request request1) {
    struct sockaddr *address;
    int socket;
    socklen_t address_len;
    if (clients[i].state == LOCAL) {
        address = (struct sockaddr *) &(clients[i].unix_address);
        socket = unix_socket;
        address_len = unix_address_size;
    } else {
        address = (struct sockaddr *) &(clients[i].inet_address);
        socket = inet_socket;
        address_len = inet_address_size;
        char ip[20];
        inet_ntop(AF_INET, &(clients[i].inet_address.sin_addr.s_addr), ip, 20);
    }
    if (sendto(socket, (void *) &request1, sizeof(request1), 0, address, address_len) == -1) {
        perror(NULL);
    }

}

