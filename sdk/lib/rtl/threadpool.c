/*
 * Thread pooling
 *
 * Copyright (c) 2006 Robert Shearman
 * Copyright (c) 2014-2016 Sebastian Lackner
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


#ifdef __REACTOS__
#include <rtl_vista.h>
#define NDEBUG
#include "wine/list.h"
#include <debug.h>

#define ERR(fmt, ...)    DPRINT1(fmt, ##__VA_ARGS__)
#define FIXME(fmt, ...)  DPRINT(fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)   DPRINT(fmt, ##__VA_ARGS__)
#define TRACE(fmt, ...)  DPRINT(fmt, ##__VA_ARGS__)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_x) (sizeof((_x))/sizeof((_x)[0]))
#endif

typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

typedef void (CALLBACK *PNTAPCFUNC)(ULONG_PTR,ULONG_PTR,ULONG_PTR);
typedef void (CALLBACK *PRTL_THREAD_START_ROUTINE)(LPVOID);
typedef DWORD (CALLBACK *PRTL_WORK_ITEM_ROUTINE)(LPVOID);
typedef void (NTAPI *RTL_WAITORTIMERCALLBACKFUNC)(PVOID,BOOLEAN);
typedef VOID (CALLBACK *PRTL_OVERLAPPED_COMPLETION_ROUTINE)(DWORD,DWORD,LPVOID);

typedef void (CALLBACK *PTP_IO_CALLBACK)(PTP_CALLBACK_INSTANCE,void*,void*,IO_STATUS_BLOCK*,PTP_IO);
NTSYSAPI NTSTATUS  WINAPI TpSimpleTryPost(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
#define PRTL_WORK_ITEM_ROUTINE WORKERCALLBACKFUNC

#define CRITICAL_SECTION RTL_CRITICAL_SECTION
#define GetProcessHeap() RtlGetProcessHeap()
#define GetCurrentProcess() NtCurrentProcess()
#define GetCurrentThread() NtCurrentThread()
#define GetCurrentThreadId() HandleToULong(NtCurrentTeb()->ClientId.UniqueThread)
#else
#include <assert.h>
#include <stdarg.h>
#include <limits.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(threadpool);
#endif

/*
 * Old thread pooling API
 */

struct rtl_work_item
{
    PRTL_WORK_ITEM_ROUTINE function;
    PVOID context;
};

#define EXPIRE_NEVER       (~(ULONGLONG)0)
#define TIMER_QUEUE_MAGIC  0x516d6954   /* TimQ */

#ifndef __REACTOS__
static RTL_CRITICAL_SECTION_DEBUG critsect_compl_debug;
#endif

static struct
{
    HANDLE                  compl_port;
    RTL_CRITICAL_SECTION    threadpool_compl_cs;
}
old_threadpool =
{
    NULL,                                       /* compl_port */
#ifdef __REACTOS__
    {0},                                        /* threadpool_compl_cs */
#else
    { &critsect_compl_debug, -1, 0, 0, 0, 0 },  /* threadpool_compl_cs */
#endif
};

#ifndef __REACTOS__
static RTL_CRITICAL_SECTION_DEBUG critsect_compl_debug =
{
    0, 0, &old_threadpool.threadpool_compl_cs,
    { &critsect_compl_debug.ProcessLocksList, &critsect_compl_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": threadpool_compl_cs") }
};
#endif

struct timer_queue;
struct queue_timer
{
    struct timer_queue *q;
    struct list entry;
    ULONG runcount;             /* number of callbacks pending execution */
    RTL_WAITORTIMERCALLBACKFUNC callback;
    PVOID param;
    DWORD period;
    ULONG flags;
    ULONGLONG expire;
    BOOL destroy;               /* timer should be deleted; once set, never unset */
    HANDLE event;               /* removal event */
};

struct timer_queue
{
    DWORD magic;
    RTL_CRITICAL_SECTION cs;
    struct list timers;         /* sorted by expiration time */
    BOOL quit;                  /* queue should be deleted; once set, never unset */
    HANDLE event;
    HANDLE thread;
};

/*
 * Object-oriented thread pooling API
 */

#define THREADPOOL_WORKER_TIMEOUT 5000
#define MAXIMUM_WAITQUEUE_OBJECTS (MAXIMUM_WAIT_OBJECTS - 1)

/* internal threadpool representation */
struct threadpool
{
    LONG                    refcount;
    LONG                    objcount;
    BOOL                    shutdown;
    CRITICAL_SECTION        cs;
    /* Pools of work items, locked via .cs, order matches TP_CALLBACK_PRIORITY - high, normal, low. */
    struct list             pools[3];
    RTL_CONDITION_VARIABLE  update_event;
    /* information about worker threads, locked via .cs */
    int                     max_workers;
    int                     min_workers;
    int                     num_workers;
    int                     num_busy_workers;
    HANDLE                  compl_port;
    TP_POOL_STACK_INFORMATION stack_info;
};

enum threadpool_objtype
{
    TP_OBJECT_TYPE_SIMPLE,
    TP_OBJECT_TYPE_WORK,
    TP_OBJECT_TYPE_TIMER,
    TP_OBJECT_TYPE_WAIT,
    TP_OBJECT_TYPE_IO,
};

struct io_completion
{
    IO_STATUS_BLOCK iosb;
    ULONG_PTR cvalue;
};

/* internal threadpool object representation */
struct threadpool_object
{
    void                   *win32_callback; /* leave space for kernelbase to store win32 callback */
    LONG                    refcount;
    BOOL                    shutdown;
    /* read-only information */
    enum threadpool_objtype type;
    struct threadpool       *pool;
    struct threadpool_group *group;
    PVOID                   userdata;
    PTP_CLEANUP_GROUP_CANCEL_CALLBACK group_cancel_callback;
    PTP_SIMPLE_CALLBACK     finalization_callback;
    BOOL                    may_run_long;
    HMODULE                 race_dll;
    TP_CALLBACK_PRIORITY    priority;
    /* information about the group, locked via .group->cs */
    struct list             group_entry;
    BOOL                    is_group_member;
    /* information about the pool, locked via .pool->cs */
    struct list             pool_entry;
    RTL_CONDITION_VARIABLE  finished_event;
    RTL_CONDITION_VARIABLE  group_finished_event;
    HANDLE                  completed_event;
    LONG                    num_pending_callbacks;
    LONG                    num_running_callbacks;
    LONG                    num_associated_callbacks;
    /* arguments for callback */
    union
    {
        struct
        {
            PTP_SIMPLE_CALLBACK callback;
        } simple;
        struct
        {
            PTP_WORK_CALLBACK callback;
        } work;
        struct
        {
            PTP_TIMER_CALLBACK callback;
            /* information about the timer, locked via timerqueue.cs */
            BOOL            timer_initialized;
            BOOL            timer_pending;
            struct list     timer_entry;
            BOOL            timer_set;
            ULONGLONG       timeout;
            LONG            period;
            LONG            window_length;
        } timer;
        struct
        {
            PTP_WAIT_CALLBACK callback;
            LONG            signaled;
            /* information about the wait object, locked via waitqueue.cs */
            struct waitqueue_bucket *bucket;
            BOOL            wait_pending;
            struct list     wait_entry;
            ULONGLONG       timeout;
            HANDLE          handle;
            DWORD           flags;
            RTL_WAITORTIMERCALLBACKFUNC rtl_callback;
        } wait;
        struct
        {
            PTP_IO_CALLBACK callback;
            /* locked via .pool->cs */
            unsigned int    pending_count, skipped_count, completion_count, completion_max;
            BOOL            shutting_down;
            struct io_completion *completions;
        } io;
    } u;
};

/* internal threadpool instance representation */
struct threadpool_instance
{
    struct threadpool_object *object;
    DWORD                   threadid;
    BOOL                    associated;
    BOOL                    may_run_long;
    struct
    {
        CRITICAL_SECTION    *critical_section;
        HANDLE              mutex;
        HANDLE              semaphore;
        LONG                semaphore_count;
        HANDLE              event;
        HMODULE             library;
    } cleanup;
};

/* internal threadpool group representation */
struct threadpool_group
{
    LONG                    refcount;
    BOOL                    shutdown;
    CRITICAL_SECTION        cs;
    /* list of group members, locked via .cs */
    struct list             members;
};

#ifndef __REACTOS__
/* global timerqueue object */
static RTL_CRITICAL_SECTION_DEBUG timerqueue_debug;
#endif

static struct
{
    CRITICAL_SECTION        cs;
    LONG                    objcount;
    BOOL                    thread_running;
    struct list             pending_timers;
    RTL_CONDITION_VARIABLE  update_event;
}
timerqueue =
{
#ifdef __REACTOS__
    {0},                                        /* cs */
#else
    { &timerqueue_debug, -1, 0, 0, 0, 0 },      /* cs */
#endif
    0,                                          /* objcount */
    FALSE,                                      /* thread_running */
    LIST_INIT( timerqueue.pending_timers ),     /* pending_timers */
#if __REACTOS__
    0,
#else
    RTL_CONDITION_VARIABLE_INIT                 /* update_event */
#endif
};

#ifndef __REACTOS__
static RTL_CRITICAL_SECTION_DEBUG timerqueue_debug =
{
    0, 0, &timerqueue.cs,
    { &timerqueue_debug.ProcessLocksList, &timerqueue_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": timerqueue.cs") }
};

/* global waitqueue object */
static RTL_CRITICAL_SECTION_DEBUG waitqueue_debug;
#endif

static struct
{
    CRITICAL_SECTION        cs;
    LONG                    num_buckets;
    struct list             buckets;
}
waitqueue =
{
#ifdef __REACTOS__
    {0},       /* cs */
#else
    { &waitqueue_debug, -1, 0, 0, 0, 0 },       /* cs */
#endif
    0,                                          /* num_buckets */
    LIST_INIT( waitqueue.buckets )              /* buckets */
};

#ifndef __REACTOS__
static RTL_CRITICAL_SECTION_DEBUG waitqueue_debug =
{
    0, 0, &waitqueue.cs,
    { &waitqueue_debug.ProcessLocksList, &waitqueue_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": waitqueue.cs") }
};
#endif

struct waitqueue_bucket
{
    struct list             bucket_entry;
    LONG                    objcount;
    struct list             reserved;
    struct list             waiting;
    HANDLE                  update_event;
    BOOL                    alertable;
};

#ifndef __REACTOS__
/* global I/O completion queue object */
static RTL_CRITICAL_SECTION_DEBUG ioqueue_debug;
#endif

static struct
{
    CRITICAL_SECTION        cs;
    LONG                    objcount;
    BOOL                    thread_running;
    HANDLE                  port;
    RTL_CONDITION_VARIABLE  update_event;
}
ioqueue =
{
#ifdef __REACTOS__
    .cs = {0},
#else
    .cs = { &ioqueue_debug, -1, 0, 0, 0, 0 },
#endif
};

#ifndef __REACTOS__
static RTL_CRITICAL_SECTION_DEBUG ioqueue_debug =
{
    0, 0, &ioqueue.cs,
    { &ioqueue_debug.ProcessLocksList, &ioqueue_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ioqueue.cs") }
};
#endif

static inline struct threadpool *impl_from_TP_POOL( TP_POOL *pool )
{
    return (struct threadpool *)pool;
}

static inline struct threadpool_object *impl_from_TP_WORK( TP_WORK *work )
{
    struct threadpool_object *object = (struct threadpool_object *)work;
    assert( object->type == TP_OBJECT_TYPE_WORK );
    return object;
}

static inline struct threadpool_object *impl_from_TP_TIMER( TP_TIMER *timer )
{
    struct threadpool_object *object = (struct threadpool_object *)timer;
    assert( object->type == TP_OBJECT_TYPE_TIMER );
    return object;
}

static inline struct threadpool_object *impl_from_TP_WAIT( TP_WAIT *wait )
{
    struct threadpool_object *object = (struct threadpool_object *)wait;
    assert( object->type == TP_OBJECT_TYPE_WAIT );
    return object;
}

static inline struct threadpool_object *impl_from_TP_IO( TP_IO *io )
{
    struct threadpool_object *object = (struct threadpool_object *)io;
    assert( object->type == TP_OBJECT_TYPE_IO );
    return object;
}

static inline struct threadpool_group *impl_from_TP_CLEANUP_GROUP( TP_CLEANUP_GROUP *group )
{
    return (struct threadpool_group *)group;
}

static inline struct threadpool_instance *impl_from_TP_CALLBACK_INSTANCE( TP_CALLBACK_INSTANCE *instance )
{
    return (struct threadpool_instance *)instance;
}

#ifdef __REACTOS__
ULONG NTAPI threadpool_worker_proc(PVOID param );
#else
static void CALLBACK threadpool_worker_proc( void *param );
#endif
static void tp_object_submit( struct threadpool_object *object, BOOL signaled );
static void tp_object_execute( struct threadpool_object *object, BOOL wait_thread );
static void tp_object_prepare_shutdown( struct threadpool_object *object );
static BOOL tp_object_release( struct threadpool_object *object );
static struct threadpool *default_threadpool = NULL;

static BOOL array_reserve(void **elements, unsigned int *capacity, unsigned int count, unsigned int size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = RtlReAllocateHeap( GetProcessHeap(), 0, *elements, new_capacity * size )))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

