#include "priv.h"
#include "sccls.h"
#include "menuband.h"
#include "itbar.h"
#include "../lib/dpastuff.h"       // COrderList_*
#include "inpobj.h"
#include "resource.h"
#include "mnbase.h"
#include "oleacc.h"
#include "apithk.h"
#include "menuisf.h"
#include "iaccess.h"
#include "uemapp.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

// BUGBUG (lamadio): Conflicts with one defined in winuserp.h
#undef WINEVENT_VALID       //It's tripping on this...
#include "winable.h"

#define DM_MISC     0               // miscellany

#define MAXUEMTIMEOUT 2000

/*----------------------------------------------------------
Purpose: Return the button command given the position.

*/
int GetButtonCmd(HWND hwnd, int iPos)
{
    ASSERT(IsWindow(hwnd));
    int nRet = -1;          // Punt on failure

    TBBUTTON tbb;
    if (ToolBar_GetButton(hwnd, iPos, &tbb))
    {
        nRet = tbb.idCommand;
    }
    return nRet;
}    



void* ItemDataFromPos(HWND hwndTB, int iPos)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
    if (ToolBar_GetButtonInfo(hwndTB, iPos, &tbbi) >= 0)
    {
        return (void*)tbbi.lParam;
    }

    return NULL;
}

long GetIndexFromChild(BOOL fTop, int iIndex)
{
    return  (fTop? TOOLBAR_MASK: 0) | iIndex + 1;
}

//--------------------------------------------------------------------------------
//
// CMenuToolbarBase
//
//--------------------------------------------------------------------------------

CMenuToolbarBase::CMenuToolbarBase(CMenuBand* pmb, DWORD dwFlags) : _pcmb(pmb)
{
#ifdef DEBUG
    _cRef = 1;
#endif
    _dwFlags = dwFlags;
    _nItemTimer = -1;
    _idCmdChevron = -1;
    _fFirstTime = TRUE;
}

// *** IObjectWithSite methods ***

HRESULT CMenuToolbarBase::SetSite(IUnknown *punkSite)
{
    ASSERT(punkSite && IS_VALID_READ_PTR(punkSite, CMenuBand*));

    // We are guaranteed the lifetime of this object is contained within
    // the menuband, so we don't addref pcmb.
    if (SUCCEEDED(punkSite->QueryInterface(CLSID_MenuBand, (LPVOID*)&_pcmb))) {
        punkSite->Release();
    } else {
        ASSERT(0);
    }

    

    _fVerticalMB = !BOOLIFY(_pcmb->_dwFlags & SMINIT_HORIZONTAL);
    _fTopLevel = BOOLIFY(_pcmb->_dwFlags & SMINIT_TOPLEVEL);
    
    return S_OK;
}

HRESULT CMenuToolbarBase::GetSite(REFIID riid, void ** ppvSite)
{
    if (!_pcmb)
        return E_FAIL;

    return _pcmb->QueryInterface(riid, ppvSite);
}

// *** IUnknown methods ***

STDMETHODIMP_(ULONG) CMenuToolbarBase::AddRef()
{
    DEBUG_CODE(_cRef++);
    if (_pcmb)
    {
        return _pcmb->AddRef();
    }

    return 0;
}


STDMETHODIMP_(ULONG) CMenuToolbarBase::Release()
{
    ASSERT(_cRef > 0);
    DEBUG_CODE(_cRef--);

    if (_pcmb)
    {
        return _pcmb->Release();
    }

    return 0;
}

HRESULT CMenuToolbarBase::QueryInterface(REFIID riid, void** ppvObj)
{
    HRESULT hres;
    if (IsEqualGUID(riid, CLSID_MenuToolbarBase) && ppvObj) 
    {
        AddRef();
        *ppvObj = (LPVOID)this;
        hres = S_OK;
    }
    else
        hres = _pcmb->QueryInterface(riid, ppvObj);

    return hres;
}

void CMenuToolbarBase::SetToTop(BOOL bToTop)
{
    // A menu toolbar can be at the top or the bottom of the menu.
    // This is an exclusive attribute.
    if (bToTop)
    {
        _dwFlags |= SMSET_TOP;
        _dwFlags &= ~SMSET_BOTTOM;
    }
    else
    {
        _dwFlags |= SMSET_BOTTOM;
        _dwFlags &= ~SMSET_TOP;
    }
}


void CMenuToolbarBase::KillPopupTimer()
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    TraceMsg(TF_MENUBAND, "(pmb=%#08lx): Killing Popout Timer...", this);
    KillTimer(_hwndMB, MBTIMER_POPOUT);
    _nItemTimer = -1;
}


void CMenuToolbarBase::SetWindowPos(LPSIZE psize, LPRECT prc, DWORD dwFlags)
{
    if (_hwndMB)
    {
        ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
        DWORD rectWidth = RECTWIDTH(*prc);
        TraceMsg(TF_MENUBAND, "CMTB::SetWindowPos %d - (%d,%d,%d,%d)", psize?psize->cx:0,
            prc->left, prc->top, prc->right, prc->bottom);
        ::SetWindowPos(_hwndMB, NULL, prc->left, prc->top, 
            rectWidth, RECTHEIGHT(*prc), SWP_NOZORDER | SWP_NOACTIVATE | dwFlags);
        // hackhack:  we only do this when multicolumn.  this call is to facilitate the size negotiation between 
        // static menu and folder menu.  Set the width of the toolbar to the width of the button in case 
        // of non-multicolumn.
        if (!(_fMulticolumnMB) && psize)
            ToolBar_SetButtonWidth(_hwndMB, psize->cx, psize->cx);

        // Force this to redraw. I put this here because the HMenu portion was painting after the shell
        // folder portion was done enumerating the folder, which is pretty slow. I wanted the HMENU portion
        // to paint right away...
        RedrawWindow(_hwndMB, NULL, NULL, RDW_UPDATENOW);
    }
}

// NOTE: if psize is (0,0) we use tb button size as param in figuring out ideal tb size
//   else we use max of psize length and tb button length as our metric
void CMenuToolbarBase::GetSize(SIZE* psize)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    if (EVAL(_hwndMB))
    {
        LRESULT lButtonSize;

        lButtonSize = SendMessage(_hwndMB, TB_GETBUTTONSIZE, 0, 0);
        if (psize->cx || psize->cy) 
        {
            int cx = max(psize->cx, LOWORD(lButtonSize));
            int cy = max(psize->cy, HIWORD(lButtonSize));
            lButtonSize = MAKELONG(cx, cy);
        }

        if (_fVerticalMB)
        {
            psize->cx = LOWORD(lButtonSize);
            SendMessage(_hwndMB, TB_GETIDEALSIZE, TRUE, (LPARAM)psize);
        }
        else
        {
            psize->cy = HIWORD(lButtonSize);
            SendMessage(_hwndMB, TB_GETIDEALSIZE, FALSE, (LPARAM)psize);
        }

        TraceMsg(TF_MENUBAND, "CMTB::GetSize (%d, %d)", psize->cx, psize->cy);
    }
}


/*----------------------------------------------------------
Purpose: Timer handler.  Used to pop open/close cascaded submenus.

*/
LRESULT CMenuToolbarBase::_OnTimer(WPARAM wParam)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    switch (wParam) 
    {

    case MBTIMER_INFOTIP:
        {
            // Do we have a hot item to display the tooltip for?
            int iHotItem = ToolBar_GetHotItem(_hwndMB);
            KillTimer(_hwndMB, wParam);
            if (iHotItem >= 0)
            {
                // Yep.
                TCHAR szTip[MAX_PATH];
                int idCmd = GetButtonCmd(_hwndMB, iHotItem);

                // Ask the superclass for the tip
                if (S_OK == v_GetInfoTip(idCmd, szTip, ARRAYSIZE(szTip)))
                {
                    // Now display it. Yawn.
                    _pcmb->_pmbState->CenterOnButton(_hwndMB, FALSE, idCmd, NULL, szTip);
                }
            }
        }
        break;

    case MBTIMER_CHEVRONTIP:
        KillTimer(_hwndMB, wParam);
       _pcmb->_pmbState->HideTooltip(TRUE);
       break;
 
    case MBTIMER_FLASH:
        {
            _cFlashCount++;
            if (_cFlashCount == COUNT_ENDFLASH)
            {
                _cFlashCount = 0;
                KillTimer(_hwndMB, wParam);
                ToolBar_MarkButton(_hwndMB, _idCmdChevron, FALSE);
                _SetTimer(MBTIMER_UEMTIMEOUT);

                // Now that we've flashed, let's show the Chevron tip.
                // This is for a confused user: If they've hovered over an item for too long,
                // or this is the first time they've seen intellimenus, then we flash and display
                // the tooltip. We only want to display this if we are shown: We would end up with
                // and dangling tooltip if you happen to move to another menu while it was flashing.
                // Ummm, is the Chevron still visible?
                if (_fShowMB && _idCmdChevron != -1)
                {
                    TCHAR szTip[MAX_PATH];
                    TCHAR szTitle[MAX_PATH];
                    if (S_OK == v_CallCBItem(_idCmdChevron, SMC_CHEVRONGETTIP, (WPARAM)szTitle, (LPARAM)szTip))
                    {
                        _pcmb->_pmbState->CenterOnButton(_hwndMB, TRUE, _idCmdChevron, szTitle, szTip);
                        _SetTimer(MBTIMER_CHEVRONTIP);
                    }
                }
            }
            else
                ToolBar_MarkButton(_hwndMB, _idCmdChevron, (_cFlashCount % 2) == 0);
        }
        break;

    case MBTIMER_UEMTIMEOUT:
        {
            POINT pt;
            RECT rect;

            // Don't fire timeouts when we're in edit mode.
            if (_fEditMode)
            {
                KillTimer(_hwndMB, wParam);
                break;
            }

            GetWindowRect(_hwndMB, &rect);
            GetCursorPos(&pt);
            if (PtInRect(&rect, pt))
            {
                TraceMsg(TF_MENUBAND, "*** UEM TimeOut. At Tick Count (%d) ***", GetTickCount());
                _FireEvent(UEM_TIMEOUT);
            }
            else
            {
                TraceMsg(TF_MENUBAND, " *** UEM TimeOut. At Tick Count (%d)."
                    " Mouse outside menu. Killing *** ", GetTickCount());
                KillTimer(_hwndMB, wParam);
            }
        }
        break;


    case MBTIMER_EXPAND:
        KillTimer(_hwndMB, wParam);
        if (_fShowMB)
        {
            v_CallCBItem(_idCmdChevron, SMC_CHEVRONEXPAND, 0, 0);
            Expand(TRUE);
            _fClickHandled = TRUE;
            _SetTimer(MBTIMER_CLICKUNHANDLE); 
        }
        break;

    case MBTIMER_DRAGPOPDOWN:
        // There has not been a drag enter in this band for a while, 
        // so we'll try to cancel the menus.
        KillTimer(_hwndMB, wParam);
        PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), g_nMBDragCancel, 0, 0);
        break;

    case MBTIMER_DRAGOVER:
        {
            TraceMsg(TF_MENUBAND, "CMenuToolbarBase::OnTimer(DRAG)");
            KillTimer(_hwndMB, wParam);
            DAD_ShowDragImage(FALSE);
            // Does this item cascade?
            int idBtn = GetButtonCmd(_hwndMB, v_GetDragOverButton());
            if (v_GetFlags(idBtn) & SMIF_SUBMENU)
            {
                TraceMsg(TF_MENUBAND, "CMenuToolbarBase::OnTimer(DRAG): Is a submenu");
                // Yes; pop it open
                if (!_fVerticalMB)
                    _pcmb->_fInvokedByDrag = TRUE;
                _DoPopup(idBtn, FALSE);
            }
            else if (idBtn == _idCmdChevron)
            {
                Expand(TRUE);

            }
            else
            {
                _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);
            }
        }
        break;

    case MBTIMER_POPOUT:
        {
            int nItemTimer = _nItemTimer;
            KillPopupTimer();

            // Popup a new submenu?
            if (-1 != nItemTimer)
            {
                if (nItemTimer != _pcmb->_nItemCur)
                {
                    // Yes;  post message since the currently expanded submenu
                    // may be a CTrackPopup object, which posts its cancel mode.

                    TraceMsg(TF_MENUBAND, "(pmb=%#08lx): Timer went off.  Expanding...", this);
                    PostPopup(nItemTimer, FALSE, FALSE);
                }
            }
            else 
            {
                // No; just collapse the currently open submenu
                TraceMsg(TF_MENUBAND, "(pmb=%#08lx): _OnTimer sending MPOS_CANCELLEVEL to submenu popup", this);
                _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);
            }
            break;
        }
    
    case MBTIMER_CLOSE:
        KillTimer(_hwndMB, wParam);

        TraceMsg(TF_MENUBAND, "(pmb=%#08lx): _OnTimer sending MPOS_FULLCANCEL", this);

        if (_fVerticalMB)
            _pcmb->_SiteOnSelect(MPOS_FULLCANCEL);
        else
        {
            _pcmb->_SubMenuOnSelect(MPOS_FULLCANCEL);
        }
        break;
    }
        
    return 1;
}


