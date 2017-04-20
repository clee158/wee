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
#define PORT "5555"
#define TEXT_DIR "" // TODO: update as updating text directory
/*
 *	text_id is consisted of owner + unique number (e.g. [0001][1] = 00011)
 *	When a client tries to access a text file, the server takes client_id and text file 
 *	information and go through following steps:
 *		1) Check if the current user_id (tied to user's ip) is allowed to edit the text file
 *		2) If yes, find if there is any open text_group for that text file (with text_id)
 *		3) Add a client_node with corresponding information at the head
 *			- If there isn't the right text group, add a text_group with corresponding information
 *		4) Send the most recent text file version to the user and keep the user notified 
 *			when there is any change.
 */

// Keeps track of live clients
typedef struct client_node {
	pthread_t id;
	char *ip_addr;
	char *text_id;
	int fd;
	uint16_t port;
	struct client_node *next;
} client_node;

// Keeps track of groups of live clients on different text_id
// Start using after text_id gets used
typedef struct text_group {
	char *text_id;
	FILE *editor_fp;
	client_node *head_client;
	size_t size;
	pthread_mutex_t mutex;
	struct text_group *next;
	size_t start_index;
} text_group;

// Running is set to 1 until SIGINT tries to kill the server
int running;

text_group *head_group;
int sock_fd;
struct addrinfo *result;

/*
 * Sends out most updated vesion of text_id
 */
void sync_send(char *text_id);

/*
 * Thread's starting point for accepting client inputs
 * Each thread holds a client
 */
void *client_interaction(void *client_fd);

/*
 * SIGINT handler
 * It cleans up before exiting
 * It joins all the running threads and deletes allocated memory
 */
void sigint_handler();

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
