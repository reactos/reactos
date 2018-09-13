//=============================================================================
//
//  File  : modeless.cxx
//
//  Sysnopsis : modeless dialog helper functinos and support classes
//
//=============================================================================
#include "headers.hxx"

#if defined(UNIX) 
#include "window.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include <coredisp.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifdef NO_MARSHALLING
#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#include "unixmodeless.cxx"

#endif

MtDefine(CDoc_aryActiveModelessDlgs, CDoc, "Modeless HWND list")


//=======================================================
//
// forward declarations & helper classes
//=======================================================
// The thread proc param are those values the dlg thread needs
// but either don't need to be marshalled (flags, HWND) or are
// marshalled explicitely (e.g .the streams)
typedef struct ThreadProcParam {
    IStream       * _pParamStream;
    EVENT_HANDLE    _hEvent;
    IMoniker      * _pMK;
    IStream      ** _ppStm;
    HWND            _hwndDialog;
    DWORD           _dwFlags;
    HWND            _hwndParent;
} ThreadProcParam ;

//----------------------------------------------------------
// CTOR
//----------------------------------------------------------
CThreadDialogProcParam::CThreadDialogProcParam(IMoniker * pmk,
                                               VARIANT  * pvarArgIn) :
        _ulRefs(1), _pParentDoc(NULL)
{
    if (pvarArgIn)
        _varParam.Copy(pvarArgIn);

    Assert(pmk);
    _pmk = pmk;
    _pmk->AddRef(); 
}

