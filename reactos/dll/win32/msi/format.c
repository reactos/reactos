/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
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

/*
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/msiformatrecord.asp 
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msipriv.h"
#include "winnls.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


static DWORD deformat_string_internal(MSIPACKAGE *package, LPCWSTR ptr, 
                                     WCHAR** data, DWORD len, MSIRECORD* record,
                                     BOOL* in_group);


static LPWSTR build_default_format(MSIRECORD* record)
{
    int i;  
    int count;
    LPWSTR rc, buf;
    static const WCHAR fmt[] = {'%','i',':',' ','%','s',' ',0};
    static const WCHAR fmt_null[] = {'%','i',':',' ',' ',0};
    static const WCHAR fmt_index[] = {'%','i',0};
    LPCWSTR str;
    WCHAR index[10];
    DWORD size, max_len, len;

    count = MSI_RecordGetFieldCount(record);

    max_len = MAX_PATH;
    buf = msi_alloc((max_len + 1) * sizeof(WCHAR));

    rc = NULL;
    size = 1;
    for (i = 1; i <= count; i++)
    {
        sprintfW(index,fmt_index,i);
        str = MSI_RecordGetString(record, i);
        len = (str) ? lstrlenW(str) : 0;
        len += (sizeof(fmt_null) - 3) + lstrlenW(index);
        size += len;

        if (len > max_len)
        {
            max_len = len;
            buf = msi_realloc(buf, (max_len + 1) * sizeof(WCHAR));
            if (!buf) return NULL;
        }

        if (str)
            sprintfW(buf,fmt,i,str);
        else
            sprintfW(buf,fmt_null,i);

        if (!rc)
        {
            rc = msi_alloc(size * sizeof(WCHAR));
            lstrcpyW(rc, buf);
        }
        else
        {
            rc = msi_realloc(rc, size * sizeof(WCHAR));
            lstrcatW(rc, buf);
        }
    }
    msi_free(buf);
    return rc;
}

static const WCHAR* scanW(LPCWSTR buf, WCHAR token, DWORD len)
{
    DWORD i;
    for (i = 0; i < len; i++)
        if (buf[i] == token)
            return &buf[i];
    return NULL;
}

/* break out helper functions for deformating */
static LPWSTR deformat_component(MSIPACKAGE* package, LPCWSTR key, DWORD* sz)
{
    LPWSTR value = NULL;
    MSICOMPONENT *comp;

    *sz = 0;
    if (!package)
        return NULL;

    FIXME("component key %s\n", debugstr_w(key));
    comp = get_loaded_component(package,key);
    if (comp)
    {
        value = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);
        *sz = (strlenW(value)) * sizeof(WCHAR);
    }

    return value;
}

static LPWSTR deformat_file(MSIPACKAGE* package, LPCWSTR key, DWORD* sz, 
                            BOOL shortname)
{
    LPWSTR value = NULL;
    MSIFILE *file;

    *sz = 0;

    if (!package)
        return NULL;

    file = get_loaded_file( package, key );
    if (file)
    {
        if (!shortname)
        {
            value = strdupW( file->TargetPath );
            *sz = (strlenW(value)) * sizeof(WCHAR);
        }
        else
        {
            DWORD size = 0;
            size = GetShortPathNameW( file->TargetPath, NULL, 0 );

            if (size > 0)
            {
                *sz = (size-1) * sizeof (WCHAR);
                size ++;
                value = msi_alloc(size * sizeof(WCHAR));
                GetShortPathNameW( file->TargetPath, value, size );
            }
            else
            {
                ERR("Unable to get ShortPath size (%s)\n",
                    debugstr_w( file->TargetPath) );
                value = strdupW( file->TargetPath );
                *sz = (lstrlenW(value)) * sizeof(WCHAR);
            }
        }
    }

    return value;
}

