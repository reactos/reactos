/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002,2003,2004,2005 Mike McCormack for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "objidl.h"
#include "objbase.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

DEFINE_GUID( CLSID_MsiDatabase, 0x000c1084, 0x0000, 0x0000,
             0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID( CLSID_MsiPatch, 0x000c1086, 0x0000, 0x0000,
             0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

/*
 *  .MSI  file format
 *
 *  An .msi file is a structured storage file.
 *  It contains a number of streams.
 *  A stream for each table in the database.
 *  Two streams for the string table in the database.
 *  Any binary data in a table is a reference to a stream.
 */

static VOID MSI_CloseDatabase( MSIOBJECTHDR *arg )
{
    MSIDATABASE *db = (MSIDATABASE *) arg;

    msi_free(db->path);
    free_cached_tables( db );
    msi_free_transforms( db );
    msi_destroy_stringtable( db->strings );
    IStorage_Release( db->storage );
    if (db->deletefile)
    {
        DeleteFileW( db->deletefile );
        msi_free( db->deletefile );
    }
}

UINT MSI_OpenDatabaseW(LPCWSTR szDBPath, LPCWSTR szPersist, MSIDATABASE **pdb)
{
    IStorage *stg = NULL;
    HRESULT r;
    MSIDATABASE *db = NULL;
    UINT ret = ERROR_FUNCTION_FAILED;
    LPCWSTR szMode, save_path;
    STATSTG stat;
    BOOL created = FALSE;
    WCHAR path[MAX_PATH];

    static const WCHAR backslash[] = {'\\',0};

    TRACE("%s %s\n",debugstr_w(szDBPath),debugstr_w(szPersist) );

    if( !pdb )
        return ERROR_INVALID_PARAMETER;

    save_path = szDBPath;
    szMode = szPersist;
    if( HIWORD( szPersist ) )
    {
        if (!CopyFileW( szDBPath, szPersist, FALSE ))
            return ERROR_OPEN_FAILED;

        szDBPath = szPersist;
        szPersist = MSIDBOPEN_TRANSACT;
        created = TRUE;
    }

    if( szPersist == MSIDBOPEN_READONLY )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    }
    else if( szPersist == MSIDBOPEN_CREATE || szPersist == MSIDBOPEN_CREATEDIRECT )
    {
        /* FIXME: MSIDBOPEN_CREATE should case STGM_TRANSACTED flag to be
         * used here: */
        r = StgCreateDocfile( szDBPath,
              STGM_CREATE|STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &stg);
        if( r == ERROR_SUCCESS )
        {
            IStorage_SetClass( stg, &CLSID_MsiDatabase );
            r = init_string_table( stg );
        }
        created = TRUE;
    }
    else if( szPersist == MSIDBOPEN_TRANSACT )
    {
        /* FIXME: MSIDBOPEN_TRANSACT should case STGM_TRANSACTED flag to be
         * used here: */
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    }
    else if( szPersist == MSIDBOPEN_DIRECT )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    }
    else
    {
        ERR("unknown flag %p\n",szPersist);
        return ERROR_INVALID_PARAMETER;
    }

    if( FAILED( r ) )
    {
        FIXME("open failed r = %08x!\n",r);
        return ERROR_FUNCTION_FAILED;
    }

    r = IStorage_Stat( stg, &stat, STATFLAG_NONAME );
    if( FAILED( r ) )
    {
        FIXME("Failed to stat storage\n");
        goto end;
    }

    if ( !IsEqualGUID( &stat.clsid, &CLSID_MsiDatabase ) &&
         !IsEqualGUID( &stat.clsid, &CLSID_MsiPatch ) ) 
    {
        ERR("storage GUID is not a MSI database GUID %s\n",
             debugstr_guid(&stat.clsid) );
        goto end;
    }

    db = alloc_msiobject( MSIHANDLETYPE_DATABASE, sizeof (MSIDATABASE),
                              MSI_CloseDatabase );
    if( !db )
    {
        FIXME("Failed to allocate a handle\n");
        goto end;
    }

    if (!strchrW( save_path, '\\' ))
    {
        GetCurrentDirectoryW( MAX_PATH, path );
        lstrcatW( path, backslash );
        lstrcatW( path, save_path );
    }
    else
        lstrcpyW( path, save_path );

    db->path = strdupW( path );

    if( TRACE_ON( msi ) )
        enum_stream_names( stg );

    db->storage = stg;
    db->mode = szMode;
    if (created)
        db->deletefile = strdupW( szDBPath );
    else
        db->deletefile = NULL;
    list_init( &db->tables );
    list_init( &db->transforms );

    db->strings = load_string_table( stg );
    if( !db->strings )
        goto end;

    ret = ERROR_SUCCESS;

    msiobj_addref( &db->hdr );
    IStorage_AddRef( stg );
    *pdb = db;

end:
    if( db )
        msiobj_release( &db->hdr );
    if( stg )
        IStorage_Release( stg );

    return ret;
}

