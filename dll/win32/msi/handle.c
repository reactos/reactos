/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"

#include "msipriv.h"
#include "winemsi_s.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static CRITICAL_SECTION handle_cs;
static CRITICAL_SECTION_DEBUG handle_cs_debug =
{
    0, 0, &handle_cs,
    { &handle_cs_debug.ProcessLocksList,
      &handle_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": handle_cs") }
};
static CRITICAL_SECTION handle_cs = { &handle_cs_debug, -1, 0, 0, 0, 0 };

static CRITICAL_SECTION object_cs;
static CRITICAL_SECTION_DEBUG object_cs_debug =
{
    0, 0, &object_cs,
    { &object_cs_debug.ProcessLocksList,
      &object_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": object_cs") }
};
static CRITICAL_SECTION object_cs = { &object_cs_debug, -1, 0, 0, 0, 0 };

struct handle_info
{
    BOOL remote;
    union {
        MSIOBJECTHDR *obj;
        MSIHANDLE rem;
    } u;
    DWORD dwThreadId;
};

static struct handle_info *handle_table = NULL;
static unsigned int handle_table_size = 0;

void msi_free_handle_table(void)
{
    free( handle_table );
    handle_table = NULL;
    handle_table_size = 0;
    DeleteCriticalSection(&handle_cs);
    DeleteCriticalSection(&object_cs);
}

static MSIHANDLE alloc_handle_table_entry(void)
{
    UINT i;

    /* find a slot */
    for(i = 0; i < handle_table_size; i++)
        if (!handle_table[i].u.obj && !handle_table[i].u.rem)
            break;
    if (i == handle_table_size)
    {
        struct handle_info *p;
        int newsize;
        if (!handle_table_size)
        {
            newsize = 256;
            p = calloc(newsize, sizeof(*p));
        }
        else
        {
            newsize = handle_table_size * 2;
            p = realloc(handle_table, newsize * sizeof(*p));
            if (p) memset(p + handle_table_size, 0, (newsize - handle_table_size) * sizeof(*p));
        }
        if (!p)
            return 0;
        handle_table = p;
        handle_table_size = newsize;
    }
    return i + 1;
}

MSIHANDLE alloc_msihandle( MSIOBJECTHDR *obj )
{
    struct handle_info *entry;
    MSIHANDLE ret;

    EnterCriticalSection( &handle_cs );

    ret = alloc_handle_table_entry();
    if (ret)
    {
        entry = &handle_table[ ret - 1 ];
        msiobj_addref( obj );
        entry->u.obj = obj;
        entry->dwThreadId = GetCurrentThreadId();
        entry->remote = FALSE;
    }

    LeaveCriticalSection( &handle_cs );

    TRACE( "%p -> %lu\n", obj, ret );

    return ret;
}

MSIHANDLE alloc_msi_remote_handle(MSIHANDLE remote)
{
    struct handle_info *entry;
    MSIHANDLE ret;

    EnterCriticalSection( &handle_cs );

    ret = alloc_handle_table_entry();
    if (ret)
    {
        entry = &handle_table[ ret - 1 ];
        entry->u.rem = remote;
        entry->dwThreadId = GetCurrentThreadId();
        entry->remote = TRUE;
    }

    LeaveCriticalSection( &handle_cs );

    TRACE( "%lu -> %lu\n", remote, ret );

    return ret;
}

void *msihandle2msiinfo(MSIHANDLE handle, UINT type)
{
    MSIOBJECTHDR *ret = NULL;

    EnterCriticalSection( &handle_cs );
    handle--;
    if (handle >= handle_table_size)
        goto out;
    if (handle_table[handle].remote)
        goto out;
    if (!handle_table[handle].u.obj)
        goto out;
    if (handle_table[handle].u.obj->magic != MSIHANDLE_MAGIC)
        goto out;
    if (type && (handle_table[handle].u.obj->type != type))
        goto out;
    ret = handle_table[handle].u.obj;
    msiobj_addref( ret );

out:
    LeaveCriticalSection( &handle_cs );

    return ret;
}

