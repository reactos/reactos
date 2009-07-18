/*
 * Server-side USER handles
 *
 * Copyright (C) 2001 Alexandre Julliard
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

#include "thread.h"
#include "user.h"

struct user_handle
{
    void          *ptr;          /* pointer to object */
    unsigned short type;         /* object type (0 if free) */
    unsigned short generation;   /* generation counter */
};

static struct user_handle *handles;
static struct user_handle *freelist;
static int nb_handles;
static int allocated_handles;

static struct user_handle *handle_to_entry( user_handle_t handle )
{
    unsigned short generation;
    int index = ((handle & 0xffff) - FIRST_USER_HANDLE) >> 1;
    if (index < 0 || index >= nb_handles) return NULL;
    if (!handles[index].type) return NULL;
    generation = handle >> 16;
    if (generation == handles[index].generation || !generation || generation == 0xffff)
        return &handles[index];
    return NULL;
}

static inline user_handle_t entry_to_handle( struct user_handle *ptr )
{
    unsigned int index = ptr - handles;
    return (index << 1) + FIRST_USER_HANDLE + (ptr->generation << 16);
}

static inline struct user_handle *alloc_user_entry(void)
{
    struct user_handle *handle;

    if (freelist)
    {
        handle = freelist;
        freelist = handle->ptr;
        return handle;
    }
    if (nb_handles >= allocated_handles)  /* need to grow the array */
    {
        struct user_handle *new_handles;
        /* grow array by 50% (but at minimum 32 entries) */
        int growth = max( 32, allocated_handles / 2 );
        int new_size = min( allocated_handles + growth, (LAST_USER_HANDLE-FIRST_USER_HANDLE+1) >> 1 );
        if (new_size <= allocated_handles) return NULL;
        if (!(new_handles = realloc( handles, new_size * sizeof(*handles) )))
            return NULL;
        handles = new_handles;
        allocated_handles = new_size;
    }
    handle = &handles[nb_handles++];
    handle->generation = 0;
    return handle;
}

static inline void *free_user_entry( struct user_handle *ptr )
{
    void *ret;
    ret = ptr->ptr;
    ptr->ptr  = freelist;
    ptr->type = 0;
    freelist  = ptr;
    return ret;
}

/* allocate a user handle for a given object */
user_handle_t alloc_user_handle( void *ptr, enum user_object type )
{
    struct user_handle *entry = alloc_user_entry();
    if (!entry) return 0;
    entry->ptr  = ptr;
    entry->type = type;
    if (++entry->generation >= 0xffff) entry->generation = 1;
    return entry_to_handle( entry );
}

/* return a pointer to a user object from its handle */
void *get_user_object( user_handle_t handle, enum user_object type )
{
    struct user_handle *entry;

    if (!(entry = handle_to_entry( handle )) || entry->type != type) return NULL;
    return entry->ptr;
}

/* get the full handle for a possibly truncated handle */
user_handle_t get_user_full_handle( user_handle_t handle )
{
    struct user_handle *entry;

    if (handle >> 16) return handle;
    if (!(entry = handle_to_entry( handle ))) return handle;
    return entry_to_handle( entry );
}

/* same as get_user_object plus set the handle to the full 32-bit value */
void *get_user_object_handle( user_handle_t *handle, enum user_object type )
{
    struct user_handle *entry;

    if (!(entry = handle_to_entry( *handle )) || entry->type != type) return NULL;
    *handle = entry_to_handle( entry );
    return entry->ptr;
}

/* free a user handle and return a pointer to the object */
void *free_user_handle( user_handle_t handle )
{
    struct user_handle *entry;

    if (!(entry = handle_to_entry( handle )))
    {
        set_error( STATUS_INVALID_HANDLE );
        return NULL;
    }
    return free_user_entry( entry );
}

/* return the next user handle after 'handle' that is of a given type */
void *next_user_handle( user_handle_t *handle, enum user_object type )
{
    struct user_handle *entry;

    if (!*handle) entry = handles;
    else
    {
        int index = ((*handle & 0xffff) - FIRST_USER_HANDLE) >> 1;
        if (index < 0 || index >= nb_handles) return NULL;
        entry = handles + index + 1;  /* start from the next one */
    }
    while (entry < handles + nb_handles)
    {
        if (!type || entry->type == type)
        {
            *handle = entry_to_handle( entry );
            return entry->ptr;
        }
        entry++;
    }
    return NULL;
}
