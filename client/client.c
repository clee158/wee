#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include ".base64.h"

pthread_t input;
pthread_t output;
//pthread_mutex_t mutex;
size_t num;
char buffer[1000];
char text[1048576]; // hold 2^20
int sock_fd;
char *get_port();
char *get_ip();
char *ip_addr;
char *ip_port;
void *accept_user_inputs(void *arg) {
	while(1) {
		printf("Data for Server:\n");
		fgets(buffer, 1000, stdin);

		if ((send(sock_fd, buffer, strlen(buffer), 0)) == -1) {
			fprintf(stderr, "Failure Sending Message\n");
			free(ip_addr);
			free(ip_port);
			close(sock_fd);
			exit(0);
		}
	}
	return NULL;
}

void *print_server_outputs(void *arg) {
	while(1) {
		num = recv(sock_fd, text, sizeof(text), 0);
      
		if (num <= 0) {
			printf("Either Connection Closed or Error\n");
			free(ip_addr);
			free(ip_port);
			close(sock_fd);
			exit(0);
		}

		text[num] = '\0';
		printf("Client:Message Received From Server -  %s\n", text);
	}
	return NULL;
}

int main(int argc, char **argv) {
	// SOCKET Initialization
	ip_addr = get_ip();
	ip_port = get_port();
  int s;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; /* IPv4 only */
  hints.ai_socktype = SOCK_STREAM; /* TCP */

	// Connecting to the wee server
  s = getaddrinfo(ip_addr, ip_port, &hints, &result);

  if (s != 0) {
 		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(1);
  }

  if(connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
    perror("connect");
    exit(2);
  }

	text[0] = '\0';

	pthread_create(&input, NULL, accept_user_inputs, NULL);
	pthread_create(&output, NULL, print_server_outputs, NULL);
	pthread_join(input, NULL);
	pthread_join(output, NULL);

	free(ip_addr);
	free(ip_port);
	close(sock_fd);
  return 0;
}
