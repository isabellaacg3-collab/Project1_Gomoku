#include <graphics.h>
#include "StartMenu.h"

void drawMenu(){
	cleardevice();
	
	setcolor(BLACK);
	
	xyprintf(360,120,"GOMOKU");
	
	rectangle(280,220,520,270);
	xyprintf(300,240,"LOCAL");
	
	rectangle(280,300,520,350);
	xyprintf(300,320,"AI(coming soon)");
	
	rectangle(280,380,520,430);
	xyprintf(300,400,"LAN(coming soon)");
}
