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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static CRITICAL_SECTION MSI_handle_cs;
static CRITICAL_SECTION_DEBUG MSI_handle_cs_debug =
{
    0, 0, &MSI_handle_cs,
    { &MSI_handle_cs_debug.ProcessLocksList, 
      &MSI_handle_cs_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": MSI_handle_cs") }
};
static CRITICAL_SECTION MSI_handle_cs = { &MSI_handle_cs_debug, -1, 0, 0, 0, 0 };

static CRITICAL_SECTION MSI_object_cs;
static CRITICAL_SECTION_DEBUG MSI_object_cs_debug =
{
    0, 0, &MSI_object_cs,
    { &MSI_object_cs_debug.ProcessLocksList, 
      &MSI_object_cs_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": MSI_object_cs") }
};
static CRITICAL_SECTION MSI_object_cs = { &MSI_object_cs_debug, -1, 0, 0, 0, 0 };

typedef struct msi_handle_info_t
{
    MSIOBJECTHDR *obj;
    DWORD dwThreadId;
} msi_handle_info;

static msi_handle_info msihandletable[MSIMAXHANDLES];

MSIHANDLE alloc_msihandle( MSIOBJECTHDR *obj )
{
    MSIHANDLE ret = 0;
    UINT i;

    EnterCriticalSection( &MSI_handle_cs );

    /* find a slot */
    for(i=0; i<MSIMAXHANDLES; i++)
        if( !msihandletable[i].obj )
            break;
    if( (i>=MSIMAXHANDLES) || msihandletable[i].obj )
        goto out;

    msiobj_addref( obj );
    msihandletable[i].obj = obj;
    msihandletable[i].dwThreadId = GetCurrentThreadId();
    ret = (MSIHANDLE) (i+1);
out:
    TRACE("%p -> %ld\n", obj, ret );

    LeaveCriticalSection( &MSI_handle_cs );
    return ret;
}

void *msihandle2msiinfo(MSIHANDLE handle, UINT type)
{
    MSIOBJECTHDR *ret = NULL;

    EnterCriticalSection( &MSI_handle_cs );
    handle--;
    if( handle<0 )
        goto out;
    if( handle>=MSIMAXHANDLES )
        goto out;
    if( !msihandletable[handle].obj )
        goto out;
    if( msihandletable[handle].obj->magic != MSIHANDLE_MAGIC )
        goto out;
    if( type && (msihandletable[handle].obj->type != type) )
        goto out;
    ret = msihandletable[handle].obj;
    msiobj_addref( ret );
    
out:
    LeaveCriticalSection( &MSI_handle_cs );

    return (void*) ret;
}

MSIHANDLE msiobj_findhandle( MSIOBJECTHDR *hdr )
{
    MSIHANDLE ret = 0;
    UINT i;

    TRACE("%p\n", hdr);

    EnterCriticalSection( &MSI_handle_cs );
    for(i=0; (i<MSIMAXHANDLES) && !ret; i++)
        if( msihandletable[i].obj == hdr )
            ret = i+1;
    LeaveCriticalSection( &MSI_handle_cs );

    TRACE("%p -> %ld\n", hdr, ret);

    msiobj_addref( hdr );
    return ret;
}

void *alloc_msiobject(UINT type, UINT size, msihandledestructor destroy )
{
    MSIOBJECTHDR *info;

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
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
    TRACE("%p\n", info);

    if( !info )
        return;

    if( info->magic != MSIHANDLE_MAGIC )
    {
        ERR("Invalid handle!\n");
        return;
    }

    info->refcount++;
}

void msiobj_lock( MSIOBJECTHDR *info )
{
    EnterCriticalSection( &MSI_object_cs );
}

void msiobj_unlock( MSIOBJECTHDR *info )
{
    LeaveCriticalSection( &MSI_object_cs );
}

int msiobj_release( MSIOBJECTHDR *info )
{
    int ret;

    TRACE("%p\n",info);

    if( !info )
        return -1;

    if( info->magic != MSIHANDLE_MAGIC )
    {
        ERR("Invalid handle!\n");
        return -1;
    }

    ret = info->refcount--;
    if (info->refcount == 0)
    {
    if( info->destructor )
            info->destructor( info );
    HeapFree( GetProcessHeap(), 0, info );
        TRACE("object %p destroyed\n", info);
    }

    return ret;
}

/***********************************************************
 *   MsiCloseHandle   [MSI.@]
 */
UINT WINAPI MsiCloseHandle(MSIHANDLE handle)
{
    MSIOBJECTHDR *info;
    UINT ret = ERROR_INVALID_HANDLE;

    TRACE("%lx\n",handle);

    EnterCriticalSection( &MSI_handle_cs );

    info = msihandle2msiinfo(handle, 0);
    if( !info )
        goto out;

    if( info->magic != MSIHANDLE_MAGIC )
    {
        ERR("Invalid handle!\n");
        goto out;
    }

    msiobj_release( info );
    msihandletable[handle-1].obj = NULL;
    ret = ERROR_SUCCESS;

    TRACE("handle %lx Destroyed\n", handle);
out:
    LeaveCriticalSection( &MSI_handle_cs );
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

    for(i=0; i<MSIMAXHANDLES; i++)
    {
        if(msihandletable[i].dwThreadId == GetCurrentThreadId())
        {
            MsiCloseHandle( i+1 );
            n++;
        }
    }

    return n;
}
