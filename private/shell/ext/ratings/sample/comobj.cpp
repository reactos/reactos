// 
// standard inprocserver DLL code, you should not need to mess with this
//

#include "project.h"


HANDLE g_hInst = NULL;
LONG g_cRefDll = 0;     // Number of locks on this DLL



STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}


STDAPI_(BOOL) DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstDll;
    }
    return TRUE;
}

STDMETHODIMP CSampleClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
	if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IClassFactory)) {
		*ppvObject = (void *)this;
		AddRef();
		return NOERROR;
	}

	*ppvObject = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSampleClassFactory::AddRef(void)
{
	DllAddRef();
	return 2;
}

STDMETHODIMP_(ULONG) CSampleClassFactory::Release(void)
{
	DllRelease();
	return 1;
}

STDMETHODIMP CSampleClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

	if (NULL != pUnkOuter)
		return CLASS_E_NOAGGREGATION;

	CSampleObtainRating *pObj = new CSampleObtainRating;	/* doing this does implicit AddRef() */

	if (NULL == pObj)
		return E_OUTOFMEMORY;

	HRESULT hr = pObj->QueryInterface(riid, ppvObject);
    pObj->Release();

	return hr;
}
        
STDMETHODIMP CSampleClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return NOERROR;
}

//
// standard COM DLL self registering entry point
//

STDAPI DllRegisterServer(void)
{
	HKEY hkeyCLSID;
	LONG err;
    TCHAR szPath[MAX_PATH];

    // get path to this DLL

    GetModuleFileName(g_hInst, szPath, MAX_PATH);

	/* First register our CLSID under HKEY_CLASSES_ROOT. */
	err = ::RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", &hkeyCLSID);
	if (err == ERROR_SUCCESS) {
    	HKEY hkeyOurs;
		err = ::RegCreateKey(hkeyCLSID, ::szOurGUID, &hkeyOurs);
		if (err == ERROR_SUCCESS) {
        	HKEY hkeyInproc;
			err = ::RegCreateKey(hkeyOurs, "InProcServer32", &hkeyInproc);
			if (err == ERROR_SUCCESS) {
				err = ::RegSetValueEx(hkeyInproc, NULL, 0, REG_SZ,
					(LPBYTE)szPath, lstrlen(szPath) + 1);
				if (err == ERROR_SUCCESS) {
					err = ::RegSetValueEx(hkeyInproc, "ThreadingModel", 0,
										  REG_SZ, (LPBYTE)"Apartment", 10);
				}
				::RegCloseKey(hkeyInproc);
			}

			::RegCloseKey(hkeyOurs);
		}

		::RegCloseKey(hkeyCLSID);

		/* Now install ourselves as a ratings helper. */
		if (err == ERROR_SUCCESS) {
			err = ::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Rating Helpers", &hkeyCLSID);
			if (err == ERROR_SUCCESS) {
				err = ::RegSetValueEx(hkeyCLSID, ::szOurGUID, 0, REG_SZ, (LPBYTE)"", 2);
				::RegCloseKey(hkeyCLSID);
			}
		}
	}

	if (err == ERROR_SUCCESS)
		return S_OK;
	else
		return HRESULT_FROM_WIN32(err);
}

//
// standard COM DLL self registering entry point
//

STDAPI DllUnregisterServer(void)
{
	HKEY hkeyCLSID;
	LONG err;

	err = ::RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", &hkeyCLSID);
	if (err == ERROR_SUCCESS) {
    	HKEY hkeyOurs;
		err = ::RegOpenKey(hkeyCLSID, ::szOurGUID, &hkeyOurs);
		if (err == ERROR_SUCCESS) {
			err = ::RegDeleteKey(hkeyOurs, "InProcServer32");

			::RegCloseKey(hkeyOurs);

			if (err == ERROR_SUCCESS)
				err = ::RegDeleteKey(hkeyCLSID, ::szOurGUID);
		}

		::RegCloseKey(hkeyCLSID);

		if (err == ERROR_SUCCESS) {
			err = ::RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Rating Helpers", &hkeyCLSID);
			if (err == ERROR_SUCCESS) {
				err = ::RegDeleteValue(hkeyCLSID, ::szOurGUID);
				::RegCloseKey(hkeyCLSID);
			}
		}
	}

	if (err == ERROR_SUCCESS)
		return S_OK;
	else
		return HRESULT_FROM_WIN32(err);
}

//
// standard COM DLL entry point
//

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
	if (IsEqualCLSID(rclsid, CLSID_Sample)) 
    {
	    static CSampleClassFactory cf;	/* note, declaring this doesn't constitute a reference */

	    return cf.QueryInterface(riid, ppv);	/* will AddRef() if successful */
	}
    // to make this support more com objects add them here

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

//
// standard COM DLL entry point
//

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}
