#include "server.h"
void *sync_send(void *tg) {
	// Loop through current target_group's nodes to send the updated message 
	text_group *target_group = (text_group *)tg;
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
			
			if (!curr_running)
				return NULL;

			pthread_cond_wait(&target_group->size_cond, &target_group->size_mutex);
		}
		pthread_mutex_unlock(&target_group->size_mutex);
		/////////////QUEUE SIZE CHECK/////////////////

		sync_send_data_packet *dp = queue_pull(target_group->queue);
		client_node *curr = (dp->target_group)->head_client;
		client_node *next = NULL;

		ssize_t tot_sent = 0;
		ssize_t s_len = 0;

		while (curr != NULL) {
			fprintf(stderr, "To client:%s:%d\n", curr->ip_addr, curr->port);
			next = curr->next;

			while ((s_len = write(curr->fd, dp->data + tot_sent, strlen(dp->data) - tot_sent)) > 0) {
				tot_sent += s_len;

				if (tot_sent == (ssize_t)strlen(dp->data)) {
					fprintf(stderr, "Sending data done\n");
					break;
				}
			}

			tot_sent = 0;
			curr = next;
		}
		
		data_packet_destructor(dp);

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

	ssize_t r_len = 0;
	char buffer[32];

	while (running) {
		r_len = read(client->fd, buffer, sizeof(buffer));

		if (!(r_len > 0)) {
			perror("read");
			break;
		}

		sync_send_data_packet *data_packet = calloc(sizeof(sync_send_data_packet), 1);
		data_packet->target_group = target_group;
		data_packet->data = strdup(buffer);
		queue_push(target_group->queue, data_packet);

		/////////////INCREMENT QUEUE SIZE/////////////////
		pthread_mutex_lock(&target_group->size_mutex);
		++target_group->queue_size;
		pthread_cond_signal(&target_group->size_cond);
		pthread_mutex_unlock(&target_group->size_mutex);
		/////////////INCREMENT QUEUE SIZE/////////////////

		data_packet_destructor(data_packet);
	}

	fprintf(stderr, "client %s:%u disconnected\n", client->ip_addr, client->port);

	return NULL;
}

int main(int argc, char **argv) {
	pthread_mutex_init(&running_mutex, 0);
	signal(SIGINT, sigint_handler);
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
		sigint_handler();
	
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

void sigint_handler() {
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
	new_group->queue = queue_create(-1, data_packet_copy_constructor, data_packet_destructor);
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

void *data_packet_copy_constructor(void *elem) {
	if (elem == NULL)
		return NULL;

	sync_send_data_packet *curr = (sync_send_data_packet *)elem;
	sync_send_data_packet *new_dp = calloc(sizeof(curr), 1);

	if (curr->data != NULL)
		new_dp->data = strdup(curr->data);

	if (curr->target_group != NULL)
		new_dp->target_group = curr->target_group;
	
	return new_dp;
}

void data_packet_destructor(void *elem) {
	if (elem == NULL)
		return;

	sync_send_data_packet *curr = (sync_send_data_packet *)elem;

	if (curr->data != NULL) {
		free(curr->data);
		curr->data = NULL;
	}
	
	free(curr);
}
