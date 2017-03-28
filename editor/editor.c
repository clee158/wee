#include "editor.h"

struct editor{
	char mode;	/* d = default mode; i = insert mode; q = quit */
	char *filename;
	int curr_x;
	int curr_y;
	Document* doc;
};

editor* create_editor_no_file(){
	editor *editor = malloc(sizeof(editor));
	editor->mode = 'd';
	editor->filename = NULL;
	editor->curr_x = getcurx(stdscr);
	editor->curr_y = getcury(stdscr);
	editor->doc = document_create();
}

editor* create_editor_file(char *filename){	
	editor *editor = malloc(sizeof(editor));
	editor->mode = 'd';
	editor->filename = malloc(strlen(filename) + 1);
	editor->filename = strcpy(editor->filename, filename);
	editor->curr_x = getcurx(stdscr);
	editor->curr_y = getcury(stdscr);
	editor->doc = document_create();
}

void init_scr(){
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
}

void handle_status(editor *editor, int ch){
	
}

void handle_input(editor *editor, int ch){
	switch(ch){
		case(KEY_DOWN):
			move_down(editor);
			return;
		case(KEY_UP):
			move_up(editor);
			return;
		case(KEY_LEFT):
			move_left(editor);
			return;
		case(KEY_RIGHT):
			move_right(editor);
			return;
	}
	switch(editor->mode){
		// i. default mode
		case 'd':
			switch(ch){
				// exit
				case 'x':
				case 'i':
				case 's':
					handle_status(editor, ch);
					break;
			}
			break;
		case 'i':
			switch(ch){
				case 27:
					handle_status(editor, ch);
					break;
				case KEY_BACKSPACE:
					if(editor->curr_x > 0){
						const char *str = document_get_line(editor->doc, x + 1);
						// copy & modify
						char temp[strlen(str) + 1];
						temp = strcpy(temp, str);
						temp[strlen(str) - 1] = '\0';
						// create new string
						const char *new_str = malloc(strlen(str) + 1);
						new_str = strcpy(new_str, temp);
						// insert into document
						document_set_line(editor->doc, x + 1, new_str);
					}
					// need to merge with previous line!
					else if(editor->curr_x == 0 && y > 0){
						/*	1. curr_line is empty -- which just means document_delete_line
						 *	2. curr_line is not empty -- meaning document_insert_line
						 */
					}
					break;
				case KEY_DC:
					break;
				case KEY_ENTER:
					break;
				case 9:
					break;
				default:
					break;
		}
		break;
	}
}

void move_down(editor *editor){
		
}

void move_up(editor *editor){

}

void move_left(editor *editor){
	
}

void move_right(editor *editor){

}

void cleanup(editor *editor){
	document_destroy(editor->doc);
	editor->doc = NULL;
	free(editor);
	editor = NULL;
}
