/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid <uschmid@mail.hh.provi.de>
 *              2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              2002, 2008 Eric Pouech <eric.pouech@wanadoo.fr>
 *              2004 Ken Belleau <jamez@ivic.qc.ca>
 *              2008 Kirill K. Smirnov <lich@math.spbu.ru>
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

#include "winhelp.h"

#include <richedit.h>
#include <commctrl.h>

WINHELP_GLOBALS Globals = {3, NULL, TRUE, NULL, NULL, NULL, NULL, NULL, {{{NULL,NULL}},0}, NULL};

#define CTL_ID_BUTTON   0x700
#define CTL_ID_TEXT     0x701


/***********************************************************************
 *
 *           WINHELP_InitFonts
 */
static void WINHELP_InitFonts(HWND hWnd)
{
    WINHELP_WINDOW *win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
    LOGFONTW logfontlist[] = {
        {-10, 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}},
        {-12, 0, 0, 0, 700, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}},
        {-12, 0, 0, 0, 700, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}},
        {-12, 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}},
        {-12, 0, 0, 0, 700, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}},
        {-10, 0, 0, 0, 700, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}},
        { -8, 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 32, {'H','e','l','v',0}}};
#define FONTS_LEN (sizeof(logfontlist)/sizeof(*logfontlist))

    static HFONT fonts[FONTS_LEN];
    static BOOL init = FALSE;

    win->fonts_len = FONTS_LEN;
    win->fonts = fonts;

    if (!init)
    {
        UINT i;

        for (i = 0; i < FONTS_LEN; i++)
	{
            fonts[i] = CreateFontIndirectW(&logfontlist[i]);
	}

        init = TRUE;
    }
}

static DWORD CALLBACK WINHELP_RtfStreamIn(DWORD_PTR cookie, BYTE* buff,
                                          LONG cb, LONG* pcb)
{
    struct RtfData*     rd = (struct RtfData*)cookie;

    if (rd->where >= rd->ptr) return 1;
    if (rd->where + cb > rd->ptr)
        cb = rd->ptr - rd->where;
    memcpy(buff, rd->where, cb);
    rd->where += cb;
    *pcb = cb;
    return 0;
}

static void WINHELP_SetupText(HWND hTextWnd, WINHELP_WINDOW* win, ULONG relative)
{
    static const WCHAR emptyW[1];
    /* At first clear area - needed by EM_POSFROMCHAR/EM_SETSCROLLPOS */
    SendMessageW(hTextWnd, WM_SETTEXT, 0, (LPARAM)emptyW);
    SendMessageW(hTextWnd, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hTextWnd, EM_SETBKGNDCOLOR, 0, (LPARAM)win->info->sr_color);
    /* set word-wrap to window size (undocumented) */
    SendMessageW(hTextWnd, EM_SETTARGETDEVICE, 0, 0);
    if (win->page)
    {
        struct RtfData  rd;
        EDITSTREAM      es;
        unsigned        cp = 0;
        POINTL          ptl;
        POINT           pt;


        if (HLPFILE_BrowsePage(win->page, &rd, win->font_scale, relative))
        {
            rd.where = rd.data;
            es.dwCookie = (DWORD_PTR)&rd;
            es.dwError = 0;
            es.pfnCallback = WINHELP_RtfStreamIn;

            SendMessageW(hTextWnd, EM_STREAMIN, SF_RTF, (LPARAM)&es);
            cp = rd.char_pos_rel;
        }
        /* FIXME: else leaking potentially the rd.first_link chain */
        HeapFree(GetProcessHeap(), 0, rd.data);
        SendMessageW(hTextWnd, EM_POSFROMCHAR, (WPARAM)&ptl, cp ? cp - 1 : 0);
        pt.x = 0; pt.y = ptl.y;
        SendMessageW(hTextWnd, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
    }
    SendMessageW(hTextWnd, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hTextWnd, NULL, NULL, RDW_FRAME|RDW_INVALIDATE);
}

/***********************************************************************
 *
 *           WINHELP_GetOpenFileName
 */
BOOL WINHELP_GetOpenFileName(LPSTR lpszFile, int len)
{
    OPENFILENAMEA openfilename;
    CHAR szDir[MAX_PATH];
    CHAR szzFilter[2 * MAX_STRING_LEN + 100];
    LPSTR p = szzFilter;

    WINE_TRACE("()\n");

    LoadStringA(Globals.hInstance, STID_HELP_FILES_HLP, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    strcpy(p, "*.hlp");
    p += strlen(p) + 1;
    LoadStringA(Globals.hInstance, STID_ALL_FILES, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    strcpy(p, "*.*");
    p += strlen(p) + 1;
    *p = '\0';

    GetCurrentDirectoryA(sizeof(szDir), szDir);

    lpszFile[0]='\0';

    openfilename.lStructSize       = sizeof(openfilename);
    openfilename.hwndOwner         = (Globals.active_win ? Globals.active_win->hMainWnd : 0);
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
    openfilename.Flags             = OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_READONLY;
    openfilename.nFileOffset       = 0;
    openfilename.nFileExtension    = 0;
    openfilename.lpstrDefExt       = 0;
    openfilename.lCustData         = 0;
    openfilename.lpfnHook          = 0;
    openfilename.lpTemplateName    = 0;

    return GetOpenFileNameA(&openfilename);
}

/***********************************************************************
 *
 *           WINHELP_MessageBoxIDS_s
 */
static INT WINHELP_MessageBoxIDS_s(UINT ids_text, LPCSTR str, UINT ids_title, WORD type)
{
    CHAR text[MAX_STRING_LEN];
    CHAR newtext[MAX_STRING_LEN + MAX_PATH];

    LoadStringA(Globals.hInstance, ids_text, text, sizeof(text));
    wsprintfA(newtext, text, str);

    return MessageBoxA(0, newtext, MAKEINTRESOURCEA(ids_title), type);
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
    if (!SearchPathA(NULL, lpszFile, ".hlp", MAX_PATH, szFullName, NULL) &&
        !SearchPathA(szAddPath, lpszFile, ".hlp", MAX_PATH, szFullName, NULL))
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
        name = Globals.active_win->info->name;

    if (hlpfile)
        for (i = 0; i < hlpfile->numWindows; i++)
            if (!lstrcmpiA(hlpfile->windows[i].name, name))
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
        if (hlpfile && hlpfile->lpszTitle[0])
        {
            char        tmp[128];
            LoadStringA(Globals.hInstance, STID_WINE_HELP, tmp, sizeof(tmp));
            snprintf(mwi.caption, sizeof(mwi.caption), "%s %s - %s",
                     hlpfile->lpszTitle, tmp, hlpfile->lpszPath);
        }
        else
            LoadStringA(Globals.hInstance, STID_WINE_HELP, mwi.caption, sizeof(mwi.caption));
        mwi.origin.x = mwi.origin.y = mwi.size.cx = mwi.size.cy = CW_USEDEFAULT;
        mwi.style = SW_SHOW;
        mwi.win_style = WS_OVERLAPPEDWINDOW;
        mwi.sr_color = mwi.nsr_color = 0xFFFFFF;
    }
    return &mwi;
}

/******************************************************************
 *		HLPFILE_GetPopupWindowInfo
 *
 *
 */
static HLPFILE_WINDOWINFO*     WINHELP_GetPopupWindowInfo(HLPFILE* hlpfile,
                                                          WINHELP_WINDOW* parent, LPARAM mouse)
{
    static      HLPFILE_WINDOWINFO      wi;

    RECT parent_rect;
    
    wi.type[0] = wi.name[0] = wi.caption[0] = '\0';

    /* Calculate horizontal size and position of a popup window */
    GetWindowRect(parent->hMainWnd, &parent_rect);
    wi.size.cx = (parent_rect.right  - parent_rect.left) / 2;
    wi.size.cy = 10; /* need a non null value, so that borders are taken into account while computing */

    wi.origin.x = (short)LOWORD(mouse);
    wi.origin.y = (short)HIWORD(mouse);
    ClientToScreen(parent->hMainWnd, &wi.origin);
    wi.origin.x -= wi.size.cx / 2;
    wi.origin.x  = min(wi.origin.x, GetSystemMetrics(SM_CXSCREEN) - wi.size.cx);
    wi.origin.x  = max(wi.origin.x, 0);

    wi.style = SW_SHOW;
    wi.win_style = WS_POPUP | WS_BORDER;
    if (parent->page->file->has_popup_color)
        wi.sr_color = parent->page->file->popup_color;
    else
        wi.sr_color = parent->info->sr_color;
    wi.nsr_color = 0xFFFFFF;

    return &wi;
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

static BOOL WINHELP_HasWorkingWindow(void)
{
    if (!Globals.active_win) return FALSE;
    if (Globals.active_win->next || Globals.win_list != Globals.active_win) return TRUE;
    return Globals.active_win->page != NULL && Globals.active_win->page->file != NULL;
}

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

    wh = cds->lpData;

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
            if (!WINHELP_HasWorkingWindow()) MACRO_Exit();
            break;
        case HELP_QUIT:
            MACRO_Exit();
            break;
        case HELP_CONTENTS:
            if (ptr)
            {
                MACRO_JumpContents(ptr, "main");
            }
            if (!WINHELP_HasWorkingWindow()) MACRO_Exit();
            break;
        case HELP_HELPONHELP:
            MACRO_HelpOn();
            if (!WINHELP_HasWorkingWindow()) MACRO_Exit();
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
        default:
            WINE_FIXME("Unhandled command (%x) for remote winhelp control\n", wh->command);
            break;
        }
    }
    /* Always return success for now */
    return 1;
}

