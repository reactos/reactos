/*
 * Server-side objects
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "process.h"
#include "thread.h"
#include "unicode.h"
#include "security.h"


struct object_name
{
    struct list         entry;           /* entry in the hash list */
    struct object      *obj;             /* object owning this name */
    struct object      *parent;          /* parent object */
    data_size_t         len;             /* name length in bytes */
    WCHAR               name[1];
};

struct namespace
{
    unsigned int        hash_size;       /* size of hash table */
    struct list         names[1];        /* array of hash entry lists */
};


#ifdef DEBUG_OBJECTS
static struct list object_list = LIST_INIT(object_list);
static struct list static_object_list = LIST_INIT(static_object_list);

void dump_objects(void)
{
    struct list *p;

    LIST_FOR_EACH( p, &static_object_list )
    {
        struct object *ptr = LIST_ENTRY( p, struct object, obj_list );
        fprintf( stderr, "%p:%d: ", ptr, ptr->refcount );
        ptr->ops->dump( ptr, 1 );
    }
    LIST_FOR_EACH( p, &object_list )
    {
        struct object *ptr = LIST_ENTRY( p, struct object, obj_list );
        fprintf( stderr, "%p:%d: ", ptr, ptr->refcount );
        ptr->ops->dump( ptr, 1 );
    }
}

void close_objects(void)
{
    struct list *ptr;

    /* release the static objects */
    while ((ptr = list_head( &static_object_list )))
    {
        struct object *obj = LIST_ENTRY( ptr, struct object, obj_list );
        /* move it back to the standard list before freeing */
        list_remove( &obj->obj_list );
        list_add_head( &object_list, &obj->obj_list );
        release_object( obj );
    }

    dump_objects();  /* dump any remaining objects */
}

#endif  /* DEBUG_OBJECTS */

/*****************************************************************/

/* malloc replacement */
void *mem_alloc( size_t size )
{
    void *ptr = malloc( size );
    if (ptr) memset( ptr, 0x55, size );
    else set_error( STATUS_NO_MEMORY );
    return ptr;
}

/* duplicate a block of memory */
void *memdup( const void *data, size_t len )
{
    void *ptr = malloc( len );
    if (ptr) memcpy( ptr, data, len );
    else set_error( STATUS_NO_MEMORY );
    return ptr;
}


/*****************************************************************/

static int get_name_hash( const struct namespace *namespace, const WCHAR *name, data_size_t len )
{
    WCHAR hash = 0;
    len /= sizeof(WCHAR);
    while (len--) hash ^= tolowerW(*name++);
    return hash % namespace->hash_size;
}

/* allocate a name for an object */
static struct object_name *alloc_name( const struct unicode_str *name )
{
    struct object_name *ptr;

    if ((ptr = mem_alloc( sizeof(*ptr) + name->len - sizeof(ptr->name) )))
    {
        ptr->len = name->len;
        ptr->parent = NULL;
        memcpy( ptr->name, name->str, name->len );
    }
    return ptr;
}

/* free the name of an object */
static void free_name( struct object *obj )
{
    struct object_name *ptr = obj->name;
    list_remove( &ptr->entry );
    if (ptr->parent) release_object( ptr->parent );
    free( ptr );
}

/* set the name of an existing object */
static void set_object_name( struct namespace *namespace,
                             struct object *obj, struct object_name *ptr )
{
    int hash = get_name_hash( namespace, ptr->name, ptr->len );

    list_add_head( &namespace->names[hash], &ptr->entry );
    ptr->obj = obj;
    obj->name = ptr;
}

/* get the name of an existing object */
const WCHAR *get_object_name( struct object *obj, data_size_t *len )
{
    struct object_name *ptr = obj->name;
    if (!ptr) return NULL;
    *len = ptr->len;
    return ptr->name;
}

/* allocate and initialize an object */
void *alloc_object( const struct object_ops *ops )
{
    struct object *obj = mem_alloc( ops->size );
    if (obj)
    {
        obj->refcount = 1;
        obj->ops      = ops;
        obj->name     = NULL;
        obj->sd       = NULL;
        list_init( &obj->wait_queue );
#ifdef DEBUG_OBJECTS
        list_add_head( &object_list, &obj->obj_list );
#endif
        return obj;
    }
    return NULL;
}

