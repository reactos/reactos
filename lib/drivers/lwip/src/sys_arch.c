#include "lwip/sys.h"

#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"

#include <debug.h>

static LIST_ENTRY ThreadListHead;
static KSPIN_LOCK ThreadListLock;

KEVENT TerminationEvent;

static LARGE_INTEGER StartTime;

typedef struct _thread_t
{
    PVOID ThreadId;
    HANDLE Handle;
    struct sys_timeouts Timeouts;
    void (* ThreadFunction)(void *arg);
    void *ThreadContext;
    LIST_ENTRY ListEntry;
    char Name[1];
} *thread_t;

u32_t sys_now(void)
{
    LARGE_INTEGER CurrentTime;
    
    KeQuerySystemTime(&CurrentTime);
    
    return (CurrentTime.QuadPart - StartTime.QuadPart) / 10000;
}

void
sys_arch_protect(sys_prot_t *lev)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&lev->Lock, &OldIrql);

    lev->OldIrql = OldIrql;
}

void
sys_arch_unprotect(sys_prot_t *lev)
{
    KeReleaseSpinLock(&lev->Lock, lev->OldIrql);
}

void
sys_arch_decl_protect(sys_prot_t *lev)
{
    KeInitializeSpinLock(&lev->Lock);
}

sys_sem_t
sys_sem_new(u8_t count)
{
    sys_sem_t sem;
    
    ASSERT(count == 0 || count == 1);
    
    sem = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
    if (!sem)
        return SYS_SEM_NULL;
    
    /* It seems lwIP uses the semaphore implementation as either a completion event or a lock
     * so I optimize for this case by using a synchronization event and setting its initial state
     * to signalled for a lock and non-signalled for a completion event */

    KeInitializeEvent(sem, SynchronizationEvent, count);
    
    return sem;
}

void
sys_sem_free(sys_sem_t sem)
{
    ExFreePool(sem);
}

void
sys_sem_signal(sys_sem_t sem)
{
    KeSetEvent(sem, IO_NO_INCREMENT, FALSE);
}

u32_t
sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
{
    LARGE_INTEGER LargeTimeout, PreWaitTime, PostWaitTime;
    UINT64 TimeDiff;
    NTSTATUS Status;
    PVOID WaitObjects[] = {sem, &TerminationEvent};

    LargeTimeout.QuadPart = Int32x32To64(timeout, -10000);
    
    KeQuerySystemTime(&PreWaitTime);

    Status = KeWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAny,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      timeout != 0 ? &LargeTimeout : NULL,
                                      NULL);
    if (Status == STATUS_WAIT_0)
    {
        KeQuerySystemTime(&PostWaitTime);
        TimeDiff = PostWaitTime.QuadPart - PreWaitTime.QuadPart;
        TimeDiff /= 10000;
        return TimeDiff;
    }
    else if (Status == STATUS_WAIT_1)
    {
        /* DON'T remove ourselves from the thread list! */
        PsTerminateSystemThread(STATUS_SUCCESS);
        
        /* We should never get here! */
        ASSERT(FALSE);
        
        return 0;
    }
    else
        return SYS_ARCH_TIMEOUT;
}

sys_mbox_t
sys_mbox_new(int size)
{
    sys_mbox_t mbox = ExAllocatePool(NonPagedPool, sizeof(struct _sys_mbox_t));
    
    if (!mbox)
        return SYS_MBOX_NULL;
    
    KeInitializeSpinLock(&mbox->Lock);
    
    InitializeListHead(&mbox->ListHead);
    
    KeInitializeEvent(&mbox->Event, NotificationEvent, FALSE);
    
    return mbox;
}

void
sys_mbox_free(sys_mbox_t mbox)
{
    ASSERT(IsListEmpty(&mbox->ListHead));
    
    ExFreePool(mbox);
}

