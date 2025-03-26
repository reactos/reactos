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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

struct offset_table
{
    USHORT uMajorVersion;
    USHORT uMinorVersion;
    USHORT uNumOfTables;
    USHORT uSearchRange;
    USHORT uEntrySelector;
    USHORT uRangeShift;
};

struct table_directory
{
    char szTag[4]; /* table name */
    ULONG uCheckSum; /* Check sum */
    ULONG uOffset; /* Offset from beginning of file */
    ULONG uLength; /* length of the table in bytes */
};

struct name_table_header
{
    USHORT uFSelector; /* format selector. Always 0 */
    USHORT uNRCount; /* Name Records count */
    USHORT uStorageOffset; /* Offset for strings storage from start of the table */
};

#define NAME_ID_FULL_FONT_NAME  4
#define NAME_ID_VERSION         5

struct name_record
{
    USHORT uPlatformID;
    USHORT uEncodingID;
    USHORT uLanguageID;
    USHORT uNameID;
    USHORT uStringLength;
    USHORT uStringOffset; /* from start of storage area */
};

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

/*
 * Code based off of code located here
 * http://www.codeproject.com/gdi/fontnamefromfile.asp
 */
static WCHAR *load_ttf_name_id( MSIPACKAGE *package, const WCHAR *filename, DWORD id )
{
    struct table_directory tblDir;
    BOOL bFound = FALSE;
    struct offset_table ttOffsetTable;
    struct name_table_header ttNTHeader;
    struct name_record ttRecord;
    DWORD dwRead;
    HANDLE handle;
    LPWSTR ret = NULL;
    int i;

    if (package)
        handle = msi_create_file( package, filename, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL );
    else
        handle = CreateFileW( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to open font file %s\n", debugstr_w(filename));
        return NULL;
    }

    if (!ReadFile(handle,&ttOffsetTable, sizeof(struct offset_table),&dwRead,NULL))
        goto end;

    ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
    ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
    ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

    if ((ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0) &&
        (ttOffsetTable.uMajorVersion != 0x4f54 || ttOffsetTable.uMinorVersion != 0x544f))
        goto end;

    for (i=0; i< ttOffsetTable.uNumOfTables; i++)
    {
        if (!ReadFile(handle, &tblDir, sizeof(tblDir), &dwRead, NULL))
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
    if (!ReadFile(handle, &ttNTHeader, sizeof(ttNTHeader), &dwRead, NULL))
        goto end;

    ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
    ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
    for(i=0; i<ttNTHeader.uNRCount; i++)
    {
        if (!ReadFile(handle, &ttRecord, sizeof(ttRecord), &dwRead, NULL))
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
            if (!(buf = calloc(ttRecord.uStringLength, sizeof(WCHAR)))) goto end;
            dwRead = 0;
            ReadFile(handle, buf, ttRecord.uStringLength, &dwRead, NULL);
            if (dwRead % sizeof(WCHAR))
            {
                free(buf);
                goto end;
            }
            for (i = 0; i < dwRead / sizeof(WCHAR); i++) buf[i] = SWAPWORD(buf[i]);
            ret = wcsdup(buf);
            free(buf);
            break;
        }
    }

end:
    CloseHandle(handle);
    return ret;
}

static WCHAR *font_name_from_file( MSIPACKAGE *package, const WCHAR *filename )
{
    WCHAR *name, *ret = NULL;

    if ((name = load_ttf_name_id( package, filename, NAME_ID_FULL_FONT_NAME )))
    {
        if (!name[0])
        {
            WARN("empty font name\n");
            free( name );
            return NULL;
        }
        ret = malloc( wcslen( name ) * sizeof(WCHAR) + sizeof( L" (TrueType)" ) );
        lstrcpyW( ret, name );
        lstrcatW( ret, L" (TrueType)" );
        free( name );
    }
    return ret;
}

WCHAR *msi_get_font_file_version( MSIPACKAGE *package, const WCHAR *filename )
{
    WCHAR *version, *p, *q, *ret = NULL;

    if ((version = load_ttf_name_id( package, filename, NAME_ID_VERSION )))
    {
        int len, major = 0, minor = 0;
        if ((p = wcschr( version, ';' ))) *p = 0;
        p = version;
        while (*p && !iswdigit( *p )) p++;
        if ((q = wcschr( p, '.' )))
        {
            major = wcstol( p, NULL, 10 );
            p = ++q;
            while (*q && iswdigit( *q )) q++;
            if (!*q || *q == ' ') minor = wcstol( p, NULL, 10 );
            else major = 0;
        }
        len = lstrlenW( L"%u.%u.0.0" ) + 20;
        ret = malloc( len * sizeof(WCHAR) );
        swprintf( ret, len, L"%u.%u.0.0", major, minor );
        free( version );
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

    RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts" ,&hkey1 );
    RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts", &hkey2 );

    if (MSI_RecordIsNull(row,2))
        name = font_name_from_file( package, file->TargetPath );
    else
        name = msi_dup_record_field(row,2);

    if (name)
    {
        msi_reg_set_val_str( hkey1, name, file->TargetPath);
        msi_reg_set_val_str( hkey2, name, file->TargetPath);
    }

    free(name);
    RegCloseKey(hkey1);
    RegCloseKey(hkey2);

    /* the UI chunk */
    uirow = MSI_CreateRecord( 1 );
    uipath = wcsdup( file->TargetPath );
    p = wcsrchr(uipath,'\\');
    if (p) p++;
    else p = uipath;
    MSI_RecordSetStringW( uirow, 1, p );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );
    free( uipath );
    /* FIXME: call msi_ui_progress? */

    return ERROR_SUCCESS;
}

UINT ACTION_RegisterFonts(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RegisterFonts");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `Font`", &view);
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

    RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", &hkey1 );
    RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts", &hkey2 );

    if (MSI_RecordIsNull( row, 2 ))
        name = font_name_from_file( package, file->TargetPath );
    else
        name = msi_dup_record_field( row, 2 );

    if (name)
    {
        RegDeleteValueW( hkey1, name );
        RegDeleteValueW( hkey2, name );
    }

    free( name );
    RegCloseKey( hkey1 );
    RegCloseKey( hkey2 );

    /* the UI chunk */
    uirow = MSI_CreateRecord( 1 );
    uipath = wcsdup( file->TargetPath );
    p = wcsrchr( uipath,'\\' );
    if (p) p++;
    else p = uipath;
    MSI_RecordSetStringW( uirow, 1, p );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );
    free( uipath );
    /* FIXME: call msi_ui_progress? */

    return ERROR_SUCCESS;
}

UINT ACTION_UnregisterFonts( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"UnregisterFonts");

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Font`", &view );
    if (r != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    r = MSI_IterateRecords( view, NULL, ITERATE_UnregisterFonts, package );
    msiobj_release( &view->hdr );
    return r;
}
