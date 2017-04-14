/**
 * Machine Problem: Vector
 * CS 241 - Spring 2017
 */

#include "vector.h"
#include <stdio.h>
#include <assert.h>

#define INITIAL_CAPACITY 10
#define GROWTH_FACTOR 2

struct vector {
  /* The function callback for the user to define the way they want to copy
   * elements */
  copy_constructor_type copy_constructor;

  /* The function callback for the user to define the way they want to destroy
   * elements */
  destructor_type destructor;

  /* The function callback for the user to define the way they a default element
   * to be constructed */
  default_constructor_type default_constructor;

  /* Void pointer to the beginning of an array of void pointers to arbitrary
   * data. */
  void **array;

  /**
   * The number of elements in the vector.
   * This is the number of actual objects held in the vector,
   * which is not necessarily equal to its capacity.
   */
  size_t size;

  /**
   * The size of the storage space currently allocated for the vector,
   * expressed in terms of elements.
   */
  size_t capacity;
};

/**
 * IMPLEMENTATION DETAILS
 *
 * The following is documented only in the .c file of vector,
 * since it is implementation specfic and does not concern the user:
 *
 * This vector is defined by the struct above.
 * The struct is complete as is and does not need any modifications.
 *
 * 'INITIAL_CAPACITY' is the macro you should use to set the capacity of the
 * vector in vector_create().
 * 'GROWTH_FACTOR' is how much you should grow your vector by in automatic
 * reallocation.
 *
 * The only conditions of automatic reallocation is that
 * they should happen logarithmically compared to the growth of the size of the
 * vector
 * inorder to achieve amortized constant time complexity for appending to the
 * vector.
 *
 * For our implementation automatic reallocation happens when -and only when-
 * adding to the vector makes its new  size surpass its current vector capacity
 * OR when the user calls on vector_reserve().
 * When this happens the new capacity will be whatever power of the
 * 'GROWTH_FACTOR' greater than or equal
 * to the target capacity.
 * In the case when the new size exceeds the current capacity the target
 * capacity is the new size.
 * In the case when the user calls vector_reserve(n) the target capacity is 'n'
 * itself.
 * We have provided get_new_capacity() to help make this less ambigious.
 */

static size_t get_new_capacity(size_t target) {
  /**
   * This function works according to 'automatic reallocation'.
   * Start at 1 and keep multiplying by the GROWTH_FACTOR untl
   * you have exceeded or met your target capacity.
   */	
	size_t new_capacity = 1;
  while (new_capacity < target) {
    new_capacity *= GROWTH_FACTOR;
  }
  return new_capacity;
}

vector *vector_create(copy_constructor_type copy_constructor,
                      destructor_type destructor,
                      default_constructor_type default_constructor) {	
	vector *rtn_vtr = malloc(sizeof(vector));

	// Assign shallow functions for passed NULL arguments accordingly
	if (copy_constructor == NULL)
		copy_constructor = shallow_copy_constructor;
	
	if (destructor == NULL)
		destructor = shallow_destructor;

	if (default_constructor == NULL)
		default_constructor = shallow_default_constructor;

	// Initialization
	rtn_vtr->copy_constructor = copy_constructor;
	rtn_vtr->destructor = destructor;
	rtn_vtr->default_constructor = default_constructor;
	rtn_vtr->size  = 0;
	rtn_vtr->capacity = INITIAL_CAPACITY;
	rtn_vtr->array = malloc(sizeof(void *) * rtn_vtr->capacity);

  return rtn_vtr;
}

void vector_destroy(vector *this) {
  assert(this);
	
	// Call destructor on every element of the array
	while (this->size > 0) {
		(this->destructor)(*((this->array) + (--(this->size))));
	}

	// Free the array and the vector
	free(this->array);
	free(this);
}

void **vector_begin(vector *this) { return this->array + 0; }

void **vector_end(vector *this) { return this->array + this->size; }

size_t vector_size(vector *this) {
  assert(this);
  
	return this->size;
}

void vector_resize(vector *this, size_t n) {
  assert(this);
	assert(0 <= (int) n);

	size_t curr = this->size;
	if (this->size < n) {
		// When capacity < n, resize its array's memory and push_back default inputs
		// When size < n <= capacity, just push_back default inputs
		
		while (curr < n) {
			*(this->array + curr) = (*(this->default_constructor))();
			(this->size)++;
			
			if (this->size == this->capacity)
				vector_reserve(this, this->capacity + 1);

			curr++;
		}
	} else if (this->size > n) {
		// When n <= size, pop_back as many as difference

		while (curr > n) {
			vector_pop_back(this);
			curr--;
		}
	}
}

