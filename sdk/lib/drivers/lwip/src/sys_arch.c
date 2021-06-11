#include "lwip/sys.h"

#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"

#include "rosip.h"

#include <debug.h>

static LIST_ENTRY ThreadListHead;
static KSPIN_LOCK ThreadListLock;

KEVENT TerminationEvent;
NPAGED_LOOKASIDE_LIST MessageLookasideList;
NPAGED_LOOKASIDE_LIST QueueEntryLookasideList;

static LARGE_INTEGER StartTime;

typedef struct _thread_t
{
    HANDLE Handle;
    void (* ThreadFunction)(void *arg);
    void *ThreadContext;
    LIST_ENTRY ListEntry;
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
    /* Preempt the dispatcher */
    KeRaiseIrql(DISPATCH_LEVEL, lev);
}

void
sys_arch_unprotect(sys_prot_t lev)
{
    KeLowerIrql(lev);
}

err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
    ASSERT(count == 0 || count == 1);

    /* It seems lwIP uses the semaphore implementation as either a completion event or a lock
     * so I optimize for this case by using a synchronization event and setting its initial state
     * to signalled for a lock and non-signalled for a completion event */

    KeInitializeEvent(&sem->Event, SynchronizationEvent, count);

    sem->Valid = 1;

    return ERR_OK;
}

int sys_sem_valid(sys_sem_t *sem)
{
    return sem->Valid;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
    sem->Valid = 0;
}

void
sys_sem_free(sys_sem_t* sem)
{
    /* No op (allocated in stack) */

    sys_sem_set_invalid(sem);
}

void
sys_sem_signal(sys_sem_t* sem)
{
    KeSetEvent(&sem->Event, IO_NO_INCREMENT, FALSE);
}

u32_t
sys_arch_sem_wait(sys_sem_t* sem, u32_t timeout)
{
    LARGE_INTEGER LargeTimeout, PreWaitTime, PostWaitTime;
    UINT64 TimeDiff;
    NTSTATUS Status;
    PVOID WaitObjects[] = {&sem->Event, &TerminationEvent};

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

    return SYS_ARCH_TIMEOUT;
}

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
    KeInitializeSpinLock(&mbox->Lock);

    InitializeListHead(&mbox->ListHead);

    KeInitializeEvent(&mbox->Event, NotificationEvent, FALSE);

    mbox->Valid = 1;

    return ERR_OK;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
    return mbox->Valid;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    mbox->Valid = 0;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
    ASSERT(IsListEmpty(&mbox->ListHead));

    sys_mbox_set_invalid(mbox);
}

void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
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
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
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

        Container = CONTAINING_RECORD(Entry, LWIP_MESSAGE_CONTAINER, ListEntry);
        Message = Container->Message;
        ExFreePool(Container);

        if (msg)
            *msg = Message;

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

    return SYS_ARCH_TIMEOUT;
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    if (sys_arch_mbox_fetch(mbox, msg, 1) != SYS_ARCH_TIMEOUT)
        return 0;
    else
        return SYS_MBOX_EMPTY;
}

err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    sys_mbox_post(mbox, msg);

    return ERR_OK;
}

VOID
NTAPI
LwipThreadMain(PVOID Context)
{
    thread_t Container = (thread_t)Context;
    KIRQL OldIrql;

    ExInterlockedInsertHeadList(&ThreadListHead, &Container->ListEntry, &ThreadListLock);

    Container->ThreadFunction(Container->ThreadContext);

    KeAcquireSpinLock(&ThreadListLock, &OldIrql);
    RemoveEntryList(&Container->ListEntry);
    KeReleaseSpinLock(&ThreadListLock, OldIrql);

    ExFreePool(Container);

    PsTerminateSystemThread(STATUS_SUCCESS);
}

sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    thread_t Container;
    NTSTATUS Status;

    Container = ExAllocatePool(NonPagedPool, sizeof(*Container));
    if (!Container)
        return 0;

    Container->ThreadFunction = thread;
    Container->ThreadContext = arg;

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

    ExInitializeNPagedLookasideList(&MessageLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(struct lwip_callback_msg),
                                    LWIP_MESSAGE_TAG,
                                    0);

    ExInitializeNPagedLookasideList(&QueueEntryLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(QUEUE_ENTRY),
                                    LWIP_QUEUE_TAG,
                                    0);
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

    ExDeleteNPagedLookasideList(&MessageLookasideList);
    ExDeleteNPagedLookasideList(&QueueEntryLookasideList);
}
