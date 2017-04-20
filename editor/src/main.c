#include "editor.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

editor *wee;

void handle_resize(int signal){
	endwin();
	refresh();
	clear();

	print_document(wee);
	refresh();
}

int main(int argc, char* argv[]){
	if(argc > 1)
		wee = create_editor_file(argv[1]);
	else
		wee = create_editor_no_file();

	init_scr();
	
	struct sigaction signal;
	memset(&signal, 0, sizeof(struct sigaction));
	signal.sa_handler = handle_resize;
	sigaction(SIGWINCH, &signal, NULL);

	int ch = 0;
	//char mode = 0;
	while(ch != 27){
		//handle_mode(wee);
		ch = getch();
		handle_input(wee, ch);
		if(ch != 27)
			print_document(wee);
	}

	refresh();
	endwin();

	return 0;
}
