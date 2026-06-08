#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "../include/game.h"
#include "./game.c"

int main(){
    char c = 'c';
    printf("\n%d\n", is_letter(c));
    return 0;
}