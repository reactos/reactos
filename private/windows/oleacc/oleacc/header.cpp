// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  HEADER.CPP
//
//  This knows how to talk to COMCTL32's header control.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "header.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOHOTKEY
#define NOTABCONTROL
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTOOLBAR
#define NOANIMATE
#include <commctrl.h>

#define MAX_HEADER_TEXT 80

// --------------------------------------------------------------------------
//
//  CreateHeaderClient()
//
// --------------------------------------------------------------------------
HRESULT CreateHeaderClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvHeader)
{
    CHeader32 * pheader;
    HRESULT     hr;

    InitPv(ppvHeader);

    pheader = new CHeader32(hwnd, idChildCur);
    if (!pheader)
        return(E_OUTOFMEMORY);

    hr = pheader->QueryInterface(riid, ppvHeader);
    if (!SUCCEEDED(hr))
        delete pheader;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CHeader32::CHeader32()
//
// --------------------------------------------------------------------------
CHeader32::CHeader32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  CHeader32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::get_accName(VARIANT varChild, BSTR* pszName)
{
LPHDITEM    lphd;
LPTSTR      lpszTextShared;
HANDLE      hProcess;
TCHAR       rgText[MAX_HEADER_TEXT];
int         nSomeInt;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accName(varChild, pszName));

    //
    // Try to get the name the easy way, by asking first.
    //
    // Need to be careful here, because we can only read and
    // write Shared Memory using the SharedRead and SharedWrite
    // functions. Need to remember what memory is where.
    //
    lphd = (LPHDITEM)SharedAlloc(sizeof(HDITEM)+((MAX_HEADER_TEXT+1)*sizeof(TCHAR)),
                                  m_hwnd,&hProcess);
    if (!lphd)
        return(E_OUTOFMEMORY);

    lpszTextShared = (LPTSTR)(lphd+1);
    SharedWrite (&lpszTextShared,&lphd->pszText,sizeof(LPTSTR),hProcess);
    nSomeInt = MAX_HEADER_TEXT;
    SharedWrite (&nSomeInt,&lphd->cchTextMax,sizeof(int),hProcess);
    nSomeInt = HDI_TEXT;
    SharedWrite (&nSomeInt,&lphd->mask,sizeof(UINT),hProcess);

    if (SendMessage(m_hwnd, HDM_GETITEM, varChild.lVal-1, (LPARAM)lphd))
    {
        SharedRead (lpszTextShared,rgText,MAX_HEADER_TEXT*sizeof(TCHAR),hProcess);
        if (*rgText)
            *pszName = TCharSysAllocString((LPTSTR)&rgText);
    }

    SharedFree(lphd,hProcess);

    //
    // BOGUS!  On failure, use tooltip trick like toolbar.
    //
    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CHeader32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (!varChild.lVal)
        pvarRole->lVal = ROLE_SYSTEM_LIST;
    else
        pvarRole->lVal = ROLE_SYSTEM_COLUMNHEADER;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CHeader32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    WINDOWINFO wi;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

	pvarState->lVal |= STATE_SYSTEM_READONLY;

    if (!MyGetWindowInfo(m_hwnd, &wi))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }
    
	if (!(wi.dwStyle & WS_VISIBLE))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

	if (wi.dwStyle & HDS_HIDDEN)
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

	if (wi.dwStyle & HDS_BUTTONS)
        pvarState->lVal |= 0;		// BOGUS! Indicate whether you can click or not.

    return(S_OK);
}

// --------------------------------------------------------------------------
//
//  CHeader32::get_accDefaultAction()
//
//  Since the default action for a header is really determined by the
//  creator of the header control, the best we can do is click on
//  the thing, and return "click" as the default action string.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
WINDOWINFO wi;

    InitPv(pszDefAction);

    //
    // Validate.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (MyGetWindowInfo(m_hwnd, &wi))
    {
    	if (wi.dwStyle & WS_VISIBLE)
        {
            if (!(wi.dwStyle & HDS_HIDDEN))
            {
                if (wi.dwStyle & HDS_BUTTONS)
                    return (HrCreateString(STR_CLICK, pszDefAction));
            }
        }
    }

    return(E_NOT_APPLICABLE);
}

// --------------------------------------------------------------------------
//
//  CHeader32::accDoDefaultAction()
//
//  As noted above, we really don't know what the default action for a 
//  header is, so unless the parent overrides us, we'll just do a
//  click on the thing.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::accDoDefaultAction(VARIANT varChild)
{
WINDOWINFO wi;
RECT		rcLoc;
HRESULT		hr;

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (MyGetWindowInfo(m_hwnd, &wi))
    {
    	if (wi.dwStyle & WS_VISIBLE)
        {
            if (!(wi.dwStyle & HDS_HIDDEN))
            {
                if (wi.dwStyle & HDS_BUTTONS)
                {
	                hr = accLocation(&rcLoc.left,&rcLoc.top,&rcLoc.right,&rcLoc.bottom,varChild);
	                if (!SUCCEEDED (hr))
		                return (hr);

	                if (ClickOnTheRect(&rcLoc,m_hwnd,FALSE))
		                return (S_OK);
                }
            }
        }
    }
    return(E_NOT_APPLICABLE);
}

