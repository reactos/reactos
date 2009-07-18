/*
 * Server-side window class management
 *
 * Copyright (C) 2002 Mike McCormack
 * Copyright (C) 2003 Alexandre Julliard
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
#include "wine/port.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "wine/list.h"

#include "request.h"
#include "object.h"
#include "process.h"
#include "user.h"
#include "winuser.h"
#include "winternl.h"

struct window_class
{
    struct list     entry;           /* entry in process list */
    struct process *process;         /* process owning the class */
    int             count;           /* reference count */
    int             local;           /* local class? */
    atom_t          atom;            /* class atom */
    mod_handle_t    instance;        /* module instance */
    unsigned int    style;           /* class style */
    int             win_extra;       /* number of window extra bytes */
    client_ptr_t    client_ptr;      /* pointer to class in client address space */
    int             nb_extra_bytes;  /* number of extra bytes */
    char            extra_bytes[1];  /* extra bytes storage */
};

static struct window_class *create_class( struct process *process, int extra_bytes, int local )
{
    struct window_class *class;

    if (!(class = mem_alloc( sizeof(*class) + extra_bytes - 1 ))) return NULL;

    class->process = (struct process *)grab_object( process );
    class->count = 0;
    class->local = local;
    class->nb_extra_bytes = extra_bytes;
    memset( class->extra_bytes, 0, extra_bytes );
    /* other fields are initialized by caller */

    /* local classes have priority so we put them first in the list */
    if (local) list_add_head( &process->classes, &class->entry );
    else list_add_tail( &process->classes, &class->entry );
    return class;
}

static void destroy_class( struct window_class *class )
{
    list_remove( &class->entry );
    release_object( class->process );
    free( class );
}

void destroy_process_classes( struct process *process )
{
    struct list *ptr;

    while ((ptr = list_head( &process->classes )))
    {
        struct window_class *class = LIST_ENTRY( ptr, struct window_class, entry );
        destroy_class( class );
    }
}

static struct window_class *find_class( struct process *process, atom_t atom, mod_handle_t instance )
{
    struct list *ptr;

    LIST_FOR_EACH( ptr, &process->classes )
    {
        struct window_class *class = LIST_ENTRY( ptr, struct window_class, entry );
        if (class->atom != atom) continue;
        if (!instance || !class->local || class->instance == instance) return class;
    }
    return NULL;
}

struct window_class *grab_class( struct process *process, atom_t atom,
                                 mod_handle_t instance, int *extra_bytes )
{
    struct window_class *class = find_class( process, atom, instance );
    if (class)
    {
        class->count++;
        *extra_bytes = class->win_extra;
    }
    else set_error( STATUS_INVALID_HANDLE );
    return class;
}

void release_class( struct window_class *class )
{
    assert( class->count > 0 );
    class->count--;
}

int is_desktop_class( struct window_class *class )
{
    return (class->atom == DESKTOP_ATOM && !class->local);
}

int is_hwnd_message_class( struct window_class *class )
{
    static const WCHAR messageW[] = {'M','e','s','s','a','g','e'};
    static const struct unicode_str name = { messageW, sizeof(messageW) };

    return (!class->local && class->atom == find_global_atom( NULL, &name ));
}

atom_t get_class_atom( struct window_class *class )
{
    return class->atom;
}

client_ptr_t get_class_client_ptr( struct window_class *class )
{
    return class->client_ptr;
}

/* create a window class */
DECL_HANDLER(create_class)
{
    struct window_class *class;
    struct unicode_str name;
    atom_t atom;

    get_req_unicode_str( &name );
    if (name.len)
    {
        atom = add_global_atom( NULL, &name );
        if (!atom) return;
    }
    else
    {
        atom = req->atom;
        if (!grab_global_atom( NULL, atom )) return;
    }

    class = find_class( current->process, atom, req->instance );
    if (class && !class->local == !req->local)
    {
        set_win32_error( ERROR_CLASS_ALREADY_EXISTS );
        release_global_atom( NULL, atom );
        return;
    }
    if (req->extra < 0 || req->extra > 4096 || req->win_extra < 0 || req->win_extra > 4096)
    {
        /* don't allow stupid values here */
        set_error( STATUS_INVALID_PARAMETER );
        release_global_atom( NULL, atom );
        return;
    }

    if (!(class = create_class( current->process, req->extra, req->local )))
    {
        release_global_atom( NULL, atom );
        return;
    }
    class->atom       = atom;
    class->instance   = req->instance;
    class->style      = req->style;
    class->win_extra  = req->win_extra;
    class->client_ptr = req->client_ptr;
    reply->atom = atom;
}

/* destroy a window class */
DECL_HANDLER(destroy_class)
{
    struct window_class *class;
    struct unicode_str name;
    atom_t atom = req->atom;

    get_req_unicode_str( &name );
    if (name.len) atom = find_global_atom( NULL, &name );

    if (!(class = find_class( current->process, atom, req->instance )))
        set_win32_error( ERROR_CLASS_DOES_NOT_EXIST );
    else if (class->count)
        set_win32_error( ERROR_CLASS_HAS_WINDOWS );
    else
    {
        reply->client_ptr = class->client_ptr;
        destroy_class( class );
    }
}


/* set some information in a class */
DECL_HANDLER(set_class_info)
{
    struct window_class *class = get_window_class( req->window );

    if (!class) return;

    if (req->flags && class->process != current->process)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    if (req->extra_size > sizeof(req->extra_value) ||
        req->extra_offset < -1 ||
        req->extra_offset > class->nb_extra_bytes - (int)req->extra_size)
    {
        set_win32_error( ERROR_INVALID_INDEX );
        return;
    }
    if ((req->flags & SET_CLASS_WINEXTRA) && (req->win_extra < 0 || req->win_extra > 4096))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (req->extra_offset != -1)
    {
        memcpy( &reply->old_extra_value, class->extra_bytes + req->extra_offset, req->extra_size );
    }
    else if (req->flags & SET_CLASS_EXTRA)
    {
        set_win32_error( ERROR_INVALID_INDEX );
        return;
    }

    reply->old_atom      = class->atom;
    reply->old_style     = class->style;
    reply->old_extra     = class->nb_extra_bytes;
    reply->old_win_extra = class->win_extra;
    reply->old_instance  = class->instance;

    if (req->flags & SET_CLASS_ATOM)
    {
        if (!grab_global_atom( NULL, req->atom )) return;
        release_global_atom( NULL, class->atom );
        class->atom = req->atom;
    }
    if (req->flags & SET_CLASS_STYLE) class->style = req->style;
    if (req->flags & SET_CLASS_WINEXTRA) class->win_extra = req->win_extra;
    if (req->flags & SET_CLASS_INSTANCE) class->instance = req->instance;
    if (req->flags & SET_CLASS_EXTRA) memcpy( class->extra_bytes + req->extra_offset,
                                              &req->extra_value, req->extra_size );
}