UINT WINAPI MsiOpenDatabaseW(LPCWSTR szDBPath, LPCWSTR szPersist, MSIHANDLE *phDB)
{
    MSIDATABASE *db;
    UINT ret;

    TRACE("%s %s %p\n",debugstr_w(szDBPath),debugstr_w(szPersist), phDB);

    ret = MSI_OpenDatabaseW( szDBPath, szPersist, &db );
    if( ret == ERROR_SUCCESS )
    {
        *phDB = alloc_msihandle( &db->hdr );
        if (! *phDB)
            ret = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &db->hdr );
    }

    return ret;
}

UINT WINAPI MsiOpenDatabaseA(LPCSTR szDBPath, LPCSTR szPersist, MSIHANDLE *phDB)
{
    HRESULT r = ERROR_FUNCTION_FAILED;
    LPWSTR szwDBPath = NULL, szwPersist = NULL;

    TRACE("%s %s %p\n", debugstr_a(szDBPath), debugstr_a(szPersist), phDB);

    if( szDBPath )
    {
        szwDBPath = strdupAtoW( szDBPath );
        if( !szwDBPath )
            goto end;
    }

    if( HIWORD(szPersist) )
    {
        szwPersist = strdupAtoW( szPersist );
        if( !szwPersist )
            goto end;
    }
    else
        szwPersist = (LPWSTR)(DWORD_PTR)szPersist;

    r = MsiOpenDatabaseW( szwDBPath, szwPersist, phDB );

end:
    if( HIWORD(szPersist) )
        msi_free( szwPersist );
    msi_free( szwDBPath );

    return r;
}

static LPWSTR msi_read_text_archive(LPCWSTR path)
{
    HANDLE file;
    LPSTR data = NULL;
    LPWSTR wdata = NULL;
    DWORD read, size = 0;

    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (file == INVALID_HANDLE_VALUE)
        return NULL;

    size = GetFileSize( file, NULL );
    data = msi_alloc( size + 1 );
    if (!data)
        goto done;

    if (!ReadFile( file, data, size, &read, NULL ))
        goto done;

    data[size] = '\0';
    wdata = strdupAtoW( data );

done:
    CloseHandle( file );
    msi_free( data );
    return wdata;
}

static void msi_parse_line(LPWSTR *line, LPWSTR **entries, DWORD *num_entries)
{
    LPWSTR ptr = *line, save;
    DWORD i, count = 1;

    *entries = NULL;

    /* stay on this line */
    while (*ptr && *ptr != '\n')
    {
        /* entries are separated by tabs */
        if (*ptr == '\t')
            count++;

        ptr++;
    }

    *entries = msi_alloc(count * sizeof(LPWSTR));
    if (!*entries)
        return;

    /* store pointers into the data */
    for (i = 0, ptr = *line; i < count; i++)
    {
        save = ptr;

        while (*ptr && *ptr != '\t' && *ptr != '\n') ptr++;

        /* NULL-separate the data */
        if (*ptr)
            *ptr++ = '\0';

        (*entries)[i] = save;
    }

    /* move to the next line if there's more, else EOF */
    *line = ptr;

    if (num_entries)
        *num_entries = count;
}

