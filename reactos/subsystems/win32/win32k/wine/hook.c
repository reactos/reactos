/*
 * Server-side window hooks support
 *
 * Copyright (C) 2002 Alexandre Julliard
 * Copyright (C) 2005 Dmitry Timoshkov
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
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winternl.h"

#include "object.h"
#include "process.h"
#include "request.h"
#include "user.h"

struct hook_table;

struct hook
{
    struct list         chain;    /* hook chain entry */
    user_handle_t       handle;   /* user handle for this hook */
    struct process     *process;  /* process the hook is set to */
    struct thread      *thread;   /* thread the hook is set to */
    struct thread      *owner;    /* owner of the out of context hook */
    struct hook_table  *table;    /* hook table that contains this hook */
    int                 index;    /* hook table index */
    int                 event_min;
    int                 event_max;
    int                 flags;
    client_ptr_t        proc;     /* hook function */
    int                 unicode;  /* is it a unicode hook? */
    WCHAR              *module;   /* module name for global hooks */
    data_size_t         module_size;
};

#define WH_WINEVENT (WH_MAXHOOK+1)

#define NB_HOOKS (WH_WINEVENT-WH_MINHOOK+1)
#define HOOK_ENTRY(p)  LIST_ENTRY( (p), struct hook, chain )

struct hook_table
{
    struct object obj;              /* object header */
    struct list   hooks[NB_HOOKS];  /* array of hook chains */
    int           counts[NB_HOOKS]; /* use counts for each hook chain */
};

static void hook_table_dump( struct object *obj, int verbose );
static void hook_table_destroy( struct object *obj );

static const struct object_ops hook_table_ops =
{
    sizeof(struct hook_table),    /* size */
    hook_table_dump,              /* dump */
    no_get_type,                  /* get_type */
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
    hook_table_destroy            /* destroy */
};


/* create a new hook table */
static struct hook_table *alloc_hook_table(void)
{
    struct hook_table *table;
    int i;

    if ((table = alloc_object( &hook_table_ops )))
    {
        for (i = 0; i < NB_HOOKS; i++)
        {
            list_init( &table->hooks[i] );
            table->counts[i] = 0;
        }
    }
    return table;
}

static struct hook_table *get_global_hooks( struct thread *thread )
{
    struct hook_table *table;
    struct desktop *desktop = get_thread_desktop( thread, 0 );

    if (!desktop) return NULL;
    table = desktop->global_hooks;
    release_object( desktop );
    return table;
}

/* create a new hook and add it to the specified table */
static struct hook *add_hook( struct desktop *desktop, struct thread *thread, int index, int global )
{
    struct hook *hook;
    struct hook_table *table = global ? desktop->global_hooks : get_queue_hooks(thread);

    if (!table)
    {
        if (!(table = alloc_hook_table())) return NULL;
        if (global) desktop->global_hooks = table;
        else set_queue_hooks( thread, table );
    }
    if (!(hook = mem_alloc( sizeof(*hook) ))) return NULL;

    if (!(hook->handle = alloc_user_handle( hook, USER_HOOK )))
    {
        free( hook );
        return NULL;
    }
    hook->thread = thread ? (struct thread *)grab_object( thread ) : NULL;
    hook->table  = table;
    hook->index  = index;
    list_add_head( &table->hooks[index], &hook->chain );
    if (thread) thread->desktop_users++;
    return hook;
}

/* free a hook, removing it from its chain */
static void free_hook( struct hook *hook )
{
    free_user_handle( hook->handle );
    free( hook->module );
    if (hook->thread)
    {
        assert( hook->thread->desktop_users > 0 );
        hook->thread->desktop_users--;
        release_object( hook->thread );
    }
    if (hook->process) release_object( hook->process );
    release_object( hook->owner );
    list_remove( &hook->chain );
    free( hook );
}

/* find a hook from its index and proc */
static struct hook *find_hook( struct thread *thread, int index, client_ptr_t proc )
{
    struct list *p;
    struct hook_table *table = get_queue_hooks( thread );

