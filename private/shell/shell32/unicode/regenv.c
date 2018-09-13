#define UNICODE 1

#include "precomp.h"
#pragma  hdrstop
#ifdef _WIN64
char * __cdecl StrTokEx (char ** pstring, const char * control);
#else
#include <iert.h>
#endif

//
// Value names for for different environment variables
//

#define PATH_VARIABLE            TEXT("Path")
#define LIBPATH_VARIABLE         TEXT("LibPath")
#define OS2LIBPATH_VARIABLE      TEXT("Os2LibPath")
#define AUTOEXECPATH_VARIABLE    TEXT("AutoexecPath")

#define HOMEDRIVE_VARIABLE       TEXT("HOMEDRIVE")
#define HOMESHARE_VARIABLE       TEXT("HOMESHARE")
#define HOMEPATH_VARIABLE        TEXT("HOMEPATH")

#define COMPUTERNAME_VARIABLE    TEXT("COMPUTERNAME")
#define USERNAME_VARIABLE        TEXT("USERNAME")
#define USERDOMAIN_VARIABLE      TEXT("USERDOMAIN")
#define USERDNSDOMAIN_VARIABLE   TEXT("USERDNSDOMAIN")
#define USERPROFILE_VARIABLE     TEXT("USERPROFILE")
#define ALLUSERSPROFILE_VARIABLE TEXT("ALLUSERSPROFILE")
#define OS_VARIABLE              TEXT("OS")
#define PROCESSOR_VARIABLE       TEXT("PROCESSOR_ARCHITECTURE")
#define PROCESSOR_LEVEL_VARIABLE TEXT("PROCESSOR_LEVEL")

#define SYSTEMDRIVE_VARIABLE     TEXT("SystemDrive")
#define SYSTEMROOT_VARIABLE      TEXT("SystemRoot")
#define PROGRAMFILES_VARIABLE    TEXT("ProgramFiles")
#define COMMONPROGRAMFILES_VARIABLE     TEXT("CommonProgramFiles")
#ifdef WX86
#define PROGRAMFILESX86_VARIABLE        TEXT("ProgramFiles(x86)")
#define COMMONPROGRAMFILESX86_VARIABLE  TEXT("CommonProgramFiles(x86)")
#endif
#define SYSTEM_ENV_SUBKEY        TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment")
#define USER_ENV_SUBKEY          TEXT("Environment")
#define USER_VOLATILE_ENV_SUBKEY TEXT("Volatile Environment")

//
// Max environment variable length
//

#define MAX_VALUE_LEN          1024

//
// Parsing information for autoexec.bat
//
#define PARSE_AUTOEXEC_KEY     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define PARSE_AUTOEXEC_ENTRY   TEXT("ParseAutoexec")
#define PARSE_AUTOEXEC_DEFAULT TEXT("1")
#define MAX_PARSE_AUTOEXEC_BUFFER 2


#ifdef _X86_
BOOL IsPathIncludeRemovable(LPCTSTR lpValue)
{
    BOOL ret = FALSE;
    LPTSTR pszDup = StrDup(lpValue);
    if (pszDup)
    {
        LPTSTR pszTemp = pszDup;
        while (*pszTemp) 
        {
            // skip spaces
            for ( ; *pszTemp && *pszTemp == TEXT(' '); pszTemp++)
                ;

            // check if the drive is removable
            if (pszTemp[0] && pszTemp[1] && pszTemp[1] == TEXT(':') && pszTemp[2]) {        // ex) "A:\"
                TCHAR c = pszTemp[3];
                pszTemp[3] = 0;
                if (PathIsRemovable(pszTemp)) {
                    pszTemp[3] = c;
                    ret = TRUE;
                    break;
                }
                pszTemp[3] = c;
            }

            // skip to the next path
            for ( ; *pszTemp && *pszTemp != TEXT(';'); pszTemp++)
                ;
            if (*pszTemp)
                pszTemp++;
        }
        LocalFree(pszDup);
    }
    return ret;
}
#endif