void *create_object( struct namespace *namespace, const struct object_ops *ops,
                     const struct unicode_str *name, struct object *parent )
{
    struct object *obj;
    struct object_name *name_ptr;

    if (!(name_ptr = alloc_name( name ))) return NULL;
    if ((obj = alloc_object( ops )))
    {
        set_object_name( namespace, obj, name_ptr );
        if (parent) name_ptr->parent = grab_object( parent );
    }
    else
        free( name_ptr );
    return obj;
}

void *create_named_object( struct namespace *namespace, const struct object_ops *ops,
                           const struct unicode_str *name, unsigned int attributes )
{
    struct object *obj;

    if (!name || !name->len) return alloc_object( ops );

    if ((obj = find_object( namespace, name, attributes )))
    {
        if (attributes & OBJ_OPENIF && obj->ops == ops)
            set_error( STATUS_OBJECT_NAME_EXISTS );
        else
        {
            release_object( obj );
            obj = NULL;
            if (attributes & OBJ_OPENIF)
                set_error( STATUS_OBJECT_TYPE_MISMATCH );
            else
                set_error( STATUS_OBJECT_NAME_COLLISION );
        }
        return obj;
    }
    if ((obj = create_object( namespace, ops, name, NULL ))) clear_error();
    return obj;
}

/* dump the name of an object to stderr */
void dump_object_name( struct object *obj )
{
    if (!obj->name) fprintf( stderr, "name=\"\"" );
    else
    {
        fprintf( stderr, "name=L\"" );
        dump_strW( obj->name->name, obj->name->len/sizeof(WCHAR), stderr, "\"\"" );
        fputc( '\"', stderr );
    }
}

/* unlink a named object from its namespace, without freeing the object itself */
void unlink_named_object( struct object *obj )
{
    if (obj->name) free_name( obj );
    obj->name = NULL;
}

/* mark an object as being stored statically, i.e. only released at shutdown */
void make_object_static( struct object *obj )
{
#ifdef DEBUG_OBJECTS
    list_remove( &obj->obj_list );
    list_add_head( &static_object_list, &obj->obj_list );
#endif
}

/* grab an object (i.e. increment its refcount) and return the object */
struct object *grab_object( void *ptr )
{
    struct object *obj = (struct object *)ptr;
    assert( obj->refcount < INT_MAX );
    obj->refcount++;
    return obj;
}

