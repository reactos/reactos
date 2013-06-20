


#pragma once

#if 0

VOID
FORCEINLINE
KdRosDumpAllThreads(VOID)
{
    KdSystemDebugControl(' soR', (PVOID)DumpAllThreads, 0, 0, 0, 0, 0);
}

VOID
FORCEINLINE
KdRosDumpUserThreads(VOID)
{
    KdSystemDebugControl(' soR', (PVOID)DumpUserThreads, 0, 0, 0, 0, 0);
}

VOID
FORCEINLINE
KdRosDumpArmPfnDatabase(VOID)
{
    KdSystemDebugControl(' soR', (PVOID)KdSpare3, 0, 0, 0, 0, 0);
}
#endif

VOID
FORCEINLINE
KdRosSetDebugCallback(
    ULONG Id,
    PVOID Callback)
{
    KdSystemDebugControl('CsoR', Callback, Id, 0, 0, 0, 0);
}

VOID
FORCEINLINE
KdRosDumpStackFrames(
    ULONG Count,
    PULONG_PTR Backtrace)
{
    KdSystemDebugControl('DsoR', Backtrace, Count, 0, 0, 0, 0);
}

#if KDBG
VOID
FORCEINLINE
KdRosRegisterCliCallback(
    PVOID Callback)
{
    KdSystemDebugControl('RbdK', Callback, FALSE, 0, 0, 0, 0);
}

VOID
FORCEINLINE
KdRosDeregisterCliCallback(
    PVOID Callback)
{
    KdSystemDebugControl('RbdK', Callback, TRUE, 0, 0, 0, 0);
}
#endif

