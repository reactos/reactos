#include "pch.c"
#pragma hdrstop
#include "maze_std.h"

#define VIEW_ANG 90

float maze_height;
double view_rot;
int maze_walls_list;

extern TEX_ENV gTexEnv[];

typedef struct _FxRay2
{
    FxPt2 p;
    FxVec2 d;
} FxRay2;

BYTE maze_desc[MAZE_ARRAY][MAZE_ARRAY];

Cell maze_cells[MAZE_GRID][MAZE_GRID];
#define CellAt(x, y) (&maze_cells[y][x])

typedef struct _Wall
{
    FxPt2 f, t;
    int col;
    TEX_ENV *pTexEnv;  // points to texture environment
} Wall;
    
FxPt2 fmaze_pts[N_MAZE_PTS];
Wall maze[N_MAZE_WALLS];
int nwalls;

typedef struct _WallHit
{
    Cell *cell;
    int cx, cy;
    WallFlags flag;
} WallHit;

void AddObject(Object *obj, Cell *cell)
{
    obj->next = cell->contents;
    cell->contents = obj;
    obj->cell = cell;
}

void PlaceObject(Object *obj, FxValue x, FxValue y)
{
    Cell *cell;
    int cx, cy;

    cx = MfxToCell(x);
    cy = MfxToCell(y);
    cell = CellAt(cx, cy);
    
    obj->p.x = x;
    obj->p.y = y;

    AddObject(obj, cell);
}

void RemoveObject(Object *obj)
{
    Object *o, *op;
        
    if (obj->cell != NULL)
    {
        op = NULL;
        for (o = obj->cell->contents; o != obj; o = o->next)
        {
            op = o;
        }

        if (op == NULL)
        {
            obj->cell->contents = obj->next;
        }
        else
        {
            op->next = obj->next;
        }

        obj->cell = NULL;
    }
}

void MoveObject(Object *obj, FxValue x, FxValue y)
{
    int cx, cy;
    Cell *cell;
    
    obj->p.x = x;
    obj->p.y = y;
    
    cx = MfxToCell(x);
    cy = MfxToCell(y);
    cell = CellAt(cx, cy);
    
    if (cell == obj->cell)
    {
        return;
    }
    
    RemoveObject(obj);
    AddObject(obj, cell);
}

Object start_obj, end_obj;

