//*************************************************************
//  File name: envvar.c
//
//  Description:  Contains the environment variable functions
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

//
// Max environment variable length
//

#define MAX_VALUE_LEN              1024

//
// Environment variables
//

#define COMPUTERNAME_VARIABLE      TEXT("COMPUTERNAME")
#define HOMEDRIVE_VARIABLE         TEXT("HOMEDRIVE")
#define HOMESHARE_VARIABLE         TEXT("HOMESHARE")
#define HOMEPATH_VARIABLE          TEXT("HOMEPATH")
#define SYSTEMDRIVE_VARIABLE       TEXT("SystemDrive")
#define SYSTEMROOT_VARIABLE        TEXT("SystemRoot")
#define USERNAME_VARIABLE          TEXT("USERNAME")
#define USERDOMAIN_VARIABLE        TEXT("USERDOMAIN")
#define USERDNSDOMAIN_VARIABLE     TEXT("USERDNSDOMAIN")
#define USERPROFILE_VARIABLE       TEXT("USERPROFILE")
#define ALLUSERSPROFILE_VARIABLE   TEXT("ALLUSERSPROFILE")
#define PATH_VARIABLE              TEXT("Path")
#define LIBPATH_VARIABLE           TEXT("LibPath")
#define OS2LIBPATH_VARIABLE        TEXT("Os2LibPath")
#define PROGRAMFILES_VARIABLE      TEXT("ProgramFiles")
#define COMMONPROGRAMFILES_VARIABLE TEXT("CommonProgramFiles")
#ifdef WX86
#define PROGRAMFILESX86_VARIABLE   TEXT("ProgramFiles(x86)")
#define COMMONPROGRAMFILESX86_VARIABLE TEXT("CommonProgramFiles(x86)")
#endif
#define USER_ENV_SUBKEY            TEXT("Environment")
#define USER_VOLATILE_ENV_SUBKEY   TEXT("Volatile Environment")

//
// Parsing information for autoexec.bat
//
#define AUTOEXECPATH_VARIABLE      TEXT("AutoexecPath")
#define PARSE_AUTOEXEC_KEY         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define PARSE_AUTOEXEC_ENTRY       TEXT("ParseAutoexec")
#define PARSE_AUTOEXEC_DEFAULT     TEXT("1")
#define MAX_PARSE_AUTOEXEC_BUFFER  2


#define SYS_ENVVARS                TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Environment")

BOOL UpdateSystemEnvironment(PVOID *pEnv);
BOOL SetEnvironmentVariableInBlock(PVOID *pEnv, LPTSTR lpVariable, LPTSTR lpValue, BOOL bOverwrite);
BOOL GetUserNameAndDomain(HANDLE hToken, LPTSTR *UserName, LPTSTR *UserDomain);
BOOL GetUserNameAndDomainSlowly(HANDLE hToken, LPTSTR *UserName, LPTSTR *UserDomain);
LPTSTR GetUserDNSDomainName(LPTSTR lpDomain);
LONG GetHKeyCU(HANDLE hToken, HKEY *hKeyCU);
BOOL ProcessAutoexec(PVOID *pEnv);
BOOL AppendNTPathWithAutoexecPath(PVOID *pEnv, LPTSTR lpPathVariable, LPTSTR lpAutoexecPath);
BOOL SetEnvironmentVariables(PVOID *pEnv, LPTSTR lpRegSubKey, HKEY hKeyCU);
#ifdef _X86_
BOOL IsPathIncludeRemovable(LPTSTR lpValue);
#endif


