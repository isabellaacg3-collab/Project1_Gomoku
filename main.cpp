#include <graphics.h>
#include <cmath>
#include <algorithm>
using namespace std;

const int windowX = 800;
const int windowY = 640;
const int start = 40;
const int gap = 40;

const int lines = 15;
int board[lines][lines] = {0};

const int clickDist = 15;
const int chessSize = 15;

bool roundOver = false;
bool gameOver = false;

int winLimit = 2;
int whiteWins = 0;
int blackWins = 0;

void drawBoard(){
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
	drawBoard();
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
	drawBoard();
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

int main(){
	initgraph(windowX,windowY,0);
	setbkcolor(EGERGB(238, 196, 126));
	drawBoard();
    
	int currentPlayer = 1;
    for(;is_run();delay_fps(60)){
		while(mousemsg()){
			mouse_msg msg = getmouse();
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

    return 0;
}
