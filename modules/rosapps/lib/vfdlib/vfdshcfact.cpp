/*
	vfdshcfact.cpp

	Virtual Floppy Drive for Windows
	Driver control library
	shell extension COM class factory class

	Copyright (c) 2003-2005 Ken Kato
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#include "vfdtypes.h"
#include "vfdlib.h"
#include "vfdshext.h"

//	class header
#include "vfdshcfact.h"

//
//	constructor
//
CVfdFactory::CVfdFactory()
{
	VFDTRACE(0, ("CVfdFactory::CVfdFactory()\n"));

	m_cRefCnt = 0L;

	g_cDllRefCnt++;
}

//
//	destructor
//
CVfdFactory::~CVfdFactory()
{
	VFDTRACE(0, ("CVfdFactory::~CVfdFactory()\n"));

	g_cDllRefCnt--;
}

//
//	IUnknown methods
//
STDMETHODIMP CVfdFactory::QueryInterface(
	REFIID			riid,
	LPVOID			*ppv)
{
	VFDTRACE(0, ("CVfdFactory::QueryInterface()\n"));

	*ppv = NULL;

	if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IClassFactory)) {
		*ppv = (LPCLASSFACTORY)this;

		AddRef();

		return NOERROR;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CVfdFactory::AddRef()
{
	VFDTRACE(0, ("CVfdFactory::AddRef()\n"));

	return ++m_cRefCnt;
}

STDMETHODIMP_(ULONG) CVfdFactory::Release()
{
	VFDTRACE(0, ("CVfdFactory::Release()\n"));

	if (--m_cRefCnt) {
		return m_cRefCnt;
	}

#ifndef __REACTOS__
	delete this;
#endif

	return 0L;
}

//
// IClassFactory methods
//
STDMETHODIMP CVfdFactory::CreateInstance(
	LPUNKNOWN		pUnkOuter,
	REFIID			riid,
	LPVOID			*ppvObj)
{
	VFDTRACE(0, ("CVfdFactory::CreateInstance()\n"));

	*ppvObj = NULL;

	// Shell extensions typically don't support
	// aggregation (inheritance)

	if (pUnkOuter) {
		return CLASS_E_NOAGGREGATION;
	}

	// Create the main shell extension object.
	// The shell will then call QueryInterface with IID_IShellExtInit
	// -- this is how shell extensions are initialized.

	LPCVFDSHEXT pVfdShExt = new CVfdShExt;

	if (!pVfdShExt) {
		return E_OUTOFMEMORY;
	}

	return pVfdShExt->QueryInterface(riid, ppvObj);
}

STDMETHODIMP CVfdFactory::LockServer(BOOL fLock)
{
	VFDTRACE(0, ("CVfdFactory::LockServer()\n"));
	UNREFERENCED_PARAMETER(fLock);
	return NOERROR;
}
