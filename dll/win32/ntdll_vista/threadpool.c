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
#define __HYBRIDSRC__
#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(threadpooling);

#include <wine/list.h>

#else
    
#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <limits.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(threadpool);
#endif 


#define TIMEOUT_INFINITE MAXLONGLONG

 
/*
 * Old thread pooling API
 */
 
struct rtl_work_item
{
    WORKERCALLBACKFUNC function;
    PVOID context;
};

#define EXPIRE_NEVER       (~(ULONGLONG)0)
#define TIMER_QUEUE_MAGIC  0x516d6954   /* TimQ */

static RTL_CRITICAL_SECTION_DEBUG critsect_compl_debug;

static struct
{
    HANDLE                  compl_port;
    RTL_CRITICAL_SECTION    threadpool_compl_cs;
}
old_threadpool =
{
    NULL,                                       /* compl_port */
    { &critsect_compl_debug, -1, 0, 0, 0, 0 },  /* threadpool_compl_cs */
};

static RTL_CRITICAL_SECTION_DEBUG critsect_compl_debug =
{
    0, 0, &old_threadpool.threadpool_compl_cs,
    { &critsect_compl_debug.ProcessLocksList, &critsect_compl_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": threadpool_compl_cs") }
};

struct wait_work_item
{
    HANDLE Object;
    HANDLE CancelEvent;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    ULONG Milliseconds;
    ULONG Flags;
    HANDLE CompletionEvent;
    LONG DeleteCount;
    volatile long int CallbackInProgress;
};

struct timer_queue;
struct queue_timer
{
    struct timer_queue *q;
    struct list entry;
    ULONG runcount;             /* number of callbacks pending execution */
    WAITORTIMERCALLBACKFUNC callback;
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
    RTL_CRITICAL_SECTION        cs;
    /* pool of work items, locked via .cs */
    struct list             pool;
    RTL_CONDITION_VARIABLE  update_event;
    /* information about worker threads, locked via .cs */
    int                     max_workers;
    int                     min_workers;
    int                     num_workers;
    int                     num_busy_workers;
};

enum threadpool_objtype
{
    TP_OBJECT_TYPE_SIMPLE,
    TP_OBJECT_TYPE_WORK,
    TP_OBJECT_TYPE_TIMER,
    TP_OBJECT_TYPE_WAIT
};

/* internal threadpool object representation */
struct threadpool_object
{
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
    /* information about the group, locked via .group->cs */
    struct list             group_entry;
    BOOL                    is_group_member;
    /* information about the pool, locked via .pool->cs */
    struct list             pool_entry;
    RTL_CONDITION_VARIABLE  finished_event;
    RTL_CONDITION_VARIABLE  group_finished_event;
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
        } wait;
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
        RTL_CRITICAL_SECTION    *critical_section;
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
    RTL_CRITICAL_SECTION        cs;
    /* list of group members, locked via .cs */
    struct list             members;
};

/* global timerqueue object */
static RTL_CRITICAL_SECTION_DEBUG timerqueue_debug;

static struct
{
    RTL_CRITICAL_SECTION        cs;
    LONG                    objcount;
    BOOL                    thread_running;
    struct list             pending_timers;
    RTL_CONDITION_VARIABLE  update_event;
}
timerqueue =
{
    { &timerqueue_debug, -1, 0, 0, 0, 0 },      /* cs */
    0,                                          /* objcount */
    FALSE,                                      /* thread_running */
    LIST_INIT( timerqueue.pending_timers ),     /* pending_timers */
    RTL_CONDITION_VARIABLE_INIT                 /* update_event */
};

static RTL_CRITICAL_SECTION_DEBUG timerqueue_debug =
{
    0, 0, &timerqueue.cs,
    { &timerqueue_debug.ProcessLocksList, &timerqueue_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": timerqueue.cs") }
};

/* global waitqueue object */
static RTL_CRITICAL_SECTION_DEBUG waitqueue_debug;

