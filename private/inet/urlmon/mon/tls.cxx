//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       tls.cxx
//
//  Contents:   Thread Local Storage initialization and cleanup.
//
//  Classes:
//
//  Functions:
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <mon.h>
#ifndef unix
#include "..\trans\transact.hxx"
#include "..\download\cdl.h"
#else
#include "../trans/transact.hxx"
#include "../download/cdl.h"
#endif /* unix */
#include <tls.h>

PerfDbgExtern(tagUrlDll)
DbgTag(tagUrlDllErr,  "Urlmon", "Log CBinding Errors", DEB_BINDING|DEB_ERROR);

// Thread Local Storage index.
DWORD gTlsIndex;
HINSTANCE g_hInst = 0;
HANDLE g_hHeap = 0;     // used for tls data

// Heap Handle
extern  HANDLE    g_hHeap;
#define HEAP_SERIALIZE 0
BOOL UnregisterUrlMkWndClass();
HRESULT DeleteOInetSession(DWORD dwReserved);

extern  URLMON_TS*  g_pHeadURLMONTSList;


HRESULT AddTSToList(URLMON_TS* pts)
{
    CLock lck(g_mxsMedia);
    pts->_pNext = g_pHeadURLMONTSList;
    g_pHeadURLMONTSList = pts;

    return NOERROR;
}

HRESULT RemoveTSFromList(DWORD tid)
{
    // this can only be called from ThreadDetach time
    CLock lck(g_mxsMedia);
    URLMON_TS*  pts = NULL;
    URLMON_TS*  ptsPrev = NULL;
    pts = g_pHeadURLMONTSList;

    while( pts )
    {
        if( pts->_dwTID == tid )
        {
            if( ptsPrev == NULL )
            {
                // this is the head of the list
                g_pHeadURLMONTSList = pts->_pNext;
            }
            else
            {
                ptsPrev->_pNext = pts->_pNext;
            }

            // destroy the window
            // can only be called from current thread
           
            DestroyWindow(pts->_hwndNotify);
            DbgLog2(
                tagUrlDllErr, 
                NULL, 
                ">>> tid: %lx -> delete hwnd:%p", 
                tid, 
                pts->_hwndNotify
            );
        
            // delete pts
            delete pts;

            break;
        }

        // advance 
        ptsPrev = pts;
        pts = pts->_pNext;
    }

    return NOERROR;
}

URLMON_TS* GetTS(DWORD tid)
{
    CLock lck(g_mxsMedia);
    URLMON_TS*  pts = NULL;
    pts = g_pHeadURLMONTSList;

    while( pts )
    {
        if( pts->_dwTID == tid )
        {
            break;
        }

        // advance
        pts = pts->_pNext;
    }

    return pts;
}

