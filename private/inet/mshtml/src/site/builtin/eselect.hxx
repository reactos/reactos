//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eselect.hxx
//
//  Contents:   CSelectElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_ESELECT_HXX_
#define I_ESELECT_HXX_
#pragma INCMSG("--- Beg 'eselect.hxx'")

#if defined(PRODUCT_PROF) && !defined(_MAC)
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
#else
#define StartCAP()
#define StopCAP()
#define SuspendCAP()
#define ResumeCAP()
#endif

#define _hxx_
#include "select.hdl"

MtExtern(CSelectElement)
MtExtern(CSelectElement_aryOptions_pv)

#define DEFAULT_COMBO_ITEMS         13

//+---------------------------------------------------------------------------
//
// CSelectElement
//
//----------------------------------------------------------------------------

class CCcs;
class CSelectLayout;
class CSelectElement : public CSite
{
    DECLARE_CLASS_TYPES(CSelectElement, CSite)

    friend class COptionElement;
    friend class CHtmSelectParseCtx;
    friend class CDBindMethodsSelect;
    friend class CSelectLayout;
    friend LRESULT CALLBACK SelectElementWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    friend LRESULT CALLBACK DropListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectElement))

    DECLARE_LAYOUT_FNS(CSelectLayout)

    typedef LRESULT (BUGCALL CSelectElement::*WIDEHOOKPROC)(WNDPROC pfnWndProc,
                                                            HWND hWnd,
                                                            UINT message,
                                                            WPARAM wParam,
                                                            LPARAM lParam);

    enum WindowMessages {
        Select_AddString = 0,
        Select_GetCount,
        Select_GetCurSel,     //  Misspelled on purpose: you shouldn't use this directly.
        Select_SetCurSel,     //  Misspelled on purpose: you shouldn't use this directly.
        Select_GetSel,
        Select_SetSel,
        Select_GetItemData,
        Select_SetItemData,
        Select_GetText,
        Select_GetTextLen,
        Select_DeleteString,
        Select_InsertString,
#ifndef WIN16
        Select_GetTopIndex,
        Select_SetTopIndex,
#endif //!WIN16
        Select_ResetContent,
        Select_SetItemHeight,
        Select_LastMessage_Guard   //  !! Keep this as the last enum value   !!
                                   //  !! Insert new messages in front of it !!
    };


    CSelectElement(CDoc *pDoc)
      : super( ETAG_SELECT, pDoc ),
        _aryOptions(Mt(CSelectElement_aryOptions_pv))
    {
#ifdef WIN16
        m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
        m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
        InvalidateCollection();
        _fOwnsRuns = TRUE;
        _fSetComboVert = TRUE;
        _iCurSel = -1;
    }


    ~CSelectElement();


    //
    // IPrivateUnknown members
    //
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init();
    virtual HRESULT Init2(CInit2Context * pContext);

    virtual DWORD GetInfo(GETINFO gi);
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT ApplyDefaultFormat ( CFormatInfo * pCFI );

    virtual HRESULT GetNaturalExtent(DWORD dwExtentMode, SIZEL *psizel);

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    virtual HRESULT ClickAction(CMessage * pMessage);

    // IDispatchEx methods
    HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);

    STDMETHOD (InvokeEx)(
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider);

    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id,
                                            BSTR *pbstrName));

    void DeferUpdateWidth();
    NV_DECLARE_ONCALL_METHOD(DeferredUpdateWidth, deferredupdatewidth, (DWORD_PTR dwContext));

#ifndef NO_DATABINDING
    // CElement OverRides

    virtual const CDBindMethods *GetDBindMethods();
