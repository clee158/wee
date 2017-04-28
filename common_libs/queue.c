/**
 * Splendid Synchronization Lab 
 * CS 241 - Spring 2017
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * This queue is implemented with a linked list of queue_node's.
 */
typedef struct queue_node {
  void *data;
  struct queue_node *next;
} queue_node;

struct queue {
  /* The function callback for the user to define the way they want to copy
   * elements */
  copy_constructor_type copy_constructor;

  /* The function callback for the user to define the way they want to destroy
   * elements */
  destructor_type destructor;

  /* queue_node pointers to the head and tail of the queue */
  queue_node *head, *tail;

  /* The number of elements in the queue */
  ssize_t size;

  /**
   * The maximum number of elements the queue can hold.
   * max_size is non-positive if the queue does not have a max size.
   */
  ssize_t max_size;

  /* Mutex and Condition Variable for thread-safety */
  pthread_cond_t cv;
  pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size, copy_constructor_type copy_constructor,
                    destructor_type destructor) {
	// Initialization
	struct queue *rtn = malloc(sizeof(struct queue));
	rtn->copy_constructor = (copy_constructor ? copy_constructor : shallow_copy_constructor);
	rtn->destructor = (destructor ? destructor : shallow_destructor);
	rtn->head = NULL;
	rtn->tail = NULL;
	rtn->max_size = max_size;
	rtn->size = 0;

	// pthread related initialization
	pthread_cond_init(&(rtn->cv), NULL);
	pthread_mutex_init(&(rtn->m), NULL);

	return rtn;
}

void queue_destroy(queue *this) {
	// destroy pthread related
	pthread_cond_destroy(&(this->cv));
	pthread_mutex_destroy(&(this->m));

	// free all nodes and their data
	while (this->head != NULL) {
		queue_node *temp = this->head;
		this->head = this->head->next;
		(*this->destructor)(temp->data);
		free(temp);
	}

	// free current queue
	free(this);
}

void queue_push(queue *this, void *data) {
	pthread_mutex_lock(&(this->m));

	// When the size reached max_size after last push, 
	// make current thread wait for another thread to pull 
	if (this->size == this->max_size) {
		while (this->size == this->max_size)
			pthread_cond_wait(&(this->cv), &(this->m));
	} else {
		// pthread to send signal for threads waiting for pulling
		pthread_cond_signal(&(this->cv));
	}
	
	// new node initialization prior to push in
	queue_node *new_node = malloc(sizeof(queue_node));
	new_node->data = (*this->copy_constructor)(data);
	new_node->next = NULL;

	// when size is 0, assign head to the new node
	// else, just place after current tail
	if (!(this->size))
		this->head = new_node;
	else
		this->tail->next = new_node;
	
	// update current tail to the new node
	this->tail = new_node;	
	++(this->size);

	pthread_mutex_unlock(&(this->m));
}

void *queue_pull(queue *this) {
	pthread_mutex_lock(&(this->m));

	// If the size is 0, wait for push to send signal
	// otherwise, send signal to notify push that the queue may push again
	if (this->size) {
		pthread_cond_signal(&(this->cv));
	} else {
		while (!(this->size))
			pthread_cond_wait(&(this->cv), &(this->m));
	}

	// return value
	void *rtn_val = (*this->copy_constructor)(this->head->data);

	// Move each node one place up toward head
	queue_node *temp = this->head;
	this->head = this->head->next;

	// memory management
	(*this->destructor)(temp->data);
	free(temp);

	// Making sure tail doesn't get dereferenced
	if (this->size == 1) { this->tail = NULL; }
	--(this->size);

	pthread_mutex_unlock(&(this->m));

  return rtn_val;
}


// The following is code generated:

queue *char_queue_create() {
  return queue_create(-1, char_copy_constructor, char_destructor);
}
queue *double_queue_create() {
  return queue_create(-1, double_copy_constructor, double_destructor);
}
queue *float_queue_create() {
  return queue_create(-1, float_copy_constructor, float_destructor);
}
queue *int_queue_create() {
  return queue_create(-1, int_copy_constructor, int_destructor);
}
queue *long_queue_create() {
  return queue_create(-1, long_copy_constructor, long_destructor);
}
queue *short_queue_create() {
  return queue_create(-1, short_copy_constructor, short_destructor);
}
queue *unsigned_char_queue_create() {
  return queue_create(-1, unsigned_char_copy_constructor,
                      unsigned_char_destructor);
}
queue *unsigned_int_queue_create() {
  return queue_create(-1, unsigned_int_copy_constructor,
                      unsigned_int_destructor);
}
queue *unsigned_long_queue_create() {
  return queue_create(-1, unsigned_long_copy_constructor,
                      unsigned_long_destructor);
}
queue *unsigned_short_queue_create() {
  return queue_create(-1, unsigned_short_copy_constructor,
                      unsigned_short_destructor);
}
