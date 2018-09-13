/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dllentry.cxx

Abstract:

    Entry point for WinInet Internet client DLL

    Contents:
        WinInetDllEntryPoint

Author:

    Richard L Firth (rfirth) 10-Nov-1994

Environment:

    Win32 (user-mode) DLL

Revision History:

    10-Nov-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include <process.h>
#include <perfdiag.hxx>
#include <shlwapi.h>
#include <advpub.h>
#include "autodial.h"

#define FLAGS_SZ "Flags"
#define FLAGS_DW PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI | PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED
#define IE_SECURITY_DIGEST_REG_KEY "Software\\Microsoft\\Internet Explorer\\Security\\Digest"

#if defined(__cplusplus)
extern "C" {
#endif

BOOL
WINAPI
DllMain(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    );


#if defined(__cplusplus)
}
#endif

//
// global data
//

GLOBAL CRITICAL_SECTION GeneralInitCritSec = {0};

//
// functions
//


BOOL
WINAPI
DllMain(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    )

/*++

Routine Description:

    Performs global initialization and termination for all protocol modules.

    This function only handles process attach and detach which are required for
    global initialization and termination, respectively. We disable thread
    attach and detach. New threads calling Wininet APIs will get an
    INTERNET_THREAD_INFO structure created for them by the first API requiring
    this structure

Arguments:

    DllHandle   - handle of this DLL. Unused

    Reason      - process attach/detach or thread attach/detach

    Reserved    - if DLL_PROCESS_ATTACH, NULL means DLL is being dynamically
                  loaded, else static. For DLL_PROCESS_DETACH, NULL means DLL
                  is being freed as a consequence of call to FreeLibrary()
                  else the DLL is being freed as part of process termination

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Failed to initialize

--*/

