#include "server.h"
void *sync_send(void *tg) {
	// Loop through current target_group's nodes to send the updated message 
	text_group *target_group = (text_group *)tg;
	char *text_id = strdup(target_group->text_id);
	int curr_running;

	pthread_mutex_lock(&running_mutex);
	curr_running = running;
	pthread_mutex_unlock(&running_mutex);

	while (curr_running) {
		/////////////QUEUE SIZE CHECK/////////////////
		pthread_mutex_lock(&target_group->size_mutex);
		while (target_group->queue_size == 0) {
			pthread_mutex_lock(&running_mutex);
			curr_running = running;
			pthread_mutex_unlock(&running_mutex);
			
			if (!curr_running) {
				printf("%s: not running anymore so exiting\n", text_id);
				free(text_id);
				return NULL;
			}

			printf("%s: nothing in the group's queue!\n", target_group->text_id);
			pthread_cond_wait(&target_group->size_cond, &target_group->size_mutex);
		}
		pthread_mutex_unlock(&target_group->size_mutex);
		printf("%s: queue has something in it\n", target_group->text_id);
		/////////////QUEUE SIZE CHECK/////////////////

		command *c = queue_pull(target_group->queue);

		/////////////INCREMENT QUEUE SIZE/////////////////
		pthread_mutex_lock(&target_group->size_mutex);
		--(target_group->queue_size);
		pthread_mutex_unlock(&target_group->size_mutex);
		/////////////INCREMENT QUEUE SIZE/////////////////

		client_node *curr = target_group->head_client;
		client_node *next = NULL;

		ssize_t tot_sent = 0;
		ssize_t s_len = 0;

		while (curr != NULL) {
			printf("%s: sending command:%d, %d, %d To client:%s:%d, fd: %d\n", 
					   target_group->text_id, c->ch, c->x, c->y, curr->ip_addr, curr->port, curr->fd);
			next = curr->next;

			while ((s_len = write(curr->fd, ((char *)c) + tot_sent, sizeof(command) - tot_sent)) > 0) {
				tot_sent += s_len;

				if (tot_sent == sizeof(command)) {
					printf("Sending data done\n");
					break;
				}
			}
			
			int err = errno;

			printf("%s: write result: tot_sent: %zd, s_len: %zd, errno: %d\n", 
			       target_group->text_id, tot_sent, s_len, err);

			if (s_len == 0 || (s_len == -1 && err == EBADF)) {
				//connection closed
				printf("%s: connection closed with %s:%d\n", 
								target_group->text_id, curr->ip_addr, curr->port);
				//destroy_client_node(curr);
			}

			tot_sent = 0;
			curr = next;
		}

		pthread_mutex_lock(&running_mutex);
		curr_running = running;
		pthread_mutex_unlock(&running_mutex);
	}

	return NULL;
}

void *client_interaction(void *client_n) {
	pthread_detach(pthread_self());

	// client information
	client_node *client = (client_node *)client_n;
	text_group *target_group = find_text_group(client->text_id);
	printf("%lu: client's text group: %s\n", pthread_self() % 1000, target_group->text_id);

	ssize_t r_len = 0;
	ssize_t tot_read = 0;
	command *c = calloc(sizeof(command), 1);

	pthread_mutex_lock(&running_mutex);
	int curr_running = running;
	pthread_mutex_unlock(&running_mutex);

	while (curr_running) {
		tot_read = 0;

		while ((r_len = read(client->fd, ((char *) c) + tot_read, sizeof(command) - tot_read)) > 0) {
			tot_read += r_len;

			if (tot_read == sizeof(command))
				break;
		}

		int err = errno;

		printf("%lu: from :%s, read command:%d, %d, %d\n", pthread_self() % 1000, client->ip_addr, c->ch, c->x, c->y);
		printf("r_len: %zd\n", r_len);

		if ((r_len == -1 && err != EINTR) || r_len == 0) {
			perror("read");
			break;
		}

		if (0 < r_len) {
			queue_push(target_group->queue, c);

			/////////////INCREMENT QUEUE SIZE/////////////////
			pthread_mutex_lock(&target_group->size_mutex);
			++(target_group->queue_size);
			pthread_cond_signal(&target_group->size_cond);
			pthread_mutex_unlock(&target_group->size_mutex);
			/////////////INCREMENT QUEUE SIZE/////////////////
		}

		pthread_mutex_lock(&running_mutex);
		curr_running = running;
		pthread_mutex_unlock(&running_mutex);
	}

	fprintf(stderr, "client %s:%u disconnected\n", client->ip_addr, client->port);
	destroy_client_node(client);

	return NULL;
}

