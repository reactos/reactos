#include <windows.h>
#include <ddk/ntddk.h>

static DWORD LastError;

DWORD RtlNtStatusToDosError(NTSTATUS Status)
{
  if (NT_SUCCESS(Status))   return(0);
  else
  {
   // FIXME must return different values
    return 1;
  }
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
