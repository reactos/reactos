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

//#define INITGUID
#include <ole32/ole32.h>

#include <debug.h>

//
//	lCOMLockCount is a reference count, when it reaches 0, all COM libraries are freed
//
static ULONG lCOMLockCount = 0;

WINOLEAPI_(ULONG) CoAddRefServerProcess()
{
  UNIMPLEMENTED;

	return 0L;
}

WINOLEAPI CoAllowSetForegroundWindow(IUnknown *pUnk, LPVOID lpvReserved)
{
  UNIMPLEMENTED;

  return E_FAIL;
}


WINOLEAPI CoCancelCall(DWORD dwThreadId, ULONG ulTimeout)
{
  UNIMPLEMENTED;

	return E_FAIL;
}

WINOLEAPI CoCopyProxy(IUnknown* pProxy, IUnknown** ppCopy)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoDisableCallCancellation(LPVOID pvReserved)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI_(BOOL) CoDosDateTimeToFileTime(WORD nDosDate, WORD nDosTime, FILETIME* lpFileTime)
{
  UNIMPLEMENTED;

	return TRUE;
}

//
//
//
WINOLEAPI CoEnableCallCancellation(LPVOID pvReserver)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI_(BOOL) CoFileTimeToDosDateTime(FILETIME* lpFileTime, LPWORD lpDosDate, LPWORD lpDosTime)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetCallContext(REFIID riid, void** ppInterface)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetCancelObject(DWORD dwThreadId, REFIID iid, void** ppUnk)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI_(DWORD) CoGetCurrentProcess()
{
  UNIMPLEMENTED;

	return 0;
}

//
//
//
WINOLEAPI CoGetInstanceFromFile(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter,
							  DWORD dwClsCtx, DWORD grfMode, OLECHAR* pwszName, DWORD dwCount, MULTI_QI* pResults)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetInstanceFromIStorage(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter,
								  DWORD dwClsCtx, IStorage *pstg, DWORD dwCount, MULTI_QI* pResults)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetInterfaceAndReleaseStream(LPSTREAM pStm, REFIID iid, LPVOID* ppv)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetMarshalSizeMax(ULONG* pulSize, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext,
							LPVOID pvdestContext, DWORD mshlflags)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetObject(LPCWSTR pszName, BIND_OPTS* pBindOptions, REFIID riid, void** ppv)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetObjectContext(REFIID riid, LPVOID FAR* ppv)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvdestContext,
							 DWORD mshlflags, LPMARSHAL* ppMarshal)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetStdMarshalEx(LPUNKNOWN pUnkOuter, DWORD smexflags, LPUNKNOWN* ppUnkInner)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew)
{
  UNIMPLEMENTED;

  return E_FAIL;
}

//
//
//
WINOLEAPI CoImpersonateClient()
{
  UNIMPLEMENTED;

  return E_FAIL;
}

WINOLEAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoRevokeMallocSpy()
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoSuspendClassObjects()
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI_(ULONG) CoReleaseServerProcess()
{
  UNIMPLEMENTED;

	return 0;
}

WINOLEAPI CoRegisterPSClsid(IN REFIID riid, IN REFCLSID rclsid)
{
  UNIMPLEMENTED;

	return S_OK;
}

#if 0
WINOLEAPI CoRegisterSurrogate(IN LPSURROGATE pSurrogate)
{
  UNIMPLEMENTED;

	return S_OK;
}
#endif

WINOLEAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvDestContext,
							 DWORD mshlflags)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT FAR * phresult)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoReleaseMarshalData(LPSTREAM pStm)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI_(BOOL) CoIsHandlerConnected(LPUNKNOWN pUnk)
{
  UNIMPLEMENTED;

	return S_OK;
}


WINOLEAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, LPUNKNOWN pUnk, LPSTREAM *ppStm)
{
  UNIMPLEMENTED;

	return S_OK;
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
  UNIMPLEMENTED;

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
  UNIMPLEMENTED;

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
  UNIMPLEMENTED;

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
  UNIMPLEMENTED;

	return S_OK;

}

WINOLEAPI CoRevertToSelf()
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoQueryAuthenticationServices(
	DWORD *pcAuthSvc,
	SOLE_AUTHENTICATION_SERVICE **asAuthSvc )
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoSwitchCallContext(IUnknown *pNewObject, IUnknown **ppOldObject )
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoSetCancelObject(IUnknown *pUnk)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoTestCancel()
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoRegisterMessageFilter( IN LPMESSAGEFILTER lpMessageFilter,
                                OUT LPMESSAGEFILTER FAR* lplpMessageFilter)
{
  UNIMPLEMENTED;

	return S_OK;
}

#if 0
WINOLEAPI CoRegisterChannelHook(REFGUID ExtensionUuid, IChannelHook *pChannelHook )
{
  UNIMPLEMENTED;

	return S_OK;
}
#endif

WINOLEAPI CoWaitForMultipleHandles(IN DWORD dwFlags,
                                    IN DWORD dwTimeout,
                                    IN ULONG cHandles,
                                    IN LPHANDLE pHandles,
                                    OUT LPDWORD  lpdwindex)
{
  UNIMPLEMENTED;

	return S_OK;
}

WINOLEAPI CoTreatAsClass(IN REFCLSID clsidOld, IN REFCLSID clsidNew)
{
  UNIMPLEMENTED;

	return S_OK;
}
