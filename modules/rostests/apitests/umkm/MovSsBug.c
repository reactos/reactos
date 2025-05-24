/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for MOV/POP SS bug
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <windef.h>

extern void RunMovSsAndSyscall(void);
extern UCHAR SyscallInstruction;

typedef enum _BREAKPOINT_TYPE
{
    Type_Write = 1,
    Type_Access = 3,
    Type_Execute = 0
} BREAKPOINT_TYPE;

BOOLEAN
SetHardwareBreakpoint(
    _In_ ULONG BreakpointNumber,
    _In_ ULONG_PTR Address,
    _In_ BREAKPOINT_TYPE Type,
    _In_ UCHAR Size)
{
    CONTEXT Context = { 0 };
    ULONG SizeSpecifier;
    NTSTATUS Status;

    if (Size == 1)
        SizeSpecifier = 0x00; // 1 byte
    else if (Size == 2)
        SizeSpecifier = 0x01; // 2 bytes
    else if (Size == 4)
        SizeSpecifier = 0x02; // 4 bytes
    else if (Size == 8)
        SizeSpecifier = 0x03; // 8 bytes
    else
        return FALSE; // Invalid size

    Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    switch (BreakpointNumber)
    {
    case 0:
        Context.Dr0 = Address;
        Context.Dr7 |= 0x00000001; // Enable DR0
        Context.Dr7 |= (Type << 16) | (SizeSpecifier << 18);
        break;
    case 1:
        Context.Dr1 = Address;
        Context.Dr7 |= 0x00000002; // Enable DR1
        Context.Dr7 |= (Type << 20) | (SizeSpecifier << 22);
        break;
    case 2:
        Context.Dr2 = Address;
        Context.Dr7 |= 0x00000004; // Enable DR2
        Context.Dr7 |= (Type << 24) | (SizeSpecifier << 26);
        break;
    case 3:
        Context.Dr3 = Address;
        Context.Dr7 |= 0x00000008; // Enable DR3
        Context.Dr7 |= (Type << 28) | (SizeSpecifier << 30);
        break;
    default:
        return FALSE; // Invalid breakpoint number
    }

    // Set the context with the new breakpoint
    Status = NtSetContextThread(NtCurrentThread(), &Context);
    if (!NT_SUCCESS(Status))
    {
        trace("Failed to set context: %lx\n", Status);
        return FALSE;
    }

    return TRUE;
}

void Test_HardwareBreakpoint()
{
    /* Set a hardware execution break point on the syscall / int 0x2e */
    SetHardwareBreakpoint(0, (ULONG_PTR)&SyscallInstruction, Type_Execute, 1);

    /* Run the code. If the bug is mitigated, the break point will be ignored.
       Otherwise ... oops */
    RunMovSsAndSyscall();
}

START_TEST(MovSsBug)
{
    Test_HardwareBreakpoint();

    /* If we got here, the test succeeded */
    trace("Test passed: MOV SS bug mitigated successfully.\n");
    ok(TRUE, "\n");
}