    if (table)
    {
        LIST_FOR_EACH( p, &table->hooks[index] )
        {
            struct hook *hook = HOOK_ENTRY( p );
            if (hook->proc == proc) return hook;
        }
    }
    return NULL;
}

/* get the first hook in the chain */
static inline struct hook *get_first_hook( struct hook_table *table, int index )
{
    struct list *elem = list_head( &table->hooks[index] );
    return elem ? HOOK_ENTRY( elem ) : NULL;
}

/* check if a given hook should run in the current thread */
static inline int run_hook_in_current_thread( struct hook *hook )
{
    if ((!hook->process || hook->process == current->process) &&
        (!(hook->flags & WINEVENT_SKIPOWNPROCESS) || hook->process != current->process))
    {
        if ((!hook->thread || hook->thread == current) &&
            (!(hook->flags & WINEVENT_SKIPOWNTHREAD) || hook->thread != current))
            return 1;
    }
    return 0;
}

/* check if a given hook should run in the owner thread instead of the current thread */
static inline int run_hook_in_owner_thread( struct hook *hook )
{
    if ((hook->index == WH_MOUSE_LL - WH_MINHOOK ||
         hook->index == WH_KEYBOARD_LL - WH_MINHOOK))
        return hook->owner != current;
    return 0;
}

/* find the first non-deleted hook in the chain */
static inline struct hook *get_first_valid_hook( struct hook_table *table, int index,
                                                 int event, user_handle_t win,
                                                 int object_id, int child_id )
{
    struct hook *hook = get_first_hook( table, index );

    while (hook)
    {
        if (hook->proc && run_hook_in_current_thread( hook ))
        {
            if (event >= hook->event_min && event <= hook->event_max)
            {
                if (hook->flags & WINEVENT_INCONTEXT) return hook;

                /* only winevent hooks may be out of context */
                assert(hook->index + WH_MINHOOK == WH_WINEVENT);
                post_win_event( hook->owner, event, win, object_id, child_id,
                                hook->proc, hook->module, hook->module_size,
                                hook->handle );
            }
        }
        hook = HOOK_ENTRY( list_next( &table->hooks[index], &hook->chain ) );
    }
    return hook;
}

/* find the next hook in the chain, skipping the deleted ones */
static struct hook *get_next_hook( struct thread *thread, struct hook *hook, int event,
                                   user_handle_t win, int object_id, int child_id )
{
    struct hook_table *global_hooks, *table = hook->table;
    int index = hook->index;

    while ((hook = HOOK_ENTRY( list_next( &table->hooks[index], &hook->chain ) )))
    {
        if (hook->proc && run_hook_in_current_thread( hook ))
        {
            if (event >= hook->event_min && event <= hook->event_max)
            {
                if (hook->flags & WINEVENT_INCONTEXT) return hook;

                /* only winevent hooks may be out of context */
                assert(hook->index + WH_MINHOOK == WH_WINEVENT);
                post_win_event( hook->owner, event, win, object_id, child_id,
                                hook->proc, hook->module, hook->module_size,
                                hook->handle );
            }
        }
    }
    global_hooks = get_global_hooks( thread );
    if (global_hooks && table != global_hooks)  /* now search through the global table */
    {
        hook = get_first_valid_hook( global_hooks, index, event, win, object_id, child_id );
    }
    return hook;
}

static void hook_table_dump( struct object *obj, int verbose )
{
    /* struct hook_table *table = (struct hook_table *)obj; */
    fprintf( stderr, "Hook table\n" );
}

static void hook_table_destroy( struct object *obj )
{
    int i;
    struct hook *hook;
    struct hook_table *table = (struct hook_table *)obj;

    for (i = 0; i < NB_HOOKS; i++)
    {
        while ((hook = get_first_hook( table, i )) != NULL) free_hook( hook );
    }
}

