/* $Id: error.c,v 1.18 2003/04/26 12:50:56 gvg Exp $
 *
 * reactos/lib/kernel32/misc/error.c
 *
 */

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


VOID
STDCALL
SetLastError (
	DWORD	dwErrorCode
	)
{
	NtCurrentTeb ()->LastErrorValue = (ULONG) dwErrorCode;
}

DWORD
STDCALL
GetLastError (VOID)
{
	return (DWORD) (NtCurrentTeb ()->LastErrorValue);
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
