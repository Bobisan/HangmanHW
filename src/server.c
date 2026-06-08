#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "../include/game.h"
#include "./game.c"

#define MAX_WORD 256
#define MAX_BUF 2048

/* Send current masked state to client fd */
static void send_state(int fd, const secret_word_t *word) {
    char msg[MAX_BUF];
    int pos = 0;

    char masked[MAX_WORD];
    for (size_t i = 0; i < word->word_length; i++) {
        char c;
        if (secret_word_letter_at(word, i, &c) == SECRET_WORD_LETTER_REVEALED)
            masked[i] = c;
        else
            masked[i] = '_';
    }
    masked[word->word_length] = '\0';

    pos += snprintf(msg + pos, sizeof(msg) - pos, "Word: %s\n", masked);

    pos += snprintf(msg + pos, sizeof(msg) - pos, "Incorrect guesses:");
    bool first = true;
    for (char c = 'a'; c <= 'z'; c++) {
        if (letter_set_contains(word->incorrect_guesses, c)) {
            if (first) { pos += snprintf(msg + pos, sizeof(msg) - pos, " %c", c); first = false; }
            else        { pos += snprintf(msg + pos, sizeof(msg) - pos, ", %c", c); }
        }
    }
    pos += snprintf(msg + pos, sizeof(msg) - pos, "\n");

    write(fd, msg, pos);
}

static void build_incorrect_str(const secret_word_t *word, char *out, size_t out_sz) {
    int pos = 0;
    bool first = true;
    for (char c = 'a'; c <= 'z'; c++) {
        if (letter_set_contains(word->incorrect_guesses, c)) {
            if (first) { pos += snprintf(out + pos, out_sz - pos, "%c", c); first = false; }
            else        { pos += snprintf(out + pos, out_sz - pos, ", %c", c); }
        }
    }
    if (first) snprintf(out, out_sz, "");
}

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

static ssize_t read_word(int fd, char *buf, size_t max) {
    ssize_t total = 0;
    while ((size_t)total < max - 1) {
        char c;
        ssize_t r;
        if (total == 0)
            r = read(fd, &c, 1);
        else
            r = recv(fd, &c, 1, MSG_DONTWAIT);
        if (r <= 0) break;
        if (c == '\n' || c == '\r') { if (total > 0) break; continue; }
        buf[total++] = c;
    }
    buf[total] = '\0';
    return total;
}

static void play_game(int fd0, int fd1, secret_word_t game[2]) {

    send_state(fd0, &game[0]);
    send_state(fd1, &game[1]);

    bool done[2] = {false, false};
    fd_set readfds;
    int maxfd = (fd0 > fd1 ? fd0 : fd1) + 1;

    while (!done[0] || !done[1]) {
        FD_ZERO(&readfds);
        if (!done[0]) FD_SET(fd0, &readfds);
        if (!done[1]) FD_SET(fd1, &readfds);

        int ready = select(maxfd, &readfds, NULL, NULL, NULL);
        if (ready < 0) break;

        int fds[2] = {fd0, fd1};
        for (int i = 0; i < 2; i++) {
            if (done[i]) continue;
            if (!FD_ISSET(fds[i], &readfds)) continue;

            char line[64];
            ssize_t n = read_line(fds[i], line, sizeof(line));
            if (n < 0) { done[i] = true; continue; }
            if (n == 0) continue;

            char guess = normalize(line[0]);
            if (!is_letter(guess)) { send_state(fds[i], &game[i]); continue; }

            secret_word_guess(&game[i], guess);
            send_state(fds[i], &game[i]);

            if (secret_word_is_solved(&game[i])) done[i] = true;
        }
    }
}

int main(int argc, char const* argv[])
{
    if (argc < 2) { fprintf(stderr, "You need to specify a port as an argument.\n"); return 1; }

    for (int i = 0; i < (int)strlen(argv[1]); i++) {
        if (!isdigit((unsigned char)argv[1][i])) { fprintf(stderr, "Port argument must be a number.\n"); return 1; }
    }

    int port = (int)strtol(argv[1], NULL, 10);
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) { perror("Socket creation failed"); return 1; }

    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if (bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("address binding failed"); return 1;
    }
    if (listen(serverfd, 2) < 0) { perror("Listening failed"); return 1; }

    printf("Listening on %d...\n", port);
    fflush(stdout);

    int clientfd[2];
    for (int i = 0; i < 2; i++) {
        clientfd[i] = accept(serverfd, NULL, NULL);
        if (clientfd[i] < 0) { perror("accept failed"); return 1; }
    }
    close(serverfd);

    char word_buf[2][MAX_WORD];
    for (int i = 0; i < 2; i++) {
        ssize_t n = read_word(clientfd[i], word_buf[i], MAX_WORD);
        if (n <= 0) {
            const char *err = "ERROR Invalid or empty word\n";
            write(clientfd[i], err, strlen(err));
            close(clientfd[0]); close(clientfd[1]);
            return 1;
        }
        for (int j = 0; j < n; j++) word_buf[i][j] = normalize(word_buf[i][j]);
    }

    secret_word_t game[2];
    for (int i = 0; i < 2; i++) {
        if (!secret_word_init_from_c_string(&game[i], word_buf[1-i])) {
            const char *err = "ERROR Invalid word\n";
            write(clientfd[0], err, strlen(err));
            write(clientfd[1], err, strlen(err));
            close(clientfd[0]); close(clientfd[1]);
            return 1;
        }
    }

    play_game(clientfd[0], clientfd[1], game);

    size_t wrong[2];
    for (int i = 0; i < 2; i++) wrong[i] = secret_word_incorrect_guess_count(&game[i]);

    const char *outcome[2];
    if (wrong[0] < wrong[1])      { outcome[0] = "YOU WIN! :)"; outcome[1] = "You Lose! :("; }
    else if (wrong[1] < wrong[0]) { outcome[0] = "You Lose! :("; outcome[1] = "YOU WIN! :)"; }
    else                          { outcome[0] = "Tie :/"; outcome[1] = "Tie :/"; }

    for (int i = 0; i < 2; i++) {
        char inc_self[MAX_BUF/2], inc_opp[MAX_BUF/2];
        build_incorrect_str(&game[i],   inc_self, sizeof(inc_self));
        build_incorrect_str(&game[1-i], inc_opp,  sizeof(inc_opp));

        char end_msg[MAX_BUF];
        snprintf(end_msg, sizeof(end_msg),
            "%s\nYour incorrect guesses: %s\nOpponent's incorrect guesses: %s\n",
            outcome[i], inc_self, inc_opp);
        write(clientfd[i], end_msg, strlen(end_msg));
    }

    for (int i = 0; i < 2; i++) { secret_word_free(&game[i]); close(clientfd[i]); }
    return 0;
}