void CMenuToolbarBase::_DrawMenuArrowGlyph( HDC hdc, RECT * prc, COLORREF rgbText )
{
    SIZE size = {_pcmb->_pmbm->_cxArrow, _pcmb->_pmbm->_cyArrow};

    //
    // If the DC is mirrred, then the Arrow should be mirrored
    // since it is done thru TextOut, NOT the 2D graphics APIs [samera]
    //

    _DrawMenuGlyph(hdc, 
                   _pcmb->_pmbm->_hFontArrow,
                   prc, 
                   (IS_DC_RTL_MIRRORED(hdc)) ? CH_MENUARROWRTLA :
                   CH_MENUARROWA, 
                   rgbText, 
                   &size);
}


void CMenuToolbarBase::_DrawMenuGlyph( HDC hdc, HFONT hFont, RECT * prc, 
                               CHAR ch, COLORREF rgbText,
                               LPSIZE psize)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (_pcmb->_pmbm->_hFontArrow)
    {
        SIZE    size;
        int cx, cy, y, x;
        HFONT hFontOld;
        int iOldBk = SetBkMode(hdc, TRANSPARENT);
        hFontOld = (HFONT)SelectObject(hdc, hFont);
        if (psize == NULL)
        {
            GetTextExtentPoint32A( hdc, &ch, 1, &size);
            psize = &size;
        }

        cy = prc->bottom - prc->top;
        y = prc->top  + ((cy - psize->cy) / 2);

        cx = prc->right - prc->left;
        x = prc->left + ((cx - psize->cx) /2);
    
        COLORREF rgbOld = SetTextColor(hdc, rgbText);

#ifndef UNIX
        TextOutA(hdc, x, y, &ch, 1);
#else
        // Paint motif look arrow.
        PaintUnixMenuArrow( hdc, prc, (DWORD)rgbText );
#endif
    
        SetTextColor(hdc, rgbOld);
        SetBkMode(hdc, iOldBk);
        SelectObject(hdc, hFontOld);
    }
}

void CMenuToolbarBase::SetMenuBandMetrics(CMenuBandMetrics* pmbm)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    // This can be called before the toolbar is created. 
    // So we'll check this condition. When the toolbar is created, then
    // the toolbar will get the metrics at that point.
    if (!_hwndMB)
        return;

    //Loop through toolbar.
    for (int iButton = ToolBar_ButtonCount(_hwndMB)-1; iButton >= 0; iButton--)
    {
        IOleCommandTarget* poct;

        int idCmd = GetButtonCmd(_hwndMB, iButton);

        // If it's not a seperator, see if there is a sub menu.
        if (idCmd != -1 &&
            SUCCEEDED(v_GetSubMenu(idCmd, NULL, IID_IOleCommandTarget, (void**)&poct)))
        {
            VARIANT Var;
            Var.vt = VT_UNKNOWN;
            Var.punkVal = SAFECAST(pmbm, IUnknown*);

            // Exec to set new Metrics.
            poct->Exec(&CGID_MenuBand, MBANDCID_SETFONTS, 0, &Var, NULL);
            poct->Release();
        }
    }

    _SetFontMetrics();
    // return
}

void CMenuToolbarBase::_SetFontMetrics()
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (_hwndMB && _pcmb->_pmbm)
    {
        SendMessage(_hwndMB, WM_SETFONT, (WPARAM)_pcmb->_pmbm->_hFontMenu, FALSE);
    }
}


void CMenuToolbarBase::CreateToolbar(HWND hwndParent)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    ASSERT( _hwndMB != NULL );
    DWORD dwToolBarStyle = TBSTYLE_TRANSPARENT;

    // if we're set up as a popup, don't do any transparent stuff
    if (_fVerticalMB) 
    {
        dwToolBarStyle  = TBSTYLE_CUSTOMERASE;    // Vertical Toolbars don't get Transparent
        DWORD dwExtendedStyle = 0;

        // This is for TBMenu which actually has a Horizontal menubar within the 
        // Vertical menuband.
        if (!_fHorizInVerticalMB)
            dwExtendedStyle |= TBSTYLE_EX_VERTICAL;

        if (_fMulticolumnMB)
            dwExtendedStyle |= TBSTYLE_EX_MULTICOLUMN;

        ToolBar_SetExtendedStyle(_hwndMB, 
            dwExtendedStyle, TBSTYLE_EX_VERTICAL | TBSTYLE_EX_MULTICOLUMN);

        ToolBar_SetListGap(_hwndMB, LIST_GAP);
    }

    SHSetWindowBits(_hwndMB, GWL_STYLE, 
        TBSTYLE_TRANSPARENT | TBSTYLE_CUSTOMERASE, dwToolBarStyle );

    ToolBar_SetInsertMarkColor(_hwndMB, GetSysColor(COLOR_MENUTEXT));

    v_UpdateIconSize(_pcmb->_uIconSize, FALSE);
    _SetFontMetrics();
}


HRESULT CMenuToolbarBase::_SetMenuBand(IShellMenu* psm)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    HRESULT hres = E_FAIL;
    IBandSite* pmbs = NULL;
    if (!_pcmb->_pmpSubMenu)
    {
        hres = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_IMenuPopup, (void**)&_pcmb->_pmpSubMenu);
        if (SUCCEEDED(hres))
        {
            IUnknown_SetSite(_pcmb->_pmpSubMenu, SAFECAST(_pcmb, IOleCommandTarget*));
            hres = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_IBandSite, (void**)&pmbs);
            if (SUCCEEDED(hres))
            {
                hres = _pcmb->_pmpSubMenu->SetClient(pmbs);
                // Don't release pmbs here. We are using below
            }
            // Menu band will Release _pmpSubMenu.
        }
    }
    else
    {
        IUnknown* punk;
        _pcmb->_pmpSubMenu->GetClient(&punk);
        if (punk)
        {
            hres = punk->QueryInterface(IID_IBandSite, (void**)&pmbs);
            punk->Release();
        }
    }

    if (pmbs)
    {
        if (SUCCEEDED(hres))
            hres = pmbs->AddBand(psm);

        pmbs->Release();
    }
 
    return hres;
}

HRESULT CMenuToolbarBase::GetSubMenu(int idCmd, GUID* pguidService, REFIID riid, void** ppvObj)
{
    // pguidService is for asking a for specifically the Shell Folder portion or the Static portion
    HRESULT hres = E_FAIL;
    if (v_GetFlags(idCmd) & SMIF_TRACKPOPUP ||
        _pcmb->_dwFlags & SMINIT_DEFAULTTOTRACKPOPUP)
    {
        hres = v_CreateTrackPopup(idCmd, riid, (void**)ppvObj);
        if (SUCCEEDED(hres))
        {
            _pcmb->SetTrackMenuPopup((IUnknown*)*ppvObj);
        }
    }
    else
    {
        IShellMenu* psm;
        hres = v_GetSubMenu(idCmd, pguidService, IID_IShellMenu, (void**)&psm);
        if (SUCCEEDED(hres)) 
        {
            TraceMsg(TF_MENUBAND, "GetUIObject psm %#lx", psm);
            _pcmb->SetTracked(this);

            hres = _SetMenuBand(psm);
            psm->Release();

            // Did we succeed in getting a menupopup?
            if (SUCCEEDED(hres))
            {
                // Yep; Sweet!
                _pcmb->_pmpSubMenu->QueryInterface(riid, ppvObj);

                HWND hwnd;
                IUnknown_GetWindow(_pcmb->_pmpSubMenu, &hwnd);
                PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), g_nMBAutomation, (WPARAM)hwnd, (LPARAM)-1);
            }
        }
    }

    return hres;
}

