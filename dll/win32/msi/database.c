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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "objidl.h"
#include "objbase.h"
#include "msiserver.h"
#include "query.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 *  .MSI  file format
 *
 *  An .msi file is a structured storage file.
 *  It contains a number of streams.
 *  A stream for each table in the database.
 *  Two streams for the string table in the database.
 *  Any binary data in a table is a reference to a stream.
 */

#define IS_INTMSIDBOPEN(x)      (((ULONG_PTR)(x) >> 16) == 0)

static void free_transforms( MSIDATABASE *db )
{
    while( !list_empty( &db->transforms ) )
    {
        MSITRANSFORM *t = LIST_ENTRY( list_head( &db->transforms ), MSITRANSFORM, entry );
        list_remove( &t->entry );
        IStorage_Release( t->stg );
        free( t );
    }
}

static void free_streams( MSIDATABASE *db )
{
    UINT i;
    for (i = 0; i < db->num_streams; i++)
    {
        if (db->streams[i].stream) IStream_Release( db->streams[i].stream );
    }
    free( db->streams );
}

void append_storage_to_db( MSIDATABASE *db, IStorage *stg )
{
    MSITRANSFORM *t;

    t = malloc( sizeof *t );
    t->stg = stg;
    IStorage_AddRef( stg );
    list_add_head( &db->transforms, &t->entry );
}

static VOID MSI_CloseDatabase( MSIOBJECTHDR *arg )
{
    MSIDATABASE *db = (MSIDATABASE *) arg;

    free( db->path );
    free_streams( db );
    free_cached_tables( db );
    free_transforms( db );
    if (db->strings) msi_destroy_stringtable( db->strings );
    IStorage_Release( db->storage );
    if (db->deletefile)
    {
        DeleteFileW( db->deletefile );
        free( db->deletefile );
    }
    free( db->tempfolder );
}

static HRESULT db_initialize( IStorage *stg, const GUID *clsid )
{
    HRESULT hr;

    hr = IStorage_SetClass( stg, clsid );
    if (FAILED( hr ))
    {
        WARN("failed to set class id %#lx\n", hr);
        return hr;
    }

    /* create the _Tables stream */
    hr = write_stream_data( stg, L"_Tables", NULL, 0, TRUE );
    if (FAILED( hr ))
    {
        WARN("failed to create _Tables stream %#lx\n", hr);
        return hr;
    }

    hr = msi_init_string_table( stg );
    if (FAILED( hr ))
    {
        WARN("failed to initialize string table %#lx\n", hr);
        return hr;
    }

    hr = IStorage_Commit( stg, 0 );
    if (FAILED( hr ))
    {
        WARN("failed to commit changes %#lx\n", hr);
        return hr;
    }

    return S_OK;
}

UINT MSI_OpenDatabaseW(LPCWSTR szDBPath, LPCWSTR szPersist, MSIDATABASE **pdb)
{
    IStorage *stg = NULL;
    HRESULT r;
    MSIDATABASE *db = NULL;
    UINT ret = ERROR_FUNCTION_FAILED;
    LPCWSTR save_path;
    UINT mode;
    STATSTG stat;
    BOOL created = FALSE, patch = FALSE;
    WCHAR path[MAX_PATH];

    TRACE("%s %s\n",debugstr_w(szDBPath),debugstr_w(szPersist) );

    if( !pdb )
        return ERROR_INVALID_PARAMETER;

    save_path = szDBPath;
    if ( IS_INTMSIDBOPEN(szPersist) )
    {
        mode = LOWORD(szPersist);
    }
    else
    {
        if (!CopyFileW( szDBPath, szPersist, FALSE ))
            return ERROR_OPEN_FAILED;

        szDBPath = szPersist;
        mode = MSI_OPEN_TRANSACT;
        created = TRUE;
    }

    if ((mode & MSI_OPEN_PATCHFILE) == MSI_OPEN_PATCHFILE)
    {
        TRACE("Database is a patch\n");
        mode &= ~MSI_OPEN_PATCHFILE;
        patch = TRUE;
    }

    if( mode == MSI_OPEN_READONLY )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    }
    else if( mode == MSI_OPEN_CREATE )
    {
        r = StgCreateDocfile( szDBPath,
              STGM_CREATE|STGM_TRANSACTED|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &stg );

        if( SUCCEEDED(r) )
            r = db_initialize( stg, patch ? &CLSID_MsiPatch : &CLSID_MsiDatabase );
        created = TRUE;
    }
    else if( mode == MSI_OPEN_CREATEDIRECT )
    {
        r = StgCreateDocfile( szDBPath,
              STGM_CREATE|STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &stg );

        if( SUCCEEDED(r) )
            r = db_initialize( stg, patch ? &CLSID_MsiPatch : &CLSID_MsiDatabase );
        created = TRUE;
    }
    else if( mode == MSI_OPEN_TRANSACT )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_TRANSACTED|STGM_READWRITE|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    }
    else if( mode == MSI_OPEN_DIRECT )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    }
    else
    {
        ERR("unknown flag %x\n",mode);
        return ERROR_INVALID_PARAMETER;
    }

    if( FAILED( r ) || !stg )
    {
        WARN("open failed r = %#lx for %s\n", r, debugstr_w(szDBPath));
        return ERROR_FUNCTION_FAILED;
    }

    r = IStorage_Stat( stg, &stat, STATFLAG_NONAME );
    if( FAILED( r ) )
    {
        FIXME("Failed to stat storage\n");
        goto end;
    }

    if ( !IsEqualGUID( &stat.clsid, &CLSID_MsiDatabase ) &&
         !IsEqualGUID( &stat.clsid, &CLSID_MsiPatch ) &&
         !IsEqualGUID( &stat.clsid, &CLSID_MsiTransform ) )
    {
        ERR("storage GUID is not a MSI database GUID %s\n",
             debugstr_guid(&stat.clsid) );
        goto end;
    }

    if ( patch && !IsEqualGUID( &stat.clsid, &CLSID_MsiPatch ) )
    {
        ERR("storage GUID is not the MSI patch GUID %s\n",
             debugstr_guid(&stat.clsid) );
        ret = ERROR_OPEN_FAILED;
        goto end;
    }

    db = alloc_msiobject( MSIHANDLETYPE_DATABASE, sizeof (MSIDATABASE),
                              MSI_CloseDatabase );
    if( !db )
    {
        FIXME("Failed to allocate a handle\n");
        goto end;
    }

    if (!wcschr( save_path, '\\' ))
    {
        GetCurrentDirectoryW( MAX_PATH, path );
        lstrcatW( path, L"\\" );
        lstrcatW( path, save_path );
    }
    else
        lstrcpyW( path, save_path );

    db->path = wcsdup( path );
    db->media_transform_offset = MSI_INITIAL_MEDIA_TRANSFORM_OFFSET;
    db->media_transform_disk_id = MSI_INITIAL_MEDIA_TRANSFORM_DISKID;

    if( TRACE_ON( msi ) )
        enum_stream_names( stg );

    db->storage = stg;
    db->mode = mode;
    if (created)
        db->deletefile = wcsdup( szDBPath );
    list_init( &db->tables );
    list_init( &db->transforms );

    db->strings = msi_load_string_table( stg, &db->bytes_per_strref );
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

    if( !IS_INTMSIDBOPEN(szPersist) )
    {
        szwPersist = strdupAtoW( szPersist );
        if( !szwPersist )
            goto end;
    }
    else
        szwPersist = (LPWSTR)(DWORD_PTR)szPersist;

    r = MsiOpenDatabaseW( szwDBPath, szwPersist, phDB );

