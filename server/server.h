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
#define PORT "5555"

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
	char *text_id;
	int fd;
	char *ip_addr;
	struct client_node *next;
} client_node;

// Keeps track of groups of live clients on different text_id
// Start using after text_id gets used
typedef struct text_group {
	char *text_id;
	size_t size;
	size_t start_index;

	pthread_mutex_t mutex;
	FILE *editor_fp;
	char text[1048576]; // hold 2^20

	client_node *head;
	struct text_group *next;
} text_group;

// Running is set to 1 until SIGINT tries to kill the server
int running;

text_group *head_group;
int sock_fd;
struct addrinfo *result;

// Sends out most updated vesion of text_id
void sync_send(char *text_id);

/*
 * Thread starting point for accepting client inputs
 * Each thread holds a client
 */
void *client_interaction(void *client_fd);

/*
 * SIGINT handler
 * It cleans up before exiting
 */
void sigint_handler();

/*
 * Create a text_group and returns the pointer to it
 */
text_group *create_text_group();