//*************************************************************
//
//  CreateEnvironmentBlock()
//
//  Purpose:    Creates the environment variables for the
//              specificed hToken.  If hToken is NULL, the
//              environment block will only contain system
//              variables.
//
//  Parameters: pEnv            -   Receives the environment block
//              hToken          -   User's token or NULL
//              bInherit        -   Inherit the current process environment
//
//  Return:     TRUE if successful
//              FALSE if not
//
//  Comments:   The pEnv value must be destroyed by
//              calling DestroyEnvironmentBlock
//
//  History:    Date        Author     Comment
//              6/19/96     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateEnvironmentBlock (LPVOID *pEnv, HANDLE  hToken, BOOL bInherit)
{
    TCHAR szBuffer[MAX_PATH+1];
    TCHAR szValue[1025];
    TCHAR szExpValue[1025];
    DWORD dwBufferSize = MAX_PATH+1;
    NTSTATUS Status;
    LPTSTR UserName = NULL;
    LPTSTR UserDomain = NULL;
    LPTSTR UserDNSDomain = NULL;
    HKEY  hKey, hKeyCU;
    DWORD dwDisp, dwType, dwSize;
    TCHAR szParseAutoexec[MAX_PARSE_AUTOEXEC_BUFFER];
    LONG  dwError;


    //
    // Arg check
    //

    if (!pEnv) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = RtlCreateEnvironment((BOOLEAN)bInherit, pEnv);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // First start by getting the systemroot and systemdrive values and
    // setting it in the new environment.
    //

    if ( GetEnvironmentVariable(SYSTEMROOT_VARIABLE, szBuffer, dwBufferSize) )
    {
        SetEnvironmentVariableInBlock(pEnv, SYSTEMROOT_VARIABLE, szBuffer, TRUE);
    }

    if ( GetEnvironmentVariable(SYSTEMDRIVE_VARIABLE, szBuffer, dwBufferSize) )
    {
        SetEnvironmentVariableInBlock(pEnv, SYSTEMDRIVE_VARIABLE, szBuffer, TRUE);
    }


    //
    // Set the all users profile location.
    //

    dwBufferSize = ARRAYSIZE(szBuffer);
    if (GetAllUsersProfileDirectory(szBuffer, &dwBufferSize)) {
        SetEnvironmentVariableInBlock(pEnv, ALLUSERSPROFILE_VARIABLE, szBuffer, TRUE);
    }


    //
    // We must examine the registry directly to suck out
    // the system environment variables, because they
    // may have changed since the system was booted.
    //

    if (!UpdateSystemEnvironment(pEnv)) {
        RtlDestroyEnvironment(*pEnv);
        return FALSE;
    }


    //
    // Set the computername
    //

    dwBufferSize = ARRAYSIZE(szBuffer);
    if (GetComputerName (szBuffer, &dwBufferSize)) {
        SetEnvironmentVariableInBlock(pEnv, COMPUTERNAME_VARIABLE, szBuffer, TRUE);
    }


    //
    // Set the default user profile location
    //

    dwSize = ARRAYSIZE(szBuffer);
    if (GetDefaultUserProfileDirectory(szBuffer, &dwSize)) {
        SetEnvironmentVariableInBlock(pEnv, USERPROFILE_VARIABLE, szBuffer, TRUE);
    }


    //
    // Set the Program Files environment variable
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("ProgramFilesDir"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetEnvironmentVariableInBlock(pEnv, PROGRAMFILES_VARIABLE, szExpValue, TRUE);
        }

        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("CommonFilesDir"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetEnvironmentVariableInBlock(pEnv, COMMONPROGRAMFILES_VARIABLE, szExpValue, TRUE);
        }

#ifdef WX86
        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("ProgramFilesDir (x86)"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetEnvironmentVariableInBlock(pEnv, PROGRAMFILESX86_VARIABLE, szExpValue, TRUE);
        }

        dwSize = sizeof(szValue);
        if (RegQueryValueEx (hKey, TEXT("CommonFilesDir (x86)"), NULL, &dwType,
                             (LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) {

            ExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetEnvironmentVariableInBlock(pEnv, PROGRAMFILESX86_VARIABLE, szExpValue, TRUE);
        }
#endif

        RegCloseKey (hKey);
    }


    //
    // If hToken is NULL, we can exit now since the caller only wants
    // system environment variables.
    //

    if (!hToken) {
        return TRUE;
    }


    //
    // Open the HKEY_CURRENT_USER for this token.
    //

    dwError = GetHKeyCU(hToken, &hKeyCU);

    //
    // if the hive is not found assume that the caller just needs the system attribute.
    //

    if ((!hKeyCU) && (dwError == ERROR_FILE_NOT_FOUND)) {
        return TRUE;
    }

    if (!hKeyCU) {
        RtlDestroyEnvironment(*pEnv);
        DebugMsg((DM_WARNING, TEXT("CreateEnvironmentBlock:  Failed to open HKEY_CURRENT_USER, error = %d"), dwError));
        return FALSE;
    }


    //
    // Set the user's name and domain.
    //

    if (!GetUserNameAndDomain(hToken, &UserName, &UserDomain)) {
        GetUserNameAndDomainSlowly(hToken, &UserName, &UserDomain);
    }
    UserDNSDomain = GetUserDNSDomainName(UserDomain);
    SetEnvironmentVariableInBlock( pEnv, USERNAME_VARIABLE, UserName, TRUE);
    SetEnvironmentVariableInBlock( pEnv, USERDOMAIN_VARIABLE, UserDomain, TRUE);
    SetEnvironmentVariableInBlock( pEnv, USERDNSDOMAIN_VARIABLE, UserDNSDomain, TRUE);
    LocalFree(UserName);
    LocalFree(UserDomain);
    LocalFree(UserDNSDomain);


    //
    // Set the user's profile location.
    //

    dwBufferSize = ARRAYSIZE(szBuffer);
    if (GetUserProfileDirectory(hToken, szBuffer, &dwBufferSize)) {
        SetEnvironmentVariableInBlock(pEnv, USERPROFILE_VARIABLE, szBuffer, TRUE);
    }


    //
    // Process autoexec.bat
    //

    lstrcpy (szParseAutoexec, PARSE_AUTOEXEC_DEFAULT);

    if (RegCreateKeyEx (hKeyCU, PARSE_AUTOEXEC_KEY, 0, 0,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {


        //
        // Query the current value.  If it doesn't exist, then add
        // the entry for next time.
        //

        dwBufferSize = sizeof (TCHAR) * MAX_PARSE_AUTOEXEC_BUFFER;
        if (RegQueryValueEx (hKey, PARSE_AUTOEXEC_ENTRY, NULL, &dwType,
                        (LPBYTE) szParseAutoexec, &dwBufferSize)
                         != ERROR_SUCCESS) {

            //
            // Set the default value
            //

            RegSetValueEx (hKey, PARSE_AUTOEXEC_ENTRY, 0, REG_SZ,
                           (LPBYTE) szParseAutoexec,
                           sizeof (TCHAR) * lstrlen (szParseAutoexec) + 1);
        }

        //
        // Close key
        //

        RegCloseKey (hKey);
     }


    //
    // Process autoexec if appropriate
    //

    if (szParseAutoexec[0] == TEXT('1')) {
        ProcessAutoexec(pEnv);
    }


    //
    // Set User environment variables.
    //
    SetEnvironmentVariables(pEnv, USER_ENV_SUBKEY, hKeyCU);


    //
    // Set User volatile environment variables.
    //
    SetEnvironmentVariables(pEnv, USER_VOLATILE_ENV_SUBKEY, hKeyCU);


    //
    // Merge the paths
    //

    AppendNTPathWithAutoexecPath(pEnv, PATH_VARIABLE, AUTOEXECPATH_VARIABLE);


    RegCloseKey (hKeyCU);

    return TRUE;
}


//*************************************************************
//
//  DestroyEnvironmentBlock()
//
//  Purpose:    Frees the environment block created by
//              CreateEnvironmentBlock
//
//  Parameters: lpEnvironment   -   Pointer to variables
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/96     ericflo    Created
//
//*************************************************************

BOOL WINAPI DestroyEnvironmentBlock (LPVOID lpEnvironment)
{

    if (!lpEnvironment) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlDestroyEnvironment(lpEnvironment);

    return TRUE;
}


//*************************************************************
//
//  UpdateSystemEnvironment()
//
//  Purpose:    Reads the system environment variables from the
//              registry.
//
//  Parameters: pEnv    -   Environment block pointer
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL UpdateSystemEnvironment(PVOID *pEnv)
{

    HKEY KeyHandle;
    DWORD Result;
    DWORD ValueNameLength;
    DWORD Type;
    DWORD DataLength;
    DWORD cValues;          /* address of buffer for number of value identifiers    */
    DWORD chMaxValueName;   /* address of buffer for longest value name length      */
    DWORD cbMaxValueData;   /* address of buffer for longest value data length      */
    DWORD junk;
    FILETIME FileTime;
    PTCHAR ValueName;
    PTCHAR  ValueData;
    DWORD i;
    BOOL Bool;
    PTCHAR ExpandedValue;
    BOOL rc = TRUE;

    DWORD ClassStringSize = MAX_PATH + 1;
    TCHAR Class[MAX_PATH + 1];

    Result = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 SYS_ENVVARS,
                 0,
                 KEY_QUERY_VALUE,
                 &KeyHandle
                 );

    if ( Result != ERROR_SUCCESS ) {

        DebugMsg((DM_WARNING, TEXT("UpdateSystemEnvironment:  RegOpenKeyEx failed, error = %d"),Result));
        return( TRUE );
    }

    Result = RegQueryInfoKey(
                 KeyHandle,
                 Class,              /* address of buffer for class string */
                 &ClassStringSize,   /* address of size of class string buffer */
                 NULL,               /* reserved */
                 &junk,              /* address of buffer for number of subkeys */
                 &junk,              /* address of buffer for longest subkey */
                 &junk,              /* address of buffer for longest class string length */
                 &cValues,           /* address of buffer for number of value identifiers */
                 &chMaxValueName,    /* address of buffer for longest value name length */
                 &cbMaxValueData,    /* address of buffer for longest value data length */
                 &junk,              /* address of buffer for descriptor length */
                 &FileTime           /* address of buffer for last write time */
                 );

    if ( Result != NO_ERROR && Result != ERROR_MORE_DATA ) {
        DebugMsg((DM_WARNING, TEXT("UpdateSystemEnvironment:  RegQueryInfoKey failed, error = %d"),Result));
        RegCloseKey(KeyHandle);
        return( TRUE );
    }

    //
    // No need to adjust the datalength for TCHAR issues
    //

    ValueData = LocalAlloc(LPTR, cbMaxValueData);

    if ( ValueData == NULL ) {
        RegCloseKey(KeyHandle);
        return( FALSE );
    }

    //
    // The maximum value name length comes back in characters, convert to bytes
    // before allocating storage.  Allow for trailing NULL also.
    //

    ValueName = LocalAlloc(LPTR, (++chMaxValueName) * sizeof( TCHAR ) );

    if ( ValueName == NULL ) {

        RegCloseKey(KeyHandle);
        LocalFree( ValueData );
        return( FALSE );
    }

    //
    // To exit from here on, set rc and jump to Cleanup
    //

    for (i=0; i<cValues ; i++) {

        ValueNameLength = chMaxValueName;
        DataLength      = cbMaxValueData;

        Result = RegEnumValue (
                     KeyHandle,
                     i,
                     ValueName,
                     &ValueNameLength,    // Size in TCHARs
                     NULL,
                     &Type,
                     (LPBYTE)ValueData,
                     &DataLength          // Size in bytes
                     );

        if ( Result != ERROR_SUCCESS ) {

            //
            // Problem getting the value.  We can either try
            // the rest or punt completely.
            //

            goto Cleanup;
        }

        //
        // If the buffer size is greater than the max allowed,
        // terminate the string at MAX_VALUE_LEN - 1.
        //

        if (DataLength >= (MAX_VALUE_LEN * sizeof(TCHAR))) {
            ValueData[MAX_VALUE_LEN-1] = TEXT('\0');
        }

        switch ( Type ) {
            case REG_SZ:
                {

                    Bool = SetEnvironmentVariableInBlock(
                               pEnv,
                               ValueName,
                               ValueData,
                               TRUE
                               );

                    if ( !Bool ) {
                        DebugMsg((DM_WARNING, TEXT("UpdateSystemEnvironment: Failed to set environment variable <%s> to <%s> with %d."),
                                 ValueName, ValueData, GetLastError()));
                    }

                    break;
                }
            default:
                {
                    continue;
                }
        }
    }

    //
    // To exit from here on, set rc and jump to Cleanup
    //

    for (i=0; i<cValues ; i++) {

        ValueNameLength = chMaxValueName;
        DataLength      = cbMaxValueData;

        Result = RegEnumValue (
                     KeyHandle,
                     i,
                     ValueName,
                     &ValueNameLength,    // Size in TCHARs
                     NULL,
                     &Type,
                     (LPBYTE)ValueData,
                     &DataLength          // Size in bytes
                     );

        if ( Result != ERROR_SUCCESS ) {

            //
            // Problem getting the value.  We can either try
            // the rest or punt completely.
            //

            goto Cleanup;
        }

        //
        // If the buffer size is greater than the max allowed,
        // terminate the string at MAX_VALUE_LEN - 1.
        //

        if (DataLength >= (MAX_VALUE_LEN * sizeof(TCHAR))) {
            ValueData[MAX_VALUE_LEN-1] = TEXT('\0');
        }

        switch ( Type ) {
            case REG_EXPAND_SZ:
                {

                    ExpandedValue =  AllocAndExpandEnvironmentStrings( ValueData );

                    Bool = SetEnvironmentVariableInBlock(
                               pEnv,
                               ValueName,
                               ExpandedValue,
                               TRUE
                               );

                    LocalFree( ExpandedValue );

                    if ( !Bool ) {
                        DebugMsg((DM_WARNING, TEXT("UpdateSystemEnvironment: Failed to set environment variable <%s> to <%s> with %d."),
                                 ValueName, ValueData, GetLastError()));
                    }

                    break;
                }
            default:
                {
                    continue;
                }
        }
    }


Cleanup:

    RegCloseKey(KeyHandle);

    LocalFree( ValueName );
    LocalFree( ValueData );

    return( rc );
}

//*************************************************************
//
//  SetEnvironmentVariableInBlock()
//
//  Purpose:    Sets the environment variable in the given block
//
//  Parameters: pEnv        -   Environment block
//              lpVariable  -   Variables
//              lpValue     -   Value
//              bOverwrite  -   Overwrite
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL SetEnvironmentVariableInBlock(PVOID *pEnv, LPTSTR lpVariable,
                                   LPTSTR lpValue, BOOL bOverwrite)
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    DWORD cb;
    DWORD cchValue = 1024;
    TCHAR szValue[1024];

    if (!*pEnv || !lpVariable || !*lpVariable) {
        return(FALSE);
    }

    RtlInitUnicodeString(&Name, lpVariable);

    cb = 1025 * sizeof(WCHAR);
    Value.Buffer = LocalAlloc(LPTR, cb);
    if (Value.Buffer) {
        Value.Length = 0;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);

        LocalFree(Value.Buffer);

        if ( NT_SUCCESS(Status) && !bOverwrite) {
            return(TRUE);
        }
    }

    if (lpValue && *lpValue) {

        //
        // Special case TEMP and TMP and shorten the path names
        //

        if ((!lstrcmpi(lpVariable, TEXT("TEMP"))) ||
            (!lstrcmpi(lpVariable, TEXT("TMP")))) {

             if (!GetShortPathName (lpValue, szValue, 1024)) {
                 lstrcpyn (szValue, lpValue, 1024);
             }
        } else {
            lstrcpyn (szValue, lpValue, 1024);
        }

        RtlInitUnicodeString(&Value, szValue);
        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    else {
        Status = RtlSetEnvironmentVariable(pEnv, &Name, NULL);
    }
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}

//*************************************************************
//
//  IsUNCPath()
//
//  Purpose:    Is the given path a UNC path
//
//  Parameters: lpPath  -   Path to check
//
//  Return:     TRUE if the path is UNC
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL IsUNCPath(LPTSTR lpPath)
{
    if (lpPath[0] == TEXT('\\') && lpPath[1] == TEXT('\\')) {
        return(TRUE);
    }
    return(FALSE);
}

//*************************************************************
//
//  GetUserNameAndDomain()
//
//  Purpose:    Gets the user's name and domain
//
//  Parameters: hToken      -   User's token
//              UserName    -   Receives pointer to user's name
//              UserDomain  -   Receives pointer to user's domain
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL GetUserNameAndDomain(HANDLE hToken, LPTSTR *UserName, LPTSTR *UserDomain)
{
    BOOL bResult = FALSE;
    LPTSTR lpTemp, lpDomain = NULL;
    LPTSTR lpUserName, lpUserDomain;
    HANDLE hOldToken;


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        DebugMsg((DM_VERBOSE, TEXT("GetUserNameAndDomain Failed to impersonate user")));
        goto Exit;
    }


    //
    // Get the username in NT4 format
    //

    lpDomain = MyGetUserName (NameSamCompatible);

    RevertToUser(&hOldToken);

    if (!lpDomain) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameAndDomain  MyGetUserName failed for NT4 style name with %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Look for the \ between the domain and username and replace
    // it with a NULL
    //

    lpTemp = lpDomain;

    while (*lpTemp && ((*lpTemp) != TEXT('\\')))
        lpTemp++;


    if (*lpTemp != TEXT('\\')) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameAndDomain  Failed to find slash in NT4 style name:  <%s>"),
                 lpDomain));
        goto Exit;
    }

    *lpTemp = TEXT('\0');
    lpTemp++;


    //
    // Allocate space for the results
    //

    lpUserName = LocalAlloc (LPTR, (lstrlen(lpTemp) + 1) * sizeof(TCHAR));

    if (!lpUserName) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameAndDomain  Failed to allocate memory with %d"),
                 GetLastError()));
        goto Exit;
    }

    lstrcpy (lpUserName, lpTemp);


    lpUserDomain = LocalAlloc (LPTR, (lstrlen(lpDomain) + 1) * sizeof(TCHAR));

    if (!lpUserDomain) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameAndDomain  Failed to allocate memory with %d"),
                 GetLastError()));
        LocalFree (lpUserName);
        goto Exit;
    }

    lstrcpy (lpUserDomain, lpDomain);


    //
    // Save the results in the outbound arguments
    //

    *UserName = lpUserName;
    *UserDomain = lpUserDomain;


    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (lpDomain) {
        LocalFree (lpDomain);
    }

    return(bResult);
}