#endif

    // Form-related methods
    virtual HRESULT DoReset(void);
    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData);

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    virtual HWND    GetHwnd() { return _hwnd; }

    inline void InvalidateCollection(void) { _lCollectionLastValid++; }

    HRESULT AddOptionHelper(COptionElement *    pOption,
                            long                lIndex,
                            const TCHAR *       pszText,
                            BOOL                fDummy);

    HRESULT RemoveOptionHelper(long lIndex);

    NV_DECLARE_REMOVEOBJECT_METHOD(RemoveFromCollection, removefromcollection, (long lCollection,
                                         long lIndex));

    NV_DECLARE_ADDNEWOBJECT_METHOD(AddNewOption, addnewoption, (long           lIndex,
                                 IDispatch *    pObject,
                                 long           index ));

    BOOL    HasWindow(void)
        {
            return !! _hwnd;
        }

    HRESULT BuildOptionsCache(void);


    #define _CSelectElement_
    #include "select.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    HRESULT OnInPlaceDeactivate(void);

    HRESULT EnsureControlWindow(void);
    HRESULT CreateControlWindow(void);
    void    DestroyControlWindow(void);

    HRESULT PushStateToControl(BOOL fFromCreate = FALSE);
    HRESULT PullStateFromControl(void);

    HRESULT Morph(void);

    HRESULT InitState(void);

    void    GenericDrawItem(LPDRAWITEMSTRUCT pdis, UINT uGetItemMessage);

    NV_DECLARE_ENSURE_METHOD(EnsureOptionCollection, ensureoptioncollection, (long lIndex,long * plCookie));
    HRESULT EnsureCollectionCache();


    HRESULT NotifyWidthChange(COptionElement * pOptionChanged);

    HRESULT AppendOption(COptionElement * pOption);

    static UINT  s_aMsgs[][2];
    static DWORD s_dwStyle[2];

    UINT SelectMessage(WindowMessages msg)
    { return s_aMsgs[msg][_fListbox ? 1 : 0]; };

    LRESULT SendSelectMessage(WindowMessages msg,
                              WPARAM wParam,
                              LPARAM lParam);

    LRESULT CALLBACK WndProc(
            HWND hWnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam);

    LRESULT CALLBACK DropWndProc(
            HWND hWnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam);

    BOOL HandleMouseEvents(
            HWND hWnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam,
            BOOL fFromDropDowm = FALSE);


    LRESULT BUGCALL WListboxHookProc(WNDPROC pfnWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT BUGCALL WComboboxHookProc(WNDPROC pfnWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT GenericCharToItem
    (
        HWND hCtlWnd,
        WORD wKey,
        WORD wCurrentIndex,
        UINT uGetItemMsg,
        UINT uGetCountMsg,
        UINT uGetCurSelMsg
    );

    static WIDEHOOKPROC s_alpfnWideHookProc[2];

    BOOL    IsMultiSelect(void)
                { return GetAAmultiple(); };

    STDMETHOD(GetEnabled)(VARIANT_BOOL * pfEnabled);

    void ChangeBackgroundBrush(void);

#define SETCURSEL_UPDATECOLL 0x1
#define SETCURSEL_DONTTOUCHHWND 0x2
#define SETCURSEL_SETTOPINDEX 0x4

    //  Window message wrappers
    LRESULT SetCurSel(int iNewSel, DWORD grfUpdate = 0);
    LRESULT GetCurSel(void);
    LRESULT SetSel(long lIndex, BOOL fSelected, DWORD grfUpdate = 0);
    LRESULT SetTopIndex(int iTopIndex);
    LRESULT AddString(LPCTSTR pstr);

    void    Fire_onchange_guarded(void)
    {
        if ( ! _fFiringOnChange )
        {
            _fFiringOnChange = TRUE;
            Fire_onchange();
            _fFiringOnChange = FALSE;
        }
    }

    void ResetContents(void)
    {
        if ( _hwnd )
        {
            SendSelectMessage(Select_ResetContent,0,0);
        }
    }

private:
    NO_COPY(CSelectElement);
    
    void ExitTree();

    HRESULT EnterTree();

    BOOL HasValueChanged(int iOldSel, int iNewSel);

    CCollectionCache *      _pCollectionCache;

    COptionElement *        _poptLongestText;

    CPtrAry <COptionElement*>     _aryOptions;

    WNDPROC     s_pfnDropListWndProc;


    SIZE        _sizeDefault;
    long        _lFontHeight;
    HFONT       _hFont;
    HWND        _hwnd;
    HWND        _hwndDropList;
    HBRUSH      _hBrush;
    HBRUSH      _hbrushHighlight;
    long        _lComboHeight;
    long        _lCollectionLastValid;
    long        _lTopIndex;
    long        _lMaxWidth;
    int         _iCurSel;
    unsigned    _fListbox : 1;
    unsigned    _fMultiple : 1;
    unsigned    _fFocus : 1;
    unsigned    _fWindowVisible : 1;
    unsigned    _fSelectedWasFound : 1;
    unsigned    _fLButtonDown : 1;
    unsigned    _fTriggerComboSubclassing : 1;
    unsigned    _fFiringOnChange : 1;
    unsigned    _fRefreshFont : 1;
    unsigned    _fNeedMorph : 1;
    unsigned    _fSendMouseMessagesToDocument : 1;
    unsigned    _fEnableLayoutRequests : 1;
    unsigned    _fOptionsDirty : 1;
    unsigned    _fWindowDirty : 1;
    unsigned    _fSetComboVert : 1;
    unsigned    _fDeferFiringOnClick : 1; // Should we fire the onclick event in selendok?
    unsigned    _iHistoryIndex:16;
};

#pragma INCMSG("--- End 'eselect.hxx'")
#else
#pragma INCMSG("*** Dup 'eselect.hxx'")
#endif
