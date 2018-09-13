// DeskMovr.cpp : Implementation of CDeskMovr
#include "stdafx.h"
#pragma hdrstop

//#include "stdafx.h"

//#include <webcheck.h>

//#include "mshtmdid.h"
//#include "shlwapi.h"
//#include "util.h"
//#include "deskstat.h"
//#include "dutil.h"
//#include "DeskMovr.h"
//#include "dsubscri.h"

#include <mluisupp.h>

#ifdef POSTSPLIT

#define DEFAULT_INTERVAL        200     // check every 1/5th of a second.
#define DEFAULT_ENABLED         TRUE
#define DETECT_TIMER_ID         2323
#ifndef SHDOC401_DLL
#define ANIMATE_TIMER_ID        2324
#define ANIMATE_TIMER_INTERVAL  (60*1000)
#endif

#ifdef DEBUG
const static TCHAR sz_DM1[]  = TEXT("no dragable part");
const static TCHAR sz_DM2[]  = TEXT("caption menu button");
const static TCHAR sz_DM3[]  = TEXT("caption close button");
const static TCHAR sz_DM4[]  = TEXT("move the component");
const static TCHAR sz_DM5[]  = TEXT("resize width and height from bottom right corner");
const static TCHAR sz_DM6[]  = TEXT("resize width and height from top left corner");
const static TCHAR sz_DM7[]  = TEXT("resize width and height from top right corner");
const static TCHAR sz_DM8[]  = TEXT("resize width and height from bottom left corner");
const static TCHAR sz_DM9[]  = TEXT("resize from the top edge");
const static TCHAR sz_DM10[]  = TEXT("resize from the bottom edge");
const static TCHAR sz_DM11[]  = TEXT("resize from the left edge");
const static TCHAR sz_DM12[] = TEXT("resize from the right edge");

LPCTSTR g_szDragModeStr[] =     {
        sz_DM1,
        sz_DM2,
        sz_DM3,
        sz_DM4,
        sz_DM5,
        sz_DM6,
        sz_DM7,
        sz_DM8,
        sz_DM9,
        sz_DM10,
        sz_DM11,
        sz_DM12
    };

#endif // DEBUG

int g_nTotalActiveCaptions;   // How many captions are being displayed?

// Globals used to track cdeskmovr instances.  Useful for optimizing the
// detection code so we can turn off the timer when the mouse is not over our
// window.  We track the cdeskmovr instances only on the first thread that instantiates
// us to keep the code simple, this should be the active desktop case.
#define CDESKMOVR_TRACK_COUNT 4     // 2 is what we use now for the active desktop, but
                                    // we need twice that amount due to the fact that a new
                                    // instance is created before the old one is destroyed
                                    // during refresh.
HHOOK g_hMouseHook;
DWORD g_dwHookThreadId;
#ifndef SHDOC401_DLL
BOOL  g_fAnimTimer = FALSE;
#endif
typedef CDeskMovr *PDM;
PDM g_apDM[CDESKMOVR_TRACK_COUNT];
BOOL CombView_EnableAnimations(BOOL fEnable);


DWORD  g_fIgnoreTimers = 0;
#define IGNORE_CONTEXTMENU_UP 0x0001
#define IGNORE_CAPTURE_SET    0x0002

#define GET_SKIP_COUNT  (2 * ((GetDoubleClickTime() / m_lInterval) + 1))

MAKE_CONST_BSTR(s_sstrNameMember,       L"name");
MAKE_CONST_BSTR(s_sstrSRCMember,        L"src");
MAKE_CONST_BSTR(s_sstrHidden,           L"hidden");
MAKE_CONST_BSTR(s_sstrVisible,          L"visible");
MAKE_CONST_BSTR(s_sstrResizeableMember, L"resizeable");

// These were declared in shellprv.h, so use DEFINE instead of MAKE
DEFINE_CONST_BSTR(s_sstrIDMember,         L"id");
DEFINE_CONST_BSTR(s_sstrSubSRCMember,     L"subscribed_url");

#define CAPTION_ONLY (m_ItemState & (IS_FULLSCREEN | IS_SPLIT))
#define ISNORMAL (m_ItemState & IS_NORMAL)
#define ISFULLSCREEN (m_ItemState & IS_FULLSCREEN)
#define ISSPLIT (m_ItemState & IS_SPLIT)
#define CAPTIONBAR_HOTAREA(cyDefaultCaption, cyCurrentCaption) (((cyCurrentCaption == 0) && CAPTION_ONLY) ? 0 : 3 * cyDefaultCaption)


// Trident will flash if you change the zindex, even if it's to the same index,
// so we prevent the no-op call.
HRESULT SafeZOrderSet(IHTMLStyle * pistyle, LONG lNewZIndex)
{
    HRESULT hr = S_OK;
    VARIANT varZ;

    ASSERT(pistyle);
    pistyle->get_zIndex(&varZ);

    // Does the component need to be moved to the top?
    if ((VT_I4 != varZ.vt) || (varZ.lVal != lNewZIndex))
    {
        // Yes.
        varZ.vt = VT_I4;
        varZ.lVal = lNewZIndex;
        hr = pistyle->put_zIndex(varZ);
    }

    return hr;
}

// Helper function used to track cdeskmovr intances so that we can turn off the
// timer if the mouse leaves our window.
void TrackMover(PDM pdm, BOOL fAdd)
{
    if (!g_dwHookThreadId)
        g_dwHookThreadId = GetCurrentThreadId();

    if (g_dwHookThreadId == GetCurrentThreadId()) {
        int i = 0;
        PDM pdmFind = fAdd ? NULL : pdm;
        PDM pdmAssign = fAdd ? pdm : NULL;

        while (i < CDESKMOVR_TRACK_COUNT) {
            if (g_apDM[i] == pdmFind) {
                g_apDM[i] = pdmAssign;
                break;
            }
            i++;
        }

        // If we ever fail to track a mover then we'll never be able to optimize
        // again.  Shouldn't happen in practice for the case we care about.
        if (i >= CDESKMOVR_TRACK_COUNT)
            g_dwHookThreadId = 0xffffffff;

        ASSERT(i < CDESKMOVR_TRACK_COUNT);
    }
}

// Hook is installed when we detect we can turn our tracking timer off.  The first
// time we get a mouse event in the hook we reactivate all the movers and unhook
// ourself.
LRESULT CALLBACK DeskMovr_MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes;

#ifndef SHDOC401_DLL
    // If we are getting mouse messages then a portion of the window must be
    // visible so enable animations.
    CombView_EnableAnimations(TRUE);
#endif

    for (int i = 0; i < CDESKMOVR_TRACK_COUNT; i++) {
        if (g_apDM[i]) 
            g_apDM[i]->SmartActivateMovr(ERROR_SUCCESS);
    }

    lRes = CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);

    UnhookWindowsHookEx(g_hMouseHook);
    g_hMouseHook = NULL;

    return lRes;
}

/////////////////////////////////////////////////////////////////////////////
// CDeskMovr

CDeskMovr::CDeskMovr()
 : m_TimerWnd(_T("STATIC"), this, 1)
{
    TraceMsg(TF_CUSTOM2, "CDeskMovr::CDeskMovr()");

    m_fEnabled = DEFAULT_ENABLED;
    m_lInterval = DEFAULT_INTERVAL;

    m_cxSMBorder = GetSystemMetrics(SM_CXBORDER);
    m_cySMBorder = GetSystemMetrics(SM_CYBORDER);
    m_cxBorder = m_cxSMBorder;
    m_cyBorder = m_cySMBorder;
    m_cyCaption = 0; 

    m_dmCur = dmNull;
    m_dmTrack = dmNull;
    m_ahcursorDM[dmNull] = LoadCursor( NULL, IDC_ARROW );
    m_ahcursorDM[dmMenu] = LoadCursor( NULL, IDC_ARROW );
    m_ahcursorDM[dmClose] = LoadCursor( NULL, IDC_ARROW );
    m_ahcursorDM[dmMove] = LoadCursor( NULL, IDC_ARROW );
    m_ahcursorDM[dmSizeWHBR] = LoadCursor( NULL, IDC_SIZENWSE );
    m_ahcursorDM[dmSizeWHTL] = LoadCursor( NULL, IDC_SIZENWSE );
    m_ahcursorDM[dmSizeWHTR] = LoadCursor( NULL, IDC_SIZENESW );
    m_ahcursorDM[dmSizeWHBL] = LoadCursor( NULL, IDC_SIZENESW );
    m_ahcursorDM[dmSizeTop] = LoadCursor( NULL, IDC_SIZENS );
    m_ahcursorDM[dmSizeBottom] = LoadCursor( NULL, IDC_SIZENS );
    m_ahcursorDM[dmSizeLeft] = LoadCursor( NULL, IDC_SIZEWE );
    m_ahcursorDM[dmSizeRight] = LoadCursor( NULL, IDC_SIZEWE );

    m_hcursor = m_ahcursorDM[dmNull];
    m_CaptionState = 0;
    m_ItemState = IS_NORMAL;
    m_hwndParent;

    m_fTimer = FALSE;
    m_fCaptured = FALSE;
    m_uiTimerID = DETECT_TIMER_ID;
    m_pistyle = NULL;

    m_pistyleTarget = NULL;
    m_pielemTarget = NULL;
    m_iSrcTarget = -1;
    
    m_bstrTargetName = NULL;
    m_bstrSizrID = NULL;

    m_dx = m_dy = 0;
    
    // Since this is static, it's making the assumption that all
    // instances of this control will be created at the same time and before
    // any one of them gets a chance to become active.
    g_nTotalActiveCaptions = 0;

    m_top = m_left = m_width = m_height = 0;

    // Tell ATL that we don't want to be Windowless
    m_bWindowOnly = TRUE;

    // Track this instance
    TrackMover(this, TRUE);
}

CDeskMovr::~CDeskMovr(void)
{
    TraceMsg(TF_CUSTOM2, "CDeskMovr::~CDeskMovr() m_bstrTargetName=%ls; m_bstrSizrID=%ls.", GEN_DEBUGSTRW(m_bstrTargetName), GEN_DEBUGSTRW(m_bstrSizrID));

 // clean up, detach from events, if necessary. 
    DeactivateMovr(TRUE);

    if ( m_bstrTargetName != NULL )
        SysFreeString( m_bstrTargetName );

    if ( m_bstrSizrID != NULL )
        SysFreeString( m_bstrSizrID );

    TrackMover(this, FALSE);
}


