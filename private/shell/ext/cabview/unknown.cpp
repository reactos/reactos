//*******************************************************************************************
//
// Filename : Unknown.cpp
//	
//				Customized CUnknown implmentations
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "Pch.H"

#include "ThisDll.H"

#include "Unknown.H"

CUnknown::~CUnknown()
{
}


HRESULT CUnknown::QIHelper(REFIID riid, LPVOID *ppvObj, const IID *apiid[],
	LPUNKNOWN aobj[])
{
	*ppvObj = NULL;

	LPUNKNOWN pObj;
 
	if (riid == IID_IUnknown)
	{
		pObj = aobj[0]; 
	}
	else
	{
		for (int i=0; ; ++i)
		{
			if (!apiid[i])
			{
		   		return(E_NOINTERFACE);
			}

			if (*apiid[i] == riid)
			{
				pObj = aobj[i];
				break;
			}
		}
	}

	pObj->AddRef();
	*ppvObj = pObj;

	return(NOERROR);
}


ULONG CUnknown::AddRefHelper()
{
	return(m_cRef.AddRef());
}


ULONG CUnknown::ReleaseHelper()
{
	if (!m_cRef.Release())
	{
   		delete this;
		return(0);
	}

	return(m_cRef.GetRef());
}
