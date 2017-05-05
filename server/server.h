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
#include <arpa/inet.h>
#include <errno.h>

#include "../libs/queue.h"

#define PORT "5555"

/**
 * Basic Mechanism
 *
 * STARTING
 * 1. Client sends the server text_id that it wants to have access to
 * 2. Server checks whether there is existing file for that text_id
 * 3. Server will send the client existence of requested file and the size of the file
 * 4. If the requested text file exists, the server will send the content back to the server 
 *
 * ENDING
 * 1. Any client closing down will send its copy back to the server
 * 2. Then the server will over-write any existing text file of the same text_id
 */

/**
 * Structures
 */
typedef struct command {
	int ch;
	int x;
	int y;
} command;

typedef struct text_id_info {
	size_t exists;
	size_t file_size;
} text_id_info;

typedef struct client_node {
	char *ip_addr;
	char *text_id;
	struct client_node *next;

	pthread_t id;
	int fd;
	uint16_t port;
} client_node;

typedef struct text_group {
	queue *queue;
	char *text_id;
	client_node *head_client;
	struct text_group *next;
	pthread_t sync_sender;
	pthread_mutex_t size_mutex;
	pthread_cond_t size_cond;

	pthread_mutex_t file_mutex;

	size_t queue_size;
	size_t size;
} text_group;

// Running is set to 1 until SIGINT tries to kill the server
int running = 1;
pthread_mutex_t running_mutex;
text_group *head_group;
int sock_fd;
struct addrinfo *result;
char *text_dir = "text_files/";

/*
 * Sends out most updated vesion of text_id
 */
void *sync_send(void *);

/*
 * Client connection initial point
 * Server receives text_id of text file that the client wants to open
 * And server returns with text_id_info struct for the following cases
 *   1) Requested text_id does not exist on database, therefore it's a new text file entry
 *      - exists: 0
 *      - file_size: 0
 *	 2) Requested text_id exists on server's database, but no one is accessing it right now
 *      * In this case, the server will send the saved text file back to the client
 *      - exists: 1
 *      - file_size: x
 */
void *initial_client_interaction(void *);

/*
 * After initial point, this is where server accepts each client's data and sync sends them
 */
void client_interaction(void *);

/*
 * SIGNAL handler
 * It cleans up before exiting
 * It joins all the running threads and deletes allocated memory
 */
void close_server(int);

/*
 * Creates a text group and returns the pointer to it
 */
text_group *create_text_group(char *);

/*
 * Creates a client node and returns the pointer to it
 */
client_node *create_client_node(pthread_t, int, char *, char *, uint16_t);

/*
 * Finds the correct group and returns it.
 * If it couldn't find one, it returns NULL
 */
text_group *find_text_group(char *);

/*
 * Frees all allocated memory in the client node and frees the node as well
 * It also locates the client node in the correct text group and takes it out
 * and takes care of the connection as it should be
 * It is designed to be called by the client thread itself and another thread as well
 * So it does not try to join back to the thread
 */
void destroy_client_node(client_node *);

/*
 * Frees all allocated memory in the text group and frees the group as well
 * It also locates the group and takes it out
 * and takes care of the connection as it should be
 */
void destroy_text_group(text_group *);

/**
 * Runs TCP server through socket, getaddrinfo, bind, listen
 */
void run_server();

void *command_copy_constructor(void *);
void command_destructor(void *);

ssize_t read_from_fd(int, char *, size_t);
ssize_t write_to_fd(int, char *, size_t);
int signal_handling();
void client_node_destructor(void *);