{
    if (Reason != DLL_PROCESS_ATTACH) {

        DEBUG_ENTER((DBG_DLL,
                     Bool,
                     "DllMain",
                     "%#x, %s, %#x",
                     DllHandle,
                     (Reason == DLL_PROCESS_ATTACH) ? "DLL_PROCESS_ATTACH"
                     : (Reason == DLL_PROCESS_DETACH) ? "DLL_PROCESS_DETACH"
                     : (Reason == DLL_THREAD_ATTACH) ? "DLL_THREAD_ATTACH"
                     : (Reason == DLL_THREAD_DETACH) ? "DLL_THREAD_DETACH"
                     : "?",
                     Reserved
                     ));

    }

    DWORD error;

    //
    // perform global dll initialization, if any.
    //
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        GlobalDllHandle = DllHandle;
        GlobalPlatformType = PlatformType(&GlobalPlatformVersion5);
        InitializeCriticalSection(&GeneralInitCritSec);

        INITIALIZE_DEBUG_REGKEY();
        INITIALIZE_DEBUG_MEMORY();
        INET_DEBUG_START();

        GlobalDllInitialize();
        if (!InternetCreateThreadInfo(TRUE)) {
            return FALSE;
        }

        DEBUG_ENTER((DBG_DLL,
                     Bool,
                     "DllMain",
                     "%#x, %s, %#x",
                     DllHandle,
                     (Reason == DLL_PROCESS_ATTACH) ? "DLL_PROCESS_ATTACH"
                     : (Reason == DLL_PROCESS_DETACH) ? "DLL_PROCESS_DETACH"
                     : (Reason == DLL_THREAD_ATTACH) ? "DLL_THREAD_ATTACH"
                     : (Reason == DLL_THREAD_DETACH) ? "DLL_THREAD_DETACH"
                     : "?",
                     Reserved
                     ));

        DEBUG_LEAVE(TRUE);

        break;

    case DLL_PROCESS_DETACH:

        //
        // signal to all APIs (and any other function that might have an
        // interest) that the DLL is being shutdown
        //

        GlobalDynaUnload = (Reserved == NULL) ? TRUE : FALSE;
        InDllCleanup = TRUE;

        DEBUG_PRINT(DLL,
                    INFO,
                    ("DLL Terminated\n"
                    ));

        DEBUG_LEAVE(TRUE);

        if (GlobalDynaUnload) {
            if (GlobalDataInitialized) {
                GlobalDataTerminate();
            }
            GlobalDllTerminate();
            ExitAutodialModule();
            InternetTerminateThreadInfo();
        }

        CloseInternetSettingsKey();

        PERF_DUMP();

        PERF_END();

        //TERMINATE_DEBUG_MEMORY(FALSE);
        TERMINATE_DEBUG_MEMORY(TRUE);
        INET_DEBUG_FINISH();
        TERMINATE_DEBUG_REGKEY();

        //InternetDestroyThreadInfo();

        DeleteCriticalSection(&GeneralInitCritSec);
        break;

    case DLL_THREAD_DETACH:

        //
        // kill the INTERNET_THREAD_INFO
        //

        DEBUG_LEAVE(TRUE);

        InternetDestroyThreadInfo();
        break;

    case DLL_THREAD_ATTACH:

        //
        // we do nothing for thread attach - if we need an INTERNET_THREAD_INFO
        // then it gets created by the function which realises we need one
        //

        AllowCAP();

        DEBUG_LEAVE(TRUE);

        break;
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {
            hr = pfnri(GlobalDllHandle, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

extern VOID MakeCacheLocationsConsistent();

STDAPI
DllInstall
(
    IN BOOL      bInstall,   // Install or Uninstall
    IN LPCWSTR    pwStr
)
{
    HRESULT hr = S_OK;

// Add entries to selfreg.inx and include the code below to support self-registration.
#ifdef WININET_SELFREG
    BOOL bUseHKLM = FALSE;
    if (pwStr && (0 == StrCmpIW(pwStr, L"HKLM")))
    {
        bUseHKLM = TRUE;
    }

    if ( bInstall )
    {
        hr = CallRegInstall(bUseHKLM ? "Reg.HKLM" : "Reg.HKCU");
    }
    else
    {
        hr = CallRegInstall(bUseHKLM ? "Unreg.HKLM" : "UnReg.HKCU");
    }
#endif

    if( bInstall && (!pwStr || !*pwStr || (StrCmpIW( pwStr, L"HKLM") == 0)))
    {
        // Write out to HKLM\Software\Microsoft\Internet Explorer\Security\Digest
        HKEY hKey;
        DWORD dwError, dwRegDisp, dwFlags;
        dwFlags = FLAGS_DW;

        dwError =  REGCREATEKEYEX(HKEY_LOCAL_MACHINE,
            IE_SECURITY_DIGEST_REG_KEY, 0, NULL,
                0, KEY_READ | KEY_WRITE, NULL, &hKey, &dwRegDisp);
        if (dwError == ERROR_SUCCESS)
        {
            dwError = RegSetValueEx(hKey, FLAGS_SZ, 0,
                REG_BINARY, (LPBYTE) &dwFlags, sizeof(DWORD));
            REGCLOSEKEY(hKey);
        }

#ifndef UNIX
        DWORD dwNSVersion;
        if( GetActiveNetscapeVersion( &dwNSVersion) == FALSE)
            dwNSVersion = 0;
        SetNetscapeImportVersion( dwNSVersion);
#endif // UNIX

        WritePrivateProfileString("compatibility", "NOTIFIER", "0x400000", "win.ini");
        WritePrivateProfileString(NULL, NULL, NULL, "win.ini");
    }
    else if(bInstall && StrCmpIW( pwStr, L"HKCU") == 0)
    {

        MakeCacheLocationsConsistent();

#ifndef UNIX
        TCHAR szNSFilename[MAX_PATH];
        DWORD cNSFilenameSize = MAX_PATH;
        DWORD dwNSVersion;

        if( GetNetscapeImportVersion( &dwNSVersion) == TRUE
            && dwNSVersion != 0
            && FindNetscapeCookieFile( dwNSVersion, szNSFilename, &cNSFilenameSize) == TRUE)
        {
            ImportCookieFile( szNSFilename );
        }
#endif // UNIX

        ie401::Import401History();

        ie401::Import401Content();
    }
    else if (!bInstall && (!pwStr || !*pwStr || (StrCmpIW(pwStr, L"HKLM") == 0)))
    {
        RegDeleteKey(HKEY_LOCAL_MACHINE, IE_SECURITY_DIGEST_REG_KEY);
    }

    return hr;
}