int main(int argc, char **argv) {
	pthread_mutex_init(&running_mutex, 0);
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, sigint_handler);
	head_group = create_text_group("00011");
	run_server();

	struct sockaddr_in client_addr;
	socklen_t addr_size;
	addr_size = sizeof(client_addr);

	int client_fd;
	int curr_running;

	pthread_mutex_lock(&running_mutex);
	curr_running = running;
	pthread_mutex_unlock(&running_mutex);

	while (curr_running) {
		if ((client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		} else {
			// Grabbing client's ip addr
			char client_ip[INET_ADDRSTRLEN];

			// Get client's information
			if(inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, 
																							client_ip, sizeof(client_ip)) != NULL) {
				fprintf(stderr, "New access: client_ip:%s, client_port:%d\n", 
																								client_ip, ntohs(client_addr.sin_port));
			} else {
				fprintf(stderr, "Unable to get client's address\n"); 
			}

			// TODO: need to put group finding with textid from editor
			pthread_t new_user_thread;
			client_node *new_node = create_client_node(new_user_thread, 
																								 client_fd, 
																								 client_ip, 
																								 "00011", 
																								 ntohs(client_addr.sin_port));

			if (pthread_create(&new_user_thread, NULL, client_interaction, new_node)) {
				fprintf(stderr, "Connection failed with client=%s:%d\n", client_ip, 
																											ntohs(client_addr.sin_port));
				destroy_client_node(new_node);

			} else {
				// When start using text_id, put right client_node into correct text_group
				new_node->next = head_group->head_client;
				head_group->head_client = new_node;
			}
		}

		pthread_mutex_lock(&running_mutex);
		curr_running = running;
		pthread_mutex_unlock(&running_mutex);
	}

	if (head_group != NULL)
		sigint_handler(SIGINT);
	
  return 0;
}

void destroy_client_node(client_node *client) {
	if (client == NULL)
		return;

	// Loop through the groups to find the correct group to detach current client before deleting
	text_group *group = find_text_group(client->text_id);
	client_node *prev_node = NULL;
	client_node *curr_node = group->head_client;

	while (curr_node) {
		if (!strcmp(curr_node->ip_addr, client->ip_addr) && curr_node->port == client->port) {
			// Correctly connect
			if (prev_node)
				prev_node->next = curr_node->next;
			else
				group->head_client = curr_node->next;

			curr_node->next = NULL;

			// Free allocated memories
			if (client->ip_addr != NULL) {
				free(client->ip_addr);
				client->ip_addr = NULL;
			}

			if (client->text_id != NULL) {
				free(client->text_id);
				client->text_id = NULL;
			}
		
			// Close open fd
			if (client->fd != -1)
				close(client->fd);

			free(client);
			break;
		}

		prev_node = curr_node;
		curr_node = curr_node->next;
	}
}


text_group *find_text_group(char *text_id) {
	text_group *target_group = head_group;

	while (target_group != NULL && strcmp(target_group->text_id, text_id))
		target_group = target_group->next;

	if (target_group == NULL) {
		perror("Failed to find the correct text_group\n");
		return NULL;
	}

	return target_group;
}

void sigint_handler(int sig) {
	if (sig == SIGPIPE) {
		printf("Caught SIGPIPE!\n");
		return;
	}

	exit(1);
	fprintf(stderr, "Cleaning up before exiting...\n");
	pthread_mutex_lock(&running_mutex);
	running = 0;
	pthread_mutex_unlock(&running_mutex);
	text_group *curr_group = head_group;
	text_group *next_group = NULL;
	//client_node *curr_node = NULL;
	//client_node *next_node = NULL;

	while (curr_group != NULL) {
		pthread_cond_signal(&curr_group->size_cond);
		pthread_join(curr_group->sync_sender, NULL);
		next_group = curr_group->next;
		destroy_text_group(curr_group);
		curr_group = next_group;
	}

	close(sock_fd);
	exit(EXIT_SUCCESS);
}

