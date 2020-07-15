#include "ncursesUtility.h"

#define ESCAPE 27

//カラー番号
//命名規則は前景色_背景色
enum COLOR_CODE{
	WHITE_BLUE=1,
	BLUE_WHITE,
	RED_WHITE
};

static void DrawMenubar(WINDOW *menubar);



void InitCurses(void){
	initscr();
	start_color();
	init_pair(WHITE_BLUE,COLOR_WHITE,COLOR_BLUE);
	init_pair(BLUE_WHITE,COLOR_BLUE,COLOR_WHITE);
	init_pair(RED_WHITE,COLOR_RED,COLOR_WHITE);
	curs_set(0);
	noecho();
	keypad(stdscr,TRUE);
}

static void DrawMenubar(WINDOW *menubar){
	wbkgd(menubar,COLOR_PAIR(BLUE_WHITE));
	waddstr(menubar,"  add");
	wattron(menubar,COLOR_PAIR(RED_WHITE));
	waddstr(menubar,"(F1)");
	wattroff(menubar,COLOR_PAIR(RED_WHITE));	
}


int main(void){
	InitCurses();
	bkgd(COLOR_PAIR(WHITE_BLUE));
	int x,y;
	getmaxyx(stdscr,y,x);
	WINDOW *menubar=subwin(stdscr,1,x,0,0);	
	DrawMenubar(menubar);
	refresh();
	int key;
	do{
		key=getch();
	}while(key!=ESCAPE);
	delwin(menubar);
	endwin();
	return 0;
}

