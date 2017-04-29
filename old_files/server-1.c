#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
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
int start_index;
int sock_fd;
FILE *editor_fp;
struct addrinfo hints, *result;
pthread_mutex_t mutex;
char text[1048576]; // hold 2^20

void sync_send() {
	thread_node *curr = head->next;
	thread_node *next = NULL;

	while (curr != NULL) {
		next = curr->next;

		printf("sending message to %lu\n", curr->thread_id);
		printf("next is NULL: %d\n", next == NULL);
		if ((send(curr->client_fd, text, strlen(text), 0)) == -1) {
			printf("Failed to send result to %d\n", curr->client_fd);
		}

		curr = next;
	}
}

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
			printf("%d: Connection closed\n", client);
			pthread_exit(NULL);
			break;
		} 

	  buffer[len] = '\0';

		// CRITICAL SECTION STARTED  --------------------------------------
		// : editing current temp.txt and applying the change to text
		pthread_mutex_lock(&mutex);
		fprintf(editor_fp, "%s", buffer);
		
		// read texts in the temp.txt
		int curr = start_index;
		for (; curr < start_index + strlen(buffer); curr++)	
			text[curr] = buffer[curr - start_index];

		start_index = strlen(text);

		sync_send();

		pthread_mutex_unlock(&mutex);
		// CRITICAL SECTION FINISHED --------------------------------------

		printf("%d -  Msg sent: %s\n", client, text);
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

	free(result);
	close(sock_fd);
	fclose(editor_fp);
	pthread_mutex_destroy(&mutex);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	start_index = 0;
	text[0] = '\0';
	pthread_mutex_init(&mutex, NULL);
	editor_fp = fopen("temp.txt", "a+");
	running = 1;
	signal(SIGINT, sigint_handler);
	head = malloc(sizeof(thread_node));
	head->next = NULL;
	head->thread_id = pthread_self();
	head->client_fd = -1;

	int s;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
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
				curr_node = new_node;
				printf("Connection made: client_fd=%d, thread:%lu\n", client_fd, new_user);
			}
		}
	}

	if (head != NULL)
		sigint_handler();
	
  return 0;
}
