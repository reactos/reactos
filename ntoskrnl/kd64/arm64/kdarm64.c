/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Kernel Debugger Support
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/**
 * @brief Get ARM64 processor state for debugger
 */
NTSTATUS
NTAPI
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 State,
    IN PCONTEXT Context
)
{
    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(Context);
    
    /* ARM64 debugger support not yet implemented */
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Set ARM64 processor state from debugger
 */
NTSTATUS
NTAPI
KdpSetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 State,
    IN PCONTEXT Context
)
{
    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(Context);
    
    /* ARM64 debugger support not yet implemented */
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Read ARM64 control registers
 */
NTSTATUS
NTAPI
KdpReadControlSpace(
    IN USHORT Processor,
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG ActualLength
)
{
    UNREFERENCED_PARAMETER(Processor);
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);
    
    *ActualLength = 0;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Write ARM64 control registers
 */
NTSTATUS
NTAPI
KdpWriteControlSpace(
    IN USHORT Processor,
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG ActualLength
)
{
    UNREFERENCED_PARAMETER(Processor);
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);
    
    *ActualLength = 0;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Get ARM64 processor context for debugging
 */
VOID
NTAPI
KdpGetContext(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT Context
)
{
    /* Use the process context functions */
    PspGetContext(TrapFrame, ExceptionFrame, Context);
}

/**
 * @brief Set ARM64 processor context from debugger
 */
VOID
NTAPI
KdpSetContext(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT Context
)
{
    /* Use the process context functions */
    PspSetContext(TrapFrame, ExceptionFrame, Context, KernelMode);
}

/**
 * @brief Handle ARM64 breakpoint for debugger
 */
BOOLEAN
NTAPI
KdpCheckBreakpoint(
    IN PKTRAP_FRAME TrapFrame
)
{
    UNREFERENCED_PARAMETER(TrapFrame);
    
    /* ARM64 breakpoint handling not yet implemented */
    return FALSE;
}

/**
 * @brief Set ARM64 hardware breakpoint
 */
NTSTATUS
NTAPI
KdpSetBreakpoint(
    IN ULONG64 Address,
    IN ULONG Flags,
    OUT PULONG Handle
)
{
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(Flags);
    
    *Handle = 0;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Clear ARM64 hardware breakpoint
 */
NTSTATUS
NTAPI
KdpClearBreakpoint(
    IN ULONG Handle
)
{
    UNREFERENCED_PARAMETER(Handle);
    
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Initialize ARM64 kernel debugger support
 */
VOID
NTAPI
KdpInitializeProcessor(VOID)
{
    DPRINT("ARM64: Kernel debugger processor support initialized\n");
}

/**
 * @brief ARM64 specific debugger entry point
 */
BOOLEAN
NTAPI
KdpTrap(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN ULONG ExceptionCode,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChanceException
)
{
    UNREFERENCED_PARAMETER(TrapFrame);
    UNREFERENCED_PARAMETER(ExceptionFrame);
    UNREFERENCED_PARAMETER(ExceptionCode);
    UNREFERENCED_PARAMETER(PreviousMode);
    UNREFERENCED_PARAMETER(SecondChanceException);
    
    DPRINT("ARM64: Kernel debugger trap - not yet implemented\n");
    
    /* For now, don't handle the trap */
    return FALSE;
}