void            WINHELP_LayoutMainWindow(WINHELP_WINDOW* win)
{
    RECT        rect, button_box_rect;
    INT         text_top = 0;
    HWND        hButtonBoxWnd = GetDlgItem(win->hMainWnd, CTL_ID_BUTTON);
    HWND        hTextWnd = GetDlgItem(win->hMainWnd, CTL_ID_TEXT);

    GetClientRect(win->hMainWnd, &rect);

    /* Update button box and text Window */
    SetWindowPos(hButtonBoxWnd, HWND_TOP,
                 rect.left, rect.top,
                 rect.right - rect.left,
                 rect.bottom - rect.top, 0);

    if (GetWindowRect(hButtonBoxWnd, &button_box_rect))
        text_top = rect.top + button_box_rect.bottom - button_box_rect.top;

    SetWindowPos(hTextWnd, HWND_TOP,
                 rect.left, text_top,
                 rect.right - rect.left,
                 rect.bottom - text_top, 0);

}

/******************************************************************
 *		WINHELP_DeleteButtons
 *
 */
static void WINHELP_DeleteButtons(WINHELP_WINDOW* win)
{
    WINHELP_BUTTON*     b;
    WINHELP_BUTTON*     bp;

    for (b = win->first_button; b; b = bp)
    {
        DestroyWindow(b->hWnd);
        bp = b->next;
        HeapFree(GetProcessHeap(), 0, b);
    }
    win->first_button = NULL;
}

/******************************************************************
 *		WINHELP_DeleteBackSet
 *
 */
void WINHELP_DeleteBackSet(WINHELP_WINDOW* win)
{
    unsigned int i;

    for (i = 0; i < win->back.index; i++)
    {
        HLPFILE_FreeHlpFile(win->back.set[i].page->file);
        win->back.set[i].page = NULL;
    }
    win->back.index = 0;
}

/******************************************************************
 *             WINHELP_DeletePageLinks
 *
 */
static void WINHELP_DeletePageLinks(HLPFILE_PAGE* page)
{
    HLPFILE_LINK*       curr;
    HLPFILE_LINK*       next;

    for (curr = page->first_link; curr; curr = next)
    {
        next = curr->next;
        HeapFree(GetProcessHeap(), 0, curr);
    }
}

/***********************************************************************
 *
 *           WINHELP_GrabWindow
 */
WINHELP_WINDOW* WINHELP_GrabWindow(WINHELP_WINDOW* win)
{
    WINE_TRACE("Grab %p#%d++\n", win, win->ref_count);
    win->ref_count++;
    return win;
}

/***********************************************************************
 *
 *           WINHELP_RelaseWindow
 */
