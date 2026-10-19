/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Forward-compatible VidInitialize() entry-point
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

// #include "precomp.h"
#include <ntifs.h>
#include <ndk/halfuncs.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief   Generic VidInitializeEx() implemented by the bootvid modules.
 **/
BOOLEAN
NTAPI
VidInitializeEx(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode,
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock);

/**
 * @brief   VidInitialize() prototype for Windows 7+ (NT 6.1+).
 **/
typedef
BOOLEAN
(NTAPI VID_INITIALIZE_NT61)(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode,
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock);
typedef VID_INITIALIZE_NT61 *PVID_INITIALIZE_NT61;

/**
 * @brief   VidInitialize() prototype for Windows Vista (NT 6.0).
 **/
typedef
BOOLEAN
(NTAPI VID_INITIALIZE_NT60)(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode);
typedef VID_INITIALIZE_NT60 *PVID_INITIALIZE_NT60;

/**
 * @brief   VidInitialize() prototype for Windows 2000-2003 (NT 5.x).
 **/
typedef
BOOLEAN
(NTAPI VID_INITIALIZE_NT5)(
    _In_ BOOLEAN SetMode);
typedef VID_INITIALIZE_NT5 *PVID_INITIALIZE_NT5;

/**
 * @brief   Version-specific PVID_INITIALIZE function typedef.
 **/
#if (NTDDI_VERSION >= NTDDI_WIN7)
//typedef PVID_INITIALIZE_NT61 PVID_INITIALIZE;
#define  VID_INITIALIZE  VID_INITIALIZE_NT61
#define PVID_INITIALIZE PVID_INITIALIZE_NT61
#elif (NTDDI_VERSION == NTDDI_VISTA)
//typedef PVID_INITIALIZE_NT60 PVID_INITIALIZE;
#define  VID_INITIALIZE  VID_INITIALIZE_NT60
#define PVID_INITIALIZE PVID_INITIALIZE_NT60
#else // #if (NTDDI_VERSION < NTDDI_VISTA)
//typedef PVID_INITIALIZE_NT5  PVID_INITIALIZE;
#define  VID_INITIALIZE  VID_INITIALIZE_NT5
#define PVID_INITIALIZE PVID_INITIALIZE_NT5
#endif


//#if (NTDDI_VERSION >= NTDDI_WIN7)
BOOLEAN
NTAPI
VidInitialize_NT61(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode,
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* On Win7+, directly invoke VidInitializeEx() at the
     * given phase and loader parameter block */
    return VidInitializeEx(Phase, SetMode, LoaderBlock);
}
//#elif (NTDDI_VERSION == NTDDI_VISTA)
BOOLEAN
NTAPI
VidInitialize_NT60(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode)
{
    /* On Vista, VidInitialize() is only invoked with
     * the default kernel loader parameter block.
     * IMPORTANT NOTE: The KeLoaderBlock kernel export
     * *MAY NOT* be initialized in Vista+ !! */
    return VidInitializeEx(Phase, SetMode, KeLoaderBlock);
}
//#else // #if (NTDDI_VERSION < NTDDI_VISTA)
BOOLEAN
NTAPI
VidInitialize_NT5(
    _In_ BOOLEAN SetMode)
{
    /* On NT 5.x, VidInitialize() is only invoked at phase 0
     * with the default kernel loader parameter block */
    return VidInitializeEx(0, SetMode, KeLoaderBlock);
}
//#endif



typedef BOOLEAN (__cdecl *PVID_INITIALIZE_STUB)(PVOID p1, PVOID p2, PVOID p3);

BOOLEAN __cdecl VidInitialize_Trampoline(PVOID p1, PVOID p2, PVOID p3);

/* Initially points to the trampoline, then to VidInitialize_NT*() */
/*extern*/ PVID_INITIALIZE_STUB VidInitialize_Stub = VidInitialize_Trampoline;

/**
 * @brief
 * Selects the VidInitialize_NT*() implementation.
 *
 * @note
 * The routine is marked no-inline, so that it isn't inlined
 * into the trampoline and messes with its stack.
 **/
static
DECLSPEC_NOINLINE
VOID
__init_VidInitialize(VOID)
{
    ULONG MajorVersion, MinorVersion;
    (void)PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);
    DbgPrint("__init_VidInitialize for NT %lu.%lu\n", MajorVersion, MinorVersion);
    if (MajorVersion < 6)
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT5;
    else if (MajorVersion == 6 && MinorVersion == 0)
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT60;
    else
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT61;
}


/*
 * Per-function code generation control. These functions must not get
 * stack-checking / runtime-check instrumentation, and must not have a
 * frame that we don't know about when we jump out of them.
 */
#ifdef _MSC_VER

///#pragma runtime_checks("", off)
#pragma runtime_checks("s", off)
#pragma check_stack(off)
#pragma strict_gs_check(push, off)

/*
 * Enable the optimizations required for tail-calls on x64 (and a predictable
 * code on x86), for the functions below only, even when compiling with /Od.
 * On x86 keep the ebp frame (no /Oy), as the inline asm relies on it.
 */
#pragma optimize("gt", on)
#ifdef _M_IX86
#pragma optimize("y", off)
#endif
#pragma warning(push)
#pragma warning(disable:4731) // "frame pointer register 'ebp' modified by inline assembly code"

