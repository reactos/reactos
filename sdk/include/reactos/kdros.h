


#pragma once

#if 0

FORCEINLINE
VOID
KdRosDumpAllThreads(VOID)
{
    KdSystemDebugControl(' soR', (PVOID)DumpAllThreads, 0, 0, 0, 0, 0);
}

FORCEINLINE
VOID
KdRosDumpUserThreads(VOID)
{
    KdSystemDebugControl(' soR', (PVOID)DumpUserThreads, 0, 0, 0, 0, 0);
}

FORCEINLINE
VOID
KdRosDumpArmPfnDatabase(VOID)
{
    KdSystemDebugControl(' soR', (PVOID)KdSpare3, 0, 0, 0, 0, 0);
}
#endif

FORCEINLINE
VOID
KdRosSetDebugCallback(
    ULONG Id,
    PVOID Callback)
{
    KdSystemDebugControl('CsoR', Callback, Id, 0, 0, 0, 0);
}

FORCEINLINE
VOID
KdRosDumpStackFrames(
    ULONG Count,
    PULONG_PTR Backtrace)
{
    KdSystemDebugControl('DsoR', Backtrace, Count, 0, 0, 0, 0);
}

#if defined(KDBG)
typedef
BOOLEAN
(NTAPI KDBG_CLI_ROUTINE)(
    IN PCHAR Command,
    IN ULONG Argc,
    IN PCH Argv[]);
typedef KDBG_CLI_ROUTINE *PKDBG_CLI_ROUTINE;

FORCEINLINE
ULONG
KdRosRegisterCliCallback(
    PKDBG_CLI_ROUTINE Callback)
{
    return KdSystemDebugControl('RbdK', Callback, FALSE, 0, 0, 0, 0);
}

FORCEINLINE
VOID
KdRosDeregisterCliCallback(
    PKDBG_CLI_ROUTINE Callback)
{
    KdSystemDebugControl('RbdK', Callback, TRUE, 0, 0, 0, 0);
}
#endif

