#include "cabinet.h"
#include "taskbar.h"

CTaskBar::CTaskBar() : CSimpleOleWindow(v_hwndTray)
{
    _fRestrictionsInited = FALSE;
}


HRESULT CTaskBar::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppvObj = SAFECAST(this, IContextMenu*);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
    else if (IsEqualIID(riid, IID_IRestrict))
    {
        *ppvObj = SAFECAST(this, IRestrict*);
    }
    else if (IsEqualIID(riid, IID_IDeskBar))
    {
        *ppvObj = SAFECAST(this, IDeskBar*);
    }
    else 
    {
        return CSimpleOleWindow::QueryInterface(riid, ppvObj);
    }

    AddRef();
    return S_OK;
}


HRESULT CTaskBar::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    int idCmd = -1;

    if (IS_INTRESOURCE(pici->lpVerb))
        idCmd = LOWORD(pici->lpVerb);

    Tray_ContextMenuInvoke(idCmd);

    return S_OK;
}

HRESULT CTaskBar::GetCommandString(UINT_PTR idCmd,
                            UINT        uType,
                            UINT      * pwReserved,
                            LPSTR       pszName,
                            UINT        cchMax)
{
    return E_NOTIMPL;
}


HRESULT CTaskBar::QueryContextMenu(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags)
{
    int i = 0;
    HMENU hmenuSrc = Tray_BuildContextMenu((S_OK == SHIsChildOrSelf(g_tasks.hwnd, GetFocus())) ? TRUE : FALSE);

    if (hmenuSrc) {
        // BUGBUG off-by-1 and by idCmdFirst+i, i think...
        i += Shell_MergeMenus(hmenu, hmenuSrc, (UINT)-1, idCmdFirst + i, idCmdLast, MM_ADDSEPARATOR) - (idCmdFirst + i);
        DestroyMenu(hmenuSrc);
    }
    
    return i;   // potentially off-by-1, but who cares...
}


// *** IServiceProvider ***
HRESULT CTaskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (ppvObj)
        *ppvObj = NULL;

    if (IsEqualGUID(guidService, SID_SRestrictionHandler))
    {
        return QueryInterface(riid, ppvObj);
    }
    
    return E_FAIL;
}


// *** IRestrict ***
HRESULT CTaskBar::IsRestricted(const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult)
{
    HRESULT hr = S_OK;

    if (!EVAL(pguidID) || !EVAL(pdwRestrictionResult))
        return E_INVALIDARG;

    *pdwRestrictionResult = RR_NOCHANGE;
    if (IsEqualGUID(RID_RDeskBars, *pguidID))
    {
        if (!_fRestrictionsInited)
        {
            _fRestrictionsInited = TRUE;
            if (SHRestricted(REST_NOCLOSE_DRAGDROPBAND))
                _fRestrictDDClose = TRUE;
            else
                _fRestrictDDClose = FALSE;

            if (SHRestricted(REST_NOMOVINGBAND))
                _fRestrictMove = TRUE;
            else
                _fRestrictMove = FALSE;
        }

        switch(dwRestrictAction)
        {
        case RA_DRAG:
        case RA_DROP:
        case RA_ADD:
        case RA_CLOSE:
            if (_fRestrictDDClose)
                *pdwRestrictionResult = RR_DISALLOW;
            break;
        case RA_MOVE:
            if (_fRestrictMove)
                *pdwRestrictionResult = RR_DISALLOW;
            break;
        }
    }

    // TODO: If we have or get a parent, we should ask them if they want to restrict.
//    if (RR_NOCHANGE == *pdwRestrictionResult)    // If we don't handle it, let our parents have a wack at it.
//        hr = IUnknown_HandleIRestrict(_punkParent, pguidID, dwRestrictAction, pvarArgs, pdwRestrictionResult);

    return hr;
}

#ifndef RECTWIDTH
#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
#endif

extern "C" void Tray_VerifySize(BOOL fWinIni);
extern "C" TrayShowWindow(int nCmdShow);
extern "C" void Tray_ClipWindow(BOOL fEnableClipping);

extern "C" int g_fInSizeMove;

