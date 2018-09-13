#include "priv.h"
#include "sccls.h"
#include "mnstatic.h"
#include "menuband.h"
#include "itbar.h"
#include "../lib/dpastuff.h"       // COrderList_*
#include "inpobj.h"
#include "resource.h"
#include "mnbase.h"
#include "oleacc.h"
#include "apithk.h"
#include "menuisf.h"

HMENU g_hmenuStopWatch = NULL;
UINT g_idCmdStopWatch = 0;

//***   IDTTOIDM -- convert idtCmd to idmMenu
// NOTES
//  as an optimization, we make the toolbar idtCmd the same as the menu idm.
// this macro (hopefully) makes things a bit clearer in the code by making
// the type conversion explicit.
#define IDTTOIDM(idtBtn)   (idtBtn)

BOOL TBHasImage(HWND hwnd, int iImageIndex);

//------------------------------------------------------------------------
//
// CMenuStaticToolbar::CMenuStaticData class
//
//------------------------------------------------------------------------


CMenuStaticToolbar::CMenuStaticData::~CMenuStaticData()
{
    ATOMICRELEASE(_punkSubMenu);
}


void CMenuStaticToolbar::CMenuStaticData::SetSubMenu(IUnknown* punk)
{
    ATOMICRELEASE(_punkSubMenu);
    _punkSubMenu = punk;
    if (_punkSubMenu)
        _punkSubMenu->AddRef();
}


HRESULT CMenuStaticToolbar::CMenuStaticData::GetSubMenu(const GUID* pguidService, REFIID riid, void** ppvObj)
{
    if (_punkSubMenu)
    {
        if (pguidService)
        {
            return IUnknown_QueryService(_punkSubMenu, *pguidService, riid, ppvObj);
        }
        else
            return _punkSubMenu->QueryInterface(riid, ppvObj);
    }
    else
        return E_NOINTERFACE;
}



//------------------------------------------------------------------------
//
// CMenuStaticToolbar
//
//------------------------------------------------------------------------



CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand* pmb, HMENU hmenu, HWND hwnd, UINT idCmd, DWORD dwFlags)
    : CMenuToolbarBase(pmb, dwFlags)
{
    _hmenu = hmenu;
    _hwndMenuOwner = hwnd;
    _idCmd = idCmd;
    _iDragOverButton = -1;
    _fDirty = TRUE;
}


CMenuStaticToolbar::~CMenuStaticToolbar()
{
    if (!(_dwFlags & SMSET_DONTOWN))
    {
        DestroyMenu(_hmenu);
    }
}


STDMETHODIMP CMenuStaticToolbar::QueryInterface(REFIID riid, void** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CMenuStaticToolbar, IDropTarget),
        { 0 },
    };

    // BUGBUG: If you QI MenuStatic for a drop target, you get a different
    // one than if you QI MenuShellFolder. This breaks COM identity rules.
    // Proper fix would be to implement a drop target that encapsulates both.
    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = CMenuToolbarBase::QueryInterface(riid, ppvObj);

    return hres;
}

void CMenuStaticToolbar::_CheckSeparators()
{
    if (_fHasTopSep)
    {
        if (_pcmb->_pmtbTop->DontShowEmpty() )
        {
            if (!_fTopSepRemoved)
            {
                SendMessage(_hwndMB, TB_DELETEBUTTON, 0, 0);
                _fTopSepRemoved = TRUE;
            }
        }
        else
        {
            if (_fTopSepRemoved)
            {
                MENUITEMINFO mii;
                mii.cbSize = sizeof(mii);
                mii.fType = MFT_SEPARATOR;
                _Insert(0, &mii);
                _fTopSepRemoved = FALSE;
            }
        }
    }

    if (_fHasBottomSep)
    {
        if (_pcmb->_pmtbBottom->DontShowEmpty() )
        {
            if (!_fBottomSepRemoved)
            {
                SendMessage(_hwndMB, TB_DELETEBUTTON, ToolBar_ButtonCount(_hwndMB) - 1, 0);
                _fBottomSepRemoved = TRUE;
            }
        }
        else
        {
            if (_fBottomSepRemoved)
            {
                MENUITEMINFO mii;
                mii.cbSize = sizeof(mii);
                mii.fType = SMIT_SEPARATOR;
                _Insert(-1, &mii);
                _fBottomSepRemoved = FALSE;
            }
        }
    }
}


