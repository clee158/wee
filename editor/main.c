#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "../libs/queue.h"
#include ".base64.h"
#include "editor.h"

editor *wee;
int sock_fd;
int fd;
pthread_t user_input;
pthread_t user_output;
pthread_t run_instruction;
char *get_port();
char *get_ip();
char *ip_addr;
char *ip_port;
queue *work_queue;

typedef struct command {
	int ch;
	int x;
	int y;
} command;

void *command_copy_constructor(void *elem) {
	if (elem == NULL)
		return NULL;

	command *c = malloc(sizeof(command));
	memcpy(c, elem, sizeof(command));
	return c;
}
void command_destructor(void *elem) {
	if (elem != NULL)
		free(elem);
}

void *accept_user_inputs(void *arg) {
	
	dprintf(fd, "[ accept_user_input() ]\n");
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_sent = 0;
	int ch = 0;
	command *c = calloc(sizeof(command), 1);

	while (1) {
		tot_sent = 0;
		c->ch = getch();
		c->x = get_curr_x(wee);
		c->y = get_curr_y(wee);

		dprintf(fd, "[ accept_user_input() ]		ch: %d, x: %d, y: %d\n", c->ch, c->x, c->y);
		
		if (c->ch == 27) {
			dprintf(fd, "[ accept_user_input() ]		received ESC!\n");
			queue_push(work_queue, c);
			break;
		}
		else if(c->ch == KEY_DOWN || c->ch == KEY_UP || c->ch == KEY_LEFT || c->ch == KEY_RIGHT)
			queue_push(work_queue, c);
		else{
			while ((s_len = write(sock_fd, ((char *)c) + tot_sent, sizeof(command) - tot_sent)) > 0) { 

				tot_sent += s_len;
				if (tot_sent == sizeof(command)) {
					//printf("sending done: %lu\n", sizeof(command));
				
			dprintf(fd, "[ accept_user_input() ] 	sent command to server!\n");
					break;
				}
			}

			//printf("sent to server: %d, %d, %d\n", c->ch, c->x, c->y);
			int err = errno;

			if ((s_len == -1 && err != EINTR) || s_len == 0) {
				//printf("something's wrong!\n");
				//printf("s_len: %zd, errno: %d\n", s_len, err);
				break;
			}
		}
	}
	
	dprintf(fd, "[ accept_user_input() ]		DONE\n");
	free(c);
	return NULL;
}

void *get_server_commands(void *arg) {
	
	dprintf(fd, "[ get_server_commands() ]\n");
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_read = 0;
	command *c = calloc(sizeof(command), 1);

	while(1) {
		tot_read = 0;

		while ((r_len = read(sock_fd, ((char *) c) + tot_read, sizeof(command) - tot_read)) > 0) {
			tot_read += r_len;

			if (tot_read == sizeof(command)) {
				//printf("reading done\n");			
				dprintf(fd, "[ get_server_commands() ]	reading done!\n");
				break;
			}
		}

		if ((r_len == -1 && errno != EINTR) || r_len == 0) {
			perror("read");
			break;
		}

		//printf("queue_push: %d, %d, %d\n", c->ch, c->x, c->y);
		
	dprintf(fd, "[ get_server_commands() ]	queue_push: ch: %d, x: %d, y: %d\n", c->ch, c->x, c->y);
		if (tot_read == sizeof(command))
			queue_push(work_queue, c);
	}

	dprintf(fd, "[ get_server_commands() ]	DONE\n");
	free(c);
	return NULL;
}

void *run_server_commands(void *arg) {
	dprintf(fd, "[ run_server_commands() ]\n");
	while(1) {
		command *c = (command *) queue_pull(work_queue);
		handle_input(wee, c->ch, c->x, c->y);

		if (c->ch != 27) {
	dprintf(fd, "[ run_server_commands() ]	handle_input ch: %d, x: %d, y: %d\n", c->ch, c->x, c->y);
			print_document(wee);
		} else {
			dprintf(fd, "[ run_server_commands() ]	shutting down!\n");
			shutdown(sock_fd, SHUT_RDWR);
			close(sock_fd);
			refresh();
			endwin();
			break;
		}

		free(c);
	}

	return NULL;
}

void run_client() {
	// SOCKET Initialization
	ip_addr = "172.22.152.54";//get_ip();
	ip_port = "5555";//get_port();
	//printf("server: %s:%s\n", ip_addr, ip_port);

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

	freeaddrinfo(result);
}

void handle_resize(int signal) {
	endwin();
	refresh();
	clear();
	print_document(wee);
	refresh();
}

int main(int argc, char **argv) {
	run_client();
	
	if(argc > 1)
		wee = create_editor_file(argv[1]);
	else
		wee = create_editor_no_file();
	
	fd = open("log.txt", O_RDWR | O_CREAT);
	dprintf(fd, "[ Main() ]\n");
	
	init_scr();

	struct sigaction signal;
	memset(&signal, 0, sizeof(struct sigaction));
	signal.sa_handler = handle_resize;
	sigaction(SIGWINCH, &signal, NULL);

	work_queue = queue_create(-1, command_copy_constructor, command_destructor);

	pthread_create(&user_input, NULL, accept_user_inputs, NULL);
	pthread_create(&user_output, NULL, get_server_commands, NULL);
	pthread_create(&run_instruction, NULL, run_server_commands, NULL);
	pthread_join(user_input, NULL);
	pthread_join(user_output, NULL);
	pthread_join(run_instruction, NULL);

	queue_destroy(work_queue);
	close(sock_fd);

  return 0;
}