// *** IDeskBar ***
HRESULT CTaskBar::OnPosRectChangeDB(LPRECT prc)
{
    int cyDelta, cxDelta;
    RECT rc, rcChild;

    // if we haven't fully initialized the tray, don't resize in response to (bogus) rebar sizes
    if (!g_ts.hbmpStartBkg)
        return S_FALSE;
    
    if (g_ts._fDeferedPosRectChange) {
        // we're in the moving code, don't do this stuff..
        return S_FALSE;
    }

    GetWindowRect(g_ts.hwndMain, &rc);
    GetClientRect(g_ts.hwndRebar, &rcChild); // Handle case of no coolbar???

    // Need to special-case sizing down to 0 rows
    if (RECTHEIGHT(rcChild) == 0)
    {
        return S_OK;
    }

    TraceMsg(TF_TRAY, "OnPosRectChangeDB: hwndMain= (%d,%d,%d,%d), hwndRebar= (%d,%d,%d,%d), prc= (%d,%d,%d,%d)",
        rc.left,rc.top,rc.right,rc.bottom,
        rcChild.left,rcChild.top,rcChild.right,rcChild.bottom,
        prc->left,prc->top,prc->right,prc->bottom);

    cyDelta = (RECTHEIGHT(*prc) - RECTHEIGHT(rcChild));
    cxDelta = (RECTWIDTH(*prc) - RECTWIDTH(rcChild));

    BOOL fHiding = (g_ts.uAutoHide & AH_HIDING);
    switch(g_ts.uStuckPlace)
    {
        case STICK_BOTTOM:
            if (fHiding)
            {
                rc.bottom -= RECTHEIGHT(rcChild);
                rc.top -= cyDelta + RECTHEIGHT(rcChild);
            }
            else
                rc.top -= cyDelta;
            break;
        case STICK_TOP:
            if (fHiding)
            {
                rc.top += RECTHEIGHT(rcChild);
                rc.bottom += cyDelta + RECTHEIGHT(rcChild);
            }
            else
                rc.bottom += cyDelta;
            break;
        case STICK_RIGHT:
            if (fHiding)
            {
                rc.right -= RECTWIDTH(*prc);
                rc.left -= cxDelta + RECTWIDTH(*prc);
            }
            else
                rc.left -= cxDelta;
            break;
        case STICK_LEFT:
            if (fHiding)
            {
                rc.left += RECTWIDTH(*prc);
                rc.right += cxDelta + RECTWIDTH(*prc);
            }
            else
                rc.right += cxDelta;
            break;
        default:
            ASSERT(0); // Where are we stuck???
    }

    TraceMsg(TF_TRAY, "OnPosRectChangeDB: moving to hwndMain = (%d,%d,%d,%d)",
        rc.left,rc.top,rc.right,rc.bottom);

    g_ts.arStuckRects[g_ts.uStuckPlace] = rc;

    if (fHiding) {
        TrayShowWindow(SW_HIDE);
    }

    if ((g_ts.uAutoHide & (AH_ON | AH_HIDING)) != (AH_ON | AH_HIDING))
    {
        g_ts.fSelfSizing = TRUE;
        SetWindowPos(g_ts.hwndMain, NULL,
            rc.left, rc.top,
            RECTWIDTH(rc),RECTHEIGHT(rc),
            SWP_NOZORDER | SWP_NOACTIVATE);

        g_ts.fSelfSizing = FALSE;

        // during 'bottom up' resizes (e.g. isfband View.Large), we don't
        // get WM_ENTERSIZEMOVE/WM_EXITSIZEMOVE.  so we send it here.
        // this fixes two bugs:
        // - nt5:168643: btm-of-screen on-top tray mmon clipping not updated
        // after view.large
        // - nt5:175287: top-of-screen on-top tray doesn't resize workarea
        // (obscuring top of 'my computer' icon) after view.large
        if (!g_fInSizeMove)
            SendMessage(v_hwndTray, WM_EXITSIZEMOVE, 0, 0);
    }
    else
    {
        ASSERT(0);
        Tray_VerifySize(TRUE);
    }

    if (fHiding) {
        TrayShowWindow(SW_SHOWNA);
    }

    return S_OK;
}
