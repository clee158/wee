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

void *initial_client_interaction(void *client_n) {
	pthread_detach(pthread_self());

	client_node *client = (client_node *)client_n;
	
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));

	// Retrieving client's requested text_id
	ssize_t num_read = read_from_fd(client->fd, buffer, 5);

	if (0 < num_read) {
		buffer[5] = '\0';
		client->text_id = calloc(sizeof(char), num_read + 1);
		strncpy(client->text_id, buffer, num_read);
		client->text_id[5] = '\0';
		printf("client: %s:%d, requested text_id: %s\n", client->ip_addr, client->port, buffer);
	} else {
		fprintf(stderr, "invalid text_id: %s\n", buffer);
		close(client->fd);
		client_node_destructor(client);

		return NULL;
	}

	// Locate where the client is supposed to be at, and insert it into the correct position
	text_group *curr_group = find_text_group(client->text_id);

	if (curr_group == NULL) {
		printf("Requested text_id is not on-going or live\n");
		curr_group = create_text_group(client->text_id);
		curr_group->next = head_group;
		head_group = curr_group;
	}

	client->next = head_group->head_client;
	head_group->head_client = client;

	// Building current text_file directory buffer
	char filename[strlen(buffer) + 1];
	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, text_dir);									// buffer becomes "text_files/"
	strcpy(buffer + strlen(text_dir), client->text_id);  // buffer now becomes "text_files/text_id"
	strncpy(filename, buffer, strlen(buffer) + 1);

	// Need to notify the client about the file
	text_id_info *tid_info = calloc(sizeof(text_id_info), 1);

	/////////////ACCESS TO TEXT FILE/////////////////
	pthread_mutex_lock(&curr_group->file_mutex);

	if (access(filename, F_OK) == 0) {
		printf("Requested text_id exists\n");
		tid_info->exists = 1;
	} else {
		printf("Requested text_id does NOT exist\n");
		tid_info->exists = 0;
		perror("access");
	}

	pthread_mutex_unlock(&curr_group->file_mutex);
	/////////////ACCESS TO TEXT FILE/////////////////

	// If the file already exists, set the size as well
	if (tid_info->exists) {
		int text_fd = open(filename, O_RDONLY);
		struct stat st;
		stat(filename, &st);
		tid_info->file_size = st.st_size;
	} 

	// Sends the text_id_info back to the client
	ssize_t num_write = write_to_fd(client->fd, (char *)tid_info, sizeof(text_id_info));
	if (num_write != sizeof(text_id_info)) {
		fprintf(stderr, "connection error\n");
		close(client->fd);
		client_node_destructor(client);

		return NULL;
	}

	/////////////ACCESS TO TEXT FILE/////////////////
	pthread_mutex_lock(&curr_group->file_mutex);

	// Sending saved text file to the client
	if (tid_info->exists) {
		int text_fd = open(filename, O_RDONLY);
		size_t file_size = tid_info->file_size;
		size_t tot_read = 0;
		memset(buffer, 0, sizeof(buffer));

		while ((num_read = read(text_fd, buffer, sizeof(buffer))) > 0) {
			num_write = write_to_fd(client->fd, buffer, num_read);

			if (num_write != num_read) {
				fprintf(stderr, "Connection error\n");
				close(client->fd);
				close(text_fd);
				client_node_destructor(client);

				return NULL;
			}

			tot_read += num_read;

			if (tot_read == file_size)
				break;
			else if (file_size < tot_read) {
				fprintf(stderr, "Sent too much data\n");
				close(client->fd);
				close(text_fd);
				client_node_destructor(client);

				return NULL;
			}
		}

		close(text_fd);
	}

	pthread_mutex_unlock(&curr_group->file_mutex);
	/////////////ACCESS TO TEXT FILE/////////////////
	
	printf("Sent text_id_info successfully to the client\n");

	free(tid_info);
	client_interaction(client);

	return NULL;
}

void client_interaction(void *client_n) {
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

		printf("%lu: from :%s, read command:%d, %d, %d\n", 
							pthread_self() % 1000, client->ip_addr, c->ch, c->x, c->y);
		printf("r_len: %zd\n", r_len);

		if ((r_len == -1 && err != EINTR) || r_len == 0) {
			perror("read");
			break;
		}

		if (0 < r_len) {
			if (c->ch == 27)
				break;

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

	printf("Client pressed closing button.  Let's save the latest contents\n");
	char filename[strlen(text_dir) + strlen(client->text_id)];
	strcpy(filename, text_dir);
	strcpy(filename + strlen(text_dir), client->text_id);

	char buffer[5120];
	memset(buffer, 0, sizeof(buffer));
	ssize_t num_read = 0;

	/////////////ACCESS TO TEXT FILE/////////////////
	pthread_mutex_lock(&target_group->file_mutex);

	int file_fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG);
	while ((num_read = read(client->fd, buffer, sizeof(buffer))) > 0) {
		printf("bytes: %lu\n", num_read);
		write(file_fd, buffer, num_read);
		memset(buffer, 0, num_read);
	}
	printf("num_read: %zd\n", num_read);
	perror("read");
	close(file_fd);

	pthread_mutex_unlock(&target_group->file_mutex);
	/////////////ACCESS TO TEXT FILE/////////////////

	printf("Client %s:%u disconnected\n", client->ip_addr, client->port);
	free(c);
	destroy_client_node(client);
}

