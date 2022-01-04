/******************************************************************************
 * [ maze ] ...
 *
 * modified:  [ 03-08-15 ] Ge van Geldorp <ge@gse.nl>
 *        ported to ReactOS
 * modified:  [ 94-10-8 ] Ge van Geldorp <Ge.vanGeldorp@lr.tudelft.nl>
 *        ported to MS Windows
 * modified:  [ 3-7-93 ]  Jamie Zawinski <jwz@lucid.com>
 *        added the XRoger logo, cleaned up resources, made
 *        grid size a parameter.
 * modified:  [ 3-3-93 ]  Jim Randell <jmr@mddjmr.fc.hp.com>
 *        Added the colour stuff and integrated it with jwz's
 *        screenhack stuff.  There's still some work that could
 *        be done on this, particularly allowing a resource to
 *        specify how big the squares are.
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

#include <windows.h>       /* required for all Windows applications */
#include <scrnsave.h>
#include <tchar.h>
#include <stdlib.h>
#include <commctrl.h>
#include <string.h>
#include <time.h>
#include "resource.h"

#define APPNAME _T("Maze")

LRESULT CALLBACK ScreenSaverProc(HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam);
static int choose_door();
static long backup();
static void draw_wall();
static void draw_solid_square(int, int, int, HDC, HBRUSH);
static void enter_square(int, HDC, HBRUSH);

extern HINSTANCE hMainInstance; /* current instance */
HBRUSH hBrushDead;
HBRUSH hBrushLiving;
HPEN   hPenWall;
HDC    hDC;

static int solve_delay, pre_solve_delay, post_solve_delay, size;

#define MAX_MAZE_SIZE_X    ((unsigned long) 1000) // Dynamic detection?
#define MAX_MAZE_SIZE_Y    ((unsigned long) 1000) // Dynamic detection?

#define MOVE_LIST_SIZE  (MAX_MAZE_SIZE_X * MAX_MAZE_SIZE_Y)

#define WALL_TOP    0x8000
#define WALL_RIGHT    0x4000
#define WALL_BOTTOM    0x2000
#define WALL_LEFT    0x1000

#define DOOR_IN_TOP    0x800
#define DOOR_IN_RIGHT    0x400
#define DOOR_IN_BOTTOM    0x200
#define DOOR_IN_LEFT    0x100
#define DOOR_IN_ANY    0xF00

#define DOOR_OUT_TOP    0x80
#define DOOR_OUT_RIGHT    0x40
#define DOOR_OUT_BOTTOM    0x20
#define DOOR_OUT_LEFT    0x10

#define START_SQUARE    0x2
#define END_SQUARE    0x1

#define    border_x        (0)
#define    border_y        (0)

#define    get_random(x)    (rand() % (x))

static unsigned short maze[MAX_MAZE_SIZE_X][MAX_MAZE_SIZE_Y];

static struct {
    unsigned int x;
    unsigned int y;
    unsigned int dir;
} move_list[MOVE_LIST_SIZE], save_path[MOVE_LIST_SIZE], path[MOVE_LIST_SIZE];

static int maze_size_x, maze_size_y;
static long sqnum, path_length;
static int cur_sq_x, cur_sq_y;
static int start_x, start_y, start_dir, end_x, end_y, end_dir;
static int grid_width, grid_height;
static int bw;
static int state = 1, pathi = 0;
static LPCWSTR registryPath = _T("Software\\Microsoft\\ScreenSavers\\mazescr");

static void SetDefaults()
{
    size = 10;
    pre_solve_delay = 5000;
    post_solve_delay = 5000;
    solve_delay = 1;
}

