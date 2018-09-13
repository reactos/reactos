//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       si.cpp
//
//  This file contains the implementation of the CSecurityInformation
//  base class.
//
//--------------------------------------------------------------------------

#include "rshx32.h"

#if(_WIN32_WINNT < 0x0500)
//
// NT4 SP4 doesn't support SetSecurityDescriptorControl, so
// emulate it here
//
BOOL
WINAPI
SetSecurityDescriptorControl(PSECURITY_DESCRIPTOR psd,
                             SECURITY_DESCRIPTOR_CONTROL wControlMask,
                             SECURITY_DESCRIPTOR_CONTROL wControlBits)
{
    DWORD dwErr = NOERROR;
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)psd;

    if (pSD)
        pSD->Control = (pSD->Control & ~wControlMask) | wControlBits;
    else
        dwErr = ERROR_INVALID_PARAMETER;

    return dwErr;
}
#endif

void
ProtectACLs(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    SECURITY_DESCRIPTOR_CONTROL wSDControl;
    DWORD dwRevision;
    PACL pAcl;
    BOOL bDefaulted;
    BOOL bPresent;
    PACE_HEADER pAce;
    UINT cAces;

    TraceEnter(TRACE_SI, "ProtectACLs");

    if (0 == si || NULL == pSD)
        TraceLeaveVoid();   // Nothing to do

    // Get the ACL protection control bits
    GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision);
    wSDControl &= SE_DACL_PROTECTED | SE_SACL_PROTECTED;

    if ((si & DACL_SECURITY_INFORMATION) && !(wSDControl & SE_DACL_PROTECTED))
    {
        wSDControl |= SE_DACL_PROTECTED;
        pAcl = NULL;
        GetSecurityDescriptorDacl(pSD, &bPresent, &pAcl, &bDefaulted);

        // Theoretically, modifying the DACL in this way can cause it to be
        // no longer canonical.  However, the only way this can happen is if
        // there is an inherited Deny ACE and a non-inherited Allow ACE.
        // Since this function is only called for root objects, this means
        // a) the server DACL must have a Deny ACE and b) the DACL on this
        // object must have been modified later.  But if the DACL was
        // modified through the UI, then we would have eliminated all of the
        // Inherited ACEs already.  Therefore, it must have been modified
        // through some other means.  Considering that the DACL originally
        // inherited from the server never has a Deny ACE, this situation
        // should be extrememly rare.  If it ever does happen, the ACL
        // Editor will just tell the user that the DACL is non-canonical.
        //
        // Therefore, let's ignore the possibility here.

        if (NULL != pAcl)
        {
            for (cAces = pAcl->AceCount, pAce = (PACE_HEADER)FirstAce(pAcl);
                 cAces > 0;
                 --cAces, pAce = (PACE_HEADER)NextAce(pAce))
            {
                pAce->AceFlags &= ~INHERITED_ACE;
            }
        }
    }

    if ((si & SACL_SECURITY_INFORMATION) && !(wSDControl & SE_SACL_PROTECTED))
    {
        wSDControl |= SE_SACL_PROTECTED;
        pAcl = NULL;
        GetSecurityDescriptorSacl(pSD, &bPresent, &pAcl, &bDefaulted);

        if (NULL != pAcl)
        {
            for (cAces = pAcl->AceCount, pAce = (PACE_HEADER)FirstAce(pAcl);
                 cAces > 0;
                 --cAces, pAce = (PACE_HEADER)NextAce(pAce))
            {
                pAce->AceFlags &= ~INHERITED_ACE;
            }
        }
    }

    SetSecurityDescriptorControl(pSD, SE_DACL_PROTECTED | SE_SACL_PROTECTED, wSDControl);

    TraceLeaveVoid();
}


CSecurityInformation::CSecurityInformation(SE_OBJECT_TYPE seType)
: m_cRef(1), m_seType(seType), m_hwndOwner(NULL)
{
    InterlockedIncrement(&g_cRefThisDll);
}

