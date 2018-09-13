/*
 * @(#)Document.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */



#include "core.hxx"
#pragma hdrstop

#include <advpub.h>
#include "core/com/classfactory.h"
#include "xml/om/xmldocument.h"
#include "mimeviewer/src/mimedownload.hxx"

// ActiveScript Engine includes
#include "xml/islands/xmlas.hxx"

#ifndef _CORE_BASE_VMM
#include "core/base/vmm.hxx"
#endif

#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"
#endif
#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _XQL_PARSE_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

#ifndef _XTL_ENGINE_PROCESSOR
#include "xtl/engine/processor.hxx"
#endif

#ifndef _DSOCTRL_HXX
#include "xml/dso/dsoctrl.hxx"
#endif

#include "mimeviewer/src/xmlmimeguid.hxx"

extern void OMInit();
extern CSMutex * g_pMutex;
long g_cServerLocks = 0; // count of locks on the class factory.
bool g_fHasExited = false; // whether runtime exit has been called already.
extern bool g_fClassInitCalled; // whether class init functions such as Name::classInit() have been called

#ifdef PRFDATA
#include "core/prfdata/msxmlprfcounters.h"
#include "core/prfdata/msxmlprfdatamap.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
#ifdef XML_HTTP_FEATURE
HRESULT STDMETHODCALLTYPE
CreateXMLHTTPRequest(REFIID iid, void **ppvObj);    // defined in xmlhttp.cxx
#endif

// these are now in uuid.lib
//const CLSID CLSID_XMLParser = {0x3aaa7326,0x09ee,0x11d2,{0x9c,0xab,0x00,0x60,0xb0,0xec,0x3d,0x39}};

HRESULT STDMETHODCALLTYPE
CreateParser(REFIID iid, void **ppvObj);    // defined in NewDocument.cxx

// MIME Viewer class factory helpers
extern 
HRESULT 
STDMETHODCALLTYPE
CreateViewer(REFIID iid, void **ppvObj);

extern 
HRESULT 
STDMETHODCALLTYPE
CreatePeerFactory(REFIID iid, void **ppvObj);

extern 
HRESULT 
STDMETHODCALLTYPE
CreateBufferedMoniker(REFIID iid, void **ppvObj);

extern
HRESULT 
STDMETHODCALLTYPE
CreateViewerPeer(REFIID iid, void **ppvObj);

extern "C" const CLSID CLSID_XMLIslandPeer;

extern 
HRESULT __stdcall 
CreateDOMFreeThreadedDocument(REFIID iid, void **ppvObj);

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
    { &CLSID_XMLDocument,       &CreateDocument, NULL },
    { &CLSID_DOMDocument,       &CreateDOMDocument, NULL },
    { &CLSID_XMLIslandPeer,     &CreateXMLIslandPeer, NULL },
    { &CLSID_CXMLScriptEngine,  &CreateXMLScriptIsland, NULL },
    { &CLSID_XMLParser,         &CreateParser, NULL },
    { &CLSID_XMLDSOControl,     &CreateDSOControl, NULL },
    { &CLSID_Viewer,            &CreateViewer, NULL },        
//    { &CLSID_PeerFactory,       &CreatePeerFactory, NULL },  // not needed until we enable the HTML DOM wrappers in mime viewer
    { &CLSID_BufferedMoniker,   &CreateBufferedMoniker, NULL },
//    { &CLSID_ViewerPeer,        &CreateViewerPeer, NULL },
#ifdef XML_HTTP_FEATURE
    { &CLSID_XMLHTTPRequest,    &CreateXMLHTTPRequest, NULL },
#endif
    { &CLSID_DOMFreeThreadedDocument,  &CreateDOMFreeThreadedDocument, NULL },
    { NULL, NULL, NULL },
};


STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;

#ifdef PRFDATA
    PrfInitCounters();