static LPWSTR deformat_environment(MSIPACKAGE* package, LPCWSTR key, 
                                   DWORD* chunk)
{
    LPWSTR value = NULL;
    DWORD sz;

    sz  = GetEnvironmentVariableW(key,NULL,0);
    if (sz > 0)
    {
        sz++;
        value = msi_alloc(sz * sizeof(WCHAR));
        GetEnvironmentVariableW(key,value,sz);
        *chunk = (strlenW(value)) * sizeof(WCHAR);
    }
    else
    {
        ERR("Unknown environment variable %s\n", debugstr_w(key));
        *chunk = 0;
        value = NULL;
    }
    return value;
}

 
static LPWSTR deformat_NULL(DWORD* chunk)
{
    LPWSTR value;

    value = msi_alloc(sizeof(WCHAR)*2);
    value[0] =  0;
    *chunk = sizeof(WCHAR);
    return value;
}

static LPWSTR deformat_escape(LPCWSTR key, DWORD* chunk)
{
    LPWSTR value;

    value = msi_alloc(sizeof(WCHAR)*2);
    value[0] =  key[0];
    *chunk = sizeof(WCHAR);

    return value;
}


static BOOL is_key_number(LPCWSTR key)
{
    INT index = 0;
    if (key[0] == 0)
        return FALSE;

    while (isdigitW(key[index])) index++;
    if (key[index] == 0)
        return TRUE;
    else
        return FALSE;
}

static LPWSTR deformat_index(MSIRECORD* record, LPCWSTR key, DWORD* chunk )
{
    INT index;
    LPWSTR value; 

    index = atoiW(key);
    TRACE("record index %i\n",index);
    value = msi_dup_record_field(record,index);
    if (value)
        *chunk = strlenW(value) * sizeof(WCHAR);
    else
    {
        value = NULL;
        *chunk = 0;
    }
    return value;
}

static LPWSTR deformat_property(MSIPACKAGE* package, LPCWSTR key, DWORD* chunk)
{
    LPWSTR value;

    if (!package)
        return NULL;

    value = msi_dup_property( package, key );

    if (value)
        *chunk = (strlenW(value)) * sizeof(WCHAR);

    return value;
}

/*
 * Groups cannot be nested. They are just treated as from { to next } 
 */
static BOOL find_next_group(LPCWSTR source, DWORD len_remaining,
                                    LPWSTR *group, LPCWSTR *mark, 
                                    LPCWSTR* mark2)
{
    int i;
    BOOL found = FALSE;

    *mark = scanW(source,'{',len_remaining);
    if (!*mark)
        return FALSE;

    for (i = 1; (*mark - source) + i < len_remaining; i++)
    {
        if ((*mark)[i] == '}')
        {
            found = TRUE;
            break;
        }
    }
    if (! found)
        return FALSE;

    *mark2 = &(*mark)[i]; 

    i = *mark2 - *mark;
    *group = msi_alloc(i*sizeof(WCHAR));

    i -= 1;
    memcpy(*group,&(*mark)[1],i*sizeof(WCHAR));
    (*group)[i] = 0;

    TRACE("Found group %s\n",debugstr_w(*group));
    return TRUE;
}


static BOOL find_next_outermost_key(LPCWSTR source, DWORD len_remaining,
                                    LPWSTR *key, LPCWSTR *mark, LPCWSTR* mark2, 
                                    BOOL *nested)
{
    INT count = 0;
    INT total_count = 0;
    int i;

    *mark = scanW(source,'[',len_remaining);
    if (!*mark)
        return FALSE;

    count = 1;
    total_count = 1;
    *nested = FALSE;
    for (i = 1; (*mark - source) + i < len_remaining && count > 0; i++)
    {
        if ((*mark)[i] == '[' && (*mark)[i-1] != '\\')
        {
            count ++;
            total_count ++;
            *nested = TRUE;
        }
        else if ((*mark)[i] == ']' && (*mark)[i-1] != '\\')
        {
            count --;
        }
    }

    if (count > 0)
        return FALSE;

    *mark2 = &(*mark)[i-1]; 

    i = *mark2 - *mark;
    *key = msi_alloc(i*sizeof(WCHAR));
    /* do not have the [] in the key */
    i -= 1;
    memcpy(*key,&(*mark)[1],i*sizeof(WCHAR));
    (*key)[i] = 0;

    TRACE("Found Key %s\n",debugstr_w(*key));
    return TRUE;
}

