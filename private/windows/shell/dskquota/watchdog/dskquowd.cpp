///////////////////////////////////////////////////////////////////////////////
/*  File: dskquowd.cpp

    Description: Main entry point for the Windows NT Disk Quota Notification
        WatchDog.  This DLL exports the functions ProcessGPTA and ProcessGPTW
        to be called periodically from WINLOGON.EXE.  When invoked, the
        module enumerates all local and connected volumes, gathers quota
        statistics for each and generates email messages and/or a popup
        dialog as specified by system policy and the volume's quota settings.

        Ideally, the underlying file system could generate notifications
        warning users of approaching a quota limit.  However, within base
        NT, the name used by the user for identifying a drive is unknown.
        Therefore, a message meaningful to the user cannot be generated
        by NTFS.  It must be performed by the client workstation.  Hence,
        this watchdog applet is required.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "precomp.hxx" // PCH
#pragma hdrstop

#define INITGUIDS
#include "dskquota.h"

#include "watchdog.h"


HINSTANCE g_hInstDll;    // DLL instance handle.

//
// Location in HKEY_CURRENT_USER for configuration and state data.
// Names are #define'd in ..\common\private.h
//
TCHAR g_szRegSubKeyAdmin[] = G_SZ_REGSUBKEY_ADMIN;
TCHAR g_szRegSubKeyUser[]  = G_SZ_REGSUBKEY_USER;


///////////////////////////////////////////////////////////////////////////////
/*  Function: OnProcessAttach

    Description: Handles all tasks associated with a process attaching to 
        the DLL.

        Try to keep processing time to a minimum.

    Arguments:
        hInstDll - The DLL instance handle passed to DllMain.

    Returns:
        NOERROR    - Success.
        E_FAIL     - Something failed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
OnProcessAttach(
    HINSTANCE hInstDll
    )
{
    HRESULT hResult = E_FAIL;

    //
    // Start IceCAP profiling.
    //
    ICAP_START_ALL;

#ifdef DEBUG
    // 
    // Default is DM_NONE.
    //
    SetDebugMask(DM_ASSERT | DM_ERROR);
#endif

    g_hInstDll = hInstDll;
    DisableThreadLibraryCalls(hInstDll);

    if (FAILED(g_OleAlloc.Initialize()))
    {
        goto proc_attach_failed;
    }

    hResult = NOERROR;

proc_attach_failed:
        NULL;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: OnProcessDetach

    Description: Handles all tasks associated with a process detaching from 
        the DLL.

    Arguments: None.

    Returns:
        NOERROR    - Success.
        E_FAIL     - Something failed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
OnProcessDetach(
    VOID
    )
{
    HRESULT hResult = NOERROR;

    //
    // Stop IceCAP profiling.
    //
    ICAP_STOP_ALL;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DllMain

    Description: Main entry point for OLE component server.

    Arguments:
        hInstDll - Instance handle of DLL

        fdwReason - Reason DllMain is being called.  Can be at Process attach/
            detach or Thread attach/detach.

        lpdwReserved - Reserved.

    Returns:
        TRUE    - Successful initialization.
        FALSE   - Failed initialization.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI 
DllMain(
    HINSTANCE hInstDll, 
    DWORD fdwReason, 
    LPVOID lpvReserved
    )
{
    BOOL bResult = FALSE;

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DebugMsg(DM_OLE, TEXT("DSKQUOWD - DLL_PROCESS_ATTACH"));
            bResult = SUCCEEDED(OnProcessAttach(hInstDll));
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            bResult = TRUE;
            break;

        case DLL_PROCESS_DETACH:
            DebugMsg(DM_OLE, TEXT("DSKQUOWD - DLL_PROCESS_DETACH"));
            bResult = SUCCEEDED(OnProcessDetach());
            break;
    }

    return bResult;
}

BOOL
ProcessGPT(
    HANDLE hUserToken,          // User's token
    HKEY hKeyCurrentUser,       // Registry key to the root of the user's profile
    LPTSTR pszGPTPath,          // UNC path to the gpt
    BOOL bMachinePolicyOnly,    // Only apply machine policy
    BOOL bBackgroundRefresh,    // This is a background refresh - ok to do slow stuff...
    BOOL bDelete                // Delete policy
    )
{
    BOOL bResult = FALSE; // Assume failure.

    try
    {
        HRESULT hr;

        //
        // Create a watchdog object and tell it to run.
        //
        CWatchDog Hooch(hUserToken);
        hr = Hooch.Run();

        bResult = SUCCEEDED(hr);

        if (FAILED(hr))
        {
            DebugMsg(DM_ERROR, 
                     TEXT("ProcessGPT - CWatchDog::Run failed with error 0x%08X"), hr);
        }
    }
    catch(OutOfMemory)
    {
        //
        // Handle general out-of-memory exceptions.
        //
        DebugMsg(DM_ERROR, TEXT("ProcessGPT - Insufficient memory"));
    }
    catch(CString::Exception& e)
    {
        //
        // The CString class throws it's own exceptions.
        //
        switch(e.Reason())
        {
            case CString::Exception::Index:
                DebugMsg(DM_ERROR, TEXT("ProcessGPT - CString::Exception::Index"));
                break;
            case CString::Exception::Memory:
                DebugMsg(DM_ERROR, TEXT("ProcessGPT - CString::Exception::Memory"));
                break;
            default:
                DebugMsg(DM_ERROR, TEXT("ProcessGPT - CString::Exception::Unknown"));
                break;
        }
    }
    catch(...)
    {
        //
        // Report any general exceptions.
        //
        DebugMsg(DM_ERROR, TEXT("ProcessGPT - Unknown exception"));
    }

    return bResult;
}


BOOL 
ProcessGPTW (
    HANDLE hUserToken,          // User's token
    HKEY hKeyCurrentUser,       // Registry key to the root of the user's profile
    LPWSTR pszGPTPathW,          // UNC path to the gpt
    BOOL bMachinePolicyOnly,    // Only apply machine policy
    BOOL bBackgroundRefresh,    // This is a background refresh - ok to do slow stuff...
    BOOL bDelete                // Delete policy
    )
{
    BOOL bResult      = FALSE;
    LPTSTR pszGPTPath = NULL;
    LPSTR pszGPTPathA = NULL;

    try
    {
#ifndef UNICODE
        //
        // Get the size of the buffer needed to hold pszGPTPathA as a UNICODE str.
        //
        INT cchA = WideCharToMultiByte(CP_ACP,
                                       0,
                                       pszGPTPathW,
                                       -1,
                                       NULL,
                                       0, NULL, NULL);
        //
        // Allocate the ANSI buffer.
        //
        LPSTR pszGPTPathA = new WCHAR[cchA];
        if (NULL != pszGPTPathA)
        {
            //
            // Now convert the UNICODE string to ANSI.
            //
            cchA = MultiByteToWideChar(CP_ACP,
                                       0,
                                       pszGPTPathW,
                                       -1,
                                       pszGPTPathA,
                                       cchA);
        }
        pszGPTPath = pszGPTPathA;
#else
        pszGPTPath = pszGPTPathW;
#endif

        if (NULL != pszGPTPath && TEXT('\0') != *pszGPTPath)
        {
            //
            // Process the GPT using the UNICODE/Ansi-sensitive version.
            // ProcessGPT will trap any exceptions.
            //
            bResult = ProcessGPT(hUserToken,
                                 hKeyCurrentUser,
                                 pszGPTPath,
                                 bMachinePolicyOnly,
                                 bBackgroundRefresh,
                                 bDelete);
        }
    }
    catch(OutOfMemory)
    {
        //
        // Handle general out-of-memory exceptions.
        //
        DebugMsg(DM_ERROR, TEXT("ProcessGPTW - Insufficient memory"));
    }
    catch(...)
    {
        //
        // Report any general exceptions.
        //
        DebugMsg(DM_ERROR, TEXT("ProcessGPTW - Unknown exception"));
    }

    if ((LPWSTR)pszGPTPath != pszGPTPathW)
    {
        //
        // If pszGPTPath isn't pointing to the UNICODE version, it is either
        // pointing to the ANSI version or it's NULL.
        //
        Assert(NULL == pszGPTPath || pszGPTPathA == (LPSTR)pszGPTPath);
        delete[] pszGPTPath;
    }

    return bResult;
}


BOOL 
ProcessGPTA (
    HANDLE hUserToken,          // User's token
    HKEY hKeyCurrentUser,       // Registry key to the root of the user's profile
    LPSTR pszGPTPathA,          // UNC path to the gpt
    BOOL bMachinePolicyOnly,    // Only apply machine policy
    BOOL bBackgroundRefresh,    // This is a background refresh - ok to do slow stuff...
    BOOL bDelete                // Delete policy
    )
{
    BOOL bResult       = FALSE;
    LPTSTR pszGPTPath  = NULL;
    LPWSTR pszGPTPathW = NULL;

    try
    {
#ifdef UNICODE
        //
        // Get the size of the buffer needed to hold pszGPTPathA as a UNICODE str.
        //
        INT cchW = MultiByteToWideChar(CP_ACP,
                                       0,
                                       pszGPTPathA,
                                       -1,
                                       NULL,
                                       0);
        //
        // Allocate the UNICODE buffer.
        //
        LPWSTR pszGPTPathW = new WCHAR[cchW];
        if (NULL != pszGPTPathW)
        {
            //
            // Now convert the ANSI string to UNICODE.
            //
            cchW = MultiByteToWideChar(CP_ACP,
                                       0,
                                       pszGPTPathA,
                                       -1,
                                       pszGPTPathW,
                                       cchW);
        }
        pszGPTPath = pszGPTPathW;
#else
        pszGPTPath = pszGPTPathA;
#endif

        if (NULL != pszGPTPath && TEXT('\0') != *pszGPTPath)
        {
            //
            // Process the GPT using the UNICODE/Ansi-sensitive version.
            // ProcessGPT will trap any exceptions.
            //
            bResult = ProcessGPT(hUserToken,
                                 hKeyCurrentUser,
                                 pszGPTPath,
                                 bMachinePolicyOnly,
                                 bBackgroundRefresh,
                                 bDelete);
        }
    }
    catch(OutOfMemory)
    {
        //
        // Handle general out-of-memory exceptions.
        //
        DebugMsg(DM_ERROR, TEXT("ProcessGPTA - Insufficient memory"));
    }
    catch(...)
    {
        //
        // Report any general exceptions.
        //
        DebugMsg(DM_ERROR, TEXT("ProcessGPTA - Unknown exception"));
    }

    if ((LPSTR)pszGPTPath != pszGPTPathA)
    {
        //
        // If pszGPTPath isn't pointing to the ANSI version, it is either
        // pointing to the UNICODE version or it's NULL.
        //
        Assert(NULL == pszGPTPath || pszGPTPathW == (LPWSTR)pszGPTPath);
        delete[] pszGPTPath;
    }

    return bResult;
}