static void set_thread_name(const WCHAR *name)
{
#ifndef __REACTOS__ // This is impossible on non vista+
    THREAD_NAME_INFORMATION info;

    RtlInitUnicodeString(&info.ThreadName, name);
    NtSetInformationThread(GetCurrentThread(), ThreadNameInformation, &info, sizeof(info));
#endif
}

#ifndef __REACTOS__
static void CALLBACK process_rtl_work_item( TP_CALLBACK_INSTANCE *instance, void *userdata )
{
    struct rtl_work_item *item = userdata;

    TRACE("executing %p(%p)\n", item->function, item->context);
    item->function( item->context );

    RtlFreeHeap( GetProcessHeap(), 0, item );
}

/***********************************************************************
 *              RtlQueueWorkItem   (NTDLL.@)
 *
 * Queues a work item into a thread in the thread pool.
 *
 * PARAMS
 *  function [I] Work function to execute.
 *  context  [I] Context to pass to the work function when it is executed.
 *  flags    [I] Flags. See notes.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 *
 * NOTES
 *  Flags can be one or more of the following:
 *|WT_EXECUTEDEFAULT - Executes the work item in a non-I/O worker thread.
 *|WT_EXECUTEINIOTHREAD - Executes the work item in an I/O worker thread.
 *|WT_EXECUTEINPERSISTENTTHREAD - Executes the work item in a thread that is persistent.
 *|WT_EXECUTELONGFUNCTION - Hints that the execution can take a long time.
 *|WT_TRANSFER_IMPERSONATION - Executes the function with the current access token.
 */
NTSTATUS WINAPI RtlQueueWorkItem( PRTL_WORK_ITEM_ROUTINE function, PVOID context, ULONG flags )
{
    TP_CALLBACK_ENVIRON environment;
    struct rtl_work_item *item;
    NTSTATUS status;

    TRACE( "%p %p %lu\n", function, context, flags );

    item = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*item) );
    if (!item)
        return STATUS_NO_MEMORY;

    memset( &environment, 0, sizeof(environment) );
    environment.Version = 1;
    environment.u.s.LongFunction = (flags & WT_EXECUTELONGFUNCTION) != 0;
    environment.u.s.Persistent   = (flags & WT_EXECUTEINPERSISTENTTHREAD) != 0;

    item->function = function;
    item->context  = context;

    status = TpSimpleTryPost( process_rtl_work_item, item, &environment );
    if (status) RtlFreeHeap( GetProcessHeap(), 0, item );
    return status;
}

/***********************************************************************
 * iocp_poller - get completion events and run callbacks
 */
static DWORD CALLBACK iocp_poller(LPVOID Arg)
{
    HANDLE cport = Arg;

    while( TRUE )
    {
        PRTL_OVERLAPPED_COMPLETION_ROUTINE callback;
        LPVOID overlapped;
        IO_STATUS_BLOCK iosb;
#ifdef __REACTOS__
        NTSTATUS res = NtRemoveIoCompletion( cport, (PVOID)&callback, (PVOID)&overlapped, &iosb, NULL );
#else
        NTSTATUS res = NtRemoveIoCompletion( cport, (PULONG_PTR)&callback, (PULONG_PTR)&overlapped, &iosb, NULL );
#endif
        if (res)
        {
            ERR("NtRemoveIoCompletion failed: 0x%lx\n", res);
        }
        else
        {
            DWORD transferred = 0;
            DWORD err = 0;

            if (iosb.Status == STATUS_SUCCESS)
                transferred = iosb.Information;
            else
                err = RtlNtStatusToDosError(iosb.Status);

            callback( err, transferred, overlapped );
        }
    }
    return 0;
}

/***********************************************************************
 *              RtlSetIoCompletionCallback  (NTDLL.@)
 *
 * Binds a handle to a thread pool's completion port, and possibly
 * starts a non-I/O thread to monitor this port and call functions back.
 *
 * PARAMS
 *  FileHandle [I] Handle to bind to a completion port.
 *  Function   [I] Callback function to call on I/O completions.
 *  Flags      [I] Not used.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 *
 */
NTSTATUS WINAPI RtlSetIoCompletionCallback(HANDLE FileHandle, PRTL_OVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags)
{
    IO_STATUS_BLOCK iosb;
    FILE_COMPLETION_INFORMATION info;

    if (Flags) FIXME("Unknown value Flags=0x%lx\n", Flags);

    if (!old_threadpool.compl_port)
    {
        NTSTATUS res = STATUS_SUCCESS;

        RtlEnterCriticalSection(&old_threadpool.threadpool_compl_cs);
        if (!old_threadpool.compl_port)
        {
            HANDLE cport;

            res = NtCreateIoCompletion( &cport, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
            if (!res)
            {
                /* FIXME native can start additional threads in case of e.g. hung callback function. */
                res = RtlQueueWorkItem( iocp_poller, cport, WT_EXECUTEDEFAULT );
                if (!res)
                    old_threadpool.compl_port = cport;
                else
                    NtClose( cport );
            }
        }
        RtlLeaveCriticalSection(&old_threadpool.threadpool_compl_cs);
        if (res) return res;
    }

    info.CompletionPort = old_threadpool.compl_port;
    info.CompletionKey = (ULONG_PTR)Function;

    return NtSetInformationFile( FileHandle, &iosb, &info, sizeof(info), FileCompletionInformation );
}
#endif

static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, ULONG timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

/************************** Timer Queue Impl **************************/

static void queue_remove_timer(struct queue_timer *t)
{
    /* We MUST hold the queue cs while calling this function.  This ensures
       that we cannot queue another callback for this timer.  The runcount
       being zero makes sure we don't have any already queued.  */
    struct timer_queue *q = t->q;

    assert(t->runcount == 0);
    assert(t->destroy);

    list_remove(&t->entry);
    if (t->event)
        NtSetEvent(t->event, NULL);
    RtlFreeHeap(GetProcessHeap(), 0, t);

    if (q->quit && list_empty(&q->timers))
        NtSetEvent(q->event, NULL);
}

static void timer_cleanup_callback(struct queue_timer *t)
{
    struct timer_queue *q = t->q;
    RtlEnterCriticalSection(&q->cs);

    assert(0 < t->runcount);
    --t->runcount;

    if (t->destroy && t->runcount == 0)
        queue_remove_timer(t);

    RtlLeaveCriticalSection(&q->cs);
}

static DWORD WINAPI timer_callback_wrapper(LPVOID p)
{
    struct queue_timer *t = p;
    t->callback(t->param, TRUE);
    timer_cleanup_callback(t);
    return 0;
}

static inline ULONGLONG queue_current_time(void)
{
    LARGE_INTEGER now, freq;
    NtQueryPerformanceCounter(&now, &freq);
    return now.QuadPart * 1000 / freq.QuadPart;
}

static void queue_add_timer(struct queue_timer *t, ULONGLONG time,
                            BOOL set_event)
{
    /* We MUST hold the queue cs while calling this function.  */
    struct timer_queue *q = t->q;
    struct list *ptr = &q->timers;

    assert(!q->quit || (t->destroy && time == EXPIRE_NEVER));

    if (time != EXPIRE_NEVER)
        LIST_FOR_EACH(ptr, &q->timers)
        {
            struct queue_timer *cur = LIST_ENTRY(ptr, struct queue_timer, entry);
            if (time < cur->expire)
                break;
        }
    list_add_before(ptr, &t->entry);

    t->expire = time;

    /* If we insert at the head of the list, we need to expire sooner
       than expected.  */
    if (set_event && &t->entry == list_head(&q->timers))
        NtSetEvent(q->event, NULL);
}

static inline void queue_move_timer(struct queue_timer *t, ULONGLONG time,
                                    BOOL set_event)
{
    /* We MUST hold the queue cs while calling this function.  */
    list_remove(&t->entry);
    queue_add_timer(t, time, set_event);
}

static void queue_timer_expire(struct timer_queue *q)
{
    struct queue_timer *t = NULL;

    RtlEnterCriticalSection(&q->cs);
    if (list_head(&q->timers))
    {
        ULONGLONG now, next;
        t = LIST_ENTRY(list_head(&q->timers), struct queue_timer, entry);
        if (!t->destroy && t->expire <= ((now = queue_current_time())))
        {
            ++t->runcount;
            if (t->period)
            {
                next = t->expire + t->period;
                /* avoid trigger cascade if overloaded / hibernated */
                if (next < now)
                    next = now + t->period;
            }
            else
                next = EXPIRE_NEVER;
            queue_move_timer(t, next, FALSE);
        }
        else
            t = NULL;
    }
    RtlLeaveCriticalSection(&q->cs);

    if (t)
    {
        if (t->flags & WT_EXECUTEINTIMERTHREAD)
            timer_callback_wrapper(t);
        else
        {
            ULONG flags
                = (t->flags
                   & (WT_EXECUTEINIOTHREAD | WT_EXECUTEINPERSISTENTTHREAD
                      | WT_EXECUTELONGFUNCTION | WT_TRANSFER_IMPERSONATION));
            NTSTATUS status = RtlQueueWorkItem(timer_callback_wrapper, t, flags);
            if (status != STATUS_SUCCESS)
                timer_cleanup_callback(t);
        }
    }
}

static ULONG queue_get_timeout(struct timer_queue *q)
{
    struct queue_timer *t;
    ULONG timeout = INFINITE;

    RtlEnterCriticalSection(&q->cs);
    if (list_head(&q->timers))
    {
        t = LIST_ENTRY(list_head(&q->timers), struct queue_timer, entry);
        assert(!t->destroy || t->expire == EXPIRE_NEVER);

        if (t->expire != EXPIRE_NEVER)
        {
            ULONGLONG time = queue_current_time();
            timeout = t->expire < time ? 0 : t->expire - time;
        }
    }
    RtlLeaveCriticalSection(&q->cs);

    return timeout;
}

#ifdef __REACTOS__
ULONG NTAPI timer_queue_thread_proc(PVOID p)
#else
static void WINAPI timer_queue_thread_proc(LPVOID p)
#endif
{
    struct timer_queue *q = p;
    ULONG timeout_ms;

    set_thread_name(L"wine_threadpool_timer_queue");
    timeout_ms = INFINITE;
    for (;;)
    {
        LARGE_INTEGER timeout;
        NTSTATUS status;
        BOOL done = FALSE;

        status = NtWaitForSingleObject(
            q->event, FALSE, get_nt_timeout(&timeout, timeout_ms));

        if (status == STATUS_WAIT_0)
        {
            /* There are two possible ways to trigger the event.  Either
               we are quitting and the last timer got removed, or a new
               timer got put at the head of the list so we need to adjust
               our timeout.  */
            RtlEnterCriticalSection(&q->cs);
            if (q->quit && list_empty(&q->timers))
                done = TRUE;
            RtlLeaveCriticalSection(&q->cs);
        }
        else if (status == STATUS_TIMEOUT)
            queue_timer_expire(q);

        if (done)
            break;

        timeout_ms = queue_get_timeout(q);
    }

    NtClose(q->event);
    RtlDeleteCriticalSection(&q->cs);
    q->magic = 0;
    RtlFreeHeap(GetProcessHeap(), 0, q);
    RtlExitUserThread( 0 );
#ifdef __REACTOS__
    return STATUS_SUCCESS;
#endif
}

static void queue_destroy_timer(struct queue_timer *t)
{
    /* We MUST hold the queue cs while calling this function.  */
    t->destroy = TRUE;
    if (t->runcount == 0)
        /* Ensure a timer is promptly removed.  If callbacks are pending,
           it will be removed after the last one finishes by the callback
           cleanup wrapper.  */
        queue_remove_timer(t);
    else
        /* Make sure no destroyed timer masks an active timer at the head
           of the sorted list.  */
        queue_move_timer(t, EXPIRE_NEVER, FALSE);
}

/***********************************************************************
 *              RtlCreateTimerQueue   (NTDLL.@)
 *
 * Creates a timer queue object and returns a handle to it.
 *
 * PARAMS
 *  NewTimerQueue [O] The newly created queue.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlCreateTimerQueue(PHANDLE NewTimerQueue)
{
    NTSTATUS status;
    struct timer_queue *q = RtlAllocateHeap(GetProcessHeap(), 0, sizeof *q);
    if (!q)
        return STATUS_NO_MEMORY;

    RtlInitializeCriticalSection(&q->cs);
    list_init(&q->timers);
    q->quit = FALSE;
    q->magic = TIMER_QUEUE_MAGIC;
    status = NtCreateEvent(&q->event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (status != STATUS_SUCCESS)
    {
        RtlFreeHeap(GetProcessHeap(), 0, q);
        return status;
    }
    status = RtlCreateUserThread(GetCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                 timer_queue_thread_proc, q, &q->thread, NULL);
    if (status != STATUS_SUCCESS)
    {
        NtClose(q->event);
        RtlFreeHeap(GetProcessHeap(), 0, q);
        return status;
    }

    *NewTimerQueue = q;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *              RtlDeleteTimerQueueEx   (NTDLL.@)
 *
 * Deletes a timer queue object.
 *
 * PARAMS
 *  TimerQueue      [I] The timer queue to destroy.
 *  CompletionEvent [I] If NULL, return immediately.  If INVALID_HANDLE_VALUE,
 *                      wait until all timers are finished firing before
 *                      returning.  Otherwise, return immediately and set the
 *                      event when all timers are done.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS if synchronous, STATUS_PENDING if not.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeleteTimerQueueEx(HANDLE TimerQueue, HANDLE CompletionEvent)
{
    struct timer_queue *q = TimerQueue;
    struct queue_timer *t, *temp;
    HANDLE thread;
    NTSTATUS status;

    if (!q || q->magic != TIMER_QUEUE_MAGIC)
        return STATUS_INVALID_HANDLE;

    thread = q->thread;

    RtlEnterCriticalSection(&q->cs);
    q->quit = TRUE;
    if (list_head(&q->timers))
        /* When the last timer is removed, it will signal the timer thread to
           exit...  */
        LIST_FOR_EACH_ENTRY_SAFE(t, temp, &q->timers, struct queue_timer, entry)
            queue_destroy_timer(t);
    else
        /* However if we have none, we must do it ourselves.  */
        NtSetEvent(q->event, NULL);
    RtlLeaveCriticalSection(&q->cs);

    if (CompletionEvent == INVALID_HANDLE_VALUE)
    {
        NtWaitForSingleObject(thread, FALSE, NULL);
        status = STATUS_SUCCESS;
    }
    else
    {
        if (CompletionEvent)
        {
            FIXME("asynchronous return on completion event unimplemented\n");
            NtWaitForSingleObject(thread, FALSE, NULL);
            NtSetEvent(CompletionEvent, NULL);
        }
        status = STATUS_PENDING;
    }

    NtClose(thread);
    return status;
}

