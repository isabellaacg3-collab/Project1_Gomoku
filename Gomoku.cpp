#include "Gomoku.h"
#include <graphics.h>
#include <cmath>
#include <algorithm>
using namespace std;

void drawBoard(int whiteWins,int blackWins){
	cleardevice();
	setcolor(BLACK);
	rectangle(630,345,690,370);
	xyprintf(650, 350, "Save");
	rectangle(630,375,690,400);
	xyprintf(650, 380, "Load");
	rectangle(630,405,690,430);
	xyprintf(650, 410, "Undo");
	xyprintf(650, 60, "White Wins: %d", whiteWins);
	xyprintf(650, 90, "Black Wins: %d", blackWins);
	rectangle(650,580,780,630);
	xyprintf(670,600,"Back to Menu");
	for(int i = 0;i < lines;i++){
		line(start,start + i * gap,start + (lines - 1) * gap,start + i * gap);
		line(start + i * gap,start,start + i * gap,start + (lines - 1) * gap);
	}
}

void drawCurrentTurn(int currentPlayer){
	if(currentPlayer == 1){
	    xyprintf(650,320, "White Turn");
	}else{
		xyprintf(650,320, "Black Turn");
	}
}

int countColorOnBoard(const int targetBoard[lines][lines],int row,int col,int dr,int dc,int currentPlayer){
	int count = 0;
	row += dr;
	col += dc;
	while(row >= 0 && col >= 0
	&& row < lines && col < lines){
		if(targetBoard[row][col] == currentPlayer){
			count++;
			row += dr;
			col += dc;
		}else{
			return count;
		}
	}
	return count;
}

int countColor(int row,int col,int dr,int dc,int currentPlayer){
	return countColorOnBoard(board,row,col,dr,dc,currentPlayer);
}


void gameWin(int currentPlayer){
	gameOver = true;
	if(currentPlayer == 1){
		rectangle(650,220,780,270);
		xyprintf(670,240,"White Win!");
	}else{
		rectangle(650,220,780,270);
		xyprintf(670,240,"Black Win!");
	}
	rectangle(650,470,780,520);
	xyprintf(670,490,"Reset Game");
}


void resetRound(int &currentPlayer){
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			board[row][col] = 0;
		}
	}
	
	roundOver = false;
	currentPlayer = 1;
	
	drawBoard(whiteWins,blackWins);
}

void resetGame(int &currentPlayer){
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			board[row][col] = 0;
		}
	}
	
	roundOver = false;
	gameOver = false;
	whiteWins = 0;
	blackWins = 0;
	currentPlayer = 1;
	
	drawBoard(whiteWins,blackWins);
}

void roundWin(int &currentPlayer){
	if(roundOver){
		return;
	}
	roundOver = true;
	if(currentPlayer == 1){
		whiteWins++;
		xyprintf(650, 60, "White Wins: %d", whiteWins);
	}else{
		blackWins++;
		xyprintf(650, 90, "Black Wins: %d", blackWins);
	}
	if(whiteWins >= winLimit || blackWins >= winLimit){
		gameWin(currentPlayer);
	}
	if(roundOver && !gameOver){
		rectangle(650,220,780,270);
		xyprintf(670,240,"Next Round");
	}
}

void roundDraw(){
	roundOver = true;
	xyprintf(670,190,"Draw!");
	rectangle(650,220,780,270);
	xyprintf(670,240,"Next Round");
}

bool checkWin(int row,int col,int &currentPlayer){
	if(isWinningMove(row,col,currentPlayer)){
	    roundWin(currentPlayer);
	    return true;
	}else{
		if(currentPlayer == 1){
	        xyprintf(650,320, "Black Turn");
	    }else{
	        xyprintf(650,320, "White Turn");
	    }
	    return false;
	} 
}

bool canPlaceChess(int msgX,int msgY,int &row,int &col){
	if(msgX > start - clickDist 
	&& msgY > start - clickDist
	&& msgX < start + (lines - 1) * gap + clickDist
	&& msgY < start + (lines - 1) * gap + clickDist){
		col = (msgX - start + gap/2) / gap;
		row = (msgY - start + gap/2) / gap;
		int dx = col * gap + start - msgX;
		int dy = row * gap + start - msgY;
		double Dist = sqrt(dx * dx + dy * dy);
		if(Dist <= clickDist && board[row][col] == 0){
			return true;
		}
	}
	return false;
}

void placeChess(int row, int col, int &currentPlayer){
	board[row][col] = currentPlayer;
	currentPlayer == 1 ? setfillcolor(WHITE) : setfillcolor(BLACK);
	fillellipse(col * gap + start,row * gap + start,chessSize,chessSize);
}

void switchPlayer(int &currentPlayer){
	currentPlayer = currentPlayer == 1 ? 2 : 1;
}

bool isWinningMoveOnBoard(const int targetBoard[lines][lines],int row,int col,int currentPlayer){
	return countColorOnBoard(targetBoard,row,col,0,1,currentPlayer) + countColorOnBoard(targetBoard,row,col,0,-1,currentPlayer) >= 4
	||countColorOnBoard(targetBoard,row,col,1,0,currentPlayer) + countColorOnBoard(targetBoard,row,col,-1,0,currentPlayer) >=4
	||countColorOnBoard(targetBoard,row,col,1,1,currentPlayer) + countColorOnBoard(targetBoard,row,col,-1,-1,currentPlayer) >=4
	||countColorOnBoard(targetBoard,row,col,1,-1,currentPlayer) + countColorOnBoard(targetBoard,row,col,-1,1,currentPlayer) >=4;
}

bool isWinningMove(int row,int col,int currentPlayer){
	return isWinningMoveOnBoard(board,row,col,currentPlayer);
}

bool isBoardFull(){
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(board[row][col] == 0){
				return false;
			}
		}
	}
	return true;
}
