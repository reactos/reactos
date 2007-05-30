/*
 *	TYPELIB 16bit part.
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1999 Rein Klazes
 * Copyright 2000 Francois Jacques
 * Copyright 2001 Huw D M Davies for CodeWeavers
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
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"

#include "objbase.h"
#include "ole2disp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/*************************************************************************
 * TYPELIB {TYPELIB}
 *
 * This dll is the 16 bit version of the Typelib API, part the original
 * implementation of Ole automation. It and its companion ole2disp.dll were
 * superseded by oleaut32.dll which provides 32 bit implementations of these
 * functions and greatly extends the Ole Api.
 *
 * Winelib developers cannot use these functions directly, they are implemented
 * solely for backwards compatibility with existing legacy applications.
 *
 * SEE ALSO
 *  oleaut32(), ole2disp().
 */

/****************************************************************************
 *		QueryPathOfRegTypeLib	[TYPELIB.14]
 *
 * Get the registry key of a registered type library.
 *
 * RETURNS
 *  Success: S_OK. path is updated with the key name
 *  Failure: E_FAIL, if guid was not found in the registry
 *
 * NOTES
 *  The key takes the form "Classes\Typelib\<guid>\<major>.<minor>\<lcid>\win16\"
 */
HRESULT WINAPI
QueryPathOfRegTypeLib16(
	REFGUID guid,	/* [in] Guid to get the key name for */
	WORD wMaj,	/* [in] Major version */
	WORD wMin,	/* [in] Minor version */
	LCID lcid,	/* [in] Locale Id */
	LPBSTR16 path)	/* [out] Destination for the registry key name */
{
	char	xguid[80];
	char	typelibkey[100],pathname[260];
	LONG	plen;

       	TRACE("\n");

	if (HIWORD(guid)) {
            sprintf( typelibkey, "SOFTWARE\\Classes\\Typelib\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\%d.%d\\%x\\win16",
                     guid->Data1, guid->Data2, guid->Data3,
                     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7],
                     wMaj,wMin,lcid);
	} else {
		sprintf(xguid,"<guid 0x%08x>",(DWORD)guid);
		FIXME("(%s,%d,%d,0x%04x,%p),can't handle non-string guids.\n",xguid,wMaj,wMin,(DWORD)lcid,path);
		return E_FAIL;
	}
	plen = sizeof(pathname);
	if (RegQueryValueA(HKEY_LOCAL_MACHINE,typelibkey,pathname,&plen)) {
		/* try again without lang specific id */
		if (SUBLANGID(lcid))
			return QueryPathOfRegTypeLib16(guid,wMaj,wMin,PRIMARYLANGID(lcid),path);
		FIXME("key %s not found\n",typelibkey);
		return E_FAIL;
	}
	*path = SysAllocString16(pathname);
	return S_OK;
}

/******************************************************************************
 * LoadTypeLib [TYPELIB.3]
 *
 * Load and register a type library.
 *
 * RETURNS
 *  Success: S_OK. pptLib contains the type libraries ITypeLib interface.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  Both parameters are FAR pointers.
 */
HRESULT WINAPI LoadTypeLib16(
    LPSTR szFile, /* [in] Name of file to load from */
    ITypeLib** pptLib) /* [out] Destination for loaded ITypeLib interface */
{
    FIXME("(%s,%p): stub\n",debugstr_a(szFile),pptLib);

    if (pptLib!=0)
      *pptLib=0;

    return E_FAIL;
}

/****************************************************************************
 *	OaBuildVersion				(TYPELIB.15)
 *
 * Get the Ole Automation build version.
 *
 * PARAMS
 *  None
 *
 * RETURNS
 *  The build version.
 *
 * NOTES
 *  Known typelib.dll versions:
 *| OLE Ver.  Comments                   Date    Build Ver.
 *| --------  -------------------------  ----    ---------
 *| OLE 2.01  Call not available         1993     N/A
 *| OLE 2.02                             1993-94  02 3002
 *| OLE 2.03                                      23 730
 *| OLE 2.03                                      03 3025
 *| OLE 2.03  W98 SE orig. file !!       1993-95  10 3024
 *| OLE 2.1   NT                         1993-95  ?? ???
 *| OLE 2.3.1 W95                                 23 700
 *| OLE2 4.0  NT4SP6                     1993-98  40 4277
 */
DWORD WINAPI OaBuildVersion16(void)
{
    /* FIXME: I'd like to return the highest currently known version value
     * in case the user didn't force a --winver, but I don't know how
     * to retrieve the "versionForced" info from misc/version.c :(
     * (this would be useful in other places, too) */
    FIXME("If you get version error messages, please report them\n");
    switch(GetVersion() & 0x8000ffff)  /* mask off build number */
    {
    case 0x80000a03:  /* WIN31 */
		return MAKELONG(3027, 3); /* WfW 3.11 */
    case 0x80000004:  /* WIN95 */
		return MAKELONG(700, 23); /* Win95A */
    case 0x80000a04:  /* WIN98 */
		return MAKELONG(3024, 10); /* W98 SE */
    case 0x00000004:  /* NT4 */
		return MAKELONG(4277, 40); /* NT4 SP6 */
    default:
	FIXME("Version value not known yet. Please investigate it!\n");
		return 0;
    }
}