static struct timer_queue *get_timer_queue(HANDLE TimerQueue)
{
    static struct timer_queue *default_timer_queue;

    if (TimerQueue)
        return TimerQueue;
    else
    {
        if (!default_timer_queue)
        {
            HANDLE q;
            NTSTATUS status = RtlCreateTimerQueue(&q);
            if (status == STATUS_SUCCESS)
            {
                PVOID p = InterlockedCompareExchangePointer( (void **) &default_timer_queue, q, NULL );
                if (p)
                    /* Got beat to the punch.  */
                    RtlDeleteTimerQueueEx(q, NULL);
            }
        }
        return default_timer_queue;
    }
}

/***********************************************************************
 *              RtlCreateTimer   (NTDLL.@)
 *
 * Creates a new timer associated with the given queue.
 *
 * PARAMS
 *  TimerQueue [I] The queue to hold the timer.
 *  NewTimer   [O] The newly created timer.
 *  Callback   [I] The callback to fire.
 *  Parameter  [I] The argument for the callback.
 *  DueTime    [I] The delay, in milliseconds, before first firing the
 *                 timer.
 *  Period     [I] The period, in milliseconds, at which to fire the timer
 *                 after the first callback.  If zero, the timer will only
 *                 fire once.  It still needs to be deleted with
 *                 RtlDeleteTimer.
 * Flags       [I] Flags controlling the execution of the callback.  In
 *                 addition to the WT_* thread pool flags (see
 *                 RtlQueueWorkItem), WT_EXECUTEINTIMERTHREAD and
 *                 WT_EXECUTEONLYONCE are supported.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlCreateTimer(HANDLE TimerQueue, HANDLE *NewTimer,
                               RTL_WAITORTIMERCALLBACKFUNC Callback,
                               PVOID Parameter, DWORD DueTime, DWORD Period,
                               ULONG Flags)
{
    NTSTATUS status;
    struct queue_timer *t;
    struct timer_queue *q = get_timer_queue(TimerQueue);

    if (!q) return STATUS_NO_MEMORY;
    if (q->magic != TIMER_QUEUE_MAGIC) return STATUS_INVALID_HANDLE;

    t = RtlAllocateHeap(GetProcessHeap(), 0, sizeof *t);
    if (!t)
        return STATUS_NO_MEMORY;

    t->q = q;
    t->runcount = 0;
    t->callback = Callback;
    t->param = Parameter;
    t->period = Period;
    t->flags = Flags;
    t->destroy = FALSE;
    t->event = NULL;

    status = STATUS_SUCCESS;
    RtlEnterCriticalSection(&q->cs);
    if (q->quit)
        status = STATUS_INVALID_HANDLE;
    else
        queue_add_timer(t, queue_current_time() + DueTime, TRUE);
    RtlLeaveCriticalSection(&q->cs);

    if (status == STATUS_SUCCESS)
        *NewTimer = t;
    else
        RtlFreeHeap(GetProcessHeap(), 0, t);

    return status;
}

/***********************************************************************
 *              RtlUpdateTimer   (NTDLL.@)
 *
 * Changes the time at which a timer expires.
 *
 * PARAMS
 *  TimerQueue [I] The queue that holds the timer.
 *  Timer      [I] The timer to update.
 *  DueTime    [I] The delay, in milliseconds, before next firing the timer.
 *  Period     [I] The period, in milliseconds, at which to fire the timer
 *                 after the first callback.  If zero, the timer will not
 *                 refire once.  It still needs to be deleted with
 *                 RtlDeleteTimer.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlUpdateTimer(HANDLE TimerQueue, HANDLE Timer,
                               DWORD DueTime, DWORD Period)
{
    struct queue_timer *t = Timer;
    struct timer_queue *q = t->q;

    RtlEnterCriticalSection(&q->cs);
    /* Can't change a timer if it was once-only or destroyed.  */
    if (t->expire != EXPIRE_NEVER)
    {
        t->period = Period;
        queue_move_timer(t, queue_current_time() + DueTime, TRUE);
    }
    RtlLeaveCriticalSection(&q->cs);

    return STATUS_SUCCESS;
}

/***********************************************************************
 *              RtlDeleteTimer   (NTDLL.@)
 *
 * Cancels a timer-queue timer.
 *
 * PARAMS
 *  TimerQueue      [I] The queue that holds the timer.
 *  Timer           [I] The timer to update.
 *  CompletionEvent [I] If NULL, return immediately.  If INVALID_HANDLE_VALUE,
 *                      wait until the timer is finished firing all pending
 *                      callbacks before returning.  Otherwise, return
 *                      immediately and set the timer is done.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS if the timer is done, STATUS_PENDING if not,
             or if the completion event is NULL.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeleteTimer(HANDLE TimerQueue, HANDLE Timer,
                               HANDLE CompletionEvent)
{
    struct queue_timer *t = Timer;
    struct timer_queue *q;
    NTSTATUS status = STATUS_PENDING;
    HANDLE event = NULL;

    if (!Timer)
        return STATUS_INVALID_PARAMETER_1;
    q = t->q;
    if (CompletionEvent == INVALID_HANDLE_VALUE)
    {
        status = NtCreateEvent(&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        if (status == STATUS_SUCCESS)
            status = STATUS_PENDING;
    }
    else if (CompletionEvent)
        event = CompletionEvent;

    RtlEnterCriticalSection(&q->cs);
    t->event = event;
    if (t->runcount == 0 && event)
        status = STATUS_SUCCESS;
    queue_destroy_timer(t);
    RtlLeaveCriticalSection(&q->cs);

    if (CompletionEvent == INVALID_HANDLE_VALUE && event)
    {
        if (status == STATUS_PENDING)
        {
            NtWaitForSingleObject(event, FALSE, NULL);
            status = STATUS_SUCCESS;
        }
        NtClose(event);
    }

    return status;
}

/***********************************************************************
 *           timerqueue_thread_proc    (internal)
 */
#ifdef __REACTOS__
ULONG NTAPI timerqueue_thread_proc(PVOID param )
#else
static void CALLBACK timerqueue_thread_proc( void *param )
#endif
{
    ULONGLONG timeout_lower, timeout_upper, new_timeout;
    struct threadpool_object *other_timer;
    LARGE_INTEGER now, timeout;
    struct list *ptr;

    TRACE( "starting timer queue thread\n" );
    set_thread_name(L"wine_threadpool_timerqueue");

    RtlEnterCriticalSection( &timerqueue.cs );
    for (;;)
    {
        NtQuerySystemTime( &now );

        /* Check for expired timers. */
        while ((ptr = list_head( &timerqueue.pending_timers )))
        {
            struct threadpool_object *timer = LIST_ENTRY( ptr, struct threadpool_object, u.timer.timer_entry );
            assert( timer->type == TP_OBJECT_TYPE_TIMER );
            assert( timer->u.timer.timer_pending );
            if (timer->u.timer.timeout > now.QuadPart)
                break;

            /* Queue a new callback in one of the worker threads. */
            list_remove( &timer->u.timer.timer_entry );
            timer->u.timer.timer_pending = FALSE;
            tp_object_submit( timer, FALSE );

            /* Insert the timer back into the queue, except it's marked for shutdown. */
            if (timer->u.timer.period && !timer->shutdown)
            {
                timer->u.timer.timeout += (ULONGLONG)timer->u.timer.period * 10000;
                if (timer->u.timer.timeout <= now.QuadPart)
                    timer->u.timer.timeout = now.QuadPart + 1;

                LIST_FOR_EACH_ENTRY( other_timer, &timerqueue.pending_timers,
                                     struct threadpool_object, u.timer.timer_entry )
                {
                    assert( other_timer->type == TP_OBJECT_TYPE_TIMER );
                    if (timer->u.timer.timeout < other_timer->u.timer.timeout)
                        break;
                }
                list_add_before( &other_timer->u.timer.timer_entry, &timer->u.timer.timer_entry );
                timer->u.timer.timer_pending = TRUE;
            }
        }

        timeout_lower = timeout_upper = MAXLONGLONG;

        /* Determine next timeout and use the window length to optimize wakeup times. */
        LIST_FOR_EACH_ENTRY( other_timer, &timerqueue.pending_timers,
                             struct threadpool_object, u.timer.timer_entry )
        {
            assert( other_timer->type == TP_OBJECT_TYPE_TIMER );
            if (other_timer->u.timer.timeout >= timeout_upper)
                break;

            timeout_lower = other_timer->u.timer.timeout;
            new_timeout   = timeout_lower + (ULONGLONG)other_timer->u.timer.window_length * 10000;
            if (new_timeout < timeout_upper)
                timeout_upper = new_timeout;
        }

        /* Wait for timer update events or until the next timer expires. */
        if (timerqueue.objcount)
        {
            timeout.QuadPart = timeout_lower;
            RtlSleepConditionVariableCS( &timerqueue.update_event, &timerqueue.cs, &timeout );
            continue;
        }

        /* All timers have been destroyed, if no new timers are created
         * within some amount of time, then we can shutdown this thread. */
        timeout.QuadPart = (ULONGLONG)THREADPOOL_WORKER_TIMEOUT * -10000;
        if (RtlSleepConditionVariableCS( &timerqueue.update_event, &timerqueue.cs,
            &timeout ) == STATUS_TIMEOUT && !timerqueue.objcount)
        {
            break;
        }
    }

    timerqueue.thread_running = FALSE;
    RtlLeaveCriticalSection( &timerqueue.cs );

    TRACE( "terminating timer queue thread\n" );
    RtlExitUserThread( 0 );
#ifdef __REACTOS__
    return STATUS_SUCCESS;
#endif
}

/***********************************************************************
 *           tp_new_worker_thread    (internal)
 *
 * Create and account a new worker thread for the desired pool.
 */
static NTSTATUS tp_new_worker_thread( struct threadpool *pool )
{
    HANDLE thread;
    NTSTATUS status;

    status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE, 0,
                                  pool->stack_info.StackReserve, pool->stack_info.StackCommit,
                                  threadpool_worker_proc, pool, &thread, NULL );
    if (status == STATUS_SUCCESS)
    {
        InterlockedIncrement( &pool->refcount );
        pool->num_workers++;
        NtClose( thread );
    }
    return status;
}

/***********************************************************************
 *           tp_timerqueue_lock    (internal)
 *
 * Acquires a lock on the global timerqueue. When the lock is acquired
 * successfully, it is guaranteed that the timer thread is running.
 */
static NTSTATUS tp_timerqueue_lock( struct threadpool_object *timer )
{
    NTSTATUS status = STATUS_SUCCESS;
    assert( timer->type == TP_OBJECT_TYPE_TIMER );

    timer->u.timer.timer_initialized    = FALSE;
    timer->u.timer.timer_pending        = FALSE;
    timer->u.timer.timer_set            = FALSE;
    timer->u.timer.timeout              = 0;
    timer->u.timer.period               = 0;
    timer->u.timer.window_length        = 0;

    RtlEnterCriticalSection( &timerqueue.cs );

    /* Make sure that the timerqueue thread is running. */
    if (!timerqueue.thread_running)
    {
        HANDLE thread;
        status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                      timerqueue_thread_proc, NULL, &thread, NULL );
        if (status == STATUS_SUCCESS)
        {
            timerqueue.thread_running = TRUE;
            NtClose( thread );
        }
    }

    if (status == STATUS_SUCCESS)
    {
        timer->u.timer.timer_initialized = TRUE;
        timerqueue.objcount++;
    }

    RtlLeaveCriticalSection( &timerqueue.cs );
    return status;
}

/***********************************************************************
 *           tp_timerqueue_unlock    (internal)
 *
 * Releases a lock on the global timerqueue.
 */
