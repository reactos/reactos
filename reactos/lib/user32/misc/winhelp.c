/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: winhelp.c,v 1.1 2002/08/27 06:40:15 robd Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/winhelp.c
 * PURPOSE:         WinHelp
 * PROGRAMMER:      Robert Dickenson(robd@reactos.org)
 * UPDATE HISTORY:
 *      23-08-2002  RDD  Created from wine sources
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* WinHelp internal structure */
typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP,*LPWINHELP;


/* FUNCTIONS *****************************************************************/

WINBOOL
STDCALL
WinHelpA(HWND hWnd, LPCSTR lpszHelp, UINT uCommand, DWORD dwData)
{
	static WORD WM_WINHELP = 0;
	HWND hDest;
	LPWINHELP lpwh;
	HGLOBAL hwh;
	int size,dsize,nlen;

	if (!WM_WINHELP) {
	    WM_WINHELP = RegisterWindowMessageA("WM_WINHELP");
	    if (!WM_WINHELP)
	      return FALSE;
    }

	hDest = FindWindowA("MS_WINHELP", NULL);
	if (!hDest) {
	    if (uCommand == HELP_QUIT) return TRUE;
        if (WinExec("winhlp32.exe -x", SW_SHOWNORMAL) < 32) {
            //ERR("can't start winhlp32.exe -x ?\n");
            return FALSE;
        } 
	    if (!(hDest = FindWindowA("MS_WINHELP", NULL))) {
	        //FIXME("did not find MS_WINHELP (FindWindow() failed, maybe global window handling still unimplemented)\n");
	        return FALSE;
        }
    }
	switch (uCommand) {
		case HELP_CONTEXT:
		case HELP_SETCONTENTS:
		case HELP_CONTENTS:
		case HELP_CONTEXTPOPUP:
		case HELP_FORCEFILE:
		case HELP_HELPONHELP:
		case HELP_FINDER:
		case HELP_QUIT:
			dsize=0;
			break;
		case HELP_KEY:
		case HELP_PARTIALKEY:
		case HELP_COMMAND:
			dsize = dwData ? strlen( (LPSTR)dwData )+1: 0;
			break;
		case HELP_MULTIKEY:
			dsize = ((LPMULTIKEYHELPA)dwData)->mkSize;
			break;
		case HELP_SETWINPOS:
			dsize = ((LPHELPWININFOA)dwData)->wStructSize;
			break;
		default:
			//FIXME("Unknown help command %d\n",uCommand);
			return FALSE;
	}
	if (lpszHelp)
		nlen = strlen(lpszHelp)+1;
	else
		nlen = 0;
	size = sizeof(WINHELP) + nlen + dsize;
	hwh = GlobalAlloc(0,size);
	lpwh = GlobalLock(hwh);
	lpwh->size = size;
	lpwh->command = uCommand;
	lpwh->data = dwData;
	if (nlen) {
		strcpy(((char*)lpwh) + sizeof(WINHELP), lpszHelp);
		lpwh->ofsFilename = sizeof(WINHELP);
 	} else {
		lpwh->ofsFilename = 0;
	}
	if (dsize) {
		memcpy(((char*)lpwh)+sizeof(WINHELP)+nlen,(LPSTR)dwData,dsize);
		lpwh->ofsData = sizeof(WINHELP)+nlen;
	} else {
		lpwh->ofsData = 0;
	}
	GlobalUnlock(hwh);
	return SendMessage(hDest, WM_WINHELP, hWnd, (LPARAM)hwh);
}

WINBOOL
STDCALL
WinHelpW(HWND hWnd, LPCWSTR lpszHelp, UINT uCommand, DWORD dwData)
{
    INT len;
    LPSTR file;
    BOOL ret = FALSE;

    if (!lpszHelp) return WinHelpA(hWnd, NULL, uCommand, dwData);

    len = WideCharToMultiByte(CP_ACP, 0, lpszHelp, -1, NULL, 0, NULL, NULL);
    if ((file = HeapAlloc(GetProcessHeap(), 0, len))) {
        WideCharToMultiByte(CP_ACP, 0, lpszHelp, -1, file, len, NULL, NULL);
        ret = WinHelpA(hWnd, file, uCommand, dwData);
        HeapFree(GetProcessHeap(), 0, file);
    }
    return ret;
}

