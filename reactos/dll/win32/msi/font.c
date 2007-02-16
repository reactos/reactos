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
#include "wine/unicode.h"

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

static const WCHAR szRegisterFonts[] =
    {'R','e','g','i','s','t','e','r','F','o','n','t','s',0};

/*
 * Code based off of code located here
 * http://www.codeproject.com/gdi/fontnamefromfile.asp
 *
 * Using string index 4 (full font name) instead of 1 (family name)
 */
static LPWSTR load_ttfname_from(LPCWSTR filename)
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

    if (ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
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
    bFound = FALSE;
    for(i=0; i<ttNTHeader.uNRCount; i++)
    {
        if (!ReadFile(handle,&ttRecord, sizeof(TT_NAME_RECORD),&dwRead,NULL))
            break;

        ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
        /* 4 is the Full Font Name */
        if(ttRecord.uNameID == 4)
        {
            int nPos;
            LPSTR buf;
            static LPCSTR tt = " (TrueType)";

            ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
            ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
            nPos = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
            SetFilePointer(handle, tblDir.uOffset +
                            ttRecord.uStringOffset +
                            ttNTHeader.uStorageOffset,
                            NULL, FILE_BEGIN);
            buf = msi_alloc_zero( ttRecord.uStringLength + 1 + strlen(tt) );
            ReadFile(handle, buf, ttRecord.uStringLength, &dwRead, NULL);
            if (strlen(buf) > 0)
            {
                strcat(buf,tt);
                ret = strdupAtoW(buf);
                msi_free(buf);
                break;
            }

            msi_free(buf);
            SetFilePointer(handle,nPos, NULL, FILE_BEGIN);
        }
    }

end:
    CloseHandle(handle);

    TRACE("Returning fontname %s\n",debugstr_w(ret));
    return ret;
}

static UINT ITERATE_RegisterFonts(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPWSTR name;
    LPCWSTR filename;
    MSIFILE *file;
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
    HKEY hkey1;
    HKEY hkey2;
    MSIRECORD *uirow;
    LPWSTR uipath, p;

    filename = MSI_RecordGetString( row, 1 );
    file = get_loaded_file( package, filename );
    if (!file)
    {
        ERR("Unable to load file\n");
        return ERROR_SUCCESS;
    }

    /* check to make sure that component is installed */
    if (!ACTION_VerifyComponentForAction( file->Component, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping: Component not scheduled for install\n");
        return ERROR_SUCCESS;
    }

    RegCreateKeyW(HKEY_LOCAL_MACHINE,regfont1,&hkey1);
    RegCreateKeyW(HKEY_LOCAL_MACHINE,regfont2,&hkey2);

    if (MSI_RecordIsNull(row,2))
        name = load_ttfname_from( file->TargetPath );
    else
        name = msi_dup_record_field(row,2);

    if (name)
    {
        msi_reg_set_val_str( hkey1, name, file->FileName );
        msi_reg_set_val_str( hkey2, name, file->FileName );
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
    ui_actiondata( package, szRegisterFonts, uirow);
    msiobj_release( &uirow->hdr );
    msi_free( uipath );
    /* FIXME: call ui_progress? */

    return ERROR_SUCCESS;
}

UINT ACTION_RegisterFonts(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','F','o','n','t','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("MSI_DatabaseOpenViewW failed: %d\n", rc);
        return ERROR_SUCCESS;
    }

    MSI_IterateRecords(view, NULL, ITERATE_RegisterFonts, package);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}
