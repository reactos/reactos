/*
 * Implementation of VERSION.DLL - Version Info access
 *
 * Copyright 1996,1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 * Copyright 1999 Ulrich Weigand
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
 *
 * TODO
 *   o Verify VerQueryValue()
 */
#include "config.h"
//#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winver.h"
#include "wine/debug.h"
#include "wine/unicode.h"

/*#ifdef __REACTOS__
DWORD WINAPI GetFileResourceSize16( LPCSTR lpszFileName, LPCSTR lpszResType,
                                    LPCSTR lpszResId, LPDWORD lpdwFileOffset );
DWORD WINAPI GetFileResource16( LPCSTR lpszFileName, LPCSTR lpszResType,
                                LPCSTR lpszResId, DWORD dwFileOffset,
                                DWORD dwResLen, LPVOID lpvData )
#endif*/

WINE_DEFAULT_DEBUG_CHANNEL(ver);

static
void
VERSION_A2W ( LPWSTR wide_str, LPCSTR ansi_str, int len )
{
	MultiByteToWideChar ( CP_ACP, 0, ansi_str, -1, wide_str, len );
}

static
void
VERSION_W2A ( LPSTR ansi_str, LPCWSTR wide_str, int len )
{
	WideCharToMultiByte ( CP_ACP, 0, wide_str, -1, ansi_str, len, NULL, NULL );
}

static
LPWSTR
VERSION_AllocA2W ( LPCSTR ansi_str )
{
	int len = MultiByteToWideChar ( CP_ACP, 0, ansi_str, -1, NULL, 0 );
	LPWSTR wide_str = HeapAlloc ( GetProcessHeap(), 0, len * sizeof(WCHAR) );
	if ( !wide_str )
		return NULL;
	VERSION_A2W ( wide_str, ansi_str, len );
	return wide_str;
}

static
LPSTR
VERSION_AllocW2A ( LPCWSTR wide_str )
{
	int len = WideCharToMultiByte ( CP_ACP, 0, wide_str, -1, NULL, 0, NULL, NULL );
	LPSTR ansi_str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
	if (!ansi_str)
		return NULL;
	VERSION_W2A ( ansi_str, wide_str, len );
	return ansi_str;
}

static
void
VERSION_Free ( LPVOID lpvoid )
{
	HeapFree ( GetProcessHeap(), 0, lpvoid );
}


/******************************************************************************
 *
 *   This function will print via dprintf[_]ver to stddeb debug info regarding
 *   the file info structure vffi.
 *      15-Feb-1998 Dimitrie Paun (dimi@cs.toronto.edu)
 *      Added this function to clean up the code.
 *
 *****************************************************************************/
