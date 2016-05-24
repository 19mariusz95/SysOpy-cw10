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
int socket1;
struct sockaddr *server_address;

struct sockaddr_un server_unix_address;
struct sockaddr_in server_inet_address;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
request to_send;

char client_path[MAX_PATH_LENGTH];
char server_path[MAX_PATH_LENGTH];

pthread_t thread;
pthread_t main_thread;

int wait_send = 0;

void *thread_fun(void *arg);

void cleanup() {
    close(socket1);
    remove(client_path);
    exit(0);
}

void sig_handler(int sig) {
    if(sig==SIGUSR1){
        pthread_mutex_lock(&mutex);
        if (send(socket1, (void *) &to_send, sizeof(to_send), 0) == -1) {
            perror(NULL);
            exit(-11);
        }
        wait_send = 0;
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
    main_thread = pthread_self();
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
        strcpy(server_ip, argv[3]);
        port = (int) strtol(argv[4], NULL, 10);

    } else {
        printf("bad arg\n");
        exit(-2);
    }

    if (global == 1) {
        socket1 = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        socket1 = socket(AF_UNIX, SOCK_STREAM, 0);
    }
    if (socket1 == -1) {
        perror(NULL);
        exit(-5);
    }

    sprintf(client_path, "%s/st.socket", client_id);

    memset(&server_unix_address, 0, sizeof(server_unix_address));
    memset(&server_inet_address, 0, sizeof(server_inet_address));

    if (global == 0) {
        server_unix_address.sun_family = AF_UNIX;
        strcpy(server_unix_address.sun_path, server_path);
        server_address = (struct sockaddr *) &server_unix_address;
    } else {
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

    if (connect(socket1, server_address, address_size) == -1) {
        perror(NULL);
        exit(-5);
    }
    strcpy(to_send.sender,client_id);
    if (send(socket1, (void *) &to_send, sizeof(to_send), 0) == -1) {
        perror(NULL);
        exit(-6);
    }

    request request1;
    fd_set sockset;
    fd_set sockset1;
    FD_ZERO(&sockset);
    FD_SET(socket1, &sockset);
    while (1) {
        sockset1 = sockset;
        if (select(socket1 + 1, &sockset1, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(socket1, &sockset1)) {
                ssize_t tmp = recv(socket1, (void *) &request1, sizeof(request1), MSG_DONTWAIT);
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
        while(wait_send)
            pthread_cond_wait(&cond,&mutex);
        char message_text[MAX_CLIENT_NAME_LENGTH], *result;
        result = fgets(message_text, MAX_CLIENT_NAME_LENGTH - 1, stdin);
        if (result != NULL) {
            if (strlen(message_text) >= MAX_MESSAGE_LENGTH) {
                printf("%s\n", "too long input");
            } else {
                strcpy(to_send.sender, client_id);
                strcpy(to_send.mess, message_text);
                wait_send = 1;
            }
        }
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        pthread_kill(main_thread,SIGUSR1);
    }
}