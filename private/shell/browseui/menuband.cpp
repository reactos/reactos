#include "priv.h"
#include "sccls.h"
#include "menuband.h"
#include "itbar.h"
#include "bands.h"
#include "isfband.h"
#include "menubar.h"
#include "../lib/dpastuff.h"       // COrderList_*
#include "inpobj.h"
#include "theater.h"
#include "resource.h"
#include "oleacc.h"
#include "apithk.h"
#include "uemapp.h"
#include "mnbase.h"
#include "mnfolder.h"
#include "mnstatic.h"
#include "iaccess.h"

#include "mluisupp.h"

// BUGBUG (lamadio): Conflicts with one defined in winuserp.h
#undef WINEVENT_VALID       //It's tripping on this...
#include "winable.h"

#define DM_MISC     0               // miscellany

#define PF_USINGNTSD    0x00000400      // set this if you're debugging on ntsd

// This must be reset to -1 on any WM_WININICHANGE.  We do it in
// shbrows2.cpp, but if there are no browser windows open when the
// metric changes, we end up running around with a stale value.  Oh well.
long g_lMenuPopupTimeout = -1;

// {AD35F50A-0CC0-11d3-AE2D-00C04F8EEA99}
static const CLSID CLSID_MenuBandMetrics =
{ 0xad35f50a, 0xcc0, 0x11d3, { 0xae, 0x2d, 0x0, 0xc0, 0x4f, 0x8e, 0xea, 0x99
} };

// Registered window messages for the menuband
UINT    g_nMBPopupOpen = 0;
UINT    g_nMBFullCancel = 0;
UINT    g_nMBDragCancel = 0;
UINT    g_nMBAutomation = 0;
UINT    g_nMBExecute = 0;
UINT    g_nMBOpenChevronMenu = 0;
HCURSOR g_hCursorArrow = NULL;
//UINT    g_nMBIgnoreNextDeselect = 0;  // Dealt with in menuisf.cpp

HRESULT IUnknown_QueryServiceExec(IUnknown* punk, REFGUID guidService, const GUID *guid,
                                 DWORD cmdID, DWORD cmdParam, VARIANT* pvarargIn, VARIANT* pvarargOut);

BOOL IsAncestor(HWND hwndChild, HWND hwndAncestor)
{
    HWND hwnd = hwndChild;
    while (hwnd != hwndAncestor && hwnd != NULL)
    {
        hwnd = GetParent(hwnd);
    } 

    return hwndAncestor == hwnd;
}

//=================================================================
// Implementation of menuband message filter
//=================================================================

extern "C" void DumpMsg(LPCTSTR pszLabel, MSG * pmsg);


// Just one of these, b/c we only need one message filter
CMBMsgFilter g_msgfilter = { 0 };     

void CMBMsgFilter::SetModal(BOOL fModal)
{
    // There was an interesting problem:
    //   Click on the Chevron menu. Right click Delete. 
    //   The menus were hosed
    // Why?
    //   Well, I'll tell you:
    //   We got a deactivate on the subclassed window. We have
    //   2 menus subclassing it: The Main menu, and the modal
    //   chevron menu. Problem is, the main menu snagged the WM_ACTIVATE
    //   and does a set context. This causes a Pop and releases the Message hook.
    //   Since I still had a menu up, this caused havoc.
    //   So I introduced a concept of a "Modal" menuband. 
    //   This says: "Ignore any request to change contexts until I'm done". When
    //   that modal band is done, it sets the old context back in.
    //   Seems like a hack, but we need a better underlying archtecture for
    //   the message passing.
    _fModal = fModal;
}

void CMBMsgFilter::ReEngage(void* pvContext)      
{ 
    // We need to make sure that we don't dis/reengage when 
    // switching contexts
    if (pvContext == _pvContext)
        _fEngaged = TRUE; 
}

void CMBMsgFilter::DisEngage(void* pvContext)     
{ 
    if (pvContext == _pvContext)
        _fEngaged = FALSE;
}

int CMBMsgFilter::GetCount()
{
    return FDSA_GetItemCount(&_fdsa);
}

int MsgFilter_GetCount()
{
    return g_msgfilter.GetCount();
}

CMenuBand * CMBMsgFilter::_GetTopPtr(void)   
{ 
    CMenuBand * pmb = NULL;
    int cItems = FDSA_GetItemCount(&_fdsa);

    if (0 < cItems)
    {
        MBELEM * pmbelem = FDSA_GetItemPtr(&_fdsa, cItems-1, MBELEM); 
        pmb = pmbelem->pmb;
    }
    return pmb;
}

CMenuBand * CMBMsgFilter::_GetBottomMostSelected(void)
{
    // Ick, I can't believe I just did this. Mix COM and C++ identities... Yuck.
    CMenuBand* pmb = NULL;
    if (_pmb)
    {
        IUnknown_QueryService(SAFECAST(_pmb, IMenuBand*), SID_SMenuBandBottomSelected, CLSID_MenuBand, (void**)&pmb);

        // Since we have the C++ identity, release the COM identity.
        if (pmb)
            pmb->Release();
    }

    return pmb;
}


CMenuBand * CMBMsgFilter::_GetWindowOwnerPtr(HWND hwnd)   
{ 
    CMenuBand * pmb = NULL;
    int cItems = FDSA_GetItemCount(&_fdsa);

    if (0 < cItems)
    {
        // Go thru the list of bands on the stack and return the
        // one who owns the given window.
        int i;

        for (i = 0; i < cItems; i++)
        {
            MBELEM * pmbelem = FDSA_GetItemPtr(&_fdsa, i, MBELEM); 
            if (pmbelem->pmb && S_OK == pmbelem->pmb->IsWindowOwner(hwnd))
            {
                pmb = pmbelem->pmb;
                break;
            }
        }
    }
    return pmb;
}


/*----------------------------------------------------------
Purpose: Return menuband or NULL based upon hittest.  pt must be 
         in screen coords
*/
CMenuBand * CMBMsgFilter::_HitTest(POINT pt, HWND * phwnd)
{ 
    CMenuBand * pmb = NULL;
    HWND hwnd = NULL;
    int cItems = FDSA_GetItemCount(&_fdsa);

    if (0 < cItems)
    {
        // Go thru the list of bands on the stack and return the
        // one who owns the given window.  Work backwards since the
        // later bands are on top (z-order), if the menus ever overlap.
        int i = cItems - 1;

        while (0 <= i)
        {
            MBELEM * pmbelem = FDSA_GetItemPtr(&_fdsa, i, MBELEM); 

            RECT rc;

            // Do this dynamically because the hwndBar hasn't been positioned
            // until after this mbelem has been pushed onto the msg filter stack.
            GetWindowRect(pmbelem->hwndBar, &rc);
            
            if (PtInRect(&rc, pt))
            {
                pmb = pmbelem->pmb;
                hwnd = pmbelem->hwndTB;
                break;
            }
            i--;
        }
    }

    if (phwnd)
        *phwnd = hwnd;

    return pmb;
}


void CMBMsgFilter::RetakeCapture(void)
{
    // The TrackPopupMenu submenus can steal the capture.  Take
    // it back.  Don't take it back if the we're in edit mode,
    // because the modal drag/drop loop has the capture at that
    // point.
    // We do not want to take capture unless we are engaged. 
    // We need to do this because we are not handling mouse messages lower down
    // in the code. When we set the capture, the messages that we do not handle
    // trickle up to the top level menu, and can cause weird problems (Such
    // as signaling a "click out of bounds" or a context menu of the ITBar)
    if (_hwndCapture && !_fPreventCapture && _fEngaged)
    {
        TraceMsg(TF_MENUBAND, "CMBMsgFilter: Setting capture to %#lx", _hwndCapture);
        SetCapture(_hwndCapture);
    }
}    

void CMBMsgFilter::SetHook(BOOL fSet, BOOL fDontIgnoreSysChar)
{
    if (fDontIgnoreSysChar)
        _iSysCharStack += fSet? 1: -1;

    if (NULL == _hhookMsg && fSet)
    {
        TraceMsg(TF_MENUBAND, "CMBMsgFilter: Initialize");
        _hhookMsg = SetWindowsHookEx(WH_GETMESSAGE, GetMsgHook, HINST_THISDLL, GetCurrentThreadId());
        _fDontIgnoreSysChar = fDontIgnoreSysChar;
    }
    else if (!fSet && _iSysCharStack == 0)
    {
        TraceMsg(TF_MENUBAND, "CMBMsgFilter: Hook removed");
        UnhookWindowsHookEx(_hhookMsg);
        _hhookMsg = NULL;
    }
}

// 1) Set Deskbars on Both Monitors and set to chevron
// 2) On Monitor #2 open a chevron
// 3) On Monitor #1 open a chevron then open the Start Menu
// Result: Start Menu does not work.

// The reason is, we set the _fModal of the global message filter. This prevents context switches. Why? 
// The modal flag was invented to solve context switching problems with the browser frame. So what causes this?
// Well, when switching from #2 to #3, we have not switched contexts. But since we got a click out of bounds, we collapse
// the previous menu. When switching from #3 to #4, neither have the context, so things get messy.

void CMBMsgFilter::ForceModalCollapse()
{
    if (_fModal)
    {
        _fModal = FALSE;
        SetContext(NULL, TRUE);
    }
}

void CMBMsgFilter::SetContext(void* pvContext, BOOL fSet)
{
    TraceMsg(TF_MENUBAND, "CMBMsgFilter::SetContext from 0x%x to 0x%x", _pvContext, pvContext);
    // When changing a menuband context, we need to pop all of the items
    // in the stack. This is to prevent a race condition that can occur.

    // We do not want to pop all of the items off the stack if we're setting the same context.
    // We do a set context on Activation, Both when we switch from one Browser frame to another
    // but also when right clicking or causing the Rename dialog to be displayed.

    BOOL fPop = FALSE;

    if (_fModal)
        return;

    // Are we setting a new context?
    if (fSet)
    {
        // Is this different than the one we've got?
        if (pvContext != _pvContext)
        {
            // Yes, then we need to pop off all of the old items.
            fPop = TRUE;
        }

        _pvContext = pvContext;
    }
    else
    {
        // Then we are trying to unset the message hook. Make sure it still belongs to
        // this context
        if (pvContext == _pvContext)
        {
            // This context is trying to unset itself, and no other context owns it.
            // remove all the old items.
            fPop = TRUE;
        }
    }

    if (fPop)
    {
        CMenuBand* pcmb = _GetTopPtr();
        if (pcmb)
        {
            PostMessage(pcmb->_pmbState->GetSubclassedHWND(), g_nMBFullCancel, 0, 0);
            // No release.

            if (FDSA_GetItemCount(&_fdsa) != 0)
            {
                while (g_msgfilter.Pop(pvContext))
                    ;
            }
        }
    }
}




/*----------------------------------------------------------
Purpose: Push another menuband onto the message filter's stack

*/
void CMBMsgFilter::Push(void* pvContext, CMenuBand * pmb, IUnknown * punkSite)
{
    ASSERT(IS_VALID_CODE_PTR(pmb, CMenuBand));
    TraceMsg(TF_MENUBAND, "CMBMsgFilter::Push called from context 0x%x", pvContext);

    if (pmb && pvContext == _pvContext)
    {
        BOOL bRet = TRUE;
        HWND hwndBand;

        pmb->GetWindow(&hwndBand);

        // If the bar isn't available use the band window
        HWND hwndBar = hwndBand;
        IOleWindow * pow;

        IUnknown_QueryService(punkSite, SID_SMenuPopup, IID_IOleWindow, 
                              (LPVOID *)&pow);
        if (pow)
        {
            pow->GetWindow(&hwndBar);
            pow->Release();
        }

        if (NULL == _hhookMsg)
        {
            // We want to ignore the WM_SYSCHAR message in the message filter because
            // we are using the IsMenuMessage call instead of the global message hook.
            SetHook(TRUE, FALSE);
            TraceMsg(TF_MENUBAND, "CMBMsgFilter::push Setting hook from context 0x%x", pvContext);
            _fSetAtPush = TRUE;
        }

        if (!_fInitialized)
        {
            ASSERT(NULL == _hwndCapture);
            _hwndCapture = hwndBar;

            _fInitialized = TRUE;

            bRet = FDSA_Initialize(sizeof(MBELEM), CMBELEM_GROW, &_fdsa, _rgmbelem, CMBELEM_INIT);

            // We need to initialize this for the top level guy so that we have the correct positioning
            // from the start of this new set of bands. This is used to eliminate spurious WM_MOUSEMOVE
            // messages which cause problems. See _HandleMouseMessages for more information
            AcquireMouseLocation();
        }

        if (EVAL(bRet))
        {
            MBELEM mbelem = {0};
            
            TraceMsg(TF_MENUBAND, "CMBMsgFilter: Push (pmp:%#08lx) onto stack", SAFECAST(pmb, IMenuPopup *));
            pmb->AddRef();

            mbelem.pmb = pmb;
            mbelem.hwndTB = hwndBand;
            mbelem.hwndBar = hwndBar;

            FDSA_AppendItem(&_fdsa, &mbelem);

            CMenuBand* pmbTop = _GetTopPtr();
            ASSERT(pmbTop);

            if ((pmbTop->GetFlags() & SMINIT_LEGACYMENU) || NULL == GetCapture())
                RetakeCapture();
        }
        else
        {
            UnhookWindowsHookEx(_hhookMsg);
            _hhookMsg = NULL;
            _hwndCapture = NULL;
        }
    }
}    


/*----------------------------------------------------------
Purpose: Pop a menuband off the message filter stack

         Returns the number of bands left on the stack
*/
int CMBMsgFilter::Pop(void* pvContext)
{
    int nRet = 0;

    TraceMsg(TF_MENUBAND, "CMBMsgFilter::pop called from context 0x%x", pvContext);

    // This can be called from a context switch or when we're exiting menu mode,
    // so we'll switch off the fact that we clear _hhookMsg when we pop the top twice.
    if (pvContext == _pvContext && _hhookMsg)
    {
        int iItem = FDSA_GetItemCount(&_fdsa) - 1;
        MBELEM * pmbelem;

        ASSERT(0 <= iItem);

        pmbelem = FDSA_GetItemPtr(&_fdsa, iItem, MBELEM);
        if (EVAL(pmbelem->pmb))
        {
            TraceMsg(TF_MENUBAND, "CMBMsgFilter: Pop (pmb=%#08lx) off stack", SAFECAST(pmbelem->pmb, IMenuPopup *));
            pmbelem->pmb->Release();
            pmbelem->pmb = NULL;
        }
        FDSA_DeleteItem(&_fdsa, iItem);

        if (0 == iItem)
        {

            TraceMsg(TF_MENUBAND, "CMBMsgFilter::pop removing hook from context 0x%x", pvContext);
            if (_fSetAtPush)
                SetHook(FALSE, FALSE);

            PreventCapture(FALSE);
            _fInitialized = FALSE;

            if (_hwndCapture && GetCapture() == _hwndCapture)
            {
                TraceMsg(TF_MENUBAND, "CMBMsgFilter: Releasing capture");
                ReleaseCapture();
            }
            _hwndCapture = NULL;
        }
#ifdef UNIX
        else if (1 == iItem)
        {
            CMenuBand * pmb = _GetTopPtr();
            if( pmb )
            {
                // Change item count only if we delete successfully.
                if( pmb->RemoveTopLevelFocus() )
                    iItem = FDSA_GetItemCount(&_fdsa);
            }
        }
#endif
        nRet = iItem;
  
        
    }
    return nRet;
}    


