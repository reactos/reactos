/*
 * Implementation of VERSION.DLL - Version Info access
 *
 * Copyright 1996,1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 * Copyright 1999 Ulrich Weigand
 * Copyright 2005 Paul Vriens
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
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "winternl.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);

/******************************************************************************
 *
 *   This function will print via standard TRACE, debug info regarding
 *   the file info structure vffi.
 *      15-Feb-1998 Dimitrie Paun (dimi@cs.toronto.edu)
 *      Added this function to clean up the code.
 *
 *****************************************************************************/
static void print_vffi_debug(const VS_FIXEDFILEINFO *vffi)
{
    BOOL    versioned_printer = FALSE;

    if((vffi->dwFileType == VFT_DLL) || (vffi->dwFileType == VFT_DRV))
    {
        if(vffi->dwFileSubtype == VFT2_DRV_VERSIONED_PRINTER)
            /* this is documented for newer w2k Drivers and up */
            versioned_printer = TRUE;
        else if( (vffi->dwFileSubtype == VFT2_DRV_PRINTER) &&
                 (vffi->dwFileVersionMS != vffi->dwProductVersionMS) &&
                 (vffi->dwFileVersionMS > 0) &&
                 (vffi->dwFileVersionMS <= 3) )
            /* found this on NT 3.51, NT4.0 and old w2k Drivers */
            versioned_printer = TRUE;
    }

    TRACE("structversion=%u.%u, ",
            HIWORD(vffi->dwStrucVersion),LOWORD(vffi->dwStrucVersion));
    if(versioned_printer)
    {
        WORD mode = LOWORD(vffi->dwFileVersionMS);
        WORD ver_rev = HIWORD(vffi->dwFileVersionLS);
        TRACE("fileversion=%u.%u.%u.%u (%s.major.minor.release), ",
            (vffi->dwFileVersionMS),
            HIBYTE(ver_rev), LOBYTE(ver_rev), LOWORD(vffi->dwFileVersionLS),
            (mode == 3) ? "Usermode" : ((mode <= 2) ? "Kernelmode" : "?") );
    }
    else
    {
        TRACE("fileversion=%u.%u.%u.%u, ",
            HIWORD(vffi->dwFileVersionMS),LOWORD(vffi->dwFileVersionMS),
            HIWORD(vffi->dwFileVersionLS),LOWORD(vffi->dwFileVersionLS));
    }
    TRACE("productversion=%u.%u.%u.%u\n",
          HIWORD(vffi->dwProductVersionMS),LOWORD(vffi->dwProductVersionMS),
          HIWORD(vffi->dwProductVersionLS),LOWORD(vffi->dwProductVersionLS));

    TRACE("flagmask=0x%x, flags=0x%x %s%s%s%s%s%s\n",
          vffi->dwFileFlagsMask, vffi->dwFileFlags,
          (vffi->dwFileFlags & VS_FF_DEBUG) ? "DEBUG," : "",
          (vffi->dwFileFlags & VS_FF_PRERELEASE) ? "PRERELEASE," : "",
          (vffi->dwFileFlags & VS_FF_PATCHED) ? "PATCHED," : "",
          (vffi->dwFileFlags & VS_FF_PRIVATEBUILD) ? "PRIVATEBUILD," : "",
          (vffi->dwFileFlags & VS_FF_INFOINFERRED) ? "INFOINFERRED," : "",
          (vffi->dwFileFlags & VS_FF_SPECIALBUILD) ? "SPECIALBUILD," : "");

    TRACE("(");

    TRACE("OS=0x%x.0x%x ", HIWORD(vffi->dwFileOS), LOWORD(vffi->dwFileOS));

    switch (vffi->dwFileOS&0xFFFF0000)
    {
    case VOS_DOS:TRACE("DOS,");break;
    case VOS_OS216:TRACE("OS/2-16,");break;
    case VOS_OS232:TRACE("OS/2-32,");break;
    case VOS_NT:TRACE("NT,");break;
    case VOS_UNKNOWN:
    default:
        TRACE("UNKNOWN(0x%x),",vffi->dwFileOS&0xFFFF0000);break;
    }

    switch (LOWORD(vffi->dwFileOS))
    {
    case VOS__BASE:TRACE("BASE");break;
    case VOS__WINDOWS16:TRACE("WIN16");break;
    case VOS__WINDOWS32:TRACE("WIN32");break;
    case VOS__PM16:TRACE("PM16");break;
    case VOS__PM32:TRACE("PM32");break;
    default:
        TRACE("UNKNOWN(0x%x)",LOWORD(vffi->dwFileOS));break;
    }

    TRACE(")\n");

    switch (vffi->dwFileType)
    {
    case VFT_APP:TRACE("filetype=APP");break;
    case VFT_DLL:
        TRACE("filetype=DLL");
        if(vffi->dwFileSubtype != 0)
        {
            if(versioned_printer) /* NT3.x/NT4.0 or old w2k Driver  */
                TRACE(",PRINTER");
            TRACE(" (subtype=0x%x)", vffi->dwFileSubtype);
        }
        break;
    case VFT_DRV:
        TRACE("filetype=DRV,");
        switch(vffi->dwFileSubtype)
        {
        case VFT2_DRV_PRINTER:TRACE("PRINTER");break;
        case VFT2_DRV_KEYBOARD:TRACE("KEYBOARD");break;
        case VFT2_DRV_LANGUAGE:TRACE("LANGUAGE");break;
        case VFT2_DRV_DISPLAY:TRACE("DISPLAY");break;
        case VFT2_DRV_MOUSE:TRACE("MOUSE");break;
        case VFT2_DRV_NETWORK:TRACE("NETWORK");break;
        case VFT2_DRV_SYSTEM:TRACE("SYSTEM");break;
        case VFT2_DRV_INSTALLABLE:TRACE("INSTALLABLE");break;
        case VFT2_DRV_SOUND:TRACE("SOUND");break;
        case VFT2_DRV_COMM:TRACE("COMM");break;
        case VFT2_DRV_INPUTMETHOD:TRACE("INPUTMETHOD");break;
        case VFT2_DRV_VERSIONED_PRINTER:TRACE("VERSIONED_PRINTER");break;
        case VFT2_UNKNOWN:
        default:
            TRACE("UNKNOWN(0x%x)",vffi->dwFileSubtype);break;
        }
        break;
    case VFT_FONT:
        TRACE("filetype=FONT,");
        switch (vffi->dwFileSubtype)
        {
        case VFT2_FONT_RASTER:TRACE("RASTER");break;
        case VFT2_FONT_VECTOR:TRACE("VECTOR");break;
        case VFT2_FONT_TRUETYPE:TRACE("TRUETYPE");break;
        default:TRACE("UNKNOWN(0x%x)",vffi->dwFileSubtype);break;
        }
        break;
    case VFT_VXD:TRACE("filetype=VXD");break;
    case VFT_STATIC_LIB:TRACE("filetype=STATIC_LIB");break;
    case VFT_UNKNOWN:
    default:
        TRACE("filetype=Unknown(0x%x)",vffi->dwFileType);break;
    }

    TRACE("\n");
    TRACE("filedate=0x%x.0x%x\n",vffi->dwFileDateMS,vffi->dwFileDateLS);
}

