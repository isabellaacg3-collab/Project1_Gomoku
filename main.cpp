#include <graphics.h>
#include <cstdlib>
#include <ctime>
#include "Gomoku.h"
#include "StartMenu.h"
#include "AI.h"
#include "History.h"
#include "Network.h"

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
	LAN_GAME,
	LAN_GAME_PANEL
};

void loadGame(History &history,const char *filename,int &currentPlayer,int *localStarter = NULL,bool *undoLocked = NULL){
	if(!history.load(filename,currentPlayer,whiteWins,blackWins,&roundOver,&gameOver,undoLocked)){
		return;
	}
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			board[row][col] = 0;
		}
	}
	drawBoard(whiteWins,blackWins);
	for(const Step &step : history.getSteps()){
		int player = step.player;
		placeChess(step.row,step.col,player);
	}
	if(localStarter){
		*localStarter = history.size() > 0 ? history.getSteps()[0].player : currentPlayer;
	}
	int winner = 0;
	if(history.size() > 0){
		const Step &last = history.getSteps().back();
		if(isWinningMove(last.row,last.col,last.player)){
			winner = last.player;
		}
		currentPlayer = roundOver ? last.player : 3 - last.player;
	}
	if(gameOver){
		gameWin(whiteWins >= winLimit ? 1 : 2);
	}else if(winner){
		xyprintf(670,190,winner == 1 ? "White Win!" : "Black Win!");
		rectangle(650,220,780,270);
		xyprintf(670,240,"Next Round");
	}else if(roundOver){
		roundDraw();
	}else{
		drawCurrentTurn(currentPlayer);
	}
}

