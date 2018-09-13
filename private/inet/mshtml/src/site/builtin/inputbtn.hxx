//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       InputBtn.hxx
//
//  Contents:   CButton, etc...
//
//----------------------------------------------------------------------------

#ifndef I_INPUTBTN_HXX_
#define I_INPUTBTN_HXX_
#pragma INCMSG("--- Beg 'inputbtn.hxx'")

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#define _hxx_
#include "inputbtn.hdl"

class CButtonLayout;

MtExtern(CButton)

class CButton : public CTxtSite,
                public CBtnHelper
{
    DECLARE_CLASS_TYPES(CButton, CTxtSite)
    friend class CButtonLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CButton))

    CButton (ELEMENT_TAG etag, CDoc *pDoc)
    : CTxtSite(etag, pDoc)
    {
#ifdef WIN16
        m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
        m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
        _vStatus.vt = VT_NULL;
        _fActsLikeButton = TRUE;
    }

    ~CButton() {}

    DECLARE_LAYOUT_FNS(CButtonLayout)

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);

    //   property helpers
    NV_DECLARE_PROPERTY_METHOD(GetValueHelper, GETValueHelper, (CStr * pbstr));
    NV_DECLARE_PROPERTY_METHOD(SetValueHelper, SETValueHelper, (CStr * bstr));

    HRESULT SetValueHelper(const TCHAR *psz, int c);
    virtual DWORD   GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE);

    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData);

    virtual HRESULT ApplyDefaultFormat (CFormatInfo *pCFI);
    STDMETHOD(GetEnabled)(VARIANT_BOOL * pfEnabled);

    virtual void    Notify(CNotification *pNF);
    virtual HRESULT YieldCurrency(CElement*pElemNew);
    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);

    //+----------------------------------------------------------------------
        //  BtnHelper methods
        //-----------------------------------------------------------------------

    virtual CElement * GetElement ( void ) { return this; }
    virtual HRESULT ClickAction(CMessage * pMessage);
    virtual HRESULT DoClick(CMessage * pMessage = NULL, CTreeNode *pNodeContext = NULL,
        BOOL fFromLabel = FALSE);
    virtual CBtnHelper * GetBtnHelper()
    {
       return (CBtnHelper *) this;
    }

    //+-----------------------------------------------------------------------
    // Hit test
    //------------------------------------------------------------------------

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    //virtual HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);


    //+-----------------------------------------------------------------------
    // JS methods
    //------------------------------------------------------------------------
//    HRESULT BUGCALL click(void);


#ifndef NO_DATABINDING
    virtual const CDBindMethods * GetDBindMethods ();
#endif

    #define _CButton_
    #include "inputbtn.hdl"

protected:

    static CLASSDESC    s_classdescTagButton;
    static CLASSDESC    s_classdescButtonReset;
    static CLASSDESC    s_classdescButtonSubmit;
    virtual const CBase::CLASSDESC *GetClassDesc() const;


private:
    NO_COPY(CButton);
    VARIANT  _vStatus;
};

#pragma INCMSG("--- End 'inputbtn.hxx'")
#else
#pragma INCMSG("*** Dup 'inputbtn.hxx'")
#endif