HRESULT CMenuToolbarBase::PositionSubmenu(int idCmd)
{
    IMenuPopup* pmp = NULL;
    HRESULT hres = E_FAIL;
    DWORD dwFlags = 0;

    if (_pcmb->_fInSubMenu)
    {
        // Since the selection has probrably changed, we use the cached item id
        // to calculate the postion rect
        idCmd = _pcmb->_nItemSubMenu;
        dwFlags = MPPF_REPOSITION | MPPF_NOANIMATE;
        pmp = _pcmb->_pmpSubMenu;
        pmp->AddRef();

        ASSERT(pmp);    // If _fInSubmenu is set, then this must be valid
        hres = S_OK;
    }
    else
    {
        // Only do these when we're not repositioning.
        if (_pcmb->_fInitialSelect)
            dwFlags |= MPPF_INITIALSELECT;

        if (g_bRunOnNT5 && !_pcmb->_fCascadeAnimate)
            dwFlags |= MPPF_NOANIMATE;

        _pcmb->_nItemSubMenu = idCmd;

        hres = GetSubMenu(idCmd, NULL, IID_IMenuPopup, (void**)&pmp);
    }

    ASSERT(idCmd != -1);    // Make sure at this point we have an item.


    if (SUCCEEDED(hres))
    {
        ASSERT(pmp);

        // Make sure the menuitem is pressed
        _PressBtn(idCmd, TRUE);

        RECT rc;
        RECT rcTB;
        RECT rcTemp;
        POINT pt;

        SendMessage(_hwndMB, TB_GETRECT, idCmd, (LPARAM)&rc);
        GetClientRect(_hwndMB, &rcTB);

        // Is the button rect within the boundries of the
        // visible toolbar?
        if (!IntersectRect(&rcTemp, &rcTB, &rc))
        {
            // No; Then we need to bias that rect into
            // the visible region of the toolbar.
            // We only want to bias one side
            if (rc.left > rcTB.right)
            {
                rc.left = rcTB.right - (rc.right - rc.left);
                rc.right = rcTB.right;
            }
        }


        MapWindowPoints(_hwndMB, HWND_DESKTOP, (POINT*)&rc, 2);

        if (_fVerticalMB) 
        {
            pt.x = rc.right;
            pt.y = rc.top;
        } 
        else 
        {
            //
            // If the shell dropdown (toolbar button) menus are mirrored,
            // then take the right edge as the anchor point
            //
            if (IS_WINDOW_RTL_MIRRORED(_hwndMB))
                pt.x = rc.right;
            else
                pt.x = rc.left;
            pt.y = rc.bottom;
        }

        // Since toolbar buttons expand almost to the end of the basebar,
        // shrink the exclude rect so if overlaps.
        // NOTE: the items are GetSystemMetrics(SM_CXEDGE) larger than before. So adjust to that.

        if (_pcmb->_fExpanded)
            InflateRect(&rc, -GetSystemMetrics(SM_CXEDGE), 0);

        // We want to stop showing the chevron tip when we cascade into another menu
        _pcmb->_pmbState->HideTooltip(TRUE);

        // Only animate the first show at this level.
        _pcmb->_fCascadeAnimate = FALSE;

        hres = pmp->Popup((POINTL*)&pt, (RECTL*)&rc, dwFlags);
        pmp->Release();

    }
    return hres;
}
/*----------------------------------------------------------
Purpose: Cascade to the _nItemCur item's menu popup.

         If the popup call was modal, S_FALSE is returned; otherwise
         it is S_OK, or error.

*/
HRESULT CMenuToolbarBase::PopupOpen(int idBtn)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    HRESULT hres = E_FAIL;


    // Tell the current submenu popup to cancel.  This must be done 
    // before the PostMessage b/c CTrackPopupBar itself posts a message
    // which it must receive before we receive our post.
    TraceMsg(TF_MENUBAND, "(pmb=%#08lx): PostPopup sending MPOS_CANCELLEVEL to submenu popup", this);
    if (_pcmb->_fInSubMenu)
        _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);

    hres = PositionSubmenu(idBtn);

    // Modal?
    if (S_FALSE == hres)
    {
        // Yes; take the capture back
        g_msgfilter.RetakeCapture();

        // return S_OK so we stay in the menu mode
        hres = S_OK;
    }
    else if (FAILED(hres))
        _PressBtn(idBtn, FALSE);

    // Since CTrackPopupBar is modal, it should be a useless blob 
    // of bits in memory by now...
    _pcmb->SetTrackMenuPopup(NULL);
   
    return hres;
}


/*----------------------------------------------------------
Purpose: Called to hide a modeless menu.

*/
void CMenuToolbarBase::PopupClose(void)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (-1 != _pcmb->_nItemCur)
    {
        _PressBtn(_pcmb->_nItemCur, FALSE);
        NotifyWinEvent(EVENT_OBJECT_FOCUS, _hwndMB, OBJID_CLIENT, 
            GetIndexFromChild(_dwFlags & SMSET_TOP, ToolBar_CommandToIndex(_hwndMB, _pcmb->_nItemCur)));

        _pcmb->_fInSubMenu = FALSE;
        _pcmb->_fInvokedByDrag = FALSE;
        _pcmb->_nItemCur = -1;
    }
}    


LRESULT CMenuToolbarBase::_OnWrapHotItem(NMTBWRAPHOTITEM* pnmwh)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (_fProcessingWrapHotItem || 
        (_pcmb->_pmtbTop == _pcmb->_pmtbBottom && !_fHasDemotedItems))
        return 0;

    _fProcessingWrapHotItem = TRUE;


    // If we want ourselves to not be wrapped into (Like for empty items) 
    // Then forward the wrap message to the other toolbar
    if (_pcmb->_pmtbTracked->_dwFlags & SMSET_TOP && !(_pcmb->_pmtbBottom->_fDontShowEmpty))
    {
        _pcmb->SetTracked(_pcmb->_pmtbBottom);
    }
    else if (!(_pcmb->_pmtbTop->_fDontShowEmpty))
    {
        _pcmb->SetTracked(_pcmb->_pmtbTop);
    }

    int iIndex;

    if (pnmwh->iDir < 0)
    {
        HWND hwnd = _pcmb->_pmtbTracked->_hwndMB;
        iIndex = ToolBar_ButtonCount(hwnd) - 1;
        int idCmd = GetButtonCmd(hwnd, iIndex);

        // We do not want to wrap onto a chevron.
        if (idCmd == _idCmdChevron)
            iIndex -= 1;

    }
    else
    {
        iIndex = 0;
    }

    _pcmb->_pmtbTracked->SetHotItem(pnmwh->iDir, iIndex, -1, pnmwh->nReason);


    _fProcessingWrapHotItem = FALSE;

    return 1;
}


LRESULT CMenuToolbarBase::_OnWrapAccelerator(NMTBWRAPACCELERATOR* pnmwa)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    int iHotItem = -1;
    int iNumTopAccel = 0;
    int iNumBottomAccel = 0;

    if (_pcmb->_fProcessingDup)
        return 0;

    // Check to see if there is only one toolbar.
    if (_pcmb->_pmtbTop == _pcmb->_pmtbBottom)
        return 0;

    ToolBar_HasAccelerator(_pcmb->_pmtbTop->_hwndMB, pnmwa->ch, &iNumTopAccel);
    ToolBar_HasAccelerator(_pcmb->_pmtbBottom->_hwndMB, pnmwa->ch, &iNumBottomAccel);

    _pcmb->_fProcessingDup = TRUE;

    CMenuToolbarBase* pmbtb = NULL;
    if (_pcmb->_pmtbTracked->_dwFlags & SMSET_TOP)
    {
        ToolBar_MapAccelerator(_pcmb->_pmtbBottom->_hwndMB, pnmwa->ch, &iHotItem);
        pmbtb = _pcmb->_pmtbBottom;
    }
    else
    {
        ToolBar_MapAccelerator(_pcmb->_pmtbTop->_hwndMB, pnmwa->ch, &iHotItem);
        pmbtb = _pcmb->_pmtbTop;
    }

    _pcmb->_fProcessingDup = FALSE;

    if (iHotItem != -1)
    {
        _pcmb->SetTracked(pmbtb);
        int idCmd = ToolBar_CommandToIndex(pmbtb->_hwndMB, iHotItem);
        DWORD dwFlags = HICF_ACCELERATOR;

        // If either (but not both) toolbars have the accelerator, and it is exactly one,
        // then cause the drop down.
        if ( (iNumTopAccel >= 1) ^ (iNumBottomAccel >= 1) &&
             (iNumTopAccel == 1 || iNumBottomAccel == 1) )
            dwFlags |= HICF_TOGGLEDROPDOWN;

        SendMessage(pmbtb->_hwndMB, TB_SETHOTITEM2, idCmd, dwFlags);
        pnmwa->iButton = -1;
        return 1;
    }

    return 0;
}


LRESULT CMenuToolbarBase::_OnDupAccelerator(NMTBDUPACCELERATOR* pnmda)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (_pcmb->_fProcessingDup || (_pcmb->_pmtbBottom == _pcmb->_pmtbTop))
        return 0;

    _pcmb->_fProcessingDup = TRUE;

    int iNumTopAccel = 0;
    int iNumBottomAccel = 0;
    
    if (_pcmb->_pmtbTop)
        ToolBar_HasAccelerator(_pcmb->_pmtbTop->_hwndMB, pnmda->ch, &iNumTopAccel);

    if (_pcmb->_pmtbBottom)
        ToolBar_HasAccelerator(_pcmb->_pmtbBottom->_hwndMB, pnmda->ch, &iNumBottomAccel);


    _pcmb->_fProcessingDup = FALSE;

    if (0 == iNumTopAccel && 0 == iNumBottomAccel)
    {
        // We want to return 1 if Both of them have one. 
        //Otherwise, return 0, and let the toolbar handle it itself.
        return 0;
    }

    pnmda->fDup = TRUE;

    return 1;
}

/*----------------------------------------------------------
Purpose: Handle WM_NOTIFY

*/
LRESULT CMenuToolbarBase::_OnNotify(LPNMHDR pnm)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    LRESULT lres = 0;

    // These are notifies we handle even when disengaged from the message hook.
    switch (pnm->code)
    {
    case NM_CUSTOMDRAW:
        // We now custom draw even the TopLevelMenuBand (for the correct font)
        lres = _OnCustomDraw((NMCUSTOMDRAW*)pnm);
        break;
    }
    
    
    // Is the Global Message filter Disengaged? This will happen when the Subclassed window
    // looses activation to a dialog box of some kind.
    if (lres == 0 && !g_msgfilter.IsEngaged())
    {
        // Yes; We've lost activation so we don't want to track like a normal menu...

        // For hot item change, return 1 so that the toolbar does not change the hot item.
        if (pnm->code == TBN_HOTITEMCHANGE && _pcmb->_fMenuMode)
            return 1;

        // For all other items, don't do anything....
        return 0;
    }

    switch (pnm->code)
    {
    case NM_RELEASEDCAPTURE:
        g_msgfilter.RetakeCapture();
        break;

    case NM_KEYDOWN:
        BLOCK
        {
            LPNMKEY pnmk = (LPNMKEY)pnm;
            lres = _OnKey(TRUE, pnmk->nVKey, pnmk->uFlags);
        }
        break;

    case NM_CHAR:
        {
            LPNMCHAR pnmc = (LPNMCHAR)pnm;
            if (pnmc->ch == TEXT(' '))
                return TRUE;

            if (pnmc->dwItemNext == -1 &&
                !_pcmb->_fVertical)
            {
                // If it's horizontal, then it must be top level.
                ASSERT(_pcmb->_fTopLevel);
                _pcmb->_CancelMode(MPOS_FULLCANCEL);
            }
        }
        break;

    case TBN_HOTITEMCHANGE:
        lres = _OnHotItemChange((LPNMTBHOTITEM)pnm);
        break;

    case NM_LDOWN:
        // We need to kill the expand timer, because the user might
        // move out of the chevron and accidentally select another item.
        if ( (int)((LPNMCLICK)pnm)->dwItemSpec == _idCmdChevron && _idCmdChevron != -1)
        {
            KillTimer(_hwndMB, MBTIMER_EXPAND);
            _fIgnoreHotItemChange = TRUE;
        }
        break;

    case NM_CLICK:
        {
            int idCmd = (int)((LPNMCLICK)pnm)->dwItemSpec;
            _fIgnoreHotItemChange = FALSE;
            if ( idCmd == -1 )
            {
                _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);
                _pcmb->SetTracked(NULL);
                lres = 1;
            }
            else if ( idCmd == _idCmdChevron )
            {
                // Retake the capture on the button-up, b/c the toolbar took
                // it away for a moment.
                g_msgfilter.RetakeCapture();

                v_CallCBItem(_idCmdChevron, SMC_CHEVRONEXPAND, 0, 0);
                Expand(TRUE);
                _fClickHandled = TRUE;
                _SetTimer(MBTIMER_CLICKUNHANDLE);
                lres = 1;
            }
            else if (!_fEmpty)
            {
                TraceMsg(TF_MENUBAND, "(pmb=%#08lx): upclick %d", this, idCmd);

                // Retake the capture on the button-up, b/c the toolbar took
                // it away for a moment.
                g_msgfilter.RetakeCapture();

                if (v_GetFlags(idCmd) & SMIF_SUBMENU)     // Submenus support double click
                {
                    if (_iLastClickedTime == 0) // First time it was clicked
                    {
                        _iLastClickedTime = GetTickCount();
                        _idCmdLastClicked = idCmd;
                    }
                    // Did they click on the same item twice?
                    else if (idCmd != _idCmdLastClicked)
                    {
                        _iLastClickedTime = _idCmdLastClicked = 0;
                    }
                    else
                    {
                        // Was this item double clicked on?
                        if ((GetTickCount() - _iLastClickedTime) < GetDoubleClickTime())
                        {
                            // We need to post this back to ourselves, because
                            // the Tray will become in active when double clicking
                            // on something like programs. This happens because the 
                            // Toolbar will set capture back to itself and the tray
                            // doesn't get any more messages.
                            PostMessage(_hwndMB, g_nMBExecute, idCmd, 0);
                            _fClickHandled = TRUE;
                        }

                        _iLastClickedTime = _idCmdLastClicked = 0;
                    }
                }

                // Sent on the button-up.  Handle the same way.
                if (!_fClickHandled && -1 != idCmd)
                    _DropDownOrExec(idCmd, FALSE);

                _fClickHandled = FALSE;
                lres = 1;
            }
        }
        break;

    case TBN_DROPDOWN:
        lres = _OnDropDown((LPNMTOOLBAR)pnm);
        break;

