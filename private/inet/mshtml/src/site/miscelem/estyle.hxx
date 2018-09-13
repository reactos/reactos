//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       style.hxx
//
//  Contents:   CStyleElement
//
//----------------------------------------------------------------------------

#ifndef I_ESTYLE_HXX_
#define I_ESTYLE_HXX_
#pragma INCMSG("--- Beg 'estyle.hxx'")

#define _hxx_
#include "estyle.hdl"

MtExtern(CStyleElement)

class CStyleSheet;

//+---------------------------------------------------------------------------
//
// CStyleElement
//
//----------------------------------------------------------------------------

class CStyleElement : public CElement
{
    DECLARE_CLASS_TYPES(CStyleElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStyleElement))

    CStyleElement(CDoc *pDoc);

    virtual void Passivate(void);

    virtual ~CStyleElement() {}

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

    static HRESULT  CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT Clone(CElement **ppElementClone, CDoc *pDoc);
    HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    HRESULT         Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);
    HRESULT         SetText(TCHAR *pch);
    HRESULT         SetReadyStateStyle(long readyStateStyle);
    void            OnReadyStateChange();

    // Makes sure that we have a stylesheet built for this element.
    HRESULT EnsureStyleSheet( void );
    // Force us to persist by looking at the internal data, instead of storing a string
    void SetDirty( void );

    CStr         _cstrText;
    CStyleSheet *_pStyleSheet;
    unsigned     _fParseFinished  : 1;  // indicates that parseCtx::Finsish has been called
    unsigned     _fEnterTreeCalled: 1;  // indicates this is in the primary tree

    #define _CStyleElement_
    #include "estyle.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

    NO_COPY(CStyleElement);


    BOOL    _fDirty;

    unsigned _readyStateStyle : 3;
    unsigned _readyStateFired : 3;

    CStyleSheetArray * _pSSATemp;       // Temporary use for when the Style element is not in a Markup
                                        // but we still want to do stuff like AddRules or CreateStyleSheet
};
#pragma INCMSG("--- End 'estyle.hxx'")
#else
#pragma INCMSG("*** Dup 'estyle.hxx'")
#endif

