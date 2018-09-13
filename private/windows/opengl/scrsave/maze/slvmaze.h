#ifndef __SLVMAZE_H__
#define __SLVMAZE_H__

#define ANI_STATE_NONE          0
#define ANI_STATE_TURN_TO       1
#define ANI_STATE_TURN_AWAY     2
#define ANI_STATE_FORWARD       3
#define ANI_STATE_REVERSE       4

#define SOL_DIR_LEFT    0
#define SOL_DIR_UP      1
#define SOL_DIR_RIGHT   2
#define SOL_DIR_DOWN    3
#define SOL_DIRS        4

#define SOL_TURN_LEFT   0
#define SOL_TURN_RIGHT  1

typedef struct _MazeGoal
{
    int clx, cly;
    void *user;
} MazeGoal;

typedef struct _MazeSolution
{
    int clx, cly;
    int dir;
    Cell *maze;
    int w, h;
    MazeGoal *goals;
    int ngoals;
    int ani_state;
    int ani_count;
    int *turn_to;
    int *turn_away;
    int dir_sign;
} MazeSolution;

void SolveMazeStart(MazeView *vw,
                    Cell *maze, int w, int h,
                    IntPt2 *start, int start_dir,
                    MazeGoal *goals, int ngoals,
                    int turn_to,
                    MazeSolution *sol);
void SolveMazeSetGoals(MazeSolution *sol, MazeGoal *goals, int ngoals);
MazeGoal *SolveMazeStep(MazeView *vw, MazeSolution *sol);

#endif
