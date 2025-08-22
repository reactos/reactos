/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
 *
 * Based on the handle implementation from wininet.
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

#ifdef __REACTOS__
#include "wine/config.h"
#else
#include "config.h"
#endif
#include "ws2tcpip.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

#define HANDLE_CHUNK_SIZE 0x10

static CRITICAL_SECTION handle_cs;
static CRITICAL_SECTION_DEBUG handle_cs_debug =
{
    0, 0, &handle_cs,
    { &handle_cs_debug.ProcessLocksList, &handle_cs_debug.ProcessLocksList },
      0, 0, { (ULONG_PTR)(__FILE__ ": handle_cs") }
};
static CRITICAL_SECTION handle_cs = { &handle_cs_debug, -1, 0, 0, 0, 0 };

static struct object_header **handles;
static ULONG_PTR next_handle;
static ULONG_PTR max_handles;

struct object_header *addref_object( struct object_header *hdr )
{
    ULONG refs = InterlockedIncrement( &hdr->refs );
    TRACE("%p -> refcount = %d\n", hdr, refs);
    return hdr;
}

struct object_header *grab_object( HINTERNET hinternet )
{
    struct object_header *hdr = NULL;
    ULONG_PTR handle = (ULONG_PTR)hinternet;

    EnterCriticalSection( &handle_cs );

    if ((handle > 0) && (handle <= max_handles) && handles[handle - 1])
        hdr = addref_object( handles[handle - 1] );

    LeaveCriticalSection( &handle_cs );

    TRACE("handle 0x%lx -> %p\n", handle, hdr);
    return hdr;
}

void release_object( struct object_header *hdr )
{
    ULONG refs = InterlockedDecrement( &hdr->refs );
    TRACE("object %p refcount = %d\n", hdr, refs);
    if (!refs)
    {
        if (hdr->type == WINHTTP_HANDLE_TYPE_REQUEST) close_connection( (struct request *)hdr );

        send_callback( hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, &hdr->handle, sizeof(HINTERNET) );

        TRACE("destroying object %p\n", hdr);
        if (hdr->type != WINHTTP_HANDLE_TYPE_SESSION) list_remove( &hdr->entry );
        hdr->vtbl->destroy( hdr );
    }
}

HINTERNET alloc_handle( struct object_header *hdr )
{
    struct object_header **p;
    ULONG_PTR handle, num;

    list_init( &hdr->children );
    hdr->handle = NULL;

    EnterCriticalSection( &handle_cs );
    if (!max_handles)
    {
        num = HANDLE_CHUNK_SIZE;
        if (!(p = heap_alloc_zero( sizeof(ULONG_PTR) * num ))) goto end;
        handles = p;
        max_handles = num;
    }
    if (max_handles == next_handle)
    {
        num = max_handles * 2;
        if (!(p = heap_realloc_zero( handles, sizeof(ULONG_PTR) * num ))) goto end;
        handles = p;
        max_handles = num;
    }
    handle = next_handle;
    if (handles[handle]) ERR("handle isn't free but should be\n");

    handles[handle] = addref_object( hdr );
    hdr->handle = (HINTERNET)(handle + 1);
    while ((next_handle < max_handles) && handles[next_handle]) next_handle++;

end:
    LeaveCriticalSection( &handle_cs );
    return hdr->handle;
}

BOOL free_handle( HINTERNET hinternet )
{
    BOOL ret = FALSE;
    ULONG_PTR handle = (ULONG_PTR)hinternet;
    struct object_header *hdr = NULL, *child, *next;

    EnterCriticalSection( &handle_cs );

    if ((handle > 0) && (handle <= max_handles))
    {
        handle--;
        if (handles[handle])
        {
            hdr = handles[handle];
            TRACE("destroying handle 0x%lx for object %p\n", handle + 1, hdr);
            handles[handle] = NULL;
            ret = TRUE;
        }
    }

    LeaveCriticalSection( &handle_cs );

    if (hdr)
    {
        LIST_FOR_EACH_ENTRY_SAFE( child, next, &hdr->children, struct object_header, entry )
        {
            TRACE("freeing child handle %p for parent handle 0x%lx\n", child->handle, handle + 1);
            free_handle( child->handle );
        }
        release_object( hdr );
    }

    EnterCriticalSection( &handle_cs );
    if (next_handle > handle && !handles[handle]) next_handle = handle;
    LeaveCriticalSection( &handle_cs );

    return ret;
}
