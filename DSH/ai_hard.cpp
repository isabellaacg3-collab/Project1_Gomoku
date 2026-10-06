/*
 * ============================================================================
 *  ai_hard.cpp  ——  五子棋「困难模式」人机对战 AI
 * ----------------------------------------------------------------------------
 *  与 ai_normal.cpp 的关系：
 *    本文件是**完全独立**的新文件。不复用普通模式的入口
 *    （chooseNormalMove），也**不修改** ai_normal.cpp 的任何代码。
 *    两者只是"思路同源"：都靠棋型评分表判断好坏，但困难模式把评分
 *    放进了 Minimax + Alpha-Beta 剪枝的博弈搜索里，因此会前瞻若干步，
 *    能看到"我走这里 → 对手必然那样应 → 我再这样杀"的连续算杀线路，
 *    而普通模式只是对当前局面做一次静态打分（贪心，无前瞻）。
 *
 *  本文件的四块核心内容：
 *    第 1 部分  棋型评估（评估函数）—— 给一个局面打分
 *    第 2 部分  候选着法与着法排序（Move Ordering）
 *    第 3 部分  Minimax + Alpha-Beta 剪枝搜索  ← 本文件重点
 *    第 4 部分  对外接口 chooseHardMove
 *
 *  对外接口：
 *      HardMove chooseHardMove(int aiPlayer);
 *      返回值 .row / .col 为选中的落点，.score 为搜索得到的分值，
 *      .depth 为实际搜索深度，.forced 表示走的是"一步成五 / 必堵"。
 *
 *  说明：本文件**不实现禁手规则**（不做三三禁手、四四禁手、长连禁手的判定），
 *        连成五子及以上即算获胜，与原工程 checkWin 的"count >= 5"一致。
 *
 *  文件编码：GBK / ANSI（无 BOM）。原因是 Dev-C++ 5.11 在中文 Windows 下
 *  按 ANSI(GBK) 打开源文件；若存成 UTF-8，中文注释在 Dev-C++ 里会变成乱码。
 *  请勿用 UTF-8 重新保存本文件。
 *
 *  在工程里启用（两步）：
 *    1) 把本文件加入 Project1_Gomoku.dev，或在 Makefile.win 的
 *       OBJ / LINKOBJ 里加 ai_hard.o 并补一条编译规则；
 *    2) 在 main.cpp 的游戏循环里调用（务必保留"轮到 AI 才走"的条件）：
 *           if(gamestate == AI_GAME_HARD && currentPlayer == 2
 *           && !roundOver && !gameOver){
 *               HardMove mv = chooseHardMove(currentPlayer);
 *               if(mv.row >= 0){
 *                   placeChess(mv.row, mv.col, currentPlayer);
 *                   if(!checkWin(mv.row, mv.col, currentPlayer)){
 *                       switchPlayer(currentPlayer);
 *                   }
 *               }
 *           }
 *       即可替换掉原来被注释掉的 //hardAIGame();
 * ============================================================================
 */

#include <cstddef>   /* size_t */
#include <cstring>   /* memset */
#include <cstdlib>   /* abs   */
#include <ctime>     /* clock */
#include <cstdio>    /* printf（仅自测用） */

/* 自用一套常量与棋盘副本，保证本文件能独立编译、也不和现有代码冲突。
 * 数值与原工程保持一致：空=0、白=1、黑=2。 */
const int HARD_LINES = 15;
const int HARD_EMPTY = 0;
const int HARD_WHITE = 1;
const int HARD_BLACK = 2;

static int g_board[HARD_LINES][HARD_LINES];

/* 四个搜索方向：横、竖、主对角线、副对角线 */
static const int DIR_R[4] = {0, 1, 1, -1};
static const int DIR_C[4] = {1, 0, 1,  1};

/* ---------------------------------------------------------------------------
 *  棋型分值表
 *  这些分值是"局部棋型分"，会把四个方向相加，因此单点最高约 4 × 18000。
 *  为了不和"胜负分"混淆，搜索里用独立的 WIN_SCORE 表示真正的输赢，
 *  二者的量纲通过 LEAF_VALUE_CAP 隔开（见下面两行注释）。
 * ------------------------------------------------------------------------- */
static const int SC_FIVE      = 100000;  /* 五连（实际胜负由 WIN_SCORE 表示） */
static const int SC_OPEN_FOUR = 18000;   /* 活四：两端都空，对手堵不住      */
static const int SC_FOUR      = 2200;    /* 冲四：一端被堵，下一手即成五    */
static const int SC_OPEN_THREE = 2000;   /* 活三：下一手可变活四            */
static const int SC_SLEEP_THREE = 220;   /* 眠三：一端被堵的三              */
static const int SC_OPEN_TWO  = 200;     /* 活二                            */
static const int SC_SLEEP_TWO = 30;      /* 眠二                            */
static const int SC_ONE       = 2;       /* 孤子                            */
static const int SC_DEAD      = -1;      /* 死点，无法形成任何连子          */

/* 叶节点评估分的绝对值上限。取 4 × SC_OPEN_FOUR 再多留一点余量，
 * 确保任何静态评估分都落在 ±LEAF_VALUE_CAP 之内；
 * 而 WIN_SCORE 远大于它，于是"评估分"不可能冒充"胜负分"。 */
static const int LEAF_VALUE_CAP = 4 * SC_OPEN_FOUR + 1000;

/* 胜负分：搜到必胜局面时返回 ±(WIN_SCORE - 层数)。
 * 减去层数（ply）的作用是"更快取胜的线路分更高"，
 * 这样 AI 会优先选择 3 步杀而不是 5 步杀。 */
static const int WIN_SCORE = 1000000;

/* 每个方向最多看 4 格：加上落子本身最多构成 5 连，更远的子无意义 */
static const int MAX_SCAN = 4;

static bool inBoard(int row, int col){
    return row >= 0 && row < HARD_LINES && col >= 0 && col < HARD_LINES;
}

