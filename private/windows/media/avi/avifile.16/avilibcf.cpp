
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#include <win32.h>
//#include <mmddk.h>
#include <compobj.h>
#include <avifmt.h>
#include "avifile.h"
#include "avifilei.h"
#include "avicmprs.h"
#include "unmarsh.h"
#include "editstrm.h"

/*	-	-	-	-	-	-	-	-	*/

HRESULT CAVIFileCF::Create(
	const CLSID FAR&	rclsid,
	REFIID	riid,
	LPVOID FAR*	ppv)
{
	CAVIFileCF FAR*	pAVIFileCF;
	IUnknown FAR*	pUnknown;
	HRESULT hresult;

	pAVIFileCF = new FAR CAVIFileCF(rclsid, &pUnknown);
	if (pAVIFileCF == NULL)
		return ResultFromScode(E_OUTOFMEMORY);
	hresult = pUnknown->QueryInterface(riid, ppv);
	if (FAILED(GetScode(hresult)))
		delete pAVIFileCF;
	return hresult;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFileCF::CAVIFileCF(
	const CLSID FAR&	rclsid,
	IUnknown FAR* FAR*	ppUnknown) :
	m_Unknown(this),
	m_Factory(this)
{
	m_clsid = rclsid;
	*ppUnknown = &m_Unknown;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFileCF::CUnknownImpl::CUnknownImpl(
	CAVIFileCF FAR*	pAVIFileCF)
{
	m_pAVIFileCF = pAVIFileCF;
	m_refs = 0;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::CUnknownImpl::QueryInterface(
	REFIID	iid,
	LPVOID FAR*	ppv)
{
	if (iid == IID_IUnknown)
		*ppv = &m_pAVIFileCF->m_Unknown;
	else if (iid == IID_IClassFactory)
		*ppv = &m_pAVIFileCF->m_Factory;
	else
		return ResultFromScode(E_NOINTERFACE);
	AddRef();
	return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFileCF::CUnknownImpl::AddRef()
{
	return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFileCF::CUnknownImpl::Release()
{
	if (!--m_refs) {
		delete this;
		return 0;
	}
	return m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFileCF::CFactoryImpl::CFactoryImpl(
	CAVIFileCF FAR*	pAVIFileCF)
{
	m_pAVIFileCF = pAVIFileCF;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::CFactoryImpl::QueryInterface(
	REFIID	iid,
	LPVOID FAR*	ppv)
{
	return m_pAVIFileCF->m_Unknown.QueryInterface(iid, ppv);
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFileCF::CFactoryImpl::AddRef()
{
	return m_pAVIFileCF->m_Unknown.AddRef();
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFileCF::CFactoryImpl::Release()
{
	return m_pAVIFileCF->m_Unknown.Release();
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::CFactoryImpl::CreateInstance(
	IUnknown FAR*	pUnknownOuter,
	REFIID	riid,
	LPVOID FAR*	ppv)
{
	if (IsEqualCLSID(m_pAVIFileCF->m_clsid, CLSID_AVIFile)) {
		return CAVIFile::Create(pUnknownOuter, riid, ppv);
	} else 
	if (IsEqualCLSID(m_pAVIFileCF->m_clsid, CLSID_AVICmprsStream)) {
		return CAVICmpStream::Create(pUnknownOuter, riid, ppv);
	} else 
	if (IsEqualCLSID(m_pAVIFileCF->m_clsid, CLSID_AVISimpleUnMarshal)) {
		return CUnMarshal::Create(pUnknownOuter, riid, ppv);
	} else 
	if (IsEqualCLSID(m_pAVIFileCF->m_clsid, CLSID_EditStream)) {
		return CEditStream::NewInstance(pUnknownOuter, riid, ppv);
	} else {
		return ResultFromScode(CO_E_CANTDETERMINECLASS); // !!!
	}
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::CFactoryImpl::LockServer(
	BOOL	fLock)
{
	fLocked = fLock;
	return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/