LRESULT CMBMsgFilter::_HandleMouseMsgs(MSG * pmsg, BOOL bRemove)
{
    LRESULT lRet = 0;
    CMenuBand * pmb;
    HWND hwnd = GetCapture();

    // Do we still have the capture?
    if (hwnd != _hwndCapture)
    {
        // No; is it b/c a CTrackPopupBar has it?

#if 0 // Nuke this trace because I was getting annoyed.
        //def DEBUG
        pmb = _GetTopPtr();
        if (!EVAL(pmb) || !pmb->IsInSubMenu())
        {
            // No
            TraceMsg(TF_WARNING, "CMBMsgFilter: someone else has the capture (%#lx)", hwnd);
        }
#endif
        if (NULL == hwnd)
        {
            // There are times that we must retake the capture because
            // TrackPopupMenuEx has taken it, or some context menu
            // might have taken it, so take it back.
            RetakeCapture();
            TraceMsg(TF_WARNING, "CMBMsgFilter: taking the capture back");
        }
    }
    else
    {
        // Yes; decide what to do with it
        POINT pt;
        HWND hwndPt;
        MSG msgT;

        pt.x = GET_X_LPARAM(pmsg->lParam);
        pt.y = GET_Y_LPARAM(pmsg->lParam);
        ClientToScreen(pmsg->hwnd, &pt);

        if (WM_MOUSEMOVE == pmsg->message)
        {
            // The mouse cursor can send repeated WM_MOUSEMOVE messages
            // with the same coordinates.  When the user tries to navigate
            // thru the menus with the keyboard, and the mouse cursor
            // happens to be over a menu item, these spurious mouse 
            // messages cause us to think the menu has been invoked under
            // the mouse cursor.  
            //
            // To avoid this unpleasant rudeness, we eat any gratuitous
            // WM_MOUSEMOVE messages.
            if (_ptLastMove.x == pt.x && _ptLastMove.y == pt.y)
            {
                pmsg->message = WM_NULL;
                goto Bail;
            }

            // Since this is not a duplicate point, we need to keep it around. 
            // We will use this stored point for the above comparison
            AcquireMouseLocation();

            if (_hcurArrow == NULL)
                _hcurArrow = LoadCursor(NULL, IDC_ARROW);

            if (GetCursor() != _hcurArrow)
                SetCursor(_hcurArrow);

        }

        pmb = _HitTest(pt, &hwndPt);

        if (pmb)
        {
            // Forward mouse message onto appropriate menuband.  Note
            // the appropriate menuband's GetMsgFilterCB (below) will call
            // ScreenToClient to convert the coords correctly.

            // Use a stack variable b/c we don't want to confuse USER32
            // by changing the coords of the real message.
            msgT = *pmsg;
            msgT.lParam = MAKELPARAM(pt.x, pt.y);
            lRet = pmb->GetMsgFilterCB(&msgT, bRemove);

            // Remember the changed message (if there was one)
            pmsg->message = msgT.message;   
        }
        // Debug note: to debug menubands on ntsd, set the prototype
        // flag accordingly.  This will keep menubands from going
        // away the moment the focus changes to the NTSD window.

        else if ((WM_LBUTTONDOWN == pmsg->message || WM_RBUTTONDOWN == pmsg->message) &&
            !(g_dwPrototype & PF_USINGNTSD))
        {
            // Mouse down happened outside the menu.  Bail.
            pmb = _GetTopPtr();
            if (EVAL(pmb))
            {
                msgT.hwnd = pmsg->hwnd;
                msgT.message = g_nMBFullCancel;
                msgT.wParam = 0;
                msgT.lParam = 0;

                TraceMsg(TF_MENUBAND, "CMBMsgFilter (pmb=%#08lx): hittest outside, bailing", SAFECAST(pmb, IMenuPopup *));
                pmb->GetMsgFilterCB(&msgT, bRemove);
            }
#if 0
            // Now send the message to the originally intended window
            SendMessage(pmsg->hwnd, pmsg->message, pmsg->wParam, pmsg->lParam);
#endif
        }
        else
        {
            pmb = _GetTopPtr();
            if (pmb)
            {
                IUnknown_QueryServiceExec(SAFECAST(pmb, IOleCommandTarget*), SID_SMenuBandBottom, 
                    &CGID_MenuBand, MBANDCID_SELECTITEM, MBSI_NONE, NULL, NULL);
            }
        }
    }

Bail:
    return lRet;    
}    


/*----------------------------------------------------------
Purpose: Message hook used to track keyboard and mouse messages
         while the menuband is "active".  
         
         The menuband can't steal the focus away -- we use this
         hook to catch messages.

*/
LRESULT CMBMsgFilter::GetMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;
    MSG * pmsg = (MSG *)lParam;
    BOOL bRemove = (PM_REMOVE == wParam);


    // The global message filter may be in a state when we are not processing messages,
    // but the menubands are still displayed. A situation where this will occur is when
    // a dialog box is displayed because of an interaction with the menus.

    // Are we engaged? (Are we allowed to process messages?)
    if (g_msgfilter._fEngaged)
    {
        if (WM_SYSCHAR == pmsg->message)
        {
            // _fDontIgnoreSysChar is set when the Menubands ONLY want to know about
            // WM_SYSCHAR and nothing else.
            if (g_msgfilter._fDontIgnoreSysChar)
            {
                CMenuBand * pmb = g_msgfilter.GetTopMostPtr();
                if (pmb)
                    lRet =  pmb->GetMsgFilterCB(pmsg, bRemove);
            }
        }
        else if (g_msgfilter._fInitialized) // Only filter if we are initalized (have items on the stack)
        {
            switch (nCode)
            {
            case HC_ACTION:
        #ifdef DEBUG
                if (g_dwDumpFlags & DF_GETMSGHOOK)
                    DumpMsg(TEXT("GetMsg"), pmsg);
        #endif

                // A lesson about GetMsgHook: it gets the same message
                // multiple times for as long as someone calls PeekMessage
                // with the PM_NOREMOVE flag.  So we want to take action 
                // only when PM_REMOVE is set (so we don't handle more than
                // once).  If we modify any messages to redirect them (on a
                // regular basis), we must modify all the time so we don't 
                // confuse the app.

                // Messages get redirected to different bands in the stack
                // in this way:
                //
                //  1) Keyboard messages go to the currently open submenu 
                //     (topmost on the stack).  
                //
                //  2) The PopupOpen message goes to the hwnd that belongs 
                //     to the menu band (via IsWindowOwner).
                //

                switch (pmsg->message)
                {
                case WM_SYSKEYDOWN:
                case WM_KEYDOWN:
                case WM_CHAR:
                case WM_KEYUP:
                case WM_CLOSE:          // only this message filter gets WM_CLOSE
                    {
                        // There is a situation that can occur when the last selected
                        // menu pane is NOT the bottom most pane.
                        // We need to see if that last selected guy is tracking a context
                        // menu so that we forward the messages correctly.
                        CMenuBand * pmb = g_msgfilter._GetBottomMostSelected();
                        if (pmb)
                        {
                            // Is it tracking a context menu?
                            if (S_OK == IUnknown_Exec(SAFECAST(pmb, IMenuBand*), &CGID_MenuBand, 
                                MBANDCID_ISTRACKING, 0, NULL, NULL))
                            {
                                // Yes, forward for proper handling.
                                lRet = pmb->GetMsgFilterCB(pmsg, bRemove);
                            }
                            else
                            {
                                // No; Then do the default processing. This can happen if there is no
                                // context menu, but there is a selected parent and not a selected child.
                                goto TopHandler;
                            }
                        }
                        else
                        {
                    TopHandler:
                            pmb = g_msgfilter._GetTopPtr();
                            if (pmb)
                                lRet = pmb->GetMsgFilterCB(pmsg, bRemove);
                        }
                    }
                    break;

                case WM_NULL:
                    // Handle this here (we do nothing) to avoid mistaking this for
                    // g_nMBPopupOpen below, in case g_nMBPopupOpen is 0 if
                    // RegisterWindowMessage fails.
                    break;

                default:
                    if (bRemove && IsInRange(pmsg->message, WM_MOUSEFIRST, WM_MOUSELAST))
                    {
                        lRet = g_msgfilter._HandleMouseMsgs(pmsg, bRemove);
                    }
                    else if (pmsg->message == g_nMBPopupOpen)
                    {
                        CMenuBand * pmb = g_msgfilter._GetWindowOwnerPtr(pmsg->hwnd);
                        if (pmb)
                            lRet = pmb->GetMsgFilterCB(pmsg, bRemove);
                    }
                    else if (pmsg->message == g_nMBExecute)
                    {
                        CMenuBand * pmb = g_msgfilter._GetWindowOwnerPtr(pmsg->hwnd);
                        if (pmb)
                        {
                            VARIANT var;
                            var.vt = VT_UINT_PTR;
                            var.ullVal = (UINT_PTR)pmsg->hwnd;
                            pmb->Exec(&CGID_MenuBand, MBANDCID_EXECUTE, (DWORD)pmsg->wParam, &var, NULL);
                        }
                    }

                    break;
                }
                break;

            default:
                if (0 > nCode)
                    return CallNextHookEx(g_msgfilter._hhookMsg, nCode, wParam, lParam);
                break;
            }
        }
    }

    // Pass it on to the next hook in the chain
    if (0 == lRet)
        return CallNextHookEx(g_msgfilter._hhookMsg, nCode, wParam, lParam);

    return 0;       // Always return 0
}    



//=================================================================
// Implementation of CMenuBand
//=================================================================

// Struct used by EXEC with a MBANDCID_GETFONTS to return fonts
typedef struct tagMBANDFONTS
{
    HFONT hFontMenu;    // [out]    TopLevelMenuBand's menu font
    HFONT hFontArrow;   // [out]    TopLevelMenuBand's font for drawing the cascade arrow
    int   cyArrow;      // [out]    Height of TopLevelMenuBand's cascade arrow
    int   cxArrow;      // [out]    Width of TopLevelMenuBand's cascade arrow
    int   cxMargin;     // [out]    Margin b/t text and arrow
} MBANDFONTS;

#define THISCLASS CMenuBand
#define SUPERCLASS CToolBand

#ifdef DEBUG
int g_nMenuLevel = 0;

#define DBG_THIS    _nMenuLevel, SAFECAST(this, IMenuPopup *)
#else
#define DBG_THIS    0, 0
#endif


CMenuBand::CMenuBand() :
    SUPERCLASS()
{
    _fCanFocus = TRUE;

    _fAppActive = TRUE;

    _nItemNew = -1;
    _nItemCur = -1;
    _nItemTimer = -1;
    _uIconSize = ISFBVIEWMODE_SMALLICONS;
    _uIdAncestor = ANCESTORDEFAULT;
    _nItemSubMenu = -1;
}


// The purpose of this method is to finish initializing Menubands, 
// since it can be initialized in many ways. 

void CMenuBand::_Initialize(DWORD dwFlags)
{
    _fVertical = !BOOLIFY(dwFlags & SMINIT_HORIZONTAL);
    _fTopLevel = BOOLIFY(dwFlags & SMINIT_TOPLEVEL);

    _dwFlags = dwFlags;

    // We cannot have a horizontal menu if it is not the toplevel menu
    ASSERT(!_fVertical && _fTopLevel || _fVertical);

    if (_fTopLevel)
    {
        if (!g_nMBPopupOpen) 
        {
            g_nMBPopupOpen  = RegisterWindowMessage(TEXT("CMBPopupOpen"));
            g_nMBFullCancel = RegisterWindowMessage(TEXT("CMBFullCancel"));
            g_nMBDragCancel = RegisterWindowMessage(TEXT("CMBDragCancel"));
            g_nMBAutomation = RegisterWindowMessage(TEXT("CMBAutomation"));
            g_nMBExecute    = RegisterWindowMessage(TEXT("CMBExecute"));
            g_nMBOpenChevronMenu = RegisterWindowMessage(TEXT("CMBOpenChevronMenu"));

            g_hCursorArrow = LoadCursor(NULL, IDC_ARROW);
            TraceMsg(TF_MENUBAND, "CMBPopupOpen message = %#lx", g_nMBPopupOpen);
            TraceMsg(TF_MENUBAND, "CMBFullCancel message = %#lx", g_nMBFullCancel);
        }

        if (!_pmbState)
            _pmbState = new CMenuBandState;
    }

    DEBUG_CODE( _nMenuLevel = -1; )
}


CMenuBand::~CMenuBand()
{
    // the message filter does not have a ref'd pointer to us!!!
    if (g_msgfilter.GetTopMostPtr() == this)
        g_msgfilter.SetTopMost(NULL);

    _CallCB(SMC_DESTROY);
    ATOMICRELEASE(_psmcb);

    // Cleanup
    CloseDW(0);

    if (_pmtbMenu)
        delete _pmtbMenu;

    if (_pmtbShellFolder)
        delete _pmtbShellFolder;

    
    ASSERT(_punkSite == NULL);
    ATOMICRELEASE(_pmpTrackPopup);

    ATOMICRELEASE(_pmbm);

    if (_fTopLevel)
    {
        if (_pmbState)
            delete _pmbState;
    }
}


/*----------------------------------------------------------
Purpose: Create-instance function for class factory

*/
HRESULT CMenuBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hres = E_OUTOFMEMORY;
    CMenuBand* pObj = new CMenuBand();
    if (pObj)
    {
        *ppunk = SAFECAST(pObj, IShellMenu*);
        hres = S_OK;
    }

    return hres;
}

/*----------------------------------------------------------
Purpose: Internal create-instance function

*/
CMenuBand * CMenuBand_Create(IShellFolder* psf, LPCITEMIDLIST pidl,
                             BOOL bHorizontal)
{
    CMenuBand * pmb = NULL;

    if (psf || pidl)
    {
        DWORD dwFlags = bHorizontal ? (SMINIT_HORIZONTAL | SMINIT_TOPLEVEL) : 0;

        pmb = new CMenuBand();
        if (pmb)
        {
            pmb->_Initialize(dwFlags);
            pmb->SetShellFolder(psf, pidl, NULL, 0);
        }
    }
    return pmb;
}
#ifdef UNIX

BOOL CMenuBand::RemoveTopLevelFocus()
{
    if( _fTopLevel )
    {
        _CancelMode( MPOS_CANCELLEVEL );
        return TRUE;
    }

    return FALSE;
}

#endif
void CMenuBand::_UpdateButtons()
{
    if (_pmtbMenu) 
        _pmtbMenu->v_UpdateButtons(FALSE);
    if (_pmtbShellFolder)
        _pmtbShellFolder->v_UpdateButtons(FALSE);

    _fForceButtonUpdate = FALSE;
}

HRESULT CMenuBand::ForwardChangeNotify(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // Given a change notify from the ShellFolder child, we will forward that notify to each of our
    // sub menus, but only if they have a shell folder child.

    HRESULT hres = E_FAIL;
    BOOL fDone = FALSE;
    CMenuToolbarBase* pmtb = _pmtbBottom;   // Start With the bottom toolbar. This is
                                            // is an optimization because typically
                                            // menus that have both a Shell Folder portion
                                            // and a static portion have the majority
                                            // of the change activity in the bottom portion.

    // This can be NULL on a shutdown, when we're deregistering change notifies
    if (pmtb)
    {
        HWND hwnd = pmtb->_hwndMB;


        for (int iButton = 0; !fDone; iButton++)
        {
            IShellChangeNotify* ptscn;

            int idCmd = GetButtonCmd(hwnd, iButton);

#ifdef DEBUG
                TCHAR szSubmenuName[MAX_PATH];
                SendMessage(hwnd, TB_GETBUTTONTEXT, idCmd, (LPARAM)szSubmenuName);
                TraceMsg(TF_MENUBAND, "CMenuBand: Forwarding Change notify to %s", szSubmenuName);
#endif

            // If it's not a seperator, see if there is a sub menu with a shell folder child.
            if (idCmd != -1 &&
                SUCCEEDED(pmtb->v_GetSubMenu(idCmd, &SID_MenuShellFolder, IID_IShellChangeNotify, (void**)&ptscn)))
            {
                IShellMenu* psm;
                // Don't forward this notify if the sub menu has specifically registered for change notify (By not passing
                // DontRegisterChangeNotify.
                if (SUCCEEDED(ptscn->QueryInterface(IID_IShellMenu, (void**)&psm)))
                {
                    UINT uIdParent = 0;
                    DWORD dwFlags = 0;
                    // Get the flags
                    psm->GetShellFolder(&dwFlags, NULL, IID_NULL, NULL);
                    psm->GetMenuInfo(NULL, &uIdParent, NULL, NULL);

                    // If this menupane is an "Optimized" pane, (meaning that we don't register for change notify
                    // and forward from a top level menu down) then we want to forward. We also
                    // forward if this is a child of Menu Folder. If it is a child,
                    // then it also does not register for change notify, but does not explicitly set it in it's flags
                    // (review: Should we set it in it's flags?)
                    // If it is not an optimized pane, then don't forward.
                    if ((dwFlags & SMSET_DONTREGISTERCHANGENOTIFY) ||
                        uIdParent == MNFOLDER_IS_PARENT)
                    {
                        // There is!, then pass to the child the change.
                        hres = ptscn->OnChange(lEvent, pidl1, pidl2);

                        // Update Dir on a Recursive change notify forces us to update everyone... Good thing
                        // this does not happen alot and is caused by user interaction most of the time.
                    }
                    psm->Release();
                }

                ptscn->Release();
            }

            // Did we go through all of the buttons on this toolbar?
            if (iButton >= ToolBar_ButtonCount(hwnd) - 1)
            {
                // Yes, then we need to switch to the next toolbar.
                if (_pmtbTop != _pmtbBottom && pmtb != _pmtbTop)
                {
                    pmtb = _pmtbTop;
                    hwnd = pmtb->_hwndMB;
                    iButton = -1;       // -1 because at the end of the loop the for loop will increment.
                }
                else
                {
                    // No; Then we must be done.
                    fDone = TRUE;
                }
            }
        }
    }
    else
        hres = S_OK;        // Return success because we're shutting down.

    return hres;
}

// Resize the parent menubar
VOID CMenuBand::ResizeMenuBar()
{
    // If we're not shown, then we do not need to do any kind of resize.
    // NOTE: Horizontal menubands are always shown. Don't do any of the 
    // vertical stuff if we're horizontal.
    if (!_fShow)
        return;

    // If we're horizontal, don't do any Vertical sizing stuff.
    if (!_fVertical)
    {
        // BandInfoChanged is only for Horizontal Menubands.
        _BandInfoChanged();
        return;
    }

    // We need to update the buttons before a resize so that the band is the right size.
    _UpdateButtons();

    // Have the menubar think about changing its height
    IUnknown_QueryServiceExec(_punkSite, SID_SMenuPopup, &CGID_MENUDESKBAR, 
        MBCID_RESIZE, 0, NULL, NULL);
}


