/*
 * Server-side window stations and desktops handling
 *
 * Copyright (C) 2002, 2005 Alexandre Julliard
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

#include <win32k.h>

#include <limits.h>

#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include "object.h"
#include "request.h"
#include "handle.h"
#include "user.h"

#define NDEBUG
#include <debug.h>


static struct list winstation_list = LIST_INIT(winstation_list);
static struct namespace *winstation_namespace;

static void winstation_dump( struct object *obj, int verbose );
static struct object_type *winstation_get_type( struct object *obj );
static int winstation_close_handle( struct object *obj, PPROCESSINFO process, obj_handle_t handle );
static void winstation_destroy( struct object *obj );
static unsigned int winstation_map_access( struct object *obj, unsigned int access );
static void desktop_dump( struct object *obj, int verbose );
static struct object_type *desktop_get_type( struct object *obj );
static int desktop_close_handle( struct object *obj, PPROCESSINFO process, obj_handle_t handle );
static void desktop_destroy( struct object *obj );
static unsigned int desktop_map_access( struct object *obj, unsigned int access );

static const struct object_ops winstation_ops =
{
    sizeof(struct winstation),    /* size */
    winstation_dump,              /* dump */
    winstation_get_type,          /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    winstation_map_access,        /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    winstation_close_handle,      /* close_handle */
    winstation_destroy            /* destroy */
};


static const struct object_ops desktop_ops =
{
    sizeof(struct desktop),       /* size */
    desktop_dump,                 /* dump */
    desktop_get_type,             /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    desktop_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    desktop_close_handle,         /* close_handle */
    desktop_destroy               /* destroy */
};

#define DESKTOP_ALL_ACCESS 0x01ff

/* create a winstation object */
static struct winstation *create_winstation( const struct unicode_str *name, unsigned int attr,
                                             unsigned int flags )
{
    struct winstation *winstation;

    if (!winstation_namespace && !(winstation_namespace = create_namespace( 7 )))
        return NULL;

    if (memchrW( name->str, '\\', name->len / sizeof(WCHAR) ))  /* no backslash allowed in name */
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if ((winstation = create_named_object( winstation_namespace, &winstation_ops, name, attr )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            winstation->flags = flags;
            winstation->clipboard = NULL;
            winstation->atom_table = NULL;
            list_add_tail( &winstation_list, &winstation->entry );
            list_init( &winstation->desktops );
        }
    }
    return winstation;
}

static void winstation_dump( struct object *obj, int verbose )
{
    struct winstation *winstation = (struct winstation *)obj;

    DPRINT1( "Winstation flags=%x clipboard=%p atoms=%p ",
             winstation->flags, winstation->clipboard, winstation->atom_table );
    dump_object_name( &winstation->obj );
    DbgPrint( "\n" );
}