//----------------------------------------------------------
// DTOR
//----------------------------------------------------------
CThreadDialogProcParam::~CThreadDialogProcParam ()
{
    ClearInterface (&_pmk);
    if (_pParentDoc)
    {
        _pParentDoc->Release();
        _pParentDoc = NULL;
    }
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_parameters(VARIANT * pvar)
{
    if (!pvar)
        return E_POINTER;

    return VariantCopy(pvar, &_varParam);
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_optionString (VARIANT * pvar)
{
    if (!pvar)
        return E_POINTER;

    return VariantCopy(pvar, &_varOptions);
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_moniker(IUnknown ** ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    *ppUnk = NULL;

    if (_pmk)
    {
        HRESULT hr = _pmk->QueryInterface(IID_IUnknown, (void**)ppUnk);
        return hr;
    }
    else
        return S_FALSE;
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_document(IUnknown ** ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    *ppUnk = NULL;

    return (!_pParentDoc) ? S_FALSE : 
                    (_pParentDoc->QueryInterface(IID_IUnknown, (void**)ppUnk));
}
//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL; 

    if (iid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (iid == IID_IHTMLModelessInit)
        *ppv = (IHTMLModelessInit*)this;
    else if (iid== IID_IDispatch)
        *ppv = (IDispatch *)this;

    if (*ppv)
    {
        AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

//==========================================================
//  Forward declarations & Helper functions
//==========================================================
DWORD WINAPI CALLBACK ModelessThreadProc( void * pv);
HRESULT               ModelessThreadInit( ThreadProcParam * pParam, 
                                          CHTMLDlg ** ppDlg);
HRESULT CreateModelessDialog(IHTMLModelessInit * pMI, 
                             IMoniker * pMoniker,
                             DWORD      dwFlags,
                             HWND       hwndParent,
                             CHTMLDlg **ppDialog);


//+---------------------------------------------------------
//
//  Helper function : InternalModelessDialog
//
//  Synopsis: spin off a thread, and cause the dialog to be brought up
//
//----------------------------------------------------------

HRESULT
InternalModelessDialog( HWND            hwndParent,
                        IMoniker      * pMk,
                        VARIANT       * pvarArgIn,
                        VARIANT       * pvarOptions,
                        DWORD           dwFlags,
                        COptionsHolder *pOptionsHolder,
                        CDoc           *pParentDoc,
                        IHTMLWindow2  **ppDialogWindow)
{
    HRESULT         hr = S_OK;
    THREAD_HANDLE   hThread = NULL;
    EVENT_HANDLE    hEvent = NULL;
    LPSTREAM        pStm = NULL;
    LPSTREAM        pStmParam = NULL;
    DWORD           idThread;
    ThreadProcParam tppBundle;

    // we are pushing a msgLoop, make sure this sticks around
    if (pParentDoc)
        pParentDoc->AddRef();

    if (ppDialogWindow)
         *ppDialogWindow = NULL;

    CThreadDialogProcParam *ptpp = new CThreadDialogProcParam(pMk, pvarArgIn);
    if (!ptpp)
        return E_OUTOFMEMORY;

#ifndef NO_MARSHALLING
    hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        delete ptpp;
        RRETURN(GetLastWin32Error());
    }
#endif

    // populate more of the ThreadProc sturcture
    //------------------------------------------
    if (pvarOptions)
        ptpp->_varOptions.Copy(pvarOptions);
    if (pParentDoc)
    {
        ptpp->_pParentDoc = pParentDoc;
        pParentDoc->AddRef();
    }

#ifndef NO_MARSHALLING
    // now put the threadprocParam into a marshallable place
    //------------------------------------------------------
    hr = THR(CoMarshalInterThreadInterfaceInStream(
                    IID_IHTMLModelessInit,
                    (LPUNKNOWN)ptpp,
                    &pStmParam ));
    if (hr)
        goto Cleanup;

    // Create the new thread
    //-----------------------
    tppBundle._hEvent = hEvent;
    tppBundle._pParamStream = pStmParam;
    tppBundle._pMK = pMk;
    tppBundle._ppStm = &pStm;
    tppBundle._hwndDialog = NULL;
    tppBundle._dwFlags = dwFlags;
    tppBundle._hwndParent = hwndParent;

    hThread = CreateThread(NULL, 
                           0,
                           ModelessThreadProc, 
                           &tppBundle, 
                           0, 
                           &idThread);
    if (hThread == NULL)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }
#else
    ptpp->AddRef();
    tppBundle._hEvent = NULL;
    tppBundle._pParamStream = (LPSTREAM)ptpp;
    tppBundle._pMK = pMk;
    tppBundle._ppStm = &pStm;
    tppBundle._hwndDialog = NULL;
    tppBundle._dwFlags = dwFlags;
    tppBundle._hwndParent = hwndParent;
    ModelessThreadProc((void*) &tppBundle); 
#endif
    {
        // the modeless dialog may have a statusbar window.
        // In this case, the activate() of the dialog and the CreateWindowEX for the status
        // window will thread block with the primary thread .  Instead of just
        // doing a WaitForSIngleEvent, we want to keep our message loop spining,
        // but we don't really want much to happen.  If there is a parentDoc (script
        // rasied dlg) then we make it temporarily modal while the modeless dialog is 
        // initialized and raised.  In the case of an API call, we do our best, but
        // let the message pump spin.

#ifndef NO_MARSHALLING
        // the faster this is the faster the dialog comes up
        while (WaitForSingleObject(hEvent, 0) != WAIT_OBJECT_0)
        {
            MSG  msg;

            if (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
#endif
    }

    // and finally deal with the return IHTMLWindow2 pointer
    //-------------------------------------------------
    if (pStm && ppDialogWindow)
    {
#ifndef NO_MARSHALLING
        hr = THR(CoGetInterfaceAndReleaseStream(pStm, 
                                            IID_IHTMLWindow2, 
                                            (void **) ppDialogWindow));
#else
        hr = S_OK;
        *ppDialogWindow = (IHTMLWindow2*)pStm;
#endif

        if (pParentDoc && tppBundle._hwndDialog)
        {
            HWND * pHwnd = pParentDoc->_aryActiveModeless.Append();
            if (pHwnd)
                *pHwnd = tppBundle._hwndDialog;
        }
    }

Cleanup:

#ifndef NO_MARSHALLING
    CloseEvent(hEvent);
#endif

    ptpp->Release();
    if (pParentDoc)
        pParentDoc->Release();

    if(hThread)
    {
        CloseHandle(hThread);
    }

    RRETURN( hr );
}

//+--------------------------------------------------------------------------
//
//  Function : ModlessThreadProc - 
//
//  Synopsis : responsible for the thread administation
//
//--------------------------------------------------------------------------

DWORD WINAPI CALLBACK
ModelessThreadProc( void * pv )
{
    HRESULT    hr;
    MSG        msg;
    CHTMLDlg * pDlg = NULL;

#ifndef NO_MARSHALLING
    hr = THR(OleInitialize(NULL));
    if (hr)
        goto Cleanup;
#else
    CDoc* pDoc = NULL;
#endif

    // Nop or start a message Q
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    PostThreadMessage(GetCurrentThreadId(), WM_USER+1, (LPARAM)pv, 0);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd ==NULL && msg.message==WM_USER+1)
        {
            // do my initialization work here...
            hr = ModelessThreadInit((ThreadProcParam *)msg.wParam,
                                     &pDlg);
#ifdef NO_MARSHALLING
            if (hr)
                break;
            else
            {
                pDoc = (CDoc *)((ThreadProcParam*)msg.wParam)->_hEvent;
            }
#endif
        }
        else
        {
            if (msg.message < WM_KEYFIRST ||
                msg.message > WM_KEYLAST  ||
                ( pDlg &&
                  THR(pDlg->TranslateAccelerator(&msg)) != S_OK))
            {
               // Process all messages in the message queue.
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

#ifdef NO_MARSHALLING
            if (pDoc && pDoc->LoadStatus() >= LOADSTATUS_PARSE_DONE)
            {
                break;
            }
#endif
        }
    }

Cleanup:
#ifndef NO_MARSHALLING

#endif
    return (0);
}


//+-------------------------------------------------------------------------
//
//  Function:   ModelessThreadInit
//
//  Synopsis:   Creates a new modeless dialog on this new thread
//
//--------------------------------------------------------------------------

HRESULT 
ModelessThreadInit(ThreadProcParam * pParam, CHTMLDlg ** ppDlg)
{
    CHTMLDlg          * pDlg = NULL;
    EVENT_HANDLE        hEvent= pParam->_hEvent;
    HRESULT             hr;
    IHTMLModelessInit * pMI = NULL;

    Assert (ppDlg);
    *ppDlg = NULL;

#ifndef NO_MARSHALLING
    // get the initialization interface from the marshalling place
    //------------------------------------------------------------------
    hr = THR(CoGetInterfaceAndReleaseStream(pParam->_pParamStream, 
                                            IID_IHTMLModelessInit, 
                                            (void**) & pMI));

#else
    hr = S_OK;
    pMI = (IHTMLModelessInit*)pParam->_pParamStream;
#endif

   if (hr)
        goto Cleanup;

    // Create the dialog
    //------------------------------------------------
    hr = THR(CreateModelessDialog(pMI, 
                                  pParam->_pMK, 
                                  pParam->_dwFlags, 
                                  pParam->_hwndParent, 
                                  &pDlg));
    if (hr)
        goto Cleanup;

    *ppDlg = pDlg;

#ifdef NO_MARSHALLING
    g_Modeless.Append(pDlg);
#endif

    // we need to return a window parameter. to do this we check
    // for a strm pointer in the init-structure, and if it is there
    // then we use it to marshal the ITHMLWindow2 into. The caller 
    //  thread will unmarshal from here and release the stream.
    if (pParam->_ppStm)
    {
        CDoc     * pDoc = NULL;

        hr = THR(pDlg->_pUnkObj->QueryInterface(CLSID_HTMLDocument, 
                                            (void **)&pDoc));
        if (SUCCEEDED(hr) && pDoc) //  && pDoc->_pPrimaryMarkup)
        {
            pDoc->EnsureOmWindow();

#ifndef NO_MARSHALLING
            hr = THR(CoMarshalInterThreadInterfaceInStream(
                        IID_IUnknown,
                        (LPUNKNOWN) pDoc->_pOmWindow,
                        pParam->_ppStm));
#else
            hr = S_OK;
            pDoc->_pOmWindow->AddRef();
            *pParam->_ppStm = (LPSTREAM)((IHTMLWindow2*)pDoc->_pOmWindow);
            pParam->_hEvent = (EVENT_HANDLE)pDoc;
#endif

            if (hr==S_OK)
                pParam->_hwndDialog = pDlg->_hwnd;
        }
    }

Cleanup:
#ifndef NO_MARSHALLING
    if (hEvent)
        SetEvent(hEvent);
#endif

    ReleaseInterface (pMI);

    if (hr)
    {
        if (pDlg)
        {
            pDlg->Release();
            *ppDlg = NULL;
        }

#ifndef NO_MARSHALLING
        CoUninitialize();
#endif
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  Method : CreateModelessDialog
//
//  Synopsis : does the actual creation of hte modeless dialog. this
//      code parallels the logic in internalShowModalDialog.
//
//------------------------------------------------------------------

HRESULT 
CreateModelessDialog(IHTMLModelessInit * pMI, 
                     IMoniker * pMoniker,
                     DWORD      dwFlags,
                     HWND       hwndParent,
                     CHTMLDlg **ppDialog) 
{
    HRESULT        hr;
    HTMLDLGINFO    dlginfo;
    CHTMLDlg     * pDlg = NULL;
    RECT           rc;
    TCHAR        * pchOptions = NULL;
    CVariant       cvarTransfer;


    Assert(ppDialog);
    Assert(pMI);
    Assert(pMoniker);

    // set the nonpropdesc options, these should match the defaults
    memset(&dlginfo,  0, sizeof(HTMLDLGINFO));
    dlginfo.fPropPage  = FALSE;
    dlginfo.pmk        = pMoniker;
    dlginfo.hwndParent = hwndParent;

    // Actually create the dialog
    //---------------------------
    pDlg = new CHTMLDlg(NULL, !(dwFlags & HTMLDLG_DONTTRUST), NULL);
    if (!pDlg)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDlg->_fIsModeless = TRUE;

    // Now set appropriate binding info and other dialog member variables
    //-------------------------------------------------------------------
    hr = pMI->get_parameters( &pDlg->_varArgIn);
    if (hr)
        goto Cleanup;

    pDlg->_lcid = g_lcidUserDefault;
    pDlg->_fKeepHidden = dwFlags & HTMLDLG_NOUI ? TRUE : FALSE;
    pDlg->_fAutoExit = dwFlags & HTMLDLG_AUTOEXIT ? TRUE : FALSE;

    hr = pMI->get_optionString( &cvarTransfer );
    if (hr==S_OK )
    {
        pchOptions = (V_VT(&cvarTransfer)==VT_BSTR ) ? V_BSTR(&cvarTransfer) : NULL;
    }
    
    hr = THR(pDlg->Create(&dlginfo, pchOptions));
    if (hr)
        goto Cleanup;

    rc.left   = pDlg->GetLeft();
    rc.top    = pDlg->GetTop();
    rc.right  = rc.left + pDlg->GetWidth();
    rc.bottom = rc.top  + pDlg->GetHeight();

    if (!pDlg->_fTrusted)
    {
        // Dialog width and heght each have a minimum size and no bigger than screen size
        // and the dialog Must be all on the screen
        // We need to pass a hwndParent that is used for multimonitor systems to determine
        // which monitor to use for restricting the dialog
        pDlg->VerifyDialogRect(&rc, dlginfo.hwndParent);
    }

    hr = THR(pDlg->Activate(dlginfo.hwndParent, &rc, FALSE));

    *ppDialog = pDlg;

Cleanup:
    if (hr && pDlg)
    {
        pDlg->Release();
        *ppDialog = NULL;
    }

    RRETURN( hr );
}

