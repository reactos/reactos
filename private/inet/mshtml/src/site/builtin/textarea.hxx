//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       textarea.hxx
//
//  Contents:   CTextarea, CRichtext
//
//----------------------------------------------------------------------------

#ifndef I_TEXTAREA_HXX_
#define I_TEXTAREA_HXX_
#pragma INCMSG("--- Beg 'textarea.hxx'")

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

// BUGBUG: These should be in a header file common to text areas and inputs (brendand)
#define TEXT_INSET_DEFAULT_TOP      1
#define TEXT_INSET_DEFAULT_BOTTOM   1
#define TEXT_INSET_DEFAULT_RIGHT    1
#define TEXT_INSET_DEFAULT_LEFT     1

#define _hxx_
#include "textarea.hdl"

class CTxtSite;
class CTextAreaLayout;
class CRichtextLayout;

//+---------------------------------------------------------------------------
//
// CRichtext: HTML textarea
//
//----------------------------------------------------------------------------

MtExtern(CRichtext)
MtExtern(CTextArea)

class CRichtext: public CTxtSite
{
    friend class CDBindMethodsTextarea;
    DECLARE_CLASS_TYPES(CRichtext, CTxtSite)

public:
    CRichtext(ELEMENT_TAG etag, CDoc *pDoc): CTxtSite(etag, pDoc)
    {
        _vStatus.vt = VT_NULL;
        _dwLastCP = 0;
    };

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);

    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
    {
        // Never want a focus rect
        Assert(ppShape);
        *ppShape = NULL;
        return S_FALSE;
    }

    DECLARE_LAYOUT_FNS(CRichtextLayout)

    virtual HRESULT SetDefaultValueHelper(CStr &cstrDefault) { return _cstrDefaultValue.Set(cstrDefault); }
    virtual HRESULT SetDefaultValueHelper(TCHAR * psz) { return _cstrDefaultValue.Set(psz); }

    virtual HRESULT DoReset(void);

    //   property helpers
    NV_DECLARE_PROPERTY_METHOD(GetValueHelper, GETValueHelper, (CStr * pbstr));
    NV_DECLARE_PROPERTY_METHOD(SetValueHelper, SETValueHelper, (CStr * bstr));
    virtual HRESULT GetSubmitValue(CStr *pstr);
    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData);

    HRESULT SetValueHelperInternal(CStr *pstr, BOOL fOM = TRUE);
    
    //+-----------------------------------------------------------------------
    //  CTxtSite methods
    //------------------------------------------------------------------------
    virtual BOOL GetWordWrap() const;
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);
    virtual HRESULT BUGCALL  HandleMessage(CMessage * pMessage);
    //+-----------------------------------------------------------------------
    // Helper methods
    //------------------------------------------------------------------------

    //virtual HRESULT LoadHistoryValue();
    virtual HRESULT SaveHistoryValue(CHistorySaveCtx *phsc);
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT DelayLoadHistoryValue();

    //+------------------------------------------------------------------------
    // CElement methods
    //-----------------------------------------------------------------------
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

#ifndef NO_DATABINDING
    // Data binding
    virtual const CDBindMethods *GetDBindMethods();
#endif

    virtual HRESULT YieldCurrency(CElement *pElemNew);
    virtual HRESULT RequestYieldCurrency(BOOL fForce);
    virtual HRESULT BecomeUIActive();
    //+-----------------------------------------------------------------------
    //  Interface methods
    //------------------------------------------------------------------------

    // IOleCommandTarget methods

    HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD  nCmdID,
            DWORD  nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    // default value html property
    CStr        _cstrDefaultValue;
    CStr                _cstrLastValue;
    DWORD       _dwLastCP;
    unsigned    _fTextChanged:1;
    unsigned    _fFiredValuePropChange:1;
    unsigned    _fDoReset:1;
    unsigned    _fInSave:1;
    unsigned    _fLastValueSet:1;
    unsigned    _fChangingEncoding:1;
    unsigned    _iHistoryIndex:16;

    #define _CRichtext_
    #include "textarea.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    // History support
    IStream *   _pStreamHistory;
    VARIANT     _vStatus;

    static ACCELS s_AccelsTextareaDesign;
    static ACCELS s_AccelsTextareaRun;

private:
    NO_COPY(CRichtext);
};

//+---------------------------------------------------------------------------
//
// CTextarea: plain text textarea
//
//----------------------------------------------------------------------------

class CTextArea : public CRichtext
{
    DECLARE_CLASS_TYPES(CTextArea, CRichtext)

    friend class CHTMLTextAreaParser;

public:

    CTextArea (ELEMENT_TAG etag, CDoc *pDoc) : CRichtext(etag, pDoc)
    {
#ifdef WIN16
            m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }
    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );

    DECLARE_LAYOUT_FNS(CTextAreaLayout)

    long GetPlainTextLengthWithBreaks();
    void GetPlainTextWithBreaks(TCHAR * pchBuff);

    //   property helpers
    virtual HRESULT GetSubmitValue(CStr *pstr);

    //+------------------------------------------------------------------------
    // CElement methods
    //-----------------------------------------------------------------------
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);


    // default value html property
    CStr        _cstrDefaultValue;

    #define _CTextArea_
    #include "textarea.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CTextArea);
};

#pragma INCMSG("--- End 'textarea.hxx'")
#else
#pragma INCMSG("*** Dup 'textarea.hxx'")
#endif