// --------------------------------------------------------------------------
//
//  CHeader32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
LPRECT  lprc;
HANDLE  hProcess;
RECT    rcItem;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    //
    // Allocate a shared memory LPRECT and get the item's rectangle.
    //
    lprc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprc)
        return(E_OUTOFMEMORY);

    if (SendMessage(m_hwnd, HDM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprc))
    {
        SharedRead (lprc,&rcItem,sizeof(RECT),hProcess);

        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcItem, 2);

        *pxLeft = rcItem.left;
        *pyTop = rcItem.top;
        *pcxWidth = rcItem.right - rcItem.left;
        *pcyHeight = rcItem.bottom - rcItem.top;
    }

    SharedFree(lprc,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHeader32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
LPINT   lpiShared;
LPINT   lpiLocal;
long    lPosition = 0;
int     iOrder;
HANDLE  hProcess;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    Assert(NAVDIR_LASTCHILD > NAVDIR_FIRSTCHILD);
    Assert(NAVDIR_MAX == NAVDIR_LASTCHILD+1);

    // Peer navigation among frame elements
    if ((!varStart.lVal) && (dwNavDir < NAVDIR_FIRSTCHILD))
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    //
    // Get the order array.
    //
    lpiShared = (LPINT)SharedAlloc((m_cChildren*sizeof(INT)),m_hwnd,&hProcess);
    if (!lpiShared)
        return(E_OUTOFMEMORY);

    lpiLocal = (LPINT)LocalAlloc (LPTR,(m_cChildren*sizeof(INT)));
    if (!lpiLocal)
    {
        SharedFree (lpiShared,hProcess);
        return(E_OUTOFMEMORY);
    }

    SendMessage(m_hwnd, HDM_GETORDERARRAY, m_cChildren, (LPARAM)lpiShared);

    //
    // BOGUS!  Only works for column headers (horizontal), not row
    // headers (vertical).
    // 
    SharedRead (lpiShared,lpiLocal,(m_cChildren*sizeof(INT)),hProcess);

    // Get the position of the item we want to move to
    switch (dwNavDir)
    {
        case NAVDIR_FIRSTCHILD:
            lPosition = 0;
            break;

        case NAVDIR_LASTCHILD:
            if (!m_cChildren)
                goto NavigateEnd;
            lPosition = m_cChildren - 1;
            break;

        case NAVDIR_NEXT:
        case NAVDIR_RIGHT:
            lPosition = lpiLocal[varStart.lVal-1];
            ++lPosition;
            if (lPosition >= m_cChildren)
                goto NavigateEnd;
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_LEFT:
            lPosition = lpiLocal[varStart.lVal-1];
            if (!lPosition)
                goto NavigateEnd;
            --lPosition;
            break;

        case NAVDIR_UP:
        case NAVDIR_DOWN:
            goto NavigateEnd;
            break;
    }

    //
    // Find the item with this positional value
    //
    for (iOrder = 0; iOrder < m_cChildren; iOrder++)
    {
        if (lpiLocal[iOrder] == lPosition)
        {
            pvarEnd->vt = VT_I4;
            pvarEnd->lVal = iOrder+1;
            break;
        }
    }

NavigateEnd:
    SharedFree(lpiShared,hProcess);
    LocalFree (lpiLocal);

    return((pvarEnd->vt == VT_EMPTY) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CHeader32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHeader32::accHitTest(long x, long y, VARIANT* pvarEnd)
{
HRESULT         hr;
LPHDHITTESTINFO lphi;
HANDLE          hProcess;
POINT           ptTest;

    InitPvar(pvarEnd);
    SetupChildren();

    //
    // Is the point in our client?  If not, bail.
    //
    hr = CClient::accHitTest(x, y, pvarEnd);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarEnd->vt != VT_I4) || (pvarEnd->lVal != 0))
        return(hr);

    //
    // It is.  Do hit test stuff.
    //
    lphi = (LPHDHITTESTINFO)SharedAlloc(sizeof(HDHITTESTINFO),m_hwnd,&hProcess);
    if (!lphi)
        return(E_OUTOFMEMORY);

    ptTest.x = x;
    ptTest.y = y;
    ScreenToClient(m_hwnd, &ptTest);

    SharedWrite (&ptTest,&lphi->pt,sizeof(POINT),hProcess);

    if (SendMessage(m_hwnd, HDM_HITTEST, 0, (LPARAM)lphi) != -1)
    {
        SharedRead (&lphi->iItem,&pvarEnd->lVal,sizeof(int),hProcess);
        pvarEnd->lVal++;
    }

    SharedFree(lphi,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHeader32::SetupChildren()
//
// --------------------------------------------------------------------------
void CHeader32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, HDM_GETITEMCOUNT, 0, 0L);
}



