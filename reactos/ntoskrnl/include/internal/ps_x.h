/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ps_x.h
* PURPOSE:         Intenral Inlined Functions for the Process Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Thomas Weidenmueller (w3seek@reactos.org)
*/

//
// Extract Quantum Settings from the Priority Separation Mask
//
#define PspPrioritySeparationFromMask(Mask)                 \
    ((Mask) & 3)

#define PspQuantumTypeFromMask(Mask)                        \
    ((Mask) & 12)

#define PspQuantumLengthFromMask(Mask)                      \
    ((Mask) & 48)

//
// Cross Thread Flag routines
//
#define PspSetCrossThreadFlag(Thread, Flag)                 \
    InterlockedOr((PLONG)&Thread->CrossThreadFlags, Flag)
#define PspClearCrossThreadFlag(Thread, Flag)               \
    InterlockedAnd((PLONG)&Thread->CrossThreadFlags, ~Flag)

//
// Process flag routines
//
#define PspSetProcessFlag(Process, Flag) \
    InterlockedOr((PLONG)&Process->Flags, Flag)
#define PspClearProcessFlag(Process, Flag) \
    InterlockedAnd((PLONG)&Process->Flags, ~Flag)

FORCEINLINE
VOID
PspRunCreateThreadNotifyRoutines(IN PETHREAD CurrentThread,
                                 IN BOOLEAN Create)
{
    ULONG i;

    /* Check if we have registered routines */
    if (PspThreadNotifyRoutineCount)
    {
        /* Loop callbacks */
        for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++)
        {
            /* Do the callback */
            ExDoCallBack(&PspThreadNotifyRoutine[i],
                         CurrentThread->Cid.UniqueProcess,
                         CurrentThread->Cid.UniqueThread,
                         (PVOID)(ULONG_PTR)Create);
        }
    }
}

FORCEINLINE
VOID
PspRunCreateProcessNotifyRoutines(IN PEPROCESS CurrentProcess,
                                  IN BOOLEAN Create)
{
    ULONG i;

    /* Check if we have registered routines */
    if (PspProcessNotifyRoutineCount)
    {
        /* Loop callbacks */
        for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++)
        {
            /* Do the callback */
            ExDoCallBack(&PspProcessNotifyRoutine[i],
                         CurrentProcess->InheritedFromUniqueProcessId,
                         (PVOID)(ULONG_PTR)Create,
                         NULL);
        }
    }
}

FORCEINLINE
VOID
PspRunLoadImageNotifyRoutines(PUNICODE_STRING FullImageName,
                              HANDLE ProcessId,
                              PIMAGE_INFO ImageInfo)
{
    ULONG i;

    /* Loop the notify routines */
    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; ++ i)
    {
        /* Do the callback */
        ExDoCallBack(&PspLoadImageNotifyRoutine[i],
                     FullImageName,
                     ProcessId,
                     ImageInfo);
    }
}

FORCEINLINE
VOID
PspRunLegoRoutine(IN PKTHREAD Thread)
{
    /* Call it */
    if (PspLegoNotifyRoutine) PspLegoNotifyRoutine(Thread);
}

FORCEINLINE
VOID
PspLockProcessSecurityShared(IN PEPROCESS Process)
{
    /* Enter a Critical Region */
    KeEnterCriticalRegion();

    /* Lock the Process */
    ExAcquirePushLockShared(&Process->ProcessLock);
}

FORCEINLINE
VOID
PspUnlockProcessSecurityShared(IN PEPROCESS Process)
{
    /* Unlock the Process */
    ExReleasePushLockShared(&Process->ProcessLock);

    /* Leave Critical Region */
    KeLeaveCriticalRegion();
}

FORCEINLINE
VOID
PspLockProcessSecurityExclusive(IN PEPROCESS Process)
{
    /* Enter a Critical Region */
    KeEnterCriticalRegion();

    /* Lock the Process */
    ExAcquirePushLockExclusive(&Process->ProcessLock);
}

FORCEINLINE
VOID
PspUnlockProcessSecurityExclusive(IN PEPROCESS Process)
{
    /* Unlock the Process */
    ExReleasePushLockExclusive(&Process->ProcessLock);

    /* Leave Critical Region */
    KeLeaveCriticalRegion();
}

FORCEINLINE
VOID
PspLockThreadSecurityShared(IN PETHREAD Thread)
{
    /* Enter a Critical Region */
    KeEnterCriticalRegion();

    /* Lock the Thread */
    ExAcquirePushLockShared(&Thread->ThreadLock);
}

FORCEINLINE
VOID
PspUnlockThreadSecurityShared(IN PETHREAD Thread)
{
    /* Unlock the Thread */
    ExReleasePushLockShared(&Thread->ThreadLock);

    /* Leave Critical Region */
    KeLeaveCriticalRegion();
}

FORCEINLINE
VOID
PspLockThreadSecurityExclusive(IN PETHREAD Thread)
{
    /* Enter a Critical Region */
    KeEnterCriticalRegion();

    /* Lock the Thread */
    ExAcquirePushLockExclusive(&Thread->ThreadLock);
}

FORCEINLINE
VOID
PspUnlockThreadSecurityExclusive(IN PETHREAD Thread)
{
    /* Unlock the Process */
    ExReleasePushLockExclusive(&Thread->ThreadLock);

    /* Leave Critical Thread */
    KeLeaveCriticalRegion();
}

FORCEINLINE
PEPROCESS
_PsGetCurrentProcess(VOID)
{
    /* Get the current process */
    return (PEPROCESS)KeGetCurrentThread()->ApcState.Process;
}
