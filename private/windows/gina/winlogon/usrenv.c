/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles logoff dialog.
*
* History:
* 2-25-92 JohanneC       Created -
*
\***************************************************************************/

#include "precomp.h"
#include "shlobj.h"
#pragma hdrstop

BOOL
SetGINAEnvVars (
    PWINDOWSTATION pWS
    );



BOOL
SetupBasicEnvironment(
    PVOID * ppEnv
    )
{
    HKEY hKey;
    DWORD dwSize, dwType, dwExpandSize;
    TCHAR szValue[1025];
    TCHAR szExpValue[1025];
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH+1;

    if (GetComputerName (szComputerName, &dwComputerNameSize)) {
        SetUserEnvironmentVariable(ppEnv, COMPUTERNAME_VARIABLE, (LPTSTR) szComputerName, TRUE);
    }

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("ProgramFilesDir"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {
            dwExpandSize = ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue) );

            if ( dwExpandSize && dwExpandSize < ARRAYSIZE(szExpValue) ) {
                SetUserEnvironmentVariable(ppEnv, PROGRAMFILES_VARIABLE, (LPTSTR) szExpValue, TRUE);
                SetEnvironmentVariable(PROGRAMFILES_VARIABLE,(LPTSTR)szExpValue);
            }
        }

        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("CommonFilesDir"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            dwExpandSize = ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue) );

            if ( dwExpandSize && dwExpandSize < ARRAYSIZE(szExpValue) ) {
                SetUserEnvironmentVariable(ppEnv, COMMONPROGRAMFILES_VARIABLE, (LPTSTR) szExpValue, TRUE);
                SetEnvironmentVariable(COMMONPROGRAMFILES_VARIABLE,(LPTSTR)szExpValue);
            }
        }

#if defined(WX86) || defined(_AXP64_)
        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("ProgramFilesDir (x86)"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            dwExpandSize = ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue) );

            if ( dwExpandSize && dwExpandSize < ARRAYSIZE(szExpValue) ) {
                SetUserEnvironmentVariable(ppEnv, PROGRAMFILESX86_VARIABLE, (LPTSTR) szExpValue, TRUE);
            }
        }

        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("CommonFilesDir (x86)"), NULL, &dwType,
                            (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            dwExpandSize = ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue) );

            if ( dwExpandSize && dwExpandSize < ARRAYSIZE(szExpValue) ) {
                SetUserEnvironmentVariable(ppEnv, COMMONPROGRAMFILESX86_VARIABLE, (LPTSTR) szExpValue, TRUE);
                SetEnvironmentVariable(COMMONPROGRAMFILESX86_VARIABLE,(LPTSTR)szExpValue);
            }
        }


#endif

        RegCloseKey (hKey);
    }


    return(TRUE);
}

/***************************************************************************\
* SetupUserEnvironment
*
* Initializes all system and user environment variables, retrieves the user's
* profile, sets current directory...
*
* Returns TRUE on success, FALSE on failure.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetupUserEnvironment(
    PTERMINAL pTerm
    )
{
    PVOID pEnv = NULL;
    TCHAR lpHomeShare[MAX_PATH] = TEXT("");
    TCHAR lpHomePath[MAX_PATH] = TEXT("");
    TCHAR lpHomeDrive[4] = TEXT("");
    TCHAR lpHomeDirectory[MAX_PATH] = TEXT("");
    TCHAR lpUserProfile[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;
    NTSTATUS Status;


    //
    // Status message will appear in RestoreUserProfile
    //

    // StatusMessage(FALSE, IDS_STATUS_LOAD_PROFILE);


    /*
     * Create a new environment for the user.
     */

    if (!CreateUserEnvironment(&pEnv))
    {
        DebugLog((DEB_ERROR, "CreateUserEnvironment failed"));
        return(FALSE);
    }

    SetupBasicEnvironment(&pEnv);

    /*
     * Initialize user's environment.
     */

    SetHomeDirectoryEnvVars(&pEnv, lpHomeDirectory,
                            lpHomeDrive, lpHomeShare, lpHomePath);

    //
    // Load the user's profile into the registry
    //

    Status = RestoreUserProfile(pTerm);
    if (Status != STATUS_SUCCESS) {
        DebugLog((DEB_ERROR, "restoring the user profile failed"));
        return(FALSE);
    }
    //
    // Set USERPROFILE environment variable
    //

    if (GetUserProfileDirectory (pWS->UserProcessData.UserToken,
                                 lpUserProfile, &dwSize)) {

        SetUserEnvironmentVariable(&pEnv, USERPROFILE_VARIABLE, lpUserProfile, TRUE);
    }

    pWS->UserProcessData.pEnvironment = pEnv;

    if (pWS->UserProcessData.CurrentDirectory = (LPTSTR)Alloc(
                          sizeof(TCHAR)*(lstrlen(lpHomeDirectory)+1)))
        lstrcpy(pWS->UserProcessData.CurrentDirectory, lpHomeDirectory);

    //
    // Set the GINA environment variables in the registry
    //

    SetGINAEnvVars (pWS);

    /*
     * Set all windows controls to be the user's settings.
     */
    InitSystemParametersInfo(pTerm, TRUE);

    return(TRUE);

}

