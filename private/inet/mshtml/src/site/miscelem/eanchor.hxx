//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eanchor.hxx
//
//  Contents:   CAnchorElement class
//
//  History:    4-Apr-1996  AnandRa     Created
//
//----------------------------------------------------------------------------

#ifndef I_EANCHOR_HXX_
#define I_EANCHOR_HXX_
#pragma INCMSG("--- Beg 'eanchor.hxx'")

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#define _hxx_
#include "anchor.hdl"

MtExtern(CAnchorElement)

class CAnchorElement : public CHyperlink
{
    DECLARE_CLASS_TYPES(CAnchorElement, CHyperlink)

    friend class CDBindMethodsAnchor;
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAnchorElement))

    CAnchorElement (CDoc *pDoc)
      : super(ETAG_A, pDoc) 
    {
#ifdef WIN16
	    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }

    ~CAnchorElement() {};

    static HRESULT CreateElement(CHtmTag *pht,
                                 CDoc *pDoc, CElement **ppElementResult);

    HRESULT EnterTree();
    
    virtual void    Notify(CNotification *pNF);
    
    HRESULT ApplyDefaultFormat (CFormatInfo *pCFI);
    COLORREF GetLinkColor();

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    // BUGBUG: (anandra) This function is necessary because of the
    // differences in arguments between HandleMessage and HandleElementMessage
    // Think about remove these distinctions.
    NV_DECLARE_MOUSECAPTURE_METHOD(OnCaptureMessage, oncapturemessage, (CMessage *pMessage));

    virtual HRESULT YieldCurrency(CElement * pElemNew)
    {
        if (GetFirstBranch())
        {
            SetActive(FALSE);
            UpdateFormats(GetFirstBranch());
        }
        RRETURN(super::YieldCurrency(pElemNew));
    }

    HRESULT ExecPseudoClassEffect(BOOL fVisited, BOOL fActive,
                                  BOOL fOldVisited, BOOL fOldActive);

    virtual HRESULT BUGCALL HandleMessage(CMessage *pmsg);

    virtual HRESULT ClickAction (CMessage *pmsg);

    STDMETHOD(QueryStatus) (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);
    STDMETHOD(Exec) (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);
            
    // Helper functions
    virtual HRESULT DoClick(CMessage * pMessage = NULL, CTreeNode *pNodeContext = NULL,
        BOOL fFromLabel = FALSE);

    // URL accessors - CHyperlink overrides
    virtual HRESULT SetUrl(BSTR bstrUrl);
    virtual LPCTSTR GetUrl() const;
    virtual LPCTSTR GetTarget() const;
    virtual HRESULT GetUrlTitle(CStr *pstr);

    static const CLSID *            s_apclsidPages[];

    void SetNeedClosingQuote ( BOOL fNeedClosingQuote )
        { _fNeedClosingQuote = fNeedClosingQuote; }
    
    BOOL GetNeedClosingQuote ( )
        { return _fNeedClosingQuote; }

    #define _CAnchorElement_
    #include "anchor.hdl"

    // interface baseimplementation property prototypes
    //NV_DECLARE_TEAROFF_METHOD(put_href, PUT_href, (BSTR v));
    //NV_DECLARE_TEAROFF_METHOD(get_href, GET_href, (BSTR * p));

    // 
    inline BOOL IsVisited() {return( _fVisited ? TRUE : FALSE) ; }
    inline BOOL IsActive() {return( _fActive ? TRUE : FALSE) ; }
    inline BOOL IsHovered() {return( _fHovered ? TRUE : FALSE) ; }
    HRESULT SetActive( BOOL fActive );
    virtual HRESULT SetVisited();

    virtual HRESULT ShowTooltip(CMessage *pmsg, POINT pt);

#ifndef NO_DATABINDING
    // databinding
    virtual const CDBindMethods *GetDBindMethods();
#endif // ndef NO_DATABINDING


    // incremental format cache updating
    HRESULT UpdateFormats(CTreeNode * pNodeContext);
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    
    HRESULT UpdateAnchorFromHref(); // Determines whether anchor's text should match the href

    BOOL _fVisitedValid:1;
    BOOL _fVisited:1;
    BOOL _fActive:1;
    BOOL _fNeedClosingQuote:1;
    BOOL _fHovered:1;            // mouse if over the link

    NO_COPY(CAnchorElement);
};

#pragma INCMSG("--- End 'eanchor.hxx'")
#else
#pragma INCMSG("*** Dup 'eanchor.hxx'")
#endif
