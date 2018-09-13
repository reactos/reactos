#include "provpch.h"
#include "resource.h"
#include "vrsscan.h"
#include "vrsprov.h"
#include "util.h"
#include "provmn.h"
#include "provfct.h"
// virumn.cpp -- autmatic registration and unregistration
//

LONG              g_cDllRef = 0;      // per-instance
CRITICAL_SECTION 	g_cs = {0};		     // per-instance
HINSTANCE		   g_hinst = NULL;     // per-instance

OBJECTINFO g_ObjectInfo[] =
{
   { NULL, &CLSID_VirusScanner1, NULL, OI_COCREATEABLE, "VirusProvider1", MAKEINTRESOURCE(IDS_PROVIDER1), 
         NULL, NULL, VERSION_0, 0, 0 },
   { NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 0, 0 },
} ;


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if((riid == IID_IClassFactory) || (riid == IID_IUnknown))
    {
        // Search thru our object array fo this type
       
        const OBJECTINFO *pcls;
        for (pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if(rclsid == *pcls->pclsid)
            {
                *ppv = pcls->cf;
                ((IUnknown *)*ppv)->AddRef();
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
    if (g_cDllRef)
        return S_FALSE;

    return S_OK;
}

// Initialize OBJECT array with each objects class factory.

void InitClassFactories()
{
   pcf = new CProvider1Factory();
   g_ObjectInfo[0].cf = (void *)pcf;
}


void DeleteClassFactories()
{
   delete pcf;
}



STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, void *lpReserved)
{
   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
         g_hinst = (HINSTANCE)hDll;
         g_hHeap = GetProcessHeap();
         InitializeCriticalSection(&g_cs);
         DisableThreadLibraryCalls(g_hinst);
         InitClassFactories();
         break;

      case DLL_PROCESS_DETACH:
         DeleteCriticalSection(&g_cs);
         DeleteClassFactories();
         break;
   
      default:
         break;
   }
   return TRUE;
}

void DllAddRef(void)
{
    InterlockedIncrement(&g_cDllRef);
}

void DllRelease(void)
{
    InterlockedDecrement(&g_cDllRef);
}



//
// UnregisterObject removes all registration stuff for this object
// Note: it blows away all other registry entries below ours -- likely not a problem.
//

BOOL UnregisterObject(const OBJECTINFO *poi)
{
    TCHAR szScratch[MAX_PATH];
    HKEY hk;

    StringFromGuid(poi->pclsid, szScratch);
    RegOpenK(HKEY_CLASSES_ROOT, TEXT("CLSID"), &hk);
    DeleteKeyAndSubKeys(hk, szScratch);
    RegCloseKey(hk);

    if (poi->nObjectType != OI_COCREATEABLE)
    {
        DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, poi->pszName);

        wsprintf(szScratch, "%s.%d", poi->pszName, poi->lVersion);
        DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);
    }

    return TRUE;
}

