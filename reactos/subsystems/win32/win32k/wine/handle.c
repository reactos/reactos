/*
 * Server-side handle management
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "security.h"
#include "request.h"

struct handle_entry
{
    struct object *ptr;       /* object */
    unsigned int   access;    /* access rights */
};

struct handle_table
{
    struct object        obj;         /* object header */
    struct process      *process;     /* process owning this table */
    int                  count;       /* number of allocated entries */
    int                  last;        /* last used entry */
    int                  free;        /* first entry that may be free */
    struct handle_entry *entries;     /* handle entries */
};

static struct handle_table *global_table;

/* reserved handle access rights */
#define RESERVED_SHIFT         26
#define RESERVED_INHERIT       (HANDLE_FLAG_INHERIT << RESERVED_SHIFT)
#define RESERVED_CLOSE_PROTECT (HANDLE_FLAG_PROTECT_FROM_CLOSE << RESERVED_SHIFT)
#define RESERVED_ALL           (RESERVED_INHERIT | RESERVED_CLOSE_PROTECT)

#define MIN_HANDLE_ENTRIES  32
#define MAX_HANDLE_ENTRIES  0x00ffffff


/* handle to table index conversion */

/* handles are a multiple of 4 under NT; handle 0 is not used */
static inline obj_handle_t index_to_handle( int index )
{
    return (obj_handle_t)((index + 1) << 2);
}
static inline int handle_to_index( obj_handle_t handle )
{
    return (handle >> 2) - 1;
}

/* global handle conversion */

#define HANDLE_OBFUSCATOR 0x544a4def

static inline int handle_is_global( obj_handle_t handle)
{
    return (handle ^ HANDLE_OBFUSCATOR) <= (MAX_HANDLE_ENTRIES << 2);
}
static inline obj_handle_t handle_local_to_global( obj_handle_t handle )
{
    if (!handle) return 0;
    return handle ^ HANDLE_OBFUSCATOR;
}
static inline obj_handle_t handle_global_to_local( obj_handle_t handle )
{
    return handle ^ HANDLE_OBFUSCATOR;
}


static void handle_table_dump( struct object *obj, int verbose );
static void handle_table_destroy( struct object *obj );

static const struct object_ops handle_table_ops =
{
    sizeof(struct handle_table),     /* size */
    handle_table_dump,               /* dump */
    no_get_type,                     /* get_type */
    no_add_queue,                    /* add_queue */
    NULL,                            /* remove_queue */
    NULL,                            /* signaled */
    NULL,                            /* satisfied */
    no_signal,                       /* signal */
    no_get_fd,                       /* get_fd */
    no_map_access,                   /* map_access */
    default_get_sd,                  /* get_sd */
    default_set_sd,                  /* set_sd */
    no_lookup_name,                  /* lookup_name */
    no_open_file,                    /* open_file */
    no_close_handle,                 /* close_handle */
    handle_table_destroy             /* destroy */
};

/* dump a handle table */
static void handle_table_dump( struct object *obj, int verbose )
{
    int i;
    struct handle_table *table = (struct handle_table *)obj;
    struct handle_entry *entry = table->entries;

    assert( obj->ops == &handle_table_ops );

    fprintf( stderr, "Handle table last=%d count=%d process=%p\n",
             table->last, table->count, table->process );
    if (!verbose) return;
    entry = table->entries;
    for (i = 0; i <= table->last; i++, entry++)
    {
        if (!entry->ptr) continue;
        fprintf( stderr, "    %04x: %p %08x ",
                 index_to_handle(i), entry->ptr, entry->access );
        entry->ptr->ops->dump( entry->ptr, 0 );
    }
}

/* destroy a handle table */
static void handle_table_destroy( struct object *obj )
{
    int i;
    struct handle_table *table = (struct handle_table *)obj;
    struct handle_entry *entry;

    assert( obj->ops == &handle_table_ops );

    /* first notify all objects that handles are being closed */
    if (table->process)
    {
        for (i = 0, entry = table->entries; i <= table->last; i++, entry++)
        {
            struct object *obj = entry->ptr;
            if (obj) obj->ops->close_handle( obj, table->process, index_to_handle(i) );
        }
    }

    for (i = 0, entry = table->entries; i <= table->last; i++, entry++)
    {
        struct object *obj = entry->ptr;
        entry->ptr = NULL;
        if (obj) release_object( obj );
    }
    free( table->entries );
}

