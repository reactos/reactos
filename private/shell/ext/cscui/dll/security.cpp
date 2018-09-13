//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       security.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "security.h"


DWORD 
Security_SetPrivilegeAttrib(
    LPCTSTR PrivilegeName, 
    DWORD NewPrivilegeAttribute, 
    DWORD *OldPrivilegeAttribute
    )
{
    LUID             PrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    //
    // First, find out the LUID Value of the privilege
    //
    if(!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) 
    {
        return GetLastError();
    }

    //
    // Get the token handle
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle)) 
    {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = sizeof(TOKEN_PRIVILEGES);
    if (!AdjustTokenPrivileges(
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof(TOKEN_PRIVILEGES),
                &OldTokenPrivileges,
                &ReturnLength
                )) 
    {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else 
    {
        if (NULL != OldPrivilegeAttribute) 
        {
            *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
        }
        CloseHandle(TokenHandle);
        return ERROR_SUCCESS;
    }
}




BOOL IsCurrentUserAnAdminMember(VOID)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    PSID AdminsDomainSid;
    BOOL bIsAdminMember = FALSE;
    //
    // Create Admins domain sid.
    //
    Status = RtlAllocateAndInitializeSid(
               &authNT,
               2,
               SECURITY_BUILTIN_DOMAIN_RID,
               DOMAIN_ALIAS_RID_ADMINS,
               0, 0, 0, 0, 0, 0,
               &AdminsDomainSid
               );


    if (STATUS_SUCCESS == Status)
    {
        CheckTokenMembership(NULL, AdminsDomainSid, &bIsAdminMember);
        RtlFreeSid(AdminsDomainSid);
    }
    return bIsAdminMember;
}


//
// Returns the SID of the currently logged on user.
// If the function succeeds, use the FreeSid API to 
// free the returned SID structure.
//
HRESULT
GetCurrentUserSid(
    PSID *ppsid
    )
{
    HRESULT hr  = NOERROR;
    DWORD dwErr = 0;

    //
    // Get the token handle. First try the thread token then the process
    // token.  If these fail we return early.  No sense in continuing
    // on if we can't get a user token.
    //
    *ppsid = NULL;
    CWin32Handle hToken;
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_READ,
                         TRUE,
                         hToken.HandlePtr()))
    {
        if (ERROR_NO_TOKEN == GetLastError())
        {
            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_READ,
                                  hToken.HandlePtr()))
            {
                dwErr = GetLastError();
                return HRESULT_FROM_WIN32(dwErr);
            }
        }
        else
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    //
    // Find operator's SID.
    //
    LPBYTE pbTokenInfo = NULL;
    DWORD cbTokenInfo = 0;
    cbTokenInfo = 0;
    if (!GetTokenInformation(hToken,
                             TokenUser,
                             NULL,
                             cbTokenInfo,
                             &cbTokenInfo))
    {
        dwErr = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwErr)
        {
            pbTokenInfo = new BYTE[cbTokenInfo];
        }
        else
        {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the user token information.
        //
        if (!GetTokenInformation(hToken,
                                 TokenUser,
                                 pbTokenInfo,
                                 cbTokenInfo,
                                 &cbTokenInfo))
        {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
        else
        {
            SID_AND_ATTRIBUTES *psa = (SID_AND_ATTRIBUTES *)pbTokenInfo;
            int cbSid = GetLengthSid(psa->Sid);
            *ppsid = (PSID)new BYTE[cbSid];
            if (NULL != *ppsid)
            {
                CopySid(cbSid, *ppsid, psa->Sid);
                hr = NOERROR;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        delete[] pbTokenInfo;
    }
    if (SUCCEEDED(hr) && NULL != *ppsid && !IsValidSid(*ppsid))
    {
        //
        // We created a SID but it's invalid.
        //
        FreeSid(*ppsid);
        *ppsid = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
    }
    return hr;
}


//
// Determines if a given SID is that of the current user.
//
BOOL IsSidCurrentUser(PSID psid)
{
    BOOL bIsCurrent = FALSE;
    PSID psidUser;
    if (SUCCEEDED(GetCurrentUserSid(&psidUser)))
    {
        bIsCurrent = EqualSid(psid, psidUser);
        FreeSid(psidUser);
    }
    return bIsCurrent;
}


