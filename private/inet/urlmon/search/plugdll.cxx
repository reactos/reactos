//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       plugdll.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <stdio.h>
#include <sem.hxx>
#include "urlcf.hxx"
#include "selfreg.hxx"

#define SZNAMESPACEROOT "PROTOCOLS\\Name-Space Handler\\"
#define SZPROTOCOLROOT  "PROTOCOLS\\Handler\\"
#define SZCLASS         "CLSID"


// ==========================================================================================================
//
// THIS IS OUR CLSID
//
GUID CLSID_ResProtocol =     {0x79eaca01, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};
//
// ==========================================================================================================


DECLARE_INFOLEVEL(UrlMk)
DECLARE_INFOLEVEL(Trans)
HINSTANCE g_hInst = NULL;

// global variables
CRefCount g_cRef(0);        // global dll refcount


#define DLL_NAME      "b4hook.dll"

STDAPI_(BOOL) TlsDllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved);


#define HANDLER_HOOK                           SZNAMESPACEROOT"Search Hook"
#define HANDLER_RES                            SZPROTOCOLROOT"search"
#define PROTOCOL_RES_CLSID                     "{79eaca01-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_RES_CLSID_REGKEY              "CLSID\\"PROTOCOL_RES_CLSID
#define PROTOCOL_HOOK_DESCRIP                  "Search Hook: Asychronous Name-Space Handler"
#define PROTOCOL_RES_DESCRIP                   "search: Asychronous Pluggable Protocol Handler"

#define HANDLER_PROTOCOLS                       HANDLER_HOOK"\\Protocols"

// protocols
//***** PROTOCOL_RES ENTRIES *****
const REGENTRY rgClassesRes[] =
{
    STD_ENTRY(PROTOCOL_RES_CLSID_REGKEY, PROTOCOL_RES_DESCRIP),
    STD_ENTRY(PROTOCOL_RES_CLSID_REGKEY"\\InprocServer32", "%s"DLL_NAME),
        { KEYTYPE_STRING, PROTOCOL_RES_CLSID_REGKEY"\\InprocServer32", "ThreadingModel", REG_SZ, (BYTE*)"Apartment" },
};


const REGENTRY rgHandlerRes   [] = 
{ 
    STD_ENTRY(HANDLER_RES  , PROTOCOL_RES_DESCRIP  ), 
        { KEYTYPE_STRING, HANDLER_RES  , "CLSID", REG_SZ, (BYTE*)PROTOCOL_RES_CLSID   },
    STD_ENTRY(HANDLER_HOOK  , PROTOCOL_HOOK_DESCRIP  ), 
        { KEYTYPE_STRING, HANDLER_HOOK  , "CLSID", REG_SZ, (BYTE*)PROTOCOL_RES_CLSID   },
    STD_ENTRY(HANDLER_PROTOCOLS  , ""  ), 
        { KEYTYPE_STRING, HANDLER_PROTOCOLS  , "http", REG_SZ, (BYTE*)""   }

};

const REGENTRYGROUP rgRegEntryGroups[] = {

    { HKEY_CLASSES_ROOT, rgClassesRes,     ARRAYSIZE(rgClassesRes) },
    { HKEY_CLASSES_ROOT, rgHandlerRes  ,   ARRAYSIZE(rgHandlerRes  ) },
    { NULL, NULL, 0 }       // terminator
};


