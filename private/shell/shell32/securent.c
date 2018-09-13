//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1995
//
// File: securent.c
//
// History:
//  06-07-95 BobDay     Created.
//
// This file contains a set of routines for the management of security
//
//---------------------------------------------------------------------------
#include "shellprv.h"
#pragma  hdrstop


#ifdef WINNT


// 
// default 
const SHELL_USER_SID susCurrentUser = {0, 0, 0};                                                                            // the current user 
const SHELL_USER_SID susSystem = {SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, 0};                                     // the "SYSTEM" group
const SHELL_USER_SID susAdministrators = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS};     // the "Administrators" group
const SHELL_USER_SID susPowerUsers = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS};    // the "Power Users" group
const SHELL_USER_SID susGuests = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS};             // the "Guests" group
const SHELL_USER_SID susEveryone = {SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID, 0};                                   // the "Everyone" group



//---------------------------------------------------------------------------
//  GetUserToken - Gets the current process's user token and returns
//                        it. It can later be free'd with LocalFree.
//---------------------------------------------------------------------------
STDAPI_(PTOKEN_USER) GetUserToken(HANDLE hUser)
{
    PTOKEN_USER pUser;
    DWORD dwSize = 64;
    HANDLE hToClose = NULL;

    if (hUser == NULL)
    {
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser);
        hToClose = hUser;
    }

    pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwSize);
    if (pUser)
    {
        DWORD dwNewSize;
        BOOL fOk = GetTokenInformation(hUser, TokenUser, pUser, dwSize, &dwNewSize);
        if (!fOk && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            LocalFree((HLOCAL)pUser);

            pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwNewSize);
            if (pUser)
            {
                fOk = GetTokenInformation( hUser, TokenUser, pUser, dwNewSize, &dwNewSize);
            }
        }
        if (!fOk)
        {
            LocalFree((HLOCAL)pUser);
            pUser = NULL;
        }
    }

    if (hToClose)
        CloseHandle(hToClose);

    return pUser;
}

//---------------------------------------------------------------------------
//  GetUserSid() - Returns a localalloc'd string containing the text
//                        version of the current user's SID.
//---------------------------------------------------------------------------
STDAPI_(LPTSTR) GetUserSid(HANDLE hToken)
{
    LPTSTR pString = NULL;
    PTOKEN_USER pUser = GetUserToken(hToken);
    if (pUser)
    {
        UNICODE_STRING UnicodeString;
        if (STATUS_SUCCESS == RtlConvertSidToUnicodeString(&UnicodeString, pUser->User.Sid, TRUE))
        {
            UINT nChars = (UnicodeString.Length / 2) + 1;
            pString = (LPTSTR)LocalAlloc(LPTR, nChars * SIZEOF(TCHAR));
            if (pString)
            {
                SHUnicodeToTChar(UnicodeString.Buffer, pString, nChars);
            }
            RtlFreeUnicodeString(&UnicodeString);
        }
        LocalFree((HLOCAL)pUser);
    }
    return pString;
}

#if 0
//----------------------------------------------------------------------------
//  GetCurrentUser - Fills in a buffer with the unique name that we are using
//                   for the currently logged on user.  On NT, this name is
//                   used for the name of the profile directory and for the
//                   name of the per-user recycle bin directory on a security
//                   aware drive.
//----------------------------------------------------------------------------
BOOL GetCurrentUser(LPTSTR lpBuff, UINT iSize) 
{
    BOOL fRet = FALSE;
    HANDLE hUser;

    static TCHAR s_szCurrentUser[MAX_PATH] = 0;

    if (s_szCurrentUser[0])
    {
        lstrcpyn(lpBuff, s_szCurrentUser, iSize);
        return TRUE;
    }

    *lpBuff = 0;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser))
    {
        LPTSTR pUserSid = GetUserSid(hUser);
        if (pUserSid)
        {
            HKEY hkeyProfileList;

            // Open the registry and find the appropriate name

            LONG lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), 0,
                                    KEY_READ, &hkeyProfileList);
            if (lStatus == ERROR_SUCCESS)
            {
                HKEY hkeyUser;
                lStatus = RegOpenKeyEx(hkeyProfileList, pUserSid, 0, KEY_READ, &hkeyUser);
                if (lStatus == ERROR_SUCCESS)
                {
                    TCHAR szProfilePath[MAX_PATH];
                    DWORD dwType, dwSize = SIZEOF(szProfilePath);

                    // First check for a "ProfileName" key

                    lStatus = SHQueryValueEx(hkeyUser, TEXT("ProfileName"), NULL, &dwType,
                                              (LPBYTE)szProfilePath, &dwSize);
                    if (lStatus == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
                    {
                        lstrcpyn(lpBuff, szProfilePath, iSize);
                        fRet = TRUE;
                    }
                    else
                    {
                        // Otherwise, grab the "ProfilePath" and get the last part of the name

                        dwSize = SIZEOF(szProfilePath);
                        lStatus = SHQueryValueEx(hkeyUser,TEXT("ProfileImagePath"), NULL, &dwType,
                                                  (LPBYTE)szProfilePath, &dwSize);

                        if (lStatus == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
                        {
                            // Return just the directory name portion of the profile path
                            lstrcpyn(lpBuff, PathFindFileName(szProfilePath), iSize);
                            fRet = TRUE;
                        }
                    }
                    RegCloseKey(hkeyUser);
                }
                RegCloseKey(hkeyProfileList);
            }
            LocalFree(pUserSid);
        }
    }

    // cache the results
    if (fRet)
        lstrcpyn(s_szCurrentUser, lpBuff, ARRAYSIZE(s_szCurrentUser));

    return fRet;
}