/* remove a hook, freeing it if the chain is not in use */
static void remove_hook( struct hook *hook )
{
    if (hook->table->counts[hook->index])
        hook->proc = 0; /* chain is in use, just mark it and return */
    else
        free_hook( hook );
}

/* release a hook chain, removing deleted hooks if the use count drops to 0 */
static void release_hook_chain( struct hook_table *table, int index )
{
    if (!table->counts[index])  /* use count shouldn't already be 0 */
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!--table->counts[index])
    {
        struct hook *hook = get_first_hook( table, index );
        while (hook)
        {
            struct hook *next = HOOK_ENTRY( list_next( &table->hooks[hook->index], &hook->chain ) );
            if (!hook->proc) free_hook( hook );
            hook = next;
        }
    }
}

/* remove all global hooks owned by a given thread */
void remove_thread_hooks( struct thread *thread )
{
    struct hook_table *global_hooks = get_global_hooks( thread );
    int index;

    if (!global_hooks) return;

    /* only low-level keyboard/mouse global hooks can be owned by a thread */
    for (index = WH_KEYBOARD_LL - WH_MINHOOK; index <= WH_MOUSE_LL - WH_MINHOOK; index++)
    {
        struct hook *hook = get_first_hook( global_hooks, index );
        while (hook)
        {
            struct hook *next = HOOK_ENTRY( list_next( &global_hooks->hooks[index], &hook->chain ) );
            if (hook->thread == thread) remove_hook( hook );
            hook = next;
        }
    }
}

/* get a bitmap of active hooks in a hook table */
static int is_hook_active( struct hook_table *table, int index )
{
    struct hook *hook = get_first_hook( table, index );

    while (hook)
    {
        if (hook->proc && run_hook_in_current_thread( hook )) return 1;
        hook = HOOK_ENTRY( list_next( &table->hooks[index], &hook->chain ) );
    }
    return 0;
}

/* get a bitmap of all active hooks for the current thread */
unsigned int get_active_hooks(void)
{
    struct hook_table *table = get_queue_hooks( current );
    struct hook_table *global_hooks = get_global_hooks( current );
    unsigned int ret = 1 << 31;  /* set high bit to indicate that the bitmap is valid */
    int id;

    for (id = WH_MINHOOK; id <= WH_WINEVENT; id++)
    {
        if ((table && is_hook_active( table, id - WH_MINHOOK )) ||
            (global_hooks && is_hook_active( global_hooks, id - WH_MINHOOK )))
            ret |= 1 << (id - WH_MINHOOK);
    }
    return ret;
}

/* set a window hook */
DECL_HANDLER(set_hook)
{
    struct process *process = NULL;
    struct thread *thread = NULL;
    struct desktop *desktop;
    struct hook *hook;
    WCHAR *module;
    int global;
    data_size_t module_size = get_req_data_size();

    if (!req->proc || req->id < WH_MINHOOK || req->id > WH_WINEVENT)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!(desktop = get_thread_desktop( current, DESKTOP_HOOKCONTROL ))) return;

    if (req->pid && !(process = get_process_from_id( req->pid ))) goto done;

    if (req->tid)
    {
        if (!(thread = get_thread_from_id( req->tid ))) goto done;
        if (process && process != thread->process)
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
        }
    }

    if (req->id == WH_KEYBOARD_LL || req->id == WH_MOUSE_LL)
    {
        /* low-level hardware hooks are special: always global, but without a module */
        if (thread)
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
        }
        module = NULL;
        global = 1;
    }
    else if (!req->tid)
    {
        /* out of context hooks do not need a module handle */
        if (!module_size && (req->flags & WINEVENT_INCONTEXT))
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
        }
        if (!(module = memdup( get_req_data(), module_size ))) goto done;
        global = 1;
    }
    else
    {
        /* module is optional only if hook is in current process */
        if (!module_size)
        {
            module = NULL;
            if (thread->process != current->process)
            {
                set_error( STATUS_INVALID_PARAMETER );
                goto done;
            }
        }
        else if (!(module = memdup( get_req_data(), module_size ))) goto done;
        global = 0;
    }

    if ((hook = add_hook( desktop, thread, req->id - WH_MINHOOK, global )))
    {
        hook->owner = (struct thread *)grab_object( current );
        hook->process = process ? (struct process *)grab_object( process ) : NULL;
        hook->event_min   = req->event_min;
        hook->event_max   = req->event_max;
        hook->flags       = req->flags;
        hook->proc        = req->proc;
        hook->unicode     = req->unicode;
        hook->module      = module;
        hook->module_size = module_size;
        reply->handle = hook->handle;
        reply->active_hooks = get_active_hooks();
    }
    else free( module );

