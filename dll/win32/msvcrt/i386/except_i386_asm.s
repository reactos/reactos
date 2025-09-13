/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x686 asm code for except_i386.c
 * COPYRIGHT:   Copyright 2017-2024 Wine team
 */

#include <asm.inc>

// See msvcrt/except_i386.c

.code

PUBLIC _continue_after_catch
_continue_after_catch:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    mov esp, [edx - 4]
    lea ebp, [edx + 12]
    jmp eax

PUBLIC _call_finally_block
_call_finally_block:
    mov ebp, [esp + 8]
    jmp dword ptr [esp + 4]

PUBLIC _call_filter
_call_filter:
    push ebp
    push dword ptr [esp + 12]
    mov ebp, [esp + 20]
    call dword ptr [esp + 12]
    pop ebp
    pop ebp
    ret

PUBLIC _call_handler
_call_handler:
    push ebp
    push ebx
    push esi
    push edi
    mov ebp, [esp + 24]
    call dword ptr [esp + 20]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

//
// DWORD
// __cdecl
// __CxxFrameHandler(
//     PEXCEPTION_RECORD rec,
//     EXCEPTION_REGISTRATION_RECORD* frame,
//     PCONTEXT context,
//     EXCEPTION_REGISTRATION_RECORD** dispatch);
//
// See except_i386.c
//
EXTERN _cxx_frame_handler:PROC
PUBLIC ___CxxFrameHandler
___CxxFrameHandler:

    push 0        /* nested_trylevel */
    //.cfi_adjust_cfa_offset 4
    push 0        /* nested_frame */
    //.cfi_adjust_cfa_offset 4
    push eax      /* descr */
    //.cfi_adjust_cfa_offset 4
    push [esp + 28]  /* dispatch */
    //.cfi_adjust_cfa_offset 4
    push [esp + 28]  /* context */
    //.cfi_adjust_cfa_offset 4
    push [esp + 28]  /* frame */
    //.cfi_adjust_cfa_offset 4
    push [esp + 28]  /* rec */
    //.cfi_adjust_cfa_offset 4
    call _cxx_frame_handler
    add esp, 28
    //.cfi_adjust_cfa_offset -28
    ret

PUBLIC __EH_prolog
__EH_prolog:
    //.cfi_adjust_cfa_offset 4
    push -1
    //.cfi_adjust_cfa_offset 4
    push eax
    //.cfi_adjust_cfa_offset 4
    push fs:0
    //.cfi_adjust_cfa_offset 4
    mov fs:0, esp
    mov eax, [esp + 12]
    mov [esp + 12], ebp
    lea ebp, [esp + 12]
    push eax
    //.cfi_adjust_cfa_offset 4
    ret

END
