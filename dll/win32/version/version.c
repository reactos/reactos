/*
 * Implementation of VERSION.DLL
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
#include <stdio.h>

#include <sys/types.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "winuser.h"
#include "winnls.h"
#include "wine/winternl.h"
#include "lzexpand.h"
#include "winerror.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(ver);

typedef struct
{
    WORD offset;
    WORD length;
    WORD flags;
    WORD id;
    WORD handle;
    WORD usage;
} NE_NAMEINFO;

typedef struct
{
    WORD  type_id;
    WORD  count;
    DWORD resloader;
} NE_TYPEINFO;

/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 * Copied from loader/pe_resource.c
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         WORD id, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int min, max, pos;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    min = dir->NumberOfNamedEntries;
    max = min + dir->NumberOfIdEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (entry[pos].u.Id == id)
            return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry[pos].u2.s2.OffsetToDirectory);
        if (entry[pos].u.Id > id) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}


/**********************************************************************
 *  find_entry_default
 *
 * Find a default entry in a resource directory
 * Copied from loader/pe_resource.c
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_default( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry->u2.s2.OffsetToDirectory);
}


/**********************************************************************
 *  push_language
 *
 * push a language onto the list of languages to try
 */
static inline int push_language( WORD *list, int pos, WORD lang )
{
    int i;
    for (i = 0; i < pos; i++) if (list[i] == lang) return pos;
    list[pos++] = lang;
    return pos;
}


/**********************************************************************
 *  find_entry_language
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_language( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                            const void *root, DWORD flags )
{
    const IMAGE_RESOURCE_DIRECTORY *ret;
    WORD list[9];
    int i, pos = 0;

    if (flags & FILE_VER_GET_LOCALISED)
    {
        /* cf. LdrFindResource_U */
        pos = push_language( list, pos, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
        pos = push_language( list, pos, LANGIDFROMLCID( NtCurrentTeb()->CurrentLocale ) );
        pos = push_language( list, pos, GetUserDefaultLangID() );
        pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(GetUserDefaultLangID()), SUBLANG_NEUTRAL ));
        pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(GetUserDefaultLangID()), SUBLANG_DEFAULT ));
        pos = push_language( list, pos, GetSystemDefaultLangID() );
        pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()), SUBLANG_NEUTRAL ));
        pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()), SUBLANG_DEFAULT ));
        pos = push_language( list, pos, MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    }
    else
    {
        /* FIXME: resolve LN file here */
        pos = push_language( list, pos, MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    }

    for (i = 0; i < pos; i++) if ((ret = find_entry_by_id( dir, list[i], root ))) return ret;
    return find_entry_default( dir, root );
}


/***********************************************************************
 *           read_xx_header         [internal]
 */
static int read_xx_header( HFILE lzfd )
{
    IMAGE_DOS_HEADER mzh;
    char magic[3];

    LZSeek( lzfd, 0, SEEK_SET );
    if ( sizeof(mzh) != LZRead( lzfd, (LPSTR)&mzh, sizeof(mzh) ) )
        return 0;
    if ( mzh.e_magic != IMAGE_DOS_SIGNATURE )
    {
        if (!memcmp( &mzh, "\177ELF", 4 )) return 1;  /* ELF */
        if (*(UINT *)&mzh == 0xfeedface || *(UINT *)&mzh == 0xcefaedfe) return 1;  /* Mach-O */
        return 0;
    }

    LZSeek( lzfd, mzh.e_lfanew, SEEK_SET );
    if ( 2 != LZRead( lzfd, magic, 2 ) )
        return 0;

    LZSeek( lzfd, mzh.e_lfanew, SEEK_SET );

    if ( magic[0] == 'N' && magic[1] == 'E' )
        return IMAGE_OS2_SIGNATURE;
    if ( magic[0] == 'P' && magic[1] == 'E' )
        return IMAGE_NT_SIGNATURE;

    magic[2] = '\0';
    WARN("Can't handle %s files.\n", magic );
    return 0;
}

/***********************************************************************
 *           find_ne_resource         [internal]
 */
static BOOL find_ne_resource( HFILE lzfd, DWORD *resLen, DWORD *resOff )
{
    const WORD typeid = VS_FILE_INFO | 0x8000;
    const WORD resid = VS_VERSION_INFO | 0x8000;
    IMAGE_OS2_HEADER nehd;
    NE_TYPEINFO *typeInfo;
    NE_NAMEINFO *nameInfo;
    DWORD nehdoffset;
    LPBYTE resTab;
    DWORD resTabSize;
    int count;

    /* Read in NE header */
    nehdoffset = LZSeek( lzfd, 0, SEEK_CUR );
    if ( sizeof(nehd) != LZRead( lzfd, (LPSTR)&nehd, sizeof(nehd) ) ) return FALSE;

    resTabSize = nehd.ne_restab - nehd.ne_rsrctab;
    if ( !resTabSize )
    {
        TRACE("No resources in NE dll\n" );
        return FALSE;
    }

    /* Read in resource table */
    resTab = HeapAlloc( GetProcessHeap(), 0, resTabSize );
    if ( !resTab ) return FALSE;

    LZSeek( lzfd, nehd.ne_rsrctab + nehdoffset, SEEK_SET );
    if ( resTabSize != LZRead( lzfd, (char*)resTab, resTabSize ) )
    {
        HeapFree( GetProcessHeap(), 0, resTab );
        return FALSE;
    }

    /* Find resource */
    typeInfo = (NE_TYPEINFO *)(resTab + 2);
    while (typeInfo->type_id)
    {
        if (typeInfo->type_id == typeid) goto found_type;
        typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                   typeInfo->count * sizeof(NE_NAMEINFO));
    }
    TRACE("No typeid entry found\n" );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_type:
    nameInfo = (NE_NAMEINFO *)(typeInfo + 1);

    for (count = typeInfo->count; count > 0; count--, nameInfo++)
        if (nameInfo->id == resid) goto found_name;

    TRACE("No resid entry found\n" );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_name:
    /* Return resource data */
    if ( resLen ) *resLen = nameInfo->length << *(WORD *)resTab;
    if ( resOff ) *resOff = nameInfo->offset << *(WORD *)resTab;

    HeapFree( GetProcessHeap(), 0, resTab );
    return TRUE;
}