STDMETHODIMP CMenuBand::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CMenuBand, IMenuPopup),
        QITABENT(CMenuBand, IMenuBand),
        QITABENT(CMenuBand, IShellMenu),
        QITABENT(CMenuBand, IWinEventHandler),
        QITABENT(CMenuBand, IShellMenuAcc),
        { 0 },
    };

    hres = QISearch(this, (LPCQITAB)qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);


    // BUGBUG (lamadio) 8.5.98: Nuke this. We should not expose the this pointer,
    // this is a bastardization of COM.
    if (FAILED(hres) && IsEqualGUID(riid, CLSID_MenuBand)) 
    {
        AddRef();
        *ppvObj = (LPVOID)this;
        hres = S_OK;
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: IServiceProvider::QueryService method

*/
STDMETHODIMP CMenuBand::QueryService(REFGUID guidService,
                                     REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL; // assume error

    if (IsEqualIID(guidService, SID_SMenuPopup) || 
        IsEqualIID(guidService, SID_SMenuBandChild) || 
        IsEqualIID(guidService, SID_SMenuBandParent) || 
        (_fTopLevel && IsEqualIID(guidService, SID_SMenuBandTop)))
    {
        if (IsEqualIID(riid, IID_IAccessible) || IsEqualIID(riid, IID_IDispatch))
        {
            hres = E_OUTOFMEMORY;
            CAccessible* pacc = new CAccessible(SAFECAST(this, IMenuBand*));

            if (pacc)
            {
                hres = pacc->InitAcc();
                if (SUCCEEDED(hres))
                {
                    hres = pacc->QueryInterface(riid, ppvObj);
                }
                pacc->Release();
            }
        }
        else
            hres = QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(guidService, SID_SMenuBandBottom) ||
             IsEqualIID(guidService, SID_SMenuBandBottomSelected))
    {
        // SID_SMenuBandBottom queries down
        BOOL fLookingForSelected = IsEqualIID(SID_SMenuBandBottomSelected, guidService);

        // Are we the leaf node?
        if (!_fInSubMenu)
        {
            if ( fLookingForSelected && 
                (_pmtbTracked == NULL ||
                 ToolBar_GetHotItem(_pmtbTracked->_hwndMB) == -1))
            {
                hres = E_FAIL;
            }
            else
            {
                hres = QueryInterface(riid, ppvObj);    // Yes; QI ourselves
            }
        }
        else 
        {
            // No; QS down...

            IMenuPopup* pmp = _pmpSubMenu;
            if (_pmpTrackPopup)
                pmp = _pmpTrackPopup;
            
            ASSERT(pmp);
            hres = IUnknown_QueryService(pmp, guidService, riid, ppvObj);
            if (FAILED(hres) && fLookingForSelected && _pmtbTracked != NULL)
            {
                hres = QueryInterface(riid, ppvObj);    // Yes; QI ourselves
            }
        }
    }
    else if (IsEqualIID(guidService, SID_MenuShellFolder))
    {
        // This is a method of some other menu in the scheme to get to specifically the MenuShellfolder,
        // This is for the COM Identity property.
        if (_pmtbShellFolder)
            hres = _pmtbShellFolder->QueryInterface(riid, ppvObj);
    }
    else
        hres = SUPERCLASS::QueryService(guidService, riid, ppvObj);

    return hres;
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::IsWindowOwner method

*/
STDMETHODIMP CMenuBand::IsWindowOwner(HWND hwnd)
{
    if (( _pmtbShellFolder && (_pmtbShellFolder->IsWindowOwner(hwnd) == S_OK) ) ||
        (_pmtbMenu && (_pmtbMenu->IsWindowOwner(hwnd) == S_OK)))
        return S_OK;
    return S_FALSE;
}

#define MB_EICH_FLAGS (EICH_SSAVETASKBAR | EICH_SWINDOWMETRICS | EICH_SPOLICY | EICH_SSHELLMENU | EICH_KWINPOLICY)

/*----------------------------------------------------------
Purpose: IWinEventHandler::OnWinEvent method

         Processes messages passed on from the bandsite.
*/
STDMETHODIMP  CMenuBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    HRESULT hres = NOERROR;

    EnterModeless();

    // Could our metrics be changing?  (We keep track of this only for the 
    // toplevel menu)
    BOOL fProcessSettingChange = FALSE;

    switch (uMsg)
    {
    case WM_SETTINGCHANGE:
        fProcessSettingChange = !lParam || (SHIsExplorerIniChange(wParam, lParam) & MB_EICH_FLAGS);
        break;

    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        fProcessSettingChange = TRUE;
        break;
    }

    if (_fTopLevel && 
        fProcessSettingChange && 
        _pmbState && !_pmbState->IsProcessingChangeNotify())
    {

        // There is a race condition that can occur during a refresh 
        // that's really nasty. It causes another one to get pumped in the
        // middle of processing this one, Yuck!
        _pmbState->PushChangeNotify();
        // There is a race condiction that can occur when the menuband is created,
        // but not yet initialized. This has been hit by the IEAK group....
        if (_pmtbTop)
        {
            // Yes; create a new metrics object and tell the submenus
            // about it.

            CMenuBandMetrics* pmbm = new CMenuBandMetrics(_pmtbTop->_hwndMB);

            if (pmbm)
            {
                ATOMICRELEASE(_pmbm);
                _pmbm = pmbm;

                if (_pmtbMenu)
                    _pmtbMenu->SetMenuBandMetrics(_pmbm);

                if (_pmtbShellFolder)
                    _pmtbShellFolder->SetMenuBandMetrics(_pmbm);

                _CallCB(SMC_REFRESH, wParam, lParam);

                // We need to force a button update at some point so that the new sizes are calculated
                // Setting this flag will cause the buttons to be updatted before the next time it 
                // is shown. If, however, the menu is currently displayed, then the ResizeMenuBar will
                // recalculate immediatly.
                
                _fForceButtonUpdate = TRUE;
                RECT rcOld;
                RECT rcNew;

                // Resize the MenuBar
                GetClientRect(_hwndParent, &rcOld);
                ResizeMenuBar();
                GetClientRect(_hwndParent, &rcNew);

                // If the rect sizes haven't changed, then we need to re-layout the
                // band because the button widths may have changed.
                if (EqualRect(&rcOld, &rcNew) && _fVertical)
                    _pmtbTop->NegotiateSize();
            }
        }

        if (_pmtbMenu)
            hres = _pmtbMenu->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);

        if (_pmtbShellFolder)
            hres = _pmtbShellFolder->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);

        _pmbState->PopChangeNotify();
    }
    else
    {
        if (_pmtbMenu && (_pmtbMenu->IsWindowOwner(hwnd) == S_OK) )
            hres = _pmtbMenu->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);

        if (_pmtbShellFolder && (_pmtbShellFolder->IsWindowOwner(hwnd) == S_OK) )
            hres = _pmtbShellFolder->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
    }

    ExitModeless();

    return hres;
}


/*----------------------------------------------------------
Purpose: IOleWindow::GetWindow method

*/
STDMETHODIMP CMenuBand::GetWindow(HWND * phwnd)
{
    // We assert that only a real menu can be hosted in a standard
    // bandsite (we're talking about the horizontal menubar).
    // So we only return the window associated with the static
    // menu.
    if (_pmtbMenu)
    {
        *phwnd = _pmtbMenu->_hwndMB;
        return NOERROR;
    }
    else
    {
        *phwnd = NULL;
        return E_FAIL;
    }
}    


/*----------------------------------------------------------
Purpose: IOleWindow::ContextSensitiveHelp method

*/
STDMETHODIMP CMenuBand::ContextSensitiveHelp(BOOL bEnterMode)
{
    return SUPERCLASS::ContextSensitiveHelp(bEnterMode);
}    


/*----------------------------------------------------------
Purpose: Handle WM_CHAR for accelerators

         This is handled for any vertical menu.  Since we have
         two toolbars (potentially), this function determines
         which toolbar gets the message depending on the 
         accelerator.

*/
HRESULT CMenuBand::_HandleAccelerators(MSG * pmsg)
{
    TCHAR ch = (TCHAR)pmsg->wParam;
    HWND hwndTop = _pmtbTop->_hwndMB;
    HWND hwndBottom = _pmtbBottom->_hwndMB;

    // Here's how this works: the menu can have one or two toolbars.
    // 
    // One toolbar: we simply forward the message onto the toolbar 
    // and let it handle any potential accelerators.
    //
    // Two toolbars: get the count of accelerators that match the
    // given char for each toolbar.  If only one toolbar has at
    // least one match, forward the message onto that toolbar.
    // Otherwise, forward the message onto the currently tracked
    // toolbar and let it negotiate which accelerator button to
    // choose (we might get a TBN_WRAPHOTITEM).
    //
    // If no match occurs, we beep.  Beep beep.
    //

    if (!_pmtbTracked)
        SetTracked(_pmtbTop);

    ASSERT(_pmtbTracked);

    if (_pmtbTop != _pmtbBottom)
    {
        int iNumBottomAccel;
        int iNumTopAccel;

        // Tell the dup handler not to handle this one....
        _fProcessingDup = TRUE;

        ToolBar_HasAccelerator(hwndTop, ch, &iNumTopAccel);
        ToolBar_HasAccelerator(hwndBottom, ch, &iNumBottomAccel);

        BOOL bBottom = (0 < iNumBottomAccel);
        BOOL bTop = (0 < iNumTopAccel);

        // Does one or the other (but not both) have an accelerator?
        if (bBottom ^ bTop)
        {
            // Yes; do the work here for that specific toolbar
            HWND hwnd = bBottom ? hwndBottom : hwndTop;
            int cAccel = bBottom ? iNumBottomAccel : iNumTopAccel;
            int idCmd;

            pmsg->message = WM_NULL;    // no need to forward the message

            // This should never really fail since we just checked
            EVAL( ToolBar_MapAccelerator(hwnd, ch, &idCmd) );

            DWORD dwFlags = HICF_ACCELERATOR | HICF_RESELECT;

            if (cAccel == 1)
                dwFlags |= HICF_TOGGLEDROPDOWN;

            int iPos = ToolBar_CommandToIndex(hwnd, idCmd);
            ToolBar_SetHotItem2(hwnd, iPos, dwFlags);
        }
        // No; were there no accelerators?
        else if ( !bTop )
        {
            // Yes
            if (_fVertical)
            {
                MessageBeep(MB_OK);
            }
            else
            {
                _CancelMode(MPOS_FULLCANCEL);
            }
        }
        // Else allow the message to go to the top toolbar

        _fProcessingDup = FALSE;
    }

    return NOERROR;
}


/*----------------------------------------------------------
Purpose: Callback for the get message filter.  We handle the
         keyboard messages here (rather than IInputObject::
         TranslateAcceleratorIO) so that we can redirect the
         message *and* have the message pump still call
         TranslateMessage to generate WM_CHAR and WM_SYSCHAR
         messages.

*/
LRESULT CMenuBand::GetMsgFilterCB(MSG * pmsg, BOOL bRemove)
{
    // (See the note in CMBMsgFilter::GetMsgHook about bRemove.)

    if (bRemove && !_fVertical && (pmsg->message == g_nMBPopupOpen) && _pmtbTracked)
    {
        // Menu is being popped open, send a WM_MENUSELECT equivalent.
        _pmtbTracked->v_SendMenuNotification((UINT)pmsg->wParam, FALSE);
    }

    if (_fTopLevel &&                           // Only do this for the top level
        _dwFlags & SMINIT_USEMESSAGEFILTER &&   // They want to use the message filter 
                                                // instead of IsMenuMessage
        bRemove &&                              // Only do this if we're removing it.
        WM_SYSCHAR == pmsg->message)            // We only care about WM_SYSCHAR
    {
        // We intercept Alt-key combos (when pressed together) here,
        // to prevent USER from going into a false menu loop check.  
        // There are compatibility problems if we let that happen.
        //
        // Sent by USER32 when the user hits an Alt-char combination.
        // We need to translate this into popping down the correct
        // menu.  Normally we intercept this in the message pump
        //
        if (_OnSysChar(pmsg, TRUE) == S_OK)
        {
            pmsg->message = WM_NULL;
        }
    }

    // If a user menu is up, then we do not want to intercept those messages. Intercepting
    // messages intended for the poped up user menu causes havoc with keyboard accessibility.
    // We also don't want to process messages if we're displaying a sub menu (It should be
    // handling them).

    BOOL fTracking = FALSE;
    if (_pmtbMenu)
        fTracking = _pmtbMenu->v_TrackingSubContextMenu();

    if (_pmtbShellFolder && !fTracking)
        fTracking = _pmtbShellFolder->v_TrackingSubContextMenu();


    if (!_fInSubMenu && !fTracking)    
    {
        // We don't process these messages when we're in a (modal) submenu

        switch (pmsg->message)
        {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            // Don't process IME message. Restore original VK value.
            if (g_fRunOnFE && VK_PROCESSKEY == pmsg->wParam)
                pmsg->wParam = ImmGetVirtualKey(pmsg->hwnd);

            if (bRemove && 
                (VK_ESCAPE == pmsg->wParam || VK_MENU == pmsg->wParam))
            {
                TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Received Esc in msg filter", DBG_THIS);

                DWORD dwSelect = (VK_ESCAPE == pmsg->wParam) ? MPOS_CANCELLEVEL : MPOS_FULLCANCEL;

                _CancelMode(dwSelect);

                pmsg->message = WM_NULL;
                return 1;
            }
            // Fall thru

        case WM_CHAR:
            // Hitting the spacebar should invoke the system menu
            if (!_fVertical && 
                WM_CHAR == pmsg->message && TEXT(' ') == (TCHAR)pmsg->wParam)
            {
                // We need to leave this modal loop before bringing
                // up the system menu (otherwise the user would need to 
                // hit Alt twice to get out.)  Post the message.
                TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Leaving menu mode for system menu", DBG_THIS);

                UIActivateIO(FALSE, NULL);

                // Say the Alt-key is down to catch DefWindowProc's attention
                pmsg->lParam |= 0x20000000;   
                pmsg->message = WM_SYSCHAR;

                // Use the parent of the toolbar, because toolbar does not
                // forward WM_SYSCHAR onto DefWindowProc.
                pmsg->hwnd = GetParent(_pmtbTop->_hwndMB);
                return 1;
            }
            else if (_fVertical && WM_CHAR == pmsg->message &&
                pmsg->wParam != VK_RETURN)
            {
#ifdef UNICODE
                // Need to do this before we ask the toolbars..
                // [msadek], On win9x we get the message thru a chain from explorer /iexplore (ANSI app.).
                // and pass it to comctl32 (Unicode) so it will fail to match the hot key.
                // the system sends the message with ANSI char and we treated it as Unicode.
                // It looks like noone is affected with this bug (US, FE) since they have hot keys always in Latin.
                // Bidi platforms are affected since they do have hot keys in native language.
                if(!g_fRunningOnNT)
                {
                    char szCh[2];
                    WCHAR wszCh[2];
                    szCh[0] = (BYTE)pmsg->wParam;
                    szCh[1] = '\0';

                    if(MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szCh, ARRAYSIZE(szCh),
                                                   wszCh, ARRAYSIZE(wszCh)))
                    {
                        memcpy(&(pmsg->wParam), wszCh, sizeof(WCHAR));
                    }
            
                }
#endif // UNICODE        


                // We do not want to pass VK_RETURN to _HandleAccelerators
                // because it will try to do a character match. When it fails
                // it will beep. Then we pass the VK_RETURN to the tracked toolbar
                // and it executes the command.

                // Handle accelerators here
                _HandleAccelerators(pmsg);
            }
            // Fall thru

        case WM_KEYUP:
            // Collection point for most key messages...

            if (NULL == _pmtbTracked)
            {
                // Normally we default to the top toolbar, unless that toolbar
                // cannot receive the selection (As is the case on the top level
                // start menu where the fast items are (Empty).                

                // Can the top toolbar be cycled into?
                if (!_pmtbTop->DontShowEmpty())
                {
                    // Yes;
                    SetTracked(_pmtbTop);      // default to the top toolbar
                }
                else
                {
                    // No; Set the tracked to the bottom, and hope that he can....
                    SetTracked(_pmtbBottom);
                }
            }

            // F10 has special meaning for menus. 
            //  - F10 alone, should toggle the selection of the first item 
            //      in a horizontal menu
            //  - Shift-F10 should display a context menu.

            if (VK_F10 == pmsg->wParam)
            {
                // Is this the Shift-F10 Case?
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    // Yes. We need to force this message into a context menu
                    // message.
                    pmsg->message = WM_CONTEXTMENU;
                    pmsg->lParam = -1;
                    pmsg->wParam = (WPARAM)_pmtbTracked->_hwndMB;
                    return 0;
                }
                else if (!_fVertical)   //No; Then we need to toggle in the horizontal case
                {
                    // Set the hot item to the first one.
                    int iHot = 0;
                    if (ToolBar_GetHotItem(_pmtbMenu->_hwndMB) != -1)
                        iHot = -1;  // We're toggling the selection off.
                    ToolBar_SetHotItem(_pmtbMenu->_hwndMB, iHot);
                    return 0;
                }
            }
                
            // Redirect to the toolbar
            pmsg->hwnd = _pmtbTracked->_hwndMB;
            return 0;

        case WM_NULL:
            // Handle this here (we do nothing) to avoid mistaking this for
            // g_nMBPopupOpen below, in case g_nMBPopupOpen is 0 if
            // RegisterWindowMessage fails.
            return 0;

        default:
            // We used to handle g_nMBPopupOpen here.  But we can't because calling TrackPopupMenu
            // (via CTrackPopupBar::Popup) w/in a GetMessageFilter is very bad.
            break;
        }
    }

    if (bRemove)
    {
        // These messages must be processed even when no submenu is open
        switch (pmsg->message)
        {
        case WM_CLOSE:
            // Being deactivated.  Bail out of menus.
            TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): sending MPOS_FULLCANCEL", DBG_THIS);

            _CancelMode(MPOS_FULLCANCEL);
            break;

        default:
            if (IsInRange(pmsg->message, WM_MOUSEFIRST, WM_MOUSELAST))
            {
                // If we move the mouse, collapse the tip. Careful not to blow away a balloon tip...
                if (_pmbState)
                    _pmbState->HideTooltip(FALSE);

                if (_pmtbShellFolder)
                    _pmtbShellFolder->v_ForwardMouseMessage(pmsg->message, pmsg->wParam, pmsg->lParam);

                if (_pmtbMenu)
                    _pmtbMenu->v_ForwardMouseMessage(pmsg->message, pmsg->wParam, pmsg->lParam);

                // Don't let the message be dispatched now that we've
                // forwarded it.
                pmsg->message = WM_NULL;
            }
            else if (pmsg->message == g_nMBFullCancel)
            {
                // Popup 
                TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Received private full cancel message", DBG_THIS);

                _SubMenuOnSelect(MPOS_CANCELLEVEL);
                _CancelMode(MPOS_FULLCANCEL);
                return 1;
            }
            break;
        }
    }
    
    return 0;    
}    


