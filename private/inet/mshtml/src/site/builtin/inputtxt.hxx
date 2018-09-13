//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       inputtxt.hxx
//
//  Contents:   CInput, etc...
//
//----------------------------------------------------------------------------

#ifndef I_INPUTTXT_HXX_
#define I_INPUTTXT_HXX_
#pragma INCMSG("--- Beg 'inputtxt.hxx'")

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

// BUGBUG: These should be in a header file common to text areas and inputs (brendand)
#define TEXT_INSET_DEFAULT_TOP      1
#define TEXT_INSET_DEFAULT_BOTTOM   1
#define TEXT_INSET_DEFAULT_RIGHT    1
#define TEXT_INSET_DEFAULT_LEFT     1

#define _hxx_
#include "inputtxt.hdl"

MtExtern(CRadioGroupAry)
MtExtern(CRadioGroupAry_pv)

class CRadioGroupAry;
class CImgHelper;
class CInputButtonLayout;
class CInputLayout;
class CInputTextLayout;
class CInputFileLayout;
class CCheckboxLayout;

//+---------------------------------------------------------------------------
//
// CInput
//
//----------------------------------------------------------------------------

HRESULT 
CreateInputElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);

BOOL IsTypeMultiline(htmlInput type);

class CInput : public CSite,
                    public CBtnHelper
{
private:
    typedef CSite super;
    friend class CDBindMethodsInputTxtBase;
    friend class CDBindMethodsCheckbox;
    friend class CDBindMethodsRadio;
    friend class CInputSlave;
    friend class CInputSlaveLayout;
    friend class CInputButtonLayout;
    friend class CInputFileLayout;
    friend class CCheckboxLayout;

    DECLARE_CLASS_TYPES(CInput, CSite)

public:
    CInput (ELEMENT_TAG etag, CDoc *pDoc, htmlInput type);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static HRESULT CreateElement(CHtmTag    *pht,
                                 CDoc       *pDoc, 
                                 CElement   **ppElementResult,
                                 htmlInput  type = htmlInputText);
    virtual HRESULT Init();
    virtual HRESULT Init2(CInit2Context * pContext);

    HRESULT EnterTree();
    virtual void Passivate();

    virtual void    Notify(CNotification *pNF);

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    
    HRESULT BUGCALL HandleButtonMessage(CMessage * pMessage);
    
    HRESULT BUGCALL HandleTextMessage(CMessage * pMessage);
    
    HRESULT BUGCALL HandleImageMessage(CMessage * pMessage);

    HRESULT BUGCALL HandleFileMessage(CMessage * pMessage);

    HRESULT BUGCALL HandleCheckboxMessage(CMessage * pMessage);

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

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
    HRESULT SaveCheckbox ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
    virtual HRESULT ClickAction(CMessage * pMessage);
    HRESULT ClickActionButton(CMessage * pMessage);
    HRESULT ClickActionFile(CMessage * pMessage);
    HRESULT ClickActionImage(CMessage * pMessage);

    virtual HRESULT DoClick(CMessage * pMessage = NULL, CTreeNode *pNodeContext = NULL,
        BOOL fFromLabel = FALSE);
    HRESULT DoClickCheckbox(CMessage * pMessage = NULL, CTreeNode *pNodeContext = NULL,
        BOOL fFromLabel = FALSE);

    virtual GLYPHSTYLE      BtnStyle(void)            
    {
        switch (GetType())
        {
        case htmlInputCheckbox:
            return GLYPHSTYLE_CHECK;
        case htmlInputRadio:
            return GLYPHSTYLE_OPTION;
        default:
            AssertSz(FALSE, "This Input doesn't have a button style");
        }
        return GLYPHSTYLE_NONE;
    }

    virtual HRESULT ApplyDefaultFormat (CFormatInfo *pCFI);
    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData);
    HRESULT GetSubmitInfoForImg(CPostData * pSubmitData);

    virtual DWORD   GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE);

    STDMETHOD(GetEnabled)(VARIANT_BOOL * pfEnabled);

    //   property helpers
    NV_DECLARE_PROPERTY_METHOD(GetValueHelper, GETValueHelper, (CStr * pbstr));
    NV_DECLARE_PROPERTY_METHOD(SetValueHelper, SETValueHelper, (CStr * bstr));
    NV_DECLARE_TEAROFF_METHOD(put_status, PUT_status, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(get_status, GET_status, (VARIANT *p));

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    // History support
    HRESULT LoadHistoryValue();

    virtual DWORD   GetInfo(GETINFO gi);

    //+-----------------------------------------------------------------------
    // Currency/ui ativation overrides.
    //------------------------------------------------------------------------

    virtual HRESULT YieldCurrency(CElement *pElemNew);
    virtual HRESULT RequestYieldCurrency(BOOL fForce);
    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual HRESULT BecomeUIActive();

    virtual HRESULT GetChecked (VARIANT_BOOL * pchecked);
    virtual HRESULT SetChecked (VARIANT_BOOL checked, BOOL fSaveData=TRUE);

#ifndef NO_DATABINDING
    // Data binding
    virtual const CDBindMethods *GetDBindMethods();
#endif

    //  CBtnHelper methods
    virtual CElement * GetElement ( void ){ return this; }
    virtual CBtnHelper * GetBtnHelper()
    {
       return (CBtnHelper *) this;
    }

    //  internal helpers
    HRESULT SetValueHelperInternal(CStr *pstr, BOOL fOM = TRUE);
    HRESULT SetValueHelper(const TCHAR *psz, int c, BOOL fOM = TRUE);
    HRESULT BUGCALL GetValueHelper(CStr * pbstr, BOOL fIsSubmit);
    HRESULT SetValueHelperReal(const TCHAR *psz, int c, BOOL fAsciiOnly = FALSE, BOOL fOM = TRUE);

    // Helpers
    HRESULT RenderGlyph(CFormDrawInfo * pDI, LPRECT prc);

    // baseimplementation properties/methods
    NV_DECLARE_PROPERTY_METHOD(put_checked, PUT_checked, (VARIANT_BOOL checked));
    NV_DECLARE_PROPERTY_METHOD(get_checked, GET_checked, (VARIANT_BOOL * pchecked));
    NV_DECLARE_PROPERTY_METHOD(SetIndeterminateHelper, SETIndeterminateHelper, (long indeterminate));
    NV_DECLARE_PROPERTY_METHOD(GetIndeterminateHelper, GETIndeterminateHelper, (long * pindeterminate));

    BOOL IsButton()
    {
        htmlInput type = GetType();
        return (type == htmlInputButton 
            || type == htmlInputReset 
            || type == htmlInputSubmit);
    }

    BOOL IsOptionButton()
    {
        htmlInput type = GetType();
        return (type == htmlInputCheckbox
            || type == htmlInputRadio);
    }

    BOOL IsTextOrFile()
    {
        htmlInput type = GetType();
        return (type == htmlInputText || type == htmlInputHidden || type == htmlInputPassword || type == htmlInputFile);
    }

    CStr * GetLastValue()
    {
        return (_fLastValueSet ? &_cstrLastValue: &_cstrDefaultValue);
    }

    htmlInput   GetType() const             { return (htmlInput)_type; }
    void        SetType(htmlInput type)     { Assert((unsigned)type < 16); _type = type; }
    void        SetTypeAtCreate(htmlInput type)
        { _typeAtCreate = type; }

    DECLARE_LAYOUT_FNS(CLayout)
    // InvokeEx to validate ready state.
    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeExReady, invokeexready, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider));

    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT putHeight(long l);
    HRESULT putWidth(long l);
    HRESULT GetHeight(long *pl);
    HRESULT GetWidth(long *pl);

    STDMETHOD(put_hspace)(long v);
    STDMETHOD(get_hspace)(long * p);

    //HRESULT STDMETHODCALLTYPE put_ie4vtblslot(BSTR p);

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (BSTR *pBSTR));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT *pReadyState));
    NV_DECLARE_TEAROFF_METHOD(get_readyStateValue, get_readyStateValue, (long *plRetValue));

    NV_DECLARE_TEAROFF_METHOD(get_onload, GET_onload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onload, PUT_onload, (VARIANT));

    NV_DECLARE_TEAROFF_METHOD(put_src, PUT_src, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_src, GET_src, (BSTR * p));
    HRESULT STDMETHODCALLTYPE put_height(long v);
    HRESULT STDMETHODCALLTYPE get_height(long*p);
    HRESULT STDMETHODCALLTYPE put_width(long v);
    HRESULT STDMETHODCALLTYPE get_width(long*p);
    HRESULT STDMETHODCALLTYPE put_align(BSTR v);
    HRESULT STDMETHODCALLTYPE get_align(BSTR*p);

    BOOL    IsHSpaceDefined() const ;

    virtual HRESULT ShowTooltip(CMessage *pmsg, POINT pt);

    virtual HRESULT DoReset(void);

    NV_DECLARE_VISIT_METHOD(FindBoundRadioElement, findboundradioelement, (DWORD_PTR dw))

    // CSite databinding OverRides
    virtual CElement *GetElementDataBound();

    void    NavigateToNext(CMessage * pMessage, BOOL fForward);

    // Visit Methods
    NV_DECLARE_VISIT_METHOD(Clear, clear, (DWORD_PTR dw))
    NV_DECLARE_VISIT_METHOD(SetByValue, setbyvalue, (DWORD_PTR dw))
    NV_DECLARE_VISIT_METHOD(GetValue, getvalue, (DWORD_PTR dw))
    NV_DECLARE_VISIT_METHOD(GetNext, getnext, (DWORD_PTR dw))

    HRESULT BUGCALL GetTabStop(DWORD dw);
    BOOL FInSameGroup(CElement * pElem);
    HRESULT ClearGroup ( CRadioGroupAry * );
    HRESULT TraverseGroup(LPCTSTR strGroupName, PFN_VISIT pfn, DWORD_PTR dw, BOOL fForward);


    HRESULT AddRadioGroup(LPCTSTR lpstrGrpName);
    HRESULT DelRadioGroup(LPCTSTR lpstrGrpName);
    HRESULT ChkRadioGroup(LPCTSTR lpstrGrpName);

