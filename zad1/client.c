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
int krowa_port;
int kura_socket;
struct sockaddr *jezioro_labedzie_address;

struct sockaddr_un jelita_server_unix_address;
struct sockaddr_in bartek_server_inet_address;
pthread_mutex_t chory_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t komar_condition_variable = PTHREAD_COND_INITIALIZER;
request jedzenie_request;

char stodola_server_path[MAX_PATH_LENGTH];
char garaz_client_path[MAX_PATH_LENGTH];

pthread_t wladek;
pthread_t matka;

int dzwon_koscielny = 0;

void *thread_fun(void *arg);

void cleanup() {
    close(kura_socket);
    pthread_mutex_destroy(&chory_mutex);
    pthread_cond_destroy(&komar_condition_variable);
    exit(0);
}

void sig_handler(int sig) {
    if (sig == SIGUSR1) {
        pthread_mutex_lock(&chory_mutex);
        if (sendto(kura_socket, (void *) &jedzenie_request, sizeof(jedzenie_request), 0, jezioro_labedzie_address, address_size) ==
            -1) {
            perror(NULL);
            exit(-11);
        }
        dzwon_koscielny = 0;
        pthread_cond_signal(&komar_condition_variable);
        pthread_mutex_unlock(&chory_mutex);
    } else
        exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("no enough args\n");
        exit(-1);
    }
    matka = pthread_self();
    atexit(cleanup);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    client_id = argv[1];
    if (strlen(client_id) >= MAX_CLIENT_NAME_LENGTH) {
        printf("too long id\n");
        exit(-3);
    }
    long global = strtol(argv[2], NULL, 10);
    if (global == 0) { // local
        strcpy(stodola_server_path, argv[3]);
        sprintf(garaz_client_path, "st.socket%s", client_id);
    } else if (global == 1) {
        if (argc < 5) {
            printf("no enough args\n");
            exit(-1);
        }
        strcpy(server_ip, argv[3]);
        krowa_port = (int) strtol(argv[4], NULL, 10);

    } else {
        printf("bad arg\n");
        exit(-2);
    }
    if (global == 1) {
        kura_socket = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        kura_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    }
    if (kura_socket == -1) {
        perror(NULL);
        exit(-5);
    }

    memset(&jelita_server_unix_address, 0, sizeof(jelita_server_unix_address));
    memset(&bartek_server_inet_address, 0, sizeof(bartek_server_inet_address));

    if (global == 0) {
        jelita_server_unix_address.sun_family = AF_UNIX;
        strcpy(jelita_server_unix_address.sun_path, garaz_client_path);
        jezioro_labedzie_address = (struct sockaddr *) &jelita_server_unix_address;
        if (bind(kura_socket, (struct sockaddr *) &jelita_server_unix_address, sizeof(jelita_server_unix_address)) == -1) {
            perror(NULL);
            exit(1);
        }
        strcpy(jelita_server_unix_address.sun_path, stodola_server_path);
    } else {
        bartek_server_inet_address.sin_family = AF_INET;
        inet_pton(AF_INET, server_ip, &bartek_server_inet_address.sin_addr.s_addr);
        bartek_server_inet_address.sin_port = htons((uint16_t) krowa_port);
        jezioro_labedzie_address = (struct sockaddr *) &bartek_server_inet_address;
    }

    if (pthread_create(&wladek, NULL, thread_fun, NULL) != 0) {
        perror(NULL);
        exit(-6);
    }

    if (global == 1) {
        address_size = sizeof(struct sockaddr_in);
    } else {
        address_size = sizeof(struct sockaddr_un);
    }

    request request1;
    while (1) {
        ssize_t tmp = recvfrom(kura_socket, (void *) &request1, sizeof(request1), 0, jezioro_labedzie_address, &address_size);
        if (tmp == -1) {
            perror(NULL);
        } else if (tmp > 0) {
            printf("client %s : %s\n", request1.sender, request1.mess);
        } else {
            printf("disconnected\n");
            exit(1);
        }
    }
}

void *thread_fun(void *arg) {
    while (1) {
        pthread_mutex_lock(&chory_mutex);
        while (dzwon_koscielny) {
            pthread_cond_wait(&komar_condition_variable, &chory_mutex);
        }
        char message_text[MAX_CLIENT_NAME_LENGTH], *result;
        result = fgets(message_text, MAX_CLIENT_NAME_LENGTH - 1, stdin);
        if (result != NULL) {
            if (strlen(message_text) >= MAX_MESSAGE_LENGTH) {
                printf("%s\n", "too long input");
            } else {
                strcpy(jedzenie_request.sender, client_id);
                strcpy(jedzenie_request.mess, message_text);
                dzwon_koscielny = 1;
            }
        }
        pthread_cond_signal(&komar_condition_variable);
        pthread_mutex_unlock(&chory_mutex);
        pthread_kill(matka, SIGUSR1);
    }
}