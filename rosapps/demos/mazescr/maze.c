/******************************************************************************
 * [ maze ] ...
 *
 * modified:  [ 03-08-15 ] Ge van Geldorp <ge@gse.nl>
 *		ported to Reactos
 * modified:  [ 94-10-8 ] Ge van Geldorp <Ge.vanGeldorp@lr.tudelft.nl>
 *		ported to MS Windows
 * modified:  [ 3-7-93 ]  Jamie Zawinski <jwz@lucid.com>
 *		added the XRoger logo, cleaned up resources, made
 *		grid size a parameter.
 * modified:  [ 3-3-93 ]  Jim Randell <jmr@mddjmr.fc.hp.com>
 *		Added the colour stuff and integrated it with jwz's
 *		screenhack stuff.  There's still some work that could
 *		be done on this, particularly allowing a resource to
 *		specify how big the squares are.
 * modified:  [ 10-4-88 ]  Richard Hess    ...!uunet!cimshop!rhess
 *              [ Revised primary execution loop within main()...
 *              [ Extended X event handler, check_events()...
 * modified:  [ 1-29-88 ]  Dave Lemke      lemke@sun.com
 *              [ Hacked for X11...
 *              [  Note the word "hacked" -- this is extremely ugly, but at
 *              [   least it does the job.  NOT a good programming example
 *              [   for X.
 * original:  [ 6/21/85 ]  Martin Weiss    Sun Microsystems  [ SunView ]
 *
 ******************************************************************************
 Copyright 1988 by Sun Microsystems, Inc. Mountain View, CA.

 All Rights Reserved

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted,
 provided that the above copyright notice appear in all copies and that
 both that copyright notice and this permission notice appear in
 supporting documentation, and that the names of Sun or MIT not be
 used in advertising or publicity pertaining to distribution of the
 software without specific prior written permission. Sun and M.I.T.
 make no representations about the suitability of this software for
 any purpose. It is provided "as is" without any express or implied warranty.

 SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE. IN NO EVENT SHALL SUN BE LIABLE FOR ANY SPECIAL, INDIRECT
 OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 OR PERFORMANCE OF THIS SOFTWARE.
 *****************************************************************************/

#define STRICT

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>   	/* required for all Windows applications */

#if !defined (APIENTRY) /* Windows NT defines APIENTRY, but 3.x doesn't */
#define APIENTRY far pascal
#endif

#if !defined(WIN32)		/* Windows 3.x uses a FARPROC for dialogs */
#define DLGPROC FARPROC
#endif

static BOOL InitInstance(HINSTANCE hInstance, HWND hParent);
LRESULT CALLBACK MazeWndProc(HWND hWnd, UINT message, WPARAM uParam,
                         LPARAM lParam);

HINSTANCE hInst;          	/* current instance */
HWND      hWnd; 			/* Main window handle.*/
HBRUSH hBrushDead;
HBRUSH hBrushLiving;
HPEN   hPenWall;
HDC    hDC;
static BOOL waiting;


WCHAR szAppName[] = L"Maze";  /* The name of this application */
WCHAR szTitle[]   = L"Maze"; 	/* The title bar text */

static int solve_delay, pre_solve_delay, post_solve_delay;

#define MAX_MAZE_SIZE_X	((unsigned long) 250)
#define MAX_MAZE_SIZE_Y	((unsigned long) 250)

#define MOVE_LIST_SIZE  (MAX_MAZE_SIZE_X * MAX_MAZE_SIZE_Y)

#define WALL_TOP	0x8000
#define WALL_RIGHT	0x4000
#define WALL_BOTTOM	0x2000
#define WALL_LEFT	0x1000

#define DOOR_IN_TOP	0x800
#define DOOR_IN_RIGHT	0x400
#define DOOR_IN_BOTTOM	0x200
#define DOOR_IN_LEFT	0x100
#define DOOR_IN_ANY	0xF00

#define DOOR_OUT_TOP	0x80
#define DOOR_OUT_RIGHT	0x40
#define DOOR_OUT_BOTTOM	0x20
#define DOOR_OUT_LEFT	0x10

#define START_SQUARE	0x2
#define END_SQUARE	0x1

#define	border_x        (0)
#define	border_y        (0)

#define	get_random(x)	(rand() % (x))

static unsigned short maze[MAX_MAZE_SIZE_X][MAX_MAZE_SIZE_Y];

