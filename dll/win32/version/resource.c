/*
 * Implementation of VERSION.DLL - Resource Access routines
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "lzexpand.h"

#include "wine/unicode.h"
#include "wine/winbase16.h"

#include "wine/debug.h"

/* winnt.h */
#define IMAGE_FILE_RESOURCE_DIRECTORY		2

WINE_DEFAULT_DEBUG_CHANNEL(ver);


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
        if (entry[pos].u1.s2.Id == id)
            return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry[pos].u2.s3.OffsetToDirectory);
        if (entry[pos].u1.s2.Id > id) max = pos - 1;
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
    return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry->u2.s3.OffsetToDirectory);
}


/**********************************************************************
 *  find_entry_by_name
 *
 * Find an entry by name in a resource directory
 * Copied from loader/pe_resource.c
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           LPCSTR name, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY *ret = NULL;
    LPWSTR nameW;
    DWORD namelen;

    if (!HIWORD(name)) return find_entry_by_id( dir, LOWORD(name), root );
    if (name[0] == '#')
    {
        return find_entry_by_id( dir, atoi(name+1), root );
    }

    namelen = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
    if ((nameW = HeapAlloc( GetProcessHeap(), 0, namelen * sizeof(WCHAR) )))
    {
        const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
        const IMAGE_RESOURCE_DIR_STRING_U *str;
        int min, max, res, pos;

        MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, namelen );
        namelen--;  /* remove terminating null */
        entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
        min = 0;
        max = dir->NumberOfNamedEntries - 1;
        while (min <= max)
        {
            pos = (min + max) / 2;
            str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const char *)root + entry[pos].u1.s1.NameOffset);
            res = strncmpiW( nameW, str->NameString, str->Length );
            if (!res && namelen == str->Length)
            {
                ret = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry[pos].u2.s3.OffsetToDirectory);
                break;
            }
            if (res < 0) max = pos - 1;
            else min = pos + 1;
        }
        HeapFree( GetProcessHeap(), 0, nameW );
    }
    return ret;
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
        return 0;

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

#ifndef __REACTOS__
/***********************************************************************
 *           find_ne_resource         [internal]
 */
