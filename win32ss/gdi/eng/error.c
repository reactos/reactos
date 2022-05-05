#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 * http://msdn.microsoft.com/en-us/library/ff564940%28VS.85%29.aspx
 */
ULONG
APIENTRY
EngGetLastError(VOID)
{
    PTEB pTeb = NtCurrentTeb();
    if (pTeb)
        return NtCurrentTeb()->LastErrorValue;
    else
        return ERROR_SUCCESS;
}

/*
 * @implemented
 * http://msdn.microsoft.com/en-us/library/ff565015%28VS.85%29.aspx
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
SetLastNtError(NTSTATUS Status)
{
    EngSetLastError(RtlNtStatusToDosError(Status));
}
