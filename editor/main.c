#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "../libs/queue.h"
#include ".base64.h"
#include "editor.h"

editor *wee;
int sock_fd;
char *get_port();
char *get_ip();
char *ip_addr;
char *ip_port;
queue *work_queue;

typedef struct command{
	int ch;
	int x;
	int y;
}command;

void *command_copy_constructor(void *elem){
	command *c = malloc(sizeof(command));
	memcpy(c, elem, sizeof(command));
	return c;
}

void command_destructor(void *elem){
	if(elem)
		free(elem);
}

void *accept_user_inputs(void *arg) {
	ssize_t s_len = 0;
	int ch = 0;
	command c;

	while (ch != 27) {
		ch = getch();

		if(ch == 27){
			shutdown(sock_fd, SHUT_RDWR);
			refresh();
			endwin();
			queue_push(work_queue, &ch);
			return NULL;
		}
		
		c.ch = ch;
		c.x = get_curr_x(wee);
		c.y = get_curr_y(wee);

		s_len = write(sock_fd, &c, sizeof(command));

		if (!(s_len > 0)) {
			perror("send()");
			return NULL;
		}
	}
	return NULL;
}

void *get_server_commands(void *text_id) {
	ssize_t r_len = 0;
	command c;

	while (1) {
		r_len = read(sock_fd, &c, sizeof(command));
		
		if (!(r_len > 0)) {
			perror("read()");
			break;
		}
		queue_push(work_queue, &c);
	}
	return NULL;
}

void *run_server_commands(void *elem){
	while(1){
		command *c = (command*)queue_pull(work_queue);
		handle_input(wee, c->ch, c->x, c->y);
		
		int ch = c->ch;
		free(c);
		
		if(ch != 27)
			print_document(wee);
		else
			break;
	}
	return NULL;
}

void run_client() {
	ip_addr = "172.22.152.54";//get_ip();
	ip_port = get_port();
	
	int s;
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval, sizeof(optval));
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(ip_addr, ip_port, &hints, &result);

	if (s != 0) {
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	
	if (connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);
}

void handle_resize(int signal){
	endwin();
	refresh();
	clear();

	print_document(wee);
	refresh();
}

int main(int argc, char* argv[]){
	// start client
	run_client();
	
	// set up editor & ncurses
	if(argc > 1)
		wee = create_editor_file(argv[1]);
	else
		wee = create_editor_no_file();

	init_scr();
	
	// handle resize signal
	struct sigaction signal;
	memset(&signal, 0, sizeof(struct sigaction));
	signal.sa_handler = handle_resize;
	sigaction(SIGWINCH, &signal, NULL);

	// set up queue
	work_queue = queue_create(-1, command_copy_constructor, command_destructor);

	// thread pool
	pthread_t user_input;
	pthread_t server_command;
	pthread_t run_instructions;
	
	pthread_create(&server_command, NULL, get_server_commands, "00011");
	pthread_create(&run_instructions, NULL, run_server_commands, "00011");
	pthread_create(&user_input, NULL, accept_user_inputs, "00011");

	pthread_join(user_input, NULL);
	pthread_join(server_command, NULL);
	pthread_join(run_instructions, NULL);

	// cleanup
	queue_destroy(work_queue);

	return 0;
}