BOOL InitMaze(IntPt2 *start_cell, MazeGoal *goals, int *ngoals)
{
    int i, j, n;
    FxPt2 p;

    if (!GenerateMaze(MAZE_GRID, MAZE_GRID, &maze_desc[0][0]))
    {
        return FALSE;
    }
    
    p.y = FxVal(0);
    n = 0;
    for (i = 0; i < MAZE_ARRAY; i++)
    {
        p.x = FxVal(0);
        for (j = 0; j < MAZE_ARRAY; j++)
        {
            fmaze_pts[n].x = p.x;
            fmaze_pts[n++].y = p.y;
            p.x += FMAZE_CELL_SIZE;
        }
        p.y += FMAZE_CELL_SIZE;
    }

    nwalls = 0;
    for (i = 0; i < MAZE_ARRAY; i++)
    {
        for (j = 0; j < MAZE_ARRAY; j++)
        {
            if (i < MAZE_ARRAY-1 && j < MAZE_ARRAY-1)
            {
                maze_cells[i][j].can_see = 0;
                maze_cells[i][j].contents = NULL;
                memset(maze_cells[i][j].walls, 0, 4*sizeof(Wall *));
            }
                
            if (maze_desc[i][j] & MAZE_WALL_HORZ)
            {
                if (j == MAZE_ARRAY-1)
                {
                    printf("MAZE_WALL_HORZ at right edge\n");
                    return FALSE;
                }
                
                maze[nwalls].f = fmaze_pts[i*MAZE_ARRAY+j];
                maze[nwalls].t = fmaze_pts[i*MAZE_ARRAY+j+1];
                maze[nwalls].col = (i+j+1) & 1;
                maze[nwalls].pTexEnv = &gTexEnv[TEX_WALL];
                
                if (i > 0)
                {
                    maze_cells[i-1][j].can_see |= MAZE_WALL_DOWN;
                    maze_cells[i-1][j].walls[WIDX_DOWN] = &maze[nwalls];
                }
                if (i < MAZE_ARRAY-1)
                {
                    maze_cells[i][j].can_see |= MAZE_WALL_UP;
                    maze_cells[i][j].walls[WIDX_UP] = &maze[nwalls];
                }
                
                nwalls++;
            }
            
            if (maze_desc[i][j] & MAZE_WALL_VERT)
            {
                if (i == MAZE_ARRAY-1)
                {
                    printf("MAZE_WALL_VERT at bottom edge\n");
                    return FALSE;
                }

                maze[nwalls].f = fmaze_pts[i*MAZE_ARRAY+j];
                maze[nwalls].t = fmaze_pts[(i+1)*MAZE_ARRAY+j];
                maze[nwalls].col = (i+j) & 1;
                maze[nwalls].pTexEnv = &gTexEnv[TEX_WALL];
            
                if (j > 0)
                {
                    maze_cells[i][j-1].can_see |= MAZE_WALL_RIGHT;
                    maze_cells[i][j-1].walls[WIDX_RIGHT] = &maze[nwalls];
                }
                if (j < MAZE_ARRAY-1)
                {
                    maze_cells[i][j].can_see |= MAZE_WALL_LEFT;
                    maze_cells[i][j].walls[WIDX_LEFT] = &maze[nwalls];
                }
                
                nwalls++;
            }
        }
    }

    // Always place the start on the left and
    // the end on the right.  This guarantees that there'll be
    // some traversing of the maze for the solution
    // Since the maze generator guarantees that the entire maze is
    // fully connected, the solution can always be found
    
    start_cell->x = 0;
    start_cell->y = rand() % MAZE_GRID;

    *ngoals = 1;
    goals[0].clx = MAZE_GRID-1;
    goals[0].cly = rand() % MAZE_GRID;
    
    start_obj.w = FMAZE_CELL_SIZE/6;
    start_obj.h = FxFltVal(.166);
    start_obj.z = FxFltVal(.5);
    start_obj.col = 12;
    start_obj.draw_style = DRAW_POLYGON;
    start_obj.pTexEnv = &gTexEnv[ TEX_START ];
    start_obj.ang = FaDeg(0);
    PlaceObject(&start_obj,
                CellToMfx(start_cell->x)+FMAZE_CELL_SIZE/2,
                CellToMfx(start_cell->y)+FMAZE_CELL_SIZE/2);
    
    end_obj.w = FMAZE_CELL_SIZE/6;
    end_obj.h = FxFltVal(.166);
    end_obj.z = FxFltVal(.5);
    end_obj.col = 10;
    end_obj.draw_style = DRAW_POLYGON;
    end_obj.pTexEnv = &gTexEnv[ TEX_END ];
    end_obj.ang = FaDeg(0);
    PlaceObject(&end_obj,
                CellToMfx(goals[0].clx)+FMAZE_CELL_SIZE/2,
                CellToMfx(goals[0].cly)+FMAZE_CELL_SIZE/2);

    // Reset some of the walls' textures to the OpenGL cover
    // for some variety
    i = (rand() % 5)+1;
    while (i-- > 0)
    {
        j = rand() % nwalls;
        maze[j].pTexEnv = &gTexEnv[TEX_COVER];
    }

#if 0
    // Make some of the walls partially covered
    n = (rand() % 50)+1;
    while (n-- > 0)
    {
        Wall *wall;
        int dir;

        // The wall picked cannot be an edge wall because that
        // would allow walking out of the maze
        i = ((rand() >> 8) % (MAZE_GRID-2))+1;
        j = ((rand() >> 8) % (MAZE_GRID-2))+1;
        dir = (rand() >> 13) % 4;
        wall = maze_cells[i][j].walls[dir];
        if (wall != NULL)
        {
            wall->pTexEnv = &gTexEnv[TEX_END];
            maze_cells[i][j].can_see |= (MAZE_WALL_LEFT_PARTIAL << dir);
            switch(dir)
            {
            case WIDX_LEFT:
                if (j > 0)
                {
                    maze_cells[i][j-1].can_see |= MAZE_WALL_RIGHT_PARTIAL;
                }
                break;
            case WIDX_RIGHT:
                if (j < MAZE_GRID-1)
                {
                    maze_cells[i][j+1].can_see |= MAZE_WALL_LEFT_PARTIAL;
                }
                break;
            case WIDX_UP:
                if (i > 0)
                {
                    maze_cells[i-1][j].can_see |= MAZE_WALL_DOWN_PARTIAL;
                }
                break;
            case WIDX_DOWN:
                if (i < MAZE_GRID-1)
                {
                    maze_cells[i+1][j].can_see |= MAZE_WALL_UP_PARTIAL;
                }
                break;
            }
        }
    }
#endif
    
    return TRUE;
}