#endif

    TRY
    {
        ComponentFactoryEntry* pent = &s_ComponentFactoryTable[0];
        while (pent->_pFunc != NULL)
        {
            if (IsEqualCLSID(clsid, *(pent->_pclsid)))
            {
                if (!pent->_pFactory)
                {
                    MutexLock lock(g_pMutex);

                    // check again in case an other enty
                    if (!pent->_pFactory)
                    {
                        if (!g_fClassInitCalled &&  !IsEqualCLSID(clsid, CLSID_XMLParser))
                        {
                            Name::classInit();
                            EnumWrapper::classInit();
                            Document::classInit();
                            OMInit();
                            g_fClassInitCalled = true;
                        }

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
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY
    return hr;
}

HINSTANCE g_hInstance;
extern void DeleteTlsData();

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
#ifdef DBG
        OutputDebugStringA("MSXML: DLL_PROCESS_ATTACH\n");
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifdef DBG
        OutputDebugStringA("MSXML: DLL_PROCESS_DETACH\n");
#endif
    }
    else if (dwReason == DLL_THREAD_ATTACH)
    {
#ifdef DBG
        OutputDebugStringA("MSXML: DLL_THREAD_ATTACH\n");
#endif
    }    
    else if (dwReason == DLL_THREAD_DETACH)
    {
        // BUGBUG - with this STACK_ENTRY stress.htm hangs after 20 or so pages
        // due to an Assert that tlsdata is not set.
        // STACK_ENTRY;
        DeleteTlsData();
#ifdef DBG
        OutputDebugStringA("MSXML: DLL_THREAD_DETACH\n");
#endif
    }

    return TRUE;    // ok
}

// the root of the tlsdata chain
extern TLSDATA * g_ptlsdata;

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#define EXIT_HACK
#ifdef EXIT_HACK
    // See bug 23019, where Office was finding that DllCanUnloadNow is being
    // called after exit.  Clearly we need to investigate how or why this is
    // happening, and perhaps Office has some other bug that is causing that.
    // In the meantime this fix stops us from crashihng.  Besides, there
    // is no point trying to garbage collect if we have already shutdown the
    // GC engine.
    if (g_fHasExited)
        return S_OK;
#endif

    { // scope STACH_ENTRY
        STACK_ENTRY;
        Base::testForGC(Base::GC_FULL | Base::GC_FORCE | Base::GC_STACKTOP);
    
        if (g_cServerLocks > 0 || GetComponentCount() > 0)
            return S_FALSE;
    }

    // cycle thru threads and look for other live threads
    for (TLSDATA * ptlsdata = g_ptlsdata; ptlsdata; ptlsdata = ptlsdata->_pNext)
        if (ptlsdata->_iRunning)
            return S_FALSE; // we are still live in another thread


    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------------
//
// Function:    GetIEPath
//
// Synopsis:    Queries the registry for the location of the path
//              of Internet Explorer and returns it in pszBuf.
//
// Returns:     TRUE on success
//              FALSE if path cannot be determined
//
// Stolen from trident
//-------------------------------------------------------------------------

char g_achIEPath[MAX_PATH];     // path to iexplore.exe
const CHAR c_szIexploreKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";

void GetIEPath()
{
    BOOL fSuccess = FALSE;
    HKEY hkey;

    if (lstrlenA(g_achIEPath))
        return;

    g_achIEPath[0] = '\0';

    // Get the path of Internet Explorer 
    if (NO_ERROR == RegOpenKeyA(HKEY_LOCAL_MACHINE, c_szIexploreKey, &hkey))  
    {
        DWORD cbData = MAX_PATH;
        DWORD dwType;

        if (NO_ERROR == RegQueryValueExA(
                hkey, 
                "", 
                NULL, 
                &dwType, 
                (LPBYTE)g_achIEPath, 
                &cbData))
        {
            fSuccess = TRUE;
        }

        RegCloseKey(hkey);
    }

    if (!fSuccess)
    {
        // Failed, just say "iexplore"
        lstrcpyA(g_achIEPath, "iexplore.exe");
    }
}

const CHAR c_szIEVersionKey[] = "Software\\Microsoft\\Internet Explorer";
const CHAR c_szIEVersionValue[] = "Version";
char g_achIEVersion[50];     // version string

/* Are we running on IE 5 or above ? */
BOOL Version_GE_IE5()
{
#ifdef UNIX
    return TRUE;
#else
    BOOL fSuccess = FALSE;
    HKEY hkey;

    // Get the path of Internet Explorer 
    if (NO_ERROR == RegOpenKeyA(HKEY_LOCAL_MACHINE, c_szIEVersionKey, &hkey))  
    {
        DWORD cbData = sizeof(g_achIEVersion);
        DWORD dwType;

        if (NO_ERROR == RegQueryValueExA(
                hkey, 
                c_szIEVersionValue, 
                NULL, 
                &dwType, 
                (LPBYTE)g_achIEVersion, 
                &cbData))
        {
            fSuccess = (g_achIEVersion[0] >= '5' && g_achIEVersion[0] <= '9');
        }

        RegCloseKey(hkey);
    }
    /* assume failure for any other reason */
    return fSuccess;
#endif
}

static HINSTANCE g_hinstAdvPack = NULL;
static REGINSTALL g_pfnri = NULL;

HRESULT LoadInstall()
{
    if (!g_hinstAdvPack)
        if ((g_hinstAdvPack = LoadLibraryA("ADVPACK.DLL")) == NULL)
            return E_FAIL;
    if (!g_pfnri)
        if ((g_pfnri = (REGINSTALL)GetProcAddress(g_hinstAdvPack, achREGINSTALL)) == NULL)
            return E_FAIL;
    return S_OK;
}

void UnloadInstall()
{
    if (g_hinstAdvPack)
    {
        FreeLibrary(g_hinstAdvPack);
        g_hinstAdvPack = NULL;
    }
    g_pfnri = NULL;
}

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    STRENTRY    seReg[] = {{ "IEXPLORE", g_achIEPath} };
    STRTABLE    stReg = { 1, seReg };

    hr = LoadInstall();
    if (hr)
        return hr;

    GetIEPath();

    return g_pfnri(g_hInstance, szSection, &stReg);
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

    if (SUCCEEDED(hr))
    {   
        if (Version_GE_IE5())
            hr = CallRegInstall("RegMIME");
    }

    if(SUCCEEDED(hr))
    {
        ITypeLib    * pTypeLib;
        dwPathLen = GetModuleFileNameA(g_hInstance, szTmp, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, szTmp, -1, wszPath, MAX_PATH);

#ifdef PRFDATA
        // Register PerfMon counters...
        g_PrfData.Install(wszPath);
#endif

        // register first type library.
        hr = LoadTypeLib(wszPath, &pTypeLib);
        if(SUCCEEDED(hr))
        {
            hr = RegisterTypeLib(pTypeLib, wszPath, NULL);
            pTypeLib->Release();
        }
        
        if (hr)
            goto CleanUp;
    }

CleanUp:
    UnloadInstall();
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
    HRESULT hr2 = S_OK;

    hr = CallRegInstall("UnReg");
    if (Version_GE_IE5())
        hr2 = CallRegInstall("UnregMIME");

#ifdef PRFDATA
    // Unregister perfmon counters.
    g_PrfData.Uninstall();
#endif

    UnloadInstall();
    // let the first error code dominate if it is set
    return (hr != S_OK) ? hr : hr2;
}

#if DBG==1 || defined(MEMSTRESS_ENABLE)

extern unsigned long ulMemAllocFailDisable;

ULONG
DebugOption(ULONG ulOption, ULONG ulParam)
{
    // support 2 phase, since this only takes 1 param
    static long lRandSeed = 1;

    // what we will return
    ULONG ulReturn = 0;

    switch(ulOption)
    {
    case 1: // Seed RandomNumber generator for OutOfMemory test
        if (ulParam > 0)
            lRandSeed = (long)ulParam;
        break;

    case 2: // enable OutOfMemory testing (threshold as percentage)
        if (100 < ulParam)
            ulParam = 100;
        SeedMemAllocFail(lRandSeed, ulParam*2147483647/100);
        break;

    }
    return ulReturn;
}
#endif

//extern void BaseInit();
extern TAG tagRefCount;
extern TAG tagPointerCache;
extern TAG tagIE4OM;
extern TAG tagDOMOM;
extern BOOL MTInit();
extern TAG tagName;
extern TAG tagElementWrapperPage;
extern TAG tagSlotAllocator;
extern TAG tagSchemas;
extern TAG tagZeroListCount;

EXTERN_C BOOL 
Runtime_init()
{
//    EnableTag(tagElementWrapperPage, TRUE);

//    EnableTag(tagRefCount, TRUE);
//    EnableTag(tagPointerCache, TRUE);
//    EnableTag(tagIE4OM, TRUE);
//    EnableTag(tagDOMOM, TRUE);
//    EnableTag(tagSlotAllocator, TRUE);
//    EnableTag(tagName, TRUE);
//    EnableTag(11, TRUE);   // !SYMBOLS
//    EnableTag(2, TRUE);
//    EnableTag(tagSchemas,TRUE);
//    EnableTag(tagZeroListCount, TRUE);

    // make sure TLS is allocated for this thread
    STACK_ENTRY;

    // init multi thread support
    if (MTInit() == FALSE)
        return FALSE;

    TRY
    {
        Exception::classInit();
    }
    CATCH
    {
        return FALSE;
    }
    ENDTRY

    return TRUE;
}

extern void ClearReferences();
extern void MTExit();
extern void XMLHTTPShutdown();
BOOL g_fBadShutDown = FALSE;
extern BOOL g_fInShutDown;


EXTERN_C void
Runtime_exit()
{
    Assert(!g_fInShutDown);
    g_fInShutDown = TRUE;

    // disable assert popup tag to avoid hang in MSXMLDBG !
    EnableTag(tagAssertPop, FALSE);

    // And disable stack trace, so that MSXMLDBG doesn't crash.
    AssertThreadDisable(TRUE);

    EnsureTlsData();

    TLSDATA * ptlscurrent = GetTlsData();
    TLSDATA * ptlsdata;

    for (ptlsdata = g_ptlsdata; ptlsdata; ptlsdata = ptlsdata->_pNext)
    {
        Assert(ptlsdata == ptlscurrent || ptlsdata->_iRunning == 0);
        if (!(ptlsdata == ptlscurrent || ptlsdata->_iRunning == 0))
        {
            g_fBadShutDown = TRUE;
            DebugBreak();
            g_fHasExited = true;
            return;
        }
    }

    TRY
    {
#if MIMEASYNC
        TerminateMimeDwn();
#endif

        // free objects on 0 count list
        Base::StartFreeObjects();

        // clean up statics
        if (g_fClassInitCalled)
            Document::classExit();

        // free global references
        ClearReferences();

        Exception::classExit();

        // free objects on 0 count list
        Base::FinishFreeObjects();

        // release global page manager
        if (g_fClassInitCalled)
        {
            SlotAllocator::classExit();
            VMManager::classExit();
        }
    }
    CATCH
    {
        Assert(0 && "Should never have an Exception thrown in *::classExit()");
    }
    ENDTRY

Exit:
    // clear multi thread support;
    MTExit();
    // Unregister Window Class
    XMLHTTPShutdown();

    g_fHasExited = true;

#ifdef PRFDATA
    PrfCleanupCounters();
#endif
}

void __cdecl main()
{
}
