//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ntfssi.cpp
//
//  This file contains the implementation of the CNTFSSecurity object.
//
//--------------------------------------------------------------------------

#include "rshx32.h"
#include <windowsx.h>   // GET_WM_COMMAND_ID, etc.
#include <atlconv.h>

#define MY_FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED                    \
                            | SYNCHRONIZE                               \
                            | FILE_READ_DATA    | FILE_LIST_DIRECTORY   \
                            | FILE_WRITE_DATA   | FILE_ADD_FILE         \
                            | FILE_APPEND_DATA  | FILE_ADD_SUBDIRECTORY \
                            | FILE_CREATE_PIPE_INSTANCE                 \
                            | FILE_READ_EA                              \
                            | FILE_WRITE_EA                             \
                            | FILE_EXECUTE      | FILE_TRAVERSE         \
                            | FILE_DELETE_CHILD                         \
                            | FILE_READ_ATTRIBUTES                      \
                            | FILE_WRITE_ATTRIBUTES)

#if(FILE_ALL_ACCESS != MY_FILE_ALL_ACCESS)
#error ACL editor needs to sync with file permissions changes in ntioapi.h (or ntioapi.h is broken)
#endif

#define INHERIT_FULL            (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

//
// Treat SYNCHRONIZE specially. In particular, always allow SYNCHRONIZE and
// never Deny SYNCHRONIZE. Do this by removing it from the Generic Mapping,
// turning it off in all ACEs and SI_ACCESS entries, and then adding it to
// all Allow ACEs before saving a new ACL.
//
#define FILE_GENERIC_READ_      (FILE_GENERIC_READ    & ~SYNCHRONIZE)
#define FILE_GENERIC_WRITE_     (FILE_GENERIC_WRITE   & ~(SYNCHRONIZE | READ_CONTROL))
#define FILE_GENERIC_EXECUTE_   (FILE_GENERIC_EXECUTE & ~SYNCHRONIZE)
#define FILE_GENERIC_ALL_       (FILE_ALL_ACCESS      & ~SYNCHRONIZE)

#define FILE_GENERAL_MODIFY     (FILE_GENERIC_READ_  | FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_ | DELETE)
#define FILE_GENERAL_PUBLISH    (FILE_GENERIC_READ_  | FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_)
#define FILE_GENERAL_DEPOSIT    (FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_)
#define FILE_GENERAL_READ_EX    (FILE_GENERIC_READ_  | FILE_GENERIC_EXECUTE_)

// The following array defines the permission names for NTFS objects.
SI_ACCESS siNTFSAccesses[] =
{
    { &GUID_NULL, FILE_GENERIC_ALL_,    MAKEINTRESOURCE(IDS_NTFS_GENERIC_ALL),      SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_MODIFY,  MAKEINTRESOURCE(IDS_NTFS_GENERAL_MODIFY),   SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_READ_EX, MAKEINTRESOURCE(IDS_NTFS_GENERAL_READ),     SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_READ_EX, MAKEINTRESOURCE(IDS_NTFS_GENERAL_LIST),     SI_ACCESS_CONTAINER | CONTAINER_INHERIT_ACE },
    { &GUID_NULL, FILE_GENERIC_READ_,   MAKEINTRESOURCE(IDS_NTFS_GENERIC_READ),     SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERIC_WRITE_,  MAKEINTRESOURCE(IDS_NTFS_GENERIC_WRITE),    SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_EXECUTE,         MAKEINTRESOURCE(IDS_NTFS_FILE_EXECUTE),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_DATA,       MAKEINTRESOURCE(IDS_NTFS_FILE_READ_DATA),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_ATTRIBUTES, MAKEINTRESOURCE(IDS_NTFS_FILE_READ_ATTR),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_EA,         MAKEINTRESOURCE(IDS_NTFS_FILE_READ_EA),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_DATA,      MAKEINTRESOURCE(IDS_NTFS_FILE_WRITE_DATA),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_APPEND_DATA,     MAKEINTRESOURCE(IDS_NTFS_FILE_APPEND_DATA), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_ATTRIBUTES,MAKEINTRESOURCE(IDS_NTFS_FILE_WRITE_ATTR),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_EA,        MAKEINTRESOURCE(IDS_NTFS_FILE_WRITE_EA),    SI_ACCESS_SPECIFIC },
#if(FILE_CREATE_PIPE_INSTANCE != FILE_APPEND_DATA)
    { &GUID_NULL, FILE_CREATE_PIPE_INSTANCE, MAKEINTRESOURCE(IDS_NTFS_FILE_CREATE_PIPE), SI_ACCESS_SPECIFIC },
#endif
    { &GUID_NULL, FILE_DELETE_CHILD,    MAKEINTRESOURCE(IDS_NTFS_FILE_DELETE_CHILD),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,               MAKEINTRESOURCE(IDS_NTFS_STD_DELETE),       SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,         MAKEINTRESOURCE(IDS_NTFS_STD_READ_CONTROL), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,            MAKEINTRESOURCE(IDS_NTFS_STD_WRITE_DAC),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,          MAKEINTRESOURCE(IDS_NTFS_STD_WRITE_OWNER),  SI_ACCESS_SPECIFIC },
//    { &GUID_NULL, SYNCHRONIZE,          MAKEINTRESOURCE(IDS_NTFS_STD_SYNCHRONIZE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                    MAKEINTRESOURCE(IDS_NONE),                  0 },
    { &GUID_NULL, FILE_GENERIC_EXECUTE_,MAKEINTRESOURCE(IDS_NTFS_GENERIC_EXECUTE),  0 },
    { &GUID_NULL, FILE_GENERAL_DEPOSIT, MAKEINTRESOURCE(IDS_NTFS_GENERAL_DEPOSIT),  0 },
    { &GUID_NULL, FILE_GENERAL_PUBLISH, MAKEINTRESOURCE(IDS_NTFS_GENERAL_PUBLISH),  0 },
};
#define iNTFSDefAccess      2   // FILE_GENERAL_READ_EX


