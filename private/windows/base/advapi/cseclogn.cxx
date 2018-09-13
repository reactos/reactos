/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 1998.
 *
 *  Name : cseclogn.cxx
 *  Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the client side for Secondary Logon Service
 * implemented as CreateProcessWithLogon API
 * in advapi32.dll
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/

#define UNICODE

#define SECURITY_WIN32

#include <Windows.h>
#include "seclogon.h"
#include "security.h"

//
// must move to winbase.h soon!
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002

//
// Global function pointers to user32 functions
// This is to dynamically load user32 when CreateProcessWithLogon
// is called.
//

HMODULE hModule1 = NULL;
HMODULE hModule2 = NULL;


typedef HDESK (*OPENDESKTOP) (
    LPWSTR lpszDesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess);

OPENDESKTOP myOpenDesktop = NULL;

typedef HDESK (*GETTHREADDESKTOP)(
    DWORD dwThreadId);

GETTHREADDESKTOP    myGetThreadDesktop = NULL;

typedef BOOL (*CLOSEDESKTOP)(
    HDESK hDesktop);

CLOSEDESKTOP    myCloseDesktop = NULL;

typedef HWINSTA (*OPENWINDOWSTATION)(
    LPWSTR lpszWinSta,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess);

OPENWINDOWSTATION   myOpenWindowStation = NULL;

typedef HWINSTA (*GETPROCESSWINDOWSTATION)(
    VOID);

GETPROCESSWINDOWSTATION myGetProcessWindowStation = NULL;

typedef BOOL (*CLOSEWINDOWSTATION)(
    HWINSTA hWinSta);

CLOSEWINDOWSTATION  myCloseWindowStation = NULL;

typedef BOOL (*SETUSEROBJECTSECURITY)(
    HANDLE hObj,
    PSECURITY_INFORMATION pSIRequested,
    PSECURITY_DESCRIPTOR pSID);

SETUSEROBJECTSECURITY   mySetUserObjectSecurity = NULL;

typedef BOOL (*GETUSEROBJECTSECURITY)(
    HANDLE hObj,
    PSECURITY_INFORMATION pSIRequested,
    PSECURITY_DESCRIPTOR pSID,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);

GETUSEROBJECTSECURITY  myGetUserObjectSecurity = NULL;

typedef BOOL (*GETUSEROBJECTINFORMATION)(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);

GETUSEROBJECTINFORMATION    myGetUserObjectInformation = NULL;

typedef BOOL (*TRANSLATENAME)(
   LPCWSTR lpAccountName,
   EXTENDED_NAME_FORMAT AccountNameFormat,
   EXTENDED_NAME_FORMAT DesiredNameFormat,
   LPWSTR lpTranslatedName,
   PULONG nSize);

TRANSLATENAME   myTranslateName = NULL;
    
/*
FARPROC myOpenDesktop = NULL;
FARPROC myGetThreadDesktop = NULL;
FARPROC myCloseDesktop = NULL;

FARPROC myOpenWindowStation = NULL;
FARPROC myGetProcessWindowStation = NULL;
FARPROC myCloseWindowStation = NULL;

FARPROC myGetUserObjectSecurity = NULL;
FARPROC mySetUserObjectSecurity = NULL;
*/


DWORD
AllowDesktopAccessToUser(
    LPTSTR  WinstaName,
    LPTSTR  DeskName,
    LPCTSTR  UserName,
    LPCTSTR  DomainName
    )

