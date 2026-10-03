#include <graphics.h>
#include "AI.h"
#include "Gomoku.h"

void drawDiffPanel(){
	cleardevice();
	
	setcolor(BLACK);
	
	rectangle(280,220,520,270);
	xyprintf(300,240,"EASY");
	
	rectangle(280,300,520,350);
	xyprintf(300,320,"NORMAL");
	
	rectangle(280,380,520,430);
	xyprintf(300,400,"HARD");
	
	rectangle(650,580,780,630);
	xyprintf(670,600,"Back to Menu");
}

int getScore(int row,int col,int dr,int dc,int aiPlayer){
	int count = 0;
	int isSameMultiple = 1;
	int flagPlayer = 0;
	int rowptr = row + dr;
	int colptr = col + dc;
	for(int i = 0;i < times;i++){
		if(rowptr >= 0 && rowptr < lines
		&& colptr >= 0 && colptr < lines){
			if(i == 0){
				flagPlayer = board[rowptr][colptr];
				if(flagPlayer == 0){
					break;
				}
			}
			if(board[rowptr][colptr] == flagPlayer){
			    count++;
			    rowptr += dr;
			    colptr += dc;
		    }else{
			    break;
		    }
		}
	}
	if(flagPlayer == aiPlayer){
		count++;
	}
	
	rowptr = row - dr;
	colptr = col - dc;
	for(int i = 0;i < times;i++){
		if(rowptr >= 0 && rowptr < lines
		&& colptr >= 0 && colptr < lines){
			if(i == 0){
				isSameMultiple = (flagPlayer == board[rowptr][colptr]) ? 2 : 1;
				flagPlayer = board[rowptr][colptr];
				if(flagPlayer == 0){
					break;
				}
			}
			if(board[rowptr][colptr] == flagPlayer){
			    count++;
			    rowptr -= dr;
			    colptr -= dc;
		    }else{
			    break;
		    }
		}
	}
	if(flagPlayer == aiPlayer){
		count++;
	}
	
	return countMultiple[count] * isSameMultiple;
}

bool chooseEasyMove(int aiPlayer,int &bestRow,int &bestCol){
	int bestScore = 0;
	bool isFull = true;
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(board[row][col] == 0){
				int tempScore = getScore(row,col,0,1,aiPlayer) + getScore(row,col,1,0,aiPlayer)
				+ getScore(row,col,1,1,aiPlayer) + getScore(row,col,-1,1,aiPlayer);
				if(tempScore > bestScore){
					bestScore = tempScore;
					bestRow = row;
					bestCol = col;
				}
				isFull = false;
			}
		}
	}
	return !isFull;
}