HRESULT CDeskMovr::SmartActivateMovr(HRESULT hrPropagate)
{
    if ((FALSE == m_nFreezeEvents) && m_fEnabled)
    {
#ifndef SHDOC401_DLL
        // Release our animation timer if it exists and create our regular one
        if (g_fAnimTimer && (m_uiTimerID == ANIMATE_TIMER_ID))
        {
            m_TimerWnd.KillTimer(m_uiTimerID);
            m_uiTimerID = DETECT_TIMER_ID;
            g_fAnimTimer = FALSE;
            m_fTimer = m_TimerWnd.SetTimer(m_uiTimerID, m_lInterval, NULL) != 0;
        }
#endif
        hrPropagate = ActivateMovr();
        if (!EVAL(SUCCEEDED(hrPropagate)))
            DeactivateMovr(FALSE);   // Clean up mess.
    }

    return hrPropagate;
}

HRESULT CDeskMovr::FreezeEvents(BOOL fFreeze)
{
    HRESULT hr = IOleControlImpl<CDeskMovr>::FreezeEvents(fFreeze);
    TraceMsg(TF_CUSTOM1, "CDeskMovr::FreezeEvents(fFreeze=%lx) m_nFreezeEvents=%lx; m_fEnabled=%lx, m_bstrTargetName=%ls", (DWORD)fFreeze, m_nFreezeEvents, m_fEnabled, GEN_DEBUGSTRW(m_bstrTargetName));

    m_nFreezeEvents = fFreeze;

    if (fFreeze)
        DeactivateMovr(FALSE);
    else
        hr = SmartActivateMovr(hr);

    return hr;
}

HRESULT CDeskMovr::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    HRESULT hr;
    VARIANT var;

    ATLTRACE(_T("IPersistPropertyBagImpl::Load\n"));

    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"Enabled", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BOOL) {
        m_fEnabled = var.boolVal;
    }

    var.vt = VT_I4;
    hr = pPropBag->Read(L"Interval", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4) {
        m_lInterval = var.lVal;
    }

    var.vt = VT_BSTR;
    var.bstrVal = NULL;
    hr = pPropBag->Read(L"TargetName", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BSTR) {
        m_bstrTargetName = var.bstrVal;
    }

    // This PARAM determines whether the control will be in the
    // windowed or windowless "layer" of the Trident layout.
    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"WindowOnly", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BOOL) {
        m_bWindowOnly = var.boolVal;
    }

    hr = _GetZOrderSlot(&m_zIndexTop, TRUE);
    ASSERT(SUCCEEDED(hr));
    hr = _GetZOrderSlot(&m_zIndexBottom, FALSE);
    ASSERT(SUCCEEDED(hr));

    return hr;
}

HRESULT CDeskMovr::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return E_NOTIMPL;
}

BOOL CDeskMovr::GetCaptionButtonRect(DragMode dm, LPRECT lprc)
{
    BOOL fSuccess;

    switch (dm) {
        case dmClose:
            *lprc = m_rectCaption;
            lprc->left = m_rectCaption.right - (m_cyCaption + m_cxSMBorder);
            fSuccess = (lprc->left > (m_rectCaption.left + 2 * m_cyCaption));
            break;

        case dmMenu:
            *lprc = m_rectCaption;
            lprc->right = lprc->left + (m_cyCaption + m_cxSMBorder);
            fSuccess = (m_rectCaption.right > (m_rectCaption.left + 2 * m_cyCaption));
            break;

        default:
            fSuccess = FALSE;
            break;
    }

    // Shrink the button within the caption and position it adjacent to the border
    if (fSuccess) {
        OffsetRect(lprc, ((dm == dmClose) ? m_cxSMBorder : -m_cxSMBorder), 0);
        InflateRect(lprc, -m_cxSMBorder, -m_cySMBorder);
        lprc->bottom -= m_cySMBorder;  // Take an extra border off the bottom
    }

    return fSuccess;
}

void CDeskMovr::DrawCaptionButton(HDC hdc, LPRECT lprc, UINT uType, UINT uState, BOOL fErase)
{
    RECT rcT;
    HRGN hrgnWnd, hrgnRect;
    int iRet;

    if (fErase)
        FillRect(hdc, lprc, (HBRUSH)(COLOR_3DFACE + 1));

    rcT = *lprc;
    InflateRect(&rcT, -2*m_cxSMBorder, -2*m_cySMBorder);

    // We need to clip the border of the outer edge in order to get the drawing effect we
    // want here...
    if (hrgnWnd = CreateRectRgn(0, 0, 0, 0)) {
        if ((iRet = GetClipRgn(hdc, hrgnWnd)) != -1) {
            if (hrgnRect = CreateRectRgnIndirect(&rcT)) {
                SelectClipRgn(hdc, hrgnRect);
                DeleteObject(hrgnRect);
            }
        }
    }

    DrawFrameControl(hdc, lprc, uType, uState);

    SelectClipRgn(hdc, (iRet == 1) ? hrgnWnd : NULL);
    if (hrgnWnd)
        DeleteObject(hrgnWnd);

    // DFCS_FLAT means no border to us
    if (!(uState & DFCS_FLAT))
        DrawEdge(hdc, lprc, ((uState & DFCS_PUSHED) ? BDR_SUNKENOUTER : BDR_RAISEDINNER), BF_RECT);
}

void CDeskMovr::DrawCaption(HDC hdc, UINT uDrawFlags, int x, int y)
{
    RECT rect;
    UINT uState;

    // Draw the caption
    if (uDrawFlags & DMDC_CAPTION) {
        rect = m_rectCaption;
        OffsetRect(&rect, x, y);
        FillRect( hdc, &rect, (HBRUSH)(COLOR_3DFACE + 1) );
    }

    // Draw the caption frame controls
    if ((uDrawFlags & DMDC_CLOSE) && GetCaptionButtonRect(dmClose, &rect)) {
        uState = DFCS_CAPTIONCLOSE;
        if (SHRestricted(REST_NOCLOSEDESKCOMP))
            uState |= DFCS_INACTIVE | DFCS_FLAT;
        else
        {
            if (m_CaptionState & CS_CLOSEPUSHED)
                uState |= DFCS_PUSHED;
            if (!(m_CaptionState & (CS_CLOSETRACKED | CS_CLOSEPUSHED)))
                uState |= DFCS_FLAT;
        }
        OffsetRect(&rect, x, y);
        DrawCaptionButton(hdc, &rect, DFC_CAPTION, uState, !(uDrawFlags & DMDC_CAPTION));
    }
    if ((uDrawFlags & DMDC_MENU) && GetCaptionButtonRect(dmMenu, &rect)) {
        uState = DFCS_SCROLLDOWN;
        if (m_CaptionState & CS_MENUPUSHED)
            uState |= DFCS_PUSHED;
        if (!(m_CaptionState & (CS_MENUTRACKED | CS_MENUPUSHED)))
            uState |= DFCS_FLAT;
        
        OffsetRect(&rect, x, y);

        DrawCaptionButton(hdc, &rect, DFC_SCROLL, uState, !(uDrawFlags & DMDC_CAPTION));
    }
}

HRESULT CDeskMovr::OnDraw(ATL_DRAWINFO& di)
{
    RECT& rc = *(RECT*)di.prcBounds;

    RECT r;
    HBRUSH  hbrush = (HBRUSH)(COLOR_3DFACE + 1);
    
    // top edge
    r.left = rc.left;
    r.top = rc.top;
    r.right = rc.right;
    r.bottom = rc.top + m_cyBorder;
    FillRect( di.hdcDraw, &r, hbrush );
    // left edge
    r.top = rc.top + m_cyBorder;
    r.right = rc.left + m_cxBorder;
    r.bottom = rc.bottom - m_cyBorder;
    FillRect( di.hdcDraw, &r, hbrush );
    // right edge
    r.right = rc.right;
    r.left = rc.right - m_cxBorder;
    FillRect( di.hdcDraw, &r, hbrush );
    // bottom edge
    r.left = rc.left;
    r.top = rc.bottom - m_cyBorder;
    r.right = rc.right;
    r.bottom = rc.bottom;
    FillRect( di.hdcDraw, &r, hbrush );

    if ( m_cyCaption != 0 ) {
        DrawCaption(di.hdcDraw, DMDC_ALL, rc.left, rc.top);
    }

    return S_OK;
}

HRESULT CDeskMovr::Duck(BOOL fDuck)
{
    TraceMsg(TF_CUSTOM2, "CDeskMovr::Duck(fDuck=%lx) m_pistyle=%lx, m_bstrTargetName=%ls", (DWORD)fDuck, m_pistyle, GEN_DEBUGSTRW(m_bstrTargetName));

    if ( m_pistyle != NULL ) {
        HRESULT hr;

        hr = m_pistyle->put_pixelWidth( 0 );
        ASSERT(SUCCEEDED(hr));
    }
    return S_OK;
}

HRESULT CDeskMovr::GetParentWindow(void)
{
    HRESULT hr = S_OK;

    if (!m_hwndParent)
    {
        if (m_spInPlaceSite) 
            hr = m_spInPlaceSite->GetWindow(&m_hwndParent);
        else 
        {
            IOleInPlaceSiteWindowless * poipsw;

            ASSERT(m_spClientSite);
            if (m_spClientSite &&
                SUCCEEDED(hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&poipsw)))
            {
                hr = poipsw->GetWindow(&m_hwndParent);
                poipsw->Release();
            }
        }

        if (!m_hwndParent)
            hr = S_FALSE;   // We failed to get it.
    }

    return hr;
}

void CDeskMovr::DeactivateMovr(BOOL fDestroy)
{
    TraceMsg(TF_CUSTOM2, "CDeskMovr::DeactivateMovr() m_fTimer=%lx, m_bstrTargetName=%ls", m_fTimer, GEN_DEBUGSTRW(m_bstrTargetName));

    if (m_fTimer && (fDestroy || (m_uiTimerID == DETECT_TIMER_ID))) {
        m_TimerWnd.KillTimer(m_uiTimerID);
        m_TimerWnd.DestroyWindow();
        m_fTimer = FALSE;
#ifndef SHDOC401_DLL
        if (m_uiTimerID == ANIMATE_TIMER_ID)
            g_fAnimTimer = FALSE;
#endif
    }

    // DismissSelfNow();

    RELEASE_OBJECT( m_pistyle );
    RELEASE_OBJECT( m_pistyleTarget );
    RELEASE_OBJECT( m_pielemTarget );

    _ChangeCapture(FALSE);
}