// The following array defines the inheritance types for NTFS directories.
SI_INHERIT_TYPE siNTFSInheritTypes[] =
{
    &GUID_NULL, 0,                                                             MAKEINTRESOURCE(IDS_NTFS_FOLDER),
    &GUID_NULL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_NTFS_FOLDER_SUBITEMS),
    &GUID_NULL, CONTAINER_INHERIT_ACE,                                         MAKEINTRESOURCE(IDS_NTFS_FOLDER_SUBFOLDER),
    &GUID_NULL, OBJECT_INHERIT_ACE,                                            MAKEINTRESOURCE(IDS_NTFS_FOLDER_FILE),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, MAKEINTRESOURCE(IDS_NTFS_SUBITEMS_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,                      MAKEINTRESOURCE(IDS_NTFS_SUBFOLDER_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,                         MAKEINTRESOURCE(IDS_NTFS_FILE_ONLY),
};


STDMETHODIMP
CheckFileAccess(LPCTSTR pszObjectName, LPDWORD pdwAccessGranted)
{
    HRESULT hr = S_OK;
    UINT i;
    UNICODE_STRING usNtFileName = {0};
    DWORD dwAccessDesired[] = { ALL_SECURITY_ACCESS,
                                READ_CONTROL,
                                WRITE_DAC,
                                WRITE_OWNER,
                                ACCESS_SYSTEM_SECURITY };

    TraceEnter(TRACE_NTFSSI, "CheckFileAccess");
    TraceAssert(pdwAccessGranted != NULL);

    *pdwAccessGranted = 0;

    if (!RtlDosPathNameToNtPathName_U(pszObjectName,
                                      &usNtFileName,
                                      NULL,
                                      NULL))
    {
        ExitGracefully(hr, E_OUTOFMEMORY, "RtlDosPathNameToNtPathName_U failed");
    }

    for (i = 0; i < ARRAYSIZE(dwAccessDesired); i++)
    {
        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK StatusBlock;
        DWORD dwErr;
        HANDLE hFile;

        if ((dwAccessDesired[i] & *pdwAccessGranted) == dwAccessDesired[i])
            continue;   // already have this access

        InitializeObjectAttributes(&oa,
                                   &usNtFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   0,
                                   0);

        dwErr = RtlNtStatusToDosError(NtOpenFile(&hFile,
                                                 dwAccessDesired[i],
                                                 &oa,
                                                 &StatusBlock,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 0));
        if (dwErr == 0)
        {
            *pdwAccessGranted |= dwAccessDesired[i];
            NtClose(hFile);
        }
        else if (dwErr != ERROR_ACCESS_DENIED &&
                 dwErr != ERROR_PRIVILEGE_NOT_HELD)
        {
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "NtOpenFile failed");
        }
    }

exit_gracefully:

    RtlFreeUnicodeString(&usNtFileName);

    Trace((TEXT("Access = 0x%08x"), *pdwAccessGranted));
    TraceLeaveResult(hr);
}


DWORD
GetCurrentToken(DWORD dwDesiredAccess, PHANDLE phToken)
{
    DWORD dwErr = NOERROR;
    if (!OpenThreadToken(GetCurrentThread(), dwDesiredAccess, TRUE, phToken)
        && !OpenProcessToken(GetCurrentProcess(), dwDesiredAccess, phToken))
    {
        dwErr = GetLastError();
    }
    return dwErr;
}



///////////////////////////////////////////////////////////
//
// Constructor/destructor
//
///////////////////////////////////////////////////////////

NTFS_COMPARE_DATA::~NTFS_COMPARE_DATA()
{
    LocalFreeString(&pszSaclConflict);
    LocalFreeString(&pszDaclConflict);
}

CNTFSSecurity::CNTFSSecurity(SE_OBJECT_TYPE seType)
: CSecurityInformation(seType)
{
}

CNTFSSecurity::~CNTFSSecurity()
{
    if (m_pCompareData != NULL)
        m_pCompareData->bAbortThread = TRUE;

    WaitForComparison();
    delete m_pCompareData;
}