void CMenuStaticToolbar::v_Show(BOOL fShow, BOOL fForceUpdate)
{
    CMenuToolbarBase::v_Show(fShow, fForceUpdate);
    _fShowMB = fShow;
    if (fShow)
    {
        _fFirstTime = FALSE;
        _fClickHandled = FALSE;
        _FillToolbar();
        _pcmb->SetTracked(NULL);
        ToolBar_SetHotItem(_hwndMB, -1);

        // Have the menubar think about changing its height
        IUnknown_QueryServiceExec(_pcmb->_punkSite, SID_SMenuPopup, &CGID_MENUDESKBAR, 
            MBCID_SETEXPAND, (int)_pcmb->_fExpanded, NULL, NULL);

        if (fForceUpdate)
            v_UpdateButtons(FALSE);
#if 0
        // need top level frame available for D&D if possible.
        _hwndDD = GetParent(_hwndMB);
        IOleWindow *pOleWindow;
        HRESULT hr = IUnknown_QueryService(_pcmb->_punkSite, SID_STopLevelBrowser
                                                , IID_IOleWindow, (void **)&pOleWindow);
        if(SUCCEEDED(hr))
        { 
            ASSERT(pOleWindow);
            pOleWindow->GetWindow(&_hwndDD);
            pOleWindow->Release();
        }
#endif
        CDelegateDropTarget::Init();
    }
    else
        KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
    // n.b. for !fShow, we don't kill the tracked site chain.  we
    // count on this in startmnu.cpp!CStartMenuCallback::_OnExecItem,
    // where we walk up the chain to find all hit 'nodes'.  if we need
    // to change this we could fire a 'pre-exec' event.
}

void CMenuStaticToolbar::_Insert(int iIndex, MENUITEMINFO* pmii)
{
    CMenuStaticData* pmsd = new CMenuStaticData();
    if (pmsd)
    {
        BYTE bTBStyle = TBSTYLE_BUTTON | TBSTYLE_DROPDOWN;

        SMINFO sminfo = {0};
        sminfo.dwMask = SMIM_TYPE | SMIM_FLAGS | SMIM_ICON;


        // These are somethings that the callback does not fill in:
        if ( pmii->hSubMenu )
            sminfo.dwFlags |= SMIF_SUBMENU;

        if ( pmii->fState & MFS_CHECKED)
            sminfo.dwFlags |= SMIF_CHECKED;

        if (pmii->fState & MFS_DISABLED || pmii->fState & MFS_GRAYED)
            sminfo.dwFlags |= SMIF_DISABLED;

        if ( pmii->fType & MFT_SEPARATOR)
        {
            sminfo.dwType = SMIT_SEPARATOR;
            bTBStyle &= ~TBSTYLE_BUTTON;
            bTBStyle |= TBSTYLE_SEP;
        }
        else
            sminfo.dwType = SMIT_STRING;

        if (!_fVerticalMB)
            bTBStyle |= TBSTYLE_AUTOSIZE;

        if (S_OK != CallCB(pmii->wID, SMC_GETINFO, 0, (LPARAM)&sminfo))
        {
            sminfo.iIcon = -1;
        }

        pmsd->_dwFlags = sminfo.dwFlags;

        // Now add it to the toolbar
        TBBUTTON tbb = {0};

        tbb.iBitmap = sminfo.iIcon;
        tbb.idCommand = pmii->wID;
        tbb.dwData = (DWORD_PTR)pmsd;
        tbb.fsState = (sminfo.dwFlags & SMIF_HIDDEN)?TBSTATE_HIDDEN : TBSTATE_ENABLED;
        tbb.fsStyle = bTBStyle; 

        TCHAR szMenuString[MAX_PATH];

        if (pmii->fType & MFT_OWNERDRAW)
        {
            // dwTypeData is user defined 32 bit value, not a string if MFT_OWNERDRAW is set
            // then the (unicode) string is the very first element in a structure dwItemData
            // points to
            LPWSTR pwsz = (LPWSTR)pmii->dwItemData;
            SHUnicodeToTChar(pwsz, szMenuString, ARRAYSIZE(szMenuString));
            tbb.iString = (INT_PTR)(szMenuString);
        }
        else
            tbb.iString = (INT_PTR)(LPTSTR)pmii->dwTypeData;

        SendMessage(_hwndMB, TB_INSERTBUTTON, iIndex, (LPARAM)&tbb);
    }
}


/*----------------------------------------------------------
Purpose: GetMenu method

*/
HRESULT CMenuStaticToolbar::GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags)
{
    if (phmenu)
        *phmenu = _hmenu;
    if (phwnd)
        *phwnd = _hwndMenuOwner;
    if (pdwFlags)
        *pdwFlags = _dwFlags;

    return NOERROR;
}

