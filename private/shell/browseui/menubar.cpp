#include "priv.h"
#include "sccls.h"
#include "basebar.h"
#include "bands.h"
#include "multimon.h"
#include "menubar.h"
#include "menuband.h"           // for CMenuBand_Create
#include "isfband.h"

#include "apithk.h"
// BUGBUG (lamadio): Conflicts with one defined in winuserp.h
#undef WINEVENT_VALID       //It's tripping on this...
#include "winable.h"
#include "oleacc.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

#define THISCLASS CMenuDeskBar
#define SUPERCLASS CBaseBar


// Don't fade the menu if it's larger than this magical number. Based on experiments
// on a Pentium II - 233
#define MAGICAL_NO_FADE_HEIGHT  600

// For TraceMsg
#define DM_POPUP   DM_TRACE

#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

#ifdef ENABLE_CHANNELS
IDeskBand * ChannelBand_Create(LPCITEMIDLIST pidl);
#endif  // ENABLE_CHANNELS

// Used by performance timing mode
extern DWORD g_dwStopWatchMode;  // Shell performance mode
extern HMENU g_hmenuStopWatch;
extern UINT g_idCmdStopWatch;

HRESULT CMenuDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CMenuDeskBar *pwbar = new CMenuDeskBar();
    if (pwbar) {
        *ppunk = SAFECAST(pwbar, IMenuPopup*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


CMenuDeskBar::CMenuDeskBar() : SUPERCLASS()
{
    _dwMode = DBIF_VIEWMODE_VERTICAL; 
    
    _iIconSize = BMICON_SMALL;
}

CMenuDeskBar::~CMenuDeskBar()
{
    SetSite(NULL);
}


STDMETHODIMP CMenuDeskBar::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(THISCLASS, IMenuPopup),
        QITABENT(THISCLASS, IObjectWithSite),
        QITABENT(THISCLASS, IBanneredBar),
        QITABENT(THISCLASS, IInitializeObject),
        { 0 },
    };

    hres = QISearch(this, (LPCQITAB)qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);

    return hres;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::SetSubmenu method

*/
STDMETHODIMP CMenuDeskBar::SetSubMenu(IMenuPopup* pmp, BOOL fSet)
{
    if (fSet) {
        if (_pmpChild)
            _pmpChild->Release();
        
        _pmpChild = pmp;
        _pmpChild->AddRef();
        
    } else {
        
        if (_pmpChild && SHIsSameObject(pmp, _pmpChild)) {
            _pmpChild->Release();
            _pmpChild = NULL;
        }
    }
    return S_OK;
}


void CMenuDeskBar::_PopDown()
{
    DAD_ShowDragImage(FALSE);
    if (_pmpChild)
        _pmpChild->OnSelect(MPOS_CANCELLEVEL);
    
//    ShowWindow(_hwnd, SW_HIDE);
    SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
    ShowDW(FALSE);
    if (_pmpParent) {
        _pmpParent->SetSubMenu(this, FALSE);
    }
    UIActivateIO(FALSE, NULL);
    _fActive = FALSE;
    DAD_ShowDragImage(TRUE);
}


/*----------------------------------------------------------
Purpose: IMenuPopup::OnSelect method

*/
STDMETHODIMP CMenuDeskBar::OnSelect(DWORD dwSelectType)
{
    switch (dwSelectType)
    {
    case MPOS_CHILDTRACKING:
        if (_pmpParent)
            _pmpParent->OnSelect(dwSelectType);
        break;
        
    case MPOS_SELECTRIGHT:
    case MPOS_SELECTLEFT:
        if (_pmpParent)
            _pmpParent->OnSelect(dwSelectType);
        break;

    case MPOS_EXECUTE:
    case MPOS_FULLCANCEL:
        _PopDown();
        if (_pmpParent)
            _pmpParent->OnSelect(dwSelectType);
        break;

    case MPOS_CANCELLEVEL:
        _PopDown();
        break;
        
    }
    
    return S_OK;
} 

void SetExpandedBorder(HWND hwnd, BOOL fExpanded)
{

#ifdef MAINWIN
    // IEUNIX : WS_DLGFRAME implementaion looks ugly on UNIX.
    fExpanded = TRUE;
#endif

    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (fExpanded)
    {
        dwStyle |= WS_BORDER;
        dwStyle &= ~WS_DLGFRAME;
    }
    else
    {
        dwStyle &= ~WS_BORDER;
        dwStyle |= WS_DLGFRAME;
    }

    SetWindowLong(hwnd, GWL_STYLE, dwStyle);
    SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyle);

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, 
        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    UpdateWindow(hwnd);
}

void CMenuDeskBar::_OnCreate()
{
    SetExpandedBorder(_hwnd, _fExpanded);
}


DWORD CMenuDeskBar::_GetClassStyle()
{
    // Faster repaint for menus when they go away
    return CS_SAVEBITS;
}


DWORD CMenuDeskBar::_GetExStyle()
{
#ifndef MAINWIN
    return WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
#else
    return WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_MW_UNMANAGED_WINDOW;
#endif
}

// We use the following structure to pass a whole bunch of information from 
// the GetPopupWindowPosition to WillFit function. We have WillFit function 
// to cut the amount of duplicate code in getpopup window position. The order 
// in which different the sides are checked is the only difference for popping 
// up a window on a particular side.
//
// Having this function helps us to do that check by means of a parameter instead 
// of repeating portions of code again and again.

typedef struct  {
    RECT rcAvail;           // Available dimensions b/t monitor edge and exclude edge
    SIZE sizeAdjust;          // Size of menu edge
    int  cyMonitor;         // Size of monitor 
    int  cxMonitor;
    int  cx;                // Size of menu
    int  cy;
    int  cyExtendDiff;      // Difference b/t calc'd size and available size
    RECT *prcResult;
    RECT *prcExclude;       // Exclude rect
    RECT *prcMonitor;
} PopupInfo;

#define TOP     0
#define BOTTOM  1
#define LEFT    2
#define RIGHT   3 

