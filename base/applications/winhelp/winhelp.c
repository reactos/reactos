/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid <uschmid@mail.hh.provi.de>
 *              2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              2002 Eric Pouech <eric.pouech@wanadoo.fr>
 *              2004 Ken Belleau <jamez@ivic.qc.ca>
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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "winhelp.h"
#include "winhelp_res.h"
#include "shellapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

static BOOL    WINHELP_RegisterWinClasses(void);
static LRESULT CALLBACK WINHELP_MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_TextWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_ButtonBoxWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_ButtonWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_HistoryWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WINHELP_ShadowWndProc(HWND, UINT, WPARAM, LPARAM);
static void    WINHELP_CheckPopup(UINT);
static BOOL    WINHELP_SplitLines(HWND hWnd, LPSIZE);
static void    WINHELP_InitFonts(HWND hWnd);
static void    WINHELP_DeleteLines(WINHELP_WINDOW*);
static void    WINHELP_DeleteWindow(WINHELP_WINDOW*);
static void    WINHELP_SetupText(HWND hWnd);
static WINHELP_LINE_PART* WINHELP_IsOverLink(WINHELP_WINDOW*, WPARAM, LPARAM);

WINHELP_GLOBALS Globals = {3, NULL, NULL, 0, TRUE, NULL, NULL, NULL, NULL};


/***********************************************************************
 *
 *           WINHELP_GetOpenFileName
 */
BOOL WINHELP_GetOpenFileName(LPSTR lpszFile, int len)
{
    OPENFILENAME openfilename;
    CHAR szDir[MAX_PATH];
    CHAR szzFilter[2 * MAX_STRING_LEN + 100];
    LPSTR p = szzFilter;

    WINE_TRACE("()\n");

    LoadString(Globals.hInstance, STID_HELP_FILES_HLP, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.hlp");
    p += strlen(p) + 1;
    LoadString(Globals.hInstance, STID_ALL_FILES, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.*");
    p += strlen(p) + 1;
    *p = '\0';

    GetCurrentDirectory(sizeof(szDir), szDir);

    lpszFile[0]='\0';

    openfilename.lStructSize       = sizeof(OPENFILENAME);
    openfilename.hwndOwner         = NULL;
    openfilename.hInstance         = Globals.hInstance;
    openfilename.lpstrFilter       = szzFilter;
    openfilename.lpstrCustomFilter = 0;
    openfilename.nMaxCustFilter    = 0;
    openfilename.nFilterIndex      = 1;
    openfilename.lpstrFile         = lpszFile;
    openfilename.nMaxFile          = len;
    openfilename.lpstrFileTitle    = 0;
    openfilename.nMaxFileTitle     = 0;
    openfilename.lpstrInitialDir   = szDir;
    openfilename.lpstrTitle        = 0;
    openfilename.Flags             = 0;
    openfilename.nFileOffset       = 0;
    openfilename.nFileExtension    = 0;
    openfilename.lpstrDefExt       = 0;
    openfilename.lCustData         = 0;
    openfilename.lpfnHook          = 0;
    openfilename.lpTemplateName    = 0;

    return GetOpenFileName(&openfilename);
}

/***********************************************************************
 *
 *           WINHELP_LookupHelpFile
 */
HLPFILE* WINHELP_LookupHelpFile(LPCSTR lpszFile)
{
    HLPFILE*        hlpfile;
    char szFullName[MAX_PATH];
    char szAddPath[MAX_PATH];
    char *p;

    /*
     * NOTE: This is needed by popup windows only.
     * In other cases it's not needed but does not hurt though.
     */
    if (Globals.active_win && Globals.active_win->page && Globals.active_win->page->file)
    {
        strcpy(szAddPath, Globals.active_win->page->file->lpszPath);
        p = strrchr(szAddPath, '\\');
        if (p) *p = 0;
    }

    /*
     * FIXME: Should we swap conditions?
     */
    if (!SearchPath(NULL, lpszFile, ".hlp", MAX_PATH, szFullName, NULL) &&
        !SearchPath(szAddPath, lpszFile, ".hlp", MAX_PATH, szFullName, NULL))
    {
        if (WINHELP_MessageBoxIDS_s(STID_FILE_NOT_FOUND_s, lpszFile, STID_WHERROR,
                                    MB_YESNO|MB_ICONQUESTION) != IDYES)
            return NULL;
        if (!WINHELP_GetOpenFileName(szFullName, MAX_PATH))
            return NULL;
    }
    hlpfile = HLPFILE_ReadHlpFile(szFullName);
    if (!hlpfile)
        WINHELP_MessageBoxIDS_s(STID_HLPFILE_ERROR_s, lpszFile,
                                STID_WHERROR, MB_OK|MB_ICONSTOP);
    return hlpfile;
}

/******************************************************************
 *		WINHELP_GetWindowInfo
 *
 *
 */
HLPFILE_WINDOWINFO*     WINHELP_GetWindowInfo(HLPFILE* hlpfile, LPCSTR name)
{
    static      HLPFILE_WINDOWINFO      mwi;
    unsigned int     i;

    if (!name || !name[0])
        name = Globals.active_win->lpszName;

    if (hlpfile)
        for (i = 0; i < hlpfile->numWindows; i++)
            if (!strcmp(hlpfile->windows[i].name, name))
                return &hlpfile->windows[i];

    if (strcmp(name, "main") != 0)
    {
        WINE_FIXME("Couldn't find window info for %s\n", name);
        assert(0);
        return NULL;
    }
    if (!mwi.name[0])
    {
        strcpy(mwi.type, "primary");
        strcpy(mwi.name, "main");
        if (!LoadString(Globals.hInstance, STID_WINE_HELP, 
                        mwi.caption, sizeof(mwi.caption)))
            strcpy(mwi.caption, hlpfile->lpszTitle);
        mwi.origin.x = mwi.origin.y = mwi.size.cx = mwi.size.cy = CW_USEDEFAULT;
        mwi.style = SW_SHOW;
        mwi.win_style = WS_OVERLAPPEDWINDOW;
        mwi.sr_color = mwi.sr_color = 0xFFFFFF;
    }
    return &mwi;
}

/******************************************************************
 *		HLPFILE_GetPopupWindowInfo
 *
 *
 */
static HLPFILE_WINDOWINFO*     WINHELP_GetPopupWindowInfo(HLPFILE* hlpfile, HWND hParentWnd, POINT* mouse)
{
    static      HLPFILE_WINDOWINFO      wi;

    RECT parent_rect;
    
    wi.type[0] = wi.name[0] = wi.caption[0] = '\0';

    /* Calculate horizontal size and position of a popup window */
    GetWindowRect(hParentWnd, &parent_rect);
    wi.size.cx = (parent_rect.right  - parent_rect.left) / 2;
    wi.size.cy = 10; /* need a non null value, so that border are taken into account while computing */

    wi.origin = *mouse;
    ClientToScreen(hParentWnd, &wi.origin);
    wi.origin.x -= wi.size.cx / 2;
    wi.origin.x  = min(wi.origin.x, GetSystemMetrics(SM_CXSCREEN) - wi.size.cx);
    wi.origin.x  = max(wi.origin.x, 0);

    wi.style = SW_SHOW;
    wi.win_style = WS_POPUPWINDOW;
    wi.sr_color = wi.sr_color = 0xFFFFFF;

    return &wi;
}

/***********************************************************************
 *
 *           WinMain
 */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG                 msg;
    LONG                lHash = 0;
    HLPFILE*            hlpfile;
    static CHAR         default_wndname[] = "main";
    LPSTR               wndname = default_wndname;
    WINHELP_DLL*        dll;

    Globals.hInstance = hInstance;

    /* Get options */
    while (*cmdline && (*cmdline == ' ' || *cmdline == '-'))
    {
        CHAR   option;
        LPCSTR topic_id;
        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline && *cmdline == ' ') cmdline++;
        switch (option)
	{
	case 'i':
	case 'I':
            topic_id = cmdline;
            while (*cmdline && *cmdline != ' ') cmdline++;
            if (*cmdline) *cmdline++ = '\0';
            lHash = HLPFILE_Hash(topic_id);
            break;

	case '3':
	case '4':
            Globals.wVersion = option - '0';
            break;

        case 'x':
            show = SW_HIDE; 
            Globals.isBook = FALSE;
            break;

        default:
            WINE_FIXME("Unsupported cmd line: %s\n", cmdline);
            break;
	}
    }

    /* Create primary window */
    if (!WINHELP_RegisterWinClasses())
    {
        WINE_FIXME("Couldn't register classes\n");
        return 0;
    }

    if (*cmdline)
    {
        char*   ptr;
        if ((*cmdline == '"') && (ptr = strchr(cmdline+1, '"')))
        {
            cmdline++;
            *ptr = '\0';
        }
        if ((ptr = strchr(cmdline, '>')))
        {
            *ptr = '\0';
            wndname = ptr + 1;
        }
        hlpfile = WINHELP_LookupHelpFile(cmdline);
        if (!hlpfile) return 0;
    }
    else hlpfile = NULL;
    WINHELP_CreateHelpWindowByHash(hlpfile, lHash, 
                                   WINHELP_GetWindowInfo(hlpfile, wndname), show);

    /* Message loop */
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    for (dll = Globals.dlls; dll; dll = dll->next)
    {
        if (dll->class & DC_INITTERM) dll->handler(DW_TERM, 0, 0);
    }
    return 0;
}

