//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       allpage.cxx
//
//  Contents:   Generic TypeInfo-based property page
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#if DBG==1    

#ifndef X_PROPUTIL_HXX_
#define X_PROPUTIL_HXX_
#include "proputil.hxx"
#endif

#ifndef X_ALLPAGE_HXX_
#define X_ALLPAGE_HXX_
#include "allpage.hxx"
#endif

#ifndef X_ELEMENT_H_
#define X_ELEMENT_H_
#include "element.h"  // for IElement->GetSTYLE(..)
#endif

extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);


#define WM_DEFERUPADTE  WM_APP + 1   // This is only used in AllPage
#define WM_CBSELCHANGED  WM_APP + 2   // This is only used in AllPage

const CBase::CLASSDESC CAllPage::s_classdesc = { 0 };

DeclareTag(tagAsyncPict, "AllPage", "Async Picture Loading")

MtDefine(CAllPage, Dialogs, "CAllPage")
MtDefine(CAllPage_aryObjs_pv, CAllPage, "CAllPage::_aryObjs::_pv")
MtDefine(CAllPageParseValue_pchString, Dialogs, "CAllPage::ParseValue pchString")

//+------------------------------------------------------------------------
//
//  Function:   CreateGenericPropertyPage
//
//  Synopsis:   Creates a new generic page.  Called from the generic
//              page's class factory
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CreateGenericPropertyPage(
        IUnknown * pUnkOuter,
        IUnknown **ppUnk)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    *ppUnk = new CAllPage(FALSE, IDS_PPG_GENERIC);
    return *ppUnk ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CreateInlineStylePropertyPage(
        IUnknown * pUnkOuter,
        IUnknown **ppUnk)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    *ppUnk = new CAllPage(TRUE, IDS_PPG_INLINE_STYLE);
    return *ppUnk ? S_OK : E_OUTOFMEMORY;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::CAllPage
