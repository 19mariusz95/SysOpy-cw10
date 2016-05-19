#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc < 4) {
        printf("no enough args\n");
        exit(-1);
    }
    char *client_id = argv[1];
    long type = strtol(argv[2], NULL, 10);
    if (type == 0) { // local
        char *path = argv[3];
    } else if (type == 1) {
        if (argc < 5) {
            printf("no enough args\n");
            exit(-1);
        }
        char *ip = argv[3];
        long port = strtol(argv[4], NULL, 10);
        printf("%s %ld %s %ld\n", client_id, type, ip, port);

    } else {
        printf("bad arg\n");
        exit(-2);
    }
    return 0;
}