//*************************************************************
//
//  GetUserNameAndDomainSlowly()
//
//  Purpose:    Gets the user's name and domain from a DC
//
//  Parameters: hToken      -   User's token
//              UserName    -   Receives pointer to user's name
//              UserDomain  -   Receives pointer to user's domain
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL GetUserNameAndDomainSlowly(HANDLE hToken, LPTSTR *UserName, LPTSTR *UserDomain)
{
    LPTSTR lpUserName = NULL;
    LPTSTR lpUserDomain = NULL;
    DWORD cbAccountName = 0;
    DWORD cbUserDomain = 0;
    SID_NAME_USE SidNameUse;
    BOOL bRet = FALSE;
    PSID pSid;


    //
    // Get the user's sid
    //

    pSid = GetUserSid (hToken);

    if (!pSid) {
        return FALSE;
    }


    //
    // Get the space needed for the User name and the Domain name
    //
    if (!LookupAccountSid(NULL,
                         pSid,
                         NULL, &cbAccountName,
                         NULL, &cbUserDomain,
                         &SidNameUse
                         ) ) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Error;
        }
    }

    lpUserName = (LPTSTR)LocalAlloc(LPTR, cbAccountName*sizeof(TCHAR));
    if (!lpUserName) {
        goto Error;
    }

    lpUserDomain = (LPTSTR)LocalAlloc(LPTR, cbUserDomain*sizeof(WCHAR));
    if (!lpUserDomain) {
        LocalFree(lpUserName);
        goto Error;
    }

    //
    // Now get the user name and domain name
    //
    if (!LookupAccountSid(NULL,
                         pSid,
                         lpUserName, &cbAccountName,
                         lpUserDomain, &cbUserDomain,
                         &SidNameUse
                         ) ) {

        LocalFree(lpUserName);
        LocalFree(lpUserDomain);
        goto Error;
    }

    *UserName = lpUserName;
    *UserDomain = lpUserDomain;
    bRet = TRUE;

