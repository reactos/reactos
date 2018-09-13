
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rmtcred.c

Abstract:

    This module implements SetProcessDefaultCredentials Win32 API

Author:

    Keith Vogel	Feb-3-1999


Revision History:

--*/



//
// Implementation notes
//
// This is a very tricky API
//
// What we do here is the following:
//
//  SetProcessDefaultCredentials accepts handle to the process
//  and various AcquireCredentialsHandle.
//  It calls CreateRemoteThread to invoke a thread in the remote process
//  which executes SetpProcessDefaultCredentialsThreadProc
//  SetpProcessDefaultCredentialsThreadProc does the following:
//  it takes the credentials and calls AcquireCredentialsHandle
//
//  Some of the things this depends on:
//  *  KERNEL32.DLL will always be mapped in exactly the same address locations
//  in all processes.
//  *  Writing to the address location based on one process will write to
//      same place in the other process
//  *  Invoking the function at address location based on one process will
//     invoke the same function in the other process.
//
#include "basedll.h"
#pragma hdrstop

#define SECURITY_WIN32
#include    "sspi.h"
#include    "rpc.h"

//
// Global Data
//

//
// BUGBUG -- all static structures are MAX_PATH for now.. we need to fix that.
//


typedef SECURITY_STATUS (SEC_ENTRY * PFNAcquireCredentialsHandleW) (
    SEC_WCHAR SEC_FAR * pszPrincipal,
    SEC_WCHAR SEC_FAR * pszPackage,
    unsigned long fCredentialUse,
    void SEC_FAR * pvLogonId,
    void SEC_FAR * pAuthData,
    SEC_GET_KEY_FN pGetKeyFn,
    void SEC_FAR * pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp ptsExpiry
    );

typedef struct _ACQCREDINFO {
    ULONG                       fCredentials;
    SEC_WINNT_AUTH_IDENTITY_W   AuthIdentity;
    WCHAR                       wszUserName[MAX_PATH];
    WCHAR                       wszDomain[MAX_PATH];
    WCHAR                       wszPassword[MAX_PATH];
    WCHAR                       wszPrincipalName[MAX_PATH];
    WCHAR                       wszPackageName[MAX_PATH];
    
    // the following must be NULL
    VOID    *               pLogonID;
    SEC_GET_KEY_FN          pGetKeyFunc;
    VOID    *               KeyFuncArg;
 } ACQCREDINFO, * PACQCREDINFO;


ACQCREDINFO acqCredInfo;


// this is used to get the process loaded by CreateProcess
DWORD
PingProcess(LPVOID  Param) {
    return(ERROR_SUCCESS);
}


DWORD
SetpProcessDefaultCredentialsThreadProc(
    LPVOID  Param
)
{
    static CredHandle               hCredentials = {0xFFFFFFFF, 0xFFFFFFFF};
    TimeStamp                       Expiry;
    SECURITY_STATUS                 SecStatus;

    HANDLE                          hMod    = NULL;
    PFNAcquireCredentialsHandleW    pfn     = NULL;

    if(hCredentials.dwLower != 0xFFFFFFFF  ||  hCredentials.dwUpper != 0xFFFFFFFF)
        return(ERROR_ALREADY_ASSIGNED);

    if( NULL != (hMod = LoadLibraryA("Secur32.dll")) ) {

        if( NULL != (pfn = (PFNAcquireCredentialsHandleW) GetProcAddress(hMod, "AcquireCredentialsHandleW")) ) {
    
        //
        // It is assumed that SetProcessDefaultCredentials
        // has already setup all the memory correctly.
        //  we just call the API.

        SecStatus = pfn( 
            acqCredInfo.wszPrincipalName,
            acqCredInfo.wszPackageName,
            acqCredInfo.fCredentials,
            acqCredInfo.pLogonID,
            &acqCredInfo.AuthIdentity,
            acqCredInfo.pGetKeyFunc,
            acqCredInfo.KeyFuncArg,
            &hCredentials,
            &Expiry); 

        }
    }

    if(pfn == NULL)
        SecStatus = GetLastError();

// we can't free the module because this will clean up the
// credential state. But this is only in the spawned app, so
// when the app dies it will get cleaned up
//    if(NULL != hMod)
//	 FreeLibrary(hMod);
                                        
    return(SecStatus);
}

