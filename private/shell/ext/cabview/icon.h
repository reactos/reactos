//*******************************************************************************************
//
// Filename : Icon.h
//	
//				Definition of CCabItemIcon
//
// Copyright (c) 1994 - 1995 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _ICON_H_
#define _ICON_H_

class CCabItemIcon : public IExtractIcon
{
public:
	CCabItemIcon(LPCTSTR pszName) {lstrcpyn(m_szName, pszName, ARRAYSIZE(m_szName));}
	~CCabItemIcon() {}

	// *** IUnknown methods ***
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

    // *** IExtractIcon methods ***
    STDMETHODIMP GetIconLocation(
		UINT   uFlags,
		LPTSTR szIconFile,
		UINT   cchMax,
		int   * piIndex,
		UINT  * pwFlags);

    STDMETHODIMP Extract(
		LPCTSTR  pszFile,
		UINT	 nIconIndex,
		HICON   *phiconLarge,
		HICON   *phiconSmall,
		UINT     nIconSize);

private:
	CRefDll m_cRefDll;

	CRefCount m_cRef;

	TCHAR m_szName[MAX_PATH];
} ;

#endif // _ICON_H_