/***********************************************************************
 *
 *           RegisterWinClasses
 */
static BOOL WINHELP_RegisterWinClasses(void)
{
    WNDCLASS class_main, class_button_box, class_text, class_shadow, class_history;

    class_main.style               = CS_HREDRAW | CS_VREDRAW;
    class_main.lpfnWndProc         = WINHELP_MainWndProc;
    class_main.cbClsExtra          = 0;
    class_main.cbWndExtra          = sizeof(LONG);
    class_main.hInstance           = Globals.hInstance;
    class_main.hIcon               = LoadIcon(0, IDI_APPLICATION);
    class_main.hCursor             = LoadCursor(0, IDC_ARROW);
    class_main.hbrBackground       = GetStockObject(WHITE_BRUSH);
    class_main.lpszMenuName        = 0;
    class_main.lpszClassName       = MAIN_WIN_CLASS_NAME;

    class_button_box               = class_main;
    class_button_box.lpfnWndProc   = WINHELP_ButtonBoxWndProc;
    class_button_box.hbrBackground = GetStockObject(GRAY_BRUSH);
    class_button_box.lpszClassName = BUTTON_BOX_WIN_CLASS_NAME;

    class_text = class_main;
    class_text.lpfnWndProc         = WINHELP_TextWndProc;
    class_text.hbrBackground       = 0;
    class_text.lpszClassName       = TEXT_WIN_CLASS_NAME;

    class_shadow = class_main;
    class_shadow.lpfnWndProc       = WINHELP_ShadowWndProc;
    class_shadow.hbrBackground     = GetStockObject(GRAY_BRUSH);
    class_shadow.lpszClassName     = SHADOW_WIN_CLASS_NAME;

    class_history = class_main;
    class_history.lpfnWndProc      = WINHELP_HistoryWndProc;
    class_history.lpszClassName    = HISTORY_WIN_CLASS_NAME;

    return (RegisterClass(&class_main) &&
            RegisterClass(&class_button_box) &&
            RegisterClass(&class_text) &&
            RegisterClass(&class_shadow) &&
            RegisterClass(&class_history));
}

typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP,*LPWINHELP;

/******************************************************************
 *		WINHELP_HandleCommand
 *
 *
 */
static LRESULT  WINHELP_HandleCommand(HWND hSrcWnd, LPARAM lParam)
{
    COPYDATASTRUCT*     cds = (COPYDATASTRUCT*)lParam;
    WINHELP*            wh;

    if (cds->dwData != 0xA1DE505)
    {
        WINE_FIXME("Wrong magic number (%08lx)\n", cds->dwData);
        return 0;
    }

    wh = (WINHELP*)cds->lpData;

    if (wh)
    {
        char*   ptr = (wh->ofsFilename) ? (LPSTR)wh + wh->ofsFilename : NULL;

        WINE_TRACE("Got[%u]: cmd=%u data=%08x fn=%s\n",
                   wh->size, wh->command, wh->data, ptr);
        switch (wh->command)
        {
        case HELP_CONTEXT:
            if (ptr)
            {
                MACRO_JumpContext(ptr, "main", wh->data);
            }
            break;
        case HELP_QUIT:
            MACRO_Exit();
            break;
        case HELP_CONTENTS:
            if (ptr)
            {
                MACRO_JumpContents(ptr, "main");
            }
            break;
        case HELP_HELPONHELP:
            MACRO_HelpOn();
            break;
        /* case HELP_SETINDEX: */
        case HELP_SETCONTENTS:
            if (ptr)
            {
                MACRO_SetContents(ptr, wh->data);
            }
            break;
        case HELP_CONTEXTPOPUP:
            if (ptr)
            {
                MACRO_PopupContext(ptr, wh->data);
            }
            break;
        /* case HELP_FORCEFILE:*/
        /* case HELP_CONTEXTMENU: */
        case HELP_FINDER:
            /* in fact, should be the topic dialog box */
            WINE_FIXME("HELP_FINDER: stub\n");
            if (ptr)
            {
                MACRO_JumpHash(ptr, "main", 0);
            }
            break;
        /* case HELP_WM_HELP: */
        /* case HELP_SETPOPUP_POS: */
        /* case HELP_KEY: */
        /* case HELP_COMMAND: */
        /* case HELP_PARTIALKEY: */
        /* case HELP_MULTIKEY: */
        /* case HELP_SETWINPOS: */
            WINE_FIXME("Unknown command (%x) for remote winhelp control\n", wh->command);
            break;
        }
    }
    return 0L;
}

/******************************************************************
 *		WINHELP_ReuseWindow
 *
 *
 */
