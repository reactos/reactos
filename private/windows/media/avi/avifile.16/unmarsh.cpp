/****************************************************************************
 *
 *  UNMARSH.CPP
 *
 *  Stupid unmarshalling stub
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <storage.h>
#include <avifmt.h>
#include "avifile.h"
#include "avifilei.h"
#include "unmarsh.h"
#include "debug.h"


HRESULT CUnMarshal::Create(
	IUnknown FAR*	pUnknownOuter,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
	IUnknown FAR*	pUnknown;
	CUnMarshal FAR*	pUnMarshal;
	HRESULT	hresult;

	DPF("Creating Simple UnMarshal Instance....\n");
	pUnMarshal = new FAR CUnMarshal(pUnknownOuter, &pUnknown);
	if (!pUnMarshal)
		return ResultFromScode(E_OUTOFMEMORY);
	hresult = pUnknown->QueryInterface(riid, ppv);
	if (FAILED(GetScode(hresult)))
		delete pUnMarshal;
	return hresult;
}

CUnMarshal::CUnMarshal(
	IUnknown FAR*	pUnknownOuter,
	IUnknown FAR* FAR*	ppUnknown)
{
	if (pUnknownOuter)
		m_pUnknownOuter = pUnknownOuter;
	else
		m_pUnknownOuter = this;
	*ppUnknown = this;
}

STDMETHODIMP CUnMarshal::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	if (iid == IID_IUnknown)
	    *ppv = this;
	else if (iid == IID_IMarshal) {
	    *ppv = this;
	} else
		return ResultFromScode(E_NOINTERFACE);
	AddRef();
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CUnMarshal::AddRef()
{
	uUseCount++;
	return ++m_refs;
}



STDMETHODIMP_(ULONG) CUnMarshal::Release()
{
    uUseCount--;
    if (!--m_refs) {

	delete this;
	return 0;
    }
    return m_refs;
}


// *** IMarshal methods ***
STDMETHODIMP CUnMarshal::GetUnmarshalClass (THIS_ REFIID riid, LPVOID pv, 
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPCLSID pCid)
{
    HRESULT hr = NOERROR;

    return hr;
}

STDMETHODIMP CUnMarshal::GetMarshalSizeMax (THIS_ REFIID riid, LPVOID pv, 
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPDWORD pSize)
{
    HRESULT hr = NOERROR;

    return hr;
}

STDMETHODIMP CUnMarshal::MarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
		    LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags)
{
    HRESULT hr = NOERROR;

    return hr;
}

STDMETHODIMP CUnMarshal::UnmarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
		    LPVOID FAR* ppv)
{
    HRESULT hr;
    IUnknown FAR * punk;

    hr = pStm->Read(&punk,sizeof(punk),NULL);
    
    DPF("Unmarshalling %08lx\n", (DWORD) (LPVOID) punk);
    
    if (hr == NOERROR) {
	hr = punk->QueryInterface(riid, ppv);

	if (hr == NOERROR)
	    punk->Release();
    }

    return hr;
}

STDMETHODIMP CUnMarshal::ReleaseMarshalData (THIS_ LPSTREAM pStm)
{
    HRESULT hr = NOERROR;

    return hr;
}

STDMETHODIMP CUnMarshal::DisconnectObject (THIS_ DWORD dwReserved)
{
    HRESULT hr = NOERROR;

    return hr;
}


