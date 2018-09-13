// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  STATBAR.CPP
//
//  This knows how to talk to COMCTL32's status bar control.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "statbar.h"

#define NOTOOLBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOHOTKEY
#define NOHEADER
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE
#include <commctrl.h>

#define LPRECT_IN_LPRECT(rc1,rc2)   ((rc1->left >= rc2->left) && \
                                     (rc1->right <= rc2->right) && \
                                     (rc1->top >= rc2->top) && \
                                     (rc1->bottom <= rc2->bottom))

#define RECT_IN_RECT(rc1,rc2)   ((rc1.left >= rc2.left) && \
                                 (rc1.right <= rc2.right) && \
                                 (rc1.top >= rc2.top) && \
                                 (rc1.bottom <= rc2.bottom))


// --------------------------------------------------------------------------
//
//  CreateStatusBarClient()
//
//  EXTERNAL for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateStatusBarClient(HWND hwnd, long idChildCur, REFIID riid, void** ppv)
{
    HRESULT hr;
    CStatusBar32 * pstatus;

    InitPv(ppv);

    pstatus = new CStatusBar32(hwnd, idChildCur);
    if (!pstatus)
        return(E_OUTOFMEMORY);

    hr = pstatus->QueryInterface(riid, ppv);
    if (!SUCCEEDED(hr))
        delete pstatus;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::CStatusBar32()
//
// --------------------------------------------------------------------------
CStatusBar32::CStatusBar32(HWND hwnd, long iChildCur)
{
    Initialize(hwnd, iChildCur);
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::SetupChildren()
//
// --------------------------------------------------------------------------
void CStatusBar32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, SB_GETPARTS, 0, 0);
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatusBar32::get_accName(VARIANT varChild, BSTR* pszValue)
{
    LPTSTR  lpszValue;
    UINT    cchValue;
    HANDLE  hProcess;
    LPTSTR  lpszValueLocal;

    InitPv(pszValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(S_FALSE);

    varChild.lVal--;
    cchValue = SendMessageINT(m_hwnd, SB_GETTEXTLENGTH, varChild.lVal, 0);

    // HIGHWORD is type info, LOWORD is length
    cchValue = LOWORD( cchValue );

    if (! cchValue)
        return(S_FALSE);

    cchValue++;
    lpszValue = (LPTSTR)SharedAlloc((cchValue*sizeof(TCHAR)),m_hwnd,&hProcess);
    if (! lpszValue)
        return(E_OUTOFMEMORY);

    if (SendMessage(m_hwnd, SB_GETTEXT, varChild.lVal, (LPARAM)lpszValue))
    {
        lpszValueLocal = (LPTSTR)LocalAlloc (LPTR,cchValue*sizeof(TCHAR));
        if (! lpszValueLocal)
        {
            SharedFree (lpszValue,hProcess);
            return(E_OUTOFMEMORY);
        }
        SharedRead (lpszValue,lpszValueLocal,cchValue*sizeof(TCHAR),hProcess);

        if (*lpszValueLocal)
            *pszValue = TCharSysAllocString(lpszValueLocal);

        SharedFree(lpszValue,hProcess);
        LocalFree (lpszValueLocal);

        if (! *pszValue)
            return(E_OUTOFMEMORY);

        return(S_OK);
    }
    else
    {
        SharedFree(lpszValue,hProcess);
        return(S_FALSE);
    }
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatusBar32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    if (! varChild.lVal)
        pvarRole->lVal = ROLE_SYSTEM_STATUSBAR;
    else
        pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatusBar32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Status bar
    //
    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    //
    // Status items
    //
    pvarState->vt = VT_I4;
    pvarState->lVal = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatusBar32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    LPRECT  lprc;
    RECT    rcLocal;
    HANDLE  hProcess;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    lprc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (! lprc)
        return(E_OUTOFMEMORY);

    if (SendMessage(m_hwnd, SB_GETRECT, varChild.lVal-1, (LPARAM)lprc))
    {
        SharedRead (lprc,&rcLocal,sizeof(RECT),hProcess);
        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

        *pxLeft = rcLocal.left;
        *pyTop = rcLocal.top;
        *pcxWidth = rcLocal.right - rcLocal.left;
        *pcyHeight = rcLocal.bottom - rcLocal.top;

        SharedFree(lprc,hProcess);

        return(S_OK);
    }
    else
    {
        SharedFree(lprc,hProcess);
        return(S_FALSE);
    }
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatusBar32::accNavigate(long dwNavDir, VARIANT varStart, 
    VARIANT* pvarEnd)
{
    LONG    lEnd;

    InitPvar(pvarEnd);

    //CWO, 1/31/97, #14023, check for HWND children
    if ((!ValidateChild(&varStart) && !ValidateHwnd(&varStart)) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    // If the action is any navigation other than first or last
    // and the child is 0
    if ((dwNavDir < NAVDIR_FIRSTCHILD) && !varStart.lVal)
        // Then call the CClient navigation method
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    // If the starting point is not a child ID, but is an
    // HWNDID, then we have to convert from the HWND to the
    // child ID by seeing which child area has coordinates
    // that contain the HWND.
    if (IsHWNDID(varStart.lVal))
        varStart.lVal = ConvertHwndToID(varStart.lVal);

    switch (dwNavDir)
    {
        case NAVDIR_FIRSTCHILD:
            lEnd = 1;
            goto NextStatusItem;

        case NAVDIR_LASTCHILD:
            lEnd = m_cChildren;
            break;

        case NAVDIR_NEXT:
        case NAVDIR_RIGHT:
            lEnd = varStart.lVal+1;
NextStatusItem:
            if (lEnd > m_cChildren)
                lEnd = 0;
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_LEFT:
            lEnd = varStart.lVal-1;
            break;

        default:
            lEnd = 0;
            break;
           
    }

    if (lEnd)
    {
        // When we get here, we know which section to look in. 
        // We need to check all our child windows to see if they
        // are contained within that section and return a dispatch
        // interface if so.
        lEnd = FindChildWindowFromID (lEnd);
        if (IsHWNDID(lEnd))
        {
            pvarEnd->vt = VT_DISPATCH;
            return (AccessibleObjectFromWindow(HwndFromHWNDID(lEnd),OBJID_WINDOW,
                                    IID_IDispatch, (void**)&pvarEnd->pdispVal));
        }
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CStatusBar32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatusBar32::accHitTest(long x, long y, VARIANT* pvarEnd)
{
    HRESULT hr;
    int*    lpi;
    HANDLE  hProcess;

    SetupChildren();

    //
    // Are we in the client area at all?
    //
    hr = CClient::accHitTest(x, y, pvarEnd);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarEnd->vt != VT_I4) || (pvarEnd->lVal != 0) ||
        !m_cChildren)
        return(hr);

    //
    // Yes.  What item are we over?
    //
    lpi = (LPINT)SharedAlloc((m_cChildren*sizeof(DWORD)),m_hwnd,&hProcess);
    if (! lpi)
        return(E_OUTOFMEMORY);

    if (SendMessage(m_hwnd, SB_GETPARTS, m_cChildren, (LPARAM)lpi))
    {
        LPINT   lpiLocal;
        POINT   pt;
        int     iPart;
        int     xCur;

        lpiLocal = (LPINT)LocalAlloc (LPTR,m_cChildren*sizeof(DWORD));
        if (! lpiLocal)
        {
            SharedFree (lpi,hProcess);
            return(E_OUTOFMEMORY);
        }
        SharedRead (lpi,lpiLocal,m_cChildren*sizeof(DWORD),hProcess);
        // 
        // Charming fact:  The right side of the last item can be -1,
        // meaning extend all the way to the right.  Turn this into MAXINT
        // so the comparison loop below will work OK.
        //

        //
        // This gets us back the right sides of each item, in order.
        //
        pt.x = x;
        pt.y = y;
        ScreenToClient(m_hwnd, &pt);

        xCur = 0;
        for (iPart = 0; iPart < m_cChildren; iPart++)
        {
            if (lpiLocal[iPart] == -1)
                lpiLocal[iPart] = 0x7FFFFFFF;

            if ((pt.x >= xCur) && (pt.x < lpiLocal[iPart]))
            {
                pvarEnd->lVal = iPart+1;
                break;
            }

            xCur = lpiLocal[iPart];
        }
    }

    SharedFree(lpi,hProcess);

    return(S_OK);
}


// --------------------------------------------------------------------------
// ConvertHwndToID
//
// This tries to find which section of the status bar the window child is
// in.
//
//  Parameters:
//      long    HwndID - this should be an HWNDID - an HWND with the high
//                      bit turned on
//  Returns:
//      long indicating which section the window is in. We will return a
//      1-based number, unless we didn't find anything, when we return 0.
// --------------------------------------------------------------------------
long CStatusBar32::ConvertHwndToID (long HwndID)
{
LPRECT  lprcPart;
int     nParts;
int     i;
RECT    rcPartLocal;
RECT    rcWindowLocal;
HANDLE  hProcess2;

    lprcPart = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess2);
    if (lprcPart == NULL)
        return(0);

    if (!GetWindowRect (HwndFromHWNDID(HwndID),&rcWindowLocal))
    {
        SharedFree(lprcPart,hProcess2);
        return (0);
    }

    nParts = SendMessageINT(m_hwnd,SB_GETPARTS,0,0);
    for (i=0;i<nParts;i++)
    {
        SendMessage (m_hwnd,SB_GETRECT,i,(LPARAM)lprcPart);
        SharedRead (lprcPart,&rcPartLocal,sizeof(RECT),hProcess2);
        MapWindowPoints (m_hwnd,NULL,(LPPOINT)&rcPartLocal,2);

        if (RECT_IN_RECT(rcWindowLocal,rcPartLocal))
        {
            SharedFree(lprcPart,hProcess2);
            return (i+1);
        }
    }// end for

    SharedFree(lprcPart,hProcess2);
    return (0);
}

// --------------------------------------------------------------------------
// FindChildWindowFromID 
//
// This tries to find a child window inside a given part of a status bar.
//
//  Parameters:
//      long    ID - this should be a 1-based child ID indicating which
//                  part of the status bar to check for a child window.
//  Returns:
//      long that actually contains an HWNDID, or just the value passed
//      in if there was no child window.
// --------------------------------------------------------------------------
long CStatusBar32::FindChildWindowFromID (long ID)
{
LPRECT  lprcPart;
HWND    hwndChild;
RECT    rcWindowLocal;
RECT    rcPartLocal;
HANDLE  hProcess2;

    lprcPart = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess2);
    if (lprcPart == NULL)
        return(0);

    SendMessage (m_hwnd,SB_GETRECT,ID-1,(LPARAM)lprcPart);
    SharedRead (lprcPart,&rcPartLocal,sizeof(RECT),hProcess2);
    MapWindowPoints (m_hwnd,NULL,(LPPOINT)&rcPartLocal,2);

    hwndChild = ::GetWindow(m_hwnd,GW_CHILD);
    while (hwndChild)
    {
        GetWindowRect (hwndChild,&rcWindowLocal);
        if (RECT_IN_RECT(rcWindowLocal,rcPartLocal))
        {
            SharedFree(lprcPart,hProcess2);
            return (HWNDIDFromHwnd(hwndChild));
        }
        hwndChild = ::GetWindow(hwndChild,GW_HWNDNEXT);
    }
    SharedFree(lprcPart,hProcess2);
    return (ID);
}
