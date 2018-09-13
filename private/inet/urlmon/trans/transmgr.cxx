//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       transmgr.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>

PerfDbgTag(tagCTransMgr, "Urlmon", "Log CTransactionMgr", DEB_TRANSMGR);
DbgTag(tagCTransMgrErr,  "Urlmon", "Log CTransMgr Errors", DEB_TRANSMGR|DEB_ERROR);

extern HINSTANCE g_hInst;
LRESULT CALLBACK TransactionWndProc(HWND hWnd, UINT msg,WPARAM wParam, LPARAM lParam);

#define szURLMonClassName "URL Moniker Notification Window"
static BOOL g_fWndClassRegistered = FALSE;
CMutexSem g_mxsTransMgr;

URLMON_TS* GetTS(DWORD);
HRESULT    AddTSToList(URLMON_TS*);

//+---------------------------------------------------------------------------
//
//  Function:   GetThreadTransactionMgr
//
//  Synopsis:
//
//  Arguments:  [fCreate] --
//
//  Returns:
//
//  History:    1-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransactionMgr * GetThreadTransactionMgr(BOOL fCreate)
{
    PerfDbgLog(tagCTransMgr, NULL, "+GetThreadTransactionMgr");
    // only one thread should be here
    CLock lck(g_mxsMedia);

    HRESULT hr = NOERROR;
    CTransactionMgr *pCTMgr = NULL;

    CUrlMkTls tls(hr);
    if (hr == NOERROR)
    {
        pCTMgr = tls->pCTransMgr;

        if ((pCTMgr == NULL) && fCreate)
        {
            // the transaction mgr has an refcount of 1
            tls->pCTransMgr = pCTMgr = new CTransactionMgr;
        }
    }

    PerfDbgLog1(tagCTransMgr, NULL, "-GetThreadTransactionMgr (pCTMgr:%lx)", pCTMgr);
    return pCTMgr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTransactionMgr
//
//  Synopsis:
//
//  Arguments:  [fCreate] --
//
//  Returns:
//
//  History:    12-06-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransactionMgr * GetTransactionMgr(BOOL fCreate)
{
    return GetThreadTransactionMgr(fCreate);
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterUrlMkWndClass
//
//  Synopsis:   register the Url Moniker window class on demand
//
//  Arguments:  (none)
//
//  Returns:    return FALSE if class could not be registered
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL RegisterUrlMkWndClass()
{
    // only one thread should be here
    CLock lck(g_mxsMedia);

    if (g_fWndClassRegistered == FALSE)
    {
        // else register the window class
        WNDCLASS    wndclass;

        wndclass.style          = 0;
        wndclass.lpfnWndProc    = &TransactionWndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = g_hInst;
        wndclass.hIcon          = NULL;
        wndclass.hCursor        = NULL;;
        wndclass.hbrBackground  = (HBRUSH)NULL;
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szURLMonClassName;

        // Register the window class
        if (!RegisterClass(&wndclass))
        {
            TransAssert(FALSE);
        }
        else
        {
            g_fWndClassRegistered = TRUE;
        }
    }

    return g_fWndClassRegistered;
}


//+---------------------------------------------------------------------------
//
//  Function:   UnregisterUrlMkWndClass
//
//  Synopsis:   unregisters the hidden windows class
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:      called during process detach
//
//----------------------------------------------------------------------------
BOOL UnregisterUrlMkWndClass()
{
    if (g_fWndClassRegistered == TRUE)
    {
        // Register the window class
        if (UnregisterClass(szURLMonClassName,g_hInst))
        {
            g_fWndClassRegistered = FALSE;
        }
    }

    return g_fWndClassRegistered;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetThreadNotificationWnd
//
//  Synopsis:   return the notification for the current
//              the window is created if
//
//  Arguments:  [fCreate] --    TRUE if the window should be created
//
//  Returns:
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#ifndef HWND_MESSAGE
#define HWND_MESSAGE     ((HWND)-3)
#endif

extern BOOL  g_bNT5OrGreater;

HWND GetThreadNotificationWnd(BOOL fCreate)
{
    HRESULT hr = NOERROR;
    HWND hwnd = NULL;

    DWORD tid = GetCurrentThreadId();
    URLMON_TS*  ts = NULL;

    ts = GetTS(tid);
    if( ts )
    {
        hwnd = ts->_hwndNotify;
        TransAssert((hwnd != NULL && "TS Corrupted!"));
    }   
    else
    {
        ts = new URLMON_TS;
        
        if( ts )
        { 
            if (   fCreate
                && (hwnd == NULL)
                && RegisterUrlMkWndClass() )
            {
                hwnd = CreateWindowEx(0, szURLMonClassName, NULL,
                                    0, 0, 0, 0, 0, 
                                    g_bNT5OrGreater ? HWND_MESSAGE : NULL, 
                                    NULL, g_hInst, NULL);

                TransAssert((hwnd != NULL && "can't create Notify window"));
                if (hwnd)
                {
                    ts->_dwTID = tid;
                    ts->_hwndNotify = hwnd;

                    AddTSToList(ts);
                    DbgLog2(
                        tagCTransMgrErr, 
                        NULL, 
                        ">>> tid: %lx -> CreateWnd: %p", 
                        tid, 
                        hwnd
                    );
                }
            }
            else
            {
                delete ts;
            }
        }

    }
/**************************************************************************
    CUrlMkTls tls(hr);

    if (hr == NOERROR)
    {
        hwnd = tls->hwndUrlMkNotify;

        if (   fCreate
            && (hwnd == NULL)
            && RegisterUrlMkWndClass() )
        {
            hwnd = CreateWindowEx(0, szURLMonClassName, NULL,
                                    0, 0, 0, 0, 0, NULL, NULL, g_hInst, NULL);

            TransAssert((hwnd != NULL && "GetNotificationWnd: could not create window"));
            if (hwnd)
            {
                tls->hwndUrlMkNotify = hwnd;
                char achProgname[256];
                achProgname[0] = '\0';
                GetModuleFileNameA(NULL, achProgname, sizeof(achProgname));
                DbgLog3(tagCTransMgrErr, NULL, "%s >>> tid: %lx -> CreateWnd: %p", achProgname, GetCurrentThreadId(), hwnd);
            }
        }
    }
***************************************************************************/


    return hwnd;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransactionMgr::CTransactionMgr
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransactionMgr::CTransactionMgr() : CLifePtr()
{
    _pCTransFirst = NULL;
    _pCTransLast = NULL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransactionMgr::~CTransactionMgr
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransactionMgr::~CTransactionMgr()
{
    // The list should be empty by the time it gets destroyed
    if (_pCTransFirst)
    {
        DbgLog1(tagCTransMgr, this, "CTransactionMgr::~CTransactionMgr (list not empty:%lx)", _pCTransFirst);
    }

    //TransAssert(( _pCTransFirst == NULL ));


}

//+---------------------------------------------------------------------------
//
//  Method:     CTransactionMgr::AddTransaction
//
//  Synopsis:   Add an internet transaction to the linked list.
//
//  Arguments:  [pCTrans] --
//
//  Returns:
//
//  History:    12-12-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransactionMgr::AddTransaction(CTransaction *pCTrans)
{
    PerfDbgLog(tagCTransMgr, this, "+CTransMgr::AddTransaction");
    TransAssert((pCTrans != NULL));
    

    TransAssert((pCTrans->GetNextTransaction() == NULL));

    pCTrans->SetNextTransaction(_pCTransFirst);
    _pCTransFirst = pCTrans;
    AddRef();

    PerfDbgLog1(tagCTransMgr, this, "-CTransMgr::AddTransaction(hr:%lx)", NOERROR);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransactionMgr::RemoveTransaction
//
//  Synopsis:   Removes a transaction from the linked list.  If the transaction
//              cannot be found, E_FAIL is returned.
//
//  Arguments:  [pCTrans] --
//
//  Returns:
//
//  History:    12-12-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransactionMgr::RemoveTransaction(CTransaction *pCTrans)
{
    PerfDbgLog(tagCTransMgr, this, "+CTransMgr::RemoveTransaction");
    CLock lck(g_mxsTransMgr);
    CTransaction *pCTransPrev = NULL;
    CTransaction *pCTransTmp;
    HRESULT hr = E_FAIL;

    TransAssert((pCTrans != NULL));


    pCTransTmp = _pCTransFirst;

    TransAssert((pCTransTmp != NULL));

    // Search all the nodes in the linked list
    if (_pCTransFirst == pCTrans)
    {
        _pCTransFirst = _pCTransFirst->GetNextTransaction();
        hr = NOERROR;
        Release();
    }
    else while (pCTransTmp != NULL)
    {
        // If a match is found
        if (pCTransTmp == pCTrans)
        {
            // Remove it from the linked list
            if (pCTransPrev == NULL)
            {
                _pCTransFirst = pCTrans->GetNextTransaction();
            }
            else
            {
                pCTransPrev->SetNextTransaction(pCTrans->GetNextTransaction());
            }

            hr = NOERROR;
            Release();
        }

        pCTransPrev = pCTransTmp;
        pCTransTmp = pCTransTmp->GetNextTransaction();
    }

    TransAssert((hr == NOERROR));

    PerfDbgLog1(tagCTransMgr, this, "-CTransMgr::RemoveTransaction (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTransactionObjects
//
//  Synopsis:
//
//  Arguments:  [pBndCtx] --
//              [wzUrl] --
//              [pUnkOuter] --
//              [ppUnk] --
//              [ppCTrans] --
//              [dwOption] --
//              [ppCTranSData] --
//
//  Returns:
//
//  History:    4-12-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetTransactionObjects(LPBC pBndCtx, LPCWSTR wzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk, 
                              IOInetProtocol **ppCTrans, DWORD dwOption, CTransData **ppCTransData)
{
    HRESULT hr = S_OK;
    PerfDbgLog(tagCTransMgr, NULL, "+GetTransactionObjects");
    TransAssert((ppCTrans != NULL));
    CTransaction *pCTransTmp = NULL;

    BOOL fFound = FALSE;
    CLock lck(g_mxsTransMgr);

    CBindCtx *pCBndCtx = NULL;

    if (dwOption & BDGFLAGS_PARTIAL)
    {
        // need new transaction object
    }
    else if (pBndCtx)
    {
        hr = pBndCtx->QueryInterface(IID_IAsyncBindCtx, (void **) &pCBndCtx);

        if (hr == NOERROR)
        {
            TransAssert((pCBndCtx));
            hr = pCBndCtx->GetTransactionObjects(&pCTransTmp, ppCTransData);
            if (hr == NOERROR)
            {
                TransAssert((pCTransTmp));

                if (!pCTransTmp->IsApartmentThread())
                {
                    pCBndCtx->SetTransactionObject(NULL);
                }

                fFound = TRUE;
                hr = S_FALSE;
            }
        }

        if (hr != S_FALSE && hr != NOERROR)
        {
            CBinding *pCBdgBindToObject = NULL;
            hr = pBndCtx->GetObjectParam(SZ_BINDING, (IUnknown **)&pCBdgBindToObject);

            if (pCBdgBindToObject)
            {
                pCTransTmp = (CTransaction *) pCBdgBindToObject->GetOInetBinding();
                fFound =  pCTransTmp ? true : false;
                pCTransTmp->AddRef();
                pCBdgBindToObject->Release();
                DbgLog1(tagCTransMgr, NULL, "=== CTransMgr::GetTransaction Found Transaction:%lx", pCTransTmp);
            }
        }

    }

    if (!fFound)
    {
        CTransactionMgr *pCTransMgr = GetThreadTransactionMgr();

        if (!pCTransMgr)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // create a new transaction add it to the list
            hr = CTransaction::Create(pCBndCtx, dwOption, 0, 0, &pCTransTmp);
            if (hr == NOERROR)
            {
                hr = pCTransTmp->QueryInterface(IID_IOInetProtocol, (void **) ppCTrans);

                if (hr == NOERROR)
                {
                    pCTransTmp->Release();
                    pCTransMgr->AddTransaction(pCTransTmp);
                }
                
                if (!pCBndCtx && pBndCtx)
                {
                    pBndCtx->QueryInterface(IID_IAsyncBindCtx, (void **) &pCBndCtx);
                }
                if (pCBndCtx)
                {
                    pCBndCtx->SetTransactionObject(pCTransTmp);
                }
            }
        }
    }
    else if (pCTransTmp)
    {
        DbgLog1(tagCTransMgr, NULL, "GetTransactionObjects Found existing transaction(%lx)", pCTransTmp);
        // return false to indicate found existing transaction
        hr = S_FALSE;
    }

    if (pCBndCtx)
    {
        pCBndCtx->Release();
    }

    *ppCTrans = pCTransTmp;

    if (*ppCTrans && ppCTransData && *ppCTransData)
    {
        hr = S_FALSE;
    }

    if( hr == S_FALSE && ppCTransData && !*ppCTransData )
    {
        hr = S_OK;
    }

    PerfDbgLog2(tagCTransMgr, NULL, "-API GetTransactionObjects (pCTrans:%lx; hr:%lx)", pCTransTmp, hr);
    return hr;
}


