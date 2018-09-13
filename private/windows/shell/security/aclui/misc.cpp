//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       misc.cpp
//
//  This file contains miscellaneous helper functions.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"


/*******************************************************************

    NAME:       GetAceSid

    SYNOPSIS:   Gets pointer to SID from an ACE

    ENTRY:      pAce - pointer to ACE

    EXIT:

    RETURNS:    Pointer to SID if successful, NULL otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

PSID
GetAceSid(PACE_HEADER pAce)
{
    switch (pAce->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        return (PSID)&((PKNOWN_ACE)pAce)->SidStart;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        return (PSID)&((PKNOWN_COMPOUND_ACE)pAce)->SidStart;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        return RtlObjectAceSid(pAce);
    }

    return NULL;
}


/*******************************************************************

    NAME:       LocalAllocSid

    SYNOPSIS:   Copies a SID

    ENTRY:      pOriginal - pointer to SID to copy

    EXIT:

    RETURNS:    Pointer to SID if successful, NULL otherwise

    NOTES:      Caller must free the returned SID with LocalFree

    HISTORY:
        JeffreyS    12-Apr-1999     Created

********************************************************************/

PSID
LocalAllocSid(PSID pOriginal)
{
    PSID pCopy = NULL;
    if (pOriginal && IsValidSid(pOriginal))
    {
        DWORD dwLength = GetLengthSid(pOriginal);
        pCopy = (PSID)LocalAlloc(LMEM_FIXED, dwLength);
        if (NULL != pCopy)
            CopyMemory(pCopy, pOriginal, dwLength);
    }
    return pCopy;
}


/*******************************************************************

    NAME:       DestroyDPA

    SYNOPSIS:   LocalFree's all pointers in a Dynamic Pointer
                Array and then frees the DPA.

    ENTRY:      hList - handle of list to destroy

    EXIT:

    RETURNS:    nothing

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

int CALLBACK
_LocalFreeCB(LPVOID pVoid, LPVOID /*pData*/)
{
    if (pVoid)
        LocalFree(pVoid);
    return 1;
}

void
DestroyDPA(HDPA hList)
{
    if (hList != NULL)
        DPA_DestroyCallback(hList, _LocalFreeCB, 0);
}



/*******************************************************************

    NAME:       GetLSAConnection

    SYNOPSIS:   Wrapper for LsaOpenPolicy

    ENTRY:      pszServer - the server on which to make the connection

    EXIT:

    RETURNS:    LSA_HANDLE if successful, NULL otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

LSA_HANDLE
GetLSAConnection(LPCTSTR pszServer, DWORD dwAccessDesired)
{
    LSA_HANDLE hPolicy = NULL;
    LSA_UNICODE_STRING uszServer = {0};
    LSA_UNICODE_STRING *puszServer = NULL;
    LSA_OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;

    sqos.Length = SIZEOF(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
    oa.SecurityQualityOfService = &sqos;

    if (pszServer &&
        *pszServer &&
        RtlCreateUnicodeString(&uszServer, pszServer))
    {
        puszServer = &uszServer;
    }

    LsaOpenPolicy(puszServer, &oa, dwAccessDesired, &hPolicy);

    if (puszServer)
        RtlFreeUnicodeString(puszServer);

    return hPolicy;
}


/*******************************************************************

    NAME:       LookupSid

    SYNOPSIS:   Gets the qualified account name for a given SID

    ENTRY:      pszServer - the server on which to do the lookup
                pSid - the SID to lookup

    EXIT:       *ppszName contains the account name. This buffer
                must be freed by the caller with LocalFree.

                *pSidType contains the SID type. pSidType is optional.

    RETURNS:    TRUE if successful, FALSE otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created
        JeffreyS    16-Jan-1998     Converted to HDPA (multiple lookup)

********************************************************************/

BOOL
LookupSids(HDPA hSids, LPCTSTR pszServer, LPSECURITYINFO2 psi2, PUSER_LIST *ppUserList)
{
    PSIDCACHE pSidCache;

    if (NULL == hSids)
        return FALSE;

    if (ppUserList != NULL)
        *ppUserList = NULL;

    pSidCache = GetSidCache();

    if (NULL != pSidCache)
    {
        BOOL bRet = pSidCache->LookupSids(hSids, pszServer, psi2, ppUserList);
        pSidCache->Release();
        return bRet;
    }

    return FALSE;
}

BOOL
LookupSid(PSID pSid, LPCTSTR pszServer, LPSECURITYINFO2 psi2, PUSER_LIST *ppUserList)
{
    BOOL fResult;
    HDPA hSids = NULL;

    if (NULL == pSid)
        return FALSE;

    hSids = DPA_Create(1);

    if (NULL == hSids)
        return FALSE;

    DPA_AppendPtr(hSids, pSid);

    fResult = LookupSids(hSids, pszServer, psi2, ppUserList);

    if (NULL != hSids)
        DPA_Destroy(hSids);

    return fResult;
}

