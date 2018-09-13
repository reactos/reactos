//--------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Rights Reserved.
//
// dll.cxx
//
// These are the DLL entry points and the the class factory for the 
// CRestrictedProcess object.
//
//--------------------------------------------------------------------

#define UNICODE

#define INITGUIDS
#include <windows.h>
#include <winsock2.h>
#include <malloc.h>
#include <olectl.h>
#include <crtdbg.h>
#include "dict.h"
#include "rprocess.h"
#include "dll.h"
#include "inst_lsp.h"

//--------------------------------------------------------------------
// The following functions are constructed by MIDL as part of the
// interface code and represent the interface proxy.
//--------------------------------------------------------------------

extern "C" HRESULT PrxyDllGetClassObject( REFCLSID rclsid, 
                                          REFIID   riid, 
                                          void   **ppv    );

extern "C" HRESULT PrxyDllRegisterServer();

extern "C" HRESULT PrxyDllUnregisterServer();

//--------------------------------------------------------------------
// Count of objects and locks:
//--------------------------------------------------------------------

static ULONG       g_cObj = 0;
static ULONG       g_cLock = 0;

static HINSTANCE   g_hInst = 0;

static const WCHAR* OBJ_GUID 
             = TEXT("{570DA105-3C30-11D1-8BF1-0000F8754035}");

static const WCHAR* APPID_GUID 
             = TEXT("{570DA105-3C30-11D1-8BF1-0000F8754035}");
//           = TEXT("{911D15AD-092B-11D1-9BF3-00A0C9063D92}");

static const WCHAR* CLSID_OBJ
             = TEXT("CLSID\\{570DA105-3C30-11D1-8BF1-0000F8754035}");

static const WCHAR* CLSID_PROXY
             = TEXT("CLSID\\{3F7EC550-80A3-11D1-B222-00A0C90C91FE}");

static const WCHAR* APPID_OBJ
             = TEXT("AppID\\{570DA105-3C30-11D1-8BF1-0000F8754035}");
//           = TEXT("AppID\\{911D15AD-092B-11D1-9BF3-00A0C9063D92}");

static const WCHAR* CLSID_INTF 
             = TEXT("CLSID\\{3AE0B7E0-3C19-11D1-8BF1-0000F8754035}");

static const WCHAR* INTF_INTF 
             = TEXT("Interface\\{3AE0B7E0-3C19-11D1-8BF1-0000F8754035}");

static const WCHAR* INTF_GUID 
             = TEXT("{3AE0B7E0-3C19-11D1-8BF1-0000F8754035}");

static const WCHAR* PROXY_GUID
             = TEXT("{3F7EC550-80A3-11D1-B222-00A0C90C91FE}");


static const unsigned char AccessPermission[] =
{ 0x01,0x00,0x04,0x80,0x70,0x00,0x00,0x00,
  0x8C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x14,0x00,0x00,0x00,0x02,0x00,0x5C,0x00,
  0x04,0x00,0x00,0x00,0x00,0x00,0x14,0x00,
  0x01,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
  0x00,0x00,0x00,0x05,0x04,0x00,0x00,0x00,
  0x00,0x00,0x14,0x00,0x01,0x00,0x00,0x00,
  0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,
  0x12,0x00,0x00,0x00,0x00,0x00,0x14,0x00,
  0x01,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
  0x00,0x00,0x00,0x05,0x0C,0x00,0x00,0x00,
  0x00,0x00,0x18,0x00,0x01,0x00,0x00,0x00,
  0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x05,
  0x20,0x00,0x00,0x00,0x20,0x02,0x00,0x00,
  0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x05,
  0x15,0x00,0x00,0x00,0xA0,0x65,0xCF,0x7E,
  0x78,0x4B,0x9B,0x5F,0xE7,0x7C,0x87,0x70,
  0x1E,0x93,0x00,0x00,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x15,0x00,0x00,0x00,
  0xA0,0x65,0xCF,0x7E,0x78,0x4B,0x9B,0x5F,
  0xE7,0x7C,0x87,0x70,0x1E,0x93,0x00,0x00
};

