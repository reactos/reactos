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
extern const TCHAR szMapWndClass[];
extern const TCHAR szLrgCellWndClass[];


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

BOOL RegisterControls(HINSTANCE hInstance);
VOID UnregisterControls(HINSTANCE hInstance);

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LrgCellWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MapWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* __DEVMGMT_PRECOMP_H */