//
// RegisterObject registers OI_COCREATEABLE, OI_AUTOMATION, OI_CONTROL
//
BOOL RegisterObject(const OBJECTINFO *poi)
{
    BOOL bRet = FALSE;
    TCHAR szGuidStr[GUID_STR_LEN];
    TCHAR szScratch[MAX_PATH];
    TCHAR szFriendlyName[50];
    LPTSTR pszFriendlyName = szFriendlyName;
    TCHAR szPath[MAX_PATH];
    DWORD dwDummy;

    HKEY  hkCLSID = NULL;
    HKEY  hkNAME = NULL;
    HKEY  hkCURVER = NULL;

    HKEY  hkSub = NULL;
    HKEY  hkSub2 = NULL;

    // set up fixed strings
    StringFromGuid(poi->pclsid, szGuidStr);
    
    if (HIWORD(poi->pszFriendlyName))
        pszFriendlyName = poi->pszFriendlyName;
    else
        if (!LoadString(g_hinst, LOWORD(poi->pszFriendlyName), szFriendlyName, sizeof(szFriendlyName)))
            goto CleanUp;
    if (!GetModuleFileName(g_hinst, szPath, sizeof(szPath)))
        goto CleanUp;

    // clean out old info
    //
    UnregisterObject(poi);

    // OI_COCREATEABLE, OI_AUTOMATION, OI_CONTROL
    //
    // HKEY_CLASSES_ROOT\CLSID\<CLSID> = <friendly name>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32\ThreadingModel = Apartment
    //
    wsprintf(szScratch, TEXT("CLSID\\%s"), szGuidStr);
    RegCreate(HKEY_CLASSES_ROOT, szScratch, &hkCLSID);
    RegSetStr(hkCLSID, pszFriendlyName);

    RegCreate(hkCLSID, TEXT("InprocServer32"), &hkSub);
    RegSetStr(hkSub, szPath);

    RegSetStrValue(hkSub, TEXT("ThreadingModel"), TEXT("Apartment"));
    RegCloseK(hkSub);

    // Virus scanners are simple OI_COCREATEABLE com objects, but this is here for generality
    
    if (poi->nObjectType != OI_COCREATEABLE)
    {
        TCHAR szCurVer[MAX_PATH];

        wsprintf(szCurVer, "%s.%d", poi->pszName, poi->lVersion);

        // OI_AUTOMATION, OI_CONTROL
        //
        // HKEY_CLASSES_ROOT\<static name> = <friendly name>
        // HKEY_CLASSES_ROOT\<static name>\CLSID = <CLSID>
        // HKEY_CLASSES_ROOT\<static name>\CurVer = <static name>.<VersionNumber>
        //
        // HKEY_CLASSES_ROOT\<static name>.<VersionNumber> = <friendly name>
        // HKEY_CLASSES_ROOT\<static name>.<VersionNumber>\CLSID = <CLSID>
        //
        // HKEY_CLASSES_ROOT\CLSID\<CLSID>\TypeLib = <LibidOfTypeLibrary>
        // HKEY_CLASSES_ROOT\CLSID\<CLSID>\ProgID = <static name>.<VersionNumber>
        // HKEY_CLASSES_ROOT\CLSID\<CLSID>\VersionIndependentProgID = <static name>
        //
        RegCreate(HKEY_CLASSES_ROOT, poi->pszName, &hkNAME);
        RegSetStr(hkNAME, pszFriendlyName);

        RegCreate(hkNAME, TEXT("CLSID"), &hkSub);
        RegSetStr(hkSub, szGuidStr);
        RegCloseK(hkSub);

        RegCreate(hkNAME, TEXT("CurVer"), &hkSub);
        RegSetStr(hkSub, szCurVer);
        RegCloseK(hkSub);

        RegCreate(HKEY_CLASSES_ROOT, szCurVer, &hkCURVER);
        RegSetStr(hkCURVER, pszFriendlyName);

        RegCreate(hkCURVER, TEXT("CLSID"), &hkSub);
        RegSetStr(hkSub, szGuidStr);
        RegCloseK(hkSub);

        RegCreate(hkCLSID, TEXT("TypeLib"), &hkSub);
//        StringFromGuid(& YOUR LIBID HERE, szScratch);
        RegSetStr(hkSub, szScratch);
        RegCloseK(hkSub);

        RegCreate(hkCLSID, TEXT("ProgID"), &hkSub);
        RegSetStr(hkSub, szCurVer);
        RegCloseK(hkSub);

        RegCreate(hkCLSID, TEXT("VersionIndependentProgID"), &hkSub);
        RegSetStr(hkSub, poi->pszName);
        RegCloseK(hkSub);

        if (poi->nObjectType == OI_AUTOMATION)
        {
            // REVIEW: do we need these? Sweeper's OI_AUTOMATION does NOT set them
            //
            // HKEY_CLASSES_ROOT\CLSID\<CLSID>\Programmable
            //
            RegCreate(hkCLSID, TEXT("Programmable"), &hkSub);
            RegCloseK(hkSub);
        }

        if (poi->nObjectType == OI_CONTROL)
        {
            // HKEY_CLASSES_ROOT\CLSID\<CLSID>\Control
            // HKEY_CLASSES_ROOT\CLSID\<CLSID>\MiscStatus = 0
            // HKEY_CLASSES_ROOT\CLSID\<CLSID>\MiscStatus\1 = <MISCSTATUSBITS>
            // HKEY_CLASSES_ROOT\CLSID\<CLSID>\ToolboxBitmap32 = <PATH TO BMP>
            // HKEY_CLASSES_ROOT\CLSID\<CLSID>\Version = <VERSION>
            //
            RegCreate(hkCLSID, TEXT("Control"), &hkSub);
            RegCloseK(hkSub);

            RegCreate(hkCLSID, TEXT("MiscStatus"), &hkSub);
            RegSetStr(hkSub, TEXT("0"));

            RegCreate(hkSub, TEXT("1"), &hkSub2);
            wsprintf(szScratch, TEXT("%d"), poi->dwOleMiscFlags);
            RegSetStr(hkSub2, szScratch);
            RegCloseK(hkSub2);

            RegCloseK(hkSub);

            RegCreate(hkCLSID, TEXT("ToolboxBitmap32"), &hkSub);
            wsprintf(szScratch, TEXT("%s,%d"), szPath, poi->nidToolbarBitmap);
            RegSetStr(hkSub, szScratch);
            RegCloseK(hkSub);

            RegCreate(hkCLSID, TEXT("Version"), &hkSub);
            wsprintf(szScratch, TEXT("%ld.0"), poi->lVersion);
            RegSetStr(hkSub, szScratch);
            RegCloseK(hkSub);

        }

        RegCloseK(hkCURVER);
        RegCloseK(hkNAME);
    }

    RegCloseK(hkCLSID);

    return TRUE;

CleanUp:
    if (hkCLSID)  RegCloseKey(hkCLSID);
    if (hkNAME)   RegCloseKey(hkNAME);
    if (hkCURVER) RegCloseKey(hkCURVER);
    if (hkSub)    RegCloseKey(hkSub);
    if (hkSub2)   RegCloseKey(hkSub2);

    UnregisterObject(poi);

    return FALSE;
}