#define PO_WALL 0
#define PO_PARTIAL 1
#define PO_COUNT 2

typedef struct _PaintWall
{
    Wall *wall;
} PaintWall;

typedef struct _PaintPartial
{
    Object *obj;
} PaintPartial;

typedef struct _PaintObject
{
    int type;
    union
    {
        PaintWall wall;
        PaintPartial partial;
    } u;
    FxValue depth;
    struct _PaintObject *closer;
} PaintObject;

#define N_PAINT_OBJECTS (4*MAZE_CELLS)
PaintObject paint[N_PAINT_OBJECTS];
int npaint;

void WallCoords(int x, int y, WallFlags flag, FxPt2 *f, FxPt2 *t)
{
    t->x = f->x = CellToMfx(x);
    t->y = f->y = CellToMfx(y);
    if (flag & MAZE_WALL_LEFT)
    {
        t->y += FMAZE_CELL_SIZE;
    }
    else if (flag & MAZE_WALL_UP)
    {
        t->x += FMAZE_CELL_SIZE;
    }
    else if (flag & MAZE_WALL_RIGHT)
    {
        f->x += FMAZE_CELL_SIZE;
        t->x = f->x;
        t->y += FMAZE_CELL_SIZE;
    }
    else if (flag & MAZE_WALL_DOWN)
    {
        f->y += FMAZE_CELL_SIZE;
        t->y = f->y;
        t->x += FMAZE_CELL_SIZE;
    }
}

void AddPaintWall(Cell *cell, int widx)
{
    PaintWall *pw;
    
    if (npaint == N_PAINT_OBJECTS)
    {
        printf("Paint list full\n");
        return;
    }
    
    pw = &paint[npaint].u.wall;
    paint[npaint].type = PO_WALL;
    npaint++;

    pw->wall = cell->walls[widx];
}

void AddPaintWalls(Cell *cell, WallFlags wf)
{
    if (wf & MAZE_WALL_LEFT)
    {
        AddPaintWall(cell, WIDX_LEFT);
    }
    if (wf & MAZE_WALL_RIGHT)
    {
        AddPaintWall(cell, WIDX_RIGHT);
    }
    if (wf & MAZE_WALL_DOWN)
    {
        AddPaintWall(cell, WIDX_DOWN);
    }
    if (wf & MAZE_WALL_UP)
    {
        AddPaintWall(cell, WIDX_UP);
    }
}

void AddPaintPartial(Object *obj)
{
    PaintPartial *pp;
    
    if (npaint == N_PAINT_OBJECTS)
    {
        printf("Paint list full\n");
        return;
    }
    
    pp = &paint[npaint].u.partial;
    paint[npaint].type = PO_PARTIAL;
    npaint++;

    pp->obj = obj;
}

void AddCell(int x, int y, WallFlags wf)
{
    Cell *cell;
    Object *obj;

    wf |= MAZE_CONTENTS;
    cell = CellAt(x, y);
    if ((cell->unseen & wf) == 0)
    {
        return;
    }
    
    AddPaintWalls(cell, (WallFlags)(wf & cell->unseen));

    if (cell->unseen & MAZE_CONTENTS)
    {
        for (obj = cell->contents; obj; obj = obj->next)
        {
            AddPaintPartial(obj);
        }
    }
    
    cell->unseen &= ~wf;
}

