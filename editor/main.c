#include "document.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	editor wee;
	if(argc > 1)
		wee = editor(argv[1]);
	else
		wee = editor();

	// initialize curses
	init_curses();
	
	while(wee.mode() != 'q'){
		// do text editor stuff
		int ch = getch();

	}

	refresh();
	endwin();

	return 0;
}