BOOL WINHELP_ReleaseWindow(WINHELP_WINDOW* win)
{
    WINE_TRACE("Release %p#%d--\n", win, win->ref_count);

    if (!--win->ref_count)
    {
        DestroyWindow(win->hMainWnd);
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *
 *           WINHELP_DeleteWindow
 */
static void WINHELP_DeleteWindow(WINHELP_WINDOW* win)
{
    WINHELP_WINDOW**    w;
    BOOL bExit;
    HWND hTextWnd;

    for (w = &Globals.win_list; *w; w = &(*w)->next)
    {
        if (*w == win)
        {
            *w = win->next;
            break;
        }
    }
    bExit = (Globals.wVersion >= 4 && !lstrcmpiA(win->info->name, "main"));

    if (Globals.active_win == win)
    {
        Globals.active_win = Globals.win_list;
        if (Globals.win_list)
            SetActiveWindow(Globals.win_list->hMainWnd);
    }

    if (win == Globals.active_popup)
        Globals.active_popup = NULL;

    hTextWnd = GetDlgItem(win->hMainWnd, CTL_ID_TEXT);
    SetWindowLongPtrA(hTextWnd, GWLP_WNDPROC, (LONG_PTR)win->origRicheditWndProc);

    WINHELP_DeleteButtons(win);

    if (win->page) WINHELP_DeletePageLinks(win->page);
    if (win->hHistoryWnd) DestroyWindow(win->hHistoryWnd);

    DeleteObject(win->hBrush);

    WINHELP_DeleteBackSet(win);

    if (win->page) HLPFILE_FreeHlpFile(win->page->file);
    HeapFree(GetProcessHeap(), 0, win);

    if (bExit) MACRO_Exit();
    if (!Globals.win_list)
        PostQuitMessage(0);
}

static char* WINHELP_GetCaption(WINHELP_WNDPAGE* wpage)
{
    if (wpage->wininfo->caption[0]) return wpage->wininfo->caption;
    return wpage->page->file->lpszTitle;
}

static void WINHELP_RememberPage(WINHELP_WINDOW* win, WINHELP_WNDPAGE* wpage)
{
    unsigned        num;

    if (!Globals.history.index || Globals.history.set[0].page != wpage->page)
    {
        num = sizeof(Globals.history.set) / sizeof(Globals.history.set[0]);
        /* we're full, remove latest entry */
        if (Globals.history.index == num)
        {
            HLPFILE_FreeHlpFile(Globals.history.set[num - 1].page->file);
            Globals.history.index--;
        }
        memmove(&Globals.history.set[1], &Globals.history.set[0],
                Globals.history.index * sizeof(Globals.history.set[0]));
        Globals.history.set[0] = *wpage;
        Globals.history.index++;
        wpage->page->file->wRefCount++;
    }
    if (win->hHistoryWnd) InvalidateRect(win->hHistoryWnd, NULL, TRUE);

    num = sizeof(win->back.set) / sizeof(win->back.set[0]);
    if (win->back.index == num)
    {
        /* we're full, remove latest entry */
        HLPFILE_FreeHlpFile(win->back.set[0].page->file);
        memmove(&win->back.set[0], &win->back.set[1],
                (num - 1) * sizeof(win->back.set[0]));
        win->back.index--;
    }
    win->back.set[win->back.index++] = *wpage;
    wpage->page->file->wRefCount++;
}

/***********************************************************************
 *
 *           WINHELP_FindLink
 */
static HLPFILE_LINK* WINHELP_FindLink(WINHELP_WINDOW* win, LPARAM pos)
{
    HLPFILE_LINK*           link;
    POINTL                  mouse_ptl, char_ptl, char_next_ptl;
    DWORD                   cp;

    if (!win->page) return NULL;

    mouse_ptl.x = (short)LOWORD(pos);
    mouse_ptl.y = (short)HIWORD(pos);
    cp = SendMessageW(GetDlgItem(win->hMainWnd, CTL_ID_TEXT), EM_CHARFROMPOS,
                      0, (LPARAM)&mouse_ptl);

    for (link = win->page->first_link; link; link = link->next)
    {
        if (link->cpMin <= cp && cp <= link->cpMax)
        {
            /* check whether we're at end of line */
            SendMessageW(GetDlgItem(win->hMainWnd, CTL_ID_TEXT), EM_POSFROMCHAR,
                         (LPARAM)&char_ptl, cp);
            SendMessageW(GetDlgItem(win->hMainWnd, CTL_ID_TEXT), EM_POSFROMCHAR,
                         (LPARAM)&char_next_ptl, cp + 1);
            if (link->bHotSpot)
            {
                HLPFILE_HOTSPOTLINK*    hslink = (HLPFILE_HOTSPOTLINK*)link;
                if ((mouse_ptl.x < char_ptl.x + hslink->x) ||
                    (mouse_ptl.x >= char_ptl.x + hslink->x + hslink->width) ||
                    (mouse_ptl.y < char_ptl.y + hslink->y) ||
                    (mouse_ptl.y >= char_ptl.y + hslink->y + hslink->height))
                    continue;
                break;
            }
            if (char_next_ptl.y != char_ptl.y || mouse_ptl.x >= char_next_ptl.x)
                link = NULL;
            break;
        }
    }
    return link;
}

static LRESULT CALLBACK WINHELP_RicheditWndProc(HWND hWnd, UINT msg,
                                                WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW *win = (WINHELP_WINDOW*) GetWindowLongPtrW(GetParent(hWnd), 0);
    DWORD messagePos;
    POINT pt;
    switch(msg)
    {
        case WM_SETCURSOR:
            messagePos = GetMessagePos();
            pt.x = (short)LOWORD(messagePos);
            pt.y = (short)HIWORD(messagePos);
            ScreenToClient(hWnd, &pt);
            if (win->page && WINHELP_FindLink(win, MAKELPARAM(pt.x, pt.y)))
            {
                SetCursor(win->hHandCur);
                return 0;
            }
            /* fall through */
        default:
            return CallWindowProcA(win->origRicheditWndProc, hWnd, msg, wParam, lParam);
    }
}

/***********************************************************************
 *
 *           WINHELP_CreateHelpWindow
 */
BOOL WINHELP_CreateHelpWindow(WINHELP_WNDPAGE* wpage, int nCmdShow, BOOL remember)
{
    WINHELP_WINDOW*     win = NULL;
    BOOL                bPrimary, bPopup, bReUsed = FALSE;
    HICON               hIcon;
    HWND                hTextWnd = NULL;

    bPrimary = !lstrcmpiA(wpage->wininfo->name, "main");
    bPopup = !bPrimary && (wpage->wininfo->win_style & WS_POPUP);

    if (!bPopup)
    {
        for (win = Globals.win_list; win; win = win->next)
        {
            if (!lstrcmpiA(win->info->name, wpage->wininfo->name))
            {
                if (win->page == wpage->page && win->info == wpage->wininfo)
                {
                    /* see #22979, some hlp files have a macro (run at page opening), which
                     * jumps to the very same page
                     * Exit gracefully in that case
                     */
                    return TRUE;
                }
                WINHELP_DeleteButtons(win);
                bReUsed = TRUE;
                SetWindowTextA(win->hMainWnd, WINHELP_GetCaption(wpage));
                if (win->info != wpage->wininfo)
                {
                    POINT   pt = {0, 0};
                    SIZE    sz = {0, 0};
                    DWORD   flags = SWP_NOSIZE | SWP_NOMOVE;

                    if (wpage->wininfo->origin.x != CW_USEDEFAULT &&
                        wpage->wininfo->origin.y != CW_USEDEFAULT)
                    {
                        pt = wpage->wininfo->origin;
                        flags &= ~SWP_NOSIZE;
                    }
                    if (wpage->wininfo->size.cx != CW_USEDEFAULT &&
                        wpage->wininfo->size.cy != CW_USEDEFAULT)
                    {
                        sz = wpage->wininfo->size;
                        flags &= ~SWP_NOMOVE;
                    }
                    SetWindowPos(win->hMainWnd, HWND_TOP, pt.x, pt.y, sz.cx, sz.cy, flags);
                }

                if (wpage->page && win->page && wpage->page->file != win->page->file)
                    WINHELP_DeleteBackSet(win);
                WINHELP_InitFonts(win->hMainWnd);

                win->page = wpage->page;
                win->info = wpage->wininfo;
                hTextWnd = GetDlgItem(win->hMainWnd, CTL_ID_TEXT);
                WINHELP_SetupText(hTextWnd, win, wpage->relative);

                InvalidateRect(win->hMainWnd, NULL, TRUE);
                if (win->hHistoryWnd) InvalidateRect(win->hHistoryWnd, NULL, TRUE);

                break;
            }
        }
    }

    if (!win)
    {
        /* Initialize WINHELP_WINDOW struct */
        win = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINHELP_WINDOW));
        if (!win) return FALSE;
        win->next = Globals.win_list;
        Globals.win_list = win;

        win->hHandCur = LoadCursorW(0, (LPWSTR)IDC_HAND);
        win->back.index = 0;
        win->font_scale = 1;
        WINHELP_GrabWindow(win);
    }
    win->page = wpage->page;
    win->info = wpage->wininfo;
    WINHELP_GrabWindow(win);

    if (!bPopup && wpage->page && remember)
    {
        WINHELP_RememberPage(win, wpage);
    }

    if (bPopup)
        Globals.active_popup = win;
    else
        Globals.active_win = win;

    /* Initialize default pushbuttons */
    if (bPrimary && wpage->page)
    {
        CHAR    buffer[MAX_STRING_LEN];

        LoadStringA(Globals.hInstance, STID_CONTENTS, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_CONTENTS", buffer, "Contents()");
        LoadStringA(Globals.hInstance, STID_INDEX, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_INDEX", buffer, "Finder()");
        LoadStringA(Globals.hInstance, STID_BACK, buffer, sizeof(buffer));
        MACRO_CreateButton("BTN_BACK", buffer, "Back()");
        if (win->back.index <= 1) MACRO_DisableButton("BTN_BACK");
    }

    if (!bReUsed)
    {
        win->hMainWnd = CreateWindowExA((bPopup) ? WS_EX_TOOLWINDOW : 0, MAIN_WIN_CLASS_NAME,
                                       WINHELP_GetCaption(wpage),
                                       bPrimary ? WS_OVERLAPPEDWINDOW : wpage->wininfo->win_style,
                                       wpage->wininfo->origin.x, wpage->wininfo->origin.y,
                                       wpage->wininfo->size.cx, wpage->wininfo->size.cy,
                                       bPopup ? Globals.active_win->hMainWnd : NULL,
                                       bPrimary ? LoadMenuW(Globals.hInstance, MAKEINTRESOURCEW(MAIN_MENU)) : 0,
                                       Globals.hInstance, win);
        if (!bPopup)
            /* Create button box and text Window */
            CreateWindowA(BUTTON_BOX_WIN_CLASS_NAME, "", WS_CHILD | WS_VISIBLE,
                         0, 0, 0, 0, win->hMainWnd, (HMENU)CTL_ID_BUTTON, Globals.hInstance, NULL);

        hTextWnd = CreateWindowA(RICHEDIT_CLASS20A, NULL,
                                ES_MULTILINE | ES_READONLY | WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE,
                                0, 0, 0, 0, win->hMainWnd, (HMENU)CTL_ID_TEXT, Globals.hInstance, NULL);
        SendMessageW(hTextWnd, EM_SETEVENTMASK, 0,
                    SendMessageW(hTextWnd, EM_GETEVENTMASK, 0, 0) | ENM_MOUSEEVENTS);
        win->origRicheditWndProc = (WNDPROC)SetWindowLongPtrA(hTextWnd, GWLP_WNDPROC,
                                                             (LONG_PTR)WINHELP_RicheditWndProc);
    }

    hIcon = (wpage->page) ? wpage->page->file->hIcon : NULL;
    if (!hIcon) hIcon = LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDI_WINHELP), IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    SendMessageW(win->hMainWnd, WM_SETICON, ICON_SMALL, (DWORD_PTR)hIcon);

    /* Initialize file specific pushbuttons */
    if (!(wpage->wininfo->win_style & WS_POPUP) && wpage->page)
    {
        HLPFILE_MACRO  *macro;
        for (macro = wpage->page->file->first_macro; macro; macro = macro->next)
            MACRO_ExecuteMacro(win, macro->lpszMacro);

        for (macro = wpage->page->first_macro; macro; macro = macro->next)
            MACRO_ExecuteMacro(win, macro->lpszMacro);
    }
    /* See #17681, in some cases, the newly created window is closed by the macros it contains
     * (braindead), so deal with this case
     */
    for (win = Globals.win_list; win; win = win->next)
    {
        if (!lstrcmpiA(win->info->name, wpage->wininfo->name)) break;
    }
    if (!win || !WINHELP_ReleaseWindow(win)) return TRUE;

    if (bPopup)
    {
        DWORD   mask = SendMessageW(hTextWnd, EM_GETEVENTMASK, 0, 0);

        win->font_scale = Globals.active_win->font_scale;
        WINHELP_SetupText(hTextWnd, win, wpage->relative);

        /* we need the window to be shown for richedit to compute the size */
        ShowWindow(win->hMainWnd, nCmdShow);
        SendMessageW(hTextWnd, EM_SETEVENTMASK, 0, mask | ENM_REQUESTRESIZE);
        SendMessageW(hTextWnd, EM_REQUESTRESIZE, 0, 0);
        SendMessageW(hTextWnd, EM_SETEVENTMASK, 0, mask);
    }
    else
    {
        WINHELP_SetupText(hTextWnd, win, wpage->relative);
        WINHELP_LayoutMainWindow(win);
        ShowWindow(win->hMainWnd, nCmdShow);
    }

    return TRUE;
}

/******************************************************************
 *             WINHELP_OpenHelpWindow
 * Main function to search for a page and display it in a window
 */
BOOL WINHELP_OpenHelpWindow(HLPFILE_PAGE* (*lookup)(HLPFILE*, LONG, ULONG*),
                            HLPFILE* hlpfile, LONG val, HLPFILE_WINDOWINFO* wi,
                            int nCmdShow)
{
    WINHELP_WNDPAGE     wpage;

    wpage.page = lookup(hlpfile, val, &wpage.relative);
    if (wpage.page) wpage.page->file->wRefCount++;
    wpage.wininfo = wi;
    return WINHELP_CreateHelpWindow(&wpage, nCmdShow, TRUE);
}

/******************************************************************
 *             WINHELP_HandleTextMouse
 *
 */
static BOOL WINHELP_HandleTextMouse(WINHELP_WINDOW* win, UINT msg, LPARAM lParam)
{
    HLPFILE*                hlpfile;
    HLPFILE_LINK*           link;
    BOOL                    ret = FALSE;

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        if ((link = WINHELP_FindLink(win, lParam)))
        {
            HLPFILE_WINDOWINFO*     wi;

            switch (link->cookie)
            {
            case hlp_link_link:
                if ((hlpfile = WINHELP_LookupHelpFile(link->string)))
                {
                    if (link->window == -1)
                    {
                        wi = win->info;
                        if (wi->win_style & WS_POPUP) wi = Globals.active_win->info;
                    }
                    else if (link->window < hlpfile->numWindows)
                        wi = &hlpfile->windows[link->window];
                    else
                    {
                        WINE_WARN("link to window %d/%d\n", link->window, hlpfile->numWindows);
                        break;
                    }
                    WINHELP_OpenHelpWindow(HLPFILE_PageByHash, hlpfile, link->hash, wi, SW_NORMAL);
                }
                break;
            case hlp_link_popup:
                if ((hlpfile = WINHELP_LookupHelpFile(link->string)))
                    WINHELP_OpenHelpWindow(HLPFILE_PageByHash, hlpfile, link->hash,
                                           WINHELP_GetPopupWindowInfo(hlpfile, win, lParam),
                                           SW_NORMAL);
                break;
            case hlp_link_macro:
                MACRO_ExecuteMacro(win, link->string);
                break;
            default:
                WINE_FIXME("Unknown link cookie %d\n", link->cookie);
            }
            ret = TRUE;
        }
        break;
    }
    return ret;
}