void TraceCells(FxPt2 *ip, FxVec2 *dp, WallHit *hit)
{
    int cx, cy;
    int sgnx, sgny;
    FxVec2 dg, dst;
    FxPt2 fp, g;
    WallFlags xwf, ywf, iwf, xpf, ypf;
    FxValue sx, sy;

    cx = MfxToCell(ip->x);
    cy = MfxToCell(ip->y);

    fp = *ip;
    
#ifdef TRACEDEB
    printf("pt %ld,%ld dp %ld,%ld\n", fp.x, fp.y, dp.x, dp.y);
#endif
    
    if (dp->x < 0)
    {
        g.x = CellToMfx(cx)-FX_MIN_VALUE;
        dg.x = -FMAZE_CELL_SIZE;
        sgnx = -1;
        xwf = MAZE_WALL_LEFT;
        xpf = MAZE_WALL_LEFT_PARTIAL;
    }
    else
    {
        g.x = CellToMfx(cx+1);
        dg.x = FMAZE_CELL_SIZE;
        sgnx = 1;
        xwf = MAZE_WALL_RIGHT;
        xpf = MAZE_WALL_RIGHT_PARTIAL;
        if (dp->x == 0)
        {
            xwf |= MAZE_WALL_LEFT;
            xpf |= MAZE_WALL_LEFT_PARTIAL;
        }
    }
    if (dp->y < 0)
    {
        g.y = CellToMfx(cy)-FX_MIN_VALUE;
        dg.y = -FMAZE_CELL_SIZE;
        sgny = -1;
        ywf = MAZE_WALL_UP;
        ypf = MAZE_WALL_UP_PARTIAL;
    }
    else
    {
        g.y = CellToMfx(cy+1);
        dg.y = FMAZE_CELL_SIZE;
        sgny = 1;
        ywf = MAZE_WALL_DOWN;
        ypf = MAZE_WALL_DOWN_PARTIAL;
        if (dp->y == 0)
        {
            ywf |= MAZE_WALL_UP;
            ypf |= MAZE_WALL_UP_PARTIAL;
        }
    }

    for (;;)
    {
        AddCell(cx, cy, (WallFlags)(xwf | ywf));

        dst.x = (g.x-fp.x)*sgnx;
        dst.y = (g.y-fp.y)*sgny;
        sx = FxMul(dst.x, dp->y);
        if (sx < 0)
        {
            sx = -sx;
        }
        sy = FxMul(dst.y, dp->x);
        if (sy < 0)
        {
            sy = -sy;
        }
        
#ifdef TRACEDEB
        printf("dx %ld, sx %ld, dy %ld, sy %ld\n", dst.x, sx, dst.y, sy);
#endif
        
        if (sx <= sy)
        {
            if ((maze_cells[cy][cx].can_see & xwf) &&
                (maze_cells[cy][cx].can_see & xpf) == 0)
            {
                iwf = xwf;
                break;
            }
            
            fp.x = g.x;
            fp.y += FxDiv(sx, dp->x)*sgnx*sgny;
            if (fp.y == g.y)
            {
                if ((maze_cells[cy][cx].can_see & ywf) &&
                    (maze_cells[cy][cx].can_see & ypf) == 0)
                {
                    iwf = ywf;
                    break;
                }
                cy += sgny;
                g.y += dg.y;
            }
            cx += sgnx;
            g.x += dg.x;
        }
        else
        {
            if ((maze_cells[cy][cx].can_see & ywf) &&
                (maze_cells[cy][cx].can_see & ypf) == 0)
            {
                iwf = ywf;
                break;
            }

            fp.y = g.y;
            fp.x += FxDiv(sy, dp->y)*sgnx*sgny;
            if (fp.x == g.x)
            {
                if ((maze_cells[cy][cx].can_see & xwf) &&
                    (maze_cells[cy][cx].can_see & xpf) == 0)
                {
                    iwf = xwf;
                    break;
                }
                cx += sgnx;
                g.x += dg.x;
            }
            cy += sgny;
            g.y += dg.y;
        }
    }
    hit->cell = CellAt(cx, cy);
    hit->cx = cx;
    hit->cy = cy;
    hit->flag = iwf;
}

void TraceView(MazeView *vw)
{
    FaAngle acc;
    FxVec2 vcc;
    WallHit hit;
    int rc;

    acc = FaAdd(vw->ang, FaDeg(VIEW_ANG)/2);
    
    for (rc = 0; rc < VIEW_ANG; rc++)
    {
        vcc.x = FaCos(acc);
        vcc.y = FaSin(acc);
        
        TraceCells(&vw->pos, &vcc, &hit);

        acc = FaAdd(acc, -FaDeg(1));
    }
}

static void WallCompute(PaintObject *po, MazeView *vw,
                        FxValue cs, FxValue sn)
{
    FxPt2 mid;
    Wall *wall;
    
    wall = po->u.wall.wall;

    // Compute depth at midpoint of wall
    // Eye coordinate depth increases along the X so
    // we only need to transform it
    
    mid.x = (wall->f.x+wall->t.x)/2-vw->pos.x;
    mid.y = (wall->f.y+wall->t.y)/2-vw->pos.y;
    
    po->depth = FxMul(mid.x, cs)+FxMul(mid.y, sn);
}

