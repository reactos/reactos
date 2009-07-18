/*
 * Wine server USER definitions
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

#ifndef __WINE_SERVER_USER_H
#define __WINE_SERVER_USER_H

#include "wine/server_protocol.h"

struct thread;
struct region;
struct window;
struct msg_queue;
struct hook_table;
struct window_class;
struct atom_table;
struct clipboard;

enum user_object
{
    USER_WINDOW = 1,
    USER_HOOK
};

#define DESKTOP_ATOM  ((atom_t)32769)

struct winstation
{
    struct object      obj;                /* object header */
    unsigned int       flags;              /* winstation flags */
    struct list        entry;              /* entry in global winstation list */
    struct list        desktops;           /* list of desktops of this winstation */
    struct clipboard  *clipboard;          /* clipboard information */
    struct atom_table *atom_table;         /* global atom table */
};

struct desktop
{
    struct object        obj;            /* object header */
    unsigned int         flags;          /* desktop flags */
    struct winstation   *winstation;     /* winstation this desktop belongs to */
    struct list          entry;          /* entry in winstation list of desktops */
    struct window       *top_window;     /* desktop window for this desktop */
    struct window       *msg_window;     /* HWND_MESSAGE top window */
    struct hook_table   *global_hooks;   /* table of global hooks on this desktop */
    struct timeout_user *close_timeout;  /* timeout before closing the desktop */
    unsigned int         users;          /* processes and threads using this desktop */
};

/* user handles functions */

extern user_handle_t alloc_user_handle( void *ptr, enum user_object type );
extern void *get_user_object( user_handle_t handle, enum user_object type );
extern void *get_user_object_handle( user_handle_t *handle, enum user_object type );
extern user_handle_t get_user_full_handle( user_handle_t handle );
extern void *free_user_handle( user_handle_t handle );
extern void *next_user_handle( user_handle_t *handle, enum user_object type );

/* clipboard functions */

extern void cleanup_clipboard_thread( struct thread *thread );

/* hook functions */

extern void remove_thread_hooks( struct thread *thread );
extern unsigned int get_active_hooks(void);

/* queue functions */

extern void free_msg_queue( struct thread *thread );
extern struct hook_table *get_queue_hooks( struct thread *thread );
extern void set_queue_hooks( struct thread *thread, struct hook_table *hooks );
extern void inc_queue_paint_count( struct thread *thread, int incr );
extern void queue_cleanup_window( struct thread *thread, user_handle_t win );
extern int init_thread_queue( struct thread *thread );
extern int attach_thread_input( struct thread *thread_from, struct thread *thread_to );
extern void detach_thread_input( struct thread *thread_from );
extern void post_message( user_handle_t win, unsigned int message,
                          lparam_t wparam, lparam_t lparam );
extern void post_win_event( struct thread *thread, unsigned int event,
                            user_handle_t win, unsigned int object_id,
                            unsigned int child_id, client_ptr_t proc,
                            const WCHAR *module, data_size_t module_size,
                            user_handle_t handle );

/* region functions */

extern struct region *create_empty_region(void);
extern struct region *create_region_from_req_data( const void *data, data_size_t size );
extern void free_region( struct region *region );
extern void set_region_rect( struct region *region, const rectangle_t *rect );
extern rectangle_t *get_region_data( const struct region *region, data_size_t max_size,
                                     data_size_t *total_size );
extern rectangle_t *get_region_data_and_free( struct region *region, data_size_t max_size,
                                              data_size_t *total_size );
extern int is_region_empty( const struct region *region );
extern void get_region_extents( const struct region *region, rectangle_t *rect );
extern void offset_region( struct region *region, int x, int y );
extern struct region *copy_region( struct region *dst, const struct region *src );
extern struct region *intersect_region( struct region *dst, const struct region *src1,
                                        const struct region *src2 );
extern struct region *subtract_region( struct region *dst, const struct region *src1,
                                       const struct region *src2 );
extern struct region *union_region( struct region *dst, const struct region *src1,
                                    const struct region *src2 );
extern struct region *xor_region( struct region *dst, const struct region *src1,
                                  const struct region *src2 );
extern int point_in_region( struct region *region, int x, int y );
extern int rect_in_region( struct region *region, const rectangle_t *rect );

/* window functions */

extern struct process *get_top_window_owner( struct desktop *desktop );
extern void close_desktop_window( struct desktop *desktop );
extern void destroy_window( struct window *win );
extern void destroy_thread_windows( struct thread *thread );
extern int is_child_window( user_handle_t parent, user_handle_t child );
extern int is_top_level_window( user_handle_t window );
extern int is_window_visible( user_handle_t window );
extern int make_window_active( user_handle_t window );
extern struct thread *get_window_thread( user_handle_t handle );
extern user_handle_t window_from_point( struct desktop *desktop, int x, int y );
extern user_handle_t find_window_to_repaint( user_handle_t parent, struct thread *thread );
extern struct window_class *get_window_class( user_handle_t window );

/* window class functions */

extern void destroy_process_classes( struct process *process );
extern struct window_class *grab_class( struct process *process, atom_t atom,
                                        mod_handle_t instance, int *extra_bytes );
extern void release_class( struct window_class *class );
extern int is_desktop_class( struct window_class *class );
extern int is_hwnd_message_class( struct window_class *class );
extern atom_t get_class_atom( struct window_class *class );
extern client_ptr_t get_class_client_ptr( struct window_class *class );

/* windows station functions */

extern struct desktop *get_desktop_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern struct winstation *get_process_winstation( struct process *process, unsigned int access );
extern struct desktop *get_thread_desktop( struct thread *thread, unsigned int access );
extern void connect_process_winstation( struct process *process, struct thread *parent );
extern void set_process_default_desktop( struct process *process, struct desktop *desktop,
                                         obj_handle_t handle );
extern void close_process_desktop( struct process *process );
extern void close_thread_desktop( struct thread *thread );

#endif  /* __WINE_SERVER_USER_H */
