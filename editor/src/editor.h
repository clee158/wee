#include "document.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct editor;
typedef struct editor editor;

editor* create_editor_no_file();

editor* create_editor_file(char *filename);

void init_scr();

char get_editor_mode(editor *editor);

void handle_mode(editor *editor);

void handle_input(editor* editor, int ch);

void move_down();

void move_up();

void move_left();

void move_right();

void print_state(editor *editor);

void print_document(editor *editor);

void cleanup(editor* editor);
