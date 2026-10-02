#include "Gomoku.h"
#include <graphics.h>

void drawBoard(int whiteWins,int blackWins){
	setcolor(BLACK);	
	xyprintf(650, 60, "White Wins: %d", whiteWins);
	xyprintf(650, 90, "Black Wins: %d", blackWins);
	xyprintf(650,320, "White Turn");
	for(int i = 0;i < lines;i++){
		line(start,start + i * gap,start + (lines - 1) * gap,start + i * gap);
		line(start + i * gap,start,start + i * gap,start + (lines - 1) * gap);
	}
}


int countColor(int row,int col,int dr,int dc,int currentPlayer){
	int count = 0;
	row += dr;
	col += dc;
	while(row >= 0
	&& col >= 0
	&& row < lines
	&& col < lines){
		if(board[row][col] == currentPlayer){
			count++;
			row += dr;
			col += dc;
		}else{
			return count;
		}
	}
	return count;
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
	rectangle(650,420,780,470);
	xyprintf(670,440,"Reset Game");
}


void resetRound(int &currentPlayer){
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			board[row][col] = 0;
		}
	}
	
	roundOver = false;
	currentPlayer = 1;
	
	cleardevice();
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
	
	cleardevice();
	drawBoard(whiteWins,blackWins);
}

void roundWin(int &currentPlayer){
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

void nextChess(int &currentPlayer,int row,int col){
	currentPlayer == 1 ? setfillcolor(WHITE) : setfillcolor(BLACK);
	fillellipse(col * gap + start,row * gap + start,chessSize,chessSize);
	currentPlayer = currentPlayer == 1 ? 2 : 1;
}
