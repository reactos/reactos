#ifndef __CHARMAP_PRECOMP_H
#define __CHARMAP_PRECOMP_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#define XCELLS 20
#define YCELLS 10
#define XLARGE 45
#define YLARGE 25

#define FM_SETFONT (WM_USER + 1)
#define FM_GETCHAR (WM_USER + 2)
#define FM_SETCHAR (WM_USER + 3)

extern HINSTANCE hInstance;

typedef struct _CELL
{
    RECT CellExt;
    RECT CellInt;
    BOOL bActive;
    BOOL bLarge;
    WCHAR ch;
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
    LOGFONTW CurrentFont;
    INT iYStart;
} MAP, *PMAP;

typedef struct {
    NMHDR hdr;
    WCHAR ch;
} MAPNOTIFY, *LPMAPNOTIFY;


LRESULT CALLBACK LrgCellWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

VOID ShowAboutDlg(HWND hWndParent);

BOOL RegisterMapClasses(HINSTANCE hInstance);
VOID UnregisterMapClasses(HINSTANCE hInstance);

#endif /* __CHARMAP_PRECOMP_H */