static LPWSTR msi_build_createsql_prelude(LPWSTR table)
{
    LPWSTR prelude;
    DWORD size;

    static const WCHAR create_fmt[] = {'C','R','E','A','T','E',' ','T','A','B','L','E',' ','`','%','s','`',' ','(',' ',0};

    size = sizeof(create_fmt) + lstrlenW(table) - 2;
    prelude = msi_alloc(size * sizeof(WCHAR));
    if (!prelude)
        return NULL;

    sprintfW(prelude, create_fmt, table);
    return prelude;
}

static LPWSTR msi_build_createsql_columns(LPWSTR *columns_data, LPWSTR *types, DWORD num_columns)
{
    LPWSTR columns;
    LPCWSTR type;
    DWORD sql_size = 1, i, len;
    WCHAR expanded[128], *ptr;
    WCHAR size[10], comma[2], extra[30];

    static const WCHAR column_fmt[] = {'`','%','s','`',' ','%','s','%','s','%','s','%','s',' ',0};
    static const WCHAR size_fmt[] = {'(','%','s',')',0};
    static const WCHAR type_char[] = {'C','H','A','R',0};
    static const WCHAR type_int[] = {'I','N','T',0};
    static const WCHAR type_long[] = {'L','O','N','G',0};
    static const WCHAR type_notnull[] = {' ','N','O','T',' ','N','U','L','L',0};
    static const WCHAR localizable[] = {' ','L','O','C','A','L','I','Z','A','B','L','E',0};

    columns = msi_alloc_zero(sql_size * sizeof(WCHAR));
    if (!columns)
        return NULL;

    for (i = 0; i < num_columns; i++)
    {
        type = NULL;
        comma[1] = size[0] = extra[0] = '\0';

        if (i == num_columns - 1)
            comma[0] = '\0';
        else
            comma[0] = ',';

        ptr = &types[i][1];
        len = atolW(ptr);
        extra[0] = '\0';

        switch (types[i][0])
        {
            case 'l':
                lstrcpyW(extra, type_notnull);
            case 'L':
                lstrcatW(extra, localizable);
                type = type_char;
                sprintfW(size, size_fmt, ptr);
                break;
            case 's':
                lstrcpyW(extra, type_notnull);
            case 'S':
                type = type_char;
                sprintfW(size, size_fmt, ptr);
                break;
            case 'i':
                lstrcpyW(extra, type_notnull);
            case 'I':
                if (len == 2)
                    type = type_int;
                else
                    type = type_long;
                break;
        }

        sprintfW(expanded, column_fmt, columns_data[i], type, size, extra, comma);
        sql_size += lstrlenW(expanded);

        columns = msi_realloc(columns, sql_size * sizeof(WCHAR));
        if (!columns)
            return NULL;

        lstrcatW(columns, expanded);
    }

    return columns;
}

static LPWSTR msi_build_createsql_postlude(LPWSTR *primary_keys, DWORD num_keys)
{
    LPWSTR postlude, keys, ptr;
    DWORD size, key_size, i;

    static const WCHAR key_fmt[] = {'`','%','s','`',',',' ',0};
    static const WCHAR postlude_fmt[] = {'P','R','I','M','A','R','Y',' ','K','E','Y',' ','%','s',')',' ','H','O','L','D',0};

    for (i = 0, size = 1; i < num_keys; i++)
        size += lstrlenW(key_fmt) + lstrlenW(primary_keys[i]) - 2;

    keys = msi_alloc(size * sizeof(WCHAR));
    if (!keys)
        return NULL;

    for (i = 0, ptr = keys; i < num_keys; i++)
    {
        key_size = lstrlenW(key_fmt) + lstrlenW(primary_keys[i]) -2;
        sprintfW(ptr, key_fmt, primary_keys[i]);
        ptr += key_size;
    }

    /* remove final ', ' */
    *(ptr - 2) = '\0';

    size = lstrlenW(postlude_fmt) + size - 1;
    postlude = msi_alloc(size * sizeof(WCHAR));
    if (!postlude)
        goto done;

    sprintfW(postlude, postlude_fmt, keys);

done:
    msi_free(keys);
    return postlude;
}