static void ReadRegistry()
{
    LONG result;
    HKEY skey;
    DWORD valuetype, valuesize, val_size, val_presd, val_postsd, val_sd;

    SetDefaults();

    result = RegOpenKeyEx(HKEY_CURRENT_USER, registryPath, 0, KEY_READ, &skey);
    if(result != ERROR_SUCCESS)
        return;

    valuesize = sizeof(DWORD);

    result = RegQueryValueEx(skey, _T("size"), NULL, &valuetype, (LPBYTE)&val_size, &valuesize);
    if(result == ERROR_SUCCESS)
        size = val_size;
    result = RegQueryValueEx(skey, _T("pre_solve_delay"), NULL, &valuetype, (LPBYTE)&val_presd, &valuesize);
    if(result == ERROR_SUCCESS)
        pre_solve_delay = val_presd;
    result = RegQueryValueEx(skey, _T("post_solve_delay"), NULL, &valuetype, (LPBYTE)&val_postsd, &valuesize);
    if(result == ERROR_SUCCESS)
        post_solve_delay = val_postsd;
    result = RegQueryValueEx(skey, _T("solve_delay"), NULL, &valuetype, (LPBYTE)&val_sd, &valuesize);
    if(result == ERROR_SUCCESS)
        solve_delay = val_sd;

    RegCloseKey(skey);
}

static void WriteRegistry()
{
    LONG result;
    HKEY skey;
    DWORD disp;

    result = RegCreateKeyEx(HKEY_CURRENT_USER, registryPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &skey, &disp);
    if(result != ERROR_SUCCESS)
        return;

    RegSetValueEx(skey, _T("size"), 0, REG_DWORD, (LPBYTE)&size, sizeof(size));
    RegSetValueEx(skey, _T("pre_solve_delay"), 0, REG_DWORD, (LPBYTE)&pre_solve_delay, sizeof(pre_solve_delay));
    RegSetValueEx(skey, _T("post_solve_delay"), 0, REG_DWORD, (LPBYTE)&post_solve_delay, sizeof(post_solve_delay));
    RegSetValueEx(skey, _T("solve_delay"), 0, REG_DWORD, (LPBYTE)&solve_delay, sizeof(solve_delay));

    RegCloseKey(skey);
}

static void set_maze_sizes(width, height)
int width, height;
{
    maze_size_x = (width -1)/ grid_width;
    maze_size_y = (height-1) / grid_height;
    if (maze_size_x > MAX_MAZE_SIZE_X)
        maze_size_x = MAX_MAZE_SIZE_X;
    if (maze_size_y > MAX_MAZE_SIZE_Y)
        maze_size_y = MAX_MAZE_SIZE_Y;
}

