#pragma once
constexpr int windowX = 800;
constexpr int windowY = 640;
constexpr int lines = 15;
constexpr int start = 40;
constexpr int gap = 40;
constexpr int clickDist = 15;
constexpr int chessSize = 15;
constexpr int winLimit = 2;

extern int board[lines][lines];
extern bool roundOver;
extern bool gameOver;
extern int whiteWins;
extern int blackWins;

void drawBoard(int whiteWins,int blackWins);
int countColor(int row,int col,int dr,int dc,int currentPlayer);
void roundWin(int &currentPlayer);
void gameWin(int currentPlayer);
void resetRound(int &currentPlayer);
void resetGame(int &currentPlayer);
void nextChess(int &currentPlayer,int row,int col);