static const unsigned char LaunchPermission[] =
{ 0x01,0x00,0x04,0x80,0x78,0x00,0x00,0x00,  
  0x94,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x14,0x00,0x00,0x00,0x02,0x00,0x64,0x00,  
  0x04,0x00,0x00,0x00,0x00,0x00,0x14,0x00,
  0x01,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
  0x00,0x00,0x00,0x05,0x0C,0x00,0x00,0x00,
  0x00,0x00,0x18,0x00,0x01,0x00,0x00,0x00,
  0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x05,
  0x20,0x00,0x00,0x00,0x20,0x02,0x00,0x00,
  0x00,0x00,0x18,0x00,0x01,0x00,0x00,0x00,
  0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,
  0x04,0x00,0x00,0x00,0x20,0x02,0x00,0x00,
  0x00,0x00,0x18,0x00,0x01,0x00,0x00,0x00,
  0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,
  0x12,0x00,0x00,0x00,0x20,0x02,0x00,0x00,
  0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x05,
  0x15,0x00,0x00,0x00,0xA0,0x5F,0x84,0x1F,
  0x5E,0x2E,0x6B,0x49,0xCE,0x12,0x03,0x03,
  0xF4,0x01,0x00,0x00,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x15,0x00,0x00,0x00,
  0xA0,0x5F,0x84,0x1F,0x5E,0x2E,0x6B,0x49,
  0xCE,0x12,0x03,0x03,0xF4,0x01,0x00,0x00
};

#if FALSE

    ... DllMain() is defined in ..\msrlsp\dllmain.cpp

//--------------------------------------------------------------------
// DllMain()
//
//--------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst, 
                     ULONG     ulReason,
                     LPVOID    pvReserved )
    {
    BOOL  fInitialized = TRUE;

    switch (ulReason)
       {
       case DLL_PROCESS_ATTACH:
          if (!DisableThreadLibraryCalls(hInst))
             {
             fInitialized = FALSE;
             }
          else
             {
             g_hInst = hInst;
             }
          break;

       case DLL_PROCESS_DETACH:
          break;

       case DLL_THREAD_ATTACH:
          // Not used. Disabled.
          _ASSERT(0);
          break;

       case DLL_THREAD_DETACH:
          // Not used. Disabled.
          _ASSERT(0);
          break;
       }

    return fInitialized;
    }
#endif

//--------------------------------------------------------------------
//  DllGetClassObject()
//
//  Provides an IClassFactory for a given CLSID that this DLL is
//  registered to support.  This DLL is placed under the CLSID
//  in the registration database as the InProcServer.
//
//  clsID           REFCLSID that identifies the class factory
//                  desired.  Since this parameter is passed this
//                  DLL can handle any number of objects simply
//                  by returning different class factories here
//                  for different CLSIDs.
//
//  riid            REFIID specifying the interface the caller wants
//                  on the class object, usually IID_ClassFactory.
//
//  ppv             PPVOID in which to return the interface
//                  pointer.
//
// Return Value:
//
//  HRESULT         NOERROR on success, otherwise an error code.
//
//--------------------------------------------------------------------
HRESULT DllGetClassObject( REFCLSID rclsid, 
                           REFIID   riid, 
                           void   **ppv    )
    {
    HRESULT   hr;
    CRestrictedProcessClassFactory *pObj;

    // Test to see if the Class Factory for the Proxy/Stub
    // is wanted.
    if (CLSID_RestrictedProcessProxy == rclsid)
       {
       return PrxyDllGetClassObject(rclsid,riid,ppv);
       }

    // The only choice left is the CLSID for the Restricted
    // Networking object.
    if (CLSID_RestrictedProcess != rclsid)
       {
       return CLASS_E_CLASSNOTAVAILABLE;
       }

    pObj = new CRestrictedProcessClassFactory();
    if (!pObj)
       {
       return E_OUTOFMEMORY;
       }

    hr = pObj->QueryInterface(riid,ppv);

    if (FAILED(hr))
       {
       delete pObj;
       }
    else
       {
       g_cObj++;
       }

    return hr;
    }

//--------------------------------------------------------------------
// DllCanUnloadNow()
//
// Indicated whether the DLL is no longer in use and should be unloaded
// from memory.
//
// Return Value:
//
//   S_OK      DLL can be unloaded now.
//
//   S_FALSE   DLL is still in use, don't unload.
//
//--------------------------------------------------------------------
HRESULT DllCanUnloadNow(void)
    {
    HRESULT hr;

    //Our answer is whether there are any object or locks
    hr = ((0==g_cObj) && (0L==g_cLock))? S_OK : S_FALSE;

    return hr;
    }

