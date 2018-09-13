//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ELink.hxx
//
//  Contents:   CLinkElem
//
//----------------------------------------------------------------------------

#ifndef I_ELINK_HXX_
#define I_ELINK_HXX_
#pragma INCMSG("--- Beg 'elink.hxx'")

#define _hxx_
#include "link.hdl"

MtExtern(CLinkElement)

class CStyleSheet;

//+---------------------------------------------------------------------------
//
// CLinkElement implementation for linking stylesheets, associated documents, etc.
//
//----------------------------------------------------------------------------
class CLinkElement : public CElement
{
public:
    DECLARE_CLASS_TYPES(CLinkElement, CElement)
    
    enum LINKTYPE
    {
        LINKTYPE_UNKNOWN,           // link type unknown
        LINKTYPE_STYLESHEET         // link to stylesheet
    };

    //
    // construction / destruction
    //

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLinkElement))

    CLinkElement(CDoc *pDoc);

    //
    // misc
    //
    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    // InvokeEx to validate ready state.
    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeExReady, invokeexready, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider));

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (BSTR *pBSTR));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT *pReadyState));
    NV_DECLARE_TEAROFF_METHOD(get_readyStateValue, get_readyStateValue, (long *plRetValue));

    static	HRESULT	CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);
    virtual void    Passivate(void);

    void    SetBitsCtx(CBitsCtx * pBitsCtx);
    void    SetActivity();
    void    OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CLinkElement *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }
    HRESULT SetReadyStateLink(long readyStateLink);
    void OnReadyStateChange();

	HRESULT HandleLinkedObjects();  // helper for OnPropertyChange
    HRESULT EnsureStyleDownload();

    LINKTYPE GetLinkType();

    //
    // standard wiring
    //

    #define _CLinkElement_
    #include "link.hdl"

	DECLARE_CLASSDESC_MEMBERS;

    //
    // data
    //

	CStyleSheet     *_pStyleSheet;  // If we link a stylesheet, this points to it.

    CBitsCtx *                  _pBitsCtx;
	BOOL                        _fIsActive;
    BOOL                        _fIsInitialized;
    DWORD                       _dwStyleCookie;
    DWORD                       _dwScriptDownloadCookie;
    
    unsigned                    _readyStateLink : 3;
    unsigned                    _readyStateFired : 3;

    NO_COPY(CLinkElement);

    CStyleSheetArray * _pSSATemp;       // Temporary use for when the Style element is not in a Markup
                                        // but we still want to do stuff like AddRules or CreateStyleSheet
};

#pragma INCMSG("--- End 'elink.hxx'")
#else
#pragma INCMSG("*** Dup 'elink.hxx'")
#endif
