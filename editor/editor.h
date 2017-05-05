#include "document.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct editor;
typedef struct editor editor;

editor* create_editor_no_file();

editor* create_editor_file(char *filename);

void init_scr(editor *editor);

char get_editor_mode(editor *editor);

int get_curr_x(editor *editor);

int get_curr_y(editor *editor);

void insert_line(int y, char *line);

size_t num_lines();

vector *get_vector_form();

void handle_mode(editor *editor);

void handle_input(editor* editor, int ch, int x, int y);

void move_down();

void move_up();

void move_left();

void move_right();

void print_state(editor *editor);

void print_document(editor *editor);

void cleanup(editor* editor);
