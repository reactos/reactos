#include "precomp.h"

#include <olectl.h>

HINSTANCE netshell_hInstance;

static const WCHAR szNetConnectClass[]    = L"CLSID\\{7007ACC7-3202-11D1-AAD2-00805FC1270E}";
static const WCHAR szLanConnectUI[]       = L"CLSID\\{7007ACC5-3202-11D1-AAD2-00805FC1270E}";
static const WCHAR szLanConnectStatusUI[] = L"CLSID\\{7007ACCF-3202-11D1-AAD2-00805FC1270E}";
static const WCHAR szNamespaceKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\{7007ACC7-3202-11D1-AAD2-00805FC1270E}";

extern "C"
{

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            netshell_hInstance = hinstDLL;
            DisableThreadLibraryCalls(netshell_hInstance);
            break;
    default:
        break;
    }

    return TRUE;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_FALSE;
}

STDAPI
DllRegisterServer(void)
{
    HKEY hKey, hSubKey;
    WCHAR szName[MAX_PATH+20] = {0};
    WCHAR szNet[20];
    UINT Length, Offset;


    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szNetConnectClass, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    if (LoadStringW(netshell_hInstance, IDS_NETWORKCONNECTION, szName, MAX_PATH))
    {
        szName[MAX_PATH-1] = L'\0';
        RegSetValueW(hKey, NULL, REG_SZ, szName, (wcslen(szName)+1) * sizeof(WCHAR));
    }

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szNamespaceKey, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueW(hSubKey, NULL, REG_SZ, szName, (wcslen(szName)+1) * sizeof(WCHAR));
        RegCloseKey(hSubKey);
    }

    Length = swprintf(szNet, L",-%u", IDS_NETWORKCONNECTION);
    Offset = GetModuleFileNameW(netshell_hInstance, &szName[1], (sizeof(szName)/sizeof(WCHAR))-1);
    if (Offset + Length + 2 < MAX_PATH)
    {
        /* set localized name */
        szName[0] = L'@';
        wcscpy(&szName[Offset+1], szNet);
        RegSetValueExW(hKey, L"LocalizedString", 0, REG_SZ, (const LPBYTE)szName, (wcslen(szName)+1) * sizeof(WCHAR));
    }

    szName[Offset+1] = L'\0';

    /* store default icon */
    if (RegCreateKeyExW(hKey, L"DefaultIcon", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueW(hSubKey, NULL, REG_SZ, &szName[1], (Offset+1) * sizeof(WCHAR));
        RegCloseKey(hSubKey);
    }
    if (RegCreateKeyExW(hKey, L"InProcServer32", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueW(hSubKey, NULL, REG_SZ, &szName[1], (Offset+1) * sizeof(WCHAR));
        RegSetValueExW(hSubKey, L"ThreadingModel", 0, REG_SZ, (LPBYTE)L"Both", 10);
        RegCloseKey(hSubKey);
    }

    if (RegCreateKeyExW(hKey, L"ShellFolder", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        DWORD dwAttributes = SFGAO_FOLDER;
        RegSetValueExW(hSubKey, L"Attributes",0, REG_BINARY, (const LPBYTE)&dwAttributes, sizeof(DWORD));
    }

    RegCloseKey(hKey);

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szLanConnectUI, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    if (RegCreateKeyExW(hKey, L"InProcServer32", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueW(hSubKey, NULL, REG_SZ, &szName[1], (Offset+1) * sizeof(WCHAR));
        RegSetValueExW(hSubKey, L"ThreadingModel", 0, REG_SZ, (LPBYTE)L"Both", 10);
        RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szLanConnectStatusUI, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    if (RegCreateKeyExW(hKey, L"InProcServer32", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueW(hSubKey, NULL, REG_SZ, &szName[1], (Offset+1) * sizeof(WCHAR));
        RegSetValueExW(hSubKey, L"ThreadingModel", 0, REG_SZ, (LPBYTE)L"Both", 10);
        RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);


    return S_OK;
}

STDAPI
DllUnregisterServer(void)
{
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szNetConnectClass);
    SHDeleteKeyW(HKEY_LOCAL_MACHINE, szNamespaceKey);
    return S_OK;
}

STDAPI
DllGetClassObject(
  REFCLSID rclsid,
  REFIID riid,
  LPVOID *ppv)
{
    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    return IClassFactory_fnConstructor(rclsid, riid, ppv);
}

VOID
WINAPI
NcFreeNetconProperties(NETCON_PROPERTIES *pProps)
{
    CoTaskMemFree(pProps->pszwName);
    CoTaskMemFree(pProps->pszwDeviceName);
    CoTaskMemFree(pProps);
}

} // extern "C"
