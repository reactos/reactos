/*
 *	exported dll functions for devenum.dll
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "devenum_private.h"
#include "wine/debug.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

LONG dll_refs;
HINSTANCE DEVENUM_hInstance;

typedef struct
{
    REFCLSID clsid;
    LPCWSTR friendly_name;
    BOOL instance;
} register_info;

static HRESULT register_clsids(int count, const register_info * pRegInfo, LPCWSTR pszThreadingModel);
static void DEVENUM_RegisterQuartz(void);

/***********************************************************************
 *		Global string constant definitions
 */
const WCHAR clsid_keyname[6] = { 'C', 'L', 'S', 'I', 'D', 0 };

/***********************************************************************
 *		DllEntryPoint
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DEVENUM_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
	break;

    case DLL_PROCESS_DETACH:
        DEVENUM_hInstance = 0;
	break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject (DEVENUM.@)
 */
HRESULT WINAPI DEVENUM_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    *ppv = NULL;

    /* FIXME: we should really have two class factories.
     * Oh well - works just fine as it is */
    if (IsEqualGUID(rclsid, &CLSID_SystemDeviceEnum) ||
        IsEqualGUID(rclsid, &CLSID_CDeviceMoniker))
        return IClassFactory_QueryInterface((IClassFactory*)&DEVENUM_ClassFactory, iid, ppv);

    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (DEVENUM.@)
 */
HRESULT WINAPI DEVENUM_DllCanUnloadNow(void)
{
    return dll_refs != 0 ? S_FALSE : S_OK;
}

/***********************************************************************
 *		DllRegisterServer (DEVENUM.@)
 */
HRESULT WINAPI DEVENUM_DllRegisterServer(void)
{
    HRESULT res;
    HKEY hkeyClsid = NULL;
    HKEY hkey1 = NULL;
    HKEY hkey2 = NULL;
    LPOLESTR pszClsidDevMon = NULL;
    IFilterMapper2 * pMapper = NULL;
    LPVOID mapvptr;
    static const WCHAR threadingModel[] = {'B','o','t','h',0};
    static const WCHAR sysdevenum[] = {'S','y','s','t','e','m',' ','D','e','v','i','c','e',' ','E','n','u','m',0};
    static const WCHAR devmon[] = {'D','e','v','i','c','e','M','o','n','i','k','e','r',0};
    static const WCHAR acmcat[] = {'A','C','M',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r',0};
    static const WCHAR vidcat[] = {'I','C','M',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r',0};
    static const WCHAR filtcat[] = {'A','c','t','i','v','e','M','o','v','i','e',' ','F','i','l','t','e','r',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r',0};
    static const WCHAR vfwcat[] = {'V','F','W',' ','C','a','p','t','u','r','e',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r',0};
    static const WCHAR wavein[] = {'W','a','v','e','I','n',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r', 0};
    static const WCHAR waveout[] = {'W','a','v','e','O','u','t',' ','a','n','d',' ','D','S','o','u','n','d',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r',0};
    static const WCHAR midiout[] = {'M','i','d','i','O','u','t',' ','C','l','a','s','s',' ','M','a','n','a','g','e','r',0};
    static const WCHAR amcat[] = {'A','c','t','i','v','e','M','o','v','i','e',' ','F','i','l','t','e','r',' ','C','a','t','e','g','o','r','i','e','s',0};
    static const WCHAR device[] = {'d','e','v','i','c','e',0};
    static const WCHAR device_1[] = {'d','e','v','i','c','e','.','1',0};
    static const register_info ri[] =
    {
        {&CLSID_SystemDeviceEnum, sysdevenum, FALSE},
	{&CLSID_CDeviceMoniker, devmon, FALSE},
	{&CLSID_AudioCompressorCategory, acmcat, TRUE},
	{&CLSID_VideoCompressorCategory, vidcat, TRUE},
	{&CLSID_LegacyAmFilterCategory, filtcat, TRUE},
	{&CLSID_VideoInputDeviceCategory, vfwcat, FALSE},
	{&CLSID_AudioInputDeviceCategory, wavein, FALSE},
	{&CLSID_AudioRendererCategory, waveout, FALSE},
	{&CLSID_MidiRendererCategory, midiout, FALSE},
	{&CLSID_ActiveMovieCategories, amcat, TRUE}
    };

    TRACE("\n");

    res = register_clsids(sizeof(ri) / sizeof(register_info), ri, threadingModel);

    /* Quartz is needed for IFilterMapper2 */
    DEVENUM_RegisterQuartz();

/*** ActiveMovieFilter Categories ***/

    CoInitialize(NULL);
    
    res = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC,
                           &IID_IFilterMapper2,  &mapvptr);
    if (SUCCEEDED(res))
    {
        static const WCHAR friendlyvidcap[] = {'V','i','d','e','o',' ','C','a','p','t','u','r','e',' ','S','o','u','r','c','e','s',0};
        static const WCHAR friendlydshow[] = {'D','i','r','e','c','t','S','h','o','w',' ','F','i','l','t','e','r','s',0};
        static const WCHAR friendlyvidcomp[] = {'V','i','d','e','o',' ','C','o','m','p','r','e','s','s','o','r','s',0};
        static const WCHAR friendlyaudcap[] = {'A','u','d','i','o',' ','C','a','p','t','u','r','e',' ','S','o','u','r','c','e','s',0};
        static const WCHAR friendlyaudcomp[] = {'A','u','d','i','o',' ','C','o','m','p','r','e','s','s','o','r','s',0};
        static const WCHAR friendlyaudrend[] = {'A','u','d','i','o',' ','R','e','n','d','e','r','e','r','s',0};
        static const WCHAR friendlymidirend[] = {'M','i','d','i',' ','R','e','n','d','e','r','e','r','s',0};
        static const WCHAR friendlyextrend[] = {'E','x','t','e','r','n','a','l',' ','R','e','n','d','e','r','e','r','s',0};
        static const WCHAR friendlydevctrl[] = {'D','e','v','i','c','e',' ','C','o','n','t','r','o','l',' ','F','i','l','t','e','r','s',0};

        pMapper = (IFilterMapper2*)mapvptr;

        IFilterMapper2_CreateCategory(pMapper, &CLSID_VideoInputDeviceCategory, MERIT_DO_NOT_USE, friendlyvidcap);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_LegacyAmFilterCategory, MERIT_NORMAL, friendlydshow);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_VideoCompressorCategory, MERIT_DO_NOT_USE, friendlyvidcomp);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioInputDeviceCategory, MERIT_DO_NOT_USE, friendlyaudcap);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioCompressorCategory, MERIT_DO_NOT_USE, friendlyaudcomp);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioRendererCategory, MERIT_NORMAL, friendlyaudrend);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_MidiRendererCategory, MERIT_NORMAL, friendlymidirend);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_TransmitCategory, MERIT_DO_NOT_USE, friendlyextrend);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_DeviceControlCategory, MERIT_DO_NOT_USE, friendlydevctrl);

        IFilterMapper2_Release(pMapper);
    }

