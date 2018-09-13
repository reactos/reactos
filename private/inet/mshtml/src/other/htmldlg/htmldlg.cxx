//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       htmldlg.cxx
//
//  Contents:   Implementation of the dlg xobject for hosting html dialogs
//
//  History:    06-14-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_COMMIT_HXX_
#define X_COMMIT_HXX_
#include "commit.hxx"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include <coredisp.h>
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>
#endif

#ifndef X_MULTIMON_H_
#define X_MULTIMON_H_
#include "multimon.h"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_RESPROT_HXX_
#define X_RESPROT_HXX_
#include "resprot.hxx"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifdef UNIX
#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include <mshtmlrc.h>
#endif

#include <mainwin.h>

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#define WINCOMMCTRLAPI
#include <commctrl.h>
#endif

#include "unixmodeless.hxx"
#endif

// BUGBUG: (anandra) Try to get rid of all this stuff below.

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"    // for MAKEUNITVALUE macro.
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"     // for propdesc components
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"    // for CSSParser
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for FRAMEOPTIONS_FOO
#endif

#ifdef WIN16
#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#include "commctrl.h"
#endif
#endif

#define MAX_NUM_ZONES_ICONS         12

HICON g_arhiconZones[MAX_NUM_ZONES_ICONS] = {0};

#define DIALOG_MIN_WIDTH    100     // in pixel
#define DIALOG_MIN_HEIGHT   100     // in pixel

// 220 is the width used in browser, we should in ssync with it
#define DIALOG_ZONE_WIDTH   220     // in pixel
#define DIALOG_STATUS_MIN   50      // in pixel

// True if we run on unicode operating system
extern BOOL g_fUnicodePlatform;


// Needs this for the IElementCollection interface
#ifndef X_COLLECT_H_
#define X_COLLECT_H_
#include <collect.h>
#endif

#define _cxx_
#include "htmldlg.hdl"


DeclareTag(tagHTMLDlgMethods, "HTML Dialog", "Methods on the html dialog")
DeclareTag(tagHTMLDlgHack, "HTML Dialog Hack", "Look for dlg in file system")
MtDefine(Dialogs, Mem, "Dialogs")
MtDefine(CHTMLDlg, Dialogs, "CHTMLDlg")
MtDefine(CHTMLDlg_aryDispObjs_pv, CHTMLDlg, "CHTMLDlg::_aryDispObjs::_pv")
MtDefine(CHTMLDLG_aryXObjs_pv, CHTMLDlg, "CHTMLDlg::_aryXObjs::_pv")
MtDefine(CHTMLPropPageCF, Dialogs, "CHTMLPropPageCF")
MtDefine(CHTMLDlgExtender, Dialogs, "CHTMLDlgExtender")
MtDefine(CHTMLDlgFrame, Dialogs, "CHTMLDlgFrame")

HRESULT CreateResourceMoniker(HINSTANCE hInst, TCHAR *pchRID, IMoniker **ppmk);
CODEPAGE CodePageFromString (TCHAR * pch, BOOL fLookForWordCharset);

HRESULT
InternalModelessDialog( HWND            hwndParent,
                        IMoniker      * pMk,
                        VARIANT       * pvarArgIn,
                        VARIANT       * pvarOptions,
                        DWORD           dwFlags,
                        COptionsHolder *pOptionsHolder,
                        CDoc           *pParentDoc,
                        IHTMLWindow2  **ppDialogWindow);


#ifdef _MAC
#ifdef __cplusplus
extern "C" {
#endif
WINBASEAPI BOOL WINAPI AfxTerminateThread(HANDLE hThread, DWORD dwExitCode);
#ifdef __cplusplus
}
#endif
#endif // _MAC

ATOM CHTMLDlg::s_atomWndClass;