static void PartialCompute(PaintObject *po, MazeView *vw,
                           FxValue cs, FxValue sn)
{
    PaintPartial *pp;
    FxPt2 c;

    pp = &po->u.partial;

    // Compute depth at center of partial
    
    c.x = pp->obj->p.x-vw->pos.x;
    c.y = pp->obj->p.y-vw->pos.y;
    
    po->depth = FxMul(c.x, cs)+FxMul(c.y, sn);
}

typedef void (*PoComputeFn)(PaintObject *po, MazeView *vw,
                            FxValue cs, FxValue sn);
static PoComputeFn PoCompute[PO_COUNT] =
{
    WallCompute,
    PartialCompute
};

static float colors[17][3] =
{
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    0.0f, 0.5f, 0.0f,
    0.0f, 0.5f, 0.5f,
    0.5f, 0.0f, 0.0f,
    0.5f, 0.0f, 0.5f,
    0.5f, 0.5f, 0.0f,
    0.5f, 0.5f, 0.5f,
    0.75f, 0.75f, 0.75f,
    0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f,
    0.75f, 0.39f, 0.0f
};

#define WALL_SET 0
#define FLOOR_SET 1
#define CEILING_SET 2

static float *smooth_sets[3][2][4] =
{
    &colors[1][0], &colors[2][0], &colors[4][0], &colors[7][0],
    &colors[2][0], &colors[1][0], &colors[7][0], &colors[4][0],
    &colors[10][0], &colors[2][0], &colors[4][0], &colors[6][0],
    &colors[10][0], &colors[2][0], &colors[4][0], &colors[6][0],
    &colors[9][0], &colors[1][0], &colors[2][0], &colors[3][0],
    &colors[9][0], &colors[1][0], &colors[2][0], &colors[3][0]
};

static float *flat_sets[3][2][4] =
{
    &colors[8][0], &colors[8][0], &colors[8][0], &colors[8][0],
    &colors[15][0], &colors[15][0], &colors[15][0], &colors[15][0],
    &colors[2][0], &colors[2][0], &colors[2][0], &colors[2][0],
    &colors[2][0], &colors[2][0], &colors[2][0], &colors[2][0],
    &colors[9][0], &colors[9][0], &colors[9][0], &colors[9][0],
    &colors[9][0], &colors[9][0], &colors[9][0], &colors[9][0]
};

void SetAlphaCol(GLfloat *fv3)
{
    if (maze_options.all_alpha)
    {
        GLfloat fv4[4];

        fv4[0] = fv3[0];
        fv4[1] = fv3[1];
        fv4[2] = fv3[2];
        fv4[3] = 0.5f;
        glColor4fv(fv4);
    }
    else
    {
        glColor3fv(fv3);
    }
}