//=--------------------------------------------------------------------------=
// UnregisterTypeLibrary
//=--------------------------------------------------------------------------=
// blows away the type library keys for a given libid.
//
// Parameters:
//    REFCLSID        - [in] libid to blow away.
//
// Output:
//    BOOL            - TRUE OK, FALSE bad.
//
// Notes:
//    - WARNING: this function just blows away the entire type library section,
//      including all localized versions of the type library.  mildly anti-
//      social, but not killer.
//
BOOL UnregisterTypeLibrary
(
    const CLSID* piidLibrary
)
{
    TCHAR szScratch[GUID_STR_LEN];
    HKEY hk;
    BOOL f;

    if(piidLibrary == NULL)
       return TRUE;
    // convert the libid into a string.
    //
    StringFromGuid(piidLibrary, szScratch);
    RegOpenK(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk);

    f = DeleteKeyAndSubKeys(hk, szScratch);

    RegCloseKey(hk);
    return f;
}




STDAPI DllRegisterServer(void)
{
/*
    ITypeLib *pTypeLib;
    HRESULT hr;
    DWORD   dwPathLen;
    char    szTmp[MAX_PATH];
*/
    const OBJECTINFO *pcls;

    for (pcls = g_ObjectInfo; pcls->pclsid; pcls++)
    {
        if (pcls->nObjectType != OI_NONE)
        {
           RegisterObject(pcls);
        }
    }

    // Load and register our type library.
    //
    /*
    dwPathLen = GetModuleFileName(g_hinst, szTmp, sizeof(szTmp));
    MAKE_WIDEPTR_FROMANSI(pwsz, szTmp);
    hr = LoadTypeLib(pwsz, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        hr = RegisterTypeLib(pTypeLib, pwsz, NULL);
        pTypeLib->Release();
    } */
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    const OBJECTINFO *pcls;
    for (pcls = g_ObjectInfo; pcls->pclsid; pcls++)
    {
        if (pcls->nObjectType != OI_NONE)
        {
            UnregisterObject(pcls);
        }
    }

    UnregisterTypeLibrary(NULL);

    return S_OK;
}