HRESULT CDeskMovr::ActivateMovr()
{
    HRESULT           hr;

    // flush out old interface pointers
    DeactivateMovr(FALSE);
    TraceMsg(TF_CUSTOM2, "CDeskMovr::ActivateMovr() m_fTimer=%lx, m_bstrTargetName=%ls", m_fTimer, GEN_DEBUGSTRW(m_bstrTargetName));

    if (m_fEnabled && SUCCEEDED(hr = GetOurStyle()))
    {
        if (!m_fTimer && (m_bstrTargetName != NULL) && (m_lInterval > 0))
        {
            // create a new timer.
            RECT rc = {0, 0, 0 , 0};

            // We attempt to get our parent HWND (m_hwndParent) now.
            // If we fail (and we will sometimes), then we will get it later when
            // we need it and Trident is then ready.
            GetParentWindow();

            m_TimerWnd.Create(NULL, rc, _T("Timer"), WS_POPUP);
            m_fTimer = m_TimerWnd.SetTimer(m_uiTimerID, m_lInterval, NULL) != 0;
        }
        else
        {
#ifdef HIDE_ALL_HANDLES
            hr = S_FALSE;
#else
            hr = E_FAIL;
#endif
        }
    }  

    return hr;
}

HRESULT CDeskMovr::GetOurStyle(void)
{
    HRESULT           hr;
    IOleControlSite   *pictlsite = 0;
    IDispatch         *pidisp = 0;

    // Reach up to get our extender, who is the custodian of our element style
    if (m_spClientSite &&
        EVAL(SUCCEEDED(hr = m_spClientSite->QueryInterface(IID_IOleControlSite, (LPVOID*)&pictlsite))) &&
        EVAL(SUCCEEDED(hr = pictlsite->GetExtendedControl(&pidisp))))
    {
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
        VARIANT var;
        
        VariantInit( &var );

        // Alas, all we have is IDispatch on our extender, so we'll have to use Invoke to get
        // the style object...
        hr = pidisp->Invoke( DISPID_IHTMLELEMENT_STYLE, IID_NULL,
                             LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                             &dispparamsNoArgs, &var, NULL, NULL );

        if ( SUCCEEDED(hr) ) {
            if ( var.vt == VT_DISPATCH )
                hr = var.pdispVal->QueryInterface( IID_IHTMLStyle, (LPVOID*)&m_pistyle );
            else
                hr = E_FAIL; // Try VariantChangeType?????

            VariantClear( &var );
        }
    }

    QUICK_RELEASE( pictlsite );
    QUICK_RELEASE( pidisp );

    return hr;
}



void CDeskMovr::UpdateCaption(UINT uDrawFlags)
{
    HDC hdc;
    int x = 0, y = 0;

    if (m_bWndLess) {
        if (!m_spInPlaceSite || !SUCCEEDED(m_spInPlaceSite->GetDC(NULL, 0, &hdc)))
            return;
    } else {
        hdc = ::GetDC(m_hWnd);
    }

    _MapPoints(&x, &y);

    DrawCaption(hdc, uDrawFlags, -x, -y);

    if (m_bWndLess) {
        m_spInPlaceSite->ReleaseDC(hdc);
    } else {
        ::ReleaseDC(m_hWnd, hdc);
    }
}

void CDeskMovr::CheckCaptionState(int x, int y)
{
    DragMode dm;
    UINT uDrawFlags = 0;

    _MapPoints (&x, &y);

    POINT pt = { x, y };

    if (m_fCaptured)
        dm = dmNull;
    else
        dm = DragModeFromPoint( pt );

    switch (dm) {
        case dmClose:
            if (!(m_CaptionState & (CS_CLOSEPUSHED | CS_CLOSETRACKED))) {
                m_CaptionState |= CS_CLOSETRACKED;
                uDrawFlags |= DMDC_CLOSE;
            }
            break;
        case dmMenu:
            if (!(m_CaptionState & (CS_MENUPUSHED | CS_MENUTRACKED))) {
                m_CaptionState |= CS_MENUTRACKED;
                uDrawFlags |= DMDC_MENU;
            }
            break;

        default:
            break;
    }

    if (dm != dmClose && (m_CaptionState & (CS_CLOSETRACKED | CS_CLOSEPUSHED))) {
        m_CaptionState &= ~(CS_CLOSETRACKED | CS_CLOSEPUSHED);
        uDrawFlags |= DMDC_CLOSE;
    }
    if (dm != dmMenu && (m_CaptionState & (CS_MENUTRACKED | CS_MENUPUSHED))) {
        m_CaptionState &= ~(CS_MENUTRACKED | CS_MENUPUSHED);
        uDrawFlags |= DMDC_MENU;
    }

    if (uDrawFlags)
        UpdateCaption(uDrawFlags);
}

//=--------------------------------------------------------------------------=
// CDeskMovr::DoMouseDown   [instance method]
//=--------------------------------------------------------------------------=
// Respond to mouse down messages in our control. Initiate move/resize.
//
// Parameters:
//    int                - [in]  mouse message key flags
//    int                - [in]  mouse x location in control coords
//    int                - [in]  mouse y location in control coords
//
// Output:
//    <none>
//
// Notes:
LRESULT CDeskMovr::OnMouseDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    int x = (short)LOWORD(lParam);
    int y = (short)HIWORD(lParam);

    _MapPoints(&x, &y);

    TraceMsg(TF_CUSTOM2, "CDeskMovr::OnMouseDown() Mouse=<%d,%d>, Inner=<%d,%d,%d,%d>, Caption=<%d,%d,%d,%d>, m_bstrTargetName=%ls", 
        x, y, m_rectInner.left, m_rectInner.top, m_rectInner.right, m_rectInner.bottom,
        m_rectCaption.left, m_rectCaption.top, m_rectCaption.right, m_rectCaption.bottom, GEN_DEBUGSTRW(m_bstrTargetName));

    POINT pt = { x, y };
    m_dmCur = DragModeFromPoint( pt );

    switch ( m_dmCur ) {
    case dmMove:
        m_dx = -x;
        m_dy = -y;
        break;
    case dmClose:
        if (!SHRestricted(REST_NOCLOSEDESKCOMP))
        {
            m_CaptionState &= ~CS_CLOSETRACKED;
            m_CaptionState |= CS_CLOSEPUSHED;
            UpdateCaption(DMDC_CLOSE);
            // Perform the close operation on the up-click of the mouse...
            // IElemCloseDesktopComp(m_pielemTarget);
        }
        return 0;
    case dmMenu:
        if (EVAL(S_OK == GetParentWindow()))
        {
            POINT point;

            if (CAPTION_ONLY)
            {
                point.x = m_left + m_cxBorder;
                point.y = m_top + (m_cyCaption + m_cyBorder) - 4 * m_cySMBorder;
            } else {
                point.x = m_left - m_cxSMBorder;
                point.y = m_top - 4 * m_cySMBorder;
            }

            ::ClientToScreen(m_hwndParent, &point);
            m_CaptionState &= ~CS_MENUTRACKED;
            m_CaptionState |= CS_MENUPUSHED;
            UpdateCaption(DMDC_MENU);

            // BUGBUG: This calculation needs to be revisited.  The reason it's so
            //         ugle and HACKy is because to look good, we want the context menu
            //         to appear on top of the 3-D edge below the triangle.
            _DisplayContextMenu(point.x, point.y);
        }
        return 0;
    case dmSizeWHBR:
        m_dx = m_rectInner.right - x;
        m_dy = m_rectInner.bottom - y;
        break;
    case dmSizeWHTL:
        m_dx = m_rectInner.left - x;
        m_dy = m_rectInner.top + m_cyCaption - y;
        break;
    case dmSizeWHTR:
        m_dx = m_rectInner.right - x;
        m_dy = m_rectInner.top + m_cyCaption - y;
        break;
    case dmSizeWHBL:
        m_dx = m_rectInner.left - x;
        m_dy = m_rectInner.bottom - y;
        break;
    case dmSizeTop:
        m_dx = 0;
        m_dy = m_rectInner.top + m_cyCaption - y;
        break;
    case dmSizeBottom:
        m_dx = 0;
        m_dy = m_rectInner.bottom - y;
        break;
    case dmSizeLeft:
        m_dx = m_rectInner.left - x;
        m_dy = 0;
        break;
    case dmSizeRight:
        m_dx = m_rectInner.right - x;
        m_dy = 0;
        break;
    default:
        bHandled = FALSE;
        return 1;
    }

#ifdef DEBUG
    TraceMsg(TF_CUSTOM2, "CDeskMovr::OnMouseDown() New DragMode=""%s""", g_szDragModeStr[m_dmCur]);
#endif // DEBUG

    //BUGBUG: (seanf, 1/31/97) temporary defense against 17902. We really
    //  shouldn't ever be in visible and non-targeted at the same time, but
    // the resize trick we pull in CDeskMovr::ActivateMovr() to get us
    // in-place active exposes a 1X1 pixel area, just big enough for StanTak
    // to click on when we don't have a target, which then kills us when
    // we try to move the non-existent target.
    if ( m_pielemTarget != NULL ) {
        BOOL fCapture = TRUE;
        if ( m_bWndLess )
        {
            ASSERT(m_spInPlaceSite);
            if (m_spInPlaceSite)
            {
                fCapture = SUCCEEDED(m_spInPlaceSite->SetCapture(TRUE));
            }
        }
        else
        {
            ::SetCapture( m_hWnd );
        }
        if (fCapture)
            _ChangeCapture(TRUE);

        // Move the target to the top and put ourselves just under it
        VARIANT varZ;

        m_pistyleTarget->get_zIndex(&varZ);

        // Does the component need to be moved to the top?
        if (!CAPTION_ONLY && ((VT_I4 != varZ.vt) || (varZ.lVal != m_zIndexTop)))
        {
            // Yes.
            varZ.vt = VT_I4;
            varZ.lVal = ++m_zIndexTop;
            // Move the DeskMover ActiveX Control on top of everything.
            m_pistyle->put_zIndex(varZ);

            // Move the Desktop Item on top of the DeskMover
            varZ.lVal = ++m_zIndexTop;
            m_pistyleTarget->put_zIndex(varZ);
        }
    }

    return 0;
}

//=--------------------------------------------------------------------------=
// CDeskMovr::DoMouseUp   [instance method]
//=--------------------------------------------------------------------------=
// Respond to mouse down messages in our control. Terminate move/resize.
//
// Parameters:
//    int                - [in]  mouse message key flags
//    int                - [in]  mouse x location in control coords
//    int                - [in]  mouse y location in control coords
//    UINT               - [in]  from the DeskMovrParts enum
//
// Output:
//    <none>
//
// Notes:

