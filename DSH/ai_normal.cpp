/*
 * ============================================================================
 *  ai_normal.cpp  ——  五子棋「普通模式」人机对战 AI
 * ----------------------------------------------------------------------------
 *  文件编码：GBK / ANSI（无 BOM）。
 *  原因是 Dev-C++ 5.11 在中文 Windows 下按 ANSI(GBK) 打开源文件，
 *  若把本文件存成 UTF-8，Dev-C++ 里的中文注释会变成乱码。
 *  请勿用 UTF-8 重新保存本文件。
 * ----------------------------------------------------------------------------
 *  说明：
 *    本文件给出普通难度（Normal）AI 的完整实现，目标是实现五子棋最基本的
 *    对战战术，而不是随机乱下。它与「简单模式」的根本区别是：
 *
 *      简单模式：只看"我下在这里，自己能形成几个连子"，从不看对手的棋。
 *                结果是既不会补自己的杀招，也永远不会去堵对手的活三 / 冲四。
 *
 *      普通模式（本文件）：
 *        第一步  —— 先找「必杀点」：我下这里立刻连成五子，直接赢。
 *        第二步  —— 再找「必堵点」：对手下这里就连成五子，我必须堵。
 *        第三步  —— 都没有时，对每个候选空点同时算「进攻分」和「防守分」，
 *                   选综合分最高的点。这样它既会自己造活三 / 冲四，
 *                   也会主动去破坏对手的活三 / 冲四。
 *
 *    想再强一档（困难）可以在此基础上加：完整整行棋型评估、极小极大搜索、
 *    算杀（VCF/连续冲四）。本文件只做到"普通"这一档。
 *
 *  接口：
 *      NormalMove chooseNormalMove(int aiPlayer);
 *      返回值 .row / .col 为选中的落点，.forced 表示是否走了"必杀 / 必堵"。
 *      aiPlayer：1 表示白棋，2 表示黑棋（与工程里的约定一致，0 表示空位）。
 *
 *  在 Dev-C++ 的工程里启用它，需要两步：
 *      1) 把本文件加入工程（Project1_Gomoku.dev），或在 Makefile.win 的
 *         OBJ / LINKOBJ 里加上 ai_normal.o 并补一条编译规则；
 *      2) 在 main.cpp 的游戏循环里调用（参照现有 AI_GAME_EASY 的写法）：
 *             if(gamestate == AI_GAME_NORMAL && currentPlayer == 2
 *             && !roundOver && !gameOver){
 *                 NormalMove mv = chooseNormalMove(currentPlayer);
 *                 if(mv.row >= 0){
 *                     placeChess(mv.row, mv.col, currentPlayer);
 *                     if(!checkWin(mv.row, mv.col, currentPlayer)){
 *                         switchPlayer(currentPlayer);
 *                     }
 *                 }
 *             }
 *         即可替换掉原来被注释掉的 //normalAIGame();。
 *         注意要保留“轮到 AI 才走”的条件，否则每帧都会落子。
 * ============================================================================
 */

#include <cstddef>   /* size_t */
#include <cstring>   /* memset */
#include <cstdlib>   /* abs  */

/* 本文件不包含 Gomoku.h，而是自用一套常量与棋盘副本，
 * 这样它既能独立编译测试，也能直接放进工程而不与现有代码冲突。
 * 若你想让它直接操作全局棋盘，只要把 board 换成 Gomoku.h 里的 board，
 * 并把下面三个常量删掉即可（值必须与原工程一致）。 */
const int NORMAL_LINES = 15;   /* 棋盘规格 15 × 15，对应原工程 lines   */
const int NORMAL_EMPTY = 0;    /* 空位，对应原工程 board 里的 0        */
const int NORMAL_BLACK = 2;    /* 黑棋，对应原工程 currentPlayer == 2  */
const int NORMAL_WHITE = 1;    /* 白棋，对应原工程 currentPlayer == 1  */