static void tp_timerqueue_unlock( struct threadpool_object *timer )
{
    assert( timer->type == TP_OBJECT_TYPE_TIMER );

    RtlEnterCriticalSection( &timerqueue.cs );
    if (timer->u.timer.timer_initialized)
    {
        /* If timer was pending, remove it. */
        if (timer->u.timer.timer_pending)
        {
            list_remove( &timer->u.timer.timer_entry );
            timer->u.timer.timer_pending = FALSE;
        }

        /* If the last timer object was destroyed, then wake up the thread. */
        if (!--timerqueue.objcount)
        {
            assert( list_empty( &timerqueue.pending_timers ) );
            RtlWakeAllConditionVariable( &timerqueue.update_event );
        }

        timer->u.timer.timer_initialized = FALSE;
    }
    RtlLeaveCriticalSection( &timerqueue.cs );
}

/***********************************************************************
 *           waitqueue_thread_proc    (internal)
 */
#ifdef __REACTOS__
void NTAPI waitqueue_thread_proc(PVOID param )
#else
static void CALLBACK waitqueue_thread_proc( void *param )
#endif
{
    struct threadpool_object *objects[MAXIMUM_WAITQUEUE_OBJECTS];
    HANDLE handles[MAXIMUM_WAITQUEUE_OBJECTS + 1];
    struct waitqueue_bucket *bucket = param;
    struct threadpool_object *wait, *next;
    LARGE_INTEGER now, timeout;
    DWORD num_handles;
    NTSTATUS status;

    TRACE( "starting wait queue thread\n" );
    set_thread_name(L"wine_threadpool_waitqueue");

    RtlEnterCriticalSection( &waitqueue.cs );

    for (;;)
    {
        NtQuerySystemTime( &now );
        timeout.QuadPart = MAXLONGLONG;
        num_handles = 0;

        LIST_FOR_EACH_ENTRY_SAFE( wait, next, &bucket->waiting, struct threadpool_object,
                                  u.wait.wait_entry )
        {
            assert( wait->type == TP_OBJECT_TYPE_WAIT );
            if (wait->u.wait.timeout <= now.QuadPart)
            {
                /* Wait object timed out. */
                if ((wait->u.wait.flags & WT_EXECUTEONLYONCE))
                {
                    list_remove( &wait->u.wait.wait_entry );
                    list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
                }
                if ((wait->u.wait.flags & (WT_EXECUTEINWAITTHREAD | WT_EXECUTEINIOTHREAD)))
                {
                    InterlockedIncrement( &wait->refcount );
                    wait->num_pending_callbacks++;
                    RtlEnterCriticalSection( &wait->pool->cs );
                    tp_object_execute( wait, TRUE );
                    RtlLeaveCriticalSection( &wait->pool->cs );
                    tp_object_release( wait );
                }
                else tp_object_submit( wait, FALSE );
            }
            else
            {
                if (wait->u.wait.timeout < timeout.QuadPart)
                    timeout.QuadPart = wait->u.wait.timeout;

                assert( num_handles < MAXIMUM_WAITQUEUE_OBJECTS );
                InterlockedIncrement( &wait->refcount );
                objects[num_handles] = wait;
                handles[num_handles] = wait->u.wait.handle;
                num_handles++;
            }
        }

        if (!bucket->objcount)
        {
            /* All wait objects have been destroyed, if no new wait objects are created
             * within some amount of time, then we can shutdown this thread. */
            assert( num_handles == 0 );
            RtlLeaveCriticalSection( &waitqueue.cs );
            timeout.QuadPart = (ULONGLONG)THREADPOOL_WORKER_TIMEOUT * -10000;
            status = NtWaitForMultipleObjects( 1, &bucket->update_event, TRUE, bucket->alertable, &timeout );
            RtlEnterCriticalSection( &waitqueue.cs );

            if (status == STATUS_TIMEOUT && !bucket->objcount)
                break;
        }
        else
        {
            handles[num_handles] = bucket->update_event;
            RtlLeaveCriticalSection( &waitqueue.cs );
            status = NtWaitForMultipleObjects( num_handles + 1, handles, TRUE, bucket->alertable, &timeout );
            RtlEnterCriticalSection( &waitqueue.cs );

            if (status >= STATUS_WAIT_0 && status < STATUS_WAIT_0 + num_handles)
            {
                wait = objects[status - STATUS_WAIT_0];
                assert( wait->type == TP_OBJECT_TYPE_WAIT );
                if (wait->u.wait.bucket)
                {
                    /* Wait object signaled. */
                    assert( wait->u.wait.bucket == bucket );
                    if ((wait->u.wait.flags & WT_EXECUTEONLYONCE))
                    {
                        list_remove( &wait->u.wait.wait_entry );
                        list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
                    }
                    if ((wait->u.wait.flags & (WT_EXECUTEINWAITTHREAD | WT_EXECUTEINIOTHREAD)))
                    {
                        wait->u.wait.signaled++;
                        wait->num_pending_callbacks++;
                        RtlEnterCriticalSection( &wait->pool->cs );
                        tp_object_execute( wait, TRUE );
                        RtlLeaveCriticalSection( &wait->pool->cs );
                    }
                    else tp_object_submit( wait, TRUE );
                }
                else
                    WARN("wait object %p triggered while object was destroyed\n", wait);
            }

            /* Release temporary references to wait objects. */
            while (num_handles)
            {
                wait = objects[--num_handles];
                assert( wait->type == TP_OBJECT_TYPE_WAIT );
                tp_object_release( wait );
            }
        }

        /* Try to merge bucket with other threads. */
        if (waitqueue.num_buckets > 1 && bucket->objcount &&
            bucket->objcount <= MAXIMUM_WAITQUEUE_OBJECTS * 1 / 3)
        {
            struct waitqueue_bucket *other_bucket;
            LIST_FOR_EACH_ENTRY( other_bucket, &waitqueue.buckets, struct waitqueue_bucket, bucket_entry )
            {
                if (other_bucket != bucket && other_bucket->objcount && other_bucket->alertable == bucket->alertable &&
                    other_bucket->objcount + bucket->objcount <= MAXIMUM_WAITQUEUE_OBJECTS * 2 / 3)
                {
                    other_bucket->objcount += bucket->objcount;
                    bucket->objcount = 0;

                    /* Update reserved list. */
                    LIST_FOR_EACH_ENTRY( wait, &bucket->reserved, struct threadpool_object, u.wait.wait_entry )
                    {
                        assert( wait->type == TP_OBJECT_TYPE_WAIT );
                        wait->u.wait.bucket = other_bucket;
                    }
                    list_move_tail( &other_bucket->reserved, &bucket->reserved );

                    /* Update waiting list. */
                    LIST_FOR_EACH_ENTRY( wait, &bucket->waiting, struct threadpool_object, u.wait.wait_entry )
                    {
                        assert( wait->type == TP_OBJECT_TYPE_WAIT );
                        wait->u.wait.bucket = other_bucket;
                    }
                    list_move_tail( &other_bucket->waiting, &bucket->waiting );

                    /* Move bucket to the end, to keep the probability of
                     * newly added wait objects as small as possible. */
                    list_remove( &bucket->bucket_entry );
                    list_add_tail( &waitqueue.buckets, &bucket->bucket_entry );

                    NtSetEvent( other_bucket->update_event, NULL );
                    break;
                }
            }
        }
    }

    /* Remove this bucket from the list. */
    list_remove( &bucket->bucket_entry );
    if (!--waitqueue.num_buckets)
        assert( list_empty( &waitqueue.buckets ) );

    RtlLeaveCriticalSection( &waitqueue.cs );

    TRACE( "terminating wait queue thread\n" );

    assert( bucket->objcount == 0 );
    assert( list_empty( &bucket->reserved ) );
    assert( list_empty( &bucket->waiting ) );
    NtClose( bucket->update_event );

    RtlFreeHeap( GetProcessHeap(), 0, bucket );
    RtlExitUserThread( 0 );
}

/***********************************************************************
 *           tp_waitqueue_lock    (internal)
 */
static NTSTATUS tp_waitqueue_lock( struct threadpool_object *wait )
{
    struct waitqueue_bucket *bucket;
    NTSTATUS status;
    HANDLE thread;
    BOOL alertable = (wait->u.wait.flags & WT_EXECUTEINIOTHREAD) != 0;
    assert( wait->type == TP_OBJECT_TYPE_WAIT );

    wait->u.wait.signaled       = 0;
    wait->u.wait.bucket         = NULL;
    wait->u.wait.wait_pending   = FALSE;
    wait->u.wait.timeout        = 0;
    wait->u.wait.handle         = INVALID_HANDLE_VALUE;

    RtlEnterCriticalSection( &waitqueue.cs );

    /* Try to assign to existing bucket if possible. */
    LIST_FOR_EACH_ENTRY( bucket, &waitqueue.buckets, struct waitqueue_bucket, bucket_entry )
    {
        if (bucket->objcount < MAXIMUM_WAITQUEUE_OBJECTS && bucket->alertable == alertable)
        {
            list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
            wait->u.wait.bucket = bucket;
            bucket->objcount++;

            status = STATUS_SUCCESS;
            goto out;
        }
    }

    /* Create a new bucket and corresponding worker thread. */
    bucket = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*bucket) );
    if (!bucket)
    {
        status = STATUS_NO_MEMORY;
        goto out;
    }

    bucket->objcount = 0;
    bucket->alertable = alertable;
    list_init( &bucket->reserved );
    list_init( &bucket->waiting );

    status = NtCreateEvent( &bucket->update_event, EVENT_ALL_ACCESS,
                            NULL, SynchronizationEvent, FALSE );
    if (status)
    {
        RtlFreeHeap( GetProcessHeap(), 0, bucket );
        goto out;
    }

    status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                  (PTHREAD_START_ROUTINE)waitqueue_thread_proc, bucket, &thread, NULL );
    if (status == STATUS_SUCCESS)
    {
        list_add_tail( &waitqueue.buckets, &bucket->bucket_entry );
        waitqueue.num_buckets++;

        list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
        wait->u.wait.bucket = bucket;
        bucket->objcount++;

        NtClose( thread );
    }
    else
    {
        NtClose( bucket->update_event );
        RtlFreeHeap( GetProcessHeap(), 0, bucket );
    }

out:
    RtlLeaveCriticalSection( &waitqueue.cs );
    return status;
}

/***********************************************************************
 *           tp_waitqueue_unlock    (internal)
 */
static void tp_waitqueue_unlock( struct threadpool_object *wait )
{
    assert( wait->type == TP_OBJECT_TYPE_WAIT );

    RtlEnterCriticalSection( &waitqueue.cs );
    if (wait->u.wait.bucket)
    {
        struct waitqueue_bucket *bucket = wait->u.wait.bucket;
        assert( bucket->objcount > 0 );

        list_remove( &wait->u.wait.wait_entry );
        wait->u.wait.bucket = NULL;
        bucket->objcount--;

        NtSetEvent( bucket->update_event, NULL );
    }
    RtlLeaveCriticalSection( &waitqueue.cs );
}