static struct {
  unsigned char x;
  unsigned char y;
  unsigned char dir;
  unsigned char dummy;
} move_list[MOVE_LIST_SIZE], save_path[MOVE_LIST_SIZE], path[MOVE_LIST_SIZE];

static int maze_size_x, maze_size_y;
static long sqnum, path_length;
static int cur_sq_x, cur_sq_y;
static int start_x, start_y, start_dir, end_x, end_y, end_dir;
static int grid_width, grid_height;

static int state = 1, pathi = 0;

static void
set_maze_sizes (width, height)
     int width, height;
{
  maze_size_x = width / grid_width;
  maze_size_y = height / grid_height;
}


static void
initialize_maze()         /* draw the surrounding wall and start/end squares */
{
  register int i, j, wall;

  /* initialize all squares */
  for ( i=0; i<maze_size_x; i++) {
    for ( j=0; j<maze_size_y; j++) {
      maze[i][j] = 0;
    }
  }

  /* top wall */
  for ( i=0; i<maze_size_x; i++ ) {
    maze[i][0] |= WALL_TOP;
  }

  /* right wall */
  for ( j=0; j<maze_size_y; j++ ) {
    maze[maze_size_x-1][j] |= WALL_RIGHT;
  }

  /* bottom wall */
  for ( i=0; i<maze_size_x; i++ ) {
    maze[i][maze_size_y-1] |= WALL_BOTTOM;
  }

  /* left wall */
  for ( j=0; j<maze_size_y; j++ ) {
    maze[0][j] |= WALL_LEFT;
  }

  /* set start square */
  wall = get_random(4);
  switch (wall) {
  case 0:
    i = get_random(maze_size_x);
    j = 0;
    break;
  case 1:
    i = maze_size_x - 1;
    j = get_random(maze_size_y);
    break;
  case 2:
    i = get_random(maze_size_x);
    j = maze_size_y - 1;
    break;
  case 3:
    i = 0;
    j = get_random(maze_size_y);
    break;
  }
  maze[i][j] |= START_SQUARE;
  maze[i][j] |= ( DOOR_IN_TOP >> wall );
  maze[i][j] &= ~( WALL_TOP >> wall );
  cur_sq_x = i;
  cur_sq_y = j;
  start_x = i;
  start_y = j;
  start_dir = wall;
  sqnum = 0;

  /* set end square */
  wall = (wall + 2)%4;
  switch (wall) {
  case 0:
    i = get_random(maze_size_x);
    j = 0;
    break;
  case 1:
    i = maze_size_x - 1;
    j = get_random(maze_size_y);
    break;
  case 2:
    i = get_random(maze_size_x);
    j = maze_size_y - 1;
    break;
  case 3:
    i = 0;
    j = get_random(maze_size_y);
    break;
  }
  maze[i][j] |= END_SQUARE;
  maze[i][j] |= ( DOOR_OUT_TOP >> wall );
  maze[i][j] &= ~( WALL_TOP >> wall );
  end_x = i;
  end_y = j;
  end_dir = wall;
}

static int choose_door ();
static long backup ();
static void draw_wall ();
static void draw_solid_square(int, int, int, HDC, HBRUSH);
static void enter_square(int, HDC, HBRUSH);

static void
create_maze()             /* create a maze layout given the intiialized maze */
{
  register int i, newdoor = 0;
  HDC hDC;

  hDC = GetDC(hWnd);
  do {
    move_list[sqnum].x = cur_sq_x;
    move_list[sqnum].y = cur_sq_y;
    move_list[sqnum].dir = newdoor;
    while ( ( newdoor = choose_door(hDC) ) == -1 ) { /* pick a door */
      if ( backup() == -1 ) { /* no more doors ... backup */
	    ReleaseDC(hWnd, hDC);
		return; /* done ... return */
      }
    }

    /* mark the out door */
    maze[cur_sq_x][cur_sq_y] |= ( DOOR_OUT_TOP >> newdoor );

    switch (newdoor) {
    case 0: cur_sq_y--;
      break;
    case 1: cur_sq_x++;
      break;
    case 2: cur_sq_y++;
      break;
    case 3: cur_sq_x--;
      break;
    }
    sqnum++;

    /* mark the in door */
    maze[cur_sq_x][cur_sq_y] |= ( DOOR_IN_TOP >> ((newdoor+2)%4) );

    /* if end square set path length and save path */
    if ( maze[cur_sq_x][cur_sq_y] & END_SQUARE ) {
      path_length = sqnum;
      for ( i=0; i<path_length; i++) {
	save_path[i].x = move_list[i].x;
	save_path[i].y = move_list[i].y;
	save_path[i].dir = move_list[i].dir;
      }
    }

  } while (1);

}


