#include <vector>
#include <fstream>
#include "History.h"
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
	return true;
}

bool History::load(const string &filename,int &currentPlayer,int &whiteWins,int &blackWins,
	bool *roundOver,bool *gameOver){
	ifstream fin(filename);
	if(!fin.is_open()){
		return false;
	}
	
	steps.clear();
	
	int stepCount;
	fin >> currentPlayer;
	fin >> whiteWins >> blackWins;
	fin >> stepCount;
	
	int ro = 0,go = 0;
	if(fin >> ro >> go){
		if(roundOver){
			*roundOver = (ro != 0);
		}
		if(gameOver){
			*gameOver = (go != 0);
		}
	}
	
	for(int i = 0;i < stepCount;i++){
		int row,col,player;
		fin >> row >> col >> player;
		steps.push_back(Step(row,col,player));
	}
	
	fin.close();
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