/*----------------------------------------------------------
Purpose: Attempt to fit and position a menu in the given direction
         relative to an exclude rect.

         Setting fForce to TRUE will cause the menu size to be adjusted
         to fit, if necessary.

         This function only sets the top and left coords, not the bottom
         and right coords.
         
         Returns TRUE if the desired direction can be accomplished.

*/
BOOL WillFit(PopupInfo * pinfo, int side, BOOL fForce)
{
    BOOL bRet = FALSE;
    LPRECT prcResult = pinfo->prcResult;
    
    pinfo->cyExtendDiff = 0;
    
    switch(side)
    {
    case TOP:
        pinfo->cyExtendDiff = pinfo->cy - pinfo->rcAvail.top;
        if (fForce)
        {
            // Doesn't make sense to subtract a negative value
            ASSERT(pinfo->cyExtendDiff >= 0);    

            // +2 for some breathing room at the edge of the screen
            pinfo->cy -= pinfo->cyExtendDiff + 2;    
        }

        // Can the menu be positioned above?
        if (pinfo->cy <= pinfo->rcAvail.top)
        {
            // Yes
            prcResult->top  = pinfo->prcExclude->top - pinfo->cy;
            
            goto AdjustHorzPos;
        }
        break;
        
    case BOTTOM:
        pinfo->cyExtendDiff = pinfo->cy - pinfo->rcAvail.bottom;
        if (fForce)
        {
            // Doesn't make sense to subtract a negative value
            ASSERT(pinfo->cyExtendDiff >= 0);    
            
            // +2 for some breathing room at the edge of the screen
            pinfo->cy -= pinfo->cyExtendDiff + 2;
        }

        // Can the menu be positioned below?
        if (pinfo->cy <= pinfo->rcAvail.bottom)
        {
            // Yes
            prcResult->top = pinfo->prcExclude->bottom;

AdjustHorzPos:            
            prcResult->left = max(pinfo->prcExclude->left, pinfo->prcMonitor->left);

            // Can the menu be positioned relative to its left edge (hanging right)?
            if (prcResult->left + pinfo->cx >= pinfo->prcMonitor->right)
            {
                // No; move it in so it is on the screen
                //  (cx has already been adjusted to fit inside the monitor dimensions)
                prcResult->left = pinfo->prcMonitor->right - pinfo->cx - 1;
            }
            bRet = TRUE;
        }
        break;
        
    case LEFT:
        // Can the menu be positioned to the left?
        if (pinfo->cx <= pinfo->rcAvail.left || fForce)
        {
            // Yes
            
            // When cascading left, the menu does not overlap.  Also align
            // so the client rect is vertically aligned with the exclude top.
            prcResult->left = pinfo->prcExclude->left - pinfo->cx - 1;

            goto AdjustVerticalPos;
        }
        break;

    case RIGHT:
        // Can the menu be positioned to the right?
        if (pinfo->cx  <=  pinfo->rcAvail.right || fForce)
        {
            // Yes
            
            // Adjust the menu to slightly overlap the parent menu.  Also align
            // so the client rect is vertically aligned with the exclude top.
            prcResult->left = pinfo->prcExclude->right - pinfo->sizeAdjust.cx;

AdjustVerticalPos:            
            prcResult->top = pinfo->prcExclude->top - pinfo->sizeAdjust.cy;

            // Can the menu be positioned relative to its top edge (hanging down)?
            if (prcResult->top + pinfo->cy >= pinfo->prcMonitor->bottom)
            {
                // No; can it be positioned relative to its bottom edge (hanging up)?
                prcResult->top = pinfo->prcExclude->bottom + pinfo->sizeAdjust.cy - pinfo->cy;
                
                if (prcResult->top < pinfo->prcMonitor->top)
                {
                    // No; move the menu so it fits, but isn't vertically snapped.
                    //  (cy has already been adjusted to fit inside the monitor
                    //  dimensions)
                    prcResult->top = pinfo->prcMonitor->bottom - pinfo->cy - 1;
                }
            }
            
            bRet = TRUE;

        }
        break;
    }
    return bRet;

}


