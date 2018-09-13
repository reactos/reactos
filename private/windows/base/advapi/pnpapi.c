
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpapi.c

Abstract:

    This module contains the user-mode plug-and-play API stubs.

Author:

    Paula Tomlinson (paulat) 9-18-1995

Environment:

    User-mode only.

Revision History:

    18-Sept-1995     paulat

        Creation and initial implementation.

--*/

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define MAX_PROFILEID_LEN     5
#define MAX_UUID_LEN          39


//
// includes
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>
#include <stdlib.h>
#include <stdio.h>
#include <wtypes.h>


//
// private prototypes
//
PSTR
UnicodeToMultiByte(
    IN PCWSTR UnicodeString,
    IN UINT   Codepage
    );

PWSTR
MultiByteToUnicode(
    IN PCSTR String,
    IN UINT  Codepage
    );

RPC_STATUS
GuidToString(
   UUID   *Uuid,
   LPTSTR StringGuid
   );


//
// global data
//
WCHAR pszRegIDConfigDB[] = L"System\\CurrentControlSet\\Control\\IDConfigDB";

WCHAR pszRegKnownDockingStates[] =  L"Hardware Profiles";
WCHAR pszRegCurrentConfig[] =       L"CurrentConfig";
WCHAR pszRegDefaultFriendlyName[] = L"Noname Hardware Profile";
WCHAR pszRegHwProfileGuid[] =       L"HwProfileGuid";
WCHAR pszRegFriendlyName[] =        L"FriendlyName";
WCHAR pszRegDockState[] =           L"DockState";
WCHAR pszRegDockingState[] =        L"DockingState";
WCHAR pszCurrentDockInfo[] =        L"CurrentDockInfo";


BOOL
GetCurrentHwProfileW (
    OUT LPHW_PROFILE_INFOW  lpHwProfileInfo
    )

/*++

Routine Description:


Arguments:

    lpHwProfileInfo  Points to a HW_PROFILE_INFO structure that will receive
                     the information for the current hardware profile.

Return Value:

    If the function succeeds, the return value is TRUE.  If the function
    fails, the return value is FALSE.  To get extended error information,
    call GetLastError.

--*/