STDMETHODIMP
CNTFSSecurity::Initialize(HDPA      hItemList,
                          DWORD     dwFlags,
                          LPTSTR    pszServer,
                          LPTSTR    pszObject)
{
    HRESULT hr;

    //
    // If we're editing the owner on a folder, turn on the Recurse button.
    //
    if (dwFlags & SI_CONTAINER)
    {
        if ((dwFlags & (SI_EDIT_OWNER | SI_OWNER_READONLY)) == SI_EDIT_OWNER)
            dwFlags |= SI_OWNER_RECURSE;

        if (!(dwFlags & SI_READONLY))
            dwFlags |= SI_RESET_DACL_TREE;

        if (dwFlags & SI_EDIT_AUDITS)
            dwFlags |= SI_RESET_SACL_TREE;
    }

    //
    // Let the base class do its thing
    //
    hr = CSecurityInformation::Initialize(hItemList,
                                          dwFlags,
                                          pszServer,
                                          pszObject);

    //
    // If multiple selection, create thread to compare security descriptors
    //
    if (m_hItemList && DPA_GetPtrCount(m_hItemList) > 1)
    {
        m_pCompareData = new NTFS_COMPARE_DATA(m_hItemList, m_dwSIFlags);

        if (m_pCompareData != NULL)
        {
            DWORD dwID;

            m_hCompareThread = CreateThread(NULL,
                                            0,
                                            NTFSCompareThreadProc,
                                            m_pCompareData,
                                            CREATE_SUSPENDED,
                                            &dwID);
            if (m_hCompareThread != NULL)
            {
                SetThreadPriority(m_hCompareThread, THREAD_PRIORITY_BELOW_NORMAL);
                ResumeThread(m_hCompareThread);
            }
            else
            {
                delete m_pCompareData;
                m_pCompareData = NULL;
            }
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CNTFSSecurity::GetAccessRights(const GUID* /*pguidObjectType*/,
                               DWORD /*dwFlags*/,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::GetAccessRights");
    TraceAssert(ppAccesses != NULL);
    TraceAssert(pcAccesses != NULL);
    TraceAssert(piDefaultAccess != NULL);

    *ppAccesses = siNTFSAccesses;
    *pcAccesses = ARRAYSIZE(siNTFSAccesses);
    *piDefaultAccess = iNTFSDefAccess;

    TraceLeaveResult(S_OK);
}

GENERIC_MAPPING NTFSMap =
{
    FILE_GENERIC_READ_,
    FILE_GENERIC_WRITE_,
    FILE_GENERIC_EXECUTE_,
    FILE_GENERIC_ALL_
};

STDMETHODIMP
CNTFSSecurity::MapGeneric(const GUID* /*pguidObjectType*/,
                          UCHAR * /*pAceFlags*/,
                          ACCESS_MASK *pmask)
{
    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::MapGeneric");
    TraceAssert(pmask != NULL);

    MapGenericMask(pmask, &NTFSMap);
    *pmask &= ~SYNCHRONIZE;

    TraceLeaveResult(S_OK);
}

STDMETHODIMP
CNTFSSecurity::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::GetInheritTypes");
    TraceAssert(ppInheritTypes != NULL);
    TraceAssert(pcInheritTypes != NULL);

    if (m_dwSIFlags & SI_CONTAINER)
    {
        *ppInheritTypes = siNTFSInheritTypes;
        *pcInheritTypes = ARRAYSIZE(siNTFSInheritTypes);
        TraceLeaveResult(S_OK);
    }

    TraceLeaveResult(E_NOTIMPL);
}

STDMETHODIMP
CNTFSSecurity::GetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault)
{
    HRESULT hr = S_OK;
    SECURITY_INFORMATION siConflict = 0;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::GetSecurity");
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    *ppSD = NULL;

    if (fDefault)
    {
#ifdef SUPPORT_NTFS_DEFAULT
        hr = GetDefaultSecurity(si, ppSD);
        ExitGracefully(hr, hr, "Returning default security descriptor");
#else
        ExitGracefully(hr, E_NOTIMPL, "Default security descriptor not supported");
#endif
    }

    WaitForComparison();

    if (m_pCompareData != NULL)
    {
        // Of the bits requested (si) figure out which ones
        // conflict and which ones don't.

        // First check DACL and SACL
        SECURITY_INFORMATION siAcl = si & m_pCompareData->siConflict;
        siAcl &= (SACL_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION);
        if (siAcl)
        {
            // To get here, we've already told the user there is a conflict
            // and they said to reset the ACL(s) and continue.

            // Build security descriptor with empty DACL and NULL SACL
            SECURITY_DESCRIPTOR sdEmpty = {0};
            ACL aclEmpty = {0};
            InitializeSecurityDescriptor(&sdEmpty, SECURITY_DESCRIPTOR_REVISION);
            InitializeAcl(&aclEmpty, sizeof(ACL), ACL_REVISION);
            SetSecurityDescriptorSacl(&sdEmpty, TRUE, &aclEmpty, FALSE);
            SetSecurityDescriptorDacl(&sdEmpty, TRUE, &aclEmpty, FALSE);

            // Reset the DACL and/or SACL
            hr = SetSecurity(siAcl, (PSECURITY_DESCRIPTOR)&sdEmpty);
        }

        // Note that SetSecurity will free m_pCompareData if there are
        // no more conflicts, so check the pointer again here.
        if (m_pCompareData != NULL)
            siConflict = si & m_pCompareData->siConflict;
    }

    // Read it from the first item.
    hr = CSecurityInformation::GetSecurity(si, ppSD, fDefault);

    if (SUCCEEDED(hr) && siConflict != 0)
    {
        //
        // Clear out any of the parts that conflict
        //

        PISECURITY_DESCRIPTOR psd = (PISECURITY_DESCRIPTOR)*ppSD;
        TraceAssert(psd != NULL);

        if (siConflict & OWNER_SECURITY_INFORMATION)
        {
            psd->Owner = NULL;
        }

        if (siConflict & GROUP_SECURITY_INFORMATION)
        {
            psd->Group = NULL;
        }

        // The following can happen if there was an error resetting ACLs above
        // BUGBUG it might be better to return what we've got instead

        if (siConflict & SACL_SECURITY_INFORMATION)
        {
            psd->Control &= ~SE_SACL_PRESENT;
            psd->Sacl = NULL;
        }

        if (siConflict & DACL_SECURITY_INFORMATION)
        {
            psd->Control &= ~SE_DACL_PRESENT;
            psd->Dacl = NULL;
        }
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


//
// See comments about SYNCHRONIZE at the top of this file
//
void
FixSynchronizeAccess(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    if (NULL != pSD && 0 != (si & DACL_SECURITY_INFORMATION))
    {
        BOOL bPresent;
        BOOL bDefault;
        PACL pDacl = NULL;

        GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefault);

        if (pDacl)
        {
            PACE_HEADER pAce;
            int i;

            for (i = 0, pAce = (PACE_HEADER)FirstAce(pDacl);
                 i < pDacl->AceCount;
                 i++, pAce = (PACE_HEADER)NextAce(pAce))
            {
                if (ACCESS_ALLOWED_ACE_TYPE == pAce->AceType)
                    ((PKNOWN_ACE)pAce)->Mask |= SYNCHRONIZE;
            }
        }
    }
}


STDMETHODIMP
CNTFSSecurity::SetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;
    SECURITY_INFORMATION siLocal = 0;
    SECURITY_DESCRIPTOR sdLocal = {0};
    ACL aclEmpty = {0};
    BOOL bUserCancelled = FALSE;

    //
    // Create a security descriptor with no SACL and an
    // empty DACL for recursively resetting security
    //
    InitializeSecurityDescriptor(&sdLocal, SECURITY_DESCRIPTOR_REVISION);
    InitializeAcl(&aclEmpty, sizeof(ACL), ACL_REVISION);
    SetSecurityDescriptorSacl(&sdLocal, TRUE, &aclEmpty, FALSE);
    SetSecurityDescriptorDacl(&sdLocal, TRUE, &aclEmpty, FALSE);

    //
    // If we need to recursively set the Owner, get the Owner &
    // Group from pSD.
    //
    if (si & SI_OWNER_RECURSE)
    {
        PSID psid;
        BOOL bDefaulted;

        TraceAssert(si & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION));

        siLocal |= si & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION);
        si &= ~(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION);

        if (GetSecurityDescriptorOwner(pSD, &psid, &bDefaulted))
            SetSecurityDescriptorOwner(&sdLocal, psid, bDefaulted);

        if (GetSecurityDescriptorGroup(pSD, &psid, &bDefaulted))
            SetSecurityDescriptorGroup(&sdLocal, psid, bDefaulted);
    }

    if (si & SI_RESET_DACL_TREE)
    {
        TraceAssert(si & DACL_SECURITY_INFORMATION);
        siLocal |= si & DACL_SECURITY_INFORMATION;
    }

    if (si & SI_RESET_SACL_TREE)
    {
        TraceAssert(si & SACL_SECURITY_INFORMATION);
        siLocal |= si & SACL_SECURITY_INFORMATION;
    }

    //
    // Recursively apply new Owner and/or reset the ACLs
    //
    if (siLocal)
    {
        hr = SetSecurityLocal(siLocal, &sdLocal, NULL);

        // Remember whether the user cancelled, since hr gets
        // reset when we call the base class below.
        if (S_FALSE == hr)
            bUserCancelled = TRUE;
    }

    si &= ~(SI_OWNER_RECURSE | SI_RESET_DACL_TREE | SI_RESET_SACL_TREE);

    if (si != 0)
    {
        // See comments about SYNCHRONIZE at the top of this file
        if (si & DACL_SECURITY_INFORMATION)
            FixSynchronizeAccess(si, pSD);

        // Call the base class to do the rest
        hr = CSecurityInformation::SetSecurity(si, pSD);
    }

    if (S_OK == hr && m_pCompareData)
    {
        // If we successfully wrote it, then it doesn't conflict anymore
        m_pCompareData->siConflict &= ~(si | siLocal);

        if (0 == m_pCompareData->siConflict)
        {
            delete m_pCompareData;
            m_pCompareData = NULL;
        }
    }

    return (bUserCancelled ? S_FALSE : hr);
}