/* release an object (i.e. decrement its refcount) */
void release_object( void *ptr )
{
    struct object *obj = (struct object *)ptr;
    assert( obj->refcount );
    if (!--obj->refcount)
    {
        /* if the refcount is 0, nobody can be in the wait queue */
        assert( list_empty( &obj->wait_queue ));
        obj->ops->destroy( obj );
        if (obj->name) free_name( obj );
        free( obj->sd );
#ifdef DEBUG_OBJECTS
        list_remove( &obj->obj_list );
        memset( obj, 0xaa, obj->ops->size );
#endif
        free( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const struct namespace *namespace, const struct unicode_str *name,
                            unsigned int attributes )
{
    const struct list *list;
    struct list *p;

    if (!name || !name->len) return NULL;

    list = &namespace->names[ get_name_hash( namespace, name->str, name->len ) ];
    LIST_FOR_EACH( p, list )
    {
        const struct object_name *ptr = LIST_ENTRY( p, struct object_name, entry );
        if (ptr->len != name->len) continue;
        if (attributes & OBJ_CASE_INSENSITIVE)
        {
            if (!strncmpiW( ptr->name, name->str, name->len/sizeof(WCHAR) ))
                return grab_object( ptr->obj );
        }
        else
        {
            if (!memcmp( ptr->name, name->str, name->len ))
                return grab_object( ptr->obj );
        }
    }
    return NULL;
}

/* find an object by its index; the refcount is incremented */
struct object *find_object_index( const struct namespace *namespace, unsigned int index )
{
    unsigned int i;

    /* FIXME: not efficient at all */
    for (i = 0; i < namespace->hash_size; i++)
    {
        const struct object_name *ptr;
        LIST_FOR_EACH_ENTRY( ptr, &namespace->names[i], const struct object_name, entry )
        {
            if (!index--) return grab_object( ptr->obj );
        }
    }
    set_error( STATUS_NO_MORE_ENTRIES );
    return NULL;
}

/* allocate a namespace */
struct namespace *create_namespace( unsigned int hash_size )
{
    struct namespace *namespace;
    unsigned int i;

    namespace = mem_alloc( sizeof(*namespace) + (hash_size - 1) * sizeof(namespace->names[0]) );
    if (namespace)
    {
        namespace->hash_size      = hash_size;
        for (i = 0; i < hash_size; i++) list_init( &namespace->names[i] );
    }
    return namespace;
}

/* functions for unimplemented/default object operations */

struct object_type *no_get_type( struct object *obj )
{
    return NULL;
}

int no_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

int no_satisfied( struct object *obj, struct thread *thread )
{
    return 0;  /* not abandoned */
}

int no_signal( struct object *obj, unsigned int access )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

struct fd *no_get_fd( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return NULL;
}

unsigned int no_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

struct security_descriptor *default_get_sd( struct object *obj )
{
    return obj->sd;
}

int default_set_sd( struct object *obj, const struct security_descriptor *sd,
                    unsigned int set_info )
{
    struct security_descriptor new_sd, *new_sd_ptr;
    int present;
    const SID *owner, *group;
    const ACL *sacl, *dacl;
    char *ptr;

    if (!set_info) return 1;

    new_sd.control = sd->control & ~SE_SELF_RELATIVE;

    owner = sd_get_owner( sd );
    if (set_info & OWNER_SECURITY_INFORMATION && owner)
        new_sd.owner_len = sd->owner_len;
    else
    {
        owner = token_get_user( current->process->token );
        new_sd.owner_len = FIELD_OFFSET(SID, SubAuthority[owner->SubAuthorityCount]);
        new_sd.control |= SE_OWNER_DEFAULTED;
    }

    group = sd_get_group( sd );
    if (set_info & GROUP_SECURITY_INFORMATION && group)
        new_sd.group_len = sd->group_len;
    else
    {
        group = token_get_primary_group( current->process->token );
        new_sd.group_len = FIELD_OFFSET(SID, SubAuthority[group->SubAuthorityCount]);
        new_sd.control |= SE_GROUP_DEFAULTED;
    }

    new_sd.control |= SE_SACL_PRESENT;
    sacl = sd_get_sacl( sd, &present );
    if (set_info & SACL_SECURITY_INFORMATION && present)
        new_sd.sacl_len = sd->sacl_len;
    else
    {
        if (obj->sd) sacl = sd_get_sacl( obj->sd, &present );

        if (obj->sd && present)
            new_sd.sacl_len = obj->sd->sacl_len;
        else
        {
            new_sd.sacl_len = 0;
            new_sd.control |= SE_SACL_DEFAULTED;
        }
    }

    new_sd.control |= SE_DACL_PRESENT;
    dacl = sd_get_dacl( sd, &present );
    if (set_info & DACL_SECURITY_INFORMATION && present)
        new_sd.dacl_len = sd->dacl_len;
    else
    {
        if (obj->sd) dacl = sd_get_dacl( obj->sd, &present );

        if (obj->sd && present)
            new_sd.dacl_len = obj->sd->dacl_len;
        else
        {
            dacl = token_get_default_dacl( current->process->token );
            new_sd.dacl_len = dacl->AclSize;
            new_sd.control |= SE_DACL_DEFAULTED;
        }
    }

    ptr = mem_alloc( sizeof(new_sd) + new_sd.owner_len + new_sd.group_len +
                     new_sd.sacl_len + new_sd.dacl_len );
    if (!ptr) return 0;
    new_sd_ptr = (struct security_descriptor*)ptr;

    memcpy( ptr, &new_sd, sizeof(new_sd) );
    ptr += sizeof(new_sd);
    memcpy( ptr, owner, new_sd.owner_len );
    ptr += new_sd.owner_len;
    memcpy( ptr, group, new_sd.group_len );
    ptr += new_sd.group_len;
    memcpy( ptr, sacl, new_sd.sacl_len );
    ptr += new_sd.sacl_len;
    memcpy( ptr, dacl, new_sd.dacl_len );

    free( obj->sd );
    obj->sd = new_sd_ptr;
    return 1;
}

struct object *no_lookup_name( struct object *obj, struct unicode_str *name,
                               unsigned int attr )
{
    return NULL;
}

struct object *no_open_file( struct object *obj, unsigned int access, unsigned int sharing,
                             unsigned int options )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return NULL;
}

int no_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    return 1;  /* ok to close */
}

void no_destroy( struct object *obj )
{
}