LRESULT CDeskMovr::OnMouseUp( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if ( m_fCaptured ) {
        PersistTargetPosition( m_pielemTarget, m_left, m_top, m_width, m_height, m_zIndexTop, FALSE, m_ItemState );

        _ChangeCapture(FALSE);

        ASSERT(m_spInPlaceSite);
        if (m_bWndLess && m_spInPlaceSite)
        {
            m_spInPlaceSite->SetCapture(FALSE);
        }
        else
            ReleaseCapture();
    } else {
        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);

        _MapPoints(&x, &y);
    
        POINT pt = { x, y };
        DragMode dm = DragModeFromPoint( pt );
    
        switch ( dm ) {
        case dmClose:
            if (m_CaptionState & CS_CLOSEPUSHED) {
                m_CaptionState &= ~CS_CLOSEPUSHED;
                m_CaptionState |= CS_CLOSETRACKED;
                UpdateCaption(DMDC_CLOSE);
                IElemCloseDesktopComp(m_pielemTarget);
                DismissSelfNow();
            }
            break;
        case dmMenu:
            if (m_CaptionState & CS_MENUPUSHED) {
                m_CaptionState &= ~CS_MENUPUSHED;
                m_CaptionState |= CS_MENUTRACKED;
                UpdateCaption(DMDC_MENU);
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

//=--------------------------------------------------------------------------=
// CDeskMovrControl::DoMouseMove   [instance method]
//=--------------------------------------------------------------------------=
// Respond to mouse move messages in our control and when moving/sizing.
//
// Parameters:
//
// Output:
//    <none>
//
// Notes:

LRESULT CDeskMovr::OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    TraceMsg(TF_CUSTOM2, "CDeskMovr::OnPaint() uMsg=%lx, wParam=%lx, lParam=%lx, m_bstrTargetName=%ls", uMsg, wParam, lParam, GEN_DEBUGSTRW(m_bstrTargetName));
    return CComControl<CDeskMovr>::OnPaint( uMsg, wParam, lParam, bHandled );
}

//=--------------------------------------------------------------------------=
// CDeskMovrControl::DoMouseMove   [instance method]
//=--------------------------------------------------------------------------=
// Respond to mouse move messages in our control and when moving/sizing.
//
// Parameters:
//
// Output:
//    <none>
//
// Notes:

LRESULT CDeskMovr::OnMouseMove( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CheckCaptionState((short)LOWORD(lParam), (short)HIWORD(lParam));

    if (m_fCaptured && EVAL(S_OK == GetParentWindow()))
    {
        // Okay, it's a hit on one of our gadgets.
        // We're only interested in mouse moves and mouse ups if we're in the
        // process of a drag or resize
        HRESULT hr;
        POINT   ptDoc; // location in document window coords
        POINT   ptScreen;
        HWND    hwndParent = m_hwndParent;

        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);

        ptScreen.x = x;
        ptScreen.y = y;
        ptDoc = ptScreen;
        if ( !m_bWndLess ) 
            ::MapWindowPoints( m_hWnd, hwndParent, &ptDoc, 1 );

        if ( m_dmCur == dmMove )
            hr = MoveSelfAndTarget( ptDoc.x + m_dx + m_cxBorder, ptDoc.y + m_dy + m_cyBorder + m_cyCaption );
        else if ( m_dmCur > dmMove )
            hr = SizeSelfAndTarget( ptDoc );

        ASSERT(SUCCEEDED(hr));
    }

    // Set m_cSkipTimer so that we delay dismissing the mover...
    m_cSkipTimer = GET_SKIP_COUNT;

    return 0;
}

LRESULT CDeskMovr::OnTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HRESULT hr;
    IHTMLElement *pielem;
    POINT   ptCursor;
    BOOL    fDidWork = FALSE;

#ifndef SHDOC401_DLL
    /*
     * Check our animation timer first.  If we are able to disable animations then
     * blow away the timer.  Otherwise reset the timer for 60 seconds and keep on
     * looking.
     */
    if (wParam == ANIMATE_TIMER_ID)
    {
        if (CombView_EnableAnimations(FALSE))
        {
            m_TimerWnd.SetTimer(ANIMATE_TIMER_ID, ANIMATE_TIMER_INTERVAL, NULL);
        }
        else
        {
            m_TimerWnd.KillTimer(m_uiTimerID);
            m_uiTimerID = DETECT_TIMER_ID;
            g_fAnimTimer = FALSE;
            m_fTimer = FALSE;
        }

        return 0;
    }
#endif

    if (!m_fTimer || g_fIgnoreTimers || !GetCursorPos( &ptCursor ))
        return 0;

    if (ptCursor.x == m_ptMouseCursor.x && ptCursor.y == m_ptMouseCursor.y)
        // Mouse stayed still from last time we did a timer so, do nothing
        return 0;

    pielem = NULL;

    if (EVAL(S_OK == GetParentWindow()))
    {
        HWND hwndParent = m_hwndParent;
        HWND hwndCursor = WindowFromPoint(ptCursor);

        if ((hwndCursor != hwndParent) && !::IsChild(hwndParent, hwndCursor))
        {
            // The mouse has drifted out of our window, so lose our target, if any
            if (m_iSrcTarget >= 0)
            {
                hr = MoveSelfToTarget( NULL, NULL );
                ASSERT(SUCCEEDED(hr));
                if (hr != S_FALSE)
                {
                    fDidWork = TRUE;
                }
            }
            if (GetCurrentThreadId() == g_dwHookThreadId) {
#ifndef SHDOC401_DLL
                // Set ourselves up so we can look to see if our animations can be turned off
                if (!g_fAnimTimer)
                {
                    if (m_fTimer)
                        m_TimerWnd.KillTimer(m_uiTimerID);

                    if (g_fAnimTimer = (m_TimerWnd.SetTimer(ANIMATE_TIMER_ID, ANIMATE_TIMER_INTERVAL / 10, NULL) != 0))
                        m_uiTimerID = ANIMATE_TIMER_ID;
                    m_fTimer = g_fAnimTimer;
                }
#endif
                DismissSelfNow();
                DeactivateMovr(FALSE);
                if (!g_hMouseHook)
                    g_hMouseHook = SetWindowsHookEx(WH_MOUSE, DeskMovr_MouseHook, NULL, GetCurrentThreadId());
            }
        }
        else if (SUCCEEDED(hr = _IsInElement(hwndParent, &ptCursor, &pielem)))
        {
            // BUGBUG: If (!pielem), MoveSelfToTarget( NULL, NULL );, else ...
            IHTMLElement *pTargElem = NULL;
            LONG iSrcTarget = -1;

            ASSERT(pielem); // elementFromPoint() shouldn't succeed with elementFromPoint NULL.
            if ( FFindTargetElement( pielem, &pTargElem ) )
            {
                hr = pTargElem->get_sourceIndex( &iSrcTarget );
                ASSERT(SUCCEEDED(hr));
            }

            // If the m_iSrcTarget isn't the same as the SrcTarget under our cursor,
            // then we should move on top of it.
            if ( m_iSrcTarget != iSrcTarget )
            {
                fDidWork = TRUE;
            
                if ((g_nTotalActiveCaptions > 1) && (-1 == iSrcTarget))
                    m_cSkipTimer = 0;

                // Yes, we need to move on top of it.
                hr = MoveSelfToTarget( pTargElem, &ptCursor );
                ASSERT(SUCCEEDED(hr));
                if (hr != S_FALSE)
                    m_iSrcTarget = iSrcTarget;
            } 
            else
            {
                // No, so that means we already have focus...
                if (TrackCaption(ptCursor))
                {
                    TrackTarget(NULL);
                }
            }

            if ( pTargElem != NULL ) { 
                pTargElem->Release(); // MoveSelfToTarget will have secured our reference
            }

            // we're done with this particular interface pointer
            pielem->Release();

        }
        else if (m_iSrcTarget != -1) {
            // Check to see if we should expand border to size border width
            if (TrackCaption ( ptCursor ))
            {
                TrackTarget(NULL);
            }
        }
    }

    if (!fDidWork)
        m_ptMouseCursor = ptCursor;
    
    return 0;
}

LRESULT CDeskMovr::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if (!m_bWndLess) {
        RECT rc;
        ::GetClientRect(m_hWnd, &rc);
        FillRect((HDC)wParam, &rc, (HBRUSH)(COLOR_3DFACE + 1));
    }
    bHandled = TRUE;

    return 0;
}

//
// DismissSelfNow - Little helper function to dismiss the mover immediately
//
// Normally dismissal of the mover is desired to be done on a delayed basis.  However,
// there are situations such as when the user clicks on UI or capture is lost etc. where
// it is desirable to dismiss the mover immediately.
//
void CDeskMovr::DismissSelfNow(void)
{
    HRESULT hr;
    m_cSkipTimer = 0;
    hr = MoveSelfToTarget(NULL, NULL);
    ASSERT(SUCCEEDED(hr) && (hr != S_FALSE));
}

LRESULT CDeskMovr::OnCaptureChanged( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if ( m_fCaptured ) {
        _ChangeCapture(FALSE);
        PersistTargetPosition( m_pielemTarget, m_left, m_top, m_width, m_height, m_zIndexTop, FALSE, m_ItemState );
        DismissSelfNow();
    }

    return 0;
}

HRESULT CDeskMovr::DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND hwndParent)
{
    TraceMsg(TF_CUSTOM1, "CDeskMovr::DoVerb(OLEIVERB_INPLACEACTIVATE) m_fEnabled=%d, m_nFreezeEvents=%d", m_fEnabled, m_nFreezeEvents);

    return IOleObjectImpl<CDeskMovr>::DoVerbInPlaceActivate(prcPosRect, hwndParent);
}

HRESULT CDeskMovr::InPlaceDeactivate(void)
{
    DeactivateMovr(FALSE);
    TraceMsg(TF_CUSTOM1, "CDeskMovr::InPlaceDeactivate()");
    return CComControl<CDeskMovr>::IOleInPlaceObject_InPlaceDeactivate();
}

LRESULT CDeskMovr::OnSetCursor( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (EVAL(S_OK == GetParentWindow()))
    {
        POINT   ptCursor;
        DragMode dm;

        GetCursorPos( &ptCursor );

        ::ScreenToClient( m_hwndParent, &ptCursor );

        // Get ptCursor into deskmovr local coords
        ptCursor.x -= m_left - (CAPTION_ONLY ? 0 : m_cxBorder);
        ptCursor.y -= m_top - (CAPTION_ONLY ? 0 : (m_cyBorder + m_cyCaption));

        dm = DragModeFromPoint(ptCursor);
        m_hcursor = CursorFromDragMode(dm);

        TraceMsg(TF_CUSTOM2, "CDeskMovr::OnSetCursor() Mouse=<%d,%d>, Inner=<%d,%d,%d,%d>, Caption=<%d,%d,%d,%d>, m_bstrTargetName=%ls", 
            ptCursor.x, ptCursor.y, m_rectInner.left, m_rectInner.top, m_rectInner.right, m_rectInner.bottom,
            m_rectCaption.left, m_rectCaption.top, m_rectCaption.right, m_rectCaption.bottom, GEN_DEBUGSTRW(m_bstrTargetName));

    #ifdef DEBUG
        TraceMsg(TF_CUSTOM2, "CDeskMovr::OnSetCursor() New DragMode=""%s""", g_szDragModeStr[dm]);
    #endif // DEBUG

        if (EVAL(m_hcursor != NULL))
            SetCursor( m_hcursor );
        else
            bHandled = FALSE;
    }

    return !bHandled;
}


