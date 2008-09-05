/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/kdb.c
 * PURPOSE:         Kernel Debugger
 *
 * PROGRAMMERS:     Gregor Anich
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG KdbDebugState = 0; /* KDBG Settings (NOECHO, KDSERIAL) */

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
KdbpGetCommandLineSettings(PCHAR p1)
{
    PCHAR p2;

    while (p1 && (p2 = strchr(p1, ' ')))
    {
        p2++;

        if (!_strnicmp(p2, "KDSERIAL", 8))
        {
            p2 += 8;
            KdbDebugState |= KD_DEBUG_KDSERIAL;
            KdpDebugMode.Serial = TRUE;
        }
        else if (!_strnicmp(p2, "KDNOECHO", 8))
        {
            p2 += 8;
            KdbDebugState |= KD_DEBUG_KDNOECHO;
        }

        p1 = p2;
    }
}

