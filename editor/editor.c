#include "editor.h"

document *doc;
//char *filename;
//char *default_mode;
//char *insert_mode;
int _x, _y, curr_x, curr_y;
struct editor{
	/* 
	 * d = default
	 * i = insert
	 * q = quit 
	 */
	//char mode;
	//char *state;
	char *filename;
	int curr_x;
	int curr_y;
	//document* doc;
};

editor* create_editor_no_file(){
	// global constants
	doc = document_create();
	/*default_mode = malloc(sizeof(15));
	strcpy(default_mode, "Default Mode\0");
	insert_mode = malloc(sizeof(14));
	strcpy(insert_mode, "Insert Mode\0");*/

	// editor struct
	editor *editor = malloc(sizeof(editor));
	//editor->mode = 'd';
	//editor->state = default_mode;
	editor->filename = NULL;
	editor->curr_x = 0;
	editor->curr_y = 0;
	return editor;
}

editor* create_editor_file(char *filename){	
	// global constants
	doc = document_create();
	//filename = strdup(filename);
	/*default_mode = malloc(sizeof(13));
	strcpy(default_mode, "Default Mode");
	insert_mode = malloc(sizeof(12));
	strcpy(insert_mode, "Insert Mode");*/

	// editor struct
	editor *editor = malloc(sizeof(editor));
	//editor->mode = 'd';
	//editor->state = default_mode;
	editor->filename = strdup(filename);
	editor->curr_x = 0;
	editor->curr_y = 0;
	return editor;
}

void init_scr(){
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	scrollok(stdscr, FALSE);
	idlok(stdscr, TRUE);
}

char get_editor_mode(editor *editor){
	return 0;
	//return editor->mode;
}


int get_curr_x(editor *editor){
	return editor->curr_x;
}

int get_curr_y(editor *editor){
	return editor->curr_y;
}

void insert_line(int y, char *line){
	document_insert_line(doc, y + 1, line);
}

size_t num_lines(){
	return document_size(doc);
}

vector *get_vector_form(){
	return document_to_vector(doc);
}
/* 
 *	Handles modes (d, i, s)
 */
void handle_mode(editor *editor){
	/*switch(editor->mode){
		case 'q':
			if(editor->filename)
				document_write_to_file(doc, editor->filename);
			cleanup(editor);
			return;
		case 's':
			if(!editor->filename)
				document_write_to_file(doc, "file.txt");
			else
				document_write_to_file(doc, editor->filename);
			return;
	}*/
}