HRESULT CMenuStaticToolbar::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{
    // When we are merging in a new menu, we need to destroy the old one if we own it.
    if (_hmenu && !(_dwFlags & SMSET_DONTOWN))
    {
        DestroyMenu(_hmenu);
    }

    _hmenu = hmenu;
    // If we're processing a change notify, we cannot do anything that will modify state.
    if (_pcmb->_pmbState && 
        _pcmb->_pmbState->IsProcessingChangeNotify())
    {
        _fDirty = TRUE;
    }
    else
    {
        EmptyToolbar();
        _pcmb->_fInSubMenu = FALSE;
        IUnknown_SetSite(_pcmb->_pmpSubMenu, NULL);
        ATOMICRELEASE(_pcmb->_pmpSubMenu);

        if (_fShowMB)
            _FillToolbar();

        BOOL fSmooth = FALSE;
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
        SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fSmooth, 0);
#endif

        // This causes a paint to occur right away instead of waiting until the
        // next message dispatch which could take a noticably long time.
        RedrawWindow(_hwndMB, NULL, NULL, (fSmooth? RDW_ERASE: 0) | RDW_INVALIDATE | RDW_UPDATENOW);  
    }
    return NOERROR;
}


CMenuStaticToolbar::CMenuStaticData* CMenuStaticToolbar::_IDToData(int idCmd)
{
    CMenuStaticData* pmsd= NULL;

    // Initialize to NULL in case the GetButtonInfo Fails. We won't fault because
    // the lParam is just stack garbage. 
    TBBUTTONINFO tbbi = {0};
    int iPos;

    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_LPARAM;

    iPos = ToolBar_GetButtonInfo(_hwndMB, idCmd, &tbbi);
    if (iPos >= 0)
        pmsd = (CMenuStaticData*)tbbi.lParam;

    return pmsd;
}

HRESULT CMenuStaticToolbar::v_CreateTrackPopup(int idCmd, REFIID riid, void** ppvObj) 
{
    HRESULT hres = E_OUTOFMEMORY;
    int iPos = (int)SendMessage(_hwndMB, TB_COMMANDTOINDEX, idCmd, 0);

    if (iPos >= 0)
    {
        CTrackPopupBar* ptpb = new CTrackPopupBar(_pcmb->_pmbState->GetContext(), iPos, _hmenu, _hwndMenuOwner);

        if (ptpb)
        {
            hres = ptpb->QueryInterface(riid, ppvObj);
            if (SUCCEEDED(hres))
                IUnknown_SetSite(SAFECAST(ptpb, IMenuPopup*), SAFECAST(_pcmb, IMenuPopup*));

            PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), g_nMBAutomation, (WPARAM)_hmenu, (LPARAM)iPos);
            ptpb->Release();
        }
    }

    return hres;
}

HRESULT CMenuStaticToolbar::v_GetSubMenu(int idCmd, const GUID* pguidService, REFIID riid, void** ppvObj)
{
    HRESULT hres = E_FAIL;
    CMenuStaticData* pmsd = _IDToData(idCmd);

    ASSERT(IS_VALID_WRITE_PTR(ppvObj, void*));

    *ppvObj = NULL;

    if (EVAL(pmsd))
    {
        // Get the cached submenu
        hres = pmsd->GetSubMenu(pguidService, riid, ppvObj);

        // Did that fail?
        if (FAILED(hres) && (pmsd->_dwFlags & SMIF_SUBMENU) && 
            IsEqualGUID(riid, IID_IShellMenu))
        {
            // Yes; ask the callback for it
            hres = CallCB(idCmd, SMC_GETOBJECT, (WPARAM)&riid, (LPARAM)ppvObj);

            if (S_OK != hres)
            {
                hres = E_OUTOFMEMORY;   // Set to error case incase something happens

                // Callback didn't handle it, try and see if we can get it
                MENUITEMINFO mii;
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU | MIIM_ID;
                if (GetMenuItemInfoWrap(_hmenu, idCmd, MF_BYCOMMAND, &mii) && mii.hSubMenu)
                {
                    IShellMenu* psm = (IShellMenu*)new CMenuBand();
                    if (psm)
                    {
                        UINT uIdAncestor = _pcmb->_uIdAncestor;
                        if (uIdAncestor == ANCESTORDEFAULT)
                            uIdAncestor = idCmd;

                        psm->Initialize(_pcmb->_psmcb, idCmd, uIdAncestor, SMINIT_VERTICAL);
                        psm->SetMenu(mii.hSubMenu, _hwndMenuOwner, SMSET_TOP | SMSET_DONTOWN);
                        hres = psm->QueryInterface(riid, ppvObj);
                        psm->Release();
                    }
                }
            }

            if (*ppvObj)
            {
                // Cache it now
                pmsd->SetSubMenu((IUnknown*)*ppvObj);

                // Initialize the fonts
                VARIANT Var;
                Var.vt = VT_UNKNOWN;
                Var.byref = SAFECAST(_pcmb->_pmbm, IUnknown*);
                IUnknown_Exec((IUnknown*)*ppvObj, &CGID_MenuBand, MBANDCID_SETFONTS, 0, &Var, NULL);

                // Set the CMenuBandState  into the new menuband
                Var.vt = VT_INT_PTR;
                Var.byref = _pcmb->_pmbState;
                IUnknown_Exec((IUnknown*)*ppvObj, &CGID_MenuBand, MBANDCID_SETSTATEOBJECT, 0, &Var, NULL);

            }
        }
    }

    return hres;
}


