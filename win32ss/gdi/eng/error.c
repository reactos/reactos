#include <win32k.h>

/*
 * @implemented
 * https://learn.microsoft.com/en-us/windows/win32/api/winddi/nf-winddi-enggetlasterror
 */
ULONG
APIENTRY
EngGetLastError(VOID)
{
    PTEB pTeb = NtCurrentTeb();
    return (pTeb ? pTeb->LastErrorValue : ERROR_SUCCESS);
}

/*
 * @implemented
 * https://learn.microsoft.com/en-us/windows/win32/api/winddi/nf-winddi-engsetlasterror
 * Win: UserSetLastError
 */
VOID
APIENTRY
EngSetLastError(_In_ ULONG iError)
{
    PTEB pTeb = NtCurrentTeb();
    if (pTeb)
        pTeb->LastErrorValue = iError;
}

VOID
FASTCALL
SetLastNtError(_In_ NTSTATUS Status)
{
    EngSetLastError(RtlNtStatusToDosError(Status));
}
