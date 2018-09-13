//
// Global counters
//
//

#include "priv.h"

typedef struct _SHELL_USER_SID
{
    SID_IDENTIFIER_AUTHORITY sidAuthority;
    DWORD dwUserGroupID;
    DWORD dwUserID;
} SHELL_USER_SID, *PSHELL_USER_SID;

const SHELL_USER_SID susSystem = {SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, 0};                                     // the "SYSTEM" group
const SHELL_USER_SID susAdministrators = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS};     // the "Administrators" group
const SHELL_USER_SID susEveryone = {SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID, 0};                                   // the "Everyone" group

typedef struct _SHELL_USER_PERMISSION
{
    SHELL_USER_SID susID;       // identifies the user for whom you want to grant permissions to
    DWORD dwAccessType;         // currently, this is either ACCESS_ALLOWED_ACE_TYPE or  ACCESS_DENIED_ACE_TYPE
    DWORD dwAccessMask;         // access granted (eg FILE_LIST_CONTENTS or KEY_ALL_ACCESS, etc...)
} SHELL_USER_PERMISSION, *PSHELL_USER_PERMISSION;

//
// AllocSafeFullAccessACL
//
// Allocate an ACL and fill it in with values that allow system and administrators
// to do everything and for everyone to do everything except change the acls.
//
// Caller must LocalFree the returned ACL or face the wrath of the LeakMeister.
// BUGBUG That said, we will leak one of these per process below in CreateAllAccessSecurityAttributes
//
// NOTE: This is yanked and simplified from Shell32 securent.c GetShellSecurityDescriptor
//
STDAPI_(PACL)  AllocSafeFullAccessACL()
{
    BOOL fSuccess = TRUE;   // assume success
    SECURITY_DESCRIPTOR* pSD = NULL;
    PSID* apSids = NULL;
    int cUserPerm = 3;
    int cAces = cUserPerm;  // one ACE for each entry to start with
    int iAceIndex = 0;      // helps us keep count of how many ACE's we have added (count as we go)
    DWORD cbSidLength = 0;
    DWORD cbAcl;
    PACL pAcl = NULL;
    int i;
    SHELL_USER_PERMISSION supEveryone;
    SHELL_USER_PERMISSION supSystem;
    SHELL_USER_PERMISSION supAdministrators;
    PSHELL_USER_PERMISSION apUserPerm[3] = {&supEveryone, &supAdministrators, &supSystem};

    // we want the everyone to have read, write, exec and sync only 
    supEveryone.susID = susEveryone;
    supEveryone.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supEveryone.dwAccessMask = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE);

    // we want the SYSTEM to have full control
    supSystem.susID = susSystem;
    supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supSystem.dwAccessMask = GENERIC_ALL;

    // we want the Administrators to have full control
    supAdministrators.susID = susAdministrators;
    supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supAdministrators.dwAccessMask = GENERIC_ALL;
    
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

        // add up all the SID lengths for an easy ACL size computation later...
        cbSid = GetLengthSid(apSids[i]);

        cbSidLength += cbSid;
    }

    // calculate the size of the ACL we will be building (note: used sizeof(ACCESS_ALLOWED_ACE) b/c all ACE's are the same
    // size (excepting wacko object ACE's which we dont deal with). 
    //
    // this makes the size computation easy, since the size of the ACL will be the size of all the ACE's + the size of the SID's.
    cbAcl = SIZEOF(ACL) + (cAces * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) + cbSidLength;

    pAcl = (PACL)LocalAlloc(LPTR, cbAcl);

    if (!pAcl)
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "Failed to allocate space for the ACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }
    
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

    }

cleanup:
    if (apSids)
    {
        for (i = 0; i < cUserPerm; i++)
        {
            if (apSids[i])
            {
                FreeSid(apSids[i]);
            }
        }

        LocalFree(apSids);
    }

    if (!fSuccess && pAcl)
    {
        LocalFree(pAcl);
        pAcl = NULL;
    }

    return pAcl;
    
}

#ifdef DEBUG
#define GLOBAL_COUNTER_WAIT_TIMEOUT 30*1000  // on debug we set this to 30 seconds
#else
#define GLOBAL_COUNTER_WAIT_TIMEOUT 0        // on retail its zero so we test the objects state and return immedaeately
#endif

