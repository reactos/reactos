/*
 * init.c - DLL startup routines module.
 */

/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "shlwapi.h"
#include "shlobj.h"

#include "init.h"
#include "clsfact.h"
#include "inetcpl.h"
#include "refcount.hpp"
#include "shlstock.h"
#include "olestock.h"
#include "ftps.hpp"     /* for CLSID_MIMEFileTypesPropSheetHook */
#include "inetps.hpp"   /* for CLSID_Internet */
#include "shguidp.h"    // for CLSID_URLExecHook
#include "cfmacros.h"   // static class factory macros

#define MLUI_INIT
#include <mluisupp.h>

/****************************** Public Functions *****************************/

/* Declare _main() so we can link with the CRT lib, but not have to
** use the DllMainCRTStartup entry point.
*/
void
_cdecl
main(void)
    {
    }


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

#ifdef MAINWIN
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, PVOID pvReserved);

extern "C" BOOL url_DllMain(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    )
{
  return DllMain(DllHandle,Reason,Reserved);
}

#endif

/*
** DllMain()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason,
                                PVOID pvReserved)
{
   BOOL bResult;

   DebugEntry(DllMain);

   /* Validate dwReason below. */
   /* pvReserved may be any value. */

   ASSERT(IS_VALID_HANDLE(hModule, MODULE));

   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
         MLLoadResources(hModule, TEXT("urllc.dll"));
         bResult = AttachProcess(hModule);
         break;

      case DLL_PROCESS_DETACH:
         MLFreeResources(hModule);
         bResult = DetachProcess(hModule);
         break;

      case DLL_THREAD_ATTACH:
         bResult = AttachThread(hModule);
         break;

      case DLL_THREAD_DETACH:
         bResult = DetachThread(hModule);
         break;

      default:
         ERROR_OUT(("LibMain() called with unrecognized dwReason %lu.",
                    dwReason));
         bResult = FALSE;
         break;
   }

   DebugExitBOOL(DllMain, bResult);

   return(bResult);
}


UINT 
WhichPlatform(void)
{
    HINSTANCE hinst;

    //
    // in retail we cache this info
    // in debug we always re-fetch it (so people can switch)
    //
#ifdef DEBUG
    UINT uInstall = PLATFORM_UNKNOWN;
#else
    static UINT uInstall = PLATFORM_UNKNOWN;

    if (uInstall != PLATFORM_UNKNOWN)
        return uInstall;
#endif

    hinst = GetModuleHandle(TEXT("SHDOCVW.DLL"));
    if (hinst)
    {
        // NOTE: GetProcAddress always takes ANSI strings!
        DLLGETVERSIONPROC pfnGetVersion =
            (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");

        if (pfnGetVersion)
        {
            DLLVERSIONINFO info;

            info.cbSize = sizeof(info);
            if (SUCCEEDED(pfnGetVersion(&info)))
            {
                if (4 <= info.dwMajorVersion &&
                    71 <= info.dwMinorVersion &&
                    429 <= info.dwBuildNumber)
                {
                    uInstall = PLATFORM_INTEGRATED;
                }
                else
                    uInstall = PLATFORM_IE3;
            }
        }

#ifdef DEBUG
        // To allow easier debugging, we can override the platform by 
        // setting InstallPlatform value in ccshell.ini.
        {
            UINT uInstallT = GetPrivateProfileInt(TEXT("urldebugoptions"),
                                                  TEXT("InstallPlatform"),
                                                  PLATFORM_UNKNOWN, 
                                                  TEXT("ohare.ini"));
            if (PLATFORM_UNKNOWN != uInstallT)
            {
                TRACE_OUT(("  ***Overriding real platform installation***\r\n"));
                uInstall = uInstallT;
            }
        }        

        switch (uInstall)
        {
        case PLATFORM_IE3:
            TRACE_OUT(("  ***Assuming IE3***\r\n"));
            break;

        case PLATFORM_INTEGRATED:
            TRACE_OUT(("  ***Assuming Nashville***\r\n"));
            break;
        }
#endif
    }

    return uInstall;
}


#pragma data_seg(DATA_SEG_PER_INSTANCE)

// DLL reference count == number of class factories +
//                        number of URLs +
//                        LockServer() count

PRIVATE_DATA ULONG s_ulcDLLRef   = 0;

#pragma data_seg()


PUBLIC_CODE ULONG DLLAddRef(void)
{
   ULONG ulcRef;

   ASSERT(s_ulcDLLRef < ULONG_MAX);

   ulcRef = ++s_ulcDLLRef;

   TRACE_OUT(("DLLAddRef(): DLL reference count is now %lu.",
              ulcRef));

   return(ulcRef);
}


PUBLIC_CODE ULONG DLLRelease(void)
{
   ULONG ulcRef;

   if (EVAL(s_ulcDLLRef > 0))
      s_ulcDLLRef--;

   ulcRef = s_ulcDLLRef;

   TRACE_OUT(("DLLRelease(): DLL reference count is now %lu.",
              ulcRef));

   return(ulcRef);
}


PUBLIC_CODE PULONG GetDLLRefCountPtr(void)
{
   // BUGBUG: this is dangerous.  Shouldn't be used like this.

   return(&s_ulcDLLRef);
}


typedef HRESULT (CALLBACK* DLLGETCLASSOBJECTPROC)(REFCLSID, REFIID, void**);

STDMETHODIMP_(BOOL)
PatchForNashville(
    REFCLSID rclsid, 
    REFIID riid, 
    void **ppv,
    HRESULT * phres)
{
    BOOL bRet = FALSE;

    *phres = CLASS_E_CLASSNOTAVAILABLE;       // assume error

    if (IsEqualIID(rclsid, CLSID_InternetShortcut) &&
        PLATFORM_INTEGRATED == WhichPlatform())
    {
        HINSTANCE hinst;

        // Normally we can just patch the registry.  But there is a valid
        // case where url.dll is the InprocServer, and that is when the
        // user has chosen to uninstall IE 4.0 and we haven't restarted 
        // the machine yet.  In this case, we don't want to patch the
        // registry.  Use the "MayChangeDefaultMenu" as an indication
        // of whether we should really patch it or not.

        // Are we uninstalling IE 4.0?
        if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, 
                                   TEXT("CLSID\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}\\shellex\\MayChangeDefaultMenu"),
                                   TEXT(""),
                                   NULL, NULL, NULL))
        {
            // No; patch the registry so shdocvw is the handler again
            SetRegKeyValue(HKEY_CLASSES_ROOT, 
                           "CLSID\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}\\InProcServer32",
                           NULL,
                           REG_SZ,
                           (PCBYTE)"shdocvw.dll", sizeof("shdocvw.dll"));

            // Now call shdocvw's DllGetClassObject
            hinst = GetModuleHandle(TEXT("SHDOCVW.DLL"));
            if (hinst)
            {
                DLLGETCLASSOBJECTPROC pfn = 
                    (DLLGETCLASSOBJECTPROC)GetProcAddress(hinst, "DllGetClassObject");

                if (pfn)
                    {
                    *phres = pfn(rclsid, riid, ppv);
                    bRet = TRUE;
                    }
            }
        }
    }
    return bRet;
}


