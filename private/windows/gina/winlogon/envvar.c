/****************************** Module Header ******************************\
* Module Name: envvar.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Sets environment variables.
*
* History:
* 2-25-92 JohanneC       Created -
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL UpdateUserEnvironment(  PVOID *pEnv  );

#define KEY_NAME TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Environment")

//
// Max environment variable length
//

#define MAX_VALUE_LEN          1024


/***************************************************************************\
* InitSystemParametersInfo
*
* Re-Initializes system parameters given the current user registry.
*
* History:
*
\***************************************************************************/

VOID
InitSystemParametersInfo(
    PTERMINAL   pTerm,
    BOOL        bUserLoggedOn
    )
{
    PWINDOWSTATION  pWS = pTerm->pWinStaWinlogon;
    HANDLE          ImpersonationHandle;
    BOOL            Result;
    
    //
    // Open the profile mapping for the logged on user
    //

    Result = OpenIniFileUserMapping(pTerm);
    if (!Result) {
        DebugLog((DEB_ERROR, "InitSystemParametersInfo: Failed to open ini file mapping for user\n"));
        return;
    }

    //
    // Impersonate the user so if the user server side has to reference
    // any ini files etc, it will do it in the correct place.
    //

    ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "InitSystemParametersInfo: Failed to impersonate user\n"));
        CloseIniFileUserMapping(pTerm);
        return;
    }

    UpdatePerUserSystemParameters(pWS->hToken, bUserLoggedOn);
    
    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        DebugLog((DEB_ERROR, "InitSystemParametersInfo : Failed to revert to self\n"));
    }

    //
    // Close the profile mapping
    //

    CloseIniFileUserMapping(pTerm);
}


/***************************************************************************\
* CreateUserEnvironment
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
CreateUserEnvironment(
    PVOID *pEnv
    )
{
    NTSTATUS Status;

    Status = RtlCreateEnvironment((BOOLEAN)TRUE, pEnv);
    if (NT_SUCCESS(Status)) {

        //
        // We must examine the registry directly to suck out
        // the system environment variables, because they
        // may have changed since the system was booted.
        //

        UpdateUserEnvironment(pEnv);

        return(TRUE);
    }

    return(FALSE);
}


/***************************************************************************\
* SetUserEnvironmentVariable
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetUserEnvironmentVariable(
    PVOID *pEnv,
    LPTSTR lpVariable,
    LPTSTR lpValue,
    BOOL bOverwrite
    )
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
    cb = 1024;
    Value.Buffer = Alloc(sizeof(TCHAR)*cb);
    if (Value.Buffer) {
        Value.Length = (USHORT)cb;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);

        Free(Value.Buffer);

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


/***************************************************************************\
* IsUNCPath
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL IsUNCPath(LPTSTR lpPath)
{
    if (lpPath[0] == BSLASH && lpPath[1] == BSLASH) {
        return(TRUE);
    }
    return(FALSE);
}

/***************************************************************************\
* SetHomeDirectoryEnvVars
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetHomeDirectoryEnvVars(
    PVOID *pEnv,
    LPTSTR lpHomeDirectory,
    LPTSTR lpHomeDrive,
    LPTSTR lpHomeShare,
    LPTSTR lpHomePath
    )
{
    TCHAR cTmp;
    LPTSTR lpHomeTmp;
    BOOL bFoundFirstBSlash = FALSE;

    if (!*lpHomeDirectory) {
        return(FALSE);
    }
    if (IsUNCPath(lpHomeDirectory)) {
        lpHomeTmp = lpHomeDirectory + 2;
        while (*lpHomeTmp) {
            if (*lpHomeTmp == BSLASH) {
                if (bFoundFirstBSlash) {
                    break;
                }
                bFoundFirstBSlash = TRUE;
            }
            lpHomeTmp++;
        }
        if (*lpHomeTmp) {
            lstrcpy(lpHomePath, lpHomeTmp);
        }
        else {
            *lpHomePath = BSLASH;
            *(lpHomePath+1) = 0;
        }

        cTmp = *lpHomeTmp;
        *lpHomeTmp = (TCHAR)0;
        lstrcpy(lpHomeShare, lpHomeDirectory);
        *lpHomeTmp = cTmp;

        //
        // If no home drive specified, than default to z:
        //
        if (!*lpHomeDrive) {
            lstrcpy(lpHomeDrive, TEXT("Z:"));
        }

    }
    else {  // local home directory

        *lpHomeShare = 0;   // no home share

        cTmp = lpHomeDirectory[2];
        lpHomeDirectory[2] = (TCHAR)0;
        lstrcpy(lpHomeDrive, lpHomeDirectory);
        lpHomeDirectory[2] = cTmp;

        lstrcpy(lpHomePath, lpHomeDirectory + 2);
    }

    SetUserEnvironmentVariable(pEnv, HOMEDRIVE_VARIABLE, lpHomeDrive, TRUE);
    SetUserEnvironmentVariable(pEnv, HOMESHARE_VARIABLE, lpHomeShare, TRUE);
    SetUserEnvironmentVariable(pEnv, HOMEPATH_VARIABLE, lpHomePath, TRUE);

    return(TRUE);
}

BOOL
UpdateUserEnvironment(
    PVOID *pEnv
    )
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
                 KEY_NAME,
                 0,
                 KEY_QUERY_VALUE,
                 &KeyHandle
                 );

    if ( Result != ERROR_SUCCESS ) {

        DebugLog((DEB_ERROR, "RegOpenKeyEx failed, error = %d\n",Result));
        return( FALSE );
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
        RegCloseKey(KeyHandle);
        return( FALSE );
    }

    //
    // No need to adjust the datalength for TCHAR issues
    //

    ValueData = Alloc( cbMaxValueData );

    if ( ValueData == NULL ) {
        RegCloseKey(KeyHandle);
        return( FALSE );
    }

    //
    // The maximum value name length comes back in characters, convert to bytes
    // before allocating storage.  Allow for trailing NULL also.
    //

    ValueName = Alloc( (++chMaxValueName) * sizeof( TCHAR ) );

    if ( ValueName == NULL ) {

        RegCloseKey(KeyHandle);
        Free( ValueData );
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

            rc = FALSE;
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

                    Bool = SetUserEnvironmentVariable(
                               pEnv,
                               ValueName,
                               ValueData,
                               TRUE
                               );

                    if ( !Bool ) {
                        DebugLog((DEB_ERROR, "UpdateUserEnvironment:  Failed to set <%s> to <%s> with %d.",
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

            rc = FALSE;
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

                    Bool = SetUserEnvironmentVariable(
                               pEnv,
                               ValueName,
                               ExpandedValue,
                               TRUE
                               );

                    Free( ExpandedValue );

                    if ( !Bool ) {
                        DebugLog((DEB_ERROR, "UpdateUserEnvironment:  Failed to set <%s> to <%s> with %d.",
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

    Free( ValueName );
    Free( ValueData );

    return( rc );
}