static BOOL     WINHELP_ReuseWindow(WINHELP_WINDOW* win, WINHELP_WINDOW* oldwin, 
                                    HLPFILE_PAGE* page, int nCmdShow)
{
    unsigned int i;

    win->hMainWnd      = oldwin->hMainWnd;
    win->hButtonBoxWnd = oldwin->hButtonBoxWnd;
    win->hTextWnd      = oldwin->hTextWnd;
    win->hHistoryWnd   = oldwin->hHistoryWnd;
    oldwin->hMainWnd = oldwin->hButtonBoxWnd = oldwin->hTextWnd = oldwin->hHistoryWnd = 0;
    win->hBrush = oldwin->hBrush;

    SetWindowLong(win->hMainWnd,      0, (LONG)win);
    SetWindowLong(win->hButtonBoxWnd, 0, (LONG)win);
    SetWindowLong(win->hTextWnd,      0, (LONG)win);
    SetWindowLong(win->hHistoryWnd,   0, (LONG)win);

    WINHELP_InitFonts(win->hMainWnd);

    if (page)
        SetWindowText(win->hMainWnd, page->file->lpszTitle);

    WINHELP_SetupText(win->hTextWnd);
    InvalidateRect(win->hTextWnd, NULL, TRUE);
    SendMessage(win->hMainWnd, WM_USER, 0, 0);
    ShowWindow(win->hMainWnd, nCmdShow);
    UpdateWindow(win->hTextWnd);

    if (!(win->info->win_style & WS_POPUP))
    {
        unsigned        num;

        memcpy(win->history, oldwin->history, sizeof(win->history));
        win->histIndex = oldwin->histIndex;

        /* FIXME: when using back, we shouldn't update the history... */

        if (page)
        {
            for (i = 0; i < win->histIndex; i++)
                if (win->history[i] == page) break;

            /* if the new page is already in the history, do nothing */
            if (i == win->histIndex)
            {
                num = sizeof(win->history) / sizeof(win->history[0]);
                if (win->histIndex == num)
                {
                    /* we're full, remove latest entry */
                    HLPFILE_FreeHlpFile(win->history[0]->file);
                    memmove(&win->history[0], &win->history[1], 
                            (num - 1) * sizeof(win->history[0]));
                    win->histIndex--;
                }
                win->history[win->histIndex++] = page;
                page->file->wRefCount++;
                if (win->hHistoryWnd) InvalidateRect(win->hHistoryWnd, NULL, TRUE);
            }
        }

        memcpy(win->back, oldwin->back, sizeof(win->back));
        win->backIndex = oldwin->backIndex;

        if (page)
        {
            num = sizeof(win->back) / sizeof(win->back[0]);
            if (win->backIndex == num)
            {
                /* we're full, remove latest entry */
                HLPFILE_FreeHlpFile(win->back[0]->file);
                memmove(&win->back[0], &win->back[1], 
                        (num - 1) * sizeof(win->back[0]));
                win->backIndex--;
            }
            win->back[win->backIndex++] = page;
            page->file->wRefCount++;
        }
    }
    else
        win->backIndex = win->histIndex = 0;

    oldwin->histIndex = oldwin->backIndex = 0;
    WINHELP_DeleteWindow(oldwin);
    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindow
 */
BOOL WINHELP_CreateHelpWindow(HLPFILE_PAGE* page, HLPFILE_WINDOWINFO* wi,
                              int nCmdShow)
{
    WINHELP_WINDOW *win, *oldwin;
    HWND hWnd;
    BOOL bPrimary;
    BOOL bPopup;
    LPSTR name;

    bPrimary = !lstrcmpi(wi->name, "main");
    bPopup = wi->win_style & WS_POPUP;

    /* Initialize WINHELP_WINDOW struct */
    win = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(WINHELP_WINDOW) + strlen(wi->name) + 1);
    if (!win) return FALSE;

    win->next = Globals.win_list;
    Globals.win_list = win;

    name = (char*)win + sizeof(WINHELP_WINDOW);
    lstrcpy(name, wi->name);
    win->lpszName = name;

    win->page = page;

    win->hArrowCur = LoadCursorA(0, (LPSTR)IDC_ARROW);
    win->hHandCur = LoadCursorA(0, (LPSTR)IDC_HAND);

    win->info = wi;

    Globals.active_win = win;

    /* Initialize default pushbuttons */
    if (bPrimary && page)
    {
        CHAR    buffer[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STID_CONTENTS, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_CONTENTS", buffer, "Contents()");
        LoadString(Globals.hInstance, STID_SEARCH,buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_SEARCH", buffer, "Search()");
        LoadString(Globals.hInstance, STID_BACK, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_BACK", buffer, "Back()");
        LoadString(Globals.hInstance, STID_HISTORY, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_HISTORY", buffer, "History()");
        LoadString(Globals.hInstance, STID_TOPICS, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_TOPICS", buffer, "Finder()");
    }

    /* Initialize file specific pushbuttons */
    if (!(wi->win_style & WS_POPUP) && page)
    {
        HLPFILE_MACRO  *macro;
        for (macro = page->file->first_macro; macro; macro = macro->next)
            MACRO_ExecuteMacro(macro->lpszMacro);

        for (macro = page->first_macro; macro; macro = macro->next)
            MACRO_ExecuteMacro(macro->lpszMacro);
    }

    /* Reuse existing window */
    if (!bPopup)
    {
        for (oldwin = win->next; oldwin; oldwin = oldwin->next)
        {
            if (!lstrcmpi(oldwin->lpszName, wi->name))
            {
                return WINHELP_ReuseWindow(win, oldwin, page, nCmdShow);
            }
        }
        if (page)
        {
            win->histIndex = win->backIndex = 1;
            win->history[0] = win->back[0] = page;
            page->file->wRefCount += 2;
            strcpy(wi->caption, page->file->lpszTitle);
        }
    }

    hWnd = CreateWindow(bPopup ? TEXT_WIN_CLASS_NAME : MAIN_WIN_CLASS_NAME,
                        wi->caption, 
                        bPrimary ? WS_OVERLAPPEDWINDOW : wi->win_style,
                        wi->origin.x, wi->origin.y, wi->size.cx, wi->size.cy,
                        NULL, bPrimary ? LoadMenu(Globals.hInstance, MAKEINTRESOURCE(MAIN_MENU)) : 0,
                        Globals.hInstance, win);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindowByHash
 */
BOOL WINHELP_CreateHelpWindowByHash(HLPFILE* hlpfile, LONG lHash, 
                                    HLPFILE_WINDOWINFO* wi, int nCmdShow)
{
    HLPFILE_PAGE*       page = NULL;

    if (hlpfile)
        page = lHash ? HLPFILE_PageByHash(hlpfile, lHash) : 
            HLPFILE_Contents(hlpfile);
    if (page) page->file->wRefCount++;
    return WINHELP_CreateHelpWindow(page, wi, nCmdShow);
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindowByMap
 */
BOOL WINHELP_CreateHelpWindowByMap(HLPFILE* hlpfile, LONG lMap,
                                   HLPFILE_WINDOWINFO* wi, int nCmdShow)
{
    HLPFILE_PAGE*       page = NULL;

    page = HLPFILE_PageByMap(hlpfile, lMap);
    if (page) page->file->wRefCount++;
    return WINHELP_CreateHelpWindow(page, wi, nCmdShow);
}

/***********************************************************************
 *
 *           WINHELP_MainWndProc
 */
static LRESULT CALLBACK WINHELP_MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW *win;
    WINHELP_BUTTON *button;
    RECT rect, button_box_rect;
    INT  text_top, curPos, min, max, dy, keyDelta;

    WINHELP_CheckPopup(msg);

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG) win);
        win->hMainWnd = hWnd;
        break;

    case WM_CREATE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        /* Create button box and text Window */
        CreateWindow(BUTTON_BOX_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
                     0, 0, 0, 0, hWnd, 0, Globals.hInstance, win);

        CreateWindow(TEXT_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
                     0, 0, 0, 0, hWnd, 0, Globals.hInstance, win);

        /* Fall through */
    case WM_USER:
    case WM_WINDOWPOSCHANGED:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        GetClientRect(hWnd, &rect);

        /* Update button box and text Window */
        SetWindowPos(win->hButtonBoxWnd, HWND_TOP,
                     rect.left, rect.top,
                     rect.right - rect.left,
                     rect.bottom - rect.top, 0);

        GetWindowRect(win->hButtonBoxWnd, &button_box_rect);
        text_top = rect.top + button_box_rect.bottom - button_box_rect.top;

        SetWindowPos(win->hTextWnd, HWND_TOP,
                     rect.left, text_top,
                     rect.right - rect.left,
                     rect.bottom - text_top, 0);

        break;

    case WM_COMMAND:
        Globals.active_win = win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        switch (wParam)
	{
            /* Menu FILE */
	case MNID_FILE_OPEN:    MACRO_FileOpen();       break;
	case MNID_FILE_PRINT:	MACRO_Print();          break;
	case MNID_FILE_SETUP:	MACRO_PrinterSetup();   break;
	case MNID_FILE_EXIT:	MACRO_Exit();           break;

            /* Menu EDIT */
	case MNID_EDIT_COPYDLG: MACRO_CopyDialog();     break;
	case MNID_EDIT_ANNOTATE:MACRO_Annotate();       break;

            /* Menu Bookmark */
	case MNID_BKMK_DEFINE:  MACRO_BookmarkDefine(); break;

            /* Menu Help */
	case MNID_HELP_HELPON:	MACRO_HelpOn();         break;
	case MNID_HELP_HELPTOP: MACRO_HelpOnTop();      break;
	case MNID_HELP_ABOUT:	MACRO_About();          break;
	case MNID_HELP_WINE:    ShellAbout(hWnd, "WINE", "Help", 0); break;

	default:
            /* Buttons */
            for (button = win->first_button; button; button = button->next)
                if (wParam == button->wParam) break;
            if (button)
                MACRO_ExecuteMacro(button->lpszMacro);
            else
                WINHELP_MessageBoxIDS(STID_NOT_IMPLEMENTED, 0x121, MB_OK);
            break;
	}
        break;
    case WM_DESTROY:
        if (Globals.hPopupWnd) DestroyWindow(Globals.hPopupWnd);
        break;
    case WM_COPYDATA:
        return WINHELP_HandleCommand((HWND)wParam, lParam);

    case WM_KEYDOWN:
        keyDelta = 0;

        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
            keyDelta = GetSystemMetrics(SM_CXVSCROLL);
            if (wParam == VK_UP)
                keyDelta = -keyDelta;

        case VK_PRIOR:
        case VK_NEXT:
            win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
            curPos = GetScrollPos(win->hTextWnd, SB_VERT);
            GetScrollRange(win->hTextWnd, SB_VERT, &min, &max);

            if (keyDelta == 0)
            {            
                GetClientRect(win->hTextWnd, &rect);
                keyDelta = (rect.bottom - rect.top) / 2;
                if (wParam == VK_PRIOR)
                    keyDelta = -keyDelta;
            }

            curPos += keyDelta;
            if (curPos > max)
                 curPos = max;
            else if (curPos < min)
                 curPos = min;

            dy = GetScrollPos(win->hTextWnd, SB_VERT) - curPos;
            SetScrollPos(win->hTextWnd, SB_VERT, curPos, TRUE);
            ScrollWindow(win->hTextWnd, 0, dy, NULL, NULL);
            UpdateWindow(win->hTextWnd);
            return 0;

        case VK_ESCAPE:
            MACRO_Exit();
            return 0;
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_ButtonBoxWndProc
 */
static LRESULT CALLBACK WINHELP_ButtonBoxWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINDOWPOS      *winpos;
    WINHELP_WINDOW *win;
    WINHELP_BUTTON *button;
    SIZE button_size;
    INT  x, y;

    WINHELP_CheckPopup(msg);

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG) win);
        win->hButtonBoxWnd = hWnd;
        break;

    case WM_WINDOWPOSCHANGING:
        winpos = (WINDOWPOS*) lParam;
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        /* Update buttons */
        button_size.cx = 0;
        button_size.cy = 0;
        for (button = win->first_button; button; button = button->next)
	{
            HDC  hDc;
            SIZE textsize;
            if (!button->hWnd)
            {
                button->hWnd = CreateWindow(STRING_BUTTON, button->lpszName,
                                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                            0, 0, 0, 0,
                                            hWnd, (HMENU) button->wParam,
                                            Globals.hInstance, 0);
                if (button->hWnd) {
                    if (Globals.button_proc == NULL)
                        Globals.button_proc = (WNDPROC) GetWindowLongPtr(button->hWnd, GWLP_WNDPROC);
                    SetWindowLongPtr(button->hWnd, GWLP_WNDPROC, (LONG_PTR) WINHELP_ButtonWndProc);
                }
            }
            hDc = GetDC(button->hWnd);
            GetTextExtentPoint(hDc, button->lpszName,
                               lstrlen(button->lpszName), &textsize);
            ReleaseDC(button->hWnd, hDc);

            button_size.cx = max(button_size.cx, textsize.cx + BUTTON_CX);
            button_size.cy = max(button_size.cy, textsize.cy + BUTTON_CY);
	}

        x = 0;
        y = 0;
        for (button = win->first_button; button; button = button->next)
	{
            SetWindowPos(button->hWnd, HWND_TOP, x, y, button_size.cx, button_size.cy, 0);

            if (x + 2 * button_size.cx <= winpos->cx)
                x += button_size.cx;
            else
                x = 0, y += button_size.cy;
	}
        winpos->cy = y + (x ? button_size.cy : 0);
        break;

    case WM_COMMAND:
        SendMessage(GetParent(hWnd), msg, wParam, lParam);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_ESCAPE:
            return SendMessage(GetParent(hWnd), msg, wParam, lParam);
        }
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_ButtonWndProc
 */
static LRESULT CALLBACK WINHELP_ButtonWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN)
    {
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_ESCAPE:
            return SendMessage(GetParent(hWnd), msg, wParam, lParam);
        }
    }

    return CallWindowProc(Globals.button_proc, hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_TextWndProc
 */
static LRESULT CALLBACK WINHELP_TextWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW    *win;
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    WINDOWPOS         *winpos;
    PAINTSTRUCT        ps;
    HDC   hDc;
    POINT mouse;
    INT   scroll_pos;
    HWND  hPopupWnd;
    BOOL  bExit;

    if (msg != WM_LBUTTONDOWN)
        WINHELP_CheckPopup(msg);

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG) win);
        win->hTextWnd = hWnd;
        win->hBrush = CreateSolidBrush(win->info->sr_color);
        if (win->info->win_style & WS_POPUP) Globals.hPopupWnd = win->hMainWnd = hWnd;
        WINHELP_InitFonts(hWnd);
        break;

    case WM_CREATE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        /* Calculate vertical size and position of a popup window */
        if (win->info->win_style & WS_POPUP)
	{
            POINT origin;
            RECT old_window_rect;
            RECT old_client_rect;
            SIZE old_window_size;
            SIZE old_client_size;
            SIZE new_client_size;
            SIZE new_window_size;

            GetWindowRect(hWnd, &old_window_rect);
            origin.x = old_window_rect.left;
            origin.y = old_window_rect.top;
            old_window_size.cx = old_window_rect.right  - old_window_rect.left;
            old_window_size.cy = old_window_rect.bottom - old_window_rect.top;

            GetClientRect(hWnd, &old_client_rect);
            old_client_size.cx = old_client_rect.right  - old_client_rect.left;
            old_client_size.cy = old_client_rect.bottom - old_client_rect.top;

            new_client_size = old_client_size;
            WINHELP_SplitLines(hWnd, &new_client_size);

            if (origin.y + POPUP_YDISTANCE + new_client_size.cy <= GetSystemMetrics(SM_CYSCREEN))
                origin.y += POPUP_YDISTANCE;
            else
                origin.y -= POPUP_YDISTANCE + new_client_size.cy;

            new_window_size.cx = old_window_size.cx - old_client_size.cx + new_client_size.cx;
            new_window_size.cy = old_window_size.cy - old_client_size.cy + new_client_size.cy;

            win->hShadowWnd =
                CreateWindow(SHADOW_WIN_CLASS_NAME, "", WS_POPUP,
                             origin.x + SHADOW_DX, origin.y + SHADOW_DY,
                             new_window_size.cx, new_window_size.cy,
                             0, 0, Globals.hInstance, 0);

            SetWindowPos(hWnd, HWND_TOP, origin.x, origin.y,
                         new_window_size.cx, new_window_size.cy,
                         0);
            SetWindowPos(win->hShadowWnd, hWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            ShowWindow(win->hShadowWnd, SW_NORMAL);
            SetActiveWindow(hWnd);
	}
        break;

    case WM_WINDOWPOSCHANGED:
        winpos = (WINDOWPOS*) lParam;

        if (!(winpos->flags & SWP_NOSIZE)) WINHELP_SetupText(hWnd);
        break;

    case WM_MOUSEWHEEL:
    {
       int wheelDelta = 0;
       UINT scrollLines = 3;
       int curPos = GetScrollPos(hWnd, SB_VERT);
       int min, max;

       GetScrollRange(hWnd, SB_VERT, &min, &max);

       SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0, &scrollLines, 0);
       if (wParam & (MK_SHIFT | MK_CONTROL))
           return DefWindowProc(hWnd, msg, wParam, lParam);
       wheelDelta -= GET_WHEEL_DELTA_WPARAM(wParam);
       if (abs(wheelDelta) >= WHEEL_DELTA && scrollLines) {
           int dy;

           curPos += wheelDelta;
           if (curPos > max)
                curPos = max;
           else if (curPos < min)
                curPos = min;

           dy = GetScrollPos(hWnd, SB_VERT) - curPos;
           SetScrollPos(hWnd, SB_VERT, curPos, TRUE);
           ScrollWindow(hWnd, 0, dy, NULL, NULL);
           UpdateWindow(hWnd);
       }
    }
    break;

    case WM_VSCROLL:
    {
	BOOL  update = TRUE;
	RECT  rect;
	INT   Min, Max;
	INT   CurPos = GetScrollPos(hWnd, SB_VERT);
	INT   dy;
	
	GetScrollRange(hWnd, SB_VERT, &Min, &Max);
	GetClientRect(hWnd, &rect);

	switch (wParam & 0xffff)
        {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: CurPos  = wParam >> 16;                   break;
        case SB_TOP:           CurPos  = Min;                            break;
        case SB_BOTTOM:        CurPos  = Max;                            break;
        case SB_PAGEUP:        CurPos -= (rect.bottom - rect.top) / 2;   break;
        case SB_PAGEDOWN:      CurPos += (rect.bottom - rect.top) / 2;   break;
        case SB_LINEUP:        CurPos -= GetSystemMetrics(SM_CXVSCROLL); break;
        case SB_LINEDOWN:      CurPos += GetSystemMetrics(SM_CXVSCROLL); break;
        default: update = FALSE;
        }
	if (update)
        {
	    if (CurPos > Max)
                CurPos = Max;
	    else if (CurPos < Min)
                CurPos = Min;
	    dy = GetScrollPos(hWnd, SB_VERT) - CurPos;
	    SetScrollPos(hWnd, SB_VERT, CurPos, TRUE);
	    ScrollWindow(hWnd, 0, dy, NULL, NULL);
	    UpdateWindow(hWnd);
        }
    }
    break;

    case WM_PAINT:
        hDc = BeginPaint(hWnd, &ps);
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        scroll_pos = GetScrollPos(hWnd, SB_VERT);

        /* No DPtoLP needed - MM_TEXT map mode */
        if (ps.fErase) FillRect(hDc, &ps.rcPaint, win->hBrush);
        for (line = win->first_line; line; line = line->next)
        {
            for (part = &line->first_part; part; part = part->next)
            {
                switch (part->cookie)
                {
                case hlp_line_part_text:
                    SelectObject(hDc, part->u.text.hFont);
                    SetTextColor(hDc, part->u.text.color);
                    SetBkColor(hDc, win->info->sr_color);
                    TextOut(hDc, part->rect.left, part->rect.top - scroll_pos,
                            part->u.text.lpsText, part->u.text.wTextLen);
                    if (part->u.text.wUnderline)
                    {
                        HPEN    hPen;

                        switch (part->u.text.wUnderline)
                        {
                        case 1: /* simple */
                        case 2: /* double */
                            hPen = CreatePen(PS_SOLID, 1, part->u.text.color);
                            break;
                        case 3: /* dotted */
                            hPen = CreatePen(PS_DOT, 1, part->u.text.color);
                            break;
                        default:
                            WINE_FIXME("Unknow underline type\n");
                            continue;
                        }

                        SelectObject(hDc, hPen);
                        MoveToEx(hDc, part->rect.left, part->rect.bottom - scroll_pos - 1, NULL);
                        LineTo(hDc, part->rect.right, part->rect.bottom - scroll_pos - 1);
                        if (part->u.text.wUnderline == 2)
                        {
                            MoveToEx(hDc, part->rect.left, part->rect.bottom - scroll_pos + 1, NULL);
                            LineTo(hDc, part->rect.right, part->rect.bottom - scroll_pos + 1);
                        }
                        DeleteObject(hPen);
                    }
                    break;
                case hlp_line_part_bitmap:
                    {
                        HDC hMemDC;

                        hMemDC = CreateCompatibleDC(hDc);
                        SelectObject(hMemDC, part->u.bitmap.hBitmap);
                        BitBlt(hDc, part->rect.left, part->rect.top - scroll_pos,
                               part->rect.right - part->rect.left, part->rect.bottom - part->rect.top,
                               hMemDC, 0, 0, SRCCOPY);
                        DeleteDC(hMemDC);
                    }
                    break;
                case hlp_line_part_metafile:
                    {
                        HDC hMemDC;
                        HBITMAP hBitmap;
                        SIZE sz;
                        RECT rc;

                        sz.cx = part->rect.right - part->rect.left;
                        sz.cy = part->rect.bottom - part->rect.top;
                        hMemDC = CreateCompatibleDC(hDc);
                        hBitmap = CreateCompatibleBitmap(hDc, sz.cx, sz.cy);
                        SelectObject(hMemDC, hBitmap);
                        SelectObject(hMemDC, win->hBrush);
                        rc.left = 0;
                        rc.top = 0;
                        rc.right = sz.cx;
                        rc.bottom = sz.cy;
                        FillRect(hMemDC, &rc, win->hBrush);
                        SetMapMode(hMemDC, part->u.metafile.mm);
                        SetWindowExtEx(hMemDC, sz.cx, sz.cy, 0);
                        SetViewportExtEx(hMemDC, sz.cx, sz.cy, 0);
                        SetWindowOrgEx(hMemDC, 0, 0, 0);
                        SetViewportOrgEx(hMemDC, 0, 0, 0);
                        PlayMetaFile(hMemDC, part->u.metafile.hMetaFile);
                        SetMapMode(hMemDC, MM_TEXT);
                        SetWindowOrgEx(hMemDC, 0, 0, 0);
                        SetViewportOrgEx(hMemDC, 0, 0, 0);
                        BitBlt(hDc, part->rect.left, part->rect.top - scroll_pos,
                               sz.cx, sz.cy, hMemDC, 0, 0, SRCCOPY);
                        DeleteDC(hMemDC);
                        DeleteObject(hBitmap);
                    }
                    break;
                }
            }
        }

        EndPaint(hWnd, &ps);
        break;

    case WM_MOUSEMOVE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        if (WINHELP_IsOverLink(win, wParam, lParam))
            SetCursor(win->hHandCur); /* set to hand pointer cursor to indicate a link */
        else
            SetCursor(win->hArrowCur); /* set to hand pointer cursor to indicate a link */

        break;

    case WM_LBUTTONDOWN:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        hPopupWnd = Globals.hPopupWnd;
        Globals.hPopupWnd = 0;

        part = WINHELP_IsOverLink(win, wParam, lParam);
        if (part)
        {
            HLPFILE*            hlpfile;
            HLPFILE_WINDOWINFO* wi;

            mouse.x = (short)LOWORD(lParam);
            mouse.y = (short)HIWORD(lParam);

            if (part->link) switch (part->link->cookie)
            {
            case hlp_link_link:
                hlpfile = WINHELP_LookupHelpFile(part->link->lpszString);
                if (part->link->window == -1)
                    wi = win->info;
                else if ((part->link->window >= 0) && (part->link->window < hlpfile->numWindows))
                    wi = &hlpfile->windows[part->link->window];
                else
                {
                    WINE_WARN("link to window %d/%d\n", part->link->window, hlpfile->numWindows);
                    break;
                }
                WINHELP_CreateHelpWindowByHash(hlpfile, part->link->lHash, wi,
                                               SW_NORMAL);
                break;
            case hlp_link_popup:
                hlpfile = WINHELP_LookupHelpFile(part->link->lpszString);
                if (hlpfile) WINHELP_CreateHelpWindowByHash(hlpfile, part->link->lHash, 
                                               WINHELP_GetPopupWindowInfo(hlpfile, hWnd, &mouse),
                                               SW_NORMAL);
                break;
            case hlp_link_macro:
                MACRO_ExecuteMacro(part->link->lpszString);
                break;
            default:
                WINE_FIXME("Unknown link cookie %d\n", part->link->cookie);
            }
        }

        if (hPopupWnd)
            DestroyWindow(hPopupWnd);
        break;

    case WM_NCDESTROY:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);

        if (hWnd == Globals.hPopupWnd) Globals.hPopupWnd = 0;

        bExit = (Globals.wVersion >= 4 && !lstrcmpi(win->lpszName, "main"));
        DeleteObject(win->hBrush);

        WINHELP_DeleteWindow(win);

        if (bExit) MACRO_Exit();

        if (!Globals.win_list)
            PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/******************************************************************
 *		WINHELP_HistoryWndProc
 *
 *
 */