void CDeskMovr::TrackTarget(POINT * pptDoc)
{
    HRESULT hr = S_OK;

    if ( m_fEnabled && m_pielemTarget != NULL ) {
        POINT pt;
        VARIANT varZ;
        varZ.vt = VT_I4;

        // CLEANUP_ON_FAILURE(hr = DocUnitTopLeft(m_pielemTarget, &pt));
        CLEANUP_ON_FAILURE(hr = CSSOM_TopLeft(m_pielemTarget, &pt));
        m_top = pt.y;
        m_left = pt.x;

        // TODO - Multimonitor, Read from component?
        if (m_top == 0)
        {
            if (m_left == 0)
                m_ItemState = IS_FULLSCREEN;
            else
                m_ItemState = IS_SPLIT;
        }
        else
        {
            m_ItemState = IS_NORMAL;
        }

        CLEANUP_ON_FAILURE(hr = m_pielemTarget->get_offsetHeight( &m_height ));
        CLEANUP_ON_FAILURE(hr = m_pielemTarget->get_offsetWidth( &m_width ));
  
        // Hack so we don't get weird painting effect of the window hopping to the new
        // target with the old target's size.
        if (!m_bWndLess && m_cyCaption == 0)
        {
            ASSERT(m_hWnd);
            ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Get our rectangle synced with the target (so TrackCaption works properly)
        SyncRectsToTarget();
        // If we discover we want to display the size-border or caption
        // right now then we need to recalculate our rects.
        if (pptDoc && TrackCaption(*pptDoc))
            SyncRectsToTarget();

        CLEANUP_ON_FAILURE(hr = m_pistyleTarget->get_zIndex( &varZ ));
        if (!CAPTION_ONLY || (m_cxBorder == m_cxSMBorder))
            --varZ.lVal;
        else
            ++varZ.lVal;
        CLEANUP_ON_FAILURE(hr = SafeZOrderSet(m_pistyle, varZ.lVal));

        // BUGBUG: If this is hosted in a window that has scrollbars,
        //         we don't correctly add the screen to document offset
        //         when changing the location of the component.
        //         This causes us to drag incorrectly.
        hr = m_pistyle->put_pixelTop( CAPTION_ONLY ? 0 : (m_top - m_cyBorder - m_cyCaption) );
        hr = m_pistyle->put_pixelLeft( m_left - (CAPTION_ONLY ? 0 : m_cxBorder) );
        hr = m_pistyle->put_pixelWidth( m_rectOuter.right );
        hr = m_pistyle->put_pixelHeight( m_rectOuter.bottom );

        hr = m_pistyle->put_visibility((BSTR)s_sstrVisible.wsz);
    }

CleanUp:
    ASSERT(SUCCEEDED(hr));
}

BOOL CDeskMovr::TrackCaption( POINT ptDoc )
{
    int         cyCaption, cyCaptionNew;
    POINT       ptMovr = ptDoc;
    DragMode    dmNew;
    BOOL        fRetVal = FALSE;

    //TraceMsg(TF_CUSTOM2, "CDeskMovr::TrackCaption() Mouse=<%d,%d>", ptMovr.x, ptMovr.y);

    // need a hit test of some sort within the deskmovr to control border swelling
    ptMovr.x -= m_left - m_cxBorder;
    ptMovr.y -= m_top - (m_cyBorder + m_cyCaption);

    dmNew = DragModeFromPoint( ptMovr );

    cyCaption = GET_CYCAPTION;

    if (dmNew == dmNull) {
        BOOL fInner;
        int iInflate;
        RECT rc;
        // Treat something near the size border as a size border hit
        // so we expand to the size border as the user nears the edge.
        fInner = PtInRect(&m_rectInner, ptMovr);

        if (fInner) {
            rc = m_rectInner;
            iInflate = -cyCaption;
        } else {
            rc = m_rectOuter;
            iInflate = cyCaption;
        }

        InflateRect(&rc, iInflate, iInflate);
        if (fInner != PtInRect(&rc, ptMovr))
            dmNew = dmSizeRight;
    }

    if ( (ptDoc.y >= m_top - (m_cyBorder + 2 * m_cyCaption) &&
          ptDoc.y <= (m_top + CAPTIONBAR_HOTAREA(cyCaption, m_cyCaption)) ) )
        cyCaptionNew = cyCaption;
    else
        cyCaptionNew = 0;

    if ( cyCaptionNew != m_cyCaption ||
        (m_dmTrack != dmNew && !((m_dmTrack > dmMove) && (dmNew > dmMove))) ) {
        m_cyCaption = cyCaptionNew;
        if (m_cyCaption == 0)
            m_CaptionState = 0;
        m_dmTrack = dmNew;
        fRetVal = TRUE;
    } else
        m_cyCaption = cyCaptionNew;

    return fRetVal;
}


//=--------------------------------------------------------------------------=
// CDeskMovr::InitResizeableAttributes  [instance method]
//=--------------------------------------------------------------------------=
// Finds out if the element is resizeable in X and Y direction and sets the
// BITBOOLs accordingly.
//
// Parameters:
//    IHTMLElement*     [in] - interface on event source element
//
// Output:
//    HRESULT      - various. S_OK if  operation succeeded.
//
HRESULT CDeskMovr::InitResizeableAttributes(IHTMLElement *pielem)
{
    HRESULT hr;
    TCHAR   szHTMLElementResizeable[3];

    ASSERT(pielem);

    m_fCanResizeX = m_fCanResizeY = FALSE;  //Assume "Can't resize!

    if (SUCCEEDED(hr = GetHTMLElementStrMember(pielem, szHTMLElementResizeable, SIZECHARS(szHTMLElementResizeable), (BSTR)(s_sstrResizeableMember.wsz))))
    {
        if(StrChr(szHTMLElementResizeable, TEXT('X')))
                m_fCanResizeX = TRUE;

        if(StrChr(szHTMLElementResizeable, TEXT('Y')))
                m_fCanResizeY = TRUE;
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// CDeskMovr::MoveSelfToTarget   [instance method]
//=--------------------------------------------------------------------------=
// Handles Trident document events as mouse moves over the desktop.
//
// Parameters:
//    IHTMLElement*     [in] - interface on event source element
//    POINT*            [in] - location of mouse (to determine if caption should be displayed)
//
// Output:
//    HRESULT      - various. S_OK if  operation succeeded.
//


HRESULT CDeskMovr::MoveSelfToTarget(IHTMLElement *pielem, POINT * pptDoc)
{
    HRESULT hr = S_OK;

    TraceMsg(TF_CUSTOM2, "CDeskMovr::MoveSelfToTarget(pielem=%lx) %s, m_bstrTargetName=%ls", pielem, (pielem ? "We are GETTING focus." : "We are LOOSING focus."), GEN_DEBUGSTRW(m_bstrTargetName));

    if (!pielem)
    {
        // The m_cSkipTimer variable is used to allow the skipping of timer ticks when determining 
        // if the mover should be dismissed.  By doing this it gives the user more time and thus
        // a better chance to manipulate the target if they are prone to drifting the mouse
        // outside the target by accident.

        // Check the m_cSkipTimer before dismissing the mover.
        if (!m_cSkipTimer)
        {
            if (m_pistyleTarget)
            {
                // We are loosing focus.
                g_nTotalActiveCaptions--;
                ASSERT(g_nTotalActiveCaptions >= 0);
            }

            if (m_pistyle)
                hr = m_pistyle->put_visibility((BSTR)s_sstrHidden.wsz);
            RELEASE_OBJECT( m_pistyleTarget );
            RELEASE_OBJECT( m_pielemTarget );
            m_ItemState = IS_NORMAL;
            m_iSrcTarget = -1;
        }
        else
        {
            m_cSkipTimer--;
            hr = S_FALSE;
        }

        // These are actions we want to happen right away.
        m_hcursor = CursorFromDragMode(dmNull);
        if (m_hcursor != NULL)
            SetCursor(m_hcursor);
    }

    // These are actions we want to happen after the Desktop Item
    // looses focus.
    if (hr != S_FALSE)
    {
        m_cyCaption = 0;
        m_cxBorder = m_cxSMBorder;
        m_cyBorder = m_cySMBorder;
        m_CaptionState = 0;
        m_dmTrack = dmNull;
    }

    if (pielem)
    {
        ASSERT(m_pielemTarget != pielem);   // If hit, g_nTotalActiveCaptions will get out of wack.

        // exchange our new target ( if any ) for the old target, if any...
        RELEASE_OBJECT( m_pistyleTarget );
        RELEASE_OBJECT( m_pielemTarget );
        hr = pielem->get_style(&m_pistyleTarget);
        if (SUCCEEDED(hr))
        {
            // We are gaining focus.
            g_nTotalActiveCaptions++;

            m_pielemTarget = pielem;
            ADDREF_OBJECT( m_pielemTarget );

            InitResizeableAttributes(m_pielemTarget);

            TrackTarget(pptDoc);
            // Set m_cSkipTimer so that we delay dismissing the mover...
            m_cSkipTimer = GET_SKIP_COUNT;
        }
    }
   
    return hr;
}

//=--------------------------------------------------------------------------=
// CDeskMovrControl::MoveSelfAndTarget   [instance method]
//=--------------------------------------------------------------------------=
// Moves the control and it's target to a new location.
//
// Parameters:
//    LONG    [in] - x location, in document coord's to move to
//    LONG    [in] - y location, in document coord's to move to
//
// Output:
//    HRESULT      - various. S_OK if  operation succeeded.
//
// Notes:
//      We read back the target's location so that we stay consistent with
//      any constraint's Trident might impose on our movement.

HRESULT CDeskMovr::MoveSelfAndTarget( LONG x, LONG y )
{
    HRESULT hr;

    m_top = y;
    CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelTop( y  - m_cyBorder - m_cyCaption )));
    CLEANUP_ON_FAILURE((hr = m_pistyleTarget->put_pixelTop( y  )));
    // read it back to catch Trident constraint.
    //CLEANUP_ON_FAILURE((hr = m_pielemTarget->get_docTop( &m_top )));
    //CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelTop( m_top )));

    m_left = x;
    CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelLeft( x - m_cxBorder )));
    CLEANUP_ON_FAILURE((hr = m_pistyleTarget->put_pixelLeft( x  )));
    // read it back to catch Trident constraint.
    //CLEANUP_ON_FAILURE((hr = m_pielemTarget->get_docLeft( &m_left )));
    //CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelLeft( m_left )));

    // if ( !m_bWndLess )
    if (EVAL(S_OK == GetParentWindow()))
        ::UpdateWindow(m_hwndParent);