static
void
print_vffi_debug(VS_FIXEDFILEINFO *vffi)
{
	TRACE(" structversion=%u.%u, fileversion=%u.%u.%u.%u, productversion=%u.%u.%u.%u, flagmask=0x%lx, flags=%s%s%s%s%s%s\n",
		    HIWORD(vffi->dwStrucVersion),LOWORD(vffi->dwStrucVersion),
		    HIWORD(vffi->dwFileVersionMS),LOWORD(vffi->dwFileVersionMS),
		    HIWORD(vffi->dwFileVersionLS),LOWORD(vffi->dwFileVersionLS),
		    HIWORD(vffi->dwProductVersionMS),LOWORD(vffi->dwProductVersionMS),
		    HIWORD(vffi->dwProductVersionLS),LOWORD(vffi->dwProductVersionLS),
		    vffi->dwFileFlagsMask,
		    (vffi->dwFileFlags & VS_FF_DEBUG) ? "DEBUG," : "",
		    (vffi->dwFileFlags & VS_FF_PRERELEASE) ? "PRERELEASE," : "",
		    (vffi->dwFileFlags & VS_FF_PATCHED) ? "PATCHED," : "",
		    (vffi->dwFileFlags & VS_FF_PRIVATEBUILD) ? "PRIVATEBUILD," : "",
		    (vffi->dwFileFlags & VS_FF_INFOINFERRED) ? "INFOINFERRED," : "",
		    (vffi->dwFileFlags & VS_FF_SPECIALBUILD) ? "SPECIALBUILD," : ""
		    );

	TRACE("(");
	TRACE(" OS=0x%x.0x%x ",
		HIWORD(vffi->dwFileOS),
		LOWORD(vffi->dwFileOS)
	);
	switch (vffi->dwFileOS&0xFFFF0000) {
	case VOS_DOS:TRACE("DOS,");break;
	case VOS_OS216:TRACE("OS/2-16,");break;
	case VOS_OS232:TRACE("OS/2-32,");break;
	case VOS_NT:TRACE("NT,");break;
	case VOS_UNKNOWN:
	default:
		TRACE("UNKNOWN(0x%lx),",vffi->dwFileOS&0xFFFF0000);break;
	}
	switch (LOWORD(vffi->dwFileOS)) {
	case VOS__BASE:TRACE("BASE");break;
	case VOS__WINDOWS16:TRACE("WIN16");break;
	case VOS__WINDOWS32:TRACE("WIN32");break;
	case VOS__PM16:TRACE("PM16");break;
	case VOS__PM32:TRACE("PM32");break;
	default:TRACE("UNKNOWN(0x%x)",LOWORD(vffi->dwFileOS));break;
	}
	TRACE(")\n");

	switch (vffi->dwFileType) {
	default:
	case VFT_UNKNOWN:
		TRACE("filetype=Unknown(0x%lx)",vffi->dwFileType);
		break;
	case VFT_APP:TRACE("filetype=APP,");break;
	case VFT_DLL:TRACE("filetype=DLL,");break;
	case VFT_DRV:
		TRACE("filetype=DRV,");
		switch(vffi->dwFileSubtype) {
		default:
		case VFT2_UNKNOWN:
			TRACE("UNKNOWN(0x%lx)",vffi->dwFileSubtype);
			break;
		case VFT2_DRV_PRINTER:
			TRACE("PRINTER");
			break;
		case VFT2_DRV_KEYBOARD:
			TRACE("KEYBOARD");
			break;
		case VFT2_DRV_LANGUAGE:
			TRACE("LANGUAGE");
			break;
		case VFT2_DRV_DISPLAY:
			TRACE("DISPLAY");
			break;
		case VFT2_DRV_MOUSE:
			TRACE("MOUSE");
			break;
		case VFT2_DRV_NETWORK:
			TRACE("NETWORK");
			break;
		case VFT2_DRV_SYSTEM:
			TRACE("SYSTEM");
			break;
		case VFT2_DRV_INSTALLABLE:
			TRACE("INSTALLABLE");
			break;
		case VFT2_DRV_SOUND:
			TRACE("SOUND");
			break;
		case VFT2_DRV_COMM:
			TRACE("COMM");
			break;
		case VFT2_DRV_INPUTMETHOD:
			TRACE("INPUTMETHOD");
			break;
		}
		break;
	case VFT_FONT:
		TRACE("filetype=FONT.");
		switch (vffi->dwFileSubtype) {
		default:
			TRACE("UNKNOWN(0x%lx)",vffi->dwFileSubtype);
			break;
		case VFT2_FONT_RASTER:TRACE("RASTER");break;
		case VFT2_FONT_VECTOR:TRACE("VECTOR");break;
		case VFT2_FONT_TRUETYPE:TRACE("TRUETYPE");break;
		}
		break;
	case VFT_VXD:TRACE("filetype=VXD");break;
	case VFT_STATIC_LIB:TRACE("filetype=STATIC_LIB");break;
	}
	TRACE("\n");
	TRACE("  filedata=0x%lx.0x%lx\n",
		    vffi->dwFileDateMS,vffi->dwFileDateLS);
}


/***********************************************************************
 * Version Info Structure
 */

typedef struct
{
    WORD  wLength;
    WORD  wValueLength;
    WORD  bText;
    WCHAR szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT32 Children[];
#endif
} VS_VERSION_INFO_STRUCT32;
/*
#define VersionInfoIs16( ver ) \
    ( ((VS_VERSION_INFO_STRUCT16 *)ver)->szKey[0] >= ' ' )
*/
#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )
/*
#define VersionInfo16_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + strlen((ver)->szKey) + 1 )
*/
#define VersionInfo32_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + strlenW((ver)->szKey) + 1 )
/*
#define VersionInfo16_Children( ver )  \
    (VS_VERSION_INFO_STRUCT16 *)( VersionInfo16_Value( ver ) + \
                           ( ( (ver)->wValueLength + 3 ) & ~3 ) )
*/
#define VersionInfo32_Children( ver )  \
    (VS_VERSION_INFO_STRUCT32 *)( VersionInfo32_Value( ver ) + \
                           ( ( (ver)->wValueLength * \
                               ((ver)->bText? 2 : 1) + 3 ) & ~3 ) )