void run_server() {
	// Socket initialization
	int sock_status;
	struct addrinfo sock_hints;
  memset(&sock_hints, 0, sizeof(struct addrinfo));
  sock_hints.ai_family = AF_INET;
  sock_hints.ai_socktype = SOCK_STREAM;
  sock_hints.ai_flags = AI_PASSIVE;
  sock_fd = socket(sock_hints.ai_family, sock_hints.ai_socktype, 0);
  sock_status = getaddrinfo(NULL, PORT, &sock_hints, &result);

  if (sock_status != 0) {
	  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sock_status));
    exit(EXIT_FAILURE);
	}

	// Socket setting with option
	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval, sizeof(optval));

	// If bind fails, exit
  if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
	 perror("bind");
   exit(EXIT_FAILURE);
  }

	// If listen fails, exit
	if (listen(sock_fd, 100) != 0) {
		perror("listen");
	  exit(EXIT_FAILURE);
	}
	
	// Socket configuration done.  Listning, binding check finished.
	// Start accepting client accesses
	struct sockaddr_in *result_addr = (struct sockaddr_in *) result->ai_addr;
	fprintf(stderr, "Listening on file descriptor %d, port %d\n", 
																								sock_fd, ntohs(result_addr->sin_port));
	freeaddrinfo(result);
}

client_node *create_client_node(pthread_t new_user_thread, int client_fd, char *client_ip, 
																														char *text_id, uint16_t port) {
	client_node *new_node = malloc(sizeof(client_node));
	new_node->id = new_user_thread;
	new_node->fd = client_fd;
	new_node->ip_addr = strdup(client_ip);
	new_node->text_id = strdup(text_id);
	new_node->port = port;
	new_node->next = NULL;

	return new_node;
}

text_group *create_text_group(char *text_id) {
	text_group *new_group = malloc(sizeof(text_group));
	new_group->queue = queue_create(-1, command_copy_constructor, command_destructor);
	new_group->text_id = strdup(text_id);
	new_group->size = 0;
	new_group->head_client = NULL;
	new_group->next = NULL;
	pthread_cond_init(&new_group->size_cond, NULL);
	pthread_mutex_init(&new_group->size_mutex, NULL);
	pthread_create(&new_group->sync_sender, NULL, sync_send, new_group);

	return new_group;
}

void destroy_text_group(text_group *group) {
	if (group == NULL)
		return;
	
	// Loop through to find the correct group to delete for connecting groups 
	text_group *prev_group = NULL;
	text_group *curr_group = head_group;

	while (curr_group) {
		if (!strcmp(group->text_id, curr_group->text_id)) {	// found it!
			// Correctly connect each other
			if (prev_group)
				prev_group->next = curr_group->next;
			else
				head_group = curr_group->next;

			// Free its client nodes
			if (curr_group->head_client != NULL) {
				client_node *curr_node = curr_group->head_client;
				client_node *next_node = NULL;

				while (curr_node) {
					next_node = curr_node->next;
					destroy_client_node(curr_node);
					curr_node = next_node;
				}
			}

			// Free allocated memories
			if (group->text_id != NULL) {
				free(group->text_id);
				group->text_id = NULL;
			}
	
			queue_destroy(curr_group->queue);
			pthread_cond_destroy(&curr_group->size_cond);
			pthread_mutex_destroy(&curr_group->size_mutex);
			free(curr_group);

			break;
		}

		prev_group = curr_group;
		curr_group = curr_group->next;
	}
}

void *command_copy_constructor(void *elem) {
	if (elem == NULL)
		return NULL;

	command *c = (command *)elem;	
	command *new_c = calloc(sizeof(command), 1);
	
	new_c->ch = c->ch;
	new_c->x = c->x;
	new_c->y = c->y;

	return new_c;
}
void command_destructor(void *elem) {
	if (elem != NULL)
		free(elem);
}
