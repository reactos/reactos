#include <windows.h>
#include <ddk/ntddk.h>

static DWORD LastError;

DWORD RtlNtStatusToDosError(NTSTATUS Status)
{
   return(0);
}


VOID SetLastError(DWORD dwErrorCode)
{
   LastError = dwErrorCode;
}

DWORD GetLastError(VOID)
{
   return(LastError);
}

BOOL __ErrorReturnFalse(ULONG ErrorCode)
{
   return(FALSE);
}

PVOID __ErrorReturnNull(ULONG ErrorCode)
{
   return(NULL);
}
