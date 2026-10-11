#include <vector>
#include <fstream>
#include <sstream>
#include "History.h"
#include "Gomoku.h"
using namespace std;

Step::Step():row(0),col(0),player(0){}
Step::Step(int r,int c,int p):row(r),col(c),player(p){}

void History::record(int row,int col,int player){
	steps.push_back(Step(row,col,player));
}

bool History::popLast(Step &removedStep){
	if(steps.empty()){
		return false;
	}
	removedStep = steps.back();
	steps.pop_back();
	return true;
}

void History::undo(){
	
}

bool History::save(const string &filename,int currentPlayer,int whiteWins,int blackWins,
	bool roundOver,bool gameOver,bool localUndoLocked){
	ofstream fout(filename);
	if(!fout.is_open()){
		return false;
	}
	
	fout << currentPlayer << endl;
	fout << whiteWins << " " << blackWins << endl;
	fout << steps.size() << endl;
	fout << (roundOver ? 1 : 0) << " " << (gameOver ? 1 : 0) << endl;
	fout << (localUndoLocked ? 1 : 0) << endl;
	for(const Step &step : steps){
		fout << step.row << " " << step.col << " " << step.player << endl;
	}
	
	fout.close();
	return bool(fout);
}

bool History::load(const string &filename,int &currentPlayer,int &whiteWins,int &blackWins,
	bool *roundOver,bool *gameOver,bool *localUndoLocked){
	ifstream fin(filename);
	int player,white,black,stepCount;
	if(!(fin >> player >> white >> black >> stepCount)
	|| (player != 1 && player != 2) || white < 0 || black < 0
	|| white > winLimit || black > winLimit || (white == winLimit && black == winLimit)
	|| stepCount < 0 || stepCount > lines * lines){
		return false;
	}
	vector<int> values,lineSizes;
	string line;
	while(getline(fin,line)){
		istringstream input(line);
		int count = 0;
		input >> ws;
		while(input.peek() != EOF){
			int value;
			if(!(input >> value) || values.size() >= (unsigned)(stepCount * 3 + 3)){
				return false;
			}
			values.push_back(value);
			count++;
			input >> ws;
		}
		if(count){
			lineSizes.push_back(count);
		}
	}
	int offset = (int)values.size() - stepCount * 3;
	if(offset != 0 && offset != 2 && offset != 3){
		return false;
	}
	int metadataLines = offset ? offset - 1 : 0;
	if(lineSizes.size() != (unsigned)(stepCount + metadataLines)
	|| (offset && (lineSizes[0] != 2 || (values[0] != 0 && values[0] != 1)
		|| (values[1] != 0 && values[1] != 1)))
	|| (offset == 3 && (lineSizes[1] != 1 || (values[2] != 0 && values[2] != 1)))){
		return false;
	}
	for(unsigned i = metadataLines;i < lineSizes.size();i++){
		if(lineSizes[i] != 3){
			return false;
		}
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
	if(localUndoLocked){
		// Old saves have no undo state: unlock only after a new move.
		*localUndoLocked = offset == 3 ? values[2] != 0 : true;
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
