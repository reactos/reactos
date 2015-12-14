/*
 * Server-side message queues
 *
 * Copyright (C) 2000 Alexandre Julliard
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
#include <undocuser.h>

#include <limits.h>
#include "object.h"
#include "request.h"
#include "handle.h"
#include "user.h"

#define NDEBUG
#include <debug.h>

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

enum message_kind { SEND_MESSAGE, POST_MESSAGE };
#define NB_MSG_KINDS (POST_MESSAGE+1)

typedef struct _QUEUE_WORKER_CONTEXT
{
    WORK_QUEUE_ITEM item;
    struct msg_queue *queue;
    struct message_result *result;
} QUEUE_WORKER_CONTEXT, *PQUEUE_WORKER_CONTEXT;

struct message_result
{
    struct list            sender_entry;  /* entry in sender list */
    struct message        *msg;           /* message the result is for */
    struct message_result *recv_next;     /* next in receiver list */
    struct msg_queue      *sender;        /* sender queue */
    struct msg_queue      *receiver;      /* receiver queue */
    int                    replied;       /* has it been replied to? */
    unsigned int           error;         /* error code to pass back to sender */
    lparam_t               result;        /* reply result */
    struct message        *callback_msg;  /* message to queue for callback */
    void                  *data;          /* message reply data */
    unsigned int           data_size;     /* size of message reply data */
    struct timeout_user   *timeout;       /* result timeout */
};

struct message
{
    struct list            entry;     /* entry in message list */
    enum message_type      type;      /* message type */
    user_handle_t          win;       /* window handle */
    unsigned int           msg;       /* message code */
    lparam_t               wparam;    /* parameters */
    lparam_t               lparam;    /* parameters */
    unsigned int           time;      /* message time */
    void                  *data;      /* message data for sent messages */
    unsigned int           data_size; /* size of message data */
    unsigned int           unique_id; /* unique id for nested hw message waits */
    struct message_result *result;    /* result in sender queue */
};

struct timer
{
    struct list     entry;     /* entry in timer list */
    timeout_t       when;      /* next expiration */
    unsigned int    rate;      /* timer rate in ms */
    user_handle_t   win;       /* window handle */
    unsigned int    msg;       /* message to post */
    lparam_t        id;        /* timer id */
    lparam_t        lparam;    /* lparam for message */
};

struct thread_input
{
    struct object          obj;           /* object header */
    struct desktop        *desktop;       /* desktop that this thread input belongs to */
    user_handle_t          focus;         /* focus window */
    user_handle_t          capture;       /* capture window */
    user_handle_t          active;        /* active window */
    user_handle_t          menu_owner;    /* current menu owner window */
    user_handle_t          move_size;     /* current moving/resizing window */
    user_handle_t          caret;         /* caret window */
    rectangle_t            caret_rect;    /* caret rectangle */
    int                    caret_hide;    /* caret hide count */
    int                    caret_state;   /* caret on/off state */
    user_handle_t          cursor;        /* current cursor */
    int                    cursor_count;  /* cursor show count */
    struct list            msg_list;      /* list of hardware messages */
    unsigned char          keystate[256]; /* state of each key */
};

struct msg_queue
{
    struct object          obj;             /* object header */
    struct fd             *fd;              /* optional file descriptor to poll */
    HANDLE                 wake_event;      /* wakeup event handle */
    PKEVENT                wake_event_ptr;  /* wakeup event pointer */
    unsigned int           wake_bits;       /* wakeup bits */
    unsigned int           wake_mask;       /* wakeup mask */
    unsigned int           changed_bits;    /* changed wakeup bits */
    unsigned int           changed_mask;    /* changed wakeup mask */
    int                    paint_count;     /* pending paint messages count */
    int                    quit_message;    /* is there a pending quit message? */
    int                    exit_code;       /* exit code of pending quit message */
    int                    cursor_count;    /* per-queue cursor show count */
    struct list            msg_list[NB_MSG_KINDS];  /* lists of messages */
    struct list            send_result;     /* stack of sent messages waiting for result */
    struct list            callback_result; /* list of callback messages waiting for result */
    struct message_result *recv_result;     /* stack of received messages waiting for result */
    struct list            pending_timers;  /* list of pending timers */
    struct list            expired_timers;  /* list of expired timers */
    lparam_t               next_timer_id;   /* id for the next timer with a 0 window */
    struct timeout_user   *timeout;         /* timeout for next timer to expire */
    struct thread_input   *input;           /* thread input descriptor */
    struct hook_table     *hooks;           /* hook table */
    timeout_t              last_get_msg;    /* time of last get message call */
};

static void msg_queue_dump( struct object *obj, int verbose );
static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int msg_queue_signaled( struct object *obj, PTHREADINFO thread );
static int msg_queue_satisfied( struct object *obj, PTHREADINFO thread );
static void msg_queue_destroy( struct object *obj );
static void msg_queue_poll_event( struct fd *fd, int event );
static void thread_input_dump( struct object *obj, int verbose );
static void thread_input_destroy( struct object *obj );
static void timer_callback( void *private );

static const struct object_ops msg_queue_ops =
{
    sizeof(struct msg_queue),  /* size */
    msg_queue_dump,            /* dump */
    no_get_type,               /* get_type */
    msg_queue_add_queue,       /* add_queue */
    msg_queue_remove_queue,    /* remove_queue */
    msg_queue_signaled,        /* signaled */
    msg_queue_satisfied,       /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    no_map_access,             /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
    no_close_handle,           /* close_handle */
    msg_queue_destroy          /* destroy */
};

static const struct fd_ops msg_queue_fd_ops =
{
    NULL,                        /* get_poll_events */
    msg_queue_poll_event,        /* poll_event */
    NULL,                        /* flush */
    NULL,                        /* get_fd_type */
    NULL,                        /* ioctl */
    NULL,                        /* queue_async */
    NULL,                        /* reselect_async */
    NULL                         /* cancel async */
};


static const struct object_ops thread_input_ops =
{
    sizeof(struct thread_input),  /* size */
    thread_input_dump,            /* dump */
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
    thread_input_destroy          /* destroy */
};

/* pointer to input structure of foreground thread */
static struct thread_input *foreground_input;
static unsigned int last_input_time;

static void free_message( struct message *msg );

/* set the caret window in a given thread input */
static void set_caret_window( struct thread_input *input, user_handle_t win )
{
    if (!win || win != input->caret)
    {
        input->caret_rect.left   = 0;
        input->caret_rect.top    = 0;
        input->caret_rect.right  = 0;
        input->caret_rect.bottom = 0;
    }
    input->caret             = win;
    input->caret_hide        = 1;
    input->caret_state       = 0;
}

/* create a thread input object */
static struct thread_input *create_thread_input( PTHREADINFO thread )
{
    struct thread_input *input;

    if ((input = alloc_object( &thread_input_ops )))
    {
        input->focus        = 0;
        input->capture      = 0;
        input->active       = 0;
        input->menu_owner   = 0;
        input->move_size    = 0;
        input->cursor       = 0;
        input->cursor_count = 0;
        list_init( &input->msg_list );
        set_caret_window( input, 0 );
        memset( input->keystate, 0, sizeof(input->keystate) );

        if (!(input->desktop = get_thread_desktop( thread, 0 /* FIXME: access rights */ )))
        {
            DPRINT1("error getting thread desktop, SHOULD FAIL! BIG BUG\n");
            //release_object( input );
            //return NULL;
        }
    }
    return input;
}

/* create a message queue object */
static struct msg_queue *create_msg_queue( PTHREADINFO thread, struct thread_input *input )
{
    struct thread_input *new_input = NULL;
    struct msg_queue *queue;
    int i;

    if (!input)
    {
        if (!(new_input = create_thread_input( thread ))) return NULL;
        input = new_input;
    }