STDMETHODIMP
CNTFSSecurity::PropertySheetPageCallback(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage)
{
    HRESULT hr;
    LPUINT pidsPrompt = NULL;
    LPCTSTR pszFile2 = NULL;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::PropertySheetPageCallback");

    hr = CSecurityInformation::PropertySheetPageCallback(hwnd, uMsg, uPage);

    if (uMsg == PSPCB_SI_INITDIALOG)
    {
        WaitForComparison();

        if (m_pCompareData != NULL)
        {
            if (SUCCEEDED(m_pCompareData->hrResult))
            {
                switch (uPage)
                {
                case SI_PAGE_PERM:
                case SI_PAGE_ADVPERM:
                    pidsPrompt = &m_pCompareData->idsDaclPrompt;
                    pszFile2 = m_pCompareData->pszDaclConflict;
                    break;

                case SI_PAGE_AUDIT:
                    pidsPrompt = &m_pCompareData->idsSaclPrompt;
                    pszFile2 = m_pCompareData->pszSaclConflict;
                    break;
                }
            }
        }

        if (pidsPrompt != NULL && *pidsPrompt != 0)
        {
            if (IDYES != MsgPopup(hwnd,
                                  MAKEINTRESOURCE(*pidsPrompt),
                                  MAKEINTRESOURCE(IDS_PROPPAGE_TITLE),
                                  MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND,
                                  g_hInstance,
                                  m_pszObjectName,
                                  pszFile2))
            {
                hr = E_FAIL;    // abort
            }

            // Don't want to prompt again for the same thing, so set
            // this to zero.
            *pidsPrompt = 0;
        }
    }

    TraceLeaveResult(hr);
}

#ifdef SUPPORT_NTFS_DEFAULT
HRESULT
CNTFSSecurity::GetParentSecurity(SECURITY_INFORMATION si,
                                 PSECURITY_DESCRIPTOR *ppSD)
{
    HRESULT hr = S_OK;
    LPTSTR pszItem;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::GetParentSecurity");
    TraceAssert(m_hItemList && DPA_GetPtrCount(m_hItemList));

    // Get the name of the first item
    pszItem = (LPTSTR)DPA_GetPtr(m_hItemList, 0);
    if (NULL != pszItem && !PathIsRoot(pszItem))
    {
        TCHAR szParent[MAX_PATH];

        lstrcpyn(szParent, pszItem, ARRAYSIZE(szParent));
        PathRemoveFileSpec(szParent);

        // Read the parent DACL/SACL
        hr = ReadObjectSecurity(szParent, si, ppSD);
    }
    TraceLeaveResult(hr);
}

STDMETHODIMP
CNTFSSecurity::GetDefaultSecurity(SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD)
{
    HRESULT hr = S_OK;
    DWORD dwErr;
    PSECURITY_DESCRIPTOR psdParent = NULL;
    SECURITY_INFORMATION siLocal;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    ULONG ulAutoInheritFlags = (SEF_DACL_AUTO_INHERIT               |
                                SEF_SACL_AUTO_INHERIT               |
                                SEF_AVOID_PRIVILEGE_CHECK           |
                                SEF_AVOID_OWNER_CHECK);

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::GetDefaultSecurity");
    TraceAssert(m_hItemList && DPA_GetPtrCount(m_hItemList));

    dwErr = GetCurrentToken(TOKEN_QUERY, &hToken);
    if (NOERROR != dwErr)
        ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "Unable to get current token");

    // Only DACL and SACL get inherited
    siLocal = si & ~(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION);
    if (siLocal)
    {
        hr = GetParentSecurity(si, &psdParent);
        FailGracefully(hr, "Unable to get parent security descriptor");
    }

