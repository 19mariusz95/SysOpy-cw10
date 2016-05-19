#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("no enough args\n");
        exit(-1);
    }
    long port = strtol(argv[1], NULL, 10);
    char *path = argv[2];
    return 0;
}