    if ((queue = alloc_object( &msg_queue_ops )))
    {
        queue->fd              = NULL;
        ZwCreateEvent(&queue->wake_event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        /* Get a pointer to the object itself */
        ObReferenceObjectByHandle(queue->wake_event,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  KernelMode,
                                  (PVOID)&queue->wake_event_ptr,
                                  NULL);
        queue->wake_bits       = 0;
        queue->wake_mask       = 0;
        queue->changed_bits    = 0;
        queue->changed_mask    = 0;
        queue->paint_count     = 0;
        queue->quit_message    = 0;
        queue->cursor_count    = 0;
        queue->recv_result     = NULL;
        queue->next_timer_id   = 0x7fff;
        queue->timeout         = NULL;
        queue->input           = (struct thread_input *)grab_object( input );
        queue->hooks           = NULL;
        get_current_time(&queue->last_get_msg);
        list_init( &queue->send_result );
        list_init( &queue->callback_result );
        list_init( &queue->pending_timers );
        list_init( &queue->expired_timers );
        for (i = 0; i < NB_MSG_KINDS; i++) list_init( &queue->msg_list[i] );

        thread->queue = queue;
    }
    if (new_input) release_object( new_input );
    return queue;
}

/* free the message queue of a thread at thread exit */
void free_msg_queue( PTHREADINFO thread )
{
    remove_thread_hooks( thread );
    if (!thread->queue) return;
    release_object( thread->queue );
    thread->queue = NULL;
}

/* change the thread input data of a given thread */
static int assign_thread_input( PTHREADINFO thread, struct thread_input *new_input )
{
    struct msg_queue *queue = thread->queue;

    if (!queue)
    {
        thread->queue = create_msg_queue( thread, new_input );
        return thread->queue != NULL;
    }
    if (queue->input)
    {
        queue->input->cursor_count -= queue->cursor_count;
        release_object( queue->input );
    }
    queue->input = (struct thread_input *)grab_object( new_input );
    new_input->cursor_count += queue->cursor_count;
    return 1;
}

/* get the hook table for a given thread */
struct hook_table *get_queue_hooks( PTHREADINFO thread )
{
    if (!thread->queue) return NULL;
    return thread->queue->hooks;
}

/* set the hook table for a given thread, allocating the queue if needed */
void set_queue_hooks( PTHREADINFO thread, struct hook_table *hooks )
{
    struct msg_queue *queue = thread->queue;
    if (!queue && !(queue = create_msg_queue( thread, NULL ))) return;
    if (queue->hooks) release_object( queue->hooks );
    queue->hooks = hooks;
}

/* check the queue status */
static inline int is_signaled( struct msg_queue *queue )
{
    return ((queue->wake_bits & queue->wake_mask) || (queue->changed_bits & queue->changed_mask));
}

/* set some queue bits */
static inline void set_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    queue->wake_bits |= bits;
    queue->changed_bits |= bits;
    if (is_signaled( queue ))
    {
        KeSetEvent(queue->wake_event_ptr, EVENT_INCREMENT, FALSE);
    }
}

/* clear some queue bits */
static inline void clear_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    queue->wake_bits &= ~bits;
    queue->changed_bits &= ~bits;
}

/* check whether msg is a keyboard message */
static inline int is_keyboard_msg( struct message *msg )
{
    return (msg->msg >= WM_KEYFIRST && msg->msg <= WM_KEYLAST);
}

/* check if message is matched by the filter */
static inline int check_msg_filter( unsigned int msg, unsigned int first, unsigned int last )
{
    return (msg >= first && msg <= last);
}

/* check whether a message filter contains at least one potential hardware message */
static inline int filter_contains_hw_range( unsigned int first, unsigned int last )
{
    /* hardware message ranges are (in numerical order):
     *   WM_NCMOUSEFIRST .. WM_NCMOUSELAST
     *   WM_KEYFIRST .. WM_KEYLAST
     *   WM_MOUSEFIRST .. WM_MOUSELAST
     */
    if (last < WM_NCMOUSEFIRST) return 0;
    if (first > WM_NCMOUSELAST && last < WM_KEYFIRST) return 0;
    if (first > WM_KEYLAST && last < WM_MOUSEFIRST) return 0;
    if (first > WM_MOUSELAST) return 0;
    return 1;
}

/* get the QS_* bit corresponding to a given hardware message */
static inline int get_hardware_msg_bit( struct message *msg )
{
    if (msg->msg == WM_MOUSEMOVE || msg->msg == WM_NCMOUSEMOVE) return QS_MOUSEMOVE;
    if (is_keyboard_msg( msg )) return QS_KEY;
    return QS_MOUSEBUTTON;
}

/* get the current thread queue, creating it if needed */
static inline struct msg_queue *get_current_queue(void)
{
    PTHREADINFO current = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    struct msg_queue *queue = current->queue;
    if (!queue) queue = create_msg_queue( current, NULL );
    return queue;
}

/* get a (pseudo-)unique id to tag hardware messages */
static inline unsigned int get_unique_id(void)
{
    static unsigned int id;
    if (!++id) id = 1;  /* avoid an id of 0 */
    return id;
}

/* try to merge a message with the last in the list; return 1 if successful */
static int merge_message( struct thread_input *input, const struct message *msg )
{
    struct message *prev;
    struct list *ptr = list_tail( &input->msg_list );

    if (!ptr) return 0;
    prev = LIST_ENTRY( ptr, struct message, entry );
    if (prev->result) return 0;
    if (prev->win && msg->win && prev->win != msg->win) return 0;
    if (prev->msg != msg->msg) return 0;
    if (prev->type != msg->type) return 0;
    /* now we can merge it */
    prev->wparam  = msg->wparam;
    prev->lparam  = msg->lparam;
    prev->time    = msg->time;
    if (msg->type == MSG_HARDWARE && prev->data && msg->data)
    {
        struct hardware_msg_data *prev_data = prev->data;
        struct hardware_msg_data *msg_data = msg->data;
        prev_data->x     = msg_data->x;
        prev_data->y     = msg_data->y;
        prev_data->info  = msg_data->info;
    }
    return 1;
}

/* free a result structure */
static void free_result( struct message_result *result )
{
    if (result->timeout) remove_timeout_user( result->timeout );
    if (result->data) ExFreePool( result->data );
    if (result->callback_msg) free_message( result->callback_msg );
    ExFreePool( result );
}

/* remove the result from the sender list it is on */
static inline void remove_result_from_sender( struct message_result *result )
{
    assert( result->sender );

    list_remove( &result->sender_entry );
    result->sender = NULL;
    if (!result->receiver) free_result( result );
}

/* store the message result in the appropriate structure */
static void store_message_result( struct message_result *res, lparam_t result, unsigned int error )
{
    res->result  = result;
    res->error   = error;
    res->replied = 1;
    if (res->timeout)
    {
        remove_timeout_user( res->timeout );
        res->timeout = NULL;
    }
    if (res->sender)
    {
        if (res->callback_msg)
        {
            /* queue the callback message in the sender queue */
            struct callback_msg_data *data = res->callback_msg->data;
            data->result = result;
            list_add_tail( &res->sender->msg_list[SEND_MESSAGE], &res->callback_msg->entry );
            set_queue_bits( res->sender, QS_SENDMESSAGE );
            res->callback_msg = NULL;
            remove_result_from_sender( res );
        }
        else
        {
            /* wake sender queue if waiting on this result */
            if (list_head(&res->sender->send_result) == &res->sender_entry)
                set_queue_bits( res->sender, QS_SMRESULT );
        }
    }

}

/* free a message when deleting a queue or window */
static void free_message( struct message *msg )
{
    struct message_result *result = msg->result;
    if (result)
    {
        result->msg = NULL;
        if (result->sender)
        {
            result->receiver = NULL;
            store_message_result( result, 0, STATUS_ACCESS_DENIED /*FIXME*/ );
        }
        else free_result( result );
    }
    if (msg->data) ExFreePool( msg->data );
    ExFreePool( msg );
}

/* remove (and free) a message from a message list */
static void remove_queue_message( struct msg_queue *queue, struct message *msg,
                                  enum message_kind kind )
{
    list_remove( &msg->entry );
    switch(kind)
    {
    case SEND_MESSAGE:
        if (list_empty( &queue->msg_list[kind] )) clear_queue_bits( queue, QS_SENDMESSAGE );
        break;
    case POST_MESSAGE:
        if (list_empty( &queue->msg_list[kind] ) && !queue->quit_message)
            clear_queue_bits( queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
        break;
    }
    free_message( msg );
}

/* message timed out without getting a reply */
static void result_timeout( void *private )
{
    struct message_result *result = private;

    assert( !result->replied );

    result->timeout = NULL;

    if (result->msg)  /* not received yet */
    {
        struct message *msg = result->msg;

        result->msg = NULL;
        msg->result = NULL;
        remove_queue_message( result->receiver, msg, SEND_MESSAGE );
        result->receiver = NULL;
        if (!result->sender)
        {
            free_result( result );
            return;
        }
    }

    store_message_result( result, 0, STATUS_TIMEOUT );
}

/* allocate and fill a message result structure */
static struct message_result *alloc_message_result( struct msg_queue *send_queue,
                                                    struct msg_queue *recv_queue,
                                                    struct message *msg, timeout_t timeout )
{
    struct message_result *result = ExAllocatePool( PagedPool, sizeof(*result) );
    if (result)
    {
        result->msg       = msg;
        result->sender    = send_queue;
        result->receiver  = recv_queue;
        result->replied   = 0;
        result->data      = NULL;
        result->data_size = 0;
        result->timeout   = NULL;

        if (msg->type == MSG_CALLBACK)
        {
            struct message *callback_msg = mem_alloc( sizeof(*callback_msg) );

            if (!callback_msg)
            {
                ExFreePool( result );
                return NULL;
            }
            callback_msg->type      = MSG_CALLBACK_RESULT;
            callback_msg->win       = msg->win;
            callback_msg->msg       = msg->msg;
            callback_msg->wparam    = 0;
            callback_msg->lparam    = 0;
            callback_msg->time      = EngGetTickCount();
            callback_msg->result    = NULL;
            /* steal the data from the original message */
            callback_msg->data      = msg->data;
            callback_msg->data_size = msg->data_size;
            msg->data = NULL;
            msg->data_size = 0;

            result->callback_msg = callback_msg;
            list_add_head( &send_queue->callback_result, &result->sender_entry );
        }
        else
        {
            result->callback_msg = NULL;
            list_add_head( &send_queue->send_result, &result->sender_entry );
        }

        if (timeout != TIMEOUT_INFINITE)
            result->timeout = add_timeout_user( timeout, result_timeout, result );
    }
    return result;
}

/* receive a message, removing it from the sent queue */
static void receive_message( void *req, struct msg_queue *queue, struct message *msg,
                             struct get_message_reply *reply )
{
    struct message_result *result = msg->result;

