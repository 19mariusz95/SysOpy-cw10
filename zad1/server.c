#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <zconf.h>
#include "trzustka.h"

int inet_socket;
int unix_socket;
char *path;
long port;


struct sockaddr_un unix_address;
struct sockaddr_in inet_address;
struct sockaddr_un client_unix_address;
struct sockaddr_in client_inet_address;

client clients[MAX_CLIENTS];

void sig_handler(int sig) {
    exit(0);
}

void cleanup() {
    close(inet_socket);
    close(unix_socket);
    remove(path);
}

int register_client(message msg, struct socaddr *addr, message_type type) {
    int i;
    for (i = 0; i < MAX_CLIENTS && clients[i].state != INACTIVE; i++);
    if (i == MAX_CLIENTS) {
        return 0;
    }
    strcpy(clients[i].name, msg.sender);
    if (type == UNIX) {
        clients[i].state = LOCAL;
        memcpy(&clients[i].unix_address, addr, sizeof(struct sockaddr_un));
    } else {
        clients[i].state = GLOBAL;
        memcpy(&clients[i].inet_address, addr, sizeof(struct sockaddr_in));
    }
    return 1;
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

    message_type message_r;
    message mess;
    socklen_t unixaddr_size = sizeof(struct sockaddr_un);
    socklen_t inetaddr_size = sizeof(struct sockaddr_in);

    int flaga = 1;
    while (flaga) {
        message_r = NO_MESSAGE;
        while (message_r == NO_MESSAGE) {
            if (recvfrom(unix_socket, &mess, sizeof(message), 0, &client_unix_address, &unixaddr_size) != 0) {
                message_r = UNIX;
            } else if (recvfrom(inet_socket, &mess, sizeof(message), 0, &client_inet_address, &inetaddr_size) != 0) {
                message_r = INET;
            }
        }
        if (mess.req == REGISTER) {
            int res = 0;
            if (message_r == UNIX) {
                res = register_client(mess, &client_unix_address, message_r);
            } else {
                res = register_client(mess, &client_inet_address, message_r);
            }
            if (res == 0) {
                printf("error\n");
            }
        } else if (message_r == MESSAGE) {

        } else if (message_r == END) {

        }

    }
}

