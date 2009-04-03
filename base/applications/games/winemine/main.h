/*
 * Copyright 2000 Joshua Thielen <jt85296@ltu.edu>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>

static const TCHAR szWineMineRegKey[] = TEXT("Software\\Microsoft\\WinMine");

// Common Controls 6.0 for MSVC 2005 or later
#if _MSC_VER >= 1400
#   pragma comment(linker, "/manifestdependency:\"type='win32' "   \
        "name='Microsoft.Windows.Common-Controls' "                \
        "version='6.0.0.0' "                                       \
        "processorArchitecture='x86' "                             \
        "publicKeyToken='6595b64144ccf1df' "                       \
        "language='*'\"")
#endif

#define ID_TIMER              1000

#define BEGINNER_MINES        10
#define BEGINNER_COLS         9
#define BEGINNER_ROWS         9

#define ADVANCED_MINES        40
#define ADVANCED_COLS         16
#define ADVANCED_ROWS         16

#define EXPERT_MINES          99
#define EXPERT_COLS           30
#define EXPERT_ROWS           16

#define MAX_COLS              30
#define MAX_ROWS              24

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

typedef enum { SPRESS_BMP, COOL_BMP, DEAD_BMP, OOH_BMP, SMILE_BMP } FACE_BMP;

typedef enum { WAITING, PLAYING, GAMEOVER, WON } GAME_STATUS;

typedef enum
{
    MPRESS_BMP, ONE_BMP, TWO_BMP, THREE_BMP, FOUR_BMP, FIVE_BMP, SIX_BMP,
    SEVEN_BMP, EIGHT_BMP, BOX_BMP, FLAG_BMP, QUESTION_BMP, EXPLODE_BMP,
    WRONG_BMP, MINE_BMP, QPRESS_BMP
} MINEBMP_OFFSET;

typedef enum { BEGINNER, ADVANCED, EXPERT, CUSTOM } DIFFICULTY;

typedef struct tagBOARD
{
    BOOL bMark;
    HINSTANCE hInst;
    HWND hWnd;
    HBITMAP hMinesBMP;
    HBITMAP hFacesBMP;
    HBITMAP hLedsBMP;
    RECT MinesRect;
    RECT FaceRect;
    RECT TimerRect;
    RECT CounterRect;

    ULONG uWidth;
    ULONG uHeight;
    POINT Pos;

    ULONG uTime;
    ULONG uNumFlags;
    ULONG uBoxesLeft;
    ULONG uNumMines;

    ULONG uRows;
    ULONG uCols;
    ULONG uMines;
    TCHAR szBestName[3][16];
    ULONG uBestTime[3];
    DIFFICULTY Difficulty;

    POINT Press;

    FACE_BMP FaceBmp;
    GAME_STATUS Status;

    struct BOX_STRUCT
    {
        UINT bIsMine    : 1;
        UINT bIsPressed : 1;
        UINT uFlagType  : 2;
        UINT uNumMines  : 4;
    } Box [MAX_COLS + 2] [MAX_ROWS + 2];

    /* defines for uFlagType */
    #define NORMAL 0
    #define QUESTION 1
    #define FLAG 2
    #define COMPLETE 3

} BOARD;

void ExitApp( int error );
void InitBoard( BOARD *pBoard );
void LoadBoard( BOARD *pBoard );
void SaveBoard( BOARD *pBoard );
void DestroyBoard( BOARD *pBoard );
void SetDifficulty( BOARD *pBoard, DIFFICULTY difficulty );
void CheckLevel( BOARD *pBoard );
void CreateBoard( BOARD *pBoard );
void CreateBoxes( BOARD *pBoard );
void TestBoard( HWND hWnd, BOARD *pBoard, LONG x, LONG y, int msg );
void TestMines( BOARD *pBoard, POINT pt, int msg );
void TestFace( BOARD *pBoard, POINT pt, int msg );
void DrawBoard( HDC hdc, HDC hMemDC, PAINTSTRUCT *ps, BOARD *pBoard );
void DrawMines( HDC hdc, HDC hMemDC, BOARD *pBoard );
void DrawMine( HDC hdc, HDC hMemDC, BOARD *pBoard, ULONG uCol, ULONG uRow, BOOL IsPressed );
void AddFlag( BOARD *pBoard, ULONG uCol, ULONG uRow );
void CompleteBox( BOARD *pBoard, ULONG uCol, ULONG uRow );
void CompleteBoxes( BOARD *pBoard, ULONG uCol, ULONG uRow );
void PressBox( BOARD *pBoard, ULONG uCol, ULONG uRow );
void PressBoxes( BOARD *pBoard, ULONG uCol, ULONG uRow );
void UnpressBox( BOARD *pBoard, ULONG uCol, ULONG uRow );
void UnpressBoxes( BOARD *pBoard, ULONG uCol, ULONG uRow );
void UpdateTimer( BOARD *pBoard );
void DrawLeds( HDC hdc, HDC hMemDC, BOARD *pBoard, LONG nNumber, LONG x, LONG y);
void DrawFace( HDC hdc, HDC hMemDC, BOARD *pBoard );
LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CustomDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK CongratsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK TimesDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

/* end of header */