static void WallDraw(PaintObject *po, MazeView *vw)
{
    Wall *wall;
    float fx, fy, tx, ty, cx, cy, nx, ny;
    float **col_set;
    int reps;
    int rept;
    GLenum old_env;

    wall = po->u.wall.wall;
    reps = wall->pTexEnv->texRep.x;
    rept = wall->pTexEnv->texRep.y;
    
    fx = (float)FxFlt(wall->f.x);
    fy = (float)FxFlt(wall->f.y);
    tx = (float)FxFlt(wall->t.x);
    ty = (float)FxFlt(wall->t.y);
    nx = -(ty-fy);
    ny = (tx-fx);
    cx = (float)FxFlt(vw->pos.x);
    cy = (float)FxFlt(vw->pos.y);

    col_set = &flat_sets[WALL_SET][wall->col][0];
    switch(maze_options.render[WALLS])
    {
    case RENDER_NONE:
        return;
    case RENDER_SMOOTH:
        col_set = &smooth_sets[WALL_SET][wall->col][0];
        break;
    case RENDER_FLAT:
    case RENDER_TEXTURED:
        break;
    }

    // Compute dot product with wall normal to determine
    // wall direction.  We need to know the wall direction
    // in order to ensure that the wall texture faces the
    // correct direction
    UseTextureEnv(wall->pTexEnv);

    if (wall->pTexEnv->bTransp)
    {
        if (!maze_options.all_alpha)
        {
            glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &old_env);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, gTexEnvMode);
            glEnable(GL_BLEND);
        }
    }

    glBegin(GL_POLYGON);
    if ((fx-cx)*nx+(fy-cy)*ny > 0)
    {
        glTexCoord2d(0, 0);
        SetAlphaCol(col_set[0]);
        glVertex3f(fx, fy, 0.0f);
        glTexCoord2d(reps, 0);
        SetAlphaCol(col_set[1]);
        glVertex3f(tx, ty, 0.0f);
        glTexCoord2d(reps, rept);
        SetAlphaCol(col_set[2]);
        glVertex3f(tx, ty, maze_height);
        glTexCoord2d(0, rept);
        SetAlphaCol(col_set[3]);
        glVertex3f(fx, fy, maze_height);
    }
    else
    {
        glTexCoord2d(reps, 0);
        SetAlphaCol(col_set[0]);
        glVertex3f(fx, fy, 0.0f);
        glTexCoord2d(0, 0);
        SetAlphaCol(col_set[1]);
        glVertex3f(tx, ty, 0.0f);
        glTexCoord2d(0, rept);
        SetAlphaCol(col_set[2]);
        glVertex3f(tx, ty, maze_height);
        glTexCoord2d(reps, rept);
        SetAlphaCol(col_set[3]);
        glVertex3f(fx, fy, maze_height);
    }
    glEnd();

    if (wall->pTexEnv->bTransp)
    {
        if (!maze_options.all_alpha)
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, old_env);
            glDisable(GL_BLEND);
        }
    }
}

void (APIENTRY *convex_solids[SPECIAL_ARG_COUNT])(GLdouble radius) =
{
    auxSolidIcosahedron,
    auxSolidOctahedron,
    auxSolidDodecahedron,
    auxSolidTetrahedron
};

static void PartialDraw(PaintObject *po, MazeView *vw)
{
    PaintPartial *pp;
    float w, h, cx, cy, cz, vx, vy, fx, fy, fz, tx, ty, tz;
    float cs, sn;
    GLenum old_env;

    pp = &po->u.partial;
    
    w = (float)FxFlt(pp->obj->w);
    h = (float)FxFlt(pp->obj->h);

    // Partials are billboarded so we want it to always be
    // perpendicular to the view direction

    cs = (float)FxFlt(FaCos(vw->ang));
    sn = (float)FxFlt(FaSin(vw->ang));
    vx = -sn*w;
    vy = cs*w;
    
    cx = (float)FxFlt(pp->obj->p.x);
    cy = (float)FxFlt(pp->obj->p.y);
    cz = (float)FxFlt(pp->obj->z);

    fx = cx-vx;
    fy = cy-vy;
    fz = (cz-h)*maze_height;
    tx = cx+vx;
    ty = cy+vy;
    tz = (cz+h)*maze_height;

    if (maze_options.render[WALLS] == RENDER_TEXTURED)
    {
        glDisable(GL_TEXTURE_2D);
    }

    switch(pp->obj->draw_style)
    {
    case DRAW_POLYGON:
        glEnable(GL_TEXTURE_2D);
        if (!maze_options.all_alpha)
        {
            glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &old_env);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, gTexEnvMode);
            glEnable(GL_BLEND);
        }
        UseTextureEnv( pp->obj->pTexEnv );
        SetAlphaCol(colors[15]);
        glBegin(GL_POLYGON);
        glNormal3f(cs, sn, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(fx, fy, fz);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(tx, ty, fz);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(tx, ty, tz);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(fx, fy, tz);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        if (!maze_options.all_alpha)
        {
            glDisable(GL_BLEND);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, old_env);
        }
        break;

    case DRAW_SPECIAL:
        SetAlphaCol(colors[pp->obj->col]);
    
        glEnable(GL_AUTO_NORMAL);
        glEnable(GL_NORMALIZE);
        glEnable(GL_LIGHTING);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DITHER);
        glPushMatrix();
        
        glTranslated(cx, cy, cz*maze_height);
        glScaled(1.0, 1.0, maze_height);
        glRotated(FaFltDegVal(pp->obj->ang), 0, 0, 1);
        glRotated(pp->obj->user3, 0, 1, 0);
        // Must use convex objects since depth testing can be off
        convex_solids[pp->obj->draw_arg](w);
        
        glPopMatrix();
        if( !maze_options.bDither )
            glDisable(GL_DITHER);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_AUTO_NORMAL);
        glDisable(GL_NORMALIZE);
        break;
    }
    
    if (maze_options.render[WALLS] == RENDER_TEXTURED)
    {
        glEnable(GL_TEXTURE_2D);
    }
}

