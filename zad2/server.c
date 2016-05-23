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
int nsc;
fd_set sockset;

struct sockaddr_un unix_address;
struct sockaddr_in inet_address;
struct sockaddr_un client_unix_address;
struct sockaddr_in client_inet_address;

void send_to_all(request request1);

void send_to(int i, request request1);

void unregister_clients();

client clients[MAX_CLIENTS];

void sig_handler(int sig) {
    exit(0);

}
void cleanup() {
    close(inet_socket);
    close(unix_socket);
    remove(path);
}

int register_client(request req, int client_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE) {
            if (strcmp(clients[i].name, req.sender) == 0) {
                clients[i].time = time(NULL);
                clients[i].client_socket = client_socket;
                return 0;
            }
        }
    }
    int free;
    for (free = 0; free < MAX_CLIENTS && clients[free].state != INACTIVE; free++);
    if (free == MAX_CLIENTS)
        return 1;
    strcpy(clients[free].name, req.sender);
    clients[free].time = time(NULL);
    clients[free].client_socket = client_socket;
    clients[free].state = ACTIVE;
    if (client_socket > nsc)
        nsc = client_socket;
    FD_SET(client_socket, &sockset);
    printf("%s registered\n", req.sender);
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("no enough args\n");
        exit(-1);
    }
    atexit(cleanup);
    signal(SIGINT, sig_handler);
    signal(SIGPIPE,SIG_IGN);
    port = strtol(argv[1], NULL, 10);
    path = argv[2];

    unix_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    inet_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

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

    if (listen(unix_socket, 16) == -1) {
        perror(NULL);
        exit(-2);
    }
    if (listen(inet_socket, 16) == -1) {
        perror(NULL);
        exit(-2);
    }

    message_type message_r;
    request request1;
    int client_socket;

    fd_set sockset1;
    FD_ZERO(&sockset);
    FD_SET(unix_socket, &sockset);
    FD_SET(inet_socket, &sockset);
    nsc = unix_socket > inet_socket ? unix_socket : inet_socket;
    while (1) {
        sockset1 = sockset;
        message_r = NO_MESSAGE;
        if (select(nsc + 1, &sockset1, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(unix_socket, &sockset1)) {
                client_socket = accept(unix_socket, NULL, NULL);
                if (client_socket != -1) {
                    message_r = ABC;
                }
            } else if (FD_ISSET(inet_socket, &sockset1)) {
                client_socket = accept(inet_socket, NULL, NULL);
                if (client_socket != -1) {
                    message_r = ABC;
                }
            } else {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (FD_ISSET(clients[i].client_socket, &sockset1)) {
                        client_socket = clients[i].client_socket;
                        message_r = ABC;
                        break;
                    }
                }
            }
        }
        if (message_r != NO_MESSAGE) {
            if (recv(client_socket, (void *) &request1, sizeof(request1), 0) > 0) {
                register_client(request1, client_socket);
                send_to_all(request1);
            }
        }
        unregister_clients();
    }
}

void unregister_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE && time(NULL) - clients[i].time > 30) {
            clients[i].state = INACTIVE;
            FD_CLR(clients[i].client_socket,&sockset);
            close(clients[i].client_socket);
        }
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
    int client_socket = clients[i].client_socket;
    if (send(client_socket, (void *) &request1, sizeof(request1), 0) == -1) {
        perror(NULL);
    }

}