#ifdef __REACTOS__
ULONG NTAPI ioqueue_thread_proc(PVOID param )
#else
static void CALLBACK ioqueue_thread_proc( void *param )
#endif
{
    struct io_completion *completion;
    struct threadpool_object *io;
    IO_STATUS_BLOCK iosb;
#ifdef __REACTOS__
    PVOID key, value;
#else
    ULONG_PTR key, value;
#endif
    BOOL destroy, skip;
    NTSTATUS status;

    TRACE( "starting I/O completion thread\n" );
    set_thread_name(L"wine_threadpool_ioqueue");

    RtlEnterCriticalSection( &ioqueue.cs );

    for (;;)
    {
        RtlLeaveCriticalSection( &ioqueue.cs );
        if ((status = NtRemoveIoCompletion( ioqueue.port, &key, &value, &iosb, NULL )))
            ERR("NtRemoveIoCompletion failed, status %#lx.\n", status);
        RtlEnterCriticalSection( &ioqueue.cs );

        destroy = skip = FALSE;
        io = (struct threadpool_object *)key;

        TRACE( "io %p, iosb.Status %#lx.\n", io, iosb.Status );

        if (io && (io->shutdown || io->u.io.shutting_down))
        {
            RtlEnterCriticalSection( &io->pool->cs );
            if (!io->u.io.pending_count)
            {
                if (io->u.io.skipped_count)
                    --io->u.io.skipped_count;

                if (io->u.io.skipped_count)
                    skip = TRUE;
                else
                    destroy = TRUE;
            }
            RtlLeaveCriticalSection( &io->pool->cs );
            if (skip) continue;
        }

        if (destroy)
        {
            --ioqueue.objcount;
            TRACE( "Releasing io %p.\n", io );
            io->shutdown = TRUE;
            tp_object_release( io );
        }
        else if (io)
        {
            RtlEnterCriticalSection( &io->pool->cs );

            TRACE( "pending_count %u.\n", io->u.io.pending_count );

            if (io->u.io.pending_count)
            {
                --io->u.io.pending_count;
                if (!array_reserve((void **)&io->u.io.completions, &io->u.io.completion_max,
                        io->u.io.completion_count + 1, sizeof(*io->u.io.completions)))
                {
                    ERR( "Failed to allocate memory.\n" );
                    RtlLeaveCriticalSection( &io->pool->cs );
                    continue;
                }

                completion = &io->u.io.completions[io->u.io.completion_count++];
                completion->iosb = iosb;
#ifdef __REACTOS__
                completion->cvalue = (ULONG_PTR)value;
#else
                completion->cvalue = value;
#endif

                tp_object_submit( io, FALSE );
            }
            RtlLeaveCriticalSection( &io->pool->cs );
        }

        if (!ioqueue.objcount)
        {
            /* All I/O objects have been destroyed; if no new objects are
             * created within some amount of time, then we can shutdown this
             * thread. */
            LARGE_INTEGER timeout = {.QuadPart = (ULONGLONG)THREADPOOL_WORKER_TIMEOUT * -10000};
            if (RtlSleepConditionVariableCS( &ioqueue.update_event, &ioqueue.cs,
                    &timeout) == STATUS_TIMEOUT && !ioqueue.objcount)
                break;
        }
    }

    ioqueue.thread_running = FALSE;
    RtlLeaveCriticalSection( &ioqueue.cs );

    TRACE( "terminating I/O completion thread\n" );

    RtlExitUserThread( 0 );

#ifdef __REACTOS__
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS tp_ioqueue_lock( struct threadpool_object *io, HANDLE file )
{
    NTSTATUS status = STATUS_SUCCESS;

    assert( io->type == TP_OBJECT_TYPE_IO );

    RtlEnterCriticalSection( &ioqueue.cs );

    if (!ioqueue.port && (status = NtCreateIoCompletion( &ioqueue.port,
            IO_COMPLETION_ALL_ACCESS, NULL, 0 )))
    {
        RtlLeaveCriticalSection( &ioqueue.cs );
        return status;
    }

    if (!ioqueue.thread_running)
    {
        HANDLE thread;

        if (!(status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE,
                                            0, 0, 0, ioqueue_thread_proc, NULL, &thread, NULL )))
        {
            ioqueue.thread_running = TRUE;
            NtClose( thread );
        }
    }

    if (status == STATUS_SUCCESS)
    {
        FILE_COMPLETION_INFORMATION info;
        IO_STATUS_BLOCK iosb;

#ifdef __REACTOS__
        info.Port = ioqueue.port;
        info.Key = io;
#else
        info.CompletionPort = ioqueue.port;
        info.CompletionKey = (ULONG_PTR)io;
#endif

        status = NtSetInformationFile( file, &iosb, &info, sizeof(info), FileCompletionInformation );
    }

    if (status == STATUS_SUCCESS)
    {
        if (!ioqueue.objcount++)
            RtlWakeConditionVariable( &ioqueue.update_event );
    }

    RtlLeaveCriticalSection( &ioqueue.cs );
    return status;
}

/***********************************************************************
 *           tp_threadpool_alloc    (internal)
 *
 * Allocates a new threadpool object.
 */
static NTSTATUS tp_threadpool_alloc( struct threadpool **out )
{
#ifdef __REACTOS__
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress );
#else
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
#endif
    struct threadpool *pool;
    unsigned int i;

    pool = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*pool) );
    if (!pool)
        return STATUS_NO_MEMORY;

    pool->refcount              = 1;
    pool->objcount              = 0;
    pool->shutdown              = FALSE;

#ifdef __REACTOS__
    RtlInitializeCriticalSection( &pool->cs );
#else
    RtlInitializeCriticalSectionEx( &pool->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );

    pool->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": threadpool.cs");
#endif

    for (i = 0; i < ARRAY_SIZE(pool->pools); ++i)
        list_init( &pool->pools[i] );
    RtlInitializeConditionVariable( &pool->update_event );

    pool->max_workers             = 500;
    pool->min_workers             = 0;
    pool->num_workers             = 0;
    pool->num_busy_workers        = 0;
    pool->stack_info.StackReserve = nt->OptionalHeader.SizeOfStackReserve;
    pool->stack_info.StackCommit  = nt->OptionalHeader.SizeOfStackCommit;

    TRACE( "allocated threadpool %p\n", pool );

    *out = pool;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           tp_threadpool_shutdown    (internal)
 *
 * Prepares the shutdown of a threadpool object and notifies all worker
 * threads to terminate (after all remaining work items have been
 * processed).
 */
static void tp_threadpool_shutdown( struct threadpool *pool )
{
    assert( pool != default_threadpool );

    pool->shutdown = TRUE;
    RtlWakeAllConditionVariable( &pool->update_event );
}

/***********************************************************************
 *           tp_threadpool_release    (internal)
 *
 * Releases a reference to a threadpool object.
 */
static BOOL tp_threadpool_release( struct threadpool *pool )
{
    unsigned int i;

    if (InterlockedDecrement( &pool->refcount ))
        return FALSE;

    TRACE( "destroying threadpool %p\n", pool );

    assert( pool->shutdown );
    assert( !pool->objcount );
    for (i = 0; i < ARRAY_SIZE(pool->pools); ++i)
        assert( list_empty( &pool->pools[i] ) );
#ifndef __REACTOS__
    pool->cs.DebugInfo->Spare[0] = 0;
#endif
    RtlDeleteCriticalSection( &pool->cs );

    RtlFreeHeap( GetProcessHeap(), 0, pool );
    return TRUE;
}

/***********************************************************************
 *           tp_threadpool_lock    (internal)
 *
 * Acquires a lock on a threadpool, specified with an TP_CALLBACK_ENVIRON
 * block. When the lock is acquired successfully, it is guaranteed that
 * there is at least one worker thread to process tasks.
 */
static NTSTATUS tp_threadpool_lock( struct threadpool **out, TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool *pool = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (environment)
    {
#ifndef __REACTOS__ //Windows 7 stuff 
        /* Validate environment parameters. */
        if (environment->Version == 3)
        {
            TP_CALLBACK_ENVIRON_V3 *environment3 = (TP_CALLBACK_ENVIRON_V3 *)environment;

            switch (environment3->CallbackPriority)
            {
                case TP_CALLBACK_PRIORITY_HIGH:
                case TP_CALLBACK_PRIORITY_NORMAL:
                case TP_CALLBACK_PRIORITY_LOW:
                    break;
                default:
                    return STATUS_INVALID_PARAMETER;
            }
        }
#endif
        pool = (struct threadpool *)environment->Pool;
    }

    if (!pool)
    {
        if (!default_threadpool)
        {
            status = tp_threadpool_alloc( &pool );
            if (status != STATUS_SUCCESS)
                return status;

            if (InterlockedCompareExchangePointer( (void *)&default_threadpool, pool, NULL ) != NULL)
            {
                tp_threadpool_shutdown( pool );
                tp_threadpool_release( pool );
            }
        }

        pool = default_threadpool;
    }
 
    RtlEnterCriticalSection( &pool->cs );

    /* Make sure that the threadpool has at least one thread. */
    if (!pool->num_workers)
        status = tp_new_worker_thread( pool );

    /* Keep a reference, and increment objcount to ensure that the
     * last thread doesn't terminate. */
    if (status == STATUS_SUCCESS)
    {
        InterlockedIncrement( &pool->refcount );
        pool->objcount++;
    }

    RtlLeaveCriticalSection( &pool->cs );

    if (status != STATUS_SUCCESS)
        return status;

    *out = pool;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           tp_threadpool_unlock    (internal)
 *
 * Releases a lock on a threadpool.
 */
static void tp_threadpool_unlock( struct threadpool *pool )
{
    RtlEnterCriticalSection( &pool->cs );
    pool->objcount--;
    RtlLeaveCriticalSection( &pool->cs );
    tp_threadpool_release( pool );
}

/***********************************************************************
 *           tp_group_alloc    (internal)
 *
 * Allocates a new threadpool group object.
 */
static NTSTATUS tp_group_alloc( struct threadpool_group **out )
{
    struct threadpool_group *group;

    group = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*group) );
    if (!group)
        return STATUS_NO_MEMORY;

    group->refcount     = 1;
    group->shutdown     = FALSE;

#ifdef __REACTOS__
    RtlInitializeCriticalSection( &group->cs );
#else
    RtlInitializeCriticalSectionEx( &group->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );

    group->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": threadpool_group.cs");
#endif

    list_init( &group->members );

    TRACE( "allocated group %p\n", group );

    *out = group;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           tp_group_shutdown    (internal)
 *
 * Marks the group object for shutdown.
 */
static void tp_group_shutdown( struct threadpool_group *group )
{
    group->shutdown = TRUE;
}

/***********************************************************************
 *           tp_group_release    (internal)
 *
 * Releases a reference to a group object.
 */
static BOOL tp_group_release( struct threadpool_group *group )
{
    if (InterlockedDecrement( &group->refcount ))
        return FALSE;

    TRACE( "destroying group %p\n", group );

    assert( group->shutdown );
    assert( list_empty( &group->members ) );

#ifndef __REACTOS__
    group->cs.DebugInfo->Spare[0] = 0;
#endif
    RtlDeleteCriticalSection( &group->cs );

    RtlFreeHeap( GetProcessHeap(), 0, group );
    return TRUE;
}

/***********************************************************************
 *           tp_object_initialize    (internal)
 *
 * Initializes members of a threadpool object.
 */
static void tp_object_initialize( struct threadpool_object *object, struct threadpool *pool,
                                  PVOID userdata, TP_CALLBACK_ENVIRON *environment )
{
    BOOL is_simple_callback = (object->type == TP_OBJECT_TYPE_SIMPLE);

    object->refcount                = 1;
    object->shutdown                = FALSE;

    object->pool                    = pool;
    object->group                   = NULL;
    object->userdata                = userdata;
    object->group_cancel_callback   = NULL;
    object->finalization_callback   = NULL;
    object->may_run_long            = 0;
    object->race_dll                = NULL;
    object->priority                = TP_CALLBACK_PRIORITY_NORMAL;

    memset( &object->group_entry, 0, sizeof(object->group_entry) );
    object->is_group_member         = FALSE;

    memset( &object->pool_entry, 0, sizeof(object->pool_entry) );
    RtlInitializeConditionVariable( &object->finished_event );
    RtlInitializeConditionVariable( &object->group_finished_event );
    object->completed_event         = NULL;
    object->num_pending_callbacks   = 0;
    object->num_running_callbacks   = 0;
    object->num_associated_callbacks = 0;

    if (environment)
    {
        if (environment->Version != 1 && environment->Version != 3)
            FIXME( "unsupported environment version %lu\n", environment->Version );

        object->group = impl_from_TP_CLEANUP_GROUP( environment->CleanupGroup );
        object->group_cancel_callback   = environment->CleanupGroupCancelCallback;
        object->finalization_callback   = environment->FinalizationCallback;
        object->may_run_long            = environment->u.s.LongFunction != 0;
        object->race_dll                = environment->RaceDll;
#ifndef __REACTOS__ //Windows 7 stuff
        if (environment->Version == 3)
        {
            TP_CALLBACK_ENVIRON_V3 *environment_v3 = (TP_CALLBACK_ENVIRON_V3 *)environment;

            object->priority = environment_v3->CallbackPriority;
            assert( object->priority < ARRAY_SIZE(pool->pools) );
        }
#endif
        if (environment->ActivationContext)
            FIXME( "activation context not supported yet\n" );

        if (environment->u.s.Persistent)
            FIXME( "persistent threads not supported yet\n" );
    }

    if (object->race_dll)
        LdrAddRefDll( 0, object->race_dll );

    TRACE( "allocated object %p of type %u\n", object, object->type );

    /* For simple callbacks we have to run tp_object_submit before adding this object
     * to the cleanup group. As soon as the cleanup group members are released ->shutdown
     * will be set, and tp_object_submit would fail with an assertion. */

    if (is_simple_callback)
        tp_object_submit( object, FALSE );

    if (object->group)
    {
        struct threadpool_group *group = object->group;
        InterlockedIncrement( &group->refcount );

        RtlEnterCriticalSection( &group->cs );
        list_add_tail( &group->members, &object->group_entry );
        object->is_group_member = TRUE;
        RtlLeaveCriticalSection( &group->cs );
    }

    if (is_simple_callback)
        tp_object_release( object );
}

static void tp_object_prio_queue( struct threadpool_object *object )
{
    ++object->pool->num_busy_workers;
    list_add_tail( &object->pool->pools[object->priority], &object->pool_entry );
}

/***********************************************************************
 *           tp_object_submit    (internal)
 *
 * Submits a threadpool object to the associated threadpool. This
 * function has to be VOID because TpPostWork can never fail on Windows.
 */
static void tp_object_submit( struct threadpool_object *object, BOOL signaled )
{
    struct threadpool *pool = object->pool;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    assert( !object->shutdown );
    assert( !pool->shutdown );

    RtlEnterCriticalSection( &pool->cs );

    /* Start new worker threads if required. */
    if (pool->num_busy_workers >= pool->num_workers &&
        pool->num_workers < pool->max_workers)
        status = tp_new_worker_thread( pool );

    /* Queue work item and increment refcount. */
    InterlockedIncrement( &object->refcount );
    if (!object->num_pending_callbacks++)
        tp_object_prio_queue( object );

    /* Count how often the object was signaled. */
    if (object->type == TP_OBJECT_TYPE_WAIT && signaled)
        object->u.wait.signaled++;

    /* No new thread started - wake up one existing thread. */
    if (status != STATUS_SUCCESS)
    {
        assert( pool->num_workers > 0 );
        RtlWakeConditionVariable( &pool->update_event );
    }

    RtlLeaveCriticalSection( &pool->cs );
}