protected:
    //  Fake button helpers
    void    BtnHelperSetFocus(void);
    void    BtnHelperKillFocus(void);

public:
    static const CLSID *            s_apclsidPages[];
    static PROP_DESC                s_apropdesc[];

    // _cstrLastValue is non NULL iff the value attribute has been
    // explicitly set in the HTML.
    // If _cstrLastValue is NULL, different types of input controls are
    // given different defaults for the value attribute in Init2().
    // Hidden input     : "  "    (two spaces for N/S compatibility)
    // Submit button    : "submit"
    // Reset button     : "reset"
    // Any other button : "button"
    // All others       : ""
    CStr    _cstrLastValue;

    // default value html property
    CStr    _cstrDefaultValue;

    VARIANT  _vStatus;

    static LONG cxButtonSpacing;

    unsigned    _fChecked               :1; // 0 Are we checked?
    unsigned    _fLastValue             :1; // 1 Should we check the last value?
    unsigned    _fTextChanged           :1; // 2 Has the text changed?
    unsigned    _fFiredValuePropChange  :1; // 3 Have we fired property change for value
    unsigned    _fInSave                :1; // 4 Are we saving?
    unsigned    _fHasInitValue          :1; // 5 Do we have an initial value?
    unsigned    _fLastValueSet          :1; // 6 Has the last value been set? BUGBUG: (krisma) Is this the same as _fLastValue?
    unsigned    _type                   :4; // 7 - 10 What type of input are we?
    unsigned    _fHasMorphed            :1; // 11 FALSE (default) if this element has morphed to another type.
    unsigned    _fButtonHasFocus        :1; // 12 Does the button on the File input have focus?
    unsigned    _fCaretHidden           :1; // 13 Are we hiding the caret?
    unsigned   _fDeferClearGroup        :1; // 14 Used to defer clearing the other radio buttons in the group, when SetChecked() is called from Init2(). This would prevent the collections from being accessed prematurely.
    unsigned    _fDoReset               :1; // 15 ESC support
    unsigned    _typeAtCreate           :4; // 16 - 19 What type of input are we?
    unsigned    _fScriptCreated         :1; // 20 input created through parser
    unsigned    _fDirtiedByOM           :1; // 21 input file dirtied by OM
    unsigned    _fRealBorderSize        :1; // 22 get real border size instead
                                            // of the fake border size.
    unsigned    _fChangingEncoding      :1; // 23 Encoding is changing.

    // for input image only
    POINT       _pt;
    CImgHelper *_pImage;

    SHORT       _icfButton;               
    unsigned short    _iHistoryIndex;               

    #define _CInput_
    #include "inputtxt.hdl"

