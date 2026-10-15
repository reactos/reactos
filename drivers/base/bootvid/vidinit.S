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
    // FIXME: missing in our kxarm.h; trivial definitions below.
    #ifndef PROLOG_PUSH
        #define PROLOG_PUSH         push
        #define EPILOG_POP          pop
        #define PROLOG_NOP
        #define EPILOG_NOP
        #define EPILOG_BRANCH_REG   EPILOG_NOP bx
    #endif
#elif defined(_M_ARM64)
#include <kxarm64.h>
    // FIXME: missing in our kxarm64.h; dummy definitions below.
    #ifndef PROLOG_NOP
        #define PROLOG_NOP
        #define EPILOG_NOP
        #define EPILOG_BRANCH_REG   EPILOG_NOP br
    #endif
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
        /* __fallthrough */
    ENDFUNC

#if (BOOTVID_EXPORTS_VERSION >= _WIN32_WINNT_WIN6)
#define VidInitialize_NAME _VidInitialize@8 // (_In_ INT Phase, _In_ BOOLEAN SetMode)
#else
#define VidInitialize_NAME _VidInitialize@4 // (_In_ BOOLEAN SetMode)
#endif
    PUBLIC VidInitialize_NAME
    FUNC VidInitialize_NAME
    #ifdef _CONTROL_FLOW_GUARD
        /* __guard_check_icall_fptr() expects the target address in ECX.
         * All VidInitialize() arguments are on the stack, so ECX is free. */
        mov  ecx, dword ptr [_VidInitialize_Stub]
        call dword ptr [___guard_check_icall_fptr]
        jmp  ecx // Tail-call the validated stub.
    #else
        jmp dword ptr [_VidInitialize_Stub] // Tail-call the stub.
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

    PUBLIC VidInitialize_Trampoline
    FUNC VidInitialize_Trampoline
        /*
         * Save registers:
         * - Save RCX and RDX (they are the 2 maximum parameters used by VidInitialize)
         *   to ensure they survive init_VidInitialize() before we invoke the final stub.
         */
        /* Save original parameters (RCX, RDX) into their home space */
        mov [rsp + 16], rdx
        mov [rsp + 8], rcx

        /* Align the stack on 16 bytes (8 bytes were already pushed by our caller)
         * and add home parameter space (4 slots) for the function being called. */
        sub rsp, (8+4*8)
        .allocstack (8+4*8)
        .endprolog

        call init_VidInitialize

        //.beginepilog
        /* Wind back the stack */
        //.freestack (8+4*8)
        add rsp, (8+4*8)

        /* Restore the original parameters for the final stub call */
        mov rcx, [rsp + 8]
        mov rdx, [rsp + 16]
        //.endepilog
        /* __fallthrough */
    ENDFUNC

    PUBLIC VidInitialize
    FUNC VidInitialize
        .endprolog
    #ifdef _CONTROL_FLOW_GUARD
        /* __guard_dispatch_icall_fptr() validates RAX and tail-jumps
         * directly to it while preserving RSP, RCX, and RDX. */
        mov rax, qword ptr [VidInitialize_Stub]
        jmp qword ptr [__guard_dispatch_icall_fptr] // Validate and tail-call the stub.
    #else
        jmp qword ptr [VidInitialize_Stub] // Tail-call the stub.
    #endif
    ENDFUNC

    END

#elif defined(_M_ARM) // ARM32 Thumb-2

#ifdef _CONTROL_FLOW_GUARD
    #define CFG_LEAF_ENTRY  NESTED_ENTRY
    #define CFG_LEAF_END    NESTED_END
#else
    #define CFG_LEAF_ENTRY  LEAF_ENTRY
    #define CFG_LEAF_END    LEAF_END
#endif

    TEXTAREA

    EXTERN init_VidInitialize ;;[CODE]
    EXTERN VidInitialize_Stub ;;[DATA]
#ifdef _CONTROL_FLOW_GUARD
    EXTERN __guard_check_icall_fptr ;;[CODE]