/* allocate a new handle table */
struct handle_table *alloc_handle_table( struct process *process, int count )
{
    struct handle_table *table;

    if (count < MIN_HANDLE_ENTRIES) count = MIN_HANDLE_ENTRIES;
    if (!(table = alloc_object( &handle_table_ops )))
        return NULL;
    table->process = process;
    table->count   = count;
    table->last    = -1;
    table->free    = 0;
    if ((table->entries = mem_alloc( count * sizeof(*table->entries) ))) return table;
    release_object( table );
    return NULL;
}

/* grow a handle table */
static int grow_handle_table( struct handle_table *table )
{
    struct handle_entry *new_entries;
    int count = min( table->count * 2, MAX_HANDLE_ENTRIES );

    if (count == table->count ||
        !(new_entries = realloc( table->entries, count * sizeof(struct handle_entry) )))
    {
        set_error( STATUS_INSUFFICIENT_RESOURCES );
        return 0;
    }
    table->entries = new_entries;
    table->count   = count;
    return 1;
}

/* allocate the first free entry in the handle table */
static obj_handle_t alloc_entry( struct handle_table *table, void *obj, unsigned int access )
{
    struct handle_entry *entry = table->entries + table->free;
    int i;

    for (i = table->free; i <= table->last; i++, entry++) if (!entry->ptr) goto found;
    if (i >= table->count)
    {
        if (!grow_handle_table( table )) return 0;
        entry = table->entries + i;  /* the entries may have moved */
    }
    table->last = i;
 found:
    table->free = i + 1;
    entry->ptr    = grab_object( obj );
    entry->access = access;
    return index_to_handle(i);
}

/* allocate a handle for an object, incrementing its refcount */
/* return the handle, or 0 on error */
obj_handle_t alloc_handle_no_access_check( struct process *process, void *ptr, unsigned int access, unsigned int attr )
{
    struct object *obj = ptr;

    access &= ~RESERVED_ALL;
    if (attr & OBJ_INHERIT) access |= RESERVED_INHERIT;
    if (!process->handles)
    {
        set_error( STATUS_NO_MEMORY );
        return 0;
    }
    return alloc_entry( process->handles, obj, access );
}

/* allocate a handle for an object, checking the dacl allows the process to */
/* access it and incrementing its refcount */
/* return the handle, or 0 on error */
obj_handle_t alloc_handle( struct process *process, void *ptr, unsigned int access, unsigned int attr )
{
    struct object *obj = ptr;
    access = obj->ops->map_access( obj, access );
    if (access && !check_object_access( obj, &access )) return 0;
    return alloc_handle_no_access_check( process, ptr, access, attr );
}

/* allocate a global handle for an object, incrementing its refcount */
/* return the handle, or 0 on error */
static obj_handle_t alloc_global_handle_no_access_check( void *obj, unsigned int access )
{
    if (!global_table)
    {
        if (!(global_table = (struct handle_table *)alloc_handle_table( NULL, 0 )))
            return 0;
        make_object_static( &global_table->obj );
    }
    return handle_local_to_global( alloc_entry( global_table, obj, access ));
}

/* allocate a global handle for an object, checking the dacl allows the */
/* process to access it and incrementing its refcount and incrementing its refcount */
/* return the handle, or 0 on error */
static obj_handle_t alloc_global_handle( void *obj, unsigned int access )
{
    if (access && !check_object_access( obj, &access )) return 0;
    return alloc_global_handle_no_access_check( obj, access );
}

/* return a handle entry, or NULL if the handle is invalid */
static struct handle_entry *get_handle( struct process *process, obj_handle_t handle )
{
    struct handle_table *table = process->handles;
    struct handle_entry *entry;
    int index;

    if (handle_is_global(handle))
    {
        handle = handle_global_to_local(handle);
        table = global_table;
    }
    if (!table) goto error;
    index = handle_to_index( handle );
    if (index < 0) goto error;
    if (index > table->last) goto error;
    entry = table->entries + index;
    if (!entry->ptr) goto error;
    return entry;

 error:
    set_error( STATUS_INVALID_HANDLE );
    return NULL;
}