void CMenuDeskBar::_GetPopupWindowPosition(RECT* prcDesired, RECT* prcExclude, 
                                           RECT *prcResult, SIZE * psizeAdjust, UINT uSide) 
{
    PopupInfo info;
    MONITORINFO mi;
    HMONITOR hMonitor;
    RECT rcMonitor;
    int cyExtendDiff = 0;

    // Is this going to display the banner bitmap?
    if (_iIconSize == BMICON_LARGE)
    {
        // Yes; add that to the dimensions
        prcDesired->right += _sizeBmp.cx;
    }

    // First get the monitor information
    hMonitor = MonitorFromRect(prcExclude, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    rcMonitor = mi.rcMonitor;

    // Set the result rectangle same as the desired window
    prcResult->left = prcDesired->left;
    prcResult->top  = prcDesired->top;

    // Calculate some sizes needed for calculation
    info.rcAvail.left   = prcExclude->left - rcMonitor.left;
    info.rcAvail.right  = rcMonitor.right - prcExclude->right;
    info.rcAvail.top    = prcExclude->top - rcMonitor.top;
    info.rcAvail.bottom = rcMonitor.bottom - prcExclude->bottom;

    info.sizeAdjust = *psizeAdjust;
    
    info.cyMonitor = RECTHEIGHT(rcMonitor); 
    info.cxMonitor = RECTWIDTH(rcMonitor); 

    info.cx  = RECTWIDTH(*prcDesired);
    info.cy = RECTHEIGHT(*prcDesired);

    // If the desired rect is bigger than monitor then clip it to the monitor size
    if (info.cy > info.cyMonitor)
        info.cy = info.cyMonitor;

    if (info.cx > info.cxMonitor)
        info.cx = info.cxMonitor;

    info.prcResult = prcResult;
    info.prcExclude = prcExclude;
    info.prcMonitor = &rcMonitor;

    //Now Adjust the rectangle for the correct position
    switch(uSide)
    {
    int iSide;

    case MENUBAR_TOP:
    
        if (WillFit(&info, TOP, FALSE))
        {
            _uSide = MENUBAR_TOP;
        }
        else 
        {
            // We couldn't fit it above, how badly did we fall short?
            cyExtendDiff = info.cyExtendDiff;
            if (WillFit(&info, BOTTOM, FALSE))
                _uSide = MENUBAR_BOTTOM;
            // We can't fit it below either, which dir was closest?
            // If they are equal, default to requested direction
            else if (info.cyExtendDiff < cyExtendDiff)
            {
                _uSide = MENUBAR_BOTTOM;
                WillFit(&info, BOTTOM, TRUE);
            }
            else
            {
                _uSide = MENUBAR_TOP;
                WillFit(&info, TOP, TRUE);
            }
        }
        break;

    case MENUBAR_BOTTOM:
    
        if (WillFit(&info, BOTTOM, FALSE))
        {
            _uSide = MENUBAR_BOTTOM;
        }
        else
        {   
            // We couldn't fit it below, how badly did we fall short?
            cyExtendDiff = info.cyExtendDiff;
            if (WillFit(&info, TOP, FALSE))
                _uSide = MENUBAR_TOP;

            // We can't fit it above either, which dir was closest?
            // If they are equal, default to requested direction
            else if (info.cyExtendDiff < cyExtendDiff)
            {
                _uSide = MENUBAR_TOP;
                WillFit(&info, TOP, TRUE);
            }
            else
            {
                _uSide = MENUBAR_BOTTOM;
                WillFit(&info, BOTTOM, TRUE);
            }
        }
        break;

    case MENUBAR_LEFT:

        if (WillFit(&info, LEFT, FALSE))
        {
            _uSide = MENUBAR_LEFT;
        }else if (WillFit(&info, RIGHT, FALSE))
        {
            _uSide = MENUBAR_RIGHT;
        }else {
            // fit where have most room and can show most of menu.

            if ((info.cx - (info.prcExclude)->right) > (info.prcExclude)->left)
            {
                _uSide = MENUBAR_RIGHT;
                iSide = RIGHT;
            }
            else
            {
                _uSide = MENUBAR_LEFT;
                iSide = LEFT;
            }
            WillFit(&info, iSide, TRUE);
        }
        break;

    case MENUBAR_RIGHT:

        if (WillFit(&info, RIGHT, FALSE))
        {
            _uSide = MENUBAR_RIGHT;
        }else if (WillFit(&info, LEFT, FALSE))
        {
            _uSide = MENUBAR_LEFT;
        }else {
            // fit where have most room and can show most of menu.

            if ((info.cx - (info.prcExclude)->right) >= (info.prcExclude)->left)
            {
                _uSide = MENUBAR_RIGHT;
                iSide = RIGHT;
            }
            else
            {
                _uSide = MENUBAR_LEFT;
                iSide = LEFT;
            }
            WillFit(&info, iSide, TRUE);
        }
        break;
    }
    
    // Finally set the bottom and right

    if (prcResult->top < rcMonitor.top)
        prcResult->top = rcMonitor.top;
    if (prcResult->left < rcMonitor.left)
        prcResult->left = rcMonitor.left;

    prcResult->bottom = prcResult->top  + info.cy;
    prcResult->right  = prcResult->left + info.cx;

    if (prcResult->bottom > rcMonitor.bottom)
    {
        // -2 for some breathing room at the edge of the screen
        prcResult->bottom = rcMonitor.bottom - 2;
        prcResult->top = prcResult->bottom - info.cy;
    }
}

HRESULT CMenuDeskBar::_PositionWindow(POINTL *ppt, RECTL* prcExclude, DWORD dwFlags)
{
    ASSERT(IS_VALID_READ_PTR(ppt, POINTL));
    ASSERT(NULL == prcExclude || IS_VALID_READ_PTR(prcExclude, RECTL));

    BOOL bSetFocus = (dwFlags & MPPF_SETFOCUS);
    RECT rcDesired;
    RECT rcExclude;
    RECT rc;
    SIZE sizeAdjust;
    UINT uAnimateSide;

    BOOL bMirroredWindow=IS_WINDOW_RTL_MIRRORED(_hwnd);

    static const iPosition[] = {MENUBAR_TOP, MENUBAR_LEFT, MENUBAR_RIGHT, MENUBAR_BOTTOM};

    if (dwFlags & MPPF_POS_MASK)
    {
        UINT uPosIndex = ((dwFlags & MPPF_POS_MASK) >> 29) - 1;
        ASSERT(uPosIndex < 4);
        _uSide = iPosition[uPosIndex];
    }

    if (bSetFocus)
        SetForegroundWindow(_hwnd);
    
    _pt = *(POINT*)ppt;

    // Get the size of the ideal client rect of the child
    RECT rcChild = {0};

    // BUGBUG (scotth): This only sets the bottom and the right values
    _pDBC->GetSize(DBC_GS_IDEAL, &rcChild);

    DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);

    // Adjust for the window border style
    rcDesired = rcChild;        // use rcDesired as a temp variable
    AdjustWindowRectEx(&rcChild, dwStyle, FALSE, dwExStyle);

    // Calculate the edge of the menu border, and add a fudge factor so
    // left/right-cascaded menus overlap the parent menu a bit and are
    // correctly aligned vertically.

    sizeAdjust.cx = (RECTWIDTH(rcChild) - RECTWIDTH(rcDesired)) / 2;
    sizeAdjust.cy = (RECTHEIGHT(rcChild) - RECTHEIGHT(rcDesired)) / 2;

    if (prcExclude)
    {
        CopyRect(&rcExclude, (RECT*)prcExclude);

        //
        // If mirroring is enabled, let's mirror this guy
        // by simulating a different mirrored rect. This is
        // only for dropdown menus. [samera]
        //  
        if (bMirroredWindow)           
        {
            if ((_uSide != MENUBAR_LEFT)    &&
                (_uSide != MENUBAR_RIGHT) )
            {  
                int x;
                int iW  = rcExclude.right-rcExclude.left;
                int icW = (rcChild.right-rcChild.left);


                if( icW > iW )
                {
                    x = icW - iW;
                    rcExclude.left  -= x ;
                    rcExclude.right -= x ;
                }
                else
                {
                    x = iW - icW;
                    rcExclude.left  += x;
                    rcExclude.right += x;
                }

                ppt->x = rcExclude.left;
            }

        }

        TraceMsg(DM_POPUP, "Parent Side is %d ", _uSide);
        switch(_uSide) 
        {
        case MENUBAR_LEFT :
            rcDesired.left = rcExclude.left - rcChild.right;  // right is width
            rcDesired.top  = rcExclude.top;
            break;

        case MENUBAR_RIGHT :
            rcDesired.left = rcExclude.right;
            rcDesired.top  = rcExclude.top;
            break;
            
        case MENUBAR_TOP:
            rcDesired.left = rcExclude.left;
            rcDesired.top  = rcExclude.top - rcChild.bottom;  // bottom is height
            break;

        case MENUBAR_BOTTOM:
            rcDesired.left = rcExclude.left;
            rcDesired.top  = rcExclude.bottom;
            break;

        default:
            rcDesired.left   = _pt.x;
            rcDesired.top    = _pt.y;
        }
    }
    else
    {
        SetRectEmpty(&rcExclude);

        rcDesired.left   = _pt.x;
        rcDesired.top    = _pt.y;
    }

    rcDesired.right  =  rcDesired.left + RECTWIDTH(rcChild);
    rcDesired.bottom =  rcDesired.top + RECTHEIGHT(rcChild);

    _GetPopupWindowPosition(&rcDesired, &rcExclude, &rc, &sizeAdjust, _uSide);

    UINT uFlags = 0;
    if (!bSetFocus)
        uFlags |= SWP_NOACTIVATE;

    //
    // Open the menus properly. In case of a RTL mirrored window,
    // flip the animation side. [samera]
    //
    if( bMirroredWindow )
    {
        switch( _uSide )
        {
        case MENUBAR_LEFT:
            uAnimateSide = MENUBAR_RIGHT;
        break;
        case MENUBAR_RIGHT:
            uAnimateSide = MENUBAR_LEFT;
        break;
        default:
            uAnimateSide = _uSide;
        }
    }
    else
    {
        uAnimateSide = _uSide;
    }

    TraceMsg(TF_MENUBAND, "CMenuBar::_PositionWindow (%d,%d,%d,%d)",
        rc.left, rc.top, rc.right, rc.bottom);

    // Last minuite tweak. Since we're in large icon, we need to add this
    // so that the bitmap is painted correctly.
    if(_iIconSize == BMICON_LARGE && _fExpanded)
        rc.right += 1;

    // We _DO_ want to do a Z-Order position when this flag is specified. This is
    // for full repositioning where we need to preserve the overlap state of all bands.
    // Otherwize we just want to size the bar without changing it's z-order.
    if (!(dwFlags & MPPF_FORCEZORDER) && 
        (S_OK == IUnknown_QueryServiceExec(_punkChild, SID_SMenuBandChild,
         &CGID_MenuBand, MBANDCID_ISINSUBMENU, 0, NULL, NULL)))
    {
        uFlags |= SWP_NOZORDER;
    }

    // If it's bigger than this magical number, then we don't animate. change to taste
  
    if (RECTHEIGHT(rc) > MAGICAL_NO_FADE_HEIGHT)
        dwFlags |= MPPF_NOANIMATE;

    AnimateSetMenuPos(_hwnd, &rc, uFlags, uAnimateSide, dwFlags & MPPF_NOANIMATE);

    // Save information so we can later resize this window
    // We already have: _pt, _uSide
    if (prcExclude)
    {
        _fExcludeRect = TRUE;
        CopyRect(&_rcExclude, (RECT*)prcExclude);
    }
    else
        _fExcludeRect = FALSE;
    return S_OK;
} 

