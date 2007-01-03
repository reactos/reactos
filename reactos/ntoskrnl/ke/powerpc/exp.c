/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/exp.c
 * PURPOSE:         Exception Dispatching and Context<->Trap Frame Conversion
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gregor Anich
 *                  Skywing (skywing@valhallalegends.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

_SEH_DEFINE_LOCALS(KiCopyInfo)
{
    volatile EXCEPTION_RECORD SehExceptRecord;
};

_SEH_FILTER(KiCopyInformation)
{
    _SEH_ACCESS_LOCALS(KiCopyInfo);

    /* Copy the exception records and return to the handler */
    RtlMoveMemory((PVOID)&_SEH_VAR(SehExceptRecord),
                  _SEH_GetExceptionPointers()->ExceptionRecord,
                  sizeof(EXCEPTION_RECORD));
    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
INIT_FUNCTION
NTAPI
KeInitExceptions(VOID)
{
}

ULONG
NTAPI
KiEspFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    return 0;
}

VOID
NTAPI
KiEspToTrapFrame(IN PKTRAP_FRAME TrapFrame,
                 IN ULONG Esp)
{
}

ULONG
NTAPI
KiSsFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    return 0;
}

VOID
NTAPI
KiSsToTrapFrame(IN PKTRAP_FRAME TrapFrame,
                IN ULONG Ss)
{
}

USHORT
NTAPI
KiTagWordFnsaveToFxsave(USHORT TagWord)
{
    return 0;
}

VOID
NTAPI
KeContextToTrapFrame(IN PCONTEXT Context,
                     IN OUT PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PKTRAP_FRAME TrapFrame,
                     IN ULONG ContextFlags,
                     IN KPROCESSOR_MODE PreviousMode)
{
}

VOID
NTAPI
KeTrapFrameToContext(IN PKTRAP_FRAME TrapFrame,
                     IN PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PCONTEXT Context)
{
}

VOID
NTAPI
KiDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN KPROCESSOR_MODE PreviousMode,
                    IN BOOLEAN FirstChance)
{
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeRaiseUserException(IN NTSTATUS ExceptionCode)
{
    return STATUS_SUCCESS;
}

