/*
 * PROJECT:     ReactOS Runtime Library (RTL)
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NtUserPfn dispatch code
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES ******************************************************************/

#if defined(_M_IX86) || defined(_M_AMD64)

#include <asm.inc>

#if (NTDDI_VERSION >= NTDDI_WIN10)
#define NUMBER_OF_NTUSER_AW_PFNS 24
#else
#define NUMBER_OF_NTUSER_AW_PFNS 23
#endif
#define NUMBER_OF_NTUSER_OTHER_PFNS 11
#define RTL_NUMBER_OF_NTUSER_PFNS (2 * NUMBER_OF_NTUSER_AW_PFNS + NUMBER_OF_NTUSER_OTHER_PFNS)

#if defined(_M_IX86)
EXTERN _RtlpNtUserPfns:DWORD
#else
EXTERN RtlpNtUserPfns:QWORD
#endif

/* ARCH SPECIFIC MACROS ******************************************************/

#if defined(_M_IX86)

MACRO(DEFINE_NTUSER_PFN_THUNK, Index)
    ALIGN 16
    RtlpNtUserPfnDispatch&Index:
        jmp dword ptr _RtlpNtUserPfns[Index * 4]
ENDM

#elif defined(_M_AMD64)

MACRO(DEFINE_NTUSER_PFN_THUNK, Index)
    ALIGN 16
    RtlpNtUserPfnDispatch&Index:
        jmp qword ptr RtlpNtUserPfns[rip + Index * 8]
ENDM

#elif defined(_M_ARM)

    MACRO
    DEFINE_NTUSER_PFN_THUNK $Index
        LEAF_ENTRY RtlpNtUserPfnDispatch$Index
            ldr r12, =RtlpNtUserPfns + (Index * 8)
            ldr r12, [r12]
            bx  r12
        LEAF_END RtlpNtUserPfnDispatch$Index
    MEND

#elif defined(_M_ARM64)

    MACRO(DEFINE_NTUSER_PFN_THUNK, Index)
        LEAF_ENTRY RtlpNtUserPfnDispatch&Index
            adrp x16, RtlpNtUserPfns + (Index * 8)
            ldr  x16, [x16, #:lo12:RtlpNtUserPfns + (Index * 8)]
            br   x16
        LEAF_END RtlpNtUserPfnDispatch&Index
    ENDM

#else
#error Unsupported architecture
#endif


/* DISPATCH THUNKS ***********************************************************/

#ifdef _WIN64
.code64
#elif defined(_M_IX86)
.code
#endif

/* Generate the thunk functions */
#ifndef _M_ARM
Index = 0
REPEAT RTL_NUMBER_OF_NTUSER_PFNS
    DEFINE_NTUSER_PFN_THUNK %Index
    Index = Index + 1
ENDR
#endif

/* DISPATCH POINTER TABLE ****************************************************/

#if defined(_M_IX86) || defined(_M_AMD64)
.const
#else
#endif

#ifdef _WIN64
MACRO(DEFINE_NTUSER_PFN_REF, Index)
    .quad RtlpNtUserPfnDispatch&Index
ENDM
#else
MACRO(DEFINE_NTUSER_PFN_REF, Index)
    .long RtlpNtUserPfnDispatch&Index
ENDM
#endif

#if defined(_M_IX86)
ASSUME CS:NOTHING
PUBLIC _NtDllUserStubs
_NtDllUserStubs:
#else
PUBLIC NtDllUserStubs
NtDllUserStubs:
#endif

Index = 0
REPEAT RTL_NUMBER_OF_NTUSER_PFNS
    DEFINE_NTUSER_PFN_REF %Index
    Index = Index + 1
ENDR


#elif defined(_M_ARM) || defined(_M_ARM64)

// TODO Implement this

#endif

    END
