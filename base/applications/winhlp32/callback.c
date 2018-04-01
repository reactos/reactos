/*
 * Help Viewer - DLL callback into WineHelp
 *
 * Copyright 2004 Eric Pouech
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

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>

#include "windows.h"
#include "winhelp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

static WORD CALLBACK WHD_GetFSError(void)
{
    WINE_FIXME("()\n");
    return 0;
}

static HANDLE CALLBACK WHD_Open(LPSTR name, BYTE flags)
{
    unsigned    mode = 0;

    WINE_FIXME("(%s %x)\n", debugstr_a(name), flags);
    switch (flags)
    {
    case 0: mode = GENERIC_READ | GENERIC_WRITE; break;
    case 2: mode = GENERIC_READ; break;
    default: WINE_FIXME("Undocumented flags %x\n", flags);
    }
    return CreateFileA(name, mode, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

static WORD CALLBACK WHD_Close(HANDLE fs)
{
    WINE_FIXME("(%p)\n", fs);
    CloseHandle(fs);
    return 0;
}

static HANDLE CALLBACK WHD_OpenBag(HANDLE fs, LPSTR name, BYTE flags)
{
    WINE_FIXME("(%p %s %x)\n", fs, debugstr_a(name), flags);
    return NULL;
}

static HANDLE CALLBACK WHD_CloseBag(HANDLE bag)
{
    WINE_FIXME("()\n");
    return NULL;
}

static LONG CALLBACK WHD_ReadBag(HANDLE bag, BYTE* ptr, LONG len)
{
    WINE_FIXME("()\n");
    return 0;
}

static LONG CALLBACK WHD_TellBag(HANDLE bag)
{
    WINE_FIXME("()\n");
    return 0;
}

static LONG CALLBACK WHD_SeekBag(HANDLE bag, LONG offset, WORD whence)
{
    WINE_FIXME("()\n");
    return 0;
}

static BOOL CALLBACK WHD_IsEofBag(HANDLE bag)
{
    WINE_FIXME("()\n");
    return FALSE;
}

static LONG CALLBACK WHD_SizeBag(HANDLE bag)
{
    WINE_FIXME("()\n");
    return 0;
}

static BOOL CALLBACK WHD_Access(HANDLE fs, LPSTR name, BYTE flags)
{
    WINE_FIXME("()\n");
    return FALSE;
}

static WORD CALLBACK WHD_LLInfoFromBag(HANDLE bag, WORD opt, LPWORD p1, LPLONG p2, LPLONG p3)
{
    WINE_FIXME("()\n");
    return 0;
}

static WORD CALLBACK WHD_LLInfoFromFile(HANDLE fs, LPSTR name, WORD opt, LPWORD p1, LPLONG p2, LPLONG p3)
{
    WINE_FIXME("()\n");
    return 0;
}

static void CALLBACK WHD_Error(int err)
{
    WINE_FIXME("()\n");
}

static void CALLBACK WHD_ErrorString(LPSTR err)
{
    WINE_FIXME("()\n");
}

static ULONG_PTR CALLBACK WHD_GetInfo(WORD what, HWND hnd)
{
    ULONG_PTR ret = 0;

    WINE_TRACE("(%x %p)\n", what, hnd);
    switch (what)
    {
    case 0: break;
    case 1: /* instance */ ret = (ULONG_PTR)Globals.hInstance; break;
    case 3: /* current window */ ret = (ULONG_PTR)Globals.active_win->hMainWnd; break;
    case 2: /* main window */
    case 4: /* handle to opened file */
    case 5: /* foreground color */
    case 6: /* background color */
    case 7: /* topic number */
    case 8: /* current opened file name */
        WINE_FIXME("NIY %u\n", what);
        break;
    default:
        WINE_FIXME("Undocumented %u\n", what);
        break;
    }
    return ret;
}

static LONG CALLBACK WHD_API(LPSTR x, WORD xx, DWORD xxx)
{
    WINE_FIXME("()\n");
    return 0;
}

const struct winhelp_callbacks Callbacks =
{
    WHD_GetFSError,
    WHD_Open,
    WHD_Close,
    WHD_OpenBag,
    WHD_CloseBag,
    WHD_ReadBag,
    WHD_TellBag,
    WHD_SeekBag,
    WHD_IsEofBag,
    WHD_SizeBag,
    WHD_Access,
    WHD_LLInfoFromBag,
    WHD_LLInfoFromFile,
    WHD_Error,
    WHD_ErrorString,
    WHD_GetInfo,
    WHD_API
};
