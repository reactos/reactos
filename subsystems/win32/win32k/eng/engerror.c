/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engerror.c
 * PURPOSE:         Error Handling Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
EngDebugPrint(IN PCHAR StandardPrefix,
              IN PCHAR DebugMessage,
              IN va_list ap)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
EngSetLastError(IN ULONG iError)
{
    PTEB Teb;

    /* Get the TEB */
    Teb = PsGetThreadTeb(PsGetCurrentThread());

    /* Check if we have one */
    if (Teb)
    {
        /* Set the error */
        Teb->LastErrorValue = iError;
    }
}

ULONG
APIENTRY
EngGetLastError(VOID)
{
    PTEB Teb;
    ULONG iError;

    /* Assume success */
    iError = NO_ERROR;

    /* Get the TEB */
    Teb = PsGetThreadTeb(PsGetCurrentThread());

    /* Check if we have one */
    if (Teb)
    {
        /* Get the error */
        iError = Teb->LastErrorValue;
    }

    /* Return the error */
	return iError;
}