end:
    if( !IS_INTMSIDBOPEN(szPersist) )
        free( szwPersist );
    free( szwDBPath );

    return r;
}

static WCHAR *read_text_archive(const WCHAR *path, DWORD *len)
{
    HANDLE file;
    LPSTR data = NULL;
    LPWSTR wdata = NULL;
    DWORD read, size = 0;

    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (file == INVALID_HANDLE_VALUE)
        return NULL;

    size = GetFileSize( file, NULL );
    if (!(data = malloc( size ))) goto done;

    if (!ReadFile( file, data, size, &read, NULL ) || read != size) goto done;

    while (!data[size - 1]) size--;
    *len = MultiByteToWideChar( CP_ACP, 0, data, size, NULL, 0 );
    if ((wdata = malloc( (*len + 1) * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, data, size, wdata, *len );
        wdata[*len] = 0;
    }

done:
    CloseHandle( file );
    free( data );
    return wdata;
}

static UINT parse_line(WCHAR **line, WCHAR ***entries, DWORD *num_entries, DWORD *len)
{
    LPWSTR ptr = *line, save;
    DWORD i, count = 1, chars_left = *len;

    *entries = NULL;

    /* stay on this line */
    while (chars_left && *ptr != '\n')
    {
        /* entries are separated by tabs */
        if (*ptr == '\t')
            count++;

        ptr++;
        chars_left--;
    }

    /*
     * make sure this line has the same number of entries as there are columns
     * which are indicated by the first line.
     */
    if (*num_entries && *num_entries != count)
        return ERROR_FUNCTION_FAILED;

    *entries = malloc(count * sizeof(WCHAR *));
    if (!*entries)
        return ERROR_OUTOFMEMORY;

    /* store pointers into the data */
    chars_left = *len;
    for (i = 0, ptr = *line; i < count; i++)
    {
        while (chars_left && *ptr == '\r')
        {
            ptr++;
            chars_left--;
        }
        save = ptr;

        while (chars_left && *ptr != '\t' && *ptr != '\n' && *ptr != '\r')
        {
            if (!*ptr) *ptr = '\n'; /* convert embedded nulls to \n */
            if (ptr > *line && *ptr == '\x19' && *(ptr - 1) == '\x11')
            {
                *ptr = '\n';
                *(ptr - 1) = '\r';
            }
            ptr++;
            chars_left--;
        }

        /* NULL-separate the data */
        if (*ptr == '\n' || *ptr == '\r')
        {
            while (chars_left && (*ptr == '\n' || *ptr == '\r'))
            {
                *(ptr++) = 0;
                chars_left--;
            }
        }
        else if (*ptr)
        {
            *(ptr++) = 0;
            chars_left--;
        }
        (*entries)[i] = save;
    }

    /* move to the next line if there's more, else EOF */
    *line = ptr;
    *len = chars_left;
    if (!*num_entries)
        *num_entries = count;

    return ERROR_SUCCESS;
}

static WCHAR *build_createsql_prelude(const WCHAR *table)
{
    LPWSTR prelude;
    DWORD size;

    size = ARRAY_SIZE(L"CREATE TABLE `%s` ( ") + lstrlenW(table) - 2;
    prelude = malloc(size * sizeof(WCHAR));
    if (!prelude)
        return NULL;

    swprintf(prelude, size, L"CREATE TABLE `%s` ( ", table);
    return prelude;
}

static WCHAR *build_createsql_columns(WCHAR **columns_data, WCHAR **types, DWORD num_columns)
{
    LPWSTR columns, p;
    LPCWSTR type;
    DWORD sql_size = 1, i, len;
    WCHAR expanded[128], *ptr;
    WCHAR size[10], comma[2], extra[30];

    columns = calloc(sql_size, sizeof(WCHAR));
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
        len = wcstol(ptr, NULL, 10);
        extra[0] = '\0';

        switch (types[i][0])
        {
            case 'l':
                lstrcpyW(extra, L" NOT NULL");
                /* fall through */
            case 'L':
                lstrcatW(extra, L" LOCALIZABLE");
                type = L"CHAR";
                swprintf(size, ARRAY_SIZE(size), L"(%s)", ptr);
                break;
            case 's':
                lstrcpyW(extra, L" NOT NULL");
                /* fall through */
            case 'S':
                type = L"CHAR";
                swprintf(size, ARRAY_SIZE(size), L"(%s)", ptr);
                break;
            case 'i':
                lstrcpyW(extra, L" NOT NULL");
                /* fall through */
            case 'I':
                if (len <= 2)
                    type = L"INT";
                else if (len == 4)
                    type = L"LONG";
                else
                {
                    WARN("invalid int width %lu\n", len);
                    free(columns);
                    return NULL;
                }
                break;
            case 'v':
                lstrcpyW(extra, L" NOT NULL");
                /* fall through */
            case 'V':
                type = L"OBJECT";
                break;
            default:
                ERR("Unknown type: %c\n", types[i][0]);
                free(columns);
                return NULL;
        }

        swprintf(expanded, ARRAY_SIZE(expanded), L"`%s` %s%s%s%s ", columns_data[i], type, size, extra, comma);
        sql_size += lstrlenW(expanded);

        p = realloc(columns, sql_size * sizeof(WCHAR));
        if (!p)
        {
            free(columns);
            return NULL;
        }
        columns = p;

        lstrcatW(columns, expanded);
    }

    return columns;
}

