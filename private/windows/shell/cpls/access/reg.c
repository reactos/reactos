/****************************** Module Header ******************************\
* Module Name: reg.c
*
* Copyright (c) 1985-95, Microsoft Corporation
*
* History:
* 01-02-96 a-jimhar 	Created based on reg.c from access351.exe
\***************************************************************************/

/*
1. on startup, check to see if we're administrator
  a) use RegOpenKeyEx on HKEY_USERS\.DEFAULT\Software with read/write
    access writes.  if it fails, we're not administrator
  b) if not, grey menu option
2. on startup
  a) use RegOpenKeyEx on HKEY_CURRENTUSER\Software...
  b) if it fails, create these keys with default values.
3. creating keys
  a) RegCreateKeyEx
  b) RegSetValue
  c) RegCloseKey

*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "Access.h"

BOOL DoAccessRegEntriesExist( HKEY hkeyRoot );
BOOL CheckRegEntry( HKEY hkeyRoot, LPSTR lpsz, REGSAM sam );
LONG OpenAccessRegKeyW( HKEY hkeyRoot, LPSTR lpstr, PHKEY phkey );
BOOL CloseAccessRegKey( HKEY hkey );
BOOL SetRegString( HKEY hkey, LPSTR lpszEntry, LPSTR lpszValue );

DWORD CopyKey( HKEY hkeySrc, HKEY hkeyDst, LPSTR szKey );


char szAccessRegPath[] = "Control Panel\\Accessibility";
char szHcColorRegPath[] = "Control Panel\\Colors";
char szHcDeskRegPath[] = "Control Panel\\Desktop";

/********************************************************************/
//
BOOL IsDefaultWritable( void )
{
    return CheckRegEntry( HKEY_USERS, ".Default", KEY_ALL_ACCESS );
}

/********************************************************************/
BOOL DoAccessRegEntriesExist( HKEY hkeyRoot )
{
    char sz[128];
    strcpy( sz, szAccessRegPath );
    strcat( sz, "\\StickyKeys" );
    return CheckRegEntry( hkeyRoot, sz, KEY_READ ); // execute means readonly
}

/********************************************************************/
BOOL CheckRegEntry( HKEY hkeyRoot, LPSTR lpsz, REGSAM sam )
{
    HKEY hkey;
    BOOL fOk = (ERROR_SUCCESS == RegOpenKeyExA( hkeyRoot, lpsz, 0, sam, &hkey ));

    if(fOk)
    {
        RegCloseKey(hkey);
    }
    
	return fOk;
}

/********************************************************************/
LONG OpenAccessRegKeyW( HKEY hkeyRoot, LPSTR lpstr, PHKEY phkey )
{
    LONG dw;
    LONG dwDisposition;
    char sz[128];
    strcpy( sz, szAccessRegPath );
    strcat( sz, "\\" );
    strcat( sz, lpstr );
//    dw = RegOpenKey( hkeyRoot, sz, phkey );
//    dw = RegOpenKey( hkeyRoot, "\\Software", phkey );
//    dw = RegOpenKey( hkeyRoot, "\\FOOBAR", phkey );
//    dw = RegOpenKey( hkeyRoot, "Software", phkey );
//    dw = RegOpenKey( hkeyRoot, "FOOBAR", phkey );
//    dw = RegOpenKey( hkeyRoot, "Software\\Microsoft\\Accessibility\\StickyKeys", phkey );
    dw = RegCreateKeyExA( hkeyRoot,
            sz,
            0,
            NULL,                // CLASS NAME??
            0,                   // by default is non-volatile
            KEY_ALL_ACCESS,
            NULL,                // default security descriptor
            phkey,
            &dwDisposition );    // yes we throw this away
    if( dw != ERROR_SUCCESS )
    {
        // should do something
    }
    return dw;
}

/********************************************************************/
BOOL CloseAccessRegKey( HKEY hkey )
{
    DWORD dw;
    dw = RegCloseKey( hkey );
    if( dw == ERROR_SUCCESS )
        return TRUE;
    else
        return FALSE;
}

/********************************************************************/
BOOL SetRegString( HKEY hkey, LPSTR lpszEntry, LPSTR lpszValue )
{
    DWORD dwResult;
    dwResult = RegSetValueExA( hkey,
                              lpszEntry,
                              0,
                              REG_SZ,
                              lpszValue,
                              strlen( lpszValue ) + sizeof( TCHAR ) );
    if( dwResult != ERROR_SUCCESS )
    {
        ; // should do something like print a message
        return FALSE;
    }
    else
        return TRUE;
}

/***********************************************************************/
#define TEMP_PROFILE     "Temp profile (access.cpl)"

typedef BOOL (*PFNGETDEFAULTUSERPROFILEDIRECTORYA)(LPSTR lpProfile, LPDWORD dwSize);

