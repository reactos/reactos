/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Forward-compatible VidInitialize() entry-point -- Assembly stubs
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#if defined(_M_IX86) || defined(_M_AMD64)
#include <asm.inc>
#elif defined(_M_ARM)
#include <kxarm.h>
#elif defined(_M_ARM64)
#include <kxarm64.h>
#else
#error Unknown architecture
#endif

#ifndef _WIN32_WINNT_WIN6 // See psdk/sdkddkver.h
#define _WIN32_WINNT_WIN6   0x0600
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

// BOOLEAN __cdecl VidInitialize_Trampoline(PVOID p1, PVOID p2);
// VOID __cdecl init_VidInitialize(VOID);

#ifndef BOOTVID_EXPORTS_VERSION
#error BOOTVID_EXPORTS_VERSION not defined!
#endif

/**
 * @brief
 * Version-specific VidInitialize() export.
 *
 * The exported VidInitialize(), that forwards to the actual implementation.
 * Because it is exported, its prototype should match what spec2def generates
 * in the exports, depending on the exports version.
 *
 * BOOLEAN
 * NTAPI
 * VidInitialize(VIDINIT_PARAMS);
 **/

#if defined(_M_IX86)
    .code

    EXTERN _init_VidInitialize:PROC
    // _init_VidInitialize PROTO
    EXTERN _VidInitialize_Stub:DWORD
#ifdef _CONTROL_FLOW_GUARD
    EXTERN ___guard_check_icall_fptr:PROC
#endif

    PUBLIC _VidInitialize_Trampoline
    FUNC _VidInitialize_Trampoline
        /* Setup local stack frame */
        push ebp
        mov  ebp, esp

        call _init_VidInitialize

        /* Restore frame */
        leave // mov esp, ebp ; pop ebp
        // __fallthrough
    ENDFUNC

#if (BOOTVID_EXPORTS_VERSION >= _WIN32_WINNT_WIN6)
#define VidInitialize_NAME _VidInitialize@8 // (_In_ INT Phase, _In_ BOOLEAN SetMode)
#else // if (BOOTVID_EXPORTS_VERSION < _WIN32_WINNT_WIN6)
#define VidInitialize_NAME _VidInitialize@4 // (_In_ BOOLEAN SetMode)
#endif
    PUBLIC VidInitialize_NAME
    FUNC VidInitialize_NAME
#ifdef _CONTROL_FLOW_GUARD
        /* __guard_check_icall_fptr() expects the target address in ECX.
         * All VidInitialize() arguments are on the stack, so ECX is free. */
        mov  ecx, [_VidInitialize_Stub]
        call [___guard_check_icall_fptr]
        jmp  ecx
#else
        jmp [_VidInitialize_Stub]
#endif
    ENDFUNC

    END

#elif defined(_M_AMD64)
    .code

    EXTERN init_VidInitialize:PROC
    EXTERN VidInitialize_Stub:QWORD
#ifdef _CONTROL_FLOW_GUARD
    EXTERN __guard_dispatch_icall_fptr:PROC
#endif

// https://godbolt.org/z/36TbKGWoz
    PUBLIC VidInitialize_Trampoline
    FUNC VidInitialize_Trampoline
        /*
         * Save registers:
         * - Save RCX and RDX (they are the 2 maximum parameters used by VidInitialize)
         *   to ensure they survive init_VidInitialize() before we invoke the final stub.
         */
        /* Save original parameters (rcx, rdx) into their home space */
        mov [rsp + 16], rdx
        mov [rsp + 8], rcx

        /* Align the stack on 16 bytes (8 bytes were already pushed by our caller)
         * and add home parameter space (4 slots) for the function being called. */
        sub rsp, (8+4*8)
        .allocstack (8+4*8)
        .endprolog

        call init_VidInitialize

        ;.beginepilog
        /* Wind back the stack */
        ;.freestack (8+4*8)
        add rsp, (8+4*8)

        /* Restore the original parameters for the final stub call */
        mov rcx, [rsp + 8]
        mov rdx, [rsp + 16]
        ;.endepilog
        // __fallthrough
    ENDFUNC

    PUBLIC VidInitialize
    FUNC VidInitialize
        .endprolog
#ifdef _CONTROL_FLOW_GUARD
        /* __guard_dispatch_icall_fptr() validates RAX and tail-jumps
         * to it while preserving RSP, RCX, and RDX. */
        mov rax, [VidInitialize_Stub]
        jmp [__guard_dispatch_icall_fptr]
#else
        jmp [VidInitialize_Stub]