#endif // 0


__inline BOOL IsCurrentUserShellSID(PSHELL_USER_SID psusID)
{
    SID_IDENTIFIER_AUTHORITY sidNULL = {0};

    if ((psusID->dwUserGroupID == 0)    &&
        (psusID->dwUserID == 0)         &&
        memcmp(&psusID->sidAuthority, &sidNULL, sizeof(SID_IDENTIFIER_AUTHORITY)) == 0)
    {
        return TRUE;
    }

    return FALSE;
}


//
// Sets the specified ACE in the ACL to have dwAccessMask permissions.
//
__inline BOOL MakeACEInheritable(PACL pAcl, int iIndex, DWORD dwAccessMask)
{
    ACE_HEADER* pAceHeader;

    if (GetAce(pAcl, iIndex, (LPVOID*)&pAceHeader))
    {
        pAceHeader->AceFlags |= dwAccessMask;
        return TRUE;
    }

    return FALSE;
}


//
// Helper function to generate a SECURITY_DESCRIPTOR with the specified rights
// 
// OUT: psd - A pointer to a uninitialized SECURITY_DESCRIPTOR struct to be inited and filled in
//            in by this function
//
// IN:  PSHELL_USER_PERMISSION  - An array of PSHELL_USER_PERMISSION pointers that specify what access to grant
//      cUserPerm               - The count of PSHELL_USER_PERMISSION pointers in the array above
// 
//
STDAPI_(SECURITY_DESCRIPTOR*) GetShellSecurityDescriptor(PSHELL_USER_PERMISSION* apUserPerm, int cUserPerm)
{
    BOOL fSuccess = TRUE;   // assume success
    SECURITY_DESCRIPTOR* pSD = NULL;
    PSID* apSids = NULL;
    int cAces = cUserPerm;  // one ACE for each entry to start with
    int iAceIndex = 0;      // helps us keep count of how many ACE's we have added (count as we go)
    PTOKEN_USER pUserToken = NULL;
    DWORD cbSidLength = 0;
    DWORD cbAcl;
    PACL pAcl;
    int i;

    ASSERT(!IsBadReadPtr(apUserPerm, sizeof(PSHELL_USER_PERMISSION) * cUserPerm));

    // healthy parameter checking
    if (!apUserPerm || cUserPerm <= 0)
    {
        return NULL;
    }

    // first find out how many additional ACE's we are going to need
    // because of inheritance
    for (i = 0; i < cUserPerm; i++)
    {
        if (apUserPerm[i]->fInherit)
        {
            cAces++;
        }

        // also check to see if any of these are using susCurrentUser, in which case
        // we want to get the users token now so we have it already
        if ((pUserToken == NULL) && IsCurrentUserShellSID(&apUserPerm[i]->susID))
        {
            pUserToken = GetUserToken(NULL);
            if (!pUserToken)    
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "Failed to get the users token.  Error = %d", dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }
        }
    }

    // alloc the array to hold all the SID's
    apSids = (PSID*)LocalAlloc(LPTR, cUserPerm * sizeof(PSID));
    
    if (!apSids)
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "Failed allocate memory for %i SID's.  Error = %d", cUserPerm, dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    // initialize the SID's
    for (i = 0; i < cUserPerm; i++)
    {
        DWORD cbSid;

        // check for the special case of susCurrentUser
        if (IsCurrentUserShellSID(&apUserPerm[i]->susID))
        {
            ASSERT(pUserToken);
            apSids[i] = pUserToken->User.Sid;
        }
        else
        {
            SID_IDENTIFIER_AUTHORITY sidAuthority = apUserPerm[i]->susID.sidAuthority;

            if (!AllocateAndInitializeSid(&sidAuthority,
                                          (BYTE)(apUserPerm[i]->susID.dwUserID ? 2 : 1),    // if dwUserID is nonzero, then there are two SubAuthorities
                                          apUserPerm[i]->susID.dwUserGroupID,
                                          apUserPerm[i]->susID.dwUserID,
                                          0, 0, 0, 0, 0, 0, &apSids[i]))
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "AllocateAndInitializeSid: Failed to initialze SID.  Error = %d", cUserPerm, dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }
        }

        // add up all the SID lengths for an easy ACL size computation later...
        cbSid = GetLengthSid(apSids[i]);

        cbSidLength += cbSid;
        
        if (apUserPerm[i]->fInherit)
        {
            // if we have an inherit ACE as well, we need to add in the size of the SID again
            cbSidLength += cbSid;
        }

    }

    // calculate the size of the ACL we will be building (note: used sizeof(ACCESS_ALLOWED_ACE) b/c all ACE's are the same
    // size (excepting wacko object ACE's which we dont deal with). 
    //
    // this makes the size computation easy, since the size of the ACL will be the size of all the ACE's + the size of the SID's.
    cbAcl = SIZEOF(ACL) + (cAces * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) + cbSidLength;

    // HACKHACK (reinerf)
    //
    // we allocate enough space for the SECURITY_DESCRIPTOR and the ACL together and pass them both back to the
    // caller to free. we need to to this since the SECURITY_DESCRIPTOR contains a pointer to the ACL
    pSD = (SECURITY_DESCRIPTOR*)LocalAlloc(LPTR, SIZEOF(SECURITY_DESCRIPTOR) + cbAcl);

    if (!pSD)
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "Failed to allocate space for the SECURITY_DESCRIPTOR and the ACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    // set the address of the ACL to right after the SECURITY_DESCRIPTOR in the 
    // block of memory we just allocated
    pAcl = (PACL)(pSD + 1);
    
    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION))
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "InitializeAcl: Failed to init the ACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    for (i = 0; i < cUserPerm; i++)
    {
        BOOL bRet;

        // add the ACE's to the ACL
        if (apUserPerm[i]->dwAccessType == ACCESS_ALLOWED_ACE_TYPE)
        {
            bRet = AddAccessAllowedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwAccessMask, apSids[i]);
        }
        else
        {
            bRet = AddAccessDeniedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwAccessMask, apSids[i]);
        }

        if (!bRet)
        {
            DWORD dwLastError = GetLastError();
            TraceMsg(TF_WARNING, "AddAccessAllowed/DeniedAce: Failed to add SID.  Error = %d", dwLastError);
            fSuccess = FALSE;
            goto cleanup;
        }

        // sucessfully added an ace
        iAceIndex++;

        ASSERT(iAceIndex <= cAces);

        // if its an inherit ACL, also add another ACE for the inheritance part
        if (apUserPerm[i]->fInherit)
        {
            // add the ACE's to the ACL
            if (apUserPerm[i]->dwAccessType == ACCESS_ALLOWED_ACE_TYPE)
            {
                bRet = AddAccessAllowedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwInheritAccessMask, apSids[i]);
            }
            else
            {
                bRet = AddAccessDeniedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwInheritAccessMask, apSids[i]);
            }

            if (!bRet)
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "AddAccessAllowed/DeniedAce: Failed to add SID.  Error = %d", dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }

            if (!MakeACEInheritable(pAcl, iAceIndex, apUserPerm[i]->dwInheritMask))
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "MakeACEInheritable: Failed to add SID.  Error = %d", dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }

            // sucessfully added another ace
            iAceIndex++;
 
            ASSERT(iAceIndex <= cAces);
        }
    }

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "InitializeSecurityDescriptor: Failed to init the descriptor.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    if (!SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE))
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "SetSecurityDescriptorDacl: Failed to set the DACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

