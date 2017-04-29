#include "editor.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include ".base64.h"

editor *wee;
int running = 1;
pthread_t input;
pthread_t output;
char buff_out[32];
char buff_in[32];
int sock_fd;
char *get_port();
char *get_ip();
char *ip_addr;
char *ip_port;
queue *work_queue;

void *accept_user_inputs(void *arg) {
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_sent = 0;

	while (running) {
		r_len = read(0, buff_out, sizeof(buff_out));

		int err = errno;
		if (!(r_len > 0)) {
			perror("read");
			break;
		}

		while (tot_sent < r_len) {
			s_len = write(sock_fd, buff_out, r_len);

			if (!(s_len > 0)) {
				perror("send");
				return NULL;
			}

			tot_sent += s_len;
		}
	}

	return NULL;
}

void *server_outputs(void *text_id) {
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_sent = 0;

	while (running) {
		r_len = read(sock_fd, buff_in, sizeof(buff_int));

		if (!(r_len > 0)) {
			perror("read");
			break;
		}

		while (tot_sent < r_len) {
			s_len = write(sock_fd, buff_out, r_len);
			
			if (!(s_len > 0)) {
				perror("send");
				return NULL;
			}

			tot_sent += s_len;
		}
	}

	return NULL;
}

void run_client() {
	ip_addr = get_ip();
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
	run_client();

	if(argc > 1)
		wee = create_editor_file(argv[1]);
	else
		wee = create_editor_no_file();

	init_scr();
	
	struct sigaction signal;
	memset(&signal, 0, sizeof(struct sigaction));
	signal.sa_handler = handle_resize;
	sigaction(SIGWINCH, &signal, NULL);

	int ch = 0;
	//char mode = 0;
	while(ch != 27){
		//handle_mode(wee);
		ch = getch();
		handle_input(wee, ch);
		if(ch != 27)
			print_document(wee);
	}

	refresh();
	endwin();

	return 0;
}