HRESULT CMenuStaticToolbar::v_GetInfoTip(int idCmd, LPTSTR psz, UINT cch)
{
    return CallCB(idCmd, SMC_GETINFOTIP, (WPARAM)psz, (LPARAM)cch);
}


HRESULT CMenuStaticToolbar::v_ExecItem(int idCmd)
{
    HRESULT hres = CallCB(idCmd, SMC_EXEC, 0, 0);

    if (S_OK != hres && _hwndMenuOwner)
    {
        PostMessage(_hwndMenuOwner, WM_COMMAND, idCmd, 0);
        hres = NOERROR;
    }

    return hres;
}

DWORD CMenuStaticToolbar::v_GetFlags(int idCmd)
{
    CMenuStaticData* pmsd = _IDToData(idCmd);

    // Toolbar is allowed to pass a bad command in the case of erasing the background
    if (pmsd)
    {
        return pmsd->_dwFlags;
    }
    else
        return 0;
}


void CMenuStaticToolbar::v_SendMenuNotification(UINT idCmd, BOOL fClear)
{
    if (S_FALSE == CallCB(idCmd, SMC_SELECTITEM, (WPARAM)fClear, 0))
    {
        UINT uFlags = (UINT)-1;
        if (v_GetFlags(idCmd) & SMIF_SUBMENU)
             uFlags = MF_POPUP;

        if (!fClear)
            uFlags = MF_HILITE;

        PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), WM_MENUSELECT,
            MAKEWPARAM(idCmd, uFlags), fClear? NULL : (LPARAM)_hmenu);

    }
}

BOOL CMenuStaticToolbar::v_TrackingSubContextMenu()
{
    return (_pcm != NULL);
}

void CMenuStaticToolbar::CreateToolbar(HWND hwndParent)
{
    if (!_hwndMB)
    {
        _hwndMB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, TEXT("Menu"),
                                 WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                                 WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                 CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                 CCS_NORESIZE  | TBSTYLE_REGISTERDROP,
                                 0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);

        if (!_hwndMB)
        {
            TraceMsg(TF_MENUBAND, "CMenuStaticToolbar::CreateToolbar: Failed to Create Toolbar");
            return;
        }

        SendMessage(_hwndMB, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndMB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        // Set the format to ANSI or UNICODE as appropriate.
        ToolBar_SetUnicodeFormat(_hwndMB, DLL_IS_UNICODE);

        _SubclassWindow(_hwndMB);
        _RegisterWindow(_hwndMB, NULL, SHCNE_UPDATEIMAGE);
        SIZE size;
        RECT rc;

        SystemParametersInfoA(SPI_GETWORKAREA, SIZEOF(RECT), &rc, FALSE);
        //HACKHACK:  THIS WILL FORCE NO WRAP TO HAPPEN FOR PROPER WIDTH CALC WHEN PAGER IS PRESENT.
        size.cx = RECTWIDTH(rc);
        size.cy = 32000;
        ToolBar_SetBoundingSize(_hwndMB, &size);
        CMenuToolbarBase::CreateToolbar(hwndParent);
    }
    else if (GetParent(_hwndMB) != hwndParent)
        ::SetParent(_hwndMB, hwndParent);
}


STDMETHODIMP CMenuStaticToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (SHCNE_UPDATEIMAGE == lEvent) // global
    {
        if (pidl1)
        {
            int iImage = *(int UNALIGNED *)((BYTE *)pidl1 + 2);

            IEInvalidateImageList();    // We may need to use different icons.
            if ( pidl2 )
            {
                iImage = SHHandleUpdateImage( pidl2 );
                if ( iImage == -1 )
                {
                    return E_FAIL;
                }
            }
            
            if (iImage == -1 || TBHasImage(_hwndMB, iImage))
                v_Refresh();
        } 
        else
            v_Refresh();

        return S_OK;
    }

    return E_FAIL;
}