static UINT msi_add_table_to_db(MSIDATABASE *db, LPWSTR *columns, LPWSTR *types, LPWSTR *labels, DWORD num_labels, DWORD num_columns)
{
    UINT r;
    DWORD size;
    MSIQUERY *view;
    LPWSTR create_sql;
    LPWSTR prelude, columns_sql, postlude;

    prelude = msi_build_createsql_prelude(labels[0]);
    columns_sql = msi_build_createsql_columns(columns, types, num_columns);
    postlude = msi_build_createsql_postlude(labels + 1, num_labels - 1); /* skip over table name */

    if (!prelude || !columns_sql || !postlude)
        return ERROR_OUTOFMEMORY;

    size = lstrlenW(prelude) + lstrlenW(columns_sql) + lstrlenW(postlude) + 1;
    create_sql = msi_alloc(size * sizeof(WCHAR));
    if (!create_sql)
        return ERROR_OUTOFMEMORY;

    lstrcpyW(create_sql, prelude);
    lstrcatW(create_sql, columns_sql);
    lstrcatW(create_sql, postlude);

    msi_free(prelude);
    msi_free(columns_sql);
    msi_free(postlude);

    r = MSI_DatabaseOpenViewW( db, create_sql, &view );
    msi_free(create_sql);

    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute(view, NULL);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    return r;
}

static LPWSTR msi_build_insertsql_prelude(LPWSTR table)
{
    LPWSTR prelude;
    DWORD size;

    static const WCHAR insert_fmt[] = {'I','N','S','E','R','T',' ','I','N','T','O',' ','`','%','s','`',' ','(',' ',0};

    size = sizeof(insert_fmt) + lstrlenW(table) - 2;
    prelude = msi_alloc(size * sizeof(WCHAR));
    if (!prelude)
        return NULL;

    sprintfW(prelude, insert_fmt, table);
    return prelude;
}

static LPWSTR msi_build_insertsql_columns(LPWSTR *columns_data, LPWSTR *types, DWORD num_columns)
{
    LPWSTR columns;
    DWORD sql_size = 1, i;
    WCHAR expanded[128];

    static const WCHAR column_fmt[] =  {'`','%','s','`',',',' ',0};

    columns = msi_alloc_zero(sql_size * sizeof(WCHAR));
    if (!columns)
        return NULL;

    for (i = 0; i < num_columns; i++)
    {
        sprintfW(expanded, column_fmt, columns_data[i]);
        sql_size += lstrlenW(expanded);

        if (i == num_columns - 1)
        {
            sql_size -= 2;
            expanded[lstrlenW(expanded) - 2] = '\0';
        }

        columns = msi_realloc(columns, sql_size * sizeof(WCHAR));
        if (!columns)
            return NULL;

        lstrcatW(columns, expanded);
    }

    return columns;
}