/***********************************************************************
 *           find_pe_resource         [internal]
 */
static BOOL find_pe_resource( HFILE lzfd, DWORD *resLen, DWORD *resOff, DWORD flags )
{
    union
    {
        IMAGE_NT_HEADERS32 nt32;
        IMAGE_NT_HEADERS64 nt64;
    } pehd;
    DWORD pehdoffset;
    PIMAGE_DATA_DIRECTORY resDataDir;
    PIMAGE_SECTION_HEADER sections;
    LPBYTE resSection;
    DWORD section_size, data_size;
    const void *resDir;
    const IMAGE_RESOURCE_DIRECTORY *resPtr;
    const IMAGE_RESOURCE_DATA_ENTRY *resData;
    int i, len, nSections;
    BOOL ret = FALSE;

    /* Read in PE header */
    pehdoffset = LZSeek( lzfd, 0, SEEK_CUR );
    len = LZRead( lzfd, (LPSTR)&pehd, sizeof(pehd) );
    if (len < sizeof(pehd.nt32.FileHeader)) return FALSE;
    if (len < sizeof(pehd)) memset( (char *)&pehd + len, 0, sizeof(pehd) - len );

    switch (pehd.nt32.OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        resDataDir = pehd.nt32.OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_RESOURCE;
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        resDataDir = pehd.nt64.OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_RESOURCE;
        break;
    default:
        return FALSE;
    }

    if ( !resDataDir->Size )
    {
        TRACE("No resources in PE dll\n" );
        return FALSE;
    }

    /* Read in section table */
    nSections = pehd.nt32.FileHeader.NumberOfSections;
    sections = HeapAlloc( GetProcessHeap(), 0,
                          nSections * sizeof(IMAGE_SECTION_HEADER) );
    if ( !sections ) return FALSE;

    len = FIELD_OFFSET( IMAGE_NT_HEADERS32, OptionalHeader ) + pehd.nt32.FileHeader.SizeOfOptionalHeader;
    LZSeek( lzfd, pehdoffset + len, SEEK_SET );

    if ( nSections * sizeof(IMAGE_SECTION_HEADER) !=
         LZRead( lzfd, (LPSTR)sections, nSections * sizeof(IMAGE_SECTION_HEADER) ) )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    /* Find resource section */
    for ( i = 0; i < nSections; i++ )
        if (    resDataDir->VirtualAddress >= sections[i].VirtualAddress
             && resDataDir->VirtualAddress <  sections[i].VirtualAddress +
                                              sections[i].SizeOfRawData )
            break;

    if ( i == nSections )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        TRACE("Couldn't find resource section\n" );
        return FALSE;
    }

    /* Read in resource section */
    data_size = sections[i].SizeOfRawData;
    section_size = max( data_size, sections[i].Misc.VirtualSize );
    resSection = HeapAlloc( GetProcessHeap(), 0, section_size );
    if ( !resSection )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    LZSeek( lzfd, sections[i].PointerToRawData, SEEK_SET );
    if (data_size != LZRead( lzfd, (char*)resSection, data_size )) goto done;
    if (data_size < section_size) memset( (char *)resSection + data_size, 0, section_size - data_size );

    /* Find resource */
    resDir = resSection + (resDataDir->VirtualAddress - sections[i].VirtualAddress);

    resPtr = resDir;
    resPtr = find_entry_by_id( resPtr, VS_FILE_INFO, resDir );
    if ( !resPtr )
    {
        TRACE("No typeid entry found\n" );
        goto done;
    }
    resPtr = find_entry_by_id( resPtr, VS_VERSION_INFO, resDir );
    if ( !resPtr )
    {
        TRACE("No resid entry found\n" );
        goto done;
    }
    resPtr = find_entry_language( resPtr, resDir, flags );
    if ( !resPtr )
    {
        TRACE("No default language entry found\n" );
        goto done;
    }

    /* Find resource data section */
    resData = (const IMAGE_RESOURCE_DATA_ENTRY*)resPtr;
    for ( i = 0; i < nSections; i++ )
        if (    resData->OffsetToData >= sections[i].VirtualAddress
             && resData->OffsetToData <  sections[i].VirtualAddress +
                                         sections[i].SizeOfRawData )
            break;

    if ( i == nSections )
    {
        TRACE("Couldn't find resource data section\n" );
        goto done;
    }

    /* Return resource data */
    if ( resLen ) *resLen = resData->Size;
    if ( resOff ) *resOff = resData->OffsetToData - sections[i].VirtualAddress
                            + sections[i].PointerToRawData;
    ret = TRUE;

 done:
    HeapFree( GetProcessHeap(), 0, resSection );
    HeapFree( GetProcessHeap(), 0, sections );
    return ret;
}


/***********************************************************************
 *           find_version_resource         [internal]
 */
static DWORD find_version_resource( HFILE lzfd, DWORD *reslen, DWORD *offset, DWORD flags )
{
    DWORD magic = read_xx_header( lzfd );

    switch (magic)
    {
    case IMAGE_OS2_SIGNATURE:
        if (!find_ne_resource( lzfd, reslen, offset )) magic = 0;
        break;
    case IMAGE_NT_SIGNATURE:
        if (!find_pe_resource( lzfd, reslen, offset, flags )) magic = 0;
        break;
    }
    return magic;
}

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
    WORD  wType; /* 1:Text, 0:Binary */
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
    DWORD_ALIGN( (ver), (ver)->szKey + lstrlenW((ver)->szKey) + 1 )

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
 *           GetFileVersionInfoSizeW         [VERSION.@]
 */