CSecurityInformation::~CSecurityInformation()
{
    LocalFreeDPA(m_hItemList);
    LocalFreeString(&m_pszObjectName);
    LocalFreeString(&m_pszServerName);

    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP
CSecurityInformation::Initialize(HDPA   hItemList,
                                 DWORD  dwFlags,
                                 LPTSTR pszServer,
                                 LPTSTR pszObject)
{
    TraceEnter(TRACE_SI, "CSecurityInformation::Initialize");
    TraceAssert(hItemList != NULL);
    TraceAssert(DPA_GetPtrCount(hItemList) > 0);
    TraceAssert(pszObject != NULL);
    TraceAssert(m_pszObjectName == NULL);   // only initialize once

    m_hItemList = hItemList;
    m_dwSIFlags = dwFlags;
    m_pszServerName = pszServer;
    m_pszObjectName = pszObject;

    TraceLeaveResult(S_OK);
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CSecurityInformation::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CSecurityInformation::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CSecurityInformation::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
    {
        *ppv = (LPSECURITYINFO)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CSecurityInformation::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    TraceEnter(TRACE_SI, "CSecurityInformation::GetObjectInformation");
    TraceAssert(pObjectInfo != NULL &&
                !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

    pObjectInfo->dwFlags = m_dwSIFlags;
    pObjectInfo->hInstance = g_hInstance;
    pObjectInfo->pszServerName = m_pszServerName;
    pObjectInfo->pszObjectName = m_pszObjectName;

    TraceLeaveResult(S_OK);
}

STDMETHODIMP
CSecurityInformation::GetSecurity(SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD,
                                  BOOL fDefault)
{
    HRESULT hr = S_OK;
    LPTSTR pszItem;

    TraceEnter(TRACE_SI, "CSecurityInformation::GetSecurity");
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    *ppSD = NULL;

    // BUGBUG: how to implement this?
    if (fDefault)
        ExitGracefully(hr, E_NOTIMPL, "Default security descriptor not supported");

    // Get the name of the first item
    pszItem = (LPTSTR)DPA_GetPtr(m_hItemList, 0);
    if (NULL == pszItem)
        ExitGracefully(hr, E_UNEXPECTED, "CSecurityInformation not initialized");

    hr = ReadObjectSecurity(pszItem, si, ppSD);

    // If this is a Root object, then we pretend that the ACLs are
    // always protected and no ACEs are inherited.
    if (SUCCEEDED(hr) && (m_dwSIFlags & SI_NO_ACL_PROTECT))
        ProtectACLs(si & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION), *ppSD);

exit_gracefully:

    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInformation::SetSecurity(SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr;
    HCURSOR hcurPrevious = (HCURSOR)INVALID_HANDLE_VALUE;
    UINT cItems;
    int i;

    TraceEnter(TRACE_SI, "CSecurityInformation::SetSecurity");
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);

    if (NULL == m_hItemList)
        ExitGracefully(hr, E_UNEXPECTED, "CSecurityInformation not initialized");

    hcurPrevious = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Apply the new permissions to every item in the list
    //
    for (i = 0; i < DPA_GetPtrCount(m_hItemList); i++)
    {
        LPTSTR pszItem = (LPTSTR)DPA_FastGetPtr(m_hItemList, i);
        hr = WriteObjectSecurity(pszItem, si, pSD);
        FailGracefully(hr, "Unable to write new security descriptor");
    }

exit_gracefully:

    // Restore previous cursor
    if (hcurPrevious != INVALID_HANDLE_VALUE)
        SetCursor(hcurPrevious);

    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInformation::PropertySheetPageCallback(HWND hwnd,
                                                UINT uMsg,
                                                SI_PAGE_TYPE uPage)
{
    if (SI_PAGE_PERM == uPage)
    {
        switch (uMsg)
        {
        case PSPCB_SI_INITDIALOG:
            do
            {
                m_hwndOwner = hwnd;
            } while (hwnd = GetParent(hwnd));

            break;

        case PSPCB_RELEASE:
            m_hwndOwner = NULL;
            break;
        }
    }
    return S_OK;
}


STDMETHODIMP
CSecurityInformation::ReadObjectSecurity(LPCTSTR pszObject,
                                         SECURITY_INFORMATION si,
                                         PSECURITY_DESCRIPTOR *ppSD)
{
    DWORD dwErr;

    TraceEnter(TRACE_SI, "CSecurityInformation::ReadObjectSecurity");
    TraceAssert(pszObject != NULL);
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    //
    // This is kinda screwy.  The new APIs are being removed from NT5, but have
    // already been added to NT4 SP4.  The old APIs have new functionality on NT5,
    // but not on NT4 SPx.  Since we need the new functionality (auto-inheritance),
    // we have to call the new (defunct) API on NT4 and the old API on NT5.
    //
#if(_WIN32_WINNT >= 0x0500)
    dwErr = GetNamedSecurityInfo((LPTSTR)pszObject,
                                 m_seType,
                                 si,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 ppSD);
#else   // _WIN32_WINNT < 0x0500
    PACTRL_ACCESS pAccessList = NULL;
    PACTRL_AUDIT pAuditList = NULL;
    LPTSTR pOwner = NULL;
    LPTSTR pGroup = NULL;

    dwErr = GetNamedSecurityInfoEx(pszObject,
                                   m_seType,
                                   si,
                                   NULL,
                                   NULL,
                                   (si & DACL_SECURITY_INFORMATION) ? &pAccessList : NULL,
                                   (si & SACL_SECURITY_INFORMATION) ? &pAuditList : NULL,
                                   (si & OWNER_SECURITY_INFORMATION) ? &pOwner : NULL,
                                   (si & GROUP_SECURITY_INFORMATION) ? &pGroup : NULL);
    if (!dwErr)
    {
        dwErr = ConvertAccessToSecurityDescriptor(pAccessList,
                                                  pAuditList,
                                                  pOwner,
                                                  pGroup,
                                                  ppSD);
        if (pAccessList)
            LocalFree(pAccessList);
        if (pAuditList)
            LocalFree(pAuditList);
        if (pOwner)
            LocalFree(pOwner);
        if (pGroup)
            LocalFree(pGroup);
    }
#endif  // _WIN32_WINNT < 0x0500

    TraceLeaveResult(HRESULT_FROM_WIN32(dwErr));
}


STDMETHODIMP
CSecurityInformation::WriteObjectSecurity(LPCTSTR pszObject,
                                          SECURITY_INFORMATION si,
                                          PSECURITY_DESCRIPTOR pSD)
{
    DWORD dwErr;

    TraceEnter(TRACE_SI, "CSecurityInformation::WriteObjectSecurity");
    TraceAssert(pszObject != NULL);
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);

    //
    // This is kinda screwy.  The new APIs are being removed from NT5, but have
    // already been added to NT4 SP4.  The old APIs have new functionality on NT5,
    // but not on NT4 SPx.  Since we need the new functionality (auto-inheritance),
    // we have to call the new (defunct) API on NT4 and the old API on NT5.
    //

#if(_WIN32_WINNT >= 0x0500)
    SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
    DWORD dwRevision;
    PSID psidOwner = NULL;
    PSID psidGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOL bDefaulted;
    BOOL bPresent;

    //
    // Get pointers to various security descriptor parts for
    // calling SetNamedSecurityInfo
    //
    GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision);
    GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted);
    GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted);
    GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted);
    GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted);

    if (si & DACL_SECURITY_INFORMATION)
    {
        if (wSDControl & SE_DACL_PROTECTED)
            si |= PROTECTED_DACL_SECURITY_INFORMATION;
        else
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
    }
    if (si & SACL_SECURITY_INFORMATION)
    {
        if (wSDControl & SE_SACL_PROTECTED)
            si |= PROTECTED_SACL_SECURITY_INFORMATION;
        else
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;
    }

    dwErr = SetNamedSecurityInfo((LPTSTR)pszObject,
                                 m_seType,
                                 si,
                                 psidOwner,
                                 psidGroup,
                                 pDacl,
                                 pSacl);
