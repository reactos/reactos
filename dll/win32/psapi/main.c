/*
 * PROJECT:     ReactOS Process Status Helper Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PSAPI Win2k3 style entrypoint
 * COPYRIGHT:   Copyright 2013 Pierre Schweitzer <pierre@reactos.org>
 */

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#define NTOS_MODE_USER
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <psapi.h>

#define NDEBUG
#include <debug.h>

static
VOID
NTAPI
PsParseCommandLine(VOID)
{
    UNIMPLEMENTED;
}

static
VOID
NTAPI
PsInitializeAndStartProfile(VOID)
{
    UNIMPLEMENTED;
}

static
VOID
NTAPI
PsStopAndAnalyzeProfile(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
WINAPI
DllMain(HINSTANCE hDllHandle,
        DWORD nReason,
        LPVOID Reserved)
{
    switch(nReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDllHandle);
            if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PROFILE_USER)
            {
                PsParseCommandLine();
                PsInitializeAndStartProfile();
            }
            break;

        case DLL_PROCESS_DETACH:
            if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PROFILE_USER)
            {
                PsStopAndAnalyzeProfile();
            }
            break;
    }
    return TRUE;
}