DWORD WINAPI GetFileVersionInfoSizeW( LPCWSTR filename, LPDWORD handle )
{
    return GetFileVersionInfoSizeExW( FILE_VER_GET_LOCALISED, filename, handle );
}

/***********************************************************************
 *           GetFileVersionInfoSizeA         [VERSION.@]
 */
DWORD WINAPI GetFileVersionInfoSizeA( LPCSTR filename, LPDWORD handle )
{
    return GetFileVersionInfoSizeExA( FILE_VER_GET_LOCALISED, filename, handle );
}

/******************************************************************************
 *           GetFileVersionInfoSizeExW       [VERSION.@]
 */
DWORD WINAPI GetFileVersionInfoSizeExW( DWORD flags, LPCWSTR filename, LPDWORD handle )
{
    DWORD len, offset, magic = 1;
    HFILE lzfd;
    HMODULE hModule;
    OFSTRUCT ofs;

    TRACE("(0x%x,%s,%p)\n", flags, debugstr_w(filename), handle );

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
    if (flags & ~FILE_VER_GET_LOCALISED)
        FIXME("flags 0x%x ignored\n", flags & ~FILE_VER_GET_LOCALISED);

    if ((lzfd = LZOpenFileW( (LPWSTR)filename, &ofs, OF_READ )) != HFILE_ERROR)
    {
        magic = find_version_resource( lzfd, &len, &offset, flags );
        LZClose( lzfd );
    }

    if ((magic == 1) && (hModule = LoadLibraryExW( filename, 0, LOAD_LIBRARY_AS_DATAFILE )))
    {
        HRSRC hRsrc = NULL;
        if (!(flags & FILE_VER_GET_LOCALISED))
        {
            LANGID english = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
            hRsrc = FindResourceExW( hModule, MAKEINTRESOURCEW(VS_VERSION_INFO),
                                     (LPWSTR)VS_FILE_INFO, english );
        }
        if (!hRsrc)
            hRsrc = FindResourceW( hModule, MAKEINTRESOURCEW(VS_VERSION_INFO),
                                   (LPWSTR)VS_FILE_INFO );
        if (hRsrc)
        {
            magic = IMAGE_NT_SIGNATURE;
            len = SizeofResource( hModule, hRsrc );
        }
        FreeLibrary( hModule );
    }

    switch (magic)
    {
    case IMAGE_OS2_SIGNATURE:
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
        SetLastError(0);
        return (len - sizeof(VS_FIXEDFILEINFO)) * 4;

    case IMAGE_NT_SIGNATURE:
        /* We have a 32bit resource.
         *
         * XP/W2K/W2K3 uses a buffer which is 2 times the actual needed space + 4 bytes "FE2X"
         * This extra buffer is used for Unicode to ANSI conversions in A-Calls
         */
        SetLastError(0);
        return (len * 2) + 4;

    default:
        if (lzfd == HFILE_ERROR)
            SetLastError(ofs.nErrCode);
        else if (GetVersion() & 0x80000000) /* Windows 95/98 */
            SetLastError(ERROR_FILE_NOT_FOUND);
        else
            SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return 0;
    }
}

/******************************************************************************
 *           GetFileVersionInfoSizeExA       [VERSION.@]
 */
DWORD WINAPI GetFileVersionInfoSizeExA( DWORD flags, LPCSTR filename, LPDWORD handle )
{
    UNICODE_STRING filenameW;
    DWORD retval;

    TRACE("(0x%x,%s,%p)\n", flags, debugstr_a(filename), handle );

    if(filename)
        RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else
        filenameW.Buffer = NULL;

    retval = GetFileVersionInfoSizeExW(flags, filenameW.Buffer, handle);

    RtlFreeUnicodeString(&filenameW);

    return retval;
}

/***********************************************************************
 *           GetFileVersionInfoExW           [VERSION.@]
 */
BOOL WINAPI GetFileVersionInfoExW( DWORD flags, LPCWSTR filename, DWORD handle, DWORD datasize, LPVOID data )
{
    static const char signature[4] = "FE2X";
    DWORD len, offset, magic = 1;
    HFILE lzfd;
    OFSTRUCT ofs;
    HMODULE hModule;
    VS_VERSION_INFO_STRUCT32* vvis = data;

    TRACE("(0x%x,%s,%d,size=%d,data=%p)\n",
          flags, debugstr_w(filename), handle, datasize, data );

    if (!data)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    if (flags & ~FILE_VER_GET_LOCALISED)
        FIXME("flags 0x%x ignored\n", flags & ~FILE_VER_GET_LOCALISED);

    if ((lzfd = LZOpenFileW( (LPWSTR)filename, &ofs, OF_READ )) != HFILE_ERROR)
    {
        if ((magic = find_version_resource( lzfd, &len, &offset, flags )) > 1)
        {
            LZSeek( lzfd, offset, 0 /* SEEK_SET */ );
            len = LZRead( lzfd, data, min( len, datasize ) );
        }
        LZClose( lzfd );
    }

    if ((magic == 1) && (hModule = LoadLibraryExW( filename, 0, LOAD_LIBRARY_AS_DATAFILE )))
    {
        HRSRC hRsrc = NULL;
        if (!(flags & FILE_VER_GET_LOCALISED))
        {
            LANGID english = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
            hRsrc = FindResourceExW( hModule, MAKEINTRESOURCEW(VS_VERSION_INFO),
                                     (LPWSTR)VS_FILE_INFO, english );
        }
        if (!hRsrc)
            hRsrc = FindResourceW( hModule, MAKEINTRESOURCEW(VS_VERSION_INFO),
                                   (LPWSTR)VS_FILE_INFO );
        if (hRsrc)
        {
            HGLOBAL hMem = LoadResource( hModule, hRsrc );
            magic = IMAGE_NT_SIGNATURE;
            len = min( SizeofResource(hModule, hRsrc), datasize );
            memcpy( data, LockResource( hMem ), len );
            FreeResource( hMem );
        }
        FreeLibrary( hModule );
    }

    switch (magic)
    {
    case IMAGE_OS2_SIGNATURE:
        /* We have a 16bit resource. */
        if (TRACE_ON(ver))
            print_vffi_debug( (VS_FIXEDFILEINFO *)VersionInfo16_Value( (VS_VERSION_INFO_STRUCT16 *)data ));
        SetLastError(0);
        return TRUE;

    case IMAGE_NT_SIGNATURE:
        /* We have a 32bit resource.
         *
         * XP/W2K/W2K3 uses a buffer which is 2 times the actual needed space + 4 bytes "FE2X"
         * This extra buffer is used for Unicode to ANSI conversions in A-Calls
         */
        len = vvis->wLength + sizeof(signature);
        if (datasize >= len) memcpy( (char*)data + vvis->wLength, signature, sizeof(signature) );
        if (TRACE_ON(ver))
            print_vffi_debug( (VS_FIXEDFILEINFO *)VersionInfo32_Value( vvis ));
        SetLastError(0);
        return TRUE;

    default:
        SetLastError( lzfd == HFILE_ERROR ? ofs.nErrCode : ERROR_RESOURCE_DATA_NOT_FOUND );
        return FALSE;
    }
}