static struct
{
    RTL_CRITICAL_SECTION        cs;
    LONG                    num_buckets;
    struct list             buckets;
}
waitqueue =
{
    { &waitqueue_debug, -1, 0, 0, 0, 0 },       /* cs */
    0,                                          /* num_buckets */
    LIST_INIT( waitqueue.buckets )              /* buckets */
};

static RTL_CRITICAL_SECTION_DEBUG waitqueue_debug =
{
    0, 0, &waitqueue.cs,
    { &waitqueue_debug.ProcessLocksList, &waitqueue_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": waitqueue.cs") }
};

struct waitqueue_bucket
{
    struct list             bucket_entry;
    LONG                    objcount;
    struct list             reserved;
    struct list             waiting;
    HANDLE                  update_event;
};

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

static inline struct threadpool_group *impl_from_TP_CLEANUP_GROUP( TP_CLEANUP_GROUP *group )
{
    return (struct threadpool_group *)group;
}

static inline struct threadpool_instance *impl_from_TP_CALLBACK_INSTANCE( TP_CALLBACK_INSTANCE *instance )
{
    return (struct threadpool_instance *)instance;
}

static void CALLBACK threadpool_worker_proc( void *param );
static void tp_object_submit( struct threadpool_object *object, BOOL signaled );
static void tp_object_prepare_shutdown( struct threadpool_object *object );
static BOOL tp_object_release( struct threadpool_object *object );
static struct threadpool *default_threadpool = NULL;

static inline LONG interlocked_inc( PLONG dest )
{
    return InterlockedExchangeAdd( dest, 1 ) + 1;
}

static inline LONG interlocked_dec( PLONG dest )
{
    return InterlockedExchangeAdd( dest, -1 ) - 1;
}

NTSTATUS WINAPI TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                 TP_CALLBACK_ENVIRON *environment );



static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, ULONG timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}



/************************** Timer Queue Impl **************************/


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

/***********************************************************************
 *           timerqueue_thread_proc    (internal)
 */