STDAPI CreateInstance_Intshcut(IUnknown *punkOuter, REFIID riid, void **ppvOut);
STDAPI CreateInstance_MIMEHook(IUnknown *punkOuter, REFIID riid, void **ppvOut);
STDAPI CreateInstance_Internet(IUnknown *punkOuter, REFIID riid, void **ppvOut);
STDAPI CreateInstance_URLExec(IUnknown *punkOuter, REFIID riid, void **ppvOut);

//
// ClassFactory methods.
// 

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        DLLAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return DLLAddRef();
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    return DLLRelease();
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    CObjectInfo *localthis = (CObjectInfo*)(this);
    return localthis->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DLLAddRef();
    else
        DLLRelease();
    return S_OK;
}

//
// we always do a linear search here so put your most often used things first
//

CF_TABLE_BEGIN(c_clsmap)

    CF_TABLE_ENTRY(&CLSID_URLExecHook, CreateInstance_URLExec) 
    CF_TABLE_ENTRY(&CLSID_InternetShortcut, CreateInstance_Intshcut)
    CF_TABLE_ENTRY(&CLSID_MIMEFileTypesPropSheetHook, CreateInstance_MIMEHook)
    CF_TABLE_ENTRY(&CLSID_Internet, CreateInstance_Internet)

CF_TABLE_END(c_clsmap)

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hres = CLASS_E_CLASSNOTAVAILABLE;
    
    *ppv = NULL;        // assume error

    if (IsEqualIID(riid, IID_IClassFactory))
    {
        // Under Nashville, the internet shortcuts are handled by shdocvw.
        // It is possible that the user installed Netscape after installing
        // Nashville, which would cause url.dll to be the handler again
        // for internet shortcuts.  This patches the registry and calls
        // shdocvw's DllGetClassObject if we're in that scenario.

        // Did we patch for nashville?
        if ( !PatchForNashville(rclsid, riid, ppv, &hres) )
        {
            // No; carry on...
            const CObjectInfo *pcls;
            for (pcls = c_clsmap; pcls->pclsid; pcls++)
            {
                if (IsEqualIID(rclsid, *(pcls->pclsid)))
                {
                    *ppv = (void *)GET_ICLASSFACTORY(pcls);
                    DLLAddRef();        // creation of the CF, CF holds DLL Ref count
                    return NOERROR;
                }
            }
        }
    }

    return hres;
}

STDAPI DllCanUnloadNow(void)
{
    if (s_ulcDLLRef > 0)
        return S_FALSE;

    return InternetCPLCanUnloadNow();
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */
