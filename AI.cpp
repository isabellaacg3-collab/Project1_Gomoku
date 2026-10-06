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
	bool hasMove = false;
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
				hasMove = true;
			}
		}
	}
	return hasMove;
}

int scoreDirection(int row, int col, int dr, int dc, int player){
	int count = 1;
	int openEnds = 0;
	int rowptr = row + dr;
	int colptr = col + dc;
	while(rowptr >= 0 && colptr >= 0
	&& rowptr < lines && colptr < lines){
		if(board[rowptr][colptr] == player){
			count++;
			rowptr += dr;
			colptr += dc;
		}else if(board[rowptr][colptr] == 0){
			openEnds++;
			break;
		}else{
			break;
		}
	}
	
	rowptr = row - dr;
	colptr = col - dc;
	while(rowptr >= 0 && colptr >= 0
	&& rowptr < lines && colptr < lines){
		if(board[rowptr][colptr] == player){
			count++;
			rowptr -= dr;
			colptr -= dc;
		}else if(board[rowptr][colptr] == 0){
			openEnds++;
			break;
		}else{
			break;
		}
	}
	
	if(count >= 5){
		return greedScore[7];
	}else if(count == 4 && openEnds == 2){
		return greedScore[6];
	}else if(count == 4 && openEnds == 1){
		return greedScore[5];
	}else if(count == 3 && openEnds == 2){
		return greedScore[4];
	}else if(count == 3 && openEnds == 1){
		return greedScore[3];
	}else if(count == 2 && openEnds == 2){
		return greedScore[2];
	}else if(count == 2 && openEnds == 1){
		return greedScore[1];
	}else{
		return greedScore[0];
	}
}

int scorePoint(int row,int col,int player){
	return scoreDirection(row,col,0,1,player) + scoreDirection(row,col,1,0,player)
	+ scoreDirection(row,col,1,1,player) + scoreDirection(row,col,-1,1,player);
}

bool chooseNormalMove(int aiPlayer,int &bestRow,int &bestCol){
	int opPlayer = aiPlayer == 1 ? 2 : 1;
	bool hasMove = false;
	
	bool isEmpty = true;
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(board[row][col] != 0){
				isEmpty = false;
				break;
			}
		}
		if(!isEmpty){
			break;
		}
	}
	if(isEmpty){
		bestRow = lines / 2;
        bestCol = lines / 2;
        return true;
	}
	
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(board[row][col] == 0){
				if(isWinningMove(row,col,aiPlayer)){
					bestRow = row;
					bestCol = col;
					return true;
				}
			}
		}
	}
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(board[row][col] == 0){
				if(isWinningMove(row,col,opPlayer)){
					bestRow = row;
					bestCol = col;
					return true;
				}
			}
		}
	}
	
	double bestGreedScore = 0;
	double power = 0.9;
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(board[row][col] == 0){
				hasMove = true;
				int attackScore = scorePoint(row,col,aiPlayer);
				int defenseScore = scorePoint(row,col,opPlayer);
				double tempScore = attackScore + defenseScore * power;
				if(tempScore > bestGreedScore){
					bestGreedScore = tempScore;
					bestRow = row;
					bestCol = col;
				}
			}
		}
	}
	return hasMove;
}