#endif // _MSC_VER


#ifdef _M_IX86
/*
 * x86: the epilogue of the function is skipped. The frame is ebp-based and
 * holds only the locals, so "leave" restores esp/ebp as they were on entry.
 */
#ifdef _MSC_VER

#define TAIL_JUMP_TO_MEM_FRAME(fn) \
    __asm { __asm leave /*mov esp,ebp; pop ebp*/ __asm jmp dword ptr [fn] }
#define TAIL_JUMP_TO_MEM(fn) \
    __asm { jmp dword ptr [fn] }

#define TRAMP_ATTR  __declspec(safebuffers)

#else // def __GNUC__

#define TAIL_JUMP_TO_MEM_FRAME(fn) \
    __asm__ __volatile__("leave\n\tjmp *%0" : : "m"(fn)); return 0
#define TAIL_JUMP_TO_MEM(fn) \
    __asm__ __volatile__("jmp *%0" : : "m"(fn)); return 0

#if (__GNUC__ >= 11)
#define NO_CHECKS   __attribute__((no_stack_protector))
#else
#define NO_CHECKS   // __attribute__((__optimize__("-fno-stack-protector"))) /* Nothing: no arrays nor address-taken locals => no canary */
#endif
#define TRAMP_ATTR  NO_CHECKS __attribute__((optimize("no-omit-frame-pointer")))

#endif // _MSC_VER

#else
/*
 * x64 and others: no callee stack cleanup, a normal call is fine.
 */

#ifdef _MSC_VER
#define TRAMP_ATTR
#else
/* Optimized, for the call below to become a direct tail-jump */
#define TRAMP_ATTR  __attribute__((optimize("O2")))
#endif

#endif // _M_IX86


// https://godbolt.org/z/939vxdvrE
// https://godbolt.org/z/MnGzr1G1f
// https://godbolt.org/z/z73qej45q
// https://godbolt.org/z/Phacqd9cY
/**
 * We are invoked as a STDCALL entry-point, but we are CDECL. This means:
 * - The caller pushes on the stack (x86) / sets up the registers (x64, etc.)
 *   the necessary parameters, but expects the callee (us) to clean them up
 *   on return.
 * - Only the ultimate version-specific STDCALL VidInitialize_NT*() routines,
 *   set up by the trampoline, know the actual number of parameters on input
 *   and can clean them up on return.
 * - The single-usage VidInitialize_Trampoline() doesn't know the actual
 *   number of parameters it is invoked with, except their maximum number
 *   it can be invoked with. Since it also invokes extra code for determining
 *   which actual VidInitialize_NT*() routine will have to be used, it has
 *   to not interfere with the unknown parameters it is invoked with.
 * - ...
 **/
TRAMP_ATTR
BOOLEAN
__cdecl
VidInitialize_Trampoline(PVOID p1, PVOID p2, PVOID p3) /* Needs to protect at most 3 parameters */
{
    __init_VidInitialize();
#ifdef _M_IX86
    TAIL_JUMP_TO_MEM_FRAME(VidInitialize_Stub);
#else
    return ((PVID_INITIALIZE_STUB)VidInitialize_Stub)(p1, p2, p3);
#endif
}


/**
 * @brief
 * Version-specific VidInitialize() export.
 *
 * The exported VidInitialize(), that forwards to the actual implementation.
 * Because it is exported, its prototype should match what spec2def generates
 * in the exports, depending on the exports version.
 **/

#if (NTDDI_VERSION >= NTDDI_WIN7)
    #define VIDINIT_PARAMS      _In_ INT Phase, _In_ BOOLEAN SetMode, _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock
    #define VIDINIT_PARAMS_CALL          Phase,              SetMode,                                  LoaderBlock
#elif (NTDDI_VERSION == NTDDI_VISTA)
    #define VIDINIT_PARAMS      _In_ INT Phase, _In_ BOOLEAN SetMode
    #define VIDINIT_PARAMS_CALL          Phase,              SetMode
#else // #if (NTDDI_VERSION < NTDDI_VISTA)
    #define VIDINIT_PARAMS      _In_ BOOLEAN SetMode
    #define VIDINIT_PARAMS_CALL              SetMode
#endif

#ifdef _M_IX86
/* Frameless, so that the jump leaves the caller's stack untouched */
#ifdef _MSC_VER
#pragma optimize("gy", on)
__declspec(naked)
#else // def __GNUC__
__attribute__((optimize("O","-fomit-frame-pointer","-foptimize-sibling-calls","-ftree-tail-merge")))
__attribute__((naked))
#endif
#endif // _M_IX86
BOOLEAN
NTAPI
VidInitialize(VIDINIT_PARAMS)
{
#ifdef _M_IX86
    TAIL_JUMP_TO_MEM(VidInitialize_Stub);
#else
    return ((PVID_INITIALIZE)VidInitialize_Stub)(VIDINIT_PARAMS_CALL);
#endif
}
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma optimize("gy", off)
#endif


#ifdef _MSC_VER
#pragma warning(pop)
#pragma optimize("", on)

#pragma strict_gs_check(pop)
#pragma check_stack() // Restore defaults
#pragma runtime_checks("s", restore)
///#pragma runtime_checks("", restore)
#endif // _MSC_VER

/* EOF */