static LPWSTR deformat_group(MSIPACKAGE* package, LPWSTR group, DWORD len, 
                      MSIRECORD* record, DWORD* size)
{
    LPWSTR value = NULL;
    LPCWSTR mark, mark2;
    LPWSTR key;
    BOOL nested;
    INT failcount;
    static const WCHAR fmt[] = {'{','%','s','}',0};
    UINT sz;

    if (!group || group[0] == 0) 
    {
        *size = 0;
        return NULL;
    }
    /* if no [] then group is returned as is */

     if (!find_next_outermost_key(group, len, &key, &mark, &mark2, &nested))
     {
         *size = (len+2)*sizeof(WCHAR);
         value = msi_alloc(*size);
         sprintfW(value,fmt,group);
         /* do not return size of the null at the end */
         *size = (len+1)*sizeof(WCHAR);
         return value;
     }

     msi_free(key);
     failcount = 0;
     sz = deformat_string_internal(package, group, &value, strlenW(group),
                                     record, &failcount);
     if (failcount==0)
     {
        *size = sz * sizeof(WCHAR);
        return value;
     }
     else if (failcount < 0)
     {
         LPWSTR v2;

         v2 = msi_alloc((sz+2)*sizeof(WCHAR));
         v2[0] = '{';
         memcpy(&v2[1],value,sz*sizeof(WCHAR));
         v2[sz+1]='}';
         msi_free(value);

         *size = (sz+2)*sizeof(WCHAR);
         return v2;
     }
     else
     {
         msi_free(value);
         *size = 0;
         return NULL;
     }
}


/*
 * len is in WCHARs
 * return is also in WCHARs
 */
