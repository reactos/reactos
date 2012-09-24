/*
 * Server-side clipboard management
 *
 * Copyright (C) 2002 Ulrich Czekalla
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

#include "object.h"
#include "request.h"
#include "user.h"

#define NDEBUG
#include <debug.h>

struct clipboard
{
    struct object  obj;              /* object header */
    PTHREADINFO    open_thread;      /* thread id that has clipboard open */
    user_handle_t  open_win;         /* window that has clipboard open */
    PTHREADINFO    owner_thread;     /* thread id that owns the clipboard */
    user_handle_t  owner_win;        /* window that owns the clipboard data */
    user_handle_t  viewer;           /* first window in clipboard viewer list */
    unsigned int   seqno;            /* clipboard change sequence number */
    time_t         seqno_timestamp;  /* time stamp of last seqno increment */
};

static void clipboard_dump( struct object *obj, int verbose );

static const struct object_ops clipboard_ops =
{
    sizeof(struct clipboard),     /* size */
    clipboard_dump,               /* dump */
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
    no_destroy                    /* destroy */
};


#define MINUPDATELAPSE 2

/* dump a clipboard object */
static void clipboard_dump( struct object *obj, int verbose )
{
    struct clipboard *clipboard = (struct clipboard *)obj;

    DPRINT1( "Clipboard open_thread=%p open_win=%08x owner_thread=%p owner_win=%08x viewer=%08x seq=%u\n",
             clipboard->open_thread, clipboard->open_win, clipboard->owner_thread,
             clipboard->owner_win, clipboard->viewer, clipboard->seqno );
}

/* retrieve the clipboard info for the current process, allocating it if needed */
static struct clipboard *get_process_clipboard(void)
{
    struct clipboard *clipboard;
    struct winstation *winstation = get_process_winstation( PsGetCurrentProcessWin32Process(), WINSTA_ACCESSCLIPBOARD );

    if (!winstation) return NULL;

    if (!(clipboard = winstation->clipboard))
    {
        if ((clipboard = alloc_object( &clipboard_ops )))
        {
            clipboard->open_thread = NULL;
            clipboard->open_win = 0;
            clipboard->owner_thread = NULL;
            clipboard->owner_win = 0;
            clipboard->viewer = 0;
            clipboard->seqno = 0;
            clipboard->seqno_timestamp = 0;
            winstation->clipboard = clipboard;
        }
    }
    release_object( winstation );
    return clipboard;
}


/* Called when thread terminates to allow release of clipboard */
void cleanup_clipboard_thread(PTHREADINFO thread)
{
    struct clipboard *clipboard;
    struct winstation *winstation;

    if (!thread->process->winstation) return;
    if (!(winstation = get_process_winstation( thread->process, WINSTA_ACCESSCLIPBOARD ))) return;

    if ((clipboard = winstation->clipboard))
    {
        if (thread == clipboard->open_thread)
        {
            clipboard->open_win = 0;
            clipboard->open_thread = NULL;
        }
        if (thread == clipboard->owner_thread)
        {
            clipboard->owner_win = 0;
            clipboard->owner_thread = NULL;
        }
    }
    release_object( winstation );
}

static int set_clipboard_window( struct clipboard *clipboard, user_handle_t win, int clear )
{
    if (clipboard->open_thread && clipboard->open_thread != PsGetCurrentThreadWin32Thread())
    {
        set_error(STATUS_WAS_LOCKED);
        return 0;
    }
    else if (!clear)
    {
        clipboard->open_win = win;
        clipboard->open_thread = PsGetCurrentThreadWin32Thread();
    }
    else
    {
        clipboard->open_thread = NULL;
        clipboard->open_win = 0;
    }
    return 1;
}


static int set_clipboard_owner( struct clipboard *clipboard, user_handle_t win, int clear )
{
    if (clipboard->open_thread && clipboard->open_thread->process != PsGetCurrentProcessWin32Process())
    {
        set_error(STATUS_WAS_LOCKED);
        return 0;
    }
    else if (!clear)
    {
        clipboard->owner_win = win;
        clipboard->owner_thread = PsGetCurrentThreadWin32Thread();
    }
    else
    {
        clipboard->owner_win = 0;
        clipboard->owner_thread = NULL;
    }
    return 1;
}


static int get_seqno( struct clipboard *clipboard )
{
    time_t tm;
    LARGE_INTEGER SystemTime;

    KeQuerySystemTime(&SystemTime);
    tm = SystemTimeToUnixTime(&SystemTime);

    if (!clipboard->owner_thread && (tm > (clipboard->seqno_timestamp + MINUPDATELAPSE)))
    {
        clipboard->seqno_timestamp = tm;
        clipboard->seqno++;
    }
    return clipboard->seqno;
}


DECL_HANDLER(set_clipboard_info)
{
    struct clipboard *clipboard = get_process_clipboard();
    PTHREADINFO current = PsGetCurrentThreadWin32Thread();

    if (!clipboard) return;

    reply->old_clipboard = clipboard->open_win;
    reply->old_owner     = clipboard->owner_win;
    reply->old_viewer    = clipboard->viewer;

    if (req->flags & SET_CB_OPEN)
    {
        if (clipboard->open_thread)
        {
            /* clipboard already opened */
            set_error(STATUS_WAS_LOCKED);
            return;
        }

        if (!set_clipboard_window( clipboard, req->clipboard, 0 )) return;
    }
    else if (req->flags & SET_CB_CLOSE)
    {
        if (clipboard->open_thread != current)
        {
            set_win32_error(ERROR_CLIPBOARD_NOT_OPEN);
            return;
        }

        if (!set_clipboard_window( clipboard, 0, 1 )) return;
    }

    if (req->flags & SET_CB_OWNER)
    {
        if (!set_clipboard_owner( clipboard, req->owner, 0 )) return;
    }
    else if (req->flags & SET_CB_RELOWNER)
    {
        if (!set_clipboard_owner( clipboard, 0, 1 )) return;
    }

    if (req->flags & SET_CB_VIEWER) clipboard->viewer = req->viewer;

    if (req->flags & SET_CB_SEQNO) clipboard->seqno++;

    reply->seqno = get_seqno( clipboard );

    if (clipboard->open_thread == current) reply->flags |= CB_OPEN;
    if (clipboard->owner_thread == current) reply->flags |= CB_OWNER;
    if (clipboard->owner_thread && clipboard->owner_thread->process == current->process)
        reply->flags |= CB_PROCESS;
}
