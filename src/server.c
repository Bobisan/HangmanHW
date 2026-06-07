#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/game.h"
#include "./game.c"

int main(int argc, char const* argv[])
{
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	

	if (serverfd < 0)
	{
		perror("socket failed");
		return -1;
	}

	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(510),
	};

	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

	if (bind(serverfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
		perror("address binding failed");
		exit(EXIT_FAILURE);
	};
	listen(serverfd, 2);
	printf("\nlistening ended");

	while (true)
	{
		printf("\nI got here");
		int clientfd = accept(serverfd, NULL, NULL);
		pid_t child_pid = fork();
		if (child_pid == 0)
		{
			printf("\nHello I am %d", getpid());
			char buffer[1024];
			ssize_t size;
			while ((size = read(clientfd, buffer, sizeof(buffer))) > 0)
				write(clientfd, buffer, size);
			close(clientfd);
		}
	}

	close(serverfd);
	return 0;
}
