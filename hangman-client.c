#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "../include/game.h"
#include "./game.c"

#define MAX_BUF 2048

static ssize_t read_line(int fd, char *buf, size_t max) {
    size_t i = 0;
    while (i < max - 1) {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r <= 0) return (r == 0 && i > 0) ? (ssize_t)i : -1;
        if (c == '\n') break;
        if (c != '\r') buf[i++] = c;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

/* Returns: 1 = need guess, 0 = word solved (wait for end), -1 = connection closed */
static int read_state(int fd) {
    char line[MAX_BUF];

    /* Read "Word: ..." */
    ssize_t n = read_line(fd, line, sizeof(line));
    if (n < 0) return -1;
    printf("%s\n", line);
    fflush(stdout);

    bool solved = (strstr(line, "_") == NULL) && (strncmp(line, "Word:", 5) == 0);

    /* Read "Incorrect guesses: ..." */
    n = read_line(fd, line, sizeof(line));
    if (n < 0) return -1;
    printf("%s\n", line);
    fflush(stdout);

    return solved ? 0 : 1;
}

/* Read end-of-game block: result line + 2 stats lines */
static void read_end(int fd) {
    char line[MAX_BUF];
    /* result line (YOU WIN! :) / You Lose! :( / Tie :/) */
    ssize_t n = read_line(fd, line, sizeof(line));
    if (n < 0) return;
    printf("%s\n", line);
    fflush(stdout);
    /* Your incorrect guesses */
    n = read_line(fd, line, sizeof(line));
    if (n < 0) return;
    printf("%s\n", line);
    fflush(stdout);
    /* Opponent's incorrect guesses */
    n = read_line(fd, line, sizeof(line));
    if (n < 0) return;
    printf("%s\n", line);
    fflush(stdout);
}

int main(int argc, char const* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host> <port> <word>\n", argv[0]);
        return 1;
    }

    for (int i = 0; i < (int)strlen(argv[2]); i++) {
        if (!isdigit((unsigned char)argv[2][i])) { fprintf(stderr, "Port argument must be a number.\n"); return 1; }
    }

    for (int i = 0; i < (int)strlen(argv[1]); i++) {
        if (!isdigit((unsigned char)argv[1][i]) && argv[1][i] != '.') {
            fprintf(stderr, "Address argument must be a valid IP.\n"); return 1;
        }
    }

    size_t wlen = strlen(argv[3]);
    char word[wlen + 1];
    for (size_t i = 0; i < wlen; i++) word[i] = normalize(argv[3][i]);
    word[wlen] = '\0';

    for (size_t i = 0; i < wlen; i++) {
        if (!is_letter(word[i])) { fprintf(stderr, "Word must contain only letters.\n"); return 1; }
    }

    int port = (int)strtol(argv[2], NULL, 10);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0) { perror("socket failed"); return 1; }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if (connect(clientfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed"); return 1;
    }

    /* Send our word */
    write(clientfd, word, wlen);

    /* Game loop */
    while (true) {
        int status = read_state(clientfd);
        if (status < 0) break;  /* connection closed */

        if (status == 0) {
            /* Word solved — wait for end-of-game message */
            read_end(clientfd);
            break;
        }

        /* Need to guess — read from stdin */
        char input[256];
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        char guess = normalize(input[0]);
        if (!is_letter(guess)) continue;

        char to_send[2] = {guess, '\n'};
        write(clientfd, to_send, 2);
    }

    close(clientfd);
    return 0;
}
