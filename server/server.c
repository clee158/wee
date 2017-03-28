#include "server.h"

void sync_send(char *text_id) {
	text_group *target_group = head_group;

	while (target_group != NULL && strcmp(target_group->text_id, text_id) == 0)
		target_group = target_group->next;

	if (target_group == NULL)
		return;

	client_node *curr = head_group->head;
	client_node *next = NULL;

	while (curr != NULL) {
		next = curr->next;

		printf("sending message to %lu\n", curr->id);
		printf("next is NULL: %d\n", next == NULL);

		if ((send(curr->fd, head_group->text, strlen(head_group->text), 0)) == -1) {
			printf("Failed to send result to %d\n", curr->fd);
		}

		curr = next;
	}
}

//////////////////////////////////////////////////////////////////////////
void *client_interaction(void *client_n) {
	client_node *client = (client_node *)client_n;
	text_group *target_group = head_group;

	while (target_group != NULL && target_group->text_id != client->text_id)
		target_group = target_group->next;

	if (target_group == NULL)
		target_group = head_group;	// permanent change until we use text_id

	int client_fd = client->fd;
	size_t len;
	char buffer[1000];

	while (1) {
		if ((len = read(client_fd, buffer, 1000 - 1)) == -1) {
			perror("recv");
			exit(1);
		} 
		else if (len == 0) {
			// Join back with the original thread
			printf("%d: Connection closed\n", client_fd);
			pthread_exit(NULL);
			break;
		} 

	  buffer[len] = '\0';

		// CRITICAL SECTION STARTED  --------------------------------------
		// : editing current temp.txt and applying the change to text
		pthread_mutex_lock(&target_group->mutex);
		fprintf(target_group->editor_fp, "%s", buffer);
		
		int curr = target_group->start_index;
		for (; curr < target_group->start_index + strlen(buffer); curr++)	
			target_group->text[curr] = buffer[curr - target_group->start_index];

		target_group->start_index = strlen(target_group->text);

		sync_send(client->text_id);

		pthread_mutex_unlock(&target_group->mutex);
		// CRITICAL SECTION FINISHED --------------------------------------

		printf("%d -  Msg sent: %s\n", client_fd, target_group->text);
	}
}

void sigint_handler() {
	printf("Cleaning up before exiting...\n");

	running = 0;
	text_group *curr_group = head_group;
	text_group *next_group = NULL;
	client_node *curr_node = NULL;
	client_node *next_node = NULL;

	while (curr_group != NULL) {
		next_group = curr_group->next;
		curr_node = curr_group->head;
		
		while (curr_node != NULL) {
			next_node = curr_node->next;

			if (curr_node->fd != -1)
				close(curr_node->fd);

			free(curr_node->ip_addr);
			curr_node->ip_addr = NULL;
			free(curr_node);
			curr_node = NULL;
			curr_node = next_node;
		}
	
		if (curr_group->editor_fp != NULL)
			fclose(curr_group->editor_fp);

		pthread_mutex_destroy(&curr_group->mutex);
		free(curr_group);
		curr_group = NULL;
		curr_group = next_group;
	}

	free(result);
	result = NULL;
	close(sock_fd);
	exit(EXIT_SUCCESS);
}

text_group *create_text_group() {
	text_group *new_group = malloc(sizeof(text_group));
	new_group->text_id = 0;
	new_group->size = 0;
	new_group->start_index = 0;

	new_group->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	new_group->editor_fp = fopen("00011", "a+"); // need to fix when we start using text_id
	new_group->text[0] = '\0';

	new_group->head = NULL;
	new_group->next = NULL;

	return new_group;
}

int main(int argc, char **argv) {
	running = 1;
	signal(SIGINT, sigint_handler);
	head_group = create_text_group();
	head_group->text_id = strdup("00011");     // TEMP: when start using text_id, remove it

	// Socket initialization
	int sock_status;
	struct addrinfo sock_hints;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&sock_hints, 0, sizeof(struct addrinfo));
  sock_hints.ai_family = AF_INET;
  sock_hints.ai_socktype = SOCK_STREAM;
  sock_hints.ai_flags = AI_PASSIVE;
  sock_status = getaddrinfo(NULL, PORT, &sock_hints, &result);

  if (sock_status != 0) {
	  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sock_status));
    exit(1);
	}

	// Socket setting with option
	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	// If bind fails, exit
  if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
	 perror("bind()");
   exit(1);
  }

	// If listen fails, exit
	if (listen(sock_fd, 10) != 0) {
		perror("listen()");
	  exit(1);
	}
	
	// Socket configuration done.  Listning, binding check finished.
	// Start accepting client accesses
	struct sockaddr_in *result_addr = (struct sockaddr_in *) result->ai_addr;
	struct sockaddr_in client_addr;
	socklen_t addr_size;
	addr_size = sizeof(client_addr);
	printf("Listening on file descriptor %d, port %d\n", sock_fd, ntohs(result_addr->sin_port));
	
	int client_fd;

	while (running) {
		if ((client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
			perror("client_accept_error");
			exit(1);
		} else {
			// Grabbing client's ip addr
			char client_name[INET_ADDRSTRLEN];

			// Get client's information
			if(inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, 
																							client_name, sizeof(client_name)) != NULL) {
				printf("New access: client_ip:%s, client_port:%s\n", 
																								client_name, ntohs(client_addr.sin_port));
			} else {
				printf("Unable to get client's address\n"); 
			}

			pthread_t new_user_thread;
			
			client_node *new_node = malloc(sizeof(client_node));
			new_node->id = new_user_thread;
			new_node->fd = client_fd;
			new_node->ip_addr = strdup(client_name);
			new_node->text_id = strdup("00011");
			new_node->next = NULL;

			if (pthread_create(&new_user_thread, NULL, client_interaction, new_node)) {
				printf("Connection failed with client_fd=%d\n", client_fd);

				free(new_node->ip_addr);
				free(new_node);
			} else {
				// When start using text_id, put right client_node into correct text_group
				if (head_group->head == NULL)
					head_group->head = new_node;
				else {
					new_node->next = head_group->head;
					head_group->head = new_node;
				}

				printf("Connection made: client_ip:%s, client_fd=%d\n", client_name, client_fd);
			}
		}
	}

	if (head_group != NULL)
		sigint_handler();
	
  return 0;
}
