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

#include "msipriv.h"

#include <stdio.h>

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

typedef struct tagMSITRANSFORM {
    struct list entry;
    IStorage *stg;
} MSITRANSFORM;

typedef struct tagMSISTREAM {
    struct list entry;
    IStorage *stg;
    IStream *stm;
} MSISTREAM;

static UINT find_open_stream( MSIDATABASE *db, IStorage *stg, LPCWSTR name, IStream **stm )
{
    MSISTREAM *stream;

    LIST_FOR_EACH_ENTRY( stream, &db->streams, MSISTREAM, entry )
    {
        HRESULT r;
        STATSTG stat;

        if (stream->stg != stg) continue;

        r = IStream_Stat( stream->stm, &stat, 0 );
        if( FAILED( r ) )
        {
            WARN("failed to stat stream r = %08x!\n", r);
            continue;
        }

        if( !strcmpW( name, stat.pwcsName ) )
        {
            TRACE("found %s\n", debugstr_w(name));
            *stm = stream->stm;
            CoTaskMemFree( stat.pwcsName );
            return ERROR_SUCCESS;
        }

        CoTaskMemFree( stat.pwcsName );
    }

    return ERROR_FUNCTION_FAILED;
}

UINT msi_clone_open_stream( MSIDATABASE *db, IStorage *stg, LPCWSTR name, IStream **stm )
{
    IStream *stream;

    if (find_open_stream( db, stg, name, &stream ) == ERROR_SUCCESS)
    {
        HRESULT r;
        LARGE_INTEGER pos;

        r = IStream_Clone( stream, stm );
        if( FAILED( r ) )
        {
            WARN("failed to clone stream r = %08x!\n", r);
            return ERROR_FUNCTION_FAILED;
        }

        pos.QuadPart = 0;
        r = IStream_Seek( *stm, pos, STREAM_SEEK_SET, NULL );
        if( FAILED( r ) )
        {
            IStream_Release( *stm );
            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    return ERROR_FUNCTION_FAILED;
}

UINT msi_get_raw_stream( MSIDATABASE *db, LPCWSTR stname, IStream **stm )
{
    HRESULT r;
    IStorage *stg;
    WCHAR decoded[MAX_STREAM_NAME_LEN + 1];

    decode_streamname( stname, decoded );
    TRACE("%s -> %s\n", debugstr_w(stname), debugstr_w(decoded));

    if (msi_clone_open_stream( db, db->storage, stname, stm ) == ERROR_SUCCESS)
        return ERROR_SUCCESS;

    r = IStorage_OpenStream( db->storage, stname, NULL,
                             STGM_READ | STGM_SHARE_EXCLUSIVE, 0, stm );
    if( FAILED( r ) )
    {
        MSITRANSFORM *transform;

        LIST_FOR_EACH_ENTRY( transform, &db->transforms, MSITRANSFORM, entry )
        {
            r = IStorage_OpenStream( transform->stg, stname, NULL,
                                     STGM_READ | STGM_SHARE_EXCLUSIVE, 0, stm );
            if (SUCCEEDED(r))
            {
                stg = transform->stg;
                break;
            }
        }
    }
    else stg = db->storage;

    if( SUCCEEDED(r) )
    {
        MSISTREAM *stream;

        if (!(stream = msi_alloc( sizeof(MSISTREAM) ))) return ERROR_NOT_ENOUGH_MEMORY;
        stream->stg = stg;
        IStorage_AddRef( stg );
        stream->stm = *stm;
        IStream_AddRef( *stm );
        list_add_tail( &db->streams, &stream->entry );
    }

    return SUCCEEDED(r) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

static void free_transforms( MSIDATABASE *db )
{
    while( !list_empty( &db->transforms ) )
    {
        MSITRANSFORM *t = LIST_ENTRY( list_head( &db->transforms ),
                                      MSITRANSFORM, entry );
        list_remove( &t->entry );
        IStorage_Release( t->stg );
        msi_free( t );
    }
}

void msi_destroy_stream( MSIDATABASE *db, const WCHAR *stname )
{
    MSISTREAM *stream, *stream2;

    LIST_FOR_EACH_ENTRY_SAFE( stream, stream2, &db->streams, MSISTREAM, entry )
    {
        HRESULT r;
        STATSTG stat;

        r = IStream_Stat( stream->stm, &stat, 0 );
        if (FAILED(r))
        {
            WARN("failed to stat stream r = %08x\n", r);
            continue;
        }

        if (!strcmpW( stname, stat.pwcsName ))
        {
            TRACE("destroying %s\n", debugstr_w(stname));

            list_remove( &stream->entry );
            IStream_Release( stream->stm );
            IStorage_Release( stream->stg );
            IStorage_DestroyElement( stream->stg, stname );
            msi_free( stream );
            CoTaskMemFree( stat.pwcsName );
            break;
        }
        CoTaskMemFree( stat.pwcsName );
    }
}

static void free_streams( MSIDATABASE *db )
{
    while( !list_empty( &db->streams ) )
    {
        MSISTREAM *s = LIST_ENTRY(list_head( &db->streams ), MSISTREAM, entry);
        list_remove( &s->entry );
        IStream_Release( s->stm );
        IStorage_Release( s->stg );
        msi_free( s );
    }
}

void append_storage_to_db( MSIDATABASE *db, IStorage *stg )
{
    MSITRANSFORM *t;

    t = msi_alloc( sizeof *t );
    t->stg = stg;
    IStorage_AddRef( stg );
    list_add_head( &db->transforms, &t->entry );

    /* the transform may add or replace streams */
    free_streams( db );
}

static VOID MSI_CloseDatabase( MSIOBJECTHDR *arg )
{
    MSIDATABASE *db = (MSIDATABASE *) arg;

    msi_free(db->path);
    free_cached_tables( db );
    free_streams( db );
    free_transforms( db );
    if (db->strings) msi_destroy_stringtable( db->strings );
    IStorage_Release( db->storage );
    if (db->deletefile)
    {
        DeleteFileW( db->deletefile );
        msi_free( db->deletefile );
    }
}

static HRESULT db_initialize( IStorage *stg, const GUID *clsid )
{
    static const WCHAR szTables[]  = { '_','T','a','b','l','e','s',0 };
    HRESULT hr;

    hr = IStorage_SetClass( stg, clsid );
    if (FAILED( hr ))
    {
        WARN("failed to set class id 0x%08x\n", hr);
        return hr;
    }

    /* create the _Tables stream */
    hr = write_stream_data( stg, szTables, NULL, 0, TRUE );
    if (FAILED( hr ))
    {
        WARN("failed to create _Tables stream 0x%08x\n", hr);
        return hr;
    }

    hr = msi_init_string_table( stg );
    if (FAILED( hr ))
    {
        WARN("failed to initialize string table 0x%08x\n", hr);
        return hr;
    }

    hr = IStorage_Commit( stg, 0 );
    if (FAILED( hr ))
    {
        WARN("failed to commit changes 0x%08x\n", hr);
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
    LPCWSTR szMode, save_path;
    STATSTG stat;
    BOOL created = FALSE, patch = FALSE;
    WCHAR path[MAX_PATH];

    TRACE("%s %s\n",debugstr_w(szDBPath),debugstr_w(szPersist) );

    if( !pdb )
        return ERROR_INVALID_PARAMETER;

    if (szPersist - MSIDBOPEN_PATCHFILE >= MSIDBOPEN_READONLY &&
        szPersist - MSIDBOPEN_PATCHFILE <= MSIDBOPEN_CREATEDIRECT)
    {
        TRACE("Database is a patch\n");
        szPersist -= MSIDBOPEN_PATCHFILE;
        patch = TRUE;
    }

    save_path = szDBPath;
    szMode = szPersist;
    if( !IS_INTMSIDBOPEN(szPersist) )
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
    else if( szPersist == MSIDBOPEN_CREATE )
    {
        r = StgCreateDocfile( szDBPath,
              STGM_CREATE|STGM_TRANSACTED|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &stg );

        if( SUCCEEDED(r) )
            r = db_initialize( stg, patch ? &CLSID_MsiPatch : &CLSID_MsiDatabase );
        created = TRUE;
    }
    else if( szPersist == MSIDBOPEN_CREATEDIRECT )
    {
        r = StgCreateDocfile( szDBPath,
              STGM_CREATE|STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &stg );

        if( SUCCEEDED(r) )
            r = db_initialize( stg, patch ? &CLSID_MsiPatch : &CLSID_MsiDatabase );
        created = TRUE;
    }
    else if( szPersist == MSIDBOPEN_TRANSACT )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_TRANSACTED|STGM_READWRITE|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
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

    if( FAILED( r ) || !stg )
    {
        WARN("open failed r = %08x for %s\n", r, debugstr_w(szDBPath));
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

    if (!strchrW( save_path, '\\' ))
    {
        GetCurrentDirectoryW( MAX_PATH, path );
        lstrcatW( path, szBackSlash );
        lstrcatW( path, save_path );
    }
    else
        lstrcpyW( path, save_path );

    db->path = strdupW( path );
    db->media_transform_offset = MSI_INITIAL_MEDIA_TRANSFORM_OFFSET;
    db->media_transform_disk_id = MSI_INITIAL_MEDIA_TRANSFORM_DISKID;

    if( TRACE_ON( msi ) )
        enum_stream_names( stg );

    db->storage = stg;
    db->mode = szMode;
    if (created)
        db->deletefile = strdupW( szDBPath );
    list_init( &db->tables );
    list_init( &db->transforms );
    list_init( &db->streams );

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
        msi_free( szwPersist );
    msi_free( szwDBPath );

    return r;
}

static LPWSTR msi_read_text_archive(LPCWSTR path, DWORD *len)
{
    HANDLE file;
    LPSTR data = NULL;
    LPWSTR wdata = NULL;
    DWORD read, size = 0;

    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (file == INVALID_HANDLE_VALUE)
        return NULL;

    size = GetFileSize( file, NULL );
    if (!(data = msi_alloc( size ))) goto done;

    if (!ReadFile( file, data, size, &read, NULL ) || read != size) goto done;

    while (!data[size - 1]) size--;
    *len = MultiByteToWideChar( CP_ACP, 0, data, size, NULL, 0 );
    if ((wdata = msi_alloc( (*len + 1) * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, data, size, wdata, *len );
        wdata[*len] = 0;
    }

done:
    CloseHandle( file );
    msi_free( data );
    return wdata;
}

static void msi_parse_line(LPWSTR *line, LPWSTR **entries, DWORD *num_entries, DWORD *len)
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

    *entries = msi_alloc(count * sizeof(LPWSTR));
    if (!*entries)
        return;

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
    if (num_entries)
        *num_entries = count;
}

static LPWSTR msi_build_createsql_prelude(LPWSTR table)
{
    LPWSTR prelude;
    DWORD size;

    static const WCHAR create_fmt[] = {'C','R','E','A','T','E',' ','T','A','B','L','E',' ','`','%','s','`',' ','(',' ',0};

    size = sizeof(create_fmt)/sizeof(create_fmt[0]) + lstrlenW(table) - 2;
    prelude = msi_alloc(size * sizeof(WCHAR));
    if (!prelude)
        return NULL;

    sprintfW(prelude, create_fmt, table);
    return prelude;
}

static LPWSTR msi_build_createsql_columns(LPWSTR *columns_data, LPWSTR *types, DWORD num_columns)
{
    LPWSTR columns, p;
    LPCWSTR type;
    DWORD sql_size = 1, i, len;
    WCHAR expanded[128], *ptr;
    WCHAR size[10], comma[2], extra[30];

    static const WCHAR column_fmt[] = {'`','%','s','`',' ','%','s','%','s','%','s','%','s',' ',0};
    static const WCHAR size_fmt[] = {'(','%','s',')',0};
    static const WCHAR type_char[] = {'C','H','A','R',0};
    static const WCHAR type_int[] = {'I','N','T',0};
    static const WCHAR type_long[] = {'L','O','N','G',0};
    static const WCHAR type_object[] = {'O','B','J','E','C','T',0};
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
                /* fall through */
            case 'L':
                lstrcatW(extra, localizable);
                type = type_char;
                sprintfW(size, size_fmt, ptr);
                break;
            case 's':
                lstrcpyW(extra, type_notnull);
                /* fall through */
            case 'S':
                type = type_char;
                sprintfW(size, size_fmt, ptr);
                break;
            case 'i':
                lstrcpyW(extra, type_notnull);
                /* fall through */
            case 'I':
                if (len <= 2)
                    type = type_int;
                else if (len == 4)
                    type = type_long;
                else
                {
                    WARN("invalid int width %u\n", len);
                    msi_free(columns);
                    return NULL;
                }
                break;
            case 'v':
                lstrcpyW(extra, type_notnull);
                /* fall through */
            case 'V':
                type = type_object;
                break;
            default:
                ERR("Unknown type: %c\n", types[i][0]);
                msi_free(columns);
                return NULL;
        }

        sprintfW(expanded, column_fmt, columns_data[i], type, size, extra, comma);
        sql_size += lstrlenW(expanded);

        p = msi_realloc(columns, sql_size * sizeof(WCHAR));
        if (!p)
        {
            msi_free(columns);
            return NULL;
        }
        columns = p;

        lstrcatW(columns, expanded);
    }

    return columns;
}

static LPWSTR msi_build_createsql_postlude(LPWSTR *primary_keys, DWORD num_keys)
{
    LPWSTR postlude, keys, ptr;
    DWORD size, key_size, i;

    static const WCHAR key_fmt[] = {'`','%','s','`',',',' ',0};
    static const WCHAR postlude_fmt[] = {'P','R','I','M','A','R','Y',' ','K','E','Y',' ','%','s',')',0};

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
    UINT r = ERROR_OUTOFMEMORY;
    DWORD size;
    MSIQUERY *view;
    LPWSTR create_sql = NULL;
    LPWSTR prelude, columns_sql, postlude;

    prelude = msi_build_createsql_prelude(labels[0]);
    columns_sql = msi_build_createsql_columns(columns, types, num_columns);
    postlude = msi_build_createsql_postlude(labels + 1, num_labels - 1); /* skip over table name */

    if (!prelude || !columns_sql || !postlude)
        goto done;

    size = lstrlenW(prelude) + lstrlenW(columns_sql) + lstrlenW(postlude) + 1;
    create_sql = msi_alloc(size * sizeof(WCHAR));
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
    msi_free(prelude);
    msi_free(columns_sql);
    msi_free(postlude);
    msi_free(create_sql);
    return r;
}

static LPWSTR msi_import_stream_filename(LPCWSTR path, LPCWSTR name)
{
    DWORD len;
    LPWSTR fullname, ptr;

    len = lstrlenW(path) + lstrlenW(name) + 1;
    fullname = msi_alloc(len*sizeof(WCHAR));
    if (!fullname)
       return NULL;

    lstrcpyW( fullname, path );

    /* chop off extension from path */
    ptr = strrchrW(fullname, '.');
    if (!ptr)
    {
        msi_free (fullname);
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
                    MSI_RecordSetInteger(*rec, i + 1, atoiW(data[i]));
                break;
            case 'V': case 'v':
                if (*data[i])
                {
                    UINT r;
                    LPWSTR file = msi_import_stream_filename(path, data[i]);
                    if (!file)
                        return ERROR_FUNCTION_FAILED;

                    r = MSI_RecordSetStreamFromFileW(*rec, i + 1, file);
                    msi_free (file);
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

static UINT msi_add_records_to_table(MSIDATABASE *db, LPWSTR *columns, LPWSTR *types,
                                     LPWSTR *labels, LPWSTR **records,
                                     int num_columns, int num_records,
                                     LPWSTR path)
{
    UINT r;
    int i;
    MSIQUERY *view;
    MSIRECORD *rec;

    static const WCHAR select[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','%','s','`',0
    };

    r = MSI_OpenQuery(db, &view, select, labels[0]);
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
    DWORD len, i;
    DWORD num_labels, num_types;
    DWORD num_columns, num_records = 0;
    LPWSTR *columns, *types, *labels;
    LPWSTR path, ptr, data;
    LPWSTR **records = NULL;
    LPWSTR **temp_records;

    static const WCHAR suminfo[] =
        {'_','S','u','m','m','a','r','y','I','n','f','o','r','m','a','t','i','o','n',0};
    static const WCHAR forcecodepage[] =
        {'_','F','o','r','c','e','C','o','d','e','p','a','g','e',0};

    TRACE("%p %s %s\n", db, debugstr_w(folder), debugstr_w(file) );

    if( folder == NULL || file == NULL )
        return ERROR_INVALID_PARAMETER;

    len = lstrlenW(folder) + lstrlenW(szBackSlash) + lstrlenW(file) + 1;
    path = msi_alloc( len * sizeof(WCHAR) );
    if (!path)
        return ERROR_OUTOFMEMORY;

    lstrcpyW( path, folder );
    lstrcatW( path, szBackSlash );
    lstrcatW( path, file );

    data = msi_read_text_archive( path, &len );

    ptr = data;
    msi_parse_line( &ptr, &columns, &num_columns, &len );
    msi_parse_line( &ptr, &types, &num_types, &len );
    msi_parse_line( &ptr, &labels, &num_labels, &len );

    if (num_columns == 1 && !columns[0][0] && num_labels == 1 && !labels[0][0] &&
        num_types == 2 && !strcmpW( types[1], forcecodepage ))
    {
        r = msi_set_string_table_codepage( db->strings, atoiW( types[0] ) );
        goto done;
    }

    if (num_columns != num_types)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    records = msi_alloc(sizeof(LPWSTR *));
    if (!records)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    /* read in the table records */
    while (len)
    {
        msi_parse_line( &ptr, &records[num_records], NULL, &len );

        num_records++;
        temp_records = msi_realloc(records, (num_records + 1) * sizeof(LPWSTR *));
        if (!temp_records)
        {
            r = ERROR_OUTOFMEMORY;
            goto done;
        }
        records = temp_records;
    }

    if (!strcmpW(labels[0], suminfo))
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
            r = msi_add_table_to_db( db, columns, types, labels, num_labels, num_columns );
            if (r != ERROR_SUCCESS)
            {
                r = ERROR_FUNCTION_FAILED;
                goto done;
            }
        }

        r = msi_add_records_to_table( db, columns, types, labels, records, num_columns, num_records, path );
    }

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

    TRACE("%x %s %s\n",handle,debugstr_w(szFolder), debugstr_w(szFilename));

    db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( handle );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        IWineMsiRemoteDatabase_Release( remote_database );
        WARN("MsiDatabaseImport not allowed during a custom action!\n");

        return ERROR_SUCCESS;
    }

    r = MSI_DatabaseImport( db, szFolder, szFilename );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseImportA( MSIHANDLE handle,
               LPCSTR szFolder, LPCSTR szFilename )
{
    LPWSTR path = NULL, file = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%x %s %s\n", handle, debugstr_a(szFolder), debugstr_a(szFilename));

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

static UINT msi_export_forcecodepage( HANDLE handle, UINT codepage )
{
    static const char fmt[] = "\r\n\r\n%u\t_ForceCodepage\r\n";
    char data[sizeof(fmt) + 10];
    DWORD sz;

    sprintf( data, fmt, codepage );

    sz = lstrlenA(data) + 1;
    if (!WriteFile(handle, data, sz, &sz, NULL))
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

static UINT MSI_DatabaseExport( MSIDATABASE *db, LPCWSTR table,
               LPCWSTR folder, LPCWSTR file )
{
    static const WCHAR query[] = {
        's','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','%','s',0 };
    static const WCHAR forcecodepage[] = {
        '_','F','o','r','c','e','C','o','d','e','p','a','g','e',0 };
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
    lstrcatW( filename, szBackSlash );
    lstrcatW( filename, file );

    handle = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    msi_free( filename );
    if (handle == INVALID_HANDLE_VALUE)
        return ERROR_FUNCTION_FAILED;

    if (!strcmpW( table, forcecodepage ))
    {
        UINT codepage = msi_get_string_table_codepage( db->strings );
        r = msi_export_forcecodepage( handle, codepage );
        goto done;
    }

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
UINT WINAPI MsiDatabaseExportW( MSIHANDLE handle, LPCWSTR szTable,
               LPCWSTR szFolder, LPCWSTR szFilename )
{
    MSIDATABASE *db;
    UINT r;

    TRACE("%x %s %s %s\n", handle, debugstr_w(szTable),
          debugstr_w(szFolder), debugstr_w(szFilename));

    db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( handle );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        IWineMsiRemoteDatabase_Release( remote_database );
        WARN("MsiDatabaseExport not allowed during a custom action!\n");

        return ERROR_SUCCESS;
    }

    r = MSI_DatabaseExport( db, szTable, szFolder, szFilename );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseExportA( MSIHANDLE handle, LPCSTR szTable,
               LPCSTR szFolder, LPCSTR szFilename )
{
    LPWSTR path = NULL, file = NULL, table = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%x %s %s %s\n", handle, debugstr_a(szTable),
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

UINT WINAPI MsiDatabaseMergeA(MSIHANDLE hDatabase, MSIHANDLE hDatabaseMerge,
                              LPCSTR szTableName)
{
    UINT r;
    LPWSTR table;

    TRACE("(%d, %d, %s)\n", hDatabase, hDatabaseMerge,
          debugstr_a(szTableName));

    table = strdupAtoW(szTableName);
    r = MsiDatabaseMergeW(hDatabase, hDatabaseMerge, table);

    msi_free(table);
    return r;
}

typedef struct _tagMERGETABLE
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
} MERGETABLE;

typedef struct _tagMERGEROW
{
    struct list entry;
    MSIRECORD *data;
} MERGEROW;

typedef struct _tagMERGEDATA
{
    MSIDATABASE *db;
    MSIDATABASE *merge;
    MERGETABLE *curtable;
    MSIQUERY *curview;
    struct list *tabledata;
} MERGEDATA;

static BOOL merge_type_match(LPCWSTR type1, LPCWSTR type2)
{
    if (((type1[0] == 'l') || (type1[0] == 's')) &&
        ((type2[0] == 'l') || (type2[0] == 's')))
        return TRUE;

    if (((type1[0] == 'L') || (type1[0] == 'S')) &&
        ((type2[0] == 'L') || (type2[0] == 'S')))
        return TRUE;

    return !strcmpW( type1, type2 );
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

        if (strcmpW( MSI_RecordGetString( dbrec, i ), MSI_RecordGetString( mergerec, i ) ))
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
        if (strcmpW( MSI_RecordGetString( dbrec, i ), MSI_RecordGetString( mergerec, i ) ))
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
    UINT r, i = 0, sz = 0;
    int cmp;

    r = MSI_ViewGetColumnInfo(view, MSICOLINFO_NAMES, &colnames);
    if (r != ERROR_SUCCESS)
        return NULL;

    do
    {
        str = msi_dup_record_field(colnames, ++i);
        cmp = strcmpW( key, str );
        msi_free(str);
    } while (cmp);

    msiobj_release(&colnames->hdr);

    r = MSI_RecordGetStringW(rec, i, NULL, &sz);
    if (r != ERROR_SUCCESS)
        return NULL;
    sz++;

    if (MSI_RecordGetString(rec, i))  /* check record field is a string */
    {
        /* quote string record fields */
        const WCHAR szQuote[] = {'\'', 0};
        sz += 2;
        val = msi_alloc(sz*sizeof(WCHAR));
        if (!val)
            return NULL;

        lstrcpyW(val, szQuote);
        r = MSI_RecordGetStringW(rec, i, val+1, &sz);
        lstrcpyW(val+1+sz, szQuote);
    }
    else
    {
        /* do not quote integer record fields */
        val = msi_alloc(sz*sizeof(WCHAR));
        if (!val)
            return NULL;

        r = MSI_RecordGetStringW(rec, i, val, &sz);
    }

    if (r != ERROR_SUCCESS)
    {
        ERR("failed to get string!\n");
        msi_free(val);
        return NULL;
    }

    return val;
}

static LPWSTR create_diff_row_query(MSIDATABASE *merge, MSIQUERY *view,
                                    LPWSTR table, MSIRECORD *rec)
{
    LPWSTR query = NULL, clause = NULL, val;
    LPCWSTR setptr, key;
    DWORD size, oldsize;
    MSIRECORD *keys;
    UINT r, i, count;

    static const WCHAR keyset[] = {
        '`','%','s','`',' ','=',' ','%','s',' ','A','N','D',' ',0};
    static const WCHAR lastkeyset[] = {
        '`','%','s','`',' ','=',' ','%','s',' ',0};
    static const WCHAR fmt[] = {'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','%','s','`',' ',
        'W','H','E','R','E',' ','%','s',0};

    r = MSI_DatabaseGetPrimaryKeys(merge, table, &keys);
    if (r != ERROR_SUCCESS)
        return NULL;

    clause = msi_alloc_zero(sizeof(WCHAR));
    if (!clause)
        goto done;

    size = 1;
    count = MSI_RecordGetFieldCount(keys);
    for (i = 1; i <= count; i++)
    {
        key = MSI_RecordGetString(keys, i);
        val = get_key_value(view, key, rec);

        if (i == count)
            setptr = lastkeyset;
        else
            setptr = keyset;

        oldsize = size;
        size += lstrlenW(setptr) + lstrlenW(key) + lstrlenW(val) - 4;
        clause = msi_realloc(clause, size * sizeof (WCHAR));
        if (!clause)
        {
            msi_free(val);
            goto done;
        }

        sprintfW(clause + oldsize - 1, setptr, key, val);
        msi_free(val);
    }

    size = lstrlenW(fmt) + lstrlenW(table) + lstrlenW(clause) + 1;
    query = msi_alloc(size * sizeof(WCHAR));
    if (!query)
        goto done;

    sprintfW(query, fmt, table, clause);

done:
    msi_free(clause);
    msiobj_release(&keys->hdr);
    return query;
}

static UINT merge_diff_row(MSIRECORD *rec, LPVOID param)
{
    MERGEDATA *data = param;
    MERGETABLE *table = data->curtable;
    MERGEROW *mergerow;
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

    mergerow = msi_alloc(sizeof(MERGEROW));
    if (!mergerow)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    mergerow->data = MSI_CloneRecord(rec);
    if (!mergerow->data)
    {
        r = ERROR_OUTOFMEMORY;
        msi_free(mergerow);
        goto done;
    }

    list_add_tail(&table->rows, &mergerow->entry);

done:
    msi_free(query);
    msiobj_release(&row->hdr);
    msiobj_release(&dbview->hdr);
    return r;
}

static UINT msi_get_table_labels(MSIDATABASE *db, LPCWSTR table, LPWSTR **labels, DWORD *numlabels)
{
    UINT r, i, count;
    MSIRECORD *prec = NULL;

    r = MSI_DatabaseGetPrimaryKeys(db, table, &prec);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(prec);
    *numlabels = count + 1;
    *labels = msi_alloc((*numlabels)*sizeof(LPWSTR));
    if (!*labels)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    (*labels)[0] = strdupW(table);
    for (i=1; i<=count; i++ )
    {
        (*labels)[i] = strdupW(MSI_RecordGetString(prec, i));
    }

end:
    msiobj_release( &prec->hdr );
    return r;
}

static UINT msi_get_query_columns(MSIQUERY *query, LPWSTR **columns, DWORD *numcolumns)
{
    UINT r, i, count;
    MSIRECORD *prec = NULL;

    r = MSI_ViewGetColumnInfo(query, MSICOLINFO_NAMES, &prec);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(prec);
    *columns = msi_alloc(count*sizeof(LPWSTR));
    if (!*columns)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    for (i=1; i<=count; i++ )
    {
        (*columns)[i-1] = strdupW(MSI_RecordGetString(prec, i));
    }

    *numcolumns = count;

end:
    msiobj_release( &prec->hdr );
    return r;
}

static UINT msi_get_query_types(MSIQUERY *query, LPWSTR **types, DWORD *numtypes)
{
    UINT r, i, count;
    MSIRECORD *prec = NULL;

    r = MSI_ViewGetColumnInfo(query, MSICOLINFO_TYPES, &prec);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(prec);
    *types = msi_alloc(count*sizeof(LPWSTR));
    if (!*types)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    *numtypes = count;
    for (i=1; i<=count; i++ )
    {
        (*types)[i-1] = strdupW(MSI_RecordGetString(prec, i));
    }

end:
    msiobj_release( &prec->hdr );
    return r;
}

static void merge_free_rows(MERGETABLE *table)
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE(item, cursor, &table->rows)
    {
        MERGEROW *row = LIST_ENTRY(item, MERGEROW, entry);

        list_remove(&row->entry);
        msiobj_release(&row->data->hdr);
        msi_free(row);
    }
}

static void free_merge_table(MERGETABLE *table)
{
    UINT i;

    if (table->labels != NULL)
    {
        for (i = 0; i < table->numlabels; i++)
            msi_free(table->labels[i]);

        msi_free(table->labels);
    }

    if (table->columns != NULL)
    {
        for (i = 0; i < table->numcolumns; i++)
            msi_free(table->columns[i]);

        msi_free(table->columns);
    }

    if (table->types != NULL)
    {
        for (i = 0; i < table->numtypes; i++)
            msi_free(table->types[i]);

        msi_free(table->types);
    }

    msi_free(table->name);
    merge_free_rows(table);

    msi_free(table);
}

static UINT msi_get_merge_table (MSIDATABASE *db, LPCWSTR name, MERGETABLE **ptable)
{
    UINT r;
    MERGETABLE *table;
    MSIQUERY *mergeview = NULL;

    static const WCHAR query[] = {'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','%','s','`',0};

    table = msi_alloc_zero(sizeof(MERGETABLE));
    if (!table)
    {
       *ptable = NULL;
       return ERROR_OUTOFMEMORY;
    }

    r = msi_get_table_labels(db, name, &table->labels, &table->numlabels);
    if (r != ERROR_SUCCESS)
        goto err;

    r = MSI_OpenQuery(db, &mergeview, query, name);
    if (r != ERROR_SUCCESS)
        goto err;

    r = msi_get_query_columns(mergeview, &table->columns, &table->numcolumns);
    if (r != ERROR_SUCCESS)
        goto err;

    r = msi_get_query_types(mergeview, &table->types, &table->numtypes);
    if (r != ERROR_SUCCESS)
        goto err;

    list_init(&table->rows);

    table->name = strdupW(name);
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
    MERGEDATA *data = param;
    MERGETABLE *table;
    MSIQUERY *dbview = NULL;
    MSIQUERY *mergeview = NULL;
    LPCWSTR name;
    UINT r;

    static const WCHAR query[] = {'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','%','s','`',0};

    name = MSI_RecordGetString(rec, 1);

    r = MSI_OpenQuery(data->merge, &mergeview, query, name);
    if (r != ERROR_SUCCESS)
        goto done;

    if (TABLE_Exists(data->db, name))
    {
        r = MSI_OpenQuery(data->db, &dbview, query, name);
        if (r != ERROR_SUCCESS)
            goto done;

        r = merge_verify_colnames(dbview, mergeview);
        if (r != ERROR_SUCCESS)
            goto done;

        r = merge_verify_primary_keys(data->db, data->merge, name);
        if (r != ERROR_SUCCESS)
            goto done;
    }

    r = msi_get_merge_table(data->merge, name, &table);
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
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','_','T','a','b','l','e','s','`',0};
    MSIQUERY *view;
    MERGEDATA data;
    UINT r;

    r = MSI_DatabaseOpenViewW(merge, query, &view);
    if (r != ERROR_SUCCESS)
        return r;

    data.db = db;
    data.merge = merge;
    data.tabledata = tabledata;
    r = MSI_IterateRecords(view, NULL, merge_diff_tables, &data);
    msiobj_release(&view->hdr);
    return r;
}

static UINT merge_table(MSIDATABASE *db, MERGETABLE *table)
{
    UINT r;
    MERGEROW *row;
    MSIVIEW *tv;

    if (!TABLE_Exists(db, table->name))
    {
        r = msi_add_table_to_db(db, table->columns, table->types,
               table->labels, table->numlabels, table->numcolumns);
        if (r != ERROR_SUCCESS)
           return ERROR_FUNCTION_FAILED;
    }

    LIST_FOR_EACH_ENTRY(row, &table->rows, MERGEROW, entry)
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

    static const WCHAR create[] = {
        'C','R','E','A','T','E',' ','T','A','B','L','E',' ',
        '`','%','s','`',' ','(','`','T','a','b','l','e','`',' ',
        'C','H','A','R','(','2','5','5',')',' ','N','O','T',' ',
        'N','U','L','L',',',' ','`','N','u','m','R','o','w','M','e','r','g','e',
        'C','o','n','f','l','i','c','t','s','`',' ','S','H','O','R','T',' ',
        'N','O','T',' ','N','U','L','L',' ','P','R','I','M','A','R','Y',' ',
        'K','E','Y',' ','`','T','a','b','l','e','`',')',0};
    static const WCHAR insert[] = {
        'I','N','S','E','R','T',' ','I','N','T','O',' ',
        '`','%','s','`',' ','(','`','T','a','b','l','e','`',',',' ',
        '`','N','u','m','R','o','w','M','e','r','g','e',
        'C','o','n','f','l','i','c','t','s','`',')',' ','V','A','L','U','E','S',
        ' ','(','\'','%','s','\'',',',' ','%','d',')',0};

    if (!TABLE_Exists(db, error))
    {
        r = MSI_OpenQuery(db, &view, create, error);
        if (r != ERROR_SUCCESS)
            return r;

        r = MSI_ViewExecute(view, NULL);
        msiobj_release(&view->hdr);
        if (r != ERROR_SUCCESS)
            return r;
    }

    r = MSI_OpenQuery(db, &view, insert, error, table, numconflicts);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute(view, NULL);
    msiobj_release(&view->hdr);
    return r;
}

UINT WINAPI MsiDatabaseMergeW(MSIHANDLE hDatabase, MSIHANDLE hDatabaseMerge,
                              LPCWSTR szTableName)
{
    struct list tabledata = LIST_INIT(tabledata);
    struct list *item, *cursor;
    MSIDATABASE *db, *merge;
    MERGETABLE *table;
    BOOL conflicts;
    UINT r;

    TRACE("(%d, %d, %s)\n", hDatabase, hDatabaseMerge,
          debugstr_w(szTableName));

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
    LIST_FOR_EACH_ENTRY(table, &tabledata, MERGETABLE, entry)
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
        MERGETABLE *table = LIST_ENTRY(item, MERGETABLE, entry);
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

    TRACE("%d\n", handle);

    db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( handle );
        if ( !remote_database )
            return MSIDBSTATE_ERROR;

        IWineMsiRemoteDatabase_Release( remote_database );
        WARN("MsiGetDatabaseState not allowed during a custom action!\n");

        return MSIDBSTATE_READ;
    }

    if (db->mode != MSIDBOPEN_READONLY )
        ret = MSIDBSTATE_WRITE;
    msiobj_release( &db->hdr );

    return ret;
}

typedef struct _msi_remote_database_impl {
    IWineMsiRemoteDatabase IWineMsiRemoteDatabase_iface;
    MSIHANDLE database;
    LONG refs;
} msi_remote_database_impl;

static inline msi_remote_database_impl *impl_from_IWineMsiRemoteDatabase( IWineMsiRemoteDatabase *iface )
{
    return CONTAINING_RECORD(iface, msi_remote_database_impl, IWineMsiRemoteDatabase_iface);
}

static HRESULT WINAPI mrd_QueryInterface( IWineMsiRemoteDatabase *iface,
                                          REFIID riid,LPVOID *ppobj)
{
    if( IsEqualCLSID( riid, &IID_IUnknown ) ||
        IsEqualCLSID( riid, &IID_IWineMsiRemoteDatabase ) )
    {
        IWineMsiRemoteDatabase_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mrd_AddRef( IWineMsiRemoteDatabase *iface )
{
    msi_remote_database_impl* This = impl_from_IWineMsiRemoteDatabase( iface );

    return InterlockedIncrement( &This->refs );
}

static ULONG WINAPI mrd_Release( IWineMsiRemoteDatabase *iface )
{
    msi_remote_database_impl* This = impl_from_IWineMsiRemoteDatabase( iface );
    ULONG r;

    r = InterlockedDecrement( &This->refs );
    if (r == 0)
    {
        MsiCloseHandle( This->database );
        msi_free( This );
    }
    return r;
}

static HRESULT WINAPI mrd_IsTablePersistent( IWineMsiRemoteDatabase *iface,
                                             LPCWSTR table, MSICONDITION *persistent )
{
    msi_remote_database_impl *This = impl_from_IWineMsiRemoteDatabase( iface );
    *persistent = MsiDatabaseIsTablePersistentW(This->database, table);
    return S_OK;
}

static HRESULT WINAPI mrd_GetPrimaryKeys( IWineMsiRemoteDatabase *iface,
                                          LPCWSTR table, MSIHANDLE *keys )
{
    msi_remote_database_impl *This = impl_from_IWineMsiRemoteDatabase( iface );
    UINT r = MsiDatabaseGetPrimaryKeysW(This->database, table, keys);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrd_GetSummaryInformation( IWineMsiRemoteDatabase *iface,
                                                UINT updatecount, MSIHANDLE *suminfo )
{
    msi_remote_database_impl *This = impl_from_IWineMsiRemoteDatabase( iface );
    UINT r = MsiGetSummaryInformationW(This->database, NULL, updatecount, suminfo);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrd_OpenView( IWineMsiRemoteDatabase *iface,
                                    LPCWSTR query, MSIHANDLE *view )
{
    msi_remote_database_impl *This = impl_from_IWineMsiRemoteDatabase( iface );
    UINT r = MsiDatabaseOpenViewW(This->database, query, view);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrd_SetMsiHandle( IWineMsiRemoteDatabase *iface, MSIHANDLE handle )
{
    msi_remote_database_impl* This = impl_from_IWineMsiRemoteDatabase( iface );
    This->database = handle;
    return S_OK;
}

static const IWineMsiRemoteDatabaseVtbl msi_remote_database_vtbl =
{
    mrd_QueryInterface,
    mrd_AddRef,
    mrd_Release,
    mrd_IsTablePersistent,
    mrd_GetPrimaryKeys,
    mrd_GetSummaryInformation,
    mrd_OpenView,
    mrd_SetMsiHandle,
};

HRESULT create_msi_remote_database( IUnknown *pOuter, LPVOID *ppObj )
{
    msi_remote_database_impl *This;

    This = msi_alloc( sizeof *This );
    if (!This)
        return E_OUTOFMEMORY;

    This->IWineMsiRemoteDatabase_iface.lpVtbl = &msi_remote_database_vtbl;
    This->database = 0;
    This->refs = 1;

    *ppObj = This;

    return S_OK;
}