/***********************************************************************
 *
 *           WINHELP_CheckPopup
 */
static BOOL WINHELP_CheckPopup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* lret)
{
    WINHELP_WINDOW*     popup;

    if (!Globals.active_popup) return FALSE;

    switch (msg)
    {
    case WM_NOTIFY:
        {
            MSGFILTER*  msgf = (MSGFILTER*)lParam;
            if (msgf->nmhdr.code == EN_MSGFILTER)
            {
                if (!WINHELP_CheckPopup(hWnd, msgf->msg, msgf->wParam, msgf->lParam, NULL))
                    return FALSE;
                if (lret) *lret = 1;
                return TRUE;
            }
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE || (HWND)lParam == Globals.active_win->hMainWnd ||
            (HWND)lParam == Globals.active_popup->hMainWnd ||
            GetWindow((HWND)lParam, GW_OWNER) == Globals.active_win->hMainWnd)
            break;
        /* fall through */
    case WM_LBUTTONDOWN:
        if (msg == WM_LBUTTONDOWN)
            WINHELP_HandleTextMouse(Globals.active_popup, msg, lParam);
        /* fall through */
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        popup = Globals.active_popup;
        Globals.active_popup = NULL;
        WINHELP_ReleaseWindow(popup);
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *
 *           WINHELP_ButtonWndProc
 */
static LRESULT CALLBACK WINHELP_ButtonWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (WINHELP_CheckPopup(hWnd, msg, wParam, lParam, NULL)) return 0;

    if (msg == WM_KEYDOWN)
    {
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_ESCAPE:
            return SendMessageA(GetParent(hWnd), msg, wParam, lParam);
        }
    }

    return CallWindowProcA(Globals.button_proc, hWnd, msg, wParam, lParam);
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

    if (WINHELP_CheckPopup(hWnd, msg, wParam, lParam, NULL)) return 0L;

    switch (msg)
    {
    case WM_WINDOWPOSCHANGING:
        winpos = (WINDOWPOS*) lParam;
        win = (WINHELP_WINDOW*) GetWindowLongPtrW(GetParent(hWnd), 0);

        /* Update buttons */
        button_size.cx = 0;
        button_size.cy = 0;
        for (button = win->first_button; button; button = button->next)
	{
            HDC  hDc;
            SIZE textsize;
            if (!button->hWnd)
            {
                button->hWnd = CreateWindowA(STRING_BUTTON, button->lpszName,
                                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                            0, 0, 0, 0,
                                            hWnd, (HMENU) button->wParam,
                                            Globals.hInstance, 0);
                if (button->hWnd)
                {
                    if (Globals.button_proc == NULL)
                    {
                        NONCLIENTMETRICSW ncm;
                        Globals.button_proc = (WNDPROC) GetWindowLongPtrA(button->hWnd, GWLP_WNDPROC);

                        ncm.cbSize = sizeof(NONCLIENTMETRICSW);
                        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,
                                              sizeof(NONCLIENTMETRICSW), &ncm, 0);
                        Globals.hButtonFont = CreateFontIndirectW(&ncm.lfMenuFont);
                    }
                    SetWindowLongPtrA(button->hWnd, GWLP_WNDPROC, (LONG_PTR) WINHELP_ButtonWndProc);
                    if (Globals.hButtonFont)
                        SendMessageW(button->hWnd, WM_SETFONT, (WPARAM)Globals.hButtonFont, TRUE);
                }
            }
            hDc = GetDC(button->hWnd);
            GetTextExtentPointA(hDc, button->lpszName, strlen(button->lpszName), &textsize);
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
        SendMessageW(GetParent(hWnd), msg, wParam, lParam);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_ESCAPE:
            return SendMessageA(GetParent(hWnd), msg, wParam, lParam);
        }
        break;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
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
    TEXTMETRICW         tm;
    unsigned int        i;
    RECT                r;

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*)((LPCREATESTRUCTA)lParam)->lpCreateParams;
        SetWindowLongPtrW(hWnd, 0, (ULONG_PTR)win);
        win->hHistoryWnd = hWnd;
        break;
    case WM_CREATE:
        hDc = GetDC(hWnd);
        GetTextMetricsW(hDc, &tm);
        GetWindowRect(hWnd, &r);

        r.right = r.left + 30 * tm.tmAveCharWidth;
        r.bottom = r.top + (sizeof(Globals.history.set) / sizeof(Globals.history.set[0])) * tm.tmHeight;
        AdjustWindowRect(&r, GetWindowLongW(hWnd, GWL_STYLE), FALSE);
        if (r.left < 0) {r.right -= r.left; r.left = 0;}
        if (r.top < 0) {r.bottom -= r.top; r.top = 0;}

        MoveWindow(hWnd, r.left, r.top, r.right, r.bottom, TRUE);
        ReleaseDC(hWnd, hDc);
        break;
    case WM_LBUTTONDOWN:
        hDc = GetDC(hWnd);
        GetTextMetricsW(hDc, &tm);
        i = HIWORD(lParam) / tm.tmHeight;
        if (i < Globals.history.index)
            WINHELP_CreateHelpWindow(&Globals.history.set[i], SW_SHOW, TRUE);
        ReleaseDC(hWnd, hDc);
        break;
    case WM_PAINT:
        hDc = BeginPaint(hWnd, &ps);
        GetTextMetricsW(hDc, &tm);

        for (i = 0; i < Globals.history.index; i++)
        {
            if (Globals.history.set[i].page->file == Globals.active_win->page->file)
            {
                TextOutA(hDc, 0, i * tm.tmHeight,
                        Globals.history.set[i].page->lpszTitle,
                        strlen(Globals.history.set[i].page->lpszTitle));
            }
            else
            {
                char        buffer[1024];
                const char* ptr1;
                const char* ptr2;
                unsigned    len;

                ptr1 = strrchr(Globals.history.set[i].page->file->lpszPath, '\\');
                if (!ptr1) ptr1 = Globals.history.set[i].page->file->lpszPath;
                else ptr1++;
                ptr2 = strrchr(ptr1, '.');
                len = ptr2 ? ptr2 - ptr1 : strlen(ptr1);
                if (len > sizeof(buffer)) len = sizeof(buffer);
                memcpy(buffer, ptr1, len);
                if (len < sizeof(buffer)) buffer[len++] = ':';
                lstrcpynA(&buffer[len], Globals.history.set[i].page->lpszTitle, sizeof(buffer) - len);
                TextOutA(hDc, 0, i * tm.tmHeight, buffer, strlen(buffer));
            }
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
        if (hWnd == win->hHistoryWnd)
            win->hHistoryWnd = 0;
        break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

/**************************************************************************
 * cb_KWBTree
 *
 * HLPFILE_BPTreeCallback enumeration function for '|KWBTREE' internal file.
 *
 */
static void cb_KWBTree(void *p, void **next, void *cookie)
{
    HWND hListWnd = cookie;
    int count;

    WINE_TRACE("Adding '%s' to search list\n", (char *)p);
    SendMessageA(hListWnd, LB_INSERTSTRING, -1, (LPARAM)p);
    count = SendMessageW(hListWnd, LB_GETCOUNT, 0, 0);
    SendMessageW(hListWnd, LB_SETITEMDATA, count-1, (LPARAM)p);
    *next = (char*)p + strlen((char*)p) + 7;
}

struct index_data
{
    HLPFILE*    hlpfile;
    BOOL        jump;
    ULONG       offset;
};

/**************************************************************************
 * WINHELP_IndexDlgProc
 *
 */
static INT_PTR CALLBACK WINHELP_IndexDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static struct index_data* id;
    int sel;

    switch (msg)
    {
    case WM_INITDIALOG:
        id = (struct index_data*)((PROPSHEETPAGEA*)lParam)->lParam;
        HLPFILE_BPTreeEnum(id->hlpfile->kwbtree, cb_KWBTree,
                           GetDlgItem(hWnd, IDC_INDEXLIST));
        id->jump = FALSE;
        id->offset = 1;
        return TRUE;
    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case LBN_DBLCLK:
            if (LOWORD(wParam) == IDC_INDEXLIST)
                SendMessageW(GetParent(hWnd), PSM_PRESSBUTTON, PSBTN_OK, 0);
            break;
        }
        break;
    case WM_NOTIFY:
	switch (((NMHDR*)lParam)->code)
	{
	case PSN_APPLY:
            sel = SendDlgItemMessageW(hWnd, IDC_INDEXLIST, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR)
            {
                BYTE *p;
                int count;

                p = (BYTE*)SendDlgItemMessageW(hWnd, IDC_INDEXLIST, LB_GETITEMDATA, sel, 0);
                count = *(short*)((char *)p + strlen((char *)p) + 1);
                if (count > 1)
                {
                    MessageBoxA(hWnd, "count > 1 not supported yet", "Error", MB_OK | MB_ICONSTOP);
                    SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, PSNRET_INVALID);
                    return TRUE;
                }
                id->offset = *(ULONG*)((char *)p + strlen((char *)p) + 3);
                id->offset = *(long*)(id->hlpfile->kwdata + id->offset + 9);
                if (id->offset == 0xFFFFFFFF)
                {
                    MessageBoxA(hWnd, "macro keywords not supported yet", "Error", MB_OK | MB_ICONSTOP);
                    SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, PSNRET_INVALID);
                    return TRUE;
                }
                id->jump = TRUE;
                SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

/**************************************************************************
 * WINHELP_SearchDlgProc
 *
 */
static INT_PTR CALLBACK WINHELP_SearchDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_NOTIFY:
	switch (((NMHDR*)lParam)->code)
	{
	case PSN_APPLY:
            SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        default:
            return FALSE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

/***********************************************************************
 *
 *           WINHELP_MainWndProc
 */
static LRESULT CALLBACK WINHELP_MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINHELP_WINDOW *win;
    WINHELP_BUTTON *button;
    HWND hTextWnd;
    LRESULT ret;

    if (WINHELP_CheckPopup(hWnd, msg, wParam, lParam, &ret)) return ret;

    switch (msg)
    {
    case WM_NCCREATE:
        win = (WINHELP_WINDOW*) ((LPCREATESTRUCTA) lParam)->lpCreateParams;
        SetWindowLongPtrW(hWnd, 0, (ULONG_PTR) win);
        if (!win->page && Globals.isBook)
            PostMessageW(hWnd, WM_COMMAND, MNID_FILE_OPEN, 0);
        win->hMainWnd = hWnd;
        break;

    case WM_WINDOWPOSCHANGED:
        WINHELP_LayoutMainWindow((WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0));
        break;

    case WM_COMMAND:
        win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
        switch (LOWORD(wParam))
	{
            /* Menu FILE */
	case MNID_FILE_OPEN:    MACRO_FileOpen();       break;
	case MNID_FILE_PRINT:	MACRO_Print();          break;
	case MNID_FILE_SETUP:	MACRO_PrinterSetup();   break;
	case MNID_FILE_EXIT:	MACRO_Exit();           break;

            /* Menu EDIT */
	case MNID_EDIT_COPYDLG:
            SendDlgItemMessageW(hWnd, CTL_ID_TEXT, WM_COPY, 0, 0);
            break;
	case MNID_EDIT_ANNOTATE:MACRO_Annotate();       break;

            /* Menu Bookmark */
	case MNID_BKMK_DEFINE:  MACRO_BookmarkDefine(); break;

            /* Menu Help */
	case MNID_HELP_HELPON:	MACRO_HelpOn();         break;
	case MNID_HELP_HELPTOP: MACRO_HelpOnTop();      break;
	case MNID_HELP_ABOUT:	MACRO_About();          break;

            /* Context help */
        case MNID_CTXT_ANNOTATE:MACRO_Annotate();       break;
        case MNID_CTXT_COPY:    MACRO_CopyDialog();     break;
        case MNID_CTXT_PRINT:   MACRO_Print();          break;
        case MNID_OPTS_HISTORY: MACRO_History();        break;
        case MNID_OPTS_FONTS_SMALL:
        case MNID_CTXT_FONTS_SMALL:
            win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
            if (win->font_scale != 0)
            {
                win->font_scale = 0;
                WINHELP_SetupText(GetDlgItem(hWnd, CTL_ID_TEXT), win, 0 /* FIXME */);
            }
            break;
        case MNID_OPTS_FONTS_NORMAL:
        case MNID_CTXT_FONTS_NORMAL:
            win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
            if (win->font_scale != 1)
            {
                win->font_scale = 1;
                WINHELP_SetupText(GetDlgItem(hWnd, CTL_ID_TEXT), win, 0 /* FIXME */);
            }
            break;
        case MNID_OPTS_FONTS_LARGE:
        case MNID_CTXT_FONTS_LARGE:
            win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
            if (win->font_scale != 2)
            {
                win->font_scale = 2;
                WINHELP_SetupText(GetDlgItem(hWnd, CTL_ID_TEXT), win, 0 /* FIXME */);
            }
            break;

	default:
            /* Buttons */
            for (button = win->first_button; button; button = button->next)
                if (wParam == button->wParam) break;
            if (button)
                MACRO_ExecuteMacro(win, button->lpszMacro);
            else if (!HIWORD(wParam))
                MessageBoxW(0, MAKEINTRESOURCEW(STID_NOT_IMPLEMENTED),
                            MAKEINTRESOURCEW(STID_WHERROR), MB_OK);
            break;
	}
        break;
/* EPP     case WM_DESTROY: */
/* EPP         if (Globals.hPopupWnd) DestroyWindow(Globals.hPopupWnd); */
/* EPP         break; */
    case WM_COPYDATA:
        return WINHELP_HandleCommand((HWND)wParam, lParam);

    case WM_CHAR:
        if (wParam == 3)
        {
            SendDlgItemMessageW(hWnd, CTL_ID_TEXT, WM_COPY, 0, 0);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
        hTextWnd = GetDlgItem(win->hMainWnd, CTL_ID_TEXT);

        switch (wParam)
        {
        case VK_UP:
            SendMessageW(hTextWnd, EM_SCROLL, SB_LINEUP, 0);
            return 0;
        case VK_DOWN:
            SendMessageW(hTextWnd, EM_SCROLL, SB_LINEDOWN, 0);
            return 0;
        case VK_PRIOR:
            SendMessageW(hTextWnd, EM_SCROLL, SB_PAGEUP, 0);
            return 0;
        case VK_NEXT:
            SendMessageW(hTextWnd, EM_SCROLL, SB_PAGEDOWN, 0);
            return 0;
        case VK_ESCAPE:
            MACRO_Exit();
            return 0;
        }
        break;

    case WM_NOTIFY:
        if (wParam == CTL_ID_TEXT)
        {
            RECT        rc;

            switch (((NMHDR*)lParam)->code)
            {
            case EN_MSGFILTER:
                {
                    const MSGFILTER*    msgf = (const MSGFILTER*)lParam;
                    switch (msgf->msg)
                    {
                    case WM_KEYUP:
                        if (msgf->wParam == VK_ESCAPE)
                            WINHELP_ReleaseWindow((WINHELP_WINDOW*)GetWindowLongPtrW(hWnd, 0));
                        break;
                    case WM_RBUTTONDOWN:
                    {
                        HMENU       hMenu;
                        POINT       pt;

                        win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
                        hMenu = LoadMenuW(Globals.hInstance, MAKEINTRESOURCEW(CONTEXT_MENU));
                        switch (win->font_scale)
                        {
                        case 0:
                            CheckMenuItem(hMenu, MNID_CTXT_FONTS_SMALL,
                                          MF_BYCOMMAND|MF_CHECKED);
                            break;
                        default:
                            WINE_FIXME("Unsupported %d\n", win->font_scale);
                            /* fall through */
                        case 1:
                            CheckMenuItem(hMenu, MNID_CTXT_FONTS_NORMAL,
                                          MF_BYCOMMAND|MF_CHECKED);
                            break;
                        case 2:
                            CheckMenuItem(hMenu, MNID_CTXT_FONTS_LARGE,
                                          MF_BYCOMMAND|MF_CHECKED);
                            break;
                        }
                        pt.x = (int)(short)LOWORD(msgf->lParam);
                        pt.y = (int)(short)HIWORD(msgf->lParam);
                        ClientToScreen(msgf->nmhdr.hwndFrom, &pt);
                        TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN|TPM_TOPALIGN,
                                       pt.x, pt.y, 0, hWnd, NULL);
                        DestroyMenu(hMenu);
                    }
                    break;
                    default:
                        return WINHELP_HandleTextMouse((WINHELP_WINDOW*)GetWindowLongPtrW(hWnd, 0),
                                                       msgf->msg, msgf->lParam);
                    }
                }
                break;

            case EN_REQUESTRESIZE:
                rc = ((REQRESIZE*)lParam)->rc;
                win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
                AdjustWindowRect(&rc, GetWindowLongW(win->hMainWnd, GWL_STYLE),
                                 FALSE);
                SetWindowPos(win->hMainWnd, HWND_TOP, 0, 0,
                             rc.right - rc.left, rc.bottom - rc.top,
                             SWP_NOMOVE | SWP_NOZORDER);
                WINHELP_LayoutMainWindow(win);
                break;
            }
        }
        break;

    case WM_INITMENUPOPUP:
        win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
        CheckMenuItem((HMENU)wParam, MNID_OPTS_FONTS_SMALL,
                      (win->font_scale == 0) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, MNID_OPTS_FONTS_NORMAL,
                      (win->font_scale == 1) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, MNID_OPTS_FONTS_LARGE,
                      (win->font_scale == 2) ? MF_CHECKED : MF_UNCHECKED);
        break;
    case WM_DESTROY:
        win = (WINHELP_WINDOW*) GetWindowLongPtrW(hWnd, 0);
        WINHELP_DeleteWindow(win);
        break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

/**************************************************************************
 * WINHELP_CreateIndexWindow
 *
 * Displays a dialog with keywords of current help file.
 *
 */
BOOL WINHELP_CreateIndexWindow(BOOL is_search)
{
    HPROPSHEETPAGE      psPage[3];
    PROPSHEETPAGEA      psp;
    PROPSHEETHEADERA    psHead;
    struct index_data   id;
    char                buf[256];

    if (Globals.active_win && Globals.active_win->page && Globals.active_win->page->file)
        id.hlpfile = Globals.active_win->page->file;
    else
        return FALSE;

    if (id.hlpfile->kwbtree == NULL)
    {
        WINE_TRACE("No index provided\n");
        return FALSE;
    }

    InitCommonControls();

    id.jump = FALSE;
    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = Globals.hInstance;

    psp.u.pszTemplate = MAKEINTRESOURCEA(IDD_INDEX);
    psp.lParam = (LPARAM)&id;
    psp.pfnDlgProc = WINHELP_IndexDlgProc;
    psPage[0] = CreatePropertySheetPageA(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCEA(IDD_SEARCH);
    psp.lParam = (LPARAM)&id;
    psp.pfnDlgProc = WINHELP_SearchDlgProc;
    psPage[1] = CreatePropertySheetPageA(&psp);

    memset(&psHead, 0, sizeof(psHead));
    psHead.dwSize = sizeof(psHead);

    LoadStringA(Globals.hInstance, STID_PSH_INDEX, buf, sizeof(buf));
    strcat(buf, Globals.active_win->info->caption);

    psHead.pszCaption = buf;
    psHead.nPages = 2;
    psHead.u2.nStartPage = is_search ? 1 : 0;
    psHead.hwndParent = Globals.active_win->hMainWnd;
    psHead.u3.phpage = psPage;
    psHead.dwFlags = PSH_NOAPPLYNOW;

    PropertySheetA(&psHead);
    if (id.jump)
    {
        WINE_TRACE("got %d as an offset\n", id.offset);
        WINHELP_OpenHelpWindow(HLPFILE_PageByOffset, id.hlpfile, id.offset,
                               Globals.active_win->info, SW_NORMAL);
    }
    return TRUE;
}

/***********************************************************************
 *
 *           RegisterWinClasses
 */
static BOOL WINHELP_RegisterWinClasses(void)
{
    WNDCLASSEXA class_main, class_button_box, class_history;

    class_main.cbSize              = sizeof(class_main);
    class_main.style               = CS_HREDRAW | CS_VREDRAW;
    class_main.lpfnWndProc         = WINHELP_MainWndProc;
    class_main.cbClsExtra          = 0;
    class_main.cbWndExtra          = sizeof(WINHELP_WINDOW *);
    class_main.hInstance           = Globals.hInstance;
    class_main.hIcon               = LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(IDI_WINHELP));
    class_main.hCursor             = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    class_main.hbrBackground       = (HBRUSH)(COLOR_WINDOW+1);
    class_main.lpszMenuName        = 0;
    class_main.lpszClassName       = MAIN_WIN_CLASS_NAME;
    class_main.hIconSm             = LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDI_WINHELP), IMAGE_ICON,
                                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                               LR_SHARED);

    class_button_box               = class_main;
    class_button_box.lpfnWndProc   = WINHELP_ButtonBoxWndProc;
    class_button_box.cbWndExtra    = 0;
    class_button_box.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    class_button_box.lpszClassName = BUTTON_BOX_WIN_CLASS_NAME;

    class_history                  = class_main;
    class_history.lpfnWndProc      = WINHELP_HistoryWndProc;
    class_history.lpszClassName    = HISTORY_WIN_CLASS_NAME;

    return (RegisterClassExA(&class_main) &&
            RegisterClassExA(&class_button_box) &&
            RegisterClassExA(&class_history));
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
    HACCEL              hAccel;

    Globals.hInstance = hInstance;

    if (LoadLibraryA("riched20.dll") == NULL)
        return MessageBoxW(0, MAKEINTRESOURCEW(STID_NO_RICHEDIT),
                           MAKEINTRESOURCEW(STID_WHERROR), MB_OK);

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
    WINHELP_OpenHelpWindow(HLPFILE_PageByHash, hlpfile, lHash,
                           WINHELP_GetWindowInfo(hlpfile, wndname), show);

    /* Message loop */
    hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(MAIN_ACCEL));
    while ((Globals.win_list || Globals.active_popup) && GetMessageW(&msg, 0, 0, 0))
    {
        HWND hWnd = Globals.active_win ? Globals.active_win->hMainWnd : NULL;
        if (!TranslateAcceleratorW(hWnd, hAccel, &msg))
	{
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    for (dll = Globals.dlls; dll; dll = dll->next)
    {
        if (dll->class & DC_INITTERM) dll->handler(DW_TERM, 0, 0);
    }
    return 0;
}