    reply->total = msg->data_size;
    if (msg->data_size > get_reply_max_size(req))
    {
        set_error( STATUS_BUFFER_OVERFLOW );
        return;
    }
    reply->type   = msg->type;
    reply->win    = msg->win;
    reply->msg    = msg->msg;
    reply->wparam = msg->wparam;
    reply->lparam = msg->lparam;
    reply->time   = msg->time;

    if (msg->data) set_reply_data_ptr( req, msg->data, msg->data_size );

    list_remove( &msg->entry );
    /* put the result on the receiver result stack */
    if (result)
    {
        result->msg = NULL;
        result->recv_next  = queue->recv_result;
        queue->recv_result = result;
    }
    ExFreePool( msg );
    if (list_empty( &queue->msg_list[SEND_MESSAGE] )) clear_queue_bits( queue, QS_SENDMESSAGE );
}

/* set the result of the current received message */
static void reply_message( struct msg_queue *queue, lparam_t result,
                           unsigned int error, int remove, const void *data, data_size_t len )
{
    struct message_result *res = queue->recv_result;

    if (remove)
    {
        queue->recv_result = res->recv_next;
        res->receiver = NULL;
        if (!res->sender)  /* no one waiting for it */
        {
            free_result( res );
            return;
        }
    }
    if (!res->replied)
    {
        if (len && (res->data = memdup( data, len ))) res->data_size = len;
        store_message_result( res, result, error );
    }
}

static int match_window( user_handle_t win, user_handle_t msg_win )
{
    if (!win) return 1;
    if (win == -1 || win == 1) return !msg_win;
    if (msg_win == win) return 1;
    return is_child_window( win, msg_win );
}

/* retrieve a posted message */
static int get_posted_message( void *req, struct msg_queue *queue, user_handle_t win,
                               unsigned int first, unsigned int last, unsigned int flags,
                               struct get_message_reply *reply )
{
    struct message *msg;

    /* check against the filters */
    LIST_FOR_EACH_ENTRY( msg, &queue->msg_list[POST_MESSAGE], struct message, entry )
    {
        if (!match_window( win, msg->win )) continue;
        if (!check_msg_filter( msg->msg, first, last )) continue;
        goto found; /* found one */
    }
    return 0;

    /* return it to the app */
found:
    reply->total = msg->data_size;
    if (msg->data_size > get_reply_max_size(req))
    {
        set_error( STATUS_BUFFER_OVERFLOW );
        return 1;
    }
    reply->type   = msg->type;
    reply->win    = msg->win;
    reply->msg    = msg->msg;
    reply->wparam = msg->wparam;
    reply->lparam = msg->lparam;
    reply->time   = msg->time;

    if (flags & PM_REMOVE)
    {
        if (msg->data)
        {
            set_reply_data_ptr( (void*)req, msg->data, msg->data_size );
            msg->data = NULL;
            msg->data_size = 0;
        }
        remove_queue_message( queue, msg, POST_MESSAGE );
    }
    else if (msg->data) set_reply_data( (void*)req, msg->data, msg->data_size );

    return 1;
}

static int get_quit_message( struct msg_queue *queue, unsigned int flags,
                             struct get_message_reply *reply )
{
    if (queue->quit_message)
    {
        reply->total  = 0;
        reply->type   = MSG_POSTED;
        reply->win    = 0;
        reply->msg    = WM_QUIT;
        reply->wparam = queue->exit_code;
        reply->lparam = 0;
        reply->time   = EngGetTickCount();

        if (flags & PM_REMOVE)
        {
            queue->quit_message = 0;
            if (list_empty( &queue->msg_list[POST_MESSAGE] ))
                clear_queue_bits( queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
        }
        return 1;
    }
    else
        return 0;
}

/* empty a message list and free all the messages */
static void empty_msg_list( struct list *list )
{
    struct list *ptr;

    while ((ptr = list_head( list )) != NULL)
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        list_remove( &msg->entry );
        free_message( msg );
    }
}

/* cleanup all pending results when deleting a queue */
static void cleanup_results( struct msg_queue *queue )
{
    struct list *entry;

    while ((entry = list_head( &queue->send_result )) != NULL)
    {
        remove_result_from_sender( LIST_ENTRY( entry, struct message_result, sender_entry ) );
    }

    while ((entry = list_head( &queue->callback_result )) != NULL)
    {
        remove_result_from_sender( LIST_ENTRY( entry, struct message_result, sender_entry ) );
    }

    while (queue->recv_result)
        reply_message( queue, 0, STATUS_ACCESS_DENIED /*FIXME*/, 1, NULL, 0 );
}

/* check if the thread owning the queue is hung (not checking for messages) */
static int is_queue_hung( struct msg_queue *queue )
{
    //struct wait_queue_entry *entry;
    timeout_t current_time;

    get_current_time(&current_time);

    if (current_time - queue->last_get_msg <= 5 * TICKS_PER_SEC)
        return 0;  /* less than 5 seconds since last get message -> not hung */

#if 0
    LIST_FOR_EACH_ENTRY( entry, &queue->obj.wait_queue, struct wait_queue_entry, entry )
    {
        if (entry->thread->queue == queue)
            return 0;  /* thread is waiting on queue -> not hung */
    }
#endif
    DPRINT1("TODO: Check if anyone is waiting on queue is missing!\n");

    return 1;
}

static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    PPROCESSINFO process = entry->thread->process;

    /* a thread can only wait on its own queue */
    if (entry->thread->queue != queue)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if (process->idle_event && !(queue->wake_mask & QS_SMRESULT)) set_event( process->idle_event );

    if (queue->fd && list_empty( &obj->wait_queue ))  /* first on the queue */
        set_fd_events( queue->fd, POLLIN );
    add_queue( obj, entry );
    return 1;
}

static void msg_queue_remove_queue(struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    remove_queue( obj, entry );
    if (queue->fd && list_empty( &obj->wait_queue ))  /* last on the queue is gone */
        set_fd_events( queue->fd, 0 );
}

static void msg_queue_dump( struct object *obj, int verbose )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    DbgPrint( "Msg queue bits=%x mask=%x\n",
             queue->wake_bits, queue->wake_mask );
}

static int msg_queue_signaled( struct object *obj, PTHREADINFO thread )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    int ret = 0;

    if (queue->fd)
    {
        if ((ret = check_fd_events( queue->fd, POLLIN )))
            /* stop waiting on select() if we are signaled */
            set_fd_events( queue->fd, 0 );
        else if (!list_empty( &obj->wait_queue ))
            /* restart waiting on poll() if we are no longer signaled */
            set_fd_events( queue->fd, POLLIN );
    }
    return ret || is_signaled( queue );
}

static int msg_queue_satisfied( struct object *obj, PTHREADINFO thread )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    queue->wake_mask = 0;
    queue->changed_mask = 0;
    return 0;  /* Not abandoned */
}

static void msg_queue_destroy( struct object *obj )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct list *ptr;
    int i;

    cleanup_results( queue );
    for (i = 0; i < NB_MSG_KINDS; i++) empty_msg_list( &queue->msg_list[i] );

    ObDereferenceObject(queue->wake_event_ptr);
    ZwClose(queue->wake_event);

    while ((ptr = list_head( &queue->pending_timers )))
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        list_remove( &timer->entry );
        ExFreePool( timer );
    }
    while ((ptr = list_head( &queue->expired_timers )))
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        list_remove( &timer->entry );
        ExFreePool( timer );
    }
    if (queue->timeout) remove_timeout_user( queue->timeout );
    if (queue->input)
    {
        queue->input->cursor_count -= queue->cursor_count;
        release_object( queue->input );
    }
    if (queue->hooks) release_object( queue->hooks );
    if (queue->fd) release_object( queue->fd );
}