static DWORD deformat_string_internal(MSIPACKAGE *package, LPCWSTR ptr, 
                                     WCHAR** data, DWORD len, MSIRECORD* record,
                                     INT* failcount)
{
    LPCWSTR mark = NULL;
    LPCWSTR mark2 = NULL;
    DWORD size=0;
    DWORD chunk=0;
    LPWSTR key;
    LPWSTR value = NULL;
    DWORD sz;
    LPBYTE newdata = NULL;
    const WCHAR* progress = NULL;
    BOOL nested;

    if (ptr==NULL)
    {
        TRACE("Deformatting NULL string\n");
        *data = NULL;
        return 0;
    }

    TRACE("Starting with %s\n",debugstr_wn(ptr,len));

    /* scan for special characters... fast exit */
    if ((!scanW(ptr,'[',len) || (scanW(ptr,'[',len) && !scanW(ptr,']',len))) && 
        (scanW(ptr,'{',len) && !scanW(ptr,'}',len)))
    {
        /* not formatted */
        *data = msi_alloc((len*sizeof(WCHAR)));
        memcpy(*data,ptr,len*sizeof(WCHAR));
        TRACE("Returning %s\n",debugstr_wn(*data,len));
        return len;
    }
  
    progress = ptr;

    while (progress - ptr < len)
    {
        /* seek out first group if existing */
        if (find_next_group(progress, len - (progress - ptr), &key,
                                &mark, &mark2))
        {
            value = deformat_group(package, key, strlenW(key)+1, record, 
                            &chunk);
            msi_free( key );
            key = NULL;
            nested = FALSE;
        }
        /* formatted string located */
        else if (!find_next_outermost_key(progress, len - (progress - ptr), 
                                &key, &mark, &mark2, &nested))
        {
            LPBYTE nd2;

            TRACE("after value %s\n", debugstr_wn((LPWSTR)newdata,
                                    size/sizeof(WCHAR)));
            chunk = (len - (progress - ptr)) * sizeof(WCHAR);
            TRACE("after chunk is %i + %i\n",size,chunk);
            if (size)
                nd2 = msi_realloc(newdata,(size+chunk));
            else
                nd2 = msi_alloc(chunk);

            newdata = nd2;
            memcpy(&newdata[size],progress,chunk);
            size+=chunk;
            break;
        }

        if (mark != progress)
        {
            LPBYTE tgt;
            DWORD old_size = size;
            INT cnt = (mark - progress);
            TRACE("%i  (%i) characters before marker\n",cnt,(mark-progress));
            size += cnt * sizeof(WCHAR);
            if (!old_size)
                tgt = msi_alloc(size);
            else
                tgt = msi_realloc(newdata,size);
            newdata  = tgt;
            memcpy(&newdata[old_size],progress,(cnt * sizeof(WCHAR)));  
        }

        progress = mark;

        if (nested)
        {
            TRACE("Nested key... %s\n",debugstr_w(key));
            deformat_string_internal(package, key, &value, strlenW(key)+1,
                                     record, failcount);

            msi_free(key);
            key = value;
        }

        TRACE("Current %s .. %s\n",debugstr_wn((LPWSTR)newdata, 
                                size/sizeof(WCHAR)),debugstr_w(key));

        if (!package)
        {
            /* only deformat number indexs */
            if (key && is_key_number(key))
            {
                value = deformat_index(record,key,&chunk);  
                if (!chunk && failcount && *failcount >= 0)
                    (*failcount)++;
            }
            else
            {
                if (failcount)
                    *failcount = -1;
                if(key)
                {
                    DWORD keylen = strlenW(key);
                    chunk = (keylen + 2)*sizeof(WCHAR);
                    value = msi_alloc(chunk);
                    value[0] = '[';
                    memcpy(&value[1],key,keylen*sizeof(WCHAR));
                    value[1+keylen] = ']';
                }
            }
        }
        else
        {
            sz = 0;
            if (key) switch (key[0])
            {
                case '~':
                    value = deformat_NULL(&chunk);
                break;
                case '$':
                    value = deformat_component(package,&key[1],&chunk);
                break;
                case '#':
                    value = deformat_file(package,&key[1], &chunk, FALSE);
                break;
                case '!': /* should be short path */
                    value = deformat_file(package,&key[1], &chunk, TRUE);
                break;
                case '\\':
                    value = deformat_escape(&key[1],&chunk);
                break;
                case '%':
                    value = deformat_environment(package,&key[1],&chunk);
                break;
                default:
                    /* index keys cannot be nested */
                    if (is_key_number(key))
                        if (!nested)
                            value = deformat_index(record,key,&chunk);
                        else
                        {
                            static const WCHAR fmt[] = {'[','%','s',']',0};
                            value = msi_alloc(10);
                            sprintfW(value,fmt,key);
                            chunk = strlenW(value)*sizeof(WCHAR);
                        }
                    else
                        value = deformat_property(package,key,&chunk);
                break;      
            }
        }

        msi_free(key);

        if (value!=NULL)
        {
            LPBYTE nd2;
            TRACE("value %s, chunk %i size %i\n",debugstr_w((LPWSTR)value),
                    chunk, size);
            if (size)
                nd2= msi_realloc(newdata,(size + chunk));
            else
                nd2= msi_alloc(chunk);
            newdata = nd2;
            memcpy(&newdata[size],value,chunk);
            size+=chunk;   
            msi_free(value);
        }
        else if (failcount && *failcount >=0 )
            (*failcount)++;

        progress = mark2+1;
    }

    TRACE("after everything %s\n",debugstr_wn((LPWSTR)newdata, 
                            size/sizeof(WCHAR)));

    *data = (LPWSTR)newdata;
    return size / sizeof(WCHAR);
}