#ifdef UNICODE
    case TBN_GETINFOTIPA:
        {
            LPNMTBGETINFOTIPA pnmTT = (LPNMTBGETINFOTIPA)pnm;
            UINT uiCmd = pnmTT->iItem;
            TCHAR szTip[MAX_PATH];

            if ( S_OK == v_GetInfoTip(pnmTT->iItem, szTip, ARRAYSIZE(szTip)) )
            {
                SHUnicodeToAnsi(szTip, pnmTT->pszText, pnmTT->cchTextMax);
            }
            else
            {
                // Set the lpszText to NULL to prevent the toolbar from setting
                // the button text by default
                pnmTT->pszText = NULL;
            }


            lres = 1;
            break;

        }
#endif
    case TBN_GETINFOTIP:
        {
            LPNMTBGETINFOTIP pnmTT = (LPNMTBGETINFOTIP)pnm;
            UINT uiCmd = pnmTT->iItem;

            if ( S_OK != v_GetInfoTip(pnmTT->iItem, pnmTT->pszText, pnmTT->cchTextMax) )
            {
                // Set the lpszText to NULL to prevent the toolbar from setting
                // the button text by default
                pnmTT->pszText = NULL;
            }
            lres = 1;
            break;
        }

    case NM_RCLICK:
        // When we go into a context menu, stop monitoring.
        KillTimer(_hwndMB, MBTIMER_EXPAND);
        KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
        break;

    case TBN_WRAPHOTITEM:
        lres = _OnWrapHotItem((NMTBWRAPHOTITEM*)pnm);
        break;

    case TBN_WRAPACCELERATOR:
        lres = _OnWrapAccelerator((NMTBWRAPACCELERATOR*)pnm);
        break;

    case TBN_DUPACCELERATOR:
        lres = _OnDupAccelerator((NMTBDUPACCELERATOR*)pnm);
        break;

    case TBN_DRAGOVER:
        // This message is sent when drag and drop within the toolbar indicates that it
        // is about to mark a button. Since this gets messed up because of LockWindowUpdate
        // we tell it not to do anything.
        lres = 1;
        break;
    }

    return(lres);
}


BOOL CMenuToolbarBase::_SetTimer(int nTimer)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    long lTimeOut;

#ifndef UNIX
    // If we're on NT5 or Win98, use the cool new SPI
    if (SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &g_lMenuPopupTimeout, 0)) {
        // Woo-hoo, all done.
    }
    else if (g_lMenuPopupTimeout == -1)
#endif
    {
        // NT4 or Win95.  Grovel the registry (yuck).
        DWORD dwType;
        TCHAR szDelay[6]; // int is 5 characters + null.
        DWORD cbSize = ARRAYSIZE(szDelay);

        g_lMenuPopupTimeout = MBTIMER_TIMEOUT;

        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"),
            TEXT("MenuShowDelay"), &dwType, (void*)szDelay, &cbSize))
        {
            g_lMenuPopupTimeout = (UINT)StrToInt(szDelay);
        }
    }

    lTimeOut = g_lMenuPopupTimeout;

    switch (nTimer)
    {
    case MBTIMER_EXPAND:
    case MBTIMER_DRAGPOPDOWN:
        lTimeOut *= 2;
        if (lTimeOut < MAXUEMTIMEOUT)
            lTimeOut = MAXUEMTIMEOUT;
        break;

    case MBTIMER_UEMTIMEOUT:
            if (!_fHasDemotedItems || _pcmb->_pmbState->GetExpand() || _fEditMode)
                return TRUE;
            lTimeOut *= 5;

            // We want a minimum of MAXUEMTIMEOUT for people who set the expand rate to zero
            if (lTimeOut < MAXUEMTIMEOUT)
                lTimeOut = MAXUEMTIMEOUT;
            TraceMsg(TF_MENUBAND, "*** UEM SetTimeOut to (%d) milliseconds" 
                "at Tick Count (%d).*** ", GetTickCount());
            break;

    case MBTIMER_CHEVRONTIP:
        lTimeOut = 60 * 1000;    // Please make the intellimenu's balloon tip go 
                                 // away after one minute of no action.
        break;

    case MBTIMER_INFOTIP:
        lTimeOut = 500;    // Half a second hovering over an item?
        break;
    }

    TraceMsg(TF_MENUBAND, "(pmb=%#08lx): Setting %d Timer to %d milliseconds at tickcount %d", 
        this, nTimer, lTimeOut, GetTickCount());
    return (BOOL)SetTimer(_hwndMB, nTimer, lTimeOut, NULL);
}

BOOL CMenuToolbarBase::_HandleObscuredItem(int idCmd)
{
    RECT rc;
    GetClientRect(_hwndMB, &rc);

    int iButton = (int)SendMessage(_hwndMB, TB_COMMANDTOINDEX, idCmd, 0);

    if (SHIsButtonObscured(_hwndMB, &rc, iButton)) 
    {
        // clear hot item
        ToolBar_SetHotItem(_hwndMB, -1);

        _pcmb->_SubMenuOnSelect(MPOS_FULLCANCEL);
        _pcmb->_CancelMode(MPOS_FULLCANCEL);        // This is for the track menus.

        HWND hwnd = _pcmb->_pmbState->GetSubclassedHWND();

        PostMessage(hwnd? hwnd: _hwndMB, g_nMBOpenChevronMenu, (WPARAM)idCmd, 0);

        return TRUE;
    }

    return FALSE;
}


LRESULT CMenuToolbarBase::_OnHotItemChange(NMTBHOTITEM * pnmhot)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    LRESULT lres = 0;

#ifdef UNIX
    // IEUNIX : If this is a mouse move check if the left button is pressed
    // deviating from Windows behavior to be motif compliant.
    if (_fVerticalMB && (pnmhot->dwFlags & HICF_MOUSE) && !(pnmhot->dwFlags & HICF_LMOUSE))
        return 1;
#endif

    if (_pcmb->_fMenuMode && _pcmb->_fShow && !_fIgnoreHotItemChange)
    {
        // Is this toolbar being entered?
        if (!(pnmhot->dwFlags & HICF_LEAVING))
        {
            // Yes; set it to be the currently tracking toolbar
            TraceMsg(TF_MENUBAND, "CMTB::OnHotItemChange. Setting Tracked....", this);
            _pcmb->SetTracked(this);
        }

        // Always kill the expand timer when something changes
        KillTimer(_hwndMB, MBTIMER_EXPAND);
        KillTimer(_hwndMB, MBTIMER_INFOTIP);

        // If the Toolbar has keybaord focus, we need to send OBJID_CLIENT so that we track correctly.
        if (!(pnmhot->dwFlags & HICF_LEAVING))
        {
            NotifyWinEvent(EVENT_OBJECT_FOCUS, _hwndMB, OBJID_CLIENT, 
                GetIndexFromChild(_dwFlags & SMSET_TOP, ToolBar_CommandToIndex(_hwndMB, pnmhot->idNew)));
        }

        DEBUG_CODE( TraceMsg(TF_MENUBAND, "(pmb=%#08lx): TBN_HOTITEMCHANGE (state:%#02lx, %d-->%d)", 
                             this, pnmhot->dwFlags, 
                             (pnmhot->dwFlags & HICF_ENTERING) ? -1 : pnmhot->idOld, 
                             (pnmhot->dwFlags & HICF_LEAVING) ? -1 : pnmhot->idNew); )

        // While in edit mode, we do not automatically cascade 
        // submenus, unless while dropping.  But the dropping case
        // is handled in HitTest, not here.  So don't deal with that
        // here.

        // Is this because an accelerator key was hit?
        if (pnmhot->dwFlags & HICF_ACCELERATOR)
        {
            KillPopupTimer();
            KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
            // Yes; now that TBSTYLE_DROPDOWN is used, let _DropDownOrExec handle it
            // in response to TBN_DROPDOWN.
        }
        // Is this because direction keys were hit?
        else if (pnmhot->dwFlags & HICF_ARROWKEYS)
        {
            // Yes
            KillPopupTimer();
            KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);

            if (!_fVerticalMB && 
                _HandleObscuredItem(pnmhot->idNew))
            {
                lres = 1;
            }
            else
            {
                // It doesn't make sense that we would get these keyboard
                // notifications if there is a submenu open...it should get
                // the messages
                ASSERT(!_pcmb->_fInSubMenu);
                v_SendMenuNotification(pnmhot->idNew, FALSE);

                // Since the only way that the chevron can get the highlight is
                // through a keyboard down, then we expand.
                if (_fHasDemotedItems && pnmhot->idNew == (int)_idCmdChevron)
                {
                    v_CallCBItem(_idCmdChevron, SMC_CHEVRONEXPAND, 0, 0);
                    Expand(TRUE);
                    lres = 1;       // We already handled the hot item change
                }
            }
        }
        // Is this because the mouse moved or an explicit sendmessage?
        else if (!(pnmhot->dwFlags & HICF_LEAVING) && 
                 (pnmhot->idNew != _pcmb->_nItemCur || // Ignore if we're moving over same item
                  (_nItemTimer != -1 && _pcmb->_nItemCur == pnmhot->idNew)))     // we need to go through here to reset if the user went back to the cascaded guy
        {
            // Yes
            if (!_fVerticalMB)    // Horizontal menus will always have an underlying hmenu
            {
                if (_HandleObscuredItem(pnmhot->idNew))
                {
                    lres = 1;
                }
                else if (_pcmb->_fInSubMenu)
                {
                    // Only popup a menu since we're already in one (as mouse
                    // moves across bar).

                    TraceMsg(TF_MENUBAND, "(pmb=%#08lx): TBN_HOTITEMCHG: Posting CMBPopup message", this);
                    PostPopup(pnmhot->idNew, FALSE, _pcmb->_fKeyboardSelected);  // Will handle menu notification on receipt of message
                }
                else
                    v_SendMenuNotification(pnmhot->idNew, FALSE);
            }
            else if (!_fEditMode)
            {
                v_SendMenuNotification(pnmhot->idNew, FALSE);

                // check to see if we have just entered a new item and it is a sub-menu...

                // Did we already set a timer?
                if (-1 != _nItemTimer)
                {
                    // Yes; kill it b/c the mouse moved to another item
                    KillPopupTimer();
                }

                // if we're not over the currently expanded guy
                // Have we moved over an item that expands OR
                // are we moving away from a cascaded item?
                DWORD dwFlags = v_GetFlags(pnmhot->idNew);
                // Reset the stupid user timer
                KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);

                // UEMStuff
                if (!(dwFlags & SMIF_SUBMENU))
                {
                    _SetTimer(MBTIMER_UEMTIMEOUT);
                    _FireEvent(UEM_HOT_ITEM);
                }

                if ( (pnmhot->dwFlags & HICF_MOUSE) && _pcmb->_nItemCur != pnmhot->idNew) 
                {
                    if (dwFlags & SMIF_SUBMENU || _pcmb->_fInSubMenu)
                    {
                        // Is this the only item in the menu?
                        if ( _cPromotedItems == 1 && 
                            !(_fHasDemotedItems && _pcmb->_fExpanded) && 
                            dwFlags & SMIF_SUBMENU)
                        {
                            // Yes; Then we want to pop it open immediatly, 
                            // instead of waiting for the timeout
                            PostPopup(pnmhot->idNew, FALSE, FALSE);
                        }
                        else if (_SetTimer(MBTIMER_POPOUT))
                        {
                            // No; fire a timer to open/close the submenu
                            TraceMsg(TF_MENUBAND, "(pmb=%#08lx): TBN_HOTITEMCHG: Starting timer for id=%d", this, pnmhot->idNew);
                            if (v_GetFlags(pnmhot->idNew) & SMIF_SUBMENU)
                                _nItemTimer = pnmhot->idNew;
                            else
                                _nItemTimer = -1;
                        }
                    }
                    
                    if (_fHasDemotedItems && pnmhot->idNew == (int)_idCmdChevron)
                    {
                        _SetTimer(MBTIMER_EXPAND);
                    }

                    _pcmb->_pmbState->HideTooltip(FALSE);
                    _SetTimer(MBTIMER_INFOTIP);
                }

            }
        }
        else if (pnmhot->dwFlags & HICF_LEAVING)
        {
            v_SendMenuNotification(pnmhot->idOld, TRUE);

            if (-1 != _nItemTimer && !_fEditMode)
            {
                // kill the cascading menu popup timer...
                TraceMsg(TF_MENUBAND, "(pmb=%#08lx): TBN_HOTITEMCHG: Killing timer", this);
            
                KillPopupTimer();
            }
            _pcmb->_pmbState->HideTooltip(FALSE);
        }

        if ( !(pnmhot->dwFlags & HICF_LEAVING) )
            _pcmb->_SiteOnSelect(MPOS_CHILDTRACKING);
    }

    return lres;
}    

