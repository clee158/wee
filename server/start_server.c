#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
	int s;
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  s = getaddrinfo(NULL, "5555", &hints, &result);

  if (s != 0) {
	  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(1);
	}

	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

  if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
	 perror("bind()");
   exit(1);
  }

	if (listen(sock_fd, 10) != 0) {
		perror("listen()");
	  exit(1);
	}
	
	struct sockaddr_in *result_addr = (struct sockaddr_in *) result->ai_addr;
	printf("Listening on file descriptor %d, port %d\n", sock_fd, ntohs(result_addr->sin_port));
	
	int client_fd;
	char buffer[1000];
	size_t len;

	while (1) {

		if ((client_fd = accept(sock_fd, NULL, NULL)) == -1) {
			perror("accept");
			exit(1);
		}
	  
		printf("Connection made: client_fd=%d\n", client_fd);

		while (1) {
			if ((len = read(client_fd, buffer, 1000 - 1)) == -1) {
				perror("recv");
				exit(1);
			}
			else if (len == 0) {
				printf("Connection closed\n");
				break;
			}
		  buffer[len] = '\0';
			printf("Server:Msg Received %s\n", buffer);
		  printf("===\n");

			int n;

			for (n = 0; n < strlen(buffer); n++)
				buffer[n] = buffer[n] + 1;

			if ((send(client_fd, buffer, strlen(buffer), 0)) == -1) {
				fprintf(stderr, "Failure Sending Message\n");
				close(client_fd);
				break;
			}

			printf("Msg sent: %s\n", buffer);
		}
	}

	close(client_fd);
	close(sock_fd);
  return 0;
}
