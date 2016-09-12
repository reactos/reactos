/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002, 2008 Eric Pouech
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

#include <shellapi.h>

/**************************************************/
/*               Macro table                      */
/**************************************************/
struct MacroDesc {
    const char* name;
    const char* alias;
    BOOL        isBool;
    const char* arguments;
    void       *fn;
};

static struct MacroDesc*MACRO_Loaded /* = NULL */;
static unsigned         MACRO_NumLoaded /* = 0 */;

/*******      helper functions     *******/

static char* StrDup(const char* str)
{
    char* dst;
    dst=HeapAlloc(GetProcessHeap(),0,strlen(str)+1);
    strcpy(dst, str);
    return dst;
}

static WINHELP_BUTTON**        MACRO_LookupButton(WINHELP_WINDOW* win, LPCSTR name)
{
    WINHELP_BUTTON**    b;

    for (b = &win->first_button; *b; b = &(*b)->next)
        if (!lstrcmpiA(name, (*b)->lpszID)) break;
    return b;
}

/******* some forward declarations *******/
static void CALLBACK MACRO_JumpID(LPCSTR lpszPathWindow, LPCSTR topic_id);

/******* real macro implementation *******/

void CALLBACK MACRO_CreateButton(LPCSTR id, LPCSTR name, LPCSTR macro)
{
    WINHELP_WINDOW *win = MACRO_CurrentWindow();
    WINHELP_BUTTON *button, **b;
    LONG            size;
    LPSTR           ptr;

    WINE_TRACE("(%s, %s, %s)\n", debugstr_a(id), debugstr_a(name), debugstr_a(macro));

    size = sizeof(WINHELP_BUTTON) + strlen(id) + strlen(name) + strlen(macro) + 3;

    button = HeapAlloc(GetProcessHeap(), 0, size);
    if (!button) return;

    button->next  = 0;
    button->hWnd  = 0;

    ptr = (char*)button + sizeof(WINHELP_BUTTON);

    strcpy(ptr, id);
    button->lpszID = ptr;
    ptr += strlen(id) + 1;

    strcpy(ptr, name);
    button->lpszName = ptr;
    ptr += strlen(name) + 1;

    strcpy(ptr, macro);
    button->lpszMacro = ptr;

    button->wParam = WH_FIRST_BUTTON;
    for (b = &win->first_button; *b; b = &(*b)->next)
        button->wParam = max(button->wParam, (*b)->wParam + 1);
    *b = button;

    WINHELP_LayoutMainWindow(win);
}