HRESULT CleanupTSOnProcessDetach()
{
    CLock lck(g_mxsMedia);

    URLMON_TS*  pts = NULL;
    URLMON_TS*  ptsToFree = NULL;
    pts = g_pHeadURLMONTSList;

    while( pts )
    {
        if( pts->_dwTID == GetCurrentThreadId() )
        {
            // destroy the window (the owner thread can do so)
            DestroyWindow(pts->_hwndNotify);
            DbgLog2(
                tagUrlDllErr, 
                NULL, 
                ">>> tid: %lx -> DestroyWindow :%p", 
                pts->_dwTID, 
                pts->_hwndNotify
            );
        }
        else
        {
            // we are on a thread different from the window owner 
            // so we can only Post message

            // set wndproc to user32's
            SetWindowLongPtr(
                pts->_hwndNotify, 
                GWLP_WNDPROC, 
                (LONG_PTR)DefWindowProc);

            // post message 
            PostMessage(pts->_hwndNotify, WM_CLOSE, 0, 0);
            DbgLog2(
                tagUrlDllErr, 
                NULL, 
                ">>> tid: %lx -> PostMessage WM_CLOSE :%p", 
                pts->_dwTID, 
                pts->_hwndNotify
            );
        }

        // save this pts since we are to free it
        ptsToFree = pts;

        // walk down the list
        pts = pts->_pNext;

        // free the pts
        if( ptsToFree )
        {
            delete ptsToFree;
            ptsToFree = NULL;
        }
    }

    // mark list empty
    g_pHeadURLMONTSList = NULL;

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   TLSAllocData
//
//  Synopsis:   Allocates the thread local storage block
//
//  Returns:    S_OK - allocated the data
//              E_OUTOFMEMORY - could not allocate the data
//
//--------------------------------------------------------------------------
HRESULT CUrlMkTls::TLSAllocData(void)
{
    Win4Assert(TlsGetValue(gTlsIndex) == 0);
    Win4Assert(g_hHeap != NULL);

    _pData = (SUrlMkTlsData *) HeapAlloc(g_hHeap, HEAP_SERIALIZE,
                                       sizeof(SUrlMkTlsData));

    if (_pData)
    {
        // This avoids having to set most fields to NULL, 0, etc and
        // is needed cause on debug builds memory is not guaranteed to
        // be zeroed.

        memset(_pData, 0, sizeof(SUrlMkTlsData));

        // fill in the non-zero values

        _pData->dwFlags = URLMKTLS_LOCALTID;

        // store the data ptr in TLS
        if (TlsSetValue(gTlsIndex, _pData))
        {
            return S_OK;
        }

        // error, cleanup and fallthru to error exit
        HeapFree(g_hHeap, HEAP_SERIALIZE, _pData);
        _pData = NULL;
    }

    UrlMkDebugOut((DEB_TRACE, "TLSAllocData failed.\n"));
    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoThreadCleanup
//
//  Synopsis:   Called to perform cleanup on all this threads data
//              structures, and to call CoUninitialize() if needed.
//
//              Could be called by DLL_THREAD_DETACH or DLL_PROCESS_DETACH
//
//
//----------------------------------------------------------------------------
void DoThreadCleanup(BOOL bInThreadDetach)
{
    UrlMkDebugOut((DEB_DLL | DEB_ITRACE,"_IN DoThreadCleanup\n"));
    SUrlMkTlsData *pTls = (SUrlMkTlsData *) TlsGetValue(gTlsIndex);
    DbgLog1(tagUrlDllErr, NULL, ">>> DoThreadCleanup %lx", GetCurrentThreadId() );

    if (pTls != NULL)
    {

        UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup TLS:%p \n", pTls));

        // Because of the DLL unload rules in NT we need to be careful
        // what we do in clean up. We notify the routines with special
        // behavior here.

        pTls->dwFlags |= URLMKTLS_INTHREADDETACH;

        if (pTls->pCTransMgr != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pCTransMgr:%p \n", pTls->pCTransMgr));

            // If the Release() returns non-zero, 
            // AND we're not really in ThreadDetach, then we have other references on 
            // the Transaction Manager.  Put back our reference and leave.
            //
            if (pTls->pCTransMgr->Release()
                && bInThreadDetach == FALSE)
            {
                pTls->pCTransMgr->AddRef();
                pTls->dwFlags &= ~URLMKTLS_INTHREADDETACH;
                goto Exit;
            }
        }

        if (pTls->pCodeDownloadList != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pCodeDownloadList:%p \n", pTls->pCodeDownloadList));
            delete pTls->pCodeDownloadList;
        }

        if (pTls->pRejectedFeaturesList != NULL)
        {
            LISTPOSITION curpos;
            LPCWSTR pwszRejectedFeature = NULL;
            int iNumRejected;
            int i;

            iNumRejected = pTls->pRejectedFeaturesList->GetCount();
            curpos = pTls->pRejectedFeaturesList->GetHeadPosition();

            // walk thru all the rejected features in the thread and delete
            for (i=0; i < iNumRejected; i++) {

                pwszRejectedFeature = pTls->pRejectedFeaturesList->GetNext(curpos);
                delete (LPWSTR)pwszRejectedFeature;

            }


            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pRejectedFeaturesList:%p \n", pTls->pRejectedFeaturesList));
            delete pTls->pRejectedFeaturesList;

        }

        if (pTls->pSetupCookie != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pSetupCookie:%p \n", pTls->pSetupCookie));
            delete pTls->pSetupCookie;
        }

        if (pTls->pTrustCookie != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pTrustCookie:%p \n", pTls->pTrustCookie));
            delete pTls->pTrustCookie;
        }

        if (pTls->pCDLPacketMgr != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pCDLPacketMgr:%p \n", pTls->pCDLPacketMgr));
            delete pTls->pCDLPacketMgr;
        }

#ifdef PER_THREAD
        if (pTls->pCMediaHolder != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete pCMediaHolder:%p \n", pTls->pCMediaHolder));
            delete pTls->pCMediaHolder;
        }
#endif //PER_THREAD

        // reset the index so we dont find this data again.
        TlsSetValue(gTlsIndex, NULL);

        // cleanup hwnd (not on TLS, but on urlmon's global table)
        DWORD tid = GetCurrentThreadId();
        if( GetTS(tid))
        {
            RemoveTSFromList(tid);
        }         
        else
        {
            DbgLog1(tagUrlDllErr, NULL, ">>> tld: %lx -> hwnd == NULL ", 
                    tid );

        }

        if (pTls->hwndUrlMkNotify != NULL)
        {
            DbgLog1(tagUrlDllErr, NULL, "ASSERT!!! tld: %lx ->hwnd !NULL", tid);

        }

        /*******************************************************************
        if (pTls->hwndUrlMkNotify != NULL)
        {
            UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete hwndUrlMkNotify:%p \n", pTls->hwndUrlMkNotify));
            HWND h = pTls->hwndUrlMkNotify;
            DbgLog3(tagUrlDllErr, NULL, "%s >>> tid: %lx -> delete hwnd:%p", achProgname, GetCurrentThreadId(), h);
            DestroyWindow(pTls->hwndUrlMkNotify);
        }
        else
        {
            DbgLog2(tagUrlDllErr, NULL, "%s >>> tld: %lx -> hwnd == NULL, ", achProgname, GetCurrentThreadId() );

        }
        ********************************************************************/

        HeapFree(g_hHeap, HEAP_SERIALIZE, pTls);
    }
    // else
    // there is no TLS for this thread, so there can't be anything
    // to cleanup.

Exit:
    UrlMkDebugOut((DEB_DLL | DEB_ITRACE,"OUT DoThreadCleanup\n"));
}

//+-------------------------------------------------------------------------
//
//  Function:   TlsDllMain
//
//  Synopsis:   Dll entry point
//
//  Arguments:  [hIntance]      -- a handle to the dll instance
//              [dwReason]      -- the reason LibMain was called
//              [lpvReserved]   - NULL - called due to FreeLibrary
//                              - non-NULL - called due to process exit
//
//  Returns:    TRUE on success, FALSE otherwise
//
//  Notes:      other one time initialization occurs in ctors for
//              global objects
//
//  WARNING:    if we are called because of FreeLibrary, then we should do as
//              much cleanup as we can. If we are called because of process
//              termination, we should not do any cleanup, as other threads in
//              this process will have already been killed, potentially while
//              holding locks around resources.
//
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) TlsDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL fResult = FALSE;

    #if DBG==1 || defined(PERFTAGS)
    if (dwReason == DLL_THREAD_ATTACH || dwReason == DLL_THREAD_DETACH)
        PerfDbgLog1(tagUrlDll, NULL, "+TlsDllMain %s", dwReason == DLL_THREAD_ATTACH ?
            "DLL_THREAD_ATTACH" : "DLL_THREAD_DETACH");
    #endif

    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:

        // new thread is starting
        {
            HRESULT hr;
            CUrlMkTls tls(hr);
            if (FAILED(hr))
            {
                goto ret;
            }
        }
        break;

    case DLL_THREAD_DETACH:
        // Thread is exiting, clean up resources associated with threads.
        DoThreadCleanup(TRUE);
        break;

    case DLL_PROCESS_ATTACH:

        // Initial setup. Get a thread local storage index for use by OLE
        g_hInst = hInstance;
        if ((g_hHeap = GetProcessHeap()) == 0)
        {
            // can continue E_OUTOFMEMORY;
            UrlMkAssert("Call GetProcessHeap failed.");
            goto ret;
        }
        gTlsIndex = TlsAlloc();
        if (gTlsIndex == 0xffffffff)
        {
            UrlMkAssert("Could not get TLS Index.");
            goto ret;
        }
        {
            HRESULT hr;
            CUrlMkTls tls(hr);
            if (FAILED(hr))
            {
                goto ret;
            }
        }
        break;

    case DLL_PROCESS_DETACH:

        UrlMkDebugOut((DEB_DLL,"DLL_PROCESS_DETACH:\n"));
        //if (NULL == lpvReserved)
        {
            // exiting because of FreeLibrary, so try to cleanup

            // DLL_PROCESS_DETACH is called when we unload. The thread that is
            // currently calling has not done thread specific cleanup yet.
            //

            DoThreadCleanup(TRUE);

            UnregisterUrlMkWndClass();

            if (g_pCMHolder != NULL)
            {
                UrlMkDebugOut((DEB_DLL | DEB_ITRACE,">>> DoThreadCleanup delete process pCMediaHolder:%p \n", g_pCMHolder));
                delete g_pCMHolder;
                g_pCMHolder = 0;
            }
            DeleteOInetSession(0);

            if (g_hSession)
            {
                // BUGBUG: do not close the session handle - check with RFirth
                InternetCloseHandle(g_hSession);
                g_hSession = NULL;
            }

            TlsFree(gTlsIndex);
            
        }
        UrlMkDebugOut((DEB_DLL,"DLL_PROCESS_DETACH: done\n"));
    
        break;
    }

    fResult = TRUE;

ret:
    #if DBG==1 || defined(PERFTAGS)
    if (dwReason == DLL_THREAD_ATTACH || dwReason == DLL_THREAD_DETACH)
        PerfDbgLog(tagUrlDll, NULL, "-TlsDllMain");
    #endif

    return fResult;
}