UINT MSI_FormatRecordW( MSIPACKAGE* package, MSIRECORD* record, LPWSTR buffer,
                        DWORD *size )
{
    LPWSTR deformated;
    LPWSTR rec;
    DWORD len;
    UINT rc = ERROR_INVALID_PARAMETER;

    TRACE("%p %p %p %i\n", package, record ,buffer, *size);

    rec = msi_dup_record_field(record,0);
    if (!rec)
        rec = build_default_format(record);

    TRACE("(%s)\n",debugstr_w(rec));

    len = deformat_string_internal(package,rec,&deformated,strlenW(rec),
                                   record, NULL);

    if (buffer)
    {
        if (*size>len)
        {
            memcpy(buffer,deformated,len*sizeof(WCHAR));
            rc = ERROR_SUCCESS;
            buffer[len] = 0;
        }
        else
        {
            if (*size > 0)
            {
                memcpy(buffer,deformated,(*size)*sizeof(WCHAR));
                buffer[(*size)-1] = 0;
            }
            rc = ERROR_MORE_DATA;
        }
    }
    else
        rc = ERROR_SUCCESS;

    *size = len;

    msi_free(rec);
    msi_free(deformated);
    return rc;
}

UINT MSI_FormatRecordA( MSIPACKAGE* package, MSIRECORD* record, LPSTR buffer,
                        DWORD *size )
{
    LPWSTR deformated;
    LPWSTR rec;
    DWORD len,lenA;
    UINT rc = ERROR_INVALID_PARAMETER;

    TRACE("%p %p %p %i\n", package, record ,buffer, *size);

    rec = msi_dup_record_field(record,0);
    if (!rec)
        rec = build_default_format(record);

    TRACE("(%s)\n",debugstr_w(rec));

    len = deformat_string_internal(package,rec,&deformated,strlenW(rec),
                                   record, NULL);
    /* If len is zero then WideCharToMultiByte will return 0 indicating 
     * failure, but that will do just as well since we are ignoring
     * possible errors.
     */
    lenA = WideCharToMultiByte(CP_ACP,0,deformated,len,NULL,0,NULL,NULL);

    if (buffer)
    {
        /* Ditto above */
        WideCharToMultiByte(CP_ACP,0,deformated,len,buffer,*size,NULL, NULL);
        if (*size>lenA)
        {
            rc = ERROR_SUCCESS;
            buffer[lenA] = 0;
        }
        else
        {
            rc = ERROR_MORE_DATA;
            if (*size)
                buffer[(*size)-1] = 0;
        }
    }
    else
        rc = ERROR_SUCCESS;

    *size = lenA;

    msi_free(rec);
    msi_free(deformated);
    return rc;
}


UINT WINAPI MsiFormatRecordW( MSIHANDLE hInstall, MSIHANDLE hRecord, 
                              LPWSTR szResult, DWORD *sz )
{
    UINT r = ERROR_INVALID_HANDLE;
    MSIPACKAGE *package;
    MSIRECORD *record;

    TRACE("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);

    record = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );

    if (!record)
        return ERROR_INVALID_HANDLE;
    if (!sz)
    {
        msiobj_release( &record->hdr );
        if (szResult)
            return ERROR_INVALID_PARAMETER;
        else
            return ERROR_SUCCESS;
    }

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );

    r = MSI_FormatRecordW( package, record, szResult, sz );
    msiobj_release( &record->hdr );
    if (package)
        msiobj_release( &package->hdr );
    return r;
}

UINT WINAPI MsiFormatRecordA( MSIHANDLE hInstall, MSIHANDLE hRecord,
                              LPSTR szResult, DWORD *sz )
{
    UINT r = ERROR_INVALID_HANDLE;
    MSIPACKAGE *package = NULL;
    MSIRECORD *record = NULL;

    TRACE("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);

    record = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );

    if (!record)
        return ERROR_INVALID_HANDLE;
    if (!sz)
    {
        msiobj_release( &record->hdr );
        if (szResult)
            return ERROR_INVALID_PARAMETER;
        else
            return ERROR_SUCCESS;
    }

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );

    r = MSI_FormatRecordA( package, record, szResult, sz );
    msiobj_release( &record->hdr );
    if (package)
        msiobj_release( &package->hdr );
    return r;
}