static LPWSTR msi_build_insertsql_data(LPWSTR **records, LPWSTR *types, DWORD num_columns, DWORD irec)
{
    LPWSTR columns;
    DWORD sql_size = 1, i;
    WCHAR expanded[128];

    static const WCHAR str_fmt[] = {'\'','%','s','\'',',',' ',0};
    static const WCHAR int_fmt[] = {'%','s',',',' ',0};
    static const WCHAR empty[] = {'\'','\'',',',' ',0};

    columns = msi_alloc_zero(sql_size * sizeof(WCHAR));
    if (!columns)
        return NULL;

    for (i = 0; i < num_columns; i++)
    {
        switch (types[i][0])
        {
            case 'L': case 'l': case 'S': case 's':
                sprintfW(expanded, str_fmt, records[irec][i]);
                break;
            case 'I': case 'i':
                if (*records[0][i])
                    sprintfW(expanded, int_fmt, records[irec][i]);
                else
                    lstrcpyW(expanded, empty);
                break;
            default:
                return NULL;
        }

        if (i == num_columns - 1)
            expanded[lstrlenW(expanded) - 2] = '\0';

        sql_size += lstrlenW(expanded);
        columns = msi_realloc(columns, sql_size * sizeof(WCHAR));
        if (!columns)
            return NULL;

        lstrcatW(columns, expanded);
    }

    return columns;
}

