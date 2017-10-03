/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004,2005 Aric Stewart for CodeWeavers
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

#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct _tagTT_OFFSET_TABLE {
    USHORT uMajorVersion;
    USHORT uMinorVersion;
    USHORT uNumOfTables;
    USHORT uSearchRange;
    USHORT uEntrySelector;
    USHORT uRangeShift;
} TT_OFFSET_TABLE;

typedef struct _tagTT_TABLE_DIRECTORY {
    char szTag[4]; /* table name */
    ULONG uCheckSum; /* Check sum */
    ULONG uOffset; /* Offset from beginning of file */
    ULONG uLength; /* length of the table in bytes */
} TT_TABLE_DIRECTORY;

typedef struct _tagTT_NAME_TABLE_HEADER {
    USHORT uFSelector; /* format selector. Always 0 */
    USHORT uNRCount; /* Name Records count */
    USHORT uStorageOffset; /* Offset for strings storage,
                            * from start of the table */
} TT_NAME_TABLE_HEADER;

#define NAME_ID_FULL_FONT_NAME  4
#define NAME_ID_VERSION         5

typedef struct _tagTT_NAME_RECORD {
    USHORT uPlatformID;
    USHORT uEncodingID;
    USHORT uLanguageID;
    USHORT uNameID;
    USHORT uStringLength;
    USHORT uStringOffset; /* from start of storage area */
} TT_NAME_RECORD;

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

static const WCHAR regfont1[] =
    {'S','o','f','t','w','a','r','e','\\',
     'M','i','c','r','o','s','o','f','t','\\',
     'W','i','n','d','o','w','s',' ','N','T','\\',
     'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
     'F','o','n','t','s',0};
static const WCHAR regfont2[] =
    {'S','o','f','t','w','a','r','e','\\',
     'M','i','c','r','o','s','o','f','t','\\',
     'W','i','n','d','o','w','s','\\',
     'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
     'F','o','n','t','s',0};

/*
 * Code based off of code located here
 * http://www.codeproject.com/gdi/fontnamefromfile.asp
 */
static WCHAR *load_ttf_name_id( const WCHAR *filename, DWORD id )
{
    TT_TABLE_DIRECTORY tblDir;
    BOOL bFound = FALSE;
    TT_OFFSET_TABLE ttOffsetTable;
    TT_NAME_TABLE_HEADER ttNTHeader;
    TT_NAME_RECORD ttRecord;
    DWORD dwRead;
    HANDLE handle;
    LPWSTR ret = NULL;
    int i;

    handle = CreateFileW(filename ,GENERIC_READ, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to open font file %s\n", debugstr_w(filename));
        return NULL;
    }

    if (!ReadFile(handle,&ttOffsetTable, sizeof(TT_OFFSET_TABLE),&dwRead,NULL))
        goto end;

    ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
    ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
    ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

    if ((ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0) &&
        (ttOffsetTable.uMajorVersion != 0x4f54 || ttOffsetTable.uMinorVersion != 0x544f))
        goto end;

    for (i=0; i< ttOffsetTable.uNumOfTables; i++)
    {
        if (!ReadFile(handle,&tblDir, sizeof(TT_TABLE_DIRECTORY),&dwRead,NULL))
            break;
        if (memcmp(tblDir.szTag,"name",4)==0)
        {
            bFound = TRUE;
            tblDir.uLength = SWAPLONG(tblDir.uLength);
            tblDir.uOffset = SWAPLONG(tblDir.uOffset);
            break;
        }
    }

    if (!bFound)
        goto end;

    SetFilePointer(handle, tblDir.uOffset, NULL, FILE_BEGIN);
    if (!ReadFile(handle,&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER), &dwRead,NULL))
        goto end;

    ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
    ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
    for(i=0; i<ttNTHeader.uNRCount; i++)
    {
        if (!ReadFile(handle,&ttRecord, sizeof(TT_NAME_RECORD),&dwRead,NULL))
            break;

        ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
        ttRecord.uPlatformID = SWAPWORD(ttRecord.uPlatformID);
        ttRecord.uEncodingID = SWAPWORD(ttRecord.uEncodingID);
        if (ttRecord.uNameID == id && ttRecord.uPlatformID == 3 &&
            (ttRecord.uEncodingID == 0 || ttRecord.uEncodingID == 1))
        {
            WCHAR *buf;
            unsigned int i;

            ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
            ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
            SetFilePointer(handle, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset,
                           NULL, FILE_BEGIN);
            if (!(buf = msi_alloc_zero( ttRecord.uStringLength + sizeof(WCHAR) ))) goto end;
            dwRead = 0;
            ReadFile(handle, buf, ttRecord.uStringLength, &dwRead, NULL);
            if (dwRead % sizeof(WCHAR))
            {
                msi_free(buf);
                goto end;
            }
            for (i = 0; i < dwRead / sizeof(WCHAR); i++) buf[i] = SWAPWORD(buf[i]);
            ret = strdupW(buf);
            msi_free(buf);
            break;
        }
    }

end:
    CloseHandle(handle);
    return ret;
}

static WCHAR *font_name_from_file( const WCHAR *filename )
{
    static const WCHAR truetypeW[] = {' ','(','T','r','u','e','T','y','p','e',')',0};
    WCHAR *name, *ret = NULL;

    if ((name = load_ttf_name_id( filename, NAME_ID_FULL_FONT_NAME )))
    {
        if (!name[0])
        {
            WARN("empty font name\n");
            msi_free( name );
            return NULL;
        }
        ret = msi_alloc( (strlenW( name ) + strlenW( truetypeW ) + 1 ) * sizeof(WCHAR) );
        strcpyW( ret, name );
        strcatW( ret, truetypeW );
        msi_free( name );
    }
    return ret;
}

