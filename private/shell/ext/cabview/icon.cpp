//*******************************************************************************************
//
// Filename : Icon.cpp
//	
//				Implementation file for CCabItemIcon
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "pch.h"

#include "thisdll.h"

#include "icon.h"

// *** IUnknown methods ***
STDMETHODIMP CCabItemIcon::QueryInterface(
   REFIID riid, 
   LPVOID FAR* ppvObj)
{
	*ppvObj = NULL;

	LPUNKNOWN pObj;
 
	if (riid == IID_IUnknown)
	{
		pObj = (LPUNKNOWN)((IExtractIcon*)this); 
	}
	else if (riid == IID_IExtractIcon)
	{
		pObj = (LPUNKNOWN)((IExtractIcon*)this); 
	}
	else
	{
   		return(E_NOINTERFACE);
	}

	pObj->AddRef();
	*ppvObj = (LPVOID)pObj;

	return(NOERROR);
}


STDMETHODIMP_(ULONG) CCabItemIcon::AddRef(void)
{
	return(m_cRef.AddRef());
}


STDMETHODIMP_(ULONG) CCabItemIcon::Release(void)
{
	if (!m_cRef.Release())
	{
   		delete this;
		return(0);
	}

	return(m_cRef.GetRef());
}


const TCHAR c_szCabViewLoc[] = TEXT("CABVIEW::");

// *** IExtractIcon methods ***
STDMETHODIMP CCabItemIcon::GetIconLocation(
	UINT   uFlags,
	LPTSTR szIconFile,
	UINT   cchMax,
	int   * piIndex,
	UINT  * pwFlags)
{
	LPCTSTR pszExt = PathFindExtension(m_szName);
	if (!pszExt || !pszExt[0])
	{
		return(E_UNEXPECTED);
	}

	TCHAR szClass[80];
	// NOTE: lLen is size in bytes - not characters!
	LONG lLen = sizeof(szClass);

	if (RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szClass, &lLen) != ERROR_SUCCESS)
	{
		// This extension has no icon
		return(E_UNEXPECTED);
	}

	if (cchMax < ARRAYSIZE(c_szCabViewLoc) + lstrlen(pszExt))
	{
		return(E_INVALIDARG);
	}

	lstrcpy(szIconFile, c_szCabViewLoc);
	lstrcpy(szIconFile+(ARRAYSIZE(c_szCabViewLoc)-1), pszExt);
	CharUpper(szIconFile);

	*piIndex = uFlags&GIL_OPENICON ? 1 : 0;
	*pwFlags = GIL_PERCLASS | GIL_NOTFILENAME;

	return(NOERROR);
}


STDMETHODIMP CCabItemIcon::Extract(
	LPCTSTR  pszFile,
	UINT	 nIconIndex,
	HICON   *phiconLarge,
	HICON   *phiconSmall,
	UINT     nIconSize)
{
	UINT uFlags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;

	switch (nIconIndex)
	{
	case 0:
		break;

	case 1:
		uFlags |= SHGFI_OPENICON;
		break;

	default:
		return(E_INVALIDARG);
	}

	TCHAR szExt[ARRAYSIZE(c_szCabViewLoc) + 5];

	lstrcpyn(szExt, pszFile, ARRAYSIZE(szExt));
	szExt[ARRAYSIZE(c_szCabViewLoc)-1] = TEXT('\0');
	if (lstrcmp(szExt, c_szCabViewLoc) != 0)
	{
		return(E_INVALIDARG);
	}

	lstrcpyn(szExt, pszFile+(ARRAYSIZE(c_szCabViewLoc)-1), ARRAYSIZE(szExt));
	LPTSTR pszExt = PathFindExtension(m_szName);
	if (lstrcmpi(szExt, pszExt) != 0)
	{
		return(E_INVALIDARG);
	}

	SHFILEINFO sfi;

	if (!SHGetFileInfo(m_szName, 0, &sfi, sizeof(sfi), uFlags | SHGFI_LARGEICON))
	{
		return(E_UNEXPECTED);
	}

	*phiconLarge = sfi.hIcon;

	if (SHGetFileInfo(m_szName, 0, &sfi, sizeof(sfi), uFlags | SHGFI_SMALLICON))
	{
		*phiconSmall = sfi.hIcon;
	}
	else
	{
		*phiconSmall = NULL;
	}

	return(S_OK);
}
