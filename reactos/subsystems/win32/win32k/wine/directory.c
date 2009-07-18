/*
 * Server-side directory object management
 *
 * Copyright (C) 2005 Vitaliy Margolen
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ddk/wdm.h"

#include "handle.h"
#include "request.h"
#include "process.h"
#include "file.h"
#include "unicode.h"

#define HASH_SIZE 7  /* default hash size */

struct object_type
{
    struct object     obj;        /* object header */
};

static void object_type_dump( struct object *obj, int verbose );
static struct object_type *object_type_get_type( struct object *obj );

static const struct object_ops object_type_ops =
{
    sizeof(struct object_type),   /* size */
    object_type_dump,             /* dump */
    object_type_get_type,         /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    no_map_access,                /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    no_destroy                    /* destroy */
};


struct directory
{
    struct object     obj;        /* object header */
    struct namespace *entries;    /* directory's name space */
};

static void directory_dump( struct object *obj, int verbose );
static struct object_type *directory_get_type( struct object *obj );
static struct object *directory_lookup_name( struct object *obj, struct unicode_str *name,
                                             unsigned int attr );
static void directory_destroy( struct object *obj );

static const struct object_ops directory_ops =
{
    sizeof(struct directory),     /* size */
    directory_dump,               /* dump */
    directory_get_type,           /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_fd_map_access,        /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    directory_lookup_name,        /* lookup_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    directory_destroy             /* destroy */
};

static struct directory *root_directory;
static struct directory *dir_objtype;


static void object_type_dump( struct object *obj, int verbose )
{
    assert( obj->ops == &object_type_ops );

    fputs( "Object type ", stderr );
    dump_object_name( obj );
    fputc( '\n', stderr );
}

static struct object_type *object_type_get_type( struct object *obj )
{
    static const WCHAR name[] = {'O','b','j','e','c','t','T','y','p','e'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static void directory_dump( struct object *obj, int verbose )
{
    assert( obj->ops == &directory_ops );

    fputs( "Directory ", stderr );
    dump_object_name( obj );
    fputc( '\n', stderr );
}

static struct object_type *directory_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','i','r','e','c','t','o','r','y'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static struct object *directory_lookup_name( struct object *obj, struct unicode_str *name,
                                             unsigned int attr )
{
    struct directory *dir = (struct directory *)obj;
    struct object *found;
    struct unicode_str tmp;
    const WCHAR *p;

    assert( obj->ops == &directory_ops );

    if (!(p = memchrW( name->str, '\\', name->len / sizeof(WCHAR) )))
        /* Last element in the path name */
        tmp.len = name->len;
    else
        tmp.len = (p - name->str) * sizeof(WCHAR);

    tmp.str = name->str;
    if ((found = find_object( dir->entries, &tmp, attr )))
    {
        /* Skip trailing \\ */
        if (p)
        {
            p++;
            tmp.len += sizeof(WCHAR);
        }
        /* Move to the next element*/
        name->str = p;
        name->len -= tmp.len;
        return found;
    }

    if (name->str)
    {
        if (tmp.len == 0) /* Double backslash */
            set_error( STATUS_OBJECT_NAME_INVALID );
        else if (p)  /* Path still has backslashes */
            set_error( STATUS_OBJECT_PATH_NOT_FOUND );
        else
            clear_error();
    }
    return NULL;
}

static void directory_destroy( struct object *obj )
{
    struct directory *dir = (struct directory *)obj;
    assert( obj->ops == &directory_ops );
    free( dir->entries );
}

static struct directory *create_directory( struct directory *root, const struct unicode_str *name,
                                           unsigned int attr, unsigned int hash_size )
{
    struct directory *dir;

    if ((dir = create_named_object_dir( root, name, attr, &directory_ops )) &&
        get_error() != STATUS_OBJECT_NAME_EXISTS)
    {
        if (!(dir->entries = create_namespace( hash_size )))
        {
            release_object( dir );
            dir = NULL;
        }
    }
    return dir;
}

struct directory *get_directory_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct directory *)get_handle_obj( process, handle, access, &directory_ops );
}

/******************************************************************************
 * Find an object by its name in a given root object
 *
 * PARAMS
 *  root      [I] directory to start search from or NULL to start from \\
 *  name      [I] object name to search for
 *  attr      [I] OBJECT_ATTRIBUTES.Attributes
 *  name_left [O] [optional] leftover name if object is not found
 *
 * RETURNS
 *  NULL:      If params are invalid
 *  Found:     If object with exact name is found returns that object
 *             (name_left->len == 0). Object's refcount is incremented
 *  Not found: The last matched parent. (name_left->len > 0)
 *             Parent's refcount is incremented.
 */