/*----------------------------------------------------------
Purpose: IMenuPopup::Popup method

*/
STDMETHODIMP CMenuDeskBar::Popup(POINTL* ppt, RECTL* prcExclude, DWORD dwFlags)
{
    HRESULT hres;

    // Is the caller telling us to reposition?
    if (dwFlags & MPPF_REPOSITION)
    {
        if (ppt == NULL)
            ppt = (POINTL*)&_pt;

        if (prcExclude == NULL)
            prcExclude = (RECTL*)&_rcExclude;

        // Yes; Then we don't need to do any First show stuff.
        _PositionWindow(ppt, prcExclude, dwFlags);
        return S_OK;
    }

    ASSERT(IS_VALID_READ_PTR(ppt, POINTL));
    ASSERT(NULL == prcExclude || IS_VALID_READ_PTR(prcExclude, RECTL));



    if (g_dwProfileCAP & 0x00002000) 
        StartCAP();

    if (g_dwStopWatchMode)
        StopWatch_Start(SWID_MENU, TEXT("Menu Start"), SPMODE_SHELL | SPMODE_DEBUGOUT);
    
    if (_pmpParent) 
    {
        _pmpParent->SetSubMenu(this, TRUE);
    }

    IOleCommandTarget* poct;
    hres = IUnknown_QueryService(_punkChild, SID_SMenuBandChild, IID_IOleCommandTarget, 
                                 (LPVOID *)&poct);

    if (SUCCEEDED(hres))
    {
        // We need to do this before the ShowDW. This saves us from doing the setting twice
        // Because in the ShowDW of MenuBand, we actually go an initialize the toolbar with
        // the current default setting which should be "No Keyboard Cues." If we set the state
        // here, then the state will be "Show keyboard cues." Then we will update the toolbar. 
        if (dwFlags & MPPF_KEYBOARD)
            poct->Exec(&CGID_MenuBand, MBANDCID_KEYBOARD, 0, NULL, NULL);
    }
    else
    {
        ASSERT(poct == NULL);
    }
    
    _NotifyModeChange(_dwMode);
    ShowDW(TRUE);

    if(_pmpParent) {
        VARIANT varg;
        hres = IUnknown_Exec(_pmpParent, &CGID_MENUDESKBAR, MBCID_GETSIDE, 0, NULL, &varg);
        if(SUCCEEDED(hres)) {

            if(varg.vt ==  VT_I4) {
                _uSide = (UINT) varg.lVal;
            }
        }
    }

    IEPlaySound(TEXT("MenuPopup"), TRUE);

    _PositionWindow(ppt, prcExclude, dwFlags);

    // Set focus
    UIActivateIO(TRUE, NULL);
    
    _fActive = TRUE;

    // Select the first/last item?
    if ((dwFlags & (MPPF_INITIALSELECT | MPPF_FINALSELECT)) && poct)
    {
        DWORD nCmd = (dwFlags & MPPF_INITIALSELECT) ? MBSI_FIRSTITEM : MBSI_LASTITEM;
        poct->Exec(&CGID_MenuBand, MBANDCID_SELECTITEM, nCmd, NULL, NULL);
    }

    ATOMICRELEASE(poct);
    
    if (g_dwStopWatchMode)
    {
        TCHAR szMenu[32];
        TCHAR szText[256];

        *szMenu = '\0';
        if(g_hmenuStopWatch != NULL)
            GetMenuString(g_hmenuStopWatch, 0, szMenu, ARRAYSIZE(szMenu)-1, MF_BYPOSITION);

        wnsprintf(szText, ARRAYSIZE(szText) - 1, TEXT("Menu %d %s%sStop"), g_idCmdStopWatch, szMenu, *szMenu ?TEXT(" ") :TEXT(""));
        StopWatch_Stop(SWID_MENU, (LPCTSTR)szText, SPMODE_SHELL | SPMODE_DEBUGOUT);
    }

    if (g_dwProfileCAP & 0x00002000) 
        StopCAP();
        
    return S_OK;
} 