static void CALLBACK timerqueue_thread_proc( void *param )
{
    ULONGLONG timeout_lower, timeout_upper, new_timeout;
    struct threadpool_object *other_timer;
    LARGE_INTEGER now, timeout;
    struct list *ptr;

    TRACE( "starting timer queue thread\n" );

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

        timeout_lower = TIMEOUT_INFINITE;
        timeout_upper = TIMEOUT_INFINITE;

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

    status = RtlCreateUserThread( (HANDLE)NtCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                  (PVOID)threadpool_worker_proc, pool, &thread, NULL );
    if (status == STATUS_SUCCESS)
    {
        interlocked_inc( &pool->refcount );
        pool->num_workers++;
        pool->num_busy_workers++;
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
        status = RtlCreateUserThread( (HANDLE)NtCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                      (PVOID)timerqueue_thread_proc, NULL, &thread, NULL );
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
static void CALLBACK waitqueue_thread_proc( void *param )
{
    struct threadpool_object *objects[MAXIMUM_WAITQUEUE_OBJECTS];
    HANDLE handles[MAXIMUM_WAITQUEUE_OBJECTS + 1];
    struct waitqueue_bucket *bucket = param;
    struct threadpool_object *wait, *next;
    LARGE_INTEGER now, timeout;
    DWORD num_handles;
    NTSTATUS status;

    TRACE( "starting wait queue thread\n" );

    RtlEnterCriticalSection( &waitqueue.cs );

    for (;;)
    {
        NtQuerySystemTime( &now );
        timeout.QuadPart = TIMEOUT_INFINITE;
        num_handles = 0;

        LIST_FOR_EACH_ENTRY_SAFE( wait, next, &bucket->waiting, struct threadpool_object,
                                  u.wait.wait_entry )
        {
            assert( wait->type == TP_OBJECT_TYPE_WAIT );
            if (wait->u.wait.timeout <= now.QuadPart)
            {
                /* Wait object timed out. */
                list_remove( &wait->u.wait.wait_entry );
                list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
                tp_object_submit( wait, FALSE );
            }
            else
            {
                if (wait->u.wait.timeout < timeout.QuadPart)
                    timeout.QuadPart = wait->u.wait.timeout;

                assert( num_handles < MAXIMUM_WAITQUEUE_OBJECTS );
                interlocked_inc( &wait->refcount );
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
            status = NtWaitForMultipleObjects( 1, &bucket->update_event, TRUE, FALSE, &timeout );
            RtlEnterCriticalSection( &waitqueue.cs );

            if (status == STATUS_TIMEOUT && !bucket->objcount)
                break;
        }
        else
        {
            handles[num_handles] = bucket->update_event;
            RtlLeaveCriticalSection( &waitqueue.cs );
            status = NtWaitForMultipleObjects( num_handles + 1, handles, TRUE, FALSE, &timeout );
            RtlEnterCriticalSection( &waitqueue.cs );

            if (status >= STATUS_WAIT_0 && status < STATUS_WAIT_0 + num_handles)
            {
                wait = objects[status - STATUS_WAIT_0];
                assert( wait->type == TP_OBJECT_TYPE_WAIT );
                if (wait->u.wait.bucket)
                {
                    /* Wait object signaled. */
                    assert( wait->u.wait.bucket == bucket );
                    list_remove( &wait->u.wait.wait_entry );
                    list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
                    tp_object_submit( wait, TRUE );
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
                if (other_bucket != bucket && other_bucket->objcount &&
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

    RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, bucket );
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
        if (bucket->objcount < MAXIMUM_WAITQUEUE_OBJECTS)
        {
            list_add_tail( &bucket->reserved, &wait->u.wait.wait_entry );
            wait->u.wait.bucket = bucket;
            bucket->objcount++;

            status = STATUS_SUCCESS;
            goto out;
        }
    }

    /* Create a new bucket and corresponding worker thread. */
    bucket = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*bucket) );
    if (!bucket)
    {
        status = STATUS_NO_MEMORY;
        goto out;
    }

    bucket->objcount = 0;
    list_init( &bucket->reserved );
    list_init( &bucket->waiting );

    status = NtCreateEvent( &bucket->update_event, EVENT_ALL_ACCESS,
                            NULL, SynchronizationEvent, FALSE );
    if (status)
    {
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, bucket );
        goto out;
    }

    status = RtlCreateUserThread( (HANDLE)NtCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                  (PVOID)waitqueue_thread_proc, bucket, &thread, NULL );
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
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, bucket );
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

/***********************************************************************
 *           tp_threadpool_alloc    (internal)
 *
 * Allocates a new threadpool object.
 */
static NTSTATUS tp_threadpool_alloc( struct threadpool **out )
{
    struct threadpool *pool;

    pool = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*pool) );
    if (!pool)
        return STATUS_NO_MEMORY;

    pool->refcount              = 1;
    pool->objcount              = 0;
    pool->shutdown              = FALSE;

    RtlInitializeCriticalSection( &pool->cs );
    pool->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": threadpool.cs");

    list_init( &pool->pool );
    RtlInitializeConditionVariable( &pool->update_event );

    pool->max_workers           = 500;
    pool->min_workers           = 0;
    pool->num_workers           = 0;
    pool->num_busy_workers      = 0;

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
    if (interlocked_dec( &pool->refcount ))
        return FALSE;

    TRACE( "destroying threadpool %p\n", pool );

    assert( pool->shutdown );
    assert( !pool->objcount );
    assert( list_empty( &pool->pool ) );

    pool->cs.DebugInfo->Spare[0] = 0;
    RtlDeleteCriticalSection( &pool->cs );

    RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, pool );
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
        pool = (struct threadpool *)environment->Pool;

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
        interlocked_inc( &pool->refcount );
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

    group = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*group) );
    if (!group)
        return STATUS_NO_MEMORY;

    group->refcount     = 1;
    group->shutdown     = FALSE;

    RtlInitializeCriticalSection( &group->cs );
    group->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": threadpool_group.cs");

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
    if (interlocked_dec( &group->refcount ))
        return FALSE;

    TRACE( "destroying group %p\n", group );

    assert( group->shutdown );
    assert( list_empty( &group->members ) );

    group->cs.DebugInfo->Spare[0] = 0;
    RtlDeleteCriticalSection( &group->cs );

    RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, group );
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

    memset( &object->group_entry, 0, sizeof(object->group_entry) );
    object->is_group_member         = FALSE;

    memset( &object->pool_entry, 0, sizeof(object->pool_entry) );
    RtlInitializeConditionVariable( &object->finished_event );
    RtlInitializeConditionVariable( &object->group_finished_event );
    object->num_pending_callbacks   = 0;
    object->num_running_callbacks   = 0;
    object->num_associated_callbacks = 0;

    if (environment)
    {
        if (environment->Version != 1 && environment->Version != 3)
            FIXME( "unsupported environment version %u\n", environment->Version );

        object->group = impl_from_TP_CLEANUP_GROUP( environment->CleanupGroup );
        object->group_cancel_callback   = environment->CleanupGroupCancelCallback;
        object->finalization_callback   = environment->FinalizationCallback;
        object->may_run_long            = environment->u.s.LongFunction != 0;
        object->race_dll                = environment->RaceDll;

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
        interlocked_inc( &group->refcount );

        RtlEnterCriticalSection( &group->cs );
        list_add_tail( &group->members, &object->group_entry );
        object->is_group_member = TRUE;
        RtlLeaveCriticalSection( &group->cs );
    }

    if (is_simple_callback)
        tp_object_release( object );
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
    interlocked_inc( &object->refcount );
    if (!object->num_pending_callbacks++)
        list_add_tail( &pool->pool, &object->pool_entry );

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
    RtlLeaveCriticalSection( &pool->cs );

    while (pending_callbacks--)
        tp_object_release( object );
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
    if (group_wait)
    {
        while (object->num_pending_callbacks || object->num_running_callbacks)
            RtlSleepConditionVariableCS( &object->group_finished_event, &pool->cs, NULL );
    }
    else
    {
        while (object->num_pending_callbacks || object->num_associated_callbacks)
            RtlSleepConditionVariableCS( &object->finished_event, &pool->cs, NULL );
    }
    RtlLeaveCriticalSection( &pool->cs );
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
}