#if(_WIN32_WINNT >= 0x0500)
    if(!CreatePrivateObjectSecurityEx(psdParent,// Inherited ACEs come from here
                                      NULL,
                                      ppSD,
                                      NULL,
                                      !!(m_dwSIFlags & SI_CONTAINER),
                                      ulAutoInheritFlags,
                                      hToken,   // owner & group come from here
                                      &NTFSMap))
#else
    if(!CreatePrivateObjectSecurity(psdParent,
                                    NULL,
                                    ppSD,
                                    !!(m_dwSIFlags & SI_CONTAINER),
                                    hToken,
                                    &NTFSMap))
#endif
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }

exit_gracefully:

    if (hToken != INVALID_HANDLE_VALUE)
        CloseHandle(hToken);

    if (psdParent)
        LocalFree(psdParent);

    TraceLeaveResult(hr);
}
#endif  // SUPPORT_NTFS_DEFAULT

#ifdef DONT_USE_ACLAPI
STDMETHODIMP
CNTFSSecurity::ReadObjectSecurity(LPCTSTR pszObject,
                                  SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD)
{
    DWORD dwErr;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::ReadObjectSecurity");

    dwErr = NTFSReadSD(pszObject, si, ppSD);

    TraceLeaveResult(HRESULT_FROM_WIN32(dwErr));
}
#endif  // DONT_USE_ACLAPI

STDMETHODIMP
CNTFSSecurity::WriteObjectSecurity(LPCTSTR pszObject,
                                   SECURITY_INFORMATION si,
                                   PSECURITY_DESCRIPTOR pSD)
{
    DWORD dwErr;
    HRESULT hr = S_OK;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::WriteObjectSecurity");
    TraceAssert(pszObject != NULL);
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);

#ifdef DONT_USE_ACLAPI
    //
    // Assume that required privileges have already been
    // enabled, if appropriate.
    //
    if (!SetFileSecurity(pszObject, si, pSD))
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
#else
    hr = CSecurityInformation::WriteObjectSecurity(pszObject, si, pSD);

    // BUGBUG
    // This is a workaround.  SetNamedSecurityInfo[Ex] fails with Access Denied
    // in some cases where the owner is trying to set the DACL
    // (typically because the propagation code can't enumerate the children
    // because owner doesn't have read access).

    if (E_ACCESSDENIED == hr)
    {
        SECURITY_DESCRIPTOR_CONTROL wControl = 0;
        DWORD dwRevision;

        // If we're setting a protected DACL (i.e. no inheritance from parent)
        // try SetFileSecurity.  If that works, try the full write again.
        //
        // Don't do this if the DACL isn't protected since it may fool the
        // system into thinking that this is a downlevel DACL that should
        // be protected.  That would be very confusing to the user.

        GetSecurityDescriptorControl(pSD, &wControl, &dwRevision);

        if ((si & DACL_SECURITY_INFORMATION)
            && ((wControl & SE_DACL_PROTECTED) || (m_dwSIFlags & SI_NO_ACL_PROTECT))
            && SetFileSecurity(pszObject, si, pSD))
        {
            hr = CSecurityInformation::WriteObjectSecurity(pszObject, si, pSD);
        }
    }
#endif  // DONT_USE_ACLAPI

    //
    // Notify the shell if we change permissions on a folder (48220)
    //
    if (SUCCEEDED(hr) &&
        (si & DACL_SECURITY_INFORMATION) &&
        (m_dwSIFlags & SI_CONTAINER))
    {
        SHChangeNotify(SHCNE_UPDATEDIR,
                       SHCNF_PATH | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT,
                       pszObject,
                       NULL);
    }

    TraceLeaveResult(hr);
}

void
CNTFSSecurity::WaitForComparison()
{
    if (m_hCompareThread != NULL)
    {
        DWORD dwResult;
        HCURSOR hcurPrevious = SetCursor(LoadCursor(NULL, IDC_WAIT));

        SetThreadPriority(m_hCompareThread, THREAD_PRIORITY_HIGHEST);

        dwResult = WaitForSingleObject(m_hCompareThread, INFINITE);

        if (m_pCompareData != NULL)
        {
            if (GetExitCodeThread(m_hCompareThread, &dwResult))
            {
                m_pCompareData->hrResult = dwResult;
            }
            else
            {
                dwResult = GetLastError();
                m_pCompareData->hrResult = HRESULT_FROM_WIN32(dwResult);
            }
        }

        CloseHandle(m_hCompareThread);
        m_hCompareThread = NULL;
        SetCursor(hcurPrevious);
    }
}

DWORD WINAPI
CNTFSSecurity::NTFSReadSD(LPCTSTR pszObject,
                          SECURITY_INFORMATION si,
                          PSECURITY_DESCRIPTOR* ppSD)
{
    DWORD dwLength = 0;
    DWORD dwErr = 0;

    TraceEnter(TRACE_NTFSSI | TRACE_NTFSCOMPARE, "CNTFSSecurity::NTFSReadSD");
    TraceAssert(pszObject != NULL);
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    //
    // Assume that required privileges have already been
    // enabled, if appropriate.
    //
    GetFileSecurity(pszObject, si, NULL, 0, &dwLength);
    if (dwLength)
    {
        *ppSD = LocalAlloc(LPTR, dwLength);
        if (*ppSD &&
            !GetFileSecurity(pszObject, si, *ppSD, dwLength, &dwLength))
        {
            dwErr = GetLastError();
            LocalFree(*ppSD);
            *ppSD = NULL;
        }
    }
    else
        dwErr = GetLastError();

    TraceLeaveValue(dwErr);
}