static LRESULT CALLBACK WINHELP_HistoryWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW*     win;
    PAINTSTRUCT         ps;
    HDC                 hDc;
    TEXTMETRIC          tm;
    unsigned int        i;
    RECT                r;

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLong(hWnd, 0, (LONG)win);
        win->hHistoryWnd = hWnd;
        break;
    case WM_CREATE:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        hDc = GetDC(hWnd);
        GetTextMetrics(hDc, &tm);
        GetWindowRect(hWnd, &r);

        r.right = r.left + 30 * tm.tmAveCharWidth;
        r.bottom = r.top + (sizeof(win->history) / sizeof(win->history[0])) * tm.tmHeight;
        AdjustWindowRect(&r, GetWindowLong(hWnd, GWL_STYLE), FALSE);
        if (r.left < 0) {r.right -= r.left; r.left = 0;}
        if (r.top < 0) {r.bottom -= r.top; r.top = 0;}

        MoveWindow(hWnd, r.left, r.top, r.right, r.bottom, TRUE);
        ReleaseDC(hWnd, hDc);
        break;
    case WM_LBUTTONDOWN:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        hDc = GetDC(hWnd);
        GetTextMetrics(hDc, &tm);
        i = HIWORD(lParam) / tm.tmHeight;
        if (i < win->histIndex)
            WINHELP_CreateHelpWindow(win->history[i], win->info, SW_SHOW);
        ReleaseDC(hWnd, hDc);
        break;
    case WM_PAINT:
        hDc = BeginPaint(hWnd, &ps);
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        GetTextMetrics(hDc, &tm);

        for (i = 0; i < win->histIndex; i++)
        {
            TextOut(hDc, 0, i * tm.tmHeight, win->history[i]->lpszTitle, 
                    strlen(win->history[i]->lpszTitle));
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
        if (hWnd == win->hHistoryWnd)
            win->hHistoryWnd = 0;
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WINHELP_ShadowWndProc
 */
static LRESULT CALLBACK WINHELP_ShadowWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_CheckPopup(msg);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           SetupText
 */
static void WINHELP_SetupText(HWND hWnd)
{
    HDC  hDc = GetDC(hWnd);
    RECT rect;
    SIZE newsize;

    ShowScrollBar(hWnd, SB_VERT, FALSE);
    if (!WINHELP_SplitLines(hWnd, NULL))
    {
        ShowScrollBar(hWnd, SB_VERT, TRUE);
        GetClientRect(hWnd, &rect);

        WINHELP_SplitLines(hWnd, &newsize);
        SetScrollRange(hWnd, SB_VERT, 0, rect.top + newsize.cy - rect.bottom, TRUE);
    }
    else
    {
        SetScrollPos(hWnd, SB_VERT, 0, FALSE);
        SetScrollRange(hWnd, SB_VERT, 0, 0, FALSE);
    }

    ReleaseDC(hWnd, hDc);
}

/***********************************************************************
 *
 *           WINHELP_AppendText
 */
static BOOL WINHELP_AppendText(WINHELP_LINE ***linep, WINHELP_LINE_PART ***partp,
			       LPSIZE space, LPSIZE textsize,
			       INT *line_ascent, INT ascent,
			       LPCSTR text, UINT textlen,
			       HFONT font, COLORREF color, HLPFILE_LINK *link,
                               unsigned underline)
{
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    LPSTR ptr;

    if (!*partp) /* New line */
    {
        *line_ascent  = ascent;

        line = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(WINHELP_LINE) + textlen);
        if (!line) return FALSE;

        line->next    = 0;
        part          = &line->first_part;
        ptr           = (char*)line + sizeof(WINHELP_LINE);

        line->rect.top    = (**linep ? (**linep)->rect.bottom : 0) + space->cy;
        line->rect.bottom = line->rect.top;
        line->rect.left   = space->cx;
        line->rect.right  = space->cx;

        if (**linep) *linep = &(**linep)->next;
        **linep = line;
        space->cy = 0;
    }
    else /* Same line */
    {
        line = **linep;

        if (*line_ascent < ascent)
	{
            WINHELP_LINE_PART *p;
            for (p = &line->first_part; p; p = p->next)
	    {
                p->rect.top    += ascent - *line_ascent;
                p->rect.bottom += ascent - *line_ascent;
	    }
            line->rect.bottom += ascent - *line_ascent;
            *line_ascent = ascent;
	}

        part = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(WINHELP_LINE_PART) + textlen);
        if (!part) return FALSE;
        **partp = part;
        ptr     = (char*)part + sizeof(WINHELP_LINE_PART);
    }

    memcpy(ptr, text, textlen);
    part->cookie            = hlp_line_part_text;
    part->rect.left         = line->rect.right + (*partp ? space->cx : 0);
    part->rect.right        = part->rect.left + textsize->cx;
    line->rect.right        = part->rect.right;
    part->rect.top          =
        ((*partp) ? line->rect.top : line->rect.bottom) + *line_ascent - ascent;
    part->rect.bottom       = part->rect.top + textsize->cy;
    line->rect.bottom       = max(line->rect.bottom, part->rect.bottom);
    part->u.text.lpsText    = ptr;
    part->u.text.wTextLen   = textlen;
    part->u.text.hFont      = font;
    part->u.text.color      = color;
    part->u.text.wUnderline = underline;

    WINE_TRACE("Appended text '%*.*s'[%d] @ (%d,%d-%d,%d)\n",
               part->u.text.wTextLen,
               part->u.text.wTextLen,
               part->u.text.lpsText,
               part->u.text.wTextLen,
               part->rect.left, part->rect.top, part->rect.right, part->rect.bottom);

    part->link = link;
    if (link) link->wRefCount++;

    part->next          = 0;
    *partp              = &part->next;

    space->cx = 0;

    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_AppendGfxObject
 */
