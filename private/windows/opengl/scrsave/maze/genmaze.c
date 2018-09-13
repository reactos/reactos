#include "pch.c"
#pragma hdrstop

/*
  Maze generation based on Kruskal's algorithm
  
  Consider all possible paths as sets of connected cells.  Initially
  all cells are unconnected and all walls are present.  Pick a wall
  at random and determine if the cells on either side of the wall are
  connected by any possible path.  If they are, try again.  If not,
  knock out the wall and union the two path sets together.
  */
#include <stdlib.h>
#include <memory.h>

#include "genmaze.h"

typedef struct _MazeCell
{
    /* Pointer to the head of the set of cells reachable from this one.
       Easily identifies an entire set for comparison */
    struct _MazeCell *set;
    /* Pointer to the next cell in this connected set */
    struct _MazeCell *next;
} MazeCell;

typedef struct _MazeWall
{
    BYTE wall;
    int x, y;
} MazeWall;

/*
  Union two connection sets together by setting all elements in one
  to the set of the other and then appending it to the end of the
  set list
  */
static void ConnectSets(MazeCell *set, MazeCell *add)
{
    /* Locate end of set */
    while (set->next != NULL)
    {
        set = set->next;
    }

    /* Change to point to head of set */
    add = add->set;
    
    /* Append new cells */
    set->next = add;

    /* Change new cells' set identity */
    while (add != NULL)
    {
        add->set = set->set;
        add = add->next;
    }
}

/*
  Determine whether the two cells are already connected.  With the
  existing data structures this is a trivial comparison of set
  identities
  */
#define SetsAreConnected(a, b) \
    ((a)->set == (b)->set)

/* Locate a cell in a known array */
#define CellAt(x, y) (cells+(x)+(y)*width)

/* Locate a cell in the output array */
#define MazeAt(x, y) (maze+(x)+(y)*(width+1))

/*
  Generate a maze by deleting walls at random
  Width and height are the counts of cells

  Result is stored in the walls array, formatted as (width+1)*(height+1)
  cells with MAZE_WALL_HORZ | MAZE_WALL_VERT set appropriately.
  X coordinates are one byte apart, so maze is treated like
  maze[height+1][width+1]
  */
BOOL GenerateMaze(int width, int height, BYTE *maze)
{
    MazeCell *cells;
    MazeWall *walls;
    int ncells;
    int nwalls;
    int i, x, y;
    MazeCell *ca, *cb;
    MazeWall *w, wt;

    ncells = width*height;
    cells = (MazeCell *)malloc(sizeof(MazeCell)*ncells);
    if (cells == NULL)
    {
        return FALSE;
    }

    nwalls = (width-1)*height+(height-1)*width;
    walls = (MazeWall *)malloc(sizeof(MazeWall)*nwalls);
    if (walls == NULL)
    {
        free(cells);
        return FALSE;
    }

    /* Initialize all cells to be unique sets */
    ca = cells;
    for (i = 0; i < ncells; i++)
    {
        ca->set = ca;
        ca->next = NULL;
        ca++;
    }

    /* Add all internal horizontal and vertical walls.
       All edge walls will be present in the final maze so
       they aren't considered here */
    w = walls;
    for (x = 1; x < width; x++)
    {
        for (y = 0; y < height; y++)
        {
            w->wall = MAZE_WALL_VERT;
            w->x = x;
            w->y = y;
            w++;
        }
    }
    for (y = 1; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            w->wall = MAZE_WALL_HORZ;
            w->x = x;
            w->y = y;
            w++;
        }
    }

    /* Randomize the wall array */
    for (i = nwalls-1; i > 0; i--)
    {
        w = walls+(rand() % i);
        wt = *w;
        *w = walls[i];
        walls[i] = wt;
    }

    /* Now walk the random list of walls, knocking walls out to
       join cells together */
    w = walls;
    for (i = 0; i < nwalls; i++)
    {
        /* Determine the two cells separated by the current wall */
        ca = CellAt(w->x, w->y);
        if (w->wall == MAZE_WALL_HORZ)
        {
            cb = CellAt(w->x, w->y-1);
        }
        else
        {
            cb = CellAt(w->x-1, w->y);
        }

        /* If the two cells aren't connected, connect them by knocking
           out the wall */
        if (!SetsAreConnected(ca, cb))
        {
            ConnectSets(ca, cb);
            w->wall = 0;
        }
        
        w++;
    }

    /* Initialize output to empty */
    memset(maze, 0, sizeof(BYTE)*(width+1)*(height+1));
    
    /* Set all edge walls in the output */
    for (x = 0; x < width; x++)
    {
        *MazeAt(x, 0) |= MAZE_WALL_HORZ;
        *MazeAt(x, height) |= MAZE_WALL_HORZ;
    }
    for (y = 0; y < height; y++)
    {
        *MazeAt(0, y) |= MAZE_WALL_VERT;
        *MazeAt(width, y) |= MAZE_WALL_VERT;
    }

    /* Copy remaining walls into the output array */
    w = walls;
    for (i = 0; i < nwalls; i++)
    {
        *MazeAt(w->x, w->y) |= w->wall;
        w++;
    }

    free(cells);
    free(walls);
    
    return TRUE;
}