static void msg_queue_poll_event( struct fd *fd, int event )
{
    struct msg_queue *queue = get_fd_user( fd );
    assert( queue->obj.ops == &msg_queue_ops );

    if (event & (POLLERR | POLLHUP)) set_fd_events( fd, -1 );
    else set_fd_events( queue->fd, 0 );
    KeSetEvent(queue->wake_event_ptr, EVENT_INCREMENT, FALSE);
}

static void thread_input_dump( struct object *obj, int verbose )
{
    struct thread_input *input = (struct thread_input *)obj;
    DbgPrint( "Thread input focus=%08x capture=%08x active=%08x\n",
             input->focus, input->capture, input->active );
}

static void thread_input_destroy( struct object *obj )
{
    struct thread_input *input = (struct thread_input *)obj;

    if (foreground_input == input) foreground_input = NULL;
    empty_msg_list( &input->msg_list );
    if (input->desktop) release_object( input->desktop );
}

/* fix the thread input data when a window is destroyed */
static inline void thread_input_cleanup_window( struct msg_queue *queue, user_handle_t window )
{
    struct thread_input *input = queue->input;

    if (window == input->focus) input->focus = 0;
    if (window == input->capture) input->capture = 0;
    if (window == input->active) input->active = 0;
    if (window == input->menu_owner) input->menu_owner = 0;
    if (window == input->move_size) input->move_size = 0;
    if (window == input->caret) set_caret_window( input, 0 );
}

/* check if the specified window can be set in the input data of a given queue */
static int check_queue_input_window( struct msg_queue *queue, user_handle_t window )
{
    PTHREADINFO thread;
    int ret = 0;

    if (!window) return 1;  /* we can always clear the data */

    if ((thread = get_window_thread( window )))
    {
        ret = (queue->input == thread->queue->input);
        if (!ret) set_error( STATUS_ACCESS_DENIED );
        ObDereferenceObject(thread->peThread);
    }
    else set_error( STATUS_INVALID_HANDLE );

    return ret;
}

/* make sure the specified thread has a queue */
int init_thread_queue( PTHREADINFO thread )
{
    if (thread->queue) return 1;
    return (create_msg_queue( thread, NULL ) != NULL);
}

/* attach two thread input data structures */
int attach_thread_input( PTHREADINFO thread_from, PTHREADINFO thread_to )
{
    struct desktop *desktop;
    struct thread_input *input;
    int ret;

    if (!thread_to->queue && !(thread_to->queue = create_msg_queue( thread_to, NULL ))) return 0;
    if (!(desktop = get_thread_desktop( thread_from, 0 ))) return 0;
    input = (struct thread_input *)grab_object( thread_to->queue->input );
    if (input->desktop != desktop)
    {
        set_error( STATUS_ACCESS_DENIED );
        release_object( input );
        release_object( desktop );
        return 0;
    }
    release_object( desktop );

    ret = assign_thread_input( thread_from, input );
    if (ret) memset( input->keystate, 0, sizeof(input->keystate) );
    release_object( input );
    return ret;
}

/* detach two thread input data structures */
void detach_thread_input( PTHREADINFO thread_from )
{
    struct thread_input *input;

    if ((input = create_thread_input( thread_from )))
    {
        assign_thread_input( thread_from, input );
        release_object( input );
    }
}


/* set the next timer to expire */
static void set_next_timer( struct msg_queue *queue )
{
    struct list *ptr;

    if (queue->timeout)
    {
        remove_timeout_user( queue->timeout );
        queue->timeout = NULL;
    }
    if ((ptr = list_head( &queue->pending_timers )))
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        queue->timeout = add_timeout_user( timer->when, timer_callback, queue );
    }
    /* set/clear QS_TIMER bit */
    if (list_empty( &queue->expired_timers ))
        clear_queue_bits( queue, QS_TIMER );
    else
        set_queue_bits( queue, QS_TIMER );
}

/* find a timer from its window and id */
static struct timer *find_timer( struct msg_queue *queue, user_handle_t win,
                                 unsigned int msg, lparam_t id )
{
    struct list *ptr;

    /* we need to search both lists */

    LIST_FOR_EACH( ptr, &queue->pending_timers )
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win && timer->msg == msg && timer->id == id) return timer;
    }
    LIST_FOR_EACH( ptr, &queue->expired_timers )
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win && timer->msg == msg && timer->id == id) return timer;
    }
    return NULL;
}

/* callback for the next timer expiration */
static void timer_callback( void *private )
{
    struct list *ptr;
    struct msg_queue *queue = private;

    queue->timeout = NULL;
    /* move on to the next timer */
    ptr = list_head( &queue->pending_timers );
    list_remove( ptr );
    list_add_tail( &queue->expired_timers, ptr );
    set_next_timer( queue );
}

/* link a timer at its rightful place in the queue list */
static void link_timer( struct msg_queue *queue, struct timer *timer )
{
    struct list *ptr;

    for (ptr = queue->pending_timers.next; ptr != &queue->pending_timers; ptr = ptr->next)
    {
        struct timer *t = LIST_ENTRY( ptr, struct timer, entry );
        if (t->when >= timer->when) break;
    }
    list_add_before( ptr, &timer->entry );
}

/* remove a timer from the queue timer list and free it */
static void free_timer( struct msg_queue *queue, struct timer *timer )
{
    list_remove( &timer->entry );
    ExFreePool( timer );
    set_next_timer( queue );
}

/* restart an expired timer */
static void restart_timer( struct msg_queue *queue, struct timer *timer )
{
    timeout_t current_time;
    get_current_time(&current_time);

    list_remove( &timer->entry );
    while (timer->when <= current_time) timer->when += (timeout_t)timer->rate * 10000;
    link_timer( queue, timer );
    set_next_timer( queue );
}

/* find an expired timer matching the filtering parameters */
static struct timer *find_expired_timer( struct msg_queue *queue, user_handle_t win,
                                         unsigned int get_first, unsigned int get_last,
                                         int remove )
{
    struct list *ptr;

    LIST_FOR_EACH( ptr, &queue->expired_timers )
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (win && timer->win != win) continue;
        if (check_msg_filter( timer->msg, get_first, get_last ))
        {
            if (remove) restart_timer( queue, timer );
            return timer;
        }
    }
    return NULL;
}

/* add a timer */
static struct timer *set_timer( struct msg_queue *queue, unsigned int rate )
{
    timeout_t current_time;
    struct timer *timer;
    get_current_time(&current_time);

    timer = mem_alloc( sizeof(*timer) );
    if (timer)
    {
        timer->rate = max( rate, 1 );
        timer->when = current_time + (timeout_t)timer->rate * 10000;
        link_timer( queue, timer );
        /* check if we replaced the next timer */
        if (list_head( &queue->pending_timers ) == &timer->entry) set_next_timer( queue );
    }
    return timer;
}

/* change the input key state for a given key */
static void set_input_key_state( struct thread_input *input, unsigned char key, int down )
{
    if (down)
    {
        if (!(input->keystate[key] & 0x80)) input->keystate[key] ^= 0x01;
        input->keystate[key] |= 0x80;
    }
    else input->keystate[key] &= ~0x80;
}

/* update the input key state for a keyboard message */
static void update_input_key_state( struct thread_input *input, const struct message *msg )
{
    unsigned char key;
    int down = 0;

    switch (msg->msg)
    {
    case WM_LBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_LBUTTONUP:
        set_input_key_state( input, VK_LBUTTON, down );
        break;
    case WM_MBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_MBUTTONUP:
        set_input_key_state( input, VK_MBUTTON, down );
        break;
    case WM_RBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_RBUTTONUP:
        set_input_key_state( input, VK_RBUTTON, down );
        break;
    case WM_XBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_XBUTTONUP:
        if (msg->wparam == XBUTTON1) set_input_key_state( input, VK_XBUTTON1, down );
        else if (msg->wparam == XBUTTON2) set_input_key_state( input, VK_XBUTTON2, down );
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = 1;
        /* fall through */
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = (unsigned char)msg->wparam;
        set_input_key_state( input, key, down );
        switch(key)
        {
        case VK_LCONTROL:
        case VK_RCONTROL:
            down = (input->keystate[VK_LCONTROL] | input->keystate[VK_RCONTROL]) & 0x80;
            set_input_key_state( input, VK_CONTROL, down );
            break;
        case VK_LMENU:
        case VK_RMENU:
            down = (input->keystate[VK_LMENU] | input->keystate[VK_RMENU]) & 0x80;
            set_input_key_state( input, VK_MENU, down );
            break;
        case VK_LSHIFT:
        case VK_RSHIFT:
            down = (input->keystate[VK_LSHIFT] | input->keystate[VK_RSHIFT]) & 0x80;
            set_input_key_state( input, VK_SHIFT, down );
            break;
        }
        break;
    }
}

