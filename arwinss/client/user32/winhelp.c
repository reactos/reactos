/*
 * Windows Help
 *
 * Copyright 1996 Martin von Loewis
 *           2002 Eric Pouech
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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

/* Wine doesn't use the way WinHelp API sends information in Windows, because:
 * 1/ it's not consistent across Win9x, NT...
 * 2/ NT implementation is not yet fully understood (and includes some shared
 *     memory mechanism)
 * 3/ uses a dynamically allocated message number (WM_WINHELP), which 
 *    obfuscates the code
 *
 * So we use (for now) the simple protocol:
 * 1/ it's based on copy data
 * 2/ we tag the message with a magic number, to make it a bit more robust 
 *   (even if it's not 100% safe)
 * 3/ data structure (WINHELP) has the same layout that the one used on Win95. 
 *    This doesn't bring much, except not going to far away from real 
 *    implementation.
 *
 * This means anyway that native winhelp.exe and winhlp32.exe cannot be 
 * called/manipulated from WinHelp API.
 */
typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP;

/* magic number for this message:
 *  aide means help is French ;-) 
 *  SOS means ???
 */
#define WINHELP_MAGIC   0xA1DE505

/**********************************************************************
 *		WinHelpA (USER32.@)
 */
BOOL WINAPI WinHelpA( HWND hWnd, LPCSTR lpHelpFile, UINT wCommand, ULONG_PTR dwData )
{
    COPYDATASTRUCT      cds;
    HWND                hDest;
    int                 size, dsize, nlen;
    WINHELP*            lpwh;

    hDest = FindWindowA("MS_WINHELP", NULL);
    if (!hDest) 
    {
        if (wCommand == HELP_QUIT) return TRUE;
        if (WinExec("winhlp32.exe -x", SW_SHOWNORMAL) < 32) 
        {
            ERR("can't start winhlp32.exe -x ?\n");
            return FALSE;
        }
        if (!(hDest = FindWindowA("MS_WINHELP", NULL))) 
        {
            FIXME("Did not find a MS_WINHELP Window\n");
            return FALSE;
        }
    }

    switch (wCommand)
    {
    case HELP_CONTEXT:
    case HELP_SETCONTENTS:
    case HELP_CONTENTS:
    case HELP_CONTEXTPOPUP:
    case HELP_FORCEFILE:
    case HELP_HELPONHELP:
    case HELP_FINDER:
    case HELP_QUIT:
        dsize = 0;
        break;
    case HELP_KEY:
    case HELP_PARTIALKEY:
    case HELP_COMMAND:
        dsize = dwData ? strlen((LPSTR)dwData) + 1 : 0;
        break;
    case HELP_MULTIKEY:
        dsize = ((LPMULTIKEYHELPA)dwData)->mkSize;
        break;
    case HELP_SETWINPOS:
        dsize = ((LPHELPWININFOA)dwData)->wStructSize;
        break;
    default:
        FIXME("Unknown help command %d\n", wCommand);
        return FALSE;
    }
    if (lpHelpFile)
        nlen = strlen(lpHelpFile) + 1;
    else
        nlen = 0;
    size = sizeof(WINHELP) + nlen + dsize;

    lpwh = HeapAlloc(GetProcessHeap(), 0, size);
    if (!lpwh) return FALSE;

    cds.dwData = WINHELP_MAGIC;
    cds.cbData = size;
    cds.lpData = (void*)lpwh;

    lpwh->size = size;
    lpwh->command = wCommand;
    lpwh->data = dwData;
    if (nlen) 
    {
        strcpy(((char*)lpwh) + sizeof(WINHELP), lpHelpFile);
        lpwh->ofsFilename = sizeof(WINHELP);
    } else
        lpwh->ofsFilename = 0;
    if (dsize) 
    {
        memcpy(((char*)lpwh) + sizeof(WINHELP) + nlen, (LPSTR)dwData, dsize);
        lpwh->ofsData = sizeof(WINHELP) + nlen;
    } else
        lpwh->ofsData = 0;
    WINE_TRACE("Sending[%u]: cmd=%u data=%08x fn=%s\n",
               lpwh->size, lpwh->command, lpwh->data,
               lpwh->ofsFilename ? (LPSTR)lpwh + lpwh->ofsFilename : "");

    return SendMessageA(hDest, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);
}


/**********************************************************************
 *		WinHelpW (USER32.@)
 */
BOOL WINAPI WinHelpW( HWND hWnd, LPCWSTR helpFile, UINT command, ULONG_PTR dwData )
{
    INT len;
    LPSTR file;
    BOOL ret = FALSE;

    if (!helpFile) return WinHelpA( hWnd, NULL, command, dwData );

    len = WideCharToMultiByte( CP_ACP, 0, helpFile, -1, NULL, 0, NULL, NULL );
    if ((file = HeapAlloc( GetProcessHeap(), 0, len )))
    {
        WideCharToMultiByte( CP_ACP, 0, helpFile, -1, file, len, NULL, NULL );
        ret = WinHelpA( hWnd, file, command, dwData );
        HeapFree( GetProcessHeap(), 0, file );
    }
    return ret;
}
