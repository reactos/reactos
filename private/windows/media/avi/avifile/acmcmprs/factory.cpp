
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <string.h>
#include <compobj.h>
#include <mmsystem.h>
#include <mmddk.h>
#define	INITGUID
#include <initguid.h>
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
#include <vfw.h>
#include "handler.h"

/*	-	-	-	-	-	-	-	-	*/

UINT	uUseCount;
BOOL	fLocked;
HINSTANCE ghInst;

/*	-	-	-	-	-	-	-	-	*/

EXTERN_C BOOL PASCAL FAR LibMain(
	HINSTANCE	hInstance,
	HGLOBAL	segDS,
	UINT	cbHeapSize,
	LPCSTR	pszCmdLine)
{
	ghInst = hInstance;		// save this for later
	
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

EXTERN_C BOOL FAR PASCAL _export WEP(
	BOOL	fSystemExit)
{
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

#ifdef WIN32

EXTERN_C BOOL WINAPI DLLEntryPoint(HINSTANCE hModule, ULONG Reason, LPVOID pv)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            LibMain(hModule, 0, 0, NULL);
            break;

        case DLL_PROCESS_DETACH:
            WEP(FALSE);
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;
    }

    return TRUE;
}

#endif


/*	-	-	-	-	-	-	-	-	*/

STDAPI DllCanUnloadNow(
	void)
{
	return ResultFromScode((fLocked || uUseCount) ? S_FALSE : S_OK);
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI DllGetClassObject(
	const CLSID FAR&	rclsid,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
	HRESULT	hresult;

	hresult = CAVIFileCF::Create(rclsid, riid, ppv);
	return hresult;
}

/*	-	-	-	-	-	-	-	-	*/

HRESULT CAVIFileCF::Create(
	const CLSID FAR&	rclsid,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
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
	IUnknown FAR* FAR*	ppUnknown)
{
	m_clsid = rclsid;
	m_refs = 0;
	*ppUnknown = this;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	if (iid == IID_IUnknown)
		*ppv = this;
	else if (iid == IID_IClassFactory)
		*ppv = this;
	else
		return ResultFromScode(E_NOINTERFACE);
	AddRef();
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFileCF::AddRef()
{
	return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFileCF::Release()
{
	if (!--m_refs) {
		delete this;
		return 0;
	}
	return m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::CreateInstance(
	IUnknown FAR*	pUnknownOuter,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
	// Actually create a real object using the CACMCmpStream class....
	return CACMCmpStream::MakeInst(pUnknownOuter, riid, ppv);
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFileCF::LockServer(
	BOOL	fLock)
{
	fLocked = fLock;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/