/*----------------------------------------------------------
Purpose: Handle WM_SYSCHAR

         This is handled for the toplevel menu only.

*/
HRESULT CMenuBand::_OnSysChar(MSG * pmsg, BOOL bFirstDibs)
{
    TCHAR ch = (TCHAR)pmsg->wParam;

    // HACKHACK (scotth): I'm only doing all this checking because I don't
    // understand why the doc-obj case sometimes (and sometimes doesn't) 
    // intercept this in its message filter.
    
    if (!bFirstDibs && _fSysCharHandled)
    {
        _fSysCharHandled = FALSE;
        return S_FALSE;
    }
    
    if (TEXT(' ') == (TCHAR)pmsg->wParam)
    {
        _fAltSpace = TRUE;  // In the words of Spock..."Remember"
        // start menu alt+space
        TraceMsg(DM_MISC, "cmb._osc: alt+space _fTopLevel(1)");
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UIINPUT, UIBL_INPMENU);
    }
    else if (!_fInSubMenu)
    {
        int idBtn;

        ASSERT(_fTopLevel);

        // There is a brief instant when we're merging a menu and pumping messages
        // This results in a null _pmtbMenu.
        if (_pmtbMenu)
        {
            // Only a toplevel menubar follows this codepath.  This means only
            // the static menu toolbar will exist (and not the shellfolder toolbar).
            _pmtbTracked = _pmtbMenu;

            HWND hwnd = _pmtbTracked->_hwndMB;
#ifdef UNICODE            
            // [msadek], On win9x we get the message thru a chain from explorer /iexplore (ANSI app.).
            // and pass it to comctl32 (Unicode) so it will fail to match the hot key.
            // the system sends the message with ANSI char and we treated it as Unicode.
            // It looks like noone is affected with this bug (US, FE) since they have hot keys always in Latin.
            // Bidi platforms are affected since they do have hot keys in native language.
            if(!g_fRunningOnNT)
            {
                char szCh[2];
                WCHAR wszCh[2];
                szCh[0] = (BYTE)ch;
                szCh[1] = '\0';

                if(MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szCh, ARRAYSIZE(szCh),
                                               wszCh, ARRAYSIZE(wszCh)))
                {
                    memcpy(&ch, wszCh, sizeof(WCHAR));
                }
                
            }
#endif // UNICODE        
            if (ToolBar_MapAccelerator(hwnd, ch, &idBtn))
            {
                // Post a message since we're already in a menu loop
                TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): WM_SYSCHAR: Posting CMBPopup message", DBG_THIS);
                UIActivateIO(TRUE, NULL);
                _pmtbTracked->PostPopup(idBtn, TRUE, TRUE);
                // browser menu alt+key, start menu alt+key
                TraceMsg(DM_MISC, "cmb._osc: alt+key _fTopLevel(1)");
                UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UIINPUT, UIBL_INPMENU);
                return S_OK;
            }
        }
    }

    // Set or reset
    _fSysCharHandled = bFirstDibs ? TRUE : FALSE;
    
    return S_FALSE;
}

HRESULT CMenuBand::_ProcessMenuPaneMessages(MSG* pmsg)
{
    if (pmsg->message == g_nMBPopupOpen)
    {
        // Popup the submenu.  Since the top-level menuband receives this first, the
        // command must be piped down the chain to the bottom-most menuband.
        IOleCommandTarget * poct;
    
        QueryService(SID_SMenuBandBottom, IID_IOleCommandTarget, (LPVOID *)&poct);

        if (poct)
        {
            BOOL bSetItem = LOWORD(pmsg->lParam);
            BOOL bInitialSelect = HIWORD(pmsg->lParam);
            VARIANTARG vargIn;

            TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Received private popup menu message", DBG_THIS);

            DWORD dwOpt = 0;

            vargIn.vt = VT_I4;
            vargIn.lVal = (LONG)pmsg->wParam;
        
            if (bSetItem)
                dwOpt |= MBPUI_SETITEM;

            if (bInitialSelect)
                dwOpt |= MBPUI_INITIALSELECT;
            
            poct->Exec(&CGID_MenuBand, MBANDCID_POPUPITEM, dwOpt, &vargIn, NULL);
            poct->Release();
            return S_OK;
        }
    }
    else if (pmsg->message == g_nMBDragCancel)
    {
        // If we got a drag cancel, make sure that the bottom most
        // menu does not have the drag enter.
        IUnknown_QueryServiceExec(SAFECAST(this, IOleCommandTarget*), 
            SID_SMenuBandBottom, &CGID_MenuBand, MBANDCID_DRAGCANCEL, 0, NULL, NULL);
        return S_OK;
    }
    else if (pmsg->message == g_nMBOpenChevronMenu)
    {

        VARIANTARG v;
        v.vt = VT_I4;
        v.lVal = (LONG)pmsg->wParam;

        IUnknown_Exec(_punkSite, &CGID_DeskBand, DBID_PUSHCHEVRON, _dwBandID, &v, NULL);
    }
    else if (pmsg->message == g_nMBFullCancel)
    {
        _SubMenuOnSelect(MPOS_CANCELLEVEL);
        _CancelMode(MPOS_FULLCANCEL);
        return S_OK;
    }

    return S_FALSE;
}

/*----------------------------------------------------------
Purpose: IMenuBand::IsMenuMessage method

         The thread's message pump calls this function to see if any
         messages need to be redirected to the menu band.

         This returns S_OK if the message is handled.  The
         message pump should not pass it onto TranslateMessage
         or DispatchMessage if it does.

*/
STDMETHODIMP CMenuBand::IsMenuMessage(MSG * pmsg)
{
    HRESULT hres = S_FALSE;

    ASSERT(IS_VALID_WRITE_PTR(pmsg, MSG));

#ifdef DEBUG
    if (g_dwDumpFlags & DF_TRANSACCELIO)
        DumpMsg(TEXT("CMB::IsMM"), pmsg);
#endif
    if (!_fShow)
        goto Return;

    switch (pmsg->message)
    {
    case WM_SYSKEYDOWN:
        // blow this off if it's a repeated keystroke
        if (!(pmsg->lParam & 0x40000000))
        {
            SendMessage(_hwndParent, WM_CHANGEUISTATE ,MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL), 0);

            // Are we pressing the Alt key to activate the menu?
            if (!_fMenuMode && pmsg->wParam == VK_MENU && _pmbState)
            {
                // Yes; The the menu was activated because of a keyboard,
                // Set the global state to show the keyboard cues.
                _pmbState->SetKeyboardCue(TRUE);

                // Since this only happens on the top level menu,
                // We only have to tell the "Top" menu to update it's state.
                _pmtbTop->SetKeyboardCue();
            }
        }
        break;

    case WM_SYSKEYUP:
        // If we're in menu mode, ignore this message. 
        // 
        if (_fMenuMode)
            hres = S_OK;
        break;

    case WM_SYSCHAR:
        // We intercept Alt-key combos (when pressed together) here,
        // to prevent USER from going into a false menu loop check.  
        // There are compatibility problems if we let that happen.
        //
        // Sent by USER32 when the user hits an Alt-char combination.
        // We need to translate this into popping down the correct
        // menu.  Normally we intercept this in the message pump
        //

        // Outlook Express needs a message hook in order to filter this 
        // message for perf we do not use that method.

        // Athena fix 222185 (lamadio) We also don't want to do this if we are not active! 
        // otherwise when WAB is on top of OE, we'll steal it's messages

        // BUGBUG: i'm removing GetTopMostPtr check below ... breaks menu accelerators for IE5 (224040)

        // (lamadio): If the Message filter is "engaged", then we can process accelerators.
        // Engaged does not mean that the filter is running.
        if (g_msgfilter.IsEngaged())
        {
            hres = (_OnSysChar(pmsg, TRUE) == S_OK) ? S_OK : S_FALSE;
        }
        break;

    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_KEYUP:
        if (_fMenuMode)
        {
            // All keystrokes should be handled or eaten by menubands
            // if we're engaged.  We must do this, otherwise hosted 
            // components like mshtml or word will try to handle the 
            // keystroke in CBaseBrowser.

            // Also, don't bother forwarding tabs
            if (VK_TAB != pmsg->wParam)
            {
                // Since we're answer S_OK, dispatch it ourselves.
                TranslateMessage(pmsg);
                DispatchMessage(pmsg);
            }

            hres = S_OK;
        }
        break;

    case WM_CONTEXTMENU:
        // HACKHACK (lamadio): Since the start button has the keyboard focus,
        // the start button will handle this. We need to forward this off to the 
        // currently tracked item at the bottom of the chain
        LRESULT lres;
        IWinEventHandler* pweh;

        if (_fMenuMode &&
            SUCCEEDED(QueryService(SID_SMenuBandBottomSelected, IID_IWinEventHandler, (LPVOID *)&pweh)))
        {
            // BUGBUG (lamadio): This will only work because only one of the two possible toolbars
            // handles this
            pweh->OnWinEvent(HWND_BROADCAST, pmsg->message, 
                pmsg->wParam, pmsg->lParam, &lres);
            pweh->Release();
            hres = S_OK;
        }
        break;

    default:

        // We only want to process the pane messages in IsMenuMessage when there is no
        // top level HWND. This is for the Deskbar menus. Outlook Express needs the 
        // TranslateMenuMessage entry point

        if (_pmbState->GetSubclassedHWND() == NULL)
            hres = _ProcessMenuPaneMessages(pmsg);
        break;
    }

Return:
    if (!_fMenuMode && hres != S_OK)
        hres = E_FAIL;


    return hres;
}

BOOL HasWindowTopmostOwner(HWND hwnd)
{
    HWND hwndOwner = hwnd;
    while (hwndOwner = GetWindowOwner(hwndOwner))
    {
        if (GetWindowLong(hwndOwner, GWL_EXSTYLE) & WS_EX_TOPMOST)
            return TRUE;
    }

    return FALSE;
}

