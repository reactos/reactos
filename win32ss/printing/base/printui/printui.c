/*
 * Implementation of the Printer User Interface Dialogs
 *
 * Copyright 2006-2007 Detlef Riekenberg
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winver.h"
#include "winnls.h"
#include "shellapi.h"

#include "wine/debug.h"
#include "printui_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(printui);

/* ################################# */

/* Must be in order with OPT_*      */
static const WCHAR optionsW[OPT_MAX+1]={'a','b','c','f','h','j','l','m','n','t','r','v',0};

/* Must be in order with FLAG_*     */
static const WCHAR flagsW[FLAG_MAX+1]={'q','w','y','z','Z',0};


/* ################################
 * get_next_wstr() [Internal]
 *
 * Get the next WSTR, when available
 *
 */

static LPWSTR get_next_wstr(context_t * cx)
{
    LPWSTR  ptr;

    ptr = cx->pNextCharW;
    if (ptr && ptr[0]) {
        cx->pNextCharW = NULL;
        return ptr;
    }

    /* Get the next Parameter, when available */
    if (cx->next_arg < cx->argc) {
        ptr = cx->argv[cx->next_arg];
        cx->next_arg++;
        cx->pNextCharW = NULL;
        return ptr;
    }
    return NULL;
}


/* ################################
 * get_next_wchar() [Internal]
 *
 * Get the next WCHAR from the Commandline or from the File (@ Filename)
 *
 * ToDo: Support Parameter from a File ( "@Filename" )
 *
 */

static WCHAR get_next_wchar(context_t * cx, BOOL use_next_parameter)
{
    WCHAR   c;

    /* Try the next WCHAR in the actual Parameter */
    if (cx->pNextCharW) {
        c = *cx->pNextCharW;
        if (c) {
            cx->pNextCharW++;
            return c;
        }
        /* We reached the end of the Parameter */
        cx->pNextCharW = NULL;
    }

    /* Get the next Parameter, when available and allowed */
    if ((cx->pNextCharW == NULL) && (cx->next_arg < cx->argc) && (use_next_parameter)) {
        cx->pNextCharW = cx->argv[cx->next_arg];
        cx->next_arg++;
    }

    if (cx->pNextCharW) {
        c = *cx->pNextCharW;
        if (c) {
            cx->pNextCharW++;
        }
        else
        {
            /* We reached the end of the Parameter */
            cx->pNextCharW = NULL;
        }
        return c;
    }
    return '\0';
}

/* ################################ */
static BOOL parse_rundll(context_t * cx)
{
    LPWSTR  ptr;
    DWORD   index;
    WCHAR   txtW[2];
    WCHAR   c;


    c = get_next_wchar(cx, TRUE);
    txtW[1] = '\0';

    while (c)
    {

        while ( (c == ' ') || (c == '\t'))
        {
            c = get_next_wchar(cx, TRUE);
        }
        txtW[0] = c;

        if (c == '@') {
            /* read commands from a File */
            ptr = get_next_wstr(cx);
            FIXME("redir not supported: %s\n", debugstr_w(ptr));
            return FALSE;
        }
        else if (c == '/') {
            c = get_next_wchar(cx, FALSE);
            while ( c )
            {
                txtW[0] = c;
                ptr = wcschr(optionsW, c);
                if (ptr) {
                    index = ptr - optionsW;
                    cx->options[index] = get_next_wstr(cx);
                    TRACE(" opt: %s  %s\n", debugstr_w(txtW), debugstr_w(cx->options[index]));
                    c = 0;
                }
                else
                {
                    ptr = wcschr(flagsW, c);
                    if (ptr) {
                        index = ptr - flagsW;
                        cx->flags[index] = TRUE;
                        TRACE("flag: %s\n", debugstr_w(txtW));
                    }
                    else
                    {
                        cx->command = c;
                        cx->subcommand = '\0';
                        TRACE(" cmd: %s\n", debugstr_w(txtW));
                    }

                    /* help has priority over all commands */
                    if (c == '?') {
                        return TRUE;
                    }

                    c = get_next_wchar(cx, FALSE);

                    /* Some commands use two wchar */
                    if ((cx->command == 'd') || (cx->command == 'g') || (cx->command == 'i') ||
                        (cx->command == 'S') || (cx->command == 'X') ){
                        cx->subcommand = c;
                        txtW[0] = c;
                        TRACE(" sub: %s\n", debugstr_w(txtW));
                        c = get_next_wchar(cx, FALSE);
                    }
                }
            }
            c = get_next_wchar(cx, TRUE);

        }
        else
        {
            /* The commands 'S' and 'X' have additional Parameter */
            if ((cx->command == 'S') || (cx->command == 'X')) {

                /* the actual WCHAR is the start from the extra Parameter */
                cx->pNextCharW--;
                TRACE("%d extra Parameter, starting with %s\n", 1 + (cx->argc - cx->next_arg), debugstr_w(cx->pNextCharW));
                return TRUE;
            }
            FIXME("0x%x: %s is unknown\n", c, debugstr_wn(&c, 1));
            return FALSE;
        }

    }
    return TRUE;
}

/*****************************************************
 *  PrintUIEntryW                [printui.@]
 *  Commandline-Interface for using printui.dll with rundll32.exe
 *
 */
void WINAPI PrintUIEntryW(HWND hWnd, HINSTANCE hInst, LPCWSTR pCommand, DWORD nCmdShow)
{
    context_t cx;
    BOOL  res = FALSE;

    TRACE("(%p, %p, %s, 0x%lx)\n", hWnd, hInst, debugstr_w(pCommand), nCmdShow);

    memset(&cx, 0, sizeof(context_t));
    cx.hWnd = hWnd;
    cx.nCmdShow = nCmdShow;

    if ((pCommand) && (pCommand[0])) {
        /* result is allocated with LocalAlloc() */
        cx.argv = CommandLineToArgvW(pCommand, &cx.argc);
        TRACE("got %d args at %p\n", cx.argc, cx.argv);

        res = parse_rundll(&cx);
    }

    if (res && cx.command) {
        switch (cx.command)
        {

            default:
            {
                WCHAR   txtW[3];
                txtW[0] = cx.command;
                txtW[1] = cx.subcommand;
                txtW[2] = '\0';
                FIXME("command not implemented: %s\n", debugstr_w(txtW));
            }
        }
    }

    if ((res == FALSE) || (cx.command == '\0')) {
        FIXME("dialog: Printer / The operation was not successful\n");
    }

    LocalFree(cx.argv);

}