void CMenuToolbarBase::s_FadeCallback(DWORD dwStep, LPVOID pvParam)
{
    CMenuToolbarBase* pmtb = (CMenuToolbarBase*)pvParam;

    if (pmtb && dwStep == FADE_BEGIN)    // Paranoia
    {
        // Command has been posted.  Exit menu.
        pmtb->_pcmb->_SiteOnSelect(MPOS_EXECUTE);
    }
}   

LRESULT CMenuToolbarBase::_DropDownOrExec(UINT idCmd, BOOL bKeyboard)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    TraceMsg(TF_MENUBAND, "(pmb=%#08lx): _DropDownOrExec %d", this, idCmd);

    // Don't do anything when we're in edit mode
    if (_fEditMode)
        return 0;

    if ( v_GetFlags(idCmd) & SMIF_SUBMENU )
    {
        v_SendMenuNotification(idCmd, FALSE);
        
        // Yes.  Post a message to pop this up.
        //
        // !!  BUGBUG (scotth): We used to call _DoPopup here, but that causes
        //     re-entrancy problems when you click on the menu b/t two windows
        //     while one menu is still down.  The better fix is to serialize
        //     the message filter objects.  
        
        PostPopup(idCmd, FALSE, bKeyboard);
    }
    else if (idCmd != -1)
    {
        RECT rc;
        AddRef();   // I might get released in the call.

        // Fading Selection
        IEPlaySound(TEXT("MenuCommand"), TRUE);
        SendMessage(_hwndMB, TB_GETRECT, idCmd, (LPARAM)&rc);
        MapWindowPoints(_hwndMB, HWND_DESKTOP, (POINT*)&rc, 2);

        if (!(GetKeyState(VK_SHIFT) < 0))
        {
            // Were we able to fade?
            if (!_pcmb->_pmbState->FadeRect(&rc, s_FadeCallback, this))
            {
                // No; Then we blow away the menus here instead of the Fade callback
                // Command has been posted.  Exit menu.
                _pcmb->_SiteOnSelect(MPOS_EXECUTE);
            }
        }

        if (g_dwProfileCAP & 0x00002000) 
            StartCAP();
        v_ExecItem(idCmd);
        if (g_dwProfileCAP & 0x00002000) 
            StopCAP();

        Release();
    }
    else
        MessageBeep(MB_OK);

    return 0;
}


/*----------------------------------------------------------
Purpose: Handles TBN_DROPDOWN, which is sent on the button-down.

*/
LRESULT CMenuToolbarBase::_OnDropDown(LPNMTOOLBAR pnmtb)
{
    DWORD dwInput = _fTopLevel ? 0 : -1;    // -1: don't track, 0: do
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    LRESULT lres = 0;

    // Expected behavior with the mouse:
    //
    // 1) For cascading menuitems-
    //    a) expand on button-down
    //    b) collapse on button-up (horizontal menu only)
    //    c) if the button-down occurs on the item that is
    //       already selected, then assume the click indicates
    //       a drag/drop scenario
    // 2) For other menuitems-
    //    a) execute on button-up

#ifdef DEBUG
    if (_fTopLevel) {
        // browser menu comes thru here; start menu goes elsewhere (via tray.c)
        //ASSERT(!_fVertical);
        TraceMsg(DM_MISC, "cmtbb._odd: _fTopLevel(1) mouse=%d", GetKeyState(VK_LBUTTON) < 0);
    }
#endif
    // Is this because the mouse button was used?
    if (GetKeyState(VK_LBUTTON) < 0)
    {
        // Yes

        // Assume it won't be handled.  This will allow the toolbar
        // to see the button-down as a potential drag and drop.
        lres = TBDDRET_TREATPRESSED;

        // Clicking on same item that is currently expanded?
        if (pnmtb->iItem == _pcmb->_nItemCur)
        {

            // Is this horizontal?
            if (!_fVerticalMB)
            {
                // Yes; toggle the dropdown
                _pcmb->_SubMenuOnSelect(MPOS_FULLCANCEL);
                
                // Say it is handled, so the button will toggle
                lres = TBDDRET_DEFAULT;
            }
            
            _fClickHandled = TRUE;
            
            // Otherwise don't do anything more, user might be starting a 
            // drag-drop procedure on the cascading menuitem
        }
        else
        {
            if (v_GetFlags(pnmtb->iItem) & SMIF_SUBMENU)
            {
                // Handle on the button-down
                _fClickHandled = TRUE;
                lres = _DropDownOrExec(pnmtb->iItem, FALSE);
            }
        }

        if (dwInput != -1)
            dwInput = UIBL_INPMOUSE;
    }
    else
    {
        // No; must be the keyboard
        _fClickHandled = TRUE;
        lres = _DropDownOrExec(pnmtb->iItem, TRUE);

        if (dwInput != -1)
            dwInput = UIBL_INPMENU;
    }

    // browser menu (*not* start menu) alt+key, mouse
    if (dwInput != -1)
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UIINPUT, dwInput);

    return lres;
}    


/*----------------------------------------------------------
Purpose: Handle WM_KEYDOWN/WM_KEYUP

Returns: TRUE if handled
*/
BOOL CMenuToolbarBase::_OnKey(BOOL bDown, UINT vk, UINT uFlags)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    int idCmd;
    HWND hwnd = _hwndMB;

    _pcmb->_pmbState->SetKeyboardCue(TRUE);

    //
    // If the menu window is RTL mirrored, then the arrow keys should
    // be mirrored to reflect proper cursor movement. [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwnd))
    {
        switch (vk)
        {
        case VK_LEFT:
          vk = VK_RIGHT;
          break;

        case VK_RIGHT:
          vk = VK_LEFT;
          break;
        }
    }

    switch (vk)
    {
    case VK_LEFT:
        if (_fVerticalMB)
        {
            _pcmb->_SiteOnSelect(MPOS_SELECTLEFT);
            return TRUE;
        }
        break;

    case VK_RIGHT:
        if (_fVerticalMB)
            goto Cascade;
        break;

    case VK_DOWN:
    case VK_UP:
        if (!_fVerticalMB)
        {
Cascade:
            idCmd = GetButtonCmd(hwnd, ToolBar_GetHotItem(hwnd));
            if (v_GetFlags(idCmd) & SMIF_SUBMENU)
            {
                // Enter the submenu
                TraceMsg(TF_MENUBAND, "(pmb=%#08lx): _OnKey: Posting CMBPopup message", this);
                
                PostPopup(idCmd, FALSE, TRUE);
            }
            else if (VK_RIGHT == vk)
            {
                // Nothing to cascade to, move to next sibling menu
                _pcmb->_SiteOnSelect(MPOS_SELECTRIGHT);
            }
            return TRUE;
        }
        else
        {
#if 0
            _pcmb->_OnSelectArrow(vk == VK_UP? -1 : 1);
            return TRUE;
#endif
        }
        break;

    case VK_SPACE:

        if (!_pcmb->_fExpanded && _fHasDemotedItems)
        {
            v_CallCBItem(_idCmdChevron, SMC_CHEVRONEXPAND, 0, 0);
            Expand(TRUE);
        }
        else
        {
            // Toolbars map the spacebar to VK_RETURN.  Menus don't except
            // in the horizontal menubar.
            if (_fVerticalMB)
                MessageBeep(MB_OK);
        }
        return TRUE;

#if 0
    case VK_RETURN:
        // Handle this now, rather than letting the toolbar handle it.
        // This way we don't have to rely on WM_COMMAND, which doesn't
        // convey whether it was invoked by the keyboard or the mouse.
        idCmd = GetButtonCmd(hwnd, ToolBar_GetHotItem(hwnd));
        _DropDownOrExec(idCmd, TRUE);
        return TRUE;
#endif
    }

    return FALSE;
}    

/*----------------------------------------------------------
Purpose: There are two flavors of this function: _DoPopup and
         PostPopup.  Both cancel the existing submenu (relative 
         to this band) and pops open a new submenu.  _DoPopup
         does it atomically.  PostPopup posts a message to
         handle it.

*/
void CMenuToolbarBase::_DoPopup(int idCmd, BOOL bInitialSelect)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (-1 != idCmd)
    {
        PopupHelper(idCmd, bInitialSelect);
    }
}    


/*----------------------------------------------------------
Purpose: See the _DoPopup comment
*/
void CMenuToolbarBase::PostPopup(int idCmd, BOOL bSetItem, BOOL bInitialSelect)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (-1 != idCmd)
    {
        _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);
        _pcmb->SetTracked(this);
        HWND hwnd = _pcmb->_pmbState->GetSubclassedHWND();

        PostMessage(hwnd? hwnd: _hwndMB, g_nMBPopupOpen, idCmd, MAKELPARAM(bSetItem, bInitialSelect));
    }
}    


