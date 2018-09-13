#include "mslocusr.h"
#include "msluglob.h"


STDMETHODIMP CLUClassFactory::QueryInterface(
	/* [in] */ REFIID riid,
	/* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IClassFactory)) {
		*ppvObject = (LPVOID)this;
		AddRef();
		return NOERROR;
	}
	return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CLUClassFactory::AddRef(void)
{
	RefThisDLL(TRUE);

	return 1;
}


STDMETHODIMP_(ULONG) CLUClassFactory::Release(void)
{
	RefThisDLL(FALSE);

	return 1;
}


HRESULT CreateUserDatabase(REFIID riid, void **ppOut)
{
	CLUDatabase *pObj = new CLUDatabase;

	if (NULL == pObj)
		return ResultFromScode(E_OUTOFMEMORY);

	HRESULT hr = pObj->QueryInterface(riid, ppOut);

	if (FAILED(hr)) {
		delete pObj;
	}
	else {
		pObj->AddRef();
	}

	return NOERROR;
}


STDMETHODIMP CLUClassFactory::CreateInstance(
	/* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
	/* [in] */ REFIID riid,
	/* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (NULL != pUnkOuter)
		return ResultFromScode(CLASS_E_NOAGGREGATION);

	return CreateUserDatabase(riid, ppvObject);
}

        
STDMETHODIMP CLUClassFactory::LockServer( 
	/* [in] */ BOOL fLock)
{
	LockThisDLL(fLock);

	return NOERROR;
}


/* This function signals our stub NP DLL in MPREXE's context to load or
 * unload MSLOCUSR as a net provider DLL.  This way, when we uninstall,
 * DllUnregisterServer can get MSLOCUSR unloaded from MPREXE's process
 * space, so it can be deleted and upgraded.  Then, DllRegisterServer
 * will tell the stub NP to reload MSLOCUSR again.
 */
void SignalStubNP(BOOL fLoad)
{
    HWND hwnd = FindWindow("WndClass_NPSTUBMonitor", NULL);
    if (hwnd != NULL) {
        SendMessage(hwnd, fLoad ? WM_USER : (WM_USER+1), 0, 0);
    }
}


extern "C" {

STDAPI DllRegisterServer(void)
{
	HKEY hkeyCLSID;
	HKEY hkeyOurs;
	HKEY hkeyInproc;
	LONG err;

	err = ::RegOpenKey(HKEY_CLASSES_ROOT, ::szCLSID, &hkeyCLSID);
	if (err == ERROR_SUCCESS) {
		err = ::RegCreateKey(hkeyCLSID, ::szOurCLSID, &hkeyOurs);
		if (err == ERROR_SUCCESS) {
			err = ::RegCreateKey(hkeyOurs, ::szINPROCSERVER32, &hkeyInproc);
			if (err == ERROR_SUCCESS) {
				err = ::RegSetValueEx(hkeyInproc, NULL, 0, REG_SZ,
									  (LPBYTE)::szDLLNAME, ::strlenf(::szDLLNAME));
				if (err == ERROR_SUCCESS) {
					err = ::RegSetValueEx(hkeyInproc, ::szTHREADINGMODEL, 0,
										  REG_SZ, (LPBYTE)::szAPARTMENT,
										  ::strlenf(::szAPARTMENT));
				}
				::RegCloseKey(hkeyInproc);
			}

			::RegCloseKey(hkeyOurs);
		}

		::RegCloseKey(hkeyCLSID);
	}

	if (err == ERROR_SUCCESS)
		return S_OK;
	else
		return HRESULT_FROM_WIN32(err);
}


STDAPI DllUnregisterServer(void)
{
	HKEY hkeyCLSID;
	HKEY hkeyOurs;
	LONG err;

	err = ::RegOpenKey(HKEY_CLASSES_ROOT, ::szCLSID, &hkeyCLSID);
	if (err == ERROR_SUCCESS) {
		err = ::RegOpenKey(hkeyCLSID, ::szOurCLSID, &hkeyOurs);
		if (err == ERROR_SUCCESS) {
			err = ::RegDeleteKey(hkeyOurs, ::szINPROCSERVER32);

			::RegCloseKey(hkeyOurs);

			if (err == ERROR_SUCCESS)
				err = ::RegDeleteKey(hkeyCLSID, ::szOurCLSID);
		}

		::RegCloseKey(hkeyCLSID);
	}

    DeinstallLogonDialog();

	if (err == ERROR_SUCCESS)
		return S_OK;
	else
		return HRESULT_FROM_WIN32(err);
}


STDAPI DllInstall(BOOL fInstall, LPCSTR psz)
{
    SignalStubNP(fInstall);
    return S_OK;
}


STDAPI DllCanUnloadNow(void)
{
	SCODE sc;

	sc = (0 == g_cRefThisDll && 0 == g_cLocks) ? S_OK : S_FALSE;
	return ResultFromScode(sc);
}


STDAPI DllGetClassObject(
	REFCLSID rclsid,
	REFIID riid,
	LPVOID FAR *ppv)
{
	if (!IsEqualCLSID(rclsid, CLSID_LocalUsers)) {
		return ResultFromScode(E_FAIL);
	}

	if (!IsEqualIID(riid, IID_IUnknown) &&
		!IsEqualIID(riid, IID_IClassFactory)) {
		return ResultFromScode(E_NOINTERFACE);
	}

	static CLUClassFactory cf;

	*ppv = (LPVOID)&cf;

	cf.AddRef();

	return NOERROR;
}

};	/* extern "C" */