static int otherPlayer(int player){
    return (player == HARD_BLACK) ? HARD_WHITE : HARD_BLACK;
}

/* ===========================================================================
 *  第 1 部分：棋型评估（评估函数）
 *  评估函数是博弈搜索的"眼睛"：搜索只能比较分值，分值从哪来就靠这里。
 *  思路与普通模式一致——数连子、数空端、查表——但困难模式把它用在
 *  搜索树的叶节点上，而不是只对当前局面用一次。
 * ========================================================================= */

/*
 * 数连子：从 (row, col) 沿 (dr, dc) 方向连续同色棋子的个数（不含自身）。
 * blocked 输出该方向是否"被挡住"（撞空位、撞敌子或撞棋盘边界都算被挡）。
 */
static int countRun(int row, int col, int dr, int dc, int player, bool *blocked){
    int count = 0;
    int r = row + dr;
    int c = col + dc;
    for(int step = 0; step < MAX_SCAN; step++){
        if(!inBoard(r, c)){                 /* 撞边界 = 被挡 */
            if(blocked) *blocked = true;
            break;
        }
        if(g_board[r][c] != player){        /* 撞空位或敌子 = 被挡 */
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
 * 判断在 (row, col) 落子后，player 是否在这条线上已经连成五子。
 * 这是"真正的胜负判定"，与评分表里的 SC_FIVE 是两回事：
 * 评分表是估值，这里才是规则上的赢。
 */
static bool lineHasFive(int row, int col, int dr, int dc, int player){
    int total = 1;
    total += countRun(row, col,  dr,  dc, player, NULL);
    total += countRun(row, col, -dr, -dc, player, NULL);
    return total >= 5;   /* 长连（6 子以上）也算胜，不作禁手判定 */
}

/* 在 (row, col) 落子后，player 是否形成五连（四个方向任一即可） */
static bool makesFive(int row, int col, int player){
    for(int d = 0; d < 4; d++){
        if(lineHasFive(row, col, DIR_R[d], DIR_C[d], player)){
            return true;
        }
    }
    return false;
}

/*
 * 单方向棋型分：统计"若 player 落在 (row, col)"这条线上的连子数与开放端数，
 * 再按 (连子数, 端点) 查表。开放端越多、连子越多，分越高。
 */
static int scoreDirection(int row, int col, int dr, int dc, int player){
    bool blockedA = false;
    bool blockedB = false;
    int forward  = countRun(row, col,  dr,  dc, player, &blockedA);
    int backward = countRun(row, col, -dr, -dc, player, &blockedB);

    int total = 1 + forward + backward;
    int ends  = (blockedA ? 0 : 1) + (blockedB ? 0 : 1);

    if(total >= 5){ return SC_FIVE; }
    if(total == 4){ return (ends == 2) ? SC_OPEN_FOUR : SC_FOUR; }
    if(total == 3){ return (ends == 2) ? SC_OPEN_THREE : SC_SLEEP_THREE; }
    if(total == 2){ return (ends == 2) ? SC_OPEN_TWO : SC_SLEEP_TWO; }
    return (ends >= 1) ? SC_ONE : SC_DEAD;
}

/* 四个方向的棋型分之和：代表"player 落此点"的价值 */
static int scorePoint(int row, int col, int player){
    int total = 0;
    for(int d = 0; d < 4; d++){
        total += scoreDirection(row, col, DIR_R[d], DIR_C[d], player);
    }
    return total;
}

/*
 * 某个 player 的**全盘**棋型分。
 * 做法：只扫已落子的格子，且只统计"该子所在方向上自己形成的棋型"，
 * 用自造的 MAX_SCAN 窗口限制长度，避免把同一条线重复算很多次。
 * 这是对局面的近似估计（不是精确枚举所有棋型），
 * 但对搜索排序与强弱判断已经足够，而且比"逐空点试下"快得多。
 */
static int boardScoreOf(int player){
    int score = 0;
    for(int row = 0; row < HARD_LINES; row++){
        for(int col = 0; col < HARD_LINES; col++){
            if(g_board[row][col] != player){
                continue;
            }
            for(int d = 0; d < 4; d++){
                /* 只从"这条线的起点"统计一次：若反向还有同色子，
                 * 说明本子不是起点，跳过，避免同一条线被反复累加。 */
                int pr = row - DIR_R[d];
                int pc = col - DIR_C[d];
                if(inBoard(pr, pc) && g_board[pr][pc] == player){
                    continue;
                }
                bool blocked = false;
                int len = 1 + countRun(row, col, DIR_R[d], DIR_C[d], player, &blocked);
                /* 反向那一格已知不是同色子；它若为空则这一端是开放的 */
                int forwardOpen = blocked ? 0 : 1;
                int backwardOpen = (inBoard(pr, pc) && g_board[pr][pc] == HARD_EMPTY) ? 1 : 0;
                int ends = forwardOpen + backwardOpen;

                if(len >= 5)      score += SC_FIVE;
                else if(len == 4) score += (ends == 2) ? SC_OPEN_FOUR : SC_FOUR;
                else if(len == 3) score += (ends == 2) ? SC_OPEN_THREE : SC_SLEEP_THREE;
                else if(len == 2) score += (ends == 2) ? SC_OPEN_TWO : SC_SLEEP_TWO;
                else              score += SC_ONE;
            }
        }
    }
    return score;
}

/* 落在 (row, col) 的这枚 player 棋子，对全盘分的贡献。
 * 与 boardScoreOf 的统计口径完全一致，专供"增量更新"使用。 */
static int stoneContribution(int row, int col, int player){
    int score = 0;
    for(int d = 0; d < 4; d++){
        int pr = row - DIR_R[d];
        int pc = col - DIR_C[d];
        if(inBoard(pr, pc) && g_board[pr][pc] == player){
            continue;   /* 不是这条线的起点，由起点统一统计 */
        }
        bool blocked = false;
        int len = 1 + countRun(row, col, DIR_R[d], DIR_C[d], player, &blocked);
        int forwardOpen = blocked ? 0 : 1;
        int backwardOpen = (inBoard(pr, pc) && g_board[pr][pc] == HARD_EMPTY) ? 1 : 0;
        int ends = forwardOpen + backwardOpen;

        if(len >= 5)      score += SC_FIVE;
        else if(len == 4) score += (ends == 2) ? SC_OPEN_FOUR : SC_FOUR;
        else if(len == 3) score += (ends == 2) ? SC_OPEN_THREE : SC_SLEEP_THREE;
        else if(len == 2) score += (ends == 2) ? SC_OPEN_TWO : SC_SLEEP_TWO;
        else              score += SC_ONE;
    }
    return score;
}

/*
 * 增量维护的双方全盘分。
 * 搜索要访问成千上万个节点，如果在每个叶节点都重算一遍全盘分，
 * 速度会慢到无法接受。所以这里记住当前分数，落子/撤销时只更新
 * 受影响的四个方向——这是让 Alpha-Beta 搜索在 15×15 棋盘上跑得动的关键。
 */
static int g_scoreWhite = 0;
static int g_scoreBlack = 0;

/*
 * 落子 (row, col) 并更新增量分。
 * 思路：放下一枚子会改变它四个方向上所有相关棋子的棋型，
 * 因此先把"紧邻的、属于自己的线起点"的旧贡献扣掉，再统一重算。
 * 这样实现简单、口径一致，而且比全盘重算快得多。
 */
static void placeStone(int row, int col, int player){
    /* 扣掉四周同色棋子原有的贡献（它们的棋型即将改变） */
    for(int d = 0; d < 4; d++){
        for(int sgn = -1; sgn <= 1; sgn += 2){
            int r = row + DIR_R[d] * sgn;
            int c = col + DIR_C[d] * sgn;
            if(inBoard(r, c) && g_board[r][c] == player){
                int old = stoneContribution(r, c, player);
                if(player == HARD_WHITE) g_scoreWhite -= old;
                else                     g_scoreBlack -= old;
            }
        }
    }

    g_board[row][col] = player;

    /* 补上自己与四周同色棋子的新贡献 */
    int mine = stoneContribution(row, col, player);
    if(player == HARD_WHITE) g_scoreWhite += mine;
    else                     g_scoreBlack += mine;

    for(int d = 0; d < 4; d++){
        for(int sgn = -1; sgn <= 1; sgn += 2){
            int r = row + DIR_R[d] * sgn;
            int c = col + DIR_C[d] * sgn;
            if(inBoard(r, c) && g_board[r][c] == player){
                int now = stoneContribution(r, c, player);
                if(player == HARD_WHITE) g_scoreWhite += now;
                else                     g_scoreBlack += now;
            }
        }
    }
}

/* 撤销 (row, col) 的落子并更新增量分：做法与 placeStone 对称 */
static void undoStone(int row, int col, int player){
    for(int d = 0; d < 4; d++){
        for(int sgn = -1; sgn <= 1; sgn += 2){
            int r = row + DIR_R[d] * sgn;
            int c = col + DIR_C[d] * sgn;
            if(inBoard(r, c) && g_board[r][c] == player){
                int old = stoneContribution(r, c, player);
                if(player == HARD_WHITE) g_scoreWhite -= old;
                else                     g_scoreBlack -= old;
            }
        }
    }

    g_board[row][col] = HARD_EMPTY;

    for(int d = 0; d < 4; d++){
        for(int sgn = -1; sgn <= 1; sgn += 2){
            int r = row + DIR_R[d] * sgn;
            int c = col + DIR_C[d] * sgn;
            if(inBoard(r, c) && g_board[r][c] == player){
                int now = stoneContribution(r, c, player);
                if(player == HARD_WHITE) g_scoreWhite += now;
                else                     g_scoreBlack += now;
            }
        }
    }
}

/* 重新计算全盘双方分（仅在搜索开始前调用一次） */
static void recomputeScores(){
    g_scoreWhite = boardScoreOf(HARD_WHITE);
    g_scoreBlack = boardScoreOf(HARD_BLACK);
}

/*
 * 评估函数：始终站在"AI（rootPlayer）"的视角给出分值。
 *     分值 = 我方棋型分 - 防守系数 × 对手棋型分
 * 防守系数取 1.05（略大于 1）的含义是：同等棋型下宁可先化解对手的威胁，
 * 这与普通模式取 0.9（偏爱进攻）不同——困难模式更看重不被算杀。
 * 再加上一个朝向棋盘中心的位置奖励，避免 AI 贴着边线下棋。
 */
static const int DEFENSE_PCT = 105;   /* 防守权重 1.05，用百分比做整数运算 */
static int g_rootPlayer = HARD_BLACK;

static int evaluate(){
    int mine = (g_rootPlayer == HARD_WHITE) ? g_scoreWhite : g_scoreBlack;
    int opp  = (g_rootPlayer == HARD_WHITE) ? g_scoreBlack : g_scoreWhite;
    int value = mine - (int)((long long)opp * DEFENSE_PCT / 100);

    /* 位置分：越靠近中心越好，让 AI 的开局与均势着法更合理 */
    int mid = HARD_LINES / 2;
    for(int row = 0; row < HARD_LINES; row++){
        for(int col = 0; col < HARD_LINES; col++){
            if(g_board[row][col] != HARD_EMPTY){ continue; }
            int dist = abs(row - mid) + abs(col - mid);
            value += (2 * mid - dist) / 4;
        }
    }

    /* 夹到评估分上限之内，保证它不会越过 WIN_SCORE 的量级 */
    if(value >  LEAF_VALUE_CAP) value =  LEAF_VALUE_CAP;
    if(value < -LEAF_VALUE_CAP) value = -LEAF_VALUE_CAP;
    return value;
}

/* ===========================================================================
 *  第 2 部分：候选着法与着法排序（Move Ordering）
 *  Alpha-Beta 剪枝的效果**极度依赖着法顺序**：
 *  先搜好的着法，就能尽早把窗口收窄，后面大量分支被整枝剪掉。
 *  这一段就是"给着法排个好顺序"。
 * ========================================================================= */

static const int MAX_CAND = HARD_LINES * HARD_LINES;

/* (row, col) 附近 offset 格内是否有棋子 */
static bool hasNeighbor(int row, int col, int offset){
    for(int dr = -offset; dr <= offset; dr++){
        for(int dc = -offset; dc <= offset; dc++){
            if(dr == 0 && dc == 0){ continue; }
            int r = row + dr;
            int c = col + dc;
            if(inBoard(r, c) && g_board[r][c] != HARD_EMPTY){
                return true;
            }
        }
    }
    return false;
}

/*
 * 生成候选着法：只考虑"贴着已有棋子"的空点。
 * 这一步把分支因子从 225 降到几十，是搜索能跑完的前提。
 * 返回候选个数；若返回 0，说明棋盘已满（平局）。
 */
static int genCandidates(int candRow[], int candCol[]){
    int count = 0;
    for(int row = 0; row < HARD_LINES; row++){
        for(int col = 0; col < HARD_LINES; col++){
            if(g_board[row][col] == HARD_EMPTY && hasNeighbor(row, col, 2)){
                candRow[count] = row;
                candCol[count] = col;
                count++;
            }
        }
    }
    return count;
}

/*
 * 着法排序用的启发分值：
 *      我方在此点的进攻价值  +  对手在此点的威胁价值 × 1.1
 * 也就是"这步棋能给自己造多大棋型、同时能破坏对手多大棋型"。
 * 注意它只是**排序用的猜测**，不参与最终判分（判分由 evaluate 负责）。
 */
static int orderScore(int row, int col, int me, int opp){
    int attack  = scorePoint(row, col, me);
    int defense = scorePoint(row, col, opp);
    int score = attack + (int)((long long)defense * 110 / 100);

    /* 一步成五的着法必须排在最前面，它是最有价值的分支 */
    if(makesFive(row, col, me)){
        score += WIN_SCORE / 2;
    }
    return score;
}

/* 选择排序：按 orderScore 从大到小整理候选数组（候选数不大，够用） */
static void sortCandidates(int candRow[], int candCol[], int score[], int count, int limit){
    int n = (count < limit) ? count : limit;
    for(int i = 0; i < n; i++){
        int best = i;
        for(int j = i + 1; j < count; j++){
            if(score[j] > score[best]){
                best = j;
            }
        }
        if(best != i){
            int tr = candRow[i]; candRow[i] = candRow[best]; candRow[best] = tr;
            int tc = candCol[i]; candCol[i] = candCol[best]; candCol[best] = tc;
            int ts = score[i];   score[i]   = score[best];   score[best]   = ts;
        }
    }
}

/* ===========================================================================
 *  第 3 部分：Minimax + Alpha-Beta 剪枝  ← 本文件的核心
 * ---------------------------------------------------------------------------
 *  【Minimax 的思想】
 *    把对局看成一颗博弈树：我走一步（MAX 层，我挑对我最有利的），
 *    对手走一步（MIN 层，对手挑对我最不利的）。双方都"理性"，
 *    于是从叶子往上回传：
 *        叶节点          -> 用评估函数打分
 *        MAX 层（我方走） -> 取所有子节点中的**最大值**（我选最好的）
 *        MIN 层（对方走） -> 取所有子节点中的**最小值**（对手选让我最差的）
 *    根节点最终选中的那步，就是"假设对手也会最优应对时，对我最好的着法"。
 *
 *  【为什么需要 Alpha-Beta 剪枝】
 *    纯 Minimax 要遍历整棵树，节点数随深度指数爆炸（分支因子 b、深度 d
 *    时约 b^d 个节点），15×15 棋盘根本搜不动。
 *    Alpha-Beta 剪枝在数学上**不改变** Minimax 的结果，却能把大量
 *    不可能影响最终决策的分支直接砍掉。它维护两个边界：
 *        alpha = MAX 层目前已经**保证能拿到**的最好分值（下界）
 *        beta  = MIN 层目前已经**保证能把对手压到**的最差分值（上界）
 *    关键剪枝条件（对 MAX 层）：若某个子节点返回的分值 v >= beta，
 *    说明这一步棋好到"对手在上一层绝不会允许走到这个局面"，
 *    于是剩下的兄弟节点**再也不用看了**，直接返回（Beta 截断）。
 *    MIN 层对称：若 v <= alpha，同样可以立刻返回（Alpha 截断）。
 *    当 alpha >= beta 时窗口失效，这整棵子树与最终决策无关。
 *
 *  【让它真的"剪得动"的三个工程手段】
 *    1. 着法排序（第 2 部分）：先搜高分着法，剪枝率能从"几乎不剪"提升到
 *       "剪掉绝大部分"，这是最有效的一招；
 *    2. 候选点限制：只考虑棋子附近的空点，并把每层分支数限制在 TOP_N；
 *    3. 置换表：同一个局面可能从不同着法顺序到达，缓存起来避免重算。
 * ========================================================================= */

/* 每层最多展开多少个候选着法（配合排序取前 TOP_N 个） */
static const int ROOT_TOP_N = 16;   /* 根节点要更全面，留多一点 */
static const int INNER_TOP_N = 10;  /* 内部节点靠排序取前 10 个即可 */

/* ---------------- 置换表：缓存"同一局面是否已经算过" ---------------- */
/* 用棋盘字符串做键：直观、无哈希冲突，代价是每次比较要拼一次字符串。
 * 对教学示例来说，正确性比极致速度更重要。 */
static const int TT_SIZE = 512;
static const int TT_KEY_LEN = HARD_LINES * HARD_LINES + 1;

struct TTEntry{
    char key[TT_KEY_LEN];
    int  depth;      /* 该分值是在多深时算出来的 */
    int  flag;       /* TT_EXACT / TT_LOWER / TT_UPPER */
    int  value;
};

static const int TT_EXACT = 0;
static const int TT_LOWER = 1;   /* 真实值 >= value（因剪枝没能算准） */
static const int TT_UPPER = 2;   /* 真实值 <= value */

static TTEntry g_tt[TT_SIZE];

static void ttClear(){
    for(int i = 0; i < TT_SIZE; i++){
        g_tt[i].depth = -1;
        g_tt[i].flag  = TT_EXACT;
        g_tt[i].key[0] = '\0';
    }
}

/* 把当前棋盘编码成字符串（0/1/2 各占一个字符） */
static void boardKey(char key[]){
    int k = 0;
    for(int row = 0; row < HARD_LINES; row++){
        for(int col = 0; col < HARD_LINES; col++){
            key[k++] = (char)('0' + g_board[row][col]);
        }
    }
    key[k] = '\0';
}

/* 查询置换表：命中且深度足够时，可直接使用缓存分值 */
static bool ttLookup(const char key[], int depth, int alpha, int beta, int *value){
    for(int i = 0; i < TT_SIZE; i++){
        if(g_tt[i].depth >= 0 && strcmp(g_tt[i].key, key) == 0){
            if(g_tt[i].depth >= depth){
                if(g_tt[i].flag == TT_EXACT){
                    *value = g_tt[i].value;
                    return true;
                }
                if(g_tt[i].flag == TT_LOWER && g_tt[i].value >= beta){
                    *value = g_tt[i].value;
                    return true;
                }
                if(g_tt[i].flag == TT_UPPER && g_tt[i].value <= alpha){
                    *value = g_tt[i].value;
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

/* 写入置换表：优先覆盖"深度更浅"的旧条目 */
static void ttStore(const char key[], int depth, int value, int flag){
    int slot = -1;
    for(int i = 0; i < TT_SIZE; i++){
        if(g_tt[i].depth >= 0 && strcmp(g_tt[i].key, key) == 0){
            slot = i;
            break;
        }
    }
    if(slot < 0){
        for(int i = 0; i < TT_SIZE; i++){
            if(g_tt[i].depth < 0){ slot = i; break; }
        }
    }
    if(slot < 0){
        slot = 0;
        for(int i = 1; i < TT_SIZE; i++){
            if(g_tt[i].depth < g_tt[slot].depth){ slot = i; }
        }
    }
    strcpy(g_tt[slot].key, key);
    g_tt[slot].depth = depth;
    g_tt[slot].value = value;
    g_tt[slot].flag  = flag;
}

/* ---------------- 搜索统计：用来直观展示剪枝效果（自测用） ---------------- */
static long g_nodes       = 0;   /* 实际访问的节点数           */
static long g_cutoffs     = 0;   /* 发生 Alpha / Beta 截断的次数 */

/* 这个开关仅供自测使用：置 1 时把 alpha/beta 当成无穷大、
 * 并停用截断条件，于是搜索退化为纯 Minimax，
 * 用来对比“有没有剪枝”到底差多少个节点。 */
static int g_disablePruning = 0;
/* 下面两个计数器用于自测时展示剪枝效果：节点越少、剪枝越多，Alpha-Beta 越有效 */

/*
 * 递归搜索函数（Negamax 写法：始终返回"当前轮到走棋一方的视角"分值）。
 * 这样只需要写一个函数，配合"取负"就能同时表达 MAX 层与 MIN 层，
 * 代码更短，也更容易和 Alpha-Beta 的窗口更新对应起来。
 *
 *   player : 当前轮到走棋的一方
 *   depth  : 还要往下搜几层（0 表示到达叶节点，直接调用评估函数）
 *   alpha  : 当前 MAX 层能保证拿到的最好分值（下界）
 *   beta   : 当前 MIN 层能把对手压到的最差分值（上界）
 *   ply    : 距离根节点的层数，用于"越快取胜分越高"的调整
 */
static int alphaBeta(int player, int depth, int alpha, int beta, int ply){
    g_nodes++;

    /* 到达搜索深度上限：用评估函数给这个局面打分 */
    if(depth <= 0){
        return evaluate();
    }

    /* 查置换表：同一局面若已算过，直接返回缓存结果，省掉整棵子树 */
    char key[TT_KEY_LEN];
    boardKey(key);
    int cached = 0;
    if(ttLookup(key, depth, alpha, beta, &cached)){
        return cached;
    }

    int candRow[MAX_CAND];
    int candCol[MAX_CAND];
    int candScore[MAX_CAND];
    int count = genCandidates(candRow, candCol);

    /* 没有候选点 = 棋盘已满 = 平局，返回 0 */
    if(count == 0){
        return 0;
    }

    int opp = otherPlayer(player);
    for(int i = 0; i < count; i++){
        candScore[i] = orderScore(candRow[i], candCol[i], player, opp);
    }
    sortCandidates(candRow, candCol, candScore, count, INNER_TOP_N);

    int limit = (count < INNER_TOP_N) ? count : INNER_TOP_N;
    int best = -WIN_SCORE * 2;

    for(int i = 0; i < limit; i++){
        int row = candRow[i];
        int col = candCol[i];

        /* 这一步是否直接连成五子？是就不必再往下搜，直接返回必胜分。
         * "减去 ply"让越早取胜的线路分值越高。 */
        if(makesFive(row, col, player)){
            return WIN_SCORE - ply;
        }

        placeStone(row, col, player);

        /* 递归搜索对手的应对：注意窗口参数要**取负并交换**，
         * 因为在子节点里视角换成了对手。 */
        int value = -alphaBeta(opp, depth - 1, -beta, -alpha, ply + 1);

        undoStone(row, col, player);

        if(value > best){
            best = value;
        }
        /* MAX 层更新下界：我方已经保证至少能拿到 value 分 */
        if(value > alpha){
            alpha = value;
        }

        /* ===== 剪枝点（Beta 截断）=====
         * alpha >= beta 说明：当前这条分支给出的分值，已经超出了
         * 上一层（对手的 MIN 层）愿意接受的范围——对手在上一层
         * 根本不会走到这个局面。因此本层剩下的候选着法毫无意义，
         * 立刻返回，整棵子树被剪掉。这正是 Alpha-Beta 的威力所在。 */
        if(!g_disablePruning && alpha >= beta){
            g_cutoffs++;
            ttStore(key, depth, best, TT_LOWER);
            return best;
        }
    }

    /* 正常搜完：分值是准确的，存为 TT_EXACT */
    ttStore(key, depth, best, TT_EXACT);
    return best;
}

/* ---------------- 根节点搜索：这一层要额外记录"哪一步最好" ---------------- */
/*
 * 根节点单独处理的原因：
 *   1. 根节点需要返回**着法坐标**而不只是分值；
 *   2. 根节点要跑完整的 TOP_N，不能像内部节点那样为省时间而收得太窄；
 *   3. 越早的候选越可能是好棋（已排序），一旦发生截断就可以直接停，
 *      因为后面的着法不可能更好。
 */
static int searchRoot(int player, int depth, int *outRow, int *outCol){
    int candRow[MAX_CAND];
    int candCol[MAX_CAND];
    int candScore[MAX_CAND];
    int count = genCandidates(candRow, candCol);
    if(count == 0){
        *outRow = -1;
        *outCol = -1;
        return 0;
    }

    int opp = otherPlayer(player);
    for(int i = 0; i < count; i++){
        candScore[i] = orderScore(candRow[i], candCol[i], player, opp);
    }
    sortCandidates(candRow, candCol, candScore, count, ROOT_TOP_N);

    int limit = (count < ROOT_TOP_N) ? count : ROOT_TOP_N;
    int best = -WIN_SCORE * 2;
    int alpha = -WIN_SCORE * 2;   /* 根节点的 alpha 初始为 -∞ */
    int beta  =  WIN_SCORE * 2;   /* 根节点的 beta  初始为 +∞ */

    for(int i = 0; i < limit; i++){
        int row = candRow[i];
        int col = candCol[i];

        /* 一步成五：直接就是最优解，无需搜索 */
        if(makesFive(row, col, player)){
            *outRow = row;
            *outCol = col;
            return WIN_SCORE;
        }

        placeStone(row, col, player);
        int value = -alphaBeta(opp, depth - 1, -beta, -alpha, 1);
        undoStone(row, col, player);

        if(value > best){
            best = value;
            *outRow = row;
            *outCol = col;
        }
        if(value > alpha){
            alpha = value;
        }
        /* 根节点的截断：一旦拿到必胜分，后面的着法不可能更好 */
        if(!g_disablePruning && alpha >= beta){
            g_cutoffs++;
            break;
        }
    }
    return best;
}

/* ===========================================================================
 *  第 4 部分：对外接口
 * ========================================================================= */

struct HardMove{
    int row;     /* 行，-1 表示没有可走的点（棋盘已满） */
    int col;     /* 列                                     */
    int score;   /* 搜索得到的分值                         */
    int depth;   /* 实际搜索到的深度                       */
    bool forced; /* 是否是"一步成五 / 必堵"这种战术着法     */
};

/* 迭代加深：从浅到深逐层搜索。
 * 好处是——(1) 浅层先给出一个可用的着法，深层若被时间预算打断也有结果；
 *          (2) 浅层的结果可以用来给深层做更好的着法排序。 */
static const int MAX_DEPTH = 4;              /* 最大搜索深度（4 层约等于双方各走 2 步） */
static const double TIME_BUDGET_SEC = 1.0;   /* 单步思考时间上限（秒） */

HardMove chooseHardMove(int aiPlayer){
    HardMove result;
    result.row = -1;
    result.col = -1;
    result.score = 0;
    result.depth = 0;
    result.forced = false;

    g_rootPlayer = aiPlayer;
    int opp = otherPlayer(aiPlayer);

    int candRow[MAX_CAND];
    int candCol[MAX_CAND];
    int count = genCandidates(candRow, candCol);

    /* 空棋盘：走天元 */
    if(count == 0){
        bool empty = true;
        for(int row = 0; row < HARD_LINES && empty; row++){
            for(int col = 0; col < HARD_LINES; col++){
                if(g_board[row][col] != HARD_EMPTY){ empty = false; break; }
            }
        }
        if(empty){
            result.row = HARD_LINES / 2;
            result.col = HARD_LINES / 2;
            return result;
        }
        /* 非空又无候选点 = 棋盘已满，交给调用方判和 */
        return result;
    }

    /* 先做一次快速战术判断，避免把明显的杀招交给搜索去慢慢找：
     *   1) 我能一步成五      -> 立刻赢
     *   2) 对手能一步成五    -> 必须堵（若同时我也有杀招，上面已优先） */
    int blockRow = -1;
    int blockCol = -1;
    bool hasBlock = false;
    for(int i = 0; i < count; i++){
        int row = candRow[i];
        int col = candCol[i];
        if(makesFive(row, col, aiPlayer)){
            result.row = row;
            result.col = col;
            result.score = WIN_SCORE;
            result.forced = true;
            return result;
        }
        if(!hasBlock && makesFive(row, col, opp)){
            blockRow = row;
            blockCol = col;
            hasBlock = true;
        }
    }
    if(hasBlock){
        result.row = blockRow;
        result.col = blockCol;
        result.score = 0;
        result.forced = true;
        return result;
    }

    /* 迭代加深 + Alpha-Beta 搜索 */
    recomputeScores();
    ttClear();
    g_nodes = 0;
    g_cutoffs = 0;

    int bestRow = -1;
    int bestCol = -1;
    int bestScore = 0;
    bool stopped = false;

    for(int depth = 1; depth <= MAX_DEPTH && !stopped; depth++){
        int row = -1;
        int col = -1;
        int score = searchRoot(aiPlayer, depth, &row, &col);

        if(row >= 0){
            bestRow = row;
            bestCol = col;
            bestScore = score;
            result.depth = depth;
        }

        /* 已经算到必胜或必败的分值就不必再往深了：结果不会改变 */
        if(score >= WIN_SCORE - 100 || score <= -(WIN_SCORE - 100)){
            break;
        }

        /* 时间预算：超了就停，用当前已搜到的最好着法 */
        double used = (double)clock() / CLOCKS_PER_SEC;
        if(used > TIME_BUDGET_SEC){
            stopped = true;
        }
    }

    result.row = bestRow;
    result.col = bestCol;
    result.score = bestScore;

    /* 兜底：极端情况下若搜索没给出着法，就退化为静态评分最高的点 */
    if(result.row < 0){
        int best = -1;
        for(int i = 0; i < count; i++){
            int s = orderScore(candRow[i], candCol[i], aiPlayer, opp);
            if(s > best){
                best = s;
                result.row = candRow[i];
                result.col = candCol[i];
            }
        }
    }
    return result;
}

/* ===========================================================================
 *  第 5 部分：自测代码（可选）
 *  加 -DAI_HARD_TEST 编译才会生效，例如：
 *      g++ -DAI_HARD_TEST ai_hard.cpp -o ai_hard_test.exe
 *  正常编译工程时不包含这段。
 *  自测除了验证战术，还会用同一个局面跑两次搜索（开/关 Alpha-Beta），
 *  打印节点数对比，直观展示剪枝到底省了多少次搜索。
 * ========================================================================= */
#ifdef AI_HARD_TEST

static const char *cellText(int v){
    if(v == HARD_BLACK){ return "黑"; }
    if(v == HARD_WHITE){ return "白"; }
    return "·";
}

static void printBoard(){
    std::printf("     ");
    for(int c = 0; c < HARD_LINES; c++){ std::printf("%2d", c); }
    std::printf("\n");
    for(int r = 0; r < HARD_LINES; r++){
        std::printf("%3d  ", r);
        for(int c = 0; c < HARD_LINES; c++){ std::printf("%s", cellText(g_board[r][c])); }
        std::printf("\n");
    }
}

static void clearBoard(){ memset(g_board, 0, sizeof(g_board)); }
static void putStone(int row, int col, int p){ g_board[row][col] = p; }

static int testCount = 0;
static int failed = 0;

/* 统一的用例检查：除验证落点外，还对比"开剪枝 / 关剪枝"的节点数。
 * 关剪枝时把 alpha/beta 当成无穷大、并停用截断条件，
 * 于是它退化成纯 Minimax——两者答案必须完全一样，差别只在速度。 */
static void checkMove(const char *title, int aiPlayer,
                      const int er[], const int ec[], int n){
    /* 第一次：正常搜索（带 Alpha-Beta 剪枝） */
    g_disablePruning = 0;
    HardMove mv = chooseHardMove(aiPlayer);
    long nodesPruned = g_nodes;
    long cutsPruned  = g_cutoffs;   /* 必须立即取出来，否则会被下一次搜索覆盖 */

    bool ok = false;
    for(int i = 0; i < n; i++){
        if(mv.row == er[i] && mv.col == ec[i]){ ok = true; break; }
    }

    testCount++;
    if(ok){
        std::printf("[通过] %s -> 落点 (%d, %d)，分=%d，深度=%d，forced=%d\n",
                    title, mv.row, mv.col, mv.score, mv.depth, (int)mv.forced);
    }else{
        failed++;
        std::printf("[失败] %s -> 期望 ", title);
        for(int i = 0; i < n; i++){
            std::printf("(%d,%d)%s", er[i], ec[i], (i + 1 < n) ? " 或 " : "");
        }
        std::printf("，实际 (%d, %d)\n", mv.row, mv.col);
    }

    /* 只有真正跑了搜索的局面才值得做剪枝对比 */
    if(mv.depth > 0){
        g_disablePruning = 1;
        HardMove mv2 = chooseHardMove(aiPlayer);
        long nodesPlain = g_nodes;
        g_disablePruning = 0;

        std::printf("       剪枝对比（固定深度 %d）：\n", mv.depth);
        std::printf("         开 Alpha-Beta : 访问节点 %ld，剪枝截断 %ld 次\n",
                    nodesPruned, cutsPruned);
        std::printf("         关 Alpha-Beta : 访问节点 %ld（纯 Minimax）\n", nodesPlain);
        if(nodesPlain > 0){
            std::printf("         节点数降到约 1/%.1f\n",
                        (double)nodesPlain / (double)(nodesPruned > 0 ? nodesPruned : 1));
        }
        if(mv2.row != mv.row || mv2.col != mv.col){
            std::printf("         注意：两种搜索给出不同着法 (%d,%d) vs (%d,%d)"
                        "（同为最优分值时允许并列）\n", mv.row, mv.col, mv2.row, mv2.col);
        }
    }
    printBoard();
    std::printf("\n");
}

int main(){
    std::printf("========== 困难模式 AI 自测（Minimax + Alpha-Beta）==========\n\n");

    /* 用例 1：白棋横着已有 4 连，AI 执白应立刻补成五连获胜 */
    clearBoard();
    for(int c = 3; c <= 6; c++){ putStone(7, c, HARD_WHITE); }
    putStone(6, 6, HARD_BLACK);
    std::printf("--- 用例1 白棋横向四连，AI 执白应一步取胜 ---\n");
    printBoard();
    { const int er[] = {7, 7}; const int ec[] = {2, 7};
      checkMove("用例1 一步成五", HARD_WHITE, er, ec, 2); }

    /* 用例 2：黑棋竖着已有 4 连，AI 执白必须堵 */
    clearBoard();
    for(int r = 4; r <= 7; r++){ putStone(r, 8, HARD_BLACK); }
    putStone(7, 7, HARD_WHITE);
    std::printf("--- 用例2 黑棋纵向四连，AI 执白必须堵 ---\n");
    printBoard();
    { const int er[] = {3, 8}; const int ec[] = {8, 8};
      checkMove("用例2 必堵四连", HARD_WHITE, er, ec, 2); }

    /* 用例 3：黑棋斜着已有 4 连，检验斜线方向 */
    clearBoard();
    for(int i = 0; i < 4; i++){ putStone(4 + i, 4 + i, HARD_BLACK); }
    putStone(7, 4, HARD_WHITE);
    std::printf("--- 用例3 黑棋斜向四连，AI 执白必须堵 ---\n");
    printBoard();
    { const int er[] = {3, 8}; const int ec[] = {3, 8};
      checkMove("用例3 必堵斜线四连", HARD_WHITE, er, ec, 2); }

    /* 用例 4：黑棋活三（第 7 行列 5~7）。活三不是"一步成五"，
     * 必须靠搜索前瞻或评估函数才发现——这正是困难模式比普通模式强的地方。 */
    clearBoard();
    for(int c = 5; c <= 7; c++){ putStone(7, c, HARD_BLACK); }
    putStone(7, 2, HARD_WHITE);
    std::printf("--- 用例4 黑棋活三，AI 执白必须防守 ---\n");
    printBoard();
    { const int er[] = {7, 7}; const int ec[] = {4, 8};
      checkMove("用例4 防守活三", HARD_WHITE, er, ec, 2); }

    /* 用例 5：算杀场景。白棋斜线已有活三 (5,5)(6,6)(7,7)，
     * AI 执白应乘胜追击，把活三扩成活四，形成必胜。
     * 体现"算杀"：不是只看眼前一步，而是看到延伸后的杀棋。 */
    clearBoard();
    putStone(5, 5, HARD_WHITE);
    putStone(6, 6, HARD_WHITE);
    putStone(7, 7, HARD_WHITE);
    putStone(9, 3, HARD_BLACK);
    putStone(9, 4, HARD_BLACK);
    std::printf("--- 用例5 白棋斜向活三，AI 执白应追杀成活四 ---\n");
    printBoard();
    { const int er[] = {4, 8}; const int ec[] = {4, 8};
      checkMove("用例5 活三追杀成活四", HARD_WHITE, er, ec, 2); }

    /* 用例 6：均势局面，AI 应选在中心附近的好点 */
    clearBoard();
    putStone(7, 7, HARD_WHITE);
    putStone(7, 8, HARD_BLACK);
    std::printf("--- 用例6 均势局面，AI 执白应选中心附近 ---\n");
    printBoard();
    {
        HardMove mv = chooseHardMove(HARD_WHITE);
        int mid = HARD_LINES / 2;
        testCount++;
        bool ok = (mv.row >= 0 && abs(mv.row - mid) <= 3 && abs(mv.col - mid) <= 3);
        if(ok){
            std::printf("[通过] 用例6 均势选点 -> (%d, %d)，分=%d，深度=%d\n",
                        mv.row, mv.col, mv.score, mv.depth);
        }else{
            failed++;
            std::printf("[失败] 用例6 落点 (%d, %d) 离中心过远\n", mv.row, mv.col);
        }
        std::printf("\n");
    }

    /* 用例 7：复杂中局。这里最能看出 Alpha-Beta 的价值：
     * 请对比"开剪枝"与"关剪枝"两行的节点数。 */
    clearBoard();
    putStone(7, 7, HARD_BLACK);
    putStone(7, 8, HARD_WHITE);
    putStone(8, 7, HARD_WHITE);
    putStone(6, 6, HARD_BLACK);
    putStone(8, 9, HARD_BLACK);
    putStone(9, 8, HARD_WHITE);
    putStone(5, 5, HARD_WHITE);
    putStone(6, 9, HARD_BLACK);
    std::printf("--- 用例7 复杂中局，观察剪枝对比 ---\n");
    printBoard();
    { g_disablePruning = 0;
      HardMove mv = chooseHardMove(HARD_BLACK);
      long nodesPruned = g_nodes;
      long cutsPruned  = g_cutoffs;
      testCount++;
      if(mv.row >= 0){
          std::printf("[通过] 用例7 中局选点 -> (%d, %d)，分=%d，深度=%d\n",
                      mv.row, mv.col, mv.score, mv.depth);
      }else{
          failed++;
          std::printf("[失败] 用例7 没有给出着法\n");
      }
      g_disablePruning = 1;
      HardMove mv2 = chooseHardMove(HARD_BLACK);
      long nodesPlain = g_nodes;
      g_disablePruning = 0;
      std::printf("       剪枝对比（固定深度 %d）：\n", mv.depth);
      std::printf("         开 Alpha-Beta : 访问节点 %ld，剪枝截断 %ld 次\n",
                  nodesPruned, cutsPruned);
      std::printf("         关 Alpha-Beta : 访问节点 %ld（纯 Minimax）\n", nodesPlain);
      if(nodesPruned > 0){
          std::printf("         节点数降到约 1/%.1f\n",
                      (double)nodesPlain / (double)nodesPruned);
      }
      if(mv2.row != mv.row || mv2.col != mv.col){
          std::printf("         两种搜索着法不同：(%d,%d) vs (%d,%d)\n",
                      mv.row, mv.col, mv2.row, mv2.col);
      }
      std::printf("\n");
    }

    std::printf("========== 共 %d 项，失败 %d 项 ==========\n", testCount, failed);
    return failed == 0 ? 0 : 1;
}

#endif /* AI_HARD_TEST */