Error:
    DeleteUserSid (pSid);

    return(bRet);
}

//*************************************************************
//
//  GetUserDNSDomainName()
//
//  Purpose:    Gets the DNS domain name for the user
//
//  Parameters: lpDomain  - User's flat domain name
//
//
//  Return:     DNS domain name if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR GetUserDNSDomainName(LPTSTR lpDomain)
{
    PDOMAIN_CONTROLLER_INFO pDCI;
    LPTSTR lpTemp = NULL;
    DWORD dwResult, dwBufferSize;
    TCHAR szBuffer[MAX_PATH];
    PNETAPI32_API pNetAPI32;
    INT iRole;


    //
    // Check if this machine is running standalone, if so, there won't be
    // a DNS domain name
    //

    if (!GetMachineRole (&iRole)) {
        DebugMsg((DM_WARNING, TEXT("GetUserDNSDomainName:  Failed to get the role of the computer.")));
        return NULL;
    }

    if (iRole == 0) {
        DebugMsg((DM_VERBOSE, TEXT("GetUserDNSDomainName:  Computer is running standalone.  No DNS domain name available.")));
        return NULL;
    }


    //
    // Get the computer name to see if the user logged on locally
    //

    dwBufferSize = ARRAYSIZE(szBuffer);

    if (GetComputerName (szBuffer, &dwBufferSize)) {
        if (!lstrcmpi(lpDomain, szBuffer)) {
            DebugMsg((DM_VERBOSE, TEXT("GetUserDNSDomainName:  Local user account.  No DNS domain name available.")));
            return NULL;
        }
    }

    if (LoadString (g_hDllInstance, IDS_NT_AUTHORITY, szBuffer, ARRAYSIZE(szBuffer))) {
        if (!lstrcmpi(lpDomain, szBuffer)) {
            DebugMsg((DM_VERBOSE, TEXT("GetUserDNSDomainName:  Domain name is NT Authority.  No DNS domain name available.")));
            return NULL;
        }
    }

    if (LoadString (g_hDllInstance, IDS_BUILTIN, szBuffer, ARRAYSIZE(szBuffer))) {
        if (!lstrcmpi(lpDomain, szBuffer)) {
            DebugMsg((DM_VERBOSE, TEXT("GetUserDNSDomainName:  Domain name is BuiltIn.  No DNS domain name available.")));
            return NULL;
        }
    }

    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        DebugMsg((DM_WARNING, TEXT("GetUserDNSDomainName:  Failed to load netapi32 with %d."),
                 GetLastError()));
        goto Exit;
    }



    dwResult = pNetAPI32->pfnDsGetDcName(NULL, lpDomain, NULL, NULL,
                                       DS_IP_REQUIRED |
                                       DS_IS_FLAT_NAME |
                                       DS_DIRECTORY_SERVICE_PREFERRED |
                                       DS_RETURN_DNS_NAME,
                                       &pDCI);

    if (dwResult == ERROR_SUCCESS) {

        if (pDCI->DomainName) {

            lpTemp = (LPTSTR)LocalAlloc(LPTR, (lstrlen(pDCI->DomainName) + 1) * sizeof(TCHAR));

            if (lpTemp) {
                lstrcpy (lpTemp, pDCI->DomainName);
            }
        }

        pNetAPI32->pfnNetApiBufferFree(pDCI);
    }