static WCHAR *build_createsql_postlude(WCHAR **primary_keys, DWORD num_keys)
{
    LPWSTR postlude, keys, ptr;
    DWORD size, i;

    for (i = 0, size = 1; i < num_keys; i++)
        size += lstrlenW(L"`%s`, ") + lstrlenW(primary_keys[i]) - 2;

    keys = malloc(size * sizeof(WCHAR));
    if (!keys)
        return NULL;

    for (i = 0, ptr = keys; i < num_keys; i++)
    {
        ptr += swprintf(ptr, size - (ptr - keys), L"`%s`, ", primary_keys[i]);
    }

    /* remove final ', ' */
    *(ptr - 2) = '\0';

    size = lstrlenW(L"PRIMARY KEY %s)") + size - 1;
    postlude = malloc(size * sizeof(WCHAR));
    if (!postlude)
        goto done;

    swprintf(postlude, size, L"PRIMARY KEY %s)", keys);

done:
    free(keys);
    return postlude;
}

static UINT add_table_to_db(MSIDATABASE *db, WCHAR **columns, WCHAR **types, WCHAR **labels, DWORD num_labels,
                            DWORD num_columns)
{
    UINT r = ERROR_OUTOFMEMORY;
    DWORD size;
    MSIQUERY *view;
    LPWSTR create_sql = NULL;
    LPWSTR prelude, columns_sql, postlude;

    prelude = build_createsql_prelude(labels[0]);
    columns_sql = build_createsql_columns(columns, types, num_columns);
    postlude = build_createsql_postlude(labels + 1, num_labels - 1); /* skip over table name */

    if (!prelude || !columns_sql || !postlude)
        goto done;

    size = lstrlenW(prelude) + lstrlenW(columns_sql) + lstrlenW(postlude) + 1;
    create_sql = malloc(size * sizeof(WCHAR));
    if (!create_sql)
        goto done;

    lstrcpyW(create_sql, prelude);
    lstrcatW(create_sql, columns_sql);
    lstrcatW(create_sql, postlude);

    r = MSI_DatabaseOpenViewW( db, create_sql, &view );
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewExecute(view, NULL);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

done:
    free(prelude);
    free(columns_sql);
    free(postlude);
    free(create_sql);
    return r;
}

static WCHAR *import_stream_filename(const WCHAR *path, const WCHAR *name)
{
    DWORD len;
    LPWSTR fullname, ptr;

    len = lstrlenW(path) + lstrlenW(name) + 1;
    fullname = malloc(len * sizeof(WCHAR));
    if (!fullname)
       return NULL;

    lstrcpyW( fullname, path );

    /* chop off extension from path */
    ptr = wcsrchr(fullname, '.');
    if (!ptr)
    {
        free(fullname);
        return NULL;
    }
    *ptr++ = '\\';
    lstrcpyW( ptr, name );
    return fullname;
}

static UINT construct_record(DWORD num_columns, LPWSTR *types,
                             LPWSTR *data, LPWSTR path, MSIRECORD **rec)
{
    UINT i;

    *rec = MSI_CreateRecord(num_columns);
    if (!*rec)
        return ERROR_OUTOFMEMORY;

    for (i = 0; i < num_columns; i++)
    {
        switch (types[i][0])
        {
            case 'L': case 'l': case 'S': case 's':
                MSI_RecordSetStringW(*rec, i + 1, data[i]);
                break;
            case 'I': case 'i':
                if (*data[i])
                    MSI_RecordSetInteger(*rec, i + 1, wcstol(data[i], NULL, 10));
                break;
            case 'V': case 'v':
                if (*data[i])
                {
                    UINT r;
                    WCHAR *file = import_stream_filename(path, data[i]);
                    if (!file)
                        return ERROR_FUNCTION_FAILED;

                    r = MSI_RecordSetStreamFromFileW(*rec, i + 1, file);
                    free (file);
                    if (r != ERROR_SUCCESS)
                        return ERROR_FUNCTION_FAILED;
                }
                break;
            default:
                ERR("Unhandled column type: %c\n", types[i][0]);
                msiobj_release(&(*rec)->hdr);
                return ERROR_FUNCTION_FAILED;
        }
    }

    return ERROR_SUCCESS;
}

static UINT add_records_to_table(MSIDATABASE *db, WCHAR **columns, WCHAR **types, WCHAR **labels, WCHAR ***records,
                                 int num_columns, int num_records, WCHAR *path)
{
    UINT r;
    int i;
    MSIQUERY *view;
    MSIRECORD *rec;

    r = MSI_OpenQuery(db, &view, L"SELECT * FROM `%s`", labels[0]);
    if (r != ERROR_SUCCESS)
        return r;

    while (MSI_ViewFetch(view, &rec) != ERROR_NO_MORE_ITEMS)
    {
        r = MSI_ViewModify(view, MSIMODIFY_DELETE, rec);
        msiobj_release(&rec->hdr);
        if (r != ERROR_SUCCESS)
            goto done;
    }

    for (i = 0; i < num_records; i++)
    {
        r = construct_record(num_columns, types, records[i], path, &rec);
        if (r != ERROR_SUCCESS)
            goto done;

        r = MSI_ViewModify(view, MSIMODIFY_INSERT, rec);
        if (r != ERROR_SUCCESS)
        {
            msiobj_release(&rec->hdr);
            goto done;
        }

        msiobj_release(&rec->hdr);
    }

done:
    msiobj_release(&view->hdr);
    return r;
}

