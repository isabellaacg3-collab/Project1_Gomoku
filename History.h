#pragma once
#include <vector>
#include <string>
using namespace std;

class Step{
	public:
		int row;
		int col;
		int player;
		
		Step();
		Step(int r,int c,int p);
};

class History{
	private:
		vector<Step> steps;
	public:
		void record(int row,int col,int player);
		
		bool popLast(Step &removedStep);

		bool undoAI(Step &removedStep);
		
		bool save(const string &filename,int currentPlayer,int whiteWins,int blackWins,
			bool roundOver = false,bool gameOver = false,bool localUndoLocked = false);
		
		bool load(const string &filename,int &currentPlayer,int &whiteWins,int &blackWins,
			bool *roundOver = NULL,bool *gameOver = NULL,bool *localUndoLocked = NULL);
		
		void clear();
		
		int size() const;
		
		const vector<Step>& getSteps() const;
};
