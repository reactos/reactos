#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/ntddbeep.h>

static DWORD LastError=0;

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

WINBOOL
STDCALL
Beep (DWORD dwFreq, DWORD dwDuration)
{
    HANDLE hBeep;
    BEEP_SET_PARAMETERS BeepSetParameters;
    DWORD dwReturned;

    hBeep = CreateFile("\\Beep",
                       FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (!hBeep)
        return FALSE;

    /* Set beep data */
    BeepSetParameters.Frequency = dwFreq;
    BeepSetParameters.Duration  = dwDuration;

    DeviceIoControl(hBeep,
                    IOCTL_BEEP_SET,
                    &BeepSetParameters,
                    sizeof(BEEP_SET_PARAMETERS),
                    NULL,
                    0,
                    &dwReturned,
                    NULL);

    CloseHandle (hBeep);

    return TRUE;
}