static UINT MSI_DatabaseImport(MSIDATABASE *db, LPCWSTR folder, LPCWSTR file)
{
    UINT r;
    DWORD len, i, num_labels = 0, num_types = 0, num_columns = 0, num_records = 0;
    WCHAR **columns, **types, **labels, *path, *ptr, *data, ***records = NULL, ***temp_records;

    TRACE("%p %s %s\n", db, debugstr_w(folder), debugstr_w(file) );

    if (!folder || !file)
        return ERROR_INVALID_PARAMETER;

    len = lstrlenW(folder) + lstrlenW(L"\\") + lstrlenW(file) + 1;
    path = malloc( len * sizeof(WCHAR) );
    if (!path)
        return ERROR_OUTOFMEMORY;

    lstrcpyW( path, folder );
    lstrcatW( path, L"\\" );
    lstrcatW( path, file );

    data = read_text_archive( path, &len );
    if (!data)
    {
        free(path);
        return ERROR_FUNCTION_FAILED;
    }

    ptr = data;
    parse_line( &ptr, &columns, &num_columns, &len );
    parse_line( &ptr, &types, &num_types, &len );
    parse_line( &ptr, &labels, &num_labels, &len );

    if (num_columns == 1 && !columns[0][0] && num_labels == 1 && !labels[0][0] &&
        num_types == 2 && !wcscmp( types[1], L"_ForceCodepage" ))
    {
        r = msi_set_string_table_codepage( db->strings, wcstol( types[0], NULL, 10 ) );
        goto done;
    }

    if (num_columns != num_types)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    records = malloc(sizeof(WCHAR **));
    if (!records)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    /* read in the table records */
    while (len)
    {
        r = parse_line( &ptr, &records[num_records], &num_columns, &len );
        if (r != ERROR_SUCCESS)
            goto done;

        num_records++;
        temp_records = realloc(records, (num_records + 1) * sizeof(WCHAR **));
        if (!temp_records)
        {
            r = ERROR_OUTOFMEMORY;
            goto done;
        }
        records = temp_records;
    }

    if (!wcscmp(labels[0], L"_SummaryInformation"))
    {
        r = msi_add_suminfo( db, records, num_records, num_columns );
        if (r != ERROR_SUCCESS)
        {
            r = ERROR_FUNCTION_FAILED;
            goto done;
        }
    }
    else
    {
        if (!TABLE_Exists(db, labels[0]))
        {
            r = add_table_to_db( db, columns, types, labels, num_labels, num_columns );
            if (r != ERROR_SUCCESS)
            {
                r = ERROR_FUNCTION_FAILED;
                goto done;
            }
        }

        r = add_records_to_table( db, columns, types, labels, records, num_columns, num_records, path );
    }

done:
    free(path);
    free(data);
    free(columns);
    free(types);
    free(labels);

    for (i = 0; i < num_records; i++)
        free(records[i]);

    free(records);
    return r;
}

