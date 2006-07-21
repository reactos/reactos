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

VOID
FORCEINLINE
PspRunCreateThreadNotifyRoutines(IN PETHREAD CurrentThread,
                                 IN BOOLEAN Create)
{
    ULONG i;
    CLIENT_ID Cid = CurrentThread->Cid;

    /* Loop the notify routines */
    for (i = 0; i < PspThreadNotifyRoutineCount; i++)
    {
        /* Call it */
        PspThreadNotifyRoutine[i](Cid.UniqueProcess, Cid.UniqueThread, Create);
    }
}

VOID
FORCEINLINE
PspRunCreateProcessNotifyRoutines(IN PEPROCESS CurrentProcess,
                                  IN BOOLEAN Create)
{
    ULONG i;
    HANDLE ProcessId = (HANDLE)CurrentProcess->UniqueProcessId;
    HANDLE ParentId = CurrentProcess->InheritedFromUniqueProcessId;

    /* Loop the notify routines */
    for(i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; ++i)
    {
        /* Make sure it exists */
        if(PspProcessNotifyRoutine[i])
        {
            /* Call it */
            PspProcessNotifyRoutine[i](ParentId, ProcessId, Create);
        }
    }
}

VOID
FORCEINLINE
PspRunLoadImageNotifyRoutines(PUNICODE_STRING FullImageName,
                              HANDLE ProcessId,
                              PIMAGE_INFO ImageInfo)
{
    ULONG i;

    /* Loop the notify routines */
    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; ++ i)
    {
        /* Make sure it exists */
        if (PspLoadImageNotifyRoutine[i])
        {
            /* Call it */
            PspLoadImageNotifyRoutine[i](FullImageName, ProcessId, ImageInfo);
        }
    }
}

VOID
FORCEINLINE
PspRunLegoRoutine(IN PKTHREAD Thread)
{
    /* Call it */
    if (PspLegoNotifyRoutine) PspLegoNotifyRoutine(Thread);
}

