/* $Id: error.c,v 1.12 1999/10/07 23:45:07 ekohl Exp $
 *
 * reactos/lib/kernel32/misc/error.c
 *
 */
#include <ddk/ntddk.h>
#include <ddk/ntddbeep.h>

// #define NDEBUG
#include <kernel32/kernel32.h>


static DWORD LastError=0;


VOID
STDCALL
SetLastError (
	DWORD	dwErrorCode
	)
{
	/* FIXME: it is per thread */
	LastError = dwErrorCode;
}

DWORD
STDCALL
GetLastError (VOID)
{
	/* FIXME: it is per thread */
	return LastError;
}


BOOL
__ErrorReturnFalse (ULONG ErrorCode)
{
   return(FALSE);
}


PVOID
__ErrorReturnNull (ULONG ErrorCode)
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

    hBeep = CreateFile("\\\\.\\Beep",
                       FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (hBeep == INVALID_HANDLE_VALUE)
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


/* EOF */