typedef void (*PoDrawFn)(PaintObject *po, MazeView *vw);
static PoDrawFn PoDraw[PO_COUNT] =
{
    WallDraw,
    PartialDraw
};


void RenderZPlane(int render, TEX_ENV *pTexEnv, int set, float zval)
{
    float **col_set;
    int reps = pTexEnv->texRep.x; 
    int rept = pTexEnv->texRep.y; 
    
    switch(render)
    {
    case RENDER_NONE:
        break;
    case RENDER_TEXTURED:
        UseTextureEnv(pTexEnv);
        glEnable(GL_TEXTURE_2D);
        // Fall through
    case RENDER_FLAT:
    case RENDER_SMOOTH:
        col_set = &flat_sets[set][0][0];
        if (render == RENDER_SMOOTH)
        {
            col_set = &smooth_sets[set][0][0];
        }
        
        glBegin(GL_POLYGON);

        // Switch texture orientation dependent on surface type
        if( set == CEILING_SET ) {
            glTexCoord2f((float)reps*MAZE_SIZE, 0.0f);
            glColor3fv(col_set[0]);
            glVertex3f(0.0f, 0.0f, zval);
            glTexCoord2f(0.0f, 0.0f);
            glColor3fv(col_set[1]);
            glVertex3f((float)MAZE_SIZE, 0.0f, zval);
            glTexCoord2f(0.0f, (float)rept*MAZE_SIZE);
            glColor3fv(col_set[2]);
            glVertex3f((float)MAZE_SIZE, (float)MAZE_SIZE, zval);
            glTexCoord2f((float)reps*MAZE_SIZE, (float)rept*MAZE_SIZE);
            glColor3fv(col_set[3]);
            glVertex3f(0.0f, (float)MAZE_SIZE, zval);
        } else {
            glTexCoord2f(0.0f, 0.0f);
            glColor3fv(col_set[0]);
            glVertex3f(0.0f, 0.0f, zval);
            glTexCoord2f((float)reps*MAZE_SIZE, 0.0f);
            glColor3fv(col_set[1]);
            glVertex3f((float)MAZE_SIZE, 0.0f, zval);
            glTexCoord2f((float)reps*MAZE_SIZE, (float)rept*MAZE_SIZE);
            glColor3fv(col_set[2]);
            glVertex3f((float)MAZE_SIZE, (float)MAZE_SIZE, zval);
            glTexCoord2f(0.0f, (float)rept*MAZE_SIZE);
            glColor3fv(col_set[3]);
            glVertex3f(0.0f, (float)MAZE_SIZE, zval);
        }

        glEnd();

        if (render == RENDER_TEXTURED)
        {
            glDisable(GL_TEXTURE_2D);
        }
        break;
    }
}

