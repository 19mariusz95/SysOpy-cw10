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

socklen_t jakas_tam_zmienna;
char server_aj_pi[15];
int ppppport;
int so_so_so_socket;
struct sockaddr *osp_server_address;

struct sockaddr_un alamakota_server_unix_address;
struct sockaddr_in alaniemakota_server_inet_address;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
request to_send;

char client_path[MAX_PATH_LENGTH];
char server_path[MAX_PATH_LENGTH];

pthread_t wateczek;
pthread_t mamusia;

int czekaj_no = 0;

void *thread_fun(void *arg);

void cleanup() {
    close(so_so_so_socket);
    remove(client_path);
    exit(0);
}

void sig_handler(int sig) {
    if(sig==SIGUSR1){
        pthread_mutex_lock(&mutex);
        if (send(so_so_so_socket, (void *) &to_send, sizeof(to_send), 0) == -1) {
            perror(NULL);
            exit(-11);
        }
        czekaj_no = 0;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }else
        exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("no enough args\n");
        exit(-1);
    }
    mamusia = pthread_self();
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
        strcpy(server_path, argv[3]);
    } else if (global == 1) {
        if (argc < 5) {
            printf("no enough args\n");
            exit(-1);
        }
        strcpy(server_aj_pi, argv[3]);
        ppppport = (int) strtol(argv[4], NULL, 10);

    } else {
        printf("bad arg\n");
        exit(-2);
    }

    if (global == 1) {
        so_so_so_socket = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        so_so_so_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    }
    if (so_so_so_socket == -1) {
        perror(NULL);
        exit(-5);
    }

    sprintf(client_path, "%s/st.socket", client_id);

    memset(&alamakota_server_unix_address, 0, sizeof(alamakota_server_unix_address));
    memset(&alaniemakota_server_inet_address, 0, sizeof(alaniemakota_server_inet_address));

    if (global == 0) {
        alamakota_server_unix_address.sun_family = AF_UNIX;
        strcpy(alamakota_server_unix_address.sun_path, server_path);
        osp_server_address = (struct sockaddr *) &alamakota_server_unix_address;
    } else {
        alaniemakota_server_inet_address.sin_family = AF_INET;
        inet_pton(AF_INET, server_aj_pi, &alaniemakota_server_inet_address.sin_addr.s_addr);
        alaniemakota_server_inet_address.sin_port = htons((uint16_t) ppppport);
        osp_server_address = (struct sockaddr *) &alaniemakota_server_inet_address;
    }

    if (pthread_create(&wateczek, NULL, thread_fun, NULL) != 0) {
        perror(NULL);
        exit(-6);
    }

    if (global == 1) {
        jakas_tam_zmienna = sizeof(struct sockaddr_in);
    } else {
        jakas_tam_zmienna = sizeof(struct sockaddr_un);
    }

    if (connect(so_so_so_socket, osp_server_address, jakas_tam_zmienna) == -1) {
        perror(NULL);
        exit(-5);
    }
    strcpy(to_send.sender,client_id);
    if (send(so_so_so_socket, (void *) &to_send, sizeof(to_send), 0) == -1) {
        perror(NULL);
        exit(-6);
    }

    request request1;
    fd_set sockset;
    fd_set sockset1;
    FD_ZERO(&sockset);
    FD_SET(so_so_so_socket, &sockset);
    while (1) {
        sockset1 = sockset;
        if (select(so_so_so_socket + 1, &sockset1, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(so_so_so_socket, &sockset1)) {
                ssize_t tmp = recv(so_so_so_socket, (void *) &request1, sizeof(request1), MSG_DONTWAIT);
                if (tmp == -1) {
                    perror(NULL);
                } else if(tmp>0) {
                    printf("client %s : %s\n", request1.sender, request1.mess);
                } else {
                    printf("disconnectd\n");
                    exit(1);
                }
            }
        }
    }
}

void *thread_fun(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (czekaj_no)
            pthread_cond_wait(&cond,&mutex);
        char message_text[MAX_CLIENT_NAME_LENGTH], *result;
        result = fgets(message_text, MAX_CLIENT_NAME_LENGTH - 1, stdin);
        if (result != NULL) {
            if (strlen(message_text) >= MAX_MESSAGE_LENGTH) {
                printf("%s\n", "too long input");
            } else {
                strcpy(to_send.sender, client_id);
                strcpy(to_send.mess, message_text);
                czekaj_no = 1;
            }
        }
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        pthread_kill(mamusia, SIGUSR1);
    }
}