MSIHANDLE msi_get_remote( MSIHANDLE handle )
{
    MSIHANDLE ret = 0;

    EnterCriticalSection( &handle_cs );
    handle--;
    if (handle >= handle_table_size)
        goto out;
    if (!handle_table[handle].remote)
        goto out;
    ret = handle_table[handle].u.rem;

out:
    LeaveCriticalSection( &handle_cs );

    return ret;
}

void *alloc_msiobject(UINT type, UINT size, msihandledestructor destroy )
{
    MSIOBJECTHDR *info;

    info = calloc( 1, size );
    if( info )
    {
        info->magic = MSIHANDLE_MAGIC;
        info->type = type;
        info->refcount = 1;
        info->destructor = destroy;
    }

    return info;
}

void msiobj_addref( MSIOBJECTHDR *info )
{
    if( !info )
        return;

    if( info->magic != MSIHANDLE_MAGIC )
    {
        ERR("Invalid handle!\n");
        return;
    }

    InterlockedIncrement(&info->refcount);
}

void msiobj_lock( MSIOBJECTHDR *info )
{
    EnterCriticalSection( &object_cs );
}

void msiobj_unlock( MSIOBJECTHDR *info )
{
    LeaveCriticalSection( &object_cs );
}

int msiobj_release( MSIOBJECTHDR *info )
{
    int ret;

    if( !info )
        return -1;

    if( info->magic != MSIHANDLE_MAGIC )
    {
        ERR("Invalid handle!\n");
        return -1;
    }

    ret = InterlockedDecrement( &info->refcount );
    if( ret==0 )
    {
        if( info->destructor )
            info->destructor( info );
        TRACE("object %p destroyed\n", info);
        free( info );
    }

    return ret;
}

/***********************************************************
 *   MsiCloseHandle   [MSI.@]
 */
UINT WINAPI MsiCloseHandle(MSIHANDLE handle)
{
    MSIOBJECTHDR *info = NULL;
    UINT ret = ERROR_INVALID_HANDLE;

    TRACE( "%lu\n", handle );

    if (!handle)
        return ERROR_SUCCESS;

    EnterCriticalSection( &handle_cs );

    handle--;
    if (handle >= handle_table_size)
        goto out;

    if (handle_table[handle].remote)
    {
        remote_CloseHandle( handle_table[handle].u.rem );
    }
    else
    {
        info = handle_table[handle].u.obj;
        if( !info )
            goto out;

        if( info->magic != MSIHANDLE_MAGIC )
        {
            ERR("Invalid handle!\n");
            goto out;
        }
    }

    handle_table[handle].u.obj = NULL;
    handle_table[handle].remote = 0;
    handle_table[handle].dwThreadId = 0;

    ret = ERROR_SUCCESS;

    TRACE( "handle %lu destroyed\n", handle + 1 );
out:
    LeaveCriticalSection( &handle_cs );
    if( info )
        msiobj_release( info );

    return ret;
}

/***********************************************************
 *   MsiCloseAllHandles   [MSI.@]
 *
 *  Closes all handles owned by the current thread
 *
 *  RETURNS:
 *   The number of handles closed
 */
UINT WINAPI MsiCloseAllHandles(void)
{
    UINT i, n=0;

    TRACE("\n");

    EnterCriticalSection( &handle_cs );
    for (i = 0; i < handle_table_size; i++)
    {
        if (handle_table[i].dwThreadId == GetCurrentThreadId())
        {
            LeaveCriticalSection( &handle_cs );
            MsiCloseHandle( i + 1 );
            EnterCriticalSection( &handle_cs );
            n++;
        }
    }
    LeaveCriticalSection( &handle_cs );

    return n;
}

UINT __cdecl s_remote_CloseHandle(MSIHANDLE handle)
{
    return MsiCloseHandle(handle);
}