/*----------------------------------------------------------
Purpose: IMenuBand::TranslateMenuMessage method

         The main app's window proc calls this so the menuband
         catches messages that are dispatched from a different
         message pump (than the thread's main pump).
         
         Translates messages specially for menubands.  Some messages
         are processed while the menuband is active.  Others are only
         processed when it is not.  Messages that are not b/t
         WM_KEYFIRST and WM_KEYLAST are handled here (the browser
         does not send these messages to IInputObject::
         TranslateAcceleratorIO).


Returns: S_OK if message is processed

*/
STDMETHODIMP CMenuBand::TranslateMenuMessage(MSG * pmsg, LRESULT * plRet)
{
    ASSERT(IS_VALID_WRITE_PTR(pmsg, MSG));

#ifdef DEBUG
    if (g_dwDumpFlags & DF_TRANSACCELIO)
        DumpMsg(TEXT("CMB::TMM"), pmsg);
#endif

    switch (pmsg->message)
    {
    case WM_SYSCHAR:
        // In certain doc-obj situations, the OLE message filter (??)
        // grabs this before the main thread's message pump gets a 
        // whack at it.  So we handle it here too, in case we're in
        // this scenario.
        //
        // See the comments in IsMenuMessage regarding this message.
        return _OnSysChar(pmsg, FALSE);

    case WM_INITMENUPOPUP:
        // Normally the LOWORD(lParam) is the index of the menu that 
        // is being popped up.  TrackPopupMenu (which CMenuISF uses) 
        // always sends this message with an index of 0.  This breaks 
        // clients (like DefView) who check this value.  We need to
        // massage this value if we find we're the source of the 
        // WM_INITMENUPOPUP.
        //
        // (This is not in TranslateAcceleratorIO b/c TrackPopupMenu's
        // message pump does not call it.  The wndproc must forward
        // the message to this function for us to get it.)

        if (_fInSubMenu && _pmtbTracked)
        {
            // Massage lParam to use the right index
            int iPos = ToolBar_CommandToIndex(_pmtbTracked->_hwndMB, _nItemCur);
            pmsg->lParam = MAKELPARAM(iPos, HIWORD(pmsg->lParam));

            // Return S_FALSE so this message will still be handled
        }
        break;

    case WM_UPDATEUISTATE:
        if (_pmbState)
        {
            // we don't care about UISF_HIDEFOCUS
            if (UISF_HIDEACCEL == HIWORD(pmsg->wParam))
                _pmbState->SetKeyboardCue(UIS_CLEAR == LOWORD(pmsg->wParam) ? TRUE : FALSE);
        }
        break;


    case WM_ACTIVATE:
        // Debug note: to debug menubands on ntsd, set the prototype
        // flag accordingly.  This will keep menubands from going
        // away the moment the focus changes.

        // Becomming inactive?
        if (WA_INACTIVE == LOWORD(pmsg->wParam))
        {
            // Yes; Free up the global object
            // Athena fix (lamadio) 08.02.1998: Athena uses menubands. Since they
            // have a band per window in one thread, we needed a mechanism to switch
            // between them. So we used the Msgfilter to forward messages. Since there 
            // are multiple windows, we need to set correct one.
            // But, On a deactivate, we need to NULL it out incase a window,
            // running in the same thread, has normal USER menu. We don't want to steal
            // their messages.
            if (g_msgfilter.GetTopMostPtr() == this)
                g_msgfilter.SetTopMost(NULL);

            g_msgfilter.DisEngage(_pmbState->GetContext());

            HWND hwndLostTo = (HWND)(pmsg->lParam);

            // We won't bail on the menus if we're loosing activation to a child.
            if (!IsAncestor(hwndLostTo, _pmbState->GetWorkerWindow(NULL)))
            {
                if (_fMenuMode &&
                    !(g_dwPrototype & PF_USINGNTSD) && 
                    !_fDragEntered)
                {
                    // Being deactivated.  Bail out of menus.  
                    // (Only the toplevel band gets this message.)
                    if (_fInSubMenu)
                    {
                        IMenuPopup* pmp = _pmpSubMenu;
                        if (_pmpTrackPopup)
                            pmp = _pmpTrackPopup;
                        ASSERT(pmp);    // This should be valid. If not, someone screwed up.
                        pmp->OnSelect(MPOS_FULLCANCEL);
                    }

                    _CancelMode(MPOS_FULLCANCEL);
                }
            }
        }
        else if (WA_ACTIVE == LOWORD(pmsg->wParam) || 
                 WA_CLICKACTIVE == LOWORD(pmsg->wParam))
        {
            // If I have activation, the Worker Window needs to be bottom...
            //
            // NOTE: Don't do this if the worker window has a topmost owner
            // (such as the tray).  Setting a window to HWND_NOTOPMOST moves
            // its owner windows to HWND_NOTOPMOST as well, which in this case
            // was breaking the tray's "always on top" feature.
            //
            HWND hwndWorker = _pmbState->GetWorkerWindow(NULL);
            if (hwndWorker && !HasWindowTopmostOwner(hwndWorker) && !_fDragEntered)
                SetWindowPos(hwndWorker, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

            // Set the context because when a menu heirarchy becomes active because the
            // subclassed HWND becomes active, we need to reenable the message hook.
            g_msgfilter.SetContext(this, TRUE);

            // When we get reactivated, we need to position ourself above the start bar.
            Exec(&CGID_MenuBand, MBANDCID_REPOSITION, TRUE, NULL, NULL);
            
            // Becomming activated. We need to reengage the message hook so that
            // we get the correct messages.
            g_msgfilter.ReEngage(_pmbState->GetContext());

            // Are we in menu mode?
            if (_fMenuMode)
            {
                // Need to reengage some things.
                // Take the capture back because we have lost it to context menus or dialogs.
                g_msgfilter.RetakeCapture();

            }
            g_msgfilter.SetTopMost(this);
        }

        //
        // Memphis and NT5 grey their horizontal menus when the windows is inactive.
        //
        if (!_fVertical && (g_bRunOnMemphis || g_bRunOnNT5) && _pmtbMenu)
        {
            // This needs to stay here because of the above check...
            if (WA_INACTIVE == LOWORD(pmsg->wParam))
            {
                _fAppActive = FALSE;
            }
            else
            {
                _fAppActive = TRUE;
            }
            // Reduces flicker by using this instead of an InvalidateWindow/UpdateWindow Pair
            RedrawWindow(_pmtbMenu->_hwndMB, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        }
        break;

    case WM_SYSCOMMAND:
        if ( !_fMenuMode )
        {
            switch (pmsg->wParam & 0xFFF0)
            {
            case SC_KEYMENU:
                // The user either hit the Alt key by itself or Alt-space.
                // If it was Alt-space, let DefWindowProc handle it so the
                // system menu comes up.  Otherwise, we'll handle it to
                // toggle the menuband.

                // Was it Alt-space?
                if (_fAltSpace)
                {
                    // Yes; let it go
                    TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Caught the Alt-space", DBG_THIS);
                    _fAltSpace = FALSE;
                }
                else if (_fShow)
                {
                    // No; activate the menu
                    TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Caught the WM_SYSCOMMAND, SC_KEYMENU", DBG_THIS);

                    UIActivateIO(TRUE, NULL);

                    // We sit in a modal loop here because typically
                    // WM_SYSCOMMAND doesn't return until the menu is finished.
                    //
                    while (_fMenuMode) 
                    {
                        MSG msg;
                        if (GetMessage(&msg, NULL, 0, 0)) 
                        {
                            if ( S_OK != IsMenuMessage(&msg) )
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                    }

                    *plRet = 0;
                    return S_OK;        // Caller shouldn't handle this
                }
                break;
            }
        }
        break;

    default:
        // We only want to process the pane messages in IsMenuMessage when there is no
        // top level HWND. This is for the Deskbar menus. Outlook Express needs the 
        // TranslateMenuMessage entry point
        if (_pmbState->GetSubclassedHWND() != NULL)
            return _ProcessMenuPaneMessages(pmsg);
        break;


    }
    return S_FALSE;
}


/*----------------------------------------------------------
Purpose: IObjectWithSite::SetSite method

         Called by the menusite to host this band.  Since the
         menuband contains two toolbars, we set their parent
         window to be the site's hwnd.

*/
STDMETHODIMP CMenuBand::SetSite(IUnknown* punkSite)
{
    // Do this first because SetParent needs to query to the top level browser for
    // sftbar who queries to the top level browser to get the drag and drop window.
    HRESULT hres = SUPERCLASS::SetSite(punkSite);

    if (_psmcb && _fTopLevel && !(_dwFlags & SMINIT_NOSETSITE))
        IUnknown_SetSite(_psmcb, punkSite);

    IUnknown_GetWindow(punkSite, &_hwndParent);

    // Need this for Closing an expanded vertical menu. Start Menu knows to do this when it's top level,
    // but the Favorites needs to know when it's parent is the horizontal menu.
    VARIANT var;
    if (SUCCEEDED(IUnknown_QueryServiceExec(punkSite, SID_SMenuBandParent, &CGID_MenuBand,
                                       MBANDCID_ISVERTICAL, 0, NULL, &var)) && 
        var.boolVal == VARIANT_FALSE)
    {
        _fParentIsHorizontal = TRUE;
    }


    // Tell the toolbars who their new parent is
    if (_pmtbMenu)
        _pmtbMenu->SetParent(_hwndParent);
    if (_pmtbShellFolder)
        _pmtbShellFolder->SetParent(_hwndParent);

    return hres;
}

/*----------------------------------------------------------
Purpose: IShellMenu::Initialize method

*/
STDMETHODIMP CMenuBand::Initialize(IShellMenuCallback* psmcb, UINT uId, UINT uIdAncestor, DWORD dwFlags)
{
    DEBUG_CODE( _fInitialized = TRUE; );

    // Initalized can be called with NULL values to only set some of them.

    // Default to Vertical
    if (!(dwFlags & SMINIT_HORIZONTAL) && !(dwFlags & SMINIT_VERTICAL))
        dwFlags |= SMINIT_VERTICAL;

    _Initialize(dwFlags);

    if (uIdAncestor != ANCESTORDEFAULT)
        _uIdAncestor = uIdAncestor;

    if (_uId != -1)
        _uId = uId;

    if (psmcb)
    {
        if (!SHIsSameObject(psmcb, _psmcb))
        {
            if (_punkSite && _fTopLevel && !(dwFlags & SMINIT_NOSETSITE))
                IUnknown_SetSite(_psmcb, NULL);

            ATOMICRELEASE(_psmcb);
            _psmcb = psmcb;
            _psmcb->AddRef();

            // We do not set the site in case this callback is shared between 2 bands (Menubar/Chevron menu)
            if (_punkSite && _fTopLevel && !(dwFlags & SMINIT_NOSETSITE))
                IUnknown_SetSite(_psmcb, _punkSite);

            // Only call this if we're setting a new one. Pass the address of the user associated
            // data section. This is so that the callback can associate data with this pane only
            _CallCB(SMC_CREATE, 0, (LPARAM)&_pvUserData);
        }
    }

    return NOERROR;
}

/*----------------------------------------------------------
Purpose: IShellMenu::GetMenuInfo method

*/
STDMETHODIMP CMenuBand::GetMenuInfo(IShellMenuCallback** ppsmc, UINT* puId, 
                                    UINT* puIdAncestor, DWORD* pdwFlags)
{
    if (ppsmc)
    {
        *ppsmc = _psmcb;
        if (_psmcb)
            ((IShellMenuCallback*)*ppsmc)->AddRef();
    }

    if (puId)
        *puId = _uId;

    if (puIdAncestor)
        *puIdAncestor = _uIdAncestor;

    if (pdwFlags)
        *pdwFlags = _dwFlags;

    return NOERROR;
}


void CMenuBand::_AddToolbar(CMenuToolbarBase* pmtb, DWORD dwFlags)
{
    pmtb->SetSite(SAFECAST(this, IMenuBand*));
    if (_hwndParent)
        pmtb->CreateToolbar(_hwndParent);
    
    // Treat this like a two-element stack, where this function
    // behaves like a "push".  The one additional trick is we 
    // could be pushing onto the top or the bottom of the "stack".

    if (dwFlags & SMSET_BOTTOM)
    {
        if (_pmtbBottom)
        {
            // I don't need to release, because _pmtbTop and _pmtbBottom are aliases for
            // _pmtbShellFolder and _pmtbMenu
            _pmtbTop = _pmtbBottom;
            _pmtbTop->SetToTop(TRUE);
        }

        _pmtbBottom = pmtb;
        _pmtbBottom->SetToTop(FALSE);
    }
    else    // Default to Top...
    {
        if (_pmtbTop)
        {
            _pmtbBottom = _pmtbTop;
            _pmtbBottom->SetToTop(FALSE);
        }

        _pmtbTop = pmtb;
        _pmtbTop->SetToTop(TRUE);
    }

    // _pmtbBottom should never be the only toolbar that exists in the menuband.
    if (!_pmtbTop)
        _pmtbTop = _pmtbBottom;

    // The menuband determines there is a single toolbar by comparing
    // the bottom with the top.  So make the bottom the same if necessary.
    if (!_pmtbBottom)
        _pmtbBottom = _pmtbTop;
}


/*----------------------------------------------------------
Purpose: IShellMenu::GetShellFolder method

*/
STDMETHODIMP CMenuBand::GetShellFolder(DWORD* pdwFlags, LPITEMIDLIST* ppidl,
                                       REFIID riid, void** ppvObj)
{
    HRESULT hres = E_FAIL;
    if (_pmtbShellFolder)
    {
        *pdwFlags = _pmtbShellFolder->GetFlags();

        hres = S_OK;

        if (ppvObj)
        {
            // HACK HACK.  this should QI for a mnfolder specific interface to do this.
            hres = _pmtbShellFolder->GetShellFolder(ppidl, riid, ppvObj);
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellMenu::SetShellFolder method

*/
STDMETHODIMP CMenuBand::SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    ASSERT(_fInitialized);
    HRESULT hres = E_OUTOFMEMORY;

    // If we're processing a change notify, we cannot do anything that will modify state.
    // NOTE: if we don't have a state, we can't possibly processing a change notify
    if (_pmbState && _pmbState->IsProcessingChangeNotify())
        return E_PENDING;

    // Only one shellfolder menu can exist per menuband.  Additionally,
    // a shellfolder menu can exist either at the top of the menu, or
    // at the bottom (when it coexists with a static menu).

    // Is there already a shellfolder menu?
    if (_pmtbShellFolder)
    {
        IShellFolderBand* psfb;
        EVAL(SUCCEEDED(_pmtbShellFolder->QueryInterface(IID_IShellFolderBand, (void**)&psfb)));
        hres = psfb->InitializeSFB(psf, pidlFolder);
        psfb->Release();
        
    }
    else
    {
        _pmtbShellFolder = new CMenuSFToolbar(this, psf, pidlFolder, hKey, dwFlags);
        if (_pmtbShellFolder)
        {
            _AddToolbar(_pmtbShellFolder, dwFlags);
            hres = NOERROR;
        }
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IMenuBand::GetMenu method

*/
STDMETHODIMP CMenuBand::GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags)
{
    HRESULT hres = E_FAIL;

    // HACK HACK.  this should QI for a menustatic specific interface to do this.
    if (_pmtbMenu)
        hres = _pmtbMenu->GetMenu(phmenu, phwnd, pdwFlags);

    return hres;
}


/*----------------------------------------------------------
Purpose: IMenuBand::SetMenu method

*/
STDMETHODIMP CMenuBand::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{
    // Passing a NULL hmenu is valid. It means destroy our menu object.
    ASSERT(_fInitialized);
    HRESULT hres = E_FAIL;

    // Only one static menu can exist per menuband.  Additionally,
    // a static menu can exist either at the top of the menu, or
    // at the bottom (when it coexists with a shellfolder menu).

    // Is there already a static menu?
    if (_pmtbMenu)
    {
        // Since we're merging in a new menu, make sure to update the cache...
        _hmenu = hmenu;
        // Yes
        // HACK HACK.  this should QI for a menustatic specific interface to do this.
        return _pmtbMenu->SetMenu(hmenu, hwnd, dwFlags);
    }
    else
    {
        // BUGBUG (lamadio): This is to work around a problem in the interface definintion: We have
        // no method of setting the Subclassed HWND outside of a SetMenu. So I'm just piggybacking
        // off of this. A better fix would be to introduce IMenuBand2::SetSubclass(HWND). IMenuBand
        // actually implements the "Subclassing", so extending this interface would be worthwhile.
        _hwndMenuOwner = hwnd;

        if (_fTopLevel)
        {
            _pmbState->SetSubclassedHWND(hwnd);
        }

        if (hmenu)
        {
            _hmenu = hmenu;
            _pmtbMenu = new CMenuStaticToolbar(this, hmenu, hwnd, _uId, dwFlags);
            if (_pmtbMenu)
            {
                _AddToolbar(_pmtbMenu, dwFlags);
                hres = S_OK;
            }
            else
                hres = E_OUTOFMEMORY;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellMenu::SetMenuToolbar method

*/
STDMETHODIMP CMenuBand::SetMenuToolbar(IUnknown* punk, DWORD dwFlags)
{
    CMenuToolbarBase* pmtb;
    if (punk && SUCCEEDED(punk->QueryInterface(CLSID_MenuToolbarBase, (void**)&pmtb)))
    {
        ASSERT(_pmtbShellFolder == NULL);
        _pmtbShellFolder = pmtb;
        _AddToolbar(pmtb, dwFlags);
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}


/*----------------------------------------------------------
Purpose: IShellMenu::InvalidateItem method

*/
STDMETHODIMP CMenuBand::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    HRESULT hres = S_FALSE;

    // If psmd is NULL, we need to just dump the toolbars and do a full reset.
    if (psmd == NULL)
    {
        // If we're processing a change notify, we cannot do anything that will modify state.
        if (_pmbState && _pmbState->IsProcessingChangeNotify())
            return E_PENDING;

        _pmbState->PushChangeNotify();

        // Tell the callback we're refreshing so that it can
        // reset any cached state
        _CallCB(SMC_REFRESH);
        _fExpanded = FALSE;

        // We don't need to refill if the caller only wanted to 
        // refresh the sub menus.

        // Refresh the Shell Folder first because
        // It may have no items after it's done, and the
        // menuband may rely on this to add a seperator
        if (_pmtbShellFolder)
            _pmtbShellFolder->v_Refresh();

        // Refresh the Static menu
        if (_pmtbMenu)
            _pmtbMenu->v_Refresh();

        if (_pmpSubMenu)
        {
            _fInSubMenu = FALSE;
            IUnknown_SetSite(_pmpSubMenu, NULL);
            ATOMICRELEASE(_pmpSubMenu);
        }

        _pmbState->PopChangeNotify();

    }
    else
    {
        if (_pmtbTop)
            hres = _pmtbTop->v_InvalidateItem(psmd, dwFlags);

        // We refresh everything at this level if the psmd is null
        if (_pmtbBottom && hres != S_OK)
            hres = _pmtbBottom->v_InvalidateItem(psmd, dwFlags);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellMenu::GetState method

*/
STDMETHODIMP CMenuBand::GetState(LPSMDATA psmd)
{
    if (_pmtbTracked)
        return _pmtbTracked->v_GetState(-1, psmd);
    // todo: might want to put stuff from _CallCB (below) in here
    return E_FAIL;
}


HRESULT CMenuBand::_CallCB(DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
    if (!_psmcb)
        return S_FALSE;

    // We don't need to check callback mask here because these are not maskable events.

    SMDATA smd = {0};
    smd.punk = SAFECAST(this, IShellMenu*);
    smd.uIdParent = _uId;
    smd.uIdAncestor = _uIdAncestor;
    smd.hwnd = _hwnd;
    smd.hmenu = _hmenu;
    smd.pvUserData = _pvUserData;
    if (_pmtbShellFolder)
        _pmtbShellFolder->GetShellFolder(&smd.pidlFolder, IID_IShellFolder, (void**)&smd.psf);
    HRESULT hres = _psmcb->CallbackSM(&smd, dwMsg, wParam, lParam);

    ILFree(smd.pidlFolder);
    if (smd.psf)
        smd.psf->Release();

    return hres;
}


/*----------------------------------------------------------
Purpose: IInputObject::TranslateAcceleratorIO

         This is called by the base browser only when the menuband
         "has the focus", and only for messages b/t WM_KEYFIRST
         and WM_KEYLAST.  This isn't very useful for menubands.
         See the explanations in GetMsgFilterCB, IsMenuMessage
         and TranslateMenuMessage.

         In addition, menubands cannot ever have the activation,
         so this method should never be called.

         Returns S_OK if handled.
*/
STDMETHODIMP CMenuBand::TranslateAcceleratorIO(LPMSG pmsg)
{
    AssertMsg(0, TEXT("Menuband has the activation but it shouldn't!"));

    return S_FALSE;
}

/*----------------------------------------------------------
Purpose: IInputObject::HasFocusIO

*/
STDMETHODIMP CMenuBand::HasFocusIO()
{
    // We consider a menuband has the focus even if it has submenus
    // that are currently cascaded out.  All menubands in the chain
    // have the focus.
    return _fMenuMode ? S_OK : S_FALSE;
}

/*----------------------------------------------------------
Purpose: IMenuPopup::SetSubMenu method

         The child menubar calls us with its IMenuPopup pointer.
*/
STDMETHODIMP CMenuBand::SetSubMenu(IMenuPopup * pmp, BOOL fSet)
{
    ASSERT(IS_VALID_CODE_PTR(pmp, IMenuPopup));

    if (fSet)
    {
        _fInSubMenu = TRUE;
    }
    else 
    {
        if (_pmtbTracked)
        {
            _pmtbTracked->PopupClose();
        }

        _fInSubMenu = FALSE;
        _nItemSubMenu = -1;
    }

    return S_OK;
}    

HRESULT CMenuBand::_SiteSetSubMenu(IMenuPopup * pmp, BOOL bSet)
{
    HRESULT hres;
    IMenuPopup * pmpSite;

    hres = IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_IMenuPopup, 
                                 (LPVOID *)&pmpSite);
    if (SUCCEEDED(hres))
    {
        hres = pmpSite->SetSubMenu(pmp, bSet);
        pmpSite->Release();
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: Tell the GetMsg filter that this menuband is ready to
         listen to messages.

*/
HRESULT CMenuBand::_EnterMenuMode(void)
{
    ASSERT(!_fMenuMode);        // Must not push onto stack more than once

    if (g_dwProfileCAP & 0x00002000) 
        StartCAP();

    DEBUG_CODE( _nMenuLevel = g_nMenuLevel++; )

    _fMenuMode = TRUE;
    _fInSubMenu = FALSE;
    _nItemMove = -1;
    _fCascadeAnimate = TRUE;

    _hwndFocusPrev = NULL;

    if (_fTopLevel)
    {
        // BUGBUG (lamadio): this piece should be moved to the shbrowse callback

        // REVIEW (scotth): some embedded controls (like the surround
        // video ctl on the carpoint website) have another thread that
        // eats all the messages when the control has the focus.
        // This prevents us from getting any messages once we're in
        // menu mode.  I don't understand why USER menus work yet.
        // One way to work around this bug is to detect this case and
        // set the focus to our main window for the duration.
        
        if (GetWindowThreadProcessId(GetFocus(), NULL) != GetCurrentThreadId())
        {
            IShellBrowser* psb;
            
            if (SUCCEEDED(QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb)))
            {
                HWND hwndT;
                
                psb->GetWindow(&hwndT);
                _hwndFocusPrev = SetFocus(hwndT);
                psb->Release();
            }
        }
    
        _hCursorOld = GetCursor();
        SetCursor(g_hCursorArrow);
        HideCaret(NULL);
    }

    _SiteSetSubMenu(this, TRUE);

    if (_pmtbTop)
    {
        HWND hwnd = _pmtbTop->_hwndMB;
        if (!_fVertical && -1 == _nItemNew)
        {
            // The Alt key always highlights the first menu item initially
            SetTracked(_pmtbTop);
            ToolBar_SetHotItem(hwnd, 0);
            NotifyWinEvent(EVENT_OBJECT_FOCUS, _pmtbTop->_hwndMB, OBJID_CLIENT, 
                GetIndexFromChild(TRUE, 0));
        }

        _pmtbTop->Activate(TRUE);

        // The toolbar usually tracks mouse events.  However, as the mouse
        // moves over submenus, we still want the parent menubar to
        // behave as if it has retained the focus (that is, keep the
        // last selected item highlighted). This also prevents the toolbar
        // from handling WM_MOUSELEAVE messages unnecessarily.
        ToolBar_SetAnchorHighlight(hwnd, TRUE);

        TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Entering menu mode", DBG_THIS);
        NotifyWinEvent(_fVertical? EVENT_SYSTEM_MENUPOPUPSTART: EVENT_SYSTEM_MENUSTART, 
            hwnd, OBJID_CLIENT, CHILDID_SELF);
    }

    if (_pmtbBottom)
    {
        _pmtbBottom->Activate(TRUE);
        ToolBar_SetAnchorHighlight(_pmtbBottom->_hwndMB, TRUE); // Turn off anchoring
    }

    g_msgfilter.Push(_pmbState->GetContext(), this, _punkSite);

    return S_OK;
}    


void CMenuBand::_ExitMenuMode(void)
{
    _fMenuMode = FALSE;
    _nItemCur = -1;
    _fPopupNewMenu = FALSE;
    _fInitialSelect = FALSE;

    if (_pmtbTop)
    {
        HWND hwnd = _pmtbTop->_hwndMB;
        ToolBar_SetAnchorHighlight(hwnd, FALSE); // Turn off anchoring
        if (!_fVertical)
        {
            // Use the first item, since we're assuming every menu must have
            // at least one item
            _pmtbTop->v_SendMenuNotification(0, TRUE);
        
            // The user may have clicked outside the menu, which would have
            // cancelled it.  But since we set the ANCHORHIGHLIGHT attribute,
            // the toolbar won't receive a message to cause it to
            // remove the highlight.  So do it explicitly now.
            SetTracked(NULL);
            UpdateWindow(hwnd);
        }

        _pmtbTop->Activate(FALSE);

        NotifyWinEvent(_fVertical? EVENT_SYSTEM_MENUPOPUPEND: EVENT_SYSTEM_MENUEND, 
            hwnd, OBJID_CLIENT, CHILDID_SELF);
    }

    if (_pmtbBottom)
    {
        _pmtbBottom->Activate(FALSE);
        ToolBar_SetAnchorHighlight(_pmtbBottom->_hwndMB, FALSE); // Turn off anchoring
    }

    g_msgfilter.Pop(_pmbState->GetContext());

    _SiteSetSubMenu(this, FALSE);

    if (_fTopLevel)
    {
        SetCursor(_hCursorOld);
        ShowCaret(NULL);
        
        g_msgfilter.SetContext(this, FALSE);

        // We do this here, because ShowDW(FALSE) does not get called on the
        // top level menu band. This resets the state, so that the accelerators 
        // are not shown.
        if (_pmbState)
            _pmbState->SetKeyboardCue(FALSE);

        // Tell the menus to update their state to the current global cue state.
        _pmtbTop->SetKeyboardCue();
        if (_pmtbTop != _pmtbBottom)
            _pmtbBottom->SetKeyboardCue();

    }

    if (_hwndFocusPrev)
        SetFocus(_hwndFocusPrev);

    if (_fTopLevel)
    {
        //
        // The top-level menu has gone away.  Win32 focus and ui-activation don't
        // actually change when this happens, so the browser and focused dude have
        // no idea that something happened and won't generate any AA event.  So, we
        // do it here for them.  Note that if there was a selection inside the focused
        // dude, we'll lose it.  This is the best we can do for now, as we don't
        // currently have a way to tell the focused/ui-active guy (who knows about the
        // current selection) to reannounce focus.
        //
        HWND hwndFocus = GetFocus();
        NotifyWinEvent(EVENT_OBJECT_FOCUS, hwndFocus, OBJID_CLIENT, CHILDID_SELF);
    }
        
    TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): Exited menu mode", DBG_THIS);
    DEBUG_CODE( g_nMenuLevel--; )
    DEBUG_CODE( _nMenuLevel = -1; )
    if (g_dwProfileCAP & 0x00002000) 
        StopCAP();
}    


/*----------------------------------------------------------
Purpose: IInputObject::UIActivateIO

         Menubands CANNOT take the activation.  Normally
         a band would return S_OK and call the site's 
         OnFocusChangeIS method, so that its TranslateAcceleratorIO
         method would receive keyboard messages.

         However, menus are different.  The window/toolbar that
         currently has the activation must retain that activation
         when the menu pops down.  Because of this, menubands use 
         a GetMessage filter to intercept messages.

*/
STDMETHODIMP CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hres;
    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    if (lpMsg != NULL) {
        // don't allow TAB to band (or any other 'non-explicit' activation).
        // (if we just cared about TAB we'd check IsVK_TABCycler).
        // all kinds of badness would result if we did.
        // the band can't take focus (see above), so it can't obey the
        // UIAct/OnFocChg rules (e.g. can't call OnFocusChangeIS), so
        // our basic activation-tracking assumptions would be broken.
        return S_FALSE;
    }

    if (fActivate)
    {
        TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): UIActivateIO(%d)", DBG_THIS, fActivate);
        
        if (!_fMenuMode)
        {
            _EnterMenuMode();

            // BUGBUG (lamadio) : Should go in the Favorites callback.
            // The toplevel menuband does not set the real activation.  
            // But the children do, so activation can be communicated
            // with the parent menuband.
            if (_fVertical) {
                UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
            } else {
                IUnknown_Exec(_punkSite, &CGID_Theater, THID_TOOLBARACTIVATED, 0, NULL, NULL);
            }
        }

        if (_fPopupNewMenu)
        {
            _nItemCur = _nItemNew;
            ASSERT(-1 != _nItemCur);
            ASSERT(_pmtbTracked);

            _fPopupNewMenu = FALSE;
            _nItemNew = -1;

            // Popup a menu
            hres = _pmtbTracked->PopupOpen(_nItemCur);
            if (FAILED(hres))
            {
                // Don't fail the activation
                TraceMsg(TF_ERROR, "%d (pmb=%#08lx): PopupOpen failed", DBG_THIS);
                MessageBeep(MB_OK);
            }
            else if (S_FALSE == hres)
            {
                // The submenu was modal and is finished now
                _ExitMenuMode();
            }
        }
    }
    else if (_fMenuMode)
    {
        TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): UIActivateIO(%d)", DBG_THIS, fActivate);

        ASSERT( !_fInSubMenu );

        if (!_fTopLevel)
            UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), FALSE);

        _ExitMenuMode();
    }

    return S_FALSE;
}


/*----------------------------------------------------------
Purpose: IDeskBand::GetBandInfo method

*/
HRESULT CMenuBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    HRESULT hres = NOERROR;

    _dwBandID = dwBandID;           // critical for perf! (BandInfoChanged)

    pdbi->dwMask &= ~DBIM_TITLE;    // no title (ever, for now)

    // We expect that _pmtbBottom should never be the only toolbar
    // that exists in the menuband.
    ASSERT(NULL == _pmtbBottom || _pmtbTop);

    pdbi->dwModeFlags = DBIMF_USECHEVRON;

    if (_pmtbTop)
    {
        // If the buttons need to be updated in the toolbars, the we should 
        // do this before we start asking them about their sizes....
        if (_fForceButtonUpdate)
        {
            _UpdateButtons();
        }

        if (_fVertical) 
        {
            pdbi->ptMaxSize.y = 0;
            pdbi->ptMaxSize.x = 0;

            SIZE size = {0};
        
            if (_pmtbMenu)
            {
                // size param zero here => it's just an out param
                _pmtbMenu->GetSize(&size);

                // HACKHACK (lamadio): On downlevel, LARGE metrics mode causes 
                // Start menu to push the programs menu item off screen.
                if (size.cy > (3 * GetSystemMetrics(SM_CYSCREEN) / 4))
                {
                    Exec(&CGID_MenuBand, MBANDCID_SETICONSIZE, ISFBVIEWMODE_SMALLICONS, NULL, NULL);
                    size.cx = 0;
                    size.cy = 0;
                    _pmtbMenu->GetSize(&size);
                }

                pdbi->ptMaxSize.y = size.cy;
                pdbi->ptMaxSize.x = size.cx;
            }
            if (_pmtbShellFolder)
            {
                // size param should be non-zero here => it's an in/out param
                _pmtbShellFolder->GetSize(&size);
                pdbi->ptMaxSize.y += size.cy + ((_pmtbMenu && !_fExpanded)? 1 : 0);   // Minor sizing problem
                pdbi->ptMaxSize.x = max(size.cx, pdbi->ptMaxSize.x);
            }

            pdbi->ptMinSize = pdbi->ptMaxSize;

        }
        else
        {
            HWND hwnd = _pmtbTop->_hwndMB;
            ShowDW(TRUE);

            SIZE rgSize;
            if ( SendMessage( hwnd, TB_GETMAXSIZE, 0, (LPARAM) &rgSize ))
            {
                pdbi->ptActual.y = rgSize.cy;
                SendMessage(hwnd, TB_GETIDEALSIZE, FALSE, (LPARAM)&pdbi->ptActual);
            }

            // make our min size identical to the size of the first button
            // (we're assuming that the toolbar has at least one button)
            RECT rc;
            SendMessage(hwnd, TB_GETITEMRECT, 0, (WPARAM)&rc);
            pdbi->ptMinSize.x = RECTWIDTH(rc);
            pdbi->ptMinSize.y = RECTHEIGHT(rc);
        }
    }
    return hres;
}

/*----------------------------------------------------------
Purpose: IOleService::Exec method

*/
STDMETHODIMP CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdExecOpt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{

    // Don't do anything if we're closing.
    if (_fClosing)
        return E_FAIL;

    if (pguidCmdGroup == NULL) 
    {
        /*NOTHING*/
    }
    else if (IsEqualGUID(CGID_MENUDESKBAR, *pguidCmdGroup)) 
    {
        switch (nCmdID) 
        {
        case MBCID_GETSIDE:
            if (pvarargOut) 
            {
                BOOL fOurChoice = FALSE;
                pvarargOut->vt = VT_I4;
                
                if (!_fTopLevel) 
                {
                    // if we are not the top level menu, we 
                    // must continue with the direction our parent was in
                    IMenuPopup* pmpParent;
                    IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_IMenuPopup, (LPVOID*)&pmpParent);
                    if (pmpParent) 
                    {
                        if (FAILED(IUnknown_Exec(pmpParent, pguidCmdGroup, nCmdID, nCmdExecOpt, pvarargIn, pvarargOut)))
                            fOurChoice = TRUE;
                        pmpParent->Release();
                    }
                } else 
                    fOurChoice = TRUE;

                if (!fOurChoice) {
                    // only use the parent's side hint if it is in the same orientation (ie, horizontal menubar to vertical popup 
                    // means we need to make a new choice)
                    BOOL fParentVertical = (pvarargOut->lVal == MENUBAR_RIGHT || pvarargOut->lVal == MENUBAR_LEFT);
                    if (BOOLIFY(_fVertical) != BOOLIFY(fParentVertical))
                        fOurChoice = TRUE;
                }

                if (fOurChoice) 
                {
                    if (_fVertical)
                    {
                        HWND hWndMenuBand;

                        //
                        // The MenuBand is Mirrored , then start the first Menu Window
                        // as Mirrored. [samera]
                        //
                        if ((SUCCEEDED(GetWindow(&hWndMenuBand))) &&
                            (IS_WINDOW_RTL_MIRRORED(hWndMenuBand)) ) 
                            pvarargOut->lVal = MENUBAR_LEFT;
                        else
                            pvarargOut->lVal = MENUBAR_RIGHT;
                    }
                    else
                        pvarargOut->lVal = MENUBAR_BOTTOM;
                }

            }
            return S_OK;
        }
    }
    else if (IsEqualGUID(CGID_MenuBand, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case MBANDCID_GETFONTS:
            // BUGBUG (lamadio): can I remove this?
            if (pvarargOut)
            {
                if (EVAL(_pmbm))
                {
                    // BUGBUG (lamadio): this is not marshal-safe.
                    pvarargOut->vt = VT_UNKNOWN;
                    _pmbm->QueryInterface(IID_IUnknown, (void**)&pvarargOut->punkVal);
                    return S_OK;
                }
                else
                    return E_FAIL;
            }
            else
                return E_INVALIDARG;
            break;

        case MBANDCID_SETFONTS:
            if (pvarargIn && VT_UNKNOWN == pvarargIn->vt && pvarargIn->punkVal)
            {
                // BUGBUG (lamadio): this is not marshal-safe.
                ATOMICRELEASE(_pmbm);
                pvarargIn->punkVal->QueryInterface(CLSID_MenuBandMetrics, (void**)&_pmbm);

                _fForceButtonUpdate = TRUE;
                // Force Update of Toolbars:
                if (_pmtbMenu)
                    _pmtbMenu->SetMenuBandMetrics(_pmbm);

                if (_pmtbShellFolder)
                    _pmtbShellFolder->SetMenuBandMetrics(_pmbm);
            }
            else
                return E_INVALIDARG;
            break;


        case MBANDCID_RECAPTURE:
            g_msgfilter.RetakeCapture();
            break;

        case MBANDCID_NOTAREALSITE:
            _fParentIsNotASite = BOOLIFY(nCmdExecOpt);
            break;

        case MBANDCID_ITEMDROPPED:
            {
                _fDragEntered = FALSE;
                HWND hwndWorker = _pmbState->GetWorkerWindow(NULL);
                if (hwndWorker && !HasWindowTopmostOwner(hwndWorker))
                    SetWindowPos(hwndWorker, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
            }
            break;

        case MBANDCID_DRAGENTER:
            _fDragEntered = TRUE;
            break;

        case MBANDCID_DRAGLEAVE:
            _fDragEntered = FALSE;
            break;

            
        case MBANDCID_SELECTITEM:
            {
                int iPos = nCmdExecOpt;

                // If they are passing vararg in, then this is an ID, not a position
                if (pvarargIn && pvarargIn->vt == VT_I4)
                {
                    _nItemNew = pvarargIn->lVal;
                    _fPopupNewItemOnShow = TRUE;
                }

                // This can be called outside of a created band.
                if (_pmtbTop)
                {
                    if (iPos == MBSI_NONE)
                    {
                        SetTracked(NULL);
                    }
                    else
                    {
                        CMenuToolbarBase* pmtb = (iPos == MBSI_LASTITEM) ? _pmtbBottom : _pmtbTop;
                        ASSERT(pmtb);

                        SetTracked(pmtb);
                        _pmtbTracked->SetHotItem(1, iPos, -1, HICF_OTHER);

                        // If the new hot item is in the obscured part of the menu, then the
                        // above call will have reentered & nulled out _pmtbTracked (since we
                        // drop down the chevron menu if the new hot item is obscured).  So we
                        // need to revalidate _pmtbTracked.
                        if (!_pmtbTracked)
                            break;

                        NotifyWinEvent(EVENT_OBJECT_FOCUS, _pmtbTracked->_hwndMB, OBJID_CLIENT, 
                            GetIndexFromChild(TRUE, iPos));
                    }
                }
            }
            break;

        case MBANDCID_KEYBOARD:
            // If we've been executed because of a keyboard, then set the global
            // state to reflect that. This is sent by MenuBar when it's ::Popup
            // member is called with the flag MPPF_KEYBOARD. This is for start menu.
            if (_pmbState)
                _pmbState->SetKeyboardCue(TRUE);
            break;

        case MBANDCID_POPUPITEM:
            if (pvarargIn && VT_I4 == pvarargIn->vt)
            {
                // we don't want to popup a sub menu if we're tracking a context menu...
                if ( !((_pmtbBottom && _pmtbBottom->v_TrackingSubContextMenu()) || 
                       (_pmtbTop && _pmtbTop->v_TrackingSubContextMenu())))
                {
                    // No tracked item? Well default to the top (For the chevron menu)
                    if (!_pmtbTracked)
                    {
                        SetTracked(_pmtbTop);
                    }

                    // We don't want to display the sub menu if we're not shown.
                    // We do this because we could have been dismissed before the message
                    // was routed.
                    if (_fShow && _pmtbTracked)
                    {
                        int iItem;
                        int iPos;

                        if (nCmdExecOpt & MBPUI_ITEMBYPOS)
                        {
                            iPos = pvarargIn->lVal;
                            iItem = GetButtonCmd(_pmtbTracked->_hwndMB, pvarargIn->lVal);
                        }
                        else
                        {
                            iPos = ToolBar_CommandToIndex(_pmtbTracked->_hwndMB, pvarargIn->lVal);
                            iItem = pvarargIn->lVal;
                        }


                        if (nCmdExecOpt & MBPUI_SETITEM)
                        {
                            // Set the hot item explicitly since this can be
                            // invoked by the keyboard and the mouse could be 
                            // anywhere.
                            _pmtbTracked->SetHotItem(1, iPos, -1, HICF_OTHER);

                            // If the new hot item is in the obscured part of the menu, then the
                            // above call will have reentered & nulled out _pmtbTracked (since we
                            // drop down the chevron menu if the new hot item is obscured).  So we
                            // need to revalidate _pmtbTracked.
                            if (!_pmtbTracked)
                                break;

                            NotifyWinEvent(EVENT_OBJECT_FOCUS, _pmtbTracked->_hwndMB, OBJID_CLIENT, 
                                GetIndexFromChild(TRUE, iPos) );
                        }

                        _pmtbTracked->PopupHelper(iItem, nCmdExecOpt & MBPUI_INITIALSELECT);
                    }
                }
            }
            break;

        case MBANDCID_ISVERTICAL:
            if (pvarargOut)
            {
                pvarargOut->vt = VT_BOOL;
                pvarargOut->boolVal = (_fVertical)? VARIANT_TRUE: VARIANT_FALSE;
            }
            break;
            
        case MBANDCID_SETICONSIZE:
            ASSERT(nCmdExecOpt == ISFBVIEWMODE_SMALLICONS || 
                nCmdExecOpt == ISFBVIEWMODE_LARGEICONS);

            _uIconSize = nCmdExecOpt;

            if (_pmtbTop)
                _pmtbTop->v_UpdateIconSize(nCmdExecOpt, TRUE);

            if (_pmtbBottom)
                _pmtbBottom->v_UpdateIconSize(nCmdExecOpt, TRUE);
            break;

        case MBANDCID_SETSTATEOBJECT:
            if (pvarargIn && VT_INT_PTR == pvarargIn->vt)
            {
                _pmbState = (CMenuBandState*)pvarargIn->byref;
            }
            break;

        case MBANDCID_ISINSUBMENU:
            if (_fInSubMenu || (_pmtbTracked && _pmtbTracked->v_TrackingSubContextMenu()))
                return S_OK;
            else
                return S_FALSE;
            break;

        case MBANDCID_ISTRACKING:
            if (_pmtbTracked && _pmtbTracked->v_TrackingSubContextMenu())
                return S_OK;
            else
                return S_FALSE;
            break;

        case MBANDCID_REPOSITION:

            // Don't reposition unless we're shown (Avoids artifacts onscreen of a bad positioning)
            if (_fShow)
            {
                // Don't forget to reposition US!!!
                IMenuPopup* pmdb;
                DWORD dwFlags = MPPF_REPOSITION | MPPF_NOANIMATE;

                // If we should force a reposition. This is so that we get
                // the trickle down reposition so things overlap correctly
                if (nCmdExecOpt)
                    dwFlags |= MPPF_FORCEZORDER;

                if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_IMenuPopup, (void**)&pmdb)))
                {
                    pmdb->Popup(NULL, NULL, dwFlags);
                    pmdb->Release();
                }

                // Reposition the Tracked sub menu based on the current popped up item 
                // since this pane has now moved
                // If they have a sub menu, tell them to reposition as well.
                if (_fInSubMenu && _pmtbTracked)
                {
                    IUnknown_QueryServiceExec(_pmpSubMenu, SID_SMenuBandChild,
                    &CGID_MenuBand, MBANDCID_REPOSITION, nCmdExecOpt, NULL, NULL);
                }
                _pmbState->PutTipOnTop();
            }
            break;

        case MBANDCID_REFRESH:
            InvalidateItem(NULL, SMINV_REFRESH);
            break;
            
        case MBANDCID_EXPAND:
            if (_pmtbShellFolder)
                _pmtbShellFolder->Expand(TRUE);

            if (_pmtbMenu)
                _pmtbMenu->Expand(TRUE);
            break;

        case MBANDCID_DRAGCANCEL:
            // If one of the Sub bands in the menu heirarchy has the drag 
            // (Either because of Drag enter or because of the drop) then 
            // we do not want to cancel. 
            if (!_pmbState->HasDrag())
                _CancelMode(MPOS_FULLCANCEL);
            break;

        case MBANDCID_EXECUTE:
            ASSERT(pvarargIn != NULL);
            if (_pmtbTop && _pmtbTop->IsWindowOwner((HWND)pvarargIn->ullVal) == S_OK)
                _pmtbTop->v_ExecItem((int)nCmdExecOpt);
            else if (_pmtbBottom && _pmtbBottom->IsWindowOwner((HWND)pvarargIn->ullVal) == S_OK)
                _pmtbBottom->v_ExecItem((int)nCmdExecOpt);
            _SiteOnSelect(MPOS_EXECUTE);
            break;
        }

        // Don't bother passing CGID_MenuBand commands to CToolBand
        return S_OK;
    }     
    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvarargIn, pvarargOut);    
}