/***********************************************************************
 *           tp_object_cancel    (internal)
 *
 * Cancels all currently pending callbacks for a specific object.
 */
static void tp_object_cancel( struct threadpool_object *object )
{
    struct threadpool *pool = object->pool;
    LONG pending_callbacks = 0;

    RtlEnterCriticalSection( &pool->cs );
    if (object->num_pending_callbacks)
    {
        pending_callbacks = object->num_pending_callbacks;
        object->num_pending_callbacks = 0;
        list_remove( &object->pool_entry );

        if (object->type == TP_OBJECT_TYPE_WAIT)
            object->u.wait.signaled = 0;
    }
    if (object->type == TP_OBJECT_TYPE_IO)
    {
        object->u.io.skipped_count += object->u.io.pending_count;
        object->u.io.pending_count = 0;
    }
    RtlLeaveCriticalSection( &pool->cs );

    while (pending_callbacks--)
        tp_object_release( object );
}

static BOOL object_is_finished( struct threadpool_object *object, BOOL group )
{
    if (object->num_pending_callbacks)
        return FALSE;
    if (object->type == TP_OBJECT_TYPE_IO && object->u.io.pending_count)
        return FALSE;

    if (group)
        return !object->num_running_callbacks;
    else
        return !object->num_associated_callbacks;
}

/***********************************************************************
 *           tp_object_wait    (internal)
 *
 * Waits until all pending and running callbacks of a specific object
 * have been processed.
 */
static void tp_object_wait( struct threadpool_object *object, BOOL group_wait )
{
    struct threadpool *pool = object->pool;

    RtlEnterCriticalSection( &pool->cs );
    while (!object_is_finished( object, group_wait ))
    {
        if (group_wait)
            RtlSleepConditionVariableCS( &object->group_finished_event, &pool->cs, NULL );
        else
            RtlSleepConditionVariableCS( &object->finished_event, &pool->cs, NULL );
    }
    RtlLeaveCriticalSection( &pool->cs );
}

static void tp_ioqueue_unlock( struct threadpool_object *io )
{
    assert( io->type == TP_OBJECT_TYPE_IO );

    RtlEnterCriticalSection( &ioqueue.cs );

    assert(ioqueue.objcount);

    if (!io->shutdown && !--ioqueue.objcount)
        NtSetIoCompletion( ioqueue.port, 0, 0, STATUS_SUCCESS, 0 );

    RtlLeaveCriticalSection( &ioqueue.cs );
}

/***********************************************************************
 *           tp_object_prepare_shutdown    (internal)
 *
 * Prepares a threadpool object for shutdown.
 */
static void tp_object_prepare_shutdown( struct threadpool_object *object )
{
    if (object->type == TP_OBJECT_TYPE_TIMER)
        tp_timerqueue_unlock( object );
    else if (object->type == TP_OBJECT_TYPE_WAIT)
        tp_waitqueue_unlock( object );
    else if (object->type == TP_OBJECT_TYPE_IO)
        tp_ioqueue_unlock( object );
}

/***********************************************************************
 *           tp_object_release    (internal)
 *
 * Releases a reference to a threadpool object.
 */
static BOOL tp_object_release( struct threadpool_object *object )
{
    if (InterlockedDecrement( &object->refcount ))
        return FALSE;

    TRACE( "destroying object %p of type %u\n", object, object->type );

    assert( object->shutdown );
    assert( !object->num_pending_callbacks );
    assert( !object->num_running_callbacks );
    assert( !object->num_associated_callbacks );

    /* release reference to the group */
    if (object->group)
    {
        struct threadpool_group *group = object->group;

        RtlEnterCriticalSection( &group->cs );
        if (object->is_group_member)
        {
            list_remove( &object->group_entry );
            object->is_group_member = FALSE;
        }
        RtlLeaveCriticalSection( &group->cs );

        tp_group_release( group );
    }

    tp_threadpool_unlock( object->pool );

    if (object->race_dll)
        LdrUnloadDll( object->race_dll );

    if (object->completed_event && object->completed_event != INVALID_HANDLE_VALUE)
        NtSetEvent( object->completed_event, NULL );

    RtlFreeHeap( GetProcessHeap(), 0, object );
    return TRUE;
}

static struct list *threadpool_get_next_item( const struct threadpool *pool )
{
    struct list *ptr;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pool->pools); ++i)
    {
        if ((ptr = list_head( &pool->pools[i] )))
            break;
    }

    return ptr;
}

/***********************************************************************
 *           tp_object_execute    (internal)
 *
 * Executes a threadpool object callback, object->pool->cs has to be
 * held.
 */
static void tp_object_execute( struct threadpool_object *object, BOOL wait_thread )
{
    TP_CALLBACK_INSTANCE *callback_instance;
    struct threadpool_instance instance;
    struct io_completion completion;
    struct threadpool *pool = object->pool;
    TP_WAIT_RESULT wait_result = 0;
    NTSTATUS status;

    object->num_pending_callbacks--;

    /* For wait objects check if they were signaled or have timed out. */
    if (object->type == TP_OBJECT_TYPE_WAIT)
    {
        wait_result = object->u.wait.signaled ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
        if (wait_result == WAIT_OBJECT_0) object->u.wait.signaled--;
    }
    else if (object->type == TP_OBJECT_TYPE_IO)
    {
        assert( object->u.io.completion_count );
        completion = object->u.io.completions[--object->u.io.completion_count];
    }

    /* Leave critical section and do the actual callback. */
    object->num_associated_callbacks++;
    object->num_running_callbacks++;
    RtlLeaveCriticalSection( &pool->cs );
    if (wait_thread) RtlLeaveCriticalSection( &waitqueue.cs );

    /* Initialize threadpool instance struct. */
    callback_instance = (TP_CALLBACK_INSTANCE *)&instance;
    instance.object                     = object;
    instance.threadid                   = GetCurrentThreadId();
    instance.associated                 = TRUE;
    instance.may_run_long               = object->may_run_long;
    instance.cleanup.critical_section   = NULL;
    instance.cleanup.mutex              = NULL;
    instance.cleanup.semaphore          = NULL;
    instance.cleanup.semaphore_count    = 0;
    instance.cleanup.event              = NULL;
    instance.cleanup.library            = NULL;

    switch (object->type)
    {
        case TP_OBJECT_TYPE_SIMPLE:
        {
            TRACE( "executing simple callback %p(%p, %p)\n",
                   object->u.simple.callback, callback_instance, object->userdata );
            object->u.simple.callback( callback_instance, object->userdata );
            TRACE( "callback %p returned\n", object->u.simple.callback );
            break;
        }

        case TP_OBJECT_TYPE_WORK:
        {
            TRACE( "executing work callback %p(%p, %p, %p)\n",
                   object->u.work.callback, callback_instance, object->userdata, object );
            object->u.work.callback( callback_instance, object->userdata, (TP_WORK *)object );
            TRACE( "callback %p returned\n", object->u.work.callback );
            break;
        }

        case TP_OBJECT_TYPE_TIMER:
        {
            TRACE( "executing timer callback %p(%p, %p, %p)\n",
                   object->u.timer.callback, callback_instance, object->userdata, object );
            object->u.timer.callback( callback_instance, object->userdata, (TP_TIMER *)object );
            TRACE( "callback %p returned\n", object->u.timer.callback );
            break;
        }

        case TP_OBJECT_TYPE_WAIT:
        {
            TRACE( "executing wait callback %p(%p, %p, %p, %lu)\n",
                   object->u.wait.callback, callback_instance, object->userdata, object, wait_result );
            object->u.wait.callback( callback_instance, object->userdata, (TP_WAIT *)object, wait_result );
            TRACE( "callback %p returned\n", object->u.wait.callback );
            break;
        }

        case TP_OBJECT_TYPE_IO:
        {
            TRACE( "executing I/O callback %p(%p, %p, %#Ix, %p, %p)\n",
                    object->u.io.callback, callback_instance, object->userdata,
                    completion.cvalue, &completion.iosb, (TP_IO *)object );
            object->u.io.callback( callback_instance, object->userdata,
                    (void *)completion.cvalue, &completion.iosb, (TP_IO *)object );
            TRACE( "callback %p returned\n", object->u.io.callback );
            break;
        }

        default:
            assert(0);
            break;
    }

    /* Execute finalization callback. */
    if (object->finalization_callback)
    {
        TRACE( "executing finalization callback %p(%p, %p)\n",
               object->finalization_callback, callback_instance, object->userdata );
        object->finalization_callback( callback_instance, object->userdata );
        TRACE( "callback %p returned\n", object->finalization_callback );
    }

    /* Execute cleanup tasks. */
    if (instance.cleanup.critical_section)
    {
        RtlLeaveCriticalSection( instance.cleanup.critical_section );
    }
    if (instance.cleanup.mutex)
    {
        status = NtReleaseMutant( instance.cleanup.mutex, NULL );
        if (status != STATUS_SUCCESS) goto skip_cleanup;
    }
    if (instance.cleanup.semaphore)
    {
        status = NtReleaseSemaphore( instance.cleanup.semaphore, instance.cleanup.semaphore_count, NULL );
        if (status != STATUS_SUCCESS) goto skip_cleanup;
    }
    if (instance.cleanup.event)
    {
        status = NtSetEvent( instance.cleanup.event, NULL );
        if (status != STATUS_SUCCESS) goto skip_cleanup;
    }
    if (instance.cleanup.library)
    {
        LdrUnloadDll( instance.cleanup.library );
    }

skip_cleanup:
    if (wait_thread) RtlEnterCriticalSection( &waitqueue.cs );
    RtlEnterCriticalSection( &pool->cs );

    /* Simple callbacks are automatically shutdown after execution. */
    if (object->type == TP_OBJECT_TYPE_SIMPLE)
    {
        tp_object_prepare_shutdown( object );
        object->shutdown = TRUE;
    }

    object->num_running_callbacks--;
    if (object_is_finished( object, TRUE ))
        RtlWakeAllConditionVariable( &object->group_finished_event );

    if (instance.associated)
    {
        object->num_associated_callbacks--;
        if (object_is_finished( object, FALSE ))
            RtlWakeAllConditionVariable( &object->finished_event );
    }
}

/***********************************************************************
 *           threadpool_worker_proc    (internal)
 */
#ifdef __REACTOS__
ULONG NTAPI threadpool_worker_proc(PVOID param )
#else
static void CALLBACK threadpool_worker_proc( void *param )
#endif
{
    struct threadpool *pool = param;
    LARGE_INTEGER timeout;
    struct list *ptr;

    TRACE( "starting worker thread for pool %p\n", pool );
    set_thread_name(L"wine_threadpool_worker");

    RtlEnterCriticalSection( &pool->cs );
    for (;;)
    {
        while ((ptr = threadpool_get_next_item( pool )))
        {
            struct threadpool_object *object = LIST_ENTRY( ptr, struct threadpool_object, pool_entry );
            assert( object->num_pending_callbacks > 0 );

            /* If further pending callbacks are queued, move the work item to
             * the end of the pool list. Otherwise remove it from the pool. */
            list_remove( &object->pool_entry );
            if (object->num_pending_callbacks > 1)
                tp_object_prio_queue( object );

            tp_object_execute( object, FALSE );

            assert(pool->num_busy_workers);
            pool->num_busy_workers--;

            tp_object_release( object );
        }

        /* Shutdown worker thread if requested. */
        if (pool->shutdown)
            break;

        /* Wait for new tasks or until the timeout expires. A thread only terminates
         * when no new tasks are available, and the number of threads can be
         * decreased without violating the min_workers limit. An exception is when
         * min_workers == 0, then objcount is used to detect if the last thread
         * can be terminated. */
        timeout.QuadPart = (ULONGLONG)THREADPOOL_WORKER_TIMEOUT * -10000;
        if (RtlSleepConditionVariableCS( &pool->update_event, &pool->cs, &timeout ) == STATUS_TIMEOUT &&
            !threadpool_get_next_item( pool ) && (pool->num_workers > max( pool->min_workers, 1 ) ||
            (!pool->min_workers && !pool->objcount)))
        {
            break;
        }
    }
    pool->num_workers--;
    RtlLeaveCriticalSection( &pool->cs );

    TRACE( "terminating worker thread for pool %p\n", pool );
    tp_threadpool_release( pool );
    RtlExitUserThread( 0 );
#ifdef __REACTOS__
    return STATUS_SUCCESS;
#endif
}

