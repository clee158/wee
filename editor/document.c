/**
 * Machine Problem: Vector
 * CS 241 - Spring 2017
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "document.h"
#include "vector.h"

struct document {
  vector *vector;
};

// This is the constructor function for string element.
// Use this as copy_constructor callback in vector.
void *string_copy_constructor(void *elem) {
  char *str = elem;
  assert(str);
  return strdup(str);
}

// This is the destructor function for string element.
// Use this as destructor callback in vector.
void string_destructor(void *elem) { free(elem); }

// This is the default constructor function for string element.
// Use this as a default constructor callback in vector.
void *string_default_constructor(void) {
  // A single null byte
  return calloc(1, sizeof(char));
}

document *document_create() {
  // Initialize document with its functions
	document *doc = malloc(sizeof(document));
	doc->vector = vector_create(&string_copy_constructor, &string_destructor,
																&string_default_constructor);
  return doc;
}

vector *document_to_vector(document *this){
	return this->vector;
}

void document_write_to_file(document *this, const char *path_to_file) {
  assert(this);
  assert(path_to_file);
	
	// Open or create the file to edit
	FILE *fp = fopen(path_to_file, "w");
	size_t curr = 0;

	// Print out all the elements into the file
	while (curr < document_size(this)) {
		fprintf(fp, "%s\n", vector_get(this->vector, curr));
		curr++;
	}

	fclose(fp);
}

document *document_create_from_file(const char *path_to_file) {
  assert(path_to_file);
  
	// Open file for reading purpose
	FILE *fp;
	fp = fopen(path_to_file, "r");
	document *doc = document_create();
	
	if (fp != NULL) {
		char c;
		char *new_line = malloc(2);
		size_t buffer = 2;
		size_t count = 0;

		// Realloc when new_line increases
		// Free and malloc after adding to vector
		while ((c = (char) fgetc(fp)) != EOF) {
			if (c == '\n') {
				// Put it into the vector as a string element
				*(new_line + count) = '\0';
				vector_push_back(doc->vector, new_line);
				memset(new_line, 0, buffer);
				count = 0;
			} else {
				if (count + 1 >= buffer) {
					new_line = realloc(new_line, buffer * 2);
					buffer = buffer * 2;
				}

				*(new_line + count) = c;
				count++;
			}
		}

		free(new_line);
		new_line = NULL;
		
		fclose(fp);
	}

	return doc;
}

void document_destroy(document *this) {
  assert(this);
  vector_destroy(this->vector);
  free(this);
}

size_t document_size(document *this) {
  assert(this);
  return vector_size(this->vector);
}

void document_set_line(document *this, size_t line_number, const char *str) {
  assert(this);
  assert(str);
  size_t index = line_number - 1;
  vector_set(this->vector, index, (void *)str);
}

const char *document_get_line(document *this, size_t line_number) {
  assert(this);
  assert(line_number > 0);
  size_t index = line_number - 1;
  return (const char *)vector_get(this->vector, index);
}

void document_insert_line(document *this, size_t line_number, const char *str) {
  assert(this);
  assert(str);
	assert(line_number > 0);
	char *str_input;
	
	if (line_number <= document_size(this)) {
		str_input = strdup(str);
		vector_insert(this->vector, line_number - 1, str_input);
	} else {
		// When line number is greater than its size
		size_t counter = line_number - document_size(this) - 1;	
		str_input = string_default_constructor();
		
		while (counter > 0) {
			vector_push_back(this->vector, str_input);
			counter--;
		}

		free(str_input);
		str_input = strdup(str);
		vector_push_back(this->vector, str_input);
	}
	free(str_input);
}

void document_delete_line(document *this, size_t line_number) {
  assert(this);
  assert(line_number > 0);
  size_t index = line_number - 1;
  vector_erase(this->vector, index);
}