/*----------------------------------------------------------
Purpose: IDockingWindow::CloseDW method.

*/
STDMETHODIMP CMenuBand::CloseDW(DWORD dw)
{
    // We don't want to destroy the band if it's cached. 
    // That means it's the caller's respocibility to Unset this bit and call CloseDW explicitly
    if (_dwFlags & SMINIT_CACHED)
        return S_OK;

    // Since we're blowing away all of the menus,
    // Top and bottom are invalid
    _pmtbTracked = _pmtbTop = _pmtbBottom = NULL;

    if (_pmtbMenu)
    {
        _pmtbMenu->v_Close();
    }

    if (_pmtbShellFolder)
    {
        _pmtbShellFolder->v_Close();
    }

    if (_pmpSubMenu)
    {
        _fInSubMenu = FALSE;
        IUnknown_SetSite(_pmpSubMenu, NULL);
        ATOMICRELEASE(_pmpSubMenu);
    }

    // We don't want our base class to blow this window away. It belongs to someone else.
    _hwnd = NULL;
    _fClosing = TRUE;
    
    return SUPERCLASS::CloseDW(dw);
}


/*----------------------------------------------------------
Purpose: IDockingWindow::ShowDW method

Notes:
    for the start menu (non-browser) case, we bracket* the top-level popup
    operation w/ a LockSetForegroundWindow so that another app can't steal
    the foreground and collapse our menu.  (nt5:172813: don't do it for
    the browser case since a) we don't want to and b) ShowDW(FALSE) isn't
    called until exit the browser so we'd be permanently locked!)
*/
STDMETHODIMP CMenuBand::ShowDW(BOOL fShow)
{   
    // Prevent rentrancy when we're already shown.
    ASSERT((int)_fShow == BOOLIFY(_fShow));
    if ((int)_fShow == BOOLIFY(fShow))
        return NOERROR;

    HRESULT hres = SUPERCLASS::ShowDW(fShow);

    if (!fShow)
    {
        _fShow = FALSE;
        if (_fTopLevel)
        {
            if (_fVertical) 
            {
                // (_fTopLevel && _fVertical) => start menu
                MyLockSetForegroundWindow(FALSE);
            }
            else if (_dwFlags & SMINIT_USEMESSAGEFILTER)
            {
                g_msgfilter.SetHook(FALSE, TRUE);
                g_msgfilter.SetTopMost(this);
            }

        }

        if ((_fTopLevel || _fParentIsHorizontal) && _pmbState)
        {
            // Reset to not have the drag when we collapse.
            _pmbState->HasDrag(FALSE);
            _pmbState->SetExpand(FALSE);
            _pmbState->SetUEMState(0);
        }

        _CallCB(SMC_EXITMENU);
    }
    else
    {
        _CallCB(SMC_INITMENU);

        _fClosing = FALSE;
        _fShow = TRUE;
        _GetFontMetrics();

        if (_fTopLevel)
        {
            // We set the context here so that the ReEngage causes the message filter
            // to start taking messages on a TopLevel::Show. This prevents a problem
            // where tracking doesn't work when switching between Favorites and Start Menu
            _pmbState->SetContext(this);
            g_msgfilter.SetContext(this, TRUE);

            g_msgfilter.ReEngage(_pmbState->GetContext());
            if (_hwndMenuOwner && _fVertical)
                SetForegroundWindow(_hwndMenuOwner);

            if (_fVertical) 
            {
                // (_fTopLevel && _fVertical) => start menu
                MyLockSetForegroundWindow(TRUE);
            }
            else if (_dwFlags & SMINIT_USEMESSAGEFILTER)
            {
                g_msgfilter.SetHook(TRUE, TRUE);
                g_msgfilter.SetTopMost(this);
            }

            _pmbState->CreateFader(_hwndParent);
        }
    }

    if (_pmtbShellFolder)
        _pmtbShellFolder->v_Show(_fShow, _fForceButtonUpdate);

    // Menu needs to be last so that it can update the seperator.
    if (_pmtbMenu)
        _pmtbMenu->v_Show(_fShow, _fForceButtonUpdate);

    if (_fPopupNewItemOnShow)
    {
        HWND hwnd = _pmbState->GetSubclassedHWND();

        PostMessage(hwnd? hwnd : _pmtbMenu->_hwndMB, g_nMBPopupOpen, 
            _nItemNew, MAKELPARAM(TRUE, TRUE));
        _fPopupNewItemOnShow = FALSE;
    }

    _fForceButtonUpdate = FALSE;
    return hres;
}