/* release the hardware message currently being processed by the given thread */
static void release_hardware_message( struct msg_queue *queue, unsigned int hw_id,
                                      int remove, user_handle_t new_win )
{
    struct thread_input *input = queue->input;
    struct message *msg;

    LIST_FOR_EACH_ENTRY( msg, &input->msg_list, struct message, entry )
    {
        if (msg->unique_id == hw_id) break;
    }
    if (&msg->entry == &input->msg_list) return;  /* not found */

    /* clear the queue bit for that message */
    if (remove || new_win)
    {
        struct message *other;
        int clr_bit;

        clr_bit = get_hardware_msg_bit( msg );
        LIST_FOR_EACH_ENTRY( other, &input->msg_list, struct message, entry )
        {
            if (other != msg && get_hardware_msg_bit( other ) == clr_bit)
            {
                clr_bit = 0;
                break;
            }
        }
        if (clr_bit) clear_queue_bits( queue, clr_bit );
    }

    if (new_win)  /* set the new window */
    {
        PTHREADINFO owner = get_window_thread( new_win );
        if (owner)
        {
            msg->win = new_win;
            if (owner->queue->input != input)
            {
                list_remove( &msg->entry );
                if (msg->msg == WM_MOUSEMOVE && merge_message( owner->queue->input, msg ))
                {
                    free_message( msg );
                    ObDereferenceObject(owner->peThread);
                    return;
                }
                list_add_tail( &owner->queue->input->msg_list, &msg->entry );
            }
            set_queue_bits( owner->queue, get_hardware_msg_bit( msg ));
            remove = 0;
            ObDereferenceObject(owner->peThread);
        }
    }
    if (remove)
    {
        update_input_key_state( input, msg );
        list_remove( &msg->entry );
        free_message( msg );
    }
}

/* find the window that should receive a given hardware message */
static user_handle_t find_hardware_message_window( struct thread_input *input, struct message *msg,
                                                   struct hardware_msg_data *data, unsigned int *msg_code )
{
    user_handle_t win = 0;

    *msg_code = msg->msg;
    if (is_keyboard_msg( msg ))
    {
        if (input && !(win = input->focus))
        {
            win = input->active;
            if (*msg_code < WM_SYSKEYDOWN) *msg_code += WM_SYSKEYDOWN - WM_KEYDOWN;
        }
    }
    else  /* mouse message */
    {
        if (!input || !(win = input->capture))
        {
            if (!(win = msg->win) || !is_window_visible( win ) || is_window_transparent( win ))
            {
                if (input) win = window_from_point( input->desktop, data->x, data->y );
            }
        }
    }
    return win;
}

/* queue a hardware message into a given thread input */
static void queue_hardware_message( struct thread_input *input, struct message *msg,
                                    struct hardware_msg_data *data )
{
    user_handle_t win;
    PTHREADINFO thread;
    unsigned int msg_code;

    last_input_time = EngGetTickCount();
    win = find_hardware_message_window( input, msg, data, &msg_code );
    if (!win || !(thread = get_window_thread(win)))
    {
        if (input) update_input_key_state( input, msg );
        ExFreePool( msg );
        return;
    }
    input = thread->queue->input;

    if (msg->msg == WM_MOUSEMOVE && merge_message( input, msg ))
        ExFreePool( msg );
    else
    {
        msg->unique_id = 0;  /* will be set once we return it to the app */
        list_add_tail( &input->msg_list, &msg->entry );
        set_queue_bits( thread->queue, get_hardware_msg_bit(msg) );
    }
    ObDereferenceObject(thread->peThread);
}

/* check message filter for a hardware message */
static int check_hw_message_filter( user_handle_t win, unsigned int msg_code,
                                    user_handle_t filter_win, unsigned int first, unsigned int last )
{
    if (msg_code >= WM_KEYFIRST && msg_code <= WM_KEYLAST)
    {
        /* we can only test the window for a keyboard message since the
         * dest window for a mouse message depends on hittest */
        if (filter_win && win != filter_win && !is_child_window( filter_win, win ))
            return 0;
        /* the message code is final for a keyboard message, we can simply check it */
        return check_msg_filter( msg_code, first, last );
    }
    else  /* mouse message */
    {
        /* we need to check all possible values that the message can have in the end */

        if (check_msg_filter( msg_code, first, last )) return 1;
        if (msg_code == WM_MOUSEWHEEL) return 0;  /* no other possible value for this one */

        /* all other messages can become non-client messages */
        if (check_msg_filter( msg_code + (WM_NCMOUSEFIRST - WM_MOUSEFIRST), first, last )) return 1;

        /* clicks can become double-clicks or non-client double-clicks */
        if (msg_code == WM_LBUTTONDOWN || msg_code == WM_MBUTTONDOWN ||
            msg_code == WM_RBUTTONDOWN || msg_code == WM_XBUTTONDOWN)
        {
            if (check_msg_filter( msg_code + (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN), first, last )) return 1;
            if (check_msg_filter( msg_code + (WM_NCLBUTTONDBLCLK - WM_LBUTTONDOWN), first, last )) return 1;
        }
        return 0;
    }
}


/* find a hardware message for the given queue */
static int get_hardware_message( void *req, PTHREADINFO thread, unsigned int hw_id, user_handle_t filter_win,
                                 unsigned int first, unsigned int last, struct get_message_reply *reply )
{
    struct thread_input *input = thread->queue->input;
    PTHREADINFO win_thread;
    struct list *ptr;
    user_handle_t win;
    int clear_bits, got_one = 0;
    unsigned int msg_code;

    ptr = list_head( &input->msg_list );
    if (hw_id)
    {
        while (ptr)
        {
            struct message *msg = LIST_ENTRY( ptr, struct message, entry );
            if (msg->unique_id == hw_id) break;
            ptr = list_next( &input->msg_list, ptr );
        }
        if (!ptr) ptr = list_head( &input->msg_list );
        else ptr = list_next( &input->msg_list, ptr );  /* start from the next one */
    }

    if (ptr == list_head( &input->msg_list ))
        clear_bits = QS_KEY | QS_MOUSEMOVE | QS_MOUSEBUTTON;
    else
        clear_bits = 0;  /* don't clear bits if we don't go through the whole list */

    while (ptr)
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        struct hardware_msg_data *data = msg->data;

        ptr = list_next( &input->msg_list, ptr );
        win = find_hardware_message_window( input, msg, data, &msg_code );
        if (!win || !(win_thread = get_window_thread( win )))
        {
            /* no window at all, remove it */
            update_input_key_state( input, msg );
            list_remove( &msg->entry );
            free_message( msg );
            continue;
        }
        if (win_thread != thread)
        {
            if (win_thread->queue->input == input)
            {
                /* wake the other thread */
                set_queue_bits( win_thread->queue, get_hardware_msg_bit(msg) );
                got_one = 1;
            }
            else
            {
                /* for another thread input, drop it */
                update_input_key_state( input, msg );
                list_remove( &msg->entry );
                free_message( msg );
            }
            ObDereferenceObject(win_thread->peThread);
            continue;
        }
        ObDereferenceObject(win_thread->peThread);

        /* if we already got a message for another thread, or if it doesn't
         * match the filter we skip it */
        if (got_one || !check_hw_message_filter( win, msg_code, filter_win, first, last ))
        {
            clear_bits &= ~get_hardware_msg_bit( msg );
            continue;
        }
        /* now we can return it */
        if (!msg->unique_id) msg->unique_id = get_unique_id();
        reply->type   = MSG_HARDWARE;
        reply->win    = win;
        reply->msg    = msg_code;
        reply->wparam = msg->wparam;
        reply->lparam = msg->lparam;
        reply->time   = msg->time;

        data->hw_id = msg->unique_id;
        set_reply_data( req, msg->data, msg->data_size );
        return 1;
    }
    /* nothing found, clear the hardware queue bits */
    clear_queue_bits( thread->queue, clear_bits );
    return 0;
}

/* increment (or decrement if 'incr' is negative) the queue paint count */
void inc_queue_paint_count( PTHREADINFO thread, int incr )
{
    struct msg_queue *queue = thread->queue;

    assert( queue );

    if ((queue->paint_count += incr) < 0) queue->paint_count = 0;

    if (queue->paint_count)
        set_queue_bits( queue, QS_PAINT );
    else
        clear_queue_bits( queue, QS_PAINT );
}


