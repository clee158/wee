#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

typedef struct thread_node {
	pthread_t thread_id;
	int client_fd;
	struct thread_node *next;
} thread_node;

static int running;
thread_node *head;
int sock_fd;

void *client_interaction(void *client_fd) {
	int client = *((int *) client_fd);
	size_t len;
	char buffer[1000];

	while (1) {
		if ((len = read(client, buffer, 1000 - 1)) == -1) {
			perror("recv");
			exit(1);
		}
		else if (len == 0) {
			// Join back with the original thread
			pthread_exit(NULL);
			printf("Connection closed\n");
			break;
		}

	  buffer[len] = '\0';
		printf("Server:Msg Received %s\n", buffer);
	  printf("===\n");

		int n;

		for (n = 0; n < strlen(buffer); n++)
			buffer[n] = buffer[n] + 1;

		if ((send(client, buffer, strlen(buffer), 0)) == -1) {
			// Join back with the original thread
			pthread_exit(NULL);
			fprintf(stderr, "Failure Sending Message\n");
			close(client);
			break;
		}

		printf("Msg sent: %s\n", buffer);
	}
}

void sigint_handler() {
	printf("Cleaning up before exiting...\n");
	running = 0;
	thread_node *curr = head;
	thread_node *next = NULL;

	while (curr != NULL) {
		next = curr->next;

		if (curr->client_fd != -1)
			close(curr->client_fd);

		free(curr);
		curr = next;
	}

	close(sock_fd);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	running = 1;
	signal(SIGINT, sigint_handler);
	head = malloc(sizeof(thread_node));
	head->next = NULL;
	head->thread_id = pthread_self();
	head->client_fd = -1;

	int s;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
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
	thread_node *curr_node = head;

	while (running) {
		if ((client_fd = accept(sock_fd, NULL, NULL)) == -1) {
			perror("accept_error");
			exit(1);
		} else {
			pthread_t new_user;
			
			if (pthread_create(&new_user, NULL, client_interaction, &client_fd)) {
				printf("Connection failed with client_fd=%d\n", client_fd);
			} else {
				thread_node *new_node = malloc(sizeof(thread_node));
				new_node->next = NULL;
				new_node->thread_id = new_user;
				new_node->client_fd = client_fd;
				curr_node->next = new_node;

				printf("Connection made: client_fd=%d\n", client_fd);
			}
		}
	}

	if (head != NULL)
		sigint_handler();
	
  return 0;
}