LRESULT CMenuStaticToolbar::_DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    switch(uMessage)
    {
    case WM_TIMER:
        if (_OnTimer(wParam)) 
            return 1;
        break;
    case WM_GETOBJECT:
        // Yet another poor design choice on the part of the accessibility team.
        // Typically, if you do not answer a WM_* you return 0. They choose 0 as their success
        // code.
        return _DefWindowProcMB(hwnd, uMessage, wParam, lParam);
        break;

    }

    return CNotifySubclassWndProc::_DefWindowProc(hwnd, uMessage, wParam, lParam);
}


//***
// NOTES
//  idtCmd is currently always -1.  we'll need other values when we're
// called from CallCB.  however we can't do that until we fix mnfolder.cpp.
HRESULT CMenuStaticToolbar::v_GetState(int idtCmd, LPSMDATA psmd)
{
    psmd->dwMask = SMDM_HMENU;

    psmd->hmenu = _hmenu;
    psmd->hwnd = _hwndMenuOwner;
    psmd->uIdParent = _idCmd;
    if (idtCmd == -1)
        idtCmd = GetButtonCmd(_hwndMB, ToolBar_GetHotItem(_hwndMB));
    psmd->uId = IDTTOIDM(idtCmd);
    psmd->punk = SAFECAST(_pcmb, IShellMenu*);
    psmd->punk->AddRef();

    return S_OK;
}

HRESULT CMenuStaticToolbar::CallCB(UINT idCmd, DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
    if (!_pcmb->_psmcb)
        return S_FALSE;

    SMDATA smd;
    HRESULT hres = S_FALSE;

    // todo: call v_GetState (but see comment in mnfolder.cpp)
    smd.dwMask = SMDM_HMENU;

    smd.hmenu = _hmenu;
    smd.hwnd = _hwndMenuOwner;
    smd.uIdParent = _idCmd;
    smd.uIdAncestor = _pcmb->_uIdAncestor;
    smd.uId = idCmd;
    smd.punk = SAFECAST(_pcmb, IShellMenu*);
    smd.pvUserData = _pcmb->_pvUserData;

    hres = _pcmb->_psmcb->CallbackSM(&smd, dwMsg, wParam, lParam);

    return hres;
}

HRESULT CMenuStaticToolbar::v_CallCBItem(int idtCmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int idm;

    idm = IDTTOIDM(idtCmd);
    return CallCB(idm, uMsg, wParam, lParam);
}



void CMenuStaticToolbar::v_UpdateButtons(BOOL fNegotiateSize)
{
    if (_hwndMB)
    {
        _SetToolbarState();
        int cxMin, cxMax;
        v_CalcWidth(&cxMin, &cxMax);

        SendMessage(_hwndMB, TB_SETBUTTONWIDTH, 0, MAKELONG(cxMin, cxMax));
        SendMessage(_hwndMB, TB_AUTOSIZE, 0, 0);

        // Should we renegotiate size? AND are we vertical,
        // because we cannot renegoitate when horizontal.
        if (fNegotiateSize && _fVerticalMB)
            NegotiateSize();
    }
}


BOOL CMenuStaticToolbar::v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons)
{
    if (-1 == uIconSize)
        uIconSize = _uIconSizeMB;
    BOOL fChanged = (_uIconSizeMB != uIconSize);
    
    _uIconSizeMB = uIconSize;

    if (_hwndMB)
    {
        HIMAGELIST himl = NULL;
        if (_fVerticalMB)
        {
            HIMAGELIST himlLarge, himlSmall;

            // set the imagelist size
            Shell_GetImageLists(&himlLarge, &himlSmall);
            himl = (_uIconSizeMB == ISFBVIEWMODE_LARGEICONS ) ? himlLarge : himlSmall;
        }

        // sending a null himl is significant..  it means no image list
        SendMessage(_hwndMB, TB_SETIMAGELIST, 0, (LPARAM)himl);
                
        if (fUpdateButtons)
            v_UpdateButtons(TRUE);
    }
    
    return fChanged;
}


void CMenuStaticToolbar::_OnGetDispInfo(LPNMHDR pnm, BOOL fUnicode) 
{
    LPNMTBDISPINFO pdi = (LPNMTBDISPINFO)pnm;
    CMenuStaticData* pdata = (CMenuStaticData*)pdi->lParam;
    
    if(pdi->dwMask & TBNF_IMAGE) 
    {
        if (_fVerticalMB)
        {
            SMINFO smi;
            smi.dwMask = SMIM_ICON;
            if (CallCB(pdi->idCommand, SMC_GETINFO, 0, (LPARAM)&smi) == S_OK)
                pdi->iImage = smi.iIcon;
            else
                pdi->iImage = -1;
        }
        else
            pdi->iImage = -1;

    }
    
    if(pdi->dwMask & TBNF_TEXT) 
    {
        if(pdi->pszText) 
        {
            if(fUnicode) 
            {
                pdi->pszText[0] = TEXT('\0');
            }
            else 
            {
                pdi->pszText[0] = 0;
            }
        }
    }
    pdi->dwMask |= TBNF_DI_SETITEM;

    return;
}

