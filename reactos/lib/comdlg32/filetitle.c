/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "cdlg.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

/***********************************************************************
 *	GetFileTitleA		(COMDLG32.@)
 *
 */
short WINAPI GetFileTitleA(LPCSTR lpFile, LPSTR lpTitle, WORD cbBuf)
{
    int ret;
    UNICODE_STRING strWFile;
    LPWSTR lpWTitle;

    RtlCreateUnicodeStringFromAsciiz(&strWFile, lpFile);
    lpWTitle = RtlAllocateHeap( GetProcessHeap(), 0, cbBuf*sizeof(WCHAR));
    ret = GetFileTitleW(strWFile.Buffer, lpWTitle, cbBuf);
    if (!ret) WideCharToMultiByte( CP_ACP, 0, lpWTitle, -1, lpTitle, cbBuf, NULL, NULL );
    RtlFreeUnicodeString( &strWFile );
    RtlFreeHeap( GetProcessHeap(), 0, lpWTitle );
    return ret;
}


/***********************************************************************
 *	GetFileTitleW		(COMDLG32.@)
 *
 */
short WINAPI GetFileTitleW(LPCWSTR lpFile, LPWSTR lpTitle, WORD cbBuf)
{
	int i, len;
        static const WCHAR brkpoint[] = {'*','[',']',0};
	TRACE("(%p %p %d); \n", lpFile, lpTitle, cbBuf);

	if(lpFile == NULL || lpTitle == NULL)
		return -1;

	len = strlenW(lpFile);

	if (len == 0)
		return -1;

	if(strpbrkW(lpFile, brkpoint))
		return -1;

	len--;

	if(lpFile[len] == '/' || lpFile[len] == '\\' || lpFile[len] == ':')
		return -1;

	for(i = len; i >= 0; i--)
	{
		if (lpFile[i] == '/' ||  lpFile[i] == '\\' ||  lpFile[i] == ':')
		{
			i++;
			break;
		}
	}

	if(i == -1)
		i++;

	TRACE("---> '%s' \n", debugstr_w(&lpFile[i]));

	len = strlenW(lpFile+i)+1;
	if(cbBuf < len)
		return len;

	strcpyW(lpTitle, &lpFile[i]);
	return 0;
}


/***********************************************************************
 *	GetFileTitle		(COMMDLG.27)
 */
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf)
{
	return GetFileTitleA(lpFile, lpTitle, cbBuf);
}
