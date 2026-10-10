#include <graphics.h>
#include <cstdlib>
#include <ctime>
#include "Gomoku.h"
#include "StartMenu.h"
#include "AI.h"
#include "History.h"

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
	srand((unsigned)time(NULL));
	GameState gamestate = MENU;
	initgraph(windowX,windowY,0);
	setbkcolor(EGERGB(238, 196, 126));
	drawMenu();
	History history;
    
	int currentPlayer = 1;
	int localStarter = 1;
    for(;is_run();delay_fps(60)){
    	if(gamestate == AI_GAME_EASY && currentPlayer == 2 && !roundOver && !gameOver){
    		int bestRow,bestCol;
    		if(chooseEasyMove(currentPlayer,bestRow,bestCol)){
    			placeChess(bestRow,bestCol,currentPlayer);
    			history.record(bestRow,bestCol,currentPlayer);
    			if(!checkWin(bestRow,bestCol,currentPlayer)){
    				if(isBoardFull()){
    					roundDraw();
					}else{
						switchPlayer(currentPlayer);
					}
				}
			}
		}else if(gamestate == AI_GAME_NORMAL && currentPlayer == 2 && !roundOver && !gameOver){
    		int bestRow,bestCol;
    		if(chooseNormalMove(currentPlayer,bestRow,bestCol)){
    			placeChess(bestRow,bestCol,currentPlayer);
    			history.record(bestRow,bestCol,currentPlayer);
    			if(!checkWin(bestRow,bestCol,currentPlayer)){
					if(isBoardFull()){
    					roundDraw();
					}else{
						switchPlayer(currentPlayer);
					}
				}
			}
		}else if(gamestate == AI_GAME_HARD && currentPlayer == 2 && !roundOver && !gameOver){
    		int bestRow,bestCol;
    		if(chooseHardMove(currentPlayer,bestRow,bestCol)){
    			placeChess(bestRow,bestCol,currentPlayer);
    			history.record(bestRow,bestCol,currentPlayer);
    			if(!checkWin(bestRow,bestCol,currentPlayer)){
					if(isBoardFull()){
    					roundDraw();
					}else{
						switchPlayer(currentPlayer);
					}
				}
			}
		}
		while(mousemsg()){
			mouse_msg msg = getmouse();
		    if(gamestate == MENU && msg.is_down() && msg.is_left()){
		    	if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					localStarter = 1;
					currentPlayer = localStarter;
					
					gamestate = LOCAL_GAME;
					drawBoard(whiteWins,blackWins);
					drawCurrentTurn(currentPlayer);
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 300 && msg.y <= 350){
					gamestate = AI_GAME_DIFFICULTY;
					drawDiffPanel();
				}
			}else if(gamestate == LOCAL_GAME && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					history.clear();
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("localSave.txt", currentPlayer, whiteWins, blackWins);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					if(history.load("localSave.txt", currentPlayer, whiteWins, blackWins,
					                &roundOver, &gameOver)){
					    for(int row = 0;row < lines;row++){
						    for(int col = 0;col < lines;col++){
							    board[row][col] = 0;
						    }
					    }
					}
					roundOver = false;
					drawBoard(whiteWins, blackWins);
					for(const Step &step : history.getSteps()){
						int player = step.player;
						placeChess(step.row,step.col,player);
					}
					if(history.size() > 0){
						const Step &last = history.getSteps()[history.size() - 1];
						int lastPlayer = last.player;
						if(isWinningMoveOnBoard(board,last.row,last.col,lastPlayer)){
							int savedWhiteWins = whiteWins;
							int savedBlackWins = blackWins;
							checkWin(last.row,last.col,lastPlayer);
							whiteWins = savedWhiteWins;
							blackWins = savedBlackWins;
						}else{
							drawCurrentTurn(currentPlayer);
						}
					}else{
						drawCurrentTurn(currentPlayer);
					}
				}
				if(!roundOver && !gameOver){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						history.record(row,col,currentPlayer);
						if(!checkWin(row,col,currentPlayer)){
							if(isBoardFull()){
    					        roundDraw();
					        }else{
						        switchPlayer(currentPlayer);
					        }
						}
					}
		        }else if(roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
		    		    history.clear();
		    		    localStarter = (localStarter == 1) ? 2 : 1;
		    		    currentPlayer = localStarter;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
		    		    history.clear();
		    		    localStarter = 1;
		    		    currentPlayer = localStarter;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }
			}else if(gamestate == AI_GAME_DIFFICULTY && msg.is_down() && msg.is_left()){
				if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					gamestate = AI_GAME_EASY;
					drawBoard(whiteWins,blackWins);
					drawCurrentTurn(currentPlayer);
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 300 && msg.y <= 350){
					gamestate = AI_GAME_NORMAL;
					currentPlayer = rand() % 2 + 1;
					drawBoard(whiteWins,blackWins);
					drawCurrentTurn(currentPlayer);
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 380 && msg.y <= 430){
					gamestate = AI_GAME_HARD;
					currentPlayer = 2;
					drawBoard(whiteWins,blackWins);
					drawCurrentTurn(currentPlayer);
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
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("easySave.txt", currentPlayer, whiteWins, blackWins);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					if(history.load("easySave.txt", currentPlayer, whiteWins, blackWins,
					                &roundOver, &gameOver)){
					    for(int row = 0;row < lines;row++){
						    for(int col = 0;col < lines;col++){
							    board[row][col] = 0;
						    }
					    }
					}
					roundOver = false;
					drawBoard(whiteWins, blackWins);
					for(const Step &step : history.getSteps()){
						int player = step.player;
						placeChess(step.row,step.col,player);
					}
					if(history.size() > 0){
						const Step &last = history.getSteps()[history.size() - 1];
						int lastPlayer = last.player;
						if(isWinningMoveOnBoard(board,last.row,last.col,lastPlayer)){
							int savedWhiteWins = whiteWins;
							int savedBlackWins = blackWins;
							checkWin(last.row,last.col,lastPlayer);
							whiteWins = savedWhiteWins;
							blackWins = savedBlackWins;
						}else{
							drawCurrentTurn(currentPlayer);
						}
					}else{
						drawCurrentTurn(currentPlayer);
					}
				}
				if(!roundOver && !gameOver && currentPlayer == 1){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						history.record(row,col,currentPlayer);
						if(!checkWin(row,col,currentPlayer)){
							if(isBoardFull()){
    					        roundDraw();
					        }else{
						        switchPlayer(currentPlayer);
					        }
						}
					}
		        }else if(roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
		    		    history.clear();
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
		    		    history.clear();
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }
			}else if(gamestate == AI_GAME_NORMAL && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("normalSave.txt", currentPlayer, whiteWins, blackWins);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					if(history.load("normalSave.txt", currentPlayer, whiteWins, blackWins,
					                &roundOver, &gameOver)){
					    for(int row = 0;row < lines;row++){
						    for(int col = 0;col < lines;col++){
							    board[row][col] = 0;
						    }
					    }
					}
					roundOver = false;
					drawBoard(whiteWins, blackWins);
					for(const Step &step : history.getSteps()){
						int player = step.player;
						placeChess(step.row,step.col,player);
					}
					if(history.size() > 0){
						const Step &last = history.getSteps()[history.size() - 1];
						int lastPlayer = last.player;
						if(isWinningMoveOnBoard(board,last.row,last.col,lastPlayer)){
							int savedWhiteWins = whiteWins;
							int savedBlackWins = blackWins;
							checkWin(last.row,last.col,lastPlayer);
							whiteWins = savedWhiteWins;
							blackWins = savedBlackWins;
						}else{
							drawCurrentTurn(currentPlayer);
						}
					}else{
						drawCurrentTurn(currentPlayer);
					}
				}
				if(!roundOver && !gameOver && currentPlayer == 1){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						history.record(row,col,currentPlayer);
						if(!checkWin(row,col,currentPlayer)){
							if(isBoardFull()){
    					        roundDraw();
					        }else{
						        switchPlayer(currentPlayer);
					        }
						}
					}
		        }else if(roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
		    		    history.clear();
		    	    	currentPlayer = rand() % 2 + 1;
		    	    	drawCurrentTurn(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
		    		    history.clear();
		    		    currentPlayer = rand() % 2 + 1;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }
			}else if(gamestate == AI_GAME_HARD && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("hardSave.txt", currentPlayer, whiteWins, blackWins);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					if(history.load("hardSave.txt", currentPlayer, whiteWins, blackWins,
					                &roundOver, &gameOver)){
					    for(int row = 0;row < lines;row++){
						    for(int col = 0;col < lines;col++){
							    board[row][col] = 0;
						    }
					    }
					}
					roundOver = false;
					drawBoard(whiteWins, blackWins);
					for(const Step &step : history.getSteps()){
						int player = step.player;
						placeChess(step.row,step.col,player);
					}
					if(history.size() > 0){
						const Step &last = history.getSteps()[history.size() - 1];
						int lastPlayer = last.player;
						if(isWinningMoveOnBoard(board,last.row,last.col,lastPlayer)){
							int savedWhiteWins = whiteWins;
							int savedBlackWins = blackWins;
							checkWin(last.row,last.col,lastPlayer);
							whiteWins = savedWhiteWins;
							blackWins = savedBlackWins;
						}else{
							drawCurrentTurn(currentPlayer);
						}
					}else{
						drawCurrentTurn(currentPlayer);
					}
				}
				if(!roundOver && !gameOver && currentPlayer == 1){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						history.record(row,col,currentPlayer);
						if(!checkWin(row,col,currentPlayer)){
							if(isBoardFull()){
    					        roundDraw();
					        }else{
						        switchPlayer(currentPlayer);
					        }
						}
					}
		        }else if(roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
		    		    history.clear();
		    		    currentPlayer = 2;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=420 && msg.y <=470){
		    		    resetGame(currentPlayer);
		    		    history.clear();
		    		    currentPlayer = 2;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }
			}
	    }
	}

    return 0;
}
