/*****************************************************************************\
*                                                                             *
* userproc.h - Addition exports from USER.EXE				      *
*                                                                             *
* Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

/*  lParam of WM_DROPOBJECT and WM_QUERYDROPOBJECT points to one of these.
 */
typedef struct _dropstruct
  {
    HWND  hwndSource;
    HWND  hwndSink;
    WORD  wFmt;
    DWORD dwData;
    POINT ptDrop;
    DWORD dwControlData;
  } DROPSTRUCT;

#define DOF_EXECUTABLE	0x8001
#define DOF_DOCUMENT	0x8002
#define DOF_DIRECTORY	0x8003
#define DOF_MULTIPLE	0x8004

typedef DROPSTRUCT FAR * LPDROPSTRUCT;

WORD FAR PASCAL GetInternalWindowPos(HWND,LPRECT,LPPOINT);
BOOL FAR PASCAL SetInternalWindowPos(HWND,WORD,LPRECT,LPPOINT);

void FAR PASCAL CalcChildScroll(HWND,WORD);
void FAR PASCAL ScrollChildren(HWND,WORD,WORD,LONG);

DWORD FAR PASCAL DragObject(HWND hwndParent, HWND hwndFrom, WORD wFmt,
    DWORD dwData, HANDLE hCursor);
BOOL FAR PASCAL DragDetect(HWND hwnd, POINT pt);

void FAR PASCAL FillWindow(HWND hwndBrush, HWND hwndPaint, HDC hdc,
    HBRUSH hBrush);
