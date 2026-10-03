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
extern bool playerTurn;

void drawBoard(int whiteWins,int blackWins);

void roundWin(int &currentPlayer);
void gameWin(int currentPlayer);

void resetRound(int &currentPlayer);
void resetGame(int &currentPlayer);

bool canPlaceChess(int msgX,int msgY,int &row,int &col);
void placeChess(int row, int col, int &currentPlayer);
bool checkWin(int row, int col, int &currentPlayer);
void switchPlayer(int &currentPlayer);

int countColor(int row,int col,int dr,int dc,int currentPlayer);
