#include "editor.h"

struct editor{
	/* 
	 * d = default
	 * i = insert
	 * q = quit 
	 */
	char mode;
	char *state;
	char *filename;
	int curr_x;
	int curr_y;
	document* doc;
};

editor* create_editor_no_file(){
	editor *editor = malloc(sizeof(editor));
	editor->mode = 'd';
	editor->state = strdup("Default Mode");
	editor->filename = NULL;
	editor->curr_x = 0;
	editor->curr_y = 0;
	editor->doc = document_create();
	return editor;
}

editor* create_editor_file(char *filename){	
	editor *editor = malloc(sizeof(editor));
	editor->mode = 'd';
	editor->state = strdup("Default Mode");
	editor->filename = strdup(filename);
	editor->curr_x = 0;
	editor->curr_y = 0;
	editor->doc = document_create_from_file(filename);
	return editor;
}

void init_scr(){
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
}

char get_mode(editor *editor){
	return editor->mode;
}

/* 
 *	Handles modes (d, i, s)
 */
void handle_mode(editor *editor){
	switch(editor->mode){
		case 'd':
			free(editor->state);
			editor->state = strdup("Default Mode");
			break;
		case 'q':
			cleanup(editor);
			return;
		case 'i':
			free(editor->state);
			editor->state = strdup("Insert Mode");
			break;
		case 's':
			// document_create_from_file() will take care of other case
			if(!editor->filename)
				document_write_to_file(editor->doc, "file.txt");
			break;
	}
}

/**
 *	WORKFLOW
 *
 *	updateStatus()
 *	printStatusLine()
 *	printBuff()
 *	input = getch()
 *	handleInput(input)
 */
void handle_input(editor *editor, int ch){
	const char * str = document_get_line(editor->doc, editor->curr_y + 1);
	char new_char[2];
	*new_char = (char)ch;
	switch(ch){
		case KEY_DOWN:
			move_down(editor);
			return;
		case KEY_UP:
			move_up(editor);
			return;
		case KEY_LEFT:
			move_left(editor);
			return;
		case KEY_RIGHT:
			move_right(editor);
			return;
	}

	switch(editor->mode){
		// i. default mode
		case 'd':
			switch(ch){
				case 'q':
					editor->mode = 'q';
					break;
				case 'i':
					editor->mode = 'i';
					break;
				case 's':
					editor->mode = 's';
					break;
			}
			break;
		case 'i':
			switch(ch){
				// Escape key
				case 27:
					editor->mode = 'n';
					break;
				case KEY_BACKSPACE:
					if(editor->curr_x > 0){
						// copy first half
						char temp[strlen(str) + 1];
						strncpy(temp, str, editor->curr_x - 1);
						temp[editor->curr_x - 1] = '\0';
						// copy second half
						strncat(temp, str + editor->curr_x, strlen(str) - editor->curr_x);
						// insert into document
						document_set_line(editor->doc, editor->curr_y + 1, temp);
						// update x
						editor->curr_x -= 1;
						move(editor->curr_y, editor->curr_x);
					}
					// need to merge with previous line!
					else if(editor->curr_x == 0 && editor->curr_y > 0){
						//	1. curr_line is empty -- which just means document_delete_line
						//	2. curr_line is not empty -- meaning document_insert_line
						// 
						//const char *str = document_get_line(editor->doc, editor->curr_y + 1);
						char *prev_line = strdup(document_get_line(editor->doc, editor->curr_y));
						editor->curr_x = strlen(prev_line) - 1;
						if(strlen(str) > 0){
							// merge
							strcat(prev_line, str);
							// set line
							document_set_line(editor->doc, editor->curr_y, prev_line);
						}
						document_delete_line(editor->doc, editor->curr_y + 1);
						move_up(editor);
						// cleanup
						free(prev_line);
						prev_line = NULL;	
					}
					break;
				case KEY_DC:
					break;
				case KEY_ENTER:
					if(editor->curr_x < (int)strlen(str)){
						// modify original line
						char new_str[strlen(str) + 1];
						strncpy(new_str, str, editor->curr_x + 1);
						document_set_line(editor->doc, editor->curr_y + 1, new_str);
						// insert second part of line
						char cut_str[strlen(str) + 1];
						strncpy(cut_str, str + editor->curr_x + 1, strlen(str) - editor->curr_x - 1);
						document_insert_line(editor->doc, editor->curr_y + 2, cut_str);
					}
					else{
						// just insert
						document_insert_line(editor->doc, editor->curr_y + 2, "");
					}
					editor->curr_x = 0;
					move_down(editor);
					break;
				case 9:
					// Tab
					break;
				default:
					if(editor->curr_x < (int)strlen(str)){
						char new_str[strlen(str) + 2];
						strncpy(new_str, str, editor->curr_x + 1);
						strcat(new_str, new_char);
						strcat(new_str, str + editor->curr_x + 1);
						document_set_line(editor->doc, editor->curr_y + 1, new_str);
					}
					else{
						char append_str[strlen(str) + 2];
						strcpy(append_str, str);
						strcat(append_str, new_char);
						document_set_line(editor->doc, editor->curr_y + 1, append_str);
					}
					editor->curr_x += 1;
					//move(editor->curr_y, editor->curr_x);
					str = NULL;
					break;
			}
			break;
	}
}

void move_down(editor *editor){
	if((editor->curr_y + 1 < LINES - 1) && (editor->curr_y + 1 < (int)document_size(editor->doc))){
		editor->curr_y += 1;
		move(editor->curr_y, editor->curr_x);
	}
}

void move_up(editor *editor){
	// moving up decreases y
	if((editor->curr_y - 1) >= 0){
		editor->curr_y -= 1;
		move(editor->curr_y, editor->curr_x);
	}
}

void move_left(editor *editor){
	if((editor->curr_x - 1) >= 0){
		editor->curr_x -= 1;
		move(editor->curr_y, editor->curr_x);
	}
}

void move_right(editor *editor){
	const char *curr_line = document_get_line(editor->doc, editor->curr_y + 1);
	if((editor->curr_x + 1 < COLS) && (editor->curr_x + 1 < (int)strlen(curr_line))){
		editor->curr_x += 1;
		move(editor->curr_y, editor->curr_x);
	}
	// [cleanup]
	curr_line = NULL;
}

void cleanup(editor *editor){
	document_destroy(editor->doc);
	if(editor){
		if(editor->filename){
			free(editor->filename);
			editor->filename = NULL;
		}
		if(editor->state){
			free(editor->state);
			editor->state = NULL;
		}
		free(editor);
		editor = NULL;
	}
}