static int
choose_door(HDC hDC)                                    /* pick a new path */
{
  int candidates[3];
  register int num_candidates;

  num_candidates = 0;

  /* top wall */
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_TOP )
    goto rightwall;
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_TOP )
    goto rightwall;
  if ( maze[cur_sq_x][cur_sq_y] & WALL_TOP )
    goto rightwall;
  if ( maze[cur_sq_x][cur_sq_y - 1] & DOOR_IN_ANY ) {
    maze[cur_sq_x][cur_sq_y] |= WALL_TOP;
    maze[cur_sq_x][cur_sq_y - 1] |= WALL_BOTTOM;
    draw_wall(cur_sq_x, cur_sq_y, 0, hDC);
    goto rightwall;
  }
  candidates[num_candidates++] = 0;

 rightwall:
  /* right wall */
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_RIGHT )
    goto bottomwall;
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_RIGHT )
    goto bottomwall;
  if ( maze[cur_sq_x][cur_sq_y] & WALL_RIGHT )
    goto bottomwall;
  if ( maze[cur_sq_x + 1][cur_sq_y] & DOOR_IN_ANY ) {
    maze[cur_sq_x][cur_sq_y] |= WALL_RIGHT;
    maze[cur_sq_x + 1][cur_sq_y] |= WALL_LEFT;
    draw_wall(cur_sq_x, cur_sq_y, 1, hDC);
    goto bottomwall;
  }
  candidates[num_candidates++] = 1;

 bottomwall:
  /* bottom wall */
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_BOTTOM )
    goto leftwall;
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_BOTTOM )
    goto leftwall;
  if ( maze[cur_sq_x][cur_sq_y] & WALL_BOTTOM )
    goto leftwall;
  if ( maze[cur_sq_x][cur_sq_y + 1] & DOOR_IN_ANY ) {
    maze[cur_sq_x][cur_sq_y] |= WALL_BOTTOM;
    maze[cur_sq_x][cur_sq_y + 1] |= WALL_TOP;
    draw_wall(cur_sq_x, cur_sq_y, 2, hDC);
    goto leftwall;
  }
  candidates[num_candidates++] = 2;

 leftwall:
  /* left wall */
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_LEFT )
    goto donewall;
  if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_LEFT )
    goto donewall;
  if ( maze[cur_sq_x][cur_sq_y] & WALL_LEFT )
    goto donewall;
  if ( maze[cur_sq_x - 1][cur_sq_y] & DOOR_IN_ANY ) {
    maze[cur_sq_x][cur_sq_y] |= WALL_LEFT;
    maze[cur_sq_x - 1][cur_sq_y] |= WALL_RIGHT;
    draw_wall(cur_sq_x, cur_sq_y, 3, hDC);
    goto donewall;
  }
  candidates[num_candidates++] = 3;

 donewall:
  if (num_candidates == 0)
    return ( -1 );
  if (num_candidates == 1)
    return ( candidates[0] );
  return ( candidates[ get_random(num_candidates) ] );

}


static long
backup()                                                  /* back up a move */
{
  sqnum--;
  if (0 <= sqnum) {
  	cur_sq_x = move_list[sqnum].x;
  	cur_sq_y = move_list[sqnum].y;
  }
  return ( sqnum );
}

int bw;

static void
draw_solid_square(i, j, dir, hDC, hBrush)          /* draw a solid square in a square */
     register int i, j, dir;
     HDC hDC;
	 HBRUSH hBrush;
{
  RECT rc;

  switch (dir) {
  case 0:
    rc.left = border_x + bw + grid_width * i;
	rc.right = rc.left + grid_width - (bw + bw);
	rc.top = border_y - bw + grid_height * j;
	rc.bottom = rc.top + grid_height;
    break;
  case 1:
	rc.left = border_x + bw + grid_width * i;
	rc.right = rc.left + grid_width;
	rc.top = border_y + bw + grid_height * j;
	rc.bottom = rc.top + grid_height - (bw + bw);
    break;
  case 2:
    rc.left = border_x + bw + grid_width * i;
	rc.right = rc.left + grid_width - (bw + bw);
	rc.top = border_y + bw + grid_height * j;
	rc.bottom = rc.top + grid_height;
    break;
  case 3:
  	rc.left = border_x - bw + grid_width * i;
	rc.right = rc.left + grid_width;
	rc.top = border_y + bw + grid_height * j;
	rc.bottom = rc.top + grid_height - (bw + bw);
    break;
  }
  (void) FillRect(hDC, &rc, hBrush);
}