cleanup:
    if (apSids)
    {
        for (i = 0; i < cUserPerm; i++)
        {
            if (apSids[i])
            {
                // if this is one of the ones we allocated (eg not the users sid), free it
                if (!pUserToken || (apSids[i] != pUserToken->User.Sid))
                {
                    FreeSid(apSids[i]);
                }
            }
        }

        LocalFree(apSids);
    }

    if (pUserToken)
        LocalFree(pUserToken);

    if (!fSuccess && pSD)
    {
        LocalFree(pSD);
        pSD = NULL;
    }

    return pSD;
}


/*++

Routine Description:

    This routine sets the security attributes for a given privilege.

Arguments:

    PrivilegeName - Name of the privilege we are manipulating.

    NewPrivilegeAttribute - The new attribute value to use.

    OldPrivilegeAttribute - Pointer to receive the old privilege value. OPTIONAL

Return value:

    NO_ERROR or WIN32 error.

--*/

DWORD SetPrivilegeAttribute(LPCTSTR PrivilegeName, DWORD NewPrivilegeAttribute, DWORD *OldPrivilegeAttribute)
{
    LUID             PrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    //
    // First, find out the LUID Value of the privilege
    //

    if(!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) {
        return GetLastError();
    }

    //
    // Get the token handle
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle)) {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = SIZEOF( TOKEN_PRIVILEGES );
    if (!AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                SIZEOF( TOKEN_PRIVILEGES ),
                &OldTokenPrivileges,
                &ReturnLength
                )) {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else {
        if (OldPrivilegeAttribute != NULL) {
            *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
        }
        CloseHandle(TokenHandle);
        return NO_ERROR;
    }
}
#endif