/***********************************************************************
 *           GetFileVersionInfoExA           [VERSION.@]
 */
BOOL WINAPI GetFileVersionInfoExA( DWORD flags, LPCSTR filename, DWORD handle, DWORD datasize, LPVOID data )
{
    UNICODE_STRING filenameW;
    BOOL retval;

    TRACE("(0x%x,%s,%d,size=%d,data=%p)\n",
          flags, debugstr_a(filename), handle, datasize, data );

    if(filename)
        RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else
        filenameW.Buffer = NULL;

    retval = GetFileVersionInfoExW(flags, filenameW.Buffer, handle, datasize, data);

    RtlFreeUnicodeString(&filenameW);

    return retval;
}

/***********************************************************************
 *           GetFileVersionInfoW             [VERSION.@]
 */
BOOL WINAPI GetFileVersionInfoW( LPCWSTR filename, DWORD handle, DWORD datasize, LPVOID data )
{
    return GetFileVersionInfoExW(FILE_VER_GET_LOCALISED, filename, handle, datasize, data);
}

/***********************************************************************
 *           GetFileVersionInfoA             [VERSION.@]
 */
BOOL WINAPI GetFileVersionInfoA( LPCSTR filename, DWORD handle, DWORD datasize, LPVOID data )
{
    return GetFileVersionInfoExA(FILE_VER_GET_LOCALISED, filename, handle, datasize, data);
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
        if (!_strnicmp( child->szKey, szKey, cbKey ) && !child->szKey[cbKey])
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
        if (!_wcsnicmp( child->szKey, szKey, cbKey ) && !child->szKey[cbKey])
            return child;

        if (!(child->wLength)) return NULL;
        child = VersionInfo32_Next( child );
    }

    return NULL;
}

/***********************************************************************
 *           VersionInfo16_QueryValue              [internal]
 *
 *    Gets a value from a 16-bit NE resource
 */
static BOOL VersionInfo16_QueryValue( const VS_VERSION_INFO_STRUCT16 *info, LPCSTR lpSubBlock,
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
static BOOL VersionInfo32_QueryValue( const VS_VERSION_INFO_STRUCT32 *info, LPCWSTR lpSubBlock,
                                      LPVOID *lplpBuffer, UINT *puLen, BOOL *pbText )
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

#ifdef __REACTOS__
    /* If the wValueLength is zero, then set a UNICODE_NULL only return string.
     * Use the NULL terminator from the key string for that. This is what Windows does, too. */
    if (!info->wValueLength)
      *lplpBuffer = (PVOID)(info->szKey + wcslen(info->szKey));
#endif

    if (puLen)
        *puLen = info->wValueLength;
    if (pbText)
        *pbText = info->wType;

    return TRUE;
}

/***********************************************************************
 *           VerQueryValueA              [VERSION.@]
 */
