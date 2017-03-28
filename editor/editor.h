#include "document.h"
#include <ncurses.h>
#include <stdlib.h>

struct editor;
typedef struct editor editor;

editor* create_editor_no_file();

editor* create_editor_file(char *filename);

void init_scr();

void handle_status(editor *editor);

void handle_input(editor* editor, int ch);

void move_down();

void move_up();

void move_left();

void move_right();

void cleanup(editor* editor);