/***********************************************************************
 *           TpAllocCleanupGroup    (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocCleanupGroup( TP_CLEANUP_GROUP **out )
{
    TRACE( "%p\n", out );

    return tp_group_alloc( (struct threadpool_group **)out );
}

/***********************************************************************
 *           TpAllocIoCompletion    (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocIoCompletion( TP_IO **out, HANDLE file, PTP_IO_CALLBACK callback,
                                     void *userdata, TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool_object *object;
    struct threadpool *pool;
    NTSTATUS status;

    TRACE( "%p %p %p %p %p\n", out, file, callback, userdata, environment );

    if (!(object = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object) )))
        return STATUS_NO_MEMORY;

    if ((status = tp_threadpool_lock( &pool, environment )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_IO;
    object->u.io.callback = callback;
    if (!(object->u.io.completions = RtlAllocateHeap( GetProcessHeap(), 0, 8 * sizeof(*object->u.io.completions) )))
    {
        tp_threadpool_unlock( pool );
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    if ((status = tp_ioqueue_lock( object, file )))
    {
        tp_threadpool_unlock( pool );
        RtlFreeHeap( GetProcessHeap(), 0, object->u.io.completions );
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_IO *)object;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpAllocPool    (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocPool( TP_POOL **out, PVOID reserved )
{
    TRACE( "%p %p\n", out, reserved );

    if (reserved)
        FIXME( "reserved argument is nonzero (%p)\n", reserved );

    return tp_threadpool_alloc( (struct threadpool **)out );
}

/***********************************************************************
 *           TpAllocTimer    (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocTimer( TP_TIMER **out, PTP_TIMER_CALLBACK callback, PVOID userdata,
                              TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool_object *object;
    struct threadpool *pool;
    NTSTATUS status;

    TRACE( "%p %p %p %p\n", out, callback, userdata, environment );

    object = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_TIMER;
    object->u.timer.callback = callback;

    status = tp_timerqueue_lock( object );
    if (status)
    {
        tp_threadpool_unlock( pool );
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_TIMER *)object;
    return STATUS_SUCCESS;
}

static NTSTATUS tp_alloc_wait( TP_WAIT **out, PTP_WAIT_CALLBACK callback, PVOID userdata,
                               TP_CALLBACK_ENVIRON *environment, DWORD flags )
{
    struct threadpool_object *object;
    struct threadpool *pool;
    NTSTATUS status;

    object = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_WAIT;
    object->u.wait.callback = callback;
    object->u.wait.flags = flags;

    status = tp_waitqueue_lock( object );
    if (status)
    {
        tp_threadpool_unlock( pool );
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_WAIT *)object;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpAllocWait     (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocWait( TP_WAIT **out, PTP_WAIT_CALLBACK callback, PVOID userdata,
                             TP_CALLBACK_ENVIRON *environment )
{
    TRACE( "%p %p %p %p\n", out, callback, userdata, environment );
    return tp_alloc_wait( out, callback, userdata, environment, WT_EXECUTEONLYONCE );
}

/***********************************************************************
 *           TpAllocWork    (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocWork( TP_WORK **out, PTP_WORK_CALLBACK callback, PVOID userdata,
                             TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool_object *object;
    struct threadpool *pool;
    NTSTATUS status;

    TRACE( "%p %p %p %p\n", out, callback, userdata, environment );

    object = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_WORK;
    object->u.work.callback = callback;
    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_WORK *)object;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpCancelAsyncIoOperation    (NTDLL.@)
 */
void WINAPI TpCancelAsyncIoOperation( TP_IO *io )
{
    struct threadpool_object *this = impl_from_TP_IO( io );

    TRACE( "%p\n", io );

    RtlEnterCriticalSection( &this->pool->cs );

    TRACE("pending_count %u.\n", this->u.io.pending_count);

    this->u.io.pending_count--;
    if (object_is_finished( this, TRUE ))
        RtlWakeAllConditionVariable( &this->group_finished_event );
    if (object_is_finished( this, FALSE ))
        RtlWakeAllConditionVariable( &this->finished_event );

    RtlLeaveCriticalSection( &this->pool->cs );
}

/***********************************************************************
 *           TpCallbackLeaveCriticalSectionOnCompletion    (NTDLL.@)
 */
VOID WINAPI TpCallbackLeaveCriticalSectionOnCompletion( TP_CALLBACK_INSTANCE *instance, CRITICAL_SECTION *crit )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );

    TRACE( "%p %p\n", instance, crit );

    if (!this->cleanup.critical_section)
        this->cleanup.critical_section = crit;
}

/***********************************************************************
 *           TpCallbackMayRunLong    (NTDLL.@)
 */
NTSTATUS WINAPI TpCallbackMayRunLong( TP_CALLBACK_INSTANCE *instance )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );
    struct threadpool_object *object = this->object;
    struct threadpool *pool;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%p\n", instance );

    if (this->threadid != GetCurrentThreadId())
    {
        ERR("called from wrong thread, ignoring\n");
        return STATUS_UNSUCCESSFUL; /* FIXME */
    }

    if (this->may_run_long)
        return STATUS_SUCCESS;

    pool = object->pool;
    RtlEnterCriticalSection( &pool->cs );

    /* Start new worker threads if required. */
    if (pool->num_busy_workers >= pool->num_workers)
    {
        if (pool->num_workers < pool->max_workers)
        {
            status = tp_new_worker_thread( pool );
        }
        else
        {
            status = STATUS_TOO_MANY_THREADS;
        }
    }

    RtlLeaveCriticalSection( &pool->cs );
    this->may_run_long = TRUE;
    return status;
}

/***********************************************************************
 *           TpCallbackReleaseMutexOnCompletion    (NTDLL.@)
 */
VOID WINAPI TpCallbackReleaseMutexOnCompletion( TP_CALLBACK_INSTANCE *instance, HANDLE mutex )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );

    TRACE( "%p %p\n", instance, mutex );

    if (!this->cleanup.mutex)
        this->cleanup.mutex = mutex;
}

/***********************************************************************
 *           TpCallbackReleaseSemaphoreOnCompletion    (NTDLL.@)
 */
VOID WINAPI TpCallbackReleaseSemaphoreOnCompletion( TP_CALLBACK_INSTANCE *instance, HANDLE semaphore, DWORD count )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );

    TRACE( "%p %p %lu\n", instance, semaphore, count );

    if (!this->cleanup.semaphore)
    {
        this->cleanup.semaphore = semaphore;
        this->cleanup.semaphore_count = count;
    }
}

/***********************************************************************
 *           TpCallbackSetEventOnCompletion    (NTDLL.@)
 */
VOID WINAPI TpCallbackSetEventOnCompletion( TP_CALLBACK_INSTANCE *instance, HANDLE event )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );

    TRACE( "%p %p\n", instance, event );

    if (!this->cleanup.event)
        this->cleanup.event = event;
}

/***********************************************************************
 *           TpCallbackUnloadDllOnCompletion    (NTDLL.@)
 */
VOID WINAPI TpCallbackUnloadDllOnCompletion( TP_CALLBACK_INSTANCE *instance, HMODULE module )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );

    TRACE( "%p %p\n", instance, module );

    if (!this->cleanup.library)
        this->cleanup.library = module;
}

/***********************************************************************
 *           TpDisassociateCallback    (NTDLL.@)
 */
VOID WINAPI TpDisassociateCallback( TP_CALLBACK_INSTANCE *instance )
{
    struct threadpool_instance *this = impl_from_TP_CALLBACK_INSTANCE( instance );
    struct threadpool_object *object = this->object;
    struct threadpool *pool;

    TRACE( "%p\n", instance );

    if (this->threadid != GetCurrentThreadId())
    {
        ERR("called from wrong thread, ignoring\n");
        return;
    }

    if (!this->associated)
        return;

    pool = object->pool;
    RtlEnterCriticalSection( &pool->cs );

    object->num_associated_callbacks--;
    if (object_is_finished( object, FALSE ))
        RtlWakeAllConditionVariable( &object->finished_event );

    RtlLeaveCriticalSection( &pool->cs );
    this->associated = FALSE;
}

/***********************************************************************
 *           TpIsTimerSet    (NTDLL.@)
 */
BOOL WINAPI TpIsTimerSet( TP_TIMER *timer )
{
    struct threadpool_object *this = impl_from_TP_TIMER( timer );

    TRACE( "%p\n", timer );

    return this->u.timer.timer_set;
}

/***********************************************************************
 *           TpPostWork    (NTDLL.@)
 */
VOID WINAPI TpPostWork( TP_WORK *work )
{
    struct threadpool_object *this = impl_from_TP_WORK( work );

    TRACE( "%p\n", work );

    tp_object_submit( this, FALSE );
}

/***********************************************************************
 *           TpReleaseCleanupGroup    (NTDLL.@)
 */
VOID WINAPI TpReleaseCleanupGroup( TP_CLEANUP_GROUP *group )
{
    struct threadpool_group *this = impl_from_TP_CLEANUP_GROUP( group );

    TRACE( "%p\n", group );

    tp_group_shutdown( this );
    tp_group_release( this );
}

/***********************************************************************
 *           TpReleaseCleanupGroupMembers    (NTDLL.@)
 */
VOID WINAPI TpReleaseCleanupGroupMembers( TP_CLEANUP_GROUP *group, BOOL cancel_pending, PVOID userdata )
{
    struct threadpool_group *this = impl_from_TP_CLEANUP_GROUP( group );
    struct threadpool_object *object, *next;
    struct list members;

    TRACE( "%p %u %p\n", group, cancel_pending, userdata );

    RtlEnterCriticalSection( &this->cs );

    /* Unset group, increase references, and mark objects for shutdown */
    LIST_FOR_EACH_ENTRY_SAFE( object, next, &this->members, struct threadpool_object, group_entry )
    {
        assert( object->group == this );
        assert( object->is_group_member );

        if (InterlockedIncrement( &object->refcount ) == 1)
        {
            /* Object is basically already destroyed, but group reference
             * was not deleted yet. We can safely ignore this object. */
            InterlockedDecrement( &object->refcount );
            list_remove( &object->group_entry );
            object->is_group_member = FALSE;
            continue;
        }

        object->is_group_member = FALSE;
        tp_object_prepare_shutdown( object );
    }

    /* Move members to a new temporary list */
    list_init( &members );
    list_move_tail( &members, &this->members );

    RtlLeaveCriticalSection( &this->cs );

    /* Cancel pending callbacks if requested */
    if (cancel_pending)
    {
        LIST_FOR_EACH_ENTRY( object, &members, struct threadpool_object, group_entry )
        {
            tp_object_cancel( object );
        }
    }

    /* Wait for remaining callbacks to finish */
    LIST_FOR_EACH_ENTRY_SAFE( object, next, &members, struct threadpool_object, group_entry )
    {
        tp_object_wait( object, TRUE );

        if (!object->shutdown)
        {
            /* Execute group cancellation callback if defined, and if this was actually a group cancel. */
            if (cancel_pending && object->group_cancel_callback)
            {
                TRACE( "executing group cancel callback %p(%p, %p)\n",
                       object->group_cancel_callback, object->userdata, userdata );
                object->group_cancel_callback( object->userdata, userdata );
                TRACE( "callback %p returned\n", object->group_cancel_callback );
            }

            if (object->type != TP_OBJECT_TYPE_SIMPLE)
                tp_object_release( object );
        }

        object->shutdown = TRUE;
        tp_object_release( object );
    }
}

/***********************************************************************
 *           TpReleaseIoCompletion    (NTDLL.@)
 */
void WINAPI TpReleaseIoCompletion( TP_IO *io )
{
    struct threadpool_object *this = impl_from_TP_IO( io );
    BOOL can_destroy;

    TRACE( "%p\n", io );

    RtlEnterCriticalSection( &this->pool->cs );
    this->u.io.shutting_down = TRUE;
    can_destroy = !this->u.io.pending_count && !this->u.io.skipped_count;
    RtlLeaveCriticalSection( &this->pool->cs );

    if (can_destroy)
    {
        tp_object_prepare_shutdown( this );
        this->shutdown = TRUE;
        tp_object_release( this );
    }
}

/***********************************************************************
 *           TpReleasePool    (NTDLL.@)
 */
VOID WINAPI TpReleasePool( TP_POOL *pool )
{
    struct threadpool *this = impl_from_TP_POOL( pool );

    TRACE( "%p\n", pool );

    tp_threadpool_shutdown( this );
    tp_threadpool_release( this );
}

/***********************************************************************
 *           TpReleaseTimer     (NTDLL.@)
 */
VOID WINAPI TpReleaseTimer( TP_TIMER *timer )
{
    struct threadpool_object *this = impl_from_TP_TIMER( timer );

    TRACE( "%p\n", timer );

    tp_object_prepare_shutdown( this );
    this->shutdown = TRUE;
    tp_object_release( this );
}

/***********************************************************************
 *           TpReleaseWait    (NTDLL.@)
 */
VOID WINAPI TpReleaseWait( TP_WAIT *wait )
{
    struct threadpool_object *this = impl_from_TP_WAIT( wait );

    TRACE( "%p\n", wait );

    tp_object_prepare_shutdown( this );
    this->shutdown = TRUE;
    tp_object_release( this );
}

/***********************************************************************
 *           TpReleaseWork    (NTDLL.@)
 */
VOID WINAPI TpReleaseWork( TP_WORK *work )
{
    struct threadpool_object *this = impl_from_TP_WORK( work );

    TRACE( "%p\n", work );

    tp_object_prepare_shutdown( this );
    this->shutdown = TRUE;
    tp_object_release( this );
}

/***********************************************************************
 *           TpSetPoolMaxThreads    (NTDLL.@)
 */
VOID WINAPI TpSetPoolMaxThreads( TP_POOL *pool, DWORD maximum )
{
    struct threadpool *this = impl_from_TP_POOL( pool );

    TRACE( "%p %lu\n", pool, maximum );

    RtlEnterCriticalSection( &this->cs );
    this->max_workers = max( maximum, 1 );
    this->min_workers = min( this->min_workers, this->max_workers );
    RtlLeaveCriticalSection( &this->cs );
}

/***********************************************************************
 *           TpSetPoolMinThreads    (NTDLL.@)
 */
