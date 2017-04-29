#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include ".base64.h"

pthread_t input;
pthread_t output;
char buff_out[32];
char buff_in[2048];
int sock_fd;
char *get_port();
char *get_ip();
char *ip_addr;
char *ip_port;

void *accept_user_inputs(void *arg) {
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_sent = 0;

	while (1) {
		r_len = read(0, buff_out, sizeof(buff_out));

		if (r_len == -1 && errno != EINTR) {
			perror("read");
			break;
		}

		while (tot_sent < r_len) {
			s_len = send(sock_fd, buff_out, r_len, 0);

			if (s_len == -1 && errno != EINTR) {
				perror("send");
				return NULL;
			}

			tot_sent += s_len;
		}
	}

	return NULL;
}

void *print_server_outputs(void *text_id) {
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_sent = 0;

	while(1) {
		r_len = read(sock_fd, buff_in, sizeof(buff_int));

		if (r_len == -1 && errno != EINTR) {
			perror("read");
			break;
		}

		while (tot_sent < r_len) {
			s_len = send(sock_fd, buff_out, r_len, 0);

			if (s_len == -1 && errno != EINTR) {
				perror("send");
				return NULL;
			}

			tot_sent += s_len;
		}
	}

	return NULL;
}

int (int argc, char **argv) {
	// SOCKET Initialization
	ip_addr = get_ip();
	ip_port = get_port();
	printf("server: %s:%s\n", ip_addr, ip_port);

  int s;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1) {
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval, sizeof(optval));
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; /* IPv4 only */
  hints.ai_socktype = SOCK_STREAM; /* TCP */

	// Connecting to the wee server
  s = getaddrinfo(ip_addr, ip_port, &hints, &result);

  if (s != 0) {
 		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  if(connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
    perror("connect()");
    exit(EXIT_FAILURE);
  }

	buffer[0] = '\0';

	pthread_create(&input, NULL, accept_user_inputs, NULL);
	pthread_create(&output, NULL, print_server_outputs, "00011" /*TODO:textid*/);
	pthread_join(input, NULL);
	pthread_join(output, NULL);

	free(ip_addr);
	free(ip_port);
	close(sock_fd);
  return 0;
}