/***************************************************************************\
* ResetEnvironment
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
VOID
ResetEnvironment(
    PTERMINAL pTerm
    )
{
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;

    //
    // If they were logged on as system, all these values will be NULL
    //

    if (pWS->UserProcessData.CurrentDirectory) {
        Free(pWS->UserProcessData.CurrentDirectory);
        pWS->UserProcessData.CurrentDirectory = NULL;
    }
    if (pWS->UserProcessData.pEnvironment) {
        RtlDestroyEnvironment(pWS->UserProcessData.pEnvironment);
        pWS->UserProcessData.pEnvironment = NULL;
    }
    if (pWS->UserProfile.ProfilePath) {
        Free(pWS->UserProfile.ProfilePath);
        pWS->UserProfile.ProfilePath = NULL;
    }
    if (pWS->UserProfile.PolicyPath) {
        Free(pWS->UserProfile.PolicyPath);
        pWS->UserProfile.PolicyPath = NULL;
    }
    if (pWS->UserProfile.NetworkDefaultUserProfile) {
        Free(pWS->UserProfile.NetworkDefaultUserProfile);
        pWS->UserProfile.NetworkDefaultUserProfile = NULL;
    }
    if (pWS->UserProfile.ServerName) {
        Free(pWS->UserProfile.ServerName);
        pWS->UserProfile.ServerName = NULL;
    }
    if (pWS->UserProfile.Environment) {
        Free(pWS->UserProfile.Environment);
        pWS->UserProfile.Environment = NULL;
    }

    pWS->UserProfile.hGPOEvent = NULL;
    pWS->UserProfile.hGPOThread = NULL;
    pWS->UserProfile.hGPONotifyEvent = NULL;
    pWS->UserProfile.hGPOWaitEvent = NULL;
    pWS->UserProfile.dwSysPolicyFlags = 0;

    //
    // Reset all windows controls to be the default settings
    //
    InitSystemParametersInfo(pTerm, FALSE);
}

/***************************************************************************\
* OpenHKeyCurrentUser
*
* Opens HKeyCurrentUser to point at the current logged on user's profile.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 06-16-92  Davidc  Created
*
\***************************************************************************/
BOOL
OpenHKeyCurrentUser(
    PWINDOWSTATION pWS
    )
{
    DWORD err;
    HANDLE ImpersonationHandle;
    BOOL Result;
    NTSTATUS Status ;


    //
    // Get in the correct context before we reference the registry
    //

    EnterCriticalSection( &pWS->UserProcessData.Lock );

    if ( pWS->UserProcessData.Ref )
    {
        pWS->UserProcessData.Ref++ ;

        LeaveCriticalSection( &pWS->UserProcessData.Lock );

        return TRUE ;
    }

    ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {

        DebugLog((DEB_ERROR, "OpenHKeyCurrentUser failed to impersonate user"));

        LeaveCriticalSection( &pWS->UserProcessData.Lock );

        return(FALSE);
    }

    Status = RtlOpenCurrentUser(
                    MAXIMUM_ALLOWED,
                    &pWS->UserProcessData.hCurrentUser );

    if ( NT_SUCCESS( Status ) )
    {
        pWS->UserProcessData.Ref++ ;
    }

    //
    // Return to our own context
    //

    Result = StopImpersonating(ImpersonationHandle);
    ASSERT(Result);

    LeaveCriticalSection( &pWS->UserProcessData.Lock );

    return NT_SUCCESS( Status );
}