static void
draw_maze_border(HWND hWnd, HDC hDC)	/* draw the maze outline */
{
  register int i, j;
  HBRUSH hBrush;

  SelectObject(hDC, hPenWall);

  for ( i=0; i<maze_size_x; i++) {
    if ( maze[i][0] & WALL_TOP ) {
	  MoveToEx(hDC, border_x + grid_width * i, border_y, NULL);
	  (void) LineTo(hDC, border_x + grid_width * (i + 1) - 1, border_y);
    }
    if ((maze[i][maze_size_y - 1] & WALL_BOTTOM)) {
	  MoveToEx(hDC, border_x + grid_width * i,
	           border_y + grid_height * (maze_size_y) - 1, NULL);
	  (void) LineTo(hDC, border_x + grid_width * (i+1) - 1,
		            border_y + grid_height * (maze_size_y) - 1);
    }
  }
  for ( j=0; j<maze_size_y; j++) {
    if ( maze[maze_size_x - 1][j] & WALL_RIGHT ) {
	  MoveToEx(hDC, border_x + grid_width * maze_size_x - 1,
		       border_y + grid_height * j, NULL);
	  (void) LineTo(hDC, border_x + grid_width * maze_size_x - 1,
		            border_y + grid_height * (j+1) - 1);
    }
    if ( maze[0][j] & WALL_LEFT ) {
	  MoveToEx(hDC, border_x, border_y + grid_height * j, NULL);
	  (void) LineTo(hDC, border_x, border_y + grid_height * (j+1) - 1);
    }
  }

  hBrush = GetStockObject(WHITE_BRUSH);  // FIXME: do not hardcode
  draw_solid_square (start_x, start_y, start_dir, hDC, hBrush);
  draw_solid_square (end_x, end_y, end_dir, hDC, hBrush);
}


static void
draw_wall(i, j, dir, hDC)                                   /* draw a single wall */
     int i, j, dir;
	 HDC hDC;
{
  SelectObject(hDC, hPenWall);

  switch (dir) {
  case 0:
  	MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * j, NULL);
	(void) LineTo(hDC, border_x + grid_width * (i+1),
	              border_y + grid_height * j);
    break;
  case 1:
	MoveToEx(hDC, border_x + grid_width * (i+1), border_y + grid_height * j,
	         NULL);
	(void) LineTo(hDC, border_x + grid_width * (i+1),
	              border_y + grid_height * (j+1));
    break;
  case 2:
	MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * (j+1),
	         NULL);
	(void) LineTo(hDC, border_x + grid_width * (i+1),
	              border_y + grid_height * (j+1));
    break;
  case 3:
	MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * j,
	         NULL);
	(void) LineTo(hDC, border_x + grid_width * i,
	              border_y + grid_height * (j+1));
    break;
  }
}

static void
begin_solve_maze()                             /* solve it with graphical feedback */
{
  static long grayPattern[] = {
	0x55555555,
	0xaaaaaaaa,
	0x55555555,
	0xaaaaaaaa,
	0x55555555,
	0xaaaaaaaa,
	0x55555555,
	0xaaaaaaaa
  };
  static RGBQUAD argbq[] = {
  	{ 0, 0, 255, 0 },
	{ 255, 255, 255, 0 }
  };
  BITMAPINFO *pbmi;

  hDC = GetDC(hWnd);
  pbmi = malloc(sizeof(BITMAPINFOHEADER) + sizeof(argbq) + sizeof(grayPattern));
  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth = 8;
  pbmi->bmiHeader.biHeight = 8;
  pbmi->bmiHeader.biPlanes = 1;
  pbmi->bmiHeader.biBitCount = 1;
  pbmi->bmiHeader.biCompression = BI_RGB;
  (void) memcpy(pbmi->bmiColors, argbq, sizeof(argbq));
  (void) memcpy(pbmi->bmiColors + 2, grayPattern, sizeof(grayPattern));
#if 0
  /* FIXME Pattern brushes not yet implemented in ReactOS */
  hBrushDead = CreateDIBPatternBrushPt(pbmi, DIB_RGB_COLORS);
#else
  hBrushDead = CreateSolidBrush(RGB(255, 0, 0));
#endif
//  hBrushDead = CreateHatchBrush(HS_DIAGCROSS, RGB(255, 0, 0));
  free(pbmi);
  hBrushLiving = CreateSolidBrush(RGB(0, 255, 0));

  /* plug up the surrounding wall */
  maze[start_x][start_y] |= (WALL_TOP >> start_dir);
  maze[end_x][end_y] |= (WALL_TOP >> end_dir);

  /* initialize search path */
  pathi = 0;
  path[pathi].x = end_x;
  path[pathi].y = end_y;
  path[pathi].dir = -1;
}

