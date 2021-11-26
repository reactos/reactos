#ifndef __CHARMAP_PRECOMP_H
#define __CHARMAP_PRECOMP_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>


#include "resource.h"

#define SIZEOF(_v)  (sizeof(_v) / sizeof(*_v))

#define MAX_GLYPHS  65536

#define XCELLS 20
#define YCELLS 10
#define XLARGE 45
#define YLARGE 25

#define FM_SETFONT    (WM_USER + 1)
#define FM_GETCHAR    (WM_USER + 2)
#define FM_SETCHAR    (WM_USER + 3)
#define FM_GETHFONT   (WM_USER + 4)
#define FM_SETCHARMAP (WM_USER + 5)

// the code pages to display in the advanced 'character set' combobox
static const UINT codePages[] = {
    864, 775, 863, 855, 737, 856, 862, 861, 852, 869, 850, 858, 865, 860, 866, 857, 437,    // OEM code pages
    1256, 1257, 1250, 1251, 1253, 1255, 932, 949, 1252, 936, 874, 950, 1254, 1258           // ANSI code pages
};

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
    INT NumRows;
    INT CharMap;

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

int WINAPI GetUName(IN WORD wCharCode, OUT LPWSTR lpBuf);

/* charmap.c */
VOID UpdateStatusBar(WCHAR wch);
extern VOID ChangeMapFont(HWND hDlg);

/* settings.c */
extern void LoadSettings(void);
extern void SaveSettings(void);

#endif /* __CHARMAP_PRECOMP_H */