/*++

Routine Description:
   This routine opens the desktop defined in lpStartupInfo, modifies DACL on it
   to grant user in the Token full control

Arguments:
   hToken -- handle to the user's token
   lpStartupInfo -- process startup structure that contains the desktop name

Return Value:
   None.

--*/
{
   PSECURITY_DESCRIPTOR Sd = NULL;
   PACL Dacl;
   DWORD Needed;
   DWORD ReturnLen;
   BOOL  DaclPresent, DaclDefaulted;
   TCHAR   AccountName[MAX_PATH];
   TCHAR   RefDom[MAX_PATH];
   DWORD    DomNameLen = MAX_PATH;
   BYTE     SidBuff[256];
   DWORD    BuffLen = 256;
   SID_NAME_USE Use;
   SECURITY_INFORMATION SeReq;
   ACL_SIZE_INFORMATION    AclSize;
   HDESK hDesk;
   HWINSTA hWinsta;
   DWORD    Length;
   DWORD    i;
   HANDLE   hToken;

   hWinsta = myOpenWindowStation( WinstaName, TRUE, MAXIMUM_ALLOWED);
   if(!hWinsta)
   {
      //
      // couldn't open the windowstation from service context!
      // we can just return and see how far this gets.
      //
      return GetLastError();
   }


   //
   // Query the security descriptor
   //
   SeReq = DACL_SECURITY_INFORMATION;

   if(!myGetUserObjectSecurity(hWinsta, &SeReq, Sd, 0, &Needed))
   {
      // allocate buffer large for returned SD and another ACE.
       Sd = (PSECURITY_DESCRIPTOR) HeapAlloc (GetProcessHeap(), 0, Needed + 100);
       if(!Sd)
       {
          //
          // Heap allocation failed.
          // can't go forward.. must return.
          //
          myCloseWindowStation(hWinsta);
          return GetLastError();
       }
       if(!myGetUserObjectSecurity(hWinsta, &SeReq, Sd, Needed, &Needed))
       {
          myCloseWindowStation(hWinsta);
          HeapFree(GetProcessHeap(), 0, Sd);
          return GetLastError();
       }
   }

   //
   // If it is a downlevel name Domain\Name, then we can do this.
   //
   if(DomainName)
   {
        lstrcpy(AccountName,DomainName);
        lstrcat(AccountName,L"\\");
        lstrcat(AccountName,UserName);
   }
   else
   {
     //
     // otherwise we first have to find the downlevel name
     // before we can proceed.
     // Lets translate name and then do the lookup
     //

   }

    if(!LookupAccountName(NULL,AccountName, (PSID) SidBuff, &BuffLen,
                         RefDom, &DomNameLen, &Use))
    {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
    }

   if(!GetSecurityDescriptorDacl(Sd, &DaclPresent, &Dacl, &DaclDefaulted))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   if(Dacl == NULL)
   {
        //
        // if Dacl is null, we don't need to do anything
        // because it gives WORLD full control...
        //
        myCloseWindowStation(hWinsta);
        HeapFree(GetProcessHeap(), 0, Sd);
        return ERROR_SUCCESS;
   }
   //
   // We should persist the 100 bytes added to the buffer only if
   // the free bytes in the DACL are less than 100 bytes
   //
   if(!GetAclInformation(Dacl, (PVOID)&AclSize, sizeof(AclSize), AclSizeInformation))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   if(AclSize.AclBytesFree < 100) Dacl->AclSize += 100;

   if(!AddAccessAllowedAce(Dacl, ACL_REVISION, 0xffffffff,
                                    (PSID)(SidBuff)))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // set the security descriptor
   //
   if(!mySetUserObjectSecurity(hWinsta, &SeReq, Sd))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // close the handle
   //
   myCloseWindowStation(hWinsta);
   HeapFree(GetProcessHeap(), 0, Sd);


   hDesk = myOpenDesktop( DeskName, 0, TRUE, MAXIMUM_ALLOWED);
   if(!hDesk)
   {
      //
      // couldn't open the desktop from service context!
      // we can just return and see how far this gets.
      //
      return GetLastError();
   }

   //
   // Query the security descriptor
   //
   SeReq = DACL_SECURITY_INFORMATION;

   if(!myGetUserObjectSecurity(hDesk, &SeReq, Sd, 0, &Needed))
   {
      // allocate buffer large for returned SD and another ACE.
       Sd = (PSECURITY_DESCRIPTOR) HeapAlloc (GetProcessHeap(), 0, Needed + 100);
       if(!Sd)
       {
          //
          // Heap allocation failed.
          // can't go forward.. must return.
          //
          myCloseDesktop(hDesk);
          return GetLastError();
       }
       if(!myGetUserObjectSecurity(hDesk, &SeReq, Sd, Needed, &Needed))
       {
          myCloseDesktop(hDesk);
          HeapFree(GetProcessHeap(), 0, Sd);
          return GetLastError();
       }
   }


   if(!GetSecurityDescriptorDacl(Sd, &DaclPresent, &Dacl, &DaclDefaulted))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   if(Dacl == NULL)
   {
        //
        // if Dacl is null, we don't need to do anything
        // because it gives WORLD full control...
        //
        myCloseDesktop(hDesk);
        HeapFree(GetProcessHeap(), 0, Sd);
        return ERROR_SUCCESS;
   }

   //
   // We should persist the 100 bytes added to the buffer only if
   // the free bytes in the DACL are less than 100 bytes
   //
   if(!GetAclInformation(Dacl, (PVOID)&AclSize, sizeof(AclSize), AclSizeInformation))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   if(AclSize.AclBytesFree < 100) Dacl->AclSize += 100;

   if(!AddAccessAllowedAce(Dacl, ACL_REVISION, 0xffffffff,
                                    (PSID)(SidBuff)))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // set the security descriptor
   //
   if(!mySetUserObjectSecurity(hDesk, &SeReq, Sd))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // close the handle
   //
   myCloseDesktop(hDesk);
   HeapFree(GetProcessHeap(), 0, Sd);
   return ERROR_SUCCESS;
}


DWORD
RemoveDesktopAccessFromUser(
    LPTSTR  WinstaName,
    LPTSTR  DeskName,
    LPCTSTR  UserName,
    LPCTSTR  DomainName
    )
