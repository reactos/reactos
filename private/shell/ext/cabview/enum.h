//*******************************************************************************************
//
// Filename : Enum.h
//	
//				Definition of CEnumCabObjs
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#ifndef _ENUM_H_
#define _ENUM_H_


#include "folder.h"

// Enumeration object for the CabFolder
class CEnumCabObjs : public IEnumIDList
{
public:
	CEnumCabObjs(CCabFolder *pcf, DWORD uFlags) : m_iCount(0)
	{
		m_uFlags = uFlags;
		m_pcfThis=pcf;
		pcf->AddRef();
	}
	~CEnumCabObjs()
	{
		m_pcfThis->Release();
	}

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt,
		      LPITEMIDLIST *rgelt,
		      ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum);

private:
	CRefDll m_cRefDll;

	CRefCount m_cRef;

	CCabFolder *m_pcfThis;

	UINT m_iCount;
	DWORD m_uFlags;
} ;

#endif // _ENUM_H_