static struct object_type *winstation_get_type( struct object *obj )
{
    static const WCHAR name[] = {'W','i','n','d','o','w','S','t','a','t','i','o','n'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static int winstation_close_handle( struct object *obj, PPROCESSINFO process, obj_handle_t handle )
{
    return (process->winstation != handle);
}

static void winstation_destroy( struct object *obj )
{
    struct winstation *winstation = (struct winstation *)obj;

    list_remove( &winstation->entry );
    if (winstation->clipboard) release_object( winstation->clipboard );
    if (winstation->atom_table) release_object( winstation->atom_table );
}

static unsigned int winstation_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES |
                                            WINSTA_ENUMERATE | WINSTA_READSCREEN;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP |
                                            WINSTA_WRITEATTRIBUTES;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_REQUIRED | WINSTA_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/* retrieve the process window station, checking the handle access rights */
struct winstation *get_process_winstation( PPROCESSINFO process, unsigned int access )
{
    return (struct winstation *)get_handle_obj( process, process->winstation,
                                                access, &winstation_ops );
}

/* build the full name of a desktop object */
static WCHAR *build_desktop_name( const struct unicode_str *name,
                                  struct winstation *winstation, struct unicode_str *res )
{
    const WCHAR *winstation_name;
    WCHAR *full_name;
    data_size_t winstation_len;

    if (memchrW( name->str, '\\', name->len / sizeof(WCHAR) ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if (!(winstation_name = get_object_name( &winstation->obj, &winstation_len )))
        winstation_len = 0;

    res->len = winstation_len + name->len + sizeof(WCHAR);
    if (!(full_name = mem_alloc( res->len ))) return NULL;
    memcpy( full_name, winstation_name, winstation_len );
    full_name[winstation_len / sizeof(WCHAR)] = '\\';
    memcpy( full_name + winstation_len / sizeof(WCHAR) + 1, name->str, name->len );
    res->str = full_name;
    return full_name;
}

/* retrieve a pointer to a desktop object */
struct desktop *get_desktop_obj( PPROCESSINFO process, obj_handle_t handle, unsigned int access )
{
    return (struct desktop *)get_handle_obj( process, handle, access, &desktop_ops );
}

/* create a desktop object */
static struct desktop *create_desktop( const struct unicode_str *name, unsigned int attr,
                                       unsigned int flags, struct winstation *winstation )
{
    struct desktop *desktop;
    struct unicode_str full_str;
    WCHAR *full_name;

    if (!(full_name = build_desktop_name( name, winstation, &full_str ))) return NULL;

    if ((desktop = create_named_object( winstation_namespace, &desktop_ops, &full_str, attr )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            desktop->flags = flags;
            desktop->winstation = (struct winstation *)grab_object( winstation );
            desktop->top_window = NULL;
            desktop->msg_window = NULL;
            desktop->global_hooks = NULL;
            desktop->close_timeout = NULL;
            desktop->users = 0;
            list_add_tail( &winstation->desktops, &desktop->entry );
        }
    }
    ExFreePool( full_name );
    return desktop;
}

static void desktop_dump( struct object *obj, int verbose )
{
    struct desktop *desktop = (struct desktop *)obj;

    DPRINT1( "Desktop flags=%x winstation=%p top_win=%p hooks=%p ",
             desktop->flags, desktop->winstation, desktop->top_window, desktop->global_hooks );
    dump_object_name( &desktop->obj );
    DbgPrint( "\n" );
}

static struct object_type *desktop_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','e','s','k','t','o','p'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static int desktop_close_handle( struct object *obj, PPROCESSINFO process, obj_handle_t handle )
{
    PLIST_ENTRY ListHead, Entry;
    PETHREAD FoundThread = NULL;
    PTHREADINFO ThreadInfo;

    /* check if the handle is currently used by the process or one of its threads */
    if (process->desktop == handle) return 0;

    // FIXME: This code relies on ETHREAD internals!

    /* Lock the process */
    KeEnterCriticalRegion();
    ExfAcquirePushLockShared(&process->peProcess->ProcessLock);

    Entry = process->peProcess->ThreadListHead.Flink;

    ListHead = &process->peProcess->ThreadListHead;
    while (ListHead != Entry)
    {
        /* Get the Thread */
        FoundThread = CONTAINING_RECORD(Entry, ETHREAD, ThreadListEntry);
        ThreadInfo = PsGetThreadWin32Thread(FoundThread);
        if (ThreadInfo && ThreadInfo->desktop == handle)
        {
            /* Unlock the process */
            ExfReleasePushLockShared(&process->peProcess->ProcessLock);
            KeLeaveCriticalRegion();

            return 0;
        }

        /* Go to the next thread */
        Entry = Entry->Flink;
    }

    /* Unlock the process */
    ExfReleasePushLockShared(&process->peProcess->ProcessLock);
    KeLeaveCriticalRegion();

    return 1;
}

static void desktop_destroy( struct object *obj )
{
    struct desktop *desktop = (struct desktop *)obj;

    if (desktop->top_window) destroy_window( desktop->top_window );
    if (desktop->msg_window) destroy_window( desktop->msg_window );
    if (desktop->global_hooks) release_object( desktop->global_hooks );
    if (desktop->close_timeout) remove_timeout_user( desktop->close_timeout );
    list_remove( &desktop->entry );
    release_object( desktop->winstation );
}

static unsigned int desktop_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | DESKTOP_READOBJECTS | DESKTOP_ENUMERATE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                            DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
                                            DESKTOP_WRITEOBJECTS;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE | DESKTOP_SWITCHDESKTOP;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_REQUIRED | DESKTOP_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/* retrieve the thread desktop, checking the handle access rights */
struct desktop *get_thread_desktop( PTHREADINFO thread, unsigned int access )
{
    return get_desktop_obj( thread->process, thread->desktop, access );
}

/* set the process default desktop handle */
void set_process_default_desktop( PPROCESSINFO process, struct desktop *desktop,
                                  obj_handle_t handle )
{
    PTHREADINFO thread;
    PETHREAD eThread;
    PEPROCESS eProcess = process->peProcess;
    PLIST_ENTRY current_entry;
    struct desktop *old_desktop;

    if (process->desktop == handle) return;  /* nothing to do */

    if (!(old_desktop = get_desktop_obj( process, process->desktop, 0 ))) clear_error();
    process->desktop = handle;

    // FIXME: This code relies on ETHREAD internals!

    /* Lock the process */
    KeEnterCriticalRegion();
    ExfAcquirePushLockShared(&eProcess->ProcessLock);

    /* set desktop for threads that don't have one yet */
    current_entry = eProcess->ThreadListHead.Flink;
    while (current_entry != &eProcess->ThreadListHead)
    {
        eThread = CONTAINING_RECORD(current_entry, ETHREAD,
            ThreadListEntry);
        thread = (PTHREADINFO)PsGetThreadWin32Thread(eThread);

        if (thread && !thread->desktop) thread->desktop = handle;

        current_entry = current_entry->Flink;
    }

    /* Unlock the process */
    ExfReleasePushLockShared(&eProcess->ProcessLock);
    KeLeaveCriticalRegion();

#if 0
    if (!process->is_system)
    {
        desktop->users++;
        if (desktop->close_timeout)
        {
            remove_timeout_user( desktop->close_timeout );
            desktop->close_timeout = NULL;
        }
        if (old_desktop) old_desktop->users--;
    }
#else
    DPRINT1("close_timeout for this process is not done yet!\n");
#endif

    if (old_desktop) release_object( old_desktop );
}

/* connect a process to its window station */
void connect_process_winstation( PPROCESSINFO process, PTHREADINFO parent )
{
    struct winstation *winstation = NULL;
    struct desktop *desktop = NULL;
    obj_handle_t handle;

    /* check for an inherited winstation handle (don't ask...) */
    if ((handle = find_inherited_handle( process, &winstation_ops )))
    {
        winstation = (struct winstation *)get_handle_obj( process, handle, 0, &winstation_ops );
    }
    else if (parent && parent->process->winstation)
    {
        handle = duplicate_handle( parent->process, parent->process->winstation,
                                   process, 0, 0, DUP_HANDLE_SAME_ACCESS );
        winstation = (struct winstation *)get_handle_obj( process, handle, 0, &winstation_ops );
    }
    if (!winstation) goto done;
    process->winstation = handle;

    if ((handle = find_inherited_handle( process, &desktop_ops )))
    {
        desktop = get_desktop_obj( process, handle, 0 );
        if (!desktop || desktop->winstation != winstation) goto done;
    }
    else if (parent && parent->desktop)
    {
        desktop = get_desktop_obj( parent->process, parent->desktop, 0 );
        if (!desktop || desktop->winstation != winstation) goto done;
        handle = duplicate_handle( parent->process, parent->desktop,
                                   process, 0, 0, DUP_HANDLE_SAME_ACCESS );
    }

    if (handle) set_process_default_desktop( process, desktop, handle );

done:
    if (desktop) release_object( desktop );
    if (winstation) release_object( winstation );
    clear_error();
}

static
VOID
NTAPI
close_desktop_timeout( PKDPC Dpc, PVOID Context, PVOID SystemArgument1, PVOID SystemArgument2 )
{
    struct desktop *desktop = Context;

    desktop->close_timeout = NULL;
    unlink_named_object( &desktop->obj );  /* make sure no other process can open it */
    close_desktop_window( desktop );  /* and signal the owner to quit */
}

/* close the desktop of a given process */
void close_process_desktop( PPROCESSINFO process )
{
    struct desktop *desktop;

    if (process->desktop && (desktop = get_desktop_obj( process, process->desktop, 0 )))
    {
        assert( desktop->users > 0 );
        desktop->users--;
        /* if we have one remaining user, it has to be the manager of the desktop window */
        if (desktop->users == 1 && get_top_window_owner( desktop ))
        {
            assert( !desktop->close_timeout );
            desktop->close_timeout = add_timeout_user( -TICKS_PER_SEC, close_desktop_timeout, desktop );
        }
        release_object( desktop );
    }
    clear_error();  /* ignore errors */
}

/* close the desktop of a given thread */
void close_thread_desktop( PTHREADINFO thread )
{
    obj_handle_t handle = thread->desktop;

    thread->desktop = 0;
    if (handle) close_handle( thread->process, handle );
    clear_error();  /* ignore errors */
}

/* set the reply data from the object name */
static void set_reply_data_obj_name( void *req, struct object *obj )
{
    data_size_t len;
    const WCHAR *ptr, *name = get_object_name( obj, &len );

    /* if there is a backslash return the part of the name after it */
    if (name && (ptr = memchrW( name, '\\', len/sizeof(WCHAR) )))
    {
        len -= (ptr + 1 - name) * sizeof(WCHAR);
        name = ptr + 1;
    }
    if (name) set_reply_data( req, name, min( len, get_reply_max_size(req) ));
}

/* create a window station */
DECL_HANDLER(create_winstation)
{
    struct winstation *winstation;
    struct unicode_str name;

    reply->handle = 0;
    get_req_unicode_str( (void*)req, &name );
    if ((winstation = create_winstation( &name, req->attributes, req->flags )))
    {
        reply->handle = alloc_handle( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), winstation, req->access, req->attributes );
        release_object( winstation );
    }
}

/* open a handle to a window station */
DECL_HANDLER(open_winstation)
{
    struct unicode_str name;

    get_req_unicode_str( (void*)req, &name );
    if (winstation_namespace)
        reply->handle = open_object( winstation_namespace, &name, &winstation_ops, req->access,
                                     req->attributes );
    else
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
}


/* close a window station */
DECL_HANDLER(close_winstation)
{
    struct winstation *winstation;

    if ((winstation = (struct winstation *)get_handle_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle,
                                                           0, &winstation_ops )))
    {
        if (!close_handle( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle )) set_error( STATUS_ACCESS_DENIED );
        release_object( winstation );
    }
}


/* get the process current window station */
DECL_HANDLER(get_process_winstation)
{
    reply->handle = ((PPROCESSINFO)PsGetCurrentProcessWin32Process())->winstation;
}


/* set the process current window station */
DECL_HANDLER(set_process_winstation)
{
    struct winstation *winstation;

    if ((winstation = (struct winstation *)get_handle_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle,
                                                           0, &winstation_ops )))
    {
        /* FIXME: should we close the old one? */
        ((PPROCESSINFO)PsGetCurrentProcessWin32Process())->winstation = req->handle;
        release_object( winstation );
    }
}

/* create a desktop */
DECL_HANDLER(create_desktop)
{
    struct desktop *desktop;
    struct winstation *winstation;
    struct unicode_str name;
    PPROCESSINFO current = (PPROCESSINFO)PsGetCurrentProcessWin32Process();

    reply->handle = 0;
    get_req_unicode_str( (void*)req, &name );
    if ((winstation = get_process_winstation( current, WINSTA_CREATEDESKTOP )))
    {
        if ((desktop = create_desktop( &name, req->attributes, req->flags, winstation )))
        {
            reply->handle = alloc_handle( current, desktop, req->access, req->attributes );
            if (get_error() != STATUS_OBJECT_NAME_EXISTS)
                CsrNotifyCreateDesktop((HDESK)reply->handle);
            release_object( desktop );
        }
        release_object( winstation );
    }
}

/* open a handle to a desktop */
DECL_HANDLER(open_desktop)
{
    struct winstation *winstation;
    struct unicode_str name;

    get_req_unicode_str( (void*)req, &name );

    /* FIXME: check access rights */
    if (!req->winsta)
        winstation = get_process_winstation( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), 0 );
    else
        winstation = (struct winstation *)get_handle_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->winsta, 0, &winstation_ops );

    if (winstation)
    {
        struct unicode_str full_str;
        WCHAR *full_name;

        if ((full_name = build_desktop_name( &name, winstation, &full_str )))
        {
            reply->handle = open_object( winstation_namespace, &full_str, &desktop_ops, req->access,
                                         req->attributes );
            ExFreePool( full_name );
        }
        release_object( winstation );
    }
}