BOOL
WINAPI
SetProcessDefaultCredentials(
    HANDLE  hProcess,
    LPWSTR  lpPrincipal,
    LPWSTR  lpPackage,
    ULONG   fCredentials,
    PVOID   LogonID,                // must be NULL for this release.
    PVOID   pvAuthData,
    SEC_GET_KEY_FN  fnGetKey,       // must be NULL for this release.
    PVOID   pvGetKeyArg             // must be NULL for this release.
    )
{

    DWORD                       BytesWritten;
    SEC_WINNT_AUTH_IDENTITY_W * pAuthData = pvAuthData;
    HANDLE                      hThread      = NULL;
    DWORD                       dwWaitReason    = 0;

    // clean out the buffer to be copied
    memset(&acqCredInfo, 0, sizeof(acqCredInfo));

    if( pAuthData->UserLength	    > MAX_PATH	||
	pAuthData->PasswordLength   > MAX_PATH	||
        pAuthData->DomainLength     > MAX_PATH  ) {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return(FALSE);
    }
    
    //
    // we do the following:
    //
    //  *   WriteProcess Memory:  Write the arguments to the exact 
    //      same address locations in the remote process as  in current process.
    //  *   Call CreateRemoteThread and pass the same address location of
    //      SetpProcessDefaultCredentialsThreadProc as in current process.
    //

    //
    // initialize the AuthData
    //
    if(pAuthData)
    {
        if(pAuthData->User) {
            acqCredInfo.AuthIdentity.User = acqCredInfo.wszUserName;
            acqCredInfo.AuthIdentity.UserLength = pAuthData->UserLength;
            memcpy(acqCredInfo.wszUserName, pAuthData->User, pAuthData->UserLength * sizeof(WCHAR));
        }
        
        if(pAuthData->Password) {
            acqCredInfo.AuthIdentity.Password = acqCredInfo.wszPassword;
            acqCredInfo.AuthIdentity.PasswordLength = pAuthData->PasswordLength;
            memcpy(acqCredInfo.wszPassword, pAuthData->Password, pAuthData->PasswordLength * sizeof(WCHAR));
        }
        
        if(pAuthData->Domain) {
            acqCredInfo.AuthIdentity.Domain = acqCredInfo.wszDomain;
            acqCredInfo.AuthIdentity.DomainLength = pAuthData->DomainLength;
            memcpy(acqCredInfo.wszDomain, pAuthData->Domain, pAuthData->DomainLength * sizeof(WCHAR));
        }

        acqCredInfo.AuthIdentity.Flags = pAuthData->Flags;
    }

    //
    // setup lpPrincipal
    //
    if(lpPrincipal)
        wcscpy(acqCredInfo.wszPrincipalName, lpPrincipal);        

    //
    // setup lpPackage
    //
    if(lpPackage)
        wcscpy(acqCredInfo.wszPackageName, lpPackage);        

    //
    // setup fCredentials
    //
    acqCredInfo.fCredentials = fCredentials;



    hThread = CreateRemoteThread( 
        hProcess,
        NULL,
        0,
        PingProcess,
        NULL,
        0,
        NULL
        );

    if(hThread) {

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }


    if(!WriteProcessMemory( hProcess,
                            &acqCredInfo,
                            &acqCredInfo,
                            sizeof(acqCredInfo),
                            &BytesWritten
                            ))  
        return(FALSE);                                

    //
    // so, after lot of painful work, we should
    // have written all the parameters such that calling the
    // ThreadProc will do the right thing (TM).
    //
    hThread = CreateRemoteThread( hProcess,
                                       NULL,
                                       0,
                                       SetpProcessDefaultCredentialsThreadProc,
                                       NULL,
                                       0,
                                       NULL
                                       );

    if(hThread)
    {

        //
        // we wait for the thread to complete execution and die.
        //
        dwWaitReason = WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    return(dwWaitReason == WAIT_OBJECT_0);
}
