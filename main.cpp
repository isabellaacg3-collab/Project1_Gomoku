#include <graphics.h>
#include <cmath>
#include <algorithm>
#include "Gomoku.h"
#include "StartMenu.h"
using namespace std;

int board[lines][lines] = {0};

bool roundOver = false;
bool gameOver = false;

int whiteWins = 0;
int blackWins = 0;

enum GameState{
	MENU,
	LOCAL_GAME,
	AI_GAME,
	LAN_GAME
};

int main(){
	GameState gamestate = MENU;
	initgraph(windowX,windowY,0);
	setbkcolor(EGERGB(238, 196, 126));
	drawMenu();
    
	int currentPlayer = 1;
    for(;is_run();delay_fps(60)){
		while(mousemsg()){
			mouse_msg msg = getmouse();
		    if(gamestate == MENU && msg.is_down() && msg.is_left()){
		    	if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					gamestate = LOCAL_GAME;
					
					cleardevice();
					drawBoard(whiteWins,blackWins);
				}
			}else if(gamestate == LOCAL_GAME){
				if(msg.is_down() && msg.is_left()){
					if(msg.x >= 650 && msg.x <= 780
					&& msg.y >= 580 && msg.y <= 630){
						resetGame(currentPlayer);
						cleardevice();
						gamestate = MENU;
						drawMenu();
					}
				}
				if(!roundOver && !gameOver && msg.is_down() && msg.is_left()){
			        if(msg.x > start - clickDist 
			        && msg.y > start - clickDist
			        && msg.x < start + (lines - 1) * gap + clickDist
			        && msg.y < start + (lines - 1) * gap + clickDist){
				        int col = (msg.x - start + gap/2) / gap;
				        int row = (msg.y - start + gap/2) / gap;
				        int dx = col * gap + start - msg.x;
				        int dy = row * gap + start - msg.y;
				        double Dist = sqrt(dx * dx + dy * dy);
				        if(Dist <= clickDist && board[row][col] == 0){
				    	    board[row][col] = currentPlayer;
				    	
				    	    int count = max({countColor(row,col,0,1,currentPlayer)+countColor(row,col,0,-1,currentPlayer),
				    	    countColor(row,col,1,0,currentPlayer)+countColor(row,col,-1,0,currentPlayer),
				    	    countColor(row,col,1,1,currentPlayer)+countColor(row,col,-1,-1,currentPlayer),
				    	    countColor(row,col,-1,1,currentPlayer)+countColor(row,col,1,-1,currentPlayer),
						    }) + 1;
						    if(count >= 5){
						        roundWin(currentPlayer);
							    nextChess(currentPlayer,row,col);
						    }else if(currentPlayer == 1){
							    xyprintf(650,320, "Black Turn");
				    		    nextChess(currentPlayer,row,col);
						    }else{
							    xyprintf(650,320, "White Turn");
							    nextChess(currentPlayer,row,col);
						    }
				        }
			        }
		        }else if(roundOver && !gameOver && msg.is_down() && msg.is_left()){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
				    }
			    }else if(gameOver && msg.is_down() && msg.is_left()){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
				    }
			    }
			}
	    }
	}

    return 0;
}
