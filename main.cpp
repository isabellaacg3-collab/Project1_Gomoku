#include <graphics.h>
#include "Gomoku.h"
#include "StartMenu.h"
#include "AI.h"

int board[lines][lines] = {0};

bool roundOver = false;
bool gameOver = false;

int whiteWins = 0;
int blackWins = 0;

enum GameState{
	MENU,
	LOCAL_GAME,
	AI_GAME_EASY,
	AI_GAME_NORMAL,
	AI_GAME_HARD,
	AI_GAME_DIFFICULTY,
	LAN_GAME
};

int main(){
	GameState gamestate = MENU;
	initgraph(windowX,windowY,0);
	setbkcolor(EGERGB(238, 196, 126));
	drawMenu();
    
	int currentPlayer = 1;
    for(;is_run();delay_fps(60)){
    	if(gamestate == AI_GAME_EASY && currentPlayer == 2 && !roundOver && !gameOver){
    		int bestRow,bestCol;
    		if(chooseEasyMove(currentPlayer,bestRow,bestCol)){
    			placeChess(bestRow,bestCol,currentPlayer);
    			if(!checkWin(bestRow,bestCol,currentPlayer)){
					switchPlayer(currentPlayer);
				}
			}
		}
		while(mousemsg()){
			mouse_msg msg = getmouse();
		    if(gamestate == MENU && msg.is_down() && msg.is_left()){
		    	if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					gamestate = LOCAL_GAME;
					drawBoard(whiteWins,blackWins);
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 300 && msg.y <= 350){
					gamestate = AI_GAME_DIFFICULTY;
					drawDiffPanel();
				}
			}else if(gamestate == LOCAL_GAME && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					gamestate = MENU;
					drawMenu();
				}
				if(!roundOver && !gameOver){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						if(!checkWin(row,col,currentPlayer)){
							switchPlayer(currentPlayer);
						}
					}
		        }else if(roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
				    }
			    }
			}else if(gamestate == AI_GAME_DIFFICULTY && msg.is_down() && msg.is_left()){
				if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					gamestate = AI_GAME_EASY;
					drawBoard(whiteWins,blackWins);
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 300 && msg.y <= 350){
					gamestate = AI_GAME_NORMAL;
					drawBoard(whiteWins,blackWins);
					//normalAIGame();
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 380 && msg.y <= 430){
					gamestate = AI_GAME_HARD;
					drawBoard(whiteWins,blackWins);
					//hardAIGame();
				}else if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					gamestate = MENU;
					drawMenu();
				}
			}else if(gamestate == AI_GAME_EASY && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					gamestate = MENU;
					drawMenu();
				}
				if(!roundOver && !gameOver && currentPlayer == 1){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						if(!checkWin(row,col,currentPlayer)){
							switchPlayer(currentPlayer);
						}
					}
		        }else if(roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
				    }
			    }
			}
	    }
	}

    return 0;
}
