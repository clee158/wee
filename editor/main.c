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
char *text_id;

typedef struct command {
	int ch;			/* user requested change */
	int x;			/* user's current x-position on wee */
	int y;			/* user's current y-position on wee */
} command;

typedef struct text_id_info{
	size_t exists;		/* indicates if the specified file is new or pre-existing */
	size_t file_size;	/* indicates size of specified file */
} text_id_info;

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

/***************************************************************************************
 * * Title: strsplit()
 * * Author: Perepelitsa, Costya
 * * Date: Dec 22, 2013
 * * Code version: 2.0
 * * Availability: https://www.quora.com/How-do-you-write-a-C-program-to-split-a-string-by-a-delimiter
 * *
 **************************************************************************************/
char** strsplit(const char* str, const char* delim, size_t* numtokens){
	char *s = strdup(str);
	size_t tokens_alloc = 1;
	size_t tokens_used = 0;
	char **tokens = calloc(tokens_alloc, sizeof(char*));
	char *token, *rest = s;
	while((token = strsep(&rest, delim)) != NULL){
		if(tokens_used == tokens_alloc){
			tokens_alloc *= 2;
			tokens = realloc(tokens, tokens_alloc * sizeof(char*));
		}
		tokens[tokens_used++] = strdup(token);
	}
	
	// cleanup
	if(tokens_used == 0){
		free(tokens);
		tokens = NULL;
	}
	else {
		tokens = realloc(tokens, tokens_used * sizeof(char*));
	}
	*numtokens = tokens_used;
	free(s);
	return tokens;
}

/*
 *	Accepts user input char-by-char using getchar() and sends each command to server.
 */
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
		
		// no need to send arrow movement to server
		if(c->ch == KEY_DOWN || c->ch == KEY_UP || c->ch == KEY_LEFT || c->ch == KEY_RIGHT)
			queue_push(work_queue, c);
		else{
			// send server the command
			while ((s_len = write(sock_fd, ((char *)c) + tot_sent, sizeof(command) - tot_sent)) > 0) { 

				tot_sent += s_len;
				if (tot_sent == sizeof(command)) {
					dprintf(fd, "[ accept_user_input() ] 	sent command to server!\n");
					break;
				}
			}

			int err = errno;

			if ((s_len == -1 && err != EINTR) || s_len == 0){
				perror("write");
				break;
			}

			// ESC key	-- save before exiting!
			if(c->ch == 27){
				tot_sent = 0;
				s_len = 0;
				r_len = 0;
				dprintf(fd, "[ accept_user_input() ]		received ESC!\n");
				// write entire file to server
				
				// get vector version
				vector *v = get_vector_form();
				char **line = (char**)vector_front(v);
				char updated_line[500];
				strcpy(updated_line, *line);
				strcat(updated_line, "\n");

				size_t total_line_num = num_lines();
				size_t curr_size = strlen(updated_line);
				size_t i = 1;
				
				// write entire file to server before closing, to save the file
				while ((s_len = write(sock_fd, updated_line + tot_sent, curr_size - tot_sent)) > 0) { 
					
					tot_sent += s_len;
					if (tot_sent == curr_size) {
						if(i == total_line_num){
							
							dprintf(fd, "[ accept_user_input() ]		done sending file!\n");
							break;
						}
						else{
							i++;
							line++;
							strcpy(updated_line, *line);
							strcat(updated_line, "\n");
							curr_size = strlen(updated_line);
							tot_sent = 0;
							dprintf(fd, "[ accept_user_input() ]		wrote file line %lu!\n", i);
						}
					}
				}
				int err = errno;
				
				if ((s_len == -1 && err != EINTR) || s_len == 0){
					perror("write");
					break;
				}

				queue_push(work_queue, c);
				shutdown(sock_fd, SHUT_RDWR);
				close(sock_fd);
				break;
			}
		}
	}
	
	dprintf(fd, "[ accept_user_input() ]		DONE\n");
	free(c);
	return NULL;
}

/*
 *	Reads in commands from server and pushes each into the worker queue.
 */
void *get_server_commands(void *arg) {
	
	dprintf(fd, "[ get_server_commands() ]\n");
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_read = 0;
	command *c = calloc(sizeof(command), 1);

	while(1) {
		tot_read = 0;

		// read in command
		while ((r_len = read(sock_fd, ((char *) c) + tot_read, sizeof(command) - tot_read)) > 0) {
			tot_read += r_len;

			if (tot_read == sizeof(command)) {
				dprintf(fd, "[ get_server_commands() ]	reading done!\n");
				break;
			}
		}

		if ((r_len == -1 && errno != EINTR) || r_len == 0) {
			perror("read");
			break;
		}

	dprintf(fd, "[ get_server_commands() ]	queue_push: ch: %d, x: %d, y: %d\n", c->ch, c->x, c->y);
		if (tot_read == sizeof(command))
			// pass on command to worker thread
			queue_push(work_queue, c);
	}

	dprintf(fd, "[ get_server_commands() ]	DONE\n");
	free(c);
	return NULL;
}

/*
 *	Pulls commands from queue one at a time and calls handle_input() to make each received change.
 */
void *run_server_commands(void *arg) {
	dprintf(fd, "[ run_server_commands() ]\n");
	while(1) {
		// send to editor
		command *c = (command *) queue_pull(work_queue);
		handle_input(wee, c->ch, c->x, c->y);

		if (c->ch != 27) {
			dprintf(fd, "[ run_server_commands() ]	handle_input ch: %d, x: %d, y: %d\n", c->ch, c->x, c->y);
			print_document(wee);
		} else {
			dprintf(fd, "[ run_server_commands() ]	shutting down!\n");
			// cleanup
			refresh();
			endwin();
			break;
		}

		free(c);
	}

	return NULL;
}

