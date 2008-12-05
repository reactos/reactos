/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/amd64/kdb.c
 * PURPOSE:         Kernel Debugger
 * PROGRAMMERS:     Gregor Anich
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG KdbDebugState = 0; /* KDBG Settings (NOECHO, KDSERIAL) */

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
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

KD_CONTINUE_TYPE
KdbEnterDebuggerException(
   IN PEXCEPTION_RECORD ExceptionRecord  OPTIONAL,
   IN KPROCESSOR_MODE PreviousMode,
   IN PCONTEXT Context,
   IN OUT PKTRAP_FRAME TrapFrame,
   IN BOOLEAN FirstChance)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
KdbpCliModuleLoaded(IN PUNICODE_STRING Name)
{
    UNIMPLEMENTED;
}

VOID
KdbpCliInit()
{
    UNIMPLEMENTED;
}

