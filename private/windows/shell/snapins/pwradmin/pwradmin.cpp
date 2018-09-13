#include "main.h"

#include <atlimpl.cpp>

//
// SnapIn GUID
//

DEFINE_GUID(CLSID_SnapIn,   0x12D3343E, 0xAD3C, 0x11D0, 0x91, 0xFE, 0x08, 0x00, 0x36, 0x64, 0x46, 0x03);
DEFINE_GUID(CLSID_Extension,0x5647DC9C, 0xAD3C, 0x11D0, 0x91, 0xFE, 0x08, 0x00, 0x36, 0x64, 0x46, 0x03);
const GUID NodeType =      {0x4366F48E, 0xAF79, 0x11D0,{0x91, 0xFE, 0x08, 0x00, 0x36, 0x64, 0x46, 0x03}};

const TCHAR szNodeType[]       = TEXT("{4366f48e-af79-11d0-91fe-080036644603}");
const TCHAR szExtNodeType[]    = TEXT("{9c010f46-b698-11d0-91fe-080036644603}");
const TCHAR szSnapInKey[]      = TEXT("{12D3343E-AD3C-11D0-91FE-080036644603}");
const TCHAR szExtensionKey[]   = TEXT("{5647DC9C-AD3C-11D0-91FE-080036644603}");
const TCHAR szSnapInLocation[] = TEXT("%SystemRoot%\\System32\\Pwradmin.dll");
const TCHAR szProvider[]       = TEXT("Microsoft");
const TCHAR szVersion[]        = TEXT("1.0");
const TCHAR szNodeTypeName[]   = TEXT("Power Administration Static Node");

//
// Global variables for this DLL
//

LONG g_cRefThisDll = 0;
HINSTANCE g_hInstance;

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_SnapIn, CComponentDataPrimary)
        OBJECT_ENTRY(CLSID_Extension, CComponentDataExtension)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason) {
    
    case DLL_PROCESS_ATTACH:
       g_hInstance = hInstance;
       DisableThreadLibraryCalls(hInstance);
       _Module.Init(ObjectMap, hInstance);
       break;
       
    case DLL_PROCESS_DETACH:
       _Module.Term();
       break;
       
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}
/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    TCHAR szSubKey[400];
    TCHAR szSnapInName[100];
    TCHAR szExtensionName[100];
    TCHAR szGptEditorKey[50];
    DWORD dwDisp;
    LONG lResult;
    HKEY hKey;
    
    LoadString (g_hInstance, IDS_SNAPIN_DESC, szSnapInName, 100);
    LoadString (g_hInstance, IDS_EXTENSION_DESC, szExtensionName, 100);

    //************************************************************************
    // Register Extension with MMC
    //************************************************************************

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szExtensionKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szExtensionName,
                   (lstrlen(szExtensionName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);
    
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes"), szExtensionKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);
    
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szExtensionKey, szExtNodeType);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);
    
    //************************************************************************
    // Register Extension with GPTEditor
    //************************************************************************

    StringFromGUID2 (NODEID_User, szGptEditorKey, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGptEditorKey, szNodeType);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szExtensionKey, 0, REG_SZ, (LPBYTE)szExtensionName,
                   (lstrlen(szExtensionName) + 1) * sizeof(TCHAR));
                   
    RegCloseKey (hKey);


    //
    // BugBug -- ericflo
    //
    // Power Admin was located under the OS policy section of the user settings
    // node, but then it was moved to the root of the User section.  We need to cleanup
    // the old node registration so people upgrading don't see 2 power admins.
    // This block can be removed after the Beta 2.
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Microsoft\\MMC\\NodeTypes\\{A6B4EEC1-B681-11D0-9484-080036B11A03}\\Extensions\\NameSpace"),
                      0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {

        RegDeleteValue (hKey, szExtensionKey);

        RegCloseKey (hKey);
    }

    // end BugBug

    //************************************************************************
    // Register NodeType with MMC
    //************************************************************************

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szNodeType);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szNodeTypeName,
                   (lstrlen(szNodeTypeName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);
    
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions"), szNodeType);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szNodeType);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szExtensionKey, 0, REG_SZ, (LPBYTE)szExtensionName,
                   (lstrlen(szExtensionName) + 1) * sizeof(TCHAR));
                   
    RegCloseKey (hKey);


    //
    // registers object, typelib and all interfaces in typelib
    //
    return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    TCHAR szSubKey[400];
    LONG lResult;
    HKEY hKey;
    TCHAR szGptEditorKey[50];

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szExtensionKey, szExtNodeType);
    RegDeleteKey (HKEY_LOCAL_MACHINE, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes"), szExtensionKey);
    RegDeleteKey (HKEY_LOCAL_MACHINE, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szExtensionKey);
    RegDeleteKey (HKEY_LOCAL_MACHINE, szSubKey);


    StringFromGUID2 (NODEID_User, szGptEditorKey, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGptEditorKey, szNodeType);
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_WRITE, &hKey);

    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szExtensionKey);
        RegCloseKey (hKey);
    }


    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szNodeType);
    RegDeleteKey (HKEY_LOCAL_MACHINE, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions"), szNodeType);
    RegDeleteKey (HKEY_LOCAL_MACHINE, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szNodeType);
    RegDeleteKey (HKEY_LOCAL_MACHINE, szSubKey);

    _Module.UnregisterServer();
    return S_OK;
}
