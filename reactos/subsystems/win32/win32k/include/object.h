/*
 * Wine server objects
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

#ifndef __WINE_SERVER_OBJECT_H
#define __WINE_SERVER_OBJECT_H

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#include <sys/time.h>
#include "wine/server_protocol.h"
#include "wine/list.h"

#define DEBUG_OBJECTS

/* kernel objects */

struct namespace;
struct object;
struct object_name;
struct thread;
struct token;
struct file;
struct wait_queue_entry;
struct async;
struct async_queue;
struct winstation;
struct directory;
struct object_type;


struct unicode_str
{
    const WCHAR *str;
    data_size_t  len;
};

/* operations valid on all objects */
struct object_ops
{
    /* size of this object type */
    size_t size;
    /* dump the object (for debugging) */
    void (*dump)(struct object *,int);
    /* return the object type */
    struct object_type *(*get_type)(struct object *);
    /* add a thread to the object wait queue */
    int  (*add_queue)(struct object *,struct wait_queue_entry *);
    /* remove a thread from the object wait queue */
    void (*remove_queue)(struct object *,struct wait_queue_entry *);
    /* is object signaled? */
    int  (*signaled)(struct object *,PTHREADINFO);
    /* wait satisfied; return 1 if abandoned */
    int  (*satisfied)(struct object *,PTHREADINFO);
    /* signal an object */
    int  (*signal)(struct object *, unsigned int);
    /* return an fd object that can be used to read/write from the object */
    struct fd *(*get_fd)(struct object *);
    /* map access rights to the specific rights for this object */
    unsigned int (*map_access)(struct object *, unsigned int);
    /* returns the security descriptor of the object */
    struct security_descriptor *(*get_sd)( struct object * );
    /* sets the security descriptor of the object */
    int (*set_sd)( struct object *, const struct security_descriptor *, unsigned int );
    /* lookup a name if an object has a namespace */
    struct object *(*lookup_name)(struct object *, struct unicode_str *,unsigned int);
    /* open a file object to access this object */
    struct object *(*open_file)(struct object *, unsigned int access, unsigned int sharing,
                                unsigned int options);
    /* close a handle to this object */
    int (*close_handle)(struct object *,PPROCESSINFO,obj_handle_t);
    /* destroy on refcount == 0 */
    void (*destroy)(struct object *);
};

struct object
{
    unsigned int              refcount;    /* reference count */
    const struct object_ops  *ops;
    struct list               wait_queue;
    struct object_name       *name;
    struct security_descriptor *sd;
#ifdef DEBUG_OBJECTS
    struct list               obj_list;
#endif
};

struct wait_queue_entry
{
    struct list     entry;
    struct object  *obj;
    PTHREADINFO     thread;
};

extern void *mem_alloc( size_t size );  /* malloc wrapper */
extern void *memdup( const void *data, size_t len );
extern void *alloc_object( const struct object_ops *ops );
extern const WCHAR *get_object_name( struct object *obj, data_size_t *len );
extern void dump_object_name( struct object *obj );
extern void *create_object( struct namespace *namespace, const struct object_ops *ops,
                            const struct unicode_str *name, struct object *parent );
extern void *create_named_object( struct namespace *namespace, const struct object_ops *ops,
                                  const struct unicode_str *name, unsigned int attributes );
extern void unlink_named_object( struct object *obj );
extern void make_object_static( struct object *obj );
extern struct namespace *create_namespace( unsigned int hash_size );
/* grab/release_object can take any pointer, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern struct object *grab_object( void *obj );
extern void release_object( void *obj );
extern struct object *find_object( const struct namespace *namespace, const struct unicode_str *name,
                                   unsigned int attributes );
extern struct object *find_object_index( const struct namespace *namespace, unsigned int index );
extern struct object_type *no_get_type( struct object *obj );
extern int no_add_queue( struct object *obj, struct wait_queue_entry *entry );
extern int no_satisfied( struct object *obj, struct thread *thread );
extern int no_signal( struct object *obj, unsigned int access );
extern struct fd *no_get_fd( struct object *obj );
extern unsigned int no_map_access( struct object *obj, unsigned int access );
extern struct security_descriptor *default_get_sd( struct object *obj );
extern int default_set_sd( struct object *obj, const struct security_descriptor *sd, unsigned int set_info );
extern struct object *no_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attributes );
extern struct object *no_open_file( struct object *obj, unsigned int access, unsigned int sharing,
                                    unsigned int options );
extern int no_close_handle( struct object *obj, PPROCESSINFO process, obj_handle_t handle );
extern void no_destroy( struct object *obj );
#ifdef DEBUG_OBJECTS
extern void dump_objects(void);
extern void close_objects(void);
#endif

/* event functions */

struct event;

extern struct event *create_event( struct directory *root, const struct unicode_str *name,
                                   unsigned int attr, int manual_reset, int initial_state,
                                   const struct security_descriptor *sd );
extern struct event *get_event_obj( PPROCESSINFO process, obj_handle_t handle, unsigned int access );
extern void pulse_event( struct event *event );
extern void set_event( struct event *event );
extern void reset_event( struct event *event );

/* mutex functions */

extern void abandon_mutexes( struct thread *thread );

/* serial functions */

int get_serial_async_timeout(struct object *obj, int type, int count);

/* socket functions */

extern void sock_init(void);

/* debugger functions */

extern int set_process_debugger( PPROCESSINFO process, struct thread *debugger );
extern void generate_debug_event( struct thread *thread, int code, const void *arg );
extern void generate_startup_debug_events( PPROCESSINFO process, client_ptr_t entry );
extern void debug_exit_thread( struct thread *thread );

/* mapping functions */

extern int get_page_size(void);

/* registry functions */

extern void init_registry(void);
extern void flush_registry(void);

/* signal functions */

extern void start_watchdog(void);
extern void stop_watchdog(void);
extern int watchdog_triggered(void);
extern void init_signals(void);

/* atom functions */

extern atom_t add_global_atom( struct winstation *winstation, const struct unicode_str *str );
extern atom_t find_global_atom( struct winstation *winstation, const struct unicode_str *str );
extern int grab_global_atom( struct winstation *winstation, atom_t atom );
extern void release_global_atom( struct winstation *winstation, atom_t atom );

/* directory functions */

extern struct directory *get_directory_obj( PPROCESSINFO process, obj_handle_t handle, unsigned int access );
extern struct object *find_object_dir( struct directory *root, const struct unicode_str *name,
                                       unsigned int attr, struct unicode_str *name_left );
extern void *create_named_object_dir( struct directory *root, const struct unicode_str *name,
                                      unsigned int attr, const struct object_ops *ops );
extern void *open_object_dir( struct directory *root, const struct unicode_str *name,
                              unsigned int attr, const struct object_ops *ops );
extern struct object_type *get_object_type( const struct unicode_str *name );
extern void init_directories(void);

/* symbolic link functions */

extern struct symlink *create_symlink( struct directory *root, const struct unicode_str *name,
                                       unsigned int attr, const struct unicode_str *target );

/* devices */
extern void create_named_pipe_device( struct directory *root, const struct unicode_str *name );
extern void create_mailslot_device( struct directory *root, const struct unicode_str *name );

/* global variables */

  /* command-line options */
extern int debug_level;
extern int foreground;
extern timeout_t master_socket_timeout;
extern const char *server_argv0;

  /* server start time used for GetTickCount() */
extern timeout_t server_start_time;

#endif  /* __WINE_SERVER_OBJECT_H */