static void initialize_maze()         /* draw the surrounding wall and start/end squares */
{
    register int i, j, wall;

    /* initialize all squares */
    for (i = 0; i < maze_size_x; i++) {
        for (j = 0; j < maze_size_y; j++) {
            maze[i][j] = 0;
        }
    }

    /* top wall */
    for (i = 0; i < maze_size_x; i++) {
        maze[i][0] |= WALL_TOP;
    }

    /* right wall */
    for (j = 0; j < maze_size_y; j++) {
        maze[maze_size_x - 1][j] |= WALL_RIGHT;
    }

    /* bottom wall */
    for (i = 0; i < maze_size_x; i++) {
        maze[i][maze_size_y - 1] |= WALL_BOTTOM;
    }

    /* left wall */
    for (j = 0; j < maze_size_y; j++) {
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
    maze[i][j] |= (DOOR_IN_TOP >> wall);
    maze[i][j] &= ~(WALL_TOP >> wall);
    cur_sq_x = i;
    cur_sq_y = j;
    start_x = i;
    start_y = j;
    start_dir = wall;
    sqnum = 0;

    /* set end square */
    wall = (wall + 2) % 4;
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
    maze[i][j] |= (DOOR_OUT_TOP >> wall);
    maze[i][j] &= ~(WALL_TOP >> wall);
    end_x = i;
    end_y = j;
    end_dir = wall;
}

static void create_maze(HWND hWnd)             /* create a maze layout given the initialized maze */
{
    register int i, newdoor = 0;

    do {
        move_list[sqnum].x = cur_sq_x;
        move_list[sqnum].y = cur_sq_y;
        move_list[sqnum].dir = newdoor;
        while ((newdoor = choose_door(hDC)) == -1) { /* pick a door */
            if (backup() == -1) { /* no more doors ... backup */
                return; /* done ... return */
            }
        }

        /* mark the out door */
        maze[cur_sq_x][cur_sq_y] |= (DOOR_OUT_TOP >> newdoor);

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
        maze[cur_sq_x][cur_sq_y] |= (DOOR_IN_TOP >> ((newdoor + 2) % 4));

        /* if end square set path length and save path */
        if (maze[cur_sq_x][cur_sq_y] & END_SQUARE) {
            path_length = sqnum;
            for (i = 0; i < path_length; i++) {
                save_path[i].x = move_list[i].x;
                save_path[i].y = move_list[i].y;
                save_path[i].dir = move_list[i].dir;
            }
        }
    } while (1);
}

static int choose_door(HDC hDC)                                    /* pick a new path */
{
    int candidates[3];
    register int num_candidates;

    num_candidates = 0;

    /* top wall */
    if (maze[cur_sq_x][cur_sq_y] & DOOR_IN_TOP)
        goto rightwall;
    if (maze[cur_sq_x][cur_sq_y] & DOOR_OUT_TOP)
        goto rightwall;
    if (maze[cur_sq_x][cur_sq_y] & WALL_TOP)
        goto rightwall;
    if (maze[cur_sq_x][cur_sq_y - 1] & DOOR_IN_ANY) {
        maze[cur_sq_x][cur_sq_y] |= WALL_TOP;
        maze[cur_sq_x][cur_sq_y - 1] |= WALL_BOTTOM;
        draw_wall(cur_sq_x, cur_sq_y, 0, hDC);
        goto rightwall;
    }
    candidates[num_candidates++] = 0;

rightwall:
    /* right wall */
    if (maze[cur_sq_x][cur_sq_y] & DOOR_IN_RIGHT)
        goto bottomwall;
    if (maze[cur_sq_x][cur_sq_y] & DOOR_OUT_RIGHT)
        goto bottomwall;
    if (maze[cur_sq_x][cur_sq_y] & WALL_RIGHT)
        goto bottomwall;
    if (maze[cur_sq_x + 1][cur_sq_y] & DOOR_IN_ANY) {
        maze[cur_sq_x][cur_sq_y] |= WALL_RIGHT;
        maze[cur_sq_x + 1][cur_sq_y] |= WALL_LEFT;
        draw_wall(cur_sq_x, cur_sq_y, 1, hDC);
        goto bottomwall;
    }
    candidates[num_candidates++] = 1;

bottomwall:
    /* bottom wall */
    if (maze[cur_sq_x][cur_sq_y] & DOOR_IN_BOTTOM)
        goto leftwall;
    if (maze[cur_sq_x][cur_sq_y] & DOOR_OUT_BOTTOM)
        goto leftwall;
    if (maze[cur_sq_x][cur_sq_y] & WALL_BOTTOM)
        goto leftwall;
    if (maze[cur_sq_x][cur_sq_y + 1] & DOOR_IN_ANY) {
        maze[cur_sq_x][cur_sq_y] |= WALL_BOTTOM;
        maze[cur_sq_x][cur_sq_y + 1] |= WALL_TOP;
        draw_wall(cur_sq_x, cur_sq_y, 2, hDC);
        goto leftwall;
    }
    candidates[num_candidates++] = 2;

leftwall:
    /* left wall */
    if (maze[cur_sq_x][cur_sq_y] & DOOR_IN_LEFT)
        goto donewall;
    if (maze[cur_sq_x][cur_sq_y] & DOOR_OUT_LEFT)
        goto donewall;
    if (maze[cur_sq_x][cur_sq_y] & WALL_LEFT)
        goto donewall;
    if (maze[cur_sq_x - 1][cur_sq_y] & DOOR_IN_ANY) {
        maze[cur_sq_x][cur_sq_y] |= WALL_LEFT;
        maze[cur_sq_x - 1][cur_sq_y] |= WALL_RIGHT;
        draw_wall(cur_sq_x, cur_sq_y, 3, hDC);
        goto donewall;
    }
    candidates[num_candidates++] = 3;

donewall:
    if (num_candidates == 0)
        return -1;
    if (num_candidates == 1)
        return candidates[0];
    return candidates[get_random(num_candidates)];

}

static long backup()                                                  /* back up a move */
{
    sqnum--;
    if (0 <= sqnum) {
        cur_sq_x = move_list[sqnum].x;
        cur_sq_y = move_list[sqnum].y;
    }
    return sqnum;
}

static void draw_solid_square(i, j, dir, hDC, hBrush)          /* draw a solid square in a square */
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

static void draw_maze_border(HWND hWnd)    /* draw the maze outline */
{
    register int i, j;
    HBRUSH hBrush;

    SelectObject(hDC, hPenWall);

    for (i = 0; i < maze_size_x; i++) {
        if (maze[i][0] & WALL_TOP) {
            MoveToEx(hDC, border_x + grid_width * i, border_y, NULL);
            (void) LineTo(hDC, border_x + grid_width * (i + 1) - 1, border_y);
        }
        if ((maze[i][maze_size_y - 1] & WALL_BOTTOM)) {
            MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * (maze_size_y) -1, NULL);
            (void) LineTo(hDC, border_x + grid_width * (i + 1) - 1, border_y + grid_height * (maze_size_y) -1);
        }
    }
    for (j = 0; j < maze_size_y; j++) {
        if (maze[maze_size_x - 1][j] & WALL_RIGHT) {
            MoveToEx(hDC, border_x + grid_width * maze_size_x - 1, border_y + grid_height * j, NULL);
            (void) LineTo(hDC, border_x + grid_width * maze_size_x - 1, border_y + grid_height * (j + 1) - 1);
        }
        if (maze[0][j] & WALL_LEFT) {
            MoveToEx(hDC, border_x, border_y + grid_height * j, NULL);
            (void) LineTo(hDC, border_x, border_y + grid_height * (j + 1) - 1);
        }
    }

    hBrush = GetStockObject(WHITE_BRUSH);
    draw_solid_square(start_x, start_y, start_dir, hDC, hBrush);
    draw_solid_square(end_x, end_y, end_dir, hDC, hBrush);
}