void
sys_mbox_post(sys_mbox_t mbox, void *msg)
{
    PLWIP_MESSAGE_CONTAINER Container;
    
    Container = ExAllocatePool(NonPagedPool, sizeof(*Container));
    ASSERT(Container);
    
    Container->Message = msg;
    
    ExInterlockedInsertTailList(&mbox->ListHead,
                                &Container->ListEntry,
                                &mbox->Lock);
    
    KeSetEvent(&mbox->Event, IO_NO_INCREMENT, FALSE);
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t timeout)
{
    LARGE_INTEGER LargeTimeout, PreWaitTime, PostWaitTime;
    UINT64 TimeDiff;
    NTSTATUS Status;
    PVOID Message;
    PLWIP_MESSAGE_CONTAINER Container;
    PLIST_ENTRY Entry;
    KIRQL OldIrql;
    PVOID WaitObjects[] = {&mbox->Event, &TerminationEvent};
    
    LargeTimeout.QuadPart = Int32x32To64(timeout, -10000);
    
    KeQuerySystemTime(&PreWaitTime);

    Status = KeWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAny,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      timeout != 0 ? &LargeTimeout : NULL,
                                      NULL);
    if (Status == STATUS_WAIT_0)
    {
        KeAcquireSpinLock(&mbox->Lock, &OldIrql);
        Entry = RemoveHeadList(&mbox->ListHead);
        ASSERT(Entry);
        if (IsListEmpty(&mbox->ListHead))
            KeClearEvent(&mbox->Event);
        KeReleaseSpinLock(&mbox->Lock, OldIrql);

        KeQuerySystemTime(&PostWaitTime);
        TimeDiff = PostWaitTime.QuadPart - PreWaitTime.QuadPart;
        TimeDiff /= 10000;
        
        Container = CONTAINING_RECORD(Entry, LWIP_MESSAGE_CONTAINER, ListEntry);
        Message = Container->Message;
        ExFreePool(Container);
        
        if (msg)
            *msg = Message;
        
        return TimeDiff;
    }
    else if (Status == STATUS_WAIT_1)
    {
        /* DON'T remove ourselves from the thread list! */
        PsTerminateSystemThread(STATUS_SUCCESS);
        
        /* We should never get here! */
        ASSERT(FALSE);
        
        return 0;
    }
    else
        return SYS_ARCH_TIMEOUT;
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t mbox, void **msg)
{
    if (sys_arch_mbox_fetch(mbox, msg, 1) != SYS_ARCH_TIMEOUT)
        return 0;
    else
        return SYS_MBOX_EMPTY;
}

err_t
sys_mbox_trypost(sys_mbox_t mbox, void *msg)
{
    sys_mbox_post(mbox, msg);

    return ERR_OK;
}

struct sys_timeouts *sys_arch_timeouts(void)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    thread_t Container;
    
    KeAcquireSpinLock(&ThreadListLock, &OldIrql);
    CurrentEntry = ThreadListHead.Flink;
    while (CurrentEntry != &ThreadListHead)
    {
        Container = CONTAINING_RECORD(CurrentEntry, struct _thread_t, ListEntry);
        
        if (Container->ThreadId == KeGetCurrentThread())
        {
            KeReleaseSpinLock(&ThreadListLock, OldIrql);
            return &Container->Timeouts;
        }
        
        CurrentEntry = CurrentEntry->Flink;
    }
    KeReleaseSpinLock(&ThreadListLock, OldIrql);
    
    Container = ExAllocatePool(NonPagedPool, sizeof(*Container));
    if (!Container)
        return SYS_ARCH_NULL;
    
    Container->Name[0] = ANSI_NULL;
    Container->ThreadFunction = NULL;
    Container->ThreadContext = NULL;
    Container->Timeouts.next = NULL;
    Container->ThreadId = KeGetCurrentThread();
    
    ExInterlockedInsertHeadList(&ThreadListHead, &Container->ListEntry, &ThreadListLock);
    
    return &Container->Timeouts;
}

VOID
NTAPI
LwipThreadMain(PVOID Context)
{
    thread_t Container = Context;
    KIRQL OldIrql;
    
    Container->ThreadId = KeGetCurrentThread();
    
    ExInterlockedInsertHeadList(&ThreadListHead, &Container->ListEntry, &ThreadListLock);
    
    Container->ThreadFunction(Container->ThreadContext);
    
    KeAcquireSpinLock(&ThreadListLock, &OldIrql);
    RemoveEntryList(&Container->ListEntry);
    KeReleaseSpinLock(&ThreadListLock, OldIrql);
    
    ExFreePool(Container);
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}

sys_thread_t
sys_thread_new(char *name, void (* thread)(void *arg), void *arg, int stacksize, int prio)
{
    thread_t Container;
    NTSTATUS Status;

    Container = ExAllocatePool(NonPagedPool, strlen(name) + sizeof(*Container));
    if (!Container)
        return 0;

    strcpy(Container->Name, name);
    Container->ThreadFunction = thread;
    Container->ThreadContext = arg;
    Container->Timeouts.next = NULL;

    Status = PsCreateSystemThread(&Container->Handle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  NULL,
                                  LwipThreadMain,
                                  Container);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Container);
        return 0;
    }

    return 0;
}

void
sys_init(void)
{
    KeInitializeSpinLock(&ThreadListLock);
    
    InitializeListHead(&ThreadListHead);
    
    KeQuerySystemTime(&StartTime);
    
    KeInitializeEvent(&TerminationEvent, NotificationEvent, FALSE);
}

void
sys_shutdown(void)
{
    PLIST_ENTRY CurrentEntry;
    thread_t Container;
    
    /* Set the termination event */
    KeSetEvent(&TerminationEvent, IO_NO_INCREMENT, FALSE);
    
    /* Loop through the thread list and wait for each to die */
    while ((CurrentEntry = ExInterlockedRemoveHeadList(&ThreadListHead, &ThreadListLock)))
    {
        Container = CONTAINING_RECORD(CurrentEntry, struct _thread_t, ListEntry);
        
        if (Container->ThreadFunction)
        {
            KeWaitForSingleObject(Container->Handle,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            
            ZwClose(Container->Handle);
        }
    }
}