// Private data structure used by LookupSidsAsync to pass
// data needed by the thread
typedef struct _LOOKUPSIDSDATA
{
    HDPA hSids;
    LPTSTR pszServer;
    HWND hWndNotify;
    UINT uMsgNotify;
} LOOKUPSIDSDATA, *PLOOKUPSIDSDATA;


DWORD WINAPI
_LookupSidsAsyncProc(LPVOID pv)
{
    PLOOKUPSIDSDATA pdata = (PLOOKUPSIDSDATA)pv;

    if (pdata)
    {
        PSIDCACHE pSidCache = GetSidCache();

        if (NULL != pSidCache)
        {
            pSidCache->LookupSidsAsync(pdata->hSids,
                                       pdata->pszServer,
                                       NULL,
                                       pdata->hWndNotify,
                                       pdata->uMsgNotify);
            pSidCache->Release();
        }

        PostMessage(pdata->hWndNotify, pdata->uMsgNotify, 0, 0);

        DestroyDPA(pdata->hSids);
        LocalFreeString(&pdata->pszServer);
        LocalFree(pdata);
    }

    FreeLibraryAndExitThread(GetModuleHandle(c_szDllName), 0);
    return 0;
}

BOOL
LookupSidsAsync(HDPA hSids,
                LPCTSTR pszServer,
                LPSECURITYINFO2 psi2,
                HWND hWndNotify,
                UINT uMsgNotify,
                PHANDLE phThread)
{
    PLOOKUPSIDSDATA pdata;

    if (phThread)
        *phThread = NULL;

    if (NULL == hSids)
        return FALSE;

    if (psi2)
    {
        // Should marshal psi2 into a stream and do this on the
        // other thread.  BUGBUG
        BOOL bResult = LookupSids(hSids, pszServer, psi2, NULL);
        PostMessage(hWndNotify, uMsgNotify, 0, 0);
        return bResult;
    }

    //
    // Copy all of the data so the thread can be abandoned if necessary
    //
    pdata = (PLOOKUPSIDSDATA)LocalAlloc(LPTR, SIZEOF(LOOKUPSIDSDATA));
    if (pdata)
    {
        int cSids;
        int i;
        HINSTANCE hInstThisDll;
        DWORD dwThreadId;
        HANDLE hThread;

        cSids = DPA_GetPtrCount(hSids);
        pdata->hSids = DPA_Create(cSids);

        if (!pdata->hSids)
        {
            LocalFree(pdata);
            return FALSE;
        }

        for (i = 0; i < cSids; i++)
        {
            PSID p2 = LocalAllocSid((PSID)DPA_FastGetPtr(hSids, i));
            if (p2)
            {
                DPA_AppendPtr(pdata->hSids, p2);
            }
        }

        if (pszServer)
            LocalAllocString(&pdata->pszServer, pszServer);

        pdata->hWndNotify = hWndNotify;
        pdata->uMsgNotify = uMsgNotify;

        // Give the thread we are about to create a ref to the dll,
        // so that the dll will remain for the lifetime of the thread
        hInstThisDll = LoadLibrary(c_szDllName);

        hThread = CreateThread(NULL,
                               0,
                               _LookupSidsAsyncProc,
                               pdata,
                               NULL,
                               &dwThreadId);
        if (hThread != NULL)
        {
            if (phThread)
                *phThread = hThread;
            else
                CloseHandle(hThread);
            return TRUE;
        }
        else
        {
            // Thread creation has failed; clean up
            DestroyDPA(pdata->hSids);
            LocalFreeString(&pdata->pszServer);
            LocalFree(pdata);
            FreeLibrary(hInstThisDll);
        }
    }
    return FALSE;
}

BOOL
BuildUserDisplayName(LPTSTR *ppszDisplayName,
                     LPCTSTR pszName,
                     LPCTSTR pszLogonName)
{
    TCHAR szDisplayName[MAX_PATH];

    if (NULL == ppszDisplayName || NULL == pszName)
        return FALSE;

    *ppszDisplayName = NULL;

    if (NULL != pszLogonName && *pszLogonName)
    {
        return (BOOL)FormatStringID(ppszDisplayName,
                                    ::hModule,
                                    IDS_FMT_USER_DISPLAY,
                                    pszName,
                                    pszLogonName);
    }

    return SUCCEEDED(LocalAllocString(ppszDisplayName, pszName));
}


