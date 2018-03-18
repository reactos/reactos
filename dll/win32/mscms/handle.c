/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005, 2008 Hans Leidekker
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
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

#ifdef HAVE_LCMS2

static CRITICAL_SECTION mscms_handle_cs;
static CRITICAL_SECTION_DEBUG mscms_handle_cs_debug =
{
    0, 0, &mscms_handle_cs,
    { &mscms_handle_cs_debug.ProcessLocksList,
      &mscms_handle_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": mscms_handle_cs") }
};
static CRITICAL_SECTION mscms_handle_cs = { &mscms_handle_cs_debug, -1, 0, 0, 0, 0 };

static struct profile *profiletable;
static struct transform *transformtable;

static unsigned int num_profile_handles;
static unsigned int num_transform_handles;

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

void free_handle_tables( void )
{
    HeapFree( GetProcessHeap(), 0, profiletable );
    profiletable = NULL;
    num_profile_handles = 0;

    HeapFree( GetProcessHeap(), 0, transformtable );
    transformtable = NULL;
    num_transform_handles = 0;

    DeleteCriticalSection( &mscms_handle_cs );
}

struct profile *grab_profile( HPROFILE handle )
{
    DWORD_PTR index;

    EnterCriticalSection( &mscms_handle_cs );

    index = (DWORD_PTR)handle - 1;
    if (index > num_profile_handles)
    {
        LeaveCriticalSection( &mscms_handle_cs );
        return NULL;
    }
    return &profiletable[index];
}

void release_profile( struct profile *profile )
{
    LeaveCriticalSection( &mscms_handle_cs );
}

struct transform *grab_transform( HTRANSFORM handle )
{
    DWORD_PTR index;

    EnterCriticalSection( &mscms_handle_cs );

    index = (DWORD_PTR)handle - 1;
    if (index > num_transform_handles)
    {
        LeaveCriticalSection( &mscms_handle_cs );
        return NULL;
    }
    return &transformtable[index];
}

void release_transform( struct transform *transform )
{
    LeaveCriticalSection( &mscms_handle_cs );
}

static HPROFILE alloc_profile_handle( void )
{
    DWORD_PTR index;
    struct profile *p;
    unsigned int count = 128;

    for (index = 0; index < num_profile_handles; index++)
    {
        if (!profiletable[index].data) return (HPROFILE)(index + 1);
    }
    if (!profiletable)
    {
        p = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, count * sizeof(struct profile) );
    }
    else
    {
        count = num_profile_handles * 2;
        p = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, profiletable, count * sizeof(struct profile) );
    }
    if (!p) return NULL;

    profiletable = p;
    num_profile_handles = count;

    return (HPROFILE)(index + 1);
}

HPROFILE create_profile( struct profile *profile )
{
    HPROFILE handle;

    EnterCriticalSection( &mscms_handle_cs );

    if ((handle = alloc_profile_handle()))
    {
        DWORD_PTR index = (DWORD_PTR)handle - 1;
        profiletable[index] = *profile;
    }
    LeaveCriticalSection( &mscms_handle_cs );
    return handle;
}

BOOL close_profile( HPROFILE handle )
{
    DWORD_PTR index;
    struct profile *profile;

    EnterCriticalSection( &mscms_handle_cs );

    index = (DWORD_PTR)handle - 1;
    if (index > num_profile_handles)
    {
        LeaveCriticalSection( &mscms_handle_cs );
        return FALSE;
    }
    profile = &profiletable[index];

    if (profile->file != INVALID_HANDLE_VALUE)
    {
        if (profile->access & PROFILE_READWRITE)
        {
            DWORD written;

            if (SetFilePointer( profile->file, 0, NULL, FILE_BEGIN ) ||
                !WriteFile( profile->file, profile->data, profile->size, &written, NULL ) ||
                written != profile->size)
            {
                ERR( "Unable to write color profile\n" );
            }
        }
        CloseHandle( profile->file );
    }
    cmsCloseProfile( profile->cmsprofile );
    HeapFree( GetProcessHeap(), 0, profile->data );

    memset( profile, 0, sizeof(struct profile) );

    LeaveCriticalSection( &mscms_handle_cs );
    return TRUE;
}

static HTRANSFORM alloc_transform_handle( void )
{
    DWORD_PTR index;
    struct transform *p;
    unsigned int count = 128;

    for (index = 0; index < num_transform_handles; index++)
    {
        if (!transformtable[index].cmstransform) return (HTRANSFORM)(index + 1);
    }
    if (!transformtable)
    {
        p = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, count * sizeof(struct transform) );
    }
    else
    {
        count = num_transform_handles * 2;
        p = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, transformtable, count * sizeof(struct transform) );
    }
    if (!p) return NULL;

    transformtable = p;
    num_transform_handles = count;

    return (HTRANSFORM)(index + 1);
}

HTRANSFORM create_transform( struct transform *transform )
{
    HTRANSFORM handle;

    EnterCriticalSection( &mscms_handle_cs );

    if ((handle = alloc_transform_handle()))
    {
        DWORD_PTR index = (DWORD_PTR)handle - 1;
        transformtable[index] = *transform;
    }
    LeaveCriticalSection( &mscms_handle_cs );
    return handle;
}

BOOL close_transform( HTRANSFORM handle )
{
    DWORD_PTR index;
    struct transform *transform;

    EnterCriticalSection( &mscms_handle_cs );

    index = (DWORD_PTR)handle - 1;
    if (index > num_transform_handles)
    {
        LeaveCriticalSection( &mscms_handle_cs );
        return FALSE;
    }
    transform = &transformtable[index];

    cmsDeleteTransform( transform->cmstransform );
    memset( transform, 0, sizeof(struct transform) );

    LeaveCriticalSection( &mscms_handle_cs );
    return TRUE;
}

#endif /* HAVE_LCMS2 */