static WINHELP_LINE_PART* WINHELP_AppendGfxObject(WINHELP_LINE ***linep, WINHELP_LINE_PART ***partp,
                                                  LPSIZE space, LPSIZE gfxSize,
                                                  HLPFILE_LINK *link, unsigned pos)
{
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    LPSTR              ptr;

    if (!*partp || pos == 1) /* New line */
    {
        line = HeapAlloc(GetProcessHeap(), 0, sizeof(WINHELP_LINE));
        if (!line) return NULL;

        line->next    = NULL;
        part          = &line->first_part;

        line->rect.top    = (**linep ? (**linep)->rect.bottom : 0) + space->cy;
        line->rect.bottom = line->rect.top;
        line->rect.left   = space->cx;
        line->rect.right  = space->cx;

        if (**linep) *linep = &(**linep)->next;
        **linep = line;
        space->cy = 0;
        ptr = (char*)line + sizeof(WINHELP_LINE);
    }
    else /* Same line */
    {
        if (pos == 2) WINE_FIXME("Left alignment not handled\n");
        line = **linep;

        part = HeapAlloc(GetProcessHeap(), 0, sizeof(WINHELP_LINE_PART));
        if (!part) return NULL;
        **partp = part;
        ptr = (char*)part + sizeof(WINHELP_LINE_PART);
    }

    /* part->cookie should be set by caller (image or metafile) */
    part->rect.left       = line->rect.right + (*partp ? space->cx : 0);
    part->rect.right      = part->rect.left + gfxSize->cx;
    line->rect.right      = part->rect.right;
    part->rect.top        = (*partp) ? line->rect.top : line->rect.bottom;
    part->rect.bottom     = part->rect.top + gfxSize->cy;
    line->rect.bottom     = max(line->rect.bottom, part->rect.bottom);

    WINE_TRACE("Appended gfx @ (%d,%d-%d,%d)\n",
               part->rect.left, part->rect.top, part->rect.right, part->rect.bottom);

    part->link = link;
    if (link) link->wRefCount++;

    part->next            = NULL;
    *partp                = &part->next;

    space->cx = 0;

    return part;
}