DWORD WINAPI
CNTFSSecurity::NTFSCompareThreadProc(LPVOID pvData)
{
    PNTFS_COMPARE_DATA pCompareData = (PNTFS_COMPARE_DATA)pvData;
    HRESULT hr;
    DWORD dwSIFlags;
    BOOL bOwnerConflict = FALSE;
    BOOL bSaclConflict = FALSE;
    BOOL bDaclConflict = FALSE;

    TraceEnter(TRACE_NTFSCOMPARE, "CNTFSSecurity::NTFSCompareThreadProc");
    TraceAssert(pCompareData != NULL);

    dwSIFlags = pCompareData->dwSIFlags;

    hr = DPA_CompareSecurityIntersection(pCompareData->hItemList,
                                         NTFSReadSD,
                                         (dwSIFlags & SI_EDIT_OWNER) ? &bOwnerConflict : NULL,
                                         NULL,
                                         (dwSIFlags & SI_EDIT_AUDITS) ? &bSaclConflict : NULL,
                                         &bDaclConflict,
                                         NULL,
                                         NULL,
                                         &pCompareData->pszSaclConflict,
                                         &pCompareData->pszDaclConflict,
                                         &pCompareData->bAbortThread);
    if (SUCCEEDED(hr))
    {
        if (bOwnerConflict)
            pCompareData->siConflict |= OWNER_SECURITY_INFORMATION;

        if (bSaclConflict)
            pCompareData->siConflict |= SACL_SECURITY_INFORMATION;

        if (bDaclConflict)
            pCompareData->siConflict |= DACL_SECURITY_INFORMATION;

        if (pCompareData->pszSaclConflict)
            pCompareData->idsSaclPrompt = IDS_BAD_SACL_INTERSECTION;

        if (pCompareData->pszDaclConflict)
            pCompareData->idsDaclPrompt = IDS_BAD_DACL_INTERSECTION;
    }

    TraceLeaveResult(hr);
}


HRESULT
CNTFSSecurity::SetSecurityLocal(SECURITY_INFORMATION si,
                                PSECURITY_DESCRIPTOR pSD,
                                LPBOOL pbNotAllApplied)
{
    HRESULT hr = S_OK;
    HCURSOR hcur;
    LPTSTR pszBuffer = NULL;
    UINT cchBuffer = 0;
    int i;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::SetSecurityLocal");
    TraceAssert(pSD != NULL);
    TraceAssert(SI_CONTAINER & m_dwSIFlags);

    if (pbNotAllApplied)
        *pbNotAllApplied = FALSE;

    if (NULL == m_hItemList)
        ExitGracefully(hr, E_UNEXPECTED, "CSecurityInformation not initialized");

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    CreateProgressDialog(si);

    //
    // Allocate a buffer to hold the path(s)
    //
    pszBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, 2 * MAX_PATH * sizeof(TCHAR));
    if (pszBuffer == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    cchBuffer = (UINT)LocalSize(pszBuffer) / sizeof(TCHAR);

    for (i = 0; i < DPA_GetPtrCount(m_hItemList); i++)
    {
        LPTSTR pszItem = (LPTSTR)DPA_FastGetPtr(m_hItemList, i);
        if (NULL != pszItem)
        {
            lstrcpy(pszBuffer, pszItem);
            hr = SetSecurityDeep(&pszBuffer,
                                 &cchBuffer,
                                 FILE_ATTRIBUTE_DIRECTORY,
                                 si,
                                 pSD,
                                 pbNotAllApplied);
            FailGracefully(hr, "Unable to recursively apply security");
        }
        else if (pbNotAllApplied)
        {
            *pbNotAllApplied = TRUE;
            hr = S_OK;
        }
        else
        {
            hr = E_UNEXPECTED;
            break;
        }
    }

exit_gracefully:

    CloseProgressDialog();

    if (pszBuffer != NULL)
        LocalFree(pszBuffer);

    if (m_psdOwnerFullControl)
    {
        LocalFree(m_psdOwnerFullControl);
        m_psdOwnerFullControl = NULL;
    }

    SetCursor(hcur);

    TraceLeaveResult(hr);
}

BOOL PathIsDotOrDotDot(LPCTSTR pszPath)
{
    if (TEXT('.') == *pszPath++)
    {
        if (TEXT('\0') == *pszPath || (TEXT('.') == *pszPath && TEXT('\0') == *(pszPath + 1)))
            return TRUE;
    }
    return FALSE;
}


typedef struct _APPLY_SECURITY_ERROR
{
    DWORD   dwError;
    LPCTSTR pszPath;
    UINT    idMsg[1];   // Optional, string resource IDs (only 1 used so far)
} APPLY_SECURITY_ERROR;

INT_PTR CALLBACK
FailedApplySecurityProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            APPLY_SECURITY_ERROR* pae = (APPLY_SECURITY_ERROR*)lParam;
            LPTSTR pszT = NULL;

            // Set the message string(s)
            for (int i = 0; i < ARRAYSIZE(pae->idMsg); i++)
            {
                if (pae->idMsg[i])
                {
                    LoadStringAlloc(&pszT, g_hInstance, pae->idMsg[i]);
                    if (pszT)
                        SetDlgItemText(hDlg, (IDC_MSG1 + i), pszT);
                    LocalFreeString(&pszT);
                }
            }

            // Compact the path so it fits on the dialog
            PathSetDlgItemPath(hDlg, IDC_FILENAME, pae->pszPath);

            // Set the error text
            if (NOERROR != pae->dwError)
            {
                if (!GetSystemErrorText(&pszT, pae->dwError))
                    FormatStringID(&pszT, g_hInstance, IDS_FMT_UNKNOWN_ERROR, pae->dwError);
                if (pszT)
                    SetDlgItemText(hDlg, IDC_ERROR_TXT, pszT);
                LocalFreeString(&pszT);
            }
            break;
        }

        case WM_COMMAND:
        {
            WORD wControlID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (wControlID)
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, wControlID);
                    return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

//
// This function displays the "An error has occured [Continue] [Cancel]" message
//
// Returns IDOK or IDCANCEL
//
int
FailedApplySecurityErrorDlg(HWND hWndParent, APPLY_SECURITY_ERROR* pae)
{
    return (int)DialogBoxParam(g_hInstance,
                               MAKEINTRESOURCE(IDD_SET_SECURITY_ERROR),
                               hWndParent,
                               FailedApplySecurityProc,
                               (LPARAM)pae);
}