BOOL WINAPI VerQueryValueA( LPCVOID pBlock, LPCSTR lpSubBlock,
                               LPVOID *lplpBuffer, PUINT puLen )
{
    static const char rootA[] = "\\";
    const VS_VERSION_INFO_STRUCT16 *info = pBlock;

    TRACE("(%p,%s,%p,%p)\n",
                pBlock, debugstr_a(lpSubBlock), lplpBuffer, puLen );

     if (!pBlock)
        return FALSE;

    if (lpSubBlock == NULL || lpSubBlock[0] == '\0')
        lpSubBlock = rootA;

    if ( !VersionInfoIs16( info ) )
    {
        BOOL ret, isText;
        INT len;
        LPWSTR lpSubBlockW;
        UINT value_len;

        len  = MultiByteToWideChar(CP_ACP, 0, lpSubBlock, -1, NULL, 0);
        lpSubBlockW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

        if (!lpSubBlockW)
            return FALSE;

        MultiByteToWideChar(CP_ACP, 0, lpSubBlock, -1, lpSubBlockW, len);

        ret = VersionInfo32_QueryValue(pBlock, lpSubBlockW, lplpBuffer, &value_len, &isText);
        if (puLen) *puLen = value_len;

        HeapFree(GetProcessHeap(), 0, lpSubBlockW);

        if (ret && isText)
        {
            /* Set lpBuffer so it points to the 'empty' area where we store
             * the converted strings
             */
            LPSTR lpBufferA = (LPSTR)pBlock + info->wLength + 4;
            DWORD pos = (LPCSTR)*lplpBuffer - (LPCSTR)pBlock;
            len = WideCharToMultiByte(CP_ACP, 0, *lplpBuffer, value_len,
                                      lpBufferA + pos, info->wLength - pos, NULL, NULL);
            *lplpBuffer = lpBufferA + pos;
            if (puLen) *puLen = len;
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

    const VS_VERSION_INFO_STRUCT32 *info = pBlock;

    TRACE("(%p,%s,%p,%p)\n",
                pBlock, debugstr_w(lpSubBlock), lplpBuffer, puLen );

    if (!pBlock)
        return FALSE;

    if (!lpSubBlock || !lpSubBlock[0])
        lpSubBlock = rootW;

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

        if (ret && wcsicmp( lpSubBlock, rootW ) && wcsicmp( lpSubBlock, varfileinfoW ))
        {
            /* Set lpBuffer so it points to the 'empty' area where we store
             * the converted strings
             */
            LPWSTR lpBufferW = (LPWSTR)((LPSTR)pBlock + info->wLength);
            DWORD pos = (LPCSTR)*lplpBuffer - (LPCSTR)pBlock;
            DWORD max = (info->wLength - sizeof(VS_FIXEDFILEINFO)) * 4 - info->wLength;

            len = MultiByteToWideChar(CP_ACP, 0, *lplpBuffer, -1,
                                      lpBufferW + pos, max/sizeof(WCHAR) - pos );
            *lplpBuffer = lpBufferW + pos;
            if (puLen) *puLen = len;
        }
        return ret;
    }

    return VersionInfo32_QueryValue(info, lpSubBlock, lplpBuffer, puLen, NULL);
}


/******************************************************************************
 *   testFileExistenceA
 *
 *   Tests whether a given path/file combination exists.  If the file does
 *   not exist, the return value is zero.  If it does exist, the return
 *   value is non-zero.
 *
 *   Revision history
 *      30-May-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Original implementation
 *
 */
static int testFileExistenceA( char const * path, char const * file, BOOL excl )
{
    char  filename[1024];
    int  filenamelen;
    OFSTRUCT  fileinfo;

    fileinfo.cBytes = sizeof(OFSTRUCT);

    if (path)
    {
        strcpy(filename, path);
        filenamelen = strlen(filename);

        /* Add a trailing \ if necessary */
        if(filenamelen)
        {
            if(filename[filenamelen - 1] != '\\')
                strcat(filename, "\\");
        }
        else /* specify the current directory */
            strcpy(filename, ".\\");
    }
    else
        filename[0] = 0;

    /* Create the full pathname */
    strcat(filename, file);

    return (OpenFile(filename, &fileinfo,
                     OF_EXIST | (excl ? OF_SHARE_EXCLUSIVE : 0)) != HFILE_ERROR);
}

/******************************************************************************
 *   testFileExistenceW
 */
static int testFileExistenceW( const WCHAR *path, const WCHAR *file, BOOL excl )
{
    char *filename;
    DWORD pathlen, filelen;
    int ret;
    OFSTRUCT fileinfo;

    fileinfo.cBytes = sizeof(OFSTRUCT);

    pathlen = WideCharToMultiByte( CP_ACP, 0, path, -1, NULL, 0, NULL, NULL );
    filelen = WideCharToMultiByte( CP_ACP, 0, file, -1, NULL, 0, NULL, NULL );
    filename = HeapAlloc( GetProcessHeap(), 0, pathlen+filelen+2 );

    WideCharToMultiByte( CP_ACP, 0, path, -1, filename, pathlen, NULL, NULL );
    /* Add a trailing \ if necessary */
    if (pathlen > 1)
    {
        if (filename[pathlen-2] != '\\') strcpy( &filename[pathlen-1], "\\" );
    }
    else /* specify the current directory */
        strcpy(filename, ".\\");

    WideCharToMultiByte( CP_ACP, 0, file, -1, filename+strlen(filename), filelen, NULL, NULL );

    ret = (OpenFile(filename, &fileinfo,
                    OF_EXIST | (excl ? OF_SHARE_EXCLUSIVE : 0)) != HFILE_ERROR);
    HeapFree( GetProcessHeap(), 0, filename );
    return ret;
}

/*****************************************************************************
 *   VerFindFileA [VERSION.@]
 *
 *   Determines where to install a file based on whether it locates another
 *   version of the file in the system.  The values VerFindFile returns are
 *   used in a subsequent call to the VerInstallFile function.
 *
 *   Revision history:
 *      30-May-1997   Dave Cuthbert (dacut@ece.cmu.edu)
 *         Reimplementation of VerFindFile from original stub.
 */
DWORD WINAPI VerFindFileA(
    DWORD flags,
    LPCSTR lpszFilename,
    LPCSTR lpszWinDir,
    LPCSTR lpszAppDir,
    LPSTR lpszCurDir,
    PUINT lpuCurDirLen,
    LPSTR lpszDestDir,
    PUINT lpuDestDirLen )
{
    DWORD  retval = 0;
    const char *curDir;
    const char *destDir;
    unsigned int  curDirSizeReq;
    unsigned int  destDirSizeReq;
    char winDir[MAX_PATH], systemDir[MAX_PATH];

    /* Print out debugging information */
    TRACE("flags = %x filename=%s windir=%s appdir=%s curdirlen=%p(%u) destdirlen=%p(%u)\n",
          flags, debugstr_a(lpszFilename), debugstr_a(lpszWinDir), debugstr_a(lpszAppDir),
          lpuCurDirLen, lpuCurDirLen ? *lpuCurDirLen : 0,
          lpuDestDirLen, lpuDestDirLen ? *lpuDestDirLen : 0 );

    /* Figure out where the file should go; shared files default to the
       system directory */

    GetSystemDirectoryA(systemDir, sizeof(systemDir));
    curDir = "";

    if(flags & VFFF_ISSHAREDFILE)
    {
        destDir = systemDir;
        /* Were we given a filename?  If so, try to find the file. */
        if(lpszFilename)
        {
            if(testFileExistenceA(destDir, lpszFilename, FALSE)) curDir = destDir;
            else if(lpszAppDir && testFileExistenceA(lpszAppDir, lpszFilename, FALSE))
                curDir = lpszAppDir;

            if(!testFileExistenceA(systemDir, lpszFilename, FALSE))
                retval |= VFF_CURNEDEST;
        }
    }
    else /* not a shared file */
    {
        destDir = lpszAppDir ? lpszAppDir : "";
        if(lpszFilename)
        {
            GetWindowsDirectoryA( winDir, MAX_PATH );
            if(testFileExistenceA(destDir, lpszFilename, FALSE)) curDir = destDir;
            else if(testFileExistenceA(winDir, lpszFilename, FALSE))
                curDir = winDir;
            else if(testFileExistenceA(systemDir, lpszFilename, FALSE))
                curDir = systemDir;

            if (lpszAppDir && lpszAppDir[0])
            {
                if(!testFileExistenceA(lpszAppDir, lpszFilename, FALSE))
                    retval |= VFF_CURNEDEST;
            }
            else if(testFileExistenceA(NULL, lpszFilename, FALSE))
                retval |= VFF_CURNEDEST;
        }
    }

    /* Check to see if the file exists and is in use by another application */
    if (lpszFilename && testFileExistenceA(curDir, lpszFilename, FALSE)) {
        if (lpszFilename && !testFileExistenceA(curDir, lpszFilename, TRUE))
           retval |= VFF_FILEINUSE;
    }

    curDirSizeReq = strlen(curDir) + 1;
    destDirSizeReq = strlen(destDir) + 1;

    /* Make sure that the pointers to the size of the buffers are
       valid; if not, do NOTHING with that buffer.  If that pointer
       is valid, then make sure that the buffer pointer is valid, too! */

    if(lpuDestDirLen && lpszDestDir)
    {
        if (*lpuDestDirLen < destDirSizeReq) retval |= VFF_BUFFTOOSMALL;
        lstrcpynA(lpszDestDir, destDir, *lpuDestDirLen);
        *lpuDestDirLen = destDirSizeReq;
    }
    if(lpuCurDirLen && lpszCurDir)
    {
        if(*lpuCurDirLen < curDirSizeReq) retval |= VFF_BUFFTOOSMALL;
        lstrcpynA(lpszCurDir, curDir, *lpuCurDirLen);
        *lpuCurDirLen = curDirSizeReq;
    }

    TRACE("ret = %u (%s%s%s) curdir=%s destdir=%s\n", retval,
          (retval & VFF_CURNEDEST) ? "VFF_CURNEDEST " : "",
          (retval & VFF_FILEINUSE) ? "VFF_FILEINUSE " : "",
          (retval & VFF_BUFFTOOSMALL) ? "VFF_BUFFTOOSMALL " : "",
          debugstr_a(lpszCurDir), debugstr_a(lpszDestDir));

    return retval;
}

/*****************************************************************************
 * VerFindFileW						[VERSION.@]
 */
DWORD WINAPI VerFindFileW( DWORD flags,LPCWSTR lpszFilename,LPCWSTR lpszWinDir,
                           LPCWSTR lpszAppDir, LPWSTR lpszCurDir,PUINT lpuCurDirLen,
                           LPWSTR lpszDestDir,PUINT lpuDestDirLen )
{
    static const WCHAR emptyW;
    DWORD retval = 0;
    const WCHAR *curDir;
    const WCHAR *destDir;
    unsigned int curDirSizeReq;
    unsigned int destDirSizeReq;
    WCHAR winDir[MAX_PATH], systemDir[MAX_PATH];

    /* Print out debugging information */
    TRACE("flags = %x filename=%s windir=%s appdir=%s curdirlen=%p(%u) destdirlen=%p(%u)\n",
          flags, debugstr_w(lpszFilename), debugstr_w(lpszWinDir), debugstr_w(lpszAppDir),
          lpuCurDirLen, lpuCurDirLen ? *lpuCurDirLen : 0,
          lpuDestDirLen, lpuDestDirLen ? *lpuDestDirLen : 0 );

    /* Figure out where the file should go; shared files default to the
       system directory */

    GetSystemDirectoryW(systemDir, ARRAY_SIZE(systemDir));
    curDir = &emptyW;

    if(flags & VFFF_ISSHAREDFILE)
    {
        destDir = systemDir;
        /* Were we given a filename?  If so, try to find the file. */
        if(lpszFilename)
        {
            if(testFileExistenceW(destDir, lpszFilename, FALSE)) curDir = destDir;
            else if(lpszAppDir && testFileExistenceW(lpszAppDir, lpszFilename, FALSE))
            {
                curDir = lpszAppDir;
                retval |= VFF_CURNEDEST;
            }
        }
    }
    else /* not a shared file */
    {
        destDir = lpszAppDir ? lpszAppDir : &emptyW;
        if(lpszFilename)
        {
            GetWindowsDirectoryW( winDir, MAX_PATH );
            if(testFileExistenceW(destDir, lpszFilename, FALSE)) curDir = destDir;
            else if(testFileExistenceW(winDir, lpszFilename, FALSE))
            {
                curDir = winDir;
                retval |= VFF_CURNEDEST;
            }
            else if(testFileExistenceW(systemDir, lpszFilename, FALSE))
            {
                curDir = systemDir;
                retval |= VFF_CURNEDEST;
            }
        }
    }

    if (lpszFilename && !testFileExistenceW(curDir, lpszFilename, TRUE))
        retval |= VFF_FILEINUSE;

    curDirSizeReq = lstrlenW(curDir) + 1;
    destDirSizeReq = lstrlenW(destDir) + 1;

    /* Make sure that the pointers to the size of the buffers are
       valid; if not, do NOTHING with that buffer.  If that pointer
       is valid, then make sure that the buffer pointer is valid, too! */

    if(lpuDestDirLen && lpszDestDir)
    {
        if (*lpuDestDirLen < destDirSizeReq) retval |= VFF_BUFFTOOSMALL;
        lstrcpynW(lpszDestDir, destDir, *lpuDestDirLen);
        *lpuDestDirLen = destDirSizeReq;
    }
    if(lpuCurDirLen && lpszCurDir)
    {
        if(*lpuCurDirLen < curDirSizeReq) retval |= VFF_BUFFTOOSMALL;
        lstrcpynW(lpszCurDir, curDir, *lpuCurDirLen);
        *lpuCurDirLen = curDirSizeReq;
    }

    TRACE("ret = %u (%s%s%s) curdir=%s destdir=%s\n", retval,
          (retval & VFF_CURNEDEST) ? "VFF_CURNEDEST " : "",
          (retval & VFF_FILEINUSE) ? "VFF_FILEINUSE " : "",
          (retval & VFF_BUFFTOOSMALL) ? "VFF_BUFFTOOSMALL " : "",
          debugstr_w(lpszCurDir), debugstr_w(lpszDestDir));
    return retval;
}

static LPBYTE
_fetch_versioninfo(LPSTR fn,VS_FIXEDFILEINFO **vffi) {
    DWORD	alloclen;
    LPBYTE	buf;
    DWORD	ret;

    alloclen = 1000;
    buf=HeapAlloc(GetProcessHeap(), 0, alloclen);
    if(buf == NULL) {
        WARN("Memory exhausted while fetching version info!\n");
        return NULL;
    }
    while (1) {
	ret = GetFileVersionInfoA(fn,0,alloclen,buf);
	if (!ret) {
	    HeapFree(GetProcessHeap(), 0, buf);
	    return NULL;
	}
	if (alloclen<*(WORD*)buf) {
	    alloclen = *(WORD*)buf;
	    HeapFree(GetProcessHeap(), 0, buf);
	    buf = HeapAlloc(GetProcessHeap(), 0, alloclen);
            if(buf == NULL) {
               WARN("Memory exhausted while fetching version info!\n");
               return NULL;
            }
	} else {
	    *vffi = (VS_FIXEDFILEINFO*)(buf+0x14);
	    if ((*vffi)->dwSignature == 0x004f0049) /* hack to detect unicode */
		*vffi = (VS_FIXEDFILEINFO*)(buf+0x28);
	    if ((*vffi)->dwSignature != VS_FFI_SIGNATURE)
                WARN("Bad VS_FIXEDFILEINFO signature 0x%08x\n",(*vffi)->dwSignature);
	    return buf;
	}
    }
}

static DWORD
_error2vif(DWORD error) {
    switch (error) {
    case ERROR_ACCESS_DENIED:
	return VIF_ACCESSVIOLATION;
    case ERROR_SHARING_VIOLATION:
	return VIF_SHARINGVIOLATION;
    default:
	return 0;
    }
}


/******************************************************************************
 * VerInstallFileA [VERSION.@]
 */
DWORD WINAPI VerInstallFileA(
	DWORD flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPCSTR curdir,LPSTR tmpfile,PUINT tmpfilelen )
{
    LPCSTR pdest;
    char	destfn[260],tmpfn[260],srcfn[260];
    HFILE	hfsrc,hfdst;
    DWORD	attr,xret,tmplast;
    LONG	ret;
    LPBYTE	buf1,buf2;
    OFSTRUCT	ofs;

    TRACE("(%x,%s,%s,%s,%s,%s,%p,%d)\n",
          flags,debugstr_a(srcfilename),debugstr_a(destfilename),
          debugstr_a(srcdir),debugstr_a(destdir),debugstr_a(curdir),
          tmpfile,*tmpfilelen);
    xret = 0;
    if (!srcdir || !srcfilename) return VIF_CANNOTREADSRC;
    sprintf(srcfn,"%s\\%s",srcdir,srcfilename);
    if (!destdir || !*destdir) pdest = srcdir;
    else pdest = destdir;
    sprintf(destfn,"%s\\%s",pdest,destfilename);
    hfsrc=LZOpenFileA(srcfn,&ofs,OF_READ);
    if (hfsrc < 0)
	return VIF_CANNOTREADSRC;
    sprintf(tmpfn,"%s\\%s",pdest,destfilename);
    tmplast=strlen(pdest)+1;
    attr = GetFileAttributesA(tmpfn);
    if (attr != INVALID_FILE_ATTRIBUTES) {
	if (attr & FILE_ATTRIBUTE_READONLY) {
	    LZClose(hfsrc);
	    return VIF_WRITEPROT;
	}
	/* FIXME: check if file currently in use and return VIF_FILEINUSE */
    }
    attr = INVALID_FILE_ATTRIBUTES;
    if (flags & VIFF_FORCEINSTALL) {
	if (tmpfile[0]) {
	    sprintf(tmpfn,"%s\\%s",pdest,tmpfile);
	    tmplast = strlen(pdest)+1;
	    attr = GetFileAttributesA(tmpfn);
	    /* if it exists, it has been copied by the call before.
	     * we jump over the copy part...
	     */
	}
    }
    if (attr == INVALID_FILE_ATTRIBUTES) {
	char	*s;

	GetTempFileNameA(pdest,"ver",0,tmpfn); /* should not fail ... */
	s=strrchr(tmpfn,'\\');
	if (s)
	    tmplast = s-tmpfn;
	else
	    tmplast = 0;
	hfdst = OpenFile(tmpfn,&ofs,OF_CREATE);
	if (hfdst == HFILE_ERROR) {
	    LZClose(hfsrc);
	    return VIF_CANNOTCREATE; /* | translated dos error */
	}
	ret = LZCopy(hfsrc,hfdst);
	_lclose(hfdst);
	if (ret < 0) {
	    /* translate LZ errors into VIF_xxx */
	    switch (ret) {
	    case LZERROR_BADINHANDLE:
	    case LZERROR_READ:
	    case LZERROR_BADVALUE:
	    case LZERROR_UNKNOWNALG:
		xret = VIF_CANNOTREADSRC;
		break;
	    case LZERROR_BADOUTHANDLE:
	    case LZERROR_WRITE:
		xret = VIF_OUTOFSPACE;
		break;
	    case LZERROR_GLOBALLOC:
	    case LZERROR_GLOBLOCK:
		xret = VIF_OUTOFMEMORY;
		break;
	    default: /* unknown error, should not happen */
		FIXME("Unknown LZCopy error %d, ignoring.\n", ret);
		xret = 0;
		break;
	    }
	    if (xret) {
		LZClose(hfsrc);
		return xret;
	    }
	}
    }
    if (!(flags & VIFF_FORCEINSTALL)) {
	VS_FIXEDFILEINFO *destvffi,*tmpvffi;
	buf1 = _fetch_versioninfo(destfn,&destvffi);
	if (buf1) {
	    buf2 = _fetch_versioninfo(tmpfn,&tmpvffi);
	    if (buf2) {
		char	*tbuf1,*tbuf2;
		static const CHAR trans_array[] = "\\VarFileInfo\\Translation";
		UINT	len1,len2;

		len1=len2=40;

		/* compare file versions */
		if ((destvffi->dwFileVersionMS > tmpvffi->dwFileVersionMS)||
		    ((destvffi->dwFileVersionMS==tmpvffi->dwFileVersionMS)&&
		     (destvffi->dwFileVersionLS > tmpvffi->dwFileVersionLS)
		    )
		)
		    xret |= VIF_MISMATCH|VIF_SRCOLD;
		/* compare filetypes and filesubtypes */
		if ((destvffi->dwFileType!=tmpvffi->dwFileType) ||
		    (destvffi->dwFileSubtype!=tmpvffi->dwFileSubtype)
		)
		    xret |= VIF_MISMATCH|VIF_DIFFTYPE;
		if (VerQueryValueA(buf1,trans_array,(LPVOID*)&tbuf1,&len1) &&
		    VerQueryValueA(buf2,trans_array,(LPVOID*)&tbuf2,&len2)
		) {
                    /* Do something with tbuf1 and tbuf2
		     * generates DIFFLANG|MISMATCH
		     */
		}
		HeapFree(GetProcessHeap(), 0, buf2);
	    } else
		xret=VIF_MISMATCH|VIF_SRCOLD;
	    HeapFree(GetProcessHeap(), 0, buf1);
	}
    }
    if (xret) {
	if (*tmpfilelen<strlen(tmpfn+tmplast)) {
	    xret|=VIF_BUFFTOOSMALL;
	    DeleteFileA(tmpfn);
	} else {
	    strcpy(tmpfile,tmpfn+tmplast);
	    *tmpfilelen = strlen(tmpfn+tmplast)+1;
	    xret|=VIF_TEMPFILE;
	}
    } else {
	if (INVALID_FILE_ATTRIBUTES!=GetFileAttributesA(destfn))
	    if (!DeleteFileA(destfn)) {
		xret|=_error2vif(GetLastError())|VIF_CANNOTDELETE;
		DeleteFileA(tmpfn);
		LZClose(hfsrc);
		return xret;
	    }
	if ((!(flags & VIFF_DONTDELETEOLD))	&&
	    curdir				&&
	    *curdir				&&
	    lstrcmpiA(curdir,pdest)
	) {
	    char curfn[260];

	    sprintf(curfn,"%s\\%s",curdir,destfilename);
	    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesA(curfn)) {
		/* FIXME: check if in use ... if it is, VIF_CANNOTDELETECUR */
		if (!DeleteFileA(curfn))
		    xret|=_error2vif(GetLastError())|VIF_CANNOTDELETECUR;
	    }
	}
	if (!MoveFileA(tmpfn,destfn)) {
	    xret|=_error2vif(GetLastError())|VIF_CANNOTRENAME;
	    DeleteFileA(tmpfn);
	}
    }
    LZClose(hfsrc);
    return xret;
}