/*----------------------------------------------------------
Purpose: IInputObjectSite::OnFocusChangeIS

Returns: 
Cond:    --
*/
HRESULT CMenuDeskBar::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IObjectWithSite::SetSite method

*/
STDMETHODIMP CMenuDeskBar::SetSite(IUnknown* punkSite)
{
    ASSERT(NULL == punkSite || IS_VALID_CODE_PTR(punkSite, IUnknown));

    if (_fShow)
        _PopDown();

    ATOMICRELEASE(_punkSite);
    ATOMICRELEASE(_pmpParent);
    
    _punkSite = punkSite;
    
    if (_punkSite) {
        
        _punkSite->AddRef();
        IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_IMenuPopup, (LPVOID*)&_pmpParent);

    } else {
        CloseDW(0);
    }
        
    return S_OK;
} 


/*----------------------------------------------------------
Purpose: IObjectWithSite::GetSite method

*/
STDMETHODIMP CMenuDeskBar::GetSite(REFIID riid, LPVOID* ppvSite)
{
    if (_punkSite)
    {
        return _punkSite->QueryInterface(riid, ppvSite);
    }

    *ppvSite = NULL;
    return E_FAIL;
} 


/*----------------------------------------------------------
Purpose: IOleCommandTarget::Exec method

*/
STDMETHODIMP CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL) 
    {
        
    } 
    else if (IsEqualGUID(CGID_DeskBarClient, *pguidCmdGroup))
    {
        switch (nCmdID) {
        case DBCID_EMPTY:
            // if we have no bands left, close
            OnSelect(MPOS_FULLCANCEL);
            return S_OK;

        default:
            return OLECMDERR_E_NOTSUPPORTED;
        }
    }

    else if (IsEqualGUID(CGID_MENUDESKBAR, *pguidCmdGroup))
    {
        switch(nCmdID) {
            case MBCID_GETSIDE :
                pvarargOut->vt = VT_I4;
                pvarargOut->lVal = _GetSide();
                return S_OK;

            case MBCID_RESIZE:
                if (_fActive)
                {
                    if (_fExcludeRect)
                        _PositionWindow((POINTL *)&_pt, (RECTL *)&_rcExclude, 0);
                    else
                        _PositionWindow((POINTL *)&_pt, NULL, 0);
                }
                return S_OK;

            case MBCID_SETEXPAND:
                if ((BOOL)_fExpanded != (BOOL)nCmdexecopt)
                {
                    _fExpanded = nCmdexecopt;

                    SetExpandedBorder(_hwnd, _fExpanded);
                }
                return S_OK;

            default : 
                return OLECMDERR_E_NOTSUPPORTED;

        }   
    }
    
    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}    

    
/*----------------------------------------------------------
Purpose: IServiceProvider::QueryService method

*/
STDMETHODIMP CMenuDeskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService, SID_SMenuPopup)) 
    {
        return QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(guidService, SID_SMenuBandBottom) ||
             IsEqualIID(guidService, SID_SMenuBandBottomSelected)||
             IsEqualIID(guidService, SID_SMenuBandChild))
    {
        // SID_SMenuBandBottom queries down
        return IUnknown_QueryService(_punkChild, guidService, riid, ppvObj);
    }
    else
    {
        HRESULT hres = SUPERCLASS::QueryService(guidService, riid, ppvObj);
        
        if (FAILED(hres)) {
            hres = IUnknown_QueryService(_punkSite, guidService, riid, ppvObj);
        }
        
        return hres;
    }
        
} 

/*----------------------------------------------------------
Purpose: IServiceProvider::QueryService method

*/
STDMETHODIMP CMenuDeskBar::SetIconSize(DWORD iIcon)
{
    HRESULT hres;

    _iIconSize = iIcon;

    hres = IUnknown_QueryServiceExec(_punkChild, SID_SMenuBandChild, &CGID_MenuBand, 
        MBANDCID_SETICONSIZE, iIcon == BMICON_SMALL? ISFBVIEWMODE_SMALLICONS: ISFBVIEWMODE_LARGEICONS, NULL, NULL);


    return hres;
}

