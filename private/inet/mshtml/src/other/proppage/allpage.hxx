//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       allpage.hxx
//
//  Contents:   Generic TypeInfo-based property page
//
//  Classes:    CAllPage
//
//-------------------------------------------------------------------------

#ifndef I_ALLPAGE_HXX_
#define I_ALLPAGE_HXX_
#pragma INCMSG("--- Beg 'allpage.hxx'")

class CAllPage;

#ifndef X_COMMIT_HXX_
#define X_COMMIT_HXX_
#include "commit.hxx"
#endif

MtExtern(CAllPage)

//+------------------------------------------------------------------------
//
//  Class:      CAllPage (tag)
//
//  Purpose:    Generic TypeInfo-based property page
//
//-------------------------------------------------------------------------

class CAllPage :
        public CBase,
        public IPropertyPage
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAllPage))

    friend INT_PTR PASCAL PageWndProc(HWND, UINT, WPARAM, LPARAM);

    CAllPage(BOOL fStyle, UINT idrTitleString);
    ~CAllPage();

    //
    //  CBase methods
    //

    void Passivate();

    //
    //  IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    //
    // IPropertyPage methods
    //

    STDMETHOD(SetPageSite)  (IPropertyPageSite * pPageSite);
    STDMETHOD(Deactivate)   (void);
    STDMETHOD(GetPageInfo)  (LPPROPPAGEINFO pPropPageInfo);
    STDMETHOD(Show)         (UINT nCmdShow);
    STDMETHOD(Move)         (const RECT * pRect);
    STDMETHOD(IsPageDirty)  (void);
    STDMETHOD(Help)         (LPCTSTR lpszHelpDir);
    STDMETHOD(SetObjects)   (ULONG cObjects, IUnknown ** ppUnk);
    STDMETHOD(Activate)     (HWND hwndParent, const RECT * prect, BOOL fModal);
    STDMETHOD(Apply)        (void);
    STDMETHOD(TranslateAccelerator) (LPMSG lpmsg);

    // CBase methods

    virtual const CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc; }

#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void) { return _pUndoMgr; }
#endif // NO_EDIT

    HRESULT     Refresh(DISPID dispid);
    HRESULT     UpdateProperty(DISPID dispid);

private:

    enum EMODE
    {
        EMODE_Edit = 0,
        EMODE_StaticCombo = 1,
        EMODE_EditCombo = 2,
        EMODE_EditButton = 3,
        EMODE_ComboButton = 4
    };

    void        ReleaseObjects();
    void        ReleaseVars();
    HRESULT     UpdatePage();

    void        UpdateList();
    void        UpdateEditor(DPD * pDPDSource);
    void        UpdateEngine();
    BOOL        IsComboMode(EMODE emode);

    TCHAR *     NameOfType(DPD * pDPD);
    void        DrawItem(DRAWITEMSTRUCT * pdis);

    BOOL        FormatValue(
                        VARIANT * pvar,
                        DPD * pDPD,
                        TCHAR * ach,
                        int cch,
                        TCHAR ** ppstr);
    HRESULT     ParseValue(VARIANT * pvar);

    void        SetDirty(DWORD dwPageStatusChange);

    void        OnSize(void);

    void        OnButtonClick(void);
    void        OpenPictureDialog(BOOL fMouseIcon);
    void        OpenColorDialog();


    //
    //  Members
    //

    HWND                _hWndPage;  //  Window handle for page
    HWND                _hWndEdit;  //  ... for Editor window, which can
                                    //    be of different classes
    HWND                _hWndList;  //  ... for owner-drawn listbox
    HWND                _hWndButton;//  For ... Button
    int                 _dyEdit;    //  Initial height of textbox; used
                                    //    when recreating Editor windows
    IPropertyPageSite * _pPageSite; //  Current property page site
    IOleUndoManager *   _pUndoMgr;  //  Pointer to undo manager
    EMODE               _emode;     //  What kind of editor is created?
    CCommitHolder *     _pHolder;   //  The commit holder
    CCommitEngine *     _pEngine;   //  The commit engine
    CPtrAry<IDispatch *>_aryObjs;   //  Current set of objects
    DPD *               _pDPDCur;
    UINT                _idrTitleString;     // Resource ID used to grab title
    unsigned            _fStyle:1;  // Use the Sytle IStyle instead of IDispatch

    unsigned            _fDirty:1;          //  TRUE if page is dirty
    unsigned            _fInUpdateEditor:1; //  TRUE if we're updating the
                                            //    editor window
    unsigned            _fInApply:1;        // TRUE if we're in the middle
                                            //    of applying some value
                                            
    const static CLASSDESC    s_classdesc;
#ifdef _MAC
    HFONT               _hfontDlg;     // Font for dialog text
#endif
};

#pragma INCMSG("--- End 'allpage.hxx'")
#else
#pragma INCMSG("*** Dup 'allpage.hxx'")
#endif