protected:
    static ACCELS s_AccelsInputTxtDesign;
    static ACCELS s_AccelsInputTxtRun;

    static CLASSDESC    s_classdescHidden;
    static CLASSDESC    s_classdescPassword;
    static CLASSDESC    s_classdescText;

    static CLASSDESC    s_classdescBtn;
    static CLASSDESC    s_classdescTagButton;   // not used
    static CLASSDESC    s_classdescSubmit;
    static CLASSDESC    s_classdescReset;
    static CLASSDESC    s_classdescFile;
    static CLASSDESC    s_classdescCheckbox;

    static CLASSDESC    s_classdescImage;

    virtual const CBase::CLASSDESC *GetClassDesc() const;

    // History support
    IStream *   _pStreamHistory;


private:
    HRESULT PrivateInit2();
    HRESULT CreateSlave();
    NO_COPY(CInput);
};

struct RadioGroupEntry
{
    CInput *        _pRadio;
    CFormElement *  _pParentForm;
};

DECLARE_CDataAry(CRadioGroupAry, RadioGroupEntry, Mt(CRadioGroupAry), Mt(CRadioGroupAry_pv))

#pragma INCMSG("--- End 'inputtxt.hxx'")
#else
#pragma INCMSG("*** Dup 'inputtxt.hxx'")
#endif
