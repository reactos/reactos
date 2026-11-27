/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Forward-compatible VidInitialize() entry-point
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <ntifs.h>
#include <ndk/ketypes.h> // For KeLoaderBlock

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief   Generic VidInitializeEx() implemented by the bootvid modules.
 **/
BOOLEAN
NTAPI
VidInitializeEx(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode,
    _In_opt_ PVOID LoaderBlock);

//#if (BOOTVID_EXPORTS_VERSION >= _WIN32_WINNT_WIN6)
/**
 * @brief   VidInitialize() implementation for Windows Vista+ (NT 6.x).
 **/
BOOLEAN
NTAPI
VidInitialize_NT6(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode)
{
    /* On Vista+, VidInitialize() is only invoked with
     * the default kernel loader parameter block.
     * IMPORTANT NOTE: The KeLoaderBlock kernel export
     * *MAY NOT* be initialized in Vista+ !! */
    return VidInitializeEx(Phase, SetMode, KeLoaderBlock);
}
//#else
/**
 * @brief   VidInitialize() implementation for Windows 2000-2003 (NT 5.x).
 **/
BOOLEAN
NTAPI
VidInitialize_NT5(
    _In_ BOOLEAN SetMode)
{
    /* On NT 5.x, VidInitialize() is only invoked at phase 1
     * with the default kernel loader parameter block */
    return VidInitializeEx(1, SetMode, KeLoaderBlock);
}
//#endif


/* Forward-declare the trampoline and function pointer */
//typedef VOID (__cdecl *PVID_INITIALIZE_STUB)(VOID);
//VOID __cdecl VidInitialize_Trampoline(VOID); // Defined in vidinit.S
typedef BOOLEAN (__cdecl *PVID_INITIALIZE_STUB)(PVOID p1, PVOID p2);
BOOLEAN __cdecl VidInitialize_Trampoline(PVOID p1, PVOID p2); // Defined in vidinit.S

/* Initially points to the trampoline, then atomically updated to VidInitialize_NT*() */
volatile PVID_INITIALIZE_STUB VidInitialize_Stub = VidInitialize_Trampoline;

/**
 * @brief
 * Selects the VidInitialize_NT*() implementation.
 *
 * @note
 * The routine is marked no-inline, so that it isn't inlined
 * into the trampoline and messes with its stack.
 * Ensure also no stripping or alteration under LTO (/GL or -flto).
 **/
DECLSPEC_NOINLINE
#if defined(__GNUC__) || defined(__clang__)
__attribute__((used))
#endif
VOID
__cdecl
init_VidInitialize(VOID)
{
    ULONG MajorVersion, MinorVersion;
    PVOID Target;

    (void)PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);
    //DbgPrint("init_VidInitialize for NT %lu.%lu\n", MajorVersion, MinorVersion);

    if (MajorVersion < 6)
        Target = (PVOID)(ULONG_PTR)VidInitialize_NT5;
    else // if (MajorVersion >= 6)
        Target = (PVOID)(ULONG_PTR)VidInitialize_NT6;

    (void)InterlockedExchangePointer((PVOID*)&VidInitialize_Stub, Target);
}


#if !defined(_M_IX86) && !defined(_M_AMD64) && !defined(_M_ARM) && !defined(_M_ARM64)
#error "Please implement an assembler version for this platform! (see vidinit.S)"
#endif

/* EOF */