//*************************************************************
//
//  IsUserAnAdmin()
//
//  Purpose:    Determines if the user is a member of the administrators group.
//
//  Parameters: void
//
//  Return:     TRUE if user is a admin
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              4/12/95     ericflo    Created
//
//*************************************************************

STDAPI_(BOOL) IsUserAnAdmin()
{
#ifdef WINNT
	// IE4 SHDOCVW cached the result of this function, I assume for perf.
	// It's much easier to do it here so all callers benefit.

	static int fIsUserAnAdmin = -1;

	if (-1 == fIsUserAnAdmin)
	{
	    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
	    NTSTATUS                 Status;
	    ULONG                    InfoLength;
	    PTOKEN_GROUPS            TokenGroupList;
	    ULONG                    GroupIndex;
	    BOOL                     FoundAdmins;
	    PSID                     AdminsDomainSid;
	    HANDLE                   hUserToken;


	    //
	    // Open the user's token
	    //

	    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUserToken)) {
	        return FALSE;
	    }


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

	    //
	    // Test if user is in the Admins domain
	    //

	    //
	    // Get a list of groups in the token
	    //

	    Status = NtQueryInformationToken(
	                 hUserToken,               // Handle
	                 TokenGroups,              // TokenInformationClass
	                 NULL,                     // TokenInformation
	                 0,                        // TokenInformationLength
	                 &InfoLength               // ReturnLength
	                 );

	    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {

	        RtlFreeSid(AdminsDomainSid);
	        CloseHandle(hUserToken);
	        return FALSE;
	    }


	    TokenGroupList = GlobalAlloc(GPTR, InfoLength);

	    if (TokenGroupList == NULL) {
	        RtlFreeSid(AdminsDomainSid);
	        CloseHandle(hUserToken);
	        return FALSE;
	    }

	    Status = NtQueryInformationToken(
	                 hUserToken,        // Handle
	                 TokenGroups,              // TokenInformationClass
	                 TokenGroupList,           // TokenInformation
	                 InfoLength,               // TokenInformationLength
	                 &InfoLength               // ReturnLength
	                 );

	    if (!NT_SUCCESS(Status)) {
	        GlobalFree(TokenGroupList);
	        RtlFreeSid(AdminsDomainSid);
	        CloseHandle(hUserToken);
	        return FALSE;
	    }


	    //
	    // Search group list for Admins alias
	    //

	    FoundAdmins = FALSE;

	    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

	        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, AdminsDomainSid)) {
	            FoundAdmins = TRUE;
	            break;
	        }
	    }

	    //
	    // Tidy up
	    //

	    GlobalFree(TokenGroupList);
	    RtlFreeSid(AdminsDomainSid);
	    CloseHandle(hUserToken);

	    fIsUserAnAdmin = FoundAdmins ? 1 : 0;
	}

    return (BOOL)fIsUserAnAdmin;

#else

    //
    // On Win95 everyone has the same privilage.  Someday this might
    // change if we want to special case the supervisor account.
    //

    return FALSE;

#endif
}

#ifdef WINNT
STDAPI_(BOOL) GetUserProfileKey(HANDLE hToken, HKEY *phkey)
{
    LPTSTR pUserSid = GetUserSid(hToken);
    if (pUserSid)
    {
        LONG err = RegOpenKeyEx(HKEY_USERS, pUserSid, 0, KEY_READ | KEY_WRITE, phkey);
        if (err == ERROR_ACCESS_DENIED)
            err = RegOpenKeyEx(HKEY_USERS, pUserSid, 0, KEY_READ, phkey);

        LocalFree(pUserSid);
        return err == ERROR_SUCCESS;
    }
    return FALSE;
}
#endif