struct object *find_object_dir( struct directory *root, const struct unicode_str *name,
                                unsigned int attr, struct unicode_str *name_left )
{
    struct object *obj, *parent;
    struct unicode_str name_tmp;

    if (name) name_tmp = *name;
    else name_tmp.len = 0;

    /* Arguments check:
     * - Either rootdir or name have to be specified
     * - If root is specified path shouldn't start with backslash */
    if (root)
    {
        if (name_tmp.len && name_tmp.str[0] == '\\')
        {
            set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
            return NULL;
        }
        parent = grab_object( root );
    }
    else
    {
        if (!name_tmp.len || name_tmp.str[0] != '\\')
        {
            set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
            return NULL;
        }
        parent = grab_object( &root_directory->obj );
        /* skip leading backslash */
        name_tmp.str++;
        name_tmp.len -= sizeof(WCHAR);
    }

    /* Special case for opening RootDirectory */
    if (!name_tmp.len) goto done;

    while ((obj = parent->ops->lookup_name( parent, &name_tmp, attr )))
    {
        /* move to the next element */
        release_object ( parent );
        parent = obj;
    }
    if (get_error())
    {
        release_object( parent );
        return NULL;
    }

    done:
    if (name_left) *name_left = name_tmp;
    return parent;
}

/* create a named (if name is present) or unnamed object. */
void *create_named_object_dir( struct directory *root, const struct unicode_str *name,
                               unsigned int attributes, const struct object_ops *ops )
{
    struct object *obj, *new_obj = NULL;
    struct unicode_str new_name;

    if (!name || !name->len) return alloc_object( ops );

    if (!(obj = find_object_dir( root, name, attributes, &new_name ))) return NULL;
    if (!new_name.len)
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

    /* ATM we can't insert objects into anything else but directories */
    if (obj->ops != &directory_ops)
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
    else
    {
        struct directory *dir = (struct directory *)obj;
        if ((new_obj = create_object( dir->entries, ops, &new_name, &dir->obj )))
            clear_error();
    }

    release_object( obj );
    return new_obj;
}

/* open a new handle to an existing object */
void *open_object_dir( struct directory *root, const struct unicode_str *name,
                       unsigned int attr, const struct object_ops *ops )
{
    struct unicode_str name_left;
    struct object *obj;

    if ((obj = find_object_dir( root, name, attr, &name_left )))
    {
        if (name_left.len) /* not fully parsed */
            set_error( STATUS_OBJECT_NAME_NOT_FOUND );
        else if (ops && obj->ops != ops)
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
        else
            return obj;

        release_object( obj );
    }
    return NULL;
}

/* retrieve an object type, creating it if needed */
struct object_type *get_object_type( const struct unicode_str *name )
{
    struct object_type *type;

    if ((type = open_object_dir( dir_objtype, name, 0, &object_type_ops )))
        return type;

    if ((type = create_named_object_dir( dir_objtype, name, 0, &object_type_ops )))
    {
        grab_object( type );
        make_object_static( &type->obj );
        clear_error();
    }
    return type;
}

/* Global initialization */

