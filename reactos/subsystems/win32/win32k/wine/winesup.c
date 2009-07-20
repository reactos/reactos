/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/wine/winesup.c
 * PURPOSE:         Wine supporting functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include "object.h"

#define NDEBUG
#include <debug.h>

timeout_t current_time = 0ULL;
int debug_level = 0;

/* PRIVATE FUNCTIONS *********************************************************/

void set_error( unsigned int err )
{
    SetLastNtError(err);
}

unsigned int get_error(void)
{
    return GetLastNtError();
}

const SID *token_get_user( void *token )
{
    UNIMPLEMENTED;
    return NULL;
}

struct timeout_user *add_timeout_user( timeout_t when, timeout_callback func, void *private )
{
    LARGE_INTEGER DueTime;
    struct timeout_user *TimeoutUser;

    DueTime.QuadPart = (LONGLONG)when;

    DPRINT1("add_timeout_user(when %I64d, func %p), current time %I64d\n", when, func, CurrentTime.QuadPart);

    /* Allocate memory for timeout structure */
    TimeoutUser = ExAllocatePool(NonPagedPool, sizeof(struct timeout_user));

    /* Initialize timer and DPC objects */
    KeInitializeTimer(&TimeoutUser->Timer);
    KeInitializeDpc(&TimeoutUser->Dpc, func, private);

    /* Set the timer */
    KeSetTimer(&TimeoutUser->Timer, DueTime, &TimeoutUser->Dpc);

    return TimeoutUser;
}

/* remove a timeout user */
void remove_timeout_user( struct timeout_user *user )
{
    DPRINT1("remove_timeout_user %p\n", user);

    /* Cancel the timer */
    KeCancelTimer(&user->Timer);

    /* Free memory */
    ExFreePool(user);
}

/* default map_access() routine for objects that behave like an fd */
unsigned int default_fd_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= FILE_GENERIC_READ;
    if (access & GENERIC_WRITE)   access |= FILE_GENERIC_WRITE;
    if (access & GENERIC_EXECUTE) access |= FILE_GENERIC_EXECUTE;
    if (access & GENERIC_ALL)     access |= FILE_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

thread_id_t get_thread_id (PTHREADINFO Thread)
{
    return (thread_id_t)Thread->peThread->Cid.UniqueThread;
}

process_id_t get_process_id(PPROCESSINFO Process)
{
    return (process_id_t)Process->peProcess->UniqueProcessId;
}

void wake_up( struct object *obj, int max )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, &obj->wait_queue )
    {
        struct wait_queue_entry *entry = LIST_ENTRY( ptr, struct wait_queue_entry, entry );
        DPRINT1("wake_thread 0x%x / process 0x%x\n",
            entry->thread->peThread->Tcb.Teb->ClientId.UniqueThread,
            entry->thread->peThread->Tcb.Teb->ClientId.UniqueProcess);
        /*if (wake_thread( entry->thread ))
        {
            if (max && !--max) break;
        }*/
    }
}

void set_fd_events( struct fd *fd, int events )
{
    UNIMPLEMENTED;
}

int check_fd_events( struct fd *fd, int events )
{
    UNIMPLEMENTED;
    return 0;
}

/* add a thread to an object wait queue; return 1 if OK, 0 on error */
int add_queue( struct object *obj, struct wait_queue_entry *entry )
{
#if 0
    grab_object( obj );
    entry->obj = obj;
    list_add_tail( &obj->wait_queue, &entry->entry );
#else
    UNIMPLEMENTED;
#endif
    return 1;
}

/* remove a thread from an object wait queue */
void remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
#if 0
    list_remove( &entry->entry );
    release_object( obj );
#else
    UNIMPLEMENTED;
#endif
}

void set_event( struct event *event )
{
    UNIMPLEMENTED;
}

void reset_event( struct event *event )
{
    UNIMPLEMENTED;
}

/* Stupid thing to hack around missing export of memcmp from the kernel */
int memcmp(const void *s1, const void *s2, size_t n)
{
    if (n != 0) {
        const unsigned char *p1 = s1, *p2 = s2;
        do {
            if (*p1++ != *p2++)
	            return (*--p1 - *--p2);
        } while (--n != 0);
    }
    return 0;
}

PVOID
NTAPI
ExReallocPool(PVOID OldPtr, ULONG NewSize, ULONG OldSize)
{
    PVOID NewPtr = ExAllocatePool(PagedPool, NewSize);

    if (OldPtr)
    {
        RtlCopyMemory(NewPtr, OldPtr, (NewSize < OldSize) ? NewSize : OldSize);
        ExFreePool(OldPtr);
    }

    return NewPtr;
}

/* EOF */
