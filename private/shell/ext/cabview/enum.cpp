//*******************************************************************************************
//
// Filename : Enum.cpp
//	
//				Implementation for CEnumCabObjs
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "pch.h"		

#include "thisdll.h"
#include "enum.h"

// *** IUnknown methods ***
STDMETHODIMP CEnumCabObjs::QueryInterface(
   REFIID riid, 
   LPVOID FAR* ppvObj)
{
	*ppvObj = NULL;

	LPUNKNOWN pObj;
 
	if (riid == IID_IUnknown)
	{
		pObj = (IUnknown*)((IEnumIDList*)this); 
	}
	else if (riid == IID_IEnumIDList)
	{
		pObj = (IUnknown*)((IEnumIDList*)this); 
	}
	else
	{
   		return(E_NOINTERFACE);
	}

	pObj->AddRef();
	*ppvObj = pObj;

	return(NOERROR);
}


STDMETHODIMP_(ULONG) CEnumCabObjs::AddRef(void)
{
	return(m_cRef.AddRef());
}


STDMETHODIMP_(ULONG) CEnumCabObjs::Release(void)
{
	if (!m_cRef.Release())
	{
   		delete this;
		return(0);
	}

	return(m_cRef.GetRef());
}


// *** IEnumIDList methods ***
STDMETHODIMP CEnumCabObjs::Next(ULONG celt,
	      LPITEMIDLIST *rgelt,
	      ULONG *pceltFetched)
{
	*rgelt = NULL;
	if (pceltFetched)
	{
		*pceltFetched = 0;
	}

	HRESULT hRes = m_pcfThis->InitItems();
	if (FAILED(hRes))
	{
		return(hRes);
	}

	for ( ; ; ++m_iCount)
	{
		if (m_iCount >= m_pcfThis->m_lItems.GetCount())
		{
			return(S_FALSE);
		}

		LPCABITEM pit = m_pcfThis->m_lItems[m_iCount];

		if ((m_uFlags&(SHCONTF_FOLDERS|SHCONTF_NONFOLDERS))
			!= (SHCONTF_FOLDERS|SHCONTF_NONFOLDERS))
		{
			DWORD gfInOut = SFGAO_FOLDER;
			if (FAILED(m_pcfThis->GetAttributesOf(1, (LPCITEMIDLIST *)&pit, &gfInOut)))
			{
				continue;
			}
			if (!(m_uFlags&SHCONTF_FOLDERS) && (gfInOut&SFGAO_FOLDER))
			{
				continue;
			}
			if ((m_uFlags&SHCONTF_FOLDERS) && !(gfInOut&SFGAO_FOLDER))
			{
				continue;
			}
		}

		if (!(m_uFlags&SHCONTF_INCLUDEHIDDEN)
			&& (pit->uFileAttribs&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)))
		{
			continue;
		}

		break;
	}

	*rgelt = ILClone((LPCITEMIDLIST)m_pcfThis->m_lItems[m_iCount]);

	++m_iCount;

	if (*rgelt)
	{
		if (pceltFetched)
		{
			*pceltFetched = 1;
		}

		return(S_OK);
	}

	return(E_OUTOFMEMORY);
}


STDMETHODIMP CEnumCabObjs::Skip(ULONG celt)
{
	return(E_NOTIMPL);
}


STDMETHODIMP CEnumCabObjs::Reset()
{
	m_iCount = 0;

	return(NOERROR);
}


STDMETHODIMP CEnumCabObjs::Clone(IEnumIDList **ppenum)
{
	return(E_NOTIMPL);
}