LPSECURITY_ATTRIBUTES g_psaCached;
SECURITY_ATTRIBUTES g_sa;
SECURITY_DESCRIPTOR g_sd;
PACL g_pacl;        //BUGBUG I am leaked, once per process.
//
// This function fills in a SECURITY_ATTRIBUTES struct such that it will be full access to everyone.
// This is needed when multiple processes need access to the same named kernel objects.
//
// If you pass (NULL, NULL, NULL), then we return a static security object with
// full access.  If you get the ACL back, you must LocalFree it.
//
STDAPI_(SECURITY_ATTRIBUTES*) CreateAllAccessSecurityAttributes(SECURITY_ATTRIBUTES* psa, SECURITY_DESCRIPTOR* psd, PACL *ppacl)
{
    PACL    pAcl = NULL;
    
    // Win9x doesn't use dacls
    if (!g_bRunningOnNT)
        return NULL;

    if (ppacl)
    {
        *ppacl = NULL;
    }
    
    // App is lazy and wants to piggyback off our copy of the dacl
    if (!psa)
    {
        if (!g_psaCached)
        {
            ENTERCRITICAL;
            if (!g_psaCached)
            {
                g_psaCached = CreateAllAccessSecurityAttributes(&g_sa, &g_sd, &g_pacl);
            }
            LEAVECRITICAL;
        }
        return g_psaCached;
    }

    //
    //  When creating a kernel syncronization object, make sure it is 
    //  created w/ "allow everyone full access".  If we weren't explicit,
    //  then we'd inherit whatever permissions belong to the current
    //  thread, which might be overly restrictive.
    //

    //
    //  There are three kinds of null-type DACLs.
    //
    //  1. No DACL.  This means that we inherit the ambient DACL
    //     from our thread.
    //  2. Null DACL.  This means "full access to everyone".
    //  3. Empty DACL.  This means "deny all access to everyone".
    //
    //  InitializeSecurityDescriptor gives you a (1) so we need to
    //  change it to a (2).
    //
    pAcl = AllocSafeFullAccessACL();
    
    if (pAcl && 
        InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION) &&
        SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE))
    {
        psa->nLength = sizeof(*psa);
        psa->lpSecurityDescriptor = psd;
        psa->bInheritHandle = FALSE;
        
        if (ppacl)
        {
            *ppacl = pAcl;
        }
        else
        {
            LocalFree(pAcl);
        }
    }
    else
    {
        // Horrible failure
        return NULL;
    }

    return psa;
}

STDAPI_(void) FreeCachedAcl()
{
    if (g_pacl)
    {
        LocalFree(g_pacl);
        g_pacl = NULL;
        g_psaCached = NULL;
    }
}


//
// This lets the user pass an ANSI String as the name of the global counter, as well as an inital value
//
STDAPI_(HANDLE) SHGlobalCounterCreateNamedA(LPCSTR szName, LONG lInitialValue)
{
    HANDLE hSem;
    //
    //  Explicitly ANSI so it runs on Win95.
    //
    char szCounterName[MAX_PATH];    // "shell.szName"
    LPSECURITY_ATTRIBUTES psa;

    lstrcpyA(szCounterName, "shell.");
    StrCatBuffA(szCounterName, szName, ARRAYSIZE(szCounterName));

    psa = CreateAllAccessSecurityAttributes(NULL, NULL, NULL);
    hSem = CreateSemaphoreA(psa, lInitialValue, 0x7FFFFFFF, szCounterName);
    if (!hSem)
        hSem = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, szCounterName);

    return hSem;
}


//
// This lets the user pass an UNICODE String as the name of the global counter, as well as an inital value
//
STDAPI_(HANDLE) SHGlobalCounterCreateNamedW(LPCWSTR szName, LONG lInitialValue)
{
    CHAR szCounterName[MAX_PATH];

    SHUnicodeToAnsi(szName, szCounterName, ARRAYSIZE(szCounterName));

    return SHGlobalCounterCreateNamedA(szCounterName, lInitialValue);
}


//
// This lets the user pass a GUID. The name of the global counter will be "shell.{guid}",
// and its initial value will be zero.
//
STDAPI_(HANDLE) SHGlobalCounterCreate(REFGUID rguid)
{
    CHAR szGUIDString[GUIDSTR_MAX];

    SHStringFromGUIDA(rguid, szGUIDString, ARRAYSIZE(szGUIDString));

    return SHGlobalCounterCreateNamedA(szGUIDString, 0);
}


// returns current value of the global counter
// Note: The result is not thread-safe in the sense that if two threads
// look at the value at the same time, one of them might read the wrong
// value.
STDAPI_(long) SHGlobalCounterGetValue(HANDLE hCounter)
{ 
    long lPreviousValue = 0;
    DWORD dwRet;

    ReleaseSemaphore(hCounter, 1, &lPreviousValue); // poll and bump the count
    dwRet = WaitForSingleObject(hCounter, GLOBAL_COUNTER_WAIT_TIMEOUT); // reduce the count

    // this shouldnt happen since we just bumped up the count above
    ASSERT(dwRet != WAIT_TIMEOUT);
    
    return lPreviousValue;
}


// returns new value
// Note: this _is_ thread safe
STDAPI_(long) SHGlobalCounterIncrement(HANDLE hCounter)
{ 
    long lPreviousValue = 0;

    ReleaseSemaphore(hCounter, 1, &lPreviousValue); // bump the count
    return lPreviousValue + 1;
}

// returns new value
// Note: The result is not thread-safe in the sense that if two threads
// try to decrement the value at the same time, whacky stuff can happen.
STDAPI_(long) SHGlobalCounterDecrement(HANDLE hCounter)
{ 
    DWORD dwRet;
    long lCurrentValue = SHGlobalCounterGetValue(hCounter);

#ifdef DEBUG
    // extra sanity check
    if (lCurrentValue == 0)
    {
        ASSERTMSG(FALSE, "SHGlobalCounterDecrement called on a counter that was already equal to 0 !!");
        return 0;
    }
#endif

    dwRet = WaitForSingleObject(hCounter, GLOBAL_COUNTER_WAIT_TIMEOUT); // reduce the count

    ASSERT(dwRet != WAIT_TIMEOUT);

    return lCurrentValue - 1;
}