/*----------------------------------------------------------
Purpose: IServiceProvider::QueryService method

*/
STDMETHODIMP CMenuDeskBar::SetBitmap(HBITMAP hBitmap)
{
    ASSERT(hBitmap);
    BITMAP bm;
    _hbmp = hBitmap;

    if (_hbmp)
    {
        if(!GetObject(_hbmp, sizeof(bm), &bm))
            return E_FAIL;
        _sizeBmp.cx = bm.bmWidth;
        _sizeBmp.cy = bm.bmHeight;

        // Hack to get color
        HDC hdc = GetDC(_hwnd);
        if (hdc)
        {
            HDC hdcMem = CreateCompatibleDC(hdc);
            if (hdcMem)
            {
                HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, _hbmp);
                _rgb = GetPixel(hdcMem, 0, 0);
                SelectObject(hdcMem, hbmpOld);
                DeleteDC(hdcMem);
            }
            ReleaseDC(_hwnd, hdc);
        }
    }

    return NOERROR;
}

void CMenuDeskBar::_OnSize()
{
    RECT rc;

    if (!_hwndChild)
        return;

    GetClientRect(_hwnd, &rc);
    if(_iIconSize == BMICON_LARGE)
    {
        rc.left += _sizeBmp.cx;
        if (_fExpanded)
            rc.left++;
    }

    SetWindowPos(_hwndChild, 0,
            rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_FRAMECHANGED);

    rc.right = rc.left;
    rc.left -= _sizeBmp.cx;
    if (_fShow)
        InvalidateRect(_hwnd, &rc, TRUE);
}

LRESULT CMenuDeskBar::_DoPaint(HWND hwnd, HDC hdc, DWORD dwFlags)
{
    HDC hdcmem;
    HBITMAP hbmpOld;
    RECT rc;
    HBRUSH   hbrush;
    int iDC = SaveDC(hdc);

    GetClientRect(hwnd, &rc);

    //Create a compatable DC
    hdcmem = CreateCompatibleDC(hdc);
    if(hdcmem)
    {
        // Offset the stuff we're paining if we're expanded
        BYTE bOffset = _fExpanded? 1 : 0;

        // Store this for the Bar fill cycle
        int cyBitmap = 0;

        if (_sizeBmp.cy <= RECTHEIGHT(rc) + 1)
        {
            //Select the bitmap into the memory DC
            hbmpOld = (HBITMAP)SelectObject(hdcmem, _hbmp);

            //Blit to the window
            BitBlt(hdc, bOffset, rc.bottom - _sizeBmp.cy - bOffset, _sizeBmp.cx, _sizeBmp.cy, hdcmem, 0, 0, SRCCOPY);

            // Ok, We need to subtract this value to see how much we need to paint for the banner.
            cyBitmap = _sizeBmp.cy;

            //Restore the DC
            SelectObject(hdcmem, hbmpOld);
        }

        rc.right = _sizeBmp.cx + bOffset;

        if (_fExpanded)
            DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_LEFT | BF_TOP | BF_BOTTOM);

        //Paint the rest
        hbrush = CreateSolidBrush(_rgb);
        if(hbrush)
        {
            rc.bottom -= cyBitmap + bOffset;

            if (_fExpanded)
            {
                rc.left += bOffset;
                rc.top += bOffset;
            }

            FillRect(hdc, &rc, hbrush);
            DeleteObject(hbrush);
        }


        //Delete the DC.
        DeleteDC(hdcmem);
    }

    RestoreDC(hdc, iDC);
    return 0;
}

void CMenuDeskBar::_DoNCPaint(HWND hwnd, HDC hdc)
{
    RECT rc;
    // Since we need to paint the border, we get the window rect
    GetWindowRect(hwnd, &rc);
    // then change the rect so that it represents values relative to 
    // the origin.
    OffsetRect(&rc, -rc.left, -rc.top);

    if (hdc)
    {
        DrawEdge(hdc, &rc, BDR_RAISEDOUTER, BF_RECT);
    }
}