/******************************************************************************
 * VerInstallFileW				[VERSION.@]
 */
DWORD WINAPI VerInstallFileW(
	DWORD flags,LPCWSTR srcfilename,LPCWSTR destfilename,LPCWSTR srcdir,
	LPCWSTR destdir,LPCWSTR curdir,LPWSTR tmpfile,PUINT tmpfilelen )
{
    LPSTR wsrcf = NULL, wsrcd = NULL, wdestf = NULL, wdestd = NULL, wtmpf = NULL, wcurd = NULL;
    DWORD ret = 0;
    UINT len;

    if (srcfilename)
    {
        len = WideCharToMultiByte( CP_ACP, 0, srcfilename, -1, NULL, 0, NULL, NULL );
        if ((wsrcf = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, srcfilename, -1, wsrcf, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (srcdir && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, srcdir, -1, NULL, 0, NULL, NULL );
        if ((wsrcd = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, srcdir, -1, wsrcd, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (destfilename && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, destfilename, -1, NULL, 0, NULL, NULL );
        if ((wdestf = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, destfilename, -1, wdestf, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (destdir && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, destdir, -1, NULL, 0, NULL, NULL );
        if ((wdestd = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, destdir, -1, wdestd, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (curdir && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, curdir, -1, NULL, 0, NULL, NULL );
        if ((wcurd = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, curdir, -1, wcurd, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (!ret)
    {
        len = *tmpfilelen * sizeof(WCHAR);
        wtmpf = HeapAlloc( GetProcessHeap(), 0, len );
        if (!wtmpf)
            ret = VIF_OUTOFMEMORY;
    }
    if (!ret)
        ret = VerInstallFileA(flags,wsrcf,wdestf,wsrcd,wdestd,wcurd,wtmpf,&len);
    if (!ret)
        *tmpfilelen = MultiByteToWideChar( CP_ACP, 0, wtmpf, -1, tmpfile, *tmpfilelen );
    else if (ret & VIF_BUFFTOOSMALL)
        *tmpfilelen = len;  /* FIXME: not correct */

    HeapFree( GetProcessHeap(), 0, wsrcf );
    HeapFree( GetProcessHeap(), 0, wsrcd );
    HeapFree( GetProcessHeap(), 0, wdestf );
    HeapFree( GetProcessHeap(), 0, wdestd );
    HeapFree( GetProcessHeap(), 0, wtmpf );
    HeapFree( GetProcessHeap(), 0, wcurd );
    return ret;
}