CleanUp:
    return hr;
}

BOOL CDeskMovr::FFindTargetElement( IHTMLElement *pielem, IHTMLElement **ppielem )
{
    *ppielem = NULL;

    if ( pielem != NULL )
    {
        IDeskMovr   *pidm = NULL;       

        // If it is over the mover return the current target, otherwise
        // find out which component if any we are over.
        if ( m_pielemTarget != NULL && 
               SUCCEEDED(pielem->QueryInterface(IID_IDeskMovr, (LPVOID*)&pidm)))
        {
            m_pielemTarget->AddRef();
            *ppielem = m_pielemTarget;
            QUICK_RELEASE(pidm);
        } else {
            HRESULT hr;
            IHTMLElement *pielem2 = pielem;

            pielem2->AddRef();

            do
            {
                VARIANT     var;
    
                VariantInit( &var );
                
                if ( SUCCEEDED(hr = pielem2->getAttribute( (BSTR)s_sstrNameMember.wsz, TRUE, &var)) ) {
                    if ( var.vt == VT_BSTR && var.bstrVal != NULL ) {
                        if ( StrCmpW( var.bstrVal, m_bstrTargetName ) == 0 )
                            hr = S_OK;
                        else
                            hr = S_FALSE;               
                    } else
                        hr = S_FALSE; // Try VariantChangeType?????
                } else
                    hr = S_FALSE; // not here, maybe in parent.
            
                VariantClear( &var );
    
                if ( hr == S_OK ) { // we found it
                    hr = pielem2->QueryInterface( IID_IHTMLElement, (LPVOID*)ppielem );
                } else if ( hr == S_FALSE ) { // not this one, climb up
                    IHTMLElement *pielemParent = NULL;
                
                    pielem2->get_parentElement( &pielemParent );
                    pielem2->Release();     // we're through at this level
                    pielem2 = pielemParent; // may be null, which just means we've reached the top.
                }
    
            } while ( SUCCEEDED(hr) && *ppielem == NULL && pielem2 != NULL );
        
            QUICK_RELEASE(pielem2);
        }
    }

    return *ppielem != NULL;
}

//=--------------------------------------------------------------------------=
// CDeskMovr::DragModeFromPoint   [instance method]
//=--------------------------------------------------------------------------=
// Moves the control and it's target to a new location.
//
// Parameters:
//    POINT        -  point to test, in local coords
//
// Output:
//    DragMode      - drag mode associated with the point
//
// Notes:
//      This is only a hit testing method. It doesn not alter state.

CDeskMovr::DragMode CDeskMovr::DragModeFromPoint( POINT pt )
{
    enum DragMode dm = dmNull;
    RECT rc;

    if ( PtInRect( &m_rectInner, pt ) ) 
    { // either no-hit, or on caption
        if ( PtInRect( &m_rectCaption, pt ) ) {
            if (GetCaptionButtonRect(dmClose, &rc) && PtInRect(&rc, pt))
                dm = dmClose;
            else if (GetCaptionButtonRect(dmMenu, &rc) && PtInRect(&rc, pt))
                dm = dmMenu;
            else if (!CAPTION_ONLY)
                dm = dmMove;
        }
    } else {
        if ( PtInRect( &m_rectOuter, pt ) ) {
            if (!CAPTION_ONLY)
            {
                // a resize border hit
                if ( pt.y <= m_sizeCorner.cy ) {
                    // upper edge or corners
                    if ( pt.x <= m_sizeCorner.cx )
                        dm = dmSizeWHTL;
                    else if ( pt.x >= m_rectOuter.right - m_sizeCorner.cx )
                        dm = dmSizeWHTR;
                    else
                        dm = dmSizeTop;
                } else if ( pt.y >= m_rectOuter.bottom - m_sizeCorner.cy ) {
                    // bottom edge or corners
                    if ( pt.x <= m_sizeCorner.cx )
                        dm = dmSizeWHBL;
                    else if ( pt.x >= m_rectOuter.right - m_sizeCorner.cx )
                    dm = dmSizeWHBR;
                    else
                        dm = dmSizeBottom;
                } else {
                    // side edge hit
                    if ( pt.x > m_rectInner.left )
                        dm = dmSizeRight;
                    else 
                        dm = dmSizeLeft;
                }
            } else {
                if (m_cyCaption == 0)
                    dm = dmSizeLeft;
                else
                    dm = dmNull;
            }
        }
        //Check if this element can be sized in both the directions.
        if(!m_fCanResizeX)
        {
            if((dm != dmSizeTop) && (dm != dmSizeBottom))
                dm = dmNull;
        }

        if(!m_fCanResizeY)
        {
            if((dm != dmSizeLeft) && (dm != dmSizeRight))
                dm = dmNull;
        }
    }

    return dm;
}

// Align our member RECTs with the dimensions of the target element.
void CDeskMovr::SyncRectsToTarget(void)
{
    // do the swelling thang
    if ( (m_dmTrack > dmMove) || m_cyCaption ) {
        m_cxBorder = GET_CXSIZE;
        m_cyBorder = GET_CYSIZE;
    } else {
        m_cxBorder = m_cxSMBorder;
        m_cyBorder = m_cySMBorder;
    }

    m_rectOuter.top = m_rectOuter.left = 0;

    if (CAPTION_ONLY)
    {
        if (m_cyCaption != 0)
        {
            // Displaying just caption
            m_rectOuter.bottom = m_cyCaption + m_cyBorder;
            m_rectOuter.right = m_width;
        } else {
            // Displaying just left size border
            m_rectOuter.bottom = m_height;
            m_rectOuter.right = m_cxBorder;
        }
    } else {
        // Displaying caption and border
        m_rectOuter.bottom = m_height + 2 * m_cyBorder + m_cyCaption;
        m_rectOuter.right = m_width + 2 * m_cxBorder;
    }

    if (CAPTION_ONLY && m_cyCaption == 0)
    {
        // Displaying just left size border
        SetRectEmpty(&m_rectInner);
        SetRectEmpty(&m_rectCaption);
    } else {
        // Displaying caption and possibly border
        m_rectInner = m_rectOuter;
        InflateRect( &m_rectInner, -m_cxBorder, -m_cyBorder );

        m_rectCaption = m_rectInner;
        m_rectCaption.bottom = m_cyBorder + m_cyCaption;

    }

    if ( m_rectOuter.bottom > 2 * m_cyCaption )
        m_sizeCorner.cy = GET_CYCAPTION;
    else
        m_sizeCorner.cy = m_rectOuter.bottom / 2;

   if ( m_rectOuter.right > 2 * m_cyCaption )
        m_sizeCorner.cx = GET_CYCAPTION;
    else
        m_sizeCorner.cx = m_rectOuter.right / 2;


}

//=--------------------------------------------------------------------------=
// CDeskMovr::SizeSelfAndTarget   [instance method]
//=--------------------------------------------------------------------------=
// Resizes our control and its target element.
//
// Parameters:
//    LONG    [in] - new width
//    LONG    [in] - new height
//
// Output:
//    HRESULT      - various. S_OK if  operation succeeded.
//
// Notes:
//      We read back the target's dimensions so that we stay consistent with
//      any constraint's Trident might impose on our sizing.

HRESULT CDeskMovr::SizeSelfAndTarget( POINT ptDoc )
{
    HRESULT hr;
    int topOld = m_top;
    int leftOld = m_left;
    int heightOld = m_height;
    int widthOld = m_width;
    int cyCaption = GET_CYCAPTION;

    switch ( m_dmCur ) {
    case dmSizeWHBR:
        m_width = (ptDoc.x + m_dx) - m_left;
        m_height = (ptDoc.y + m_dy) - m_top;
        break;
    case dmSizeWHTL:
        m_top = ptDoc.y + m_dy;
        m_height += topOld - m_top;
        m_left = ptDoc.x + m_dx;
        m_width += leftOld - m_left;
        break;
    case dmSizeWHTR:
        m_top = ptDoc.y + m_dy;
        m_height += topOld - m_top;
        m_width = (ptDoc.x + m_dx) - m_left;
        break;
    case dmSizeWHBL:
        m_height = (ptDoc.y + m_dy) - m_top;
        m_left = ptDoc.x + m_dx;
        m_width += leftOld - m_left;
        break;
    case dmSizeTop:
        m_top = ptDoc.y + m_dy;
        m_height += topOld - m_top;
        break;
    case dmSizeBottom:
        m_height = (ptDoc.y + m_dy) - m_top;
        break;
    case dmSizeLeft:
        m_left = ptDoc.x + m_dx;
        m_width += leftOld - m_left;
        break;
    case dmSizeRight:
        m_width = (ptDoc.x + m_dx) - m_left;
        break;
    default:
        ASSERT(FALSE);
        return E_FAIL;
    }

    // limit shrinkage to keep the handle accessible
    if ( m_height < cyCaption ) {
        m_height = cyCaption;
        if ( m_top != topOld )
            m_top = topOld + heightOld - m_height;
    }
 
    // limit shrinkage to keep the handle accessible
    if ( m_width < (4 * cyCaption) ) {
        m_width = 4 * cyCaption;
        if ( m_left != leftOld )
            m_left = leftOld + widthOld - m_width;
    }

    SyncRectsToTarget();

    if ( m_top != topOld ) {
        CLEANUP_ON_FAILURE((hr = m_pistyleTarget->put_pixelTop( m_top )));
        CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelTop( m_top - (m_cyBorder + m_cyCaption) )));
    }

    if ( m_left != leftOld ) {
        CLEANUP_ON_FAILURE((hr = m_pistyleTarget->put_pixelLeft( m_left )));
        CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelLeft( m_left - (CAPTION_ONLY ? 0 : m_cxBorder) )));
    }
 
    CLEANUP_ON_FAILURE((hr = m_pistyleTarget->put_pixelHeight( m_height )));
    // read it back to catch Trident constraint.
    //CLEANUP_ON_FAILURE((hr = m_pielemTarget->get_docHeight( &m_height )));
    CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelHeight( m_rectOuter.bottom )));

    CLEANUP_ON_FAILURE((hr = m_pistyleTarget->put_pixelWidth( m_width )));
    // read it back to catch Trident constraint.
    //CLEANUP_ON_FAILURE((hr = m_pielemTarget->get_docWidth( &m_width )));
    CLEANUP_ON_FAILURE((hr = m_pistyle->put_pixelWidth( m_rectOuter.right )));

    if (EVAL(S_OK == GetParentWindow()))
        ::UpdateWindow(m_hwndParent);