{
   BOOL     Status = TRUE;
   WCHAR    RegStr[MAX_PATH];
   HKEY     hKey = NULL, hCfgKey = NULL;
   HKEY     hCurrentDockInfoKey = NULL;
   ULONG    ulCurrentConfig = 1, ulSize = 0;
   UUID     NewGuid;


   try {
      //
      // validate parameter
      //
      if (lpHwProfileInfo == NULL) {
         SetLastError(ERROR_INVALID_PARAMETER);
         Status = FALSE;
         goto Clean0;
      }
      

      //
      // open the IDConfigDB key
      //
      if (RegOpenKeyEx(
               HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
               KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
	
         SetLastError(ERROR_REGISTRY_CORRUPT);
         Status = FALSE;
         goto Clean0;
      }
      
      //
      // retrieve the current config id
      //
      ulSize = sizeof(ULONG);
      if (RegQueryValueEx(hKey, pszRegCurrentConfig, NULL, NULL,
                          (LPBYTE)&ulCurrentConfig, &ulSize) != ERROR_SUCCESS) {          
          SetLastError(ERROR_REGISTRY_CORRUPT);
          Status = FALSE;
          goto Clean0;
      }

      //
      // open the profile key for the current configuration
      //
      _snwprintf(RegStr, MAX_PATH, L"%s\\%04u",
                 pszRegKnownDockingStates,
                 ulCurrentConfig);
      
      if (RegOpenKeyEx(hKey, RegStr, 0, KEY_QUERY_VALUE,
                       &hCfgKey) != ERROR_SUCCESS) {
          SetLastError(ERROR_REGISTRY_CORRUPT);
          Status = FALSE;
          goto Clean0;
      }
      
      //
      // retrieve the dock state for the current profile
      //      
      if (RegOpenKeyEx(hKey, pszCurrentDockInfo, 0,  KEY_QUERY_VALUE,
                       &hCurrentDockInfoKey) != ERROR_SUCCESS) {
          //
          // No CurrentDockInfo Key, something's wrong.
          //
          SetLastError(ERROR_REGISTRY_CORRUPT);
          Status = FALSE;
          goto Clean0;
      }      
      
      //
      // Look in CurrentDockInfo for
      // a hardware determined DockingState value.
      //
      ulSize = sizeof(ULONG);                  
      if ((RegQueryValueEx(hCurrentDockInfoKey,
                           pszRegDockingState, 
                           NULL, 
                           NULL,
                           (LPBYTE)&lpHwProfileInfo->dwDockInfo,
                           &ulSize) != ERROR_SUCCESS)
          || (!lpHwProfileInfo->dwDockInfo) 
          || ((lpHwProfileInfo->dwDockInfo & DOCKINFO_UNDOCKED) && 
              (lpHwProfileInfo->dwDockInfo & DOCKINFO_DOCKED))) {          

          //
          // If there's no such value, or the value was set to 0 (unspported),
          // or if the value is "unknown", resort to user supplied docking info.
          // Look under the IDConfigDB profile for a user set DockState value.
          //                    
          if ((RegQueryValueEx(hCfgKey, pszRegDockState, NULL, NULL,
                               (LPBYTE)&lpHwProfileInfo->dwDockInfo,
                               &ulSize) != ERROR_SUCCESS)
              || (!lpHwProfileInfo->dwDockInfo)) {
              
              //
              // If there's no such value, or the value was set to 0,
              // there is no user specified docking state to resort to;
              // return the "user-supplied unknown" docking state.
              //
              lpHwProfileInfo->dwDockInfo =
                  DOCKINFO_USER_SUPPLIED | DOCKINFO_DOCKED | DOCKINFO_UNDOCKED;
          }
      }
      
      //
      // retrieve the profile guid.  if we can't get one, set it to NULL
      //
      ulSize = HW_PROFILE_GUIDLEN * sizeof(WCHAR);
      if (RegQueryValueEx(hCfgKey, pszRegHwProfileGuid, NULL, NULL,
                          (LPBYTE)&lpHwProfileInfo->szHwProfileGuid,
                          &ulSize) != ERROR_SUCCESS) {
          lpHwProfileInfo->szHwProfileGuid[0] = '\0';
      }
      
      //
      // retrieve the friendly name.  if we can't get one, set it to NULL
      //
      ulSize = MAX_PROFILE_LEN * sizeof(WCHAR);
      if (RegQueryValueEx(hCfgKey, pszRegFriendlyName, NULL, NULL,
                          (LPBYTE)&lpHwProfileInfo->szHwProfileName,
                          &ulSize) != ERROR_SUCCESS) {
          lpHwProfileInfo->szHwProfileName[0] = '\0';
      }
      
      
   Clean0:
      ;
      
   } except(EXCEPTION_EXECUTE_HANDLER) {
       
       SetLastError(ERROR_INVALID_PARAMETER);
       Status = FALSE;
   }
   
   if (hKey != NULL) RegCloseKey(hKey);
   if (hCfgKey != NULL) RegCloseKey(hCfgKey);
   if (hCurrentDockInfoKey != NULL) RegCloseKey(hCurrentDockInfoKey);
   
   return Status;
   
} // GetCurrentHwProfileW





BOOL
GetCurrentHwProfileA (
    OUT LPHW_PROFILE_INFOA  lpHwProfileInfo
    )

/*++

Routine Description:


Arguments:

    lpHwProfileInfo  Points to a HW_PROFILE_INFO structure that will receive
                     the information for the current hardware profile.

Return Value:

    If the function succeeds, the return value is TRUE.  If the function
    fails, the return value is FALSE.  To get extended error information,
    call GetLastError.

--*/

{
   BOOL              Status = TRUE;
   HW_PROFILE_INFOW  HwProfileInfoW;
   LPSTR             pAnsiString;


   try {
      //
      // validate parameter
      //
      if (lpHwProfileInfo == NULL) {
         SetLastError(ERROR_INVALID_PARAMETER);
         Status = FALSE;
         goto Clean0;
      }

      //
      // call the Unicode version
      //
      if (!GetCurrentHwProfileW(&HwProfileInfoW)) {
         Status = FALSE;
         goto Clean0;
      }

      //
      // on successful return, convert unicode form of struct
      // to ANSI and copy struct members to callers struct
      //
      lpHwProfileInfo->dwDockInfo = HwProfileInfoW.dwDockInfo;

      pAnsiString = UnicodeToMultiByte(
               HwProfileInfoW.szHwProfileGuid, CP_ACP);
      lstrcpyA(lpHwProfileInfo->szHwProfileGuid, pAnsiString);
      LocalFree(pAnsiString);

      pAnsiString = UnicodeToMultiByte(
               HwProfileInfoW.szHwProfileName, CP_ACP);
      lstrcpyA(lpHwProfileInfo->szHwProfileName, pAnsiString);
      LocalFree(pAnsiString);


      Clean0:
         ;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      SetLastError(ERROR_INTERNAL_ERROR);
      Status = FALSE;
   }

   return Status;

} // GetCurrentHwProfileA





PSTR
UnicodeToMultiByte(
    IN PCWSTR UnicodeString,
    IN UINT   Codepage
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if out of memory or invalid codepage.
    Caller can free buffer with MyFree().

--*/

{
    UINT WideCharCount;
    PSTR String;
    UINT StringBufferSize;
    UINT BytesInString;
    PSTR p;

    WideCharCount = lstrlenW(UnicodeString) + 1;

    //
    // Allocate maximally sized buffer.
    // If every unicode character is a double-byte
    // character, then the buffer needs to be the same size
    // as the unicode string. Otherwise it might be smaller,
    // as some unicode characters will translate to
    // single-byte characters.
    //
    StringBufferSize = WideCharCount * sizeof(WCHAR);
    String = (PSTR)LocalAlloc(LPTR, StringBufferSize);
    if(String == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    BytesInString = WideCharToMultiByte(
                        Codepage,
                        0,                      // default composite char behavior
                        UnicodeString,
                        WideCharCount,
                        String,
                        StringBufferSize,
                        NULL,
                        NULL
                        );

    if(BytesInString == 0) {
        LocalFree(String);
        return(NULL);
    }

    //
    // Resize the string's buffer to its correct size.
    // If the realloc fails for some reason the original
    // buffer is not freed.
    //
    if(p = LocalReAlloc(String,BytesInString, LMEM_ZEROINIT | LMEM_MOVEABLE)) {
        String = p;
    }

    return(String);

} // UnicodeToMultiByte




PWSTR
MultiByteToUnicode(
    IN PCSTR String,
    IN UINT  Codepage
    )

/*++

Routine Description:

    Convert a string to unicode.

Arguments:

    String - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if string could not be converted (out of memory or invalid cp)
    Caller can free buffer with MyFree().

--*/

{
    UINT BytesIn8BitString;
    UINT CharsInUnicodeString;
    PWSTR UnicodeString;
    PWSTR p;

    BytesIn8BitString = lstrlenA(String) + 1;

    //
    // Allocate maximally sized buffer.
    // If every character is a single-byte character,
    // then the buffer needs to be twice the size
    // as the 8bit string. Otherwise it might be smaller,
    // as some characters are 2 bytes in their unicode and
    // 8bit representations.
    //
    UnicodeString = (PWSTR)LocalAlloc(LPTR, BytesIn8BitString * sizeof(WCHAR));
    if(UnicodeString == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    CharsInUnicodeString = MultiByteToWideChar(
                                Codepage,
                                MB_PRECOMPOSED,
                                String,
                                BytesIn8BitString,
                                UnicodeString,
                                BytesIn8BitString
                                );

    if(CharsInUnicodeString == 0) {
        LocalFree(UnicodeString);
        return(NULL);
    }

    //
    // Resize the unicode string's buffer to its correct size.
    // If the realloc fails for some reason the original
    // buffer is not freed.
    //
    if(p = (PWSTR)LocalReAlloc(UnicodeString,CharsInUnicodeString*sizeof(WCHAR),
            LMEM_ZEROINIT | LMEM_MOVEABLE)) {

        UnicodeString = p;
    }

    return(UnicodeString);

} // MultiByteToUnicode




RPC_STATUS
GuidToString(
   UUID   *Uuid,
   LPTSTR StringGuid
   )
{
   RPC_STATUS  Status;
   LPTSTR      pTempStringGuid;
   UNICODE_STRING UnicodeStringGuid;

   Status = UuidToString(Uuid, &pTempStringGuid);

   if (Status == RPC_S_OK) {
      //
      // the form we want is all uppercase and with curly brackets around,
      // like what OLE does
      //
      lstrcpy(StringGuid, TEXT("{"));
      lstrcat(StringGuid, pTempStringGuid);
      lstrcat(StringGuid, TEXT("}"));
      RtlInitUnicodeString(&UnicodeStringGuid, StringGuid);
      RtlUpcaseUnicodeString(&UnicodeStringGuid, &UnicodeStringGuid, FALSE);

      RpcStringFree(&pTempStringGuid);
   }

   return Status;

} // GuidToString
