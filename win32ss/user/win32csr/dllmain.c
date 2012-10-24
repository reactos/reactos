/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 */

/* INCLUDES ******************************************************************/

#include "win32csr.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE Win32CsrApiHeap;
HINSTANCE Win32CsrDllHandle = NULL;

PCSR_API_ROUTINE Win32CsrApiDefinitions[3] =
{
    CsrCreateDesktop,
    CsrShowDesktop,
    CsrHideDesktop,
};

BOOLEAN Win32CsrApiServerValidTable[3] =
{
    FALSE,
    FALSE,
    FALSE
};

PCHAR Win32CsrApiNameTable[3] =
{
    "CsrCreateDesktop",
    "CsrShowDesktop",
    "CsrHideDesktop",
};

static HHOOK hhk = NULL;

/* FUNCTIONS *****************************************************************/

LRESULT
CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
   return CallNextHookEx(hhk, nCode, wParam, lParam);
}

BOOL WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        Win32CsrDllHandle = hDll;
//
// HACK HACK HACK ReactOS to BOOT! Initialization BUG ALERT! See bug 5655.
//
        hhk = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
// BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//  BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//   BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
    }

    return TRUE;
}

VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
    NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
}

DWORD
WINAPI
CreateSystemThreads(PVOID pParam)
{
    NtUserCallOneParam((DWORD)pParam, ONEPARAM_ROUTINE_CREATESYSTEMTHREADS);
    DPRINT1("This thread should not terminate!\n");
    return 0;
}

CSR_SERVER_DLL_INIT(Win32CsrInitialization)
{
    HANDLE ServerThread;
    CLIENT_ID ClientId;
    NTSTATUS Status;

    Win32CsrApiHeap = RtlGetProcessHeap();

    NtUserInitialize(0, NULL, NULL);

    PrivateCsrssManualGuiCheck(0);

    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = 4;
    LoadedServerDll->HighestApiSupported = 3;
    LoadedServerDll->DispatchTable = Win32CsrApiDefinitions;
    LoadedServerDll->ValidTable = Win32CsrApiServerValidTable;
    LoadedServerDll->NameTable = Win32CsrApiNameTable;
    // LoadedServerDll->SizeOfProcessData = 0;
    // LoadedServerDll->ConnectCallback = NULL;
    // LoadedServerDll->DisconnectCallback = NULL;
    // LoadedServerDll->HardErrorCallback = Win32CsrHardError;
    // LoadedServerDll->NewProcessCallback = NULL;
    // LoadedServerDll->DisconnectCallback = NULL;

    /* Start Raw Input Threads */
    Status = RtlCreateUserThread(NtCurrentProcess(), NULL, TRUE, 0, 0, 0, (PTHREAD_START_ROUTINE)CreateSystemThreads, (PVOID)0, &ServerThread, &ClientId);
    if (NT_SUCCESS(Status))
    {
        NtResumeThread(ServerThread, NULL);
        NtClose(ServerThread);
    }
    else
        DPRINT1("Cannot start Raw Input Thread!\n");

    return STATUS_SUCCESS;
}

/* EOF */