static void draw_wall(i, j, dir, hDC)                                   /* draw a single wall */
register int i, j, dir;
HDC hDC;
{
    SelectObject(hDC, hPenWall);

    switch (dir) {
    case 0:
        MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * j, NULL);
        (void) LineTo(hDC, border_x + grid_width * (i + 1), border_y + grid_height * j);
        break;
    case 1:
        MoveToEx(hDC, border_x + grid_width * (i + 1), border_y + grid_height * j, NULL);
        (void) LineTo(hDC, border_x + grid_width * (i + 1), border_y + grid_height * (j + 1));
        break;
    case 2:
        MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * (j + 1), NULL);
        (void) LineTo(hDC, border_x + grid_width * (i + 1), border_y + grid_height * (j + 1));
        break;
    case 3:
        MoveToEx(hDC, border_x + grid_width * i, border_y + grid_height * j, NULL);
        (void) LineTo(hDC, border_x + grid_width * i, border_y + grid_height * (j + 1));
        break;
    }
}

static void begin_solve_maze(HWND hWnd)                             /* solve it with graphical feedback */
{
    /* plug up the surrounding wall */
    maze[start_x][start_y] |= (WALL_TOP >> start_dir);
    maze[end_x][end_y] |= (WALL_TOP >> end_dir);

    /* initialize search path */
    pathi = 0;
    path[pathi].x = end_x;
    path[pathi].y = end_y;
    path[pathi].dir = -1;
}

static int solve_maze(HWND hWnd)                             /* solve it with graphical feedback */
{
    int ret;
    int action_done;

    do {
        action_done = 1;
        if (++path[pathi].dir >= 4) {
            pathi--;
            draw_solid_square((int) (path[pathi].x), (int) (path[pathi].y), (int) (path[pathi].dir), hDC, hBrushDead);
            ret = 0;
        }
        else if (!(maze[path[pathi].x][path[pathi].y] & (WALL_TOP >> path[pathi].dir)) &&
            ((pathi == 0) || ((path[pathi].dir != (int) (path[pathi - 1].dir + 2) % 4)))) {
            enter_square(pathi, hDC, hBrushLiving);
            pathi++;
            if (maze[path[pathi].x][path[pathi].y] & START_SQUARE) {

                ret = 1;
            }
            else {
                ret = 0;
            }
        }
        else {
            action_done = 0;
        }
    } while (!action_done);
    return ret;
}