LRESULT CMenuStaticToolbar::_OnGetObject(NMOBJECTNOTIFY* pon)
{
    pon->hResult = QueryInterface(*pon->piid, &pon->pObject);

    return 1;
}


LRESULT CMenuStaticToolbar::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;

    switch (pnm->code)
    {
    case TBN_DRAGOUT:
        lres = 0;
        break;
    
    case TBN_DELETINGBUTTON:
    {
        if (!_fEmptyingToolbar)
        {
            TBNOTIFY *ptbn = (TBNOTIFY*)pnm;
            CMenuStaticData* pmsd = (CMenuStaticData*)ptbn->tbButton.dwData;
            if (pmsd)
                delete pmsd;
        }
        break;    
    }

    case NM_TOOLTIPSCREATED:
        SHSetWindowBits(((NMTOOLTIPSCREATED*)pnm)->hwndToolTips, GWL_STYLE, TTS_ALWAYSTIP | TTS_TOPMOST, TTS_ALWAYSTIP | TTS_TOPMOST);
        SendMessage(((NMTOOLTIPSCREATED*)pnm)->hwndToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAXSHORT);        
        break;

    case NM_RCLICK:
        lres = _OnContextMenu(NULL, GetMessagePos());
        break;

    case NM_CUSTOMDRAW:
        lres =  _OnCustomDraw((NMCUSTOMDRAW*)pnm);
        g_hmenuStopWatch = _hmenu;
        g_idCmdStopWatch = _idCmd;
        break;
    
    case  TBN_GETDISPINFOA:
        _OnGetDispInfo(pnm,  FALSE);
        break;
    
    case  TBN_GETDISPINFOW:
        _OnGetDispInfo(pnm,  TRUE);
        break;

    case TBN_GETOBJECT:
        lres = _OnGetObject((NMOBJECTNOTIFY*)pnm);
        break;

    case TBN_MAPACCELERATOR:
        lres = _OnAccelerator((NMCHAR*)pnm);
        break;

    default:
        lres = CMenuToolbarBase::_OnNotify(pnm);

    }

    return(lres);
}

void CMenuStaticToolbar::_FillToolbar()
{
    if (_fDirty && _hmenu && _hwndMB && !_pcmb->_fClosing)
    {
        EmptyToolbar();
        BOOL_PTR fRedraw = SendMessage(_hwndMB, WM_SETREDRAW, FALSE, 0);

        TCHAR szName[MAX_PATH];
        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE | MIIM_DATA;

        int iCount = GetMenuItemCount(_hmenu);
        for (int i = 0; i < iCount; i++)
        {
            mii.dwTypeData = szName;
            mii.cch = ARRAYSIZE(szName);
            if (GetMenuItemInfoWrap(_hmenu, i, MF_BYPOSITION, &mii))
            {
                if (mii.fType & MFT_SEPARATOR)
                {
                    if (i == 0)
                        _fHasTopSep = TRUE;
                    else if (i == iCount - 1)
                        _fHasBottomSep = TRUE;
                }

                _Insert(i, &mii);
            }
        }

        if (iCount == 0)
            _fEmpty = TRUE;

        SendMessage(_hwndMB, WM_SETREDRAW, fRedraw, 0);

        _fDirty = FALSE;
        v_UpdateButtons(FALSE);
        _pcmb->ResizeMenuBar();
    }
}

void CMenuStaticToolbar::v_OnDeleteButton(LPVOID pData)
{
    CMenuStaticData* pmsd = (CMenuStaticData*)pData;
    if (pmsd)
        delete pmsd;
}

void CMenuStaticToolbar::v_OnEmptyToolbar()
{
    CMenuToolbarBase::v_OnEmptyToolbar();
    _fDirty = TRUE;
    _fHasTopSep = FALSE;
    _fHasBottomSep = FALSE;
    _fTopSepRemoved = FALSE;
    _fBottomSepRemoved = FALSE;
}

void CMenuStaticToolbar::v_Close()
{
    if (_hwndMB)
    {
        _UnregisterWindow(_hwndMB);
        _UnsubclassWindow(_hwndMB);
    }
    CMenuToolbarBase::v_Close();
}

