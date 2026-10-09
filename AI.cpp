#include <graphics.h>
#include "AI.h"
#include "Gomoku.h"

const double WIN_SCORE = 1e9;

int searchBoard[lines][lines];

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

int scoreDirectionOnBoard(const int targetBoard[lines][lines],int row,int col,int dr,int dc,int player){
	int count = 1;
	int openEnds = 0;
	int rowptr = row + dr;
	int colptr = col + dc;
	while(rowptr >= 0 && colptr >= 0
	&& rowptr < lines && colptr < lines){
		if(targetBoard[rowptr][colptr] == player){
			count++;
			rowptr += dr;
			colptr += dc;
		}else if(targetBoard[rowptr][colptr] == 0){
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
		if(targetBoard[rowptr][colptr] == player){
			count++;
			rowptr -= dr;
			colptr -= dc;
		}else if(targetBoard[rowptr][colptr] == 0){
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

int scoreDirection(int row, int col, int dr, int dc, int player){
	return scoreDirectionOnBoard(board,row,col,dr,dc,player);
}

int scorePointOnBoard(const int targetBoard[lines][lines],int row,int col,int player){
	return scoreDirectionOnBoard(targetBoard,row,col,0,1,player) + scoreDirectionOnBoard(targetBoard,row,col,1,0,player)
	+ scoreDirectionOnBoard(targetBoard,row,col,1,1,player) + scoreDirectionOnBoard(targetBoard,row,col,-1,1,player);
}

int scorePoint(int row,int col,int player){
	return scorePointOnBoard(board,row,col,player);
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

void copyBoardForSearch(){
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			searchBoard[row][col] = board[row][col];
		}
	}
}

void placeSearchMove(int row, int col, int player){
	searchBoard[row][col] = player;
}
void undoSearchMove(int row, int col){
	searchBoard[row][col] = 0;
}

bool hasNeighbor(const int targetBoard[lines][lines],int row,int col,int distance){
	for(int i = row - distance;i <= row + distance;i++){
		for(int j = col - distance;j <= col + distance;j++){
			if(i >= 0 && i < lines && j >= 0 && j < lines){
				if(targetBoard[i][j] != 0){
					return true;
				}
			}
		}
	}
	return false;
}

double evaluateSearchBoard(int aiPlayer){
    int opPlayer = aiPlayer == 1 ? 2 : 1;

    int aiBest_1 = 0;
    int aiBest_2 = 0;
    int opBest_1 = 0;
    int opBest_2 = 0;

    for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(searchBoard[row][col] == 0 && hasNeighbor(searchBoard,row,col,distance)){
				int aiTempScore = scorePointOnBoard(searchBoard,row,col,aiPlayer);
				int opTempScore = scorePointOnBoard(searchBoard,row,col,opPlayer);
				if(aiTempScore > aiBest_1){
					aiBest_2 = aiBest_1;
					aiBest_1 = aiTempScore;
				}else if(aiTempScore > aiBest_2){
					aiBest_2 = aiTempScore;
				}
				if(opTempScore > opBest_1){
					opBest_2 = opBest_1;
					opBest_1 = opTempScore;
				}else if(opTempScore > opBest_2){
					opBest_2 = opTempScore;
				}
			}
		}
	}

    return aiBest_1 + aiBest_2 * 0.5 - opBest_1 - opBest_2 * 0.5;
}

double evaluateHardMove(int row, int col, int aiPlayer,double alpha){
	int opPlayer = aiPlayer == 1 ? 2 : 1;
	placeSearchMove(row,col,aiPlayer);
	if(isWinningMoveOnBoard(searchBoard,row,col,aiPlayer)){
		undoSearchMove(row,col);
		return WIN_SCORE;
	}
	
	double worstScore = WIN_SCORE;
    bool hasReply = false;
	
	for(int opRow = 0;opRow < lines;opRow++){
		for(int opCol = 0;opCol < lines;opCol++){
			if(searchBoard[opRow][opCol] == 0 && hasNeighbor(searchBoard,opRow,opCol,distance)){
				hasReply = true;
				placeSearchMove(opRow,opCol,opPlayer);
				double replyScore;
				if(isWinningMoveOnBoard(searchBoard,opRow,opCol,opPlayer)){
					undoSearchMove(row,col);
					undoSearchMove(opRow,opCol);
		            return -WIN_SCORE;
	            }else{
	            	replyScore = evaluateSearchBoard(aiPlayer);
				}
				undoSearchMove(opRow,opCol);
				worstScore = worstScore <= replyScore ? worstScore : replyScore;
				if(worstScore <= alpha){
					undoSearchMove(row,col);
					return worstScore;
				}
			}
		}
	}
	undoSearchMove(row,col);
	
	if(!hasReply){
		return 0;
	}
	
	return worstScore;
}

bool chooseHardMove(int aiPlayer,int &bestRow,int &bestCol){
	copyBoardForSearch();
	bool hasMove = false;
	
	bool isEmpty = true;
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(searchBoard[row][col] != 0){
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
	
	double bestScore = -WIN_SCORE - 1;
	for(int row = 0;row < lines;row++){
		for(int col = 0;col < lines;col++){
			if(searchBoard[row][col] == 0 && hasNeighbor(searchBoard,row,col,distance)){
				hasMove = true;
				double tempScore = evaluateHardMove(row,col,aiPlayer,bestScore);
				if(tempScore > bestScore){
					bestScore = tempScore;
					bestRow = row;
					bestCol = col;
				}
			}
		}
	}
	return hasMove;
}
