//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       xicon.cxx
//
//  Contents:   Implementation of CSharesEI & CSharesEIA, implementations
//              of IExtractIcon
//
//  History:    14-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "xicon.hxx"
#include "resource.h"
#include "shares.h"

STDMETHODIMP
CSharesEI::GetIconLocation(
    UINT uFlags,
    LPTSTR szIconFile,
    UINT cchMax,
    int* piIndex,
    UINT* pwFlags)
{
    if (uFlags & GIL_OPENICON)
    {
        return S_FALSE;
    }

    lstrcpy(szIconFile, TEXT("shareui.dll"));

    WORD wIcon = 0;
    switch (m_bFlags)
    {
    case SHID_SHARE:
    {
        DWORD dwServiceCount = 0;
        if (m_bService & SHARE_SERVICE_SMB)  ++dwServiceCount;
        if (m_bService & SHARE_SERVICE_SFM)  ++dwServiceCount;
        if (m_bService & SHARE_SERVICE_FPNW) ++dwServiceCount;
        appAssert(dwServiceCount > 0);
        appAssert(dwServiceCount == 1);	// until we implement merging

        if (m_bService & SHARE_SERVICE_SMB)
        {
            wIcon = IDI_SHARE;
        }
        if (m_bService & SHARE_SERVICE_SFM)
        {
            wIcon = IDI_SFMSHARE;
        }
        if (m_bService & SHARE_SERVICE_FPNW)
        {
            wIcon = IDI_FPNWSHARE;
        }
        
        break;
    }

    default: appAssert(!"Unknown share type");
    }

    *piIndex = -(int)wIcon;
    *pwFlags = GIL_PERINSTANCE;

    return S_OK;
}

STDMETHODIMP
CSharesEI::Extract(
    LPCTSTR pszFile,
    UINT   nIconIndex,
    HICON* phiconLarge,
    HICON* phiconSmall,
    UINT   nIconSize
    )
{
    return S_FALSE;
}


#ifdef UNICODE

STDMETHODIMP
CSharesEIA::GetIconLocation(
    UINT uFlags,
    LPSTR szIconFile,
    UINT cchMax,
    int* piIndex,
    UINT* pwFlags
    )
{
    if (uFlags & GIL_OPENICON)
    {
        return S_FALSE;
    }

    lstrcpyA(szIconFile, "shareui.dll");

    WORD wIcon = 0;
    switch (m_bFlags)
    {
    case SHID_SHARE:
    {
        DWORD dwServiceCount = 0;
        if (m_bService & SHARE_SERVICE_SMB)  ++dwServiceCount;
        if (m_bService & SHARE_SERVICE_SFM)  ++dwServiceCount;
        if (m_bService & SHARE_SERVICE_FPNW) ++dwServiceCount;
        appAssert(dwServiceCount > 0);
        appAssert(dwServiceCount == 1);	// until we implement merging

        if (m_bService & SHARE_SERVICE_SMB)
        {
            wIcon = IDI_SHARE;
        }
        if (m_bService & SHARE_SERVICE_SFM)
        {
            wIcon = IDI_SFMSHARE;
        }
        if (m_bService & SHARE_SERVICE_FPNW)
        {
            wIcon = IDI_FPNWSHARE;
        }
        break;
    }

    default: appAssert(!"Unknown share type");
    }

    *piIndex = -(int)wIcon;
    *pwFlags = GIL_PERINSTANCE;

    return S_OK;
}


STDMETHODIMP
CSharesEIA::Extract(
    LPCSTR pszFile,
    UINT   nIconIndex,
    HICON* phiconLarge,
    HICON* phiconSmall,
    UINT   nIconSize)
{
    return S_FALSE;
}

#endif // UNICODE
