/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid
 * Copyright    2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              2002 Eric Pouech
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

#define MAX_LANGUAGE_NUMBER     255
#define MAX_STRING_LEN          255

#define INTERNAL_BORDER_WIDTH   5
#define POPUP_YDISTANCE         20
#define SHADOW_DX               10
#define SHADOW_DY               10
#define BUTTON_CX               6
#define BUTTON_CY               6

#ifndef RC_INVOKED

#include <stdarg.h>

#include "hlpfile.h"
#include "windef.h"
#include "winbase.h"
#include "macro.h"
#include "winhelp_res.h"

typedef struct tagHelpButton
{
    HWND                hWnd;

    LPCSTR              lpszID;
    LPCSTR              lpszName;
    LPCSTR              lpszMacro;

    WPARAM              wParam;

    RECT                rect;

    struct tagHelpButton*next;
} WINHELP_BUTTON;

typedef struct
{
    HLPFILE_PAGE*       page;
    HLPFILE_WINDOWINFO* wininfo;
    ULONG               relative;
} WINHELP_WNDPAGE;

typedef struct tagPageSet
{
    /* FIXME: for now it's a fixed size */
    WINHELP_WNDPAGE     set[40];
    unsigned            index;
} WINHELP_PAGESET;

typedef struct tagWinHelp
{
    unsigned            ref_count;
    WINHELP_BUTTON*     first_button;
    HLPFILE_PAGE*       page;

    HWND                hMainWnd;
    HWND                hHistoryWnd;

    WNDPROC             origRicheditWndProc;

    HFONT*              fonts;
    UINT                fonts_len;

    HCURSOR             hHandCur;

    HBRUSH              hBrush;

    HLPFILE_WINDOWINFO* info;

    WINHELP_PAGESET     back;
    unsigned            font_scale; /* 0 = small, 1 = normal, 2 = large */

    struct tagWinHelp*  next;
} WINHELP_WINDOW;

#define DC_NOMSG     0x00000000
#define DC_MINMAX    0x00000001
#define DC_INITTERM  0x00000002
#define DC_JUMP      0x00000004
#define DC_ACTIVATE  0x00000008
#define DC_CALLBACKS 0x00000010

#define DW_NOTUSED    0
#define DW_WHATMSG    1
#define DW_MINMAX     2
#define DW_SIZE       3
#define DW_INIT       4
#define DW_TERM       5
#define DW_STARTJUMP  6
#define DW_ENDJUMP    7
#define DW_CHGFILE    8
#define DW_ACTIVATE   9
#define	DW_CALLBACKS 10

typedef LONG (CALLBACK *WINHELP_LDLLHandler)(WORD, LONG_PTR, LONG_PTR);

typedef struct tagDll
{
    HANDLE              hLib;
    const char*         name;
    WINHELP_LDLLHandler handler;
    DWORD               class;
    struct tagDll*      next;
} WINHELP_DLL;

typedef struct
{
    UINT                wVersion;
    HANDLE              hInstance;
    BOOL                isBook;
    WINHELP_WINDOW*     active_win;
    WINHELP_WINDOW*     active_popup;
    WINHELP_WINDOW*     win_list;
    WNDPROC             button_proc;
    WINHELP_DLL*        dlls;
    WINHELP_PAGESET     history;
    HFONT               hButtonFont;
} WINHELP_GLOBALS;

extern const struct winhelp_callbacks
{
    WORD      (WINAPI *GetFSError)(void);
    HANDLE    (WINAPI *HfsOpenSz)(LPSTR,BYTE);
    WORD      (WINAPI *RcCloseHfs)(HANDLE);
    HANDLE    (WINAPI *HfOpenHfs)(HANDLE,LPSTR,BYTE);
    HANDLE    (WINAPI *RcCloseHf)(HANDLE);
    LONG      (WINAPI *LcbReadHf)(HANDLE,BYTE*,LONG);
    LONG      (WINAPI *LTellHf)(HANDLE);
    LONG      (WINAPI *LSeekHf)(HANDLE,LONG,WORD);
    BOOL      (WINAPI *FEofHf)(HANDLE);
    LONG      (WINAPI *LcbSizeHf)(HANDLE);
    BOOL      (WINAPI *FAccessHfs)(HANDLE,LPSTR,BYTE);
    WORD      (WINAPI *RcLLInfoFromHf)(HANDLE,WORD,LPWORD,LPLONG,LPLONG);
    WORD      (WINAPI *RcLLInfoFromHfs)(HANDLE,LPSTR,WORD,LPWORD,LPLONG,LPLONG);
    void      (WINAPI *ErrorW)(int);
    void      (WINAPI *ErrorSz)(LPSTR);
    ULONG_PTR (WINAPI *GetInfo)(WORD,HWND);
    LONG      (WINAPI *API)(LPSTR,WORD,DWORD);
} Callbacks;

extern WINHELP_GLOBALS Globals;

BOOL WINHELP_CreateHelpWindow(WINHELP_WNDPAGE*, int, BOOL);
BOOL WINHELP_OpenHelpWindow(HLPFILE_PAGE* (*)(HLPFILE*, LONG, ULONG*),
                            HLPFILE*, LONG, HLPFILE_WINDOWINFO*, int);
BOOL WINHELP_GetOpenFileName(LPSTR, int);
BOOL WINHELP_CreateIndexWindow(BOOL);
void WINHELP_DeleteBackSet(WINHELP_WINDOW*);
HLPFILE* WINHELP_LookupHelpFile(LPCSTR lpszFile);
HLPFILE_WINDOWINFO* WINHELP_GetWindowInfo(HLPFILE* hlpfile, LPCSTR name);
void WINHELP_LayoutMainWindow(WINHELP_WINDOW* win);
WINHELP_WINDOW* WINHELP_GrabWindow(WINHELP_WINDOW*);
BOOL WINHELP_ReleaseWindow(WINHELP_WINDOW*);

extern const char MAIN_WIN_CLASS_NAME[];
extern const char BUTTON_BOX_WIN_CLASS_NAME[];
extern const char TEXT_WIN_CLASS_NAME[];
extern const char SHADOW_WIN_CLASS_NAME[];
extern const char HISTORY_WIN_CLASS_NAME[];
extern const char STRING_BUTTON[];
extern const char STRING_MENU_Xx[];
extern const char STRING_DIALOG_TEST[];
#endif

/* Buttons */
#define WH_FIRST_BUTTON     500
