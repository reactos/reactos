/*
 * @(#)msxql.cxx 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include <advpub.h>
#include "core\com\ClassFactory.h"

#ifndef _XQL_QUERY_CHILDRENQUERY
#include "xql\query\ChildrenQuery.hxx"
#endif

#ifndef _XQL_QUERY_STRINGOPERAND
#include "xql\query\StringOperand.hxx"
#endif

#ifndef _XQL_QUERY_CONDITION
#include "xql\query\Condition.hxx"
#endif

#ifndef _XQL_PARSE_XQLPARSER
#include "xql\parser\XQLParser.hxx"
#endif

extern CSMutex * g_pMutex;
long g_cServerLocks = 0; // count of locks on the class factory.
HINSTANCE g_hInstance;
#if DBG==1
DWORD g_dwFALSE;
#endif

///////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//----------------------------------------------------------------
typedef struct
{
    const CLSID*                    _pclsid;
    PFN_CREATEINSTANCE              _pFunc;
    _staticreference<IClassFactory> _pFactory;
} ComponentFactoryEntry;

ComponentFactoryEntry s_ComponentFactoryTable [] =
{
    { &CLSID_XQLParser, CreateXQLParser, NULL },
    { NULL, NULL, NULL },
};


STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;


    TRY
    {
        ComponentFactoryEntry* pent = &s_ComponentFactoryTable[0];
        while (pent->_pFunc != NULL)
        {
            if (IsEqualCLSID(clsid, *(pent->_pclsid)))
            {
                if (pent->_pFactory == null)
                {
                    MutexLock lock(g_pMutex);

                    // check again in case an other enty
                    if (pent->_pFactory == null)
                    {
                        pent->_pFactory = new CClassFactory(pent->_pFunc);
                        // necessary because the reference already addref-ed it once !
                        pent->_pFactory->Release();
                    }
                }
                hr = pent->_pFactory->QueryInterface(iid, ppv);
                break;
            }
            else
                pent++;
        }

        if (pent->_pFunc == NULL)
        {
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
        return hr;
    }
    CATCH
    {
        ERETURN;
    }
}


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
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    STACK_ENTRY;
    Base::checkZeroCountList();
    if (g_cServerLocks > 0 || GetComponentCount() > 0)
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

    DWORD dwPathLen;
    char*    szTmp;
    OLECHAR* wszPath;
    HRESULT hr = E_FAIL;
    
    szTmp = new char[MAX_PATH];
    if (szTmp == null)
        goto CleanUp;
    wszPath = new OLECHAR[MAX_PATH];
    if (wszPath == null)
        goto CleanUp;

    hr = CallRegInstall("Reg");
    if(SUCCEEDED(hr))
    {
        ITypeLib    * pTypeLib;
        dwPathLen = GetModuleFileNameA(g_hInstance, szTmp, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, szTmp, -1, wszPath, MAX_PATH);

        // register first type library.
        hr = LoadTypeLib(wszPath, &pTypeLib);
        if(SUCCEEDED(hr))
        {
            hr = RegisterTypeLib(pTypeLib, wszPath, NULL);
            pTypeLib->Release();
        }
    }

CleanUp:

    if (szTmp)
        delete [] szTmp;
    if (wszPath)
        delete [] wszPath;
    return hr;
}

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall("UnReg");

    return hr;
}

extern void MTInit();
extern void MTExit();
extern void TlsInit();
extern void TlsExit();


EXTERN_C void 
Runtime_init()
{
	EnableTag(11, FALSE);	// !SYMBOLS
	EnableTag(3, FALSE);	// tagAssertExit
    
	MTInit();

    // init TLS storage manager
    TlsInit();

    STACK_ENTRY;

	g_dwTlsIndex = GetTlsIndex();
}

extern void ClearReferences();

EXTERN_C void
Runtime_exit()
{
    // free global references
    ClearReferences();

    // delete any objects on the zero list now.
    Base::checkZeroCountList();

    // free TLS manager
    TlsExit();

    // release global page manager
    SlotAllocator::classExit();
    VMManager::classExit();

    MTExit();
}

void __cdecl main()
{
}