/***************************************************************************\
* SetUserEvironmentVariable
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL SetUserEnvironmentVariable(void **pEnv, LPTSTR lpVariable, LPTSTR lpValue, BOOL bOverwrite)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    DWORD cb;
    TCHAR szValue[1024];

    if (!*pEnv || !lpVariable || !*lpVariable) {
        return(FALSE);
    }
    RtlInitUnicodeString(&Name, lpVariable);
    cb = 1024;
    Value.Buffer = (PTCHAR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
    if (Value.Buffer) {
        Value.Length = (USHORT)cb;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
        LocalFree(Value.Buffer);
        if (NT_SUCCESS(Status) && !bOverwrite) {
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
        Status = RtlSetEnvironmentVariable( pEnv, &Name, NULL);
    }
    return NT_SUCCESS(Status);
}


/***************************************************************************\
* ExpandUserEvironmentVariable
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
DWORD ExpandUserEnvironmentStrings(void *pEnv, LPTSTR lpSrc, LPTSTR lpDst, DWORD nSize)
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;
    
    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(nSize*SIZEOF(WCHAR));
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
* BuildEnvironmentPath
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL BuildEnvironmentPath(void **pEnv, LPTSTR lpPathVariable, LPTSTR lpPathValue)
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
* SetEnvironmentVariables
*
* Reads the user-defined environment variables from the user registry
* and adds them to the environment block at pEnv.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL SetEnvironmentVariables(void **pEnv, LPTSTR lpRegSubKey)
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

    if (RegOpenKeyExW(HKEY_CURRENT_USER, lpRegSubKey, 0, KEY_READ, &hkey)) {
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

                    SetUserEnvironmentVariable(pEnv, lpValueName, (LPTSTR)lpData, TRUE);
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

                    SetUserEnvironmentVariable(pEnv, lpValueName, (LPTSTR)lpExpandedValue, TRUE);
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

/***************************************************************************\
* SetSystemEnvironmentVariables
*
* Reads the system environment variables from the LOCAL_MACHINE registry
* and adds them to the environment block at pEnv.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL SetSystemEnvironmentVariables(void **pEnv)
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

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, SYSTEM_ENV_SUBKEY, 0, KEY_READ, &hkey)) {
        return(FALSE);
    }

    cbDataBuffer = 4096;
    lpDataBuffer = (LPBYTE)LocalAlloc(LPTR, cbDataBuffer*sizeof(WCHAR));
    if (lpDataBuffer == NULL) {
        KdPrint(("REGENENV: SetSystemEnvironmentVariables: Failed to allocate %d bytes\n", cbDataBuffer));
        RegCloseKey(hkey);
        return(FALSE);
    }

    //
    // First start by getting the systemroot and systemdrive values and
    // setting it in the new environment.
    //
    GetEnvironmentVariable(SYSTEMROOT_VARIABLE, (LPTSTR)lpDataBuffer, cbDataBuffer);
    SetUserEnvironmentVariable(pEnv, SYSTEMROOT_VARIABLE, (LPTSTR)lpDataBuffer, TRUE);

    GetEnvironmentVariable(SYSTEMDRIVE_VARIABLE, (LPTSTR)lpDataBuffer, cbDataBuffer);
    SetUserEnvironmentVariable(pEnv, SYSTEMDRIVE_VARIABLE, (LPTSTR)lpDataBuffer, TRUE);

    GetEnvironmentVariable(ALLUSERSPROFILE_VARIABLE, (LPTSTR)lpDataBuffer, cbDataBuffer);
    SetUserEnvironmentVariable(pEnv, ALLUSERSPROFILE_VARIABLE, (LPTSTR)lpDataBuffer, TRUE);

    lpData = lpDataBuffer;
    cbData = cbDataBuffer;
    bResult = TRUE;

    //
    // To generate the environment, this requires two passes.  First pass
    // sets all the variables which do not need to be expanded.  The
    // second pass expands variables (so it can use the variables from
    // the first pass.
    //

    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_VALUE_LEN-1] = TEXT('\0');

            if (dwType == REG_SZ) {
                SetUserEnvironmentVariable(pEnv, lpValueName, (LPTSTR)lpData, TRUE);
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
                        lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb);
                        if (lpExpandedValue) {
                            ExpandUserEnvironmentStrings(*pEnv, (LPTSTR)lpData, lpExpandedValue, cb);
                        }
                    }
                }

                if (lpExpandedValue == NULL) {
                    bResult = FALSE;
                    break;
                }

                SetUserEnvironmentVariable(pEnv, lpValueName, (LPTSTR)lpExpandedValue, TRUE);

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
LPTSTR ProcessAutoexecPath(void *pEnv, LPTSTR lpValue, DWORD cb)
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
BOOL ProcessCommand(LPSTR lpStart, void **pEnv)
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
            SetUserEnvironmentVariable(pEnv, AUTOEXECPATH_VARIABLE, TEXT(""), TRUE);
        }
        else
        {
            SetUserEnvironmentVariable(pEnv, lpVariable, TEXT(""), TRUE);
        }
        LocalFree (lpu);
        LocalFree (lpVariable);
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
        SetUserEnvironmentVariable(pEnv, lpVariable, lpExpandedValue, FALSE);
    }
    else {
        SetUserEnvironmentVariable(pEnv, AUTOEXECPATH_VARIABLE, lpExpandedValue, TRUE);
        
    }
    
    if (lpExpandedValue != lpValue) {
        LocalFree(lpExpandedValue);
    }
    LocalFree(lpVariable);
    LocalFree(lpValue);
    
    return(TRUE);
}

/***************************************************************************\
* ProcessSetCommand
*
* History:
* 01-24-92  Johannec     Created.
*
\***************************************************************************/
BOOL ProcessSetCommand(LPSTR lpStart, void **pEnv)
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
BOOL ProcessAutoexec(void **pEnv, LPTSTR lpPathVariable)
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
	CHAR *lpszStrTokBegin = NULL;


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

	// save off lpBuffer 
	lpszStrTokBegin = lpBuffer;
	
    token = StrTokEx(&lpBuffer, Seps);
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
        token = StrTokEx(&lpBuffer, Seps);
    }
	lpBuffer=lpszStrTokBegin;
	
