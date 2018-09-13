/****************************** Module Header ******************************\
* Module Name: imminit.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module implements IMM32 initialization
*
* History:
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// required for wow.obj in userrtl.lib
extern ULONG_PTR gHighestUserAddress;


BOOL ImmInitializeGlobals(HINSTANCE hmod)
{
    SYSTEM_BASIC_INFORMATION SystemInformation;

    if (hmod) {
        /*
         * Remember IMM32.DLL's hmodule so we can grab resources from it later.
         */
        ghInst = hmod;
    }
    if (gfInitialized) {
        return TRUE;
    }
    if (!NT_SUCCESS(RtlInitializeCriticalSection(&gcsImeDpi))) {
        RIPMSG0(RIP_WARNING, "ImmInitializeGlobals: failed to initialize critical section at startup. Just bail.");
        return FALSE;
    }
    if (!NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation,
            &SystemInformation,
            sizeof(SystemInformation),
            NULL))) {
        RIPMSG0(RIP_WARNING, "ImmInitializeGlobals: failed to query system information. Just bail.");
        return FALSE;
    }
    gHighestUserAddress = SystemInformation.MaximumUserModeAddress;

    gfInitialized = TRUE;

    return TRUE;
}


BOOL ImmRegisterClient(
    IN PSHAREDINFO psiClient, HINSTANCE hmod)
{
    gSharedInfo = *psiClient;
    gpsi = gSharedInfo.psi;
    /* Raid #97316
     * Dlls loaded earlier than imm32.dll could make
     * user32 call which calls back imm routines.
     * ImmRegisterClient() is called from User32's init routine,
     * so we can expect to reach here early enough.
     * We need to initialize globals as much as possible
     * here.
     */
    return ImmInitializeGlobals(hmod);
}

BOOL ImmDllInitialize(
    IN PVOID hmod,
    IN DWORD Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:
        UserAssert(!gfInitialized || hmod == ghInst);

        if (!ImmInitializeGlobals(hmod))
            return FALSE;

        UserAssert(hmod != NULL);

        // Initialize USER32.DLL in case if USER32 has not bound itself to IMM32
        if (!User32InitializeImmEntryTable(IMM_MAGIC_CALLER_ID))
            return FALSE;
        break;

    case DLL_PROCESS_DETACH:
        if (gfInitialized) {
            RtlDeleteCriticalSection(&gcsImeDpi);
        }
        break;

    case DLL_THREAD_DETACH:
        if (IS_IME_ENABLED()) {
            DestroyInputContext(
                (HIMC)NtUserGetThreadState(UserThreadStateDefaultInputContext),
                GetKeyboardLayout(0),
                TRUE);
        }
        break;

    default:
        break;
    }

    return TRUE;
}


/***************************************************************************\
* Allocation routines for RTL functions.
*
*
\***************************************************************************/

PVOID UserRtlAllocMem(
    ULONG uBytes)
{
    return LocalAlloc(LPTR, uBytes);
}

VOID UserRtlFreeMem(
    PVOID pMem)
{
    LocalFree(pMem);
}

VOID UserRtlRaiseStatus(
    NTSTATUS Status)
{
    RtlRaiseStatus(Status);
}

DWORD GetRipComponent(VOID) { return RIP_IMM; }

DWORD GetDbgTagFlags(int tag)
{
#if DEBUGTAGS
    return (gpsi != NULL ? gpsi->adwDBGTAGFlags[tag] : 0);
#else
    return 0;
    UNREFERENCED_PARAMETER(tag);
#endif // DEBUGTAGS
}

DWORD GetRipPID(VOID) { return (gpsi != NULL ? gpsi->wRIPPID : 0); }
DWORD GetRipFlags(VOID) { return (gpsi != NULL ? gpsi->wRIPFlags : RIPF_DEFAULT); }

extern VOID NtUserSetRipFlags(DWORD, DWORD);

VOID SetRipFlags(DWORD dwRipFlags, DWORD dwRipPID)
{
    NtUserSetRipFlags(dwRipFlags, dwRipPID);
}

VOID SetDbgTag(int tag, DWORD dwBitFlags)
{
    RIPMSG0(RIP_ERROR, "SetDbgTag not available in imm32.dll");
    return;
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(dwBitFlags);
}