size_t vector_capacity(vector *this) {
  assert(this);
  
	return this->capacity;
}

bool vector_empty(vector *this) {
  assert(this);
  
	return (this->size == 0);
}

void vector_reserve(vector *this, size_t n) {
  assert(this);
	assert(0 <= (int) n);

	// Act iff n is greater than its capacity
	if (this->capacity < n) {
		this->capacity = get_new_capacity(n);
		this->array = realloc(this->array, sizeof(void *) * this->capacity);
	}
}

void **vector_at(vector *this, size_t position) {
  assert(this);
	assert(0 <= position && position <= (this->size) - 1);
  
	return ((this->array) + position);
}

void vector_set(vector *this, size_t position, void *element) {
  assert(this);
	
	// Erase the element at the position and insert new element there
	vector_erase(this, position);
	vector_insert(this, position, element);
}

void *vector_get(vector *this, size_t position) {
	return *vector_at(this, position);
}

void **vector_front(vector *this) {
  return vector_begin(this);
}

void **vector_back(vector *this) {
  assert(this);
	assert(this->size > 0);

	// vector_end returns the reference to after the last element
	// so go an element back
	return vector_end(this) - 1;
}

void vector_push_back(vector *this, void *element) {
  assert(this);
	
	// Insert the element at the end
	vector_insert(this, this->size, element); 
}

void vector_pop_back(vector *this) {
  assert(this);
	assert(0 < this->size);

	// Erase the last element
	vector_erase(this, this->size - 1);
}

void vector_insert(vector *this, size_t position, void *element) {
	assert(this && (0 <= position) && (position <= this->size));

	size_t curr = 0;

	if (this->capacity <= this->size) {
		// Need to increase the capacity
		vector_reserve(this, this->capacity + 1);
	}
	
	curr = this->size;

	// Make array elements point to the one before its current position
	while (curr > position) {

		*(this->array + curr) = *(this->array + (curr - 1));
		curr--;		
	}

	// Copy the element at the designated position and increment its size by one
	*(this->array + curr) = (*(this->copy_constructor))(element);
	(this->size)++;
}

void vector_erase(vector *this, size_t position) {
	assert(this && 0 <= position && position < this->size);

	// Call destructor unless it is NULL
	(*(this->destructor))(*vector_at(this, position));
	--(this->size);

	size_t curr = position;

	// Moves the pointer of position + 1 to position
	while (curr < this->size) {
		*(this->array + curr) = *(this->array + curr + 1);
		curr++;
	}

	// Assign NULL to the last element
	*(this->array + curr) = NULL;
}

void vector_clear(vector *this) {
	assert(this);

	// Call pop back until the size equals to 0
	while (this->size > 0)
		vector_pop_back(this);
}

// The following is code generated:

vector *char_vector_create() {
  return vector_create(char_copy_constructor, char_destructor,
                       char_default_constructor);
}
vector *double_vector_create() {
  return vector_create(double_copy_constructor, double_destructor,
                       double_default_constructor);
}
vector *float_vector_create() {
  return vector_create(float_copy_constructor, float_destructor,
                       float_default_constructor);
}
vector *int_vector_create() {
  return vector_create(int_copy_constructor, int_destructor,
                       int_default_constructor);
}
vector *long_vector_create() {
  return vector_create(long_copy_constructor, long_destructor,
                       long_default_constructor);
}
vector *short_vector_create() {
  return vector_create(short_copy_constructor, short_destructor,
                       short_default_constructor);
}
vector *unsigned_char_vector_create() {
  return vector_create(unsigned_char_copy_constructor, unsigned_char_destructor,
                       unsigned_char_default_constructor);
}
vector *unsigned_int_vector_create() {
  return vector_create(unsigned_int_copy_constructor, unsigned_int_destructor,
                       unsigned_int_default_constructor);
}
vector *unsigned_long_vector_create() {
  return vector_create(unsigned_long_copy_constructor, unsigned_long_destructor,
                       unsigned_long_default_constructor);
}
vector *unsigned_short_vector_create() {
  return vector_create(unsigned_short_copy_constructor,
                       unsigned_short_destructor,
                       unsigned_short_default_constructor);
}