int main(){
	srand((unsigned)time(NULL));
	GameState gamestate = MENU;
	initgraph(windowX,windowY,0);
	setbkcolor(EGERGB(238, 196, 126));
	drawMenu();
	History history;
	
	if(!initNetwork()){
	    return 1;
	}
    
	bool localUndoLocked = true;
	bool isLanHost = false;
	bool lanConnected = false;
	
	int currentPlayer = 1;
	int localStarter = 1;
	int lanStarter = 1;
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
		}else if(gamestate == LAN_GAME_PANEL && isLanHost && !lanConnected){
		    if(checkForClient()){
			    lanConnected = true;
			    currentPlayer = 1;
			    history.clear();
			    gamestate = LAN_GAME;
			    drawBoard(whiteWins, blackWins);
			    drawCurrentTurn(currentPlayer);
            }
        }else if(gamestate == LAN_GAME && lanConnected && !isLanHost){
		    char buffer[128];
			bool disconnected = false;
		    if(receiveMessage(buffer,sizeof(buffer),disconnected)){
		    	int row,col,player;
		    	int starter;
		    	if(sscanf(buffer, "MOVE %d %d %d", &row, &col, &player) == 3){
		    		placeChess(row,col,player);
		    		history.record(row,col,player);
		    		if(!checkWin(row,col,player)){
						if(isBoardFull()){
    					    roundDraw();
					    }else{
						    currentPlayer = 3 - player;
						    drawCurrentTurn(currentPlayer);
					    }
					}
				}else if(sscanf(buffer, "NEXT_ROUND %d", &starter) == 1){
					resetRound(currentPlayer);
					history.clear();
					currentPlayer = starter;
					drawCurrentTurn(currentPlayer);
				}else if(sscanf(buffer, "RESET_GAME %d", &starter) == 1){
					resetGame(currentPlayer);
					history.clear();
					currentPlayer = starter;
					drawCurrentTurn(currentPlayer);
				}
			}
			if(disconnected){
			    lanConnected = false;
                isLanHost = false;
				resetGame(currentPlayer);
				history.clear();
				gamestate = LAN_GAME_PANEL;
				drawLanPanel();
				xyprintf(280, 500, "Connection lost.");
			}
        }else if(gamestate == LAN_GAME && lanConnected && isLanHost){
		    char buffer[128];
		    bool disconnected = false;
		    if(receiveMessage(buffer,sizeof(buffer),disconnected)){
		    	int row,col;
		    	if(sscanf(buffer, "REQUEST_MOVE %d %d", &row, &col) == 2){
		    		if(currentPlayer == 2 && !roundOver && !gameOver 
					&& row >= 0 && row < lines && col >= 0 && col < lines && board[row][col] == 0){
						int player = 2;
		    		    placeChess(row,col,player);
		    		    history.record(row,col,player);
		    		    char message[64];
		    		    sprintf(message, "MOVE %d %d %d", row, col, player);
		    		    sendMessage(message);
		    		    if(!checkWin(row,col,player)){
						    if(isBoardFull()){
    					        roundDraw();
					        }else{
						        currentPlayer = 1;
						        drawCurrentTurn(currentPlayer);
						    }
					    }
					}
				}
			}
			if(disconnected){
			    lanConnected = false;
                isLanHost = false;
				resetGame(currentPlayer);
				history.clear();
				gamestate = LAN_GAME_PANEL;
				drawLanPanel();
				xyprintf(280, 500, "Connection lost.");
			}
        }
		while(mousemsg()){
			mouse_msg msg = getmouse();
		    if(gamestate == MENU && msg.is_down() && msg.is_left()){
		    	if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					localStarter = 1;
					localUndoLocked = true;
					currentPlayer = localStarter;
					
					gamestate = LOCAL_GAME;
					drawBoard(whiteWins,blackWins);
					drawCurrentTurn(currentPlayer);
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 300 && msg.y <= 350){
					gamestate = AI_GAME_DIFFICULTY;
					drawDiffPanel();
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 380 && msg.y <= 430){
					gamestate = LAN_GAME_PANEL;
					drawLanPanel();
				}
			}else if(gamestate == LOCAL_GAME && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					history.clear();
					localUndoLocked = true;
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("localSave.txt", currentPlayer, whiteWins, blackWins, roundOver, gameOver, localUndoLocked);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					loadGame(history,"localSave.txt",currentPlayer,&localStarter,&localUndoLocked);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 405 && msg.y <= 430){
					if(history.size() != 0 && !roundOver && !gameOver && !localUndoLocked){
						Step removed;
						if(history.popLast(removed)){
							board[removed.row][removed.col] = 0;
							currentPlayer = removed.player;
							localUndoLocked = true;
							drawBoard(whiteWins,blackWins);
							for(const Step &step : history.getSteps()){
								int player = step.player;
								placeChess(step.row,step.col,player);
							}
							drawCurrentTurn(currentPlayer);
						}
					}
				}
				if(!roundOver && !gameOver){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						localUndoLocked = false;
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
						localUndoLocked = true;
		    		    localStarter = (localStarter == 1) ? 2 : 1;
		    		    currentPlayer = localStarter;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }else if(gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=470 && msg.y <=520){
		    		    resetGame(currentPlayer);
		    		    history.clear();
						localUndoLocked = true;
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
			}else if(gamestate == LAN_GAME_PANEL && msg.is_down() && msg.is_left()){
				if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 220 && msg.y <= 270){
					if(!isLanHost && startHost(11451)){
						isLanHost = true;
						lanConnected = false;
						xyprintf(280, 500, "Waiting for Client...");
					}
				}else if(msg.x >= 280 && msg.x <= 520
				&& msg.y >= 300 && msg.y <= 350){
					if(connectToHost("10.103.239.193",11451)){
						isLanHost = false;
						lanConnected = true;
						currentPlayer = 1;
						history.clear();
						gamestate = LAN_GAME;
						drawBoard(whiteWins, blackWins);
						drawCurrentTurn(currentPlayer);
					}
				}else if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					closeConnection();
					lanConnected = false;
					isLanHost = false;
					gamestate = MENU;
					drawMenu();
				}
			}else if(gamestate == LAN_GAME && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					closeConnection();
					lanConnected = false;
					isLanHost = false;
					resetGame(currentPlayer);
					history.clear();
					gamestate = MENU;
					drawMenu();
				}else if(isLanHost && lanConnected && !roundOver && !gameOver && currentPlayer == 1){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						placeChess(row,col,currentPlayer);
						history.record(row,col,currentPlayer);
						char message[64];
						sprintf(message, "MOVE %d %d %d", row, col, currentPlayer);
						sendMessage(message);
						if(!checkWin(row,col,currentPlayer)){
							if(isBoardFull()){
    					        roundDraw();
					        }else{
						        switchPlayer(currentPlayer);
					        }
						}
					}
		        }else if(!isLanHost && lanConnected && !roundOver && !gameOver && currentPlayer == 2){
					int row,col;
					if(canPlaceChess(msg.x,msg.y,row,col)){
						char message[64];
						sprintf(message, "REQUEST_MOVE %d %d", row, col);
						sendMessage(message);
					}
		        }else if(isLanHost && lanConnected && roundOver && !gameOver){
		    	    if(msg.x >= 650 && msg.x <=780 && msg.y >=220 && msg.y <=270){
		    		    resetRound(currentPlayer);
		    		    history.clear();
		    		    lanStarter = (lanStarter == 1) ? 2 : 1;
		    		    currentPlayer = lanStarter;
		    		    drawCurrentTurn(currentPlayer);
		    		    char message[64];
						sprintf(message, "NEXT_ROUND %d", lanStarter);
						sendMessage(message);
				    }
			    }else if(isLanHost && lanConnected && gameOver){
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=470 && msg.y <=520){
		    		    resetGame(currentPlayer);
		    		    history.clear();
						lanStarter = 1;
		    		    currentPlayer = 1;
		    		    drawCurrentTurn(currentPlayer);
		    		    char message[64];
						sprintf(message, "RESET_GAME %d", lanStarter);
						sendMessage(message);
				    }
			    }
			}else if(gamestate == AI_GAME_EASY && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					history.clear();
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("easySave.txt", currentPlayer, whiteWins, blackWins, roundOver, gameOver);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					loadGame(history,"easySave.txt",currentPlayer);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 405 && msg.y <= 430){
					if(currentPlayer == 1 && !roundOver && !gameOver){
						Step removed;
						if(history.undoAI(removed)){
							currentPlayer = removed.player;
							for(int row = 0;row < lines;row++){
								for(int col = 0;col < lines;col++){
									board[row][col] = 0;
								}
							}
							drawBoard(whiteWins,blackWins);
							for(const Step &step : history.getSteps()){
								int player = step.player;
								placeChess(step.row,step.col,player);
							}
							drawCurrentTurn(currentPlayer);
						}
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
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=470 && msg.y <=520){
		    		    resetGame(currentPlayer);
		    		    history.clear();
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }
			}else if(gamestate == AI_GAME_NORMAL && msg.is_down() && msg.is_left()){
				if(msg.x >= 650 && msg.x <= 780
				&& msg.y >= 580 && msg.y <= 630){
					resetGame(currentPlayer);
					history.clear();
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("normalSave.txt", currentPlayer, whiteWins, blackWins, roundOver, gameOver);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					loadGame(history,"normalSave.txt",currentPlayer);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 405 && msg.y <= 430){
					if(currentPlayer == 1 && !roundOver && !gameOver){
						Step removed;
						if(history.undoAI(removed)){
							currentPlayer = removed.player;
							for(int row = 0;row < lines;row++){
								for(int col = 0;col < lines;col++){
									board[row][col] = 0;
								}
							}
							drawBoard(whiteWins,blackWins);
							for(const Step &step : history.getSteps()){
								int player = step.player;
								placeChess(step.row,step.col,player);
							}
							drawCurrentTurn(currentPlayer);
						}
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
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=470 && msg.y <=520){
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
					history.clear();
					gamestate = MENU;
					drawMenu();
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 345 && msg.y <= 370){
					history.save("hardSave.txt", currentPlayer, whiteWins, blackWins, roundOver, gameOver);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 375 && msg.y <= 400){
					loadGame(history,"hardSave.txt",currentPlayer);
				}
				if(msg.x >= 630 && msg.x <= 690
				&& msg.y >= 405 && msg.y <= 430){
					if(currentPlayer == 1 && !roundOver && !gameOver){
						Step removed;
						if(history.undoAI(removed)){
							currentPlayer = removed.player;
							for(int row = 0;row < lines;row++){
								for(int col = 0;col < lines;col++){
									board[row][col] = 0;
								}
							}
							drawBoard(whiteWins,blackWins);
							for(const Step &step : history.getSteps()){
								int player = step.player;
								placeChess(step.row,step.col,player);
							}
							drawCurrentTurn(currentPlayer);
						}
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
				    if(msg.x >= 650 && msg.x <=780 && msg.y >=470 && msg.y <=520){
		    		    resetGame(currentPlayer);
		    		    history.clear();
		    		    currentPlayer = 2;
		    		    drawCurrentTurn(currentPlayer);
				    }
			    }
			}
	    }
	}
	closeConnection();
	cleanupNetwork();

    return 0;
}