/*++

Routine Description:
   This routine opens the desktop defined in lpStartupInfo, modifies DACL on it
   to remove user ACE.

Arguments:
   hToken -- handle to the user's token
   lpStartupInfo -- process startup structure that contains the desktop name

Return Value:
   None.

--*/
{
   PSECURITY_DESCRIPTOR Sd = NULL;
   DWORD Needed;
   TCHAR   AccountName[MAX_PATH];
   TCHAR   RefDom[MAX_PATH];
   DWORD    DomNameLen = MAX_PATH;
   BYTE     SidBuff[256];
   DWORD    BuffLen = 256;
   SECURITY_INFORMATION SeReq;
   PACL  Dacl;
   BOOL  DaclPresent, DaclDefaulted;
   ACL_SIZE_INFORMATION DaclSize;
   PACCESS_ALLOWED_ACE  Ace = NULL;
   DWORD i;
   HDESK    hDesk;
   HWINSTA  hWinsta;
   SID_NAME_USE Use;


   hWinsta = myOpenWindowStation( WinstaName, TRUE, MAXIMUM_ALLOWED);
   if(!hWinsta)
   {
      //
      // couldn't open the windowstation from service context!
      // we can just return and see how far this gets.
      //
      return GetLastError();
   }

   lstrcpy(AccountName,DomainName);
   lstrcat(AccountName,L"\\");
   lstrcat(AccountName,UserName);

   if(!LookupAccountName(NULL,AccountName, (PSID) SidBuff, &BuffLen,
                        RefDom, &DomNameLen, &Use))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // Query the security descriptor
   //
   SeReq = DACL_SECURITY_INFORMATION;
   if(!myGetUserObjectSecurity(hWinsta, &SeReq, Sd, 0, &Needed))
   {
      // allocate buffer large for returned SD and another ACE.
       Sd = (PBYTE) HeapAlloc (GetProcessHeap(), 0, Needed + 40);
       if(!Sd)
       {
          //
          // Heap allocation failed.
          // can't go forward.. must return.
          //
          myCloseWindowStation(hWinsta);
          return GetLastError();
       }
       if(!myGetUserObjectSecurity(hWinsta, &SeReq, Sd, Needed, &Needed))
       {
          myCloseWindowStation(hWinsta);
          HeapFree(GetProcessHeap(), 0, Sd);
          return GetLastError();
       }
   }

   if(!GetSecurityDescriptorDacl(Sd, &DaclPresent, &Dacl, &DaclDefaulted))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   if(Dacl == NULL)
   {
        //
        // if Dacl is null, we don't need to do anything
        // because it gives WORLD full control...
        //
        myCloseWindowStation(hWinsta);
        HeapFree(GetProcessHeap(), 0, Sd);
        return ERROR_SUCCESS;
   }
   //
   // delete the first ACE found for this User.
   //
   if(!GetAclInformation(Dacl, &DaclSize, sizeof(DaclSize), AclSizeInformation))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   for(i=0;i<DaclSize.AceCount;i++)
   {
      if(!GetAce(Dacl, i, (PVOID*)(&Ace)))
      {
         myCloseWindowStation(hWinsta);
         HeapFree(GetProcessHeap(), 0, Sd);
         return GetLastError();
      }
      if(EqualSid( (PSID)(SidBuff), (PSID)(&(Ace->SidStart))))
      {
         if(!DeleteAce(Dacl, i))
         {
            myCloseWindowStation(hWinsta);
            HeapFree(GetProcessHeap(), 0, Sd);
            return GetLastError();
         }

         break;
      }
   }


   //
   // set the security descriptor
   //
   if(!mySetUserObjectSecurity(hWinsta, &SeReq, Sd))
   {
      myCloseWindowStation(hWinsta);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // close the handle
   //
   myCloseWindowStation(hWinsta);
   HeapFree(GetProcessHeap(), 0, Sd);


   hDesk = myOpenDesktop( DeskName, 0, TRUE, MAXIMUM_ALLOWED);
   if(!hDesk)
   {
      //
      // couldn't open the desktop from service context!
      // we can just return and see how far this gets.
      //
      return GetLastError();
   }

   //
   // Query the security descriptor
   //
   SeReq = DACL_SECURITY_INFORMATION;
   if(!myGetUserObjectSecurity(hDesk, &SeReq, Sd, 0, &Needed))
   {
      // allocate buffer large for returned SD and another ACE.
       Sd = (PBYTE) HeapAlloc (GetProcessHeap(), 0, Needed + 40);
       if(!Sd)
       {
          //
          // Heap allocation failed.
          // can't go forward.. must return.
          //
          myCloseDesktop(hDesk);
          return GetLastError();
       }
       if(!myGetUserObjectSecurity(hDesk, &SeReq, Sd, Needed, &Needed))
       {
          myCloseDesktop(hDesk);
          HeapFree(GetProcessHeap(), 0, Sd);
          return GetLastError();
       }
   }

   if(!GetSecurityDescriptorDacl(Sd, &DaclPresent, &Dacl, &DaclDefaulted))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   if(Dacl == NULL)
   {
        //
        // if Dacl is null, we don't need to do anything
        // because it gives WORLD full control...
        //
        myCloseDesktop(hDesk);
        HeapFree(GetProcessHeap(), 0, Sd);
        return ERROR_SUCCESS;
   }
   //
   // delete the first ACE found for this User.
   //
   if(!GetAclInformation(Dacl, &DaclSize, sizeof(DaclSize), AclSizeInformation))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   for(i=0;i<DaclSize.AceCount;i++)
   {
      if(!GetAce(Dacl, i, (PVOID*)(&Ace)))
      {
         myCloseDesktop(hDesk);
         HeapFree(GetProcessHeap(), 0, Sd);
         return GetLastError();
      }
      if(EqualSid( (PSID)(SidBuff), (PSID)(&(Ace->SidStart))))
      {
         if(!DeleteAce(Dacl, i))
         {
            myCloseDesktop(hDesk);
            HeapFree(GetProcessHeap(), 0, Sd);
            return GetLastError();
         }

         break;
      }
   }


   //
   // set the security descriptor
   //
   if(!mySetUserObjectSecurity(hDesk, &SeReq, Sd))
   {
      myCloseDesktop(hDesk);
      HeapFree(GetProcessHeap(), 0, Sd);
      return GetLastError();
   }

   //
   // close the handle
   //
   myCloseDesktop(hDesk);
   HeapFree(GetProcessHeap(), 0, Sd);
   return ERROR_SUCCESS;
}




static BOOL
WINAPI
FreePackedStructAndData(
      PVOID pvPackedStructAndData)
{
   return(HeapFree(GetProcessHeap(), 0, pvPackedStructAndData));
}

static PVOID
__cdecl
PackStructAndDataW(
      PDWORD pcbBytesToSend,
      PVOID pvStruct,
      int cbStruct,
      int nDataPtrOffset0,
      int nDataLen0, ...)

/*++

Routine Description:

Arguments:

Return Value:

--*/
{

   int nNumDataElems = 0, cbPackedData = 0;
   int nDataPtrOffset = nDataPtrOffset0, nDataLen = nDataLen0;
   va_list va_params;

   va_start(va_params, nDataLen0);
   while (nDataPtrOffset != PSAD_NO_MORE_DATA) {
      nNumDataElems++;

      LPBYTE pbUnpackedData = * (LPBYTE*) ((PBYTE) pvStruct + nDataPtrOffset);
      if (pbUnpackedData == NULL) {
         //
         // User passed a NULL pointer
         //
         nDataLen = 0;
      } else {
         if (nDataLen == PSAD_STRING_DATA) {
            //
            // We are assuming that this is a string,
            // calc the number of bytes in the string
            //
            nDataLen = sizeof(WCHAR) * (lstrlenW((LPCWSTR) pbUnpackedData) + 1);
         }
      }

      //
      // Prepend a DWORD before the data. The DWORD contains the # of bytes.
      //
      cbPackedData += nDataLen + sizeof(DWORD);

      //
      // Get the offset of the next data element & its length
      //    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
      //
      nDataPtrOffset = va_arg(va_params, int);
      nDataLen       = va_arg(va_params, int);
   }
   va_end(va_params);

   //
   // Allocate a memory block large enough to hold
   // the structure and all of its data
   //
   PBYTE pbPackedStruct = (PBYTE) HeapAlloc(GetProcessHeap(), 0,
                                            cbStruct + cbPackedData);

   if(pbPackedStruct == NULL)
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return((PVOID) pbPackedStruct);
   }

   //
   // Copy the original structure to the beginning of the new block
   //
   CopyMemory(pbPackedStruct, pvStruct, cbStruct);

   //
   // Append the data to the end of the block &
   // change data pointer members to offets
   //
   PBYTE pbPackedData = pbPackedStruct + cbStruct;

   nDataPtrOffset = nDataPtrOffset0;
   nDataLen = nDataLen0;

   va_start(va_params, nDataLen0);
   while (nDataPtrOffset != PSAD_NO_MORE_DATA) {
      LPBYTE pbUnpackedData = * (LPBYTE*) ((PBYTE) pvStruct + nDataPtrOffset);
      if (pbUnpackedData == NULL) {
         //
         // User passed a NULL pointer
         //
         nDataLen = PSAD_NULL_DATA;
      } else {
         if (nDataLen == PSAD_STRING_DATA) {
            //
            // We are assuming that this is a string,
            //calc the number of bytes in the string
            //
            nDataLen = sizeof(WCHAR) * (lstrlenW((LPCWSTR) pbUnpackedData) + 1);
         }
      }

      * (PDWORD) pbPackedData = nDataLen; // Store the data'a length
      pbPackedData += sizeof(DWORD);
      * (PDWORD) &pbPackedStruct[nDataPtrOffset] =
                              (DWORD)(pbPackedData - pbPackedStruct);

      if (nDataLen != PSAD_NULL_DATA) {
         CopyMemory(pbPackedData, pbUnpackedData, nDataLen);
         pbPackedData += nDataLen;
      }

      //
      // Get the offset of the next data element & its length
      //
      //    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
      //
      nDataPtrOffset = va_arg(va_params, int);
      nDataLen       = va_arg(va_params, int);
   }
   va_end(va_params);
   *pcbBytesToSend += cbStruct + cbPackedData;
   return((PVOID) pbPackedStruct);
}