/*
 *	Sends text_id to server.
 *	Server replies with text_id_info, which indicates whether:
 *		=> file is new or pre-existing
 *
 *	If (exists == 1)
 *		=> read in file strings and update doc
 */
void handle_text_id(){
	
	ssize_t r_len = 0;
	ssize_t s_len = 0;
	ssize_t tot_sent = 0;
	ssize_t tot_read = 0;

	// i. write text_id to server
	while ((s_len = write(sock_fd, text_id + tot_sent, strlen(text_id) - tot_sent)) > 0) { 

		tot_sent += s_len;
		if (tot_sent == strlen(text_id)) {
			dprintf(fd, "[ handle_text_id() ] 	sent text_id to server!\n");
			printf("text_id sent to server!\n");
			break;
		}
	}

	if ((s_len == -1 && errno != EINTR) || s_len == 0){
		perror("write");
		return;
	}
	printf("waiting to read!\n");
	tot_read = 0;
	// ii. read in text_id_info from server
	text_id_info *tii = calloc(sizeof(text_id_info), 1);
	while ((r_len = read(sock_fd, (char*)tii + tot_read, sizeof(text_id_info) - tot_read)) > 0) {
		printf("reading!\n");
		tot_read += r_len;
		printf("total bytes read: %lu\n", tot_read);
		if (tot_read == sizeof(text_id_info)) {
			dprintf(fd, "[ handle_text_id() ] received text_id_info from server!\n");
			printf("text_id_info received from server!\n");
			break;	
		}
	}

	if ((r_len == -1 && errno != EINTR) || r_len == 0) {
		perror("read");
		free(tii);
		return;
	}

	// iii. file exists
	if(tii->exists && tii->file_size){
		ssize_t read_size = 100;
		size_t y = 0;
		tot_read = 0;
		r_len = 0;
		
		printf("reading in requested file from server...\n");
		dprintf(fd, "[ handle_text_id() ] reading in pre-existing file from server!\n");
		
		char buffer[1001];
		char remaining[1501];
		remaining[0] = '\0';
		while ((r_len = read(sock_fd, buffer + tot_read, read_size)) > 0) {
		
			tot_read += r_len;
			buffer[r_len] = '\0';

			printf("total_bytes_read: %lu\n", tot_read);
				
			size_t num_tokens, i;
			char **split_str = strsplit(buffer, "\n", &num_tokens);
				
			printf("num_tokens: %lu\n", num_tokens);

			for(i = 0; i < num_tokens; i++){
				if(i != (num_tokens - 1)){
					// check if there was remaining string from last read
					if(i == 0 && remaining[0] != '\0'){
						strcat(remaining, *(split_str + i));
						insert_line((int)(y), remaining);
						remaining[0] = '\0';
					}
					else{
						insert_line((int)(y), *(split_str + i));
					}
					y++;
				}
				// check if last line has "\n"
				else{
					if(buffer[r_len - 1] == '\n'){
						insert_line((int)(y), *(split_str + i));
						y++;
					}
					else
						strcpy(remaining, *(split_str + i));
				}
				free(*(split_str + i));
			}
			free(split_str);
			
			if(tot_read == tii->file_size)
				break;
		}

		if (r_len == -1 && errno != EINTR) {
			perror("read");
		}
		printf("done reading in requested file!\n");
		dprintf(fd, "[ handle_text_id() ] finished reading in file from server!\n");
	}
	free(tii);
}

/*	
 *	Initialize socket & set up connection.
 */
void run_client() {
	/*	NOTE:
	 *
	 *	If you would like to run the client and the server locally,
	 *	replace "get_ip()" with your own ip address.
	 */
	ip_port = get_port();

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
	printf("connected to server!\n");

	freeaddrinfo(result);
}

/*
 *	Main thread running the whole biz.
 */
int main(int argc, char **argv) {
	
	// initialize editor
	if(argc < 3){
		printf("./editor [server] [text_id]\n");
		printf("	[server]: 'local' or 'remote'\n");
		printf("	[text_id]: 5 integer number\n");
		printf("	example: ./editor local 00011\n");
		exit(1);
	}
	else{
		// check arguments
		if(strcmp(argv[1], "local") == 0)
			ip_addr = "127.0.0.1";
		else if(strcmp(argv[1], "remote") == 0)
			ip_addr = get_ip();
		else{
			printf("[server]: must be 'local' or 'remote'\n");
			exit(1);
		}

		if(strlen(argv[2]) != 5){
			printf("<text_id> must be a 5 character number.\n");
			exit(1);
		}
		wee = create_editor_no_file();
	}
	
	// init text_id
	text_id = malloc(strlen(argv[2]) + 1);
	strcpy(text_id, argv[2]);
	
	// log for debugging purposes
	fd = open("log.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG);
	dprintf(fd, "[ Main() ]\n");
	
	// set up socket & connection
	run_client();
	
	// send text_id & receive file updates
	handle_text_id();

	// start ncurses scrren
	init_scr(wee);

	// queue for holding commands
	work_queue = queue_create(-1, command_copy_constructor, command_destructor);

	// worker threads
	pthread_create(&user_input, NULL, accept_user_inputs, NULL);
	pthread_create(&user_output, NULL, get_server_commands, NULL);
	pthread_create(&run_instruction, NULL, run_server_commands, NULL);

	pthread_join(user_input, NULL);
	pthread_join(user_output, NULL);
	pthread_join(run_instruction, NULL);
	
	// cleanup
	queue_destroy(work_queue);
	close(sock_fd);
	free(text_id);

  return 0;
}
