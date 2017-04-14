#include "editor.h"

document *doc;
//char *filename;
//char *default_mode;
//char *insert_mode;

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
}

char get_editor_mode(editor *editor){
	return 0;
	//return editor->mode;
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

void handle_input(editor *editor, int ch){
	char *str = NULL;
	if(document_size(doc) > 0)
		str = strdup(document_get_line(doc, editor->curr_y + 1));
	char new_char[2];
	*new_char = (char)ch;
	new_char[1] = '\0';
	switch(ch){
		case KEY_DOWN:
			move_down(editor);
			break;
		case KEY_UP:
			move_up(editor);
			break;
		case KEY_LEFT:
			move_left(editor);
			break;
		case KEY_RIGHT:
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
					if(editor->curr_x > 0){
						// copy first half
						char temp[strlen(str) + 1];
						strncpy(temp, str, editor->curr_x - 1);
						temp[editor->curr_x - 1] = '\0';
						// copy second half
						strncat(temp, str + editor->curr_x, strlen(str) - editor->curr_x);
						// insert into document
						document_set_line(doc, editor->curr_y + 1, temp);
						// update x
						editor->curr_x -= 1;
						move(editor->curr_y, editor->curr_x);
					}
					// need to merge with previous line!
					else if(editor->curr_x == 0 && editor->curr_y > 0){
						const char *prev_line = document_get_line(doc, editor->curr_y);
						if(strlen(str) > 0){
							char new_str[strlen(prev_line) + strlen(str) + 1];
							strcpy(new_str, prev_line);
							editor->curr_x = strlen(prev_line);
							// merge
							strcat(new_str, str);
							// set line
							document_set_line(doc, editor->curr_y, new_str);
						}
						else
							editor->curr_x = strlen(prev_line);

						document_delete_line(doc, editor->curr_y + 1);
						editor->curr_y -= 1;
						move(editor->curr_y, editor->curr_x);
					}
					break;
				case KEY_DC:
					// merge with next line
					if(editor->curr_x >= (int)strlen(str) && editor->curr_y + 1 < (int)document_size(doc)){
						// concatenate
						const char *prev_line = document_get_line(doc, editor->curr_y + 2);
						char new_str[strlen(str) + strlen(prev_line) + 1];
						strcpy(new_str, str);
						strcat(new_str, prev_line);
						// set line
						document_set_line(doc, editor->curr_y + 1, new_str);
						// remove next line
						document_delete_line(doc, editor->curr_y + 2);
					}
					else if(strlen(str) > 0 && editor->curr_x < (int)strlen(str)){
						// copy first half
						char new_str[strlen(str)];
						strncpy(new_str, str, editor->curr_x);
						new_str[editor->curr_x] = '\0';
						// copy second half
						strcat(new_str, str + editor->curr_x + 1);
						// set line
						document_set_line(doc, editor->curr_y + 1, new_str);
					}
					break;
				case 10:
				case KEY_ENTER:
					if(editor->curr_x < (int)strlen(str)){
						// modify original line
						char new_str[strlen(str) + 1];
						strncpy(new_str, str, editor->curr_x);
						new_str[editor->curr_x] = '\0';
						document_set_line(doc, editor->curr_y + 1, new_str);
						// insert second part of line
						char cut_str[strlen(str) + 1];
						strncpy(cut_str, str + editor->curr_x, strlen(str) - editor->curr_x);
						cut_str[strlen(str) - editor->curr_x] = '\0';
						document_insert_line(doc, editor->curr_y + 2, cut_str);
					}
					else{
						// just insert
						document_insert_line(doc, editor->curr_y + 2, "");
					}
					editor->curr_x = 0;
					editor->curr_y += 1;
					move(editor->curr_y, editor->curr_x);
					break;
				/* Tab */
				case KEY_BTAB:
				case KEY_CTAB:
				case KEY_STAB:
				case KEY_CATAB:
				case 9:
					// i. simple insert
					if(strlen(str) == 0)
						document_set_line(doc, editor->curr_y + 1, "	");
					// ii. append
					else if(editor->curr_x >= (int)strlen(str)){
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
						document_set_line(doc, editor->curr_y + 1, new_str);
					}
					// iii. insert in middle
					else{
						char new_str[strlen(str) + 5];
						strncpy(new_str, str, editor->curr_x);				
						new_str[editor->curr_x] = '\0';
						int i;
						for(i = 1; i < 5; i++){
							strcat(new_str, " ");
							new_str[editor->curr_x + i] = '\0';
						}
						strcat(new_str, str + editor->curr_x);
						new_str[strlen(new_str)] = '\0';
						document_set_line(doc, editor->curr_y + 1, new_str);
					}
					editor->curr_x += 4;
					break;
				/* Inserting Characters */
				default:
					// i. new file
					if(document_size(doc) < 1){
						document_insert_line(doc, 1, new_char);
					}
					else {
						// ii. insert in middle
						if(editor->curr_x < (int)strlen(str)){
							char new_str[strlen(str) + 2];
							strncpy(new_str, str, editor->curr_x);
							new_str[editor->curr_x] = '\0';
							strcat(new_str, new_char);
							new_str[strlen(new_str)] = '\0';
							strcat(new_str, str + editor->curr_x);
							new_str[strlen(new_str)] = '\0';
							document_set_line(doc, editor->curr_y + 1, new_str);
						}
						// iii. append
						else{
							char append_str[strlen(str) + 2];
							strcpy(append_str, str);
							append_str[strlen(str)] = '\0';
							strcat(append_str, new_char);
							append_str[strlen(str) + 1] = '\0';
							document_set_line(doc, editor->curr_y + 1, append_str);
						}
					}
					editor->curr_x += 1;
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
			&& (editor->curr_x + 1 <= (int)strlen(document_get_line(doc, editor->curr_y + 2)))){	
		editor->curr_y += 1;
		move(editor->curr_y, editor->curr_x);
	}
}

void move_up(editor *editor){
	// moving up decreases y
	if((editor->curr_y - 1) >= 0 && 
			(editor->curr_x + 1 <= (int)strlen(document_get_line(doc, editor->curr_y)))){
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
