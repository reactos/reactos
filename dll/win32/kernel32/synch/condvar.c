/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Condition Variable functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FIXME: Move these RTL declarations to the NDK */
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                            IN OUT PRTL_CRITICAL_SECTION CriticalSection,
                            IN PLARGE_INTEGER TimeOut  OPTIONAL);

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                             IN OUT PRTL_SRWLOCK SRWLock,
                             IN PLARGE_INTEGER TimeOut  OPTIONAL,
                             IN ULONG Flags);

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
SleepConditionVariableCS(IN OUT PCONDITION_VARIABLE ConditionVariable,
                         IN OUT PCRITICAL_SECTION CriticalSection,
                         IN DWORD dwMilliseconds)
{
    NTSTATUS Status = 0;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER TimeOutPtr = NULL;

    if (dwMilliseconds != INFINITE)
    {
        TimeOut.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimeOutPtr = &TimeOut;
    }

#if 0
    Status = RtlSleepConditionVariableCS((PRTL_CONDITION_VARIABLE)ConditionVariable,
                                         (PRTL_CRITICAL_SECTION)CriticalSection,
                                         TimeOutPtr);
#endif
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SleepConditionVariableSRW(IN OUT PCONDITION_VARIABLE ConditionVariable,
                          IN OUT PSRWLOCK SRWLock,
                          IN DWORD dwMilliseconds,
                          IN ULONG Flags)
{
    NTSTATUS Status = 0;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER TimeOutPtr = NULL;

    if (dwMilliseconds != INFINITE)
    {
        TimeOut.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimeOutPtr = &TimeOut;
    }

#if 0
    Status = RtlSleepConditionVariableSRW((PRTL_CONDITION_VARIABLE)ConditionVariable,
                                          (PRTL_SRWLOCK)SRWLock,
                                          TimeOutPtr,
                                          Flags);
#endif
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}