static void enter_square(int n, HDC hDC, HBRUSH hBrush)  /* move into a neighboring square */
{
    draw_solid_square((int) path[n].x, (int) path[n].y, (int) path[n].dir, hDC, hBrush);

    path[n + 1].dir = -1;
    switch (path[n].dir) {
    case 0: path[n + 1].x = path[n].x;
        path[n + 1].y = path[n].y - 1;
        break;
    case 1: path[n + 1].x = path[n].x + 1;
        path[n + 1].y = path[n].y;
        break;
    case 2: path[n + 1].x = path[n].x;
        path[n + 1].y = path[n].y + 1;
        break;
    case 3: path[n + 1].x = path[n].x - 1;
        path[n + 1].y = path[n].y;
        break;
    }
}

static void start_timer(HWND hWnd, int iTimeout)
{
    SetTimer(hWnd, 1, iTimeout, NULL);
}

static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    srand((unsigned) time(NULL));

    ReadRegistry();

    if (size < 2) {
        size = 7 + (rand() % 30);
    }
    grid_width = grid_height = size;
    bw = (size > 6 ? 3 : (size - 1) / 2);

#if 0
    /* FIXME Pattern brushes not yet implemented in ReactOS */
    {
        static long grayPattern [] = {
            0x55555555,
            0xaaaaaaaa,
            0x55555555,
            0xaaaaaaaa,
            0x55555555,
            0xaaaaaaaa,
            0x55555555,
            0xaaaaaaaa
        };
        static RGBQUAD argbq [] = {
            { 0, 0, 255, 0 },
            { 255, 255, 255, 0 }
        };
        BITMAPINFO *pbmi;

        pbmi = malloc(sizeof(BITMAPINFOHEADER) + sizeof(argbq) + sizeof(grayPattern));
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth = 8;
        pbmi->bmiHeader.biHeight = 8;
        pbmi->bmiHeader.biPlanes = 1;
        pbmi->bmiHeader.biBitCount = 1;
        pbmi->bmiHeader.biCompression = BI_RGB;
        (void) memcpy(pbmi->bmiColors, argbq, sizeof(argbq));
        (void) memcpy(pbmi->bmiColors + 2, grayPattern, sizeof(grayPattern));
        hBrushDead = CreateDIBPatternBrushPt(pbmi, DIB_RGB_COLORS);
        //    hBrushDead = CreateHatchBrush(HS_DIAGCROSS, RGB(255, 0, 0));
        free(pbmi);
    }
#else
    hBrushDead = CreateSolidBrush(RGB(255, 0, 0));
#endif
    hBrushLiving = CreateSolidBrush(RGB(0, 255, 0));
    hPenWall = CreatePen(PS_SOLID, 3, RGB(150, 150, 150));

    hDC = GetDC(hWnd);

    start_timer(hWnd, 1);

    return TRUE;
}

BOOL WINAPI AboutProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message){
    case WM_COMMAND:
        switch(LOWORD(wparam)){
        case IDOK:
            EndDialog(hWnd, LOWORD(wparam));
            return TRUE;
        }
    }
    return FALSE;
}

static void ReadSettings(HWND hWnd)
{
    size = SendDlgItemMessage(hWnd, IDC_SLIDER_SIZE, TBM_GETPOS, 0, 0);
    SetDlgItemInt(hWnd, IDC_TEXT_SIZE, size, FALSE);

    pre_solve_delay = SendDlgItemMessage(hWnd, IDC_SLIDER_PRESD, TBM_GETPOS, 0, 0);
    SetDlgItemInt(hWnd, IDC_TEXT_PRESD, pre_solve_delay, FALSE);

    post_solve_delay = SendDlgItemMessage(hWnd, IDC_SLIDER_POSTSD, TBM_GETPOS, 0, 0);
    SetDlgItemInt(hWnd, IDC_TEXT_POSTSD, post_solve_delay, FALSE);

    solve_delay = SendDlgItemMessage(hWnd, IDC_SLIDER_SD, TBM_GETPOS, 0, 0);
    SetDlgItemInt(hWnd, IDC_TEXT_SD, solve_delay, FALSE);
}

