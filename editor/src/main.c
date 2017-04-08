#include "editor.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	editor* wee;
	if(argc > 1)
		wee = create_editor_file(argv[1]);
	else
		wee = create_editor_no_file();

	// initialize curses
	init_scr();
	
	char mode;
	while((mode = get_mode(wee)) != 'q'){
		// do text editor stuff
		int ch = getch();
	}

	refresh();
	endwin();

	return 0;
}