/***************************************************************************\
* CloseHKeyCurrentUser
*
* Closes key for user's hive.
*
* Returns nothing
*
* History:
* 06-16-92  Davidc  Created
*
\***************************************************************************/
VOID
CloseHKeyCurrentUser(
    PWINDOWSTATION pWS
    )
{
    DWORD err;

    EnterCriticalSection( &pWS->UserProcessData.Lock );

    pWS->UserProcessData.Ref-- ;

    if ( pWS->UserProcessData.Ref == 0 )
    {
        err = RegCloseKey( pWS->UserProcessData.hCurrentUser );
        pWS->UserProcessData.hCurrentUser = NULL ;
        ASSERT(err == ERROR_SUCCESS);
    }

    LeaveCriticalSection( &pWS->UserProcessData.Lock );


}


/***************************************************************************\
* FUNCTION: SetEnvironmentULong
*
* PURPOSE:  Sets the value of an environment variable to the string
*           representation of the passed data.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetEnvironmentULong(
    PVOID * Env,
    LPTSTR Variable,
    ULONG_PTR Value
    )
{
    WCHAR Buffer[16];
    int Result;
    NTSTATUS Status ;
    UNICODE_STRING Var ;
    UNICODE_STRING Val ;

#if defined(_WIN64)

    if ((Value >> 32) != 0) {
        Result = _snwprintf(Buffer,
                            sizeof(Buffer)/sizeof(TCHAR),
                            TEXT("%x"),
                            (ULONG)(Value >> 32));

        Result += _snwprintf(Buffer + Result/sizeof(TCHAR),
                             (sizeof(Buffer) - Result)/sizeof(TCHAR),
                             TEXT("%08x"),
                             (ULONG)Value);

    } else {
        Result = _snwprintf(Buffer,
                            sizeof(Buffer)/sizeof(TCHAR),
                            TEXT("%x"),
                            (ULONG)Value);
    }

#else

    Result = _snwprintf(Buffer, sizeof(Buffer)/sizeof(TCHAR), TEXT("%x"), Value);

#endif

    ASSERT(Result < sizeof(Buffer));


    RtlInitUnicodeString( &Val, Buffer );
    RtlInitUnicodeString( &Var, Variable );
    Status = RtlSetEnvironmentVariable(
                    Env,
                    &Var,
                    &Val );

    return NT_SUCCESS( Status );
}


/***************************************************************************\
* FUNCTION: SetEnvironmentLargeInt
*
* PURPOSE:  Sets the value of an environment variable to the string
*           representation of the passed data.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetEnvironmentLargeInt(
    PVOID * Env,
    LPTSTR Variable,
    LARGE_INTEGER Value
    )
{
    TCHAR Buffer[20];
    int Result;
    UNICODE_STRING Var ;
    UNICODE_STRING Val ;
    NTSTATUS Status ;

    Result = _snwprintf(Buffer, sizeof(Buffer)/sizeof(TCHAR), TEXT("%x:%x"), Value.HighPart, Value.LowPart);
    ASSERT(Result < sizeof(Buffer));

    RtlInitUnicodeString( &Var, Variable );
    RtlInitUnicodeString( &Val, Buffer );
    Status = RtlSetEnvironmentVariable(
                Env,
                &Var,
                &Val );

    return NT_SUCCESS( Status );
}

/***************************************************************************\
* FUNCTION: SetGINAEnvVars
*
* PURPOSE:  Sets the environment variables GINA passed back to the
*           volatile environment in the user's profile
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   09-26-95 EricFlo       Created.
*
\***************************************************************************/

#define MAX_VALUE_LEN  1024