BOOL WINAPI TpSetPoolMinThreads( TP_POOL *pool, DWORD minimum )
{
    struct threadpool *this = impl_from_TP_POOL( pool );
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%p %lu\n", pool, minimum );

    RtlEnterCriticalSection( &this->cs );

    while (this->num_workers < minimum)
    {
        status = tp_new_worker_thread( this );
        if (status != STATUS_SUCCESS)
            break;
    }

    if (status == STATUS_SUCCESS)
    {
        this->min_workers = minimum;
        this->max_workers = max( this->min_workers, this->max_workers );
    }

    RtlLeaveCriticalSection( &this->cs );
    return !status;
}

/***********************************************************************
 *           TpSetTimer    (NTDLL.@)
 */
VOID WINAPI TpSetTimer( TP_TIMER *timer, LARGE_INTEGER *timeout, LONG period, LONG window_length )
{
    struct threadpool_object *this = impl_from_TP_TIMER( timer );
    struct threadpool_object *other_timer;
    BOOL submit_timer = FALSE;
    ULONGLONG timestamp;

    TRACE( "%p %p %lu %lu\n", timer, timeout, period, window_length );

    RtlEnterCriticalSection( &timerqueue.cs );

    assert( this->u.timer.timer_initialized );
    this->u.timer.timer_set = timeout != NULL;

    /* Convert relative timeout to absolute timestamp and handle a timeout
     * of zero, which means that the timer is submitted immediately. */
    if (timeout)
    {
        timestamp = timeout->QuadPart;
        if ((LONGLONG)timestamp < 0)
        {
            LARGE_INTEGER now;
            NtQuerySystemTime( &now );
            timestamp = now.QuadPart - timestamp;
        }
        else if (!timestamp)
        {
            if (!period)
                timeout = NULL;
            else
            {
                LARGE_INTEGER now;
                NtQuerySystemTime( &now );
                timestamp = now.QuadPart + (ULONGLONG)period * 10000;
            }
            submit_timer = TRUE;
        }
    }

    /* First remove existing timeout. */
    if (this->u.timer.timer_pending)
    {
        list_remove( &this->u.timer.timer_entry );
        this->u.timer.timer_pending = FALSE;
    }

    /* If the timer was enabled, then add it back to the queue. */
    if (timeout)
    {
        this->u.timer.timeout       = timestamp;
        this->u.timer.period        = period;
        this->u.timer.window_length = window_length;

        LIST_FOR_EACH_ENTRY( other_timer, &timerqueue.pending_timers,
                             struct threadpool_object, u.timer.timer_entry )
        {
            assert( other_timer->type == TP_OBJECT_TYPE_TIMER );
            if (this->u.timer.timeout < other_timer->u.timer.timeout)
                break;
        }
        list_add_before( &other_timer->u.timer.timer_entry, &this->u.timer.timer_entry );

        /* Wake up the timer thread when the timeout has to be updated. */
        if (list_head( &timerqueue.pending_timers ) == &this->u.timer.timer_entry )
            RtlWakeAllConditionVariable( &timerqueue.update_event );

        this->u.timer.timer_pending = TRUE;
    }

    RtlLeaveCriticalSection( &timerqueue.cs );

    if (submit_timer)
       tp_object_submit( this, FALSE );
}

/***********************************************************************
 *           TpSetWait    (NTDLL.@)
 */
VOID WINAPI TpSetWait( TP_WAIT *wait, HANDLE handle, LARGE_INTEGER *timeout )
{
    struct threadpool_object *this = impl_from_TP_WAIT( wait );
    ULONGLONG timestamp = MAXLONGLONG;

    TRACE( "%p %p %p\n", wait, handle, timeout );

    RtlEnterCriticalSection( &waitqueue.cs );

    assert( this->u.wait.bucket );
    this->u.wait.handle = handle;

    if (handle || this->u.wait.wait_pending)
    {
        struct waitqueue_bucket *bucket = this->u.wait.bucket;
        list_remove( &this->u.wait.wait_entry );

        /* Convert relative timeout to absolute timestamp. */
        if (handle && timeout)
        {
            timestamp = timeout->QuadPart;
            if ((LONGLONG)timestamp < 0)
            {
                LARGE_INTEGER now;
                NtQuerySystemTime( &now );
                timestamp = now.QuadPart - timestamp;
            }
        }

        /* Add wait object back into one of the queues. */
        if (handle)
        {
            list_add_tail( &bucket->waiting, &this->u.wait.wait_entry );
            this->u.wait.wait_pending = TRUE;
            this->u.wait.timeout = timestamp;
        }
        else
        {
            list_add_tail( &bucket->reserved, &this->u.wait.wait_entry );
            this->u.wait.wait_pending = FALSE;
        }

        /* Wake up the wait queue thread. */
        NtSetEvent( bucket->update_event, NULL );
    }

    RtlLeaveCriticalSection( &waitqueue.cs );
}

/***********************************************************************
 *           TpSimpleTryPost    (NTDLL.@)
 */
NTSTATUS WINAPI TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                 TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool_object *object;
    struct threadpool *pool;
    NTSTATUS status;

    TRACE( "%p %p %p\n", callback, userdata, environment );

    object = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( GetProcessHeap(), 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_SIMPLE;
    object->u.simple.callback = callback;
    tp_object_initialize( object, pool, userdata, environment );

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpStartAsyncIoOperation    (NTDLL.@)
 */
void WINAPI TpStartAsyncIoOperation( TP_IO *io )
{
    struct threadpool_object *this = impl_from_TP_IO( io );

    TRACE( "%p\n", io );

    RtlEnterCriticalSection( &this->pool->cs );

    this->u.io.pending_count++;

    RtlLeaveCriticalSection( &this->pool->cs );
}

/***********************************************************************
 *           TpWaitForIoCompletion    (NTDLL.@)
 */
void WINAPI TpWaitForIoCompletion( TP_IO *io, BOOL cancel_pending )
{
    struct threadpool_object *this = impl_from_TP_IO( io );

    TRACE( "%p %d\n", io, cancel_pending );

    if (cancel_pending)
        tp_object_cancel( this );
    tp_object_wait( this, FALSE );
}

/***********************************************************************
 *           TpWaitForTimer    (NTDLL.@)
 */
VOID WINAPI TpWaitForTimer( TP_TIMER *timer, BOOL cancel_pending )
{
    struct threadpool_object *this = impl_from_TP_TIMER( timer );

    TRACE( "%p %d\n", timer, cancel_pending );

    if (cancel_pending)
        tp_object_cancel( this );
    tp_object_wait( this, FALSE );
}

/***********************************************************************
 *           TpWaitForWait    (NTDLL.@)
 */
VOID WINAPI TpWaitForWait( TP_WAIT *wait, BOOL cancel_pending )
{
    struct threadpool_object *this = impl_from_TP_WAIT( wait );

    TRACE( "%p %d\n", wait, cancel_pending );

    if (cancel_pending)
        tp_object_cancel( this );
    tp_object_wait( this, FALSE );
}

/***********************************************************************
 *           TpWaitForWork    (NTDLL.@)
 */
VOID WINAPI TpWaitForWork( TP_WORK *work, BOOL cancel_pending )
{
    struct threadpool_object *this = impl_from_TP_WORK( work );

    TRACE( "%p %u\n", work, cancel_pending );

    if (cancel_pending)
        tp_object_cancel( this );
    tp_object_wait( this, FALSE );
}

/***********************************************************************
 *           TpSetPoolStackInformation    (NTDLL.@)
 */
NTSTATUS WINAPI TpSetPoolStackInformation( TP_POOL *pool, TP_POOL_STACK_INFORMATION *stack_info )
{
    struct threadpool *this = impl_from_TP_POOL( pool );

    TRACE( "%p %p\n", pool, stack_info );

    if (!stack_info)
        return STATUS_INVALID_PARAMETER;

    RtlEnterCriticalSection( &this->cs );
    this->stack_info = *stack_info;
    RtlLeaveCriticalSection( &this->cs );

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpQueryPoolStackInformation    (NTDLL.@)
 */
NTSTATUS WINAPI TpQueryPoolStackInformation( TP_POOL *pool, TP_POOL_STACK_INFORMATION *stack_info )
{
    struct threadpool *this = impl_from_TP_POOL( pool );

    TRACE( "%p %p\n", pool, stack_info );

    if (!stack_info)
        return STATUS_INVALID_PARAMETER;

    RtlEnterCriticalSection( &this->cs );
    *stack_info = this->stack_info;
    RtlLeaveCriticalSection( &this->cs );

    return STATUS_SUCCESS;
}

static void CALLBACK rtl_wait_callback( TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WAIT *wait, TP_WAIT_RESULT result )
{
    struct threadpool_object *object = impl_from_TP_WAIT(wait);
    object->u.wait.rtl_callback( userdata, result != STATUS_WAIT_0 );
}

/***********************************************************************
 *              RtlRegisterWait   (NTDLL.@)
 *
 * Registers a wait for a handle to become signaled.
 *
 * PARAMS
 *  NewWaitObject [I] Handle to the new wait object. Use RtlDeregisterWait() to free it.
 *  Object   [I] Object to wait to become signaled.
 *  Callback [I] Callback function to execute when the wait times out or the handle is signaled.
 *  Context  [I] Context to pass to the callback function when it is executed.
 *  Milliseconds [I] Number of milliseconds to wait before timing out.
 *  Flags    [I] Flags. See notes.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 *
 * NOTES
 *  Flags can be one or more of the following:
 *|WT_EXECUTEDEFAULT - Executes the work item in a non-I/O worker thread.
 *|WT_EXECUTEINIOTHREAD - Executes the work item in an I/O worker thread.
 *|WT_EXECUTEINPERSISTENTTHREAD - Executes the work item in a thread that is persistent.
 *|WT_EXECUTELONGFUNCTION - Hints that the execution can take a long time.
 *|WT_TRANSFER_IMPERSONATION - Executes the function with the current access token.
 */
NTSTATUS WINAPI RtlRegisterWait( HANDLE *out, HANDLE handle, RTL_WAITORTIMERCALLBACKFUNC callback,
                                 void *context, ULONG milliseconds, ULONG flags )
{
    struct threadpool_object *object;
    TP_CALLBACK_ENVIRON environment;
    LARGE_INTEGER timeout;
    NTSTATUS status;
    TP_WAIT *wait;

    TRACE( "out %p, handle %p, callback %p, context %p, milliseconds %lu, flags %lx\n",
            out, handle, callback, context, milliseconds, flags );

    memset( &environment, 0, sizeof(environment) );
    environment.Version = 1;
    environment.u.s.LongFunction = (flags & WT_EXECUTELONGFUNCTION) != 0;
    environment.u.s.Persistent   = (flags & WT_EXECUTEINPERSISTENTTHREAD) != 0;

    flags &= (WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD | WT_EXECUTEINIOTHREAD);
    if ((status = tp_alloc_wait( &wait, rtl_wait_callback, context, &environment, flags )))
        return status;

    object = impl_from_TP_WAIT(wait);
    object->u.wait.rtl_callback = callback;

    RtlEnterCriticalSection( &waitqueue.cs );
    TpSetWait( (TP_WAIT *)object, handle, get_nt_timeout( &timeout, milliseconds ) );

    *out = object;
    RtlLeaveCriticalSection( &waitqueue.cs );

    return STATUS_SUCCESS;
}

/***********************************************************************
 *              RtlDeregisterWaitEx   (NTDLL.@)
 *
 * Cancels a wait operation and frees the resources associated with calling
 * RtlRegisterWait().
 *
 * PARAMS
 *  WaitObject [I] Handle to the wait object to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeregisterWaitEx( HANDLE handle, HANDLE event )
{
    struct threadpool_object *object = handle;
    NTSTATUS status;

    TRACE( "handle %p, event %p\n", handle, event );

    if (!object) return STATUS_INVALID_HANDLE;

    TpSetWait( (TP_WAIT *)object, NULL, NULL );

    if (event == INVALID_HANDLE_VALUE) TpWaitForWait( (TP_WAIT *)object, TRUE );
    else
    {
        assert( object->completed_event == NULL );
        object->completed_event = event;
    }

    RtlEnterCriticalSection( &object->pool->cs );
    if (object->num_pending_callbacks + object->num_running_callbacks
        + object->num_associated_callbacks) status = STATUS_PENDING;
    else status = STATUS_SUCCESS;
    RtlLeaveCriticalSection( &object->pool->cs );

    TpReleaseWait( (TP_WAIT *)object );
    return status;
}

/***********************************************************************
 *              RtlDeregisterWait   (NTDLL.@)
 *
 * Cancels a wait operation and frees the resources associated with calling
 * RtlRegisterWait().
 *
 * PARAMS
 *  WaitObject [I] Handle to the wait object to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeregisterWait(HANDLE WaitHandle)
{
    return RtlDeregisterWaitEx(WaitHandle, NULL);
}

#ifdef __REACTOS__
VOID
NTAPI
RtlpInitializeThreadPooling(
    VOID)
{
    RtlInitializeCriticalSection(&old_threadpool.threadpool_compl_cs);
    RtlInitializeCriticalSection(&timerqueue.cs);
    RtlInitializeCriticalSection(&waitqueue.cs);
    RtlInitializeCriticalSection(&ioqueue.cs);
}
#endif