/* 棋盘副本：0 = 空，1 = 白，2 = 黑。全局静态，外部看不见。 */
static int board[NORMAL_LINES][NORMAL_LINES];

/* 四个搜索方向：横、竖、主对角线（左上-右下）、副对角线（右上-左下）。
 * 每条直线只需正向一个方向即可遍历完整，因为直线是双向的。 */
static const int DIR_ROW[4] = {0, 1, 1, -1};
static const int DIR_COL[4] = {1, 0, 1,  1};

/* ---------------------------------------------------------------------------
 *  棋型分值表（评分的核心）
 *  五子棋的战术本质就是"棋型"：连子数越多、两端越开放，威胁越大。
 *  下面是按经验设定的权重，数值本身不是唯一答案，关键是它们的相对大小：
 *      五连  >>>  活四  >  冲四  >  活三  >  眠三 / 活二  >  活一
 * ------------------------------------------------------------------------- */
static const int SCORE_FIVE      = 100000;  /* 五连：直接获胜              */
static const int SCORE_OPEN_FOUR = 10000;   /* 活四：两端都空，对手堵不住  */
static const int SCORE_FOUR      = 1000;    /* 冲四：一端被堵，只能成五    */
static const int SCORE_OPEN_THREE = 1000;   /* 活三：再下一子即可变活四    */
static const int SCORE_SLEEP_THREE = 100;   /* 眠三：一端被堵的三          */
static const int SCORE_OPEN_TWO  = 100;     /* 活二                        */
static const int SCORE_SLEEP_TWO = 10;      /* 眠二                        */
static const int SCORE_ONE       = 1;       /* 孤子                        */
static const int SCORE_MIN       = -1;      /* 无法形成任何连子的死点      */

/* 防守权重：对手的分要乘上这个系数再与自己的分比较。
 * 取 0.9（略小于 1）的含义是——同等威胁下优先走自己的进攻，
 * 但对手的威胁绝不被忽略。想更保守可以调大（例如 1.0 ~ 1.1）。 */
static const double DEFENSE_WEIGHT = 0.9;

/* 每个方向最多只需要看 4 格：加上落子本身最多构成 5 连，
 * 更远的子已不可能与本点连成五。 */
static const int MAX_SCAN = 4;

/* 坐标越界判断 */
static bool inBoard(int row, int col){
    return row >= 0 && row < NORMAL_LINES && col >= 0 && col < NORMAL_LINES;
}

/* ===========================================================================
 *  第一部分：读棋盘的基本工具
 * ========================================================================= */

/*
 * 数连子：从 (row, col) 出发，沿 (dr, dc) 方向连续同色棋子的个数。
 * 注意不包含 (row, col) 本身——调用方把它单独 +1 处理。
 * 遇到空位、异色或棋盘边界立即停止，并报告是否因"被堵"而停止。
 *
 * 例如一行是 [空][白][白][白][空]，在中间那子处向左数得 1、向右数得 1。
 */
static int countRun(int row, int col, int dr, int dc, int player, bool *blocked){
    int count = 0;
    int r = row + dr;
    int c = col + dc;
    for(int step = 0; step < MAX_SCAN; step++){
        if(!inBoard(r, c)){        /* 撞到棋盘边界：视为被堵 */
            if(blocked) *blocked = true;
            break;
        }
        if(board[r][c] != player){ /* 撞到空位或敌子：被堵 */
            if(blocked) *blocked = true;
            break;
        }
        count++;
        r += dr;
        c += dc;
    }
    return count;
}

/*
 * 判断某个方向上是否已经连成五。
 * 只看连续同色子，不区分活四 / 冲四——当且仅当出现 5 连及以上即为胜。
 */
static bool lineHasFive(int row, int col, int dr, int dc, int player){
    int total = 1;
    total += countRun(row, col,  dr,  dc, player, NULL);
    total += countRun(row, col, -dr, -dc, player, NULL);
    return total >= 5;
}