done:
    if (process) release_object( process );
    if (thread) release_object( thread );
    release_object( desktop );
}


/* remove a window hook */
DECL_HANDLER(remove_hook)
{
    struct hook *hook;

    if (req->handle)
    {
        if (!(hook = get_user_object( req->handle, USER_HOOK )))
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
    }
    else
    {
        if (!req->proc || req->id < WH_MINHOOK || req->id > WH_WINEVENT)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
        if (!(hook = find_hook( current, req->id - WH_MINHOOK, req->proc )))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }
    remove_hook( hook );
    reply->active_hooks = get_active_hooks();
}


/* start calling a hook chain */
DECL_HANDLER(start_hook_chain)
{
    struct hook *hook;
    struct hook_table *table = get_queue_hooks( current );

    if (req->id < WH_MINHOOK || req->id > WH_WINEVENT)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    reply->active_hooks = get_active_hooks();

    if (!table || !(hook = get_first_valid_hook( table, req->id - WH_MINHOOK, req->event,
                                                 req->window, req->object_id, req->child_id )))
    {
        /* try global table */
        if (!(table = get_global_hooks( current )) ||
            !(hook = get_first_valid_hook( table, req->id - WH_MINHOOK, req->event,
                                           req->window, req->object_id, req->child_id )))
            return;  /* no hook set */
    }

    if (run_hook_in_owner_thread( hook ))
    {
        reply->pid  = get_process_id( hook->owner->process );
        reply->tid  = get_thread_id( hook->owner );
    }
    else
    {
        reply->pid  = 0;
        reply->tid  = 0;
    }
    reply->proc    = hook->proc;
    reply->handle  = hook->handle;
    reply->unicode = hook->unicode;
    table->counts[hook->index]++;
    if (hook->module) set_reply_data( hook->module, hook->module_size );
}


/* finished calling a hook chain */
DECL_HANDLER(finish_hook_chain)
{
    struct hook_table *table = get_queue_hooks( current );
    struct hook_table *global_hooks = get_global_hooks( current );
    int index = req->id - WH_MINHOOK;

    if (req->id < WH_MINHOOK || req->id > WH_WINEVENT)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (table) release_hook_chain( table, index );
    if (global_hooks) release_hook_chain( global_hooks, index );
}


/* get the hook information */
DECL_HANDLER(get_hook_info)
{
    struct hook *hook;

    if (!(hook = get_user_object( req->handle, USER_HOOK ))) return;
    if (hook->thread && (hook->thread != current))
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if (req->get_next && !(hook = get_next_hook( current, hook, req->event, req->window,
                                                 req->object_id, req->child_id )))
        return;

    reply->handle  = hook->handle;
    reply->id      = hook->index + WH_MINHOOK;
    reply->unicode = hook->unicode;
    if (hook->module) set_reply_data( hook->module, min(hook->module_size,get_reply_max_size()) );
    if (run_hook_in_owner_thread( hook ))
    {
        reply->pid  = get_process_id( hook->owner->process );
        reply->tid  = get_thread_id( hook->owner );
    }
    else
    {
        reply->pid  = 0;
        reply->tid  = 0;
    }
    reply->proc = hook->proc;
}
