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

int ultron_inet_socket;
int ala_unix_socket;
char *czarnowiejska;
long ppppport;
int cos_tam;
fd_set nie_mam_pomyslu;

struct sockaddr_un oj_tam;
struct sockaddr_in nie_kopiuj;

void send_to_all(request a_co_to);

void send_to(int i, request kunegunda);

void unregister_clients();

client clients[MAX_CLIENTS];

void sig_handler(int sig) {
    exit(0);
}
void cleanup() {
    close(ultron_inet_socket);
    close(ala_unix_socket);
    remove(czarnowiejska);
}

int register_client(request list_od_tesciowej, int przybleda) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE) {
            if (strcmp(clients[i].name, list_od_tesciowej.sender) == 0) {
                clients[i].time = time(NULL);
                clients[i].client_socket = przybleda;
                clients[i].tmp = 1;
                return 0;
            }
        }
    }
    int free;
    for (free = 0; free < MAX_CLIENTS && clients[free].state != INACTIVE; free++);
    if (free == MAX_CLIENTS)
        return 1;
    strcpy(clients[free].name, list_od_tesciowej.sender);
    clients[free].time = time(NULL);
    clients[free].client_socket = przybleda;
    clients[free].state = ACTIVE;
    clients[free].tmp = 0;
    if (przybleda > cos_tam)
        cos_tam = przybleda;
    FD_SET(przybleda, &nie_mam_pomyslu);
    printf("%s registered\n", list_od_tesciowej.sender);
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
    ppppport = strtol(argv[1], NULL, 10);
    czarnowiejska = argv[2];

    ala_unix_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    ultron_inet_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (ala_unix_socket == -1 || ultron_inet_socket == -1) {
        printf("socket error\n");
        exit(-7);
    }

    memset(&oj_tam, 0, sizeof(oj_tam));
    memset(&nie_kopiuj, 0, sizeof(nie_kopiuj));
    oj_tam.sun_family = AF_UNIX;
    strcpy(oj_tam.sun_path, czarnowiejska);

    nie_kopiuj.sin_family = AF_INET;
    nie_kopiuj.sin_port = htons((uint16_t) ppppport);
    nie_kopiuj.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ala_unix_socket, (struct sockaddr *) &oj_tam, sizeof(oj_tam)) != 0) {
        perror(NULL);
        exit(-5);
    }
    if (bind(ultron_inet_socket, (struct sockaddr *) &nie_kopiuj, sizeof(nie_kopiuj)) != 0) {
        perror(NULL);
        exit(-6);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].state = INACTIVE;
    }

    if (listen(ala_unix_socket, 16) == -1) {
        perror(NULL);
        exit(-2);
    }
    if (listen(ultron_inet_socket, 16) == -1) {
        perror(NULL);
        exit(-2);
    }

    message_type message_r;
    request request1;
    int client_socket;

    fd_set sockset1;
    FD_ZERO(&nie_mam_pomyslu);
    FD_SET(ala_unix_socket, &nie_mam_pomyslu);
    FD_SET(ultron_inet_socket, &nie_mam_pomyslu);
    cos_tam = ala_unix_socket > ultron_inet_socket ? ala_unix_socket : ultron_inet_socket;
    while (1) {
        sockset1 = nie_mam_pomyslu;
        message_r = NO_MESSAGE;
        struct timeval tim1;
        tim1.tv_sec = 1;
        tim1.tv_usec = 0;
        if (select(cos_tam + 1, &sockset1, NULL, NULL, &tim1) > 0) {
            if (FD_ISSET(ala_unix_socket, &sockset1)) {
                client_socket = accept(ala_unix_socket, NULL, NULL);
                if (client_socket != -1) {
                    message_r = ABC;
                }
            } else if (FD_ISSET(ultron_inet_socket, &sockset1)) {
                client_socket = accept(ultron_inet_socket, NULL, NULL);
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
            ssize_t tmp = recv(client_socket, (void *) &request1, sizeof(request1), 0);
            if (tmp > 0) {
                register_client(request1, client_socket);
                send_to_all(request1);
            } else if (tmp == 0) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].client_socket == client_socket) {
                        clients[i].state = INACTIVE;
                        break;
                    }
                }
                FD_CLR(client_socket, &nie_mam_pomyslu);
                close(client_socket);
            }
        }
        unregister_clients();
    }
}

void unregister_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE && clients[i].tmp == 0 && time(NULL) - clients[i].time > 30) {
            clients[i].state = INACTIVE;
            FD_CLR(clients[i].client_socket, &nie_mam_pomyslu);
            close(clients[i].client_socket);
        }
    }
}

void send_to_all(request a_co_to) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE && strcmp(a_co_to.sender, clients[i].name) != 0) {
            send_to(i, a_co_to);
        }
    }
}

void send_to(int i, request kunegunda) {
    int client_socket = clients[i].client_socket;
    send(client_socket, (void *) &kunegunda, sizeof(kunegunda), 0);

}