// BUGBUG (lamadio): move this to shlwapi
HRESULT IUnknown_QueryServiceExec(IUnknown* punk, REFGUID guidService, const GUID *guid,
                                 DWORD cmdID, DWORD cmdParam, VARIANT* pvarargIn, VARIANT* pvarargOut)
{
    HRESULT hres;
    IOleCommandTarget* poct;

    hres = IUnknown_QueryService(punk, guidService, IID_IOleCommandTarget, 
        (void**)&poct);
    if (SUCCEEDED(hres))
    {
        hres = poct->Exec(guid, cmdID, cmdParam, pvarargIn, pvarargOut);
        poct->Release();
    }

    return hres;
}


void CMenuBand::_GetFontMetrics()
{
    if (_pmbm)
        return;

    if (_fTopLevel)
    {
        ASSERT(_pmtbTop);
        // We need only 1 HWND
        _pmbm = new CMenuBandMetrics(_pmtbTop->_hwndMB);
    }
    else
    {
        AssertMsg(0, TEXT("When this menuband was created, someone forgot to set the metrics"));
        IOleCommandTarget *poct;
    
        HRESULT hres = IUnknown_QueryService(_punkSite, SID_SMenuBandTop, IID_IOleCommandTarget, (LPVOID *)&poct);
        if (SUCCEEDED(hres))
        {
            VARIANTARG vargOut;

            // Ask the toplevel menuband for their font info
            if (SUCCEEDED(poct->Exec(&CGID_MenuBand, MBANDCID_GETFONTS, 0, NULL, &vargOut)))
            {
                if (vargOut.vt == VT_UNKNOWN && vargOut.punkVal)
                {
                    vargOut.punkVal->QueryInterface(CLSID_MenuBandMetrics, (void**)&_pmbm);
                    vargOut.punkVal->Release();
                }
            }
            poct->Release();
        }
    }
}


/*----------------------------------------------------------
Purpose: IMenuPopup::OnSelect method

         This allows the child menubar to tell us when and how
         to bail out of the menu.
*/
STDMETHODIMP CMenuBand::OnSelect(DWORD dwType)
{
    int iIndex;

    switch (dwType)
    {
    case MPOS_CHILDTRACKING:
        // this means that our child did get tracked over it, so we should abort any timeout to destroy it

        if (_pmtbTracked)
        {
            HWND hwnd = _pmtbTracked->_hwndMB;
            if (_nItemTimer) 
            {
                _pmtbTracked->KillPopupTimer();
        
                // Use the command id of the SubMenu that we actually have cascaded out.
                iIndex = ToolBar_CommandToIndex(hwnd, _nItemSubMenu);
                ToolBar_SetHotItem(hwnd, iIndex);
            }
            KillTimer(hwnd, MBTIMER_DRAGOVER);
            _SiteOnSelect(dwType);
        }
        break;
        
    case MPOS_SELECTLEFT:
        if (!_fVertical)
            _OnSelectArrow(-1);
        else
        {
            // Cancel the child submenu.  Hitting left arrow is like
            // hitting escape.
            _SubMenuOnSelect(MPOS_CANCELLEVEL);
        }
        break;

    case MPOS_SELECTRIGHT:
        if (!_fVertical)
            _OnSelectArrow(1);
        else
        {
            // The right arrow gets propagated up to the top, so
            // a fully cascaded menu will be cancelled and the
            // top level menuband will move to the next menu to the
            // right.
            _SiteOnSelect(dwType);
        }
        break;
        
    case MPOS_CANCELLEVEL:
        // Forward onto submenu
        _SubMenuOnSelect(dwType);
        break;

    case MPOS_FULLCANCEL:
    case MPOS_EXECUTE:
        DEBUG_CODE( TraceMsg(TF_MENUBAND, "%d (pmb=%#08lx): CMenuToolbarBase received %s", 
                    DBG_THIS, MPOS_FULLCANCEL == dwType ? TEXT("MPOS_FULLCANCEL") : TEXT("MPOS_EXECUTE")); )

        _CancelMode(dwType);
        break;
    }
    return S_OK;    
}    

void CMenuBand::SetTrackMenuPopup(IUnknown* punk)
{ 
    ATOMICRELEASE(_pmpTrackPopup);
    if (punk)
    {
        punk->QueryInterface(IID_IMenuPopup, (void**)&_pmpTrackPopup);
    }
}


/*----------------------------------------------------------
Purpose: Set the currently tracked toolbar.  Only one
         of the toolbars can have the "activation" at one time.
*/
BOOL CMenuBand::SetTracked(CMenuToolbarBase* pmtb)
{
    if (pmtb == _pmtbTracked)
        return FALSE;

    if (_pmtbTracked)
    {
        // Tell the existing toolbar we're leaving him
        SendMessage(_pmtbTracked->_hwndMB, TB_SETHOTITEM2, -1, HICF_LEAVING);
    }

    _pmtbTracked = pmtb;

    if (_pmtbTracked)
    {
        // This is for accessibility.
        HWND hwnd = _pmtbTracked->_hwndMB;
        int iHotItem = ToolBar_GetHotItem(hwnd);

        if (iHotItem >= 0)
        {
            // Toolbar Items are 0 based, Accessibility apps require 1 based
            NotifyWinEvent(EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, 
                GetIndexFromChild(_pmtbTracked->GetFlags() & SMSET_TOP, iHotItem)); 
        }
    }

    return TRUE;
}


void CMenuBand::_OnSelectArrow(int iDir)
{
    _fKeyboardSelected = TRUE;
    int iIndex;

    if (!_pmtbTracked)
    {
        if (iDir < 0)
        {
            SetTracked(_pmtbBottom);
            iIndex = ToolBar_ButtonCount(_pmtbTracked->_hwndMB) - 1;
        }
        else
        {
            SetTracked(_pmtbTop);
            iIndex = 0;
        }

        // This can happen when going to the chevron.
        if (_pmtbTracked)
            _pmtbTracked->SetHotItem(iDir, iIndex, -1, HICF_ARROWKEYS);
    }
    else
    {
        HWND hwnd = _pmtbTracked->_hwndMB;
        iIndex = ToolBar_GetHotItem(hwnd);
        int iCount = ToolBar_ButtonCount(hwnd);
    
        // Set the hot item explicitly since this is invoked by the 
        // keyboard and the mouse could be anywhere.

        // cycle iIndex by iDir (add extra iCount to avoid negative number problems
        iIndex = (iIndex + iCount + iDir) % iCount; 

        ToolBar_SetHotItem(hwnd, iIndex);
    }

    if (_pmtbTracked)
    {
        NotifyWinEvent(EVENT_OBJECT_FOCUS, _pmtbTracked->_hwndMB, OBJID_CLIENT, 
            GetIndexFromChild(_pmtbTracked->GetFlags() & SMSET_TOP, iIndex));
    }
    _fKeyboardSelected = FALSE;
}

void CMenuBand::_CancelMode(DWORD dwType)
{
    // Tell the hosting site to cancel this level
    if (_fParentIsNotASite)
        UIActivateIO(FALSE, NULL);
    else
        _SiteOnSelect(dwType);
}    

HRESULT CMenuBand::OnPosRectChangeDB (LPRECT prc)
{
    // We want the HMENU portion to ALWAYS have the maximum allowed.
    RECT rcMenu = {0};
    SIZE sizeMenu = {0};
    SIZE sizeSF = {0};
    SIZE sizeMax;

    if (_pmtbMenu)
        _pmtbMenu->GetSize(&sizeMenu);

    if (_pmtbShellFolder)
        _pmtbShellFolder->GetSize(&sizeSF);

    if (sizeSF.cx > sizeMenu.cx)
        sizeMax = sizeSF;
    else
        sizeMax = sizeMenu;

    if (_pmtbMenu)
    {
        if (_pmtbMenu->GetFlags() & SMSET_TOP)
        {

            rcMenu.bottom = sizeMenu.cy;
            rcMenu.right = prc->right;
        }
        else
        {
            rcMenu.bottom = prc->bottom;
            rcMenu.right = prc->right;

            rcMenu.top = prc->bottom - sizeMenu.cy;
            rcMenu.left = 0;
        }

        _pmtbMenu->SetWindowPos(&sizeMax, &rcMenu, 0);
    }

    if (_pmtbShellFolder)
    {
        RECT rc = *prc;

        if (_pmtbShellFolder->GetFlags() & SMSET_TOP)
        {
            rc.bottom = prc->bottom - RECTHEIGHT(rcMenu) + 1;
        }
        else
        {
            rc.top = prc->top + RECTHEIGHT(rcMenu);
        }

        _pmtbShellFolder->SetWindowPos(&sizeMax, &rc, 0);
    }

    return NOERROR;
}


