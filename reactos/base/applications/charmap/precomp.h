#ifndef __CHARMAP_PRECOMP_H
#define __CHARMAP_PRECOMP_H

#include <windows.h>

#include "resource.h"

#define SIZEOF(_v)  (sizeof(_v) / sizeof(*_v))

#define MAX_GLYPHS  65536

#define XCELLS 20
#define YCELLS 10
#define XLARGE 45
#define YLARGE 25

#define FM_SETFONT  (WM_USER + 1)
#define FM_GETCHAR  (WM_USER + 2)
#define FM_SETCHAR  (WM_USER + 3)
#define FM_GETHFONT (WM_USER + 4)

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

    USHORT ValidGlyphs[MAX_GLYPHS];
    USHORT NumValidGlyphs;

} MAP, *PMAP;

typedef struct {
    NMHDR hdr;
    WCHAR ch;
} MAPNOTIFY, *LPMAPNOTIFY;

typedef struct {
    BOOL IsAdvancedView;
} SETTINGS;

extern SETTINGS Settings;
extern HWND hCharmapDlg;

LRESULT CALLBACK LrgCellWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

VOID ShowAboutDlg(HWND hWndParent);

BOOL RegisterMapClasses(HINSTANCE hInstance);
VOID UnregisterMapClasses(HINSTANCE hInstance);

/* charmap.c */
extern VOID ChangeMapFont(HWND hDlg);

/* settings.c */
extern void LoadSettings(void);
extern void SaveSettings(void);

#endif /* __CHARMAP_PRECOMP_H */