/*----------------------------------------------------------
Purpose: Helper function to finally invoke submenu.  Use _DoPopup
         or PostPopup
*/
void CMenuToolbarBase::PopupHelper(int idCmd, BOOL bInitialSelect)
{
    // We do not want to pop open a sub menu if we are not displayed. This is especially
    // a problem during drag and drop.
    if (_fShowMB)
    {
        _pcmb->_nItemNew = idCmd;
        ASSERT(-1 != _pcmb->_nItemNew);
        _pcmb->SetTracked(this);
        _pcmb->_fPopupNewMenu = TRUE;
        _pcmb->_fInitialSelect = BOOLIFY(bInitialSelect);
        _pcmb->UIActivateIO(TRUE, NULL);
        _FireEvent(UEM_HOT_FOLDER);
        _SetTimer(MBTIMER_UEMTIMEOUT);
    }
}

void    CMenuToolbarBase::_PaintButton(HDC hdc, int idCmd, LPRECT prc, DWORD dwSMIF)
{
    if (!_pcmb->_fExpanded)
        return;

    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    RECT rcClient;
    GetClientRect(_hwndMB, &rcClient);
#ifndef DRAWEDGE
    // Draw Left Edge
    HPEN hPenOld = (HPEN)SelectObject(hdc, _pcmb->_pmbm->_hPenHighlight);
    MoveToEx(hdc, prc->left, prc->top, NULL);
    LineTo(hdc, prc->left, prc->bottom);
#endif

    if (!(dwSMIF & SMIF_DEMOTED))
    {
#ifdef DRAWEDGE
        DWORD dwEdge = BF_RIGHT;

        // Don't paint the edge next to the bitmap.
        if (_uIconSizeMB == ISFBVIEWMODE_SMALLICONS)
            dwEdge |= BF_LEFT;


        RECT rc = *prc;
#else
        // Draw Right Edge:
        SelectObject(hdc, _pcmb->_pmbm->_hPenShadow);
        MoveToEx(hdc, prc->right-1, prc->top, NULL);
        LineTo(hdc, prc->right-1, prc->bottom);
#endif

        HWND hwnd = _hwndMB;
        int iPos = ToolBar_CommandToIndex(hwnd, idCmd);
        if (iPos == -1)
        {
            iPos = ToolBar_ButtonCount(hwnd) - 1;
        }

        if (iPos >= 0)
        {
            int iNumButtons = ToolBar_ButtonCount(hwnd);
            int idCmd2 = GetButtonCmd(hwnd, iPos + 1);
            CMenuToolbarBase* pmtb = this;
            BOOL    fOverflowed = FALSE;

            // Situations for Drawing the Bottom line
            // 1) This button is at the bottom.
            // 2) This button is at the bottom and the toolbar
            //      below is not visible (_fDontShowEmpty).
            // 3) This button is at the bottom and the button
            //      at the top of the bottom toolbar is demoted.
            // 4) The button below this one in the toolbar is
            //      demoted.
            // 5) The botton below this one is demoted and we're
            //      not expanded
    
            if (iPos + 1 >= iNumButtons)
            {
                if (_pcmb->_pmtbBottom != this &&
                    !_pcmb->_pmtbBottom->_fDontShowEmpty)
                {
                    pmtb = _pcmb->_pmtbBottom;
                    hwnd = pmtb->_hwndMB;
                    idCmd2 = GetButtonCmd(hwnd, 0);
                }
                else
                    fOverflowed = TRUE;
            }
            else if (prc->bottom == rcClient.bottom &&
                _pcmb->_pmtbBottom == this)   // This button is at the top.
                fOverflowed = TRUE;


            DWORD dwFlags = pmtb->v_GetFlags(idCmd2);

            if ((_pcmb->_fExpanded && dwFlags & SMIF_DEMOTED) || 
                 fOverflowed)
            {
#ifdef DRAWEDGE
                dwEdge |= BF_BOTTOM;
#else
                int iLeft = prc->left;
                if (iPos != iNumButtons - 1)   
                    iLeft ++;   // Move the next line in.

                MoveToEx(hdc, iLeft, prc->bottom-1, NULL);
                LineTo(hdc, prc->right-1, prc->bottom-1);
#endif
            }

            // Situations for Drawing the Top line
            // 1) This button is at the top.
            // 2) This button is at the top and the toolbar
            //      above is not visible (_fDontShowEmpty).
            // 3) This button is at the top and the button
            //      at the bottom of the top toolbar is demoted.
            // 4) The button above this one in the toolbar is
            //      demoted.
            // 5) If the button above this is demoted, and we're
            //      not expanded

            fOverflowed = FALSE; 

            if (iPos - 1 < 0)
            {
                if (_pcmb->_pmtbTop != this && 
                    !_pcmb->_pmtbTop->_fDontShowEmpty)
                {
                    pmtb = _pcmb->_pmtbTop;
                    hwnd = pmtb->_hwndMB;
                    idCmd2 = GetButtonCmd(hwnd, ToolBar_ButtonCount(hwnd) - 1);
                }
                else
                    fOverflowed = TRUE; // There is nothing at the top of this menu, draw the line.
            }
            else
            {
                hwnd = _hwndMB;
                idCmd2 = GetButtonCmd(hwnd, iPos - 1);
                pmtb = this;

                if (prc->top == rcClient.top &&
                    _pcmb->_pmtbTop == this)   // This button is at the top.
                    fOverflowed = TRUE;
            }

            dwFlags = pmtb->v_GetFlags(idCmd2);

            if ((_pcmb->_fExpanded && dwFlags & SMIF_DEMOTED) ||
                fOverflowed)
            {
#ifdef DRAWEDGE
                dwEdge |= BF_TOP;
#else
                SelectObject(hdc, _pcmb->_pmbm->_hPenHighlight);
                MoveToEx(hdc, prc->left, prc->top, NULL);
                LineTo(hdc, prc->right-1, prc->top);
#endif
            }
        }

#ifdef DRAWEDGE
        DrawEdge(hdc, &rc, BDR_RAISEDINNER, dwEdge);
#endif
    }

#ifndef DRAWEDGE
    SelectObject(hdc, hPenOld);
#endif
}

LRESULT CMenuToolbarBase::_OnCustomDraw(NMCUSTOMDRAW * pnmcd)
{
    // Make it look like a menu
    NMTBCUSTOMDRAW * ptbcd = (NMTBCUSTOMDRAW *)pnmcd;
    DWORD dwRet = 0;
        
    // Edit mode never hot tracks, and the selected item being
    // moved has a black frame around it.  Items that cascade are 
    // still highlighted normally, even in edit mode.

    DWORD dwSMIF = v_GetFlags((UINT)pnmcd->dwItemSpec);

    switch(pnmcd->dwDrawStage)
    {
    case CDDS_PREPAINT:
        dwRet = CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
        break;

    case CDDS_ITEMPREPAINT:
        if (_fVerticalMB)
        {
            if (pnmcd->dwItemSpec == -1)
            {
                // a -1 is sent with a seperator
                RECT rc = pnmcd->rc;
                rc.top += 3;    // Hard coded in toolbar.
                rc.left += GetSystemMetrics(SM_CXEDGE);
                rc.right -= GetSystemMetrics(SM_CXEDGE);
                DrawEdge(pnmcd->hdc, &rc, EDGE_ETCHED, BF_TOP);

                _PaintButton(pnmcd->hdc, -1, &pnmcd->rc, dwSMIF);

                dwRet = CDRF_SKIPDEFAULT;
            }
            else
            {
                ptbcd->clrText = _pcmb->_pmbm->_clrMenuText;

                // This is for Darwin Ads.
                if (dwSMIF & SMIF_ALTSTATE)
                {
                    ptbcd->clrText = GetSysColor(COLOR_BTNSHADOW);
                }

                ptbcd->rcText.right = ptbcd->rcText.right - _pcmb->_pmbm->_cxMargin;
                ptbcd->clrBtnFace = _pcmb->_pmbm->_clrBackground;
                if (_fHasSubMenu)
                    ptbcd->rcText.right -= _pcmb->_pmbm->_cxArrow;

                if ( _fHasDemotedItems && _idCmdChevron == (int)pnmcd->dwItemSpec)
                {
                    _DrawChevron(pnmcd->hdc, &pnmcd->rc, 
                        (BOOL)(pnmcd->uItemState & CDIS_HOT) ||
                         (BOOL)(pnmcd->uItemState & CDIS_MARKED), 
                        (BOOL)(pnmcd->uItemState & CDIS_SELECTED) );

                    dwRet |= CDRF_SKIPDEFAULT;

                }
                else
                {
#ifdef MARK_DRAGGED_ITEM
                    // We have no good way to undo this on a multi pane drop.
                    if (_idCmdDragging != -1 &&
                        _idCmdDragging == (int)pnmcd->dwItemSpec)
                        pnmcd->uItemState |= CDIS_HOT;
#endif

                    // Yes; draw with highlight
                    if (pnmcd->uItemState & (CDIS_CHECKED | CDIS_SELECTED | CDIS_HOT))
                    {
#ifdef UNIX
                        if( MwCurrentLook() == LOOK_MOTIF )
                            SelectMotifMenu(pnmcd->hdc, &pnmcd->rc, TRUE );
                        else
#endif
                        {
                            ptbcd->clrHighlightHotTrack = GetSysColor(COLOR_HIGHLIGHT);
                            ptbcd->clrBtnFace = GetSysColor(COLOR_HIGHLIGHT);
                            ptbcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                            dwRet |= TBCDRF_HILITEHOTTRACK;
                        }
                    }


                    // Is this menu empty?
                    if (_fEmpty)
                    {
                        // Yes, draw the empty string as disabled.
                        pnmcd->uItemState |= CDIS_DISABLED;
                        ptbcd->clrText = ptbcd->clrBtnFace;

                        // Don't draw the etched effect if it is selected
                        if (pnmcd->uItemState & CDIS_HOT)
                            dwRet |= TBCDRF_NOETCHEDEFFECT;
                    }

                    // When this item is demoted, we only want to paint his background
                    // then we are in edit mode _OR_ it is not selected, checked or hot.
                    if (dwSMIF & SMIF_DEMOTED)
                    {
                        BOOL fDrawDemoted = TRUE;
                        if (_fEditMode)
                            fDrawDemoted = TRUE;

                        if (pnmcd->uItemState & (CDIS_CHECKED | CDIS_SELECTED | CDIS_HOT))
                            fDrawDemoted = FALSE;

                        if (fDrawDemoted)
                        {
                            ptbcd->clrBtnFace = _pcmb->_pmbm->_clrDemoted;
                            SHFillRectClr(pnmcd->hdc, &pnmcd->rc, ptbcd->clrBtnFace);
                        }
                    }

                    // We draw our own highlighting
                    dwRet |= (TBCDRF_NOEDGES | TBCDRF_NOOFFSET);
                }
            }
        }
        else
        {
            // If g_fRunOnMemphis or g_fRunOnNT5 are not defined then the menus will
            // never be grey.
            if (!_pcmb->_fAppActive)
                // menus from user use Button Shadow for non active menus
                ptbcd->clrText = GetSysColor(COLOR_3DSHADOW);
            else
                ptbcd->clrText = _pcmb->_pmbm->_clrMenuText;

            // If we're in high contrast mode, make the menu bar look like
            // veritcal items on select.
            if (_pcmb->_pmbm->_fHighContrastMode)
            {
                // Yes; draw with highlight
                if (pnmcd->uItemState & (CDIS_CHECKED | CDIS_SELECTED | CDIS_HOT))
                {
#ifdef UNIX
                    if( MwCurrentLook() == LOOK_MOTIF )
                        SelectMotifMenu(pnmcd->hdc, &pnmcd->rc, TRUE );
                    else
#endif
                    {
                        ptbcd->clrHighlightHotTrack = GetSysColor(COLOR_HIGHLIGHT);
                        ptbcd->clrBtnFace = GetSysColor(COLOR_HIGHLIGHT);
                        ptbcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                        dwRet |= TBCDRF_HILITEHOTTRACK;
                    }
                }
            }
        }
        dwRet |= CDRF_NOTIFYPOSTPAINT | TBCDRF_NOMARK;
        break;
    case CDDS_ITEMPOSTPAINT:
        if (_fVerticalMB)
        {
            RECT rc = pnmcd->rc;
            COLORREF rgbText;
            if (pnmcd->uItemState & (CDIS_SELECTED | CDIS_HOT))
                rgbText = GetSysColor( COLOR_HIGHLIGHTTEXT );
            else
                rgbText = _pcmb->_pmbm->_clrMenuText;

            // Is this item Checked?
            if (dwSMIF & SMIF_CHECKED)
            {
                rc.right = rc.left + (rc.bottom - rc.top);
                _DrawMenuGlyph(pnmcd->hdc, _pcmb->_pmbm->_hFontArrow
                    , &rc, CH_MENUCHECKA, rgbText, NULL);
                rc = pnmcd->rc;
            }
    
            // Is this a cascading item?
            if (dwSMIF & SMIF_SUBMENU)
            {
                // Yes; draw the arrow
                RECT rcT = rc;
        
                rcT.left = rcT.right - _pcmb->_pmbm->_cxArrow;
                _DrawMenuArrowGlyph(pnmcd->hdc, &rcT, rgbText);
            }

            _PaintButton(pnmcd->hdc, (UINT)pnmcd->dwItemSpec, &rc, dwSMIF);
        }
        break;
    case CDDS_PREERASE:
        {
            RECT rcClient;
            GetClientRect(_hwndMB, &rcClient);
            ptbcd->clrBtnFace = _pcmb->_pmbm->_clrBackground;
            SHFillRectClr(pnmcd->hdc, &rcClient, _pcmb->_pmbm->_clrBackground);
            dwRet = CDRF_SKIPDEFAULT;
        }
        break;
    }
    return dwRet;
}    



