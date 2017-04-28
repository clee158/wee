#pragma once
#include "callbacks.h"
#include <stdbool.h>
#include <stdlib.h>
/**
 * In computer science, a queue (/ˈkjuː/ kyew) is a particular kind of abstract data type or collection in which
 * the entities in the collection are kept in order and the principal (or only) operations on the collection are
 * the addition of entities to the rear terminal position, known as enqueue (push), and
 * removal of entities from the front terminal position, known as dequeue (pop).
 * This makes the queue a First-In-First-Out (FIFO) data structure.
 * In a FIFO data structure, the first element added to the queue will be the first one to be removed.
 * This is equivalent to the requirement that once a new element is added,
 * all elements that were added before have to be removed before the new element can be removed.
 * Often a peek or front operation is also entered, returning the value of the front element without dequeuing it.
 * A queue is an example of a linear data structure, or more abstractly a sequential collection.
 *
 * https://en.wikipedia.org/wiki/Queue_(abstract_data_type)
 *
 * This queue also had the added benefit of being thread-safe.
 * https://en.wikipedia.org/wiki/Thread_safety
 */


/**
 * Struct representing a queue
 */
typedef struct queue queue;

/**
 * Note:
 * queue_empty(), queue_size(), and queue_front() are not provided with the thread-safe version of this queue.
 * Those values are owned by the queue and can not be passed off to any individual thread.
 * Therefore it is impossible to expose them in a thread-safe manner.
 *
 * Consider the following timeline of two threads with a brand new queue:
 *
 *   time |            thread 1            |         thread 2
 * --------------------------------------------------------------------
 *    0  | size_t size = queue_size(queue);|
 *    1  |                                 |  queue_push(queue, blah);
 *    2  | if(size == 0) {do_something()}; |
 *
 * Notice that do_something will execute when the queue in fact has 1 element (not 0).
 * This is a race condition and is caused even if queue_size() is implemented to be
 * thread-safe.
 *
 * A similiar argument can be made for queue_empty() and queue_front().
 */

/**
 * Allocate and return a pointer to a new queue (on the heap).
 *
 * 'max_size' determines how many elements the user can add to the queue before it blocks.
 * If non-positive, the queue will never block upon a push (the queue does not have a 'max_size').
 * Else, the queue will block if the user tries to push when the queue is full.
 *
 * If you would like to make 'shallow' copies of the elements of the 'queue',
 * then you may pass in NULL for the parameters (ex. queue_create(-1, NULL, NULL)).
 * This means that everytime an element is to be copied or removed from the 'queue'
 * the pointer to that element is copied or removed instead of using the user
 * supplied copy constructor and destructor.
 */
queue* queue_create (ssize_t max_size, copy_constructor_type copy_constructor, destructor_type destructor);

/**
 * Destroys all queue elements by
 * calling on the user provided destructor for every element,
 * and deallocates all the storage capacity allocated by the 'queue'.
*/
void queue_destroy (queue* this);

/**
 * Adds a new 'element' to the end of the queue in constant time.
 * Note: Can be called by multiple threads.
 * Note: Blocks if the queue is full (defined by it's max_size).
 */
void queue_push (queue* this, void* element);

/**
 * Removes the element at the front of the queue in constant time.
 * Note: Can be called by multiple threads.
 * Note: Blocks if the queue is empty.
 */
void* queue_pull (queue* this);


// The following is code generated:
/**
 * Creates a queue meant for char(s).
 */
queue *char_queue_create();

/**
 * Creates a queue meant for double(s).
 */
queue *double_queue_create();

/**
 * Creates a queue meant for float(s).
 */
queue *float_queue_create();

/**
 * Creates a queue meant for int(s).
 */
queue *int_queue_create();

/**
 * Creates a queue meant for long(s).
 */
queue *long_queue_create();

/**
 * Creates a queue meant for short(s).
 */
queue *short_queue_create();

/**
 * Creates a queue meant for unsigned char(s).
 */
queue *unsigned_char_queue_create();

/**
 * Creates a queue meant for unsigned int(s).
 */
queue *unsigned_int_queue_create();

/**
 * Creates a queue meant for unsigned long(s).
 */
queue *unsigned_long_queue_create();

/**
 * Creates a queue meant for unsigned short(s).
 */
queue *unsigned_short_queue_create();