Exit:
    CloseHandle(fh);
    if (lpBuffer) {
        LocalFree(lpBuffer);
    }
    return(Status);
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
    void **pEnv,
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

STDAPI_(BOOL) GetUserNameAndDomain(LPTSTR *ppszUserName, LPTSTR *ppszUserDomain)
{
    BOOL bRet = FALSE;
    HANDLE hToken;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        DWORD cbTokenBuffer = 0;
        if (GetTokenInformation(hToken, TokenUser, NULL, 0, &cbTokenBuffer) ||
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            PTOKEN_USER pUserToken = (PTOKEN_USER)LocalAlloc(LPTR, cbTokenBuffer * sizeof(WCHAR));
            if (pUserToken) 
            {
                if (GetTokenInformation(hToken, TokenUser, pUserToken, cbTokenBuffer, &cbTokenBuffer)) 
                {
                    DWORD cbAccountName = 0, cbUserDomain = 0;
                    SID_NAME_USE SidNameUse;

                    if (LookupAccountSid(NULL, pUserToken->User.Sid, NULL, &cbAccountName, NULL, &cbUserDomain, &SidNameUse) || 
                        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        LPTSTR pszUserName   = (LPTSTR)LocalAlloc(LPTR, cbAccountName * sizeof(WCHAR));
                        LPTSTR pszUserDomain = (LPTSTR)LocalAlloc(LPTR, cbUserDomain * sizeof(WCHAR));

                        if (pszUserName && pszUserDomain &&
                            LookupAccountSid(NULL, pUserToken->User.Sid, 
                                pszUserName, &cbAccountName, 
                                pszUserDomain, &cbUserDomain, &SidNameUse))
                        {
                            if (ppszUserName)
                            {
                                *ppszUserName = pszUserName;
                                pszUserName = NULL;
                            }
                            if (ppszUserDomain)
                            {
                                *ppszUserDomain = pszUserDomain;
                                pszUserDomain = NULL;
                            }

                            bRet = TRUE;
                        }

                        if (pszUserName)
                            LocalFree(pszUserName);
                        if (pszUserDomain)
                            LocalFree(pszUserDomain);
                    }
                }
                LocalFree(pUserToken);
            }
        }
        CloseHandle(hToken);
    }
    return bRet;
}