HRESULT IUnknown_OnSelect(IUnknown* punk, DWORD dwType, REFGUID guid)
{
    HRESULT hres;
    IMenuPopup * pmp;

    hres = IUnknown_QueryService(punk, guid, IID_IMenuPopup, 
                                 (LPVOID *)&pmp);
    if (SUCCEEDED(hres))
    {
        pmp->OnSelect(dwType);
        pmp->Release();
    }

    return hres;
}

HRESULT CMenuBand::_SiteOnSelect(DWORD dwType)
{
    return IUnknown_OnSelect(_punkSite, dwType, SID_SMenuPopup);
}

HRESULT CMenuBand::_SubMenuOnSelect(DWORD dwType)
{
    IMenuPopup* pmp = _pmpSubMenu;
    if (_pmpTrackPopup)
        pmp = _pmpTrackPopup;

    return IUnknown_OnSelect(pmp, dwType, SID_SMenuPopup);
}

HRESULT CMenuBand::GetTop(CMenuToolbarBase** ppmtbTop)
{
    *ppmtbTop = _pmtbTop;

    if (*ppmtbTop)
    {
        (*ppmtbTop)->AddRef();
        return NOERROR;
    }

    return E_FAIL;
}

HRESULT CMenuBand::GetBottom(CMenuToolbarBase** ppmtbBottom)
{
    *ppmtbBottom = _pmtbBottom;

    if (*ppmtbBottom)
    {
        (*ppmtbBottom)->AddRef();
        return NOERROR;
    }

    return E_FAIL;

}

HRESULT CMenuBand::GetTracked(CMenuToolbarBase** ppmtbTracked)
{
    *ppmtbTracked = _pmtbTracked;

    if (*ppmtbTracked)
    {
        (*ppmtbTracked)->AddRef();
        return NOERROR;
    }

    return E_FAIL;

}

HRESULT CMenuBand::GetParentSite(REFIID riid, void** ppvObj)
{
    if (_punkSite)
        return _punkSite->QueryInterface(riid, ppvObj);

    return E_FAIL;
}

HRESULT CMenuBand::GetState(BOOL* pfVertical, BOOL* pfOpen)
{
    *pfVertical = _fVertical;
    *pfOpen = _fMenuMode;
    return NOERROR;
}

HRESULT CMenuBand::DoDefaultAction(VARIANT* pvarChild)
{
    if (pvarChild->lVal != CHILDID_SELF)
    {
        CMenuToolbarBase* pmtb = (pvarChild->lVal & TOOLBAR_MASK)? _pmtbTop : _pmtbBottom;
        int idCmd = GetButtonCmd(pmtb->_hwndMB, (pvarChild->lVal & ~TOOLBAR_MASK) - 1);

        SendMessage(pmtb->_hwndMB, TB_SETHOTITEM2, idCmd, HICF_OTHER | HICF_TOGGLEDROPDOWN);
    }
    else
    {
        _CancelMode(MPOS_CANCELLEVEL);
    }

    return NOERROR;
}

HRESULT CMenuBand::GetSubMenu(VARIANT* pvarChild, REFIID riid, void** ppvObj)
{
    HRESULT hres = E_FAIL;
    CMenuToolbarBase* pmtb = (pvarChild->lVal & TOOLBAR_MASK)? _pmtbTop : _pmtbBottom;
    int idCmd = GetButtonCmd(pmtb->_hwndMB, (pvarChild->lVal & ~TOOLBAR_MASK) - 1);

    *ppvObj = NULL;

    if (idCmd != -1 && pmtb)
    {
        hres = pmtb->v_GetSubMenu(idCmd, &SID_SMenuBandChild, riid, ppvObj);
    }

    return hres;
}

HRESULT CMenuBand::IsEmpty()
{
    BOOL fReturn = TRUE;
    if (_pmtbShellFolder)
        fReturn = _pmtbShellFolder->IsEmpty();

    if (fReturn && _pmtbMenu)
        fReturn = _pmtbMenu->IsEmpty();

    return fReturn? S_OK : S_FALSE;
}


//----------------------------------------------------------------------------
// CMenuBandMetrics
//
//----------------------------------------------------------------------------


COLORREF GetDemotedColor()
{
    WORD iHue;
    WORD iLum;
    WORD iSat;
    COLORREF clr = (COLORREF)GetSysColor(COLOR_MENU);
    HDC hdc = GetDC(NULL);

    // Office CommandBars use this same algorithm for their "intellimenus"
    // colors.  We prefer to call them "expando menus"...

    if (hdc)
    {
        int cColors = GetDeviceCaps(hdc, BITSPIXEL);
        
        ReleaseDC(NULL, hdc);
        
        switch (cColors)
        {
        case 4:     // 16 Colors
        case 8:     // 256 Colors
            // Default to using Button Face
            break;
            
        default:    // 256+ colors
            
            ColorRGBToHLS(clr, &iHue, &iLum, &iSat);
            
            if (iLum > 220)
                iLum -= 20;
            else if (iLum <= 20)
                iLum += 40;
            else
                iLum += 20;
            
            clr = ColorHLSToRGB(iHue, iLum, iSat);
            break;
        }
    }
    
    return  clr;
}


ULONG CMenuBandMetrics::AddRef()
{
    return ++_cRef;
}

ULONG CMenuBandMetrics::Release()
{
    ASSERT(_cRef > 0);
    if (--_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CMenuBandMetrics::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
    }
    else if (IsEqualIID(riid, CLSID_MenuBandMetrics))
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_FAIL;
    }

    AddRef();
    return S_OK;
}

CMenuBandMetrics::CMenuBandMetrics(HWND hwnd)
                : _cRef(1)
{
    _SetMenuFont();
    _SetArrowFont(hwnd);
    _SetChevronFont(hwnd);
#ifndef DRAWEDGE
    _SetPaintMetrics(hwnd);
#endif
    _SetTextBrush(hwnd);
    _SetColors();

    HIGHCONTRAST hc = {sizeof(HIGHCONTRAST)};

    if (SystemParametersInfoA(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
    {
        _fHighContrastMode = (HCF_HIGHCONTRASTON & hc.dwFlags);
    }
}

CMenuBandMetrics::~CMenuBandMetrics()
{
    if (_hFontMenu)
        DeleteObject(_hFontMenu);

    if (_hFontArrow)
        DeleteObject(_hFontArrow);

    if (_hFontChevron)
        DeleteObject(_hFontChevron);

    if (_hbrText)
        DeleteObject(_hbrText);

#ifndef DRAWEDGE
    if (_hPenHighlight)
        DeleteObject(_hPenHighlight);

    if (_hPenShadow)
        DeleteObject(_hPenShadow);
#endif
}

HFONT CMenuBandMetrics::_CalcFont(HWND hwnd, LPCTSTR pszFont, DWORD dwCharSet, TCHAR ch, int* pcx, 
                                  int* pcy, int* pcxMargin, int iOrientation, int iWeight)
{
    ASSERT(hwnd);

    HFONT hFontOld, hFontRet;
    TEXTMETRIC tm;
    RECT rect={0};

    int cx, cy, cxM;

    HDC hdc = GetDC(hwnd);
    hFontOld = (HFONT)SelectObject(hdc, _hFontMenu);
    GetTextMetrics(hdc, &tm);

    // Set the font height (based on original USER code)
    cy = ((tm.tmHeight + tm.tmExternalLeading + GetSystemMetrics(SM_CYBORDER)) & 0xFFFE) - 1;

    // Use the menu font's avg character width as the margin.
    cxM = tm.tmAveCharWidth; // Not exactly how USER does it, but close

    // Shlwapi wraps the ansi/unicode behavior.
    hFontRet = CreateFontWrap(cy, 0, iOrientation, 0, iWeight, 0, 0, 0, dwCharSet, 0, 0, 0, 0, pszFont);
    if (EVAL(hFontRet))
    {
        // Calc width of arrow using this new font
        SelectObject(hdc, hFontRet);
        if (EVAL(DrawText(hdc, &ch, 1, &rect, DT_CALCRECT | DT_SINGLELINE | DT_LEFT | DT_VCENTER)))
            cx = rect.right;
        else
            cx = tm.tmMaxCharWidth;
    }
    else
    {
        cx = tm.tmMaxCharWidth;
    }
    
    SelectObject(hdc, hFontOld);   
    ReleaseDC(hwnd, hdc);

    *pcx = cx;
    *pcy = cy;
    *pcxMargin = cxM;
    
    return hFontRet;

}


/*
    Call after _SetMenuFont()
*/
void CMenuBandMetrics::_SetChevronFont(HWND hwnd)
{
    ASSERT(!_hFontChevron);
    TCHAR szPath[MAX_PATH];

    NONCLIENTMETRICSA ncm;

    ncm.cbSize = sizeof(ncm);
    // Should only fail with bad parameters...
    EVAL(SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0));
   
    // Obtain the font's metrics
    SHAnsiToTChar(ncm.lfMenuFont.lfFaceName, szPath, ARRAYSIZE(szPath));
    _hFontChevron = _CalcFont(hwnd, szPath, DEFAULT_CHARSET, CH_MENUARROW, &_cxChevron, &_cyChevron, 
        &_cxChevron, -900, FW_NORMAL);
}

/*
    Call after _SetMenuFont()
*/
void CMenuBandMetrics::_SetArrowFont(HWND hwnd)
{
    ASSERT(!_hFontArrow);
    ASSERT(_hFontMenu);
   
    // Obtain the font's metrics
    if (_hFontMenu)
    {
        _hFontArrow = _CalcFont(hwnd, szfnMarlett, SYMBOL_CHARSET, CH_MENUARROW, &_cxArrow, &_cyArrow, 
            &_cxMargin, 0, FW_NORMAL);
    }
    else
    {
        _cxArrow = _cyArrow = _cxMargin = 0;
    }
}

void CMenuBandMetrics::_SetMenuFont()
{
    NONCLIENTMETRICSA ncm;

    ncm.cbSize = sizeof(ncm);
    // Should only fail with bad parameters...
    EVAL(SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0));
    // Should only fail under low mem conditions...
    
    EVAL(_hFontMenu = CreateFontIndirectA(&ncm.lfMenuFont));
}

void CMenuBandMetrics::_SetColors()
{
    _clrBackground = GetSysColor(COLOR_MENU);
    _clrMenuText = GetSysColor(COLOR_MENUTEXT);
    _clrDemoted = GetDemotedColor();
}


#ifndef DRAWEDGE
// Office "IntelliMenu" style
void CMenuBandMetrics::_SetPaintMetrics(HWND hwnd)
{
    DWORD dwSysHighlight = GetSysColor(COLOR_3DHIGHLIGHT);
    DWORD dwSysShadow = GetSysColor(COLOR_3DSHADOW);

    _hPenHighlight = CreatePen(PS_SOLID, 1, dwSysHighlight);
    _hPenShadow = CreatePen(PS_SOLID, 1, dwSysShadow);
}
#endif

void CMenuBandMetrics::_SetTextBrush(HWND hwnd)
{
    _hbrText = CreateSolidBrush(GetSysColor(COLOR_MENUTEXT));
}


CMenuBandState::CMenuBandState()                
{ 
    // We will default to NOT show the keyboard cues. This
    // is overridden based on the User Settings.
    _fKeyboardCue = FALSE;
}

CMenuBandState::~CMenuBandState()
{
    ATOMICRELEASE(_ptFader);

    ATOMICRELEASE(_pScheduler);

    if (_hwndToolTip)
        DestroyWindow(_hwndToolTip);

    if (_hwndWorker)
        DestroyWindow(_hwndWorker);
}

int CMenuBandState::GetKeyboardCue()
{
    return _fKeyboardCue;
}

void CMenuBandState::SetKeyboardCue(BOOL fKC)
{
    _fKeyboardCue = fKC;
}

IShellTaskScheduler* CMenuBandState::GetScheduler()
{
    if (!_pScheduler)
    {
        CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC,
                               IID_IShellTaskScheduler, (void **) &_pScheduler);
    }

    if (_pScheduler)
        _pScheduler->AddRef();


    return _pScheduler;
}

BOOL CMenuBandState::FadeRect(PRECT prc, PFNFADESCREENRECT pfn, LPVOID pvParam)
{
    BOOL    fFade = FALSE;
    SystemParametersInfo(SPI_GETSELECTIONFADE, 0, &fFade, 0);
    if (g_bRunOnNT5 && _ptFader && fFade)
    {
        // Set the callback into the fader window. Do this each time, as the pane 
        // may have changed between fades
        if (_ptFader->FadeRect(prc, pfn, pvParam))
        {
            IShellTaskScheduler* pScheduler = GetScheduler();
            if (pScheduler)
            {
                fFade = pScheduler->AddTask(_ptFader, TASKID_Fader, 
                    ITSAT_DEFAULT_LPARAM, ITSAT_DEFAULT_PRIORITY) == S_OK;
            }
        }
    }

    return fFade;
}

void CMenuBandState::CreateFader(HWND hwnd)
{
    // We do this on first show, because in the Constuctor of CMenuBandState,
    // the Window classes might not be registered yet (As is the case with start menu).
    if (g_bRunOnNT5 && !_ptFader)
    {
        _ptFader = new CFadeTask();
    }
}


void CMenuBandState::CenterOnButton(HWND hwndTB, BOOL fBalloon, int idCmd, LPTSTR pszTitle, LPTSTR pszTip)
{
    // Balloon style holds presidence over info tips
    if (_fTipShown && _fBalloonStyle)
        return;

    if (!_hwndToolTip)
    {
        _hwndToolTip = CreateWindow(TOOLTIPS_CLASS, NULL,
                                         WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         NULL, NULL, g_hinst,
                                         NULL);

        if (_hwndToolTip) 
        {
            // set the version so we can have non buggy mouse event forwarding
            SendMessage(_hwndToolTip, CCM_SETVERSION, COMCTL32_VERSION, 0);
            SendMessage(_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);

        }
    }

    if (_hwndToolTip)
    {
        // Collapse the previous tip because we're going to be doing some stuff to it before displaying again.
        SendMessage(_hwndToolTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);

        // Balloon tips don't have a border, but regular tips do. Swap now...
        SHSetWindowBits(_hwndToolTip, GWL_STYLE, TTS_BALLOON | WS_BORDER, (fBalloon) ? TTS_BALLOON : WS_BORDER);

        if (pszTip && pszTip[0])
        {
            RECT rc;
            TOOLINFO ti = {0};
            ti.cbSize = SIZEOF(ti);

            // This was pretty bad: I kept adding tools, but never deleteing them. Now we get rid of the current
            // one then add the new one.
            if (SendMessage(_hwndToolTip, TTM_ENUMTOOLS, 0, (LPARAM)&ti))
            {
                SendMessage(_hwndToolTip, TTM_DELTOOL, 0, (LPARAM)&ti);   // Delete the current tool.
            }

            ti.cbSize = SIZEOF(ti);
            ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | (fBalloon? TTF_TRACK : 0);
            ti.hwnd = hwndTB;
            ti.uId = (UINT_PTR)hwndTB;
            SendMessage(_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);

            ti.lpszText = pszTip;
            SendMessage(_hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

            SendMessage(_hwndToolTip, TTM_SETTITLE, TTI_INFO, (LPARAM)pszTitle);

            SendMessage(hwndTB, TB_GETRECT, idCmd, (LPARAM)&rc);
            MapWindowPoints(hwndTB, HWND_DESKTOP, (POINT*)&rc, 2);

            // Notice the correction for the bottom: gsierra wanted it up a couple of pixels.
            SendMessage(_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.bottom - 3));

            SetWindowPos(_hwndToolTip, HWND_TOPMOST,
                         0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            SendMessage(_hwndToolTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
            _fTipShown = TRUE;
            _fBalloonStyle = fBalloon;
        }
    }

}

void CMenuBandState::HideTooltip(BOOL fAllowBalloonCollapse)
{
    if (_hwndToolTip && _fTipShown)
    {
        // Now we're going to latch the Balloon style. The rest of menuband blindly
        // collapses the tooltip when selection changes. Here's where we say "Don't collapse
        // the chevron balloon tip because of a selection change."
        if ((_fBalloonStyle && fAllowBalloonCollapse) || !_fBalloonStyle)
        {
            SendMessage(_hwndToolTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);
            _fTipShown = FALSE;
        }
    }
}

void CMenuBandState::PutTipOnTop()
{
    // Force the tooltip to the topmost.
    if (_hwndToolTip)
    {
        SetWindowPos(_hwndToolTip, HWND_TOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

HWND CMenuBandState::GetWorkerWindow(HWND hwndParent)
{
    if (!_hwndSubclassed)
        return NULL;

    if (!_hwndWorker)
    {
        // We need a worker window, so that dialogs show up on top of our menus.
        // HiddenWndProc is included from sftbar.h
        _hwndWorker = SHCreateWorkerWindow(HiddenWndProc, _hwndSubclassed, 
            WS_EX_TOOLWINDOW, WS_POPUP, 0, (void*)_hwndSubclassed);
    }

    //hwndParent is unused at this time. I plan on using it to prevent the parenting to the subclassed window.

    return _hwndWorker;
}