/*** CDeviceMoniker ***/
    if (SUCCEEDED(res))
    {
	res = StringFromCLSID(&CLSID_CDeviceMoniker, &pszClsidDevMon);
    }
    if (SUCCEEDED(res))
    {
        res = RegOpenKeyW(HKEY_CLASSES_ROOT, clsid_keyname, &hkeyClsid)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegOpenKeyW(hkeyClsid, pszClsidDevMon, &hkey1)
	       == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        static const WCHAR wszProgID[] = {'P','r','o','g','I','D',0};
        res = RegCreateKeyW(hkey1, wszProgID, &hkey2)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegSetValueW(hkey2, NULL, REG_SZ, device_1, (lstrlenW(device_1) + 1) * sizeof(WCHAR))
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }

    if (hkey2)
    {
        RegCloseKey(hkey2);
        hkey2 = NULL;
    }

    if (SUCCEEDED(res))
    {
        static const WCHAR wszVProgID[] = {'V','e','r','s','i','o','n','I','n','d','e','p','e','d','e','n','t','P','r','o','g','I','D',0};
	res = RegCreateKeyW(hkey1, wszVProgID, &hkey2)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegSetValueW(hkey2, NULL, REG_SZ, device, (lstrlenW(device) + 1) * sizeof(WCHAR))
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }

    if (hkey2)
    {
        RegCloseKey(hkey2);
        hkey2 = NULL;
    }

    if (hkey1)
    {
        RegCloseKey(hkey1);
        hkey1 = NULL;
    }

    if (SUCCEEDED(res))
    {
        res = RegCreateKeyW(HKEY_CLASSES_ROOT, device, &hkey1)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegCreateKeyW(hkey1, clsid_keyname, &hkey2)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegSetValueW(hkey2, NULL, REG_SZ, pszClsidDevMon, (lstrlenW(pszClsidDevMon) + 1) * sizeof(WCHAR))
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (hkey2)
    {
        RegCloseKey(hkey2);
        hkey2 = NULL;
    }

    if (hkey1)
    {
        RegCloseKey(hkey1);
        hkey1 = NULL;
    }

    if (SUCCEEDED(res))
    {
        res = RegCreateKeyW(HKEY_CLASSES_ROOT, device_1, &hkey1)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegCreateKeyW(hkey1, clsid_keyname, &hkey2)
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(res))
    {
        res = RegSetValueW(hkey2, NULL, REG_SZ, pszClsidDevMon, (lstrlenW(pszClsidDevMon) + 1) * sizeof(WCHAR))
	      == ERROR_SUCCESS ? S_OK : E_FAIL;
    }

    if (hkey2)
        RegCloseKey(hkey2);

    if (hkey1)
        RegCloseKey(hkey1);

    if (hkeyClsid)
        RegCloseKey(hkeyClsid);

    if (pszClsidDevMon)
        CoTaskMemFree(pszClsidDevMon);

    CoUninitialize();

    return res;
}