CleanUp:
    return hr;
}


HRESULT CDeskMovr::get_Engaged(BOOL * pVal)
{
    *pVal = m_fCaptured;
    return S_OK;
}


// IQuickActivate
HRESULT CDeskMovr::QuickActivate(QACONTAINER *pQACont, QACONTROL *pQACtrl)
{
    HRESULT hr = IQuickActivate_QuickActivate(pQACont, pQACtrl);

    if (pQACont)
    {
        ClearFlag(pQACtrl->dwViewStatus, VIEWSTATUS_OPAQUE);
    }

    return hr;
}


HRESULT CDeskMovr::_GetHTMLDoc(IOleClientSite * pocs, IHTMLDocument2 ** pphd2)
{
    HRESULT hr;
    IOleContainer * poc = NULL;

    if (!EVAL(pocs) || !EVAL(pphd2))
        return E_INVALIDARG;

    *pphd2 = NULL;
    hr = pocs->GetContainer(&poc);
    if (SUCCEEDED(hr))
    {
         hr = poc->QueryInterface(IID_IHTMLDocument2, (LPVOID*) pphd2);
         poc->Release();
    }

    return hr;
}



HRESULT CDeskMovr::_IsInElement(HWND hwndParent, POINT * ppt, IHTMLElement ** pphe)
{
    HRESULT hr = E_FAIL;
    ASSERT(ppt && pphe);

    *pphe = NULL;
    if (::ScreenToClient(hwndParent, ppt))
    {
        IHTMLDocument2 * phd2;

        ASSERT(m_spClientSite);
        hr = _GetHTMLDoc(m_spClientSite, &phd2);
        if (SUCCEEDED(hr))
        {
            hr = phd2->elementFromPoint(ppt->x, ppt->y, pphe);
            if (!*pphe && SUCCEEDED(hr))
                hr = E_FAIL;    // Sometimes Trident returns S_FALSE on error.

            phd2->Release();
        }
    }

    return hr;
}



HRESULT CDeskMovr::_EnumComponents(LPFNCOMPENUM lpfn, LPVOID lpvData, DWORD dwData)
{
    HRESULT hr = E_FAIL;
    IActiveDesktop * padt = NULL;

    hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC, IID_IActiveDesktop, (LPVOID *)&padt);
    if (SUCCEEDED(hr))
    {
        int nCount;
        int nIndex;

        hr = padt->GetDesktopItemCount(&nCount, 0);

        if (EVAL(SUCCEEDED(hr)))
        {
            COMPONENT comp;

            for (nIndex = 0; nIndex < nCount; nIndex++)
            {
                comp.dwSize = sizeof(COMPONENT);

                hr = padt->GetDesktopItem(nIndex, &comp, 0);
                if (EVAL(SUCCEEDED(hr)))
                {
                    if ((hr = lpfn(&comp, lpvData, dwData)) != S_OK)
                        break;
                }
            }
        }

        padt->Release();
    }

    return hr;
}

HRESULT CDeskMovr::_EnumElements(LPFNELEMENUM lpfn, LPVOID lpvData, DWORD dwData)
{
    HRESULT hr;
    IHTMLDocument2 * phd2;

    ASSERT(m_spClientSite);

    if (SUCCEEDED(hr = _GetHTMLDoc(m_spClientSite, &phd2)))
    {
        IHTMLElementCollection * pelems;

        if (SUCCEEDED(hr = phd2->get_all(&pelems)))
        {
            VARIANT varIndex;
            VARIANT varDummy;
            IDispatch * pidisp;

            VariantInit(&varDummy);
            varIndex.vt = VT_I4;
            varIndex.lVal = 0;

            // Note:  This loop terminates when trident returns SUCCESS - but with a NULL pidisp.
            while (SUCCEEDED(hr = pelems->item(varIndex, varDummy, &pidisp)) && pidisp)
            {
                IHTMLElement * pielem;

                if (SUCCEEDED(hr = pidisp->QueryInterface(IID_IHTMLElement, (LPVOID *)&pielem)))
                {
                    hr = lpfn(pielem, lpvData, dwData);
                    pielem->Release();
                }

                pidisp->Release();

                if (hr != S_OK)
                    break;

                varIndex.lVal++;
            }

            pelems->Release();
        }
        phd2->Release();
    }
    return hr;
}


HRESULT lpfnZOrderCB(COMPONENT * pcomp, LPVOID lpvData, DWORD dwData)
{
    #define LPZORDERSLOT ((LONG *)lpvData)

    if (dwData ? (pcomp->cpPos.izIndex > *LPZORDERSLOT) : (pcomp->cpPos.izIndex < *LPZORDERSLOT))
        *LPZORDERSLOT = pcomp->cpPos.izIndex;

    return S_OK;
}

HRESULT CDeskMovr::_GetZOrderSlot(LONG * plZOrderSlot, BOOL fTop)
{
    HRESULT hr;

    ASSERT(plZOrderSlot);

    *plZOrderSlot = m_bWindowOnly ? 10000 : 5000;

    hr = _EnumComponents(lpfnZOrderCB, (LPVOID)plZOrderSlot, (DWORD)fTop);

    *plZOrderSlot += fTop ? 2 : -2; // Make sure we are above / below.

    return hr;
}


//=--------------------------------------------------------------------------=
// PersistTargetPosition   [helper function]
//=--------------------------------------------------------------------------=
// Update the registry entries that are the persistence of the desktop HTML.
//
// Parameters:
//    <none>
//
// Output:
//    <none>
//
// Notes:
//      If we fail, we do it quietly.
//=--------------------------------------------------------------------------=

void PersistTargetPosition( IHTMLElement *pielem,
                            int left,
                            int top,
                            int width,
                            int height,
                            int zIndex,
                            BOOL fSaveState,
                            DWORD dwNewState)
{
    // only do this persistence thing if we're in ( or completing ) an operation
    TCHAR szID[80];
    BOOL fOK;

    if (SUCCEEDED(GetHTMLElementStrMember(pielem, szID, SIZECHARS(szID), (BSTR)(s_sstrIDMember.wsz))))
    	fOK = UpdateDesktopPosition(szID, left, top, width, height, zIndex, fSaveState, dwNewState);

    TraceMsg(TF_CUSTOM2, "PersistTargetPosition(pielem=%s, <left=%d, top=%d, wid=%d, h=%d>)", szID, left, top, width, height);

}

void ObtainSavedStateForElem( IHTMLElement *pielem,
                       LPCOMPSTATEINFO pCompState, BOOL fRestoredState)
{
    // only do this persistence thing if we're in ( or completing ) an operation
    TCHAR szID[80];
    BOOL fOK;

    if (SUCCEEDED(GetHTMLElementStrMember(pielem, szID, SIZECHARS(szID), (BSTR)(s_sstrIDMember.wsz))))
    	fOK = GetSavedStateInfo(szID, pCompState, fRestoredState);

    TraceMsg(TF_CUSTOM2, "ObtainSavedStateForElem(pielem=%s, <left=%d, top=%d, wid=%d, h=%d>)", szID, pCompState->iLeft, pCompState->iTop, pCompState->dwWidth, pCompState->dwHeight);

}

// IOleObject
HRESULT CDeskMovr::GetMiscStatus(DWORD  dwAspect, DWORD *pdwStatus)
{
    if (dwAspect == DVASPECT_CONTENT)
    {
        *pdwStatus = OLEMISMOVR;
        return S_OK;
    }
    else
    {
        return DV_E_DVASPECT;
    }

    // dead code
}


HRESULT CDeskMovr::SetClientSite(IOleClientSite * pClientSite)
{
    if (!pClientSite)
        DeactivateMovr(FALSE);

    return CComControlBase::IOleObject_SetClientSite(pClientSite);
}

void HandleRestore(IHTMLElement * pielem)
{
    VARIANT varZ;
    COMPSTATEINFO csiRestore;
    IHTMLStyle * pistyle;

    if (SUCCEEDED(pielem->get_style(&pistyle)))
    {
        csiRestore.dwSize = sizeof(csiRestore);

        ObtainSavedStateForElem(pielem, &csiRestore, TRUE); // TRUE => Get restored state!

        pistyle->put_pixelLeft(csiRestore.iLeft);
        pistyle->put_pixelTop(csiRestore.iTop);
        pistyle->put_pixelWidth(csiRestore.dwWidth);
        pistyle->put_pixelHeight(csiRestore.dwHeight);

        varZ.vt = VT_I4;
        pistyle->get_zIndex(&varZ);

        PersistTargetPosition(pielem, csiRestore.iLeft, csiRestore.iTop, csiRestore.dwWidth, csiRestore.dwHeight, varZ.lVal, FALSE, IS_NORMAL);
        pistyle->Release();
    }
}

HRESULT lpfnRestoreCB(IHTMLElement * pielem, LPVOID lpvData, DWORD dwData)
{
    HRESULT hres = S_OK;
    TCHAR szID[80];

    if (SUCCEEDED(GetHTMLElementStrMember(pielem, szID, SIZECHARS(szID), (BSTR)(s_sstrIDMember.wsz))))
    {
        DWORD dwState = GetCurrentState(szID);

        // Since there is only one in this state we can stop the enumeration if we
        // find a fullscreen/split item.
        if (dwState & (IS_FULLSCREEN | IS_SPLIT)) {
            HandleRestore(pielem);
            hres = S_FALSE;
        }
    }

    return hres;
}