static int
solve_maze()                             /* solve it with graphical feedback */
{
  int ret;
  int action_done;

  do {
    action_done = 1;
    if ( ++path[pathi].dir >= 4 ) {
      pathi--;
      draw_solid_square( (int)(path[pathi].x), (int)(path[pathi].y),
	  	       (int)(path[pathi].dir), hDC, hBrushDead);
      ret = 0;
    }
    else if ( ! (maze[path[pathi].x][path[pathi].y] &
	  	(WALL_TOP >> path[pathi].dir))  &&
	     ( (pathi == 0) || ( (path[pathi].dir !=
		  	    (int)(path[pathi-1].dir+2)%4) ) ) ) {
      enter_square(pathi, hDC, hBrushLiving);
      pathi++;
      if ( maze[path[pathi].x][path[pathi].y] & START_SQUARE ) {
	    DeleteObject(hBrushLiving);
	    DeleteObject(hBrushDead);
	    ReleaseDC(hWnd, hDC);
          ret = 1;
      } else {
        ret = 0;
      }
    } else {
      action_done = 0;
    }
  } while (! action_done);

  return ret;
}


static void
enter_square(int n, HDC hDC, HBRUSH hBrush)  /* move into a neighboring square */
{
  draw_solid_square( (int)path[n].x, (int)path[n].y,
		    (int)path[n].dir, hDC, hBrush);

  path[n+1].dir = -1;
  switch (path[n].dir) {
  case 0: path[n+1].x = path[n].x;
    path[n+1].y = path[n].y - 1;
    break;
  case 1: path[n+1].x = path[n].x + 1;
    path[n+1].y = path[n].y;
    break;
  case 2: path[n+1].x = path[n].x;
    path[n+1].y = path[n].y + 1;
    break;
  case 3: path[n+1].x = path[n].x - 1;
    path[n+1].y = path[n].y;
    break;
  }
}

static void
start_timer(HWND hWnd, int iTimeout)
{
	waiting = TRUE;
	SetTimer(hWnd, 1, iTimeout, NULL);
}

/****************************************************************************

	FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

	PURPOSE: calls initialization function, processes message loop

	COMMENTS:

		Windows recognizes this function by name as the initial entry point
		for the program.  This function calls the application initialization
		routine, if no other instance of the program is running, and always
		calls the instance initialization routine.  It then executes a message
		retrieval and dispatch loop that is the top-level control structure
		for the remainder of execution.  The loop is terminated when a WM_QUIT
		message is received, at which time this function exits the application
		instance by returning the value passed by PostQuitMessage().

		If this function must abort before entering the message loop, it
		returns the conventional value NULL.

****************************************************************************/

int APIENTRY MazeMain(
	HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	HWND hParent)
{
	MSG msg;
	HDC hDC;

	/* Perform initializations that apply to a specific instance */

	if (!InitInstance(hInstance, hParent)) {
		return (FALSE);
	}

	waiting = FALSE;
	state = 1;

	/* Acquire and dispatch messages until a WM_QUIT message is received. */

	while (0 != state) {
		if (waiting) {
			(void) WaitMessage();
		}
		while (0 != state && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (WM_QUIT == msg.message) {
				state = 0;
			} else {
				DispatchMessage(&msg); /* Dispatches message to window */
			}
		}
    		switch (state) {
    		case 1:
      			initialize_maze();
			state = 2;
      			break;
	    	case 2:
			hDC = GetDC(hWnd);
			SendMessage(hWnd, WM_ERASEBKGND, (WPARAM) hDC, (LPARAM) 0);
			draw_maze_border(hWnd, hDC);
			ReleaseDC(hWnd, hDC);
			state = 3;
      			break;
	    	case 3:
			create_maze();
			state = 4;
			break;
	    	case 4:
			start_timer(hWnd, pre_solve_delay);
			state = 5;
			break;
		case 5:
			if (! waiting) {
				state = 6;
			}
			break;
    		case 6:
			begin_solve_maze();
			if (0 != solve_delay) {
				start_timer(hWnd, solve_delay);
				state = 7;
			} else {
				state = 8;
			}
			break;
		case 7:
			if (! waiting) {
				state = 8;
			}
			break;
		case 8:
			if (! solve_maze()) {
				if (0 != solve_delay) {
					start_timer(hWnd, solve_delay);
					state = 7;
				}
			} else {
				state = 9;
			}
			break;
		case 9:
			start_timer(hWnd, post_solve_delay);
			state = 10;
			break;
		case 10:
			if (! waiting) {
				state = 11;
			}
			break;
		case 11:
			state = 1;
			break;
		}
	}

	return (msg.wParam); /* Returns the value from PostQuitMessage */
}