Exit:

    return lpTemp;
}

//*************************************************************
//
//  GetHKeyCU()
//
//  Purpose:    Get HKEY_CURRENT_USER for the given hToken
//
//  Parameters: hToken  -   token handle
//
//  Return:     hKey if successful
//              NULL if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Created
//
//*************************************************************

LONG GetHKeyCU(HANDLE hToken, HKEY *hKeyCU)
{
    LPTSTR lpSidString;
    LONG    dwError;


    *hKeyCU = NULL;

    lpSidString = GetSidString (hToken);

    if (!lpSidString) {
        return GetLastError();
    }

    dwError = RegOpenKeyEx (HKEY_USERS, lpSidString, 0, KEY_READ, hKeyCU);

    if (!(*hKeyCU))
        DebugMsg((DM_VERBOSE, TEXT("GetHkeyCU: RegOpenKey failed with error %d"), dwError));

    DeleteSidString(lpSidString);

    return dwError;
}

/***************************************************************************\
* ExpandUserEvironmentVariable
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
DWORD
ExpandUserEnvironmentStrings(
    PVOID pEnv,
    LPCTSTR lpSrc,
    LPTSTR lpDst,
    DWORD nSize
    )
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;

    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(nSize*sizeof(WCHAR));
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U( pEnv,
                                          (PUNICODE_STRING)&Source,
                                          (PUNICODE_STRING)&Destination,
                                          &Length
                                        );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return( Length );
        }
    else {
        return( 0 );
        }
}

/***************************************************************************\
* ProcessAutoexecPath
*
* Creates AutoexecPath environment variable using autoexec.bat
* LpValue may be freed by this routine.
*
* History:
* 06-02-92  Johannec     Created.
*
\***************************************************************************/
LPTSTR ProcessAutoexecPath(PVOID pEnv, LPTSTR lpValue, DWORD cb)
{
    LPTSTR lpt;
    LPTSTR lpStart;
    LPTSTR lpPath;
    DWORD cbt;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    BOOL bPrevAutoexecPath;
    WCHAR ch;
    DWORD dwTemp, dwCount = 0;

    cbt = 1024;
    lpt = (LPTSTR)LocalAlloc(LPTR, cbt*sizeof(WCHAR));
    if (!lpt) {
        return(lpValue);
    }
    *lpt = 0;
    lpStart = lpValue;

    RtlInitUnicodeString(&Name, AUTOEXECPATH_VARIABLE);
    Value.Buffer = (PWCHAR)LocalAlloc(LPTR, cbt*sizeof(WCHAR));
    if (!Value.Buffer) {
        goto Fail;
    }

    while (NULL != (lpPath = wcsstr (lpValue, TEXT("%")))) {
        if (!_wcsnicmp(lpPath+1, TEXT("PATH%"), 5)) {
            //
            // check if we have an autoexecpath already set, if not just remove
            // the %path%
            //
            Value.Length = (USHORT)cbt;
            Value.MaximumLength = (USHORT)cbt;
            bPrevAutoexecPath = (BOOL)!RtlQueryEnvironmentVariable_U(pEnv, &Name, &Value);

            *lpPath = 0;
            dwTemp = dwCount + lstrlen (lpValue);
            if (dwTemp < cbt) {
               lstrcat(lpt, lpValue);
               dwCount = dwTemp;
            }
            if (bPrevAutoexecPath) {
                dwTemp = dwCount + lstrlen (Value.Buffer);
                if (dwTemp < cbt) {
                    lstrcat(lpt, Value.Buffer);
                    dwCount = dwTemp;
                }
            }

            *lpPath++ = TEXT('%');
            lpPath += 5;  // go passed %path%
            lpValue = lpPath;
        }
        else {
            lpPath = wcsstr(lpPath+1, TEXT("%"));
            if (!lpPath) {
                lpStart = NULL;
                goto Fail;
            }
            lpPath++;
            ch = *lpPath;
            *lpPath = 0;
            dwTemp = dwCount + lstrlen (lpValue);
            if (dwTemp < cbt) {
                lstrcat(lpt, lpValue);
                dwCount = dwTemp;
            }
            *lpPath = ch;
            lpValue = lpPath;
        }
    }

    if (*lpValue) {
       dwTemp = dwCount + lstrlen (lpValue);
       if (dwTemp < cbt) {
           lstrcat(lpt, lpValue);
           dwCount = dwTemp;
       }
    }

    LocalFree(Value.Buffer);
    LocalFree(lpStart);

    return(lpt);
Fail:
    LocalFree(lpt);
    return(lpStart);
}

