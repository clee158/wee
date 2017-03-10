#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include ".base64.h"

char *get_port();
char *get_ip();

int main(int argc, char **argv) {
	// SOCKET Initialization

	char *ip_addr = get_ip();
	char *ip_port = get_port();
  int s;
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

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

	char buffer[1000];
	size_t num;

	while (1) {
		printf("Data for Server:\n");
   	fgets(buffer, 1000, stdin);

    if ((send(sock_fd, buffer, strlen(buffer), 0))== -1) {
	    fprintf(stderr, "Failure Sending Message\n");
 	    close(sock_fd);
      exit(1);
 	  }
    else {
 			num = recv(sock_fd, buffer, sizeof(buffer), 0);
      
			if (num <= 0) {
      	printf("Either Connection Closed or Error\n");
       	//Break from the While
       	break;
      }

		  buffer[num] = '\0';
      printf("Client:Message Received From Server -  %s\n",buffer);
    }

/*
		fgets(input_buffer, sizeof(input_buffer), stdin);
		printf("===\n");

		char input[strlen(input_buffer) + 1];
		strcpy(input, input_buffer);
		input[strlen(input_buffer)] = '\n'; 

		write(sock_fd, input, strlen(input));
		char resp[1000];
		int len = read(sock_fd, resp, 999);
		resp[len] = '\0';
		printf("%s\n", resp);
		fflush(stdin);
		//fflush(sock_fd);
*/
	}
	
	free(ip_addr);
	free(ip_port);
	close(sock_fd);
  return 0;
}
