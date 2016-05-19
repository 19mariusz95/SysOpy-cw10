#ifndef ZAD1_TRZUSTKA_H
#define ZAD1_TRZUSTKA_H

#define MAX_CLIENTS 100
#define MAX_MESSAGE_LENGTH 256
#define MAX_CLIENT_NAME_LENGTH 32
#define MAX_RESPONSE_TIME 5

typedef enum {
    NO_MESSAGE,
    UNIX,
    INET
} message_type;

typedef enum {
    INACTIVE = 0,
    LOCAL,
    GLOBAL
} client_state;

typedef enum {
    REGISTER,
    MESSAGE,
    END
} request;

typedef struct {
    char sender[MAX_CLIENT_NAME_LENGTH];
    request req;
    int client_num;
} message;

typedef struct {
    char name[MAX_CLIENT_NAME_LENGTH];
    client_state state;
    union {
        struct sockaddr_un unix_address;
        struct sockaddr_in inet_address;
    };
} client;

#endif //ZAD1_TRZUSTKA_H