void handle_input(editor *editor, int ch, int x, int y){
	char *str = NULL;
	if(document_size(doc) > 0 && y < document_size(doc))
		str = strdup(document_get_line(doc, y + 1));
	char new_char[2];
	*new_char = (char)ch;
	new_char[1] = '\0';

	///// FOR TESTING! /////
	/*_x = x;
	_y = y;
	curr_x = editor->curr_x;
	curr_y = editor->curr_y;*/
	////////////////////////
	switch(ch){
		case KEY_DOWN:
			if(x == editor->curr_x && y == editor->curr_y)
				move_down(editor);
			break;
		case KEY_UP:
			if(x == editor->curr_x && y == editor->curr_y)
				move_up(editor);
			break;
		case KEY_LEFT:
			if(x == editor->curr_x && y == editor->curr_y)
				move_left(editor);
			break;
		case KEY_RIGHT:
			if(x == editor->curr_x && y == editor->curr_y)
				move_right(editor);
			break;
	/*}

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
			switch(ch){*/
				// Escape key
				case 27:
					if(editor->filename)
						document_write_to_file(doc, editor->filename);
					cleanup(editor);
					//editor->mode = 'd';
					break;
				case KEY_BACKSPACE:
					// i. normal backspace
					if(str && x > 0){
						// copy first half
						char temp[strlen(str) + 1];
						strncpy(temp, str, x - 1);
						temp[x - 1] = '\0';
						// copy second half
						strncat(temp, str + x, strlen(str) - x);
						// insert into document
						document_set_line(doc, y + 1, temp);
						// update curr_x
						if(editor->curr_y == y && ((editor->curr_x >= x) || 
									(editor->curr_x + 1 == x && editor->curr_x >= strlen(str) - 1)))
							editor->curr_x -= 1;
					}
					// ii. need to merge with previous line!
					else if(str && x == 0 && y > 0){
						const char *prev_line = document_get_line(doc, y);
						if(strlen(str) > 0){
							char new_str[strlen(prev_line) + strlen(str) + 1];
							strcpy(new_str, prev_line);
							// merge
							strcat(new_str, str);
							// set line
							document_set_line(doc, y, new_str);
						}

						if(editor->curr_y == y){
							if(editor->curr_x == x)
								editor->curr_x = strlen(prev_line);
							else if(editor->curr_x > x)
								editor->curr_x += strlen(prev_line);
						}
						document_delete_line(doc, y + 1);
						if(editor->curr_y >= y)
							editor->curr_y -= 1;
					}
					move(editor->curr_y, editor->curr_x);
					break;
				case KEY_DC:
					// i. merge with next line
					if(str && x >= (int)strlen(str) && y + 1 < (int)document_size(doc)){
						// concatenate
						const char *next_line = document_get_line(doc, y + 2);
						char new_str[strlen(str) + strlen(next_line) + 1];
						strcpy(new_str, str);
						strcat(new_str, next_line);
						// set line
						document_set_line(doc, y + 1, new_str);
						// update coords
						if(editor->curr_y == y + 1){
							editor->curr_x += strlen(str);
							editor->curr_y = y;
						}
						// remove next line
						document_delete_line(doc, y + 2);
					}
					// ii. just delete
					else if(str && strlen(str) > 0 && x < (int)strlen(str)){
						// copy first half
						char new_str[strlen(str)];
						strncpy(new_str, str, x);
						new_str[x] = '\0';
						// copy second half
						strcat(new_str, str + x + 1);
						// set line
						document_set_line(doc, y + 1, new_str);
						// update coords
						if(editor->curr_y == y && editor->curr_x > x)
							editor->curr_x -= 1;
					}
					break;
				case 10:
				case KEY_ENTER:
					// i. non-empty line
					if(str && x < (int)strlen(str)){
						// modify original line
						char new_str[strlen(str) + 1];
						strncpy(new_str, str, x);
						new_str[x] = '\0';
						document_set_line(doc, y + 1, new_str);
						// insert second part of line
						char cut_str[strlen(str) + 1];
						strncpy(cut_str, str + x, strlen(str) - x);
						cut_str[strlen(str) - x] = '\0';
						document_insert_line(doc, y + 2, cut_str);
					}
					// i. empty line
					else if(str){
						// just insert
						document_insert_line(doc, y + 2, "");
					}
					if(str){
						if(editor->curr_y == y){
							// non-empty line
							if(editor->curr_x >= x){
								editor->curr_x -= strlen(document_get_line(doc, y + 1));
								editor->curr_y += 1;
							}
							// empty line
							else if(editor->curr_x == x){
								editor->curr_x = 0;
								editor->curr_y += 1;
							}
						}
						else if(editor->curr_y > y)
							editor->curr_y += 1;
					}
					move(editor->curr_y, editor->curr_x);
					break;
				/* Tab */
				case KEY_BTAB:
				case KEY_CTAB:
				case KEY_STAB:
				case KEY_CATAB:
				case 9:
					// i. append at end
					if(str && x >= (int)strlen(str)){
						// transfer old string
						char new_str[strlen(str) + 5];
						strcpy(new_str, str);
						new_str[strlen(str)] = '\0';
						// add tab
						int i;
						/*
						 *	Issue: was adding actual tab character, but system recognizes it as
						 *	one char, while I need to move by 4 chars -- misalignment with curser
						 */
						for(i = 1; i < 5; i++){
							strcat(new_str, " ");
							new_str[strlen(str) + i] = '\0';
						}
						document_set_line(doc, y + 1, new_str);
						if(editor->curr_x == x && editor->curr_y == y)
							editor->curr_x += 4;
					}
					// ii. insert in middle
					else if(str){
						char new_str[strlen(str) + 5];
						strncpy(new_str, str, x);				
						new_str[x] = '\0';
						int i;
						for(i = 1; i < 5; i++){
							strcat(new_str, " ");
							new_str[x + i] = '\0';
						}
						strcat(new_str, str + x);
						new_str[strlen(new_str)] = '\0';
						document_set_line(doc, y + 1, new_str);
						if(editor->curr_x >= x && editor->curr_y == y)
							editor->curr_x += 4;
					}
					break;
				/* Inserting Characters */
				default:
					// i. new file
					if(document_size(doc) < 1){
						document_insert_line(doc, 1, new_char);
						editor->curr_x += 1;
					}
					else {
						// ii. insert in middle
						if(str && x < (int)strlen(str)){
							// first half
							char new_str[strlen(str) + 2];
							strncpy(new_str, str, x);
							new_str[x] = '\0';
							// second half
							strcat(new_str, new_char);
							new_str[strlen(new_str)] = '\0';
							strcat(new_str, str + x);
							new_str[strlen(new_str)] = '\0';
							// set line
							document_set_line(doc, y + 1, new_str);
							// update coords
							if(editor->curr_x >= x && editor->curr_y == y)
								editor->curr_x += 1;
						}
						// iii. append
						else if(str){
							// get string
							char append_str[strlen(str) + 2];
							strcpy(append_str, str);
							append_str[strlen(str)] = '\0';
							// add char
							strcat(append_str, new_char);
							append_str[strlen(str) + 1] = '\0';
							// set line
							document_set_line(doc, y + 1, append_str);
							// update coords
							
		/*TODO: Fix the misalignment here!! */
							
							if(editor->curr_x == x && editor->curr_y == y){
								editor->curr_x += 1;
							}
						}
					}
					move(editor->curr_y, editor->curr_x);
					break;
			//}
			//break;
	}
	if(str){
		free(str);
		str = NULL;
	}
}