//
//  Synopsis:   Constructor
//
//-------------------------------------------------------------------------
CAllPage::CAllPage(BOOL fStyle, UINT idrTitleString)
    : _aryObjs(Mt(CAllPage_aryObjs_pv))
{
    _fStyle = fStyle;
    _idrTitleString = idrTitleString;
    _hWndPage = NULL;
    _hWndEdit = NULL;
    _hWndButton = NULL;
    _hWndList = NULL;
    _dyEdit = 0;
    _pPageSite = NULL;
    _pEngine = NULL;
    _pHolder = NULL;
    _emode = EMODE_Edit;

    _fDirty =
    _fInUpdateEditor =
    _fInApply = FALSE;

#ifndef NO_EDIT
    _pUndoMgr = &g_DummyUndoMgr;
#endif // NO_EDIT
#ifdef _MAC
    _hfontDlg = CreateFont(9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        FF_DONTCARE, _T("Geneva"));
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::~CAllPage
//
//  Synopsis:   Destructor
//
//-------------------------------------------------------------------------

void
CAllPage::Passivate()
{
    Deactivate();
    ClearInterface(&_pPageSite);
    ClearInterface(&_pUndoMgr);
    ReleaseObjects();
    CBase::Passivate();
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::~CAllPage
//
//  Synopsis:   Destructor
//
//-------------------------------------------------------------------------

CAllPage::~CAllPage( )
{
    ReleaseInterface(_pUndoMgr);
    ReleaseObjects();

#ifdef _MAC
    DeleteObject (_hfontDlg);
#endif
}


STDMETHODIMP_(ULONG)
CAllPage::AddRef( )
{
    return PrivateAddRef();
}


STDMETHODIMP_(ULONG)
CAllPage::Release( )
{
    return PrivateRelease();
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::QueryInterface
//
//  Synopsis:   The generic page supports the following interfaces:
//
//                  IUnknown
//                  IPropertyPage
//
//  Arguments:  [iid]
//              [ppv]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IPropertyPage)
    {
        *ppv = (IPropertyPage *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::SetPageSite
//
//  Synopsis:   Replaces the current page site, which might be NULL
//
//  Arguments:  [pPageSite]     New page site
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::SetPageSite(IPropertyPageSite * pPageSite)
{
    if (_pPageSite && pPageSite)
        RRETURN(E_UNEXPECTED);

    ReplaceInterface(&_pPageSite, pPageSite);
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Activate
//
//  Synopsis:   Creates the window for this page.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::Activate(
        HWND hWndParent,
        const RECT * prc,
        BOOL fModal)
{
    RECT    rc;
    HRESULT hr;

    Assert(_hWndPage == NULL);

    //
    // Verify that we have a commit holder
    //

    hr = THR(EnsureCommitHolder((DWORD_PTR)hWndParent, &_pHolder));
    if (hr)
        goto Cleanup;

    //
    // Now let commit engine know about objects
    //

    hr = THR(_pHolder->GetEngine(_aryObjs.Size(), _aryObjs, &_pEngine));
    if (hr)
        goto Cleanup;

    _hWndPage = CreateDialogParam(
                    GetResourceHInst(),
                    MAKEINTRESOURCE(IDD_GENERICPAGE),
                    hWndParent,
                    PageWndProc,
                    (LPARAM) this);
    if (!_hWndPage)
        RRETURN(HRESULT_FROM_WIN32(GetLastError()));


    _emode = EMODE_Edit;

    ::GetWindowRect(_hWndEdit, &rc);
    _dyEdit = rc.bottom - rc.top;

    Verify(!Move(prc));
    hr = THR(UpdatePage());
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Deactivate
//
//  Synopsis:   Destroys the window for the page
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::Deactivate( )
{
    if (_hWndPage)
    {
        Verify(DestroyWindow(_hWndPage));
        _hWndPage = NULL;
        Assert(!_hWndPage);
    }

    if (_pHolder)
    {
        _pHolder->Release();
        _pHolder = NULL;
    }
    _pEngine = NULL;


    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::GetPageInfo
//
//  Synopsis:   Returns page description information
//
//  Arguments:  [pppi]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::GetPageInfo(LPPROPPAGEINFO pppi)
{
    HRESULT hr;
    DWORD   dw;
    TCHAR   szTitle[FORMS_BUFLEN + 1];

#if DBG == 1
    //
    // This function should fail if pppi->cb != sizeof(PROPPAGEINFO),
    // but we ignore it because the CDK  property frame does not set
    // this member.
    //
    if (pppi->cb != sizeof(PROPPAGEINFO))
    {
        TraceTag((tagWarning, "PROPPAGEINFO with bad cb (CDK prop frame known to do this)"));
#if 0
        RRETURN(E_FAIL);
#endif  // 0
    }
#endif  // DBG == 1

    memset(
            (BYTE *) pppi + sizeof(ULONG),
            0,
            sizeof(PROPPAGEINFO) - sizeof(ULONG));



    if (!LoadString(GetResourceHInst(), _idrTitleString, szTitle, FORMS_BUFLEN))
        return HRESULT_FROM_WIN32(GetLastError());

    dw = GetDialogBaseUnits();
    pppi->size.cx = 171 * LOWORD(dw) / 4;
    pppi->size.cy = 90 * HIWORD(dw) / 8;

    hr = TaskAllocString(szTitle, &pppi->pszTitle);
    if (hr)
        goto Error;

    // pppi->pszDocString not filled in
    // pppi->pszHelpFile not filled in
    // pppi->dwHelpContext not filled in

Cleanup:
    RRETURN(hr);

Error:
    TaskFreeString(pppi->pszTitle);
    goto Cleanup;
}



HRESULT
CAllPage::UpdatePage( )
{
    if (_hWndList)
    {
        SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);
        SendMessage(_hWndList, LB_RESETCONTENT, 0, 0);
    }

    UpdateList();

    if (_hWndList)
    {
        SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
    }

    UpdateEditor(NULL);
    UpdateEngine();

    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::SetObjects
//
//  Synopsis:   Releases the old set of objects, and makes a new set
//              current.  The TypeInfo's for the objects are examined, a
//              shared subset of properties is determined, then the
//              common values for these properties are retrieved.
//
//  Arguments:  [cObjects]
//              [apUnk]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::SetObjects(ULONG cObjects, IUnknown ** apUnk)
{
    HRESULT         hr;
    int             i;
    IUnknown  **    ppUnk     = NULL;
    IDispatch **    ppDisp    = NULL;

    if (TLS(prop.fModalDialogUp))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Flush the new values to the existing objects first.
    //

    if (_fDirty)
    {
        Apply();
    }

    ReleaseObjects();

    hr = THR(_aryObjs.EnsureSize(cObjects));
    if (hr)
        goto Error;

    for (i = 0, ppUnk = apUnk, ppDisp = _aryObjs;
         i < (int) cObjects;
         ppUnk++, ppDisp++)
    {

        if (_fStyle)
        {
            IHTMLStyle *pStyle;

            hr = THR((*ppUnk)->QueryInterface(IID_IHTMLElement, (void **) ppDisp));
            if (hr)
            {
                ClearInterface(ppDisp);
                goto Error;
            }

            hr = THR(  (  (IHTMLElement*)  *ppDisp)->get_style(&pStyle));
            ClearInterface(ppDisp);
            if (hr)
            {
                goto Error;
            }
            *ppDisp = pStyle;
        }
        else
        {
            hr = THR((*ppUnk)->QueryInterface(IID_IDispatch, (void **) ppDisp));
        }

        //BUGBUG --This looks fishy.  Shouldn't this be if(hr) ???
        if (!hr)
        {
            //if this fails we still want to the other object's pages
            //  or a blank page
            _aryObjs.SetSize(++i);
        }
        hr = S_OK;
    }


Cleanup:
    RRETURN(hr);

Error:
    ReleaseObjects();
    goto Cleanup;
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Show
//
//  Synopsis:   Shows the property page
//
//  Arguments:  [nCmdShow]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::Show(UINT nCmdShow)
{
    ShowWindow(_hWndPage, nCmdShow);
    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Move
//
//  Synopsis:   Moves the property page window.  The child windows are
//              repositioned to match.
//
//  Arguments:  [prc]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::Move(const RECT * prc)
{
    MoveWindow(
            _hWndPage,
            prc->left,
            prc->top,
            prc->right - prc->left,
            prc->bottom - prc->top,
            TRUE);

    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::IsPageDirty
//
//  Synopsis:   Returns S_OK if this page is dirty, S_FALSE otherwise.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::IsPageDirty( )
{
    return (_fDirty) ? S_OK : S_FALSE;
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Apply
//
//  Synopsis:   Applies any pending changes from the page.  The UI is
//              updated to pick up the new value.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::Apply( )
{
    HRESULT     hr;

    if (!_fDirty || _fInApply)
        return S_OK;

    _fInApply = TRUE;

    hr = THR(_pEngine->Commit());
    if (hr)
        goto Cleanup;

    SetDirty(FALSE);

Cleanup:
    _fInApply = FALSE;
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::OnButtonClick
//
//  Synopsis:   Called when ... Button is clicked. It opens Font/Picture
//              Dialog.
//
//  Returns:    None
//
//-------------------------------------------------------------------------
#ifndef NO_PROPERTY_PAGE
void
CAllPage::OnButtonClick(void)
{
    Assert(_pDPDCur);

    TLS(prop.fModalDialogUp) = TRUE;

    if (_pDPDCur->fSpecialCaseFont)
    {
        BOOL fRet;

        OpenFontDialog(
                this,
                _hWndEdit,
                _aryObjs.Size(),
                (IUnknown **)(IDispatch **) _aryObjs,
                &fRet);
        if (fRet)
        {
            // Open change font/ForeColor proeprty, update edit box
            UpdateEditor(_pDPDCur);
        }
    }
    else if (_pDPDCur->fSpecialCaseColor)
    {
        OpenColorDialog();
    }
    else
    {
        OpenPictureDialog(_pDPDCur->fSpecialCaseMouseIcon);
    }

    TLS(prop.fModalDialogUp) = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::OpenPictureDialog
//
//  Synopsis:   Opens Picture Dialog. If successful, Update edit box
//              and save the change
//
//  Returns:    None
//
//-------------------------------------------------------------------------

void
CAllPage::OpenPictureDialog(BOOL fMouseIcon)
{
    HRESULT hr = E_FAIL;
    TCHAR achFile[MAX_PATH];

    achFile[0] = 0;

    hr = THR(FormsGetFileName(
			      FALSE, // indicates OpenFileName
                    _hWndPage,
                    fMouseIcon  ? IDS_PROPERTYOPENMOUSEICON
                                : IDS_PROPERTYOPENPICTURE,
                    achFile,
                    ARRAY_SIZE(achFile), (LPARAM)0));

    if (!hr)
    {
        // Update edit box and save the change
        SetWindowText(_hWndEdit, achFile);
        _fDirty = 1;
        Apply();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::OpenColorDialog
//
//  Synopsis:   Opens Color Dialog. If successful, Update edit box
//              and save the change
//
//  Returns:    None
//
//-------------------------------------------------------------------------

void
CAllPage::OpenColorDialog()
{
    HRESULT     hr;
    BOOL        fRet;
    CHOOSECOLOR cc;
    COLORREF    aclr[16];
    int         i;
    VARIANT     var;

    //
    // initialize CHOOSECOLOR structure
    //

    memset((void *)&cc, 0, sizeof(CHOOSECOLOR));

    for (i = ARRAY_SIZE(aclr) - 1; i >= 0; i--)
    {
        aclr[i] = RGB(255, 255, 255);
    }

    VariantInit(&var);

    // Find the current color
    hr = THR_NOTRACE(GetCommonPropertyValue(
            _pDPDCur->dispid,
            _aryObjs.Size(),
            _aryObjs,
            &var));

    // If there is common color, set it as default selection
    if (hr == S_OK)
    {
        Assert(V_VT(&var) == VT_I4);
        cc.rgbResult = ColorRefFromOleColor(V_I4(&var));
        // set it be the first custom color
        aclr[0] = cc.rgbResult;
    }

    cc.lStructSize      = sizeof(CHOOSECOLOR);
    cc.hwndOwner        = _hWndPage;
    cc.hInstance        = NULL;     // no template; this is ignored
    cc.Flags            = CC_RGBINIT;
    cc.lpCustColors     = aclr;
    cc.lpfnHook         = NULL;
    cc.lpTemplateName   = NULL;


    fRet = ChooseColor(&cc);

    if (fRet)
    {

        V_VT(&var) = VT_I4;
        V_I4(&var) = cc.rgbResult;

#ifndef NO_EDIT
        CParentUndoUnit * pCPUU;

        pCPUU = OpenParentUnit(this, IDS_UNDOPROPCHANGE);
#endif // NO_EDIT
        hr = THR(SetCommonPropertyValue(
                _pDPDCur->dispid,
                _aryObjs.Size(),
                _aryObjs,
                &var));

        // BUGBUG: Is this still being called? New ShowLastErrorInfo
        // requires CDoc or CElement pointer.
        // ShowLastErrorInfo(NULL, hr);       
        Assert(FALSE);
#ifndef NO_EDIT
        hr = CloseParentUnit(pCPUU, hr);
#endif // NO_EDIT
    }
}
#endif // NO_PROPERTY_PAGE



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Help
//
//  Synopsis:   NYI
//
//  Arguments:  [szHelpDir]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::Help(LPCTSTR szHelpDir)
{
    RRETURN(E_NOTIMPL);
}





//+------------------------------------------------------------------------
//
//  Member:     CAllPage::TranslateAccelerator
//
//  Synopsis:   Handles the keyboard interface for the page.
//
//  Arguments:  [pmsg]      Message to translate
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CAllPage::TranslateAccelerator(LPMSG pmsg)
{
    if (_hWndPage && (pmsg->hwnd == _hWndPage || IsChild(_hWndPage, pmsg->hwnd)))
    {
        HRESULT     hr;

        if (pmsg->hwnd == _hWndList || IsChild(_hWndList, pmsg->hwnd))
        {
            hr = THR(_pPageSite->TranslateAccelerator(pmsg));
            if (hr != S_FALSE)
                return hr;
        }

        if (pmsg->message == WM_KEYDOWN &&
            pmsg->wParam == VK_DELETE && pmsg->hwnd == _hWndEdit )
        {
            // We use delete key to set NULL picture
            if (_pDPDCur && _pDPDCur->fSpecialCasePicture)
            {
                // Delete key is used to NULL picture
                SetWindowText(_hWndEdit, _T("NULL"));
                _fDirty = 1;
                Apply();
            }
        }
    }
    else if (pmsg->message == WM_KEYDOWN &&
             pmsg->wParam == VK_TAB &&
             GetKeyState(VK_CONTROL) >= 0)
    {
        HWND    hWndFoc;
        BOOL    fPrev = !!(GetKeyState(VK_SHIFT) & 0x8000);

        hWndFoc = GetNextDlgTabItem(
            _hWndPage,
            fPrev ? _hWndEdit : _hWndList,
            fPrev);

        if (hWndFoc)
        {
            SetFocus(hWndFoc);
            return S_OK;
        }
    }

    return S_FALSE;
}




//+------------------------------------------------------------------------
//
//  Member:     CAllPage::ReleaseObjects
//
//  Synopsis:   Releases the current set of objects.  Also releases the
//              current set of property descriptors and cached values.
//
//-------------------------------------------------------------------------

void
CAllPage::ReleaseObjects( )
{
    _aryObjs.ReleaseAll();

    ReleaseVars();
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::ReleaseVars
//
//  Synopsis:   Releases the current set of property descriptors and
//              cached values.
//
//-------------------------------------------------------------------------

void
CAllPage::ReleaseVars( )
{
    _pDPDCur = NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::UpdateList
//
//  Synopsis:   Update the owner-draw listbox with the property names.
//              Note that even though we do all the drawing for the list,
//              we still pass in strings, so that we can use the listbox
//              window's prefix matching.
//
//-------------------------------------------------------------------------

void
CAllPage::UpdateList( )
{
    int             i;
    DPD *           pDPD;
    int             index;
    CDataAry<DPD> * paryDPD;

    if (!_hWndPage)
        return;

    paryDPD = _pEngine->GetDPDs();

    for (i = 0, pDPD = *paryDPD;
         i < paryDPD->Size();
         i++, pDPD++)
    {
        // Do not display member not found property or read-only property
        if (pDPD->fMemberNotFound || pDPD->fReadOnly)
        {
            continue;
        }

        // If an item with the same name already exists in the list,
        // skip adding the duplicate
        // (Even though it refers to a different property, only the
        //  first instance seen "wins" and should be exposed)
        index = (int) SendMessage(
                            _hWndList,
                            LB_FINDSTRINGEXACT,
                            (WPARAM) -1,
                            (LPARAM) pDPD->bstrName);
        if (index != LB_ERR)
        {
            continue;
        }

        // Add the item to the list
        index = (int) SendMessage(
                            _hWndList,
                            LB_ADDSTRING,
                            0,
                            (LPARAM) pDPD->bstrName);
        if (index == LB_ERR)
            continue;

        SendMessage(_hWndList, LB_SETITEMDATA, (WPARAM) index, (LPARAM) i);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::UpdateEngine
//
//  Synopsis:   Updates the engine based on the current prop value.
//
//-------------------------------------------------------------------------

void
CAllPage::UpdateEngine()
{
    HRESULT     hr;
    VARIANT     var;

    VariantInit(&var);

    if (_pDPDCur)
    {
        hr = THR(ParseValue(&var));
        if (hr)
            goto Cleanup;

        _pEngine->SetProperty(_pDPDCur->dispid, &var);
    }

    if (_hWndPage)
    {
        Assert(_hWndList);
        InvalidateRect(_hWndList, (GDIRECT *)NULL, FALSE);
    }

Cleanup:
    VariantClear(&var);
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::UpdateEditor
//
//  Synopsis:   Updates the editor window to match the currently selected
//              property.  This may require creating a new editor window,
//              filling any drop-down list, and always includes updating
//              the window's value.
//
//  Arguments:  [pDPDSource]    The source of the property update.
//                              If non-NULL, then the editor is assumed
//                              to match the property in all ways
//                              except the property value.
//
//-------------------------------------------------------------------------

void
CAllPage::UpdateEditor(DPD * pDPDSource)
{
    DPD *       pDPD;
    int         i, j;
    BOOL        fEnableEditor;
    EMODE       emode;
    DWORD       dwStyle;
    RECT        rcEdit;
    RECT        rcButton;
    EVAL *      pEVAL;
    HFONT       hFont;
    VARIANT     var;
    LPTSTR      szValue;
    TCHAR       ach[FORMS_BUFLEN + 1];
    LPTSTR      szValueEnum;
    TCHAR       achEnum[FORMS_BUFLEN + 1];
    CDataAry<DPD> *paryDPD = _pEngine->GetDPDs();

    _fInUpdateEditor = TRUE;

    pDPD = pDPDSource;
    if (!pDPD)
    {
        i = SendMessage(_hWndList, LB_GETCURSEL, 0, 0);
        if (i != LB_ERR)
        {
            j = SendMessage(_hWndList, LB_GETITEMDATA, i, 0);

            Assert(j >= 0);
            // BUGBUG: ChrisF: remove assert on Julia's advise
            // but don't understand it !
            Assert(j < paryDPD->Size());

            pDPD = &(*paryDPD)[j];
        }
    }

    if (pDPD == NULL)
    {
        szValue = _T("");
        emode = EMODE_Edit;

        fEnableEditor = FALSE;
    }
    else
    {
        fEnableEditor = FormatValue(
                            &pDPD->var,
                            pDPD,
                            ach,
                            ARRAY_SIZE(ach),
                            &szValue);

        // Find out what the mode should be
        if (fEnableEditor && pDPD->pAryEVAL != NULL)
        {
            if (pDPD->fSpecialCaseColor)
            {
                emode = EMODE_ComboButton;
            }
            else
            {
                emode = EMODE_StaticCombo;
            }
        }
        else if(pDPD->fSpecialCasePicture || pDPD->fSpecialCaseFont )
        {
            emode = EMODE_EditButton;
        }
        else
        {
            emode = EMODE_Edit;
        }

    }

    ::GetWindowRect(_hWndEdit, &rcEdit);
    ScreenToClient(_hWndPage, (LPPOINT) &rcEdit.left);
    ScreenToClient(_hWndPage, (LPPOINT) &rcEdit.right);

    if (_hWndButton)
    {
        ::GetWindowRect(_hWndButton, &rcButton);
        ScreenToClient(_hWndPage, (LPPOINT) &rcButton.right);
        rcEdit.right = rcButton.right;
    }

    if (emode != _emode)
    {
        Assert(!pDPDSource);

        //
        // Destory old windows
        //

        SetWindowPos(
                _hWndEdit,
                NULL,
                0, 0, 0, 0,
                SWP_HIDEWINDOW | SWP_NOMOVE |
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);

        Verify(DestroyWindow(_hWndEdit));
        _hWndEdit = NULL;

        if (_hWndButton)
        {
            SetWindowPos(
                    _hWndButton,
                    NULL,
                    0, 0, 0, 0,
                    SWP_HIDEWINDOW | SWP_NOMOVE |
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);

            Verify(DestroyWindow(_hWndButton));
            _hWndButton = NULL;
        }

        //
        // Create new windows
        //
        if ((emode == EMODE_EditButton) || (emode == EMODE_ComboButton))
        {
            if (emode == EMODE_EditButton)
            {
                // Create read-only edit widnow
                dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | ES_READONLY;
            }
            else
            {
                // Create dropdown combo widnow
                dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
                            CBS_AUTOHSCROLL | CBS_DROPDOWN;
            }

            _hWndEdit = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    (emode == EMODE_EditButton)
#ifndef WIN16
                        ? _T("EDIT")
#else
                        ? "EDIT32"
#endif
                        : _T("COMBOBOX"),
                    szValue,
                    dwStyle,
                    rcEdit.left,
                    rcEdit.top,
                    rcEdit.right - rcEdit.left - _dyEdit,
                    _dyEdit,
                    _hWndPage,
                    (HMENU) IDE_PROPVALUE,
                    g_hInstCore,
                    NULL);

            // Create button
            dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP
                        | BS_PUSHBUTTON;

            _hWndButton = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    _T("BUTTON") ,
                    _T("..."), //szValue,
                    dwStyle,
                    rcEdit.right - _dyEdit,
                    rcEdit.top,
                    _dyEdit,
                    _dyEdit,
                    _hWndPage,
                    (HMENU) IDB_OPENDLG,
                    g_hInstCore,
                    NULL);
        }
        else
        {
            if (emode == EMODE_Edit)
            {
                dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL;
            }
            else if (emode == EMODE_StaticCombo)
            {
                dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
                            CBS_AUTOHSCROLL | CBS_DROPDOWNLIST;
            }
            else
            {
                // For emode == EMODE_EditCombo
                dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
                            CBS_AUTOHSCROLL | CBS_DROPDOWN;
            }

            _hWndEdit = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    (emode == EMODE_Edit)
#ifndef WIN16
                        ? _T("EDIT")
#else
                        ? "EDIT32"
#endif
                        : _T("COMBOBOX"),
                    szValue,
                    dwStyle,
                    rcEdit.left,
                    rcEdit.top,
                    rcEdit.right - rcEdit.left,
                    _dyEdit,
                    _hWndPage,
                    (HMENU) IDE_PROPVALUE,
                    g_hInstCore,
                    NULL);
        }

        hFont = (HFONT) SendMessage(_hWndList, WM_GETFONT, 0, 0);
        SendMessage(_hWndEdit, WM_SETFONT, (WPARAM) hFont, FALSE);

        _emode = emode;
    }

    if (!pDPDSource && IsComboMode(emode))
    {
        while (SendMessage(_hWndEdit, CB_DELETESTRING, 0, 0) > 0)
            ;

        VariantInit(&var);

        for (i = pDPD->pAryEVAL->Size(), pEVAL = *pDPD->pAryEVAL;
             i > 0;
             i--, pEVAL++)
        {
            var.vt = VT_I4;
            V_I4(&var) = pEVAL->value;

            FormatValue(
                    &var,
                    pDPD,
                    achEnum,
                    ARRAY_SIZE(achEnum),
                    &szValueEnum);

            SendMessage(_hWndEdit, CB_ADDSTRING, 0, (LPARAM) szValueEnum);

            //  Resize the dropdown appropriately

            SetWindowPos(
                    _hWndEdit,
                    NULL,
                    0, 0, 0, 0,
                    SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOZORDER |
                        SWP_NOSIZE | SWP_NOREDRAW);

            SetWindowPos(
                    _hWndEdit,
                    NULL,
                    0,
                    0,
                    (emode == EMODE_ComboButton)
                        ? rcEdit.right - rcEdit.left - _dyEdit
                        : rcEdit.right - rcEdit.left,
                    _dyEdit + HIWORD(GetDialogBaseUnits()) *
		    min((int)8, pDPD->pAryEVAL->Size()) + 2,    // IEUNIX : (long) -> (int) due to Size() change
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);
        }
    }

    if (emode != EMODE_EditCombo && emode != EMODE_ComboButton )
    {
        SendMessage(
                _hWndEdit,
                IsComboMode(emode) ? CB_SELECTSTRING : WM_SETTEXT,
                0,
                (LPARAM) szValue);
    }
    else
    {
        // For EMODE_EditCombo, if value is in list, select it
        // Otherwise, set user defined value into editbox
        if (CB_ERR == SendMessage(
                _hWndEdit,
                 CB_SELECTSTRING ,
                0,
                (LPARAM) szValue))
        {
            SendMessage(
                    _hWndEdit,
                    WM_SETTEXT,
                    0,
                    (LPARAM) szValue);

        }
    }

    EnableWindow(_hWndEdit, fEnableEditor);
    ShowWindow(_hWndEdit, SW_SHOWNA);

    if (_hWndButton)
    {
        EnableWindow(_hWndButton, fEnableEditor);
        ShowWindow(_hWndButton, SW_SHOWNA);
    }

    _pDPDCur = pDPD;

    _fInUpdateEditor = FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CAllPage::IsComboMode
//
//  Synopsis:   Check if emode is for combo
//
//  Arguments:  EMDOE emode
//
//  Returns:    BOOL
//
//-------------------------------------------------------------------------

BOOL
CAllPage::IsComboMode(EMODE emode)
{
    if (emode == EMODE_EditCombo || emode == EMODE_StaticCombo ||
            emode == EMODE_ComboButton)
        return TRUE;
    else
        return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CAllPage::NameOfType
//
//  Synopsis:   Hard-wired lookup of type name based on VT
//
//  Arguments:  [pDPD]
//
//  Returns:    TCHAR *
//
//-------------------------------------------------------------------------

TCHAR *
CAllPage::NameOfType(DPD * pDPD)
{
    int     i;

    //  This page will not ship, and therefore does not need to
    //    be localized

    static struct tagTYPEANDNAME
    {
        VARTYPE vt;
        TCHAR * szName;
    }
    s_VarTypes[] =
    {
        VT_I2,          _T("I2"),
        VT_I4,          _T("I4"),
        VT_R4,          _T("R4"),
        VT_R8,          _T("R8"),
        VT_BOOL,        _T("BOOL"),
        VT_ERROR,       _T("ERROR"),
        VT_CY,          _T("CY"),
        VT_DATE,        _T("DATE"),
        VT_BSTR,        _T("BSTR"),
        VT_UNKNOWN,     _T("UNKNOWN"),
        VT_DISPATCH,    _T("DISPATCH"),
        VT_VARIANT,     _T("VARIANT"),
        VT_USERDEFINED, _T("USERDEF"),
        VT_FILETIME,    _T("FILETIME"),
        VT_NULL,        _T("NULL"),
        VT_PTR,         _T("POINTER"),
        0,              _T("error"),
    };

    for (i = 0; i < ARRAY_SIZE(s_VarTypes) - 1; i++)
    {
        if (pDPD->vt == s_VarTypes[i].vt)
            break;
    }

    return s_VarTypes[i].szName;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::FormatValue
//
//  Synopsis:   Formats a value for display, either in the summary list
//              or in the editor.
//
//  Arguments:  [pvar]      Value to format
//              [pDPD]      Property descriptor
//              [ach]       Buffer to use if necessary
//              [cch]       ditto
//              [ppstr]     String pointer returned in *ppstr
//
//  Returns:    BOOL; FALSE if the editor window should be disabled.
//
//-------------------------------------------------------------------------

BOOL
CAllPage::FormatValue(
        VARIANT * pvar,
        DPD * pDPD,
        TCHAR * ach,
        int cch,
        TCHAR ** ppstr)
{
    int     i;
    EVAL *  pEVAL;
    TCHAR * pstrFriendly;
    TCHAR * pstr;
    int     value;
    HRESULT hr;
    BSTR    bstr    = NULL;

    if (pDPD->fMemberNotFound)
    {
        *ppstr = _T("");
        return FALSE;
    }

    if (pDPD->fSpecialCaseFont)
    {
        IFont *     pFont   = NULL;
        CY          cy;
        BOOL        fBold;
        BOOL        fItalic;
        BOOL        fUnderline;
        BOOL        fStrikethrough;

        if (pvar->vt == VT_UNKNOWN || pvar->vt == VT_DISPATCH)
        {
            IGNORE_HR(V_UNKNOWN(pvar)->QueryInterface(
                    IID_IFont,
                    (void **) &pFont));
        }

        if (pFont)
        {
            pFont->get_Name(&bstr);
            pFont->get_Size(&cy);
            pFont->get_Bold(&fBold);
            pFont->get_Italic(&fItalic);
            pFont->get_Underline(&fUnderline);
            pFont->get_Strikethrough(&fStrikethrough);

            Assert(cy.Hi == 0);

            Format( 0,
                    ach, cch,
                    _T("<0d>pt <1s><2s><3s><4s><5s>"),
                    (long) ((cy.Lo + 5000) / 10000),
                    bstr ? bstr: _T(""),
                    (fBold) ? _T(", Bold") : _T(""),
                    (fItalic) ? _T(", Italic") : _T(""),
                    (fUnderline) ? _T(", Underline") : _T(""),
                    (fStrikethrough) ? _T(", Strikethrough") : _T(""));

            *ppstr = ach;

            FormsFreeString(bstr);
            pFont->Release();
        }
        else
        {
            *ppstr = _T("(Error)");
        }
    }
    else if (pDPD->fSpecialCasePicture)
    {
        SHORT sPicType = PICTYPE_NONE;

        //
        // Because VB does not accept NULL value for picture property,
        // there is Empty picture with PICTTYPE_NONE type, so need
        // distingish it from real picture.
        //
        if (V_DISPATCH(&(pDPD->var)))
        {
            IPicture *  pPicture = NULL;
            if (!V_DISPATCH(&(pDPD->var))->QueryInterface(
                    IID_IPicture,
                    (void **) &pPicture))
            {
                pPicture->get_Type(&sPicType);
            }
            ReleaseInterface(pPicture);
        }

        if (sPicType == PICTYPE_NONE)
        {
            *ppstr = _T("(None)");
        }
        else
        {
            *ppstr = _T("(Picture)");
        }
    }
    else if (pDPD->pAryEVAL)
    {
        pstrFriendly = _T("Unknown");

        switch (pvar->vt)
        {
        case VT_BOOL:
        case VT_I2:
            value = V_I2(pvar);
            break;

        case VT_I4:
            value = V_I4(pvar);
            break;

        default:
            *ppstr = _T("Type mismatch");
            return FALSE;
        }

        for (i = pDPD->pAryEVAL->Size(), pEVAL = *pDPD->pAryEVAL;
             i > 0;
             i--, pEVAL++)
        {
            if (pEVAL->value == value)
            {
                pstrFriendly = pEVAL->bstr;
                break;
            }
        }

        Format(
                0,
                ach, cch,
                (pDPD->fSpecialCaseColor) ?
                    _T("<0x> - <1s>") :
                    _T("<0d> - <1s>"),
                (long)value,
                pstrFriendly);

        *ppstr = ach;
    }
    else
    {
        *ppstr = ach;

        switch (pvar->vt)
        {
        case VT_BSTR:
            *ppstr = V_BSTR(pvar);
            if (!*ppstr)
                *ppstr = _T("");
            break;

        case VT_I2:
            _ltot(V_I2(pvar), ach, 10);
            break;

        case VT_I4:
            _ltot(V_I4(pvar), ach, 10);
            break;

        case VT_R4:
            hr = THR(VarBstrFromR4(
                            V_R4(pvar),
                            g_lcidUserDefault,
                            0,
                            &bstr));
            if (hr)
            {
                *ppstr = _T("");
            }
            else
            {
                _tcsncpy(ach, bstr, cch);
                FormsFreeString(bstr);
            }
            break;

        case VT_R8:
            hr = THR(VarBstrFromR8(
                            V_R8(pvar),
                            g_lcidUserDefault,
                            0,
                            &bstr));
            if (hr)
            {
                *ppstr = _T("");
            }
            else
            {
                _tcsncpy(ach, bstr, cch);
                FormsFreeString(bstr);
            }
            break;

        case VT_CY:
#ifdef NEVER
            _ltot((int) (V_CY(pvar).int64 / 10000), ach, 10);
            pstr = _tcschr(ach, 0);
            _ltot((int) (V_CY(pvar).int64 % 10000) / 100 + 100, pstr, 10);
#endif // NEVER
            Assert(V_CY(pvar).Hi == 0);

            _ltot((int) (V_CY(pvar).Lo / 10000), ach, 10);
            pstr = _tcschr(ach, 0);
            _ltot((int) (V_CY(pvar).Lo % 10000) / 100 + 100, pstr, 10);

            *pstr = _T('.');
            break;

        case VT_BOOL:
            if (V_BOOL(pvar))
                *ppstr = _T("True");
            else
                *ppstr = _T("False");
            break;

        case VT_NULL:
            *ppstr = _T("");
            break;

        default:
            *ppstr = _T("");
            return FALSE;
        }
    }

    return TRUE;
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::ParseValue
//
//  Synopsis:   Parses the value currently in the editing window, based
//              on the property descriptor last used to fill in the
//              editor.
//
//  Arguments:  [pvar]      Parsed value returned in *pvar.  Note that
//                          pvar->vt may not match _pDPDCur->vt; this
//                          method relies on normal OLE Automation practice
//                          to coerce the value to its final type.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

#if defined(DEBUG) && !defined(MAC) 
STDAPI OleLoadPicturePath( LPOLESTR szUrl, LPUNKNOWN punkCaller, DWORD dwFlags,
                           OLE_COLOR clrBackgnd, REFIID, LPVOID * );
#endif

HRESULT
CAllPage::ParseValue(VARIANT * pvar)
{
    HRESULT     hr;
    int         i;
    int         cch;
    int         cch2;
    FONTDESC    fd;
    TCHAR *     pch;
    TCHAR *     pch2;
    TCHAR *     pchN;
    TCHAR *     pchString = NULL;

    if (_emode != EMODE_StaticCombo )
    {
        cch = SendMessage(_hWndEdit, WM_GETTEXTLENGTH, 0, 0);
        pchString = new(Mt(CAllPageParseValue_pchString)) TCHAR[cch + 1];
        if (pchString == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        SendMessage(
                _hWndEdit,
                WM_GETTEXT,
                cch + 1,
                (LPARAM) pchString);

        hr = E_INVALIDARG;

        if (_pDPDCur->fSpecialCaseFont)
        {
            pch = _tcsstr(pchString, _T("pt"));
            if (pch)
            {
                memset(&fd, 0, sizeof(FONTDESC));

                fd.cbSizeofstruct = sizeof(FONTDESC);
                fd.sWeight = 400;
                fd.sCharset = DEFAULT_CHARSET;

                *pch = 0;
#if !defined(_MAC) && !defined(UNIX)
                fd.cySize.int64 = _ttol(pchString) * 10000;
#else
                IGNORE_HR(ttol_with_error(pchString, (long *) &fd.cySize.Lo));
                fd.cySize.Lo *= 10000;
                fd.cySize.Hi = 0;
#endif
                pch += 2;
                pch += _tcsspn(pch, _T(" "));

                pch2 = _tcschr(pch, _T(','));
                if (pch2)
                    *pch2 = 0;

                fd.lpstrName = pch;

                while (pch2)
                {
                    pch2++;
                    pch2 += _tcsspn(pch2, _T(" "));
                    pchN  = _tcschr(pch2, _T(','));
                    cch2  = pchN ? pchN - pch2 : _tcslen(pch2);

                    if (_tcsnipre(_T("bold"), 4, pch2, cch2))
                    {
                        fd.sWeight = 700;
                    }
                    else if (_tcsnipre(_T("ital"), 4, pch2, cch2))
                    {
                        fd.fItalic = TRUE;
                    }
                    else if (_tcsnipre(_T("under"), 5, pch2, cch2))
                    {
                        fd.fUnderline = TRUE;
                    }
                    else if (_tcsnipre(_T("strike"), 6, pch2, cch2))
                    {
                        fd.fStrikethrough = TRUE;
                    }

                    pch2 = pchN;
                }

                hr = THR(OleCreateFontIndirect(
                        &fd,
                        IID_IFont,
                        (void **) &V_UNKNOWN(pvar)));
                if (!hr)
                    pvar->vt = VT_UNKNOWN;
            }
        }
        else if (_pDPDCur->fSpecialCasePicture)
        {
            IStream *   pStm;

            if (StrCmpIC(pchString, _T("NULL")) == 0)
            {
                pvar->vt = VT_UNKNOWN;
                V_UNKNOWN(pvar) = NULL;
                hr = S_OK;
            }
            else
            {
#if defined(DEBUG) && !defined(_MAC) && !defined(WINCE)
#define OPCTPATH_DEFAULTS    0 // SYNC | AUTHORTIME | OPAQUE
#define OPCTPATH_ASYCHRONOUS 1
#define OPCTPATH_RUNTIME     2
#define OPCTPATH_RESERVED1   4
                if (IsTagEnabled(tagAsyncPict))
                {
                    // exercise async picture loading thru the new API
                    pStm = pStm;
                    pch2 = new TCHAR[cch + 6];
                    if (pch2 == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }
                    _tcscpy(pch2, _T("FILE:"));
                    _tcscat(pch2, pchString);

                    hr = THR(OleLoadPicturePath(
                            pch2,
                            NULL,
                            OPCTPATH_ASYCHRONOUS,
                            (OLE_COLOR)RGB(255,0,0),
                            IID_IPicture,
                            (void **) &V_UNKNOWN(pvar)));
                    delete[] pch2;
                    if (!hr)
                        pvar->vt = VT_UNKNOWN;
                }
                else
#endif // defined(DEBUG) && !defined(_MAC) && !defined(WINCE)
                {
                    hr = THR(CreateStreamOnFile(
                            pchString,
                            STGM_SHARE_DENY_WRITE,
                            &pStm));
                    if (!hr)
                    {
                        hr = THR(OleLoadPicture(
                                pStm,
                                0,
                                FALSE,
                                IID_IPicture,
                                (void **) &V_UNKNOWN(pvar)));
                        if (!hr)
                            pvar->vt = VT_UNKNOWN;

                        pStm->Release();
                    }
                }
            }
        }
        else if (_pDPDCur->fSpecialCaseColor)
        {
            // NULL string is not allowed for color value
            if (!*pchString)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            V_I4(pvar) = _tcstoul(pchString, NULL, 16);
            if (V_I4(pvar) != ULONG_MAX)
            {
                pvar->vt = VT_I4;
                hr = S_OK;
            }
        }
        else
        {
            // If property value is numberic, and there is nothing in editbox
            // set default value be 0.
            if (!_tcslen(pchString) && ( (_pDPDCur->vt == VT_I2) ||
                    (_pDPDCur->vt == VT_I4) || (_pDPDCur->vt == VT_R4) ||
                    (_pDPDCur->vt == VT_R8) ))
            {
                hr = THR(FormsAllocString(_T("0"), &V_BSTR(pvar)));
            }
            else
            {
                hr = THR(FormsAllocString(pchString, &V_BSTR(pvar)));
            }
            if (!hr)
            {
                pvar->vt = VT_BSTR;
            }
        }
    }
    else
    {
        //
        //  This assumes that the combobox edit field is read-only
        //  (ie, the combobox is CBS_DROPDOWNLIST).
        //
        i = SendMessage(_hWndEdit, CB_GETCURSEL, 0, 0);
        if (i == CB_ERR)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            pvar->vt = VT_I4;
            Assert(_pDPDCur && _pDPDCur->pAryEVAL);
            V_I4(pvar) = (*_pDPDCur->pAryEVAL)[i].value;

            hr = S_OK;
        }
    }

Cleanup:
    if ( pchString )
        delete[] pchString;
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::DrawItem
//
//  Synopsis:   Renders a line in the owner-draw listbox.
//
//  Arguments:  [pdis]
//
//-------------------------------------------------------------------------

void
CAllPage::DrawItem(DRAWITEMSTRUCT * pdis)
{
    DPD *       pDPD;
    DWORD       dw;
    int         cch;
    int         dxName;
    GDIRECT     rc;
    LPTSTR      szValue;
    TCHAR       ach[FORMS_BUFLEN + 1];
    CDataAry<DPD> *paryDPD = _pEngine->GetDPDs();
    ULONG       cElem = paryDPD->Size();
#ifdef _MAC
    HFONT       hfontOld=NULL;
#endif

    HPEN        hPen = NULL;
    HPEN        hPenOld = NULL;

    //
    // See if there's anything in the array.  If not then skip to exit
    // code, as the only thing that can happen here is to draw the
    // focus rect.
    //

    if (!cElem)
        goto ExitCode;

    dw = GetDialogBaseUnits();
    cch = (pdis->rcItem.right - pdis->rcItem.left) / LOWORD(dw);

    dxName = LOWORD(dw) * cch * 4 / 10;

    if (pdis->itemState & ODS_SELECTED)
    {
        SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
    }
    else
    {
        SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
    }

    Assert(cElem > pdis->itemData);
    pDPD = &(*paryDPD)[pdis->itemData];

    if (pDPD->fMemberNotFound)
    {
        SetTextColor(pdis->hDC, GetSysColor(COLOR_GRAYTEXT));
    }
#ifdef _MAC
    if (_hfontDlg)
        hfontOld = (HFONT)SelectObject(pdis->hDC, _hfontDlg);
#endif
    rc = pdis->rcItem;
    rc.right = rc.left + dxName;

    ExtTextOut(
            pdis->hDC,
            rc.left  + 1,
            rc.top + 1,
            ETO_OPAQUE,
            &rc,
            pDPD->bstrName,
            FormsStringLen(pDPD->bstrName),
            NULL);

    // Create Grey Color Pen
    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
    hPenOld = (HPEN) SelectObject(pdis->hDC, hPen);

    // Draw vertical grid line
    MoveToEx(pdis->hDC, rc.right, rc.top, (POINT *)NULL);
    LineTo(pdis->hDC, rc.right, rc.bottom);

    rc.left = rc.right + 1;
    rc.right = pdis->rcItem.right;

    if (pDPD->fNoMatch)
    {
        szValue = _T("(mixed)");
    }
    else if (pDPD->fMemberNotFound)
    {
        szValue = _T("(not found)");
    }
    else
    {
        FormatValue(
                &pDPD->var,
                pDPD,
                ach,
                ARRAY_SIZE(ach),
                &szValue);
    }

    ExtTextOut(
            pdis->hDC,
            rc.left  + 1,
            rc.top + 1,
            ETO_OPAQUE,
            &rc,
            szValue,
            _tcslen(szValue),
            NULL);

    // Draw horizontal grid line
    MoveToEx(pdis->hDC, pdis->rcItem.left, pdis->rcItem.bottom - 1, (POINT *)NULL);
    LineTo(pdis->hDC, pdis->rcItem.right, pdis->rcItem.bottom -1);

ExitCode:
    if (pdis->itemState & ODS_FOCUS)
        DrawFocusRect(pdis->hDC, &pdis->rcItem);
#ifdef _MAC
    if (hfontOld)
        SelectObject(pdis->hDC, hfontOld);
#endif

    SelectObject(pdis->hDC, hPenOld);
    DeleteObject(hPen);
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::SetDirty
//
//  Synopsis:   Changes the dirty status for the page.
//
//  Arguments:  [dw]        Combination of PROPPAGESTATUS_* values
//
//-------------------------------------------------------------------------

void
CAllPage::SetDirty(DWORD dw)
{
    _fDirty = (dw & PROPPAGESTATUS_DIRTY) != 0;
#ifndef PRODUCT_96
    _pPageSite->OnStatusChange(dw | PROPPAGESTATUS_DIRTY);
#endif
}



//+------------------------------------------------------------------------
//
//  Member:     CAllPage::OnSize
//
//  Synopsis:   Called when the page changes size; the size of child
//              control is adjusted to match.
//
//-------------------------------------------------------------------------

void
CAllPage::OnSize( )
{
    RECT    rc;
    RECT    rcList;
    RECT    rcEdit;
    int     cy;
    CDataAry<DPD> *paryDPD = _pEngine->GetDPDs();

    GetClientRect(_hWndPage, &rc);

    ::GetWindowRect(_hWndList, &rcList);
    ScreenToClient(_hWndPage, (LPPOINT) &rcList.left);
    ScreenToClient(_hWndPage, (LPPOINT) &rcList.right);

    ::GetWindowRect(_hWndEdit, &rcEdit);
    ScreenToClient(_hWndPage, (LPPOINT) &rcEdit.left);
    ScreenToClient(_hWndPage, (LPPOINT) &rcEdit.right);

    MoveWindow(
            _hWndList,
            rcList.left,
            rcList.top,
            (rc.right - rc.left) - 2 * rcList.left,
            (rc.bottom - rc.top) - rcList.top - rcEdit.top,
            TRUE);

    //  Since the columns are proportionately sized, we need to
    //    invalidate on the size done above

    Assert(_hWndList);
    InvalidateRect(_hWndList, (GDIRECT *)NULL, FALSE);

    // Now move on to resizing the editor
    cy = rcEdit.bottom - rcEdit.top;    //(it equals _dyEdit)
    if (IsComboMode(_emode))
    {
        DPD *   pDPD = NULL;
        int     i;
        int     j;

        // If this is in combo box mode then add the size of the list
        // to the y axis
        i = SendMessage(_hWndList, LB_GETCURSEL, 0, 0);
        if (i != LB_ERR)
        {
            j = SendMessage(_hWndList, LB_GETITEMDATA, i, 0);

            Assert((j >= 0) && (j < paryDPD->Size()));
            pDPD = &(*paryDPD)[j];
        }

        Assert(NULL != pDPD);
        // Allow a max of 8 items on the combo box.
        cy += HIWORD(GetDialogBaseUnits()) * min((int)8, pDPD->pAryEVAL->Size()) + 2;
    }

    if (!_hWndButton)
    {
        MoveWindow(
                _hWndEdit,
                rcEdit.left,
                rcEdit.top,
                (rc.right - rc.left) - rcList.left - rcEdit.left,
                cy,
                TRUE);
    }
    else
    {
        MoveWindow(
                _hWndEdit,
                rcEdit.left,
                rcEdit.top,
                (rc.right - rc.left) - rcList.left - rcEdit.left - _dyEdit,
                cy,
                TRUE);

        MoveWindow(
                _hWndButton,
                rc.right- rc.left - rcList.left - _dyEdit,
                rcEdit.top,
                _dyEdit,
                _dyEdit,
                TRUE);
    }

    //  Combo boxes do not invalidate correctly when resized, so
    //    we need to force the invalidation manually

    Assert(_hWndEdit);
    RedrawWindow(
            _hWndEdit,
            (GDIRECT *)NULL,
            NULL,
            RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}


//+------------------------------------------------------------------------
//
//  Member:     CAllPage::Refresh
//
//  Synopsis:   Clear old values and update new values in both listbox and
//              Edit window. If dispid is DISPID_UNKNOW, update everything,
//              otherwise, update specify proprety.
//
//  Arguments:  [dispid]
//
//-------------------------------------------------------------------------

HRESULT
CAllPage::Refresh(DISPID dispid)
{
    UpdateEditor(NULL);

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Function:   PageWndProc
//
//  Synopsis:   Page window proc.
//
//  Arguments:  [hWnd]
//              [msg]
//              [wParam]
//              [lParam]
//
//  Returns:    long
//
//-------------------------------------------------------------------------

INT_PTR CALLBACK
PageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CAllPage *  pAllPg = (CAllPage *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg)
    {
#ifdef _MAC
    case WM_MACINTOSH:
        switch (LOWORD(wParam))
        {
            case WLM_SETMENUBAR:
            // dont change the menu bar
                return TRUE;
        }
        break;
#endif
    case WM_INITDIALOG:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);

        pAllPg = (CAllPage *)lParam;
        pAllPg->_hWndEdit = GetDlgItem(hWnd, IDE_PROPVALUE);
        pAllPg->_hWndList = GetDlgItem(hWnd, IDC_PROPNAME);

        if (pAllPg->_hWndList)
        {
            // NOTE: we could put a Verify() around this line, but it may fail
            // if "no-one" has the focus currently.
            ::SetFocus(pAllPg->_hWndList);
        }
        break;

#ifndef WINCE
    case WM_NCDESTROY:
#else
    case WM_DESTROY:
#endif
        pAllPg->_hWndPage =
        pAllPg->_hWndEdit =
        pAllPg->_hWndButton =
        pAllPg->_hWndList = NULL;
        break;

    case WM_DRAWITEM:
        pAllPg->DrawItem((DRAWITEMSTRUCT *) lParam);
        return TRUE;

    case WM_SIZE:
        pAllPg->OnSize();
        break;

   case WM_SHOWWINDOW:
        // Only do something if the message is sent because of
        // a call to the ShowWindow function;
        if (! (int)lParam)
        {
            if (wParam)
            {
                IGNORE_HR(pAllPg->UpdatePage());
            }
            else
            {
                pAllPg->ReleaseVars();
            }
        }
        break;

    case WM_DEFERUPADTE:
        if (IsWindowVisible(hWnd))
        {
            pAllPg->Refresh(0);
        }
        break;

    case WM_CBSELCHANGED:
        pAllPg->UpdateEngine();
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_PROPNAME)
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case LBN_SELCHANGE:
                pAllPg->UpdateEditor(NULL);
                pAllPg->UpdateEngine();
                break;

            case LBN_DBLCLK:
                if (pAllPg->_pDPDCur)
                {
                    switch (pAllPg->_emode)
                    {
                    case CAllPage::EMODE_Edit:
                        // set focus to the edit window and select all to start typing
                        SetFocus(pAllPg->_hWndEdit);
                        SendMessage(pAllPg->_hWndEdit, EM_SETSEL, 0, -1);
                        break;

                    case CAllPage::EMODE_StaticCombo:
                    case CAllPage::EMODE_EditCombo:
                        // advance the combo list to the next enum element, wrapping around 0
                        {
                            long cnt = SendMessage(pAllPg->_hWndEdit, CB_GETCOUNT, 0, 0);
                            long sel = SendMessage(pAllPg->_hWndEdit, CB_GETCURSEL, 0, 0);
                            if (sel != CB_ERR)
                            {
                                if (++sel >= cnt)
                                    sel = 0;
                                SendMessage(pAllPg->_hWndEdit, CB_SETCURSEL, sel, 0);
                                goto ComboSelectChanged;
                            }
                        }
                        break;

                    case CAllPage::EMODE_EditButton:
                    case CAllPage::EMODE_ComboButton:
                        // do the button action (bring up the picker)
                        pAllPg->OnButtonClick();
                        break;
                    }
                }
                break;
            }
        }
        else if (GET_WM_COMMAND_ID(wParam, lParam) == IDE_PROPVALUE &&
                 !pAllPg->_fInUpdateEditor)
        {
            if (pAllPg->_emode == CAllPage::EMODE_Edit)
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                case EN_CHANGE:
                    pAllPg->UpdateEngine();
                    pAllPg->SetDirty(PROPPAGESTATUS_DIRTY);
                    break;
                }
            }
            else
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                case CBN_EDITCHANGE:
                    pAllPg->UpdateEngine();
                    pAllPg->SetDirty(PROPPAGESTATUS_DIRTY);
                    break;

ComboSelectChanged:
                case CBN_SELCHANGE:
                    pAllPg->SetDirty(PROPPAGESTATUS_DIRTY | PROPPAGESTATUS_VALIDATE);
                    PostMessage(pAllPg->_hWndPage, WM_CBSELCHANGED, 0, 0);
                    break;
                }
            }
        }
        else if (GET_WM_COMMAND_ID(wParam, lParam) == IDB_APPLY)
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case BN_CLICKED:
                pAllPg->Apply();
                break;
            }
        }
        else if (GET_WM_COMMAND_ID(wParam, lParam) == IDB_OPENDLG)
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case BN_CLICKED:
                pAllPg->OnButtonClick();
                break;
            }
        }
        break;
    }

    return FALSE;
}


#endif // DBG==1    

