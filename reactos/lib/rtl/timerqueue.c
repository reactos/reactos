/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Timer Queue implementation
 * FILE:              lib/rtl/timerqueue.c
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include <wine/list.h>

/* FUNCTIONS ***************************************************************/

typedef VOID (CALLBACK *WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );

HANDLE TimerThreadHandle = NULL;

NTSTATUS
RtlpInitializeTimerThread(VOID)
{
    return STATUS_NOT_IMPLEMENTED;
}

static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, ULONG timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

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
    BOOL destroy;      /* timer should be deleted; once set, never unset */
    HANDLE event;      /* removal event */
};

struct timer_queue
{
    RTL_CRITICAL_SECTION cs;
    struct list timers;          /* sorted by expiration time */
    BOOL quit;         /* queue should be deleted; once set, never unset */
    HANDLE event;
    HANDLE thread;
};

#define EXPIRE_NEVER (~(ULONGLONG) 0)

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
    RtlFreeHeap(RtlGetProcessHeap(), 0, t);

    if (q->quit && list_count(&q->timers) == 0)
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
    LARGE_INTEGER now;
    NtQuerySystemTime(&now);
    return now.QuadPart / 10000;
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
        t = LIST_ENTRY(list_head(&q->timers), struct queue_timer, entry);
        if (!t->destroy && t->expire <= queue_current_time())
        {
            ++t->runcount;
            queue_move_timer(
                t, t->period ? queue_current_time() + t->period : EXPIRE_NEVER,
                FALSE);
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
            NTSTATUS status = RtlQueueWorkItem((WORKERCALLBACKFUNC)timer_callback_wrapper, t, flags);
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

static void WINAPI timer_queue_thread_proc(LPVOID p)
{
    struct timer_queue *q = p;
    ULONG timeout_ms;

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
            if (q->quit && list_count(&q->timers) == 0)
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
    RtlFreeHeap(RtlGetProcessHeap(), 0, q);
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
    struct timer_queue *q = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof *q);
    if (!q)
        return STATUS_NO_MEMORY;

    RtlInitializeCriticalSection(&q->cs);
    list_init(&q->timers);
    q->quit = FALSE;
    status = NtCreateEvent(&q->event, EVENT_ALL_ACCESS, NULL, FALSE, FALSE);
    if (status != STATUS_SUCCESS)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, q);
        return status;
    }
    status = RtlCreateUserThread(NtCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                 (PTHREAD_START_ROUTINE)timer_queue_thread_proc, q, &q->thread, NULL);
    if (status != STATUS_SUCCESS)
    {
        NtClose(q->event);
        RtlFreeHeap(RtlGetProcessHeap(), 0, q);
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

    if (!q)
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
            DPRINT1("asynchronous return on completion event unimplemented\n");
            NtWaitForSingleObject(thread, FALSE, NULL);
            NtSetEvent(CompletionEvent, NULL);
        }
        status = STATUS_PENDING;
    }

    NtClose(thread);
    return status;
}

static struct timer_queue *default_timer_queue;

static struct timer_queue *get_timer_queue(HANDLE TimerQueue)
{
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
                PVOID p = _InterlockedCompareExchangePointer(
                    (void **) &default_timer_queue, q, NULL);
                if (p)
                    /* Got beat to the punch.  */
                    RtlDeleteTimerQueueEx(p, NULL);
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
 *  NewTimer   [O] The newly created timer.
 *  TimerQueue [I] The queue to hold the timer.
 *  Callback   [I] The callback to fire.
 *  Parameter  [I] The argument for the callback.
 *  DueTime    [I] The delay, in milliseconds, before first firing the
 *                 timer.
 *  Period     [I] The period, in milliseconds, at which to fire the timer
 *                 after the first callback.  If zero, the timer will only
 *                 fire once.  It still needs to be deleted with
 *                 RtlDeleteTimer.
 * Flags       [I] Flags controling the execution of the callback.  In
 *                 addition to the WT_* thread pool flags (see
 *                 RtlQueueWorkItem), WT_EXECUTEINTIMERTHREAD and
 *                 WT_EXECUTEONLYONCE are supported.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlCreateTimer(HANDLE TimerQueue, PHANDLE NewTimer,
                               WAITORTIMERCALLBACKFUNC Callback,
                               PVOID Parameter, DWORD DueTime, DWORD Period,
                               ULONG Flags)
{
    NTSTATUS status;
    struct queue_timer *t;
    struct timer_queue *q = get_timer_queue(TimerQueue);
    if (!q)
        return STATUS_NO_MEMORY;

    t = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof *t);
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
        RtlFreeHeap(RtlGetProcessHeap(), 0, t);

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
    struct timer_queue *q = t->q;
    NTSTATUS status = STATUS_PENDING;
    HANDLE event = NULL;

    if (CompletionEvent == INVALID_HANDLE_VALUE)
        status = NtCreateEvent(&event, EVENT_ALL_ACCESS, NULL, FALSE, FALSE);
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
            NtWaitForSingleObject(event, FALSE, NULL);
        NtClose(event);
    }

    return status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteTimerQueue(HANDLE TimerQueue)
{
    return RtlDeleteTimerQueueEx(TimerQueue, INVALID_HANDLE_VALUE);
}

/* EOF */