HRESULT
CNTFSSecurity::SetSecurityDeep(LPTSTR *ppszBuffer,
                               LPUINT pcchBuffer,
                               DWORD dwAttributes,
                               SECURITY_INFORMATION si,
                               PSECURITY_DESCRIPTOR pSD,
                               LPBOOL pbNotAllApplied)
{
    HRESULT hr = S_OK;
    DWORD dwErr = NOERROR;
    LPTSTR pszBuffer;
    BOOL bWriteDone = FALSE;

    TraceEnter(TRACE_NTFSSI, "CNTFSSecurity::SetSecurityDeep");
    TraceAssert(ppszBuffer != NULL);
    TraceAssert(pcchBuffer != NULL);
    TraceAssert(pSD != NULL);

    pszBuffer = *ppszBuffer;
    TraceAssert(pszBuffer != NULL);
    Trace((pszBuffer));

    if (S_FALSE == SetProgress(pszBuffer))
        TraceLeaveResult(S_FALSE);

    //
    // Recursively apply the new SD to subfolders
    //
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        HANDLE hFind;
        WIN32_FIND_DATA fd;
        UINT cchFolder;
        UINT cchSizeRequired;

        cchFolder = lstrlen(pszBuffer);

        //
        // If the buffer is getting tight, realloc
        //
        TraceAssert(cchFolder < *pcchBuffer);
        cchSizeRequired = cchFolder + 1 + ARRAYSIZE(fd.cFileName); // 1 for backslash
        if (cchSizeRequired > *pcchBuffer)
        {
            cchSizeRequired += MAX_PATH;  // so we don't realloc as often
            pszBuffer = (LPTSTR)LocalReAlloc(*ppszBuffer, cchSizeRequired * sizeof(TCHAR), LMEM_MOVEABLE);
            if (pszBuffer)
            {
                *ppszBuffer = pszBuffer;
                *pcchBuffer = cchSizeRequired;
            }
            else
            {
                // fd.cFileName typically has some empty space, so we
                // may be able to continue
                pszBuffer = *ppszBuffer;
                if (*pcchBuffer < cchFolder + 3) // backslash, '*', and NULL
                    TraceLeaveResult(E_OUTOFMEMORY);
            }
        }

        // Append a backslash if it's not there already
        if (pszBuffer[cchFolder-1] != TEXT('\\'))
        {
            pszBuffer[cchFolder] = TEXT('\\');
            cchFolder++;
        }

        // Append the '*' wildcard
        pszBuffer[cchFolder] = TEXT('*');
        pszBuffer[cchFolder+1] = TEXT('\0');

        //
        // Enumerate the folder contents
        //
        hFind = FindFirstFile(pszBuffer, &fd);

        if (INVALID_HANDLE_VALUE == hFind)
        {
            dwErr = GetLastError();

            if (ERROR_ACCESS_DENIED == dwErr)
            {
                // Remove the '*' wildcard
                pszBuffer[cchFolder-1] = TEXT('\0');

                if (si & DACL_SECURITY_INFORMATION)
                {
                    //
                    // The user may be granting themselves access, so call
                    // WriteObjectSecurity and retry FindFirstFile.
                    // (Raid #280016 et al)
                    //
                    // Don't blindly call WriteObjectSecurity before FindFirstFile
                    // since it's possible the user has access now but is removing
                    // their own access.
                    //
                    bWriteDone = TRUE;
                    hr = WriteObjectSecurity(pszBuffer, si, pSD);
                    if (SUCCEEDED(hr))
                    {
                        // Retry FindFirstFile
                        pszBuffer[cchFolder-1] = TEXT('\\');
                        hFind = FindFirstFile(pszBuffer, &fd);
                    }
                }
                else if (si & OWNER_SECURITY_INFORMATION)
                {
                    //
                    // If we can successfully set the owner, then it's possible
                    // to grant the owner more access and retry FindFirstFile.
                    // (Raid #280016 et al)
                    //
                    bWriteDone = TRUE;
                    hr = WriteObjectSecurity(pszBuffer, si, pSD);
                    if (SUCCEEDED(hr))
                    {
                        // Ask the user if they want to grant themselves access
                        if (!m_psdOwnerFullControl)
                        {
                            if (IDYES == MsgPopup(m_hwndPopupOwner,
                                                  MAKEINTRESOURCE(IDS_FMT_WRITE_OWNER_ERR),
                                                  MAKEINTRESOURCE(IDS_PROPPAGE_TITLE),
                                                  MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND,
                                                  g_hInstance,
                                                  pszBuffer))
                            {
                                BuildOwnerFullControlSD(pSD);
                            }
                            else
                            {
                                // Continue without enumerating this folder
                                TraceLeaveResult(S_OK);
                            }
                        }
                        if (m_psdOwnerFullControl)
                        {
                            // Prevent 2nd "unable to read directory" popup below
                            dwErr = NOERROR;

                            // Give the owner Full Control
                            // BUGBUG use SetFileSecurity instead?
                            //SetFileSecurity(pszBuffer, DACL_SECURITY_INFORMATION, m_psdOwnerFullControl);
                            hr = WriteObjectSecurity(pszBuffer,
                                                     DACL_SECURITY_INFORMATION,
                                                     m_psdOwnerFullControl);
                            if (SUCCEEDED(hr))
                            {
                                // Retry FindFirstFile
                                pszBuffer[cchFolder-1] = TEXT('\\');
                                hFind = FindFirstFile(pszBuffer, &fd);
                            }
                        }
                    }
                }
            }
        }

        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (PathIsDotOrDotDot(fd.cFileName))
                    continue;

                //
                // Build full path name and recurse
                //
                lstrcpyn(pszBuffer + cchFolder,
                         fd.cFileName,
                         *pcchBuffer - cchFolder);
                hr = SetSecurityDeep(ppszBuffer,
                                     pcchBuffer,
                                     fd.dwFileAttributes,
                                     si,
                                     pSD,
                                     pbNotAllApplied);

                // In case the buffer was reallocated
                pszBuffer = *ppszBuffer;
            }
            while (S_OK == hr && FindNextFile(hFind, &fd));

            FindClose(hFind);
        }
        else if (NOERROR != dwErr)
        {
            APPLY_SECURITY_ERROR ae = { dwErr, pszBuffer, { IDS_READ_DIR_ERR } };
            if (IDOK != FailedApplySecurityErrorDlg(m_hwndPopupOwner, &ae))
            {
                hr = S_FALSE;   // abort
            }
            else
            {
                hr = S_OK;      // continue
            }
        }

        // Truncate the path back to the original length (sans backslash)
        pszBuffer[cchFolder-1] = TEXT('\0');

        if (S_OK != hr || S_FALSE == SetProgress(pszBuffer))
            TraceLeaveResult(S_FALSE);
    }

    //
    // Finally, write out the new security descriptor
    //
    if (!bWriteDone)
        hr = WriteObjectSecurity(pszBuffer, si, pSD);

    if (SUCCEEDED(hr))
    {
        //
        // Notify the shell if we change permissions on a folder (48220)
        //
        if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (si & DACL_SECURITY_INFORMATION))
        {
            SHChangeNotify(SHCNE_UPDATEDIR,
                           SHCNF_PATH | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT,
                           pszBuffer,
                           NULL);
        }
    }
    else
    {
        APPLY_SECURITY_ERROR ae = { hr, pszBuffer, { 0 } };
        if (IDOK != FailedApplySecurityErrorDlg(m_hwndPopupOwner, &ae))
        {
            hr = S_FALSE;   // abort
        }
        else
        {
            hr = S_OK;      // continue
        }
        if (pbNotAllApplied)
        {
            *pbNotAllApplied = TRUE;
        }
    }

    TraceLeaveResult(hr);
}