#endif

    NESTED_ENTRY VidInitialize_Trampoline, |.text$$mn|
        /*
         * Save registers:
         * - Save R0,1 (they are the 2 maximum parameters used by VidInitialize)
         *   to ensure they survive init_VidInitialize() before we invoke the final stub.
         * - Push LR since we invoke a function below, and FP even if we don't
         *   modify it so as to keep the stack 8-byte aligned.
         */
        PROLOG_PUSH {r0,r1,fp,lr}
        add fp, sp, #8 // Point FP to the previous one on the stack.
        //PROLOG_END

        bl init_VidInitialize

        /*
         * Restore registers:
         * - Restore the original R0,1 for the final stub call.
         * - Restore FP and LR without popping it into PC since we fall-through below.
         */
        EPILOG_POP  {r0,r1,fp,lr}
        /* __fallthrough */
    NESTED_END VidInitialize_Trampoline

    CFG_LEAF_ENTRY VidInitialize, |.text$$mn|
    #ifdef _CONTROL_FLOW_GUARD
        /* __guard_check_icall_fptr() expects the target in R0, preserves R0-R3,
         * and clobbers R12 (IP), CPSR, and LR. Save 4 registers (16 bytes) to
         * maintain 8-byte stack alignment and preserve R0, R1, and LR. */
        PROLOG_PUSH {r0,r1,r4,lr}

        ldr r0, =VidInitialize_Stub
        ldr r0, [r0]
        ldr r12, =__guard_check_icall_fptr
        ldr r12, [r12]
        blx r12
        mov r12, r0 // Move the target back into R12 before restoring R0

        /* Restore the original R0,1,4 and LR, before tail-calling */
        EPILOG_POP  {r0,r1,r4,lr}
        EPILOG_BRANCH_REG r12 // Tail-call the validated stub.
    #else
        /* NOTE: Or use IP with wide instructions */
        ldr r3, =VidInitialize_Stub
        ldr pc, [r3] // Tail-call the stub.
            //ldr r3, [r3]
            //bx  r3
    #endif
    CFG_LEAF_END VidInitialize

    END

#elif defined(_M_ARM64)

#ifdef _CONTROL_FLOW_GUARD
    #define CFG_LEAF_ENTRY  NESTED_ENTRY
    #define CFG_LEAF_END    NESTED_END
#else
    #define CFG_LEAF_ENTRY  LEAF_ENTRY
    #define CFG_LEAF_END    LEAF_END
#endif

    TEXTAREA

    EXTERN init_VidInitialize ;;[CODE]
    EXTERN VidInitialize_Stub ;;[DATA]
#ifdef _CONTROL_FLOW_GUARD
    EXTERN __guard_check_icall_fptr ;;[CODE]
#endif

    NESTED_ENTRY VidInitialize_Trampoline //, |.text$$mn|
        /*
         * Save registers:
         * - Save X0,1 (they are the 2 maximum parameters used by VidInitialize)
         *   to ensure they survive init_VidInitialize() before we invoke the final stub.
         * - Save FP (X29) and LR (X30) since we invoke a function below, keeping
         *   SP 16-byte aligned as required by the ARM64 ABI.
         */
        stp fp, lr, [sp, #-32]!
        mov fp, sp // Per the Windows ARM64 frame pointer convention.
            // PROLOG_SAVE_REG_PAIR  fp, lr, #-32! // mov fp, sp is covered automatically in this case.
        PROLOG_NOP stp x0, x1, [sp, #16]

        bl init_VidInitialize

        /*
         * Restore registers:
         * - Restore the original X0,1 for the final stub call.
         * - Restore FP and LR without popping it into PC since we fall-through below.
         */
        EPILOG_NOP ldp x0, x1, [sp, #16]
        ldp fp, lr, [sp], #32
            // EPILOG_RESTORE_REG_PAIR fp, lr, #32!
        /* __fallthrough */
    NESTED_END VidInitialize_Trampoline

    CFG_LEAF_ENTRY VidInitialize //, |.text$$mn|
    #ifdef _CONTROL_FLOW_GUARD
        /* __guard_check_icall_fptr() validates X15 and returns. It preserves
         * X0-X8 and X15, but clobbers X16 (XIP0), X17, NZCV flags, and LR. */
        stp fp, lr, [sp, #-16]!
        mov fp, sp // Per the Windows ARM64 frame pointer convention.
            // PROLOG_SAVE_REG_PAIR  fp, lr, #-16! // mov fp, sp is covered automatically in this case.

        adrp x15, VidInitialize_Stub
        ldr  x15, [x15, VidInitialize_Stub]
            //ldr x15, =VidInitialize_Stub
            //ldr x15, [x15]
        adrp x16, __guard_check_icall_fptr
        ldr  x16, [x16, __guard_check_icall_fptr]
            //ldr x16, =__guard_check_icall_fptr
            //ldr x16, [x16]
        blr x16

        ldp fp, lr, [sp], #16
            // EPILOG_RESTORE_REG_PAIR fp, lr, #16!
        EPILOG_BRANCH_REG x15 // Tail-call the validated stub.
    #else
        adrp x16, VidInitialize_Stub
        ldr  x16, [x16, VidInitialize_Stub]
            //ldr x16, =VidInitialize_Stub
            //ldr x16, [x16]
        br  x16 // Tail-call the stub.
    #endif
    CFG_LEAF_END VidInitialize

    END

#else
#error Unknown architecture
#endif

/* EOF */