/* remove all messages and timers belonging to a certain window */
void queue_cleanup_window( PTHREADINFO thread, user_handle_t win )
{
    struct msg_queue *queue = thread->queue;
    struct list *ptr;
    int i;

    if (!queue) return;

    /* remove timers */

    ptr = list_head( &queue->pending_timers );
    while (ptr)
    {
        struct list *next = list_next( &queue->pending_timers, ptr );
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win) free_timer( queue, timer );
        ptr = next;
    }
    ptr = list_head( &queue->expired_timers );
    while (ptr)
    {
        struct list *next = list_next( &queue->expired_timers, ptr );
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win) free_timer( queue, timer );
        ptr = next;
    }

    /* remove messages */
    for (i = 0; i < NB_MSG_KINDS; i++)
    {
        struct list *ptr, *next;

        LIST_FOR_EACH_SAFE( ptr, next, &queue->msg_list[i] )
        {
            struct message *msg = LIST_ENTRY( ptr, struct message, entry );
            if (msg->win == win) remove_queue_message( queue, msg, i );
        }
    }

    thread_input_cleanup_window( queue, win );
}

/* post a message to a window; used by socket handling */
void post_message( user_handle_t win, unsigned int message, lparam_t wparam, lparam_t lparam )
{
    struct message *msg;
    PTHREADINFO thread = get_window_thread( win );

    if (!thread) return;

    if (thread->queue && (msg = ExAllocatePool( PagedPool, sizeof(*msg) )))
    {
        msg->type      = MSG_POSTED;
        msg->win       = get_user_full_handle( win );
        msg->msg       = message;
        msg->wparam    = wparam;
        msg->lparam    = lparam;
        msg->time      = EngGetTickCount();
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = 0;

        list_add_tail( &thread->queue->msg_list[POST_MESSAGE], &msg->entry );
        set_queue_bits( thread->queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
    }
    ObDereferenceObject(thread->peThread);
}

/* post a win event */
void post_win_event( PTHREADINFO thread, unsigned int event,
                     user_handle_t win, unsigned int object_id,
                     unsigned int child_id, client_ptr_t hook_proc,
                     const WCHAR *module, data_size_t module_size,
                     user_handle_t hook)
{
    struct message *msg;

    if (thread->queue && (msg = ExAllocatePool( PagedPool, sizeof(*msg) )))
    {
        struct winevent_msg_data *data;

        msg->type      = MSG_WINEVENT;
        msg->win       = get_user_full_handle( win );
        msg->msg       = event;
        msg->wparam    = object_id;
        msg->lparam    = child_id;
        msg->time      = EngGetTickCount();
        msg->result    = NULL;

        if ((data = ExAllocatePool( PagedPool, sizeof(*data) + module_size )))
        {
            data->hook = hook;
            data->tid  = get_thread_id( (PTHREADINFO)PsGetCurrentThreadWin32Thread() );
            data->hook_proc = hook_proc;
            memcpy( data + 1, module, module_size );

            msg->data = data;
            msg->data_size = sizeof(*data) + module_size;

            if (debug_level > 1)
                DPRINT1( "post_win_event: tid %04x event %04x win %08x object_id %d child_id %d\n",
                         get_thread_id(thread), event, win, object_id, child_id );
            list_add_tail( &thread->queue->msg_list[SEND_MESSAGE], &msg->entry );
            set_queue_bits( thread->queue, QS_SENDMESSAGE );
        }
        else
            ExFreePool( msg );
    }
}


/* check if the thread owning the window is hung */
DECL_HANDLER(is_window_hung)
{
    PTHREADINFO thread;

    thread = get_window_thread( req->win );
    if (thread)
    {
        reply->is_hung = is_queue_hung( thread->queue );
        ObDereferenceObject(thread->peThread);
    }
    else reply->is_hung = 0;
}


/* get the message queue of the current thread */
DECL_HANDLER(get_msg_queue)
{
    struct msg_queue *queue = get_current_queue();

    reply->handle = 0;
    //if (queue) reply->handle = alloc_handle( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), queue, SYNCHRONIZE, 0 );
    if (queue) reply->handle = (obj_handle_t)queue->wake_event;
}


/* set the file descriptor associated to the current thread queue */
DECL_HANDLER(set_queue_fd)
{
#if 0
    struct msg_queue *queue = get_current_queue();
    struct file *file;
    int unix_fd;

    if (queue->fd)  /* fd can only be set once */
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (!(file = get_file_obj( (PPROCESSINFO)PsGetCurrentProcessWin32Process(), req->handle, SYNCHRONIZE ))) return;

    if ((unix_fd = get_file_unix_fd( file )) != -1)
    {
        if ((unix_fd = dup( unix_fd )) != -1)
            queue->fd = create_anonymous_fd( &msg_queue_fd_ops, unix_fd, &queue->obj, 0 );
        else
            file_set_error();
    }
    release_object( file );
#else
    UNIMPLEMENTED;
#endif
}


/* set the current message queue wakeup mask */
DECL_HANDLER(set_queue_mask)
{
    struct msg_queue *queue = get_current_queue();

    if (queue)
    {
        queue->wake_mask    = req->wake_mask;
        queue->changed_mask = req->changed_mask;
        reply->wake_bits    = queue->wake_bits;
        reply->changed_bits = queue->changed_bits;
        if (is_signaled( queue ))
        {
            /* if skip wait is set, do what would have been done in the subsequent wait */
            if (req->skip_wait)
            {
                msg_queue_satisfied( &queue->obj, PsGetCurrentThreadWin32Thread() );
            }
            else
            {
                KeSetEvent(queue->wake_event_ptr, EVENT_INCREMENT, FALSE);
            }
        }
    }
}


/* get the current message queue status */
DECL_HANDLER(get_queue_status)
{
    PTHREADINFO current = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    struct msg_queue *queue = current->queue;
    if (queue)
    {
        reply->wake_bits    = queue->wake_bits;
        reply->changed_bits = queue->changed_bits;
        if (req->clear) queue->changed_bits = 0;
    }
    else reply->wake_bits = reply->changed_bits = 0;
}


/* send a message to a thread queue */
DECL_HANDLER(send_message)
{
    struct message *msg;
    struct msg_queue *send_queue = get_current_queue();
    struct msg_queue *recv_queue = NULL;
    PETHREAD ethread = NULL;
    PTHREADINFO thread = NULL;
    NTSTATUS status;

    status = PsLookupThreadByThreadId((HANDLE)req->id, &ethread);
    if (!NT_SUCCESS(status)) return;
    if (!(thread = PsGetThreadWin32Thread(ethread)))
    {
        ObDereferenceObject(ethread);
        return;
    }

    if (!(recv_queue = thread->queue))
    {
        set_error( STATUS_INVALID_PARAMETER );
        ObDereferenceObject(thread->peThread);
        return;
    }
    if ((req->flags & SEND_MSG_ABORT_IF_HUNG) && is_queue_hung(recv_queue))
    {
        set_error( STATUS_TIMEOUT );
        ObDereferenceObject(thread->peThread);
        return;
    }

    if ((msg = ExAllocatePool( PagedPool, sizeof(*msg) )))
    {
        msg->type      = req->type;
        msg->win       = get_user_full_handle( req->win );
        msg->msg       = req->msg;
        msg->wparam    = req->wparam;
        msg->lparam    = req->lparam;
        msg->time      = EngGetTickCount();
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = get_req_data_size((void*)req);

        if (msg->data_size && !(msg->data = memdup( get_req_data(), msg->data_size )))
        {
            ExFreePool( msg );
            ObDereferenceObject(thread->peThread);
            return;
        }

        switch(msg->type)
        {
        case MSG_OTHER_PROCESS:
        case MSG_ASCII:
        case MSG_UNICODE:
        case MSG_CALLBACK:
            if (!(msg->result = alloc_message_result( send_queue, recv_queue, msg, req->timeout )))
            {
                free_message( msg );
                break;
            }
            /* fall through */
        case MSG_NOTIFY:
            list_add_tail( &recv_queue->msg_list[SEND_MESSAGE], &msg->entry );
            set_queue_bits( recv_queue, QS_SENDMESSAGE );
            break;
        case MSG_POSTED:
            list_add_tail( &recv_queue->msg_list[POST_MESSAGE], &msg->entry );
            set_queue_bits( recv_queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
            break;
        case MSG_HARDWARE:  /* should use send_hardware_message instead */
        case MSG_CALLBACK_RESULT:  /* cannot send this one */
        default:
            set_error( STATUS_INVALID_PARAMETER );
            ExFreePool( msg );
            break;
        }
    }
    ObDereferenceObject(thread->peThread);
}

/* send a hardware message to a thread queue */
DECL_HANDLER(send_hardware_message)
{
    struct message *msg;
    PETHREAD ethread = NULL;
    PTHREADINFO thread = NULL;
    struct hardware_msg_data *data;
    struct thread_input *input = foreground_input;
    NTSTATUS status;

    if (req->id)
    {
        status = PsLookupThreadByThreadId((HANDLE)req->id, &ethread);
        if (!NT_SUCCESS(status)) return;

        if (!(thread = PsGetThreadWin32Thread(ethread)))
        {
            ObDereferenceObject(ethread);
            return;
        }
        if (!thread->queue)
        {
            set_error( STATUS_INVALID_PARAMETER );
            ObDereferenceObject(ethread);
            return;
        }
        input = thread->queue->input;
        reply->cursor = input->cursor;
        reply->count  = input->cursor_count;
    }

    if (!req->msg || !(data = mem_alloc( sizeof(*data) )))
    {
        if (thread) ObDereferenceObject(thread->peThread);
        return;
    }
    memset( data, 0, sizeof(*data) );
    data->x    = req->x;
    data->y    = req->y;
    data->info = req->info;

    if ((msg = ExAllocatePool( PagedPool, sizeof(*msg) )))
    {
        msg->type      = MSG_HARDWARE;
        msg->win       = get_user_full_handle( req->win );
        msg->msg       = req->msg;
        msg->wparam    = req->wparam;
        msg->lparam    = req->lparam;
        msg->time      = req->time;
        msg->result    = NULL;
        msg->data      = data;
        msg->data_size = sizeof(*data);
        queue_hardware_message( input, msg, data );
    }
    else ExFreePool( data );

    if (thread) ObDereferenceObject( thread->peThread );
}

/* post a quit message to the current queue */
DECL_HANDLER(post_quit_message)
{
    struct msg_queue *queue = get_current_queue();

    if (!queue)
        return;

    queue->quit_message = 1;
    queue->exit_code = req->exit_code;
    set_queue_bits( queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
}

/* get a message from the current queue */
DECL_HANDLER(get_message)
{
    timeout_t current_time;
    struct timer *timer;
    struct list *ptr;
    PPROCESSINFO process = PsGetCurrentProcessWin32Process();
    struct msg_queue *queue = get_current_queue();
    user_handle_t get_win = get_user_full_handle( req->get_win );
    unsigned int filter = req->flags >> 16;

    reply->active_hooks = get_active_hooks();

    if (!queue) return;
    get_current_time(&current_time);
    queue->last_get_msg = current_time;
    if (!filter) filter = QS_ALLINPUT;

    /* first check for sent messages */
    if ((ptr = list_head( &queue->msg_list[SEND_MESSAGE] )))
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        receive_message( (void*)req, queue, msg, reply );
        return;
    }

    /* clear changed bits so we can wait on them if we don't find a message */
    if (filter & QS_POSTMESSAGE)
    {
        queue->changed_bits &= ~(QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER);
        if (req->get_first == 0 && req->get_last == ~0U) queue->changed_bits &= ~QS_ALLPOSTMESSAGE;
    }
    if (filter & QS_INPUT) queue->changed_bits &= ~QS_INPUT;
    if (filter & QS_PAINT) queue->changed_bits &= ~QS_PAINT;

    /* then check for posted messages */
    if ((filter & QS_POSTMESSAGE) &&
        get_posted_message( (void*)req, queue, get_win, req->get_first, req->get_last, req->flags, reply ))
        return;

    /* only check for quit messages if not posted messages pending.
     * note: the quit message isn't filtered */
    if (get_quit_message( queue, req->flags, reply ))
        return;

    /* then check for any raw hardware message */
    if ((filter & QS_INPUT) &&
        filter_contains_hw_range( req->get_first, req->get_last ) &&
        get_hardware_message( (void*)req, (PTHREADINFO)PsGetCurrentThreadWin32Thread(), req->hw_id, get_win, req->get_first, req->get_last, reply ))
        return;

    /* now check for WM_PAINT */
    if ((filter & QS_PAINT) &&
        queue->paint_count &&
        check_msg_filter( WM_PAINT, req->get_first, req->get_last ) &&
        (reply->win = find_window_to_repaint( get_win, (PTHREADINFO)PsGetCurrentThreadWin32Thread() )))
    {
        reply->type   = MSG_POSTED;
        reply->msg    = WM_PAINT;
        reply->wparam = 0;
        reply->lparam = 0;
        reply->time   = EngGetTickCount();
        return;
    }

    /* now check for timer */
    if ((filter & QS_TIMER) &&
        (timer = find_expired_timer( queue, get_win, req->get_first,
                                     req->get_last, (req->flags & PM_REMOVE) )))
    {
        reply->type   = MSG_POSTED;
        reply->win    = timer->win;
        reply->msg    = timer->msg;
        reply->wparam = timer->id;
        reply->lparam = timer->lparam;
        reply->time   = EngGetTickCount();
        if (!(req->flags & PM_NOYIELD) && process->idle_event)
            set_event( process->idle_event );
        return;
    }

    if (get_win == -1 && process->idle_event) set_event( process->idle_event );
    queue->wake_mask = req->wake_mask;
    queue->changed_mask = req->changed_mask;
    set_error( STATUS_PENDING );  /* FIXME */
}


/* reply to a sent message */
DECL_HANDLER(reply_message)
{
    PTHREADINFO current = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    if (!current->queue) set_error( STATUS_ACCESS_DENIED );
    else if (current->queue->recv_result)
        reply_message( current->queue, req->result, 0, req->remove,
                       get_req_data(), get_req_data_size((void*)req) );
}


/* accept the current hardware message */
DECL_HANDLER(accept_hardware_message)
{
    PTHREADINFO current = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    if (current->queue)
        release_hardware_message( current->queue, req->hw_id, req->remove, req->new_win );
    else
        set_error( STATUS_ACCESS_DENIED );
}


/* retrieve the reply for the last message sent */
DECL_HANDLER(get_message_reply)
{
    struct message_result *result;
    struct list *entry;
    PTHREADINFO current = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    struct msg_queue *queue = current->queue;

    if (queue)
    {
        set_error( STATUS_PENDING );
        reply->result = 0;

        if (!(entry = list_head( &queue->send_result ))) return;  /* no reply ready */

        result = LIST_ENTRY( entry, struct message_result, sender_entry );
        if (result->replied || req->cancel)
        {
            if (result->replied)
            {
                reply->result = result->result;
                set_error( result->error );
                if (result->data)
                {
                    data_size_t data_len = min( result->data_size, get_reply_max_size((void*)req) );
                    set_reply_data_ptr( (void*)req, result->data, data_len );
                    result->data = NULL;
                    result->data_size = 0;
                }
            }
            remove_result_from_sender( result );

            entry = list_head( &queue->send_result );
            if (!entry) clear_queue_bits( queue, QS_SMRESULT );
            else
            {
                result = LIST_ENTRY( entry, struct message_result, sender_entry );
                if (!result->replied) clear_queue_bits( queue, QS_SMRESULT );
            }
        }
    }
    else set_error( STATUS_ACCESS_DENIED );
}


/* set a window timer */
DECL_HANDLER(set_win_timer)
{
    struct timer *timer;
    struct msg_queue *queue;
    PTHREADINFO thread = NULL;
    user_handle_t win = 0;
    lparam_t id = req->id;

    if (req->win)
    {
        if (!(win = get_user_full_handle( req->win )) || !(thread = get_window_thread( win )))
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
        if (thread->process != ((PTHREADINFO)PsGetCurrentThreadWin32Thread())->process)
        {
            ObDereferenceObject(thread->peThread);
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
        queue = thread->queue;
        /* remove it if it existed already */
        if ((timer = find_timer( queue, win, req->msg, id ))) free_timer( queue, timer );
    }
    else
    {
        queue = get_current_queue();
        /* look for a timer with this id */
        if (id && (timer = find_timer( queue, 0, req->msg, id )))
        {
            /* free and reuse id */
            free_timer( queue, timer );
        }
        else
        {
            /* find a free id for it */
            do
            {
                id = queue->next_timer_id;
                if (--queue->next_timer_id <= 0x100) queue->next_timer_id = 0x7fff;
            }
            while (find_timer( queue, 0, req->msg, id ));
        }
    }

    if ((timer = set_timer( queue, req->rate )))
    {
        timer->win    = win;
        timer->msg    = req->msg;
        timer->id     = id;
        timer->lparam = req->lparam;
        reply->id     = id;
    }
    if (thread) ObDereferenceObject(thread->peThread);
}

/* kill a window timer */
DECL_HANDLER(kill_win_timer)
{
    struct timer *timer;
    PTHREADINFO thread;
    user_handle_t win = 0;

    if (req->win)
    {
        if (!(win = get_user_full_handle( req->win )) || !(thread = get_window_thread( win )))
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
        if (thread->process != ((PTHREADINFO)PsGetCurrentThreadWin32Thread())->process)
        {
            ObDereferenceObject(thread->peThread);
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
    }
    else
    {
        thread = PsGetCurrentThreadWin32Thread();
        ObReferenceObjectByPointer(thread->peThread, 0, NULL, KernelMode);
    }

    if (thread->queue && (timer = find_timer( thread->queue, win, req->msg, req->id )))
        free_timer( thread->queue, timer );
    else
        set_error( STATUS_INVALID_PARAMETER );

    ObDereferenceObject(thread->peThread);
}


/* attach (or detach) thread inputs */
DECL_HANDLER(attach_thread_input)
{
    PTHREADINFO thread_from, thread_to;
    PETHREAD ethread_from, ethread_to;
    NTSTATUS status;

    status = PsLookupThreadByThreadId((HANDLE)req->tid_from, &ethread_from);
    if (!NT_SUCCESS(status)) return;

    status = PsLookupThreadByThreadId((HANDLE)req->tid_to, &ethread_to);
    if (!NT_SUCCESS(status))
    {
        ObDereferenceObject(ethread_from);
        return;
    }

    thread_from = PsGetThreadWin32Thread(ethread_from);
    thread_to = PsGetThreadWin32Thread(ethread_to);

    if (!thread_from || !thread_to)
    {
        if (thread_from) ObDereferenceObject(thread_from->peThread);
        if (thread_to) ObDereferenceObject(thread_to->peThread);
        return;
    }
    if (thread_from != thread_to)
    {
        if (req->attach) attach_thread_input( thread_from, thread_to );
        else
        {
            if (thread_from->queue && thread_to->queue &&
                thread_from->queue->input == thread_to->queue->input)
                detach_thread_input( thread_from );
            else
                set_error( STATUS_ACCESS_DENIED );
        }
    }
    else set_error( STATUS_ACCESS_DENIED );
    ObDereferenceObject(thread_from->peThread);
    ObDereferenceObject(thread_to->peThread);
}


/* get thread input data */
DECL_HANDLER(get_thread_input)
{
    PTHREADINFO thread = NULL;
    struct thread_input *input;
    PETHREAD ethread = NULL;
    NTSTATUS status;

    if (req->tid)
    {
        status = PsLookupThreadByThreadId((HANDLE)req->tid, &ethread);
        if (!NT_SUCCESS(status)) return;
        if (!(thread = PsGetThreadWin32Thread(ethread)))
        {
            ObDereferenceObject(ethread);
            return;
        }
        input = thread->queue ? thread->queue->input : NULL;
    }
    else input = foreground_input;  /* get the foreground thread info */

    if (input)
    {
        reply->focus      = input->focus;
        reply->capture    = input->capture;
        reply->active     = input->active;
        reply->menu_owner = input->menu_owner;
        reply->move_size  = input->move_size;
        reply->caret      = input->caret;
        reply->cursor     = input->cursor;
        reply->show_count = input->cursor_count;
        reply->rect       = input->caret_rect;
    }

    /* foreground window is active window of foreground thread */
    reply->foreground = foreground_input ? foreground_input->active : 0;
    if (thread) ObDereferenceObject(thread->peThread);
}


/* retrieve queue keyboard state for a given thread */
DECL_HANDLER(get_key_state)
{
    PTHREADINFO thread;
    PETHREAD ethread;
    struct thread_input *input;
    NTSTATUS status;

    status = PsLookupThreadByThreadId((HANDLE)req->tid, &ethread);
    if (!NT_SUCCESS(status)) return;
    if (!(thread = PsGetThreadWin32Thread(ethread)))
    {
        ObDereferenceObject(ethread);
        return;
    }

    input = thread->queue ? thread->queue->input : NULL;
    if (input)
    {
        if (req->key >= 0) reply->state = input->keystate[req->key & 0xff];
        set_reply_data( (void*)req, input->keystate, min( get_reply_max_size((void*)req), sizeof(input->keystate) ));
    }
    ObDereferenceObject(thread->peThread);
}


/* set queue keyboard state for a given thread */
DECL_HANDLER(set_key_state)
{
    PTHREADINFO thread;
    PETHREAD ethread;
    struct thread_input *input;
    NTSTATUS status;

    status = PsLookupThreadByThreadId((HANDLE)req->tid, &ethread);
    if (!NT_SUCCESS(status)) return;
    if (!(thread = PsGetThreadWin32Thread(ethread)))
    {
        ObDereferenceObject(ethread);
        return;
    }

    input = thread->queue ? thread->queue->input : NULL;
    if (input)
    {
        data_size_t size = min( sizeof(input->keystate), get_req_data_size((void*)req) );
        if (size) memcpy( input->keystate, get_req_data(), size );
    }
    ObDereferenceObject(thread->peThread);
}


/* set the system foreground window */
DECL_HANDLER(set_foreground_window)
{
    PTHREADINFO thread;
    struct msg_queue *queue = get_current_queue();

    reply->previous = foreground_input ? foreground_input->active : 0;
    reply->send_msg_old = (reply->previous && foreground_input != queue->input);
    reply->send_msg_new = FALSE;

    if (is_top_level_window( req->handle ) &&
        ((thread = get_window_thread( req->handle ))))
    {
        foreground_input = thread->queue->input;
        reply->send_msg_new = (foreground_input != queue->input);
        ObDereferenceObject(thread->peThread);
    }
    else set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
}


/* set the current thread focus window */
DECL_HANDLER(set_focus_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        reply->previous = queue->input->focus;
        queue->input->focus = get_user_full_handle( req->handle );
    }
}


/* set the current thread active window */
DECL_HANDLER(set_active_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        if (!req->handle || make_window_active( req->handle ))
        {
            reply->previous = queue->input->active;
            queue->input->active = get_user_full_handle( req->handle );
        }
        else set_error( STATUS_INVALID_HANDLE );
    }
}


/* set the current thread capture window */
DECL_HANDLER(set_capture_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = reply->full_handle = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        struct thread_input *input = queue->input;

        /* if in menu mode, reject all requests to change focus, except if the menu bit is set */
        if (input->menu_owner && !(req->flags & CAPTURE_MENU))
        {
            set_error(STATUS_ACCESS_DENIED);
            return;
        }
        reply->previous = input->capture;
        input->capture = get_user_full_handle( req->handle );
        input->menu_owner = (req->flags & CAPTURE_MENU) ? input->capture : 0;
        input->move_size = (req->flags & CAPTURE_MOVESIZE) ? input->capture : 0;
        reply->full_handle = input->capture;
    }
}


/* Set the current thread caret window */
DECL_HANDLER(set_caret_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        struct thread_input *input = queue->input;

        reply->previous  = input->caret;
        reply->old_rect  = input->caret_rect;
        reply->old_hide  = input->caret_hide;
        reply->old_state = input->caret_state;

        set_caret_window( input, get_user_full_handle(req->handle) );
        input->caret_rect.right  = input->caret_rect.left + req->width;
        input->caret_rect.bottom = input->caret_rect.top + req->height;
    }
}