/*
 * 落子后在某个方向上的得分。
 * 思路很简单：假设 player 刚把子下在 (row, col)，统计这条直线上
 *   - 正向连子数 + 反向连子数 + 1（自己）= total
 *   - 两端还有几个空端 = ends
 * 再用 (连子数, 开放端数) 查表得到棋型分。这种"数连子 + 数端点"的
 * 判定方式与 Gomoku.cpp 里 checkWin 的思路一致，容易对照理解。
 */
static int scoreDirection(int row, int col, int dr, int dc, int player){
    bool blockedA = false;
    bool blockedB = false;

    int forward  = countRun(row, col,  dr,  dc, player, &blockedA);
    int backward = countRun(row, col, -dr, -dc, player, &blockedB);

    int total = 1 + forward + backward;
    int ends  = (blockedA ? 0 : 1) + (blockedB ? 0 : 1);

    /* 连成五子及以上：直接是杀招。
     * 注意把 6 连（长连）也归入五连，与工程里 checkWin 的
     * "count >= 5 即胜"保持一致。 */
    if(total >= 5){
        return SCORE_FIVE;
    }
    if(total == 4){
        return (ends == 2) ? SCORE_OPEN_FOUR : SCORE_FOUR;
    }
    if(total == 3){
        return (ends == 2) ? SCORE_OPEN_THREE : SCORE_SLEEP_THREE;
    }
    if(total == 2){
        return (ends == 2) ? SCORE_OPEN_TWO : SCORE_SLEEP_TWO;
    }
    return (ends >= 1) ? SCORE_ONE : SCORE_MIN;
}

/*
 * 综合评分：把四个方向的得分相加。
 * 之所以按"方向"而不是"整条线"评估，是为了控制复杂度、便于理解；
 * 它的代价是看不出一条线上的所有子加起来有多强（例如双活三）。
 * 想升级到困难难度，可以在这里换成"扫描整条线并做棋型匹配"。
 */
static int scorePoint(int row, int col, int player){
    int total = 0;
    for(int d = 0; d < 4; d++){
        total += scoreDirection(row, col, DIR_ROW[d], DIR_COL[d], player);
    }
    return total;
}

/* ===========================================================================
 *  第二部分：候选点生成
 * ========================================================================= */

/*
 * (row, col) 周围 offset 格内是否有棋子。
 * 落子必须贴着已有的棋子才有意义——远离棋子的空点不可能形成任何棋型，
 * 全部评估一遍纯属浪费。
 */
static bool hasNeighbor(int row, int col, int offset){
    for(int dr = -offset; dr <= offset; dr++){
        for(int dc = -offset; dc <= offset; dc++){
            if(dr == 0 && dc == 0) continue;
            int r = row + dr;
            int c = col + dc;
            if(inBoard(r, c) && board[r][c] != NORMAL_EMPTY){
                return true;
            }
        }
    }
    return false;
}

/*
 * 收集所有"值得考虑"的空点，并统计棋盘上的棋子总数。
 * 返回值：棋盘上已有棋子时返回 true；棋盘全空时返回 false，
 *         此时候选点为空，由调用方直接走天元（中心点），这是五子棋的常识开局。
 */
static bool collectCandidates(int offset, int candRow[], int candCol[], int &count, int &stoneCount){
    count = 0;
    stoneCount = 0;
    for(int row = 0; row < NORMAL_LINES; row++){
        for(int col = 0; col < NORMAL_LINES; col++){
            if(board[row][col] != NORMAL_EMPTY){
                stoneCount++;
            }else if(hasNeighbor(row, col, offset)){
                candRow[count] = row;
                candCol[count] = col;
                count++;
            }
        }
    }
    return stoneCount > 0;
}

/* ===========================================================================
 *  第三部分：对外接口
 * ========================================================================= */

/* 落点结果 */
struct NormalMove{
    int  row;      /* 行坐标，-1 表示没有可走的位置（棋盘已满） */
    int  col;      /* 列坐标                                    */
    bool forced;   /* true 表示走了"必杀"或"必堵"，属于战术着法 */
};