/***********************************************************************
 *           tp_object_release    (internal)
 *
 * Releases a reference to a threadpool object.
 */
static BOOL tp_object_release( struct threadpool_object *object )
{
    if (interlocked_dec( &object->refcount ))
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

    RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
    return TRUE;
}

/***********************************************************************
 *           threadpool_worker_proc    (internal)
 */
static void CALLBACK threadpool_worker_proc( void *param )
{
    TP_CALLBACK_INSTANCE *callback_instance;
    struct threadpool_instance instance;
    struct threadpool *pool = param;
    TP_WAIT_RESULT wait_result = 0;
    LARGE_INTEGER timeout;
    struct list *ptr;
    NTSTATUS status;

    TRACE( "starting worker thread for pool %p\n", pool );

    RtlEnterCriticalSection( &pool->cs );
    pool->num_busy_workers--;
    for (;;)
    {
        while ((ptr = list_head( &pool->pool )))
        {
            struct threadpool_object *object = LIST_ENTRY( ptr, struct threadpool_object, pool_entry );
            assert( object->num_pending_callbacks > 0 );

            /* If further pending callbacks are queued, move the work item to
             * the end of the pool list. Otherwise remove it from the pool. */
            list_remove( &object->pool_entry );
            if (--object->num_pending_callbacks)
                list_add_tail( &pool->pool, &object->pool_entry );

            /* For wait objects check if they were signaled or have timed out. */
            if (object->type == TP_OBJECT_TYPE_WAIT)
            {
                wait_result = object->u.wait.signaled ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
                if (wait_result == WAIT_OBJECT_0) object->u.wait.signaled--;
            }

            /* Leave critical section and do the actual callback. */
            object->num_associated_callbacks++;
            object->num_running_callbacks++;
            pool->num_busy_workers++;
            RtlLeaveCriticalSection( &pool->cs );

            /* Initialize threadpool instance struct. */
            callback_instance = (TP_CALLBACK_INSTANCE *)&instance;
            instance.object                     = object;
            instance.threadid                   = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
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
                    TRACE( "executing wait callback %p(%p, %p, %p, %u)\n",
                           object->u.wait.callback, callback_instance, object->userdata, object, wait_result );
                    object->u.wait.callback( callback_instance, object->userdata, (TP_WAIT *)object, wait_result );
                    TRACE( "callback %p returned\n", object->u.wait.callback );
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
            RtlEnterCriticalSection( &pool->cs );
            pool->num_busy_workers--;

            /* Simple callbacks are automatically shutdown after execution. */
            if (object->type == TP_OBJECT_TYPE_SIMPLE)
            {
                tp_object_prepare_shutdown( object );
                object->shutdown = TRUE;
            }

            object->num_running_callbacks--;
            if (!object->num_pending_callbacks && !object->num_running_callbacks)
                RtlWakeAllConditionVariable( &object->group_finished_event );

            if (instance.associated)
            {
                object->num_associated_callbacks--;
                if (!object->num_pending_callbacks && !object->num_associated_callbacks)
                    RtlWakeAllConditionVariable( &object->finished_event );
            }

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
            !list_head( &pool->pool ) && (pool->num_workers > max( pool->min_workers, 1 ) ||
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

    object = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_TIMER;
    object->u.timer.callback = callback;

    status = tp_timerqueue_lock( object );
    if (status)
    {
        tp_threadpool_unlock( pool );
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
        return status;
    }

    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_TIMER *)object;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpAllocWait     (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocWait( TP_WAIT **out, PTP_WAIT_CALLBACK callback, PVOID userdata,
                             TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool_object *object;
    struct threadpool *pool;
    NTSTATUS status;

    TRACE( "%p %p %p %p\n", out, callback, userdata, environment );

    object = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_WAIT;
    object->u.wait.callback = callback;

    status = tp_waitqueue_lock( object );
    if (status)
    {
        tp_threadpool_unlock( pool );
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
        return status;
    }

    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_WAIT *)object;
    return STATUS_SUCCESS;
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

    object = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_WORK;
    object->u.work.callback = callback;
    tp_object_initialize( object, pool, userdata, environment );

    *out = (TP_WORK *)object;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           TpCallbackLeaveCriticalSectionOnCompletion    (NTDLL.@)
 */
VOID WINAPI TpCallbackLeaveCriticalSectionOnCompletion( TP_CALLBACK_INSTANCE *instance, RTL_CRITICAL_SECTION *crit )
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

    if (this->threadid != HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread))
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

    TRACE( "%p %p %u\n", instance, semaphore, count );

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

    if (this->threadid != HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread))
    {
        ERR("called from wrong thread, ignoring\n");
        return;
    }

    if (!this->associated)
        return;

    pool = object->pool;
    RtlEnterCriticalSection( &pool->cs );

    object->num_associated_callbacks--;
    if (!object->num_pending_callbacks && !object->num_associated_callbacks)
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

        if (interlocked_inc( &object->refcount ) == 1)
        {
            /* Object is basically already destroyed, but group reference
             * was not deleted yet. We can safely ignore this object. */
            interlocked_dec( &object->refcount );
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

    TRACE( "%p %u\n", pool, maximum );

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

    TRACE( "%p %u\n", pool, minimum );

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

    TRACE( "%p %p %u %u\n", timer, timeout, period, window_length );

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
    ULONGLONG timestamp = TIMEOUT_INFINITE;
    BOOL submit_wait = FALSE;

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
            else if (!timestamp)
            {
                submit_wait = TRUE;
                handle = NULL;
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

    if (submit_wait)
        tp_object_submit( this, FALSE );
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

    object = RtlAllocateHeap( NtCurrentPeb()->ProcessHeap, 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    status = tp_threadpool_lock( &pool, environment );
    if (status)
    {
        RtlFreeHeap( NtCurrentPeb()->ProcessHeap, 0, object );
        return status;
    }

    object->type = TP_OBJECT_TYPE_SIMPLE;
    object->u.simple.callback = callback;
    tp_object_initialize( object, pool, userdata, environment );

    return STATUS_SUCCESS;
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