/***********************************************************************
 *		DllUnregisterServer (DEVENUM.@)
 */
HRESULT WINAPI DEVENUM_DllUnregisterServer(void)
{
	FIXME("stub!\n");
	return E_FAIL;
}

static HRESULT register_clsids(int count, const register_info * pRegInfo, LPCWSTR pszThreadingModel)
{
    HRESULT res = S_OK;
    WCHAR dll_module[MAX_PATH];
    LPOLESTR clsidString;
    HKEY hkeyClsid;
    HKEY hkeySub;
    HKEY hkeyInproc32;
    HKEY hkeyInstance = NULL;
    int i;
    static const WCHAR wcszInproc32[] = {'I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};
    static const WCHAR wcszThreadingModel[] = {'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, clsid_keyname, &hkeyClsid)
          == ERROR_SUCCESS ? S_OK : E_FAIL;

    TRACE("HModule = %p\n", DEVENUM_hInstance);
    i = GetModuleFileNameW(DEVENUM_hInstance, dll_module,
                           sizeof(dll_module) / sizeof(WCHAR));
    if (!i)
	return HRESULT_FROM_WIN32(GetLastError());
    if (i >= sizeof(dll_module) / sizeof(WCHAR))
	return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    for (i = 0; i < count; i++)
    {
        if (SUCCEEDED(res))
	{
	    res = StringFromCLSID(pRegInfo[i].clsid, &clsidString);
	}
	if (SUCCEEDED(res))
	{
	    res = RegCreateKeyW(hkeyClsid, clsidString, &hkeySub)
	          == ERROR_SUCCESS ? S_OK : E_FAIL;
	}
	if (pRegInfo[i].instance && SUCCEEDED(res))
	{
	    res = RegCreateKeyW(hkeySub, wszInstanceKeyName, &hkeyInstance)
	          == ERROR_SUCCESS ? S_OK : E_FAIL;
            RegCloseKey(hkeyInstance);
	}
	if (SUCCEEDED(res))
	{
	    RegSetValueW(hkeySub,
	                 NULL,
		         REG_SZ,
	                 pRegInfo->friendly_name ? pRegInfo[i].friendly_name : clsidString,
		         (lstrlenW(pRegInfo[i].friendly_name ? pRegInfo->friendly_name : clsidString) + 1) * sizeof(WCHAR));
	    res = RegCreateKeyW(hkeySub, wcszInproc32, &hkeyInproc32)
	          == ERROR_SUCCESS ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(res))
	{
	    RegSetValueW(hkeyInproc32,
	                 NULL,
	                 REG_SZ,
			 dll_module,
			 (lstrlenW(dll_module) + 1) * sizeof(WCHAR));
	    RegSetValueExW(hkeyInproc32,
	                   wcszThreadingModel,
			   0,
			   REG_SZ,
			   (LPCVOID)pszThreadingModel,
			   (lstrlenW(pszThreadingModel) + 1) * sizeof(WCHAR));
            RegCloseKey(hkeyInproc32);
        }
        RegCloseKey(hkeySub);
	CoTaskMemFree(clsidString);
	clsidString = NULL;
    }

    RegCloseKey(hkeyClsid);

    return res;
}

typedef HRESULT (WINAPI *DllRegisterServer_func)(void);

/* calls DllRegisterServer() for the Quartz DLL */
static void DEVENUM_RegisterQuartz()
{
    HANDLE hDLL = LoadLibraryA("quartz.dll");
    DllRegisterServer_func pDllRegisterServer = NULL;
    if (hDLL)
        pDllRegisterServer = (DllRegisterServer_func)GetProcAddress(hDLL, "DllRegisterServer");
    if (pDllRegisterServer)
    {
        HRESULT hr = pDllRegisterServer();
        if (FAILED(hr))
            ERR("Failed to register Quartz. Error was 0x%lx)\n", hr);
    }
}