/* attempt to shrink a table */
static void shrink_handle_table( struct handle_table *table )
{
    struct handle_entry *entry = table->entries + table->last;
    struct handle_entry *new_entries;
    int count = table->count;

    while (table->last >= 0)
    {
        if (entry->ptr) break;
        table->last--;
        entry--;
    }
    if (table->last >= count / 4) return;  /* no need to shrink */
    if (count < MIN_HANDLE_ENTRIES * 2) return;  /* too small to shrink */
    count /= 2;
    if (!(new_entries = realloc( table->entries, count * sizeof(*new_entries) ))) return;
    table->count   = count;
    table->entries = new_entries;
}

/* copy the handle table of the parent process */
/* return 1 if OK, 0 on error */
struct handle_table *copy_handle_table( struct process *process, struct process *parent )
{
    struct handle_table *parent_table = parent->handles;
    struct handle_table *table;
    int i;

    assert( parent_table );
    assert( parent_table->obj.ops == &handle_table_ops );

    if (!(table = (struct handle_table *)alloc_handle_table( process, parent_table->count )))
        return NULL;

    if ((table->last = parent_table->last) >= 0)
    {
        struct handle_entry *ptr = table->entries;
        memcpy( ptr, parent_table->entries, (table->last + 1) * sizeof(struct handle_entry) );
        for (i = 0; i <= table->last; i++, ptr++)
        {
            if (!ptr->ptr) continue;
            if (ptr->access & RESERVED_INHERIT) grab_object( ptr->ptr );
            else ptr->ptr = NULL; /* don't inherit this entry */
        }
    }
    /* attempt to shrink the table */
    shrink_handle_table( table );
    return table;
}

/* close a handle and decrement the refcount of the associated object */
/* return 1 if OK, 0 on error */
int close_handle( struct process *process, obj_handle_t handle )
{
    struct handle_table *table;
    struct handle_entry *entry;
    struct object *obj;

    if (!(entry = get_handle( process, handle ))) return 0;
    if (entry->access & RESERVED_CLOSE_PROTECT)
    {
        set_error( STATUS_HANDLE_NOT_CLOSABLE );
        return 0;
    }
    obj = entry->ptr;
    if (!obj->ops->close_handle( obj, process, handle ))
    {
        set_error( STATUS_HANDLE_NOT_CLOSABLE );
        return 0;
    }
    entry->ptr = NULL;
    table = handle_is_global(handle) ? global_table : process->handles;
    if (entry < table->entries + table->free) table->free = entry - table->entries;
    if (entry == table->entries + table->last) shrink_handle_table( table );
    release_object( obj );
    return 1;
}

/* retrieve the object corresponding to one of the magic pseudo-handles */
static inline struct object *get_magic_handle( obj_handle_t handle )
{
    switch(handle)
    {
        case 0xfffffffe:  /* current thread pseudo-handle */
            return &current->obj;
        case 0x7fffffff:  /* current process pseudo-handle */
        case 0xffffffff:  /* current process pseudo-handle */
            return (struct object *)current->process;
        default:
            return NULL;
    }
}

/* retrieve the object corresponding to a handle, incrementing its refcount */
struct object *get_handle_obj( struct process *process, obj_handle_t handle,
                               unsigned int access, const struct object_ops *ops )
{
    struct handle_entry *entry;
    struct object *obj;

    if (!(obj = get_magic_handle( handle )))
    {
        if (!(entry = get_handle( process, handle ))) return NULL;
        if ((entry->access & access) != access)
        {
            set_error( STATUS_ACCESS_DENIED );
            return NULL;
        }
        obj = entry->ptr;
    }
    if (ops && (obj->ops != ops))
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );  /* not the right type */
        return NULL;
    }
    return grab_object( obj );
}

/* retrieve the access rights of a given handle */
unsigned int get_handle_access( struct process *process, obj_handle_t handle )
{
    struct handle_entry *entry;

    if (get_magic_handle( handle )) return ~RESERVED_ALL;  /* magic handles have all access rights */
    if (!(entry = get_handle( process, handle ))) return 0;
    return entry->access & ~RESERVED_ALL;
}

/* find the first inherited handle of the given type */
/* this is needed for window stations and desktops (don't ask...) */
obj_handle_t find_inherited_handle( struct process *process, const struct object_ops *ops )
{
    struct handle_table *table = process->handles;
    struct handle_entry *ptr;
    int i;

    if (!table) return 0;

    for (i = 0, ptr = table->entries; i <= table->last; i++, ptr++)
    {
        if (!ptr->ptr) continue;
        if (ptr->ptr->ops != ops) continue;
        if (ptr->access & RESERVED_INHERIT) return index_to_handle(i);
    }
    return 0;
}