/*
#define VersionInfo16_Next( ver ) \
    (VS_VERSION_INFO_STRUCT16 *)( (LPBYTE)ver + (((ver)->wLength + 3) & ~3) )
*/
#define VersionInfo32_Next( ver ) \
    (VS_VERSION_INFO_STRUCT32 *)( (LPBYTE)ver + (((ver)->wLength + 3) & ~3) )

/***********************************************************************
 *           VERSION_GetFileVersionInfo_PE             [internal]
 *
 *    NOTE: returns size of the PE VERSION resource or 0xFFFFFFFF
 *    in the case if file exists, but VERSION_INFO not found.
 *    FIXME: handle is not used.
 */
static
DWORD
VERSION_GetFileVersionInfo_PE (
	LPCWSTR filename, LPDWORD handle,
	DWORD datasize, LPVOID data )
{
	VS_FIXEDFILEINFO *vffi;
	DWORD len;
	BYTE *buf;
	HMODULE hModule;
	HRSRC hRsrc;
	HGLOBAL hMem;

	TRACE("(%s,%p)\n", debugstr_w(filename), handle );

	hModule = GetModuleHandleW(filename);
	if(!hModule)
		hModule = LoadLibraryExW(filename, 0, LOAD_LIBRARY_AS_DATAFILE);
	else
		hModule = LoadLibraryExW(filename, 0, 0);
	if ( !hModule )
	{
		WARN("Could not load %s\n", debugstr_w(filename));
		return 0;
	}
	hRsrc = FindResourceW(hModule,
	                      MAKEINTRESOURCEW(VS_VERSION_INFO),
	                      MAKEINTRESOURCEW(VS_FILE_INFO));
	if ( !hRsrc )
	{
		WARN("Could not find VS_VERSION_INFO in %s\n", debugstr_w(filename));
		FreeLibrary(hModule);
		return 0xFFFFFFFF;
	}
	len = SizeofResource(hModule, hRsrc);
	hMem = LoadResource(hModule, hRsrc);
	if(!hMem)
	{
		WARN("Could not load VS_VERSION_INFO from %s\n", debugstr_a(filename));
		FreeLibrary(hModule);
		return 0xFFFFFFFF;
	}
	buf = LockResource(hMem);

	vffi = (VS_FIXEDFILEINFO *)VersionInfo32_Value( (VS_VERSION_INFO_STRUCT32 *)buf );

	if ( vffi->dwSignature != VS_FFI_SIGNATURE )
	{
		WARN("vffi->dwSignature is 0x%08lx, but not 0x%08lx!\n",
		     vffi->dwSignature, VS_FFI_SIGNATURE );
		len = 0xFFFFFFFF;
		goto END;
	}

	if ( TRACE_ON(ver) )
		print_vffi_debug( vffi );

	if(data)
	{
		if(datasize < len)
			len = datasize; /* truncate data */
		if(len)
			memcpy(data, buf, len);
		else
			len = 0xFFFFFFFF;
	}
END:
	FreeResource(hMem);
	FreeLibrary(hModule);

	return len;
}

/***********************************************************************
 *           GetFileVersionInfoSizeW         [VERSION.@]
 */
DWORD
WINAPI
GetFileVersionInfoSizeW ( LPWSTR filename, LPDWORD handle )
{
	DWORD len;

	TRACE("(%s,%p)\n", debugstr_w(filename), handle );
	if ( handle ) *handle = 0; //offset;

	len = VERSION_GetFileVersionInfo_PE ( filename, handle, 0, NULL );
	/* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
	if ( len == 0xFFFFFFFF )
	{
		SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
		return 0;
	}
	if ( len ) return (len<<1) + 4; // This matches MS's version.dll behavior, from what I can tell

	/* there used to be a bunch of 16-bit stuff here, but MS's docs say 16-bit not supported */

	return 0;
}

/***********************************************************************
 *           GetFileVersionInfoSizeA         [VERSION.@]
 */
DWORD
WINAPI
GetFileVersionInfoSizeA ( LPSTR filename, LPDWORD handle )
{
	LPWSTR filenameW = VERSION_AllocA2W(filename);
	DWORD ret = GetFileVersionInfoSizeW ( filenameW, handle );
	VERSION_Free ( filenameW );
	return ret;
}

/***********************************************************************
 *           GetFileVersionInfoW             [VERSION.@]
 */
