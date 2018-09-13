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
    case SHID_SHARE_1:
    case SHID_SHARE_2:
		if ((m_dwType & ~STYPE_SPECIAL) == STYPE_IPC)
		{
        	wIcon = IDI_IPC;
		}
		else
		{
        	wIcon = IDI_SHARE;
		}
        break;

#ifdef WIZARDS
    case SHID_SHARE_NW:
        wIcon = IDI_NWSHARE;
        break;

    case SHID_SHARE_MAC:
        wIcon = IDI_MACSHARE;
        break;

    case SHID_SHARE_ALL:
        wIcon = IDI_ALLSHARE;
        break;

    case SHID_SHARE_NEW:
        wIcon = IDI_NEWSHARE;
        break;
#endif // WIZARDS

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
    case SHID_SHARE_1:
    case SHID_SHARE_2:
		if ((m_dwType & ~STYPE_SPECIAL) == STYPE_IPC)
		{
        	wIcon = IDI_IPC;
		}
		else
		{
        	wIcon = IDI_SHARE;
		}
        break;

#ifdef WIZARDS
    case SHID_SHARE_NW:
        wIcon = IDI_NWSHARE;
        break;

    case SHID_SHARE_MAC:
        wIcon = IDI_MACSHARE;
        break;

    case SHID_SHARE_ALL:
        wIcon = IDI_ALLSHARE;
        break;

    case SHID_SHARE_NEW:
        wIcon = IDI_NEWSHARE;
        break;
#endif // WIZARDS

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