void CMenuStaticToolbar::v_Refresh()
{
    EmptyToolbar();
    _FillToolbar();
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::IsWindowOwner method

         Processes messages passed on from the menuband.
*/
STDMETHODIMP CMenuStaticToolbar::IsWindowOwner(HWND hwnd) 
{ 
    if ( hwnd == _hwndMB || hwnd == HWND_BROADCAST) 
        return S_OK;
    else 
        return S_FALSE; 
}




/*----------------------------------------------------------
Purpose: CDelegateDropTarget::GetWindowsDDT

*/
HRESULT CMenuStaticToolbar::GetWindowsDDT (HWND * phwndLock, HWND * phwndScroll) 
{ 
    *phwndLock = _hwndMB;
    *phwndScroll = _hwndMB; 
    return S_OK;
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::HitTestDDT

*/
HRESULT CMenuStaticToolbar::HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD * pdwId, DWORD *pdwEffect)
{
    switch (nEvent)
    {
    case HTDDT_ENTER:
        // OLE is in its modal drag/drop loop, and it has the capture.
        // We shouldn't take the capture back during this time.
        if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_DRAGDROP))
        {
            _pcmb->_pmbState->HasDrag(TRUE);
            g_msgfilter.PreventCapture(TRUE);
            if (_pcmb->_pmtbShellFolder &&
                _pcmb->_pmtbShellFolder->DontShowEmpty())
            {
                DAD_ShowDragImage(FALSE);
                _pcmb->_pmtbShellFolder->DontShowEmpty(FALSE);
                _pcmb->ResizeMenuBar();
                UpdateWindow(_hwndMB);
                DAD_ShowDragImage(TRUE);
            }
            return S_OK;
        }
        else
            return S_FALSE;

    case HTDDT_OVER:
        {
            TBINSERTMARK tbim;
            *pdwEffect = DROPEFFECT_NONE;

            POINT pt = *ppt;

            if (!ToolBar_InsertMarkHitTest(_hwndMB, &pt, &tbim))
            {
                int idCmd = GetButtonCmd(_hwndMB, tbim.iButton);
                if (v_GetFlags(idCmd) & SMIF_DROPCASCADE &&
                    tbim.iButton != _iDragOverButton)
                {
                    DAD_ShowDragImage(FALSE);
                    _pcmb->SetTracked(this);
                    _iDragOverButton = tbim.iButton;
                    SetTimer(_hwndMB, MBTIMER_DRAGOVER, MBTIMER_TIMEOUT, NULL);
                    _pcmb->_SiteOnSelect(MPOS_CHILDTRACKING);
                    BOOL_PTR fOldAnchor = ToolBar_SetAnchorHighlight(_hwndMB, FALSE);
                    ToolBar_SetHotItem(_hwndMB, _iDragOverButton);
                    ToolBar_SetAnchorHighlight(_hwndMB, fOldAnchor);
                    UpdateWindow(_hwndMB);
                    DAD_ShowDragImage(TRUE);
                }
            }
        }
        break;

    case HTDDT_LEAVE:
        // We can take the capture back anytime now
        _pcmb->_pmbState->HasDrag(FALSE);
        _SetTimer(MBTIMER_DRAGPOPDOWN);
        g_msgfilter.PreventCapture(FALSE);
        _iDragOverButton = -1;
#if 0
        DAD_ShowDragImage(FALSE);
        ToolBar_SetHotItem(_hwndMB, -1);
        DAD_ShowDragImage(TRUE);
#endif
        break;
    }
    return S_OK;
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::GetObjectDDT

*/
HRESULT CMenuStaticToolbar::GetObjectDDT (DWORD dwId, REFIID riid, LPVOID * ppvObj)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::OnDropDDT

*/
HRESULT CMenuStaticToolbar::OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, 
                            DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}



HRESULT CMenuStaticToolbar::v_InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    HRESULT hres = S_FALSE;

    if (NULL == psmd)
    {
        if (dwFlags & SMINV_REFRESH)
        {
            // Refresh the whole thing
            v_Refresh();
            hres = TRUE;
        }
    }

    // Are we dealing with an Hmenu?
    // Have we filled it yet?  (If not, then we can skip the invalidate
    // here, because we'll catch it when we fill it.)
    else if ((psmd->dwMask & SMDM_HMENU) && !_fDirty)
    {
        // Yes; What are they asking for?

        int iPos = -1;   // Assume this is a position
        int idCmd = -1;

        // Did they pass an ID instead of a position?
        if (dwFlags & SMINV_ID)
        {
            // Yes; Crack out the position.
            iPos = GetMenuPosFromID(_hmenu, psmd->uId);
            idCmd = psmd->uId;
        }

        if (dwFlags & SMINV_POSITION)
        {
            iPos = psmd->uId;
            idCmd = GetMenuItemID(_hmenu, iPos);
        }


        if (dwFlags & SMINV_REFRESH)
        {
            // Do they want to refresh a sepcific button?
            if (idCmd >= 0)
            {
                // Yes;

                // First delete the old one if it exists.
                int iTBPos = ToolBar_CommandToIndex(_hwndMB, idCmd);

                if (iTBPos >= 0)
                    SendMessage(_hwndMB, TB_DELETEBUTTON, iTBPos, 0);

                // Now Insert a new one
                MENUITEMINFO mii;
                TCHAR szName[MAX_PATH];
                mii.cbSize = sizeof(mii);
                mii.cch = ARRAYSIZE(szName);
                mii.dwTypeData = szName;
                mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE | MIIM_DATA;

                // This can fail...
                if (GetMenuItemInfoWrap(_hmenu, iPos, MF_BYPOSITION, &mii))
                {
                    _Insert(iPos, &mii);
                    hres = S_OK;
                }
            }
            else
            {
                // No; Refresh the whole thing
                v_Refresh();
            }

            if (!_fShowMB)
                _pcmb->_fForceButtonUpdate = TRUE;

            _pcmb->ResizeMenuBar();
        }
    }

    return hres;
}