#ifndef IDA_APPLYATTRIBS
// BUGBUG this is the resource ID of an AVI in shell32.dll. If shell32's
// resource ID's change, we'll get the wrong animation (or none).
// We could steal the AVI and build it into rshx32's resources, except
// it almost doubles the size of rshx32.dll (~35k to ~57k).
#define IDA_APPLYATTRIBS        165     // animation for applying file attributes
#endif

void
CNTFSSecurity::CreateProgressDialog(SECURITY_INFORMATION si)
{
    // Shouldn't be necessary, but just in case
    CloseProgressDialog();

    // m_hwndOwner is the toplevel parent of the Security page
    m_hwndPopupOwner = GetLastActivePopup(m_hwndOwner);

    CoCreateInstance(CLSID_ProgressDialog,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IProgressDialog,
                     (void**)&m_pProgressDlg);

    if (m_pProgressDlg)
    {
        WCHAR szT[256];
        UINT ids = IDS_RESET_SEC_TREE;
        IOleWindow *pWindow;

        LoadStringW(g_hInstance, IDS_PROPPAGE_TITLE, szT, ARRAYSIZE(szT));
        m_pProgressDlg->SetTitle(szT);

        switch (si)
        {
        case OWNER_SECURITY_INFORMATION:
            ids = IDS_RESET_OWNER_TREE;
            break;

        case SACL_SECURITY_INFORMATION:
            ids = IDS_RESET_SACL_TREE;
            break;

        case DACL_SECURITY_INFORMATION:
            ids = IDS_RESET_DACL_TREE;
            break;
        }
        LoadStringW(g_hInstance, ids, szT, ARRAYSIZE(szT));
        m_pProgressDlg->SetLine(1, szT, FALSE, NULL);

        m_pProgressDlg->SetAnimation(GetModuleHandle(TEXT("shell32.dll")), IDA_APPLYATTRIBS);
        m_pProgressDlg->StartProgressDialog(m_hwndPopupOwner,
                                            NULL,
                                            PROGDLG_MODAL | PROGDLG_NOTIME
                                             | PROGDLG_NOMINIMIZE | PROGDLG_NOPROGRESSBAR,
                                            NULL);

        if (SUCCEEDED(m_pProgressDlg->QueryInterface(IID_IOleWindow, (void**)&pWindow)))
        {
            pWindow->GetWindow(&m_hwndPopupOwner);
            pWindow->Release();
        }
    }
}

void
CNTFSSecurity::CloseProgressDialog(void)
{
    m_hwndPopupOwner = NULL;

    if (m_pProgressDlg)
    {
        m_pProgressDlg->StopProgressDialog();
        m_pProgressDlg->Release();
        m_pProgressDlg = NULL;
    }
}

HRESULT
CNTFSSecurity::SetProgress(LPTSTR pszFile)
{
    USES_CONVERSION;

    if (m_pProgressDlg)
    {
        m_pProgressDlg->SetLine(2, T2W(pszFile), TRUE, NULL);
        if (m_pProgressDlg->HasUserCancelled())
            return S_FALSE;
    }
    return S_OK;
}

HRESULT
CNTFSSecurity::BuildOwnerFullControlSD(PSECURITY_DESCRIPTOR pSD)
{
    PSID psidOwner;
    BOOL bDefaulted;
    DWORD dwAclLen;
    PACL pAcl;
    PACE_HEADER pAce;

    if (!GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted))
        return E_INVALIDARG;

    dwAclLen = sizeof(ACL)
        + sizeof(KNOWN_ACE) - sizeof(DWORD)
        + GetLengthSid(psidOwner);

    m_psdOwnerFullControl = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + dwAclLen);
    if (NULL == m_psdOwnerFullControl)
        return E_OUTOFMEMORY;

    InitializeSecurityDescriptor(m_psdOwnerFullControl, SECURITY_DESCRIPTOR_REVISION);
    pAcl = (PACL)ByteOffset(m_psdOwnerFullControl, SECURITY_DESCRIPTOR_MIN_LENGTH);
    InitializeAcl(pAcl, dwAclLen, ACL_REVISION);
    AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidOwner);
    pAce = (PACE_HEADER)FirstAce(pAcl);
    pAce->AceFlags = INHERIT_FULL;
    SetSecurityDescriptorDacl(m_psdOwnerFullControl, TRUE, pAcl, TRUE);

    return S_OK;
}
