/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 User Mode System Calls
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* ARM64 system call dispatch table */
extern PVOID KiServiceTable[];
extern ULONG KiServiceLimit;

/* FUNCTIONS *****************************************************************/

/**
 * @brief ARM64 system call dispatcher
 */
NTSTATUS
NTAPI  
KiSystemCallDispatch(
    IN ULONG SystemCallNumber,
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    PVOID ServiceRoutine;
    NTSTATUS Status;
    ULONG ArgumentCount;
    ULONG64 Arguments[16];  /* Maximum 16 arguments for ARM64 */
    
    DPRINT("KiSystemCallDispatch: Call %u from PC=0x%llX\n", 
           SystemCallNumber, TrapFrame->Pc);
    
    /* Validate system call number */
    if (SystemCallNumber >= KiServiceLimit)
    {
        DPRINT1("ARM64: Invalid system call number %u (limit %u)\n",
                SystemCallNumber, KiServiceLimit);
        return STATUS_INVALID_SYSTEM_SERVICE;
    }
    
    /* Get service routine */
    ServiceRoutine = KiServiceTable[SystemCallNumber];
    if (!ServiceRoutine)
    {
        DPRINT1("ARM64: System call %u not implemented\n", SystemCallNumber);
        return STATUS_NOT_IMPLEMENTED;
    }
    
    /* Extract arguments from registers and stack */
    /* ARM64 calling convention: x0-x7 for first 8 arguments */
    Arguments[0] = TrapFrame->X0;
    Arguments[1] = TrapFrame->X1;
    Arguments[2] = TrapFrame->X2;
    Arguments[3] = TrapFrame->X3;
    Arguments[4] = TrapFrame->X4;
    Arguments[5] = TrapFrame->X5;
    Arguments[6] = TrapFrame->X6;
    Arguments[7] = TrapFrame->X7;
    
    /* Additional arguments would be on the user stack */
    /* For now, limit to 8 arguments */
    ArgumentCount = 8;
    
    /* TODO: Implement actual system call dispatch */
    /* This is a simplified version - real implementation would need */
    /* proper argument parsing based on system call signature */
    
    __try
    {
        /* Call the system service */
        switch (ArgumentCount)
        {
            case 0:
                Status = ((NTSTATUS(NTAPI *)(VOID))ServiceRoutine)();
                break;
                
            case 1:
                Status = ((NTSTATUS(NTAPI *)(ULONG64))ServiceRoutine)(Arguments[0]);
                break;
                
            case 2:
                Status = ((NTSTATUS(NTAPI *)(ULONG64, ULONG64))ServiceRoutine)
                         (Arguments[0], Arguments[1]);
                break;
                
            case 3:
                Status = ((NTSTATUS(NTAPI *)(ULONG64, ULONG64, ULONG64))ServiceRoutine)
                         (Arguments[0], Arguments[1], Arguments[2]);
                break;
                
            case 4:
                Status = ((NTSTATUS(NTAPI *)(ULONG64, ULONG64, ULONG64, ULONG64))ServiceRoutine)
                         (Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                break;
                
            default:
                /* For more arguments, use a generic dispatcher */
                Status = KiSystemCallGenericDispatch(ServiceRoutine, Arguments, ArgumentCount);
                break;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
        DPRINT1("ARM64: Exception in system call %u: 0x%08X\n", SystemCallNumber, Status);
    }
    
    DPRINT("KiSystemCallDispatch: Call %u returned 0x%08X\n", SystemCallNumber, Status);
    return Status;
}

/**
 * @brief Generic system call dispatcher for many arguments
 */
NTSTATUS
NTAPI
KiSystemCallGenericDispatch(
    IN PVOID ServiceRoutine,
    IN PULONG64 Arguments,
    IN ULONG ArgumentCount
)
{
    /* This would need assembly implementation for variable arguments */
    /* For now, return not implemented */
    UNREFERENCED_PARAMETER(ServiceRoutine);
    UNREFERENCED_PARAMETER(Arguments);
    UNREFERENCED_PARAMETER(ArgumentCount);
    
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Handle ARM64 system call from user mode
 */
VOID
NTAPI
KiSystemCall64(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    ULONG SystemCallNumber;
    NTSTATUS Status;
    PKTHREAD Thread;
    BOOLEAN PreviousMode;
    
    /* Get system call number from x8 register */
    SystemCallNumber = (ULONG)TrapFrame->X8;
    
    /* Get current thread */
    Thread = KeGetCurrentThread();
    
    /* Save previous mode */
    PreviousMode = KeGetPreviousMode();
    
    /* Set kernel mode for system call execution */
    Thread->PreviousMode = UserMode;
    
    DPRINT("KiSystemCall64: Call %u from user mode, PC=0x%llX\n",
           SystemCallNumber, TrapFrame->Pc);
    
    /* Enable interrupts for system call execution */
    _enable();
    
    __try
    {
        /* Probe user mode parameters if needed */
        /* TODO: Add parameter probing */
        
        /* Dispatch the system call */
        Status = KiSystemCallDispatch(SystemCallNumber, TrapFrame);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
        DPRINT1("ARM64: Exception during system call %u: 0x%08X\n", 
                SystemCallNumber, Status);
    }
    
    /* Disable interrupts before returning */
    _disable();
    
    /* Restore previous mode */
    Thread->PreviousMode = PreviousMode;
    
    /* Set return value in x0 register */
    TrapFrame->X0 = (ULONG64)Status;
    
    /* Check for pending APCs */
    if (Thread->ApcState.UserApcPending)
    {
        DPRINT("KiSystemCall64: User APC pending\n");
        /* TODO: Deliver user APC */
    }
    
    DPRINT("KiSystemCall64: Returning 0x%08X\n", Status);
}

/**
 * @brief Handle ARM64 system call from 32-bit user mode (compatibility)
 */
VOID
NTAPI
KiSystemCall32(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("KiSystemCall32: 32-bit system call not implemented\n");
    
    /* For now, return error */
    TrapFrame->X0 = STATUS_NOT_SUPPORTED;
}

/**
 * @brief Initialize ARM64 system call support
 */
VOID
NTAPI
KiInitializeSystemCalls(VOID)
{
    DPRINT("KiInitializeSystemCalls: ARM64 system call support initialized\n");
    
    /* TODO: Initialize system call dispatch table */
    /* TODO: Set up system call entry points */
}

/**
 * @brief Handle user mode callback
 */
NTSTATUS
NTAPI
KiCallUserMode(
    IN OUT PVOID *OutputBuffer,
    IN OUT PULONG OutputLength
)
{
    DPRINT("KiCallUserMode: Not implemented for ARM64\n");
    
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputLength);
    
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Test if we're in user mode context
 */
BOOLEAN
NTAPI
KiIsUserModeThread(VOID)
{
    PKTRAP_FRAME TrapFrame;
    PKTHREAD Thread;
    
    Thread = KeGetCurrentThread();
    if (!Thread)
        return FALSE;
        
    TrapFrame = KiGetTrapFrame(Thread);
    if (!TrapFrame)
        return FALSE;
    
    /* Check if previous mode was user mode */
    return (TrapFrame->PreviousMode == UserMode);
}

/**
 * @brief Validate user mode address
 */
BOOLEAN
NTAPI
KiValidateUserAddress(
    IN PVOID Address,
    IN SIZE_T Length
)
{
    ULONG_PTR StartAddress = (ULONG_PTR)Address;
    ULONG_PTR EndAddress = StartAddress + Length;
    
    /* Check if address is in user space */
    if (StartAddress >= USER_SPACE_END)
        return FALSE;
        
    /* Check for overflow */
    if (EndAddress < StartAddress)
        return FALSE;
        
    /* Check if end address is still in user space */
    if (EndAddress > USER_SPACE_END)
        return FALSE;
    
    return TRUE;
}

/**
 * @brief Copy data from user mode safely
 */
NTSTATUS
NTAPI
KiCopyFromUser(
    OUT PVOID Destination,
    IN PVOID UserSource,
    IN SIZE_T Length
)
{
    if (!KiValidateUserAddress(UserSource, Length))
        return STATUS_ACCESS_VIOLATION;
    
    __try
    {
        RtlCopyMemory(Destination, UserSource, Length);
        return STATUS_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
}

/**
 * @brief Copy data to user mode safely
 */
NTSTATUS
NTAPI
KiCopyToUser(
    OUT PVOID UserDestination,
    IN PVOID Source,
    IN SIZE_T Length
)
{
    if (!KiValidateUserAddress(UserDestination, Length))
        return STATUS_ACCESS_VIOLATION;
    
    __try
    {
        RtlCopyMemory(UserDestination, Source, Length);
        return STATUS_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
}