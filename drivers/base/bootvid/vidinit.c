/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Forward-compatible VidInitialize() entry-point
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

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
(NTAPI *PVID_INITIALIZE_NT61)(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode,
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock);

/**
 * @brief   VidInitialize() prototype for Windows Vista (NT 6.0).
 **/
typedef
BOOLEAN
(NTAPI *PVID_INITIALIZE_NT60)(
    _In_ INT Phase,
    _In_ BOOLEAN SetMode);

/**
 * @brief   VidInitialize() prototype for Windows 2000-2003 (NT 5.x).
 **/
typedef
BOOLEAN
(NTAPI *PVID_INITIALIZE_NT5)(
    _In_ BOOLEAN SetMode);

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

BOOLEAN (__cdecl *VidInitialize_Stub)(PVOID p1, PVOID p2, PVOID p3) = VidInitialize_Trampoline;

/*static*/ VOID
__cdecl
_init_VidInitialize(VOID)
{
    ULONG MajorVersion, MinorVersion;
    (void)PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);
    DbgPrint("VidInitialize_Trampoline\n");////////////////////////////////////
    if (MajorVersion < 6)
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT5;
    else if (MajorVersion == 6 && MinorVersion == 0)
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT60;
    else
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT61;
}

// https://godbolt.org/z/939vxdvrE
#if !defined(_MSC_VER)
//__attribute__((optimize("O","-fomit-frame-pointer",/*"-fcaller-saves",*/"-foptimize-sibling-calls","-ftree-tail-merge")))
//__attribute__((optimize("-fno-omit-frame-pointer","-fcaller-saves","-foptimize-sibling-calls","-ftree-tail-merge")))
#else
//#pragma optimize("gy", on)
#endif
BOOLEAN
__cdecl
VidInitialize_Trampoline(PVOID p1, PVOID p2, PVOID p3) /* Needs to protect at most 3 parameters */
{
#if 0
    _init_VidInitialize();
    /*
     * IMPORTANT NOTES:
     *
     * - We won't use the trick of modifying the return address, as in:
     *   *(void**)_AddressOfReturnAddress() = fn;
     *   because this will fail with recent CPU return address protection systems.
     *
     * Don't make _init_VidInitialize() return the initialized stub and
     * invoke it directly as:
     *   return _init_VidInitialize()(p1, p2, p3);
     * because with some optimization levels on MSVC this would unwind
     * the stack too late and will mess up with our calling trick.
     */
#else
    ULONG MajorVersion, MinorVersion;
    (void)PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);
    if (MajorVersion < 6)
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT5;
    else if (MajorVersion == 6 && MinorVersion == 0)
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT60;
    else
        VidInitialize_Stub = (PVID_INITIALIZE_STUB)VidInitialize_NT61;
#endif
    return VidInitialize_Stub(p1, p2, p3);
}
#if !defined(_MSC_VER)
//#pragma optimize("gy", off)
#endif

/* EOF */