HRESULT CDeskMovr::_HandleZoom(LONG lCommand)
{
    RECT rc;
    LONG x, y, cx, cy, zIndex;
    VARIANT varZ;
    DWORD   dwOldItemState = m_ItemState;

    if (lCommand == IDM_DCCM_RESTORE) {
        COMPSTATEINFO   csiRestore;

        csiRestore.dwSize = sizeof(csiRestore);
        ObtainSavedStateForElem(m_pielemTarget, &csiRestore, TRUE); // TRUE => Get restored state!
        x = csiRestore.iLeft;
        y = csiRestore.iTop;
        cx = csiRestore.dwWidth;
        cy = csiRestore.dwHeight;
        m_ItemState = csiRestore.dwItemState;
        
        m_zIndexTop += 2;
        zIndex = m_zIndexTop;
    } else {
        SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rc, 0);
        // TODO: Multi-monitor is busted!
        if (lCommand == IDM_DCCM_FULLSCREEN) {
            x = y = 0;
            cx = rc.right - rc.left;
            cy = rc.bottom - rc.top;
            m_ItemState = IS_FULLSCREEN;
        } else {
            HWND hwndLV = FindWindowEx(::GetParent(m_hwndParent), NULL, WC_LISTVIEW, NULL);

            // For the split case we shrink the work area down temporarily to the smallest rectangle
            // that can bound the current number of icons.  This will force the icons into that rectangle,
            // then restore it back to the way it was before.  Finally, we set the size of the split
            // component to fill the rest of the space.

            if (hwndLV) {
                int iCount, iItemsPerColumn, icxWidth, icWorkAreas = 0, icWorkAreasAdd = 0;
                DWORD dwSpacing;
                RECT rcWork[LV_MAX_WORKAREAS];

                iCount = ListView_GetItemCount(hwndLV);
                // Decrement the count so that rounding works right
                if (iCount)     
                    iCount--;

                // Calculate the new width for the view rectangle
                dwSpacing = ListView_GetItemSpacing(hwndLV, FALSE);
                iItemsPerColumn = (rc.bottom - rc.top) / (HIWORD(dwSpacing));
                icxWidth = ((iCount / iItemsPerColumn) + 1) * (LOWORD(dwSpacing));

                // Don't let it get smaller than half the screen
                if (icxWidth > ((rc.right - rc.left) / 2))
                    icxWidth = (rc.right - rc.left) / 2;

                ListView_GetNumberOfWorkAreas(hwndLV, &icWorkAreas);

                // This case should never happen in practice, but just in case...
                if (!icWorkAreas)
                {
                    icWorkAreasAdd++;
                    rcWork[0] = rc;
                }

                if (icWorkAreas <= LV_MAX_WORKAREAS)
                {
                    int iRightOld;

                    // Now get the old work area rectangle and shrink it to our new width
                    ListView_GetWorkAreas(hwndLV, icWorkAreas, &rcWork);
                    iRightOld = rcWork[0].right;
                    rcWork[0].right = rcWork[0].left + icxWidth;
                    ListView_SetWorkAreas(hwndLV, icWorkAreas + icWorkAreasAdd, &rcWork);

                    // Weird case if there wasn't a workarea before, it doesn't bully the icons into
                    // the rectangle in that case.  So size it one pixel bigger so that it will
                    // actually move the icons.
                    if (icWorkAreasAdd)
                    {
                        rcWork[0].right++;
                        icxWidth++;
                        ListView_SetWorkAreas(hwndLV, icWorkAreas + icWorkAreasAdd, &rcWork);
                    }

                    // Finally restore the old work area
                    rcWork[0].right = iRightOld;
                    ListView_SetWorkAreas(hwndLV, icWorkAreas, &rcWork);

                    // Calculate the position and width of the split component so it fills
                    // the rest of the screen.
                    x = icxWidth;
                    cx = (rc.right - rc.left) - icxWidth;
                }
            } else {
                // Failure case, use 20% of the screen for the icons.
                x = (rc.right - rc.left) * 2 / 10;
                cx = (rc.right - rc.left) * 8 / 10;
            }

            y = 0;
            cy = rc.bottom - rc.top;

            m_ItemState = IS_SPLIT;
        }

        // We currently only allow 1 component to be either split or full screen, so
        // restore any other component that is currently in this state.
        _EnumElements(lpfnRestoreCB, NULL, 0);

        m_zIndexBottom -= 2;
        zIndex = m_zIndexBottom;
    }

    m_pistyleTarget->put_pixelLeft(x);
    m_pistyleTarget->put_pixelTop(y);
    m_pistyleTarget->put_pixelWidth(cx);
    m_pistyleTarget->put_pixelHeight(cy);

    varZ.vt = VT_I4;
    varZ.lVal = zIndex;
    m_pistyleTarget->put_zIndex(varZ);

    PersistTargetPosition(m_pielemTarget, x, y, cx, cy, zIndex, 
                            (BOOL)((dwOldItemState == IS_NORMAL) && (m_ItemState != IS_NORMAL)),
                            m_ItemState);

    return S_OK;
}


/************************************************************************\
    FUNCTION: CDeskMovr::_DisplayContextMenu

    PARAMETERS:
        x,y - Coordinates relative to the desktop window.
\************************************************************************/
HRESULT CDeskMovr::_DisplayContextMenu(int x, int y)
{
    HRESULT hr = S_OK;
    HMENU hmenuContext = LoadMenuPopup(MENU_DESKCOMP_CONTEXTMENU);

    TraceMsg(TF_CUSTOM2, "CDeskMovr::DisplayContextMenu(x = %d, y = %d), m_bstrTargetName=%ls", x, y, GEN_DEBUGSTRW(m_bstrTargetName));
    if (hmenuContext)
    {
        int nSelection;
        BOOL fSubscribe = FALSE;
        BOOL fRemoveSubscribe = FALSE;
        TCHAR szHTMLElementName[MAX_URL_STRING];

        if (SUCCEEDED(GetHTMLElementStrMember(m_pielemTarget, szHTMLElementName, SIZECHARS(szHTMLElementName), (BSTR)(s_sstrSubSRCMember.wsz))))
        {
            int nScheme = GetUrlScheme(szHTMLElementName);

            if ((URL_SCHEME_FILE == nScheme) || (URL_SCHEME_INVALID == nScheme))
                fRemoveSubscribe = TRUE;
        }

        // check to see if we need to turn some things off or on
        // Mainly because we are disabling features Admins don't want users to have.

        hr = IElemCheckForExistingSubscription(m_pielemTarget);
        if (fRemoveSubscribe || FAILED(hr))    // This object/thing cannot be subscribed to. (Channel Changer, Orenge Blob).
        {
            DeleteMenu(hmenuContext, IDM_DCCM_OFFLINE, MF_BYCOMMAND);
            DeleteMenu(hmenuContext, IDM_DCCM_SYNCHRONIZE, MF_BYCOMMAND);
            DeleteMenu(hmenuContext, IDM_DCCM_PROPERTIES, MF_BYCOMMAND);

            // Is the top item in the list a separator?
            if (-1 == GetMenuItemID(hmenuContext, 0))
            {
                // Yes, it is, so remove it.
                DeleteMenu(hmenuContext, 0, MF_BYPOSITION);
            }
        }
        else if (S_FALSE == hr)      // Not subscribed
        {
            DeleteMenu(hmenuContext, IDM_DCCM_SYNCHRONIZE, MF_BYCOMMAND);
            DeleteMenu(hmenuContext, IDM_DCCM_PROPERTIES, MF_BYCOMMAND);

            fSubscribe = TRUE;
        }
        else if (S_OK == hr)
        {
            if (SHRestricted2(REST_NoManualUpdates, NULL, 0))
                DeleteMenu(hmenuContext, IDM_DCCM_SYNCHRONIZE, MF_BYCOMMAND);

            CheckMenuItem(hmenuContext, IDM_DCCM_OFFLINE, MF_BYCOMMAND |MF_CHECKED);
        }

        if (SHRestricted(REST_NOCLOSEDESKCOMP))
            EnableMenuItem(hmenuContext, IDM_DCCM_CLOSE, MF_BYCOMMAND | MF_GRAYED);

        if (ISSPLIT)
            EnableMenuItem(hmenuContext, IDM_DCCM_SPLIT, MF_BYCOMMAND | MF_GRAYED);
        else if (ISFULLSCREEN)
            EnableMenuItem(hmenuContext, IDM_DCCM_FULLSCREEN, MF_BYCOMMAND | MF_GRAYED);
        else if (ISNORMAL)  // TODO - This item should toggle with a "Reset Size" item
            EnableMenuItem(hmenuContext, IDM_DCCM_RESTORE, MF_BYCOMMAND | MF_GRAYED);

        g_fIgnoreTimers |= IGNORE_CONTEXTMENU_UP;

        nSelection = TrackPopupMenu(hmenuContext, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, m_hwndParent, NULL);
        
        DestroyMenu(hmenuContext);

        m_CaptionState &= ~CS_MENUPUSHED;
        UpdateCaption(DMDC_MENU);

        switch (nSelection)
        {
            case IDM_DCCM_OFFLINE:
                if (fSubscribe)
                    hr = IElemSubscribeDialog(m_pielemTarget, m_hWnd);
                else
                    hr = IElemUnsubscribe(m_pielemTarget);
                break;

            case IDM_DCCM_SYNCHRONIZE:
                hr = IElemUpdate(m_pielemTarget);
                break;

            case IDM_DCCM_PROPERTIES:   // Subscriptions Dialog (Don't let the name fool you)
                TraceMsg(TF_CUSTOM2, "CDeskMovr::_DisplayContextMenu() IDM_DCCM_PROPERTIES m_bstrTargetName=%ls.", GEN_DEBUGSTRW(m_bstrTargetName));
                    hr = IElemGetSubscriptionsDialog(m_pielemTarget, m_hWnd);
                break;

            case IDM_DCCM_CUSTOMIZE:   // Show Display Control Panel set to Components Sheet
                TraceMsg(TF_CUSTOM2, "CDeskMovr::_DisplayContextMenu() IDM_DCCM_CUSTOMIZE m_bstrTargetName=%ls", GEN_DEBUGSTRW(m_bstrTargetName));
                hr = ShowComponentSettings();
                break;

            case IDM_DCCM_CLOSE:
                ASSERT(!SHRestricted(REST_NOCLOSEDESKCOMP));  // We should never be able to get here.
    
                TraceMsg(TF_CUSTOM2, "CDeskMovr::_DisplayContextMenu() IDM_DCCM_CLOSE m_bstrTargetName=%ls", GEN_DEBUGSTRW(m_bstrTargetName));
                hr = IElemCloseDesktopComp(m_pielemTarget);
                break;

            case IDM_DCCM_RESTORE:
            case IDM_DCCM_FULLSCREEN:
            case IDM_DCCM_SPLIT:
                hr = _HandleZoom(nSelection);
                break;

        }

        g_fIgnoreTimers &= ~IGNORE_CONTEXTMENU_UP;

        if (nSelection)
            DismissSelfNow();
    }

    return hr;
}

void CDeskMovr::_MapPoints(int * px, int * py)
{
    if (m_bWndLess)
    {
        *px -= m_left - (CAPTION_ONLY ? 0 : m_cxBorder);
        *py -= (CAPTION_ONLY ? 0 : (m_top - (m_cyBorder + m_cyCaption)));
    }
}

void CDeskMovr::_ChangeCapture(BOOL fSet)
{
    if (m_fCaptured != fSet)
    {
        m_fCaptured = fSet;
        if (fSet)
            g_fIgnoreTimers |= IGNORE_CAPTURE_SET;
        else
            g_fIgnoreTimers &= ~IGNORE_CAPTURE_SET;
    }
}


#endif