static UINT msi_add_records_to_table(MSIDATABASE *db, LPWSTR *columns, LPWSTR *types,
                                     LPWSTR *labels, LPWSTR **records,
                                     int num_columns, int num_records)
{
    MSIQUERY *view;
    LPWSTR insert_sql;
    DWORD size, i;
    UINT r = ERROR_SUCCESS;

    static const WCHAR mid[] = {' ',')',' ','V','A','L','U','E','S',' ','(',' ',0};
    static const WCHAR end[] = {' ',')',0};

    LPWSTR prelude = msi_build_insertsql_prelude(labels[0]);
    LPWSTR columns_sql = msi_build_insertsql_columns(columns, types, num_columns);
    
    for (i = 0; i < num_records; i++)
    {
        LPWSTR data = msi_build_insertsql_data(records, types, num_columns, i);

        size = lstrlenW(prelude) + lstrlenW(columns_sql) + sizeof(mid) + lstrlenW(data) + sizeof(end) - 1; 
        insert_sql = msi_alloc(size * sizeof(WCHAR));
        if (!insert_sql)
            return ERROR_OUTOFMEMORY;
    
        lstrcpyW(insert_sql, prelude);
        lstrcatW(insert_sql, columns_sql);
        lstrcatW(insert_sql, mid);
        lstrcatW(insert_sql, data);
        lstrcatW(insert_sql, end);

        msi_free(data);

        r = MSI_DatabaseOpenViewW( db, insert_sql, &view );
        msi_free(insert_sql);

        if (r != ERROR_SUCCESS)
            goto done;

        r = MSI_ViewExecute(view, NULL);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

done:
    msi_free(prelude);
    msi_free(columns_sql);

    return r;
}

UINT MSI_DatabaseImport(MSIDATABASE *db, LPCWSTR folder, LPCWSTR file)
{
    UINT r;
    DWORD len, i;
    DWORD num_labels;
    DWORD num_columns, num_records = 0;
    LPWSTR *columns, *types, *labels;
    LPWSTR path, ptr, data;
    LPWSTR **records;

    static const WCHAR backslash[] = {'\\',0};

    TRACE("%p %s %s\n", db, debugstr_w(folder), debugstr_w(file) );

    if( folder == NULL || file == NULL )
        return ERROR_INVALID_PARAMETER;

    len = lstrlenW(folder) + lstrlenW(backslash) + lstrlenW(file) + 1;
    path = msi_alloc( len * sizeof(WCHAR) );
    if (!path)
        return ERROR_OUTOFMEMORY;

    lstrcpyW( path, folder );
    lstrcatW( path, backslash );
    lstrcatW( path, file );

    data = msi_read_text_archive( path );

    ptr = data;
    msi_parse_line( &ptr, &columns, &num_columns );
    msi_parse_line( &ptr, &types, NULL );
    msi_parse_line( &ptr, &labels, &num_labels );

    records = msi_alloc(sizeof(LPWSTR *));
    if (!records)
        return ERROR_OUTOFMEMORY;

    /* read in the table records */
    while (*ptr)
    {
        msi_parse_line( &ptr, &records[num_records], NULL );

        num_records++;
        records = msi_realloc(records, (num_records + 1) * sizeof(LPWSTR *));
        if (!records)
            return ERROR_OUTOFMEMORY;
    }

    r = msi_add_table_to_db( db, columns, types, labels, num_labels, num_columns );
    if (r != ERROR_SUCCESS)
        goto done;

    r = msi_add_records_to_table( db, columns, types, labels, records, num_columns, num_records );

done:
    msi_free(path);
    msi_free(data);
    msi_free(columns);
    msi_free(types);
    msi_free(labels);

    for (i = 0; i < num_records; i++)
        msi_free(records[i]);

    msi_free(records);

    return r;
}

UINT WINAPI MsiDatabaseImportW(MSIHANDLE handle, LPCWSTR szFolder, LPCWSTR szFilename)
{
    MSIDATABASE *db;
    UINT r;

    TRACE("%lx %s %s\n",handle,debugstr_w(szFolder), debugstr_w(szFilename));

    db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
    if( !db )
        return ERROR_INVALID_HANDLE;
    r = MSI_DatabaseImport( db, szFolder, szFilename );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseImportA( MSIHANDLE handle,
               LPCSTR szFolder, LPCSTR szFilename )
{
    LPWSTR path = NULL, file = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%lx %s %s\n", handle, debugstr_a(szFolder), debugstr_a(szFilename));

    if( szFolder )
    {
        path = strdupAtoW( szFolder );
        if( !path )
            goto end;
    }

    if( szFilename )
    {
        file = strdupAtoW( szFilename );
        if( !file )
            goto end;
    }

    r = MsiDatabaseImportW( handle, path, file );

end:
    msi_free( path );
    msi_free( file );

    return r;
}

static UINT msi_export_record( HANDLE handle, MSIRECORD *row, UINT start )
{
    UINT i, count, len, r = ERROR_SUCCESS;
    const char *sep;
    char *buffer;
    DWORD sz;

    len = 0x100;
    buffer = msi_alloc( len );
    if ( !buffer )
        return ERROR_OUTOFMEMORY;

    count = MSI_RecordGetFieldCount( row );
    for ( i=start; i<=count; i++ )
    {
        sz = len;
        r = MSI_RecordGetStringA( row, i, buffer, &sz );
        if (r == ERROR_MORE_DATA)
        {
            char *p = msi_realloc( buffer, sz + 1 );
            if (!p)
                break;
            len = sz + 1;
            buffer = p;
        }
        sz = len;
        r = MSI_RecordGetStringA( row, i, buffer, &sz );
        if (r != ERROR_SUCCESS)
            break;

        if (!WriteFile( handle, buffer, sz, &sz, NULL ))
        {
            r = ERROR_FUNCTION_FAILED;
            break;
        }

        sep = (i < count) ? "\t" : "\r\n";
        if (!WriteFile( handle, sep, strlen(sep), &sz, NULL ))
        {
            r = ERROR_FUNCTION_FAILED;
            break;
        }
    }
    msi_free( buffer );
    return r;
}

static UINT msi_export_row( MSIRECORD *row, void *arg )
{
    return msi_export_record( arg, row, 1 );
}

UINT MSI_DatabaseExport( MSIDATABASE *db, LPCWSTR table,
               LPCWSTR folder, LPCWSTR file )
{
    static const WCHAR query[] = {
        's','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','%','s',0 };
    static const WCHAR szbs[] = { '\\', 0 };
    MSIRECORD *rec = NULL;
    MSIQUERY *view = NULL;
    LPWSTR filename;
    HANDLE handle;
    UINT len, r;

    TRACE("%p %s %s %s\n", db, debugstr_w(table),
          debugstr_w(folder), debugstr_w(file) );

    if( folder == NULL || file == NULL )
        return ERROR_INVALID_PARAMETER;

    len = lstrlenW(folder) + lstrlenW(file) + 2;
    filename = msi_alloc(len * sizeof (WCHAR));
    if (!filename)
        return ERROR_OUTOFMEMORY;

    lstrcpyW( filename, folder );
    lstrcatW( filename, szbs );
    lstrcatW( filename, file );

    handle = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    msi_free( filename );
    if (handle == INVALID_HANDLE_VALUE)
        return ERROR_FUNCTION_FAILED;

    r = MSI_OpenQuery( db, &view, query, table );
    if (r == ERROR_SUCCESS)
    {
        /* write out row 1, the column names */
        r = MSI_ViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
        if (r == ERROR_SUCCESS)
        {
            msi_export_record( handle, rec, 1 );
            msiobj_release( &rec->hdr );
        }

        /* write out row 2, the column types */
        r = MSI_ViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
        if (r == ERROR_SUCCESS)
        {
            msi_export_record( handle, rec, 1 );
            msiobj_release( &rec->hdr );
        }

        /* write out row 3, the table name + keys */
        r = MSI_DatabaseGetPrimaryKeys( db, table, &rec );
        if (r == ERROR_SUCCESS)
        {
            MSI_RecordSetStringW( rec, 0, table );
            msi_export_record( handle, rec, 0 );
            msiobj_release( &rec->hdr );
        }

        /* write out row 4 onwards, the data */
        r = MSI_IterateRecords( view, 0, msi_export_row, handle );
        msiobj_release( &view->hdr );
    }

    CloseHandle( handle );

    return r;
}

/***********************************************************************
 * MsiExportDatabaseW        [MSI.@]
 *
 * Writes a file containing the table data as tab separated ASCII.
 *
 * The format is as follows:
 *
 * row1 : colname1 <tab> colname2 <tab> .... colnameN <cr> <lf>
 * row2 : coltype1 <tab> coltype2 <tab> .... coltypeN <cr> <lf>
 * row3 : tablename <tab> key1 <tab> key2 <tab> ... keyM <cr> <lf>
 *
 * Followed by the data, starting at row 1 with one row per line
 *
 * row4 : data <tab> data <tab> data <tab> ... data <cr> <lf>
 */
UINT WINAPI MsiDatabaseExportW( MSIHANDLE handle, LPCWSTR szTable,
               LPCWSTR szFolder, LPCWSTR szFilename )
{
    MSIDATABASE *db;
    UINT r;

    TRACE("%lx %s %s %s\n", handle, debugstr_w(szTable),
          debugstr_w(szFolder), debugstr_w(szFilename));

    db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
    if( !db )
        return ERROR_INVALID_HANDLE;
    r = MSI_DatabaseExport( db, szTable, szFolder, szFilename );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseExportA( MSIHANDLE handle, LPCSTR szTable,
               LPCSTR szFolder, LPCSTR szFilename )
{
    LPWSTR path = NULL, file = NULL, table = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%lx %s %s %s\n", handle, debugstr_a(szTable),
          debugstr_a(szFolder), debugstr_a(szFilename));

    if( szTable )
    {
        table = strdupAtoW( szTable );
        if( !table )
            goto end;
    }

    if( szFolder )
    {
        path = strdupAtoW( szFolder );
        if( !path )
            goto end;
    }

    if( szFilename )
    {
        file = strdupAtoW( szFilename );
        if( !file )
            goto end;
    }

    r = MsiDatabaseExportW( handle, table, path, file );

end:
    msi_free( table );
    msi_free( path );
    msi_free( file );

    return r;
}

MSIDBSTATE WINAPI MsiGetDatabaseState( MSIHANDLE handle )
{
    MSIDBSTATE ret = MSIDBSTATE_READ;
    MSIDATABASE *db;

    TRACE("%ld\n", handle);

    db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
    if (!db)
        return MSIDBSTATE_ERROR;
    if (db->mode != MSIDBOPEN_READONLY )
        ret = MSIDBSTATE_WRITE;
    msiobj_release( &db->hdr );

    return ret;
}
