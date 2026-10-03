#pragma once
constexpr int countMultiple[11] = {0,1,2,3,5,8,13,21,34,55,89};
constexpr int times = 4;

void drawDiffPanel();

bool chooseEasyMove(int aiPlyer,int &bestRow,int &bestCol);
