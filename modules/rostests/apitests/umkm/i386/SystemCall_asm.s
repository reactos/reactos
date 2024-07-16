/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     x86 ASM helper functions for syscall tests
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

.code

#define STACK_ARGUMENT_SPACE 16*4

 EXTERN _RtlCaptureContext@4:PROC

DoSyscall:
    mov edx, esp
    sysenter
    ret

/*
 * VOID
 * DoSyscallAndCaptureContext(
 *     _In_ ULONG64 SyscallNumber,
 *     _Out_ PCONTEXT PreContext,
 *     _Out_ PCONTEXT PostContext);
 */
PUBLIC _DoSyscallAndCaptureContext
.PROC _DoSyscallAndCaptureContext
    push ebp
    mov ebp, esp

    /* Allocate enough space for the system call handler */
    sub esp, STACK_ARGUMENT_SPACE

    /* Save the pre-context */
    push [ebp + 12]
    call _RtlCaptureContext@4

    /* Do the system call */
    mov eax, [ebp + 8]
    Call DoSyscall

    /* Save eax */
    push eax

    /* Save the post-context */
    push dword ptr [ebp + 16]
    call _RtlCaptureContext@4

    /* Restore eax and save it in the context */
    pop eax
    mov ecx, [ebp + 16]
    mov [ecx + CsEax], eax

    mov esp, ebp
    pop ebp
    ret
.ENDP

/*
 * ULONG64
 * DoSyscallWithUnalignedStack(
 *     _In_ ULONG64 SyscallNumber);
 */
PUBLIC _DoSyscallWithUnalignedStack
.PROC _DoSyscallWithUnalignedStack
    push ebp
    mov ebp, esp

    /* Allocate enough space for the system call handler */
    sub esp, STACK_ARGUMENT_SPACE

    /* Do the sysenter */
    mov eax, [ebp + 8]
    sysenter

    mov esp, ebp
    pop ebp
    ret
.ENDP

END
