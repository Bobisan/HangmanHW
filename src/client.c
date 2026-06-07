#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "../include/game.h"
#include "./game.c"

int main()
{
    int status;
    char* message = "This is my message";
    char buffer[1024] = {0};
    struct sockaddr_in serv_addr;
    ssize_t size = sizeof(buffer);
	int clientfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (clientfd < 0)
	{
		perror("socket failed");
		return -1;
	}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(510);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);


    if ((status = connect(clientfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
        int errsv = errno;
        printf ("Errno: %d", errsv);
        return -1;
    }
    

    write(clientfd, message, strlen(message));
    printf("\nMessage sent");

    size_t valread = read(clientfd, buffer, size);

    printf("\n%s\n", buffer);

    close (clientfd);

    

	/*struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(510),
	};

	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

	bind(clientfd, (struct sockaddr *) &server_addr, sizeof(server_addr));*/


	/*while (true)
	{
		int clientfd = accept(clientfd, NULL, NULL);
		pid_t child_pid = fork();
		if (child_pid == 0)
		{
			printf("Hello I am %d", getpid());
			char buffer[1024];
			ssize_t size;
			while ((size = read(clientfd, buffer, sizeof(buffer))) > 0)
				write(clientfd, buffer, size);
			close(clientfd);
		}
	}*/


	return 0;
}