/*
 * 普通模式主函数：为 aiPlayer 选择一步棋。
 *
 * 决策顺序（这是本 AI 的全部"战术"，按优先级从高到低）：
 *   1. 棋盘全空          → 走天元
 *   2. 我能立刻成五      → 直接下，获胜（必杀）
 *   3. 对手能立刻成五    → 堵住那个点（必堵）
 *   4. 其他情况          → 对候选点算 进攻分 + 0.9 × 防守分，取最高
 */
NormalMove chooseNormalMove(int aiPlayer){
    NormalMove best;
    best.row = -1;
    best.col = -1;
    best.forced = false;

    int humanPlayer = (aiPlayer == NORMAL_BLACK) ? NORMAL_WHITE : NORMAL_BLACK;

    int candRow[NORMAL_LINES * NORMAL_LINES];
    int candCol[NORMAL_LINES * NORMAL_LINES];
    int candCount = 0;
    int stoneCount = 0;
    collectCandidates(2, candRow, candCol, candCount, stoneCount);

    /* 情况 1：空棋盘，天元开局 */
    if(stoneCount == 0){
        best.row = NORMAL_LINES / 2;
        best.col = NORMAL_LINES / 2;
        return best;
    }

    /* 情况 2 与 3：先找必杀点，再找必堵点。
     * 两者都只需"在某个方向能连成五"，因此可以直接用 lineHasFive 判定，
     * 不必走完整的评分流程。胜负永远优先于局面分。 */
    int blockRow = -1;
    int blockCol = -1;
    bool hasBlock = false;

    for(int i = 0; i < candCount; i++){
        int row = candRow[i];
        int col = candCol[i];

        /* 这一步下去我是否直接五连？ */
        bool winHere = false;
        for(int d = 0; d < 4; d++){
            if(lineHasFive(row, col, DIR_ROW[d], DIR_COL[d], aiPlayer)){
                winHere = true;
                break;
            }
        }
        if(winHere){
            best.row = row;
            best.col = col;
            best.forced = true;
            return best;   /* 有杀招就不必再看了 */
        }

        /* 这一步如果让对手下下去，对手是否五连？记住这个必堵点。
         * 不立即返回，是为了把所有必堵点都找出来。 */
        if(!hasBlock){
            for(int d = 0; d < 4; d++){
                if(lineHasFive(row, col, DIR_ROW[d], DIR_COL[d], humanPlayer)){
                    blockRow = row;
                    blockCol = col;
                    hasBlock = true;
                    break;
                }
            }
        }
    }

    /* 情况 3：对手有一步成五的杀招，必须堵。
     * 补充说明：如果对手同时有两个成五点（双四），堵一个也会输——
     * 那是前一步防守失误造成的，此处已无法挽回，属于正常现象。 */
    if(hasBlock){
        best.row = blockRow;
        best.col = blockCol;
        best.forced = true;
        return best;
    }

    /* 情况 4：综合评分选点。
     * 对每个候选点，把子"试下"在该点，然后：
     *     进攻分 = 我下这里后四个方向的棋型分之和
     *     防守分 = 假设对手下这里，他能形成的棋型分之和
     * 综合分 = 进攻分 + DEFENSE_WEIGHT × 防守分
     * 这样既会主动造活三 / 冲四（进攻分高），
     * 也会去堵对手的活三 / 冲四（对手在此点的分高，防守分就高）。 */
    int bestScore = -1;
    for(int i = 0; i < candCount; i++){
        int row = candRow[i];
        int col = candCol[i];

        int attack  = scorePoint(row, col, aiPlayer);
        int defense = scorePoint(row, col, humanPlayer);

        /* 已经保证没有"一步成五"的点，所以这里出现 SCORE_FIVE 只可能来自
         * 我方（理论上已被上面的必杀分支拦掉），保险起见仍然特判一次。 */
        if(attack >= SCORE_FIVE){
            best.row = row;
            best.col = col;
            best.forced = true;
            return best;
        }

        int total = attack + (int)(defense * DEFENSE_WEIGHT);

        /* 同分时选更靠近棋盘中心的点：中心附近的棋型发展空间最大，
         * 这是五子棋的通用经验，也让 AI 不至于贴着边线乱下。 */
        if(total > bestScore){
            bestScore = total;
            best.row = row;
            best.col = col;
        }else if(total == bestScore && best.row >= 0){
            int mid = NORMAL_LINES / 2;
            int newDist = abs(row - mid) + abs(col - mid);
            int oldDist = abs(best.row - mid) + abs(best.col - mid);
            if(newDist < oldDist){
                best.row = row;
                best.col = col;
            }
        }
    }

    /* candCount 为 0 说明棋盘已满（没有相邻空点），此时 best.row 保持 -1，
     * 调用方应当据此判和，而不是继续调用 placeChess。 */
    return best;
}

