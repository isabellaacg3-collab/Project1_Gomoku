#pragma once
constexpr int countMultiple[11] = {0,1,3,7,15,31,63,127,255,511,1023};
constexpr int times = 4;
constexpr int greedScore[8] = {1,20,100,200,1000,2000,10000,100000};

void drawDiffPanel();

bool chooseEasyMove(int aiPlyer,int &bestRow,int &bestCol);
bool chooseNormalMove(int aiPlayer,int &bestRow,int &bestCol);