LRESULT CMenuDeskBar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    
    LRESULT lres;

    switch (uMsg) 
    {
#ifdef MAINWIN
    case WM_NCPAINTSPECIALFRAME:
        // In  case  of  motif look  the  MwPaintBorder paints a Etched In
        // border if WM_NCPAINTSPECIALFRAME returns FALSE. We are handling
        // this message here and drawing the Etched Out frame explicitly.
        // wParam - HDC
        if (MwCurrentLook() == LOOK_MOTIF)
        {
            MwPaintSpecialEOBorder( hwnd, (HDC)wParam );
            return TRUE;
        }
        break;
#endif

    case WM_GETOBJECT:
        if (lParam == OBJID_MENU)
        {
            IAccessible* pacc;
            if (SUCCEEDED(QueryService(SID_SMenuBandChild, IID_IAccessible, (void**)&pacc)))
            {
                lres = LresultFromObject(IID_IAccessible, wParam, SAFECAST(pacc, IAccessible*));
                pacc->Release();

                return lres;
            }
        }
        break;


    case WM_NCCREATE:
        //
        // Since this is a mirrored menu, then open it
        // on the left (mirrored) edge if possible. WillFit(...) will
        // ensure this for us [samera]
        //
        // Mirror the menu initially if its window is mirrored
        //
        ASSERT(_uSide == 0);
        if (IS_WINDOW_RTL_MIRRORED(_hwnd))
            _uSide = MENUBAR_LEFT;
        else
            _uSide = MENUBAR_RIGHT;
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) 
        {
            if (_fActive && !_pmpChild) 
            {
                
                // if we were active, and the thing going active now
                // is not one of our parent menubars, then cancel everything.
                
                // if it's a parent of ours going active, assume that
                // they will tell us to die when they want us to...
                if (!_IsMyParent((HWND)lParam))
                    OnSelect(MPOS_FULLCANCEL);
            }
        } 
        else 
        {
            if (_pmpChild) 
            {
                // if we're becoming active, and we have a child, that child should go away
                _pmpChild->OnSelect(MPOS_CANCELLEVEL);
            }
        }
        break;

    case WM_PRINTCLIENT:
        if (_iIconSize == BMICON_LARGE)
        {
            _DoPaint(hwnd, (HDC)wParam, (DWORD)lParam);
            return 0;
        }
        break;

    case WM_PAINT:
        // Paint the banner if we're in showing large icons
        if (_iIconSize == BMICON_LARGE)
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            _DoPaint(hwnd, ps.hdc, 0);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

   case WM_PRINT:
        if (_fExpanded && PRF_NONCLIENT & lParam)
        {
            HDC hdc = (HDC)wParam;

            DefWindowProcWrap(hwnd, WM_PRINT, wParam, lParam);

            // Do this after so that we look right...
            _DoNCPaint(hwnd, hdc);

            return 1;
        }
        break;

    case WM_NCPAINT:
        if (_fExpanded)
        {    
            HDC hdc;    
            hdc = GetWindowDC(hwnd);
            _DoNCPaint(hwnd, hdc);
            ReleaseDC(hwnd, hdc);
            return 1;
        } 
        break;

    case WM_NCHITTEST:
        lres = SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);

        switch (lres)
        {
        case HTBOTTOM:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTTOPLEFT:
        case HTTOPRIGHT:
            // Don't allow the window to be resized
            lres = HTBORDER;
            break;

        case HTTRANSPARENT:
            // Don't let a click go thru to the window underneath
            lres = HTCLIENT;
            break;

        }
        return lres;

        // HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
        // (lamadio) 1.25.99
        // This hack is here to fix a problem on down level Windows with Integrated
        // IE4.01, IE5 and Office 2000.
        // The bug revolves around Start Menu not being destroyed when Explorer.exe shuts
        // down. Start Menu unregisters itself at CloseDW, but since the menubar never gets
        // destroyed, Start Menu never deregisters itself.
        // When an System service, such as MSTASK.dll keeps shell32 alive in the background,
        // it leaves an outstanding reference to a change notify. When a new user logs in,
        // O2k and IE5 fire up group conv, generating more than 10 change notify events in the
        // start menu. This causes the batching code to be fired up: Which does not really
        // work without the shell started. GroupConv also adds these events using memory 
        // alloced from it's process heap. Since there is an outstanding change notify handler
        // these pidls get forced to be handled. Shell32 then faults derefing a bad pidl.
        // By detecting an Endsession, we can eliminate this problem. Doing a SetClient(NULL)
        // cause Menubar to free it's references to MenuSite. Menusite, calls CloseDW on menuband
        // menuband then causes MNFolder to unregister itself. Since no one is listening any more
        // the crash is avoided.

    case WM_ENDSESSION:
        if (wParam != 0)
        {
            SetClient(NULL);
        }
        break;

    }
    
    return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
} 

IMenuPopup* CMenuDeskBar::_GetMenuBarParent(IUnknown* punk)
{
    IMenuPopup *pmp = NULL;
    IObjectWithSite* pows;
    punk->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pows);

    if (pows) {
        
        IServiceProvider* psp;
        pows->GetSite(IID_IServiceProvider, (LPVOID*)&psp);
        
        if (psp) {
            
            psp->QueryService(SID_SMenuPopup, IID_IMenuPopup, (LPVOID*)&pmp);
            psp->Release();
        }
        
        pows->Release();
    }
    
    return pmp;
}


// this assumes that hwnd is a toplevel window and that the menudeskbars are also 
// the only hosts and are themselves toplevel
BOOL CMenuDeskBar::_IsMyParent(HWND hwnd)
{
    BOOL fRet = FALSE;
    if (hwnd) {
        HWND hwndMenu;
        
        IMenuPopup *pmpParent = _pmpParent;
        if (pmpParent)
            pmpParent->AddRef();
        
        while (pmpParent && !fRet &&
               SUCCEEDED(IUnknown_GetWindow(pmpParent, &hwndMenu))) {
            
            if (hwndMenu == hwnd) {
                fRet = TRUE;
            }
            
            IMenuPopup* pmpNext = _GetMenuBarParent(pmpParent);
            pmpParent->Release();
            pmpParent = pmpNext;
        }
    }

    return fRet;
}

IBandSite* BandSiteFromBar(IMenuPopup* pmp)
{
    ASSERT(IS_VALID_CODE_PTR(pmp, IMenuPopup));
    IDeskBar* pdbar;
    IBandSite* pbs = NULL;
    pmp->QueryInterface(IID_IDeskBar, (LPVOID*)&pdbar);
    if (pdbar) 
    {
        IUnknown *punk;
        pdbar->GetClient(&punk);
        if (punk) 
        {
            punk->QueryInterface(IID_IBandSite, (LPVOID*)&pbs);
            ASSERT(IS_VALID_CODE_PTR(pbs, IBandSite));
            punk->Release();
        }
        pdbar->Release();
    }

    return pbs;
}