/* close a desktop */
DECL_HANDLER(close_desktop)
{
    struct desktop *desktop;

    /* make sure it is a desktop handle */
    if ((desktop = (struct desktop *)get_handle_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle,
                                                     0, &desktop_ops )))
    {
        if (!close_handle( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle )) set_error( STATUS_DEVICE_BUSY );
        release_object( desktop );
    }
}


/* get the thread current desktop */
DECL_HANDLER(get_thread_desktop)
{
    PETHREAD ethread = NULL;
    PTHREADINFO thread = NULL;
    NTSTATUS status;

    status = PsLookupThreadByThreadId((HANDLE)req->tid, &ethread);
    if (!NT_SUCCESS(status)) return;
    if (!(thread = (PTHREADINFO)ethread->Tcb.Win32Thread)) return;
    ObReferenceObjectByPointer(ethread, 0, NULL, KernelMode);

    reply->handle = thread->desktop;

    ObDereferenceObject(ethread);
}


/* set the thread current desktop */
DECL_HANDLER(set_thread_desktop)
{
    struct desktop *old_desktop, *new_desktop;
    struct winstation *winstation;
    PTHREADINFO current = (PTHREADINFO)PsGetCurrentThreadWin32Thread();

    if (!(winstation = get_process_winstation( current->process, 0 /* FIXME: access rights? */ )))
        return;

    if (!(new_desktop = get_desktop_obj( current->process, req->handle, 0 )))
    {
        release_object( winstation );
        return;
    }
    if (new_desktop->winstation != winstation)
    {
        set_error( STATUS_ACCESS_DENIED );
        release_object( new_desktop );
        release_object( winstation );
        return;
    }

    /* check if we are changing to a new desktop */

    if (!(old_desktop = get_desktop_obj( current->process, current->desktop, 0)))
        clear_error();  /* ignore error */

    /* when changing desktop, we can't have any users on the current one */
    if (old_desktop != new_desktop && current->desktop_users > 0)
        set_error( STATUS_DEVICE_BUSY );
    else
        current->desktop = req->handle;  /* FIXME: should we close the old one? */

    if (!current->desktop)
        set_process_default_desktop( current->process, new_desktop, req->handle );

    if (old_desktop != new_desktop && current->queue) detach_thread_input( current );

    if (old_desktop) release_object( old_desktop );
    release_object( new_desktop );
    release_object( winstation );
}