/***************************************************************************\
* RegenerateUserEnvironment
*
*
* History:
* 11-5-92  Johannec     Created
*
\***************************************************************************/
BOOL APIENTRY RegenerateUserEnvironment(void **pNewEnv, BOOL bSetCurrentEnv)
{
    NTSTATUS Status;
    WCHAR szValue[1025];
    TCHAR szExpValue[1025];
    void *pEnv = NULL;
    void *pPrevEnv;
    LPTSTR UserName = NULL;
    LPTSTR UserDomain = NULL;
    HKEY  hKey;
    DWORD dwDisp, dwType, dwMaxBufferSize;
    TCHAR szParseAutoexec[MAX_PARSE_AUTOEXEC_BUFFER];
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH+1;



    /*
     * Create a new environment for the user.
     */
    Status = RtlCreateEnvironment((BOOLEAN)FALSE, &pEnv);
    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    SetSystemEnvironmentVariables(&pEnv);

    /*
     * Initialize user's environment.
     */
    if (GetComputerName (szComputerName, &dwComputerNameSize)) {
        SetUserEnvironmentVariable(&pEnv, COMPUTERNAME_VARIABLE, (LPTSTR) szComputerName, TRUE);
    }
    if (GetUserNameAndDomain(&UserName, &UserDomain))
    {
        SetUserEnvironmentVariable( &pEnv, USERNAME_VARIABLE, UserName, TRUE);
        SetUserEnvironmentVariable( &pEnv, USERDOMAIN_VARIABLE, UserDomain, TRUE);
        LocalFree(UserName);
        LocalFree(UserDomain);
    }

    if (GetEnvironmentVariable(USERDNSDOMAIN_VARIABLE, szValue, sizeof(szValue)))
        SetUserEnvironmentVariable( &pEnv, USERDNSDOMAIN_VARIABLE, szValue, TRUE);

    //
    // Set home directory env. vars.
    //
    if (GetEnvironmentVariable(HOMEDRIVE_VARIABLE, szValue, sizeof(szValue)))
        SetUserEnvironmentVariable( &pEnv, HOMEDRIVE_VARIABLE, szValue, TRUE);

    if (GetEnvironmentVariable(HOMESHARE_VARIABLE, szValue, sizeof(szValue)))
        SetUserEnvironmentVariable( &pEnv, HOMESHARE_VARIABLE, szValue, TRUE);

    if (GetEnvironmentVariable(HOMEPATH_VARIABLE, szValue, sizeof(szValue)))
        SetUserEnvironmentVariable( &pEnv, HOMEPATH_VARIABLE, szValue, TRUE);

    //
    // Set the user profile dir env var
    //

    if (GetEnvironmentVariable(USERPROFILE_VARIABLE, szValue, sizeof(szValue)))
        SetUserEnvironmentVariable( &pEnv, USERPROFILE_VARIABLE, szValue, TRUE);


    //
    // Set the program files env var
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwMaxBufferSize = sizeof(szValue);
        if (SHQueryValueEx (hKey, TEXT("ProgramFilesDir"), NULL, &dwType,
                             (LPBYTE) szValue, &dwMaxBufferSize) == ERROR_SUCCESS) {

            SHExpandEnvironmentStrings(szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetUserEnvironmentVariable(&pEnv, PROGRAMFILES_VARIABLE, szExpValue, TRUE);
        }

        dwMaxBufferSize = sizeof(szValue);
        if (SHQueryValueEx (hKey, TEXT("CommonFilesDir"), NULL, &dwType,
                             (LPBYTE) szValue, &dwMaxBufferSize) == ERROR_SUCCESS) {

            SHExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetUserEnvironmentVariable(&pEnv, COMMONPROGRAMFILES_VARIABLE, szExpValue, TRUE);
        }

#ifdef WX86
        dwMaxBufferSize = sizeof(szValue);
        if (SHQueryValueEx (hKey, TEXT("ProgramFilesDir (x86)"), NULL, &dwType,
                             (LPBYTE) szValue, &dwMaxBufferSize) == ERROR_SUCCESS) {

            SHExpandEnvironmentStrings(szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetUserEnvironmentVariable(&pEnv, PROGRAMFILESX86_VARIABLE, szExpValue, TRUE);
        }

        dwMaxBufferSize = sizeof(szValue);
        if (SHQueryValueEx (hKey, TEXT("CommonFilesDir (x86)"), NULL, &dwType,
                             (LPBYTE) szValue, &dwMaxBufferSize) == ERROR_SUCCESS) {

            SHExpandEnvironmentStrings (szValue, szExpValue, ARRAYSIZE(szExpValue));
            SetUserEnvironmentVariable(&pEnv, COMMONPROGRAMFILESX86_VARIABLE, szExpValue, TRUE);
        }

#endif

        RegCloseKey (hKey);
    }

    /*
     * Set 16-bit apps environment variables by processing autoexec.bat
     * User can turn this off and on via the registry.
     */

    //
    // Set the default case, and open the key
    //

    lstrcpy (szParseAutoexec, PARSE_AUTOEXEC_DEFAULT);

    if (RegCreateKeyEx (HKEY_CURRENT_USER, PARSE_AUTOEXEC_KEY, 0, 0,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {


        //
        // Query the current value.  If it doesn't exist, then add
        // the entry for next time.
        //

        dwMaxBufferSize = sizeof (TCHAR) * MAX_PARSE_AUTOEXEC_BUFFER;
        if (SHQueryValueEx (hKey, PARSE_AUTOEXEC_ENTRY, NULL, &dwType,
                        (LPBYTE) szParseAutoexec, &dwMaxBufferSize)
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
    // Process the autoexec if appropriate
    //

    if (szParseAutoexec[0] == TEXT('1')) {
        ProcessAutoexec(&pEnv, PATH_VARIABLE);
    }


    /*
     * Set User environment variables.
     */
    SetEnvironmentVariables( &pEnv, USER_ENV_SUBKEY);

    /*
     * Set User volatile environment variables.
     */
    SetEnvironmentVariables( &pEnv, USER_VOLATILE_ENV_SUBKEY);

    AppendNTPathWithAutoexecPath( &pEnv, PATH_VARIABLE, AUTOEXECPATH_VARIABLE);

    if (bSetCurrentEnv) {
        Status = RtlSetCurrentEnvironment( pEnv, &pPrevEnv);
        if (!NT_SUCCESS(Status)) {
//            RtlDestroyEnvironment(pEnv);
            return(FALSE);
        }
        else {
            RtlDestroyEnvironment(pPrevEnv);
        }
    }

    *pNewEnv = pEnv;

    return(TRUE);
}