UINT WINAPI MsiDatabaseImportW( MSIHANDLE handle, const WCHAR *szFolder, const WCHAR *szFilename )
{
    MSIDATABASE *db;
    UINT r;

    TRACE( "%lu %s %s\n", handle, debugstr_w(szFolder), debugstr_w(szFilename) );

    if (!(db = msihandle2msiinfo(handle, MSIHANDLETYPE_DATABASE)))
        return ERROR_INVALID_HANDLE;

    r = MSI_DatabaseImport( db, szFolder, szFilename );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseImportA( MSIHANDLE handle, const char *szFolder, const char *szFilename )
{
    WCHAR *path = NULL, *file = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE( "%lu %s %s\n", handle, debugstr_a(szFolder), debugstr_a(szFilename) );

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
    free( path );
    free( file );

    return r;
}

static UINT export_field( HANDLE handle, MSIRECORD *row, UINT field )
{
    char *buffer, *ptr;
    BOOL ret;
    DWORD sz = 0x100;
    UINT r;

    buffer = malloc( sz );
    if (!buffer)
        return ERROR_OUTOFMEMORY;

    r = MSI_RecordGetStringA( row, field, buffer, &sz );
    if (r == ERROR_MORE_DATA)
    {
        char *tmp;

        sz++; /* leave room for NULL terminator */
        tmp = realloc( buffer, sz );
        if (!tmp)
        {
            free( buffer );
            return ERROR_OUTOFMEMORY;
        }
        buffer = tmp;

        r = MSI_RecordGetStringA( row, field, buffer, &sz );
        if (r != ERROR_SUCCESS)
        {
            free( buffer );
            return r;
        }
    }
    else if (r != ERROR_SUCCESS)
    {
        free( buffer );
        return r;
    }

    ptr = buffer;
    while( *ptr )
    {
         if (*ptr == '\r' && *( ptr + 1 ) == '\n')
         {
             *ptr++ = '\x11';
             *ptr++ = '\x19';
             continue;
         }

         if (*ptr == '\n')
             *ptr = '\x19';

         ptr++;
    }

    ret = WriteFile( handle, buffer, sz, &sz, NULL );
    free( buffer );
    return ret ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

static UINT export_stream( const WCHAR *folder, const WCHAR *table, MSIRECORD *row, UINT field, UINT start )
{
    WCHAR stream[MAX_STREAM_NAME_LEN + 1], *path;
    DWORD sz, read_size, write_size;
    char buffer[1024];
    HANDLE file;
    UINT len, r;

    sz = ARRAY_SIZE( stream );
    r = MSI_RecordGetStringW( row, start, stream, &sz );
    if (r != ERROR_SUCCESS)
        return r;

    len = sz + lstrlenW( folder ) + lstrlenW( table ) + ARRAY_SIZE( L"%s\\%s" ) + 1;
    if (!(path = malloc( len * sizeof(WCHAR) )))
        return ERROR_OUTOFMEMORY;

    len = swprintf( path, len, L"%s\\%s", folder, table );
    if (!CreateDirectoryW( path, NULL ) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        free( path );
        return ERROR_FUNCTION_FAILED;
    }

    path[len++] = '\\';
    lstrcpyW( path + len, stream );
    file = CreateFileW( path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    free( path );
    if (file == INVALID_HANDLE_VALUE)
        return ERROR_FUNCTION_FAILED;

    read_size = sizeof(buffer);
    while (read_size == sizeof(buffer))
    {
        r = MSI_RecordReadStream( row, field, buffer, &read_size );
        if (r != ERROR_SUCCESS)
        {
            CloseHandle( file );
            return r;
        }
        if (!WriteFile( file, buffer, read_size, &write_size, NULL ) || read_size != write_size)
        {
            CloseHandle( file );
            return ERROR_WRITE_FAULT;
        }
    }
    CloseHandle( file );
    return r;
}

struct row_export_info
{
    HANDLE       handle;
    const WCHAR *folder;
    const WCHAR *table;
};

static UINT export_record( struct row_export_info *row_export_info, MSIRECORD *row, UINT start )
{
    HANDLE handle = row_export_info->handle;
    UINT i, count, r = ERROR_SUCCESS;
    const char *sep;
    DWORD sz;

    count = MSI_RecordGetFieldCount( row );
    for (i = start; i <= count; i++)
    {
        r = export_field( handle, row, i );
        if (r == ERROR_INVALID_PARAMETER)
        {
            r = export_stream( row_export_info->folder, row_export_info->table, row, i, start );
            if (r != ERROR_SUCCESS)
                return r;

            /* exporting a binary stream, repeat the "Name" field */
            r = export_field( handle, row, start );
            if (r != ERROR_SUCCESS)
                return r;
        }
        else if (r != ERROR_SUCCESS)
            return r;

        sep = (i < count) ? "\t" : "\r\n";
        if (!WriteFile( handle, sep, strlen(sep), &sz, NULL ))
            return ERROR_FUNCTION_FAILED;
    }
    return r;
}

static UINT export_row( MSIRECORD *row, void *arg )
{
    return export_record( arg, row, 1 );
}

static UINT export_forcecodepage( HANDLE handle, UINT codepage )
{
    static const char fmt[] = "\r\n\r\n%u\t_ForceCodepage\r\n";
    char data[sizeof(fmt) + 10];
    DWORD sz = sprintf( data, fmt, codepage );

    if (!WriteFile(handle, data, sz, &sz, NULL))
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

static UINT export_summaryinformation( MSIDATABASE *db, HANDLE handle )
{
    static const char header[] = "PropertyId\tValue\r\n"
                                 "i2\tl255\r\n"
                                 "_SummaryInformation\tPropertyId\r\n";
    DWORD sz = ARRAY_SIZE(header) - 1;

    if (!WriteFile(handle, header, sz, &sz, NULL))
        return ERROR_WRITE_FAULT;

    return msi_export_suminfo( db, handle );
}

static UINT MSI_DatabaseExport( MSIDATABASE *db, LPCWSTR table, LPCWSTR folder, LPCWSTR file )
{
    MSIRECORD *rec = NULL;
    MSIQUERY *view = NULL;
    WCHAR *filename;
    HANDLE handle;
    UINT len, r;

    TRACE("%p %s %s %s\n", db, debugstr_w(table),
          debugstr_w(folder), debugstr_w(file) );

    if (!folder || !file)
        return ERROR_INVALID_PARAMETER;

    len = lstrlenW(folder) + lstrlenW(file) + 2;
    filename = malloc(len * sizeof(WCHAR));
    if (!filename)
        return ERROR_OUTOFMEMORY;

    lstrcpyW( filename, folder );
    lstrcatW( filename, L"\\" );
    lstrcatW( filename, file );

    handle = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    free( filename );
    if (handle == INVALID_HANDLE_VALUE)
        return ERROR_FUNCTION_FAILED;

    if (!wcscmp( table, L"_ForceCodepage" ))
    {
        UINT codepage = msi_get_string_table_codepage( db->strings );
        r = export_forcecodepage( handle, codepage );
        goto done;
    }

    if (!wcscmp( table, L"_SummaryInformation" ))
    {
        r = export_summaryinformation( db, handle );
        goto done;
    }

    r = MSI_OpenQuery( db, &view, L"SELECT * FROM %s", table );
    if (r == ERROR_SUCCESS)
    {
        struct row_export_info row_export_info = { handle, folder, table };

        /* write out row 1, the column names */
        r = MSI_ViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
        if (r == ERROR_SUCCESS)
        {
            export_record( &row_export_info, rec, 1 );
            msiobj_release( &rec->hdr );
        }

        /* write out row 2, the column types */
        r = MSI_ViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
        if (r == ERROR_SUCCESS)
        {
            export_record( &row_export_info, rec, 1 );
            msiobj_release( &rec->hdr );
        }

        /* write out row 3, the table name + keys */
        r = MSI_DatabaseGetPrimaryKeys( db, table, &rec );
        if (r == ERROR_SUCCESS)
        {
            MSI_RecordSetStringW( rec, 0, table );
            export_record( &row_export_info, rec, 0 );
            msiobj_release( &rec->hdr );
        }

        /* write out row 4 onwards, the data */
        r = MSI_IterateRecords( view, 0, export_row, &row_export_info );
        msiobj_release( &view->hdr );
    }

done:
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
UINT WINAPI MsiDatabaseExportW( MSIHANDLE handle, const WCHAR *szTable, const WCHAR *szFolder, const WCHAR *szFilename )
{
    MSIDATABASE *db;
    UINT r;

    TRACE( "%lu %s %s %s\n", handle, debugstr_w(szTable), debugstr_w(szFolder), debugstr_w(szFilename) );

    if (!(db = msihandle2msiinfo(handle, MSIHANDLETYPE_DATABASE)))
        return ERROR_INVALID_HANDLE;

    r = MSI_DatabaseExport( db, szTable, szFolder, szFilename );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseExportA( MSIHANDLE handle, const char *szTable, const char *szFolder, const char *szFilename )
{
    WCHAR *path = NULL, *file = NULL, *table = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE( "%lu %s %s %s\n", handle, debugstr_a(szTable), debugstr_a(szFolder), debugstr_a(szFilename) );

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
    free( table );
    free( path );
    free( file );

    return r;
}

UINT WINAPI MsiDatabaseMergeA( MSIHANDLE hDatabase, MSIHANDLE hDatabaseMerge, const char *szTableName )
{
    UINT r;
    WCHAR *table;

    TRACE("%lu, %lu, %s\n", hDatabase, hDatabaseMerge, debugstr_a(szTableName) );

    table = strdupAtoW(szTableName);
    r = MsiDatabaseMergeW(hDatabase, hDatabaseMerge, table);

    free(table);
    return r;
}

struct merge_table
{
    struct list entry;
    struct list rows;
    LPWSTR name;
    DWORD numconflicts;
    LPWSTR *columns;
    DWORD numcolumns;
    LPWSTR *types;
    DWORD numtypes;
    LPWSTR *labels;
    DWORD numlabels;
};

struct merge_row
{
    struct list entry;
    MSIRECORD *data;
};

struct merge_data
{
    MSIDATABASE *db;
    MSIDATABASE *merge;
    struct merge_table *curtable;
    MSIQUERY *curview;
    struct list *tabledata;
};

static BOOL merge_type_match(LPCWSTR type1, LPCWSTR type2)
{
    if (((type1[0] == 'l') || (type1[0] == 's')) &&
        ((type2[0] == 'l') || (type2[0] == 's')))
        return TRUE;

    if (((type1[0] == 'L') || (type1[0] == 'S')) &&
        ((type2[0] == 'L') || (type2[0] == 'S')))
        return TRUE;

    return !wcscmp( type1, type2 );
}

static UINT merge_verify_colnames(MSIQUERY *dbview, MSIQUERY *mergeview)
{
    MSIRECORD *dbrec, *mergerec;
    UINT r, i, count;

    r = MSI_ViewGetColumnInfo(dbview, MSICOLINFO_NAMES, &dbrec);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewGetColumnInfo(mergeview, MSICOLINFO_NAMES, &mergerec);
    if (r != ERROR_SUCCESS)
    {
        msiobj_release(&dbrec->hdr);
        return r;
    }

    count = MSI_RecordGetFieldCount(dbrec);
    for (i = 1; i <= count; i++)
    {
        if (!MSI_RecordGetString(mergerec, i))
            break;

        if (wcscmp( MSI_RecordGetString( dbrec, i ), MSI_RecordGetString( mergerec, i ) ))
        {
            r = ERROR_DATATYPE_MISMATCH;
            goto done;
        }
    }

    msiobj_release(&dbrec->hdr);
    msiobj_release(&mergerec->hdr);
    dbrec = mergerec = NULL;

    r = MSI_ViewGetColumnInfo(dbview, MSICOLINFO_TYPES, &dbrec);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewGetColumnInfo(mergeview, MSICOLINFO_TYPES, &mergerec);
    if (r != ERROR_SUCCESS)
    {
        msiobj_release(&dbrec->hdr);
        return r;
    }

    count = MSI_RecordGetFieldCount(dbrec);
    for (i = 1; i <= count; i++)
    {
        if (!MSI_RecordGetString(mergerec, i))
            break;

        if (!merge_type_match(MSI_RecordGetString(dbrec, i),
                     MSI_RecordGetString(mergerec, i)))
        {
            r = ERROR_DATATYPE_MISMATCH;
            break;
        }
    }

done:
    msiobj_release(&dbrec->hdr);
    msiobj_release(&mergerec->hdr);

    return r;
}

static UINT merge_verify_primary_keys(MSIDATABASE *db, MSIDATABASE *mergedb,
                                      LPCWSTR table)
{
    MSIRECORD *dbrec, *mergerec = NULL;
    UINT r, i, count;

    r = MSI_DatabaseGetPrimaryKeys(db, table, &dbrec);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_DatabaseGetPrimaryKeys(mergedb, table, &mergerec);
    if (r != ERROR_SUCCESS)
        goto done;

    count = MSI_RecordGetFieldCount(dbrec);
    if (count != MSI_RecordGetFieldCount(mergerec))
    {
        r = ERROR_DATATYPE_MISMATCH;
        goto done;
    }

    for (i = 1; i <= count; i++)
    {
        if (wcscmp( MSI_RecordGetString( dbrec, i ), MSI_RecordGetString( mergerec, i ) ))
        {
            r = ERROR_DATATYPE_MISMATCH;
            goto done;
        }
    }

done:
    msiobj_release(&dbrec->hdr);
    msiobj_release(&mergerec->hdr);

    return r;
}

static LPWSTR get_key_value(MSIQUERY *view, LPCWSTR key, MSIRECORD *rec)
{
    MSIRECORD *colnames;
    LPWSTR str, val;
    UINT r, i = 0;
    DWORD sz = 0;
    int cmp;

    r = MSI_ViewGetColumnInfo(view, MSICOLINFO_NAMES, &colnames);
    if (r != ERROR_SUCCESS)
        return NULL;

    do
    {
        str = msi_dup_record_field(colnames, ++i);
        cmp = wcscmp( key, str );
        free(str);
    } while (cmp);

    msiobj_release(&colnames->hdr);

    r = MSI_RecordGetStringW(rec, i, NULL, &sz);
    if (r != ERROR_SUCCESS)
        return NULL;
    sz++;

    if (MSI_RecordGetString(rec, i))  /* check record field is a string */
    {
        /* quote string record fields */
        sz += 2;
        val = malloc(sz * sizeof(WCHAR));
        if (!val)
            return NULL;

        lstrcpyW(val, L"'");
        r = MSI_RecordGetStringW(rec, i, val + 1, &sz);
        lstrcpyW(val + 1 + sz, L"'");
    }
    else
    {
        /* do not quote integer record fields */
        val = malloc(sz * sizeof(WCHAR));
        if (!val)
            return NULL;

        r = MSI_RecordGetStringW(rec, i, val, &sz);
    }

    if (r != ERROR_SUCCESS)
    {
        ERR("failed to get string!\n");
        free(val);
        return NULL;
    }

    return val;
}

static LPWSTR create_diff_row_query(MSIDATABASE *merge, MSIQUERY *view,
                                    LPWSTR table, MSIRECORD *rec)
{
    WCHAR *query = NULL, *clause = NULL, *new_clause, *val;
    LPCWSTR setptr, key;
    DWORD size, oldsize;
    MSIRECORD *keys;
    UINT r, i, count;

    r = MSI_DatabaseGetPrimaryKeys(merge, table, &keys);
    if (r != ERROR_SUCCESS)
        return NULL;

    size = 1;
    count = MSI_RecordGetFieldCount(keys);
    for (i = 1; i <= count; i++)
    {
        key = MSI_RecordGetString(keys, i);
        val = get_key_value(view, key, rec);

        if (i == count)
            setptr = L"`%s` = %s ";
        else
            setptr = L"`%s` = %s AND ";

        oldsize = size;
        size += lstrlenW(setptr) + lstrlenW(key) + lstrlenW(val) - 4;
        new_clause = realloc(clause, size * sizeof(WCHAR));
        if (!new_clause)
        {
            free(val);
            goto done;
        }
        clause = new_clause;

        swprintf(clause + oldsize - 1, size - (oldsize - 1), setptr, key, val);
        free(val);
    }

    size = lstrlenW(L"SELECT * FROM `%s` WHERE %s") + lstrlenW(table) + lstrlenW(clause) + 1;
    query = malloc(size * sizeof(WCHAR));
    if (!query)
        goto done;

    swprintf(query, size, L"SELECT * FROM `%s` WHERE %s", table, clause);

done:
    free(clause);
    msiobj_release(&keys->hdr);
    return query;
}

static UINT merge_diff_row(MSIRECORD *rec, LPVOID param)
{
    struct merge_data *data = param;
    struct merge_table *table = data->curtable;
    struct merge_row *mergerow;
    MSIQUERY *dbview = NULL;
    MSIRECORD *row = NULL;
    LPWSTR query = NULL;
    UINT r = ERROR_SUCCESS;

    if (TABLE_Exists(data->db, table->name))
    {
        query = create_diff_row_query(data->merge, data->curview, table->name, rec);
        if (!query)
            return ERROR_OUTOFMEMORY;

        r = MSI_DatabaseOpenViewW(data->db, query, &dbview);
        if (r != ERROR_SUCCESS)
            goto done;

        r = MSI_ViewExecute(dbview, NULL);
        if (r != ERROR_SUCCESS)
            goto done;

        r = MSI_ViewFetch(dbview, &row);
        if (r == ERROR_SUCCESS && !MSI_RecordsAreEqual(rec, row))
        {
            table->numconflicts++;
            goto done;
        }
        else if (r != ERROR_NO_MORE_ITEMS)
            goto done;

        r = ERROR_SUCCESS;
    }

    mergerow = malloc(sizeof(*mergerow));
    if (!mergerow)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    mergerow->data = MSI_CloneRecord(rec);
    if (!mergerow->data)
    {
        r = ERROR_OUTOFMEMORY;
        free(mergerow);
        goto done;
    }

    list_add_tail(&table->rows, &mergerow->entry);

done:
    free(query);
    msiobj_release(&row->hdr);
    msiobj_release(&dbview->hdr);
    return r;
}

static UINT get_table_labels(MSIDATABASE *db, const WCHAR *table, WCHAR ***labels, DWORD *numlabels)
{
    UINT r, i, count;
    MSIRECORD *prec = NULL;

    r = MSI_DatabaseGetPrimaryKeys(db, table, &prec);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(prec);
    *numlabels = count + 1;
    *labels = malloc((*numlabels) * sizeof(WCHAR *));
    if (!*labels)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    (*labels)[0] = wcsdup(table);
    for (i=1; i<=count; i++ )
    {
        (*labels)[i] = wcsdup(MSI_RecordGetString(prec, i));
    }

end:
    msiobj_release( &prec->hdr );
    return r;
}

static UINT get_query_columns(MSIQUERY *query, WCHAR ***columns, DWORD *numcolumns)
{
    UINT r, i, count;
    MSIRECORD *prec = NULL;

    r = MSI_ViewGetColumnInfo(query, MSICOLINFO_NAMES, &prec);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(prec);
    *columns = malloc(count * sizeof(WCHAR *));
    if (!*columns)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    for (i=1; i<=count; i++ )
    {
        (*columns)[i-1] = wcsdup(MSI_RecordGetString(prec, i));
    }

    *numcolumns = count;

end:
    msiobj_release( &prec->hdr );
    return r;
}

static UINT get_query_types(MSIQUERY *query, WCHAR ***types, DWORD *numtypes)
{
    UINT r, i, count;
    MSIRECORD *prec = NULL;

    r = MSI_ViewGetColumnInfo(query, MSICOLINFO_TYPES, &prec);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(prec);
    *types = malloc(count * sizeof(WCHAR *));
    if (!*types)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    *numtypes = count;
    for (i=1; i<=count; i++ )
    {
        (*types)[i-1] = wcsdup(MSI_RecordGetString(prec, i));
    }

end:
    msiobj_release( &prec->hdr );
    return r;
}

static void merge_free_rows(struct merge_table *table)
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE(item, cursor, &table->rows)
    {
        struct merge_row *row = LIST_ENTRY(item, struct merge_row, entry);

        list_remove(&row->entry);
        msiobj_release(&row->data->hdr);
        free(row);
    }
}

static void free_merge_table(struct merge_table *table)
{
    UINT i;

    if (table->labels != NULL)
    {
        for (i = 0; i < table->numlabels; i++)
            free(table->labels[i]);

        free(table->labels);
    }

    if (table->columns != NULL)
    {
        for (i = 0; i < table->numcolumns; i++)
            free(table->columns[i]);

        free(table->columns);
    }

    if (table->types != NULL)
    {
        for (i = 0; i < table->numtypes; i++)
            free(table->types[i]);

        free(table->types);
    }

    free(table->name);
    merge_free_rows(table);

    free(table);
}

static UINT get_merge_table(MSIDATABASE *db, const WCHAR *name, struct merge_table **ptable)
{
    UINT r;
    struct merge_table *table;
    MSIQUERY *mergeview = NULL;

    table = calloc(1, sizeof(*table));
    if (!table)
    {
       *ptable = NULL;
       return ERROR_OUTOFMEMORY;
    }

    r = get_table_labels(db, name, &table->labels, &table->numlabels);
    if (r != ERROR_SUCCESS)
        goto err;

    r = MSI_OpenQuery(db, &mergeview, L"SELECT * FROM `%s`", name);
    if (r != ERROR_SUCCESS)
        goto err;

    r = get_query_columns(mergeview, &table->columns, &table->numcolumns);
    if (r != ERROR_SUCCESS)
        goto err;

    r = get_query_types(mergeview, &table->types, &table->numtypes);
    if (r != ERROR_SUCCESS)
        goto err;

    list_init(&table->rows);

    table->name = wcsdup(name);
    table->numconflicts = 0;

    msiobj_release(&mergeview->hdr);
    *ptable = table;
    return ERROR_SUCCESS;

err:
    msiobj_release(&mergeview->hdr);
    free_merge_table(table);
    *ptable = NULL;
    return r;
}

static UINT merge_diff_tables(MSIRECORD *rec, LPVOID param)
{
    struct merge_data *data = param;
    struct merge_table *table;
    MSIQUERY *dbview = NULL;
    MSIQUERY *mergeview = NULL;
    LPCWSTR name;
    UINT r;

    name = MSI_RecordGetString(rec, 1);

    r = MSI_OpenQuery(data->merge, &mergeview, L"SELECT * FROM `%s`", name);
    if (r != ERROR_SUCCESS)
        goto done;

    if (TABLE_Exists(data->db, name))
    {
        r = MSI_OpenQuery(data->db, &dbview, L"SELECT * FROM `%s`", name);
        if (r != ERROR_SUCCESS)
            goto done;

        r = merge_verify_colnames(dbview, mergeview);
        if (r != ERROR_SUCCESS)
            goto done;

        r = merge_verify_primary_keys(data->db, data->merge, name);
        if (r != ERROR_SUCCESS)
            goto done;
    }

    r = get_merge_table(data->merge, name, &table);
    if (r != ERROR_SUCCESS)
        goto done;

    data->curtable = table;
    data->curview = mergeview;
    r = MSI_IterateRecords(mergeview, NULL, merge_diff_row, data);
    if (r != ERROR_SUCCESS)
    {
        free_merge_table(table);
        goto done;
    }

    list_add_tail(data->tabledata, &table->entry);

done:
    msiobj_release(&dbview->hdr);
    msiobj_release(&mergeview->hdr);
    return r;
}

static UINT gather_merge_data(MSIDATABASE *db, MSIDATABASE *merge,
                              struct list *tabledata)
{
    MSIQUERY *view;
    struct merge_data data;
    UINT r;

    r = MSI_DatabaseOpenViewW(merge, L"SELECT * FROM `_Tables`", &view);
    if (r != ERROR_SUCCESS)
        return r;

    data.db = db;
    data.merge = merge;
    data.tabledata = tabledata;
    r = MSI_IterateRecords(view, NULL, merge_diff_tables, &data);
    msiobj_release(&view->hdr);
    return r;
}

static UINT merge_table(MSIDATABASE *db, struct merge_table *table)
{
    UINT r;
    struct merge_row *row;
    MSIVIEW *tv;

    if (!TABLE_Exists(db, table->name))
    {
        r = add_table_to_db(db, table->columns, table->types, table->labels, table->numlabels, table->numcolumns);
        if (r != ERROR_SUCCESS)
           return ERROR_FUNCTION_FAILED;
    }

    LIST_FOR_EACH_ENTRY(row, &table->rows, struct merge_row, entry)
    {
        r = TABLE_CreateView(db, table->name, &tv);
        if (r != ERROR_SUCCESS)
            return r;

        r = tv->ops->insert_row(tv, row->data, -1, FALSE);
        tv->ops->delete(tv);

        if (r != ERROR_SUCCESS)
            return r;
    }

    return ERROR_SUCCESS;
}

static UINT update_merge_errors(MSIDATABASE *db, LPCWSTR error,
                                LPWSTR table, DWORD numconflicts)
{
    UINT r;
    MSIQUERY *view;

    if (!TABLE_Exists(db, error))
    {
        r = MSI_OpenQuery(db, &view, L"CREATE TABLE `%s` (`Table` CHAR(255) NOT NULL, `NumRowMergeConflicts` SHORT "
                                      "NOT NULL PRIMARY KEY `Table`)" , error);
        if (r != ERROR_SUCCESS)
            return r;

        r = MSI_ViewExecute(view, NULL);
        msiobj_release(&view->hdr);
        if (r != ERROR_SUCCESS)
            return r;
    }

    r = MSI_OpenQuery(db, &view, L"INSERT INTO `%s` (`Table`, `NumRowMergeConflicts`) VALUES ('%s', %d)", error,
                      table, numconflicts);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute(view, NULL);
    msiobj_release(&view->hdr);
    return r;
}

UINT WINAPI MsiDatabaseMergeW( MSIHANDLE hDatabase, MSIHANDLE hDatabaseMerge, const WCHAR *szTableName )
{
    struct list tabledata = LIST_INIT(tabledata);
    struct list *item, *cursor;
    MSIDATABASE *db, *merge;
    struct merge_table *table;
    BOOL conflicts;
    UINT r;

    TRACE( "%lu, %lu, %s\n", hDatabase, hDatabaseMerge, debugstr_w(szTableName) );

    if (szTableName && !*szTableName)
        return ERROR_INVALID_TABLE;

    db = msihandle2msiinfo(hDatabase, MSIHANDLETYPE_DATABASE);
    merge = msihandle2msiinfo(hDatabaseMerge, MSIHANDLETYPE_DATABASE);
    if (!db || !merge)
    {
        r = ERROR_INVALID_HANDLE;
        goto done;
    }

    r = gather_merge_data(db, merge, &tabledata);
    if (r != ERROR_SUCCESS)
        goto done;

    conflicts = FALSE;
    LIST_FOR_EACH_ENTRY(table, &tabledata, struct merge_table, entry)
    {
        if (table->numconflicts)
        {
            conflicts = TRUE;

            r = update_merge_errors(db, szTableName, table->name,
                                    table->numconflicts);
            if (r != ERROR_SUCCESS)
                break;
        }
        else
        {
            r = merge_table(db, table);
            if (r != ERROR_SUCCESS)
                break;
        }
    }

    LIST_FOR_EACH_SAFE(item, cursor, &tabledata)
    {
        struct merge_table *table = LIST_ENTRY(item, struct merge_table, entry);
        list_remove(&table->entry);
        free_merge_table(table);
    }

    if (conflicts)
        r = ERROR_FUNCTION_FAILED;

done:
    msiobj_release(&db->hdr);
    msiobj_release(&merge->hdr);
    return r;
}

MSIDBSTATE WINAPI MsiGetDatabaseState( MSIHANDLE handle )
{
    MSIDBSTATE ret = MSIDBSTATE_READ;
    MSIDATABASE *db;

    TRACE( "%lu\n", handle );

    if (!(db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE )))
        return MSIDBSTATE_ERROR;

    if (db->mode != MSI_OPEN_READONLY )
        ret = MSIDBSTATE_WRITE;
    msiobj_release( &db->hdr );

    return ret;
}

MSICONDITION __cdecl s_remote_DatabaseIsTablePersistent(MSIHANDLE db, LPCWSTR table)
{
    return MsiDatabaseIsTablePersistentW(db, table);
}

UINT __cdecl s_remote_DatabaseGetPrimaryKeys(MSIHANDLE db, LPCWSTR table, struct wire_record **rec)
{
    MSIHANDLE handle;
    UINT r = MsiDatabaseGetPrimaryKeysW(db, table, &handle);
    *rec = NULL;
    if (!r)
        *rec = marshal_record(handle);
    MsiCloseHandle(handle);
    return r;
}

UINT __cdecl s_remote_DatabaseGetSummaryInformation(MSIHANDLE db, UINT updatecount, MSIHANDLE *suminfo)
{
    return MsiGetSummaryInformationW(db, NULL, updatecount, suminfo);
}

UINT __cdecl s_remote_DatabaseOpenView(MSIHANDLE db, LPCWSTR query, MSIHANDLE *view)
{
    return MsiDatabaseOpenViewW(db, query, view);
}