/* get/set information about a user object (window station or desktop) */
DECL_HANDLER(set_user_object_info)
{
    struct object *obj;

    if (!(obj = get_handle_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle, 0, NULL ))) return;

    if (obj->ops == &desktop_ops)
    {
        struct desktop *desktop = (struct desktop *)obj;
        reply->is_desktop = 1;
        reply->old_obj_flags = desktop->flags;
        if (req->flags & SET_USER_OBJECT_FLAGS) desktop->flags = req->obj_flags;
    }
    else if (obj->ops == &winstation_ops)
    {
        struct winstation *winstation = (struct winstation *)obj;
        reply->is_desktop = 0;
        reply->old_obj_flags = winstation->flags;
        if (req->flags & SET_USER_OBJECT_FLAGS) winstation->flags = req->obj_flags;
    }
    else
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
        return;
    }
    if (get_reply_max_size((void*)req)) set_reply_data_obj_name( (void*)req, obj );
    release_object( obj );
}


/* enumerate window stations */
DECL_HANDLER(enum_winstation)
{
    unsigned int index = 0;
    struct winstation *winsta;

    LIST_FOR_EACH_ENTRY( winsta, &winstation_list, struct winstation, entry )
    {
        unsigned int access = WINSTA_ENUMERATE;
        if (req->index > index++) continue;
        if (!check_object_access( &winsta->obj, &access )) continue;
        set_reply_data_obj_name( (void*)req, &winsta->obj );
        clear_error();
        reply->next = index;
        return;
    }
    set_error( STATUS_NO_MORE_ENTRIES );
}


/* enumerate desktops */
DECL_HANDLER(enum_desktop)
{
    struct winstation *winstation;
    struct desktop *desktop;
    unsigned int index = 0;

    if (!(winstation = (struct winstation *)get_handle_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->winstation,
                                                            WINSTA_ENUMDESKTOPS, &winstation_ops )))
        return;

    LIST_FOR_EACH_ENTRY( desktop, &winstation->desktops, struct desktop, entry )
    {
        unsigned int access = DESKTOP_ENUMERATE;
        if (req->index > index++) continue;
        if (!desktop->obj.name) continue;
        if (!check_object_access( &desktop->obj, &access )) continue;
        set_reply_data_obj_name( (void*)req, &desktop->obj );
        release_object( winstation );
        clear_error();
        reply->next = index;
        return;
    }

    release_object( winstation );
    set_error( STATUS_NO_MORE_ENTRIES );
}