/***************************************************************************\
* ProcessCommand
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL ProcessCommand(LPSTR lpStart, PVOID *pEnv)
{
    LPTSTR lpt, lptt;
    LPTSTR lpVariable;
    LPTSTR lpValue;
    LPTSTR lpExpandedValue = NULL;
    WCHAR c;
    DWORD cb, cbNeeded;
    LPTSTR lpu;

    //
    // convert to Unicode
    //
    lpu = (LPTSTR)LocalAlloc(LPTR, (cb=lstrlenA(lpStart)+1)*sizeof(WCHAR));

    if (!lpu) {
        return FALSE;
    }

    MultiByteToWideChar(CP_OEMCP, 0, lpStart, -1, lpu, cb);

    //
    // Find environment variable.
    //
    for (lpt = lpu; *lpt && *lpt == TEXT(' '); lpt++) //skip spaces
        ;

    if (!*lpt) {
        LocalFree (lpu);
        return(FALSE);
    }


    lptt = lpt;
    for (; *lpt && *lpt != TEXT(' ') && *lpt != TEXT('='); lpt++) //find end of variable name
        ;

    c = *lpt;
    *lpt = 0;
    lpVariable = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lptt) + 1)*sizeof(WCHAR));
    if (!lpVariable) {
        LocalFree (lpu);
        return(FALSE);
    }

    lstrcpy(lpVariable, lptt);
    *lpt = c;

    //
    // Find environment variable value.
    //
    for (; *lpt && (*lpt == TEXT(' ') || *lpt == TEXT('=')); lpt++)
        ;

    if (!*lpt) {
       // if we have a blank path statement in the autoexec file,
       // then we don't want to pass "PATH" as the environment
       // variable because it trashes the system's PATH.  Instead
       // we want to change the variable AutoexecPath.  This would have
       // be handled below if a value had been assigned to the
       // environment variable.
       if (lstrcmpi(lpVariable, PATH_VARIABLE) == 0)
          {
          SetEnvironmentVariableInBlock(pEnv, AUTOEXECPATH_VARIABLE, TEXT(""), TRUE);
          }
       else
          {
          SetEnvironmentVariableInBlock(pEnv, lpVariable, TEXT(""), TRUE);
          }
        LocalFree (lpVariable);
        LocalFree (lpu);
        return(FALSE);
    }

    lptt = lpt;
    for (; *lpt; lpt++)  //find end of varaible value
        ;

    c = *lpt;
    *lpt = 0;
    lpValue = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lptt) + 1)*sizeof(WCHAR));
    if (!lpValue) {
        LocalFree (lpu);
        LocalFree(lpVariable);
        return(FALSE);
    }

    lstrcpy(lpValue, lptt);
    *lpt = c;

#ifdef _X86_
    // NEC98
    //
    // If the path includes removable drive,
    //  it is assumed that the drive assignment has changed from DOS.
    //
    if (IsNEC_98 && (lstrcmpi(lpVariable, PATH_VARIABLE) == 0) && IsPathIncludeRemovable(lpValue)) {
        LocalFree (lpu);
        LocalFree(lpVariable);
        LocalFree(lpValue);
        return(FALSE);
    }
#endif

    cb = 1024;
    lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
    if (lpExpandedValue) {
        if (!lstrcmpi(lpVariable, PATH_VARIABLE)) {
            lpValue = ProcessAutoexecPath(*pEnv, lpValue, (lstrlen(lpValue)+1)*sizeof(WCHAR));
        }
        cbNeeded = ExpandUserEnvironmentStrings(*pEnv, lpValue, lpExpandedValue, cb);
        if (cbNeeded > cb) {
            LocalFree(lpExpandedValue);
            cb = cbNeeded;
            lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
            if (lpExpandedValue) {
                ExpandUserEnvironmentStrings(*pEnv, lpValue, lpExpandedValue, cb);
            }
        }
    }

    if (!lpExpandedValue) {
        lpExpandedValue = lpValue;
    }
    if (lstrcmpi(lpVariable, PATH_VARIABLE)) {
        SetEnvironmentVariableInBlock(pEnv, lpVariable, lpExpandedValue, FALSE);
    }
    else {
        SetEnvironmentVariableInBlock(pEnv, AUTOEXECPATH_VARIABLE, lpExpandedValue, TRUE);

    }

    if (lpExpandedValue != lpValue) {
        LocalFree(lpExpandedValue);
    }
    LocalFree(lpVariable);
    LocalFree(lpValue);
    LocalFree (lpu);

    return(TRUE);
}

/***************************************************************************\
* ProcessSetCommand
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL ProcessSetCommand(LPSTR lpStart, PVOID *pEnv)
{
    LPSTR lpt;

    //
    // Find environment variable.
    //
    for (lpt = lpStart; *lpt && *lpt != TEXT(' '); lpt++)
        ;

    if (!*lpt)
       return(FALSE);

    return (ProcessCommand(lpt, pEnv));

}

/***************************************************************************\
* ProcessAutoexec
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL
ProcessAutoexec(
    PVOID *pEnv
    )
{
    HANDLE fh;
    DWORD dwFileSize;
    DWORD dwBytesRead;
    CHAR *lpBuffer = NULL;
    CHAR *token;
    CHAR Seps[] = "&\n\r";   // Seperators for tokenizing autoexec.bat
    BOOL Status = FALSE;
    TCHAR szAutoExecBat [] = TEXT("c:\\autoexec.bat");
#ifdef _X86_
    TCHAR szTemp[3];
#endif
    UINT uiErrMode;


    // There is a case where the OS might not be booting from drive
    // C, so we can not assume that the autoexec.bat file is on c:\.
    // Set the error mode so the user doesn't see the critical error
    // popup and attempt to open the file on c:\.

    uiErrMode = SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

#ifdef _X86_
    if (IsNEC_98) {
        if (GetEnvironmentVariable (TEXT("SystemDrive"), szTemp, 3)) {
	    szAutoExecBat[0] = szTemp[0];
	}
    }
#endif

    fh = CreateFile (szAutoExecBat, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    SetErrorMode (uiErrMode);

    if (fh ==  INVALID_HANDLE_VALUE) {
        return(FALSE);  //could not open autoexec.bat file, we're done.
    }
    dwFileSize = GetFileSize(fh, NULL);
    if (dwFileSize == -1) {
        goto Exit;      // can't read the file size
    }

    lpBuffer = (PCHAR)LocalAlloc(LPTR, dwFileSize+1);
    if (!lpBuffer) {
        goto Exit;
    }

    Status = ReadFile(fh, lpBuffer, dwFileSize, &dwBytesRead, NULL);
    if (!Status) {
        goto Exit;      // error reading file
    }

    //
    // Zero terminate the buffer so we don't walk off the end
    //

    ASSERT(dwBytesRead <= dwFileSize);
    lpBuffer[dwBytesRead] = 0;

    //
    // Search for SET and PATH commands
    //

    token = strtok(lpBuffer, Seps);
    while (token != NULL) {
        for (;*token && *token == ' ';token++) //skip spaces
            ;
        if (*token == TEXT('@'))
            token++;
        for (;*token && *token == ' ';token++) //skip spaces
            ;
        if (!_strnicmp(token, "Path", 4)) {
            ProcessCommand(token, pEnv);
        }
        if (!_strnicmp(token, "SET", 3)) {
            ProcessSetCommand(token, pEnv);
        }
        token = strtok(NULL, Seps);
    }
Exit:
    CloseHandle(fh);
    if (lpBuffer) {
        LocalFree(lpBuffer);
    }
    if (!Status) {
        DebugMsg((DM_WARNING, TEXT("ProcessAutoexec: Cannot process autoexec.bat.")));
    }
    return(Status);
}

/***************************************************************************\
* BuildEnvironmentPath
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL BuildEnvironmentPath(PVOID *pEnv,
                          LPTSTR lpPathVariable,
                          LPTSTR lpPathValue)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    WCHAR lpTemp[1025];
    DWORD cb;


    if (!*pEnv) {
        return(FALSE);
    }
    RtlInitUnicodeString(&Name, lpPathVariable);
    cb = 1024;
    Value.Buffer = (PWCHAR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
    if (!Value.Buffer) {
        return(FALSE);
    }
    Value.Length = (USHORT)(sizeof(WCHAR) * cb);
    Value.MaximumLength = (USHORT)(sizeof(WCHAR) * cb);
    Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
    if (!NT_SUCCESS(Status)) {
        LocalFree(Value.Buffer);
        Value.Length = 0;
        *lpTemp = 0;
    }
    if (Value.Length) {
        lstrcpy(lpTemp, Value.Buffer);
        if ( *( lpTemp + lstrlen(lpTemp) - 1) != TEXT(';') ) {
            lstrcat(lpTemp, TEXT(";"));
        }
        LocalFree(Value.Buffer);
    }
    if (lpPathValue && ((lstrlen(lpTemp) + lstrlen(lpPathValue) + 1) < (INT)cb)) {
        lstrcat(lpTemp, lpPathValue);

        RtlInitUnicodeString(&Value, lpTemp);

        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}

/***************************************************************************\
* AppendNTPathWithAutoexecPath
*
* Gets the AutoexecPath created in ProcessAutoexec, and appends it to
* the NT path.
*
* History:
* 05-28-92  Johannec     Created.
*
\***************************************************************************/
BOOL
AppendNTPathWithAutoexecPath(
    PVOID *pEnv,
    LPTSTR lpPathVariable,
    LPTSTR lpAutoexecPath
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    WCHAR AutoexecPathValue[1024];
    DWORD cb;
    BOOL Success;

    if (!*pEnv) {
        return(FALSE);
    }

    RtlInitUnicodeString(&Name, lpAutoexecPath);
    cb = 1024;
    Value.Buffer = (PWCHAR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
    if (!Value.Buffer) {
        return(FALSE);
    }

    Value.Length = (USHORT)cb;
    Value.MaximumLength = (USHORT)cb;
    Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
    if (!NT_SUCCESS(Status)) {
        LocalFree(Value.Buffer);
        return(FALSE);
    }

    if (Value.Length) {
        lstrcpy(AutoexecPathValue, Value.Buffer);
    }

    LocalFree(Value.Buffer);

    Success = BuildEnvironmentPath(pEnv, lpPathVariable, AutoexecPathValue);
    RtlSetEnvironmentVariable( pEnv, &Name, NULL);

    return(Success);
}

/***************************************************************************\
* SetEnvironmentVariables
*
* Reads the user-defined environment variables from the user registry
* and adds them to the environment block at pEnv.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetEnvironmentVariables(
    PVOID *pEnv,
    LPTSTR lpRegSubKey,
    HKEY hKeyCU
    )
{
    WCHAR lpValueName[MAX_PATH];
    LPBYTE  lpDataBuffer;
    DWORD cbDataBuffer;
    LPBYTE  lpData;
    LPTSTR lpExpandedValue = NULL;
    DWORD cbValueName = MAX_PATH;
    DWORD cbData;
    DWORD dwType;
    DWORD dwIndex = 0;
    HKEY hkey;
    BOOL bResult;

    if (RegOpenKeyExW(hKeyCU, lpRegSubKey, 0, KEY_READ, &hkey)) {
        return(FALSE);
    }

    cbDataBuffer = 4096;
    lpDataBuffer = (LPBYTE)LocalAlloc(LPTR, cbDataBuffer*sizeof(WCHAR));
    if (lpDataBuffer == NULL) {
        RegCloseKey(hkey);
        return(FALSE);
    }
    lpData = lpDataBuffer;
    cbData = cbDataBuffer;
    bResult = TRUE;
    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_VALUE_LEN-1] = TEXT('\0');


            if (dwType == REG_SZ) {
                //
                // The path variables PATH, LIBPATH and OS2LIBPATH must have
                // their values apppended to the system path.
                //

                if ( !lstrcmpi(lpValueName, PATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, LIBPATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, OS2LIBPATH_VARIABLE) ) {

                    BuildEnvironmentPath(pEnv, lpValueName, (LPTSTR)lpData);
                }
                else {

                    //
                    // the other environment variables are just set.
                    //

                    SetEnvironmentVariableInBlock(pEnv, lpValueName, (LPTSTR)lpData, TRUE);
                }
            }
        }
        dwIndex++;
        cbData = cbDataBuffer;
        cbValueName = MAX_PATH;
    }

    dwIndex = 0;
    cbData = cbDataBuffer;
    cbValueName = MAX_PATH;


    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_VALUE_LEN-1] = TEXT('\0');


            if (dwType == REG_EXPAND_SZ) {
                DWORD cb, cbNeeded;

                cb = 1024;
                lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
                if (lpExpandedValue) {
                    cbNeeded = ExpandUserEnvironmentStrings(*pEnv, (LPTSTR)lpData, lpExpandedValue, cb);
                    if (cbNeeded > cb) {
                        LocalFree(lpExpandedValue);
                        cb = cbNeeded;
                        lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
                        if (lpExpandedValue) {
                            ExpandUserEnvironmentStrings(*pEnv, (LPTSTR)lpData, lpExpandedValue, cb);
                        }
                    }
                }

                if (lpExpandedValue == NULL) {
                    bResult = FALSE;
                    break;
                }


                //
                // The path variables PATH, LIBPATH and OS2LIBPATH must have
                // their values apppended to the system path.
                //

                if ( !lstrcmpi(lpValueName, PATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, LIBPATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, OS2LIBPATH_VARIABLE) ) {

                    BuildEnvironmentPath(pEnv, lpValueName, (LPTSTR)lpExpandedValue);
                }
                else {

                    //
                    // the other environment variables are just set.
                    //

                    SetEnvironmentVariableInBlock(pEnv, lpValueName, (LPTSTR)lpExpandedValue, TRUE);
                }

                LocalFree(lpExpandedValue);

            }

        }
        dwIndex++;
        cbData = cbDataBuffer;
        cbValueName = MAX_PATH;
    }



    LocalFree(lpDataBuffer);
    RegCloseKey(hkey);

    return(bResult);
}

//*************************************************************
//
//  ExpandEnvironmentStringsForUser()
//
// Purpose:  Expands the source string using the environment block for the
//           specified user.  If hToken is null, the system environment block
//           will be used (no user environment variables).
//
//  Parameters: hToken  -  User's token (or null for system env vars)
//              lpSrc   -  String to be expanded
//              lpDest  -  Buffer to receive string
//              dwSize  -  Size of dest buffer
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI ExpandEnvironmentStringsForUser(HANDLE hToken, LPCTSTR lpSrc,
                                            LPTSTR lpDest, DWORD dwSize)
{
    LPVOID pEnv;
    DWORD  dwNeeded;
    BOOL bResult = FALSE;


    //
    // Arg check
    //
    
    if ( !lpDest || !lpSrc )
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    
    //
    // Get the user's environment block
    //

    if (!CreateEnvironmentBlock (&pEnv, hToken, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("ExpandEnvironmentStringsForUser:  CreateEnvironmentBlock failed with = %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Expand the string
    //

    dwNeeded = ExpandUserEnvironmentStrings(pEnv, lpSrc, lpDest, dwSize);

    if (dwNeeded && (dwNeeded < dwSize)) {
        bResult = TRUE;
    } else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER );
    }


    //
    // Free the environment block
    //

    DestroyEnvironmentBlock (pEnv);


    return bResult;
}

//*************************************************************
//
//  GetSystemTempDirectory()
//
//  Purpose:    Gets the system temp directory in short form
//
//  Parameters: lpDir     - Receives the directory
//              lpcchSize - Size of the lpDir buffer
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI GetSystemTempDirectory(LPTSTR lpDir, LPDWORD lpcchSize)
{
    TCHAR  szTemp[MAX_PATH];
    TCHAR  szDirectory[MAX_PATH];
    DWORD  dwLength;
    HKEY   hKey;
    LONG   lResult;
    DWORD  dwSize, dwType;
    BOOL   bRetVal = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA fad;


    szTemp[0] = TEXT('\0');
    szDirectory[0] = TEXT('\0');

    //
    // Look in the system environment variables
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, SYS_ENVVARS, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        //
        // Check for TEMP
        //

        dwSize = sizeof(szTemp);

        if (RegQueryValueEx (hKey, TEXT("TEMP"), NULL, &dwType,
                             (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {
            RegCloseKey (hKey);
            goto FoundTemp;
        }


        //
        // Check for TMP
        //

        dwSize = sizeof(szTemp);

        if (RegQueryValueEx (hKey, TEXT("TMP"), NULL, &dwType,
                             (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {
            RegCloseKey (hKey);
            goto FoundTemp;
        }


        RegCloseKey (hKey);
    }


    //
    // Check if %SystemRoot%\Temp exists
    //

    lstrcpy (szDirectory, TEXT("%SystemRoot%\\Temp"));
    ExpandEnvironmentStrings (szDirectory, szTemp, ARRAYSIZE (szTemp));

    if (GetFileAttributesEx (szTemp, GetFileExInfoStandard, &fad) &&
        fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        goto FoundTemp;
    }


    //
    // Check if %SystemDrive%\Temp exists
    //

    lstrcpy (szDirectory, TEXT("%SystemDrive%\\Temp"));
    ExpandEnvironmentStrings (szDirectory, szTemp, ARRAYSIZE (szTemp));

    if (GetFileAttributesEx (szTemp, GetFileExInfoStandard, &fad) &&
        fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        goto FoundTemp;
    }


    //
    // Last resort is %SystemRoot%
    //

    lstrcpy (szTemp, TEXT("%SystemRoot%"));



FoundTemp:

    ExpandEnvironmentStrings (szTemp, szDirectory, ARRAYSIZE (szDirectory));
    GetShortPathName (szDirectory, szTemp, ARRAYSIZE(szTemp));

    dwLength = lstrlen(szTemp) + 1;

    if (lpDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpDir, szTemp);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    } else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    *lpcchSize = dwLength;

    return bRetVal;
}

#ifdef _X86_
BOOL
IsPathIncludeRemovable(LPTSTR lpValue)
{
    LPTSTR      lpt, tmp;
    BOOL        ret = FALSE;
    WCHAR       c;

    tmp = LocalAlloc(LPTR, (lstrlen(lpValue) + 1) * sizeof(WCHAR));
    if (!tmp)
        DebugMsg((DM_WARNING, TEXT("IsPathIncludeRemovable : Failed to LocalAlloc (%d)"), GetLastError()));
    else {
	lstrcpy(tmp, lpValue);

	lpt = tmp;
	while (*lpt) {
	    // skip spaces
	    for ( ; *lpt && *lpt == TEXT(' '); lpt++)
		;

	    // check if the drive is removable
	    if (lpt[0] && lpt[1] && lpt[1] == TEXT(':') && lpt[2]) {        // ex) "A:\"
		c = lpt[3];
		lpt[3] = 0;
		if (GetDriveType(lpt) == DRIVE_REMOVABLE) {
		    lpt[3] = c;
		    ret = TRUE;
		    break;
		}
		lpt[3] = c;
	    }

	    // skip to the next path
	    for ( ; *lpt && *lpt != TEXT(';'); lpt++)
		;
	    if (*lpt)
		lpt++;
	}
	LocalFree(tmp);
    }
    return(ret);
}
#endif