/*******************************************************************

    NAME:       LoadImageList

    SYNOPSIS:   Creates an image list from a bitmap resource

    ENTRY:      hInstance - the bitmap lives here
                pszBitmapID - resource ID of the bitmap

    EXIT:

    RETURNS:    HIMAGELIST if successful, NULL otherwise

    NOTES:
        In order to calculate the number of images, it is assumed
        that the width and height of a single image are the same.

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

HIMAGELIST
LoadImageList(HINSTANCE hInstance, LPCTSTR pszBitmapID)
{
    HIMAGELIST himl = NULL;
    HBITMAP hbm = LoadBitmap(hInstance, pszBitmapID);

    if (hbm != NULL)
    {
        BITMAP bm;
        GetObject(hbm, SIZEOF(bm), &bm);

        himl = ImageList_Create(bm.bmHeight,    // height == width
                                bm.bmHeight,
                                ILC_COLOR | ILC_MASK,
                                bm.bmWidth / bm.bmHeight,
                                0);  // don't need to grow
        if (himl != NULL)
            ImageList_AddMasked(himl, hbm, CLR_DEFAULT);

        DeleteObject(hbm);
    }

    return himl;
}


/*******************************************************************

    NAME:       GetSidImageIndex

    SYNOPSIS:   Gets the image index for the given SID type

    ENTRY:      sidType - type of SID
                sidSys - well-known group type
                fRemoteUser - TRUE if SID is a user on a remote system

    EXIT:

    RETURNS:    index into image list

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

SID_IMAGE_INDEX
GetSidImageIndex(PSID psid,
                 SID_NAME_USE sidType)
{
    SID_IMAGE_INDEX idBitmap;

    switch (sidType)
    {
    case SidTypeUser:
        idBitmap = SID_IMAGE_USER;
        break;

    case SidTypeAlias:
        // BUGBUG DsAdmin doesn't show a separate icon for aliases,
        // so we shouldn't either.  172193
        //idBitmap = SID_IMAGE_LOCALGROUP;
        //break;
    case SidTypeGroup:
    case SidTypeWellKnownGroup:
        idBitmap = SID_IMAGE_GROUP;
        break;

#if(_WIN32_WINNT >= 0x0500)
    case SidTypeComputer:
        idBitmap = SID_IMAGE_COMPUTER;
        break;
#endif

    default:
        idBitmap = SID_IMAGE_UNKNOWN;
        break;
    }

    return idBitmap;
}

#if(_WIN32_WINNT >= 0x0500)

#include <dsrole.h>
BOOL IsStandalone(LPCTSTR pszMachine, PBOOL pbIsDC)
{
    BOOL bStandalone = TRUE;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDsRole = NULL;

    //
    // Find out if target machine is a standalone machine or joined to
    // an NT domain.
    //

    __try
    {
        if (pbIsDC)
            *pbIsDC = FALSE;

        DsRoleGetPrimaryDomainInformation(pszMachine,
                                          DsRolePrimaryDomainInfoBasic,
                                          (PBYTE*)&pDsRole);
    }
    __finally
    {
    }

    if (NULL != pDsRole)
    {
        if (pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation ||
            pDsRole->MachineRole == DsRole_RoleStandaloneServer)
        {
            bStandalone = TRUE;
        }
        else
            bStandalone = FALSE;

        if (pbIsDC)
        {
            if (pDsRole->MachineRole == DsRole_RolePrimaryDomainController ||
                pDsRole->MachineRole == DsRole_RoleBackupDomainController)
            {
                *pbIsDC = TRUE;
            }
        }

        DsRoleFreeMemory(pDsRole);
    }

    return bStandalone;
}

#else   // _WIN32_WINNT < 0x0500

BOOL IsStandalone(LPCTSTR pszMachine, PBOOL pbIsDC)
{
    BOOL bStandalone = FALSE;

    // BUGBUG implement an NT4 version of this
    if (pbIsDC)
        *pbIsDC = FALSE;

    return bStandalone;
}


//
// Stuff used by GetUserGroup below
//
#include <getuser.h>

const TCHAR c_szGetUserLib[]        = TEXT("netui2.dll");
const char c_szOpenUB[]             = "OpenUserBrowser";
const char c_szEnumUBSelection[]    = "EnumUserBrowserSelection";
const char c_szCloseUB[]            = "CloseUserBrowser";

typedef HUSERBROW (WINAPI *PFN_UB_OPEN)(LPUSERBROWSER);
typedef BOOL (WINAPI *PFN_UB_ENUM)(HUSERBROW, LPUSERDETAILS, LPDWORD);
typedef BOOL (WINAPI *PFN_UB_CLOSE)(HUSERBROW);

PFN_UB_OPEN pfnUBOpen;
PFN_UB_ENUM pfnUBEnum;
PFN_UB_CLOSE pfnUBClose;

#ifndef HC_SED_USER_BROWSER_DIALOG
#define HC_SED_USER_BROWSER_DIALOG          4300
#define HC_SED_USER_BROWSER_AUDIT_DLG       4325
#endif


/*******************************************************************

    NAME:       GetUserGroup

    SYNOPSIS:   Invokes the old NT4 user/group picker dialog

    ENTRY:      hwndOwner - owner window
                dwFlags - indicate multi-select, etc.
                pszServer - initial focus of dialog
                ppUserList - out parameter

    EXIT:       *ppUserList contains a list of USER_INFO structures

    RETURNS:    HRESULT

    NOTES:      *ppUserList must be LocalFree'd by the caller.

    HISTORY:
        JeffreyS    16-Jan-1998     Created

********************************************************************/

