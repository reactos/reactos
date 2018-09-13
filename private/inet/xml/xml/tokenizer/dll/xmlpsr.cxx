/*
 * @(#)Document.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include <unknwn.h>
#include <advpub.h>
#include <stdlib.h>

#include "myclassfactory.h"
#ifdef _DEBUG
#include "msxmldbg.h"

// used for assert to fool the compiler
DWORD g_dwFALSE = 0;
#endif
#include "../parser/xmlparser.hxx"
#include "../xmlstream/xmlstream.hxx"

// these are now in uuid.lib
//const CLSID CLSID_XMLParser = {0x3aaa7326,0x09ee,0x11d2,{0x9c,0xab,0x00,0x60,0xb0,0xec,0x3d,0x39}};


extern long g_cComponents;
// long g_cComponents = 0; // defined in common.hxx
long g_cServerLocks = 0; // count of locks on the class factory.

IClassFactory* s_Factory = NULL;

//------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CreateParser(REFIID iid, void **ppvObj)
{
    XMLParser * str = new_ne XMLParser();
    if (str == NULL)
        return E_OUTOFMEMORY;
    return str->QueryInterface(iid, ppvObj);      
}

//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    if (clsid == CLSID_XMLParser)
    {
        if (s_Factory == NULL)
        {
            s_Factory = new_ne MyClassFactory(&CreateParser);
            if (s_Factory == NULL)
                return E_OUTOFMEMORY;
        }
        return s_Factory->QueryInterface(iid, ppv);
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}

HINSTANCE g_hInstance;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        delete s_Factory;
        s_Factory = NULL;
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    if (g_cComponents > 0 || g_cServerLocks > 0) 
        return S_FALSE;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibraryA("ADVPACK.DLL");

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {
            hr = pfnri(g_hInstance, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

STDAPI DllRegisterServer(void)
{
    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.

    char* szTmp = new_ne char[MAX_PATH];
    if (szTmp == NULL)
        return E_OUTOFMEMORY;
    OLECHAR* wszPath = new_ne WCHAR[MAX_PATH];
    if (wszPath == NULL)
    {
        delete szTmp;
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    
    hr = CallRegInstall("Reg");
/*
    if(SUCCEEDED(hr))
    {
        ITypeLib    * pTypeLib;
        dwPathLen = GetModuleFileNameA(g_hInstance, szTmp, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTmp, -1, wszPath, MAX_PATH);

        // register first type library.
        hr = LoadTypeLib(wszPath, &pTypeLib);
        if(SUCCEEDED(hr))
        {
            hr = RegisterTypeLib(pTypeLib, wszPath, NULL);
            pTypeLib->Release();
        }
    }
*/
    delete szTmp;
    delete wszPath;
    return hr;
}

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall("UnReg");

    return hr;
}

void *
MemAllocNe(size_t cb)
{
    void* pv = LocalAlloc(LMEM_FIXED, cb);
    return pv;
}

void *
MemAlloc(size_t cb)
{
    void* pv = LocalAlloc(LMEM_FIXED, cb);
    return pv;
}

void
MemFree(void *pv)
{
    ::LocalFree(pv);
}


//==============================================================================
WCHAR HexToUnicode(const WCHAR* text, ULONG len)
{
    WCHAR result = 0;
    for (ULONG i = 0; i < len; i++)
    {
        if (text[i] >= L'a' && text[i] <= L'f')
        {
            result = (result*16) + 10 + (text[i] - L'a');
        }
        else if (text[i] >= L'A' && text[i] <= L'F')
        {
            result = (result*16) + 10 + (text[i] - L'A');
        }
        else
            result = (result*16) + (text[i] - L'0');
    }
    return result;
}

//==============================================================================
WCHAR DecimalToUnicode(const WCHAR* text, ULONG len)
{
    WCHAR result = 0;
    for (ULONG i = 0; i < len; i++)
    {
        result = (result*10) + (text[i] - L'0');
    }
    return result;
}