/* ===========================================================================
 *  第四部分：自测代码（可选）
 *  用 g++ 编译时加上 -DAI_NORMAL_TEST 才会生效，例如：
 *      g++ -DAI_NORMAL_TEST ai_normal.cpp -o ai_normal_test.exe
 *  在 Dev-C++ 里正常编译工程时不会包含这段，不影响原有程序。
 *  自测覆盖了普通模式 AI 的六种基本战术场景，用于验证代码确实"会堵会杀"。
 * ========================================================================= */
#ifdef AI_NORMAL_TEST

#include <cstdio>

static const char *cellText(int v){
    if(v == NORMAL_BLACK) return "黑";
    if(v == NORMAL_WHITE) return "白";
    return "·";
}

/* 以文字形式打印棋盘，便于肉眼核对自测用例的局面 */
static void printBoard(){
    std::printf("     ");
    for(int c = 0; c < NORMAL_LINES; c++) std::printf("%2d", c);
    std::printf("\n");
    for(int r = 0; r < NORMAL_LINES; r++){
        std::printf("%3d  ", r);
        for(int c = 0; c < NORMAL_LINES; c++){
            std::printf("%s", cellText(board[r][c]));
        }
        std::printf("\n");
    }
}

static void clearBoard(){ memset(board, 0, sizeof(board)); }
static void putStone(int row, int col, int player){ board[row][col] = player; }

static int testCount = 0;
static int failed = 0;

/*
 * 检查一个用例：AI 必须在给定的一批"可接受落点"之一落子。
 * 之所以允许一批点而不是唯一解，是因为像"四连补五""活三堵点"这类局面
 * 本来就有两个等价的正确位置（堵左边或堵右边都行）。
 */
static void checkCase(const char *title, int aiPlayer,
                      const int expectRow[], const int expectCol[], int expectCount){
    NormalMove mv = chooseNormalMove(aiPlayer);

    bool ok = false;
    for(int i = 0; i < expectCount; i++){
        if(mv.row == expectRow[i] && mv.col == expectCol[i]){
            ok = true;
            break;
        }
    }

    testCount++;
    if(ok){
        std::printf("[通过] %s -> 落点 (%d, %d)，forced=%d\n", title, mv.row, mv.col, (int)mv.forced);
    }else{
        failed++;
        std::printf("[失败] %s -> 期望 ", title);
        for(int i = 0; i < expectCount; i++){
            std::printf("(%d,%d)%s", expectRow[i], expectCol[i], (i + 1 < expectCount) ? " 或 " : "");
        }
        std::printf("，实际 (%d, %d)\n", mv.row, mv.col);
    }

    printBoard();
    std::printf("\n");
}