#else   // _WIN32_WINNT < 0x0500
    PACTRL_ACCESS pAccessList = NULL;
    PACTRL_AUDIT pAuditList = NULL;
    LPTSTR pOwner = NULL;
    LPTSTR pGroup = NULL;

    dwErr = ConvertSecurityDescriptorToAccessNamed(pszObject,
                                                   m_seType,
                                                   pSD,
                                                   (si & DACL_SECURITY_INFORMATION) ? &pAccessList : NULL,
                                                   (si & SACL_SECURITY_INFORMATION) ? &pAuditList : NULL,
                                                   (si & OWNER_SECURITY_INFORMATION) ? &pOwner : NULL,
                                                   (si & GROUP_SECURITY_INFORMATION) ? &pGroup : NULL);
    if (!dwErr)
    {
        dwErr = SetNamedSecurityInfoEx(pszObject,
                                       m_seType,
                                       si,
                                       NULL,
                                       pAccessList,
                                       pAuditList,
                                       pOwner,
                                       pGroup,
                                       NULL);
        if (pAccessList)
            LocalFree(pAccessList);
        if (pAuditList)
            LocalFree(pAuditList);
        if (pOwner)
            LocalFree(pOwner);
        if (pGroup)
            LocalFree(pGroup);
    }
#endif  // _WIN32_WINNT < 0x0500

    TraceLeaveResult(HRESULT_FROM_WIN32(dwErr));
}