static void CALLBACK MACRO_DestroyButton(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

void CALLBACK MACRO_DisableButton(LPCSTR id)
{
    WINHELP_BUTTON**    b;

    WINE_TRACE("(%s)\n", debugstr_a(id));

    b = MACRO_LookupButton(MACRO_CurrentWindow(), id);
    if (!*b) {WINE_FIXME("Couldn't find button %s\n", debugstr_a(id)); return;}

    EnableWindow((*b)->hWnd, FALSE);
}

static void CALLBACK MACRO_EnableButton(LPCSTR id)
{
    WINHELP_BUTTON**    b;

    WINE_TRACE("(%s)\n", debugstr_a(id));

    b = MACRO_LookupButton(MACRO_CurrentWindow(), id);
    if (!*b) {WINE_FIXME("Couldn't find button %s\n", debugstr_a(id)); return;}

    EnableWindow((*b)->hWnd, TRUE);
}

void CALLBACK MACRO_JumpContents(LPCSTR lpszPath, LPCSTR lpszWindow)
{
    HLPFILE*    hlpfile;

    WINE_TRACE("(%s, %s)\n", debugstr_a(lpszPath), debugstr_a(lpszWindow));
    if ((hlpfile = WINHELP_LookupHelpFile(lpszPath)))
        WINHELP_OpenHelpWindow(HLPFILE_PageByHash, hlpfile, 0,
                               WINHELP_GetWindowInfo(hlpfile, lpszWindow),
                               SW_NORMAL);
}


void CALLBACK MACRO_About(void)
{
    WCHAR name[256];
    HICON icon = LoadImageW( Globals.hInstance, MAKEINTRESOURCEW(IDI_WINHELP),
                             IMAGE_ICON, 48, 48, LR_SHARED );
    LoadStringW( Globals.hInstance, STID_WINE_HELP, name, sizeof(name)/sizeof(WCHAR) );
    ShellAboutW( MACRO_CurrentWindow()->hMainWnd, name, NULL, icon );
}

static void CALLBACK MACRO_AddAccelerator(LONG u1, LONG u2, LPCSTR str)
{
    WINE_FIXME("(%u, %u, %s)\n", u1, u2, debugstr_a(str));
}

static void CALLBACK MACRO_ALink(LPCSTR str1, LONG u, LPCSTR str2)
{
    WINE_FIXME("(%s, %u, %s)\n", debugstr_a(str1), u, debugstr_a(str2));
}

void CALLBACK MACRO_Annotate(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_AppendItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4)
{
    WINE_FIXME("(%s, %s, %s, %s)\n", debugstr_a(str1), debugstr_a(str2), debugstr_a(str3), debugstr_a(str4));
}

static void CALLBACK MACRO_Back(void)
{
    WINHELP_WINDOW* win = MACRO_CurrentWindow();

    WINE_TRACE("()\n");

    if (win && win->back.index >= 2)
        WINHELP_CreateHelpWindow(&win->back.set[--win->back.index - 1], SW_SHOW, FALSE);
}

static void CALLBACK MACRO_BackFlush(void)
{
    WINHELP_WINDOW* win = MACRO_CurrentWindow();

    WINE_TRACE("()\n");

    if (win) WINHELP_DeleteBackSet(win);
}

void CALLBACK MACRO_BookmarkDefine(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_BookmarkMore(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_BrowseButtons(void)
{
    HLPFILE_PAGE*       page = MACRO_CurrentWindow()->page;
    ULONG               relative;

    WINE_TRACE("()\n");

    MACRO_CreateButton("BTN_PREV", "&<<", "Prev()");
    MACRO_CreateButton("BTN_NEXT", "&>>", "Next()");

    if (!HLPFILE_PageByOffset(page->file, page->browse_bwd, &relative))
        MACRO_DisableButton("BTN_PREV");
    if (!HLPFILE_PageByOffset(page->file, page->browse_fwd, &relative))
        MACRO_DisableButton("BTN_NEXT");
}

static void CALLBACK MACRO_ChangeButtonBinding(LPCSTR id, LPCSTR macro)
{
    WINHELP_WINDOW*     win = MACRO_CurrentWindow();
    WINHELP_BUTTON*     button;
    WINHELP_BUTTON**    b;
    LONG                size;
    LPSTR               ptr;

    WINE_TRACE("(%s, %s)\n", debugstr_a(id), debugstr_a(macro));

    b = MACRO_LookupButton(win, id);
    if (!*b) {WINE_FIXME("Couldn't find button %s\n", debugstr_a(id)); return;}

    size = sizeof(WINHELP_BUTTON) + strlen(id) +
        strlen((*b)->lpszName) + strlen(macro) + 3;

    button = HeapAlloc(GetProcessHeap(), 0, size);
    if (!button) return;

    button->next  = (*b)->next;
    button->hWnd  = (*b)->hWnd;
    button->wParam = (*b)->wParam;

    ptr = (char*)button + sizeof(WINHELP_BUTTON);

    strcpy(ptr, id);
    button->lpszID = ptr;
    ptr += strlen(id) + 1;

    strcpy(ptr, (*b)->lpszName);
    button->lpszName = ptr;
    ptr += strlen((*b)->lpszName) + 1;

    strcpy(ptr, macro);
    button->lpszMacro = ptr;

    *b = button;

    WINHELP_LayoutMainWindow(win);
}

static void CALLBACK MACRO_ChangeEnable(LPCSTR id, LPCSTR macro)
{
    WINE_TRACE("(%s, %s)\n", debugstr_a(id), debugstr_a(macro));

    MACRO_ChangeButtonBinding(id, macro);
    MACRO_EnableButton(id);
}

static void CALLBACK MACRO_ChangeItemBinding(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("(%s, %s)\n", debugstr_a(str1), debugstr_a(str2));
}

static void CALLBACK MACRO_CheckItem(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_CloseSecondarys(void)
{
    WINHELP_WINDOW *win;
    WINHELP_WINDOW *next;

    WINE_TRACE("()\n");
    for (win = Globals.win_list; win; win = next)
    {
        next = win->next;
        if (lstrcmpiA(win->info->name, "main"))
            WINHELP_ReleaseWindow(win);
    }
}

static void CALLBACK MACRO_CloseWindow(LPCSTR lpszWindow)
{
    WINHELP_WINDOW *win;
    WINHELP_WINDOW *next;

    WINE_TRACE("(%s)\n", debugstr_a(lpszWindow));

    if (!lpszWindow || !lpszWindow[0]) lpszWindow = "main";

    for (win = Globals.win_list; win; win = next)
    {
        next = win->next;
        if (!lstrcmpiA(win->info->name, lpszWindow))
            WINHELP_ReleaseWindow(win);
    }
}

static void CALLBACK MACRO_Compare(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_Contents(void)
{
    HLPFILE_PAGE*       page = MACRO_CurrentWindow()->page;

    WINE_TRACE("()\n");

    if (page)
        MACRO_JumpContents(page->file->lpszPath, NULL);
}

static void CALLBACK MACRO_ControlPanel(LPCSTR str1, LPCSTR str2, LONG u)
{
    WINE_FIXME("(%s, %s, %u)\n", debugstr_a(str1), debugstr_a(str2), u);
}

void CALLBACK MACRO_CopyDialog(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_CopyTopic(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_DeleteItem(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_DeleteMark(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_DisableItem(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_EnableItem(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_EndMPrint(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_ExecFile(LPCSTR pgm, LPCSTR args, LONG cmd_show, LPCSTR topic)
{
    HINSTANCE ret;

    WINE_TRACE("(%s, %s, %u, %s)\n",
               debugstr_a(pgm), debugstr_a(args), cmd_show, debugstr_a(topic));

    ret = ShellExecuteA(Globals.active_win ? Globals.active_win->hMainWnd : NULL, "open",
                        pgm, args, ".", cmd_show);
    if ((DWORD_PTR)ret < 32)
    {
        WINE_WARN("Failed with %p\n", ret);
        if (topic) MACRO_JumpID(NULL, topic);
    }
}

static void CALLBACK MACRO_ExecProgram(LPCSTR str, LONG u)
{
    WINE_FIXME("(%s, %u)\n", debugstr_a(str), u);
}

void CALLBACK MACRO_Exit(void)
{
    WINE_TRACE("()\n");

    while (Globals.win_list)
        WINHELP_ReleaseWindow(Globals.win_list);
}

static void CALLBACK MACRO_ExtAbleItem(LPCSTR str, LONG u)
{
    WINE_FIXME("(%s, %u)\n", debugstr_a(str), u);
}

static void CALLBACK MACRO_ExtInsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u1, LONG u2)
{
    WINE_FIXME("(%s, %s, %s, %s, %u, %u)\n", debugstr_a(str1), debugstr_a(str2), debugstr_a(str3), debugstr_a(str4), u1, u2);
}

static void CALLBACK MACRO_ExtInsertMenu(LPCSTR str1, LPCSTR str2, LPCSTR str3, LONG u1, LONG u2)
{
    WINE_FIXME("(%s, %s, %s, %u, %u)\n", debugstr_a(str1), debugstr_a(str2), debugstr_a(str3), u1, u2);
}

static BOOL CALLBACK MACRO_FileExist(LPCSTR str)
{
    WINE_TRACE("(%s)\n", debugstr_a(str));
    return GetFileAttributesA(str) != INVALID_FILE_ATTRIBUTES;
}

void CALLBACK MACRO_FileOpen(void)
{
    char szFile[MAX_PATH];

    if (WINHELP_GetOpenFileName(szFile, MAX_PATH))
    {
        MACRO_JumpContents(szFile, "main");
    }
}

static void CALLBACK MACRO_Find(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_Finder(void)
{
    WINHELP_CreateIndexWindow(FALSE);
}

static void CALLBACK MACRO_FloatingMenu(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_Flush(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_FocusWindow(LPCSTR lpszWindow)
{
    WINHELP_WINDOW *win;

    WINE_TRACE("(%s)\n", debugstr_a(lpszWindow));

    if (!lpszWindow || !lpszWindow[0]) lpszWindow = "main";

    for (win = Globals.win_list; win; win = win->next)
        if (!lstrcmpiA(win->info->name, lpszWindow))
            SetFocus(win->hMainWnd);
}

static void CALLBACK MACRO_Generate(LPCSTR str, LONG w, LONG l)
{
    WINE_FIXME("(%s, %x, %x)\n", debugstr_a(str), w, l);
}

static void CALLBACK MACRO_GotoMark(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

void CALLBACK MACRO_HelpOn(void)
{
    WINHELP_WINDOW *win = MACRO_CurrentWindow();
    LPCSTR      file = NULL;

    WINE_TRACE("()\n");
    if (win && win->page && win->page->file)
        file = win->page->file->help_on_file;

    if (!file)
        file = (Globals.wVersion > 4) ? "winhlp32.hlp" : "winhelp.hlp";

    MACRO_JumpContents(file, NULL);
}

void CALLBACK MACRO_HelpOnTop(void)
{
    static BOOL on_top = FALSE;
    WINHELP_WINDOW *win;
    HWND main_wnd = NULL;
    HMENU menu;

    for (win = Globals.win_list; win; win = win->next)
        if (!lstrcmpiA(win->info->name, "main"))
            main_wnd = win->hMainWnd;
    if (!main_wnd)
    {
        WINE_ERR("could not find the main window!\n");
        return;
    }
    menu = GetMenu(main_wnd);

    on_top = !on_top;
    if (on_top) {
        CheckMenuItem(menu, MNID_HELP_HELPTOP, MF_BYCOMMAND|MF_CHECKED);
        SetWindowPos(main_wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    } else {
        CheckMenuItem(menu, MNID_HELP_HELPTOP, MF_BYCOMMAND|MF_UNCHECKED);
        SetWindowPos(main_wnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }
}

void CALLBACK MACRO_History(void)
{
    WINE_TRACE("()\n");

    if (Globals.active_win && !Globals.active_win->hHistoryWnd)
    {
        HWND hWnd = CreateWindowA(HISTORY_WIN_CLASS_NAME, "History", WS_OVERLAPPEDWINDOW,
                                 0, 0, 0, 0, 0, 0, Globals.hInstance, Globals.active_win);
        ShowWindow(hWnd, SW_NORMAL);
    }
}

static void CALLBACK MACRO_IfThen(BOOL b, LPCSTR t)
{
    if (b) MACRO_ExecuteMacro(MACRO_CurrentWindow(), t);
}

static void CALLBACK MACRO_IfThenElse(BOOL b, LPCSTR t, LPCSTR f)
{
    if (b) MACRO_ExecuteMacro(MACRO_CurrentWindow(), t);
    else MACRO_ExecuteMacro(MACRO_CurrentWindow(), f);
}

static BOOL CALLBACK MACRO_InitMPrint(void)
{
    WINE_FIXME("()\n");
    return FALSE;
}

static void CALLBACK MACRO_InsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u)
{
    WINE_FIXME("(%s, %s, %s, %s, %u)\n", debugstr_a(str1), debugstr_a(str2), debugstr_a(str3), debugstr_a(str4), u);
}

static void CALLBACK MACRO_InsertMenu(LPCSTR str1, LPCSTR str2, LONG u)
{
    WINE_FIXME("(%s, %s, %u)\n", debugstr_a(str1), debugstr_a(str2), u);
}

static BOOL CALLBACK MACRO_IsBook(void)
{
    WINE_TRACE("()\n");
    return Globals.isBook;
}

static BOOL CALLBACK MACRO_IsMark(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
    return FALSE;
}

static BOOL CALLBACK MACRO_IsNotMark(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
    return TRUE;
}

void CALLBACK MACRO_JumpContext(LPCSTR lpszPath, LPCSTR lpszWindow, LONG context)
{
    HLPFILE*    hlpfile;

    WINE_TRACE("(%s, %s, %d)\n", debugstr_a(lpszPath), debugstr_a(lpszWindow), context);
    if ((hlpfile = WINHELP_LookupHelpFile(lpszPath)))
        /* Some madness: what user calls 'context', hlpfile calls 'map' */
        WINHELP_OpenHelpWindow(HLPFILE_PageByMap, hlpfile, context,
                               WINHELP_GetWindowInfo(hlpfile, lpszWindow),
                               SW_NORMAL);
}

void CALLBACK MACRO_JumpHash(LPCSTR lpszPath, LPCSTR lpszWindow, LONG lHash)
{
    HLPFILE*    hlpfile;

    WINE_TRACE("(%s, %s, %u)\n", debugstr_a(lpszPath), debugstr_a(lpszWindow), lHash);
    if (!lpszPath || !lpszPath[0])
        hlpfile = MACRO_CurrentWindow()->page->file;
    else
        hlpfile = WINHELP_LookupHelpFile(lpszPath);
    if (hlpfile)
        WINHELP_OpenHelpWindow(HLPFILE_PageByHash, hlpfile, lHash,
                               WINHELP_GetWindowInfo(hlpfile, lpszWindow),
                               SW_NORMAL);
}

static void CALLBACK MACRO_JumpHelpOn(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_JumpID(LPCSTR lpszPathWindow, LPCSTR topic_id)
{
    LPSTR       ptr;

    WINE_TRACE("(%s, %s)\n", debugstr_a(lpszPathWindow), debugstr_a(topic_id));
    if (lpszPathWindow && (ptr = strchr(lpszPathWindow, '>')) != NULL)
    {
        LPSTR   tmp;
        size_t  sz;

        tmp = HeapAlloc(GetProcessHeap(), 0, strlen(lpszPathWindow) + 1);
        if (tmp)
        {
            strcpy(tmp, lpszPathWindow);
            tmp[ptr - lpszPathWindow] = '\0';
            ptr += tmp - lpszPathWindow; /* ptr now points to '>' in tmp buffer */
            /* in some cases, we have a trailing space that we need to get rid of */
            /* FIXME: check if it has to be done in lexer rather than here */
            for (sz = strlen(ptr + 1); sz >= 1 && ptr[sz] == ' '; sz--) ptr[sz] = '\0';
            MACRO_JumpHash(tmp, ptr + 1, HLPFILE_Hash(topic_id));
            HeapFree(GetProcessHeap(), 0, tmp);
        }
    }
    else
        MACRO_JumpHash(lpszPathWindow, NULL, HLPFILE_Hash(topic_id));
}

/* FIXME: this macros is wrong
 * it should only contain 2 strings, path & window are coded as path>window
 */
static void CALLBACK MACRO_JumpKeyword(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR keyword)
{
    WINE_FIXME("(%s, %s, %s)\n", debugstr_a(lpszPath), debugstr_a(lpszWindow), debugstr_a(keyword));
}

static void CALLBACK MACRO_KLink(LPCSTR str1, LONG u, LPCSTR str2, LPCSTR str3)
{
    WINE_FIXME("(%s, %u, %s, %s)\n", debugstr_a(str1), u, debugstr_a(str2), debugstr_a(str3));
}

static void CALLBACK MACRO_Menu(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_MPrintHash(LONG u)
{
    WINE_FIXME("(%u)\n", u);
}

static void CALLBACK MACRO_MPrintID(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_Next(void)
{
    WINHELP_WNDPAGE     wp;

    WINE_TRACE("()\n");
    wp.page = MACRO_CurrentWindow()->page;
    wp.page = HLPFILE_PageByOffset(wp.page->file, wp.page->browse_fwd, &wp.relative);
    if (wp.page)
    {
        wp.page->file->wRefCount++;
        wp.wininfo = MACRO_CurrentWindow()->info;
        WINHELP_CreateHelpWindow(&wp, SW_NORMAL, TRUE);
    }
}

static void CALLBACK MACRO_NoShow(void)
{
    WINE_FIXME("()\n");
}

void CALLBACK MACRO_PopupContext(LPCSTR str, LONG u)
{
    WINE_FIXME("(%s, %u)\n", debugstr_a(str), u);
}

static void CALLBACK MACRO_PopupHash(LPCSTR str, LONG u)
{
    WINE_FIXME("(%s, %u)\n", debugstr_a(str), u);
}

static void CALLBACK MACRO_PopupId(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("(%s, %s)\n", debugstr_a(str1), debugstr_a(str2));
}

static void CALLBACK MACRO_PositionWindow(LONG i1, LONG i2, LONG u1, LONG u2, LONG u3, LPCSTR str)
{
    WINE_FIXME("(%i, %i, %u, %u, %u, %s)\n", i1, i2, u1, u2, u3, debugstr_a(str));
}

static void CALLBACK MACRO_Prev(void)
{
    WINHELP_WNDPAGE     wp;

    WINE_TRACE("()\n");
    wp.page = MACRO_CurrentWindow()->page;
    wp.page = HLPFILE_PageByOffset(wp.page->file, wp.page->browse_bwd, &wp.relative);
    if (wp.page)
    {
        wp.page->file->wRefCount++;
        wp.wininfo = MACRO_CurrentWindow()->info;
        WINHELP_CreateHelpWindow(&wp, SW_NORMAL, TRUE);
    }
}

void CALLBACK MACRO_Print(void)
{
    PRINTDLGW printer;

    WINE_TRACE("()\n");

    printer.lStructSize         = sizeof(printer);
    printer.hwndOwner           = MACRO_CurrentWindow()->hMainWnd;
    printer.hInstance           = Globals.hInstance;
    printer.hDevMode            = 0;
    printer.hDevNames           = 0;
    printer.hDC                 = 0;
    printer.Flags               = 0;
    printer.nFromPage           = 0;
    printer.nToPage             = 0;
    printer.nMinPage            = 0;
    printer.nMaxPage            = 0;
    printer.nCopies             = 0;
    printer.lCustData           = 0;
    printer.lpfnPrintHook       = 0;
    printer.lpfnSetupHook       = 0;
    printer.lpPrintTemplateName = 0;
    printer.lpSetupTemplateName = 0;
    printer.hPrintTemplate      = 0;
    printer.hSetupTemplate      = 0;

    if (PrintDlgW(&printer)) {
        WINE_FIXME("Print()\n");
    }
}

void CALLBACK MACRO_PrinterSetup(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_RegisterRoutine(LPCSTR dll_name, LPCSTR proc, LPCSTR args)
{
    void               *fn = NULL;
    int                 size;
    WINHELP_DLL*        dll;

    WINE_TRACE("(%s, %s, %s)\n", debugstr_a(dll_name), debugstr_a(proc), debugstr_a(args));

    /* FIXME: are the registered DLLs global or linked to the current file ???
     * We assume globals (as we did for macros, but is this really the case ???)
     */
    for (dll = Globals.dlls; dll; dll = dll->next)
    {
        if (!strcmp(dll->name, dll_name)) break;
    }
    if (!dll)
    {
        HANDLE hLib = LoadLibraryA(dll_name);

        /* FIXME: the library will not be unloaded until exit of program 
         * We don't send the DW_TERM message
         */
        WINE_TRACE("Loading %s\n", debugstr_a(dll_name));
        /* FIXME: should look in the directory where current hlpfile
         * is loaded from
         */
        if (hLib == NULL)
        {
            /* FIXME: internationalisation for error messages */
            WINE_FIXME("Cannot find dll %s\n", debugstr_a(dll_name));
        }
        else if ((dll = HeapAlloc(GetProcessHeap(), 0, sizeof(*dll))))
        {
            dll->hLib = hLib;
            dll->name = StrDup(dll_name); /* FIXME: never freed */
            dll->next = Globals.dlls;
            Globals.dlls = dll;
            dll->handler = (WINHELP_LDLLHandler)GetProcAddress(dll->hLib, "LDLLHandler");
            dll->class = dll->handler ? (dll->handler)(DW_WHATMSG, 0, 0) : DC_NOMSG;
            WINE_TRACE("Got class %x for DLL %s\n", dll->class, debugstr_a(dll_name));
            if (dll->class & DC_INITTERM) dll->handler(DW_INIT, 0, 0);
            if (dll->class & DC_CALLBACKS) dll->handler(DW_CALLBACKS, (LONG_PTR)&Callbacks, 0);
        }
        else WINE_WARN("OOM\n");
    }
    if (dll && !(fn = GetProcAddress(dll->hLib, proc)))
    {
        /* FIXME: internationalisation for error messages */
        WINE_FIXME("Cannot find proc %s in dll %s\n", debugstr_a(dll_name), debugstr_a(proc));
    }

    size = ++MACRO_NumLoaded * sizeof(struct MacroDesc);
    if (!MACRO_Loaded) MACRO_Loaded = HeapAlloc(GetProcessHeap(), 0, size);
    else MACRO_Loaded = HeapReAlloc(GetProcessHeap(), 0, MACRO_Loaded, size);
    MACRO_Loaded[MACRO_NumLoaded - 1].name      = StrDup(proc); /* FIXME: never freed */
    MACRO_Loaded[MACRO_NumLoaded - 1].alias     = NULL;
    MACRO_Loaded[MACRO_NumLoaded - 1].isBool    = FALSE;
    MACRO_Loaded[MACRO_NumLoaded - 1].arguments = StrDup(args); /* FIXME: never freed */
    MACRO_Loaded[MACRO_NumLoaded - 1].fn        = fn;
    WINE_TRACE("Added %s(%s) at %p\n", debugstr_a(proc), debugstr_a(args), fn);
}

static void CALLBACK MACRO_RemoveAccelerator(LONG u1, LONG u2)
{
    WINE_FIXME("(%u, %u)\n", u1, u2);
}

static void CALLBACK MACRO_ResetMenu(void)
{
    WINE_FIXME("()\n");
}

static void CALLBACK MACRO_SaveMark(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_Search(void)
{
    WINHELP_CreateIndexWindow(TRUE);
}

void CALLBACK MACRO_SetContents(LPCSTR str, LONG u)
{
    WINE_FIXME("(%s, %u)\n", debugstr_a(str), u);
}

static void CALLBACK MACRO_SetHelpOnFile(LPCSTR str)
{
    HLPFILE_PAGE*       page = MACRO_CurrentWindow()->page;

    WINE_TRACE("(%s)\n", debugstr_a(str));

    HeapFree(GetProcessHeap(), 0, page->file->help_on_file);
    page->file->help_on_file = HeapAlloc(GetProcessHeap(), 0, strlen(str) + 1);
    if (page->file->help_on_file)
        strcpy(page->file->help_on_file, str);
}

static void CALLBACK MACRO_SetPopupColor(LONG r, LONG g, LONG b)
{
    HLPFILE_PAGE*       page = MACRO_CurrentWindow()->page;

    WINE_TRACE("(%x, %x, %x)\n", r, g, b);
    page->file->has_popup_color = TRUE;
    page->file->popup_color = RGB(r, g, b);
}

static void CALLBACK MACRO_ShellExecute(LPCSTR str1, LPCSTR str2, LONG u1, LONG u2, LPCSTR str3, LPCSTR str4)
{
    WINE_FIXME("(%s, %s, %u, %u, %s, %s)\n", debugstr_a(str1), debugstr_a(str2), u1, u2, debugstr_a(str3), debugstr_a(str4));
}

static void CALLBACK MACRO_ShortCut(LPCSTR str1, LPCSTR str2, LONG w, LONG l, LPCSTR str)
{
    WINE_FIXME("(%s, %s, %x, %x, %s)\n", debugstr_a(str1), debugstr_a(str2), w, l, debugstr_a(str));
}

static void CALLBACK MACRO_TCard(LONG u)
{
    WINE_FIXME("(%u)\n", u);
}

static void CALLBACK MACRO_Test(LONG u)
{
    WINE_FIXME("(%u)\n", u);
}

static BOOL CALLBACK MACRO_TestALink(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
    return FALSE;
}

static BOOL CALLBACK MACRO_TestKLink(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
    return FALSE;
}

static void CALLBACK MACRO_UncheckItem(LPCSTR str)
{
    WINE_FIXME("(%s)\n", debugstr_a(str));
}

static void CALLBACK MACRO_UpdateWindow(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("(%s, %s)\n", debugstr_a(str1), debugstr_a(str2));
}


/**************************************************/
/*               Macro table                      */
/**************************************************/

/* types:
 *      U:      32 bit unsigned int
 *      I:      32 bit signed int
 *      S:      string
 *      v:      unknown (32 bit entity)
 */

static struct MacroDesc MACRO_Builtins[] = {
    {"About",               NULL, 0, "",       MACRO_About},
    {"AddAccelerator",      "AA", 0, "UUS",    MACRO_AddAccelerator},
    {"ALink",               "AL", 0, "SUS",    MACRO_ALink},
    {"Annotate",            NULL, 0, "",       MACRO_Annotate},
    {"AppendItem",          NULL, 0, "SSSS",   MACRO_AppendItem},
    {"Back",                NULL, 0, "",       MACRO_Back},
    {"BackFlush",           "BF", 0, "",       MACRO_BackFlush},
    {"BookmarkDefine",      NULL, 0, "",       MACRO_BookmarkDefine},
    {"BookmarkMore",        NULL, 0, "",       MACRO_BookmarkMore},
    {"BrowseButtons",       NULL, 0, "",       MACRO_BrowseButtons},
    {"ChangeButtonBinding", "CBB",0, "SS",     MACRO_ChangeButtonBinding},
    {"ChangeEnable",        "CE", 0, "SS",     MACRO_ChangeEnable},
    {"ChangeItemBinding",   "CIB",0, "SS",     MACRO_ChangeItemBinding},
    {"CheckItem",           "CI", 0, "S",      MACRO_CheckItem},
    {"CloseSecondarys",     "CS", 0, "",       MACRO_CloseSecondarys},
    {"CloseWindow",         "CW", 0, "S",      MACRO_CloseWindow},
    {"Compare",             NULL, 0, "S",      MACRO_Compare},
    {"Contents",            NULL, 0, "",       MACRO_Contents},
    {"ControlPanel",        NULL, 0, "SSU",    MACRO_ControlPanel},
    {"CopyDialog",          NULL, 0, "",       MACRO_CopyDialog},
    {"CopyTopic",           "CT", 0, "",       MACRO_CopyTopic},
    {"CreateButton",        "CB", 0, "SSS",    MACRO_CreateButton},
    {"DeleteItem",          NULL, 0, "S",      MACRO_DeleteItem},
    {"DeleteMark",          NULL, 0, "S",      MACRO_DeleteMark},
    {"DestroyButton",       NULL, 0, "S",      MACRO_DestroyButton},
    {"DisableButton",       "DB", 0, "S",      MACRO_DisableButton},
    {"DisableItem",         "DI", 0, "S",      MACRO_DisableItem},
    {"EnableButton",        "EB", 0, "S",      MACRO_EnableButton},
    {"EnableItem",          "EI", 0, "S",      MACRO_EnableItem},
    {"EndMPrint",           NULL, 0, "",       MACRO_EndMPrint},
    {"ExecFile",            "EF", 0, "SSUS",   MACRO_ExecFile},
    {"ExecProgram",         "EP", 0, "SU",     MACRO_ExecProgram},
    {"Exit",                NULL, 0, "",       MACRO_Exit},
    {"ExtAbleItem",         NULL, 0, "SU",     MACRO_ExtAbleItem},
    {"ExtInsertItem",       NULL, 0, "SSSSUU", MACRO_ExtInsertItem},
    {"ExtInsertMenu",       NULL, 0, "SSSUU",  MACRO_ExtInsertMenu},
    {"FileExist",           "FE", 1, "S",      MACRO_FileExist},
    {"FileOpen",            "FO", 0, "",       MACRO_FileOpen},
    {"Find",                NULL, 0, "",       MACRO_Find},
    {"Finder",              "FD", 0, "",       MACRO_Finder},
    {"FloatingMenu",        NULL, 0, "",       MACRO_FloatingMenu},
    {"Flush",               "FH", 0, "",       MACRO_Flush},
    {"FocusWindow",         NULL, 0, "S",      MACRO_FocusWindow},
    {"Generate",            NULL, 0, "SUU",    MACRO_Generate},
    {"GotoMark",            NULL, 0, "S",      MACRO_GotoMark},
    {"HelpOn",              NULL, 0, "",       MACRO_HelpOn},
    {"HelpOnTop",           NULL, 0, "",       MACRO_HelpOnTop},
    {"History",             NULL, 0, "",       MACRO_History},
    {"InitMPrint",          NULL, 1, "",       MACRO_InitMPrint},
    {"InsertItem",          NULL, 0, "SSSSU",  MACRO_InsertItem},
    {"InsertMenu",          NULL, 0, "SSU",    MACRO_InsertMenu},
    {"IfThen",              "IF", 0, "BS",     MACRO_IfThen},
    {"IfThenElse",          "IE", 0, "BSS",    MACRO_IfThenElse},
    {"IsBook",              NULL, 1, "",       MACRO_IsBook},
    {"IsMark",              NULL, 1, "S",      MACRO_IsMark},
    {"IsNotMark",           "NM", 1, "S",      MACRO_IsNotMark},
    {"JumpContents",        NULL, 0, "SS",     MACRO_JumpContents},
    {"JumpContext",         "JC", 0, "SSU",    MACRO_JumpContext},
    {"JumpHash",            "JH", 0, "SSU",    MACRO_JumpHash},
    {"JumpHelpOn",          NULL, 0, "",       MACRO_JumpHelpOn},
    {"JumpID",              "JI", 0, "SS",     MACRO_JumpID},
    {"JumpKeyword",         "JK", 0, "SSS",    MACRO_JumpKeyword},
    {"KLink",               "KL", 0, "SUSS",   MACRO_KLink},
    {"Menu",                "MU", 0, "",       MACRO_Menu},
    {"MPrintHash",          NULL, 0, "U",      MACRO_MPrintHash},
    {"MPrintID",            NULL, 0, "S",      MACRO_MPrintID},
    {"Next",                NULL, 0, "",       MACRO_Next},
    {"NoShow",              "NS", 0, "",       MACRO_NoShow},
    {"PopupContext",        "PC", 0, "SU",     MACRO_PopupContext},
    {"PopupHash",           NULL, 0, "SU",     MACRO_PopupHash},
    {"PopupId",             "PI", 0, "SS",     MACRO_PopupId},
    {"PositionWindow",      "PW", 0, "IIUUUS", MACRO_PositionWindow},
    {"Prev",                NULL, 0, "",       MACRO_Prev},
    {"Print",               NULL, 0, "",       MACRO_Print},
    {"PrinterSetup",        NULL, 0, "",       MACRO_PrinterSetup},
    {"RegisterRoutine",     "RR", 0, "SSS",    MACRO_RegisterRoutine},
    {"RemoveAccelerator",   "RA", 0, "UU",     MACRO_RemoveAccelerator},
    {"ResetMenu",           NULL, 0, "",       MACRO_ResetMenu},
    {"SaveMark",            NULL, 0, "S",      MACRO_SaveMark},
    {"Search",              NULL, 0, "",       MACRO_Search},
    {"SetContents",         NULL, 0, "SU",     MACRO_SetContents},
    {"SetHelpOnFile",       NULL, 0, "S",      MACRO_SetHelpOnFile},
    {"SetPopupColor",       "SPC",0, "UUU",    MACRO_SetPopupColor},
    {"ShellExecute",        "SE", 0, "SSUUSS", MACRO_ShellExecute},
    {"ShortCut",            "SH", 0, "SSUUS",  MACRO_ShortCut},
    {"TCard",               NULL, 0, "U",      MACRO_TCard},
    {"Test",                NULL, 0, "U",      MACRO_Test},
    {"TestALink",           NULL, 1, "S",      MACRO_TestALink},
    {"TestKLink",           NULL, 1, "S",      MACRO_TestKLink},
    {"UncheckItem",         "UI", 0, "S",      MACRO_UncheckItem},
    {"UpdateWindow",        "UW", 0, "SS",     MACRO_UpdateWindow},
    {NULL,                  NULL, 0, NULL,     NULL}
};

static int MACRO_DoLookUp(struct MacroDesc* start, const char* name, struct lexret* lr, unsigned len)
{
    struct MacroDesc*   md;

    for (md = start; md->name && len != 0; md++, len--)
    {
        if (strcasecmp(md->name, name) == 0 || (md->alias != NULL && strcasecmp(md->alias, name) == 0))
        {
            lr->proto = md->arguments;
            lr->function = md->fn;
            return md->isBool ? BOOL_FUNCTION : VOID_FUNCTION;
        }
    }
    return EMPTY;
}

int MACRO_Lookup(const char* name, struct lexret* lr)
{
    int ret;

    if ((ret = MACRO_DoLookUp(MACRO_Builtins, name, lr, -1)) != EMPTY)
        return ret;
    if (MACRO_Loaded && (ret = MACRO_DoLookUp(MACRO_Loaded, name, lr, MACRO_NumLoaded)) != EMPTY)
        return ret;
    if (!strcmp(name, "hwndApp"))
    {
        WINHELP_WINDOW* win;
        lr->integer = 0;
        for (win = Globals.win_list; win; win = win->next)
        {
            if (!strcmp(win->info->name, "main"))
            {
                lr->integer = (LONG_PTR)win->hMainWnd;
                break;
            }
        }
        return INTEGER;
    }
    if (!strcmp(name, "hwndContext"))
    {
        lr->integer = Globals.active_win ?
            (LONG_PTR)Globals.active_win->hMainWnd : 0;
        return INTEGER;
    }
    if (!strcmp(name, "qchPath") || !strcmp(name, "qError") || !strcmp(name, "lTopicNo") ||
        !strcmp(name, "hfs") || !strcmp(name, "coForeground") || !strcmp(name, "coBackground"))
    {
        WINE_FIXME("keyword %s not substituted in macro parsing\n", debugstr_a(name));
        return EMPTY;
    }

    lr->string = name;
    return IDENTIFIER;
}