void CMenuStaticToolbar::GetSize(SIZE* psize)
{
    _CheckSeparators();

    CMenuToolbarBase::GetSize(psize);
}

LRESULT CMenuStaticToolbar::_OnAccelerator(NMCHAR* pnmChar)
{
    SMDATA smdOut = {0};
    SMDATA smd = {0};
    smd.punk = SAFECAST(_pcmb, IShellMenu*);
    smd.uIdParent = _pcmb->_uId;

    if (_pcmb->_psmcb &&
        S_FALSE != _pcmb->_psmcb->CallbackSM(&smd, SMC_MAPACCELERATOR, (WPARAM)pnmChar->ch, (LPARAM)&smdOut))
    {
        pnmChar->dwItemNext = ToolBar_CommandToIndex(_hwndMB, smdOut.uId);;
        return TRUE;
    }

    return FALSE;
}

LRESULT CMenuStaticToolbar::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    MyLockSetForegroundWindow(FALSE);

    if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_CONTEXTMENU))
    {
        RECT rc;
        LPRECT prcExclude = NULL;
        POINT pt;
        int i;

        if (lParam != (LPARAM)-1) 
        {
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            POINT pt2 = pt;
            MapWindowPoints(HWND_DESKTOP, _hwndMB, &pt2, 1);

            i = ToolBar_HitTest(_hwndMB, &pt2);
        } 
        else 
        {
            // keyboard context menu.
            i = (int)SendMessage(_hwndMB, TB_GETHOTITEM, 0, 0);
            if (i >= 0) 
            {
                SendMessage(_hwndMB, TB_GETITEMRECT, i, (LPARAM)&rc);
                MapWindowPoints(_hwndMB, HWND_DESKTOP, (LPPOINT)&rc, 2);
                pt.x = rc.left;
                pt.y = rc.bottom;
                prcExclude = &rc;
            }
        }
        if (i >= 0)
        {
            UINT idCmd = GetButtonCmd(_hwndMB, i);
            if (S_OK == CallCB(idCmd, SMC_GETOBJECT, (WPARAM)(GUID*)&IID_IContextMenu, (LPARAM)(VOID**)(&_pcm)))
            {
                TPMPARAMS tpm;
                TPMPARAMS * ptpm = NULL;

                if (prcExclude)
                {
                    tpm.cbSize = SIZEOF(tpm);
                    tpm.rcExclude = *((LPRECT)prcExclude);
                    ptpm = &tpm;
                }
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    KillTimer(_hwndMB, MBTIMER_INFOTIP);
                    _pcmb->_pmbState->HideTooltip(FALSE);

                    _pcm->QueryContextMenu(hmenu, 0, 0, -1, 0);

                    idCmd = TrackPopupMenuEx(hmenu,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        pt.x, pt.y, _hwndMB, ptpm);

                    CMINVOKECOMMANDINFO ici = {
                        SIZEOF(CMINVOKECOMMANDINFO),
                        0,
                        _hwndMB,
                        MAKEINTRESOURCEA(idCmd),
                        NULL, NULL,
                        SW_NORMAL,
                    };

                    _pcm->InvokeCommand(&ici);

                    DestroyMenu(hmenu);
                }

                ATOMICRELEASE(_pcm);
            }
        }

        g_msgfilter.RetakeCapture();
    }

    return lres;
}

STDMETHODIMP CMenuStaticToolbar::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if (WM_CONTEXTMENU == dwMsg)
    {
        *plres = _OnContextMenu(wParam, lParam);
    }
    else
        return CMenuToolbarBase::OnWinEvent(hwnd, dwMsg, wParam, lParam, plres);

    return S_OK;
}
