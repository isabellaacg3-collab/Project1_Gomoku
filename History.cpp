#include <vector>
#include <fstream>
#include "History.h"
#include "Gomoku.h"
using namespace std;

Step::Step(int r,int c,int p):row(r),col(c),player(p){}

void History::record(int row,int col,int player){
	steps.push_back(Step(row,col,player));
}

void History::undo(){
	
}

bool History::save(const string &filename,int currentPlayer,int whiteWins,int blackWins,
	bool roundOver,bool gameOver){
	ofstream fout(filename);
	if(!fout.is_open()){
		return false;
	}
	
	fout << currentPlayer << endl;
	fout << whiteWins << " " << blackWins << endl;
	fout << steps.size() << endl;
	fout << (roundOver ? 1 : 0) << " " << (gameOver ? 1 : 0) << endl;
	for(const Step &step : steps){
		fout << step.row << " " << step.col << " " << step.player << endl;
	}
	
	fout.close();
	return bool(fout);
}

bool History::load(const string &filename,int &currentPlayer,int &whiteWins,int &blackWins,
	bool *roundOver,bool *gameOver){
	ifstream fin(filename);
	int player,white,black,stepCount;
	if(!(fin >> player >> white >> black >> stepCount)
	|| (player != 1 && player != 2) || white < 0 || black < 0
	|| white > winLimit || black > winLimit || (white == winLimit && black == winLimit)
	|| stepCount < 0 || stepCount > lines * lines){
		return false;
	}
	vector<int> values;
	fin >> ws;
	while(fin.peek() != EOF){
		int value;
		if(!(fin >> value) || values.size() >= (unsigned)(stepCount * 3 + 2)){
			return false;
		}
		values.push_back(value);
		fin >> ws;
	}
	int offset = values.size() == (unsigned)(stepCount * 3 + 2) ? 2 : 0;
	if(values.size() != (unsigned)(stepCount * 3 + offset)
	|| (offset && ((values[0] != 0 && values[0] != 1) || (values[1] != 0 && values[1] != 1)))){
		return false;
	}
	vector<Step> loadedSteps;
	int savedBoard[lines][lines] = {0};
	int occupied = 0;
	bool ended = false;
	for(int i = 0;i < stepCount;i++){
		if(ended){
			for(int row = 0;row < lines;row++){
				for(int col = 0;col < lines;col++){
					savedBoard[row][col] = 0;
				}
			}
			loadedSteps.clear();
			occupied = 0;
		}
		int row = values[offset + i * 3],col = values[offset + i * 3 + 1],color = values[offset + i * 3 + 2];
		if(row < 0 || row >= lines || col < 0 || col >= lines
		|| (color != 1 && color != 2) || savedBoard[row][col] != 0){
			return false;
		}
		savedBoard[row][col] = color;
		loadedSteps.push_back(Step(row,col,color));
		occupied++;
		ended = isWinningMoveOnBoard(savedBoard,row,col,color) || occupied == lines * lines;
	}
	steps.swap(loadedSteps);
	currentPlayer = player;
	whiteWins = white;
	blackWins = black;
	if(roundOver){
		*roundOver = ended || white >= winLimit || black >= winLimit;
	}
	if(gameOver){
		*gameOver = white >= winLimit || black >= winLimit;
	}
	return true;
}

void History::clear(){
	steps.clear();
}

int History::size() const{
	return steps.size();
}

const vector<Step>& History::getSteps() const{
    return steps;
}