/* Set the current thread caret information */
DECL_HANDLER(set_caret_info)
{
    struct msg_queue *queue = get_current_queue();
    struct thread_input *input;

    if (!queue) return;
    input = queue->input;
    reply->full_handle = input->caret;
    reply->old_rect    = input->caret_rect;
    reply->old_hide    = input->caret_hide;
    reply->old_state   = input->caret_state;

    if (req->handle && get_user_full_handle(req->handle) != input->caret)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (req->flags & SET_CARET_POS)
    {
        input->caret_rect.right  += req->x - input->caret_rect.left;
        input->caret_rect.bottom += req->y - input->caret_rect.top;
        input->caret_rect.left = req->x;
        input->caret_rect.top  = req->y;
    }
    if (req->flags & SET_CARET_HIDE)
    {
        input->caret_hide += req->hide;
        if (input->caret_hide < 0) input->caret_hide = 0;
    }
    if (req->flags & SET_CARET_STATE)
    {
        if (req->state == -1) input->caret_state = !input->caret_state;
        else input->caret_state = !!req->state;
    }
}


/* get the time of the last input event */
DECL_HANDLER(get_last_input_time)
{
    reply->time = last_input_time;
}

/* set/get the current cursor */
DECL_HANDLER(set_cursor)
{
    struct msg_queue *queue = get_current_queue();
    struct thread_input *input;

    if (!queue) return;
    input = queue->input;

    reply->prev_handle = input->cursor;
    reply->prev_count  = input->cursor_count;

    if (req->flags & SET_CURSOR_HANDLE)
    {
        if (req->handle && !get_user_object( req->handle, USER_CLIENT ))
        {
            set_win32_error( ERROR_INVALID_CURSOR_HANDLE );
            return;
        }
        input->cursor = req->handle;
    }

    if (req->flags & SET_CURSOR_COUNT)
    {
        queue->cursor_count += req->show_count;
        input->cursor_count += req->show_count;
    }
}
