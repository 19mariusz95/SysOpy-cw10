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

int zebra_inet_socket;
int lis_unix_socket;
char *kawiory;
long krowa_port;
fd_set kielbasa_sockset;


struct sockaddr_un marmolada_unix_address;
struct sockaddr_in dzem_inet_address;
struct sockaddr_un stoperan_client_unix_address;
struct sockaddr_in leki_client_inet_address;

socklen_t rosol = sizeof(struct sockaddr_un);
socklen_t krupnik = sizeof(struct sockaddr_in);

void send_to_all(request basia);

void send_to(int i, request otwieracz);

void unregister_clients();

client clients[MAX_CLIENTS];

void sig_handler(int sig) {
    exit(0);
}

void cleanup() {
    close(zebra_inet_socket);
    close(lis_unix_socket);
    remove(kawiory);
}

int register_client(request zygfryd, struct sockaddr *obojniak, message_type e_tam) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE) {
            if (strcmp(clients[i].name, zygfryd.sender) == 0) {
                clients[i].time = time(NULL);
                return 0;
            }
        }
    }
    int wolny_ptak;
    for (wolny_ptak = 0; wolny_ptak < MAX_CLIENTS && clients[wolny_ptak].state != INACTIVE; wolny_ptak++);
    if (wolny_ptak == MAX_CLIENTS)
        return 1;
    strcpy(clients[wolny_ptak].name, zygfryd.sender);
    clients[wolny_ptak].time = time(NULL);
    if (e_tam == UNIX) {
        clients[wolny_ptak].state = LOCAL;
        clients[wolny_ptak].unix_address = *((struct sockaddr_un *) obojniak);
    } else if (e_tam == INET) {
        clients[wolny_ptak].state = GLOBAL;
        clients[wolny_ptak].inet_address = *((struct sockaddr_in *) obojniak);
    }
    printf("%s registered\n", zygfryd.sender);
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("no enough args\n");
        exit(-1);
    }
    atexit(cleanup);
    signal(SIGINT, sig_handler);
    krowa_port = strtol(argv[1], NULL, 10);
    kawiory = argv[2];

    lis_unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    zebra_inet_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (lis_unix_socket == -1 || zebra_inet_socket == -1) {
        printf("socket error\n");
        exit(-7);
    }

    memset(&marmolada_unix_address, 0, sizeof(marmolada_unix_address));
    memset(&dzem_inet_address, 0, sizeof(dzem_inet_address));
    marmolada_unix_address.sun_family = AF_UNIX;
    strcpy(marmolada_unix_address.sun_path, kawiory);

    dzem_inet_address.sin_family = AF_INET;
    dzem_inet_address.sin_port = htons((uint16_t) krowa_port);
    dzem_inet_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(lis_unix_socket, (struct sockaddr *) &marmolada_unix_address, sizeof(marmolada_unix_address)) != 0) {
        perror(NULL);
        exit(-5);
    }
    if (bind(zebra_inet_socket, (struct sockaddr *) &dzem_inet_address, sizeof(dzem_inet_address)) != 0) {
        perror(NULL);
        exit(-6);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].state = INACTIVE;
    }

    message_type projektor;
    request vader;

    fd_set sockset1;
    FD_ZERO(&kielbasa_sockset);
    FD_SET(lis_unix_socket, &kielbasa_sockset);
    FD_SET(zebra_inet_socket, &kielbasa_sockset);
    int flaga = 1;
    while (flaga) {
        sockset1 = kielbasa_sockset;
        projektor = NO_MESSAGE;
        struct timeval tim1;
        tim1.tv_sec = 1;
        tim1.tv_usec = 0;
        if (select(lis_unix_socket > zebra_inet_socket ? lis_unix_socket + 1 : zebra_inet_socket + 1, &sockset1, NULL, NULL, &tim1) > 0) {
            if (FD_ISSET(lis_unix_socket, &sockset1)) {
                ssize_t tmp = recvfrom(lis_unix_socket, &vader, sizeof(vader), MSG_DONTWAIT,
                                       (struct sockaddr *) &stoperan_client_unix_address,
                                       &rosol);
                if (tmp > 0) {
                    projektor = UNIX;
                } else if (tmp == 0) {
                    printf("disconnected\n");
                    exit(1);
                }
            } else if (FD_ISSET(zebra_inet_socket, &sockset1)) {
                ssize_t tmp = recvfrom(zebra_inet_socket, &vader, sizeof(vader), MSG_DONTWAIT,
                                       (struct sockaddr *) &leki_client_inet_address,
                                       &krupnik);
                if (tmp > 0) {
                    projektor = INET;
                } else if (tmp == 0) {
                    printf("disconnected\n");
                    exit(1);
                }
            }
            if (projektor != NO_MESSAGE) {
                register_client(vader, projektor == UNIX ? (struct sockaddr *) &stoperan_client_unix_address
                                                            : (struct sockaddr *) &leki_client_inet_address, projektor);
                send_to_all(vader);
            }
        }
        unregister_clients();
    }
}

void unregister_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE && time(NULL) - clients[i].time > 30) {
            clients[i].state = INACTIVE;
            printf("client %s unregistered\n", clients[i].name);
        }
    }
}

void send_to_all(request basia) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state != INACTIVE && strcmp(basia.sender, clients[i].name) != 0) {
            send_to(i, basia);
        }
    }
}

void send_to(int i, request otwieracz) {
    struct sockaddr *address;
    int socket;
    socklen_t address_len;
    if (clients[i].state == LOCAL) {
        address = (struct sockaddr *) &(clients[i].unix_address);
        socket = lis_unix_socket;
        address_len = rosol;
    } else {
        address = (struct sockaddr *) &(clients[i].inet_address);
        socket = zebra_inet_socket;
        address_len = krupnik;
        char ip[20];
        inet_ntop(AF_INET, &(clients[i].inet_address.sin_addr.s_addr), ip, 20);
    }
    if (sendto(socket, (void *) &otwieracz, sizeof(otwieracz), 0, address, address_len) == -1) {
        perror(NULL);
    }
}

