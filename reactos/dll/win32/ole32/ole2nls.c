/*
 *	OLE2NLS library
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1998  David Lee Lambert
 *      Copyright 2000  Julio César Gázquez
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <locale.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"
#include "winver.h"

#include "wine/winbase16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static LPVOID lpNLSInfo = NULL;

/******************************************************************************
 *		GetLocaleInfoA	[OLE2NLS.5]
 * Is the last parameter really WORD for Win16?
 */
INT16 WINAPI GetLocaleInfo16(LCID lcid,LCTYPE LCType,LPSTR buf,INT16 len)
{
	return GetLocaleInfoA(lcid,LCType,buf,len);
}

/******************************************************************************
 *		GetStringTypeA	[OLE2NLS.7]
 */
BOOL16 WINAPI GetStringType16(LCID locale,DWORD dwInfoType,LPCSTR src,
                              INT16 cchSrc,LPWORD chartype)
{
	return GetStringTypeExA(locale,dwInfoType,src,cchSrc,chartype);
}

/******************************************************************************
 *		GetUserDefaultLCID	[OLE2NLS.1]
 */
LCID WINAPI GetUserDefaultLCID16(void)
{
    return GetUserDefaultLCID();
}

/******************************************************************************
 *		GetSystemDefaultLCID	[OLE2NLS.2]
 */
LCID WINAPI GetSystemDefaultLCID16(void)
{
    return GetSystemDefaultLCID();
}

/******************************************************************************
 *		GetUserDefaultLangID	[OLE2NLS.3]
 */
LANGID WINAPI GetUserDefaultLangID16(void)
{
    return GetUserDefaultLangID();
}

/******************************************************************************
 *		GetSystemDefaultLangID	[OLE2NLS.4]
 */
LANGID WINAPI GetSystemDefaultLangID16(void)
{
    return GetSystemDefaultLangID();
}

/******************************************************************************
 *		LCMapStringA	[OLE2NLS.6]
 */
INT16 WINAPI LCMapString16(LCID lcid, DWORD mapflags, LPCSTR srcstr, INT16 srclen,
			   LPSTR dststr, INT16 dstlen)
{
    return LCMapStringA(lcid, mapflags, srcstr, srclen, dststr, dstlen);
}

/***********************************************************************
 *           CompareStringA       (OLE2NLS.8)
 */
UINT16 WINAPI CompareString16(DWORD lcid,DWORD fdwStyle,
                              LPCSTR s1,DWORD l1,LPCSTR s2,DWORD l2)
{
	return (UINT16)CompareStringA(lcid,fdwStyle,s1,l1,s2,l2);
}

/******************************************************************************
 *      RegisterNLSInfoChanged  [OLE2NLS.9]
 */
BOOL16 WINAPI RegisterNLSInfoChanged16(LPVOID lpNewNLSInfo) /* [???] FIXME */
{
	FIXME("Fully implemented, but doesn't effect anything.\n");

	if (!lpNewNLSInfo) {
		lpNLSInfo = NULL;
		return TRUE;
	}
	else {
		if (!lpNLSInfo) {
			lpNLSInfo = lpNewNLSInfo;
			return TRUE;
		}
	}

	return FALSE; /* ptr not set */
}