LRESULT CALLBACK ScreenSaverProc(
    HWND hWnd,         // window handle
    UINT message,      // type of message
    WPARAM wParam,     // additional information
    LPARAM lParam)     // additional information
{
    switch (message)
    {
    case WM_CREATE:
        OnCreate(hWnd, (LPCREATESTRUCT) lParam);
        break;
    case WM_SIZE:
        set_maze_sizes(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_TIMER:
        switch (state)
        {
        case 2:
            begin_solve_maze(hWnd);

            state = 3;

            start_timer(hWnd, solve_delay);
            break;

        case 3:
            if (!solve_maze(hWnd))
            {
                start_timer(hWnd, solve_delay);
            }
            else
            {
                state = 1;
                start_timer(hWnd, post_solve_delay);
            }
            break;

        default:
            initialize_maze();

            SendMessage(hWnd, WM_ERASEBKGND, (WPARAM) hDC, (LPARAM) 0);
            draw_maze_border(hWnd);

            create_maze(hWnd);

            state = 2;

            start_timer(hWnd, pre_solve_delay);
            break;
        }
        break;

    case WM_DESTROY:  // message: window being destroyed
        DeleteObject(hBrushLiving);
        DeleteObject(hBrushDead);
        ReleaseDC(hWnd, hDC);
        break;

    default:          // Passes it on if unproccessed
        return DefScreenSaverProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            ReadRegistry();
            //Set slider ranges
            SendDlgItemMessage(hWnd, IDC_SLIDER_SIZE, TBM_SETRANGE, FALSE, MAKELPARAM(5, 64));
            SendDlgItemMessage(hWnd, IDC_SLIDER_PRESD, TBM_SETRANGE, FALSE, MAKELPARAM(1, 10000));
            SendDlgItemMessage(hWnd, IDC_SLIDER_POSTSD, TBM_SETRANGE, FALSE, MAKELPARAM(1, 10000));
            SendDlgItemMessage(hWnd, IDC_SLIDER_SD, TBM_SETRANGE, FALSE, MAKELPARAM(1, 10000));
            //Set current values to slider
            SendDlgItemMessage(hWnd, IDC_SLIDER_SIZE, TBM_SETPOS, TRUE, size);
            SendDlgItemMessage(hWnd, IDC_SLIDER_PRESD, TBM_SETPOS, TRUE, pre_solve_delay);
            SendDlgItemMessage(hWnd, IDC_SLIDER_POSTSD, TBM_SETPOS, TRUE, post_solve_delay);
            SendDlgItemMessage(hWnd, IDC_SLIDER_SD, TBM_SETPOS, TRUE, solve_delay);
            //Set current values to texts
            SetDlgItemInt(hWnd, IDC_TEXT_SIZE, size, FALSE);
            SetDlgItemInt(hWnd, IDC_TEXT_PRESD, pre_solve_delay, FALSE);
            SetDlgItemInt(hWnd, IDC_TEXT_POSTSD, post_solve_delay, FALSE);
            SetDlgItemInt(hWnd, IDC_TEXT_SD, solve_delay, FALSE);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wparam))
            {
                case IDOK:
                    WriteRegistry();
                    EndDialog(hWnd, TRUE);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hWnd, TRUE);
                    break;
                case IDABOUT:
                    DialogBox(hMainInstance, MAKEINTRESOURCE(IDD_DLG_ABOUT), hWnd, (DLGPROC)AboutProc);
                    break;
            }
        case WM_HSCROLL:
            ReadSettings(hWnd);
            return TRUE;
    }
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hmodule)
{
    return TRUE;
}