DWORD SaveDefaultSettings( BOOL saveL, BOOL saveU )
{
    NTSTATUS Status;
    DWORD iStatus, dwSize;
    HKEY hkeyDst;
    BOOLEAN WasEnabled;
    char acFile[MAX_PATH];
    HANDLE hInstDll;
    PFNGETDEFAULTUSERPROFILEDIRECTORYA pfnGetDefaultUserProfileDirectory;

    // If save to Logon
    if ( saveL )
    {
        iStatus  = RegOpenKeyA( HKEY_USERS, ".DEFAULT", &hkeyDst );
        if( iStatus != ERROR_SUCCESS )
            return iStatus;
        iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szAccessRegPath );
    
        // a-anilk 
        // Now copy the colors and Desktop to .Default required for HighContrast setting
        iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szHcColorRegPath );
        iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szHcDeskRegPath );

        RegCloseKey( hkeyDst );
    }

    if ( saveU )
    {
        hInstDll = LoadLibrary (TEXT("userenv.dll"));

        if (!hInstDll) {
            return (GetLastError());
        }
        pfnGetDefaultUserProfileDirectory = (PFNGETDEFAULTUSERPROFILEDIRECTORYA)GetProcAddress (hInstDll,
                                            "GetDefaultUserProfileDirectoryA");

        if (!pfnGetDefaultUserProfileDirectory) {
            FreeLibrary (hInstDll);
            return (GetLastError());
        }

        dwSize = MAX_PATH;
        if (!pfnGetDefaultUserProfileDirectory(acFile, &dwSize)) {
            FreeLibrary (hInstDll);
            return (GetLastError());
        }

        FreeLibrary (hInstDll);

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

        if (!NT_SUCCESS(Status)) return iStatus;

        strcat(acFile,"\\ntuser.dat");
        iStatus = RegLoadKeyA(HKEY_USERS, TEMP_PROFILE, acFile);

        if (iStatus == ERROR_SUCCESS) {

            iStatus  = RegOpenKeyA( HKEY_USERS, TEMP_PROFILE, &hkeyDst );
            if( iStatus == ERROR_SUCCESS ) {

                iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szAccessRegPath );
                // a-anilk 
                // Now copy the colors and Desktop to .Default required for HighContrast setting
                iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szHcColorRegPath );
                iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szHcDeskRegPath );

                RegCloseKey( hkeyDst );
            }

            RegUnLoadKeyA(HKEY_USERS, TEMP_PROFILE);
        }
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    }
    return iStatus;
}

/***********************************************************************/
// CopyKey( hKey, hKeyDst, name )
//     create the destination key
//     for each value
//         CopyValue
//     for each subkey
//         CopyKey

DWORD CopyKey( HKEY hkeySrc, HKEY hkeyDst, LPSTR szKey )
{
    HKEY hkeyOld, hkeyNew;
    char szValue[128];
    char szData[128];
    char szBuffer[128];
    DWORD iStatus;
    UINT nValue, nKey;
    UINT iValueLen, iDataLen;
	DWORD dwType;

    iStatus = RegOpenKeyA( hkeySrc, szKey, &hkeyOld );
    if( iStatus != ERROR_SUCCESS )
        return iStatus;
    iStatus = RegOpenKeyA( hkeyDst, szKey, &hkeyNew );
    if( iStatus != ERROR_SUCCESS )
    {
        iStatus = RegCreateKeyA( hkeyDst, szKey, &hkeyNew );
        if( iStatus != ERROR_SUCCESS )
        {
            RegCloseKey( hkeyOld );
            return iStatus;
        }
    }
    //*********** copy the values **************** //

    for( nValue = 0, iValueLen=sizeof szValue, iDataLen=sizeof szValue;
         ERROR_SUCCESS == (iStatus = RegEnumValueA(hkeyOld,
                                                  nValue,
                                                  szValue,
                                                  &iValueLen,
                                                  NULL, // reserved
                                                  &dwType, // don't need type
                                                  szData,
                                                  &iDataLen ) );
         nValue ++, iValueLen=sizeof szValue, iDataLen=sizeof szValue )
     {
         iStatus = RegSetValueExA( hkeyNew,
                                  szValue,
                                  0, // reserved
                                  dwType,
                                  szData,
                                  iDataLen);
     }
    if( iStatus != ERROR_NO_MORE_ITEMS )
    {
        RegCloseKey( hkeyOld );
        RegCloseKey( hkeyNew );
        return iStatus;
    }

    //*********** copy the subtrees ************** //

    for( nKey = 0;
         ERROR_SUCCESS == (iStatus = RegEnumKeyA(hkeyOld,nKey,szBuffer,sizeof(szBuffer)));
         nKey ++ )
     {
         iStatus = CopyKey( hkeyOld, hkeyNew, szBuffer );
         if( iStatus != ERROR_NO_MORE_ITEMS && iStatus != ERROR_SUCCESS )
            {
                RegCloseKey( hkeyOld );
                RegCloseKey( hkeyNew );
                return iStatus;
            }
     }
    RegCloseKey( hkeyOld );
    RegCloseKey( hkeyNew );
    if( iStatus == ERROR_NO_MORE_ITEMS )
        return ERROR_SUCCESS;
    else
        return iStatus;
}
