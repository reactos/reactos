/*
 * Copyright 2000 Joshua Thielen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WINMINE_H_
#define _WINMINE_H_

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>

#include "resource.h"

#define BEGINNER_MINES        10
#define BEGINNER_COLS         9
#define BEGINNER_ROWS         9

#define ADVANCED_MINES        40
#define ADVANCED_COLS         16
#define ADVANCED_ROWS         16

#define EXPERT_MINES          99
#define EXPERT_COLS           30
#define EXPERT_ROWS           16

#define MAX_COLS        30
#define MAX_ROWS        24

#define BOTTOM_MARGIN 20
#define BOARD_WMARGIN 5
#define BOARD_HMARGIN 5

/* mine defines */
#define MINE_WIDTH       16
#define MINE_HEIGHT      16
#define LED_WIDTH        12
#define LED_HEIGHT       23
#define FACE_WIDTH       24
#define FACE_HEIGHT      24

#define MAX_PLAYER_NAME_SIZE 31

typedef enum { SPRESS_BMP, COOL_BMP, DEAD_BMP, OOH_BMP, SMILE_BMP } FACE_BMP;

typedef enum { WAITING, PLAYING, GAMEOVER, WON } GAME_STATUS;

typedef enum {
     MPRESS_BMP, ONE_BMP, TWO_BMP, THREE_BMP, FOUR_BMP, FIVE_BMP, SIX_BMP,
     SEVEN_BMP, EIGHT_BMP, BOX_BMP, FLAG_BMP, QUESTION_BMP, EXPLODE_BMP,
     WRONG_BMP, MINE_BMP, QPRESS_BMP
} MINEBMP_OFFSET;

typedef enum { BEGINNER, ADVANCED, EXPERT, CUSTOM } DIFFICULTY;

typedef struct tagBOARD
{
    BOOL IsMarkQ;
    HDC    hdc;
    HINSTANCE hInst;
    HWND    hWnd;
    HBITMAP hMinesBMP;
    HBITMAP hFacesBMP;
    HBITMAP hLedsBMP;
    RECT mines_rect;
    RECT face_rect;
    RECT timer_rect;
    RECT counter_rect;

    unsigned width;
    unsigned height;
    POINT pos;

    unsigned time;
    unsigned num_flags;
    unsigned boxes_left;
    unsigned num_mines;

    /* difficulty info */
    unsigned rows;
    unsigned cols;
    unsigned mines;
    WCHAR best_name [3][MAX_PLAYER_NAME_SIZE+1];
    DWORD best_time [3];
    DIFFICULTY difficulty;

    POINT press;

    /* defines for mb */
#define MB_NONE 0
#define MB_LEFTDOWN 1
#define MB_LEFTUP 2
#define MB_RIGHTDOWN 3
#define MB_RIGHTUP 4
#define MB_BOTHDOWN 5
#define MB_BOTHUP 6
    unsigned mb;

    FACE_BMP face_bmp;
    GAME_STATUS status;
    struct BOX_STRUCT
    {
        unsigned IsMine    : 1;
        unsigned IsPressed : 1;
        unsigned FlagType  : 2;
        unsigned NumMines  : 4;
    } box [MAX_COLS + 2] [MAX_ROWS + 2];

    /* defines for FlagType */
#define NORMAL 0
#define QUESTION 1
#define FLAG 2
#define COMPLETE 3

} BOARD;

void CheckLevel( BOARD *p_board );

INT_PTR CALLBACK CustomDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK CongratsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK TimesDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

#endif /* _WINMINE_H_ */