/* enumerate handles of a given type */
/* this is needed for window stations and desktops */
obj_handle_t enumerate_handles( struct process *process, const struct object_ops *ops,
                                unsigned int *index )
{
    struct handle_table *table = process->handles;
    unsigned int i;
    struct handle_entry *entry;

    if (!table) return 0;

    for (i = *index, entry = &table->entries[i]; i <= table->last; i++, entry++)
    {
        if (!entry->ptr) continue;
        if (entry->ptr->ops != ops) continue;
        *index = i + 1;
        return index_to_handle(i);
    }
    return 0;
}

/* get/set the handle reserved flags */
/* return the old flags (or -1 on error) */
static int set_handle_flags( struct process *process, obj_handle_t handle, int mask, int flags )
{
    struct handle_entry *entry;
    unsigned int old_access;

    if (get_magic_handle( handle ))
    {
        /* we can retrieve but not set info for magic handles */
        if (mask) set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if (!(entry = get_handle( process, handle ))) return -1;
    old_access = entry->access;
    mask  = (mask << RESERVED_SHIFT) & RESERVED_ALL;
    flags = (flags << RESERVED_SHIFT) & mask;
    entry->access = (entry->access & ~mask) | flags;
    return (old_access & RESERVED_ALL) >> RESERVED_SHIFT;
}

/* duplicate a handle */
obj_handle_t duplicate_handle( struct process *src, obj_handle_t src_handle, struct process *dst,
                               unsigned int access, unsigned int attr, unsigned int options )
{
    obj_handle_t res;
    struct handle_entry *entry;
    unsigned int src_access;
    struct object *obj = get_handle_obj( src, src_handle, 0, NULL );

    if (!obj) return 0;
    if ((entry = get_handle( src, src_handle )))
        src_access = entry->access;
    else  /* pseudo-handle, give it full access */
    {
        src_access = obj->ops->map_access( obj, GENERIC_ALL );
        clear_error();
    }
    src_access &= ~RESERVED_ALL;

    if (options & DUP_HANDLE_SAME_ACCESS)
        access = src_access;
    else
        access = obj->ops->map_access( obj, access ) & ~RESERVED_ALL;

    /* asking for the more access rights than src_access? */
    if (access & ~src_access)
    {
        if (options & DUP_HANDLE_MAKE_GLOBAL)
            res = alloc_global_handle( obj, access );
        else
            res = alloc_handle( dst, obj, access, attr );
    }
    else
    {
        if (options & DUP_HANDLE_MAKE_GLOBAL)
            res = alloc_global_handle_no_access_check( obj, access );
        else
            res = alloc_handle_no_access_check( dst, obj, access, attr );
    }

    release_object( obj );
    return res;
}

/* open a new handle to an existing object */
obj_handle_t open_object( const struct namespace *namespace, const struct unicode_str *name,
                          const struct object_ops *ops, unsigned int access, unsigned int attr )
{
    obj_handle_t handle = 0;
    struct object *obj = find_object( namespace, name, attr );
    if (obj)
    {
        if (ops && obj->ops != ops)
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
        else
            handle = alloc_handle( current->process, obj, access, attr );
        release_object( obj );
    }
    else
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
    return handle;
}

/* return the size of the handle table of a given process */
unsigned int get_handle_table_count( struct process *process )
{
    if (!process->handles) return 0;
    return process->handles->count;
}

/* close a handle */
DECL_HANDLER(close_handle)
{
    close_handle( current->process, req->handle );
}

/* set a handle information */
DECL_HANDLER(set_handle_info)
{
    reply->old_flags = set_handle_flags( current->process, req->handle, req->mask, req->flags );
}

/* duplicate a handle */
DECL_HANDLER(dup_handle)
{
    struct process *src, *dst;

    reply->handle = 0;
    if ((src = get_process_from_handle( req->src_process, PROCESS_DUP_HANDLE )))
    {
        if (req->options & DUP_HANDLE_MAKE_GLOBAL)
        {
            reply->handle = duplicate_handle( src, req->src_handle, NULL,
                                              req->access, req->attributes, req->options );
        }
        else if ((dst = get_process_from_handle( req->dst_process, PROCESS_DUP_HANDLE )))
        {
            reply->handle = duplicate_handle( src, req->src_handle, dst,
                                              req->access, req->attributes, req->options );
            release_object( dst );
        }
        /* close the handle no matter what happened */
        if (req->options & DUP_HANDLE_CLOSE_SOURCE)
        {
            unsigned int err = get_error();  /* don't overwrite error from the above calls */
            reply->closed = close_handle( src, req->src_handle );
            set_error( err );
        }
        reply->self = (src == current->process);
        release_object( src );
    }
}

DECL_HANDLER(get_object_info)
{
    struct object *obj;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL ))) return;

    reply->access = get_handle_access( current->process, req->handle );
    reply->ref_count = obj->refcount;
    release_object( obj );
}

