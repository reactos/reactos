/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/security.cxx
 * PURPOSE:     Initializes the COM Object Security Model and Parameters
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

extern "C"
{
#include "svchost.h"

#include <aclapi.h>
#include <objidl.h>
}

/* GLOBALS *******************************************************************/

SID NtSid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } };

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
DwInitializeSdFromThreadToken (
    _Out_ PVOID *ppSecurityDescriptor,
    _Out_ PACL *ppAcl
    )
{
    HANDLE hToken;
    DWORD dwGroupLength, dwUserLength, dwError, dwAlignLength;
    PTOKEN_PRIMARY_GROUP pTokenGroup;
    PTOKEN_USER pTokenUser;
    EXPLICIT_ACCESS_W pListOfExplicitEntries;
    PACL pAcl = NULL;
    PISECURITY_DESCRIPTOR pSd;

    /* Assume failure */
    *ppSecurityDescriptor = NULL;
    *ppAcl = NULL;

    /* Open the token of the current thread */
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, 0, &hToken) == FALSE)
    {
        /* The thread is not impersonating, use the process token */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == FALSE)
        {
            /* No token could be queried, fail */
            return GetLastError();
        }
    }

    /* Get the size of the token's user */
    if ((GetTokenInformation(hToken, TokenUser, NULL, 0, &dwUserLength) != FALSE) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        return GetLastError();
    }

    /* Get the size of the token's primary group */
    if ((GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwGroupLength) != FALSE) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        return GetLastError();
    }

    /* Allocate an SD large enough to hold the SIDs for the above */
    dwAlignLength = ALIGN_UP(dwUserLength, ULONG);
    pSd = (PISECURITY_DESCRIPTOR)MemAlloc(0,
                                          dwAlignLength +
                                          dwGroupLength +
                                          sizeof(*pSd));
    if (pSd == NULL) return ERROR_OUTOFMEMORY;

    /* Assume success for now */
    dwError = ERROR_SUCCESS;

    /* We'll put them right after the SD itself */
    pTokenUser = (PTOKEN_USER)(pSd + 1);
    pTokenGroup = (PTOKEN_PRIMARY_GROUP)((ULONG_PTR)pTokenUser + dwAlignLength);

    /* Now initialize it */
    if (InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION) == FALSE)
    {
        dwError = GetLastError();
    }

    /* And do the actual query for the user */
    if (GetTokenInformation(hToken,
                            TokenUser,
                            pTokenUser,
                            dwUserLength,
                            &dwUserLength) == FALSE)
    {
        dwError = GetLastError();
    }

    /* And then the actual query for the primary group */
    if (GetTokenInformation(hToken,
                            TokenPrimaryGroup,
                            pTokenGroup,
                            dwGroupLength,
                            &dwGroupLength) == FALSE)
    {
        dwError = GetLastError();
    }

    /* Set the user as owner */
    if (SetSecurityDescriptorOwner(pSd, pTokenUser->User.Sid, FALSE) == FALSE)
    {
        dwError = GetLastError();
    }

    /* Set the group as primary */
    if (SetSecurityDescriptorGroup(pSd, pTokenGroup->PrimaryGroup, FALSE) == FALSE)
    {
        dwError = GetLastError();
    }

    /* Did everything so far work out? */
    if (dwError == ERROR_SUCCESS)
    {
        /* Yes, create an ACL granting the SYSTEM account access */
        pListOfExplicitEntries.grfAccessMode = SET_ACCESS;
        pListOfExplicitEntries.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        pListOfExplicitEntries.grfAccessPermissions = 1;
        pListOfExplicitEntries.grfInheritance = 0;
        pListOfExplicitEntries.Trustee.pMultipleTrustee = 0;
        pListOfExplicitEntries.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        pListOfExplicitEntries.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        pListOfExplicitEntries.Trustee.ptstrName = (LPWSTR)&NtSid;
        dwError = SetEntriesInAclW(1, &pListOfExplicitEntries, NULL, &pAcl);
        if (dwError == ERROR_SUCCESS)
        {
            /* Make that ACL the DACL of the SD we just built */
            if (SetSecurityDescriptorDacl(pSd, 1, pAcl, FALSE) == FALSE)
            {
                /* We failed, bail out */
                LocalFree(pAcl);
                dwError = GetLastError();
            }
            else
            {
                /* Now we have the SD and the ACL all ready to go */
                *ppSecurityDescriptor = pSd;
                *ppAcl = pAcl;
                return ERROR_SUCCESS;
            }
        }
    }

    /* Failure path, we'll free the SD since the caller can't use it */
    MemFree(pSd);
    return dwError;
}

BOOL
WINAPI
InitializeSecurity (
    _In_ DWORD dwParam,
    _In_ DWORD dwAuthnLevel,
    _In_ DWORD dwImpLevel,
    _In_ DWORD dwCapabilities
    )
{
    HRESULT hr;
    PACL pAcl;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    IGlobalOptions *pGlobalOptions;
    ASSERT(dwParam != 0);

    /* Create a valid SD and ACL based on the current thread's token */
    if (DwInitializeSdFromThreadToken(&pSecurityDescriptor, &pAcl) == ERROR_SUCCESS)
    {
        /* It worked -- initialize COM without DDE support */
        hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE);
    }
    else
    {
        /* Don't keep going if we don't have an SD */
        hr = E_FAIL;
    }

    /* Did we make it? */
    if (SUCCEEDED(hr))
    {
        /* Indeed, initialize COM security now */
        DBG_TRACE("Calling CoInitializeSecurity (dwAuthCapabilities = 0x%08x)\n",
                  dwCapabilities);
        hr = CoInitializeSecurity(pSecurityDescriptor,
                                  -1,
                                  NULL,
                                  NULL,
                                  dwAuthnLevel,
                                  dwImpLevel,
                                  NULL,
                                  dwCapabilities,
                                  NULL);
        if (FAILED(hr)) DBG_ERR("CoInitializeSecurity returned hr=0x%08x\n", hr);
    }

    /* Free the SD and ACL since we no longer need it */
    MemFree(pSecurityDescriptor);
    LocalFree(pAcl);

    /* Did we initialize COM correctly? */
    if (SUCCEEDED(hr))
    {
        /* Get the COM Global Options Interface */
        hr = CoCreateInstance(CLSID_GlobalOptions,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IGlobalOptions,
                              (LPVOID*)&pGlobalOptions);
        if (SUCCEEDED(hr))
        {
            /* Use it to disable COM exception handling */
            hr = pGlobalOptions->Set(COMGLB_EXCEPTION_HANDLING,
                                     COMGLB_EXCEPTION_DONOT_HANDLE);
            pGlobalOptions->Release();
            ASSERT(SUCCEEDED(hr));
        }
    }

    /* Return whether all COM calls were successful or not */
    return SUCCEEDED(hr);
}