void CMenuToolbarBase::_PressBtn(int idBtn, BOOL bDown)
{
    if (!_fVerticalMB)
    {
        DWORD dwState = ToolBar_GetState(_hwndMB, idBtn);

        if (bDown)
            dwState |= TBSTATE_PRESSED;
        else
            dwState &= ~TBSTATE_PRESSED;

        ToolBar_SetState(_hwndMB, idBtn, dwState);

        // Avoid ugly late repaints
        UpdateWindow(_hwndMB);
    }
}    


/*----------------------------------------------------------
Purpose: IWinEventHandler::OnWinEvent method

         Processes messages passed on from the menuband.
*/
STDMETHODIMP CMenuToolbarBase::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    HRESULT hres = S_FALSE;

    EnterModeless();

    switch (uMsg)
    {
    case WM_SETTINGCHANGE:
        if ((SHIsExplorerIniChange(wParam, lParam) == EICH_UNKNOWN) || 
            (wParam == SPI_SETNONCLIENTMETRICS))
        {
            v_UpdateIconSize(-1, TRUE);
            v_Refresh();
            goto L_WM_SYSCOLORCHANGE;
        }
        break;

    case WM_SYSCOLORCHANGE:
    L_WM_SYSCOLORCHANGE:
        ToolBar_SetInsertMarkColor(_hwndMB, GetSysColor(COLOR_MENUTEXT));
        SendMessage(_hwndMB, uMsg, wParam, lParam);
        InvalidateRect(_hwndMB, NULL, TRUE);
        hres = S_OK;
        break;

    case WM_PALETTECHANGED:
        InvalidateRect( _hwndMB, NULL, FALSE );
        SendMessage( _hwndMB, uMsg, wParam, lParam );
        hres = S_OK;
        break;

#if 0
        // BUGBUG (lamadio): is this needed anymore?
    case WM_COMMAND:
        {
            UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
            *plres = _DropDownOrExec(idCmd, FALSE);
            hres = S_OK;
        }
        break;
#endif
        
    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        hres = S_OK;
        break;
    }

    ExitModeless();

    return hres;
}


void CMenuToolbarBase::v_CalcWidth(int* pcxMin, int* pcxMax)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    ASSERT(IS_VALID_WRITE_PTR(pcxMin, int));
    ASSERT(IS_VALID_WRITE_PTR(pcxMax, int));

     
    *pcxMin = 0;
    *pcxMax = 0;

    if (_fVerticalMB && _pcmb->_pmbm && _pcmb->_pmbm->_hFontMenu)
    {
        HIMAGELIST himl;
        int cel;
        int cxItemMax = 0;
        HWND hwnd = _hwndMB;
        
        ASSERT(hwnd);
        
        HDC hdc = GetDC(hwnd);
        HFONT hFontOld = (HFONT) SelectObject(hdc, _pcmb->_pmbm->_hFontMenu);
        
        TCHAR sz[MAX_PATH];
        cel = ToolBar_ButtonCount(hwnd);

        // Find the maximum length text
        for(int i = 0; i < cel; i++)
        {
            int idCmd = GetButtonCmd(hwnd, i);
            if (_idCmdChevron != idCmd &&
                !(!_pcmb->_fExpanded && v_GetFlags(idCmd) & SMIF_DEMOTED) &&
                SendMessage(hwnd, TB_GETBUTTONTEXT, idCmd, (LPARAM)sz) > 0)
            {
                RECT rect = {0};
                DrawText(hdc, sz, -1, &rect, DT_CALCRECT | DT_SINGLELINE | DT_LEFT | DT_VCENTER);
                cxItemMax = max(rect.right, cxItemMax);
            }
        }

        SelectObject(hdc, hFontOld);
        ReleaseDC(hwnd, hdc);
        
        himl = (HIMAGELIST)SendMessage(hwnd, TB_GETIMAGELIST, 0, 0);
        if (himl)
        {
            int cy;
            
            // Start with the width of the button
            ImageList_GetIconSize(himl, pcxMin, &cy);

            // We want at least a bit of space around the icon
            if (_uIconSizeMB != ISFBVIEWMODE_SMALLICONS)
            {
                // Old FSMenu code took the height of the larger of 
                // the icon and text then added 2.
                ToolBar_SetPadding(hwnd, 0, 0);
                *pcxMin += 10;
            }
            else 
            {
                // Old FSMenu code took the height of the larger of 
                // the icon and text then added cySpacing, which defaults to 6.
                ToolBar_SetPadding(hwnd, 0, 4);
                *pcxMin += 3 * GetSystemMetrics(SM_CXEDGE);
            }
        }

        
        RECT rect = {0};
        int cxDesired = _pcmb->_pmbm->_cxMargin + cxItemMax + _pcmb->_pmbm->_cxArrow;
        int cxMax = 0;
           
        if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &rect, 0))
        {
            // We're figuring a third of the screen is a good max width
            cxMax = (rect.right-rect.left) / 3;
        }

        *pcxMin += min(cxDesired, cxMax) + LIST_GAP;
        *pcxMax = *pcxMin;
    }
    TraceMsg(TF_MENUBAND, "CMenuToolbarBase::v_CalcWidth(%d, %d)", *pcxMin, *pcxMax);
}


void CMenuToolbarBase::_SetToolbarState()
{
    SHSetWindowBits(_hwndMB, GWL_STYLE, TBSTYLE_LIST, TBSTYLE_LIST);
}


void CMenuToolbarBase::v_ForwardMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    POINT pt;
    
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    GetWindowRect(_hwndMB, &rc);

    if (PtInRect(&rc, pt))
    {
        ScreenToClient(_hwndMB, &pt);
        SendMessage(_hwndMB, uMsg, wParam, MAKELONG(pt.x, pt.y));
    }
}

void CMenuToolbarBase::NegotiateSize()
{
    RECT rc;
    GetClientRect(GetParent(_hwndMB), &rc);
    _pcmb->OnPosRectChangeDB(&rc);

    // If we came in here it's because the Menubar did not change sizes or position.

}

void CMenuToolbarBase::SetParent(HWND hwndParent) 
{ 
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    if (hwndParent)
    {
        if (!_hwndMB)
            CreateToolbar(hwndParent);
    }
    else
    {
        // As an optimization, we implement "disowning" ourselves
        // as just moving ourselves offscreen.  The previous parent
        // still owns us.  The parent is invariably the menusite.
        RECT rc = {-1,-1,-1,-1};
        SetWindowPos(NULL, &rc, 0);
    }

    // We want to set the parent all the time because we don't want to destroy the 
    // window with it's parent..... Sizing to -1,-1,-1,-1 causes it not to be displayed.
    if (_hwndMB)
    {
        ::SetParent(_hwndMB, hwndParent); 
        SendMessage(_hwndMB, TB_SETPARENT, (WPARAM)hwndParent, NULL);
    }
}


void CMenuToolbarBase::v_OnEmptyToolbar()
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    for (int iNumButtons = ToolBar_ButtonCount(_hwndMB) -1;
         iNumButtons >= 0; 
         iNumButtons--)
    {
        // HACKHACK (lamadio): For some reason, _fEmptyingToolbar gets set to FALSE.
        // We then Do a TB_DELETEBUTTON, which sends a notify. This does go through on
        // the top level menubands (Start Menu, Browser menu bar), and deletes the 
        // associated data. We then try and delete it again.
        // So now, I set null into the sub menu, so that the other code gracefully fails.

        TBBUTTONINFO tbbi;
        tbbi.cbSize = SIZEOF(tbbi);
        tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
        ToolBar_GetButtonInfo(_hwndMB, iNumButtons, &tbbi);

        LPVOID pData = (LPVOID)tbbi.lParam;
        tbbi.lParam = NULL;

        ToolBar_SetButtonInfo(_hwndMB, iNumButtons, &tbbi);

        SendMessage(_hwndMB, TB_DELETEBUTTON, iNumButtons, 0);
        v_OnDeleteButton(pData);
    }
}

void CMenuToolbarBase::EmptyToolbar()
{
    if (_hwndMB)
    {
        _fEmptyingToolbar = TRUE;
        v_OnEmptyToolbar();
        _fEmptyingToolbar = FALSE;
    }
}

void CMenuToolbarBase::v_Close()
{
    EmptyToolbar();
    if (_hwndMB)
    {
        //Kill timers to prevent race condition
        KillTimer(_hwndMB, MBTIMER_POPOUT);
        KillTimer(_hwndMB, MBTIMER_DRAGOVER);
        KillTimer(_hwndMB, MBTIMER_EXPAND);
        KillTimer(_hwndMB, MBTIMER_ENDEDIT);
        KillTimer(_hwndMB, MBTIMER_CLOSE);
        KillTimer(_hwndMB, MBTIMER_CLICKUNHANDLE);
        KillTimer(_hwndMB, MBTIMER_DRAGPOPDOWN);

        DestroyWindow(_hwndMB);
        _hwndMB = NULL;
    }
}

