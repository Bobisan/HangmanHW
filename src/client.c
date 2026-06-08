#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "../include/game.h"
#include "./game.c"

int main(int argc, char const* argv[])
{

    if (argc <= 1){
		perror("\nYou need to specify a host ip address, a port and an opponent word as an argument.\n");
		return 1;
	}
	
	for(int i=0; i < strlen(argv[2]); i++){
		if (isdigit(argv[2][i]) < 1){
			perror("Port argument must be a number.\n");
			return 1;
		}
	}

    for(int i=0; i < strlen(argv[1]); i++){
		if ((isdigit(argv[1][i]) < 1) && (argv[1][i] != '.')){
			perror("Address argument must be a number.\n");
			return 1;
		}
	}


    char word[strlen(argv[3])+1];

    for(int i=0; i < strlen(argv[3]); i++){
		word[i] = normalize(argv[3][i]);
	}

    for(int i=0; i < strlen(word); i++){
        if (is_letter(word[i]) < 1){
            perror("Word must contain only letters.\n");
            return 1;
        }
	}

    int port = strtol(argv[2], NULL, 10);
    char* address = argv[1];
    char buffer[1024] = {0};
    struct sockaddr_in serv_addr;
    ssize_t size = sizeof(buffer);
	int clientfd = socket(AF_INET, SOCK_STREAM, 0);

	if (clientfd < 0){
		perror("socket failed");
		return 1;
	}
    
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &serv_addr.sin_addr);


    if (connect(clientfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("\nConnection Failed");
        return 1;
    }
    

    write(clientfd, word, strlen(word));
    printf("\nword sent");

    size_t valread = read(clientfd, buffer, size);

    printf("\n%s\n", buffer);

    close (clientfd);
	return 0;
}
