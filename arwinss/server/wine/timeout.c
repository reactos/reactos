/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL v2.1 or any later - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/wine/timer.c
 * PURPOSE:         Timer code for user server
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#include "object.h"

#define NDEBUG
#include <debug.h>

PKTIMER MasterTimer;
static HANDLE TimerThreadHandle;
static CLIENT_ID TimerThreadId;
static KEVENT TimerThreadsStart;
static PVOID TimerWaitObjects[2];

/* PRIVATE FUNCTIONS *********************************************************/

/****************************************************************/
/* timeouts support */

struct timeout_user
{
    struct list           entry;      /* entry in sorted timeout list */
    timeout_t             when;       /* timeout expiry (absolute time) */
    timeout_callback      callback;   /* callback function */
    void                 *private;    /* callback private data */
};

static struct list timeout_list = LIST_INIT(timeout_list);   /* sorted timeouts list */

/* add a timeout user */
struct timeout_user *add_timeout_user( timeout_t when, timeout_callback func, void *private )
{
    struct timeout_user *user;
    struct list *ptr;
    timeout_t current_time;

    DPRINT("add_timeout_user(when %I64d, func %p)\n", when, func);

    get_current_time(&current_time);

    if (!(user = mem_alloc( sizeof(*user) ))) return NULL;
    user->when     = (when > 0) ? when : current_time - when;
    user->callback = func;
    user->private  = private;

    /* Now insert it in the linked list */

    LIST_FOR_EACH( ptr, &timeout_list )
    {
        struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
        if (timeout->when >= user->when) break;
    }
    list_add_before( ptr, &user->entry );

    /* Inform timeout thread that we have a new timer */
    KeSetEvent(&TimerThreadsStart, IO_NO_INCREMENT, FALSE);

    return user;
}

/* remove a timeout user */
void remove_timeout_user( struct timeout_user *user )
{
    list_remove( &user->entry );
    ExFreePool( user );
}

/* return a text description of a timeout for debugging purposes */
const char *get_timeout_str( timeout_t timeout )
{
    static char buffer[64];
    long secs, nsecs;
    timeout_t current_time;

    get_current_time(&current_time);

    if (!timeout) return "0";
    if (timeout == TIMEOUT_INFINITE) return "infinite";

    if (timeout < 0)  /* relative */
    {
        secs = -timeout / TICKS_PER_SEC;
        nsecs = -timeout % TICKS_PER_SEC;
        sprintf( buffer, "+%ld.%07ld", secs, nsecs );
    }
    else  /* absolute */
    {
        secs = (timeout - current_time) / TICKS_PER_SEC;
        nsecs = (timeout - current_time) % TICKS_PER_SEC;
        if (nsecs < 0)
        {
            nsecs += TICKS_PER_SEC;
            secs--;
        }
        if (secs >= 0)
            sprintf( buffer, "%x%08x (+%ld.%07ld)",
                     (unsigned int)(timeout >> 32), (unsigned int)timeout, secs, nsecs );
        else
            sprintf( buffer, "%x%08x (-%ld.%07ld)",
                     (unsigned int)(timeout >> 32), (unsigned int)timeout,
                     -(secs + 1), TICKS_PER_SEC - nsecs );
    }
    return buffer;
}

/* process pending timeouts and return the time until the next timeout, in milliseconds */
static void get_next_timeout(LARGE_INTEGER *result)
{
    timeout_t current_time;
    get_current_time(&current_time);

    if (!list_empty( &timeout_list ))
    {
        struct list expired_list, *ptr;

        /* first remove all expired timers from the list */

        list_init( &expired_list );
        while ((ptr = list_head( &timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );

            if (timeout->when <= current_time)
            {
                list_remove( &timeout->entry );
                list_add_tail( &expired_list, &timeout->entry );
            }
            else break;
        }

        /* now call the callback for all the removed timers */

        while ((ptr = list_head( &expired_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            list_remove( &timeout->entry );
            timeout->callback( timeout->private );
            ExFreePool( timeout );
        }

        if ((ptr = list_head( &timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            LARGE_INTEGER diff;
            diff.QuadPart = timeout->when - current_time;
            //DPRINT1("diff %d, when %I64d, current %I64d\n", diff, timeout->when, current_time);
            if (diff.QuadPart < 0) diff.QuadPart = 0;

            result->QuadPart = diff.QuadPart;
            return;
        }
    }

    result->QuadPart = -1LL;
    return;  /* no pending timeouts */
}

VOID
ProcessTimers()
{
    LARGE_INTEGER timeout;
    LARGE_INTEGER DueTime;

    get_next_timeout(&timeout);
    DPRINT("ProcessTimers() timeout %I64d\n", timeout.QuadPart);
    if (timeout.QuadPart != -1LL)
    {
        /* Wait for the next time out */
        DueTime.QuadPart = -timeout.QuadPart;
        KeSetTimer(MasterTimer, DueTime, NULL);
    }
    else
    {
        /* Reset the timer making it signal in 60 seconds */
        DueTime.QuadPart = Int32x32To64(60, -TICKS_PER_SEC);
        KeSetTimer(MasterTimer, DueTime, NULL);
    }
}

static VOID APIENTRY
TimerThreadMain(PVOID StartContext)
{
    NTSTATUS Status;

    TimerWaitObjects[0] = &TimerThreadsStart;
    TimerWaitObjects[1] = MasterTimer;

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb, LOW_REALTIME_PRIORITY + 3);

    for(;;)
    {
        Status = KeWaitForMultipleObjects(2,
                                          TimerWaitObjects,
                                          WaitAny,
                                          WrUserRequest,
                                          KernelMode,
                                          TRUE,
                                          NULL,
                                          NULL);

        /* Process timers inside a user lock */
        UserEnterExclusive();
        ProcessTimers();
        UserLeave();
    }
    DPRINT1("Timer thread exit! Status 0x%08X\n", Status);
}

VOID
NTAPI
InitTimeThread()
{
    NTSTATUS Status;

    /* Event used to signal when a new timer is added */
    KeInitializeEvent(&TimerThreadsStart, SynchronizationEvent, FALSE);

    MasterTimer = ExAllocatePool(NonPagedPool, sizeof(KTIMER));
    if (!MasterTimer)
    {
        ASSERT(FALSE);
        return;
    }

    KeInitializeTimer(MasterTimer);

    Status = PsCreateSystemThread(&TimerThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &TimerThreadId,
                                  TimerThreadMain,
                                  NULL);
    if (!NT_SUCCESS(Status)) DPRINT1("Win32K: Failed to create timer thread.\n");
}

/* EOF */