BOOL
WINAPI
GetFileVersionInfoW (
	LPWSTR filename, DWORD handle,
	DWORD datasize, LPVOID data )
{
	DWORD len, extradata;
	VS_VERSION_INFO_STRUCT32* vvis = (VS_VERSION_INFO_STRUCT32*)data;

	TRACE("(%s,%ld,size=%ld,data=%p)\n",
		debugstr_w(filename), handle, datasize, data );

	len = VERSION_GetFileVersionInfo_PE(filename, &handle, datasize, data);

	if ( len == 0xFFFFFFFF )
		return FALSE;

	extradata = datasize - vvis->wLength;
	memmove ( ((char*)(data))+vvis->wLength, "FE2X", extradata > 4 ? 4 : extradata );

	return TRUE;
}

/***********************************************************************
 *           GetFileVersionInfoA             [VERSION.@]
 */
BOOL
WINAPI
GetFileVersionInfoA (
	LPSTR filename, DWORD handle,
	DWORD datasize, LPVOID data )
{
	LPWSTR filenameW = VERSION_AllocA2W ( filename );
	BOOL ret = GetFileVersionInfoW ( filenameW, handle, datasize, data );
	VERSION_Free ( filenameW );
	return ret;
}

/***********************************************************************
 *           VersionInfo32_FindChild             [internal]
 */
static
VS_VERSION_INFO_STRUCT32*
VersionInfo32_FindChild (
	VS_VERSION_INFO_STRUCT32 *info,
	LPCWSTR szKey, UINT cbKey )
{
	VS_VERSION_INFO_STRUCT32 *child = VersionInfo32_Children( info );

	while ( (DWORD)child < (DWORD)info + info->wLength )
	{
		if ( !strncmpiW( child->szKey, szKey, cbKey ) )
			return child;

		child = VersionInfo32_Next( child );
	}

	return NULL;
}

static
VS_VERSION_INFO_STRUCT32*
VERSION_VerQueryValue ( VS_VERSION_INFO_STRUCT32* info, LPCWSTR lpSubBlock )
{
	while ( *lpSubBlock )
	{
		/* Find next path component */
		LPCWSTR lpNextSlash;
		for ( lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++ )
		{
			if ( *lpNextSlash == '\\' )
				break;
		}

		/* Skip empty components */
		if ( lpNextSlash == lpSubBlock )
		{
			lpSubBlock++;
			continue;
		}

		/* We have a non-empty component: search info for key */
		info = VersionInfo32_FindChild ( info, lpSubBlock, lpNextSlash-lpSubBlock );
		if ( !info ) return NULL;

		/* Skip path component */
		lpSubBlock = lpNextSlash;
	}
	return info;
}

/***********************************************************************
 *           VerQueryValueW              [VERSION.@]
 */
BOOL
WINAPI
VerQueryValueW (
	const LPVOID pBlock,
	LPWSTR lpSubBlock,
	LPVOID *lplpBuffer,
	UINT *puLen )
{
	VS_VERSION_INFO_STRUCT32 *info = (VS_VERSION_INFO_STRUCT32 *)pBlock;

	TRACE("(%p,%s,%p,%p)\n",
		pBlock, debugstr_a(lpSubBlock), lplpBuffer, puLen );

	info = VERSION_VerQueryValue ( info, lpSubBlock );
	if ( !info )
	{
		// FIXME: what should SetLastError be set to???
		return FALSE;
	}

	*lplpBuffer = VersionInfo32_Value ( info );
	*puLen = info->wValueLength;

	return TRUE;
}

/***********************************************************************
 *           VerQueryValueA              [VERSION.@]
 */
BOOL
WINAPI
VerQueryValueA (
	LPVOID pBlock,
	LPSTR lpSubBlockA,
	LPVOID *lplpBuffer,
	UINT *puLen )
{
	VS_VERSION_INFO_STRUCT32 *info = (VS_VERSION_INFO_STRUCT32 *)pBlock;
	LPWSTR lpSubBlockW = VERSION_AllocA2W ( lpSubBlockA );

	TRACE("(%p,%s,%p,%p)\n",
		pBlock, debugstr_a(lpSubBlockA), lplpBuffer, puLen );

	info = VERSION_VerQueryValue ( info, lpSubBlockW );

	VERSION_Free ( lpSubBlockW );

	if ( !info )
	{
		// FIXME: what should SetLastError be set to???
		return FALSE;
	}

	*lplpBuffer = VersionInfo32_Value ( info );
	*puLen = info->wValueLength;

	if ( info->bText )
	{
		LPSTR str = VERSION_AllocW2A ( *lplpBuffer );
		memmove ( *lplpBuffer, str, info->wValueLength + 1 );
		VERSION_Free ( str );
	}

	return TRUE;
}