int main(int argc, char **argv) {
	pthread_mutex_init(&running_mutex, 0);

	if (!signal_handling())
		return EXIT_FAILURE;

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
																								 "NULL", // temporary
																								 ntohs(client_addr.sin_port));

			if (pthread_create(&new_user_thread, NULL, initial_client_interaction, new_node)) {
				fprintf(stderr, "Connection failed with client=%s:%d\n", client_ip, 
																											ntohs(client_addr.sin_port));
				destroy_client_node(new_node);

			}
		}

		pthread_mutex_lock(&running_mutex);
		curr_running = running;
		pthread_mutex_unlock(&running_mutex);
	}

	if (head_group != NULL)
		close_server(SIGINT);
	
  return 0;
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
					client_node_destructor(curr_node);
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

			client_node_destructor(curr_node);
			break;
		}

		prev_node = curr_node;
		curr_node = curr_node->next;
	}
}

void client_node_destructor(void *elem) {
	if (elem == NULL)
		return;
	
	client_node *client = (client_node *) elem;

	if (client->ip_addr != NULL) {
		free(client->ip_addr);
		client->ip_addr = NULL;
	}

	if (client->text_id != NULL) {
		free(client->text_id);
		client->text_id = NULL;
	}

	// Close open fd
	close(client->fd);

	free(client);
	client = NULL;
}

void close_server(int sig) {
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

ssize_t read_from_fd(int fd, char *buffer, size_t count) {
  ssize_t total = 0;
  ssize_t rtn_val = 0;
  errno = 0;

  while (total < (ssize_t)count) {
    //fprintf(stderr, "read_all_from_fd: continuing reading\n");
    rtn_val = read(fd, buffer, count- total);
		printf("rtn_val: %zd\n", rtn_val);

    if (rtn_val == -1) {
      if (errno != EINTR) {
        //perror(NULL);
        return -1;
      } else {
        //perror(NULL);
        errno = 0;
      }
    } else {
      total += rtn_val;
      buffer += rtn_val;

      if (rtn_val == 0) {
        //fprintf(stderr, "read_all_from_fd: connection closed: stop reading\n");
        return total;
      }
    }
  }
  
  //fprintf(stderr, "read_all_from_fd: finished reading successfully\n");
  return total;
}

ssize_t write_to_fd(int fd, char *buffer, size_t count) {
  ssize_t total = 0;
  ssize_t rtn_val = 0;
  errno = 0;

  while (total < (ssize_t)count) {
    //fprintf(stderr, "write_all_to_fd: continuing writing\n");
    rtn_val = write(fd, buffer + total, count - total);
		printf("rtn_val: %zd\n", rtn_val);

    if (rtn_val == -1) {
      if (errno != EINTR) {
        //perror(NULL);
        return -1;
      } else {
        //perror(NULL);
        errno = 0;
      }
    } else {
      total += rtn_val;

      if (rtn_val == 0) {
        //fprintf(stderr, "write_all_to_fd: connection closed: stop writing\n");
        return total;
      }
    }
  }

  //fprintf(stderr, "write_all_to_fd: finished writing successfully\n");
  return total;
}

client_node *create_client_node(pthread_t new_user_thread, int client_fd, char *client_ip, 
																														char *text_id, uint16_t port) {
	client_node *new_node = malloc(sizeof(client_node));
	new_node->id = new_user_thread;
	new_node->fd = client_fd;
	new_node->port = port;
	new_node->next = NULL;

	if (client_ip != NULL)
		new_node->ip_addr = strdup(client_ip);

	if (text_id != NULL)
		new_node->text_id = strdup(text_id);

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
	pthread_mutex_init(&new_group->file_mutex, NULL);
	pthread_create(&new_group->sync_sender, NULL, sync_send, new_group);

	return new_group;
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
	if (elem != NULL) {
		free(elem);
		elem = NULL;
	}
}

int signal_handling() {
  struct sigaction act_int, act_pipe;
  memset(&act_int, '\0', sizeof(act_int));
  memset(&act_pipe, '\0', sizeof(act_pipe));
  act_int.sa_handler = close_server;
  act_pipe.sa_handler = close_server;
  if (sigaction(SIGINT, &act_int, NULL) < 0) {
    perror("sigaction-SIGINT");
    return 0;
  }
  if (sigaction(SIGPIPE, &act_pipe, NULL) < 0) {
    perror("sigaction-SIGPIPE");
    return 0;
  }

  return 1;
}

text_group *find_text_group(char *text_id) {
	text_group *target_group = head_group;

	while (target_group != NULL && strcmp(target_group->text_id, text_id))
		target_group = target_group->next;

	if (target_group == NULL)
		return NULL;

	return target_group;
}