#endif
    ENDFUNC

    END

#elif defined(_M_ARM) // ARM32 Thumb-2
    TEXTAREA

    EXTERN init_VidInitialize ;;[CODE]
    EXTERN VidInitialize_Stub ;;[DATA]
#ifdef _CONTROL_FLOW_GUARD
    EXTERN __guard_check_icall_fptr ;;[CODE]
#endif

// https://godbolt.org/z/a3reecea1
    NESTED_ENTRY VidInitialize_Trampoline, |.text$$mn|
        /*
         * Save registers:
         * - Save R0,1 (they are the 2 maximum parameters used by VidInitialize)
         *   to ensure they survive init_VidInitialize() before we invoke the final stub.
         * - Push LR since we invoke a function below, and FP even if we don't
         *   modify it so as to keep the stack 8-byte aligned.
         */
        /*PROLOG_PUSH*/ push {R0,R1,FP,LR}
        add FP, SP, #8 // Make FP point to the previous FP on the stack.
        //PROLOG_END

        bl init_VidInitialize

        /*
         * Restore registers:
         * - Restore the original R0,1 for the final stub call.
         * - Restore FP and LR without popping it into PC since we fall-through below.
         */
        pop  {R0,R1,FP,LR}
        // __fallthrough
    NESTED_END VidInitialize_Trampoline

    LEAF_ENTRY VidInitialize, |.text$$mn|
#ifdef _CONTROL_FLOW_GUARD
        /* __guard_check_icall_fptr() expects the target in R0 and clobbers LR.
         * Save {R0, R1, R3, LR} (16 bytes, 8-byte aligned) across the check. */
        ldr  R3, =VidInitialize_Stub
        ldr  R3, [R3]
        push {R0, R1, R3, LR}
        mov  R0, R3
        ldr  R3, =__guard_check_icall_fptr
        ldr  R3, [R3]
        blx  R3
        pop  {R0, R1, R3, LR}
        bx   R3 // Tail-call the validated stub
#else
        /* NOTE: Or use IP with wide instructions */
        ldr R3, =VidInitialize_Stub
        ldr PC, [R3] // Tail-call the stub
            //ldr R3, [R3]
            //bx  R3
#endif
    LEAF_END VidInitialize

    END

#elif defined(_M_ARM64)
    TEXTAREA

    EXTERN init_VidInitialize ;;[CODE]
    EXTERN VidInitialize_Stub ;;[DATA]
#ifdef _CONTROL_FLOW_GUARD
    EXTERN __guard_dispatch_icall_fptr ;;[CODE]
#endif

    NESTED_ENTRY VidInitialize_Trampoline //, |.text$$mn|
        /*
         * Save registers:
         * - Save X0,1 (they are the 2 maximum parameters used by VidInitialize)
         *   to ensure they survive init_VidInitialize() before we invoke the final stub.
         * - Save FP (X29) and LR (X30) since we invoke a function below, keeping
         *   SP 16-byte aligned as required by the ARM64 ABI.
         */
#if 0
PROLOG_SAVE_REG_PAIR   fp, lr, #-32!
// mov fp, sp is covered automatically by PROLOG_SAVE_REG_PAIR with '!'
PROLOG_NOP             stp x0, x1, [sp, #16]
#endif
        stp FP, LR, [SP, #-32]!
        mov FP, SP // per the Windows ARM64 frame pointer convention.
        stp X0, X1, [SP, #16]

        bl init_VidInitialize

        /*
         * Restore registers:
         * - Restore the original X0,1 for the final stub call.
         * - Restore FP and LR without popping it into PC since we fall-through below.
         */
        ldp X0, X1, [SP, #16]
        ldp FP, LR, [SP], #32
        // __fallthrough
    NESTED_END VidInitialize_Trampoline

    LEAF_ENTRY VidInitialize //, |.text$$mn|
#ifdef _CONTROL_FLOW_GUARD
        /* __guard_dispatch_icall_fptr() validates X15 and branches directly to X15
         * without modifying LR or X0-X7, so 'blr x16' acts as a tail-call. */
        ldr X16, =VidInitialize_Stub
        ldr X15, [X16]
        ldr X16, =__guard_dispatch_icall_fptr
        ldr X16, [X16]
        blr X16
#else
        // NOTE: XIP0 aliases to X16
        ldr XIP0, =VidInitialize_Stub
        ldr XIP0, [XIP0]
        br  XIP0 // Tail-call the stub
#endif
    LEAF_END VidInitialize

    END

#else
#error Unknown architecture
#endif

/* EOF */