const CBase::CLASSDESC CHTMLDlg::s_classdesc =
{
    &CLSID_HTMLDialog,              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDialog,               // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+------------------------------------------------------------------------
//
//  Member:     CHTMLPropPageCF::CHTMLPropPageCF
//
//  Synopsis:   ctor of class factory
//
//-------------------------------------------------------------------------

CHTMLPropPageCF::CHTMLPropPageCF(IMoniker *pmk) : CDynamicCF()
{
    Assert(pmk);
    _pmk = pmk;
    _pmk->AddRef();   
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLPropPageCF::~CHTMLPropPageCF
//
//  Synopsis:   dtor of class factory
//
//-------------------------------------------------------------------------

CHTMLPropPageCF::~CHTMLPropPageCF()
{
    if (_pmk)
        _pmk->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPropPageCF::CreateInstance
//
//  Synopsis:   per IClassFactory
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPropPageCF::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT                 hr;
    HTMLDLGINFO             dlginfo;    

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        goto Cleanup;

    Assert(_pmk);
   
    dlginfo.pmk             = _pmk;
    dlginfo.fPropPage       = TRUE;

    hr = THR(CHTMLDlg::CreateHTMLDlgIndirect(pUnkOuter, &dlginfo, riid, ppv));
    if (hr)
        goto Cleanup;

Cleanup:    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CreateHTMLDlgIndirect
//
//  Synopsis:   static ctor
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::CreateHTMLDlgIndirect(
    IUnknown *pUnkOuter,
    HTMLDLGINFO *pdlginfo,
    REFIID riid,
    void **ppv)
{
    HRESULT     hr;
    CHTMLDlg *  pDlg = new CHTMLDlg(pUnkOuter, TRUE, NULL);

    if (!pDlg)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pDlg->Create(pdlginfo));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pDlg->PrivateQueryInterface(riid, ppv));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pDlg)
    {
        pDlg->PrivateRelease();
    }
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------

CHTMLDlg::CHTMLDlg(IUnknown *pUnkOuter, BOOL fTrusted, IUnknown *pUnkHost)
         : CBase(), _aryDispObjs(Mt(CHTMLDlg_aryDispObjs_pv)), _aryXObjs(Mt(CHTMLDLG_aryXObjs_pv))
{
    TraceTag((tagHTMLDlgMethods, "constructing CHTMLDlg"));

    if (pUnkHost)
    {
        pUnkHost->QueryInterface(IID_IServiceProvider, (void **)&_pHostServiceProvider);
    }

    _aryXObjs.SetSize(0);

    // nonpropdesc member defaults. should match the pdl file
    _lDefaultFontSize = 16;
    _enumfScroll     = HTMLDlgFlagYes;
    _enumEdge        = HTMLDlgEdgeRaised;
    _enumfStatus     = HTMLDlgFlagNotSet;
    _enumfResizeable = HTMLDlgFlagNo;

    _fTrusted    = fTrusted;
}


//+------------------------------------------------------------------------
//
//  Member:     ~CHTMLDlg
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------

CHTMLDlg::~CHTMLDlg( )
{
    TraceTag((tagHTMLDlgMethods, "destroying CHTMLDlg"));

    Assert(_aryDispObjs.Size() == 0);
    Assert(_aryXObjs.Size() == 0);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Create
//
//  Synopsis:   Creation function the primary task here is to process the
//          dialog info structure in order to initialize the propdesc and
//          other member variables.  If we got here from formsSetmodalDialog
//          then the propdesc has already been set and most of the info-struct
//          is empty.
//
//  Arguments:  pdlginfo    Struct containing creation info
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::Create(HTMLDLGINFO * pdlginfo, TCHAR * pchOptions)
{
    HRESULT              hr;
    IBindCtx *           pBCtx = NULL;
    IPersistMoniker *    pPMk = NULL;
    IStream *            pStm = NULL;
    TCHAR *              pchUrl = NULL;


    if (!pdlginfo->pmk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // create the doc
    hr = THR(CoCreateInstance(CLSID_HTMLDocument,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IUnknown,
                              (void **) &_pUnkObj));

    if (hr)
        goto Cleanup;

    // set trusted dialog bit on the doc
    hr = THR(SetTrustedOnDoc(pdlginfo));
    if (hr)
        goto Cleanup;

    // process and store incoming options
    _fPropPageMode  = pdlginfo->fPropPage;

    hr = THR(DefaultMembers());
    if (hr)
        goto Cleanup;

    // parse the options string
    if (pchOptions && *pchOptions )
    {
        // we need an attr array to parse into.  The parser has changed.
        if (!_pAA)
        {
            _pAA = new CAttrArray;
            if ( !_pAA )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        CCSSParser ps(
            NULL,
            &_pAA,
#ifdef XMV_PARSE
            FALSE,
#endif
            eSingleStyle,
            BaseDesc() ? BaseDesc()->_apHdlDesc : &CStyle::s_apHdlDescs,
            this);

        ps.Open();
        ps.Write( pchOptions, lstrlen( pchOptions ) );
        ps.Close();
    }

    _dwFrameOptions = (_enumfScroll ? FRAMEOPTIONS_SCROLL_AUTO :
                                      FRAMEOPTIONS_SCROLL_NO) |
                       (_enumEdge);

    // Set the Window Style flag:
    _dwWS = GetAAborder() | WS_SYSMENU |
            (_enumfMin  ? WS_MINIMIZEBOX    : 0) |
            (_enumfMax  ? WS_MAXIMIZEBOX    : 0);

    _dwWSEx = WS_EX_DLGMODALFRAME | 
              ((!!_enumfHelp) ? WS_EX_CONTEXTHELP : 0);

    hr = THR(_pUnkObj->QueryInterface(IID_IOleObject, (void **) &_pOleObj));
    if (hr)
        goto Cleanup;

    hr = THR(_pOleObj->SetClientSite(&_Site));
    if (hr)
        goto Cleanup;

    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBCtx, 0));
    if (hr)
        goto Cleanup;

    //
    // In proppage mode, just go sync because the design of IPropertyPage
    // and IPropertyPageSite are not suited for aync operation.
    //

    if (_fPropPageMode)
    {
        //
        // Special case res: and just extract stream right here.  Urlmon
        // is too stupid and returns a failure if you do a synchronous
        // bind to something which does not return a file.  (anandra)
        //

        hr = THR(pdlginfo->pmk->GetDisplayName(pBCtx, NULL, &pchUrl));
        if (hr)
            goto Cleanup;

        if (!StrCmpNIC(_T("res:"), pchUrl, 4))
        {
            CStr    cstrResName;
            CStr    cstrResType;
            CStr    cstrRID;
            hr = THR(CResProtocol::DoParseAndBind(
                    pchUrl,
                    cstrResName,
                    cstrResType,
                    cstrRID,
                    &pStm,
                    NULL));
        }
        else
        {
            hr = THR(pdlginfo->pmk->BindToStorage(
                    pBCtx,
                    NULL,
                    IID_IStream,
                    (void **)&pStm));
        }
        if (hr)
            goto Cleanup;

        hr = THR(LoadDocSynchronous(pStm, pchUrl));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // asynchronous moniker binding
        hr = THR(_pUnkObj->QueryInterface(IID_IPersistMoniker, (void **)&pPMk));
        if (hr)
            goto Cleanup;

        hr = THR(pPMk->Load(TRUE, pdlginfo->pmk, pBCtx, STGM_READ));
        if (hr)
            goto Cleanup;
    }

    //Connect a Property Notify sink to the document so that we can catch property changes
    // which we might be interested in
    hr = THR(ConnectSink(
            _pOleObj,
            IID_IPropertyNotifySink,
            &_PNS,
            &_PNS._dwCookie));
    if (hr)
        goto Cleanup;

    _PNS._pDlg = this;

Cleanup:
    if (pchUrl)
        CoTaskMemFree(pchUrl);

    ReleaseInterface(pBCtx);
    ReleaseInterface(pPMk);
    ReleaseInterface(pStm);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Passivate
//
//  Synopsis:   1st stage destruction
//
//-------------------------------------------------------------------------

void
CHTMLDlg::Passivate()
{        
    if (_fPropPageMode)  
        OnClose();        

    //Release _pUnkObj before the VariantClear to work around OLE fault
    //Bug #69805
    ClearInterface(&_pUnkObj);
    VariantClear(&_varArgIn);

    ClearInterface(&_pOleObj);
    ClearInterface(&_pInPlaceObj);
    ClearInterface(&_pInPlaceActiveObj);
    ClearInterface(&_pPageSite);
    ClearInterface(&_pHostServiceProvider);

    super::Passivate();
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::GetViewRect
//
//  Synopsis:   Get the rectangular extent of the window.
//
//-------------------------------------------------------------------------

void
CHTMLDlg::GetViewRect(RECT *prc)
{
    *prc = _rcView;
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::OnClose
//
//  Synopsis:   Handle the WM_CLOSE message
//
//-------------------------------------------------------------------------

LRESULT
CHTMLDlg::OnClose()
{
    TraceTag((tagHTMLDlgMethods, "OnClose"));

    IGNORE_HR(Deactivate());

    if (_hwndTopParent)
    {
        //
        // Re-enable the top level parent window upon
        // destruction.
        //

        ::EnableWindow(_hwndTopParent, TRUE);
    }

    Terminate();

    return 0;
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::OnDestroy
//
//  Synopsis:   Handle the WM_DESTROY message
//
//-------------------------------------------------------------------------

LRESULT
CHTMLDlg::OnDestroy()
{
    TraceTag((tagHTMLDlgMethods, "OnDestroy"));

    _hwnd = NULL;

    return 0;
}


LRESULT
CHTMLDlg::OnActivate(WORD wFlags)
{
    TraceTag((tagHTMLDlgMethods, "OnActivate"));

    if (_pInPlaceActiveObj)
    {
        IGNORE_HR(_pInPlaceActiveObj->OnFrameWindowActivate(wFlags != WA_INACTIVE));
    }
    return 0;
}


LRESULT
CHTMLDlg::OnWindowPosChanged(RECT *prc)
{
    TraceTag((tagHTMLDlgMethods, "OnWindowPosChanged"));

    SIZEL       sizel;
    RECT        rcStatus;
    CUnitValue  cuvParam;

    if (_hwndStatus)
    {
        GetWindowRect(_hwndStatus, &rcStatus);
        prc->bottom += rcStatus.top - rcStatus.bottom;
    }

    sizel.cx = HimetricFromHPix(prc->right - prc->left);
    sizel.cy = HimetricFromHPix(prc->bottom - prc->top);

    IGNORE_HR(_pOleObj->SetExtent(DVASPECT_CONTENT, &sizel));
    if (_pInPlaceObj)
    {
        IGNORE_HR(_pInPlaceObj->SetObjectRects(ENSUREOLERECT(prc), ENSUREOLERECT(prc)));
    }

    _rcView = *prc;

    if (_hwndStatus)
        MoveStatusWindow();

    // and don't forget the dialogTop, left, width and height properties.
    //---------------------------------------------------------------------
    GetWindowRect(_hwnd, &rcStatus);
    cuvParam.SetValue (rcStatus.top, CUnitValue::UNIT_PIXELS);
    SetAAdialogTop(cuvParam);

    cuvParam.SetValue ( rcStatus.left, CUnitValue::UNIT_PIXELS);
    SetAAdialogLeft(cuvParam);

    cuvParam.SetValue ( rcStatus.right-rcStatus.left, CUnitValue::UNIT_PIXELS);
    SetAAdialogWidth(cuvParam);

    cuvParam.SetValue ( rcStatus.bottom-rcStatus.top, CUnitValue::UNIT_PIXELS);
    SetAAdialogHeight(cuvParam);

    return 0;
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::GetDoc
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::GetDoc(IHTMLDocument2 ** ppDoc)
{
    Assert (ppDoc);

    RRETURN (THR(_pUnkObj->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc)));
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::InitValues
//
//  Synopsis:   Given the objects populate the dialog with values
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::InitValues()
{
    IHTMLElementCollection *    pElements = NULL;
    IEnumVARIANT *              pEnumVar = NULL;
    IUnknown *              pUnk = NULL;
    IHTMLElement *              pHTMLElement = NULL;
    BSTR                        bstrID = NULL;
    HRESULT                     hr = S_OK;
    VARIANT                     var;
    IHTMLDocument2 *            pDoc = NULL;

    //
    // First get a list of all elements thru an enumerator
    //

    hr = THR(GetDoc (&pDoc));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->get_all(&pElements));
    if (hr)
        goto Cleanup;

    // avoid casts of the type -> (BaseClass **)&DerivedClass. -
    //hr = THR(pElements->get__newEnum((IUnknown **)&pEnumVar));
    hr = THR(pElements->get__newEnum(&pUnk));
    if (hr)
        goto Cleanup;

    pEnumVar = (IEnumVARIANT *)pUnk;
    //
    // Iterate thru elements, looking for ID attributes that start
    // with _*.  The * signifies which attribute name to
    // load in it.  E.g. _Align will load the Align attribute
    // into the field.
    //

    while (S_OK == (hr = pEnumVar->Next(1, &var, NULL)))
    {
        Assert(V_VT(&var) == VT_DISPATCH);

        hr = THR_NOTRACE(V_DISPATCH(&var)->QueryInterface(
                IID_IHTMLElement,
                (void **)&pHTMLElement));
        if (hr)
            goto LoopCleanup;

        // get id
        hr = THR(pHTMLElement->get_id(&bstrID));

        // if id begins with '_'
        if (S_OK == hr && bstrID && _T('_') == bstrID[0])
        {
            hr = THR(ConnectElement(pHTMLElement, bstrID + 1));
            if (hr)
                goto LoopCleanup;
        }

LoopCleanup:
        FormsFreeString(bstrID);
        bstrID = NULL;
        ClearInterface(&pHTMLElement);
        VariantClear(&var);
    }

    hr = S_OK;

Cleanup:
    ReleaseInterface(pElements);
    ReleaseInterface(pEnumVar);
    ReleaseInterface(pDoc);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::SetValue
//
//  Synopsis:   Set the value of some control element.
//
//  Arguments:  pCtrl       The control element to set
//              bstrID      ID to look for
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::ConnectElement(IHTMLElement * pHTMLElement, BSTR bstrID)
{
    HRESULT             hr = S_OK;
    DISPID              dispid;
    CHTMLDlgExtender *  pExtender = NULL;

    //
    // The underlying assumption here is that given an attribute,
    // the dispid of this attribute is the same across all objects
    // for this dialog.
    //

    //
    // find out dispid of the property
    //

    hr = THR(_aryDispObjs[0]->GetIDsOfNames(
            IID_NULL,
            &bstrID,
            1,
            _lcid,
            &dispid));
    if (hr)
        goto Cleanup;

    //
    // create, hookup and update extender
    //

    pExtender = new CHTMLDlgExtender(this, pHTMLElement, dispid);
    if (!pExtender)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pExtender->Value_ObjectToPropPage());
    if (hr)
        goto Error;

    hr = THR(::ConnectSink(
            pHTMLElement,
            IID_IPropertyNotifySink,
            pExtender,
            &pExtender->_dwCookie));
    if (hr)
        goto Error;

    hr = THR(_aryXObjs.Append(pExtender));
    if (hr)
        goto Error;

Cleanup:

    RRETURN(hr);

Error:
    delete pExtender;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::UpdateValues
//
//  Synopsis:   Update dialog with values based on the commit engine
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::UpdateValues()
{
    HRESULT             hr = S_OK;
    long                c;
    CHTMLDlgExtender ** ppExtender;

    if (!_fPropPageMode)
        goto Cleanup;

    for (c = _aryXObjs.Size(), ppExtender = _aryXObjs; c > 0; c--, ppExtender++)
    {
        hr = THR((*ppExtender)->Value_ObjectToPropPage());
        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::SetDirty
//
//  Synopsis:   Set the dirty property and if necessary call on the pagesite.
//
//  Arguments:  dw      As per IPropertyPageSite::OnStatusChange
//
//-------------------------------------------------------------------------

void
CHTMLDlg::SetDirty(DWORD dw)
{
    _fDirty = (dw & PROPPAGESTATUS_DIRTY) != 0;
    if (_pPageSite)
    {
        _pPageSite->OnStatusChange(dw | PROPPAGESTATUS_DIRTY);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::OnPropertyChange
//
//  Synopsis:   Receive an on property change for some dispid
//              and propagate value down to commit engine.
//
//  Arguments:  pExtend      Extender info on the control
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::OnPropertyChange(CHTMLDlgExtender *pExtender)
{
    HRESULT     hr = S_OK;

    hr = THR(pExtender->Value_PropPageToObject());
    if (hr)
        goto Cleanup;

    SetDirty(PROPPAGESTATUS_DIRTY);

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::GetDocumentTitle
//
//  Synopsis:   Retrieve the document's title in a bstr
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::GetDocumentTitle(BSTR *pbstr)
{
    HRESULT             hr;
    IHTMLDocument2 *    pDoc = NULL;

    hr = THR(GetDoc (&pDoc));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->get_title(pbstr));

Cleanup:
    ReleaseInterface (pDoc);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::PrivateQueryInterface
//
//  Synopsis:   per IPrivateUnknown
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlg::PrivateQueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS2(this, IPropertyPage, IPropertyPage2)
        QI_INHERITS(this, IPropertyPage2)
        QI_TEAROFF(this, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo2, IProvideMultipleClassInfo, NULL)
        QI_HTML_TEAROFF(this, IHTMLDialog2, NULL)

        default:
            //
            // Check for the primary dispatch interface first
            //

            if (iid == IID_IHTMLDialog)
            {
                hr = THR(CreateTearOffThunk(this, (void *)s_apfnpdIHTMLDialog, NULL, ppv, (void *)(CHTMLDlg::s_ppropdescsInVtblOrderIHTMLDialog)));
                if (hr)
                    goto Cleanup;
            }
            else if (iid == CLSID_HTMLDialog)
            {
                *ppv = this;
                goto Cleanup;
            }
            break;
    }

    if (!*ppv)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::SetPageSite
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::SetPageSite(IPropertyPageSite *pPageSite)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::SetPageSite"));
    IServiceProvider *  pSP = NULL;
    IDispatch *         pDisp = NULL;

    HRESULT hr = S_OK;

    if (_pPageSite && pPageSite)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    ReplaceInterface(&_pPageSite, pPageSite);

    if (!pPageSite)
    {
        if (_pOleObj)
        {
            hr = THR(_pOleObj->Close(OLECLOSE_NOSAVE));
            if (hr)
                goto Cleanup;
        }
        ClearInterface(&_pOleObj);
    }
    else
    {
        hr = THR(_pPageSite->GetLocaleID(&_lcid));
        if (hr)
            goto Cleanup;

        // query the IPageSite for the SID_SHTMLProperyPageArg service,        
        if (_pPageSite->QueryInterface(IID_IServiceProvider,
            (void **)&pSP))               
            goto Cleanup;
                    
        if (pSP->QueryService(SID_SHTMLProperyPageArg,        
            IID_IDispatch, (void **)&pDisp))       
            goto Cleanup;
        else
        {
        
            hr = VariantClear(&_varArgIn);
            if (hr)
                goto Cleanup;

            V_VT(&_varArgIn) = VT_DISPATCH;
            V_DISPATCH(&_varArgIn) = pDisp;
            pDisp->AddRef();
            }
     }

Cleanup:              
     ReleaseInterface(pDisp);     
     ReleaseInterface(pSP);    
     RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Activate
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::Activate(HWND hwndParent, LPCRECT prc, BOOL fModal)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::Activate"));

    HRESULT     hr = S_OK;
    HDC         hdc;
    TEXTMETRIC  tmMetrics;
    TCHAR     * pszBuf;


    if (!prc)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    _fInitializing = TRUE;
    _fActive = TRUE;

    //
    // Create a frame window if we're not in the property page mode.
    // I.e. This is a standard html dialog.
    //

    if (!_fPropPageMode)
    {
        HWND    hwndTemp;

        if (!s_atomWndClass)
        {
            HICON hIconSm;

            // NOTE (lmollico): icons are in mshtml.dll

            hIconSm = (HICON)LoadImage( g_hInstCore,                    
                                        MAKEINTRESOURCE(RES_ICO_HTML),
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        LR_SHARED );

            hr = THR(RegisterWindowClass(
                    _T("TridentDlgFrame"),
                    HTMLDlgWndProc,
                    CS_DBLCLKS,
                    NULL,
                    NULL,
                    &s_atomWndClass,
                    hIconSm));

            if (hr)
                goto Cleanup;
        }

#ifdef WIN16
        char szBuf[128];
        GlobalGetAtomName(s_atomWndClass, szBuf, ARRAY_SIZE(szBuf));
        pszBuf = szBuf;
#else
        pszBuf = (TCHAR *)(DWORD_PTR)s_atomWndClass;
#endif


        _hwnd = CreateWindowEx(
#ifndef UNIX
                _dwWSEx,
#else
                _dwWSEx | WS_EX_TOPMOST | (fModal ? WS_EX_MWMODAL_POPUP : 0),
#endif
                pszBuf,
                _T(""),
                ((!!_enumfResizeable) ? (WS_OVERLAPPED|WS_THICKFRAME) : 0) | WS_POPUP | 
                    WS_CLIPCHILDREN | WS_CAPTION | WS_CLIPSIBLINGS | _dwWS, 
                prc->left,
                prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                hwndParent,
                NULL,
                g_hInstCore,
                this);
        if (!_hwnd)
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        // Use the Status flag to determine whether to put this in or not.
        // trusted defaults to off, untrusted defaults to on
        if ((!_fTrusted && !!_enumfStatus) ||
            (_fTrusted && _enumfStatus==HTMLDlgFlagYes))
        {
            // Create the status window.
            _hwndStatus = CreateWindowEx(
                    0,                       // no extended styles
                    STATUSCLASSNAME,         // name of status window class
                    (LPCTSTR) NULL,          // no text when first created
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS ,   // creates a child window
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    _hwnd,                   // handle to parent window
                    (HMENU) 0,               // child window identifier
                    g_hInstCore,             // handle to application instance
                    NULL);                   // no window creation data
            if (!_hwndStatus)
            {
                hr = GetLastWin32Error();
                goto Cleanup;
            }

            DividePartsInStatusWindow();

            // Get the inner document
            CDoc     * pDoc;

            hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void **)&pDoc));
            if (hr)
                goto Cleanup;

            Assert(!pDoc->_cstrUrl.IsNull());

            // Set URL text
            SendMessage(_hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPCTSTR)pDoc->_cstrUrl);

            IInternetZoneManager * pizm = NULL;
            IInternetSecurityManager * pism = NULL;

            CoInternetCreateSecurityManager(NULL, &pism, 0);
            CoInternetCreateZoneManager(NULL, &pizm, 0);

            if (pism && pizm)
            {
                DWORD dwZone = 0;
                ZONEATTRIBUTES za = {sizeof(za)};
                HICON hIcon = NULL;

                pism->MapUrlToZone(pDoc->_cstrUrl, &dwZone, 0);
                pizm->GetZoneAttributes(dwZone, &za);

                if(!g_arhiconZones[0])
                {
                    CacheZonesIcons();
                }
                hIcon = g_arhiconZones[dwZone];

                // Set zone information
#ifdef WIN16
                SendMessage(_hwndStatus, SB_SETTEXT, 1, (LPARAM)za.szDisplayName);
#else
                SendMessage(_hwndStatus, SB_SETTEXTW, 1, (LPARAM)za.szDisplayName);

                if (hIcon)
                    SendMessage(_hwndStatus, SB_SETICON, 1, (LPARAM)hIcon);
#endif
            }
            ReleaseInterface(pizm);
            ReleaseInterface(pism);
        }

        //
        // Compute top-level parent and disable this window
        // because we want to function as a modal dialog
        //

        // we are looking for first window in parent chain which is not
        // child

        hwndTemp = _hwndTopParent = hwndParent;
        while (::GetWindowLong(hwndTemp, GWL_STYLE) & WS_CHILD)
        {
            hwndTemp = GetParent(hwndTemp);
            if (!hwndTemp)
                break;
            _hwndTopParent = hwndTemp;
        }

        if (fModal)
            EnableWindow(_hwndTopParent, FALSE);

        GetClientRect(_hwnd, &_rcView);
    }
    else
    {
        Assert(V_VT(&_varArgIn) == VT_DISPATCH && V_DISPATCH(&_varArgIn));

        //
        // Ensure that we have a commit holder and engine
        //

        hr = THR(EnsureCommitHolder((DWORD_PTR)hwndParent, &_pHolder));
        if (hr)
            goto Cleanup;

        hr = THR(_pHolder->GetEngine(_aryDispObjs.Size(), _aryDispObjs, &_pEngine));
        if (hr)
            goto Cleanup;

        //
        // Initialize the array of binding information
        //

        hr = THR(InitValues());
        if (hr)
            goto Cleanup;

        _hwnd = hwndParent;
        _rcView = *prc;
    }

    _fInitializing = FALSE;

    Assert(_hwnd);

    hdc = GetDC(_hwnd);
    if (GetTextMetrics(hdc, &tmMetrics))
        _lDefaultFontSize = tmMetrics.tmHeight;

    ReleaseDC(_hwnd, hdc);

    //
    // Only do a show if in proppage mode or if we've already gone interactive
    //

    if (_fPropPageMode || _fInteractive)
    {
        Show(SW_SHOW);
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Deactivate
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::Deactivate()
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::Deactivate"));

    HRESULT             hr = S_OK;
    long                i;
    CHTMLDlgExtender ** ppExtend;
    CVariant            varBool;
    IOleCommandTarget * pCommandTarget = NULL;

    if (!_pInPlaceObj)
        goto Cleanup;

    // give events a chance to fire.
    hr = THR_NOTRACE(_pInPlaceObj->QueryInterface(
                        IID_IOleCommandTarget,
                        (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    hr = pCommandTarget->Exec(
            (GUID *)&CGID_MSHTML,
            OLECMDID_ONUNLOAD,
            MSOCMDEXECOPT_DONTPROMPTUSER,
            &varBool,
            NULL);

    if ((V_VT(&varBool) == VT_BOOL && !V_BOOL(&varBool)))
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (_pInPlaceActiveObj)
    {
        hr = THR(_pInPlaceObj->UIDeactivate());
        if (hr)
            goto Cleanup;
        ClearInterface(&_pInPlaceActiveObj);
    }

    if (_pInPlaceObj)
    {
        hr = THR(_pInPlaceObj->InPlaceDeactivate());
        if (hr)
            goto Cleanup;
        ClearInterface(&_pInPlaceObj);
    }
    // disconnect the Prop Notify Sink
    if (_PNS._dwCookie && _pOleObj)
    {
        DisconnectSink(_pOleObj, IID_IPropertyNotifySink, &_PNS._dwCookie);
        Assert(_PNS._dwCookie == 0);
    }

    if (_pOleObj)
    {
        hr = THR(_pOleObj->Close(OLECLOSE_NOSAVE));
        if (hr)
            goto Cleanup;
        ClearInterface(&_pOleObj);
    }

    _aryDispObjs.ReleaseAll();

    for (i = _aryXObjs.Size(), ppExtend = _aryXObjs; i > 0; i--, ppExtend++)
    {
        DisconnectSink(
            (*ppExtend)->_pHTMLElement,
            IID_IPropertyNotifySink,
            &((*ppExtend)->_dwCookie));
    }
    _aryXObjs.ReleaseAll();

    if (_pHolder)
    {
        _pHolder->Release();
        _pHolder = NULL;
    }
    _pEngine = NULL;

Cleanup:
    ReleaseInterface(pCommandTarget);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::GetPageInfo
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::GetPageInfo(PROPPAGEINFO *ppageinfo)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::GetPageInfo"));

    HRESULT     hr;
    BSTR        bstrTitle;
    long        cb;

    Assert(ppageinfo->cb == sizeof(PROPPAGEINFO));

    hr = THR(GetDocumentTitle(&bstrTitle));
    if (hr)
        goto Cleanup;

    // Convert this bstr into a tchar *
    cb = FormsStringByteLen(bstrTitle);
    if (!cb)
    {
        ppageinfo->pszTitle = NULL;
    }
    else
    {
        ppageinfo->pszTitle = (TCHAR *)CoTaskMemAlloc(cb + sizeof(TCHAR));
        if (!ppageinfo->pszTitle)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memcpy(ppageinfo->pszTitle, bstrTitle, cb);
        ppageinfo->pszTitle[cb / sizeof(TCHAR)] = 0;
    }

    ppageinfo->size.cx = GetWidth();
    ppageinfo->size.cy = GetHeight();
    ppageinfo->pszDocString = NULL;
    ppageinfo->pszHelpFile = NULL;
    ppageinfo->dwHelpContext = 0;

Cleanup:
    FormsFreeString(bstrTitle);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::SetObjects
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::SetObjects(ULONG cUnk, IUnknown **ppUnks)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::SetObjects"));

    long                    i;
    IUnknown **             ppUnk;
    IDispatch *             pDisp;
    IServiceProvider *      pSP = NULL;
    IDispatch *             pService = NULL;
    BOOL                    fHaveServiceProvider;
    HRESULT                 hr = S_OK;

    Assert((_aryDispObjs.Size() == 0 && cUnk) ||
           (_aryDispObjs.Size() > 0 && !cUnk));

    //
    // First release any existing objects.
    //

    _aryDispObjs.ReleaseAll();
    
    //
    // Check if the page site provided the SID_SHTMLProperyPageArg service
    //
    fHaveServiceProvider = (V_VT(&_varArgIn) == VT_DISPATCH && V_DISPATCH(&_varArgIn));            

    //
    // Cache a ptr to the object's dispatch
    //

    hr = THR(_aryDispObjs.EnsureSize(cUnk));
    if (hr)
        goto Cleanup;

    for (i = cUnk, ppUnk = ppUnks; i > 0; i--, ppUnk++)
    {
        hr = THR((*ppUnk)->QueryInterface(IID_IDispatch, (void **)&pDisp));
        if (hr)
            goto Error;

        hr = THR(_aryDispObjs.Append(pDisp));
        if (hr)
        {
            ReleaseInterface(pDisp);
            goto Error;
        }

        // cache the first service provider that provides us
        // with the SID_SHTMLProperyPageArg service                                               
        if (!fHaveServiceProvider &&                
            !(*ppUnk)->QueryInterface(IID_IServiceProvider, (void**)&pSP) &&
            !pSP->QueryService(SID_SHTMLProperyPageArg, IID_IDispatch, (void **)&pService))                
        {
            fHaveServiceProvider = TRUE;
            VariantClear(&_varArgIn);
            V_VT(&_varArgIn) = VT_DISPATCH;
            V_DISPATCH(&_varArgIn) = pService;
            V_DISPATCH(&_varArgIn)->AddRef();
        }
        ClearInterface(&pService);
        ClearInterface(&pSP);       
    }

Cleanup:
    RRETURN(hr);

Error:
    _aryDispObjs.ReleaseAll();
    goto Cleanup;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Show
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::Show(UINT nCmdShow)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::Show"));

    HRESULT hr = S_OK;

    _fInitializing = TRUE;

    if (!_fPropPageMode)
    {
        if(!_fKeepHidden)
        {
            ::ShowWindow(_hwnd, nCmdShow);
        }
    }

    if (SW_SHOW == nCmdShow || SW_SHOWNORMAL == nCmdShow || SW_SHOWNA == nCmdShow)
    {
        if (!_fInteractive)
        {
            //
            // Cause a SetExtent to occur on the object.  No need to do this
            // if already interactive because SetExtent has already happened
            // in that case.
            //

            OnWindowPosChanged(&_rcView);
        }

        hr = THR(_pOleObj->DoVerb(
                OLEIVERB_UIACTIVATE,
                NULL,
                &_Site,
                0,
                _hwnd,
                ENSUREOLERECT(&_rcView)));
        if (hr)
            goto Cleanup;

        hr = THR(UpdateValues());
        if (hr)
            goto Cleanup;

    }
    else
    {
        Assert(SW_HIDE == nCmdShow);

        hr = THR(_pOleObj->DoVerb(
                OLEIVERB_HIDE,
                NULL,
                &_Site,
                0,
                _hwnd,
                ENSUREOLERECT(&_rcView)));
        if (hr)
            goto Cleanup;
    }

    // We're not done initializing unless we're done with the layout
    if (nCmdShow != SW_HIDE)
    {
        hr = THR(EnsureLayout());
        if (hr)
            goto Cleanup;
    }

    _fInitializing = FALSE;

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Move
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::Move(LPCRECT prc)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::Move"));

    Assert(_fPropPageMode);

    OnWindowPosChanged((RECT *)prc);

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::IsPageDirty
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::IsPageDirty()
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::IsPageDirty"));

    RRETURN (S_FALSE);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Apply
//
//  Synopsis:   per IPropertyPage, a no-op in browse mode
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::Apply()
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::Apply"));
     
    RRETURN(S_OK);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::Help
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::Help(LPCOLESTR bstrHelpDir)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::Help"));

    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::TranslateAccelerator
//
//  Synopsis:   per IPropertyPage
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::TranslateAccelerator(LPMSG lpmsg)
{
    TraceTag((tagHTMLDlgMethods, "IPropertyPage::TranslateAccelerator"));

    HRESULT         hr = S_FALSE;

    if (!_pInPlaceObj)
        goto Cleanup;

    if (_pInPlaceActiveObj)
    {
        hr = THR(_pInPlaceActiveObj->TranslateAccelerator(lpmsg));
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::EditProperty
//
//  Synopsis:   per IPropertyPage2
//
//---------------------------------------------------------------------------
//lookatme
HRESULT
CHTMLDlg::EditProperty(DISPID dispid)
{
    HRESULT hr = S_OK;
#ifdef NEVER
    IControls *     pCtrls;
    IEnumControl *  pEnum = NULL;
    IControl *      pCtrl;
    CPPXObject *    pPPXO;
    DISPID          dispidCtrl;
    VARIANT         var;

    if (dispid == DISPID_UNKNOWN)
        return E_FAIL;

    hr = THR(_pForm->GetControls(&pCtrls));
    if (hr)
        goto Cleanup;

    hr = THR(pCtrls->Enum(&pEnum));
    if (hr)
        goto Cleanup;

    while (!(hr = pEnum->Next(1, &pCtrl, NULL)))
    {
        hr = THR(pCtrl->QueryInterface(g_iidPPXObject, (void **) &pPPXO));
        if (hr)
            continue;

        pPPXO->GetControlSource(&dispidCtrl);
        pPPXO->Release();
        if (dispidCtrl == dispid)
        {
            V_VT(&var) = VT_BOOL;
            V_BOOL(&var) = -1;
            hr = THR(pCtrl->SetFocus(var));
            pCtrl->Release();
            goto Cleanup;
        }

        pCtrl->Release();
    }

    hr = E_FAIL;

  Cleanup:
    ReleaseInterface(pCtrls);
    ReleaseInterface(pEnum);

    RRETURN(hr);
#endif
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlg::Invoke
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlg::Invoke(
        DISPID dispidMember,
        REFIID,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        UINT *)
{
    RRETURN(InvokeEx(dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, NULL));
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlg::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlg::GetIDsOfNames(REFIID,
                     LPOLESTR *  rgszNames,
                     UINT,
                     LCID,
                     DISPID *    rgdispid)
{
    RRETURN(GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid));
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlg::GetDispID, IDispatchEx
//
//  Synopsis:   First try GetIDsOfNames, then try expando version.
//
//--------------------------------------------------------------------------

HRESULT
CHTMLDlg::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    RRETURN(THR_NOTRACE(super::GetInternalDispID(bstrName,
                                              pid,
                                              grfdex,
                                              NULL,
                                              NULL)));
}

#ifdef WIN16
#pragma code_seg ("htmldlg2_TEXT")
#endif

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::GetNextDispID
//
//  Synopsis:   per IDispatchEx, use the window's properties (as usable ing GINEx
//              and ::invoke) when that returns S_FALS then use CBase's for std
//              dialog and expando
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::GetNextDispID(DWORD grfdex,
                        DISPID id,
                        DISPID *prgid)
{
    HRESULT     hr;

    // if the id is in the range of the htmldlg properties get the dialog's properties
    // BUGBUG (carled) expandos do not work properly, and this has to be by-design.
    if ((id >= STDPROPID_XOBJ_LEFT  && id <= STDPROPID_XOBJ_HEIGHT) ||
        (id >= DISPID_HTMLDLG && id < DISPID_HTMLDLGMODEL) ||
        id == DISPID_STARTENUM)
    {
        hr = THR(super::GetNextDispID(grfdex, id, prgid));

        if (hr == S_FALSE)
        {
            // we've exhausted the dialog's properties
            id = DISPID_STARTENUM;
        }
        else
            goto Cleanup;
    }

    *prgid = DISPID_UNKNOWN;
    hr = S_FALSE;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT
CHTMLDlg::InvokeEx(DISPID dispidMember,
                   LCID lcid,
                   WORD wFlags,
                   DISPPARAMS * pdispparams,
                   VARIANT * pvarResult,
                   EXCEPINFO * pexcepinfo,
                   IServiceProvider *pSrvProvider)
{

    TraceTag((tagHTMLDlgMethods, "IDispatch::Invoke"));

    RRETURN(THR_NOTRACE(super::InvokeEx(dispidMember,
                                     lcid,
                                     wFlags,
                                     pdispparams,
                                     pvarResult,
                                     pexcepinfo,
                                     pSrvProvider)));
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::HTMLDlgWndProc
//
//  Synopsis:   Window procedure
//
//---------------------------------------------------------------------------


LRESULT CALLBACK
CHTMLDlg::HTMLDlgWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    CHTMLDlg * pDlg = (CHTMLDlg *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (!pDlg && wm != WM_NCCREATE && wm != WM_CREATE)
        goto Cleanup;

    switch (wm)
    {
    case WM_ERASEBKGND:
        if(pDlg->_pUnkObj)
            return TRUE;
        break;

#ifndef WINCE
    case WM_NCCREATE:
#else
    case WM_CREATE:
#endif // !WINCE
        pDlg = (CHTMLDlg *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pDlg);

        pDlg->_hwnd = hwnd;
        pDlg->AddRef();
#ifndef WINCE
        break;

    case WM_CREATE:
        HMENU   hMenu;

        hMenu = GetSystemMenu(hwnd, FALSE);
        if (hMenu)
        {
            if (!pDlg->_enumfMin)
                RemoveMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);

            if (!pDlg->_enumfMax)
                RemoveMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);

            if (!pDlg->_enumfResizeable)
                RemoveMenu(hMenu, SC_SIZE, MF_BYCOMMAND);

            if (!pDlg->_enumfMin && !pDlg->_enumfMax)
                RemoveMenu(hMenu, SC_RESTORE, MF_BYCOMMAND);
        }
#endif // !WINCE
        break;

#ifndef WINCE
    case WM_NCDESTROY:
#else
    case WM_DESTROY:
#endif // !WINCE
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        pDlg->_hwnd = NULL;
        pDlg->Release();
#ifndef WINCE
        break;

    case WM_DESTROY:
#endif // !WINCE
        return pDlg->OnDestroy();
        break;

    case WM_CLOSE:
        return pDlg->OnClose();
        break;

    case WM_WINDOWPOSCHANGED:
        if (! IsIconic(hwnd) 
            && !(SWP_NOSIZE & ((WINDOWPOS *) lParam)->flags ))
        {
            RECT    rc;

            GetClientRect(hwnd, &rc);
            pDlg->OnWindowPosChanged(&rc);

        }
        return 0;

    case WM_ACTIVATE:
        return pDlg->OnActivate(LOWORD(wParam));

    case WM_PALETTECHANGED:
    case WM_QUERYNEWPALETTE:
        {
            HWND hwndDoc;

            if (pDlg->_pInPlaceObj)
            {
                pDlg->_pInPlaceObj->GetWindow(&hwndDoc);
                return SendMessage(hwndDoc, wm, wParam, lParam);
            }
        }
        break;
    }

Cleanup:
    return DefWindowProc(hwnd, wm, wParam, lParam);
}



//+-------------------------------------------------------------------------
//
//  member:CHTMLDlg::Terminate () - 
//
//  Synopsis: for Modeless dialogs this needs to shut down the thread and 
//      for modal it just needs to destroy the window.
//
//--------------------------------------------------------------------------

HRESULT
CHTMLDlg::Terminate ()
{     
    if (!_fIsModeless)
    {
        if (_fPropPageMode)
            _hwnd = NULL;
        else
        {
            HWND hwnd = _hwnd;
            _hwnd = NULL;
            DestroyWindow(hwnd);
        }
    }
    else
    {
        Assert(!_fPropPageMode);

        HANDLE  hThread = GetCurrentThread();
        HWND    hwndCurrent = _hwnd;
        _hwnd = NULL;

#ifdef NO_MARSHALLING
        g_Modeless.Remove(this);
#endif

        // release the ref that the thread has on the CModelessDlg
        Release();

        // now destory the hwnd. we cached it in case the above release
        // DTor's *this*
        DestroyWindow(hwndCurrent);

#ifndef NO_MARSHALLING
        OleUninitialize();

        if (hThread)
        {
            DWORD dwExitCode;

    #ifndef WIN16
            if (GetExitCodeThread(hThread, &dwExitCode) &&
                dwExitCode == STILL_ACTIVE)
            {
                CloseHandle(hThread);
                ExitThread(0);
            }
    #endif
        }
#endif
    }
    return S_OK; 
}





//---------------------------------------------------------------------------
//
//  Member:     InternalShowModalDialog
//
//  Synopsis:   Internal helper to show the modal html dialog.
//
//  Arguments:  hwndParent      Window to parent to
//              pMk             Moniker to dialog page
//              pVarIn          argument
//              pVarOut         The return value from the dialog.
//
//---------------------------------------------------------------------------

HRESULT
InternalShowModalDialog(
    HWND        hwndParent,
    IMoniker *  pMk,
    VARIANT *   pvarArgIn,
    TCHAR *     pchOptions,
    VARIANT *   pvarArgOut,
    IUnknown *  punkHost,
    COptionsHolder *pOptionsHolder,
    DWORD       dwFlags)
{
    HRESULT             hr = S_OK;
    CHTMLDlg *          pDlg = NULL;
    HTMLDLGINFO         dlginfo;
    RECT                rc;
    MSG                 msg;
    BOOL                fTrusted;

    // ReleaseCapture to cancel any drag-drop operation
    // BUGBUG (lmollico): This call should be in EnableModeless in shdocvw instead
    ::ReleaseCapture();

    // set the nonpropdesc options, these should match the
    // defaults
    memset(&dlginfo,  0, sizeof(HTMLDLGINFO));
    dlginfo.pmk             = pMk;
    dlginfo.fPropPage       = FALSE;
    dlginfo.hwndParent      = hwndParent;


    // decode the trusted flag
    fTrusted = (dwFlags & HTMLDLG_DONTTRUST) ? FALSE : TRUE;

    // First create the dialog
    pDlg = new CHTMLDlg(NULL, fTrusted, punkHost);
    if (!pDlg)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Now set appropriate binding info
    if (pvarArgIn)
    {
        hr = THR(VariantCopy (&pDlg->_varArgIn, pvarArgIn));
        if (hr)
            goto Cleanup;
    }

    pDlg->_lcid = g_lcidUserDefault;
    pDlg->_fKeepHidden = dwFlags & HTMLDLG_NOUI ? TRUE : FALSE;
    pDlg->_fAutoExit = dwFlags & HTMLDLG_AUTOEXIT ? TRUE : FALSE;

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
        pDlg->VerifyDialogRect(&rc, hwndParent);
    }

    hr = THR(pDlg->Activate(hwndParent, &rc, TRUE));
    if (hr)
        goto Cleanup;
    if (pOptionsHolder)
    {
        pOptionsHolder->SetParentHWnd(pDlg->_hwnd);
    }

    // Finally push a message loop and give messages to the dialog

    while (pDlg->_hwnd)
    {
        ::GetMessage(&msg, NULL, 0, 0);

        if (msg.message < WM_KEYFIRST ||
            msg.message > WM_KEYLAST  ||
            THR(pDlg->TranslateAccelerator(&msg)) != S_OK)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    if (pvarArgOut)
    {
        hr = THR(VariantCopy(pvarArgOut, &pDlg->_varRetVal));
        if (hr)
            goto Cleanup;

        // if the _varRetVal is the dlg's window then the dlg
        // is keeping a ref count on itself and will never release
        // therefore we need to Clear that now. in this case the
        // dlg.returnValue property will subsequently return
        // VT_EMPTY/NULL from pvarArgOut.
        if ( (V_VT(&pDlg->_varRetVal) == VT_DISPATCH ||
              V_VT(&pDlg->_varRetVal) == VT_UNKNOWN) )
        {
            CHTMLDlg *pRetTest = NULL;

            hr = THR_NOTRACE(
                V_UNKNOWN(&pDlg->_varRetVal)->QueryInterface(CLSID_HTMLDialog,
                          (void**)&pRetTest));
            if (!hr && pRetTest==pDlg)
            {
                VariantClear(&pDlg->_varRetVal);
            }
            hr = S_OK;
        }

    }

Cleanup:
    if (pDlg)
        pDlg->Release();

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     ShowModalDialog
//
//  Synopsis:   Show an html dialog given a resource to load from.
//
//  Arguments:  hwndParent      Window to parent to
//              pMk             Moniker to dialog page
//              pVarIn          argument
//              pVarOut         The return value from the dialog.
//
//---------------------------------------------------------------------------

STDAPI
ShowModalDialog(HWND        hwndParent,
                IMoniker *  pMk,
                VARIANT *   pvarArgIn,
                TCHAR *     pchOptions,
                VARIANT *   pvarArgOut)
{
    HRESULT hr;

    CoInitialize(NULL);

    hr = THR(InternalShowModalDialog(
        hwndParent,
        pMk,
        pvarArgIn,
        pchOptions,
        pvarArgOut,
        NULL,
        NULL,
        0));  // by pasing 0, this is a trusted dialog.  this is correct (carled)

    CoUninitialize();

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     ShowHTMLDialog
//
//  Synopsis:   Just like ShowModalDialog, but with a prettier name.
//
//  Arguments:  hwndParent      Window to parent to
//              pMk             Moniker to dialog page
//              pVarIn          argument
//              pVarOut         The return value from the dialog.
//
//---------------------------------------------------------------------------

STDAPI
ShowHTMLDialog(HWND        hwndParent,
                IMoniker *  pMk,
                VARIANT *   pvarArgIn,
                TCHAR *     pchOptions,
                VARIANT *   pvarArgOut)
{
    HRESULT hr;

    hr = THR(ShowModalDialog(
        hwndParent,
        pMk,
        pvarArgIn,
        pchOptions,
        pvarArgOut));

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     ShowModelessHTMLDialog
//
//  Synopsis:   like the window::createModelessDialog, this creates and brings up
//              a modeless version of the HTMLDialog.  This function is the api 
//              implementation rather then the OM.
//
//  Arguments:  hwndParent      Window to parent to
//              pMk             Moniker to dialog page
//              pVarIn          argument
//              pVarOut         The return value from the dialog.
//
//---------------------------------------------------------------------------

STDAPI
ShowModelessHTMLDialog(HWND       hwndParent,
                       IMoniker * pMK,
                       VARIANT  * pvarArgIn,
                       VARIANT  * pvarOptions,
                       IHTMLWindow2 ** ppDialog)
{
    HRESULT hr = InternalModelessDialog(hwndParent, 
                                        pMK,
                                        pvarArgIn,
                                        pvarOptions,    // options string
                                        0,              // Trust flag, should be trusted
                                        NULL,           // options holder
                                        NULL,           // ParentDoc, OK to be NULL
                                        ppDialog);      // IOmWindow2 

    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
//  Member:     ShowModalDialogHelper
//
//  Synopsis:   A helper used by internal forms code to call the
//              html dialog api.
//
//  Arguments:  pBase       -   Pointer to associated CBase instance.
//              pchHTML         HTML name - resource name or Url
//              pDisp           The IDispatch to work on
//              pvarArgOut      The return value from the dialog.
//              pcoh            COptionsHolder (if any) passed down
//              dwFlags         Option flags
//
// HTMLDLG_NOUI             0x1     // run the dialog but don't show it
// HTMLDLG_RESOURCEURL      0x2     // create the moniker for a resource
// HTMLDLG_AUTOEXIT         0x4     // exit immediatly after running script
// HTMLDLG_DONTTRUST        0x8     // don't trust the moniker passed in
//
//---------------------------------------------------------------------------

HRESULT
ShowModalDialogHelper(CDoc *pDoc,
                      TCHAR *pchHTML,
                      IDispatch *pDisp,
                      COptionsHolder * pcoh,
                      VARIANT *pvarArgOut,
                      DWORD   dwFlags)
{
    HRESULT         hr;
    CVariant        varArgIn;
    IMoniker *      pmk = NULL;
    HWND            hwndParent;
    BOOL            fTrusted;

    V_VT(&varArgIn) = VT_DISPATCH;
    V_DISPATCH(&varArgIn) = pDisp;
    pDisp->AddRef();

    if(dwFlags & HTMLDLG_RESOURCEURL)
    {
#if DBG == 1
        if (IsTagEnabled(tagHTMLDlgHack))
        {
            TCHAR   achTemp[MAX_PATH];
            TCHAR * pch;

            _tcscpy(achTemp, _T("file://"));
            Verify(GetModuleFileName(g_hInstCore, achTemp + 7, MAX_PATH - 7));
            pch = _tcsrchr(achTemp, _T('\\'));
            _tcscpy(pch + 1, pchHTML);
            hr = THR(CreateURLMoniker(NULL, achTemp, &pmk));
        }
        else
#endif
        hr = THR(CreateResourceMoniker(
                GetResourceHInst(),
                pchHTML,
                &pmk));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(CreateURLMoniker(NULL, pchHTML, &pmk));
        if(hr)
            goto Cleanup;
    }

    fTrusted = (dwFlags & HTMLDLG_DONTTRUST) ? FALSE : TRUE ;

    {
        CDoEnableModeless   dem(pDoc);

        hwndParent = dem._hwnd;
        
        // CHROME
        // This will enable script errors to show up on
        // pages hosted in chrome textures.  Passing
        // the top level document's hwnd down via IOleWindow::GetWindow
        // means that the top level hwnd gets abused by other entities
        // wishing to create child windows.
        if (hwndParent || ((NULL != pDoc) && pDoc->IsChromeHosted()))
        {
            hr = THR(InternalShowModalDialog(
                hwndParent,
                pmk,
                &varArgIn,
                NULL,
                pvarArgOut,
                NULL,
                pcoh,
                dwFlags));
        }
    }
    
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pmk);
    RRETURN(hr);
}



//+----------------------------------------------------------
//
//  member : VerifyDialogRect()
//
//  Synopsis : for security type reasons we do not want the
//      dialogs to get relocated (via script or bad styles)
//      offscreen.  similarly we want a minimum size (which was
//      set in the pdl
//
//  parameters : pRect is an In/Out parameter.
//               hwndRef - HWND that is used to obtain current monitor
//                      on multimonitor systems
//-----------------------------------------------------------
void
CHTMLDlg::VerifyDialogRect(RECT * pRect, HWND hwndRef)
{
    long     dlgWidth  = pRect->right - pRect->left;
    long     dlgHeight = pRect->bottom - pRect->top;

#ifdef WIN16
    long    lLeftMonitor     = 0;
    long    lTopMonitor      = 0;
    long    lWidthMonitor    = GetSystemMetrics(SM_CXSCREEN);
    long    lHeightMonitor   = GetSystemMetrics(SM_CYSCREEN);
#else
    Assert(IsWindow(hwndRef));

    HMONITOR        curMonitor;
    MONITORINFO     monitorInfo;

    // Get the current monitor
    curMonitor = MonitorFromWindow(hwndRef, MONITOR_DEFAULTTONEAREST);

    monitorInfo.cbSize = sizeof(monitorInfo);

    Verify(GetMonitorInfo(curMonitor, &monitorInfo));

    long    lLeftMonitor     = monitorInfo.rcMonitor.left;
    long    lTopMonitor      = monitorInfo.rcMonitor.top;
    long    lWidthMonitor    = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    long    lHeightMonitor   = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
#endif

    // make sure width is no larger than screen
    dlgWidth  = min(max(dlgWidth, (long)DIALOG_MIN_WIDTH), lWidthMonitor);
    dlgHeight = min(max(dlgHeight, (long)DIALOG_MIN_HEIGHT ), lHeightMonitor);

    // Always keep whole dialog visable
    pRect->left = max(pRect->left, (long) lLeftMonitor);
    pRect->top  = max(pRect->top, (long) lTopMonitor);

    if ( (pRect->left + dlgWidth) > lLeftMonitor + lWidthMonitor)
    {
        pRect->left = lLeftMonitor + lWidthMonitor - dlgWidth;
    }
    if ( (pRect->top + dlgHeight) > lTopMonitor + lHeightMonitor )
    {
        pRect->top = lTopMonitor + lHeightMonitor - dlgHeight;
    }

    pRect->right  = pRect->left + dlgWidth;
    pRect->bottom = pRect->top  + dlgHeight;
}


void
CHTMLDlg::MoveStatusWindow()
{
    RECT    rcStatus;
    RECT    rc;

    Assert(_hwndStatus);

    GetClientRect(_hwnd, &rc);
    GetWindowRect(_hwndStatus, &rcStatus);

    MoveWindow(
            _hwndStatus,
            0,
            rc.bottom - (rcStatus.bottom - rcStatus.top),
            rc.right - rc.left,
            rc.bottom,
            TRUE);

    DividePartsInStatusWindow();
}


void
CHTMLDlg::DividePartsInStatusWindow()
{
    RECT        rc;
    UINT        cxZone;

    Assert(_hwndStatus);
    GetClientRect(_hwndStatus, &rc);

    cxZone = (rc.right > (DIALOG_ZONE_WIDTH + DIALOG_STATUS_MIN) )
            ? DIALOG_ZONE_WIDTH: (rc.right /2);

    INT aWidths[] = {rc.right - cxZone, -1 };

    // Create 2 parts: URL text, zone information
    SendMessage(_hwndStatus, SB_SETPARTS, 2, (LPARAM)aWidths);
}


//+=========================================================
HRESULT
CHTMLDlg::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};

void
CHTMLDlg::CacheZonesIcons()
{
    IInternetZoneManager * pizm = NULL;
    DWORD dwZoneCount = 0;

    // Create ZoneManager
    CoInternetCreateZoneManager(NULL, &pizm, 0);

    if (pizm)
    {
        DWORD dwZoneEnum;

        if (SUCCEEDED(pizm->CreateZoneEnumerator(&dwZoneEnum, &dwZoneCount, 0)))
        {
            for (int nIndex=0; (DWORD)nIndex < dwZoneCount; nIndex++)
            {
                DWORD           dwZone;
                ZONEATTRIBUTES  za = {sizeof(ZONEATTRIBUTES)};
                WORD            iIcon=0;
                HICON           hIcon = NULL;

                pizm->GetZoneAt(dwZoneEnum, nIndex, &dwZone);

                // get the zone attributes for this zone
                pizm->GetZoneAttributes(dwZone, &za);

#ifndef WIN16
                // Zone icons are in two formats.
                // wininet.dll#1200 where 1200 is the res id.
                // or foo.ico directly pointing to an icon file.
                // search for the '#'
                LPWSTR pwsz = StrChrW(za.szIconPath, L'#');

                if (pwsz)
                {
                    // if we found it, then we have the foo.dll#00001200 format
                    pwsz[0] = L'\0';
                    iIcon = (WORD)StrToIntW(pwsz+1);
                    ExtractIconExW(za.szIconPath,(UINT)(-1*iIcon), NULL, &hIcon, 1 );
                }
                else
                    hIcon = (HICON)ExtractAssociatedIconW(g_hInstCore, za.szIconPath, (LPWORD)&iIcon);

                if (nIndex < MAX_NUM_ZONES_ICONS)
                     g_arhiconZones[nIndex] = hIcon;
#endif

            }
            pizm->DestroyZoneEnumerator(dwZoneEnum);
        }
    }

    ReleaseInterface(pizm);
}


//+-------------------------------------------------------------------------
//
//  Function:   DeinitHTMLDialogs
//
//  Synopsis:   Clears global created by find dialogs "memory"
//
//--------------------------------------------------------------------------

void
DeinitHTMLDialogs()
{
    extern  BSTR    g_bstrFindText;
    FormsFreeString(g_bstrFindText);
    g_bstrFindText = NULL;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::OnReadyStateChange
//
//  Synopsis:   Handle changes in readystate of document
//
//---------------------------------------------------------------------------

// this is defined in winuser.h for WINVER >= 0x0500
// We need this to compile
#if(WINVER < 0x0500)
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif /* WINVER >= 0x0500 */

void
CHTMLDlg::OnReadyStateChange()
{
    HRESULT     hr;
    CVariant    Var;
    IDispatch * pDisp = NULL;
    long        lReadyState;
    LPTSTR      pszTitle  = NULL;
    BSTR        bstrTitle = NULL;

    if (_fPropPageMode) // Readystate changes are not interesting if proppage
        goto Cleanup;

    hr = THR(_pUnkObj->QueryInterface(IID_IDispatch, (void **)&pDisp));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(GetDispProp(
            pDisp,
            DISPID_READYSTATE,
            g_lcidUserDefault,
            &Var,
            NULL));
    if (hr)
        goto Cleanup;

    //
    // Look for either VT_I4 or VT_I2
    //

    if (V_VT(&Var) == VT_I4)
    {
        lReadyState = V_I4(&Var);
    }
    else if (V_VT(&Var) == VT_I2)
    {
        lReadyState = V_I2(&Var);
    }
    else
    {
        Assert(0 && "Bad VT for readystate");
        goto Cleanup;
    }

    if (lReadyState >= READYSTATE_INTERACTIVE && !_fInteractive)
    {
        HTMLDlgCenter        attrCenter;

        _fInteractive = TRUE;

        BOOL        fRightToLeft = FALSE;
        CDoc      * pDoc;

        hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void **)&pDoc));
        if (hr)
            goto Cleanup;

        pDoc->GetDocDirection(&fRightToLeft);

        // if the document is RTL change the window's extended style 
        // so the caption is right aligned.
        if(fRightToLeft)
        {
            LONG lExStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);
            // (paulnel) We need to conditionally set WS_EX_LAYOUTRTL here. Due to the way
            //           it is designed we do not want to set WS_EX_RIGHT or WS_EX_RTLREADING
            //           if we have mirrored support.
            //           The document takes care of WS_EX_LEFTSCROLLBAR | WS_EX_RTLREADING
            //           internally.
            if(!g_fMirroredBidiLayout)
                lExStyle |= WS_EX_RIGHT | WS_EX_RTLREADING;
            else
                lExStyle |= WS_EX_LAYOUTRTL;
            SetWindowLong(_hwnd, GWL_EXSTYLE, lExStyle);
        }

        //
        // Set the title of the window to be the document's title
        //

        hr = THR(GetDocumentTitle(&bstrTitle));
        if (!hr)
        {
            if (!_fTrusted)
            {
                hr = Format(FMT_OUT_ALLOC,
                            &pszTitle,
                            0,
                            _T("<0s><1i>"),
                            bstrTitle,
                            GetResourceHInst(), IDS_WEBPAGEDIALOG);
                if (hr)
                    goto Cleanup;
            }
            SetWindowText(_hwnd, _fTrusted ? bstrTitle : pszTitle);
        }

        // handle setting the Center value
        attrCenter = GetAAcenter();
        if (HTMLDlgCenterParent == attrCenter || HTMLDlgCenterDesktop == attrCenter)
        {
            HWND        hwndParent;
            CUnitValue  cuvParam;
            RECT        r;

            hwndParent = (HTMLDlgCenterParent == attrCenter && _hwndTopParent) ?
                        _hwndTopParent : GetDesktopWindow();

            Assert (hwndParent);
            GetWindowRect (hwndParent, &r);

            //
            //  check if left/top value has been set to no default value,
            //  if so, do not move to center.
            //

            if (!_pAA || _pAA->FindAAIndex(s_propdescCHTMLDlgdialogLeft.a.GetDispid(),
                                  CAttrValue::AA_Attribute) == AA_IDX_UNKNOWN)
            {
                cuvParam.SetValue (r.left + (r.right - r.left - GetWidth()) / 2, CUnitValue::UNIT_PIXELS);
                SetAAdialogLeft(cuvParam);
            }

            if (!_pAA || _pAA->FindAAIndex(s_propdescCHTMLDlgdialogTop.a.GetDispid(),
                                  CAttrValue::AA_Attribute) == AA_IDX_UNKNOWN)
            {
                cuvParam.SetValue (r.top  + (r.bottom - r.top - GetHeight()) / 2, CUnitValue::UNIT_PIXELS);
                SetAAdialogTop(cuvParam);
            }
        }
        OnPropertyChange(STDPROPID_XOBJ_WIDTH, 0);

        //
        // If Activate has already been called, go ahead and do a show now.
        //

        if (_fActive)
        {
            Show(SW_SHOW);
        }
    }

Cleanup:
    delete pszTitle;
    FormsFreeString(bstrTitle);
    ReleaseInterface(pDisp);
}












//
// Do NOT under any circumstances move this part above.  We don't
// want to contaminate any other part of the dialog with private
// knowledge of the document. (anandra)
//

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::OnPropertyChange
//
//  Synopsis:   Set the return value
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    if (!_fPropPageMode)
    {
        //
        // If we're in dialog mode, then let the top/left/width/height change
        // trigger a resize of the frame window.
        //

        switch (dispid)
        {
        case DISPID_READYSTATE:
            OnReadyStateChange();
            break;

        case STDPROPID_XOBJ_WIDTH:
        case STDPROPID_XOBJ_HEIGHT:
        case STDPROPID_XOBJ_TOP:
        case STDPROPID_XOBJ_LEFT:
        case DISPID_A_FONT :
        case DISPID_A_FONTSIZE :
        case DISPID_A_FONTWEIGHT :
        case DISPID_A_FONTFACE :
        case DISPID_A_FONTSTYLE :
        case DISPID_A_FONTVARIANT :
            // invalidate the HTML element's CFPF since getCascaded* needs to
            // pick up this change.
            CElement *pHtml = GetHTML();
            if (pHtml)
                pHtml->GetFirstBranch()->VoidCachedInfo();

            RECT rect;

            rect.top    = GetTop();
            rect.left   = GetLeft();
            rect.bottom = rect.top + GetHeight();
            rect.right  = rect.left + GetWidth();

            // We need to pass a _hwnd that is used for multimonitor systems to determine
            // which monitor to use for restricting the dialog
            if (!_fTrusted)
                VerifyDialogRect(&rect, _hwndTopParent);

            MoveWindow(_hwnd,
                       rect.left,
                       rect.top,
                       rect.right-rect.left,
                       rect.bottom-rect.top,
                       TRUE);
            if ((!_fTrusted && !!_enumfStatus) ||
                (_fTrusted && _enumfStatus==HTMLDlgFlagYes))
            {
                MoveStatusWindow();
            }

            // we also want to invalidate the caches on the doc at this point...

            break;
        }
    }

    RRETURN(super::OnPropertyChange(dispid, dwFlags));
}


//
//   Get*()  helper functions. These are responsible for
//      returning the appropriate value for the property
//   GetFontSize() returns the size in TWIPS
//
//   Height, Width, Top, Left, and Font (size)
//
//===========================================================

CElement *
CHTMLDlg::GetHTML()
{
    HRESULT    hr;
    CDoc     * pDoc;
    CElement * pElemRet = NULL;

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void **)&pDoc));
    if (hr)
        goto Cleanup;
    
    if (pDoc && pDoc->_pPrimaryMarkup)
    {
        pElemRet = pDoc->_pPrimaryMarkup->GetHtmlElement();
    }

Cleanup:
    return pElemRet;
}

long
CHTMLDlg::GetFontSize(CElement * pHTML)
{
    long   lFontSize = 0;

    if (!pHTML)
        pHTML = GetHTML();

    if (pHTML)
    {
        lFontSize = pHTML->GetFirstBranch()->GetParaFormat()->_lFontHeightTwips;

        if (lFontSize <=0)
        {
            lFontSize = _lDefaultFontSize*20;
            goto Cleanup;
        }
    }

Cleanup:
    return lFontSize;
}


long
CHTMLDlg::GetTop()
{
    CElement * pElem = GetHTML();

    if (pElem)
    {
        CUnitValue cuvTop = GetAAdialogTop();
        long       lTwipsFontHeight = GetFontSize(pElem);

        if (cuvTop.GetUnitType()==CUnitValue::UNIT_NULLVALUE)
            cuvTop.SetRawValue(s_propdescCHTMLDlgdialogTop.a.ulTagNotPresentDefault);

        return cuvTop.YGetPixelValue(NULL, 100, lTwipsFontHeight);
    }

    return 0;
}

long
CHTMLDlg::GetLeft()
{
    CElement * pElem = GetHTML();

    if (pElem)
    {
        CUnitValue cuvLeft = GetAAdialogLeft();
        long       lTwipsFontHeight = GetFontSize(pElem);

        if (cuvLeft.GetUnitType()==CUnitValue::UNIT_NULLVALUE)
            cuvLeft.SetRawValue(s_propdescCHTMLDlgdialogLeft.a.ulTagNotPresentDefault);

        return cuvLeft.XGetPixelValue(NULL, 100, lTwipsFontHeight);
    }

    return 0;
}

long
CHTMLDlg::GetWidth()
{
    CElement * pElem = GetHTML();

    if (pElem)
    {
        CUnitValue cuvWidth = pElem->GetFirstBranch()->GetCascadedwidth();
        long       lTwipsFontHeight = GetFontSize(pElem);

        if (cuvWidth.GetUnitType()==CUnitValue::UNIT_NULLVALUE)
            cuvWidth.SetRawValue(s_propdescCHTMLDlgdialogWidth.a.ulTagNotPresentDefault);

        return cuvWidth.XGetPixelValue(NULL, 100, lTwipsFontHeight);
    }

    return 0;
}

long
CHTMLDlg::GetHeight()
{
    CElement * pElem = GetHTML();

    if (pElem)
    {
        CUnitValue cuvHeight = pElem->GetFirstBranch()->GetCascadedheight();
        long       lTwipsFontHeight = GetFontSize(pElem);

        if (cuvHeight.GetUnitType()==CUnitValue::UNIT_NULLVALUE)
            cuvHeight.SetRawValue(s_propdescCHTMLDlgdialogHeight.a.ulTagNotPresentDefault);

        return cuvHeight.YGetPixelValue(NULL, 100, lTwipsFontHeight);
    }

    return 0;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::LoadDocSynchronous
//
//  Synopsis:   Load the inner document synchronously.  Lovely hack, huh?
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::LoadDocSynchronous(IStream *pStm, TCHAR *pchUrl)
{
    HRESULT     hr;
    CDoc *      pDoc;
    TCHAR *     pchCodePage = NULL;
    CODEPAGE    cp = 0;

    Assert(_pUnkObj);

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void **)&pDoc));
    if (hr)
        goto Cleanup;

    //
    // Again, just a reminder that the only reason this call
    // exists is to force the doc to have a valid base url set in
    // the synchronous load case.  Since we use IPersistStreamInit
    // to load, the doc will never know how to resolve relative
    // references unless we do this or a base tag exists in
    // the html.  We don't want to force everyone to have a base
    // tag, hence this hack.
    //

    hr = THR(pDoc->SetUrl(pchUrl));
    if (hr)
        goto Cleanup;

    if (_fPropPageMode)
    {
        //
        // In proppage mode, just go load up the codepage from the
        // resource dll and convert it to a dwCodePage.  Use this
        // to prime the parser.  This is acceptable because we don't
        // expose any mechanism for third parties to create property
        // pages using trident.
        //

        hr = THR(Format(
                FMT_OUT_ALLOC,
                &pchCodePage,
                0,
                MAKEINTRESOURCE(IDS_CODEPAGE)));
        if (hr)
            goto Cleanup;

        cp = CodePageFromString(pchCodePage, FALSE);
    }

    hr = THR(pDoc->LoadFromStream(pStm, TRUE, cp));
    if (hr)
        goto Cleanup;

Cleanup:
    delete pchCodePage;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::SetTrustedOnDoc
//
//  Synopsis:   Set Doc._fInTrustedHTMLDlg based on the trusted state
//              of the dialog
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::SetTrustedOnDoc(HTMLDLGINFO * pdlginfo)
{
    HRESULT hr;
    CDoc *  pDoc;

    Assert(_pUnkObj);

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void **)&pDoc));
    if (hr)
        goto Cleanup;

    pDoc->_fInTrustedHTMLDlg = _fTrusted;

    // Also set the flag that indicates that the document is a dialog
    // We need to set this so the size is applyed correctly
    pDoc->_fInHTMLDlg = TRUE;
    pDoc->_fInHTMLPropPage = pdlginfo->fPropPage;

Cleanup:
    RRETURN(hr);
}


//+--------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::EnsureLayout
//
//  Synopsis:   Ensure view is ready before clearing _fInitializing
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::EnsureLayout()
{
    HRESULT hr;
    CDoc *  pDoc;

    Assert(_pUnkObj);

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void **)&pDoc));
    if (hr)
        goto Cleanup;

    pDoc->GetView()->EnsureView(LAYOUT_SYNCHRONOUS);

Cleanup:
    RRETURN(hr);
    
}