/***********************************************************************
 * Version Info Structure
 */

typedef struct
{
    WORD  wLength;
    WORD  wValueLength;
    CHAR  szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT16 Children[];
#endif
} VS_VERSION_INFO_STRUCT16;

typedef struct
{
    WORD  wLength;
    WORD  wValueLength;
    WORD  wType;
    WCHAR szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT32 Children[];
#endif
} VS_VERSION_INFO_STRUCT32;

#define VersionInfoIs16( ver ) \
    ( ((const VS_VERSION_INFO_STRUCT16 *)ver)->szKey[0] >= ' ' )

#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )

#define VersionInfo16_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + strlen((ver)->szKey) + 1 )
#define VersionInfo32_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + strlenW((ver)->szKey) + 1 )

#define VersionInfo16_Children( ver )  \
    (const VS_VERSION_INFO_STRUCT16 *)( VersionInfo16_Value( ver ) + \
                           ( ( (ver)->wValueLength + 3 ) & ~3 ) )
#define VersionInfo32_Children( ver )  \
    (const VS_VERSION_INFO_STRUCT32 *)( VersionInfo32_Value( ver ) + \
                           ( ( (ver)->wValueLength * \
                               ((ver)->wType? 2 : 1) + 3 ) & ~3 ) )

