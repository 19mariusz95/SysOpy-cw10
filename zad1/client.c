#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "trzustka.h"
#include <arpa/inet.h>
#include <zconf.h>
#include <signal.h>

char *client_id;

socklen_t address_size;
char server_ip[15];
int port;
int client_port;
int client_socket;
struct sockaddr *server_address;
struct sockaddr *client_address;

struct sockaddr_un server_unix_address;
struct sockaddr_in server_inet_address;
struct sockaddr_un client_unix_address;
struct sockaddr_in client_inet_address;

char client_path[MAX_PATH_LENGTH];
char server_path[MAX_PATH_LENGTH];

pthread_t thread;

void *thread_fun(void *arg);

void cleanup() {
    close(client_socket);
    remove(client_path);
    exit(0);
}

void sig_handler(int sig) {
    exit(0);
}

int main(int argc, char *argv[]){
    if (argc < 4) {
        printf("no enough args\n");
        exit(-1);
    }
    atexit(cleanup);
    signal(SIGINT, sig_handler);
    client_id = argv[1];
    if (strlen(client_id) >= MAX_CLIENT_NAME_LENGTH) {
        printf("too long id\n");
        exit(-3);
    }
    long global = strtol(argv[2], NULL, 10);
    if (global == 0) { // local
        strcpy(server_path, argv[3]);
    } else if (global == 1) {
        if (argc < 6) {
            printf("no enough args\n");
            exit(-1);
        }
        strcpy(server_ip, argv[3]);
        port = (int) strtol(argv[4], NULL, 10);
        client_port = (int) strtol(argv[5],NULL,10);

    } else {
        printf("bad arg\n");
        exit(-2);
    }
    if (global == 1) {
        client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    } else {
        client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    }
    if (client_socket == -1) {
        perror(NULL);
        exit(-5);
    }

    sprintf(client_path, "%s/st.socket", client_id);

    memset(&server_unix_address, 0, sizeof(server_unix_address));
    memset(&client_unix_address, 0, sizeof(client_unix_address));
    memset(&server_inet_address, 0, sizeof(server_inet_address));
    memset(&client_inet_address, 0, sizeof(client_inet_address));

    if (global == 0) {
        client_unix_address.sun_family = AF_UNIX;
        strcpy(client_unix_address.sun_path, client_path);
        client_address = (struct sockaddr *) &client_unix_address;

        server_unix_address.sun_family = AF_UNIX;
        strcpy(server_unix_address.sun_path, server_path);
        server_address = (struct sockaddr *) &server_unix_address;
    } else {
        client_inet_address.sin_family = AF_INET;
        client_inet_address.sin_addr.s_addr = htonl(INADDR_ANY);
        client_inet_address.sin_port = htons(client_port);
        client_address = (struct sockaddr *) &client_inet_address;

        server_inet_address.sin_family = AF_INET;
        inet_pton(AF_INET, server_ip, &server_inet_address.sin_addr.s_addr);
        server_inet_address.sin_port = htons((uint16_t) port);
        server_address = (struct sockaddr *) &server_inet_address;
    }

    if (pthread_create(&thread, NULL, thread_fun, NULL) != 0) {
        perror(NULL);
        exit(-6);
    }

    if (global == 1) {
        address_size = sizeof(struct sockaddr_in);
    } else {
        address_size = sizeof(struct sockaddr_un);
    }

    if (bind(client_socket, client_address, address_size) == -1) {
        perror(NULL);
        exit(-5);
    }

    request request1;

    while (1) {
        if (recvfrom(client_socket, (void *) &request1, sizeof(request1), 0, server_address, &address_size) == -1) {
            perror(NULL);
        } else {
            printf("client %s : %s\n", request1.sender, request1.mess);
        }
        sleep(1);
    }
}

void *thread_fun(void *arg) {
    while (1) {
        char message_text[MAX_CLIENT_NAME_LENGTH], *result;
        result = fgets(message_text, MAX_CLIENT_NAME_LENGTH - 1, stdin);
        if (result != NULL) {
            if (strlen(message_text) >= MAX_MESSAGE_LENGTH) {
                printf("%s\n", "too long input");
            } else {
                request request1;
                strcpy(request1.sender, client_id);
                strcpy(request1.mess, message_text);
                if (sendto(client_socket, (void *) &request1, sizeof(request1), 0, server_address, address_size) ==
                    -1) {
                    perror(NULL);
                    exit(-11);
                }
            }
        }
    }
}