WCHAR *msi_font_version_from_file( const WCHAR *filename )
{
    static const WCHAR fmtW[] = {'%','u','.','%','u','.','0','.','0',0};
    WCHAR *version, *p, *q, *ret = NULL;

    if ((version = load_ttf_name_id( filename, NAME_ID_VERSION )))
    {
        int len, major = 0, minor = 0;
        if ((p = strchrW( version, ';' ))) *p = 0;
        p = version;
        while (*p && !isdigitW( *p )) p++;
        if ((q = strchrW( p, '.' )))
        {
            major = atoiW( p );
            p = ++q;
            while (*q && isdigitW( *q )) q++;
            if (!*q || *q == ' ') minor = atoiW( p );
            else major = 0;
        }
        len = strlenW( fmtW ) + 20;
        ret = msi_alloc( len * sizeof(WCHAR) );
        sprintfW( ret, fmtW, major, minor );
        msi_free( version );
    }
    return ret;
}

static UINT ITERATE_RegisterFonts(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPWSTR name;
    LPCWSTR filename;
    MSIFILE *file;
    MSICOMPONENT *comp;
    HKEY hkey1, hkey2;
    MSIRECORD *uirow;
    LPWSTR uipath, p;

    filename = MSI_RecordGetString( row, 1 );
    file = msi_get_loaded_file( package, filename );
    if (!file)
    {
        WARN("unable to find file %s\n", debugstr_w(filename));
        return ERROR_SUCCESS;
    }
    comp = msi_get_loaded_component( package, file->Component->Component );
    if (!comp)
    {
        WARN("unable to find component %s\n", debugstr_w(file->Component->Component));
        return ERROR_SUCCESS;
    }
    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(comp->Component));
        return ERROR_SUCCESS;
    }

    RegCreateKeyW(HKEY_LOCAL_MACHINE,regfont1,&hkey1);
    RegCreateKeyW(HKEY_LOCAL_MACHINE,regfont2,&hkey2);

    if (MSI_RecordIsNull(row,2))
        name = font_name_from_file( file->TargetPath );
    else
        name = msi_dup_record_field(row,2);

    if (name)
    {
        msi_reg_set_val_str( hkey1, name, file->TargetPath);
        msi_reg_set_val_str( hkey2, name, file->TargetPath);
    }

    msi_free(name);
    RegCloseKey(hkey1);
    RegCloseKey(hkey2);

    /* the UI chunk */
    uirow = MSI_CreateRecord( 1 );
    uipath = strdupW( file->TargetPath );
    p = strrchrW(uipath,'\\');
    if (p) p++;
    else p = uipath;
    MSI_RecordSetStringW( uirow, 1, p );
    msi_ui_actiondata( package, szRegisterFonts, uirow );
    msiobj_release( &uirow->hdr );
    msi_free( uipath );
    /* FIXME: call msi_ui_progress? */

    return ERROR_SUCCESS;
}

UINT ACTION_RegisterFonts(MSIPACKAGE *package)
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','`','F','o','n','t','`',0};
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW(package->db, query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_RegisterFonts, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_UnregisterFonts( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPWSTR name;
    LPCWSTR filename;
    MSIFILE *file;
    MSICOMPONENT *comp;
    HKEY hkey1, hkey2;
    MSIRECORD *uirow;
    LPWSTR uipath, p;

    filename = MSI_RecordGetString( row, 1 );
    file = msi_get_loaded_file( package, filename );
    if (!file)
    {
        WARN("unable to find file %s\n", debugstr_w(filename));
        return ERROR_SUCCESS;
    }
    comp = msi_get_loaded_component( package, file->Component->Component );
    if (!comp)
    {
        WARN("unable to find component %s\n", debugstr_w(file->Component->Component));
        return ERROR_SUCCESS;
    }
    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(comp->Component));
        return ERROR_SUCCESS;
    }

    RegCreateKeyW( HKEY_LOCAL_MACHINE, regfont1, &hkey1 );
    RegCreateKeyW( HKEY_LOCAL_MACHINE, regfont2, &hkey2 );

    if (MSI_RecordIsNull( row, 2 ))
        name = font_name_from_file( file->TargetPath );
    else
        name = msi_dup_record_field( row, 2 );

    if (name)
    {
        RegDeleteValueW( hkey1, name );
        RegDeleteValueW( hkey2, name );
    }

    msi_free( name );
    RegCloseKey( hkey1 );
    RegCloseKey( hkey2 );

    /* the UI chunk */
    uirow = MSI_CreateRecord( 1 );
    uipath = strdupW( file->TargetPath );
    p = strrchrW( uipath,'\\' );
    if (p) p++;
    else p = uipath;
    MSI_RecordSetStringW( uirow, 1, p );
    msi_ui_actiondata( package, szUnregisterFonts, uirow );
    msiobj_release( &uirow->hdr );
    msi_free( uipath );
    /* FIXME: call msi_ui_progress? */

    return ERROR_SUCCESS;
}

UINT ACTION_UnregisterFonts( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','`','F','o','n','t','`',0};
    MSIQUERY *view;
    UINT r;

    r = MSI_DatabaseOpenViewW( package->db, query, &view );
    if (r != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    r = MSI_IterateRecords( view, NULL, ITERATE_UnregisterFonts, package );
    msiobj_release( &view->hdr );
    return r;
}