/***********************************************************************
 *
 *           WINHELP_SplitLines
 */
static BOOL WINHELP_SplitLines(HWND hWnd, LPSIZE newsize)
{
    WINHELP_WINDOW     *win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
    HLPFILE_PARAGRAPH  *p;
    WINHELP_LINE      **line = &win->first_line;
    WINHELP_LINE_PART **part = 0;
    INT                 line_ascent = 0;
    SIZE                space;
    RECT                rect;
    HDC                 hDc;

    if (newsize) newsize->cx = newsize->cy = 0;

    if (!win->page) return TRUE;

    WINHELP_DeleteLines(win);

    GetClientRect(hWnd, &rect);

    rect.top    += INTERNAL_BORDER_WIDTH;
    rect.left   += INTERNAL_BORDER_WIDTH;
    rect.right  -= INTERNAL_BORDER_WIDTH;
    rect.bottom -= INTERNAL_BORDER_WIDTH;

    space.cy = rect.top;
    space.cx = rect.left;

    hDc = GetDC(hWnd);

    for (p = win->page->first_paragraph; p; p = p->next)
    {
        switch (p->cookie)
        {
        case para_normal_text:
        case para_debug_text:
            {
                TEXTMETRIC tm;
                SIZE textsize    = {0, 0};
                LPCSTR text      = p->u.text.lpszText;
                UINT indent      = 0;
                UINT len         = strlen(text);
                unsigned underline = 0;

                HFONT hFont = 0;
                COLORREF color = RGB(0, 0, 0);

                if (p->u.text.wFont < win->page->file->numFonts)
                {
                    HLPFILE*    hlpfile = win->page->file;

                    if (!hlpfile->fonts[p->u.text.wFont].hFont)
                        hlpfile->fonts[p->u.text.wFont].hFont = CreateFontIndirect(&hlpfile->fonts[p->u.text.wFont].LogFont);
                    hFont = hlpfile->fonts[p->u.text.wFont].hFont;
                    color = hlpfile->fonts[p->u.text.wFont].color;
                }
                else
                {
                    UINT  wFont = (p->u.text.wFont < win->fonts_len) ? p->u.text.wFont : 0;

                    hFont = win->fonts[wFont];
                }

                if (p->link && p->link->bClrChange)
                {
                    underline = (p->link->cookie == hlp_link_popup) ? 3 : 1;
                    color = RGB(0, 0x80, 0);
                }
                if (p->cookie == para_debug_text) color = RGB(0xff, 0, 0);

                SelectObject(hDc, hFont);

                GetTextMetrics(hDc, &tm);

                if (p->u.text.wIndent)
                {
                    indent = p->u.text.wIndent * 5 * tm.tmAveCharWidth;
                    if (!part)
                        space.cx = rect.left + indent - 2 * tm.tmAveCharWidth;
                }

                if (p->u.text.wVSpace)
                {
                    part = 0;
                    space.cx = rect.left + indent;
                    space.cy += (p->u.text.wVSpace - 1) * tm.tmHeight;
                }

                if (p->u.text.wHSpace)
                {
                    space.cx += p->u.text.wHSpace * 2 * tm.tmAveCharWidth;
                }

                WINE_TRACE("splitting text %s\n", text);

                while (len)
                {
                    INT free_width = rect.right - (part ? (*line)->rect.right : rect.left) - space.cx;
                    UINT low = 0, curr = len, high = len, textlen = 0;

                    if (free_width > 0)
                    {
                        while (1)
                        {
                            GetTextExtentPoint(hDc, text, curr, &textsize);

                            if (textsize.cx <= free_width) low = curr;
                            else high = curr;

                            if (high <= low + 1) break;

                            if (textsize.cx) curr = (curr * free_width) / textsize.cx;
                            if (curr <= low) curr = low + 1;
                            else if (curr >= high) curr = high - 1;
                        }
                        textlen = low;
                        while (textlen && text[textlen] && text[textlen] != ' ') textlen--;
                    }
                    if (!part && !textlen) textlen = max(low, 1);

                    if (free_width <= 0 || !textlen)
                    {
                        part = 0;
                        space.cx = rect.left + indent;
                        space.cx = min(space.cx, rect.right - rect.left - 1);
                        continue;
                    }

                    WINE_TRACE("\t => %d %*s\n", textlen, textlen, text);

                    if (!WINHELP_AppendText(&line, &part, &space, &textsize,
                                            &line_ascent, tm.tmAscent,
                                            text, textlen, hFont, color, p->link, underline) ||
                        (!newsize && (*line)->rect.bottom > rect.bottom))
                    {
                        ReleaseDC(hWnd, hDc);
                        return FALSE;
                    }

                    if (newsize)
                        newsize->cx = max(newsize->cx, (*line)->rect.right + INTERNAL_BORDER_WIDTH);

                    len -= textlen;
                    text += textlen;
                    if (text[0] == ' ') text++, len--;
                }
            }
            break;
        case para_bitmap:
        case para_metafile:
            {
                SIZE                    gfxSize;
                INT                     free_width;
                WINHELP_LINE_PART*      ref_part;

                if (p->u.gfx.pos & 0x8000)
                {
                    space.cx = rect.left;
                    if (*line)
                        space.cy += (*line)->rect.bottom - (*line)->rect.top;
                    part = 0;
                }

                if (p->cookie == para_bitmap)
                {
                    DIBSECTION              dibs;
                    
                    GetObject(p->u.gfx.u.bmp.hBitmap, sizeof(dibs), &dibs);
                    gfxSize.cx = dibs.dsBm.bmWidth;
                    gfxSize.cy = dibs.dsBm.bmHeight;
                }
                else
                {
                    LPMETAFILEPICT lpmfp = &p->u.gfx.u.mfp;
                    if (lpmfp->mm == MM_ANISOTROPIC || lpmfp->mm == MM_ISOTROPIC)
                    {
                        gfxSize.cx = MulDiv(lpmfp->xExt, GetDeviceCaps(hDc, HORZRES),
                                            100*GetDeviceCaps(hDc, HORZSIZE));
                        gfxSize.cy = MulDiv(lpmfp->yExt, GetDeviceCaps(hDc, VERTRES),
                                            100*GetDeviceCaps(hDc, VERTSIZE));
                    }
                    else
                    {
                        gfxSize.cx = lpmfp->xExt;
                        gfxSize.cy = lpmfp->yExt;
                    }
                }

                free_width = rect.right - ((part && *line) ? (*line)->rect.right : rect.left) - space.cx;
                if (free_width <= 0)
                {
                    part = NULL;
                    space.cx = rect.left;
                    space.cx = min(space.cx, rect.right - rect.left - 1);
                }
                ref_part = WINHELP_AppendGfxObject(&line, &part, &space, &gfxSize,
                                                   p->link, p->u.gfx.pos);
                if (!ref_part || (!newsize && (*line)->rect.bottom > rect.bottom))
                {
                    return FALSE;
                }
                if (p->cookie == para_bitmap)
                {
                    ref_part->cookie = hlp_line_part_bitmap;
                    ref_part->u.bitmap.hBitmap = p->u.gfx.u.bmp.hBitmap;
                }
                else
                {
                    ref_part->cookie = hlp_line_part_metafile;
                    ref_part->u.metafile.hMetaFile = p->u.gfx.u.mfp.hMF;
                    ref_part->u.metafile.mm = p->u.gfx.u.mfp.mm;
                }
            }
            break;
        }
    }

    if (newsize)
        newsize->cy = (*line)->rect.bottom + INTERNAL_BORDER_WIDTH;

    ReleaseDC(hWnd, hDc);
    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_CheckPopup
 */
static void WINHELP_CheckPopup(UINT msg)
{
    if (!Globals.hPopupWnd) return;

    switch (msg)
    {
    case WM_COMMAND:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        DestroyWindow(Globals.hPopupWnd);
        Globals.hPopupWnd = 0;
    }
}

/***********************************************************************
 *
 *           WINHELP_DeleteLines
 */
static void WINHELP_DeleteLines(WINHELP_WINDOW *win)
{
    WINHELP_LINE      *line, *next_line;
    WINHELP_LINE_PART *part, *next_part;
    for (line = win->first_line; line; line = next_line)
    {
        next_line = line->next;
        for (part = &line->first_part; part; part = next_part)
	{
            next_part = part->next;
            HLPFILE_FreeLink(part->link);
            HeapFree(GetProcessHeap(), 0, part);
	}
    }
    win->first_line = 0;
}

/***********************************************************************
 *
 *           WINHELP_DeleteWindow
 */
static void WINHELP_DeleteWindow(WINHELP_WINDOW* win)
{
    WINHELP_WINDOW**    w;
    unsigned int        i;
    WINHELP_BUTTON*     b;
    WINHELP_BUTTON*     bp;

    for (w = &Globals.win_list; *w; w = &(*w)->next)
    {
        if (*w == win)
        {
            *w = win->next;
            break;
        }
    }

    if (Globals.active_win == win)
    {
        Globals.active_win = Globals.win_list;
        if (Globals.win_list)
            SetActiveWindow(Globals.win_list->hMainWnd);
    }

    for (b = win->first_button; b; b = bp)
    {
        DestroyWindow(b->hWnd);
        bp = b->next;
        HeapFree(GetProcessHeap(), 0, b);
    }

    if (win->hShadowWnd) DestroyWindow(win->hShadowWnd);
    if (win->hHistoryWnd) DestroyWindow(win->hHistoryWnd);

    for (i = 0; i < win->histIndex; i++)
    {
        HLPFILE_FreeHlpFile(win->history[i]->file);
    }

    for (i = 0; i < win->backIndex; i++)
        HLPFILE_FreeHlpFile(win->back[i]->file);

    if (win->page) HLPFILE_FreeHlpFile(win->page->file);
    WINHELP_DeleteLines(win);
    HeapFree(GetProcessHeap(), 0, win);
}

/***********************************************************************
 *
 *           WINHELP_InitFonts
 */
static void WINHELP_InitFonts(HWND hWnd)
{
    WINHELP_WINDOW *win = (WINHELP_WINDOW*) GetWindowLong(hWnd, 0);
    LOGFONT logfontlist[] = {
        {-10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        {-10, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"},
        { -8, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 32, "Helv"}};
#define FONTS_LEN (sizeof(logfontlist)/sizeof(*logfontlist))

    static HFONT fonts[FONTS_LEN];
    static BOOL init = 0;

    win->fonts_len = FONTS_LEN;
    win->fonts = fonts;

    if (!init)
    {
        UINT i;

        for (i = 0; i < FONTS_LEN; i++)
	{
            fonts[i] = CreateFontIndirect(&logfontlist[i]);
	}

        init = 1;
    }
}

/***********************************************************************
 *
 *           WINHELP_MessageBoxIDS
 */
INT WINHELP_MessageBoxIDS(UINT ids_text, UINT ids_title, WORD type)
{
    CHAR text[MAX_STRING_LEN];
    CHAR title[MAX_STRING_LEN];

    LoadString(Globals.hInstance, ids_text, text, sizeof(text));
    LoadString(Globals.hInstance, ids_title, title, sizeof(title));

    return MessageBox(0, text, title, type);
}

/***********************************************************************
 *
 *           MAIN_MessageBoxIDS_s
 */
INT WINHELP_MessageBoxIDS_s(UINT ids_text, LPCSTR str, UINT ids_title, WORD type)
{
    CHAR text[MAX_STRING_LEN];
    CHAR title[MAX_STRING_LEN];
    CHAR newtext[MAX_STRING_LEN + MAX_PATH];

    LoadString(Globals.hInstance, ids_text, text, sizeof(text));
    LoadString(Globals.hInstance, ids_title, title, sizeof(title));
    wsprintf(newtext, text, str);

    return MessageBox(0, newtext, title, type);
}

/******************************************************************
 *		WINHELP_IsOverLink
 *
 *
 */
WINHELP_LINE_PART* WINHELP_IsOverLink(WINHELP_WINDOW* win, WPARAM wParam, LPARAM lParam)
{
    POINT mouse;
    WINHELP_LINE      *line;
    WINHELP_LINE_PART *part;
    int scroll_pos = GetScrollPos(win->hTextWnd, SB_VERT);

    mouse.x = LOWORD(lParam);
    mouse.y = HIWORD(lParam);
    for (line = win->first_line; line; line = line->next)
    {
        for (part = &line->first_part; part; part = part->next)
        {
            if (part->link && 
                part->link->lpszString &&
                part->rect.left   <= mouse.x &&
                part->rect.right  >= mouse.x &&
                part->rect.top    <= mouse.y + scroll_pos &&
                part->rect.bottom >= mouse.y + scroll_pos)
            {
                return part;
            }
        }
    }

    return NULL;
}