HRESULT
GetUserGroup(HWND       hwndOwner,
             DWORD      dwFlags,
             LPCTSTR    pszServer,
             BOOL       /*bStandalone*/,
             PUSER_LIST *ppUserList)
{
    HRESULT hr = S_OK;
    HUSERBROW hUserBrowser = NULL;
    USERBROWSER ub;
    DWORD dwUDLength = 1024;
    PUSERDETAILS pUserDetails = NULL;
    PSID_CACHE_ENTRY pEntry;
    HDPA hEntryList = NULL;
    PSIDCACHE pSidCache = NULL;

    TraceEnter(TRACE_MISC, "GetUserGroup");
    TraceAssert(ppUserList != NULL);

    if (!ppUserList)
        TraceLeaveResult(E_INVALIDARG);

    *ppUserList = NULL;

    if (!g_hGetUserLib)
    {
        g_hGetUserLib = LoadLibrary(c_szGetUserLib);
        if (g_hGetUserLib == NULL)
            ExitGracefully(hr, E_FAIL, "Unable to load netui2.dll");

        pfnUBOpen = (PFN_UB_OPEN)GetProcAddress(g_hGetUserLib, c_szOpenUB);
        pfnUBEnum = (PFN_UB_ENUM)GetProcAddress(g_hGetUserLib, c_szEnumUBSelection);
        pfnUBClose = (PFN_UB_CLOSE)GetProcAddress(g_hGetUserLib, c_szCloseUB);

        if (!pfnUBOpen || !pfnUBEnum || !pfnUBClose)
        {
            FreeLibrary(g_hGetUserLib);
            g_hGetUserLib = NULL;
            ExitGracefully(hr, E_FAIL, "Unable to link to netui2.dll");
        }
    }

    //
    // Create the global sid cache object, if necessary
    //
    pSidCache = GetSidCache();

    if (pSidCache == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create SID cache");

    ub.ulStructSize = sizeof(ub);
    ub.fUserCancelled = FALSE;
    ub.fExpandNames = TRUE;
    ub.hwndOwner = hwndOwner;
    ub.pszTitle = NULL;
    ub.pszInitialDomain = (LPTSTR)pszServer;
    ub.Flags = USRBROWS_SHOW_ALL | USRBROWS_INCL_ALL;
    ub.ulHelpContext = HC_SED_USER_BROWSER_DIALOG;
    ub.pszHelpFileName = (LPWSTR)c_szAcluiHelpFile;

#ifdef USRBROWS_INCL_RESTRICTED
    ub.Flags &= ~USRBROWS_INCL_RESTRICTED;  // NT5 only
#endif

    if (!(dwFlags & GU_CONTAINER))
        ub.Flags &= ~USRBROWS_INCL_CREATOR;

    if (!(dwFlags & GU_MULTI_SELECT))
        ub.Flags |= USRBROWS_SINGLE_SELECT;

    if (dwFlags & GU_AUDIT_HLP)
        ub.ulHelpContext = HC_SED_USER_BROWSER_AUDIT_DLG;

    //
    // Open the dialog
    //
    hUserBrowser = (*pfnUBOpen)(&ub);
    if (hUserBrowser == NULL)
        ExitGracefully(hr, E_FAIL, "OpenUserBrowser returned false");

    pUserDetails = (PUSERDETAILS)LocalAlloc(LPTR, dwUDLength);
    if (!pUserDetails)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to allocate UserDetails buffer");

    hEntryList = DPA_Create(4);
    if (!hEntryList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create SID cache entry list");

    //
    // Enumerate the results
    //
    for (;;)
    {
        if (!(*pfnUBEnum)(hUserBrowser, pUserDetails, &dwUDLength))
        {
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
                // The details buffer wasn't big enough, reallocate it
                LocalFree(pUserDetails);
                pUserDetails = (PUSERDETAILS)LocalAlloc(LPTR, dwUDLength);
                if (pUserDetails == NULL)
                    break;

                if (!(*pfnUBEnum)(hUserBrowser, pUserDetails, &dwUDLength))
                    break;
            }
            else // probably ERROR_NO_MORE_ITEMS
                break;
        }

        //
        // See if it's already in the cache
        //
        pEntry = pSidCache->FindSid(pUserDetails->psidUser);

        if (NULL == pEntry)
        {
            //
            // Not in the cache, add it
            //
            TCHAR szAccountName[MAX_PATH];
            TCHAR szDomainName[MAX_PATH];
            ULONG nAccountLength;

            lstrcpy(szAccountName, pUserDetails->pszAccountName);
            lstrcpy(szDomainName, pUserDetails->pszDomainName);

            switch (pUserDetails->UserType)
            {
            case SidTypeUnknown:
            case SidTypeInvalid:
                // Load unknown account string
                LoadString(::hModule, IDS_SID_UNKNOWN, szAccountName, ARRAYSIZE(szAccountName));
                break;

            case SidTypeAlias:
                //if (IsAliasSid(pSid))
                //    szDomainName[0] = TEXT('\0');   // The domain is "BUILTIN"
                break;

            case SidTypeDeletedAccount:
                // Load deleted account string
                LoadString(::hModule, IDS_SID_DELETED, szAccountName, ARRAYSIZE(szAccountName));
                break;

            case SidTypeWellKnownGroup:
                // Don't include the domain for a well-known group
                szDomainName[0] = TEXT('\0');
                break;
            }

            //
            // Build NT4 "domain\user" style name (logon name)
            //
            if (szDomainName[0] != TEXT('\0'))
            {
                lstrcat(szDomainName, TEXT("\\"));
                lstrcat(szDomainName, szAccountName);
            }

            LPCTSTR pszCommonName = pUserDetails->pszFullName;
            if (!pszCommonName || !*pszCommonName)
                pszCommonName = pUserDetails->pszAccountName;

            pEntry = pSidCache->MakeEntry(pUserDetails->psidUser,
                                          pUserDetails->UserType,
                                          pszCommonName,
                                          szDomainName);
            if (NULL != pEntry)
                pSidCache->AddEntry(pEntry);
        }

        if (NULL != pEntry)
            DPA_AppendPtr(hEntryList, pEntry);
    }

    //
    // Build return list
    //
    if (DPA_GetPtrCount(hEntryList))
        pSidCache->BuildUserList(hEntryList, pszServer, ppUserList);

    if (NULL == *ppUserList)
        hr = E_FAIL;

exit_gracefully:

    if (pSidCache)
        pSidCache->Release();

    if (NULL != hUserBrowser)
        (*pfnUBClose)(hUserBrowser);

    if (pUserDetails != NULL)
        LocalFree(pUserDetails);

    DPA_Destroy(hEntryList);

    TraceLeaveResult(hr);
}

#endif  // _WIN32_WINNT < 0x0500


/*******************************************************************

    NAME:       IsDACLCanonical

    SYNOPSIS:   Checks a DACL for canonical ordering

    ENTRY:      pDacl - points to DACL to check

    EXIT:

    RETURNS:    Nonzero if DACL is in canonical order, zero otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created
        JeffreyS    03-Oct-1997     Make object aces same as non-object aces

********************************************************************/

enum ACELEVEL
{
    alNonInheritAccessDenied,
    alNonInheritAccessAllowed,
    alInheritedAces,
};

BOOL
IsDACLCanonical(PACL pDacl)
{
    PACE_HEADER pAce;
    ACELEVEL currentAceLevel;
    DWORD dwAceCount;

    if (pDacl == NULL)
        return TRUE;

    currentAceLevel = alNonInheritAccessDenied;
    dwAceCount = pDacl->AceCount;

    if (dwAceCount == 0)
        return TRUE;

    for (pAce = (PACE_HEADER)FirstAce(pDacl);
         dwAceCount > 0;
         --dwAceCount, pAce = (PACE_HEADER)NextAce(pAce))
    {
        ACELEVEL aceLevel;

        //
        // NOTE: We do not skip INHERIT_ONLY aces because we want them in
        // canonical order too.
        //

        if (pAce->AceFlags & INHERITED_ACE)
        {
            aceLevel = alInheritedAces;      // don't check order here
        }
        else
        {
            switch(pAce->AceType)
            {
            case ACCESS_DENIED_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                aceLevel = alNonInheritAccessDenied;
                break;

            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                aceLevel = alNonInheritAccessAllowed;
                break;

            default:
                return FALSE;
            }
        }

        //
        // If the ace type is less than the level we are currently at,
        // then it is not canonical.
        //
        if (aceLevel < currentAceLevel)
            return FALSE;

        //
        // Update the current ace level.
        //
        currentAceLevel = aceLevel;
    }

    //
    // If we get here, then the DACL is in canonical order.
    //
    return TRUE;
}


/*******************************************************************

    NAME:       IsDenyACL

    SYNOPSIS:   Checks a DACL for Deny ACEs.  Also looks for "Deny
                All" ACEs.

    ENTRY:      pDacl - points to DACL to check

    EXIT:       *pdwWarning is 0, IDS_PERM_DENY, or IDS_PERM_DENY_ALL 

    RETURNS:    Nonzero if DACL contains any Deny ACEs, zero otherwise

    NOTES:

    HISTORY:
        JeffreyS    05-Sep-1997     Created

********************************************************************/

BOOL
IsDenyACL(PACL pDacl,
          BOOL fProtected,
          DWORD dwFullControlMask,
          LPDWORD pdwWarning)
{
    DWORD dwWarning = 0;

    TraceEnter(TRACE_MISC, "IsDenyACL");

    // NULL DACL implies "Allow Everyone Full Control"
    if (pDacl == NULL)
        goto exit_gracefully;

    // Check for empty DACL (no access to anyone)
    if (pDacl->AceCount == 0)
    {
        if (fProtected)
            dwWarning = IDS_PERM_DENY_ALL;
        // else the object will inherit permissions from the parent.
    }
    else
    {
        PACE_HEADER pAce;
        int iEntry;

        // Iterate through the ACL looking for "Deny All"
        for (iEntry = 0, pAce = (PACE_HEADER)FirstAce(pDacl);
             iEntry < pDacl->AceCount;
             iEntry++, pAce = (PACE_HEADER)NextAce(pAce))
        {
            if (pAce->AceType != ACCESS_DENIED_ACE_TYPE &&
                pAce->AceType != ACCESS_DENIED_OBJECT_ACE_TYPE)
            {
                // Assuming the ACL is in canonical order, we can
                // stop as soon as we find something that isn't
                // a Deny ACE.  (Deny ACEs come first)
                break;
            }

            // Found a Deny ACE
            dwWarning = IDS_PERM_DENY;

            // Check for "Deny Everyone Full Control". Don't look
            // for ACCESS_DENIED_OBJECT_ACE_TYPE here since Object
            // aces don't have as wide an effect as normal aces.
            if (pAce->AceType == ACCESS_DENIED_ACE_TYPE &&
                ((PKNOWN_ACE)pAce)->Mask == dwFullControlMask &&
                EqualSid(GetAceSid(pAce), QuerySystemSid(UI_SID_World)))
            {
                // Found "Deny All"
                dwWarning = IDS_PERM_DENY_ALL;
                break;
            }
        }
    }

exit_gracefully:

    if (pdwWarning != NULL)
        *pdwWarning = dwWarning;

    TraceLeaveValue(dwWarning != 0);
}


/*******************************************************************

    NAME:       QuerySystemSid

    SYNOPSIS:   Retrieves the requested SID

    ENTRY:      SystemSidType - Which SID to retrieve

    EXIT:

    RETURNS:    PSID if successful, NULL otherwise

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

//
// Global array of static system SIDs, corresponding to UI_SystemSid
//
const struct
{
    SID sid;            // contains 1 subauthority
    DWORD dwSubAuth[1]; // we currently need at most 2 subauthorities
} g_StaticSids[COUNT_SYSTEM_SID_TYPES] =
{
    {{SID_REVISION,1,SECURITY_WORLD_SID_AUTHORITY,  {SECURITY_WORLD_RID}},              {0}                             },
    {{SID_REVISION,1,SECURITY_CREATOR_SID_AUTHORITY,{SECURITY_CREATOR_OWNER_RID}},      {0}                             },
    {{SID_REVISION,1,SECURITY_CREATOR_SID_AUTHORITY,{SECURITY_CREATOR_GROUP_RID}},      {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_DIALUP_RID}},             {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_NETWORK_RID}},            {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_BATCH_RID}},              {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_INTERACTIVE_RID}},        {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_SERVICE_RID}},            {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_ANONYMOUS_LOGON_RID}},    {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_PROXY_RID}},              {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_ENTERPRISE_CONTROLLERS_RID}},{0}                          },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_PRINCIPAL_SELF_RID}},     {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_AUTHENTICATED_USER_RID}}, {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_RESTRICTED_CODE_RID}},    {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_TERMINAL_SERVER_RID}},    {0}                             },
    {{SID_REVISION,1,SECURITY_NT_AUTHORITY,         {SECURITY_LOCAL_SYSTEM_RID}},       {0}                             },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_ADMINS}       },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_USERS}        },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_GUESTS}       },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_POWER_USERS}  },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_ACCOUNT_OPS}  },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_SYSTEM_OPS}   },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_PRINT_OPS}    },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_BACKUP_OPS}   },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_REPLICATOR}   },
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_RAS_SERVERS}  },
};

PSID
QuerySystemSid(UI_SystemSid SystemSidType)
{
    if (SystemSidType == UI_SID_Invalid || SystemSidType >= UI_SID_Count)
        return NULL;

    return (PSID)&g_StaticSids[SystemSidType];
}


//
// Global array of cached token SIDs
//
struct
{
    SID sid;            // SID contains 1 subauthority
    DWORD dwSubAuth[SID_MAX_SUB_AUTHORITIES - 1];
} g_TokenSids[COUNT_TOKEN_SID_TYPES] = {0};

PSID
QueryTokenSid(UI_TokenSid TokenSidType)
{
    if (TokenSidType == UI_TSID_Invalid || TokenSidType >= UI_TSID_Count)
        return NULL;

    if (0 == *GetSidSubAuthorityCount((PSID)&g_TokenSids[TokenSidType]))
    {
        HANDLE hProcessToken;

        // Get the current process's user's SID
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
        {
            BYTE buffer[sizeof(TOKEN_USER) + sizeof(g_TokenSids[0])];
            ULONG cbBuffer = sizeof(buffer);

            switch (TokenSidType)
            {
            case UI_TSID_CurrentProcessUser:
                if (GetTokenInformation(hProcessToken,
                                        TokenUser,
                                        buffer,
                                        cbBuffer,
                                        &cbBuffer))
                {
                    PTOKEN_USER ptu = (PTOKEN_USER)buffer;
                    CopyMemory(&g_TokenSids[UI_TSID_CurrentProcessUser],
                               ptu->User.Sid,
                               GetLengthSid(ptu->User.Sid));
                }
                break;

            case UI_TSID_CurrentProcessOwner:
                if (GetTokenInformation(hProcessToken,
                                        TokenOwner,
                                        buffer,
                                        cbBuffer,
                                        &cbBuffer))
                {
                    PTOKEN_OWNER pto = (PTOKEN_OWNER)buffer;
                    CopyMemory(&g_TokenSids[UI_TSID_CurrentProcessOwner],
                               pto->Owner,
                               GetLengthSid(pto->Owner));
                }
                break;

            case UI_TSID_CurrentProcessPrimaryGroup:
                if (GetTokenInformation(hProcessToken,
                                        TokenPrimaryGroup,
                                        buffer,
                                        cbBuffer,
                                        &cbBuffer))
                {
                    PTOKEN_PRIMARY_GROUP ptg = (PTOKEN_PRIMARY_GROUP)buffer;
                    CopyMemory(&g_TokenSids[UI_TSID_CurrentProcessPrimaryGroup],
                               ptg->PrimaryGroup,
                               GetLengthSid(ptg->PrimaryGroup));
                }
                break;
            }
            CloseHandle(hProcessToken);
        }

        if (0 == *GetSidSubAuthorityCount((PSID)&g_TokenSids[TokenSidType]))
            return NULL;
    }

    return (PSID)&g_TokenSids[TokenSidType];
}


/*******************************************************************

    NAME:       GetAuthenticationID

    SYNOPSIS:   Retrieves the SID associated with the credentials
                currently being used for network access.
                (runas /netonly credentials)

    ENTRY:      pszServer = server on which to lookup the account.
                            NULL indicates local system.

    EXIT:

    RETURNS:    PSID if successful, NULL otherwise.  Caller must
                free with LocalFree.

    HISTORY:
        JeffreyS    05-Aug-1999     Created

********************************************************************/
PSID
GetAuthenticationID(LPCWSTR pszServer)
{
    PSID pSid = NULL;
    HANDLE hLsa;
    NTSTATUS Status;

    //
    // These LSA calls are delay-loaded from secur32.dll using the linker's
    // delay-load mechanism.  Therefore, wrap with an exception handler.
    //
    __try
    {
        Status = LsaConnectUntrusted(&hLsa);

        if (Status == 0)
        {
            NEGOTIATE_CALLER_NAME_REQUEST Req = {0};
            PNEGOTIATE_CALLER_NAME_RESPONSE pResp;
            ULONG cbSize;
            NTSTATUS SubStatus;

            Req.MessageType = NegGetCallerName;

            Status = LsaCallAuthenticationPackage(
                            hLsa,
                            0,
                            &Req,
                            sizeof(Req),
                            (void**)&pResp,
                            &cbSize,
                            &SubStatus);

            if ((Status == 0) && (SubStatus == 0))
            {
                BYTE sid[sizeof(SID) + (SID_MAX_SUB_AUTHORITIES - 1)*sizeof(DWORD)];
                PSID psid = (PSID)sid;
                DWORD cbSid = sizeof(sid);
                WCHAR szDomain[MAX_PATH];
                DWORD cchDomain = ARRAYSIZE(szDomain);
                SID_NAME_USE sidType;

                if (LookupAccountNameW(pszServer,
                                       pResp->CallerName,
                                       psid,
                                       &cbSid,
                                       szDomain,
                                       &cchDomain,
                                       &sidType))
                {
                    pSid = LocalAllocSid(psid);
                }

                LsaFreeReturnBuffer(pResp);
            }

            LsaDeregisterLogonProcess(hLsa);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return pSid;
}


/*******************************************************************

    NAME:       CopyUnicodeString

    SYNOPSIS:   Allocates a buffer and copies a string from
                a UNICODE_STRING sources.

    ENTRY:      pszDest - pointer to destination buffer
                cchDest - # of chars in pszDest (bytes for MBCS)
                pSrc - pointer to UNICODE_STRING to copy

    EXIT:       pszDest - containing copy of string

    RETURNS:    # of chars copied, or 0 if not successful.

    NOTES:

    HISTORY:
        JeffreyS    22-Jan-1998     Created

********************************************************************/

int
CopyUnicodeString(LPTSTR pszDest, ULONG cchDest, PLSA_UNICODE_STRING pSrc)
{
    int nResult;
    ULONG cchSrc;

    // If UNICODE, cchDest is size of destination buffer in chars
    // Else (MBCS) cchDest is size of destination buffer in bytes

    if (pszDest == NULL || 0 == cchDest)
        return 0;

    *pszDest = TEXT('\0');

    if (pSrc == NULL || pSrc->Buffer == NULL)
        return 0;

    // Get # of chars in source (not including NULL)
    cchSrc = pSrc->Length/sizeof(WCHAR);

#ifdef UNICODE
    //
    // Note that pSrc->Buffer may not be NULL terminated so we can't just
    // call lstrcpynW with cchDest.  Also, if we call lstrcpynW with cchSrc,
    // it copies the correct # of chars, but then overwrites the last char
    // with NULL giving an incorrect result.  If we call lstrcpynW with
    // (cchSrc+1) it reads past the end of the buffer, which may fault (360251)
    // causing lstrcpynW's exception handler to return 0 without NULL-
    // terminating the resulting string.
    //
    // So let's just copy the bits.
    //
    nResult = min(cchSrc, cchDest);
    CopyMemory(pszDest, pSrc->Buffer, sizeof(WCHAR)*nResult);
    if (nResult == (int)cchDest)
        --nResult;
    pszDest[nResult] = L'\0';
#else
    nResult = WideCharToMultiByte(CP_ACP,
                                  0,
                                  pSrc->Buffer,
                                  cchSrc,
                                  pszDest,
                                  cchDest,
                                  NULL,
                                  NULL);
#endif

    return nResult;
}


/*******************************************************************

    NAME:       CopyUnicodeString

    SYNOPSIS:   Allocates a buffer and copies a string from
                a UNICODE_STRING sources.

    ENTRY:      pSrc - pointer to UNICODE_STRING to copy

    EXIT:       *ppszResult - points to LocalAlloc'd buffer containing copy.

    RETURNS:    # of chars copied, or 0 if not successful.

    NOTES:

    HISTORY:
        JeffreyS    22-Jan-1998     Created

********************************************************************/

int
CopyUnicodeString(LPTSTR *ppszResult, PLSA_UNICODE_STRING pSrc)
{
    int nResult = 0;

    if (NULL == ppszResult)
        return 0;

    *ppszResult = NULL;

    if (NULL != pSrc)
    {
        ULONG cchResult;

        *ppszResult = NULL;

        // Get # of chars in source (including NULL)
        cchResult = pSrc->Length/SIZEOF(WCHAR) + 1;

        // Allocate buffer big enough for either UNICODE or MBCS result
        *ppszResult = (LPTSTR)LocalAlloc(LPTR, cchResult * 2);

        if (*ppszResult)
        {
            nResult = CopyUnicodeString(*ppszResult, cchResult, pSrc);

            if (0 == nResult)
            {
                LocalFree(*ppszResult);
                *ppszResult = NULL;
            }
        }
    }

    return nResult;
}


//
// Test GUIDs safely
//
BOOL IsSameGUID(const GUID *p1, const GUID *p2)
{
    BOOL bResult = FALSE;

    if (!p1) p1 = &GUID_NULL;
    if (!p2) p2 = &GUID_NULL;

    __try
    {
        bResult = InlineIsEqualGUID(*p1, *p2);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return bResult;
}