//-------------------------------------------------------------------
// SetKeyAndValue()
//
// Private helper function for DllRegisterServer() that creates
// a key, sets a value, then closes the key.
//
//  Parameters:
//    pszKey       WCHAR* The ame of the key
//    pszSubkey    WCHAR* The name of a subkey
//    pszValueName WCHAR* The value name.
//    pszValue     WCHAR* The data value to store
//
//  Return:
//    BOOL         TRUE if successful, FALSE otherwise.
//
//-------------------------------------------------------------------
static BOOL SetKeyAndValue( const WCHAR *pwsKey, 
                            const WCHAR *pwsSubKey,
                            const WCHAR *pwsValueName,
                            const WCHAR *pwsValue,
                            const DWORD  dwType = REG_SZ,
                                  DWORD  dwDataSize = 0 )
    {
    HKEY   hKey;
    DWORD  dwSize = 0;
    WCHAR  *pwsCompleteKey;

    if (pwsKey)
        dwSize = wcslen(pwsKey);

    if (pwsSubKey)
        dwSize += wcslen(pwsSubKey);

    _ASSERTE(dwSize > 0);

    dwSize = (1+1+dwSize)*sizeof(WCHAR);  // Extra +1 for the backslash...
    pwsCompleteKey = (WCHAR*)_alloca(dwSize);
    if (!pwsCompleteKey)
        {
        return FALSE;   // Out of stack memory.
        }

    wcscpy(pwsCompleteKey,pwsKey);

    if (NULL!=pwsSubKey)
        {
        wcscat(pwsCompleteKey, TEXT("\\"));
        wcscat(pwsCompleteKey, pwsSubKey);
        }

    if (ERROR_SUCCESS!=RegCreateKeyEx( HKEY_CLASSES_ROOT,
                                       pwsCompleteKey, 
                                       0, 
                                       NULL, 
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_ALL_ACCESS, NULL, &hKey, NULL))
        {
        return FALSE;
        }

    if (pwsValue)
        {
        if ((dwType == REG_SZ)||(dwType == REG_EXPAND_SZ))
          dwDataSize = (1+wcslen(pwsValue))*sizeof(WCHAR);

        _ASSERT(dwDataSize);

        RegSetValueEx( hKey, 
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, dwDataSize );
        }
    else
        {
        RegSetValueEx( hKey, 
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, 0 );
        }

    RegCloseKey(hKey);
    return TRUE;
    }


