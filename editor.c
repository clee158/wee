#include <stdio.h>
#include <ncurses.h>

#define DOWN 1
#define UP 2
#define LEFT 3
#define RIGHT 4

int x, y;
int min_x, max_x;
int min_y, max_y;

int check_valid_coords(int position){
	switch(position){
		case DOWN:
			if(x >= min_x && x <= max_x && y < max_y && y >= min_y)
				return 1;
			return 0;
		case UP:
			if(x >= min_x && x <= max_x && y <= max_y && y > min_y)
				return 1;
			return 0;
		case LEFT:
			if(x > min_x && x <= max_x && y <= max_y && y >= min_y)
				return 1;
			return 0;
		case RIGHT:
			if(x >= min_x && x < max_x && y <= max_y && y >= min_y)
				return 1;
			return 0;
	}
	return 0;
}

int main(){
	
	/* 
	 * Determines terminal type & initializes all curses data structures.
	 * Makes the first call to refresh() to clear the screen.
	 * Returns a pointer to stdscr.
	 */
	initscr();

	/* Set up x and y corrdinates */
	int ch;
	min_x = getbegx(stdscr); max_x = getmaxx(stdscr);
	min_y = getbegy(stdscr); max_y = getmaxy(stdscr);
	
	keypad(stdscr, TRUE);
	/* TO-DO:
	 *
	 * 1. backspacing at beginning of a line -- merge or delete (consider using vector?)
	 * 2. moving cursor onto an already written line
	 *
	 */
	while((ch = getch()) != EOF){
		x = getcurx(stdscr);
		y = getcury(stdscr);
		// newline
		if(ch == 10)
			printf("\n");
		// backspace and delete
		else if(ch == KEY_BACKSPACE || ch == KEY_DC)
			delch();
		// arrow movement
		else if(ch == KEY_DOWN){
			if(check_valid_coords(DOWN))
				move(y + 1, x);
		}
		else if(ch == KEY_UP){
			if(check_valid_coords(UP))
				move(y - 1, x);
		}
		else if(ch == KEY_LEFT){
			if(check_valid_coords(LEFT))
				move(y, x - 1);
			/*else if(x == min_x && y > min_y)
				move(y - 1, );*/
		}
		else if(ch == KEY_RIGHT){
			if(check_valid_coords(RIGHT))
				move(y, x + 1);
		}
		refresh();
	}
	endwin();
	
	return 0;
}