BOOL
SetGINAEnvVars (
    PWINDOWSTATION pWS
    )
{

    TCHAR szValueName[MAX_VALUE_LEN + 1];
    TCHAR szValue[MAX_VALUE_LEN + 1];
    LPTSTR lpEnv = pWS->UserProfile.Environment;
    LPTSTR lpEnd, lpBegin, lpTemp;
    HKEY hKey = NULL;
    DWORD dwDisp;
    BOOL bRetVal = FALSE;
    HINSTANCE hinstShell32 = NULL;
    DWORD cPercent, len, i;



    if (!lpEnv) {
        return TRUE;
    }


    if (!OpenHKeyCurrentUser(pWS)) {
        DebugLog((DEB_ERROR, "SetGINAEnvVars: Failed to open HKeyCurrentUser"));
        return FALSE;
    }


    if (RegCreateKeyEx(pWS->UserProcessData.hCurrentUser,
                       TEXT("Volatile Environment"),
                       0,
                       NULL,
                       REG_OPTION_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       &dwDisp) != ERROR_SUCCESS) {
        goto Exit;
    }



    lpEnd = lpBegin = lpEnv;

    for (;;) {

       //
       // Skip leading blanks
       //

       while (*lpEnd == TEXT(' ')) {
            lpEnd++;
       }

       lpBegin = lpEnd;



       //
       // Search for the = sign
       //

       while (*lpEnd && *lpEnd != TEXT('=')) {
           lpEnd++;
       }

       if (!*lpEnd) {
          goto Exit;
       }

       //
       // Null terminate and copy to value name buffer
       //

       *lpEnd = TEXT('\0');

       if (lstrlen(lpBegin) + 1 > MAX_VALUE_LEN) {
           goto Exit;
       }

       lstrcpy (szValueName, lpBegin);


       *lpEnd++ = TEXT('=');


       //
       // Trim off any trailing spaces
       //

       lpTemp = szValueName + (lstrlen (szValueName) - 1);

       while (*lpTemp && (*lpTemp == TEXT(' ')) ) {
           lpTemp--;
       }

       lpTemp++;
       *lpTemp = TEXT('\0');



       //
       // Skip leading blanks before value data
       //

       while (*lpEnd == TEXT(' ')) {
            lpEnd++;
       }

       lpBegin = lpEnd;


       //
       // Search for the null terminator
       //

       while (*lpEnd) {
           lpEnd++;
       }

       if (lstrlen(lpBegin) + 1 > MAX_VALUE_LEN) {
           goto Exit;
       }

       lstrcpy (szValue, lpBegin);


       //
       // Trim off any trailing spaces
       //

       lpTemp = szValue + (lstrlen (szValue) - 1);

       while (*lpTemp && (*lpTemp == TEXT(' ')) ) {
           lpTemp--;
       }

       lpTemp++;
       *lpTemp = TEXT('\0');



       //
       // Scan the value data to see if a 2 % signs exist.
       // If so, then this is an expand_sz type.
       //

       cPercent = 0;
       len = lstrlen (szValue);

       for (i = 0; i < len; i++) {
           if (szValue[i] == TEXT('%')) {
               cPercent++;
           }
       }


       //
       // Set it in the user profile
       //

       RegSetValueEx (hKey,
                      szValueName,
                      0,
                      (cPercent >= 2) ? REG_EXPAND_SZ : REG_SZ,
                      (LPBYTE) szValue,
                      (len + 1) * sizeof(TCHAR));

       lpEnd++;

       if (!*lpEnd) {
           break;
       }

       lpBegin = lpEnd;
    }

    //
    // Set the SESSIONNAME environment variable in the user profile
    //
    if (g_IsTerminalServer && gpfnWinStationNameFromSessionId) {

        if (gpfnWinStationNameFromSessionId(SERVERNAME_CURRENT, g_SessionId, szValue)) {

            RegSetValueEx (hKey,
                           SESSIONNAME_VARIABLE,
                           0,
                           REG_SZ,
                           (LPBYTE) szValue,
                           (lstrlen(szValue) + 1) * sizeof(TCHAR));
        }
    }

    //
    // Set the APPDATA variable for the current user.
    //

    hinstShell32 = LoadLibraryW(L"shell32.dll");
    if ( hinstShell32 )
    {
        typedef HRESULT (__stdcall * PFNSHGETFOLDERPATHW)(HWND, int, HANDLE, DWORD, LPWSTR);

        PFNSHGETFOLDERPATHW pfnSHGetFolderPath;

        pfnSHGetFolderPath = (PFNSHGETFOLDERPATHW)GetProcAddress(hinstShell32, "SHGetFolderPathW");
        if ( pfnSHGetFolderPath &&
                SUCCEEDED(pfnSHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_DONT_VERIFY, pWS->hToken, 0, szValue)) )
        {
            RegSetValueEx (hKey,
                           APPDATA_VARAIBLE,
                           0,
                           REG_SZ,
                           (LPBYTE)szValue,
                           (lstrlen(szValue) + 1) * sizeof(TCHAR));
        }

        FreeLibrary(hinstShell32);
    }


    bRetVal = TRUE;

Exit:

    if (hKey) {
        RegCloseKey (hKey);
    }

    CloseHKeyCurrentUser(pWS);

    return bRetVal;

}
