#pragma once
#include "Gomoku.h"
constexpr int countMultiple[11] = {0,1,3,7,15,31,63,127,255,511,1023};
constexpr int times = 4;
constexpr int greedScore[8] = {1,20,100,200,1000,2000,10000,100000};
constexpr int distance = 1;

void drawDiffPanel();

bool chooseEasyMove(int aiPlayer,int &bestRow,int &bestCol);
bool chooseNormalMove(int aiPlayer,int &bestRow,int &bestCol);
bool chooseHardMove(int aiPlayer,int &bestRow,int &bestCol);
double evaluateHardMove(int row, int col, int aiPlayer);

int scoreDirectionOnBoard(const int targetBoard[lines][lines],int row,int col,int dr,int dc,int player);
int scorePointOnBoard(const int targetBoard[lines][lines],int row,int col,int player);
int scoreDirection(int row, int col, int dr, int dc, int player);
int scorePoint(int row,int col,int player);

extern int searchBoard[lines][lines];
void copyBoardForSearch();
void placeSearchMove(int row, int col, int player);
void undoSearchMove(int row, int col);
