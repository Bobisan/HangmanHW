#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "../include/game.h"
#include "./game.c"

int main(int argc, char const* argv[])
{
	
	if (argc <= 1){
		perror("\nYou need to specify a port as an argument.\n");
		return 1;
	}
	
	for(int i=0; i < strlen(argv[1]); i++){
		if (isdigit(argv[1][i]) < 1){
			perror("\nPort argument must be a number.\n");
			return 1;
		}
	}

	int port = strtol(argv[1], NULL, 10);
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);

	if (serverfd < 0)
	{
		perror("\nSocket creation failed");
		return 1;
	}

	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port),
	};

	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

	if (bind(serverfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
		perror("address binding failed");
		exit(EXIT_FAILURE);
	}

	if(listen(serverfd, 2) < 0){
		perror("\nListening failed");
		return 1;
	}

	printf("Listening on %d...\n", port);
	int fork_count = 0;
	while (true)
	{
		if(fork_count < 2)
		{
			int clientfd = accept(serverfd, NULL, NULL);
			pid_t child_pid = fork();
			fork_count++;
			if (child_pid == 0)
			{
				while (true){

				}
				/*printf("\nHello I am %d", getpid());
				char buffer[1024];
				ssize_t size;
				while ((size = read(clientfd, buffer, sizeof(buffer))) > 0)
					write(clientfd, buffer, size);*/
				close(clientfd);
			}
			printf("Fork count is %d\n", fork_count);
		}
		else
		{
			break;
		}
	}
	printf("Client limit reached\n");
	close(serverfd);
	return 0;
}
