/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib\ole32\CoXxx.c
 * PURPOSE:         The Co... function implementation
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 01/05/2001
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
#define INITGUID
#include "Ole32.h"

//
//	lCOMLockCount is a reference count, when it reaches 0, all COM libraries are freed
//
static ULONG lCOMLockCount = 0;

/*WINOLEAPI_(ULONG) CoAddRefServerProcess()
{
	//	not implemented
	return 0L;
}

WINOLEAPI CoAllowSetForegroundWindow(IUnknown *pUnk, LPVOID lpvReserved)
{
	//	not implemented
	return 0L;
}*/

//
//	The winapi states this function has becaome obsolete
//
WINOLEAPI_(DWORD) CoBuildVersion()
{
	return 0;
}

/*WINOLEAPI CoCancelCall(DWORD dwThreadId, ULONG ulTimeout)
{
	//	not implemented
	return E_FAIL;
}

WINOLEAPI CoCopyProxy(IUnknown* pProxy, IUnknown** ppCopy)
{
	//	not implemented
	return E_FAIL;
}

WINOLEAPI CoCreateFreeThreadedMarshaler(LPUNKNOWN punkOuter, LPUNKNOWN* ppunkMarshal)
{
	//	not implemented
	return E_FAIL;
}

//
//	CoCreateGuid creates a uuid
//
WINOLEAPI CoCreateGuid(GUID* pguid)
{
	return UuidCreate(pguid);
}

//
//	CoCreateInstanceEx can call multiple inteerfaces
//
WINOLEAPI CoCreateInstanceEx(REFCLSID Clsid, IUnknown* pUnkOuter, DWORD dwClsCtx, COSERVERINFO* pServerInfo, 
							 DWORD dwCount, MULTI_QI* pResults)
{
	IUnknown*	pUnknown	=	NULL;
	HRESULT		hr			=	S_OK;
	DWORD		nSuccess	=	0;
	
	if((dwCount == 0) || (pResults == NULL))
		return E_INVALIDARG;
	
	//	we do not implement object creation on other machines
	if(pServerInfo != NULL)
		return E_NOTIMPL;
	
	//	init the pResults structure
	for(DWORD i = 0; i < dwCount; i++)
	{
		pResults[i].pItf	= NULL;
		pResults[i].hr		= E_NOINTERFACE;
	}
	
	//	create the object
	hr = CoCreateInstance(Clsid, pUnkOuter, dwClsCtx, IID_IUnknown, (void**) &pUnknown);
	if(FAILED(hr))
		return hr;
	
	//Then, query for all the interfaces requested
	for(i = 0; i < dwCount; i++)
	{
		pResults[i].hr = pUnknown->QueryInterface((REFIID) pResults[i].pIID, (void**) &(pResults[i].pItf));
		if(!FAILED(pResults[i].hr))
			nSuccess++;
	}
	
	//	clean up
	pUnknown->Release();
	if(nSuccess == 0)
		return E_NOINTERFACE;

	if(nSuccess != dwCount)
		return CO_S_NOTALLINTERFACES;
	
	return S_OK;
}

//
//
//
WINOLEAPI CoDisableCallCancellation(LPVOID pvReserved)
{
	return E_FAIL;
}

WINOLEAPI CoDisconnectObject(LPUNKNOWN pUnk, DWORD dwReserved)
{
	//	not implemented
	return E_FAIL;
}

//
//
//
WINOLEAPI_(BOOL) CoDosDateTimeToFileTime(WORD nDosDate, WORD nDosTime, FILETIME* lpFileTime)
{
	return TRUE;
}

//
//
//
WINOLEAPI CoEnableCallCancellation(LPVOID pvReserver)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoFileTimeNow(FILETIME* lpFileTime)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI_(BOOL) CoFileTimeToDosDateTime(FILETIME* lpFileTime, LPWORD lpDosDate, LPWORD lpDosTime)
{
	return E_FAIL;
}*/

//
//	CoFreeAllLibraries frees all libraries loaded by CoLoadLibrary
//	from memory.
//
WINOLEAPI_(VOID) CoFreeAllLibraries()
{
	return;
}
/*
//
//
//
WINOLEAPI_(void) CoFreeLibrary(HINSTANCE hInst)
{
	return;
}

//
//
//
WINOLEAPI_(void) CoFreeUnusedLibraries()
{
	return;
}

//
//
//
WINOLEAPI CoGetCallContext(REFIID riid, void** ppInterface)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetCancelObject(DWORD dwThreadId, REFIID iid, void** ppUnk)
{
	return E_FAIL;
}*/

//
//	CoCreateInstance creates an instance of some COM object
//
WINOLEAPI CoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, VOID** ppv)
{
	HRESULT	hr = S_OK;
	IClassFactory*	pCF;

	if(ppv == NULL)
		return E_POINTER;

	*ppv = NULL;

	//	create the class factory
	hr = CoGetClassObject(rclsid, dwClsContext, NULL, &IID_IClassFactory, (VOID**) &pCF);
	if(FAILED(hr))
		return hr;

	//	create one instance of this class
	pCF->lpVtbl->CreateInstance(pCF, pUnkOuter, riid, ppv);
	pCF->lpVtbl->Release(pCF);

	return hr;
}