int main(){
    std::printf("========== 普通模式 AI 战术自测 ==========\n\n");

    /* 用例 1：白棋横着已有 4 连（第 7 行列 3~6），AI 执白应当立刻补成五连。
     * 检验"必杀"战术：有赢棋的机会必须先赢。 */
    clearBoard();
    for(int c = 3; c <= 6; c++) putStone(7, c, NORMAL_WHITE);
    putStone(6, 6, NORMAL_BLACK);
    std::printf("--- 用例1 白棋横向四连，AI 执白应有杀招 ---\n");
    printBoard();
    { const int er[] = {7, 7}; const int ec[] = {2, 7};
      checkCase("用例1 白棋四连补五", NORMAL_WHITE, er, ec, 2); }

    /* 用例 2：黑棋竖着已有 4 连（第 4~7 行列 8），AI 执白自己没有杀招，
     * 必须堵住 (3,8) 或 (8,8)。检验"必堵"战术，这是简单模式做不到的。 */
    clearBoard();
    for(int r = 4; r <= 7; r++) putStone(r, 8, NORMAL_BLACK);
    putStone(7, 7, NORMAL_WHITE);
    std::printf("--- 用例2 黑棋纵向四连，AI 执白必须堵 ---\n");
    printBoard();
    { const int er[] = {3, 8}; const int ec[] = {8, 8};
      checkCase("用例2 堵黑棋四连", NORMAL_WHITE, er, ec, 2); }

    /* 用例 3：黑棋斜着已有 4 连（(4,4)~(7,7)），检验斜线方向的必堵判断。 */
    clearBoard();
    for(int i = 0; i < 4; i++) putStone(4 + i, 4 + i, NORMAL_BLACK);
    putStone(7, 4, NORMAL_WHITE);
    std::printf("--- 用例3 黑棋斜向四连，AI 执白必须堵 ---\n");
    printBoard();
    { const int er[] = {3, 8}; const int ec[] = {3, 8};
      checkCase("用例3 堵黑棋斜线四连", NORMAL_WHITE, er, ec, 2); }

    /* 用例 4：黑棋活三（第 7 行列 5~7，两端为空）。活三不堵，对手下一步
     * 就是活四，必输。这一条检验综合评分里的"防守分"是否真的起作用——
     * 注意此时没有一步成五的紧急情况，所以 forced 应为 0。 */
    clearBoard();
    for(int c = 5; c <= 7; c++) putStone(7, c, NORMAL_BLACK);
    putStone(7, 2, NORMAL_WHITE);
    std::printf("--- 用例4 黑棋活三，AI 执白应主动防守 ---\n");
    printBoard();
    { const int er[] = {7, 7}; const int ec[] = {4, 8};
      checkCase("用例4 堵黑棋活三", NORMAL_WHITE, er, ec, 2); }

    /* 用例 5：均势局面，双方都没有立刻成五的机会。AI 应选择一个有价值的
     * 位置，而不是贴着边线下棋。这里只要求落点靠近棋盘中心附近。 */
    clearBoard();
    putStone(7, 7, NORMAL_WHITE);
    putStone(7, 8, NORMAL_BLACK);
    std::printf("--- 用例5 均势局面，AI 执白应选在中心附近 ---\n");
    printBoard();
    {
        NormalMove mv = chooseNormalMove(NORMAL_WHITE);
        int mid = NORMAL_LINES / 2;
        testCount++;
        bool ok = (mv.row >= 0 && abs(mv.row - mid) <= 3 && abs(mv.col - mid) <= 3);
        if(ok) std::printf("[通过] 用例5 均势下选点靠近中心 -> (%d, %d)\n", mv.row, mv.col);
        else { failed++; std::printf("[失败] 用例5 落点 (%d, %d) 离中心过远\n", mv.row, mv.col); }
        std::printf("\n");
    }

    /* 用例 6：空棋盘应当直接走天元 (7,7)，这是五子棋常识开局。 */
    clearBoard();
    std::printf("--- 用例6 空棋盘，AI 执黑应先占天元 ---\n");
    { const int er[] = {7}; const int ec[] = {7};
      checkCase("用例6 空盘走天元", NORMAL_BLACK, er, ec, 1); }

    std::printf("========== 共 %d 项，失败 %d 项 ==========\n", testCount, failed);
    return failed == 0 ? 0 : 1;
}

#endif /* AI_NORMAL_TEST */