extern "C" void *__cdecl _alloca(size_t);

BOOL
WINAPI
CreateProcessWithLogonW(
      LPCWSTR lpUsername,
      LPCWSTR lpDomain,
      LPCWSTR lpPassword,
      DWORD dwLogonFlags,
      LPCWSTR lpApplicationName,
      LPWSTR lpCommandLine,
      DWORD dwCreationFlags,
      LPVOID lpEnvironment,
      LPCWSTR lpCurrentDirectory,
      LPSTARTUPINFOW lpStartupInfo,
      LPPROCESS_INFORMATION lpProcessInformation)

/*++

Routine Description:

Arguments:

Return Value:

--*/
{

   BOOL fOk = FALSE;
   //
   // This can't fail (outside of __try)
   //
   HANDLE hheap = GetProcessHeap();
   PSECONDARYLOGONINFOW psliT = NULL;
   LPSTARTUPINFO psiT = NULL;
   SECONDARYLOGONINFOW sli;
   SECONDARYLOGONRETINFO slri;
   HANDLE hpipe = INVALID_HANDLE_VALUE;
   DWORD cbBytesToSend = 0;
   DWORD cbBytesToSend1 = 0;
   WCHAR chEndOfFileName = 0;
   int nCharsInFileName = 0;
   TCHAR    DesktopName[MAX_PATH];
   BOOL     AccessWasAllowed = FALSE;
   TCHAR    WinstaName[MAX_PATH];
   TCHAR    DeskName[MAX_PATH];

   WCHAR    TranslatedName[MAX_PATH];
   DWORD    TranslatedNameSize = MAX_PATH;
   PWCHAR   UserName = (LPTSTR)lpUsername;
   PWCHAR   DomainName = (LPTSTR)lpDomain;
   DWORD    LastError;
   LPWSTR   pszApplName = NULL;
   LPWSTR   pszCmdLine = NULL;

   //
   // dynamically load user32.dll and resolve the functions.
   //
   // note: last error is left as returned by loadlib or getprocaddress

   if(hModule1 == NULL)
   {
       hModule1 = LoadLibrary(L"user32.dll");
       if(hModule1)
       {
            myOpenDesktop = (OPENDESKTOP) GetProcAddress(hModule1,
                                                         "OpenDesktopW");
            if(!myOpenDesktop) return FALSE;


            myGetThreadDesktop = (GETTHREADDESKTOP)
                                        GetProcAddress( hModule1,
                                                        "GetThreadDesktop");
            if(!myGetThreadDesktop) return FALSE;


            myCloseDesktop = (CLOSEDESKTOP) GetProcAddress(hModule1,
                                                           "CloseDesktop");
            if(!myCloseDesktop) return FALSE;


            myOpenWindowStation = (OPENWINDOWSTATION)
                                        GetProcAddress(hModule1,
                                                       "OpenWindowStationW");
            if(!myOpenWindowStation) return FALSE;


            myGetProcessWindowStation = (GETPROCESSWINDOWSTATION)
                                        GetProcAddress(hModule1,
                                                    "GetProcessWindowStation");
            if(!myGetProcessWindowStation) return FALSE;


            myCloseWindowStation = (CLOSEWINDOWSTATION) GetProcAddress(hModule1,
                                                "CloseWindowStation");

            if(!myCloseWindowStation) return FALSE;


            myGetUserObjectSecurity = (GETUSEROBJECTSECURITY)
                                                GetProcAddress(hModule1,
                                                "GetUserObjectSecurity");
            if(!myGetUserObjectSecurity) return FALSE;

            mySetUserObjectSecurity = (SETUSEROBJECTSECURITY)
                                                GetProcAddress(hModule1,
                                                "SetUserObjectSecurity");
            if(!mySetUserObjectSecurity) return FALSE;


            myGetUserObjectInformation = (GETUSEROBJECTINFORMATION)
                                                GetProcAddress(hModule1,
                                                "GetUserObjectInformationW");
            if(!mySetUserObjectSecurity) return FALSE;
       }
       else
       {
            return FALSE;
       }
   }

   if(hModule2 == NULL)
   {
       hModule2 = LoadLibrary(L"secur32.dll");
       if(hModule2)
       {
            myTranslateName = (TRANSLATENAME) GetProcAddress(hModule2,
                                                         "TranslateNameW");
            if(!myTranslateName) return FALSE;
       }
       else
       {
            return FALSE;
       }

   }

   __try {


      //
      // JMR: Do these flags work: CREATE_SEPARATE_WOW_VDM,
      //       CREATE_SHARED_WOW_VDM
      // Valid flags: CREATE_SUSPENDED, CREATE_UNICODE_ENVIRONMENT,
      //              *_PRIORITY_CLASS
      //
      // The following flags are illegal. Fail the call if any are specified.
      //
      if ((dwCreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS
                              | DETACHED_PROCESS)) != 0) {
         LastError = ERROR_INVALID_PARAMETER;
         __leave;
      }

      if(dwLogonFlags & ~(LOGON_WITH_PROFILE | LOGON_NETCREDENTIALS_ONLY))
      {
         LastError = ERROR_INVALID_PARAMETER;
         __leave;
      }

      //
      // Turn on the flags that MUST be turned on
      //
      // We are overloading CREATE_NEW_CONSOLE to 
      // CREATE_WITH_NETWORK_LOGON
      //
      dwCreationFlags |= CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE
                           | CREATE_NEW_PROCESS_GROUP;

      //
      // If no priority class explicitly specified and this process is IDLE, force IDLE (See CreateProcess documentation)
      //
      if ((dwCreationFlags & (NORMAL_PRIORITY_CLASS | IDLE_PRIORITY_CLASS
                              | HIGH_PRIORITY_CLASS
                              | REALTIME_PRIORITY_CLASS)) == 0) {

         if (GetPriorityClass(GetCurrentProcess()) == IDLE_PRIORITY_CLASS)
                  dwCreationFlags |= IDLE_PRIORITY_CLASS;
      }

      pszApplName = (LPWSTR) HeapAlloc(hheap, 0, sizeof(WCHAR) * (MAX_PATH));
      //
      // Lookup the fullpathname of the specified executable
      //
      pszCmdLine = (LPWSTR) HeapAlloc(hheap, 0, sizeof(WCHAR) *
                                             (MAX_PATH + lstrlenW(lpCommandLine)));

      if(pszApplName == NULL || pszCmdLine == NULL)
      {
        LastError = ERROR_INVALID_PARAMETER;
        __leave;
      }


      if(lpApplicationName == NULL)
      {
         if(lpCommandLine != NULL)
         {
            //
            // Commandline contains the name, we should parse it out and get
            // the full path so that correct executable is invoked.
            //

            DWORD   Length;
            DWORD   fileattr;
            WCHAR   TempChar = L'\0';
            LPWSTR  TempApplName = NULL;
            LPWSTR  TempRemainderString = NULL;
            LPWSTR  WhiteScan = NULL;
            BOOL    SearchRetry = TRUE;
            LPWSTR  ApplName = (LPWSTR) HeapAlloc(
                                                hheap, 0,
                                                sizeof(WCHAR) * (MAX_PATH+1));

            LPWSTR  NameBuffer = (LPWSTR) HeapAlloc(
                                                hheap, 0,
                                                sizeof(WCHAR) * (MAX_PATH+1));
            lstrcpy(ApplName, lpCommandLine);
            WhiteScan = ApplName;

            //
            // if there is a leading quote
            //
            if(*WhiteScan == L'\"')
            {
                // we will NOT retry search, as app name is quoted.
            
                SearchRetry = FALSE;
                WhiteScan++;
                TempApplName = WhiteScan;
                while(*WhiteScan) {
                    if( *WhiteScan == L'\"') 
                    {
                        TempChar = *WhiteScan;
                        *WhiteScan = L'\0';
                        TempRemainderString = WhiteScan;
                        break;
                    }
                    WhiteScan++;
                } 
            }
            else
            {
                // skip to the first non-white char
                while(*WhiteScan) {
                    if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                    {
                        WhiteScan++;
                    }
                    else
                        break;
                }
                TempApplName = WhiteScan;

                while(*WhiteScan) {
                    if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                    {
                        TempChar = *WhiteScan;
                        *WhiteScan = L'\0';
                        TempRemainderString = WhiteScan;
                        break;
                    }
                    WhiteScan++;
                }

            }

RetrySearch:
            Length = SearchPathW(
                            NULL,
                            TempApplName,
                            (PWSTR)L".exe",
                            MAX_PATH,
                            NameBuffer,
                            NULL
                            );
            
            if(!Length || Length > MAX_PATH)
            {
                if(LastError)
                    SetLastError(LastError);
                else
                    LastError = GetLastError();
                
CoverForDirectoryCase:
                    //
                    // If we still have command line left, then keep going
                    // the point is to march through the command line looking
                    // for whitespace so we can try to find an image name
                    // launches of things like:
                    // c:\word 95\winword.exe /embedding -automation
                    // require this. Our first iteration will 
                    // stop at c:\word, our next
                    // will stop at c:\word 95\winword.exe
                    //
                    if(TempRemainderString)
                    {
                        *TempRemainderString = TempChar;
                        WhiteScan++;
                    }
                    if(*WhiteScan & SearchRetry)
                    {
                        // again skip to the first non-white char
                        while(*WhiteScan) {
                            if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                            {
                                WhiteScan++;
                            }
                            else
                                break;
                        }
                        while(*WhiteScan) {
                            if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                            {
                                TempChar = *WhiteScan;
                                *WhiteScan = L'\0';
                                TempRemainderString = WhiteScan;
                                break;
                            }
                            WhiteScan++;
                        }
                        // we'll do one last try of the whole string.
                        if(!WhiteScan) SearchRetry = FALSE;
                        goto RetrySearch;
                    }

                    //
                    // otherwise we have failed.
                    //
                    if(NameBuffer) HeapFree(hheap, 0, NameBuffer);
                    if(ApplName) HeapFree(hheap, 0, ApplName);
                    
                    // we should let CreateProcess do its job.
                    if (pszApplName)
                    {
                        HeapFree(hheap, 0, pszApplName);
                        pszApplName = NULL;
                    }
                    lstrcpy(pszCmdLine, lpCommandLine);
            }
            else
            {
                // searchpath succeeded.
                // but it can find a directory!
                fileattr = GetFileAttributesW(NameBuffer);
                if ( fileattr != 0xffffffff &&
                        (fileattr & FILE_ATTRIBUTE_DIRECTORY) ) {
                        Length = 0;
                        goto CoverForDirectoryCase;
                }

                //
                // so it is not a directory.. it must be the real thing!
                //
                lstrcpy(pszApplName, NameBuffer);
                lstrcpy(pszCmdLine, lpCommandLine);

                HeapFree(hheap, 0, ApplName);
                HeapFree(hheap, 0, NameBuffer);
            }

         }
         else
         {

            LastError = ERROR_INVALID_PARAMETER;
            __leave;
         }

      }
      else
      {

         //
         // If ApplicationName is not null, we need to handle
         // one case here -- the application name is present in
         // current directory.  All other cases will be handled by
         // CreateProcess in the server side anyway.
         //

         //
         // let us get a FullPath relative to current directory
         // and try to open it.  If it succeeds, then the full path
         // is what we'll give as app name.. otherwise will just
         // pass what we got from caller and let CreateProcess deal with it.

         LPWSTR lpFilePart;

         DWORD  cchFullPath = GetFullPathName(
                                            lpApplicationName,
                                            MAX_PATH,
                                            pszApplName,
                                            &lpFilePart
                                            );

         if(cchFullPath)
         {
             HANDLE hFile;
             //
             // let us try to open it.
             // if it works, pszApplName is already setup correctly.
             // just close the handle.
            
             hFile = CreateFile(lpApplicationName, GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL
                                );
    
             if(hFile == INVALID_HANDLE_VALUE)
             {
                // otherwise, keep what the caller gave us.
                lstrcpy(pszApplName,lpApplicationName);
             }
             else
                CloseHandle(hFile);

         }
         else
            // lets keep what the caller gave us.
            lstrcpy(pszApplName, lpApplicationName);
    
         //
         // Commandline should be kept as is.
         //
         if(lpCommandLine != NULL)
                 lstrcpy(pszCmdLine, lpCommandLine);
         else
         {
            HeapFree(hheap, 0, pszCmdLine);
            pszCmdLine = NULL;
         }
      }

#if 0
      if(lpApplicationName != NULL) lstrcpy(pszApplName,lpApplicationName);
      else {
            HeapFree(hheap, 0, pszApplName);
            pszApplName = NULL;
      }
      if(lpCommandLine != NULL) lstrcpy(pszCmdLine, lpCommandLine);
      else {
            HeapFree(hheap, 0, pszCmdLine);
            pszCmdLine = NULL;
      }
#endif

      //
      // Construct a memory block will all the info that needs to go to the server
      //

      //
      // first conver the name form we might have got.
      //
      if(lpDomain == NULL || lpDomain[0] == L'\0')
      {
         PWCHAR str;
         //
         // Assume the name is UPN (user@domain)
         //
         if(!myTranslateName(lpUsername,
                        NameUserPrincipal,
                        NameSamCompatible,
                        TranslatedName,
                        &TranslatedNameSize
                        )) 
         {
            //
            // even if it fails, we don't worry too much..
            // it is only best effort.
            //
            LastError = GetLastError();
            sli.lpUsername         = (LPWSTR) lpUsername;
            sli.lpDomain           = (LPWSTR) lpDomain;
            
          }
         else
         {
              //
              // we have the Domain\Name form now,
              // let us split it and use it.
              //
              DomainName = TranslatedName;
              str = DomainName;
              while(*str != L'\0')
              {
                if(*str == L'\\')
                {
                    *str = L'\0';
                    UserName = str+1;
                    break;
                }
                str++;
              }
              //
              // now we have something so set it up
              //
              sli.lpUsername         = (LPWSTR) UserName;
              sli.lpDomain           = (LPWSTR) DomainName;
         }

      }
      else
      {
            //
            // they have been passed old style correctly.
            //
            sli.lpUsername         = (LPWSTR) lpUsername;
            sli.lpDomain           = (LPWSTR) lpDomain;
      }

      sli.LogonIdHighPart    = 0;
      sli.LogonIdLowPart     = 0;
      sli.dwLogonFlags       = dwLogonFlags;
      sli.dwProcessId        = GetCurrentProcessId();
      sli.lpPassword         = (LPWSTR) lpPassword;
      sli.lpApplicationName  = pszApplName;
      sli.lpCommandLine      = pszCmdLine;
      sli.dwCreationFlags    = dwCreationFlags;
      sli.lpEnvironment      = lpEnvironment;
      sli.lpCurrentDirectory = lpCurrentDirectory;
      sli.lpStartupInfo      = lpStartupInfo;

      if (sli.lpStartupInfo->lpDesktop == NULL
            || sli.lpStartupInfo->lpDesktop[0] == L'\0')
      {
           DWORD    Length;
           HWINSTA Winsta = myGetProcessWindowStation();
           HDESK Desk = myGetThreadDesktop(GetCurrentThreadId());

           if(myGetUserObjectInformation(Winsta,UOI_NAME,WinstaName,
                                        (MAX_PATH*sizeof(TCHAR)), &Length))
           {
                if(myGetUserObjectInformation(Desk,UOI_NAME,DeskName,
                                (MAX_PATH*sizeof(TCHAR)), &Length))
                {
                    lstrcpy((LPTSTR)DesktopName,L"");
                    lstrcat((LPTSTR)DesktopName,WinstaName);
                    lstrcat((LPTSTR)DesktopName,L"\\");
                    lstrcat((LPTSTR)DesktopName,DeskName);
                    sli.lpStartupInfo->lpDesktop = DesktopName;
               }
               else
                    sli.lpStartupInfo->lpDesktop = L"";
           }
           else
               sli.lpStartupInfo->lpDesktop = L"";

           DWORD ret = AllowDesktopAccessToUser(WinstaName,DeskName,
                                        lpUsername, lpDomain);
            if(ret == ERROR_SUCCESS)
                AccessWasAllowed = TRUE;

      }

      //
      // Package all data used in this INput structure into an
      // all-encompassing block
      //
      psiT = (LPSTARTUPINFOW) PackStructAndDataW(&cbBytesToSend1,
                                                 sli.lpStartupInfo,
                                                 sizeof(*psiT),
                                                 FIELDOFFSET(STARTUPINFOW, lpDesktop),
                                                 PSAD_STRING_DATA,
                                                 FIELDOFFSET(STARTUPINFOW, lpTitle),
                                                 PSAD_STRING_DATA,
                                                 PSAD_NO_MORE_DATA,
                                                 PSAD_NO_MORE_DATA);
      if (psiT == NULL)
      {
            LastError = ERROR_INVALID_PARAMETER;
            __leave;
      }

      //
      // Point to the packed structure
      //
      sli.lpStartupInfo = psiT;

      //
      // Calculate the size of the environment block (if one is passed)
      //
      int nEnvironment = 0;
      if (sli.lpEnvironment != NULL) {
         if ((sli.dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) != 0) {
            while (((LPCWSTR) lpEnvironment)[nEnvironment] != 0)
               nEnvironment += lstrlenW(&((LPCWSTR) lpEnvironment)[nEnvironment]) + 1;
            nEnvironment *= sizeof(WCHAR);   // Convert to number of bytes
         } else {
            while (((LPCSTR) lpEnvironment)[nEnvironment] != 0)
               nEnvironment += lstrlenA(&((LPCSTR) lpEnvironment)[nEnvironment]) + 1;
         }
      }

      //
      // Package all data used in this INput structure into an all-encompassing block
      //
      psliT = (PSECONDARYLOGONINFOW) PackStructAndDataW(&cbBytesToSend, &sli, sizeof(*psliT),
         FIELDOFFSET(SECONDARYLOGONINFOW, lpUsername),           PSAD_STRING_DATA,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpDomain),             PSAD_STRING_DATA,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpPassword),           PSAD_STRING_DATA,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpApplicationName),    PSAD_STRING_DATA,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpCommandLine),        PSAD_STRING_DATA,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpEnvironment),        nEnvironment,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpCurrentDirectory),   PSAD_STRING_DATA,
         FIELDOFFSET(SECONDARYLOGONINFOW, lpStartupInfo),        cbBytesToSend1,
         PSAD_NO_MORE_DATA, PSAD_NO_MORE_DATA);
      if (psliT == NULL)
      {
            LastError = ERROR_INVALID_PARAMETER;
            __leave;
      }

      //
      // Connect to the pipe
      //
      hpipe = CreateFileW(L"\\\\.\\pipe\\SecondaryLogon",
            GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
      if (hpipe == INVALID_HANDLE_VALUE)
      {

         LastError = GetLastError();
         if(LastError == ERROR_FILE_NOT_FOUND)
         {
            //
            // try to start the service.
            // if we can, try CreateFile again.
            //
            SC_HANDLE   hSCM;
            SC_HANDLE   hService;

            hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
            if(hSCM == NULL)
            {
                LastError = GetLastError();
                __leave;
            }

            hService = OpenService(hSCM, g_szSvcName, SERVICE_START);
            if(hService == NULL)
            {
               LastError = GetLastError();
               CloseServiceHandle(hSCM);
               __leave;
            }

            if(!StartService(hService, NULL, NULL)) 
            {
                LastError = GetLastError();
                CloseServiceHandle(hSCM);
                CloseServiceHandle(hService);
                __leave;
            }

            CloseServiceHandle(hSCM);
            CloseServiceHandle(hService);

            //
            // BUGBUG -- during testing I found out that
            // immediately opening the pipe after starting the service
            // did not give enough time to actually start the service.
            //
            // So we simulate a wait here for few milliseconds.
            //

            SleepEx(100,FALSE);

            //
            // Now open the pipe again.
            //

            hpipe = CreateFileW(L"\\\\.\\pipe\\SecondaryLogon",
                  GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                  FILE_FLAG_OVERLAPPED, NULL);
            if(hpipe == INVALID_HANDLE_VALUE) 
            {
                LastError = GetLastError();
                __leave;
            }

         }
         else
            __leave;
      }

      DWORD cbBytesTransferred;
      OVERLAPPED    Overlapped;
      SECURITY_ATTRIBUTES   SecurityAttr;

      Overlapped.Offset = 0;
      Overlapped.OffsetHigh = 0;

      SecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
      SecurityAttr.lpSecurityDescriptor = NULL;
      SecurityAttr.bInheritHandle = FALSE;

      Overlapped.hEvent = CreateEvent( &SecurityAttr, FALSE, FALSE, NULL );
      if(Overlapped.hEvent == NULL) __leave;

      if (!WriteFile(hpipe, &cbBytesToSend, sizeof(cbBytesToSend),
                     &cbBytesTransferred, &Overlapped))
      {
            LastError = GetLastError();
            if(LastError == ERROR_IO_PENDING)
            {
                if(!GetOverlappedResult(hpipe, &Overlapped,
                                        &cbBytesTransferred, TRUE))
                {
                   if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);
                    __leave;
                }
            }
            else    
            {
               if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);
                __leave;
            }
      }
      Overlapped.Offset = 0;
      Overlapped.OffsetHigh = 0;
      if (!WriteFile(hpipe, psliT, cbBytesToSend, &cbBytesTransferred,
                     &Overlapped))
      {
            LastError = GetLastError();
            if(LastError == ERROR_IO_PENDING)
            {
                if(!GetOverlappedResult(hpipe, &Overlapped,
                                        &cbBytesTransferred, TRUE))
                {
                   if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);
                    __leave;
                }
            }
            else    
            {
               if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);
                __leave;
            }
      }
      Overlapped.Offset = 0;
      Overlapped.OffsetHigh = 0;
      if (!ReadFile(hpipe, &slri, sizeof(slri), &cbBytesTransferred,
                    &Overlapped))
      {
            LastError = GetLastError();
            if(LastError == ERROR_IO_PENDING)
            {
                if(!GetOverlappedResult(hpipe, &Overlapped,
                                        &cbBytesTransferred, TRUE))
                {
                   if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);
                    __leave;
                }
            }
            else    
            {
               if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);
                __leave;
            }
      }

      if(Overlapped.hEvent) CloseHandle(Overlapped.hEvent);

      fOk = (slri.dwErrorCode == NO_ERROR);  // This function succeeds if the server's function succeeds

      if (!fOk) {
         //
         // If the server function failed, set the server's
         // returned eror code as this thread's error code
         //
         LastError = slri.dwErrorCode;
         SetLastError(slri.dwErrorCode);
      } else {
         //
         // The server function succeeded, return the
         // PROCESS_INFORMATION info
         //
         *lpProcessInformation = slri.pi;
         LastError = ERROR_SUCCESS;
      }
   }
   __finally {

      if(LastError != ERROR_SUCCESS)
      {
            if(AccessWasAllowed)
                RemoveDesktopAccessFromUser(WinstaName,DeskName,
                                        lpUsername, lpDomain);
      }

      if (hpipe != INVALID_HANDLE_VALUE) CloseHandle(hpipe);
      if (psiT  != NULL) FreePackedStructAndData(psiT);
      if (psliT != NULL) FreePackedStructAndData(psliT);
      if (pszCmdLine)  HeapFree(hheap, 0, pszCmdLine);
      if (pszApplName)  HeapFree(hheap, 0, pszApplName);

      SetLastError(LastError);
   }

   return(fOk);
}


//////////////////////////////// End Of File /////////////////////////////////