//
//	CoGetClassObject gets the class factory for a specified object
//
WINOLEAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, VOID* pvReserved, REFIID riid, VOID** ppv)
{
	HMODULE		hComDll;
	HRESULT		hr = S_OK;
	typedef		HRESULT		(*PF) (REFCLSID, REFIID, VOID**);
	PF fpDllGetClassObject;

	//	sanity checks
	if(ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	//	only the CLSCTX_INPROC_SERVER is valid now
	if(dwClsContext != CLSCTX_INPROC_SERVER)
		return E_FAIL;

	//	fix:	CoGetClassObject should call CoLoadLibrary as stated in the win32 api docs
	hComDll = LoadLibrary("C:\\Com.dll");
	fpDllGetClassObject = (PF) GetProcAddress(hComDll, "DllGetClassObject");
	
	
	//	get the class object
	fpDllGetClassObject(rclsid, riid, (VOID**) ppv);
	if(*ppv == NULL)
		return E_FAIL;

	/*
	//HKEY		hCLSID;
	HKEY		hKey;
	HKEY		hInprocServer;
	char		szDllPath[MAX_PATH + 1];
	DWORD		dwSize;
	

	//	convert the CLISD to a string
	char szCLSID[37] = CLSIDToString(rclsid);

	//	get the dll path from the registry
	if(RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return E_FAIL;
	if(RegOpenKeyEx(hKey, szCLSID, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return E_FAIL;
	if(RegOpenKeyEx(hKey, "InprocServer32", 0, KEY_READ, &hInprocServer) != ERROR_SUCCESS)
		return E_FAIL;
	if(RegQueryValueEx(hInprocServer, NULL, NULL, NULL, (unsigned char*) szDllPath, &dwSize) != ERROR_SUCCESS)
		return E_FAIL;
	*/

	return hr;
}
/*
//
//
//
WINOLEAPI_(DWORD) CoGetCurrentProcess()
{
	return 0;
}

//
//
//
WINOLEAPI CoGetInstanceFromFile(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter,
							  DWORD dwClsCtx, DWORD grfMode, OLECHAR* pwszName, DWORD dwCount, MULTI_QI* pResults)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetInstanceFromIStorage(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter,
								  DWORD dwClsCtx, IStorage *pstg, DWORD dwCount, MULTI_QI* pResults)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetInterfaceAndReleaseStream(LPSTREAM pStm, REFIID iid, LPVOID* ppv)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetMalloc(DWORD dwMemContext, LPMALLOC* ppMalloc)
{
    return S_OK;
}

//
//
//
WINOLEAPI CoGetMarshalSizeMax(ULONG* pulSize, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext,
							LPVOID pvdestContext, DWORD mshlflags)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetObject(LPCWSTR pszName, BIND_OPTS* pBindOptions, REFIID riid, void** ppv)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetObjectContext(REFIID riid, LPVOID FAR* ppv)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetPSClsid(REFIID riid, CLSID* pClsid)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvdestContext,
							 DWORD mshlflags, LPMARSHAL* ppMarshal)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetStdMarshalEx(LPUNKNOWN pUnkOuter, DWORD smexflags, LPUNKNOWN* ppUnkInner)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew)
{
	return E_FAIL;
}

//
//
//
WINOLEAPI CoImpersonateClient()
{
	return E_FAIL;
}*/

//
//	CoInitializeEx is the new function to init COM runtime on
//	the current thread, setting it's concurrency model, and
//	creating an appartment as needed.
//
WINOLEAPI CoInitializeEx(VOID* lpReserved, DWORD dwCoInit) 
{
	HRESULT hr = S_OK;
	
	if(lpReserved != NULL)
		return E_INVALIDARG;

	//	only apartement threaded COM is supported for now
	if(dwCoInit != COINIT_APARTMENTTHREADED)
		return E_UNEXPECTED;

	lCOMLockCount++;
	return hr;
}

//
//	CoInitialize is called to initialize the COM runtime on
//	the current thread.
//
WINOLEAPI CoInitialize(VOID* lpReserved)
{
	//	CoInitializeEx is actually the new function to init COM
	return CoInitializeEx(lpReserved, COINIT_APARTMENTTHREADED);
}

//
//	CoUninitialzie closes the COM runtime on the current thread
//	unloads all COM dll's and resources loaded by the thread, and
//	forces all RPC connections on this thread to close.
//
WINOLEAPI_(VOID) CoUninitialize()
{
	lCOMLockCount--;

	//	free all resources for the last Unitialize call
	if(lCOMLockCount == 0)
	{
		/*RunningObjectTableImpl_UnInitialize();	WINE
		
		COM_RevokeAllClasses();
		COM_ExternalLockFreeList();*/
		CoFreeAllLibraries();
	}
}
/*
/*WINOLEAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy)
{
	return S_OK;
}

WINOLEAPI CoRevokeMallocSpy()
{
	return S_OK;
}

WINOLEAPI CoRegisterClassObject(REFCLSID rclsid, LPUNKNOWN pUnk, DWORD dwClsContext, DWORD flags, LPDWORD lpdwRegister)
{
	return S_OK;
}

WINOLEAPI CoRevokeClassObject(DWORD dwRegister)
{
	return S_OK;
}

WINOLEAPI CoResumeClassObjects()
{
	return S_OK;
}

WINOLEAPI CoSuspendClassObjects()
{
	return S_OK;
}

WINOLEAPI_(ULONG) CoReleaseServerProcess()
{
	return 0;
}

WINOLEAPI  CoRegisterPSClsid(IN REFIID riid, IN REFCLSID rclsid)
{
	return S_OK;
}

WINOLEAPI  CoRegisterSurrogate(IN LPSURROGATE pSurrogate)
{
	return S_OK;
}


WINOLEAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvDestContext,
							 DWORD mshlflags)
{
	return S_OK;
}

WINOLEAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv)
{
	return S_OK;
}

WINOLEAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult)
{
	return S_OK;
}

WINOLEAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT FAR * phresult)
{
	return S_OK;
}

WINOLEAPI CoReleaseMarshalData(LPSTREAM pStm)
{
	return S_OK;
}

WINOLEAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock, BOOL fLastUnlockReleases)
{
	return S_OK;
}

WINOLEAPI_(BOOL) CoIsHandlerConnected(LPUNKNOWN pUnk)
{
	return S_OK;
}


WINOLEAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, LPUNKNOWN pUnk, LPSTREAM *ppStm)
{
	return S_OK;
}

WINOLEAPI_(HINSTANCE) CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree)
{
	return NULL;
}

WINOLEAPI CoInitializeSecurity(
	PSECURITY_DESCRIPTOR         pSecDesc,
	LONG                         cAuthSvc,
	SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
	void                        *pReserved1,
	DWORD                        dwAuthnLevel,
	DWORD                        dwImpLevel,
	void                        *pAuthList,
	DWORD                        dwCapabilities,
	void                        *pReserved3 )
{
	return S_OK;
}

WINOLEAPI CoQueryProxyBlanket(
	IUnknown                  *pProxy,
	DWORD                     *pwAuthnSvc,
	DWORD                     *pAuthzSvc,
	OLECHAR                  **pServerPrincName,
	DWORD                     *pAuthnLevel,
	DWORD                     *pImpLevel,
	RPC_AUTH_IDENTITY_HANDLE  *pAuthInfo,
	DWORD                     *pCapabilites )
{
	return S_OK;
}

WINOLEAPI CoSetProxyBlanket(
	IUnknown                 *pProxy,
	DWORD                     dwAuthnSvc,
	DWORD                     dwAuthzSvc,
	OLECHAR                  *pServerPrincName,
	DWORD                     dwAuthnLevel,
	DWORD                     dwImpLevel,
	RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
	DWORD                     dwCapabilities )
{
	return S_OK;
}

WINOLEAPI CoQueryClientBlanket(
	DWORD             *pAuthnSvc,
	DWORD             *pAuthzSvc,
	OLECHAR          **pServerPrincName,
	DWORD             *pAuthnLevel,
	DWORD             *pImpLevel,
	RPC_AUTHZ_HANDLE  *pPrivs,
	DWORD             *pCapabilities )
{
	return S_OK;
}

WINOLEAPI CoRevertToSelf()
{
	return S_OK;
}

WINOLEAPI CoQueryAuthenticationServices(
	DWORD *pcAuthSvc,
	SOLE_AUTHENTICATION_SERVICE **asAuthSvc )
{
	return S_OK;
}

WINOLEAPI CoSwitchCallContext(IUnknown *pNewObject, IUnknown **ppOldObject )
{
	return S_OK;
}

WINOLEAPI CoSetCancelObject(IUnknown *pUnk)
{
	return S_OK;
}

WINOLEAPI CoTestCancel()
{
	return S_OK;
}

WINOLEAPI CoRegisterMessageFilter( IN LPMESSAGEFILTER lpMessageFilter,
                                OUT LPMESSAGEFILTER FAR* lplpMessageFilter)
{
	return S_OK;
}

WINOLEAPI CoRegisterChannelHook(REFGUID ExtensionUuid, IChannelHook *pChannelHook )
{
	return S_OK;
}

WINOLEAPI CoWaitForMultipleHandles(IN DWORD dwFlags,
                                    IN DWORD dwTimeout,
                                    IN ULONG cHandles,
                                    IN LPHANDLE pHandles,
                                    OUT LPDWORD  lpdwindex)
{
	return S_OK;
}

WINOLEAPI CoTreatAsClass(IN REFCLSID clsidOld, IN REFCLSID clsidNew)
{
	return S_OK;
}

WINOLEAPI_(LPVOID) CoTaskMemAlloc(IN SIZE_T cb)
{
	return NULL;
}

WINOLEAPI_(LPVOID) CoTaskMemRealloc(IN LPVOID pv, IN SIZE_T cb)
{
	return NULL;
}

WINOLEAPI_(void)   CoTaskMemFree(IN LPVOID pv)
{
	return;
}*/
