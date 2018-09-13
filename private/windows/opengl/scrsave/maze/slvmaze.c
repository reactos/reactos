#include "pch.c"
#pragma hdrstop

static int left_turn[SOL_DIRS] =
{
    SOL_DIR_DOWN, SOL_DIR_LEFT, SOL_DIR_UP, SOL_DIR_RIGHT
};

static int right_turn[SOL_DIRS] =
{
    SOL_DIR_UP, SOL_DIR_RIGHT, SOL_DIR_DOWN, SOL_DIR_LEFT
};

static BYTE dir_wall[SOL_DIRS] =
{
    MAZE_WALL_LEFT, MAZE_WALL_UP, MAZE_WALL_RIGHT, MAZE_WALL_DOWN
};

static int dir_cloff[SOL_DIRS][2] =
{
    -1,0, 0,-1, 1,0, 0,1
};

static FaAngle dir_ang[SOL_DIRS];
static FxPt2 dir_off[SOL_DIRS];

/* We want to traverse one quarter of a circle in the given number of
   steps.  The distance is the arc length which is r*pi/2.  Divide that
   by the number of steps to get the distance each step should travel */
#define ARC_STEP 5
#define ARC_STEPS (90/ARC_STEP)

#define REVERSE_STEP (2*ARC_STEP)
#define REVERSE_STEPS (180/REVERSE_STEP)

static void SetView(MazeSolution *sol, MazeView *vw)
{
    vw->ang = dir_ang[sol->dir];
    vw->pos.x = CellToMfx(sol->clx)+dir_off[sol->dir].x;
    vw->pos.y = CellToMfx(sol->cly)+dir_off[sol->dir].y;
}

void SolveMazeSetGoals(MazeSolution *sol, MazeGoal *goals, int ngoals)
{
    sol->goals = goals;
    sol->ngoals = ngoals;
}

void SolveMazeStart(MazeView *vw,
                    Cell *maze, int w, int h,
                    IntPt2 *start, int start_dir,
                    MazeGoal *goals, int ngoals,
                    int turn_to,
                    MazeSolution *sol)
{
    dir_ang[SOL_DIR_LEFT] = FaDeg(180);
    dir_ang[SOL_DIR_UP] = FaDeg(90);
    dir_ang[SOL_DIR_RIGHT] = FaDeg(0);
    dir_ang[SOL_DIR_DOWN] = FaDeg(270);

    dir_off[SOL_DIR_LEFT].x = CellToMfx(1)-FX_MIN_VALUE;
    dir_off[SOL_DIR_LEFT].y = CellToMfx(1)/2;
    dir_off[SOL_DIR_UP].x = CellToMfx(1)/2;
    dir_off[SOL_DIR_UP].y = CellToMfx(1)-FX_MIN_VALUE;
    dir_off[SOL_DIR_RIGHT].x = FxVal(0);
    dir_off[SOL_DIR_RIGHT].y = CellToMfx(1)/2;
    dir_off[SOL_DIR_DOWN].x = CellToMfx(1)/2;
    dir_off[SOL_DIR_DOWN].y = FxVal(0);
    
    sol->clx = start->x;
    sol->cly = start->y;
    sol->dir = start_dir;
    sol->maze = maze;
    sol->w = w;
    sol->h = h;
    sol->ani_state = ANI_STATE_NONE;

    switch(turn_to)
    {
    case SOL_TURN_RIGHT:
        sol->turn_to = left_turn;
        sol->turn_away = right_turn;
        sol->dir_sign = 1;
        break;

    case SOL_TURN_LEFT:
        sol->turn_to = right_turn;
        sol->turn_away = left_turn;
        sol->dir_sign = -1;
        break;
    }

    SolveMazeSetGoals(sol, goals, ngoals);
    
    SetView(sol, vw);
}

#define MazeAt(x, y) (sol->maze+(x)+(y)*(sol->w))

MazeGoal *SolveMazeStep(MazeView *vw, MazeSolution *sol)
{
    Cell *cell;
    int i, dir, turn_to;

    if (sol->ani_state != ANI_STATE_NONE)
    {
        if (--sol->ani_count == 0)
        {
            sol->ani_state = ANI_STATE_NONE;
            SetView(sol, vw);
        }
    }
    
    switch(sol->ani_state)
    {
    case ANI_STATE_TURN_TO:
        vw->pos.x += FxMulDiv(FaCos(vw->ang),
                              FxFltVal(PI*MAZE_CELL_SIZE/2),
                              FxVal(ARC_STEPS*2));
        vw->pos.y += FxMulDiv(FaSin(vw->ang),
                              FxFltVal(PI*MAZE_CELL_SIZE/2),
                              FxVal(ARC_STEPS*2));
        vw->ang = FaAdd(vw->ang, sol->dir_sign*FaDeg(ARC_STEP));
        return NULL;

    case ANI_STATE_TURN_AWAY:
        vw->pos.x += FxMulDiv(FaCos(vw->ang),
                              FxFltVal(PI*MAZE_CELL_SIZE/2),
                              FxVal(ARC_STEPS*2));
        vw->pos.y += FxMulDiv(FaSin(vw->ang),
                              FxFltVal(PI*MAZE_CELL_SIZE/2),
                              FxVal(ARC_STEPS*2));
        vw->ang = FaAdd(vw->ang, sol->dir_sign * -FaDeg(ARC_STEP));
        return NULL;

    case ANI_STATE_FORWARD:
        vw->pos.x += FxMulDiv(FaCos(vw->ang), MAZE_CELL_SIZE, ARC_STEPS);
        vw->pos.y += FxMulDiv(FaSin(vw->ang), MAZE_CELL_SIZE, ARC_STEPS);
        return NULL;
        
    case ANI_STATE_REVERSE:
        vw->ang = FaAdd(vw->ang, sol->dir_sign*FaDeg(REVERSE_STEP));
        return NULL;
    }

    for (i = 0; i < sol->ngoals; i++)
    {
        if (sol->clx == sol->goals[i].clx &&
            sol->cly == sol->goals[i].cly)
        {
            return &sol->goals[i];
        }
    }

    cell = MazeAt(sol->clx, sol->cly);

    dir = sol->dir;
    for (i = 0; i < SOL_DIRS-1; i++)
    {
        turn_to = sol->turn_to[dir];
        if ((dir_wall[turn_to] & cell->can_see) == 0)
        {
            /* No wall present when turned, so turn that way */
            sol->clx += dir_cloff[turn_to][0];
            sol->cly += dir_cloff[turn_to][1];
            sol->dir = turn_to;

            sol->ani_count = ARC_STEPS;
            switch(i)
            {
            case 0:
                sol->ani_state = ANI_STATE_TURN_TO;
                break;

            case 1:
                sol->ani_state = ANI_STATE_FORWARD;
                break;

            case 2:
                sol->ani_state = ANI_STATE_TURN_AWAY;
                break;
            }
            break;
        }
        else
        {
            /* Wall present, turn away and check again */
            dir = sol->turn_away[dir];
        }
    }

    if (i == SOL_DIRS-1)
    {
        /* Dead end.  Turn around */
        dir = sol->turn_to[sol->turn_to[sol->dir]];
        sol->clx += dir_cloff[dir][0];
        sol->cly += dir_cloff[dir][1];
        sol->dir = dir;

        sol->ani_state = ANI_STATE_REVERSE;
        sol->ani_count = REVERSE_STEPS;
    }

    return NULL;
}
