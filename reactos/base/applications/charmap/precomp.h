#ifndef __CHARMAP_PRECOMP_H
#define __CHARMAP_PRECOMP_H
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

#define XCELLS 20
#define YCELLS 10
#define XLARGE 45
#define YLARGE 25

#define FM_SETFONT (WM_USER + 1)

extern HINSTANCE hInstance;


typedef struct _CELL
{
    RECT CellExt;
    RECT CellInt;
    BOOL bActive;
    BOOL bLarge;
    TCHAR ch;
} CELL, *PCELL;

typedef struct _MAP
{
    HWND hMapWnd;
    HWND hParent;
    HWND hLrgWnd;
    SIZE ClientSize;
    SIZE CellSize;
    CELL Cells[YCELLS][XCELLS];
    PCELL pActiveCell;
    HFONT hFont;
    LOGFONT CurrentFont;
    INT iPage;
} MAP, *PMAP;

VOID ShowAboutDlg(HWND hWndParent);

BOOL RegisterMapClasses(HINSTANCE hInstance);
VOID UnregisterMapClasses(HINSTANCE hInstance);

#endif /* __DEVMGMT_PRECOMP_H */