void Render(MazeView *vw)
{
    FxValue cs, sn;
    PaintObject *sorted, *so, *pso;
    PaintObject *po;
    int i;
    FxPt2 at;
    BOOL special;
    float viewHeight;

    cs = FaCos(vw->ang);
    sn = FaSin(vw->ang);
    
    at.x = vw->pos.x+cs;
    at.y = vw->pos.y+sn;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glRotated(view_rot, 0, 0, 1);
    gluPerspective(VIEW_ANG, 1, .01, 100);
    viewHeight = 0.5f;
    gluLookAt(FxFlt(vw->pos.x), FxFlt(vw->pos.y), viewHeight,
              FxFlt(at.x), FxFlt(at.y), viewHeight,
              0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    RenderZPlane(maze_options.render[FLOOR], &gTexEnv[TEX_FLOOR], FLOOR_SET, 0.0f);
    RenderZPlane(maze_options.render[CEILING], &gTexEnv[TEX_CEILING], CEILING_SET, 1.0f);
        
    sorted = NULL;
    special = FALSE;
    for (i = 0, po = paint; i < npaint; i++, po++)
    {
        if (po->type == PO_PARTIAL &&
            po->u.partial.obj->draw_style == DRAW_SPECIAL)
        {
            special = TRUE;
        }
        
        PoCompute[po->type](po, vw, cs, sn);
        
        for (so = sorted, pso = NULL; so; pso = so, so = so->closer)
        {
            if (so->depth <= po->depth)
            {
                break;
            }
        }
        if (pso == NULL)
        {
            sorted = po;
        }
        else
        {
            pso->closer = po;
        }
        po->closer = so;
    }

#if 0
    // Unnecessary at the moment, but might be handy later
    if (special && !maze_options.depth_test)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
#endif
    
    if (maze_options.render[WALLS] == RENDER_TEXTURED)
    {
        glEnable(GL_TEXTURE_2D);
    }
    
    for (so = sorted; so; so = so->closer)
    {
        PoDraw[so->type](so, vw);
    }

    if (maze_options.render[WALLS] == RENDER_TEXTURED)
    {
        glDisable(GL_TEXTURE_2D);
    }
}

void InitPaint(void)
{
    int i, j;

    npaint = 0;
    for (i = 0; i < MAZE_GRID; i++)
    {
        for (j = 0; j < MAZE_GRID; j++)
        {
            maze_cells[i][j].unseen = maze_cells[i][j].can_see | MAZE_CONTENTS;
        }
    }
}

void DrawMaze(MazeView *vw)
{
    InitPaint();
    TraceView(vw);
    Render(vw);
}

void DrawMazeWalls(void)
{
    int w;
    Wall *wall;
    
    wall = maze;
    
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_LINES);
    for (w = 0; w < nwalls; w++)
    {
        glVertex2f((float)FxFltVal(wall->f.x), (float)FxFltVal(wall->f.y));
        glVertex2f((float)FxFltVal(wall->t.x), (float)FxFltVal(wall->t.y));
        wall++;
    }
    glEnd();
}

#define SQRT2_2 0.707107f

void DrawTopView(MazeView *vw)
{
    int c;
    Cell *cell;
    Object *obj;
    float vx, vy, cx, cy, width, ang;
    extern float gfAspect;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
//mf: if image being stretched, gfAspect isn't enought to make this straight -
// need to compensate by using aspect of base dimensions as well.
//mf? maybe use glScale ?
    gluOrtho2D( -MAZE_SIZE/2.0, MAZE_SIZE/2.0,
            -MAZE_SIZE/2.0/gfAspect, MAZE_SIZE/2.0/gfAspect );
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();

    ang = (float)FaFltDegVal(vw->ang)+90.0f;
    glRotatef(ang, 0.0f, 0.0f, 1.0f);
    vx = (float)FxFltVal(vw->pos.x);
    vy = (float)FxFltVal(vw->pos.y);
    glTranslatef(-vx, -vy, 0.0f);
    
#define AA_LINES 1
#ifdef AA_LINES
    // Turn on antialiased lines
    glEnable( GL_BLEND );
    glEnable( GL_LINE_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
#endif

    glCallList(maze_walls_list);

#ifdef AA_LINES
    glDisable( GL_BLEND );
    glDisable( GL_LINE_SMOOTH );
#endif

    // Objects aren't put in the walls display list so that they
    // can move around
    
    cell = &maze_cells[0][0];
    for (c = 0; c < MAZE_CELLS; c++)
    {
        for (obj = cell->contents; obj != NULL; obj = obj->next)
        {
            cx = (float)FxFltVal(obj->p.x);
            cy = (float)FxFltVal(obj->p.y);
            width = (float)FxFltVal(obj->w);

            glColor3fv(colors[obj->col]);

            glPushMatrix();
            glTranslatef(cx, cy, 0.0f);
            glRotated(FaFltDegVal(obj->ang), 0, 0, 1);
#if 1
            glBegin(GL_POLYGON);
            glVertex2f(width, 0.0f);
            glVertex2f(-width*SQRT2_2, width*0.5f);
            glVertex2f(-width*SQRT2_2, -width*0.5f);
            glEnd();
#else
            glRectf(-width, -width, width, width);
#endif
            glPopMatrix();
        }

        cell++;
    }
    
    glPopMatrix();
    
    // Draw self
    glColor3f(0.0f, 0.0f, 1.0f);
    width = MAZE_CELL_SIZE/4.0f;
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, width);
    glVertex2f(width*0.5f, -width*SQRT2_2);
    glVertex2f(-width*0.5f, -width*SQRT2_2);
    glEnd();
}