static BOOL find_ne_resource( HFILE lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_OS2_HEADER nehd;
    NE_TYPEINFO *typeInfo;
    NE_NAMEINFO *nameInfo;
    DWORD nehdoffset;
    LPBYTE resTab;
    DWORD resTabSize;
    int count;

    /* Read in NE header */
    nehdoffset = LZSeek( lzfd, 0, SEEK_CUR );
    if ( sizeof(nehd) != LZRead( lzfd, (LPSTR)&nehd, sizeof(nehd) ) ) return 0;

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

    if (HIWORD(typeid) != 0)  /* named type */
    {
        BYTE len = strlen( typeid );
        while (typeInfo->type_id)
        {
            if (!(typeInfo->type_id & 0x8000))
            {
                BYTE *p = resTab + typeInfo->type_id;
                if ((*p == len) && !strncasecmp( (char*)p+1, typeid, len )) goto found_type;
            }
            typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                       typeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    else  /* numeric type id */
    {
        WORD id = LOWORD(typeid) | 0x8000;
        while (typeInfo->type_id)
        {
            if (typeInfo->type_id == id) goto found_type;
            typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                       typeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    TRACE("No typeid entry found for %p\n", typeid );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_type:
    nameInfo = (NE_NAMEINFO *)(typeInfo + 1);

    if (HIWORD(resid) != 0)  /* named resource */
    {
        BYTE len = strlen( resid );
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
        {
            BYTE *p = resTab + nameInfo->id;
            if (nameInfo->id & 0x8000) continue;
            if ((*p == len) && !strncasecmp( (char*)p+1, resid, len )) goto found_name;
        }
    }
    else  /* numeric resource id */
    {
        WORD id = LOWORD(resid) | 0x8000;
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
            if (nameInfo->id == id) goto found_name;
    }
    TRACE("No resid entry found for %p\n", typeid );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_name:
    /* Return resource data */
    if ( resLen ) *resLen = nameInfo->length << *(WORD *)resTab;
    if ( resOff ) *resOff = nameInfo->offset << *(WORD *)resTab;

    HeapFree( GetProcessHeap(), 0, resTab );
    return TRUE;
}
#endif /* ! __REACTOS__ */

/***********************************************************************
 *           find_pe_resource         [internal]
 */
static BOOL find_pe_resource( HFILE lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_NT_HEADERS pehd;
    DWORD pehdoffset;
    PIMAGE_DATA_DIRECTORY resDataDir;
    PIMAGE_SECTION_HEADER sections;
    LPBYTE resSection;
    DWORD resSectionSize;
    const void *resDir;
    const IMAGE_RESOURCE_DIRECTORY *resPtr;
    const IMAGE_RESOURCE_DATA_ENTRY *resData;
    int i, nSections;
    BOOL ret = FALSE;

    /* Read in PE header */
    pehdoffset = LZSeek( lzfd, 0, SEEK_CUR );
    if ( sizeof(pehd) != LZRead( lzfd, (LPSTR)&pehd, sizeof(pehd) ) ) return 0;

    resDataDir = pehd.OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_RESOURCE;
    if ( !resDataDir->Size )
    {
        TRACE("No resources in PE dll\n" );
        return FALSE;
    }

    /* Read in section table */
    nSections = pehd.FileHeader.NumberOfSections;
    sections = HeapAlloc( GetProcessHeap(), 0,
                          nSections * sizeof(IMAGE_SECTION_HEADER) );
    if ( !sections ) return FALSE;

    LZSeek( lzfd, pehdoffset +
                    sizeof(DWORD) + /* Signature */
                    sizeof(IMAGE_FILE_HEADER) +
                    pehd.FileHeader.SizeOfOptionalHeader, SEEK_SET );

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
    resSectionSize = sections[i].SizeOfRawData;
    resSection = HeapAlloc( GetProcessHeap(), 0, resSectionSize );
    if ( !resSection )
    {
        HeapFree( GetProcessHeap(), 0, sections );
        return FALSE;
    }

    LZSeek( lzfd, sections[i].PointerToRawData, SEEK_SET );
    if ( resSectionSize != LZRead( lzfd, (char*)resSection, resSectionSize ) ) goto done;

    /* Find resource */
    resDir = resSection + (resDataDir->VirtualAddress - sections[i].VirtualAddress);

    resPtr = resDir;
    resPtr = find_entry_by_name( resPtr, typeid, resDir );
    if ( !resPtr )
    {
        TRACE("No typeid entry found for %p\n", typeid );
        goto done;
    }
    resPtr = find_entry_by_name( resPtr, resid, resDir );
    if ( !resPtr )
    {
        TRACE("No resid entry found for %p\n", resid );
        goto done;
    }
    resPtr = find_entry_default( resPtr, resDir );
    if ( !resPtr )
    {
        TRACE("No default language entry found for %p\n", resid );
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


/*************************************************************************
 * GetFileResourceSize                     [VER.2]
 */
DWORD WINAPI GetFileResourceSize16( LPCSTR lpszFileName, LPCSTR lpszResType,
                                    LPCSTR lpszResId, LPDWORD lpdwFileOffset )
{
    BOOL retv = FALSE;
    HFILE lzfd;
    OFSTRUCT ofs;
    DWORD reslen;

    TRACE("(%s,type=%p,id=%p,off=%p)\n",
          debugstr_a(lpszFileName), lpszResType, lpszResId, lpszResId );

    lzfd = LZOpenFileA( (LPSTR)lpszFileName, &ofs, OF_READ );
    if ( lzfd < 0 ) return 0;

    switch ( read_xx_header( lzfd ) )
    {
    case IMAGE_OS2_SIGNATURE:
#ifdef __REACTOS__
        ERR("OS2 Images not supported under ReactOS at this time.");
        retv = 0;
#else
        retv = find_ne_resource( lzfd, lpszResType, lpszResId,
                                 &reslen, lpdwFileOffset );
#endif
        break;

    case IMAGE_NT_SIGNATURE:
        retv = find_pe_resource( lzfd, lpszResType, lpszResId,
                                 &reslen, lpdwFileOffset );
        break;
    }

    LZClose( lzfd );
    return retv? reslen : 0;
}


/*************************************************************************
 * GetFileResource                         [VER.3]
 */
DWORD WINAPI GetFileResource16( LPCSTR lpszFileName, LPCSTR lpszResType,
                                LPCSTR lpszResId, DWORD dwFileOffset,
                                DWORD dwResLen, LPVOID lpvData )
{
    BOOL retv = FALSE;
    HFILE lzfd;
    OFSTRUCT ofs;
    DWORD reslen = dwResLen;

    TRACE("(%s,type=%p,id=%p,off=%d,len=%d,data=%p)\n",
		debugstr_a(lpszFileName), lpszResType, lpszResId,
                dwFileOffset, dwResLen, lpvData );

    lzfd = LZOpenFileA( (LPSTR)lpszFileName, &ofs, OF_READ );
    if ( lzfd < 0 ) return 0;

    if ( !dwFileOffset )
    {
        switch ( read_xx_header( lzfd ) )
        {
        case IMAGE_OS2_SIGNATURE:
#ifdef __REACTOS__
            ERR("OS2 Images not supported under ReactOS at this time.");
            retv = 0;
#else
            retv = find_ne_resource( lzfd, lpszResType, lpszResId,
                                     &reslen, &dwFileOffset );
#endif
            break;

        case IMAGE_NT_SIGNATURE:
            retv = find_pe_resource( lzfd, lpszResType, lpszResId,
                                     &reslen, &dwFileOffset );
            break;
        }

        if ( !retv )
        {
            LZClose( lzfd );
            return 0;
        }
    }

    LZSeek( lzfd, dwFileOffset, SEEK_SET );
    reslen = LZRead( lzfd, lpvData, min( reslen, dwResLen ) );
    LZClose( lzfd );

    return reslen;
}