#define VersionInfo16_Next( ver ) \
    (VS_VERSION_INFO_STRUCT16 *)( (LPBYTE)ver + (((ver)->wLength + 3) & ~3) )
#define VersionInfo32_Next( ver ) \
    (VS_VERSION_INFO_STRUCT32 *)( (LPBYTE)ver + (((ver)->wLength + 3) & ~3) )

/***********************************************************************
 *           VERSION_GetFileVersionInfo_PE             [internal]
 *
 *    NOTE: returns size of the PE VERSION resource or 0xFFFFFFFF
 *    in the case the file is a PE module, but VERSION_INFO not found.
 */
static DWORD VERSION_GetFileVersionInfo_PE( LPCWSTR filename, DWORD datasize, LPVOID data )
{
    const VS_FIXEDFILEINFO *vffi;
    DWORD len;
    BYTE *buf;
    HMODULE hModule;
    HRSRC hRsrc;
    HGLOBAL hMem;

    TRACE("%s\n", debugstr_w(filename));

    if (!GetModuleHandleExW(0, filename, &hModule))
	hModule = LoadLibraryExW(filename, 0, LOAD_LIBRARY_AS_DATAFILE);

    if(!hModule)
    {
	WARN("Could not load %s\n", debugstr_w(filename));
	return 0;
    }
    hRsrc = FindResourceW(hModule,
			  MAKEINTRESOURCEW(VS_VERSION_INFO),
			  MAKEINTRESOURCEW(VS_FILE_INFO));
    if(!hRsrc)
    {
	WARN("Could not find VS_VERSION_INFO in %s\n", debugstr_w(filename));
	FreeLibrary(hModule);
	return 0xFFFFFFFF;
    }
    len = SizeofResource(hModule, hRsrc);
    hMem = LoadResource(hModule, hRsrc);
    if(!hMem)
    {
	WARN("Could not load VS_VERSION_INFO from %s\n", debugstr_w(filename));
	FreeLibrary(hModule);
	return 0xFFFFFFFF;
    }
    buf = LockResource(hMem);

    vffi = (VS_FIXEDFILEINFO *)VersionInfo32_Value( (VS_VERSION_INFO_STRUCT32 *)buf );

    if ( vffi->dwSignature != VS_FFI_SIGNATURE )
    {
        WARN("vffi->dwSignature is 0x%08x, but not 0x%08lx!\n",
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

#ifndef __REACTOS__
/***********************************************************************
 *           VERSION_GetFileVersionInfo_16             [internal]
 *
 *    NOTE: returns size of the 16-bit VERSION resource or 0xFFFFFFFF
 *    in the case the file exists, but VERSION_INFO not found.
 */
static DWORD VERSION_GetFileVersionInfo_16( LPCSTR filename, DWORD datasize, LPVOID data )
{
    const VS_FIXEDFILEINFO *vffi;
    DWORD len, offset;
    BYTE *buf;
    HMODULE16 hModule;
    HRSRC16 hRsrc;
    HGLOBAL16 hMem;
    char dllname[20], owner[20], *p;
    const char *basename;
    BOOL is_builtin = FALSE;

    TRACE("%s\n", debugstr_a(filename));

    /* strip path information */

    basename = filename;
    if (basename[0] && basename[1] == ':') basename += 2;  /* strip drive specification */
    if ((p = strrchr( basename, '\\' ))) basename = p + 1;
    if ((p = strrchr( basename, '/' ))) basename = p + 1;

    if (strlen(basename) < sizeof(dllname)-4)
    {
        int file_exists;

        strcpy( dllname, basename );
        p = strrchr( dllname, '.' );
        if (!p) strcat( dllname, ".dll" );
        for (p = dllname; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;

        if (wine_dll_get_owner( dllname, owner, sizeof(owner), &file_exists ) == 0)
            is_builtin = TRUE;
    }

    /* first try without loading a 16-bit module */
    if (is_builtin)
        len = 0;
    else
        len = GetFileResourceSize16( filename,
                                     MAKEINTRESOURCEA(VS_FILE_INFO),
                                     MAKEINTRESOURCEA(VS_VERSION_INFO),
                                     &offset );
    if (len)
    {
        if (!data) return len;

        len = GetFileResource16( filename,
                                 MAKEINTRESOURCEA(VS_FILE_INFO),
                                 MAKEINTRESOURCEA(VS_VERSION_INFO),
                                 offset, datasize, data );
        if (len)
        {
            vffi = (VS_FIXEDFILEINFO *)VersionInfo16_Value( (VS_VERSION_INFO_STRUCT16 *)data );

            if ( vffi->dwSignature == VS_FFI_SIGNATURE )
            {
                if ( ((VS_VERSION_INFO_STRUCT16 *)data)->wLength < len )
                    len = ((VS_VERSION_INFO_STRUCT16 *)data)->wLength;

                if ( TRACE_ON(ver) )
                    print_vffi_debug( vffi );

                return len;
            }
        }
    }

    /* this might be a builtin 16-bit module */
    hModule = LoadLibrary16(filename);
    if(hModule < 32)
    {
	WARN("Could not load %s\n", debugstr_a(filename));
	return 0;
    }
    hRsrc = FindResource16(hModule,
			  MAKEINTRESOURCEA(VS_VERSION_INFO),
			  MAKEINTRESOURCEA(VS_FILE_INFO));
    if(!hRsrc)
    {
	WARN("Could not find VS_VERSION_INFO in %s\n", debugstr_a(filename));
	FreeLibrary16(hModule);
	return 0xFFFFFFFF;
    }
    len = SizeofResource16(hModule, hRsrc);
    hMem = LoadResource16(hModule, hRsrc);
    if(!hMem)
    {
	WARN("Could not load VS_VERSION_INFO from %s\n", debugstr_a(filename));
	FreeLibrary16(hModule);
	return 0xFFFFFFFF;
    }
    buf = LockResource16(hMem);

    if(!VersionInfoIs16(buf))
    {
        len = 0xFFFFFFFF;
	goto END;
    }

    vffi = (VS_FIXEDFILEINFO *)VersionInfo16_Value( (VS_VERSION_INFO_STRUCT16 *)buf );

    if ( vffi->dwSignature != VS_FFI_SIGNATURE )
    {
        WARN("vffi->dwSignature is 0x%08x, but not 0x%08lx!\n",
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
    FreeResource16(hMem);
    FreeLibrary16(hModule);

    return len;
}
#endif /* ! __REACTOS__ */

/***********************************************************************
 *           GetFileVersionInfoSizeW         [VERSION.@]
 */
DWORD WINAPI GetFileVersionInfoSizeW( LPCWSTR filename, LPDWORD handle )
{
    DWORD len;

    TRACE("(%s,%p)\n", debugstr_w(filename), handle );

    if (handle) *handle = 0;

    if (!filename)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!*filename)
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return 0;
    }

    len = VERSION_GetFileVersionInfo_PE(filename, 0, NULL);
    /* 0xFFFFFFFF means: file is a PE module, but VERSION_INFO not found */
    if(len == 0xFFFFFFFF)
    {
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return 0;
    }

    if (!len)
    {
#ifdef __REACTOS__
        SetLastError(ERROR_FILE_NOT_FOUND);
        return 0;
#else /* __REACTOS__ */
        LPSTR filenameA;

        len = WideCharToMultiByte( CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL );
        filenameA = HeapAlloc( GetProcessHeap(), 0, len );
        WideCharToMultiByte( CP_ACP, 0, filename, -1, filenameA, len, NULL, NULL );

        len = VERSION_GetFileVersionInfo_16(filenameA, 0, NULL);
        HeapFree( GetProcessHeap(), 0, filenameA );
        /* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
        if (!len)
        {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return 0;
        }
        if (len == 0xFFFFFFFF)
        {
            SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
            return 0;
        }

        /* We have a 16bit resource.
         *
         * XP/W2K/W2K3 uses a buffer which is more than the actual needed space:
         *
         * (info->wLength - sizeof(VS_FIXEDFILEINFO)) * 4
         *
         * This extra buffer is used for ANSI to Unicode conversions in W-Calls.
         * info->wLength should be the same as len. Currently it isn't but that
         * doesn't seem to be a problem (len is bigger than info->wLength).
         */
         len = (len - sizeof(VS_FIXEDFILEINFO)) * 4;
#endif /* ! __REACTOS__ */
    }
    else
    {
        /* We have a 32bit resource.
         *
         * XP/W2K/W2K3 uses a buffer which is 2 times the actual needed space + 4 bytes "FE2X"
         * This extra buffer is used for Unicode to ANSI conversions in A-Calls
         */
         len = (len * 2) + 4;
    }

    SetLastError(0);
    return len;
}

/***********************************************************************
 *           GetFileVersionInfoSizeA         [VERSION.@]
 */
DWORD WINAPI GetFileVersionInfoSizeA( LPCSTR filename, LPDWORD handle )
{
    UNICODE_STRING filenameW;
    DWORD retval;

    TRACE("(%s,%p)\n", debugstr_a(filename), handle );

    if(filename)
        RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else
        filenameW.Buffer = NULL;

    retval = GetFileVersionInfoSizeW(filenameW.Buffer, handle);

    RtlFreeUnicodeString(&filenameW);

    return retval;
}

/***********************************************************************
 *           GetFileVersionInfoW             [VERSION.@]
 */
BOOL WINAPI GetFileVersionInfoW( LPCWSTR filename, DWORD handle,
                                    DWORD datasize, LPVOID data )
{
    DWORD len;
    VS_VERSION_INFO_STRUCT32* vvis = (VS_VERSION_INFO_STRUCT32*)data;

    TRACE("(%s,%d,size=%d,data=%p)\n",
                debugstr_w(filename), handle, datasize, data );

    if (!data)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    len = VERSION_GetFileVersionInfo_PE(filename, datasize, data);
    /* 0xFFFFFFFF means: file is a PE module, but VERSION_INFO not found */
    if (len == 0xFFFFFFFF)
    {
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
    }

    if (!len)
    {
#ifdef __REACTOS__
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
#else /* __REACTOS__ */
        LPSTR filenameA;

        len = WideCharToMultiByte( CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL );
        filenameA = HeapAlloc( GetProcessHeap(), 0, len );
        WideCharToMultiByte( CP_ACP, 0, filename, -1, filenameA, len, NULL, NULL );

        len = VERSION_GetFileVersionInfo_16(filenameA, datasize, data);
        HeapFree( GetProcessHeap(), 0, filenameA );
        /* 0xFFFFFFFF means: file exists, but VERSION_INFO not found */
        if (!len || len == 0xFFFFFFFF)
        {
            SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
            return FALSE;
        }
        /* We have a 16bit resource. */
#endif /* ! __REACTOS__ */
    }
    else 
    {
        static const char signature[] = "FE2X";
        DWORD bufsize = vvis->wLength + strlen(signature);
        DWORD convbuf;
 
        /* We have a 32bit resource.
         *
         * XP/W2K/W2K3 uses a buffer which is 2 times the actual needed space + 4 bytes "FE2X"
         * This extra buffer is used for Unicode to ANSI conversions in A-Calls
         */

        /* information is truncated to datasize bytes */
        if (datasize >= bufsize)
        {
            convbuf = datasize - vvis->wLength;
            memcpy( ((char*)(data))+vvis->wLength, signature, convbuf > 4 ? 4 : convbuf );
        }
    }

    SetLastError(0);
    return TRUE;
}

/***********************************************************************
 *           GetFileVersionInfoA             [VERSION.@]
 */
BOOL WINAPI GetFileVersionInfoA( LPCSTR filename, DWORD handle,
                                    DWORD datasize, LPVOID data )
{
    UNICODE_STRING filenameW;
    BOOL retval;

    TRACE("(%s,%d,size=%d,data=%p)\n",
                debugstr_a(filename), handle, datasize, data );

    if(filename)
        RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else
        filenameW.Buffer = NULL;

    retval = GetFileVersionInfoW(filenameW.Buffer, handle, datasize, data);

    RtlFreeUnicodeString(&filenameW);

    return retval;
}

/***********************************************************************
 *           VersionInfo16_FindChild             [internal]
 */
static const VS_VERSION_INFO_STRUCT16 *VersionInfo16_FindChild( const VS_VERSION_INFO_STRUCT16 *info,
                                            LPCSTR szKey, UINT cbKey )
{
    const VS_VERSION_INFO_STRUCT16 *child = VersionInfo16_Children( info );

    while ((char *)child < (char *)info + info->wLength )
    {
        if (!strncasecmp( child->szKey, szKey, cbKey ) && !child->szKey[cbKey])
            return child;

	if (!(child->wLength)) return NULL;
        child = VersionInfo16_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VersionInfo32_FindChild             [internal]
 */
static const VS_VERSION_INFO_STRUCT32 *VersionInfo32_FindChild( const VS_VERSION_INFO_STRUCT32 *info,
                                            LPCWSTR szKey, UINT cbKey )
{
    const VS_VERSION_INFO_STRUCT32 *child = VersionInfo32_Children( info );

    while ((char *)child < (char *)info + info->wLength )
    {
        if (!strncmpiW( child->szKey, szKey, cbKey ) && !child->szKey[cbKey])
            return child;

        child = VersionInfo32_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VersionInfo16_QueryValue              [internal]
 *
 *    Gets a value from a 16-bit NE resource
 */
static BOOL WINAPI VersionInfo16_QueryValue( const VS_VERSION_INFO_STRUCT16 *info, LPCSTR lpSubBlock,
                               LPVOID *lplpBuffer, UINT *puLen )
{
    while ( *lpSubBlock )
    {
        /* Find next path component */
        LPCSTR lpNextSlash;
        for ( lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++ )
            if ( *lpNextSlash == '\\' )
                break;

        /* Skip empty components */
        if ( lpNextSlash == lpSubBlock )
        {
            lpSubBlock++;
            continue;
        }

        /* We have a non-empty component: search info for key */
        info = VersionInfo16_FindChild( info, lpSubBlock, lpNextSlash-lpSubBlock );
        if ( !info )
        {
            if (puLen) *puLen = 0 ;
            SetLastError( ERROR_RESOURCE_TYPE_NOT_FOUND );
            return FALSE;
        }

        /* Skip path component */
        lpSubBlock = lpNextSlash;
    }

    /* Return value */
    *lplpBuffer = VersionInfo16_Value( info );
    if (puLen)
        *puLen = info->wValueLength;

    return TRUE;
}

/***********************************************************************
 *           VersionInfo32_QueryValue              [internal]
 *
 *    Gets a value from a 32-bit PE resource
 */
static BOOL WINAPI VersionInfo32_QueryValue( const VS_VERSION_INFO_STRUCT32 *info, LPCWSTR lpSubBlock,
                               LPVOID *lplpBuffer, UINT *puLen )
{
    TRACE("lpSubBlock : (%s)\n", debugstr_w(lpSubBlock));

    while ( *lpSubBlock )
    {
        /* Find next path component */
        LPCWSTR lpNextSlash;
        for ( lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++ )
            if ( *lpNextSlash == '\\' )
                break;

        /* Skip empty components */
        if ( lpNextSlash == lpSubBlock )
        {
            lpSubBlock++;
            continue;
        }

        /* We have a non-empty component: search info for key */
        info = VersionInfo32_FindChild( info, lpSubBlock, lpNextSlash-lpSubBlock );
        if ( !info )
        {
            if (puLen) *puLen = 0 ;
            SetLastError( ERROR_RESOURCE_TYPE_NOT_FOUND );
            return FALSE;
        }

        /* Skip path component */
        lpSubBlock = lpNextSlash;
    }

    /* Return value */
    *lplpBuffer = VersionInfo32_Value( info );
    if (puLen)
        *puLen = info->wValueLength;

    return TRUE;
}

/***********************************************************************
 *           VerQueryValueA              [VERSION.@]
 */
BOOL WINAPI VerQueryValueA( LPCVOID pBlock, LPCSTR lpSubBlock,
                               LPVOID *lplpBuffer, PUINT puLen )
{
    static const char rootA[] = "\\";
    static const char varfileinfoA[] = "\\VarFileInfo\\Translation";
    const VS_VERSION_INFO_STRUCT16 *info = (const VS_VERSION_INFO_STRUCT16 *)pBlock;

    TRACE("(%p,%s,%p,%p)\n",
                pBlock, debugstr_a(lpSubBlock), lplpBuffer, puLen );

     if (!pBlock)
        return FALSE;

    if ( !VersionInfoIs16( info ) )
    {
        BOOL ret;
        INT len;
        LPWSTR lpSubBlockW;

        len  = MultiByteToWideChar(CP_ACP, 0, lpSubBlock, -1, NULL, 0);
        lpSubBlockW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

        if (!lpSubBlockW)
            return FALSE;

        MultiByteToWideChar(CP_ACP, 0, lpSubBlock, -1, lpSubBlockW, len);

        ret = VersionInfo32_QueryValue(pBlock, lpSubBlockW, lplpBuffer, puLen);

        HeapFree(GetProcessHeap(), 0, lpSubBlockW);

        if (ret && strcasecmp( lpSubBlock, rootA ) && strcasecmp( lpSubBlock, varfileinfoA ))
        {
            /* Set lpBuffer so it points to the 'empty' area where we store
             * the converted strings
             */
            LPSTR lpBufferA = (LPSTR)pBlock + info->wLength + 4;
            DWORD pos = (LPCSTR)*lplpBuffer - (LPCSTR)pBlock;

            len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)*lplpBuffer, -1,
                                      lpBufferA + pos, info->wLength - pos, NULL, NULL);
            *lplpBuffer = lpBufferA + pos;
            *puLen = len;
        }
        return ret;
    }

    return VersionInfo16_QueryValue(info, lpSubBlock, lplpBuffer, puLen);
}

/***********************************************************************
 *           VerQueryValueW              [VERSION.@]
 */
BOOL WINAPI VerQueryValueW( LPCVOID pBlock, LPCWSTR lpSubBlock,
                               LPVOID *lplpBuffer, PUINT puLen )
{
    static const WCHAR rootW[] = { '\\', 0 };
    static const WCHAR varfileinfoW[] = { '\\','V','a','r','F','i','l','e','I','n','f','o',
                                          '\\','T','r','a','n','s','l','a','t','i','o','n', 0 };

    const VS_VERSION_INFO_STRUCT32 *info = (const VS_VERSION_INFO_STRUCT32 *)pBlock;

    TRACE("(%p,%s,%p,%p)\n",
                pBlock, debugstr_w(lpSubBlock), lplpBuffer, puLen );

    if (!pBlock)
        return FALSE;

    if ( VersionInfoIs16( info ) )
    {
        BOOL ret;
        int len;
        LPSTR lpSubBlockA;

        len = WideCharToMultiByte(CP_ACP, 0, lpSubBlock, -1, NULL, 0, NULL, NULL);
        lpSubBlockA = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));

        if (!lpSubBlockA)
            return FALSE;

        WideCharToMultiByte(CP_ACP, 0, lpSubBlock, -1, lpSubBlockA, len, NULL, NULL);

        ret = VersionInfo16_QueryValue(pBlock, lpSubBlockA, lplpBuffer, puLen);

        HeapFree(GetProcessHeap(), 0, lpSubBlockA);

        if (ret && strcmpiW( lpSubBlock, rootW ) && strcmpiW( lpSubBlock, varfileinfoW ))
        {
            /* Set lpBuffer so it points to the 'empty' area where we store
             * the converted strings
             */
            LPWSTR lpBufferW = (LPWSTR)((LPSTR)pBlock + info->wLength);
            DWORD pos = (LPCSTR)*lplpBuffer - (LPCSTR)pBlock;
            DWORD max = (info->wLength - sizeof(VS_FIXEDFILEINFO)) * 4 - info->wLength;

            len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*lplpBuffer, -1,
                                      lpBufferW + pos, max/sizeof(WCHAR) - pos );
            *lplpBuffer = lpBufferW + pos;
            *puLen = len;
        }
        return ret;
    }

    return VersionInfo32_QueryValue(info, lpSubBlock, lplpBuffer, puLen);
}
