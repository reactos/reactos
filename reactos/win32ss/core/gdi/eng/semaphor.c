#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
HSEMAPHORE
APIENTRY
EngCreateSemaphore(VOID)
{
    // www.osr.com/ddk/graphics/gdifncs_95lz.htm
    PERESOURCE psem = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), GDITAG_SEMAPHORE);
    if (!psem)
        return NULL;

    if (!NT_SUCCESS(ExInitializeResourceLite(psem)))
    {
        ExFreePoolWithTag ( psem, GDITAG_SEMAPHORE );
        return NULL;
    }

    return (HSEMAPHORE)psem;
}

VOID
FASTCALL
IntGdiAcquireSemaphore(HSEMAPHORE hsem)
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite ((PERESOURCE)hsem, TRUE);
}

/*
 * @implemented
 */
VOID
APIENTRY
EngAcquireSemaphore(IN HSEMAPHORE hsem)
{
    // www.osr.com/ddk/graphics/gdifncs_14br.htm
    PTHREADINFO W32Thread;
    ASSERT(hsem);
    IntGdiAcquireSemaphore(hsem);
    W32Thread = PsGetThreadWin32Thread(PsGetCurrentThread());
    if (W32Thread) W32Thread->dwEngAcquireCount++;
}


VOID
FASTCALL
IntGdiReleaseSemaphore ( HSEMAPHORE hsem )
{
    ExReleaseResourceLite((PERESOURCE)hsem);
    KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
VOID
APIENTRY
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
    // www.osr.com/ddk/graphics/gdifncs_5u3r.htm
    PTHREADINFO W32Thread;
    ASSERT(hsem);
    W32Thread = PsGetThreadWin32Thread(PsGetCurrentThread());
    if (W32Thread) --W32Thread->dwEngAcquireCount;
    IntGdiReleaseSemaphore(hsem);
}

VOID
NTAPI
EngAcquireSemaphoreShared(
    IN HSEMAPHORE hsem)
{
    PTHREADINFO pti;

    ASSERT(hsem);
    ExEnterCriticalRegionAndAcquireResourceShared((PERESOURCE)hsem);
    pti = PsGetThreadWin32Thread(PsGetCurrentThread());
    if (pti) ++pti->dwEngAcquireCount;
}

/*
 * @implemented
 */
VOID
APIENTRY
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
    // www.osr.com/ddk/graphics/gdifncs_13c7.htm
    ASSERT(hsem);

    ExDeleteResourceLite((PERESOURCE)hsem);
    ExFreePoolWithTag((PVOID)hsem, GDITAG_SEMAPHORE);
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngIsSemaphoreOwned ( IN HSEMAPHORE hsem )
{
    // www.osr.com/ddk/graphics/gdifncs_6wmf.htm
    ASSERT(hsem);
    return (((PERESOURCE)hsem)->ActiveCount > 0);
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngIsSemaphoreOwnedByCurrentThread ( IN HSEMAPHORE hsem )
{
    // www.osr.com/ddk/graphics/gdifncs_9yxz.htm
    ASSERT(hsem);
    return ExIsResourceAcquiredExclusiveLite((PERESOURCE)hsem);
}

/*
 * @implemented
 */
BOOL APIENTRY
EngInitializeSafeSemaphore(
    OUT ENGSAFESEMAPHORE *Semaphore)
{
    HSEMAPHORE hSem;

    if (InterlockedIncrement(&Semaphore->lCount) == 1)
    {
        /* Create the semaphore */
        hSem = EngCreateSemaphore();
        if (hSem == 0)
        {
            InterlockedDecrement(&Semaphore->lCount);
            return FALSE;
        }
        /* FIXME: Not thread-safe! Check result of InterlockedCompareExchangePointer
                  and delete semaphore if already initialized! */
        (void)InterlockedExchangePointer((volatile PVOID *)&Semaphore->hsem, hSem);
    }
    else
    {
        /* Wait for the other thread to create the semaphore */
        ASSERT(Semaphore->lCount > 1);
        ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);
        while (Semaphore->hsem == NULL);
    }

    return TRUE;
}

/*
 * @implemented
 */
VOID APIENTRY
EngDeleteSafeSemaphore(
    IN OUT ENGSAFESEMAPHORE *Semaphore)
{
    if (InterlockedDecrement(&Semaphore->lCount) == 0)
    {
        /* FIXME: Not thread-safe! Use result of InterlockedCompareExchangePointer! */
        EngDeleteSemaphore(Semaphore->hsem);
        (void)InterlockedExchangePointer((volatile PVOID *)&Semaphore->hsem, NULL);
    }
}

/* EOF */