void move_down(editor *editor){
	//if((editor->curr_y + 1 < LINES - 1) && (editor->curr_y + 1 < (int)document_size(doc))){
	if(editor->curr_y + 1 < (int)document_size(doc) 
			&& (editor->curr_x <= (int)strlen(document_get_line(doc, editor->curr_y + 2)))){	
		editor->curr_y += 1;
		move(editor->curr_y, editor->curr_x);
	}
}

void move_up(editor *editor){
	// moving up decreases y
	if((editor->curr_y - 1) >= 0 && 
			(editor->curr_x <= (int)strlen(document_get_line(doc, editor->curr_y)))){
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
	const char *curr_line = document_get_line(doc, editor->curr_y + 1);
	//if((editor->curr_x + 1 < COLS) && (editor->curr_x + 1 < (int)strlen(curr_line))){
	if(editor->curr_x + 1 < (int)strlen(curr_line) + 1) {	
		editor->curr_x += 1;
		move(editor->curr_y, editor->curr_x);
	}
}

void print_state(editor *editor){
	/*attron(A_REVERSE);
	mvprintw(LINES - 1, 0, editor->state);
	clrtoeol();
	attroff(A_REVERSE);*/
}

void print_document(editor *editor){
	size_t i;
	for(i = 0; i < (size_t)LINES - 1; i++){
		// clear previous written junk from window
		if(i >= document_size(doc))
			move(i, 0);
		else 
			mvprintw(i, 0, document_get_line(doc, i + 1));
		clrtoeol();
	}
	move(editor->curr_y, editor->curr_x);
	/*mvprintw(document_size(doc), 0, "x: %d, y: %d\n", _x, _y);
	mvprintw(document_size(doc) + 1, 0, "[ BEFORE ] curr_x: %d, curr_y: %d\n", curr_x, curr_y);
	mvprintw(document_size(doc) + 2, 0, "[ AFTER ] curr_x: %d, curr_y: %d\n", 
				editor->curr_x, editor->curr_y);*/
	refresh();
}

void cleanup(editor *editor){
	if(doc)
		document_destroy(doc);
	/*if(filename){
		free(filename);
		filename = NULL;
	}*/
	/*if(default_mode){
		free(default_mode);
		default_mode = NULL;
	}
	if(insert_mode){
		free(insert_mode);
		insert_mode = NULL;
	}*/
	if(editor){
		if(editor->filename){
			free(editor->filename);
			editor->filename = NULL;
		}
		free(editor);
		editor = NULL;
	}
}