//--------------------------------------------------------------------
// DllUnregisterServer()
//
// Remove the registry entries created through DllRegisterServer().
//
// Return Value:
//
//   S_OK              Registry entries were deleted successfully.
//
//   SELFREG_E_TYPELIB
//   SELFREG_E_CLASS
//   E_OUTOFMEMORY
//   E_UNEXPECTED
//
//--------------------------------------------------------------------
HRESULT DllUnregisterServer()
    {
    DWORD  dwStatus;
    BOOL   fIsInstalled;
    WCHAR  wszTemp[256];


    //
    // Delete entries in \AppID
    //
    // NOTE: Don't delete this entry, since it is shared by urlmon.dll
    //
    // RegDeleteKey(HKEY_CLASSES_ROOT, APPID_OBJ );

    //
    // Delete Version Independent ProgID entries
    //
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("RestrictedProcess\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("RestrictedProcess\\CurVer"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("RestrictedProcess"));

    //
    // Delete ProgID entries:
    //
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("RestrictedProcess1.0\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("RestrictedProcess1.0"));

    //
    // Delete entries in \CLSID\{570da105-3c30-11d1-8bf1-0000f8754035}
    //
    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_OBJ, TEXT("InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_OBJ, TEXT("NotInsertable"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_OBJ, TEXT("ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_OBJ, TEXT("ThreadingModel"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_OBJ, TEXT("VersionIndependentProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    RegDeleteKey(HKEY_CLASSES_ROOT, CLSID_OBJ);

    //
    // Delete entries in \CLSID\{3F7EC550-80A3-11D1-B222-00A0C90C91FE}
    //
    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_PROXY, TEXT("InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_PROXY, TEXT("ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_PROXY, TEXT("ThreadingModel"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_PROXY, TEXT("VersionIndependentProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    RegDeleteKey(HKEY_CLASSES_ROOT, CLSID_PROXY);

    //
    // Delete entries in \CLSID\{3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
    //
    wsprintf(wszTemp, TEXT("%s\\%s"), CLSID_INTF, TEXT("InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    RegDeleteKey(HKEY_CLASSES_ROOT, CLSID_INTF);

    //
    // Delete entries in \Interface:
    //
    wsprintf(wszTemp, TEXT("%s\\%s"), INTF_INTF, TEXT("NumMethods"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    wsprintf(wszTemp, TEXT("%s\\%s"), INTF_INTF, TEXT("ProxyStubClassid32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, wszTemp);

    RegDeleteKey(HKEY_CLASSES_ROOT, INTF_INTF);

    //
    // Delete the interface proxy:
    //
    // PrxyDllUnregisterServer(); currently done manually (above...)


    // This is the registration code used to uninstall a LSP from
    // WinSock2:
    dwStatus = LSPAlreadyInstalled(&fIsInstalled);
    if (fIsInstalled)
        {
        UninstallLSP();
        }

    return S_OK;
    }

//--------------------------------------------------------------------
// DllRegisterServer()
//
// Create registry entries for all classes supported in this DLL.
//
// Return Value:
//
//   S_OK              Registry entries created successfully.
//
//   SELFREG_E_TYPELIB
//   SELFREG_E_CLASS
//   E_OUTOFMEMORY
//   E_UNEXPECTED
//
//   HKEY_CLASSES_ROOT
//
//     \AppID
//        \{911d15ad-092b-11d1-9bf3-00a0c9063d92}
//            DllSurrogate:REG_SZ:--
//            AccessPermission:REG_BINARY:<binary-const-at-top-of-this-file>
//            LaunchPermission:REG_BINARY:<binary-const-at-top-of-this-file>
//            RunAs:REG_SZ:Interactive User
//
//     \RestrictedProcess
//        --:REG_SZ:Restricted Process WinSock2 Helper
//        \CLSID
//           --:REG_SZ:{570da105-3c30-11d1-8bf1-0000f8754035}
//        \CurVer
//           --:REG_SZ:RestrictedProcess1.0
//
//     \RestrictedProcess1.0
//        --:REG_SZ:Restricted Process WinSock2 Helper
//        \CLSID
//           --:REG_SZ:{570da105-3c30-11d1-8bf1-0000f8754035}
//
//     \CLSID
//        \{570da105-3c30-11d1-8bf1-0000f8754035}
//           --:REG_SZ:Restricted Process WinSock2 Helper
//           AppID:REG_SZ:{911d15ad-092b-11d1-9bf3-00a0c9063d92}
//           \InprocServer32  
//              --:REG_SZ:msrlsp.dll
//           \NoInsertable
//              --:REG_SZ:--
//           \ProgID
//              --:REG_SZ:RestrictedProcess1.0
//           \ThreadingModel
//              --:REG_SZ:Both
//           \VersionIndependentProgID
//              --:REG_SZ:RestrictedProcess
//        \{3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
//           --:REG_SZ:IRestrictedProcess Proxy/Stub Factory
//           \InprocServer32
//              --:REG_EXPAND_SZ:%SystemRoot%\system32\msrlsp.dll
//
//     \Interface
//        \{3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
//           --:REG_SZ:IRestrictedProcess
//           \NumMethods
//              --:REG_SZ:14
//           \ProxyStubClsid32
//              --:REG_SZ:{3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
//
//--------------------------------------------------------------------
HRESULT DllRegisterServer()
    {
    // Disable LSP, remove entries from registry and list of Winsock2
    // providers:
    HRESULT hr = DllUnregisterServer();

    return hr;

    #if FALSE
    DWORD  dwStatus;
    BOOL   fIsInstalled;
    // WCHAR  wszModule[256];

    //
    // Name of this DLL:
    //
    #if FALSE
    if (!GetModuleFileName( g_hInst, wszModule,
                            sizeof(wszModule)/sizeof(WCHAR)))
       {
       return SELFREG_E_CLASS;
       }
    #endif

    //
    // Create \HKEY_CLASSES_ROOT\AppID entries:
    //
    if (  !SetKeyAndValue( APPID_OBJ,
                           NULL, 
                           NULL, 
                           TEXT("Restricted Process WinSock2 Helper"))
       || !SetKeyAndValue( APPID_OBJ,NULL,TEXT("DllSurrogate"),NULL)
       || !SetKeyAndValue( APPID_OBJ,NULL,TEXT("AccessPermission"),(WCHAR*)AccessPermission,REG_BINARY,sizeof(AccessPermission))
       || !SetKeyAndValue( APPID_OBJ,NULL,TEXT("LaunchPermission"),(WCHAR*)LaunchPermission,REG_BINARY,sizeof(LaunchPermission))
       || !SetKeyAndValue( APPID_OBJ,NULL,TEXT("RunAs"),TEXT("Interactive User")) )
       {
       return SELFREG_E_CLASS;
       }

    //
    // Create VersionIndependentProgID keys:
    //
    if (  !SetKeyAndValue( TEXT("RestrictedProcess"), 
                            NULL, 
                            NULL, 
                            TEXT("Restricted Process WinSock2 Helper"))
       || !SetKeyAndValue( TEXT("RestrictedProcess"), 
                            TEXT("CLSID"), 
                            NULL, 
                            OBJ_GUID)
       || !SetKeyAndValue( TEXT("RestrictedProcess"), 
                            TEXT("CurVer"), 
                            NULL, 
                            TEXT("RestrictedProcess1.0")) )
       {
       return SELFREG_E_CLASS;
       }

    //
    // Create ProgID keys:
    //
    if (  !SetKeyAndValue( TEXT("RestrictedProcess1.0"), 
                           NULL, 
                           NULL, 
                           TEXT("Restricted Process WinSock2 Helper"))
       || !SetKeyAndValue( TEXT("RestrictedProcess1.0"), 
                           TEXT("CLSID"), 
                           NULL, 
                           OBJ_GUID) )
       {
       return SELFREG_E_CLASS;
       }

    //
    // Create entries under \HKEY_CLASSES_ROOT\CLSID:
    //
    if (  !SetKeyAndValue(CLSID_OBJ, NULL, NULL, TEXT("RestrictedProcess WinSock2 Helper"))
       || !SetKeyAndValue(CLSID_OBJ, NULL, TEXT("AppID"), APPID_GUID)
       || !SetKeyAndValue(CLSID_OBJ, TEXT("InprocServer32"), NULL, TEXT("%SystemRoot%\\system32\\msrlsp.dll"), REG_EXPAND_SZ)
       || !SetKeyAndValue(CLSID_OBJ, TEXT("NotInsertable"), NULL, NULL)
       || !SetKeyAndValue(CLSID_OBJ, TEXT("ProgID"), NULL, TEXT("RestrictedProcess1.0"))
       || !SetKeyAndValue(CLSID_OBJ, TEXT("ThreadingModel"), NULL, TEXT("Both"))
       || !SetKeyAndValue(CLSID_OBJ, TEXT("VersionIndependentProgID"), NULL, TEXT("RestrictedProcess")) )
       {
       return SELFREG_E_CLASS;
       }

    if (  !SetKeyAndValue( CLSID_INTF, 
                           NULL, 
                           NULL, 
                           TEXT("IRestrictedProcess Proxy/Stub Factory"))
       || !SetKeyAndValue( CLSID_INTF, 
                           TEXT("InprocServer32"), 
                           NULL, 
                           TEXT("%SystemRoot%\\system32\\msrlsp.dll"),
                           REG_EXPAND_SZ) )
       {
       return SELFREG_E_CLASS;
       }

    //
    // Create the entries under \HKEY_CLASSES_ROOT\Interface
    //
    if (  !SetKeyAndValue(INTF_INTF, NULL, NULL, TEXT("IRestrictedProcess"))
       || !SetKeyAndValue(INTF_INTF, TEXT("ProxyStubClsid32"), NULL, PROXY_GUID)
       || !SetKeyAndValue(INTF_INTF, TEXT("NumMethods"), NULL, TEXT("14")) )
       {
       // !SetKeyAndValue(INTF_INTF, TEXT("ProxyStubClsid32"), NULL, INTF_GUID)
       return SELFREG_E_CLASS;
       }

    //
    // These entries are for the Proxy CLSID (created from the interface IDL:
    //
    if (  !SetKeyAndValue(CLSID_PROXY, NULL, NULL, TEXT("RestrictedProcess WinSock2 Helper Proxy"))
       || !SetKeyAndValue(CLSID_PROXY, NULL, TEXT("AppID"), APPID_GUID)
       || !SetKeyAndValue(CLSID_PROXY, TEXT("InprocServer32"), NULL, TEXT("%SystemRoot%\\system32\\msrlsp.dll"), REG_EXPAND_SZ)
       || !SetKeyAndValue(CLSID_PROXY, TEXT("ProgID"), NULL, TEXT("RestrictedProcess1.0"))
       || !SetKeyAndValue(CLSID_PROXY, TEXT("ThreadingModel"), NULL, TEXT("Both"))
       || !SetKeyAndValue(CLSID_PROXY, TEXT("VersionIndependentProgID"), NULL, TEXT("RestrictedProcess")) )
       {
       return SELFREG_E_CLASS;
       }

    // return PrxyDllRegisterServer();    not currently used...


    // Check to see if the LSP (msrlsp.dll) is registered with
    // WinSock2, if not the register it:
    dwStatus = LSPAlreadyInstalled(&fIsInstalled);
    if (!fIsInstalled)
        {
        InstallLSP();
        }

    return S_OK;

    #endif
    }

//--------------------------------------------------------------------
// ObjectDestroyed
//
// Function for the RestrictedProcess object to call when it gets destroyed.
//
//--------------------------------------------------------------------
void ObjectDestroyed(void)
    {
    if (g_cObj)
       {
       g_cObj--;
       }

    return;
    }

//------------------------------------------------------
//  CRestrictedProcessClassFactor::CRestrictedProcessClassFactory()
//
//  Class Factory Constructor.
//------------------------------------------------------
CRestrictedProcessClassFactory::CRestrictedProcessClassFactory(void)
    {
    m_cRef=0L;
    return;
    }

//------------------------------------------------------
//  CRestrictedProcessClassFactory::~CRestrictedProcessClassFactory()
//
//  Class Factory Destructor.
//------------------------------------------------------
CRestrictedProcessClassFactory::~CRestrictedProcessClassFactory(void)
    {
    return;
    }

//------------------------------------------------------
//  CRestrictedProcessClassFactory::QueryInterface()
//
//------------------------------------------------------
STDMETHODIMP CRestrictedProcessClassFactory::QueryInterface(
                                    REFIID riid,
                                    PPVOID ppv  )
    {
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


//------------------------------------------------------
//  CRestrictedProcessClassFactory::AddRef()
//
//------------------------------------------------------
STDMETHODIMP_(ULONG) CRestrictedProcessClassFactory::AddRef(void)
    {
    return ++m_cRef;
    }


//------------------------------------------------------
//  CRestrictedProcessClassFactory::Release()
//
//------------------------------------------------------
STDMETHODIMP_(ULONG) CRestrictedProcessClassFactory::Release(void)
    {
    if (0L!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
    }


//------------------------------------------------------
//  CRestrictedProcessClassFactory::CreateInstance()
//
//  Instantiates an RestrictedProcess object, returns an interface pointer
//
//  Parameters:
//
//    pUnkOuter     LPUNKNOWN to a controlling IUnknown for
//                  aggregation.
//    riid          REFIID to identify the interface that the
//                  caller desires for the new object.
//    ppvObj        PPVOID returns the interface pointer for
//                  the new object.
//
//  Returns:
//
//    HRESULT       NOERROR on success, otherwise E_NOINTERFACE.
//
//------------------------------------------------------
STDMETHODIMP CRestrictedProcessClassFactory::CreateInstance(
                                     LPUNKNOWN   pUnkOuter,
                                     REFIID      riid, 
                                     void      **ppvObj )
    {
    PCRestrictedProcess             pObj;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=ResultFromScode(E_OUTOFMEMORY);

    //Verify that a controlling unknown asks for IUnknown
    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    //Create the object telling us to notify us when it's gone.
    pObj=new CRestrictedProcess(pUnkOuter, ObjectDestroyed);

    if (NULL==pObj)
        {
        //This starts shutdown if there are no other objects.
        g_cObj++;
        ObjectDestroyed();
        return hr;
        }

    if (pObj->Init())
        hr=pObj->QueryInterface(riid, ppvObj);

    g_cObj++;

    /*
     * Kill the object if initial creation or Init failed. If
     * the object failed, we handle the g_cObj increment above
     * in ObjectDestroyed.
     */
    if (FAILED(hr))
        {
        delete pObj;
        ObjectDestroyed();  //Handle shutdown cases.
        }

    return hr;
    }


//--------------------------------------------------------------
//  CRestrictedProcessClassFactory::LockServer()
//
//  Increments/decrements the lock count for the IClassFactory
//  object. When the number of locks is decremented to zero, and
//  the number of objects equals zero, shut down the application.
//
//  Parameters:
//
//    fLock        BOOL, TRUE is increment the lock count, FALSE
//                 decrements the lock count.
//
//  Return:
//
//    HRESULT      Always returns NOERROR.
//
//--------------------------------------------------------------
STDMETHODIMP CRestrictedProcessClassFactory::LockServer(BOOL fLock)
    {
    if (fLock)
        g_cLock++;
    else
        {
        g_cLock--;

        /*
         * Fake an object destruction:  this centralizes
         * all the shutdown code in the ObjectDestroyed
         * function, eliminating duplicate code here.
         */
        g_cObj++;
        ObjectDestroyed();
        }

    return NOERROR;
    }