void init_directories(void)
{
    /* Directories */
    static const WCHAR dir_globalW[] = {'\\','?','?'};
    static const WCHAR dir_driverW[] = {'D','r','i','v','e','r'};
    static const WCHAR dir_deviceW[] = {'D','e','v','i','c','e'};
    static const WCHAR dir_basenamedW[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s'};
    static const WCHAR dir_named_pipeW[] = {'\\','D','e','v','i','c','e','\\','N','a','m','e','d','P','i','p','e'};
    static const WCHAR dir_mailslotW[] = {'\\','D','e','v','i','c','e','\\','M','a','i','l','S','l','o','t'};
    static const WCHAR dir_objtypeW[] = {'O','b','j','e','c','t','T','y','p','e','s',};
    static const struct unicode_str dir_global_str = {dir_globalW, sizeof(dir_globalW)};
    static const struct unicode_str dir_driver_str = {dir_driverW, sizeof(dir_driverW)};
    static const struct unicode_str dir_device_str = {dir_deviceW, sizeof(dir_deviceW)};
    static const struct unicode_str dir_basenamed_str = {dir_basenamedW, sizeof(dir_basenamedW)};
    static const struct unicode_str dir_named_pipe_str = {dir_named_pipeW, sizeof(dir_named_pipeW)};
    static const struct unicode_str dir_mailslot_str = {dir_mailslotW, sizeof(dir_mailslotW)};
    static const struct unicode_str dir_objtype_str = {dir_objtypeW, sizeof(dir_objtypeW)};

    /* symlinks */
    static const WCHAR link_dosdevW[] = {'D','o','s','D','e','v','i','c','e','s'};
    static const WCHAR link_globalW[] = {'G','l','o','b','a','l'};
    static const WCHAR link_localW[]  = {'L','o','c','a','l'};
    static const WCHAR link_pipeW[]   = {'P','I','P','E'};
    static const WCHAR link_mailslotW[] = {'M','A','I','L','S','L','O','T'};
    static const struct unicode_str link_dosdev_str = {link_dosdevW, sizeof(link_dosdevW)};
    static const struct unicode_str link_global_str = {link_globalW, sizeof(link_globalW)};
    static const struct unicode_str link_local_str  = {link_localW, sizeof(link_localW)};
    static const struct unicode_str link_pipe_str   = {link_pipeW, sizeof(link_pipeW)};
    static const struct unicode_str link_mailslot_str = {link_mailslotW, sizeof(link_mailslotW)};

    /* devices */
    static const WCHAR named_pipeW[] = {'N','a','m','e','d','P','i','p','e'};
    static const WCHAR mailslotW[] = {'M','a','i','l','S','l','o','t'};
    static const struct unicode_str named_pipe_str = {named_pipeW, sizeof(named_pipeW)};
    static const struct unicode_str mailslot_str = {mailslotW, sizeof(mailslotW)};

    struct directory *dir_driver, *dir_device, *dir_global, *dir_basenamed;
    struct symlink *link_dosdev, *link_global1, *link_global2, *link_local, *link_pipe, *link_mailslot;

    root_directory = create_directory( NULL, NULL, 0, HASH_SIZE );
    dir_driver     = create_directory( root_directory, &dir_driver_str, 0, HASH_SIZE );
    dir_device     = create_directory( root_directory, &dir_device_str, 0, HASH_SIZE );
    dir_objtype    = create_directory( root_directory, &dir_objtype_str, 0, HASH_SIZE );
    make_object_static( &root_directory->obj );
    make_object_static( &dir_driver->obj );
    make_object_static( &dir_objtype->obj );

    dir_global     = create_directory( NULL, &dir_global_str, 0, HASH_SIZE );
    /* use a larger hash table for this one since it can contain a lot of objects */
    dir_basenamed  = create_directory( NULL, &dir_basenamed_str, 0, 37 );

    /* devices */
    create_named_pipe_device( dir_device, &named_pipe_str );
    create_mailslot_device( dir_device, &mailslot_str );

    /* symlinks */
    link_dosdev    = create_symlink( root_directory, &link_dosdev_str, 0, &dir_global_str );
    link_global1   = create_symlink( dir_global, &link_global_str, 0, &dir_global_str );
    link_global2   = create_symlink( dir_basenamed, &link_global_str, 0, &dir_basenamed_str );
    link_local     = create_symlink( dir_basenamed, &link_local_str, 0, &dir_basenamed_str );
    link_pipe      = create_symlink( dir_global, &link_pipe_str, 0, &dir_named_pipe_str );
    link_mailslot  = create_symlink( dir_global, &link_mailslot_str, 0, &dir_mailslot_str );
    make_object_static( (struct object *)link_dosdev );
    make_object_static( (struct object *)link_global1 );
    make_object_static( (struct object *)link_global2 );
    make_object_static( (struct object *)link_local );
    make_object_static( (struct object *)link_pipe );
    make_object_static( (struct object *)link_mailslot );

    /* the symlinks or devices hold references so we can release these */
    release_object( dir_global );
    release_object( dir_device );
    release_object( dir_basenamed );
}

/* create a directory object */
DECL_HANDLER(create_directory)
{
    struct unicode_str name;
    struct directory *dir, *root = NULL;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((dir = create_directory( root, &name, req->attributes, HASH_SIZE )))
    {
        reply->handle = alloc_handle( current->process, dir, req->access, req->attributes );
        release_object( dir );
    }

    if (root) release_object( root );
}

/* open a directory object */
DECL_HANDLER(open_directory)
{
    struct unicode_str name;
    struct directory *dir, *root = NULL;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((dir = open_object_dir( root, &name, req->attributes, &directory_ops )))
    {
        reply->handle = alloc_handle( current->process, &dir->obj, req->access, req->attributes );
        release_object( dir );
    }

    if (root) release_object( root );
}

/* get a directory entry by index */
DECL_HANDLER(get_directory_entry)
{
    struct directory *dir = get_directory_obj( current->process, req->handle, DIRECTORY_QUERY );
    if (dir)
    {
        struct object *obj = find_object_index( dir->entries, req->index );
        if (obj)
        {
            data_size_t name_len, type_len = 0;
            const WCHAR *type_name = NULL;
            const WCHAR *name = get_object_name( obj, &name_len );
            struct object_type *type = obj->ops->get_type( obj );

            if (type) type_name = get_object_name( &type->obj, &type_len );

            if (name_len + type_len <= get_reply_max_size())
            {
                void *ptr = set_reply_data_size( name_len + type_len );
                if (ptr)
                {
                    reply->name_len = name_len;
                    memcpy( ptr, name, name_len );
                    memcpy( (char *)ptr + name_len, type_name, type_len );
                }
            }
            else set_error( STATUS_BUFFER_OVERFLOW );

            if (type) release_object( type );
            release_object( obj );
        }
        release_object( dir );
    }
}

/* unlink a named object */
DECL_HANDLER(unlink_object)
{
    struct object *obj = get_handle_obj( current->process, req->handle, 0, NULL );

    if (obj)
    {
        unlink_named_object( obj );
        release_object( obj );
    }
}