DECL_HANDLER(set_security_object)
{
    data_size_t sd_size = get_req_data_size();
    const struct security_descriptor *sd = get_req_data();
    struct object *obj;
    unsigned int access = 0;

    if (!sd_is_valid( sd, sd_size ))
    {
        set_error( STATUS_ACCESS_VIOLATION );
        return;
    }

    if (req->security_info & OWNER_SECURITY_INFORMATION ||
        req->security_info & GROUP_SECURITY_INFORMATION)
        access |= WRITE_OWNER;
    if (req->security_info & SACL_SECURITY_INFORMATION)
        access |= ACCESS_SYSTEM_SECURITY;
    if (req->security_info & DACL_SECURITY_INFORMATION)
        access |= WRITE_DAC;

    if (!(obj = get_handle_obj( current->process, req->handle, access, NULL ))) return;

    obj->ops->set_sd( obj, sd, req->security_info );
    release_object( obj );
}

DECL_HANDLER(get_security_object)
{
    const struct security_descriptor *sd;
    struct object *obj;
    unsigned int access = READ_CONTROL;
    struct security_descriptor req_sd;
    int present;
    const SID *owner, *group;
    const ACL *sacl, *dacl;

    if (req->security_info & SACL_SECURITY_INFORMATION)
        access |= ACCESS_SYSTEM_SECURITY;

    if (!(obj = get_handle_obj( current->process, req->handle, access, NULL ))) return;

    sd = obj->ops->get_sd( obj );
    if (sd)
    {
        req_sd.control = sd->control & ~SE_SELF_RELATIVE;

        owner = sd_get_owner( sd );
        if (req->security_info & OWNER_SECURITY_INFORMATION)
            req_sd.owner_len = sd->owner_len;
        else
            req_sd.owner_len = 0;

        group = sd_get_group( sd );
        if (req->security_info & GROUP_SECURITY_INFORMATION)
            req_sd.group_len = sd->group_len;
        else
            req_sd.group_len = 0;

        req_sd.control |= SE_SACL_PRESENT;
        sacl = sd_get_sacl( sd, &present );
        if (req->security_info & SACL_SECURITY_INFORMATION && present)
            req_sd.sacl_len = sd->sacl_len;
        else
            req_sd.sacl_len = 0;

        req_sd.control |= SE_DACL_PRESENT;
        dacl = sd_get_dacl( sd, &present );
        if (req->security_info & DACL_SECURITY_INFORMATION && present)
            req_sd.dacl_len = sd->dacl_len;
        else
            req_sd.dacl_len = 0;

        reply->sd_len = sizeof(req_sd) + req_sd.owner_len + req_sd.group_len +
            req_sd.sacl_len + req_sd.dacl_len;
        if (reply->sd_len <= get_reply_max_size())
        {
            char *ptr = set_reply_data_size(reply->sd_len);

            memcpy( ptr, &req_sd, sizeof(req_sd) );
            ptr += sizeof(req_sd);
            memcpy( ptr, owner, req_sd.owner_len );
            ptr += req_sd.owner_len;
            memcpy( ptr, group, req_sd.group_len );
            ptr += req_sd.group_len;
            memcpy( ptr, sacl, req_sd.sacl_len );
            ptr += req_sd.sacl_len;
            memcpy( ptr, dacl, req_sd.dacl_len );
        }
        else
            set_error(STATUS_BUFFER_TOO_SMALL);
    }

    release_object( obj );
}