//+---------------------------------------------------------------------------
//
//  Function:   DllAddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void DllAddRef(void)
{
    g_cRef++;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRelease
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void DllRelease(void)
{
    UrlMkAssert((g_cRef > 0));
    if (g_cRef > 0)
    {
        g_cRef--;
    }
}

//+---------------------------------------------------------------------------
//
//  Operator:   new
//
//  Synopsis:
//
//  Arguments:  [size] --
//
//  Returns:
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:      BUBUG: get and use IMalloc
//
//----------------------------------------------------------------------------
void * _cdecl operator new(size_t size)
{
    void * pBuffer;
    pBuffer = CoTaskMemAlloc(size);
    if (pBuffer)
    {
        memset(pBuffer,0, size);
    }
    return pBuffer;
}

//+---------------------------------------------------------------------------
//
//  Operator:   delete
//
//  Synopsis:
//
//  Arguments:  [lpv] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:      BUBUG: get and use IMalloc
//
//----------------------------------------------------------------------------
void _cdecl operator delete(void *lpv)
{
    UrlMkAssert((lpv != NULL));
    if (lpv == NULL)
    {
        return;
    }

    CoTaskMemFree(lpv);
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Dll entry point
//
//  Arguments:  [clsid] - class id for new class
//              [iid] - interface required of class
//              [ppv] - where to put new interface
//
//  Returns:    S_OK - class object created successfully created.
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllGetClassObject\n"));

    HRESULT hr = E_FAIL;

    if (clsid == CLSID_ResProtocol)
    {
        /*
        IClassFactory *pCF = NULL;

        //hr = tsaMain.GetClassFactory(&pCF);
        if (hr == NOERROR)
        {
            UrlMkAssert((pCF != NULL));
            hr = pCF->QueryInterface(iid, ppv);
            UrlMkAssert((hr == NOERROR));
            pCF->Release();
        }
        */
        CUrlClsFact *pCF = NULL;
        hr = CUrlClsFact::Create(clsid, &pCF);
        if (hr == NOERROR)
        {
            UrlMkAssert((pCF != NULL));
            hr = pCF->QueryInterface(iid, ppv);
            pCF->Release();
        }


    }


    UrlMkDebugOut((DEB_URLMON, "API OUT DllGetClassObject (hr:%lx, ppv:%p)\n",hr,*ppv));
    return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:
//
//  Arguments:  [hDll]          - a handle to the dll instance
//              [dwReason]      - the reason LibMain was called
//              [lpvReserved]   - NULL - called due to FreeLibrary
//                              - non-NULL - called due to process exit
//
//  Returns:    TRUE on success, FALSE otherwise
//
//  Notes:
//
//              The officially approved DLL entrypoint name is DllMain. This
//              entry point will be called by the CRT Init function.
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID lpvReserved)
{
    BOOL fResult = TRUE;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
#if DBG==1
        {
            UrlMkInfoLevel = (DWORD) GetProfileIntA("UrlMon","UrlMk", (DEB_ERROR | DEB_WARN));
            TransInfoLevel = (DWORD) GetProfileIntA("UrlMon","Trans", (DEB_ERROR | DEB_WARN));
        }
#endif //DBG==1
        g_hInst = hInstance;
        //tsaMain.InitApp(NULL);

        //fResult = TlsDllMain(hInstance, dwReason, lpvReserved);
        break;

    case DLL_PROCESS_DETACH:

        // Fall through

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        //fResult = TlsDllMain(hInstance, dwReason, lpvReserved);
        break;

    }
    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    return (g_cRef ? S_FALSE : S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllRegisterServer\n"));
    HRESULT hr;


    hr = HrDllRegisterServer(rgRegEntryGroups, g_hInst, NULL /*pfnLoadString*/);


    UrlMkDebugOut((DEB_URLMON, "API OUT DllRegisterServer (hr:%lx)\n",hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI DllUnregisterServer()
{
    UrlMkDebugOut((DEB_URLMON, "API _IN DllUnregisterServer\n"));
    HRESULT hr;

    hr = HrDllUnregisterServer(rgRegEntryGroups, g_hInst, NULL /*pfnLoadString*/);

    UrlMkDebugOut((DEB_URLMON, "API OUT DllUnregisterServer (hr:%lx)\n",hr));
    return hr;
}


#if DBG==1

#include <sem.hxx>
CMutexSem   mxs;
void TransUrlSpy(int iOption, const char *pscFormat, ...)
{
    static char szOutBuffer[2048];
    CLock       lck(mxs);
    DWORD tid = GetCurrentThreadId();
    DWORD cbBufLen;
    sprintf(szOutBuffer,"%08x> ", tid );
    cbBufLen = strlen(szOutBuffer);

    va_list args;
    if (iOption & TransInfoLevel)
    {
        va_start(args, pscFormat);
        wvsprintf(szOutBuffer + cbBufLen, pscFormat, args);
        va_end(args);
        UrlSpySendEntry(szOutBuffer);
    }
}
void UrlMkUrlSpy(int iOption, const char *pscFormat, ...)
{
    static char szOutBuffer[2048];
    CLock       lck(mxs);
    DWORD tid = GetCurrentThreadId();
    DWORD cbBufLen;
    sprintf(szOutBuffer,"%08x> ", tid );
    cbBufLen = strlen(szOutBuffer);

    va_list args;
    if (iOption & UrlMkInfoLevel)
    {
        va_start(args, pscFormat);
        wvsprintf(szOutBuffer + cbBufLen, pscFormat, args);
        va_end(args);
        UrlSpySendEntry(szOutBuffer);
    }
}

void UrlSpy(int iOption, const char *pscFormat, ...)
{
    static char szOutBuffer[2048];
    CLock       lck(mxs);
    DWORD tid = GetCurrentThreadId();
    DWORD cbBufLen;
    //sprintf(szOutBuffer,"%08x.%08x> ", pid, tid );
    sprintf(szOutBuffer,"%08x> ", tid );
    cbBufLen = strlen(szOutBuffer);

    va_list args;
    //if (   (iOption & DEB_INVOKES) )
    {
        va_start(args, pscFormat);
        wvsprintf(szOutBuffer + cbBufLen, pscFormat, args);
        va_end(args);
        UrlSpySendEntry(szOutBuffer);
    }
}

IDebugOut *v_pDbgOut = NULL;

void UrlSpySendEntry(LPSTR szOutBuffer)
{
    if (v_pDbgOut)
    {
        v_pDbgOut->SendEntry(szOutBuffer);
    }
    {
        OutputDebugString(szOutBuffer);
    }
}

HRESULT RegisterDebugOut(IDebugOut *pDbgOut)
{
    if (v_pDbgOut)
    {
        v_pDbgOut->Release();
        v_pDbgOut = NULL;
    }
    if (pDbgOut)
    {

        v_pDbgOut = pDbgOut;
        pDbgOut->AddRef();
    }
    return NOERROR;
}


#endif //DBG==1