HRESULT FindBandInBandSite(IMenuPopup* pmpParent, IBandSite** ppbs, LPCITEMIDLIST pidl, REFIID riid, void** ppvOut)
{
    ASSERT(IS_VALID_CODE_PTR(pmpParent, IMenuPopup));
    ASSERT(pidl && IS_VALID_PIDL(pidl));
    ASSERT(ppvOut);
    HRESULT hres = E_FAIL;
    IBandSite* pbs;

    /* 
        the first part finds the deskband if pmpParent has it.  
        if it cannot find it, the second block creates a menu popup deskbar and bandsite.
    */
    *ppvOut = NULL;

    pbs = BandSiteFromBar(pmpParent);
    if (pbs) 
    {
        // find the deskband with pidl as it's content
        int i = 0;  
        DWORD dwID;
        IDeskBand *pdeskband;

        *ppvOut = NULL;

        while(*ppvOut && SUCCEEDED(pbs->EnumBands(i, &dwID)) &&
              SUCCEEDED(pbs->QueryBand(dwID, &pdeskband, NULL, NULL, 0))) 
        {
            CISFBand* pisfband;
            i++;

            if (SUCCEEDED(pdeskband->QueryInterface(CLSID_ISFBand, (LPVOID*)&pisfband))) 
            {
                TraceMsg(TF_MENUBAND, "FindBandInBar : Looking for (0x%x) Got (0x%x)", 
                    pidl, pisfband->_pidl);
                if (pisfband->_pidl && ILIsEqual(pidl, pisfband->_pidl)) 
                {
                    TraceMsg(TF_MENUBAND, "FindBandInBar : Found this pidl (0x%x) in"
                        " the bandsite. Using it", pidl);
                    // found it!
                    // this will end the loop
                    hres = pdeskband->QueryInterface(riid, ppvOut);
                }
                pisfband->Release();
            }
            pdeskband->Release();
        }

        // If the caller wants to know what the band site is, then let them have it.
        if (ppbs && SUCCEEDED(hres))
        {
            *ppbs = pbs;
        }
        else
            pbs->Release();
    }
    return hres;
}

HRESULT ShowBandInBandSite(IUnknown* punkBS, IUnknown* punkDB)
{
    ASSERT(IS_VALID_CODE_PTR(punkBS, IUnknown));
    ASSERT(IS_VALID_CODE_PTR(punkDB, IUnknown));
    HRESULT hres = E_FAIL;

    if (punkDB) 
    {
        // make this guy the one and only one shown
        VARIANTARG vaIn = { 0 };
        vaIn.vt = VT_UNKNOWN;
        vaIn.punkVal = punkDB;
        punkDB->AddRef();
        hres = IUnknown_Exec(punkBS, &CGID_DeskBand, DBID_SHOWONLY, OLECMDEXECOPT_PROMPTUSER, &vaIn, NULL);
        VariantClearLazy(&vaIn);
    }
    return hres;
}

IMenuPopup* CreateMenuPopup(IMenuPopup* pmpParent, IShellFolder* psf, LPCITEMIDLIST pidl, 
                            BANDINFOSFB * pbi, BOOL bMenuBand)
{
    return CreateMenuPopup2(pmpParent, NULL, psf, pidl, pbi, bMenuBand);
}

IMenuPopup* CreateMenuPopup2(IMenuPopup* pmpParent, IMenuBand* pmb, IShellFolder* psf, LPCITEMIDLIST pidl, 
                            BANDINFOSFB * pbi, BOOL bMenuBand)
{
    
    ASSERT(pmb == NULL || IS_VALID_CODE_PTR(pmb, IMenuBand));
    ASSERT(psf == NULL || IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(pmpParent == NULL || IS_VALID_CODE_PTR(pmpParent, IMenuPopup));
    ASSERT(pidl && IS_VALID_PIDL(pidl));
    ASSERT(pbi == NULL || IS_VALID_READ_PTR(pbi, BANDINFOSFB));

    IMenuPopup* pmp = NULL;
    IDeskBand *pdb = NULL;
    IBandSite *pbs = NULL;
    HRESULT hres = E_FAIL;

    // Assume that if they pass a band, that it's not in the band site.
    if (pmpParent && !pmb) 
    {
        FindBandInBandSite(pmpParent, &pbs, pidl, IID_IDeskBand, (void**)&pdb);
    }
    // Part 2: create the deskband itself if it's not found
    
    if (!pdb) 
    {
        TraceMsg(TF_MENUBAND, "CreateMenuPopup2 : Did not find a this (0x%x) band.", pidl);

        if (bMenuBand)
        {
            if (pmb)
            {
                pmb->QueryInterface(IID_IDeskBand, (void**)&pdb);
                TraceMsg(TF_MENUBAND, "CreateMenuPopup2 : I was given a band.");
            }
            else
                pdb = CMenuBand_Create(psf, pidl, FALSE);
        }
        else
            pdb = CISFBand_CreateEx(psf, pidl);

        if (pdb) 
        {
            if (pbi) 
            {
                IShellFolderBand *pisfBand;
                if (SUCCEEDED(pdb->QueryInterface(IID_IShellFolderBand, (LPVOID*)&pisfBand))) 
                {
                    pisfBand->SetBandInfoSFB(pbi);
                    pisfBand->Release();
                }
            }

            if (!pmpParent) 
            {
                const CLSID * pclsid;

                if (bMenuBand)
                    pclsid = &CLSID_MenuBandSite;
                else
                    pclsid = &CLSID_RebarBandSite;

                CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER, IID_IBandSite, (LPVOID*)&pbs);

                if (pbs) 
                {

                    if (bMenuBand)
                    {
                        BANDSITEINFO bsinfo;

                        // Don't show the gripper for vertical menubands
                        bsinfo.dwMask = BSIM_STYLE;
                        bsinfo.dwStyle = BSIS_NOGRIPPER | BSIS_NODROPTARGET;
                        pbs->SetBandSiteInfo(&bsinfo);
                    }

                    CMenuDeskBar *pcmdb = new CMenuDeskBar();
                    if (pcmdb)
                    {
                        if (SUCCEEDED(pcmdb->SetClient(pbs))) 
                            pcmdb->QueryInterface(IID_IMenuPopup, (LPVOID *)&pmp);

                        pcmdb->Release();
                    }
                }
            }
            else if (!pbs)
            {
                pbs = BandSiteFromBar(pmpParent);
            }

                 

            if (pbs) 
            {
                pbs->AddBand(pdb);
            }
        }
    }
    
    ShowBandInBandSite(pbs, pdb);

    ATOMICRELEASE(pdb);
    ATOMICRELEASE(pbs);
    if (!pmp)
        IUnknown_Set((IUnknown**) &pmp, pmpParent);
    

    return pmp;
}