/****************************************************************************

	FUNCTION:  InitInstance(HINSTANCE, int)

	PURPOSE:  Saves instance handle and creates main window

	COMMENTS:

		This function is called at initialization time for every instance of
		this application.  This function performs initialization tasks that
		cannot be shared by multiple instances.

		In this case, we save the instance handle in a static variable and
		create and display the main program window.

****************************************************************************/

static BOOL InitInstance(
	HINSTANCE          hInstance,
	HWND             hParent)
{
	RECT rect;

	/* Save the instance handle in static variable, which will be used in
	   many subsequence calls from this application to Windows. */

	hInst = hInstance; /* Store instance handle in our global variable */

	GetClientRect(hParent, &rect);
#if 0
	/* Create a main window for this application instance. */
	hWnd = CreateWindow(
		szAppName,	     	/* See RegisterClass() call. */
		szTitle,	     	/* Text for window title bar. */
		WS_CHILD,/* Window style. */
		0, 0, rect.right/2, rect.bottom/2, /* Use default positioning */
		hParent,		     /* We use a Parent. */
		NULL,		     	/* Use the window class menu. */
		hInstance,	     	/* This instance owns this window. */
		NULL		     	/* We don't use any data in our WM_CREATE */
	);
#endif
hWnd = hParent;
	// If window could not be created, return "failure"
	if (!hWnd) {
		return (FALSE);
	}

	// Make the window visible; update its client area; and return "success"
	ShowWindow(hWnd, SW_SHOW); // Show the window
	UpdateWindow(hWnd);         // Sends WM_PAINT message

	hPenWall = CreatePen(PS_SOLID, 3, RGB(150,150,150));

	return (TRUE);              // We succeeded...

}

static BOOL
OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	RECT rc;
	int size;

	srand((unsigned) time(NULL));

#if 0
	/* FIXME GetPrivateProfileInt not yet implemented in ReactOS */
	size = GetPrivateProfileInt("maze", "gridsize", 0, "maze.ini");
	pre_solve_delay = GetPrivateProfileInt("maze", "predelay", 5000,
	                                       "maze.ini");
	post_solve_delay = GetPrivateProfileInt("maze", "postdelay", 5000,
	                                        "maze.ini");
	solve_delay = GetPrivateProfileInt("maze", "solvedelay", 10,
	                                   "maze.ini");
#else
	size = 10;
	pre_solve_delay = 5000;
	post_solve_delay = 5000;
	solve_delay = 1;
#endif

  	if (size < 2) {
  		size = 7 + (rand() % 30);
	}
  	grid_width = grid_height = size;
    bw = (size > 6 ? 3 : (size-1)/2);

	GetClientRect(hWnd, &rc);
	set_maze_sizes(rc.right - rc.left, rc.bottom - rc.top);

	return TRUE;
}

void OnTimer(HWND hwnd, UINT id)
{
	waiting = FALSE;
}

/****************************************************************************

	FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)

	PURPOSE:  Processes messages

	MESSAGES:

	WM_DESTROY    - destroy window

	COMMENTS:

****************************************************************************/

LRESULT CALLBACK MazeWndProc(
		HWND hWnd,         // window handle
		UINT message,      // type of message
		WPARAM wParam,     // additional information
		LPARAM lParam)     // additional information
{
	PAINTSTRUCT ps;

	switch (message) {
		case WM_CREATE:
			OnCreate(hWnd, (LPCREATESTRUCT) lParam);
			break;
		case WM_PAINT:
			BeginPaint(hWnd, &ps);
			state = 1;
			EndPaint(hWnd, &ps);
		case WM_TIMER:
			OnTimer(hWnd, wParam);
			break;
		case WM_DESTROY:  // message: window being destroyed
			PostQuitMessage(0);
			break;

		default:          // Passes it on if unproccessed
			return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}
