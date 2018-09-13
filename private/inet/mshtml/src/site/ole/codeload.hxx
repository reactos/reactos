//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       codeload.hxx
//
//  Contents:   CCodeLoad
//
//----------------------------------------------------------------------------

#ifndef I_CODELOAD_HXX_
#define I_CODELOAD_HXX_
#pragma INCMSG("--- Beg 'codeload.hxx'")

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

MtExtern(CBindContextParam)
MtExtern(CCodeLoad)

HRESULT AddBindContextParam(CDoc *pDoc,
                            IBindCtx *pbctx,
                            COleSite *pOleSite = NULL,
                            IPropertyBag *pPropBag = NULL);

///////////////////////////////////////////////////////////////////////////////////////////
//
// CBindContextParam
//
//

class CBindContextParam : public IOleCommandTarget
{

public:
    // construction / initialization / destruction

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBindContextParam))

    CBindContextParam();

    HRESULT Init(TCHAR * pchBaseUrl, IPropertyBag *pPropBag);

    // IUnknown
    DECLARE_FORMS_STANDARD_IUNKNOWN (CBindContextParam);

    // IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus(
        const GUID *    pguidCmdGroup,
        ULONG           cCmds,
        OLECMD          prgCmds[],
        OLECMDTEXT *    pCmdText)
        { return E_NOTIMPL; };

    HRESULT STDMETHODCALLTYPE Exec(
        const GUID *    pguidCmdGroup,
        DWORD           nCmdID,
        DWORD           nCmdexecopt,
        VARIANT *       pvarIn,
        VARIANT *       pvarOut);

    // data
    CStr            _cstrBaseUrl;
    IPropertyBag *  _pPropBag;
};

#define KEY_BINDCONTEXTPARAM            _T("BIND_CONTEXT_PARAM")


class CCodeLoad : public CDwnBindInfo, public IWindowForBindingUI
{
    typedef CDwnBindInfo super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCodeLoad))

    CCodeLoad();
    HRESULT Init(COleSite *pSiteOle, COleSite::OLECREATEINFO *pinfo);
    void Terminate();

    // IUnknown overrides

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)()  { return(super::AddRef()); }
    STDMETHOD_(ULONG,Release)() { return(super::Release()); }

    // IBindStatusCallback overrides
    
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding *pbinding);
    STDMETHOD(OnStopBinding)(HRESULT hrErr, LPCWSTR szErr);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);
    STDMETHOD(GetBindInfo)(DWORD *pdwBindf, BINDINFO *pbindinfo);

    // IServiceProvider overrides

    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppvObj);

    // IWindowForBindingUI methods

    STDMETHOD(GetWindow)(REFGUID rguidReason, HWND *phwnd);

private:

    // Private methods

    HRESULT BindToObject();
    HRESULT CreateStreamFromData();
    
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CCodeLoad *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }

    class CLock
    {
    public:
        CLock(CCodeLoad * pCodeLoad)
            { _pCodeLoad = pCodeLoad; pCodeLoad->AddRef(); }
        ~CLock()
            { _pCodeLoad->Release(); }

        CCodeLoad * _pCodeLoad;
    };

    // Data members

    IBinding *                  _pbinding;
    IBindCtx *                  _pbctx;
    COleSite *                  _pSiteOle;
    CBitsCtx *                  _pBitsCtx;      // The bits context for
                                                //   data downloading
    COleSite::OLECREATEINFO     _info;          // The creation info
    IUnknown *                  _punkObject;    // downloaded object
    IID                         _iidObject;     // iid of downloaded object (we are specifically
                                                // paying attention to case when the object is
                                                // IID_IClassFactory)
                                                //   if available.
    IProgSink *                 _pProgSink;     // progress sink.
    DWORD                       _dwProgCookie;  // cookie for progress sink.
    DWORD                       _dwScriptCookie;// cookie for EnterScriptDownload

    BOOL                        _fGotObject:1;  // OnObjectAvailable called
    BOOL                        _fGotData:1;    // Data is available.
    BOOL                        _fGetClassObject:1; // Needs class factory.
};

#pragma INCMSG("--- End 'codeload.hxx'")
#else
#pragma INCMSG("*** Dup 'codeload.hxx'")
#endif
