/* $Id$
 *
 * reactos/lib/kernel32/misc/error.c
 *
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/*
 * @implemented
 */
VOID
STDCALL
SetLastError (
	DWORD	dwErrorCode
	)
{
	NtCurrentTeb ()->LastErrorValue = (ULONG) dwErrorCode;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetLastError (VOID)
{
	return (DWORD) (NtCurrentTeb ()->LastErrorValue);
}


/*
 * @implemented
 */
BOOL
STDCALL
Beep (DWORD dwFreq, DWORD dwDuration)
{
    HANDLE hBeep;
    BEEP_SET_PARAMETERS BeepSetParameters;
    DWORD dwReturned;

    hBeep = CreateFileW(L"\\\\.\\Beep",
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
