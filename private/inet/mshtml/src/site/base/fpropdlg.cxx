//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       fpropdlg.cxx
//
//  Contents:   Display property dialog.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifdef NEVER

        #ifndef X_ELEMENT_HXX_
        #define X_ELEMENT_HXX_
        #include "element.hxx"
        #endif

        #ifndef X_COMMCTRL_H_
        #define X_COMMCTRL_H_
        #include "commctrl.h"
        #endif

        #ifndef X_MSHTMLRC_H_
        #define X_MSHTMLRC_H_
        #include "mshtmlrc.h"
        #endif

        #ifndef X_SITEGUID_H_
        #define X_SITEGUID_H_
        #include "siteguid.h"
        #endif

        #ifndef X_CGUID_H_
        #define X_CGUID_H_
        #include <cguid.h>
        #endif

        #ifdef UNIX
        #include <mainwin.h>
        #endif

        #ifndef NO_HTML_DIALOG
        class CPropertyDialog;

        MtDefine(CPropertyPageSite, Dialogs, "CPropertyPageSite")
        MtDefine(ShowPropertyDialog, Dialogs, "ShowPropertyDialog (temp array)")
        MtDefine(CPropertyDialog, Dialogs, "CPropertyDialog")
        MtDefine(CPropertyDialog_arySite_pv, Dialogs, "CPropertyDialog::_arySite::_pv")

        //+---------------------------------------------------------------------------
        //
        //  Class:      CPropertyPageSite
        //
        //  Synopsis:   Manage single page in property dialog.
        //
        //----------------------------------------------------------------------------

        class CPropertyPageSite :
            public IPropertyPageSite,
            public IServiceProvider
        {
        public:

            DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPropertyPageSite))

            CPropertyPageSite()
                { _ulRefs = 1; }

            ~CPropertyPageSite()
                { Close(); }

            DECLARE_FORMS_STANDARD_IUNKNOWN(CPropertyPageSite)

            void    Close();
            HRESULT Show();
            void    Hide();
            HRESULT Init(CPropertyDialog *pFrame, int iLevel, CLSID *pclsid);

            // IPropertyPageSite methods

            STDMETHOD(OnStatusChange)   (DWORD dw);
            STDMETHOD(GetLocaleID)      (LCID * pLocaleID);
            STDMETHOD(GetPageContainer) (LPUNKNOWN * ppUnk);
            STDMETHOD(TranslateAccelerator)(LPMSG lpMsg);

            // IServiceProvider methods

            STDMETHOD(QueryService)(REFGUID, REFIID, void **);

            // Data members

            CPropertyDialog *   _pDialog;
            CLSID               _clsid;
            IPropertyPage *     _pPage;
            PROPPAGEINFO        _ppi;
            BOOL                _fActive;
            int                 _iLevel;
        };


        //+---------------------------------------------------------------------------
        //
        //  Class:      CPropertyDialog
        //
        //  Synopsis:   Run the property dialog.
        //
        //----------------------------------------------------------------------------

        class CPropertyDialog
        {
        public:

            DECLARE_MEMALLOC_NEW_DELETE(Mt(CPropertyDialog))
            // Construct / destruct

            ~CPropertyDialog()
                { Close(); }

            // Helper functions

            HRESULT         GetCommonPages(int cUnk, IUnknown **apUnk, CAUUID *pca);
            void            SetPage(int i, BOOL fSetTab = TRUE);
            void            UpdateApplyButton();
            HRESULT         CreatePageSites(int iLevel, int cUnk, IUnknown **apUnk);
            void            UpdateTabs(int iLevel);
            BOOL            Apply();
            void            Close();
            HRESULT         PreTranslateMessage(LPMSG);
            HRESULT         TranslateTabCtrlAccelerators(LPMSG);

            // Dialog procedure

            static BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            BOOL OnInitDialog(HWND);
            void OnNotify(int idiCtrl, LPNMHDR lpnmhdr);
            void OnCommand(WORD wNotifyCode, WORD idi, HWND hwnd);

            HWND                _hwndDialog;    // dialog window
            HWND                _hwndTabs;      // tab control window
            HWND                _hwndCombo;     // combo box window
            HWND                _hwndTopParent; // top level parent window to be reenabled before close
            RECT                _rcPage;
            int                 _iPage;
            LCID                _lcid;

            BOOL                _fApplyWasHit;

            IUnknown **         _apUnk;
            int                 _cUnk;
            IUnknown *          _apUnkLevel[32];
            int                 _cUnkLevel;

            IServiceProvider *  _pServiceProvider;
            int                 _aiPageLevel[32];
            HRESULT             _hr;
            BOOL                _fMessageTranslated;
            IUnknown *          _punkBrowseDefault; // Not NULL implies we're in browse
                                                    //  mode

            DECLARE_CPtrAry(CArySite, CPropertyPageSite *, Mt(Mem), Mt(CPropertyDialog_arySite_pv))
            CArySite            _arySite;

        };


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::Init
        //
        //  Synopsis:   Initialize the page.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::Init(CPropertyDialog *pDialog, int iLevel, CLSID *pclsid)
        {
            HRESULT         hr;
            IClassFactory * pCF = NULL;

            _pDialog = pDialog;
            _iLevel = iLevel;

            //
            // First try getting a local class object.  If that fails, then
            // go out to OLE and the registry.
            //

            hr = THR_NOTRACE(LocalGetClassObject(
                    *pclsid,
                    IID_IClassFactory,
                    (void **)&pCF));
            if (!hr)
            {
                hr = THR(pCF->CreateInstance(
                        NULL,
                        IID_IPropertyPage,
                        (void **)&_pPage));
            }
            else
            {
                hr = THR(CoCreateInstance(
                        *pclsid,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPropertyPage,
                        (void **) &_pPage));
            }
            if (hr)
                goto Cleanup;

            hr = THR(_pPage->SetPageSite(this));
            if (hr)
                goto Cleanup;

            _ppi.cb = sizeof(PROPPAGEINFO);

            hr = THR(_pPage->GetPageInfo(&_ppi));
            if (hr)
                goto Cleanup;

        Cleanup:
            ReleaseInterface(pCF);
            RRETURN(hr);
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::Close
        //
        //  Synopsis:   Release everything.  Called at frame exit.
        //
        //-------------------------------------------------------------------------

        void
        CPropertyPageSite::Close()
        {
            if (_pPage)
            {
                if (_fActive)
                {
                    IGNORE_HR(_pPage->Deactivate());
                }
                IGNORE_HR(_pPage->SetPageSite(NULL));
            }

            CoTaskMemFree(_ppi.pszTitle);
            _ppi.pszTitle = NULL;

            CoTaskMemFree(_ppi.pszDocString);
            _ppi.pszDocString = NULL;

            CoTaskMemFree(_ppi.pszHelpFile);
            _ppi.pszHelpFile = NULL;

            //  BUGBUG (laszlog) : Shouldn't we call SetObjects(0,NULL) here to ensure
            //                     robust refcounting?
            //                     See Help for IPropertyPage::SetObjects


            ClearInterface(&_pPage);

            _pDialog = NULL;
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::Show
        //
        //  Synopsis:   Activates a page, passing it the current set of objects
        //              and showing it.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::Show()
        {
            HRESULT hr = S_OK;

            if (!_fActive)
            {
                if (_iLevel == 0)
                {
                    hr = THR(_pPage->SetObjects(
                            _pDialog->_cUnk,
                            _pDialog->_apUnk));
                }
                else
                {
                    hr = THR(_pPage->SetObjects(
                            1,
                            &_pDialog->_apUnkLevel[_iLevel]));
                }

                if (hr)
                    goto Cleanup;
                hr = THR(_pPage->Activate(_pDialog->_hwndTabs, &_pDialog->_rcPage, FALSE));
                if (hr)
                    goto Cleanup;

                _fActive = TRUE;
            }

            hr = THR(_pPage->Show(SW_SHOWNA));
            if (hr)
                goto Cleanup;

        Cleanup:
            RRETURN(hr);
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::Hide
        //
        //  Synopsis:   Hide the page
        //
        //-------------------------------------------------------------------------

        void
        CPropertyPageSite::Hide()
        {
            IGNORE_HR(_pPage->Show(SW_HIDE));
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::QueryInterface, IUnknown
        //
        //  Synopsis:   Per IUnknown.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::QueryInterface(REFIID iid, void ** ppv)
        {
            if (iid == IID_IUnknown || iid == IID_IPropertyPageSite)
            {
                *ppv = (IPropertyPageSite *) this;
            }
            else if (iid == IID_IServiceProvider)
            {
                *ppv = (IServiceProvider *) this;
            }
            else
            {
                *ppv = 0;
                RRETURN(E_NOINTERFACE);
            }

            (*(IUnknown **)ppv)->AddRef();
            return S_OK;
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::OnStatusChange, IPropertyPageSite
        //
        //  Synopsis:   Note that status changed.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::OnStatusChange(DWORD dw)
        {
            if (!_pDialog || !_pPage)
                RRETURN(E_UNEXPECTED);

            if (dw & PROPPAGESTATUS_DIRTY)
            {
                _pDialog->UpdateApplyButton();
            }
            return S_OK;
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::GetLocaleID
        //
        //  Synopsis:   Returns the current locale ID.
        //
        //-------------------------------------------------------------------------

        STDMETHODIMP
        CPropertyPageSite::GetLocaleID(LCID * plcid)
        {
            if (!_pDialog)
                RRETURN(E_UNEXPECTED);

            *plcid = _pDialog->_lcid;
            return S_OK;
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::GetPageContainer, IPropertyPageSite
        //
        //  Synopsis:   Per IPropertyPageSite, not supported.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::GetPageContainer(IUnknown ** ppUnk)
        {
            if (!_pDialog)
                RRETURN(E_UNEXPECTED);

            *ppUnk = NULL;
            return E_NOTIMPL;
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::TranslateAccelerator, IPropertyPageSite
        //
        //  Synopsis:   Handle accelerator.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::TranslateAccelerator(LPMSG pmsg)
        {
            if (!_pDialog)
                RRETURN(E_UNEXPECTED);

            _pDialog->_fMessageTranslated = TRUE;
            return IsDialogMessage(_pDialog->_hwndDialog, pmsg) ? S_OK : S_FALSE;
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyPageSite::QueryService, IServiceProvider
        //
        //  Synopsis:   Per IServiceProvider.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyPageSite::QueryService(REFGUID guid, REFIID iid, void **ppv)
        {
            HRESULT hr;

            if (!_pDialog)
                RRETURN(E_UNEXPECTED);

            if (_pDialog->_pServiceProvider)
            {
                hr = THR(_pDialog->_pServiceProvider->QueryService(guid, iid, ppv));
            }
            else
            {
                hr = E_FAIL;
            }

            RRETURN(hr);
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::Close
        //
        //  Synopsis:   Cleanup for shutdown.
        //
        //-------------------------------------------------------------------------

        void
        CPropertyDialog::Close()
        {
            int i;

            for (i = 0; i < _arySite.Size(); i++)
            {
                if (_arySite[i])
                {
                    _arySite[i]->Close();
                    _arySite[i]->Release();
                    _arySite[i] = 0;
                }
            }
            _arySite.SetSize(0);

            while (_cUnkLevel--)
            {
                ClearInterface(&_apUnkLevel[_cUnkLevel]);
            }
            _cUnkLevel = 0;

            if ( _hwndTopParent )
            {
                ::EnableWindow(_hwndTopParent, TRUE);
            }

            if (_hwndDialog)
            {
                DestroyWindow(_hwndDialog);
                _hwndDialog = 0;
            }
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::UpdateAppyButton
        //
        //  Synopsis:   Enable apply button based on page dirty.
        //
        //-------------------------------------------------------------------------

        void
        CPropertyDialog::UpdateApplyButton()
        {
            int     i;
            BOOL    fEnable = FALSE;

            for (i = 0; i < _arySite.Size(); i++)
            {
                if (S_OK == _arySite[i]->_pPage->IsPageDirty())
                {
                    fEnable = TRUE;
                    break;
                }
            }

            EnableWindow(GetDlgItem(_hwndDialog, IDC_PROPFRM_APPLY), fEnable);
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::UpdateTabs
        //
        //  Synopsis:   Set tabs to page titles.
        //
        //-------------------------------------------------------------------------

        void
        CPropertyDialog::UpdateTabs(int iLevel)
        {
            int     i;
            TC_ITEM item;

            TabCtrl_DeleteAllItems(_hwndTabs);

            memset(&item, 0, sizeof(item));
            item.mask = TCIF_TEXT;

            for (i = 0; i < _arySite.Size(); i++)
            {
                if (_arySite[i]->_iLevel == iLevel)
                {
                    item.pszText = _arySite[i]->_ppi.pszTitle;
                    Verify(TabCtrl_InsertItem(_hwndTabs, i, &item) != -1);
                }
            }
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::Apply
        //
        //  Synopsis:   Applies changes to dirty pages.
        //
        //  Returns:    True if all ok.
        //
        //-------------------------------------------------------------------------

        BOOL
        CPropertyDialog::Apply()
        {
            int i;
            HRESULT hr = S_OK;
            IPropertyPage *pPage;

            for (i = 0; i < _arySite.Size(); i++)
            {
                pPage = _arySite[i]->_pPage;
                if (S_OK == pPage->IsPageDirty())
                {
                    hr = THR(pPage->Apply());
                    if (hr)
                    {
                        // BUGBUG (garybu) Need to display error message.
                        SetPage(i);
                        break;
                    }
                }
            }

            UpdateApplyButton();

            return hr == S_OK;
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::SetPage
        //
        //  Synopsis:   Changes the current page.
        //
        //  Arguments:  i Index of page to activate
        //
        //-------------------------------------------------------------------------

        void
        CPropertyDialog::SetPage(int iPage, BOOL fSetTab)
        {
            HRESULT hr = S_OK;
            int     iLevel;

            if (iPage == _iPage)
                return;

            if (_iPage >= 0)
            {
                _arySite[_iPage]->Hide();
                iLevel = _arySite[_iPage]->_iLevel;
            }
            else
            {
                iLevel = -1;
            }

            _iPage = iPage;

            if (_iPage >= 0)
            {
                if (_arySite[_iPage]->_iLevel != iLevel)
                {
                    iLevel = _arySite[_iPage]->_iLevel;
                    UpdateTabs(iLevel);
                    SendMessage(_hwndCombo, CB_SETCURSEL, iLevel, 0);
                }
                if (fSetTab)
                {
                    TabCtrl_SetCurSel(_hwndTabs, _iPage - _aiPageLevel[iLevel]);
                }
                IGNORE_HR(_arySite[_iPage]->Show());
            }
        }

        //+---------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::GetCommonPages
        //
        //  Synopsis:   Find common property pages for selected objects.
        //
        //----------------------------------------------------------------------------

        HRESULT
        CPropertyDialog::GetCommonPages(int cUnk, IUnknown **apUnk, CAUUID *pca)
        {
            HRESULT                hr;
            int                    i;
            UINT                   iScan, iFill, iCompare;
            BOOL                   fFirst = TRUE;
            CAUUID                 caCurrent;
            IUnknown *             pUnk;
            ISpecifyPropertyPages *pSPP;

            pca->cElems = 0;
            pca->pElems = NULL;

            //
            // If _punkBrowseDefault is set, then we show the properties on that
            // object only if no other objects in the list have any pages.
            //
            // Loop one more time than the number of elements to get pages on
            // _punkBrowseDefault if necessary.
            for (i = 0; i <= cUnk; i++)
            {
                if (i == cUnk && _punkBrowseDefault && pca->cElems == 0)
                {
                    // Force the default object's page(s) to be loaded because the
                    // other object(s) did not provide any.
                    pUnk = _punkBrowseDefault;
                    fFirst = TRUE;
                }
                else if (i < cUnk)
                {
                    pUnk = apUnk[i];
                }
                else
                {
                    break;
                }

                if (OK(THR(pUnk->QueryInterface(
                        IID_ISpecifyPropertyPages,
                        (void **)&pSPP))))
                {
                    hr = THR(pSPP->GetPages(fFirst ? pca : &caCurrent));
                    pSPP->Release();
                    if (hr)
                        continue;

                    if (fFirst)
                    {
                        fFirst = FALSE;
                    }
                    else
                    {
                        for (iScan = 0, iFill = 0; iScan < pca->cElems; iScan++)
                        {
                            for (iCompare = 0; iCompare < caCurrent.cElems; iCompare++)
                            {
                                if (caCurrent.pElems[iCompare] == pca->pElems[iScan])
                                    break;
                            }
                            if (iCompare != caCurrent.cElems)
                            {
                                pca->pElems[iFill++] = pca->pElems[iScan];
                            }
                        }
                        pca->cElems = iFill;

                        CoTaskMemFree(caCurrent.pElems);
                    }
                }
            }

            return S_OK;
        }

        //+---------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::CreatePageSites
        //
        //  Synopsis:   Create page sites for the specified objects.
        //
        //----------------------------------------------------------------------------

        HRESULT
        CPropertyDialog::CreatePageSites(int iLevel, int cUnk, IUnknown **apUnk)
        {
            HRESULT hr;
            ULONG   i;
            CAUUID  ca      = { 0, 0 };
            CPropertyPageSite *pSite;

            _aiPageLevel[iLevel] = _arySite.Size();

            // Compute pages to load.

            hr = THR(GetCommonPages(cUnk, apUnk, &ca));
        #if DBG==1    
            if (hr)
            {
                   // put in a blank page in arySites()
                ca.cElems = 1;
                ca.pElems = (GUID *)&CLSID_CCDGenericPropertyPage;
            }
        #endif // DBG==1    

            // Create the sites.

            hr = THR(_arySite.EnsureSize(_arySite.Size() + ca.cElems));
            if (hr)
                goto Cleanup;

            for (i = 0; i < ca.cElems; i++)
            {
                pSite = new CPropertyPageSite();
                if (!pSite)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(pSite->Init(this, iLevel, &ca.pElems[i]));
                if (hr)
                {
                    // If we can't initalize the page, then ignore it.
                    pSite->Close();
                    pSite->Release();
                }
                else
                {
                    _arySite.Append(pSite);
                }

            }

        Cleanup:
            CoTaskMemFree(ca.pElems);
            RRETURN(hr);
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::TranslateWndAccelerator
        //
        //  Synopsis:   Handle accelerators for the tab control.
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyDialog::TranslateTabCtrlAccelerators(LPMSG pmsg)
        {
            HRESULT hr = S_FALSE;
            int iPage;
            int iLevel;
            int d;

            if ((pmsg->message == WM_KEYDOWN || pmsg->message == WM_SYSKEYDOWN) &&
                pmsg->wParam == VK_TAB &&
                (GetKeyState(VK_CONTROL) & 0x8000) &&
                (GetKeyState(VK_MENU) & 0x8000) == 0)
            {
                d = (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1;

                iPage = _iPage + d;
                iLevel = _arySite[_iPage]->_iLevel;
                if (iPage >= _arySite.Size() ||
                    iPage < 0 ||
                    _arySite[iPage]->_iLevel != iLevel)
                {
                    if (d > 0)
                    {
                        for (iPage = 0;
                            _arySite[iPage]->_iLevel != iLevel;
                            iPage++)
                            ;
                    }
                    else
                    {
                        for (iPage = _arySite.Size() - 1;
                            _arySite[iPage]->_iLevel != iLevel;
                            iPage--)
                            ;
                    }
                }

                if (_iPage != iPage)
                {
                    SetPage(iPage);
                }

                hr = S_OK;
            }

            RRETURN1(hr, S_FALSE);
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::PreTranslateMessage
        //
        //  Synopsis:   Handle accelerators
        //
        //-------------------------------------------------------------------------

        HRESULT
        CPropertyDialog::PreTranslateMessage(LPMSG pmsg)
        {
            HRESULT hr;
            HWND hwndFocus, hwndBeforePage, hwndAfterPage;

            if (pmsg->message < WM_KEYFIRST || pmsg->message > WM_KEYLAST)
            {
                hr = S_FALSE;
                goto Cleanup;
            }

            // CDK pages eat the keys we use for flipping pages.
            // Handle these keys first.

            hr = THR(TranslateTabCtrlAccelerators(pmsg));
            if (hr != S_FALSE)
                goto Cleanup;

            hwndFocus      = GetFocus();
            hwndBeforePage = _hwndTabs;
            hwndAfterPage  = _hwndCombo;

            if (GetParent(hwndFocus) != _hwndDialog ||
                    (hwndFocus == hwndBeforePage &&
                        pmsg->wParam == VK_TAB &&
                        pmsg->message == WM_KEYDOWN &&
                        (GetKeyState(VK_SHIFT) & 0x8000) == 0) ||
                    (hwndFocus == hwndAfterPage &&
                        pmsg->wParam == VK_TAB &&
                        pmsg->message == WM_KEYDOWN &&
                        (GetKeyState(VK_SHIFT) & 0x8000)))
            {
                // The focus is in the property page or we are about
                // to tab into the property page.  We let the property
                // page handle the accelerator first.

                _fMessageTranslated = FALSE;
            }
            else
            {
                // Focus is in our child. We get first crack at the message.

                hr = IsDialogMessage(_hwndDialog, pmsg) ? S_OK : S_FALSE;
                if (hr != S_FALSE)
                    goto Cleanup;

                _fMessageTranslated = TRUE;
            }

            // Give the property page a chance to handle the message.

            if (_iPage >= 0)
            {
                hr = THR(_arySite[_iPage]->_pPage->TranslateAccelerator(pmsg));
                if (hr != S_FALSE)
                    goto Cleanup;
            }

            // The CDK pages don't always bubble messages up to the
            // site.  Handle the message now if we have not seen it
            // before.

            if (!_fMessageTranslated)
            {
                hr = IsDialogMessage(_hwndDialog, pmsg) ? S_OK : S_FALSE;
            }

        Cleanup:
            RRETURN1(hr, S_FALSE);
        }

        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::OnCommand
        //
        //  Synopsis:   Handle WM_COMMAND message
        //
        //-------------------------------------------------------------------------

        void
        CPropertyDialog::OnCommand(WORD wNotifyCode, WORD idiCtrl, HWND hwndCtrl)
        {
            switch (idiCtrl)
            {
            case IDCANCEL:
                _hr = _fApplyWasHit ? S_OK : S_FALSE;
                Close();
                break;

            case IDI_PROPDLG_APPLY:
                if (Apply())
                {
                    _fApplyWasHit = TRUE;
                }
                break;

            case IDOK:
                if (Apply())
                {
                    Close();
                }
                break;

            case IDI_PROPDLG_COMBO:
                if (wNotifyCode == CBN_SELCHANGE)
                {
                    int i = SendMessage(_hwndCombo, CB_GETCURSEL, 0, 0);

                    if (i >= 0 && i <= _cUnkLevel)
                    {
                        SetPage(_aiPageLevel[i]);
                    }
                }
                break;
            }
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::OnNotify
        //
        //  Synopsis:   Handle WM_NOTIFY message
        //
        //-------------------------------------------------------------------------

        void
        CPropertyDialog::OnNotify(int idiCtrl, LPNMHDR lpnmhdr)
        {
            int i;

            if (idiCtrl == IDI_PROPDLG_TABS &&
                    lpnmhdr->code == TCN_SELCHANGE &&
                    _iPage >= 0)
            {
                i = TabCtrl_GetCurSel(_hwndTabs);
                SetPage(_aiPageLevel[_arySite[_iPage]->_iLevel] + i, FALSE);
            }
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::InitDialog
        //
        //  Synopsis:   Initialize the dialog
        //
        //-------------------------------------------------------------------------

        BOOL
        CPropertyDialog::OnInitDialog(HWND hwnd)
        {
            static int aidiButtons[] =
                { IDOK, IDCANCEL, IDI_PROPDLG_APPLY, IDI_PROPDLG_COMBO };

            HRESULT         hr = S_OK;
            GDIRECT         rcMax;
            GDIRECT         rc;
            int             i;
            int             dx, dy;
            IHTMLElement *  pElement;
            CElement *      pElem;

            _hwndDialog = hwnd;
            _hwndTabs = GetDlgItem( _hwndDialog, IDI_PROPDLG_TABS );
            _hwndCombo = GetDlgItem( _hwndDialog, IDI_PROPDLG_COMBO );

            _fApplyWasHit = FALSE;

            // Crawl up the parent chain.

            _apUnkLevel[0] = _apUnk[ 0 ];
            _apUnkLevel[0]->AddRef();       // Keeps the cleanup code simple.

            //
            // Don't walk the tree in browse mode
            //
            if (!_punkBrowseDefault)
            {
                for ( _cUnkLevel = 1 ; !hr && _cUnkLevel < ARRAY_SIZE(_apUnkLevel) ; )
                {
                    hr =
                        THR(
                            _apUnkLevel [ _cUnkLevel - 1 ]->QueryInterface(
                                IID_IHTMLElement, (void * *) & pElement ) );

                    if (OK(hr))
                    {
                        //
                        // Get parent of this element
                        //

                        hr =
                            THR(
                                pElement->get_parentElement(
                                    (IHTMLElement **) & _apUnkLevel [ _cUnkLevel ] ) );

                        if ( !hr )
                        {
                            hr =
                                _apUnkLevel [ _cUnkLevel ]->QueryInterface (
                                    CLSID_CElement, (void * *) & pElem );

                            if (!hr)
                            {
                                // Ignore the HTML tag
                                if ( pElem->Tag() == ETAG_HTML )
                                {
                                    _apUnkLevel[ _cUnkLevel ]->Release();
                                    _apUnkLevel[ _cUnkLevel ] = NULL;
                                    hr = E_FAIL;
                                }
                                else
                                {
                                    _cUnkLevel++;
                                }
                            }
                        }

                        pElement->Release();
                    }
                }
            }
            else
            {
                //
                // In browse mode we hide the combo-box and the apply button.
                //
                _cUnkLevel = 1;
                ::EnableWindow(_hwndCombo, FALSE);
                ::ShowWindow(_hwndCombo, SW_HIDE);
                hwnd = GetDlgItem(_hwndDialog, IDI_PROPDLG_APPLY);
                ::ShowWindow(hwnd, SW_HIDE);
            }

            // Load pages for base objects.

            hr = THR(CreatePageSites(0, _cUnk, (IUnknown **) _apUnk));
            if (hr)
                goto Cleanup;

            // Load pages for parent objects.

            for (i = 1; i < _cUnkLevel; ++i)
            {
                hr = THR(CreatePageSites(i, 1, (IUnknown **) &_apUnkLevel[i]));
            }

            // Combobox is hidden, no need to load.
            // Load combobox when _apUnkLevel[i] is (IHTMLDocument2 *) pDoc will set
            // hr to E_FAIL, and eventually fails IDM_PROPERTIES.
            //
            if (!(_punkBrowseDefault))
            {
                // Load the combo box

                for (i = 0; i < _cUnkLevel; ++i)
                {
                    BSTR           bstrTagName = 0;
                    IHTMLElement * pElement;

                    if (OK(_apUnkLevel[i]->QueryInterface(IID_IHTMLElement, (void **)&pElement)))
                    {
                        hr = THR(pElement->get_tagName(&bstrTagName));
                        pElement->Release();
                    }
                    else
                    {
                        hr = E_FAIL;
                    }

                    // BUGBUG Pick default name here?2
                    if (hr)
                        break;

                    SendMessage(_hwndCombo, CB_ADDSTRING, 0, (LPARAM) bstrTagName);
                    SysFreeString(bstrTagName);
                }
            }

            // Compute maximum page size.

            memset(&rcMax, 0, sizeof(rcMax));

            for (i = 0; i < _arySite.Size(); i++)
            {
                if (_arySite[i]->_ppi.size.cx > rcMax.right)
                    rcMax.right = _arySite[i]->_ppi.size.cx;

                if (_arySite[i]->_ppi.size.cy > rcMax.bottom)
                    rcMax.bottom = _arySite[i]->_ppi.size.cy;
            }

            // Load titles into tabs.

            UpdateTabs(0);

            // Move controls to where they belong based on max page rectangle.

            TabCtrl_AdjustRect(_hwndTabs, TRUE, &rcMax);
            GetWindowRect(_hwndTabs, &rc);
            dx = rcMax.right - rcMax.left - rc.right + rc.left;
            dy = rcMax.bottom - rcMax.top - rc.bottom + rc.top;

            // Allow Trident to shrink the properties dialog in browse mode to honor
            // the size given by PMs.
            //
            if (dx < 0 && !_punkBrowseDefault) dx = 0;
            if (dy < 0 && !_punkBrowseDefault) dy = 0;

            MapWindowPoints(NULL, _hwndDialog, (GDIPOINT *)&rc, 2);
            rc.right += dx;
            rc.bottom += dy;
            MoveWindow(_hwndTabs, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);

            _rcPage.left = 0;
            _rcPage.right = rc.right - rc.left;
            _rcPage.top = 0;
            _rcPage.bottom = rc.bottom - rc.top;

            TabCtrl_AdjustRect(_hwndTabs, FALSE, (GDIRECT *) ENSUREOLERECT(&_rcPage));
	        
            GetWindowRect(_hwndDialog, &rc);

            GDIRECT rcDesktop;
            LONG lWidth, lHeight;

            GetWindowRect(GetDesktopWindow(), &rcDesktop);
            lWidth  = rc.right - rc.left + dx;
            lHeight = rc.bottom - rc.top + dy;
            MoveWindow(_hwndDialog,
                    rcDesktop.left + (rcDesktop.right - rcDesktop.left - lWidth) / 2,
                    rcDesktop.top + (rcDesktop.bottom - rcDesktop.top - lHeight) / 2,
                    lWidth,
                    lHeight,
                    FALSE);

            //
            // Move the OK and Cancel buttons over in browse mode since we hide
            // the Apply button.
            //
            if (_punkBrowseDefault)
            {
                GDIRECT rc2;
                hwnd = GetDlgItem(_hwndDialog, IDI_PROPDLG_APPLY);
                GetWindowRect(hwnd, &rc);
                hwnd = GetDlgItem(_hwndDialog, IDCANCEL);
                GetWindowRect(hwnd, &rc2);
                dx += (rc.right - rc.left) + (rc.left - rc2.right);
            }

            for (i = 0; i < ARRAY_SIZE(aidiButtons); i++)
            {
                hwnd = GetDlgItem(_hwndDialog, aidiButtons[i]);
                GetWindowRect(hwnd, &rc);
                MapWindowPoints(NULL, _hwndDialog, (GDIPOINT *)&rc, 2);
                OffsetRect(&rc, aidiButtons[i] != IDI_PROPDLG_COMBO ? dx : 0, dy);
                MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
            }

            // Activate the first page.

            if (_arySite.Size() > 0)
            {
                SetPage(0);
            }

            //  Disable the top-level parent, makes the propdialog modal.

            ::EnableWindow(_hwndTopParent, FALSE);

        Cleanup:
            _hr = hr;
            return FALSE;
        }


        //+------------------------------------------------------------------------
        //
        //  Member:     CPropertyDialog::DlgProc
        //
        //  Synopsis:   Dialog procedure.
        //
        //-------------------------------------------------------------------------

        BOOL CALLBACK
        CPropertyDialog::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            CPropertyDialog *pDialog = (CPropertyDialog *)GetWindowLong(hwnd, DWL_USER);

            switch (msg)
            {
            case WM_INITDIALOG:
                SetWindowLong(hwnd, DWL_USER, lParam);
                pDialog = (CPropertyDialog *)lParam;
                return pDialog->OnInitDialog(hwnd);
                break;

            case WM_CLOSE:
                pDialog->Close();
                break;

            case WM_NOTIFY:
                pDialog->OnNotify((int)wParam, (LPNMHDR)lParam);
                break;

            case WM_COMMAND:
                pDialog->OnCommand(GET_WM_COMMAND_CMD(wParam, lParam),
                                   GET_WM_COMMAND_ID(wParam, lParam), 
                                   GET_WM_COMMAND_HWND(wParam, lParam));
                break;

            default:
                return FALSE;
            }

            return TRUE;
        }

        //+---------------------------------------------------------------------------
        //
        //  Member:     ShowPropertyDialog
        //
        //  Synopsis:   Show the property dialog for the given objects.
        //
        //----------------------------------------------------------------------------

        HRESULT
        ShowPropertyDialog(
                int                 cUnk,
                IUnknown **         apUnk,
                HWND                hwndOwner,
                IServiceProvider *  pServiceProvider,
                LCID                lcid,
                IUnknown *          punkBrowseDefault)
        {
            if (!cUnk)
                return S_OK;

            IDispatch *         pDispBase = NULL;
            CPropertyDialog     Dialog;
            MSG                 msg;
            HWND                hwndTop;
            HWND                hwnd;
    
            memset(&Dialog, 0, sizeof(Dialog));

            Dialog._iPage               = -1;
            Dialog._cUnk                = cUnk;
            Dialog._apUnk               = apUnk;
            Dialog._pServiceProvider    = pServiceProvider;
            Dialog._lcid                = lcid;
            Dialog._punkBrowseDefault   = punkBrowseDefault;

            //  compute top-level parent

            for ( hwnd = hwndTop = hwndOwner;
                  hwnd;
                  hwnd = GetParent(hwnd) )
            {
                hwndTop = hwnd;
            }
            Dialog._hwndTopParent    = hwndTop;

            if (!CreateDialogParam(
                    GetResourceHInst(),
                    MAKEINTRESOURCE(IDR_PROPERTIES_DIALOG),
                    hwndOwner,
                    &CPropertyDialog::DlgProc,
                    (LPARAM)&Dialog))
            {
                Dialog._hr = E_FAIL;
                goto Cleanup;
            }

        #ifdef UNIX
            // IEUNIX: Need to tell window manager that I'm modal
            MwSetModalPopup(Dialog._hwndDialog, TRUE);
        #endif

            ShowWindow(Dialog._hwndDialog, SW_SHOWNA);

            while (Dialog._hwndDialog)
            {
                GetMessage(&msg, NULL, 0, 0);

                if (Dialog.PreTranslateMessage(&msg) != S_OK)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }


        Cleanup:
            RRETURN1(Dialog._hr, S_FALSE);
        }
        #endif // NO_HTML_DIALOG

#endif NEVER


//----------------------------------------------------------------------------
//  WARNING - We don't want to "taint" the property dialog with with any
//  knowledge of our internals.  This so that our hosts can duplicate our
//  UI from clean interfaces.  So don't move this include any higher in this
//  file and don't put any new functions below this line.
//

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ShowPropertyDialog
//
//  Synopsis:   Show the property dialog for the given array of sites.
//
//----------------------------------------------------------------------------


HRESULT
CDoc::ShowPropertyDialog(int cElements, CElement ** apElement)
{
#ifdef NO_HTML_DIALOG
    return S_OK;
#else        
    HRESULT             hr = E_FAIL;   
    SAFEARRAY         * psafearray = NULL;
    IUnknown * HUGEP  * apUnk = NULL;
    IOleCommandTarget * pBackupHostUICommandHandler = NULL;  
    EVENTPARAM          param(this, TRUE);
    VARIANT             varIn;
    int                 i;           
    HWND                hwnd = NULL;
    HWND                hwndParent;

    Assert(cElements >= 0);
    Assert(apElement || cElements==0);
    VariantInit(&varIn);

    psafearray = SafeArrayCreateVector(VT_UNKNOWN, 0, cElements);
    if (!psafearray)        
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // fill the safe array with punks
    if (cElements)
    {
        hr = SafeArrayAccessData(psafearray, (void HUGEP* FAR*)&apUnk);
        if (hr)
            goto Cleanup;
        Assert(apUnk);
        
        for (i = 0; i < cElements; ++i)
        {
            Assert(apElement[i]);           
            apUnk[i] = apElement[i]->PunkInner();
            apUnk[i]->AddRef();
        }
    }

    {
        CDoEnableModeless   dem(this);
    
        if (!dem._hwnd)
        {
            hr = E_FAIL;
            goto Cleanup;
        }    
     
        hwnd = dem._hwnd;

        // set up expandos         
        param.SetType(_T("propertysheet"));             
        param.propertysheetParams.paPropertysheetPunks     = psafearray;
        
        V_VT(&varIn) = VT_UNKNOWN;    

        QueryInterface(IID_IUnknown, (void**)&V_UNKNOWN(&varIn));

        //V_UNKNOWN(&varIn) = (IUnknown*)(IPrivateUnknown *)this;
        //V_UNKNOWN(&varIn)->AddRef();
        
        // Query host to show dialog    
        if (_pHostUICommandHandler)               
        {                             
            hr = _pHostUICommandHandler->Exec(                
                &CGID_DocHostCommandHandler,                        
                OLECMDID_PROPERTIES,                        
                0,                                           
                &varIn,                                            
                NULL);              
            if (SUCCEEDED(hr))
                goto Cleanup;        
        }
            
        // Let backup show dialog                                            
        EnsureBackupUIHandler();                            
        if (_pBackupHostUIHandler)                                        
        {                                                                                                  
            hr = _pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,                                            
                (void **) &pBackupHostUICommandHandler);                                                            
            if (hr)                                                                         
                goto Cleanup;                                            
                        
            hr = pBackupHostUICommandHandler->Exec(                                
                &CGID_DocHostCommandHandler,                                            
                OLECMDID_PROPERTIES,                                            
                0,                                            
                &varIn,                                            
                NULL);                                                                                   
        }
    }
    
Cleanup:
    // need to re-focus parent frame, since OleCreatePropertyFrameIndirect
    // does not return focus to its window owner
    if (hwnd)
    {
        for(hwndParent = GetParent(hwnd); 
            hwndParent;
            hwndParent = GetParent(hwnd) )
            hwnd = hwndParent;
        SetActiveWindow(hwnd);
    }
    SetFocus(TRUE);
    
    if (psafearray)
    {
        if (apUnk)
            SafeArrayUnaccessData(psafearray);
        SafeArrayDestroy(psafearray);
    }

    ReleaseInterface(pBackupHostUICommandHandler);
    VariantClear(&varIn);   

    RRETURN1(hr, S_FALSE);
#endif // NO_HTML_DIALOG
}


STDAPI CreateHTMLPropertyPage(       
        IMoniker *          pmk,
        IPropertyPage **    ppPP)
{
    HRESULT             hr = E_FAIL;    
    HTMLDLGINFO         dlginfo;
    CEnsureThreadState  ets;
       
    Assert(pmk);
    Assert(ppPP);       
  
    if (ppPP)
        *ppPP = NULL;

    if (!pmk || !ppPP)
        goto Cleanup;    

    hr = ets._hr;
    if (FAILED(hr))
        goto Cleanup;    
        
    dlginfo.pmk             = pmk;
    dlginfo.fPropPage       = TRUE;

    hr = THR(CHTMLDlg::CreateHTMLDlgIndirect(NULL, &dlginfo, IID_IPropertyPage, (void**)ppPP));
    if (hr)
        goto Cleanup;   

Cleanup:    
    if (hr && ppPP)    
    {
        ReleaseInterface(*ppPP);   
        *ppPP = NULL;
    }

    RRETURN(hr);
}