void CMenuToolbarBase::Activate(BOOL fActivate)
{
    if (fActivate == FALSE)
    {
        _fEditMode = FALSE;
    }
}

int CMenuToolbarBase::_CalcChevronSize()
{

    int dSeg;
    int dxy = _pcmb->_pmbm->_cyChevron;

    dxy -= 4;
    dSeg = dxy / 4;

    return dSeg * 4 + 4;
}

void CMenuToolbarBase::_DrawChevron(HDC hdc, LPRECT prect, BOOL fFocus, BOOL fSelected)
{
    RECT rcBox = *prect;
    RECT rcDrop;

    const int dExtra = 3;
    int dxy;

    rcBox.left += dExtra;
    rcBox.right -= dExtra;
    dxy = _CalcChevronSize();

    rcDrop.left = ((rcBox.right + rcBox.left) >> 1) - (dxy/4);
    rcDrop.right = rcDrop.left + dxy - 1;

    int dSeg = ((RECTWIDTH(rcDrop) - 2) >> 2);

    rcDrop.top = (rcBox.top + rcBox.bottom)/2 - (2 * dSeg + 1);
    //rcDrop.bottom = rcBox.top;

    if (fFocus)
    {
        InflateRect(&rcBox, 0, -3);
        SHFillRectClr(hdc, &rcBox, _pcmb->_pmbm->_clrDemoted);
        DrawEdge(hdc, &rcBox, fSelected? BDR_SUNKENINNER : BDR_RAISEDINNER, BF_RECT);

        if (fSelected)
        {
            rcDrop.top += 1;
            rcDrop.left += 1;
        }
    }

    HBRUSH hbrOld = SelectBrush(hdc, _pcmb->_pmbm->_hbrText);


    int y = rcDrop.top + 1;
    int xBase = rcDrop.left+ dSeg;

    for (int x = -dSeg; x <= dSeg; x++)
    {
        PatBlt(hdc, xBase + x, y, 1, dSeg, PATCOPY);
        PatBlt(hdc, xBase + x, y+(dSeg<<1), 1, dSeg, PATCOPY);

        y += (x >= 0) ? -1 : 1;
    }

    SelectBrush(hdc, hbrOld);
}


// Takes into accout Separators, hidden and Disabled items
/*----------------------------------------------------------
Purpose: This function sets the nearest legal button to be
         the hot item, skipping over any separators, or hidden
         or disabled buttons.
    

*/

int CMenuToolbarBase::GetValidHotItem(int iDir, int iIndex, int iCount, DWORD dwFlags)
{
    if (iIndex == MBSI_LASTITEM)
    {
        // -2 is special value meaning "last item on toolbar"
        int cButtons = (int)SendMessage(_hwndMB, TB_BUTTONCOUNT, 0, 0);
        iIndex = cButtons - 1;
    }

    while ( (iCount == -1 || iIndex < iCount) && iIndex >= 0)
    {
        TBBUTTON tbb;

        // Toolbar will trap out of bounds condition when iCount is -1
        if (!SendMessage(_hwndMB, TB_GETBUTTON, iIndex, (LPARAM)&tbb))
            return -1;

        int idCmd = GetButtonCmd(_hwndMB, iIndex);


        if (tbb.fsState & TBSTATE_ENABLED && 
            !(tbb.fsStyle & TBSTYLE_SEP || 
              tbb.fsState & TBSTATE_HIDDEN) &&
              !(v_GetFlags(idCmd) & SMIF_DEMOTED && !_pcmb->_fExpanded) )
        {
            return iIndex;
        }
        else
            iIndex += iDir;
    }

    return -1;
}

BOOL CMenuToolbarBase::SetHotItem(int iDir, int iIndex, int iCount, DWORD dwFlags)
{
    int iPos = GetValidHotItem(iDir, iIndex, iCount, dwFlags);
    if (iPos >= 0)
        SendMessage(_hwndMB, TB_SETHOTITEM2, iPos, dwFlags);

    return (BOOL)(iPos >= 0);
}


static const BYTE g_rgsStateMap[][3] = 
{
#if defined(FIRST)
//     T,  I,  F
    {  0,  1,  2},      // State 0
    {  3,  1,  2},      // State 1
    {  4,  1,  2},      // State 2
    { 11,  5,  2},      // State 3
    { 10,  1,  6},      // State 4
    {  7,  1,  2},      // State 5
    {  8,  1,  2},      // State 6
    { 11,  9,  2},      // State 7
    { 10,  1, 10},      // State 8
    { 11,  1,  2},      // State 9
    { 10,  1,  2},      // State 10     // End State
    { 12,  1,  2},      // State 11     // Flash.
    { 10,  1,  2},      // State 12
#elif defined(SECOND)
//     T,  I,  F
    {  0,  1,  2},      // State 0
    {  3,  1,  2},      // State 1
    {  4,  1,  2},      // State 2
    { 11,  5,  6},      // State 3
    { 10,  5,  6},      // State 4
    {  7,  5,  6},      // State 5
    {  8,  9,  6},      // State 6
    { 11,  9,  8},      // State 7
    { 10,  9, 10},      // State 8
    { 11,  9,  8},      // State 9
    { 10, 10, 10},      // State 10     // End State
    { 10,  9,  8},      // State 11     // Flash.
    { 10,  9,  8},      // State 12
    { 10,  9,  8},      // State 13
#elif defined(THIRD)
//     T,  I,  F
    {  0,  1,  2},      // State 0
    {  3,  1,  2},      // State 1
    { 12,  1,  2},      // State 2
    { 11,  5,  6},      // State 3
    { 10,  5,  6},      // State 4
    {  7,  5,  6},      // State 5
    { 13,  5,  6},      // State 6
    { 11,  9,  8},      // State 7
    { 10,  9, 10},      // State 8
    { 11,  9,  8},      // State 9
    { 10, 10, 10},      // State 10     // End State
    { 10,  9,  8},      // State 11     // Flash.
    {  4,  1,  2},      // State 12
    {  8,  5,  6},      // State 13
#else
//     T,  I,  F
    {  0,  1,  2},      // State 0
    {  3,  1,  2},      // State 1
    {  4,  1,  2},      // State 2
    { 11,  5,  6},      // State 3
    { 10,  5,  6},      // State 4
    {  7,  5,  6},      // State 5
    {  8,  5,  6},      // State 6
    { 11,  9,  8},      // State 7
    { 10,  9, 10},      // State 8
    { 11,  9,  8},      // State 9
    { 10, 10, 10},      // State 10     // End State
    {  4,  3,  4},      // State 11     // Flash.
#endif
};

#define MAX_STATE 13

void CMenuToolbarBase::_FireEvent(BYTE bEvent)
{
    // We don't want to expand and cover up any dialogs.
    if (_fSuppressUserMonitor)
        return;

    if (!_fHasDemotedItems)
        return;

    if (UEM_RESET == bEvent)
    {
        TraceMsg(TF_MENUBAND, "CMTB::UEM Reset state to 0");
        _pcmb->_pmbState->SetUEMState(0);
        return;
    }

    ASSERT(bEvent >= UEM_TIMEOUT && 
            bEvent <= UEM_HOT_FOLDER);

    BYTE bOldState = _pcmb->_pmbState->GetUEMState();
    BYTE bNewState = g_rgsStateMap[_pcmb->_pmbState->GetUEMState()][bEvent];

    ASSERT(bOldState >= 0 &&  bOldState <= MAX_STATE);

    TraceMsg(TF_MENUBAND, "*** UEM OldState (%d), New State (%d) ***", bOldState, bNewState);

    _pcmb->_pmbState->SetUEMState(bNewState);

    switch (bNewState)
    {
    case 10:    // End State
        TraceMsg(TF_MENUBAND, "*** UEM Entering State 10. Expanding *** ", bOldState, bNewState);
        KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
        if (_pcmb->_fInSubMenu)
        {
            IUnknown_QueryServiceExec(_pcmb->_pmpSubMenu, SID_SMenuBandChild,
                &CGID_MenuBand, MBANDCID_EXPAND, 0, NULL, NULL);
        }
        else
        {
            Expand(TRUE);
        }
        _pcmb->_pmbState->SetUEMState(0);
        break;

    case 11:   // Flash
        // This gets reset when the flash is done...
        TraceMsg(TF_MENUBAND, "*** UEM Entering State 11 Flashing *** ");
        KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
        _FlashChevron();
        break;
    }
}


void CMenuToolbarBase::_FlashChevron()
{
    if (_idCmdChevron != -1)
    {
        _cFlashCount = 0;
        ToolBar_MarkButton(_hwndMB, _idCmdChevron, FALSE);
        SetTimer(_hwndMB, MBTIMER_FLASH, MBTIMER_FLASHTIME, NULL);
    }
}


LRESULT CMenuToolbarBase::_DefWindowProcMB(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Are we being asked for the IAccessible for the client?
    if (uMsg == WM_GETOBJECT && (OBJID_CLIENT == lParam))
    {
        // Don't process OBJID_MENU. By the time we get here, we ARE the menu.
        LRESULT lres = 0;
        CAccessible* pacc = new CAccessible(SAFECAST(_pcmb, IMenuBand*));
        if (pacc)
        {
            lres = pacc->InitAcc();
            if (SUCCEEDED((HRESULT)lres))
            {
                lres = LresultFromObject(IID_IAccessible, wParam, SAFECAST(pacc, IAccessible*));

                // The correct OLEAcc has been checked into the NT builds, so Oleacc
                // no longer assumes transfer sematics
                if (FAILED((HRESULT)lres))
                    pacc->Release();
            }
            else
            {   // Failed to initialize
                pacc->Release();
            }
        }

        return lres;
    }

    return 0;
}

void CMenuToolbarBase::v_Show(BOOL fShow, BOOL fForceUpdate)
{
    // HACKHACK (lamadio): When we create the menubands, we do not set the
    // TOP level band's fonts until a refresh. This code here fixes it.
    if (_fFirstTime && _pcmb->_fTopLevel)
    {
        SetMenuBandMetrics(_pcmb->_pmbm);
    }

    if (fShow)
    {
        SetKeyboardCue();
        _pcmb->_pmbState->PutTipOnTop();
    }
    else
    {
        _fHasDrop = FALSE;
        KillTimer(_hwndMB, MBTIMER_DRAGPOPDOWN);
        KillTimer(_hwndMB, MBTIMER_INFOTIP);    // Don't show it if we're not displayed :-)
        _pcmb->_pmbState->HideTooltip(TRUE);
    }

    _fSuppressUserMonitor = FALSE;

#ifdef UNIX
    if (_fVerticalMB)
    {
        ToolBar_SetHotItem(_hwndMB, 0);
    }
#endif
}

void CMenuToolbarBase::SetKeyboardCue()
{
    if (_pcmb->_pmbState)
    {
        SendMessage(GetParent(_hwndMB), WM_CHANGEUISTATE, 
            MAKEWPARAM(_pcmb->_pmbState->GetKeyboardCue() ? UIS_CLEAR : UIS_SET,
            UISF_HIDEACCEL), 0);
    }
}
