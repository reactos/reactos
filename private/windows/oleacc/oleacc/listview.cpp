// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  LISTVIEW.CPP
//
//  Wrapper for COMCTL32's listview control
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "listview.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOTOOLBAR
#define NOHOTKEY
#define NOPROGRESS
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE
#include <commctrl.h>


#define MAX_NAME_TEXT   256

BOOL Rect1IsOutsideRect2( RECT const & rc1, RECT const & rc2 )
{
    return ( rc1.right  <= rc2.left ) ||
		   ( rc1.bottom <= rc2.top )  ||
		   ( rc1.left  >= rc2.right ) ||
           ( rc1.top >= rc2.bottom );
}

// in outline.cpp...
typedef int (*PFNGetImageIndex)( HWND hwnd, int id );
int LVGetImageIndex( HWND hwnd, int id );

extern "C" {
BOOL GetRoleFromStateImageMap( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, DWORD * pdwRole );
BOOL GetStateFromStateImageMap( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, DWORD * pdwState );
}



// --------------------------------------------------------------------------
//
//  CreateListViewClient()
//
// --------------------------------------------------------------------------
HRESULT CreateListViewClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvList)
{
    CListView32 * plist;
    HRESULT     hr;

    InitPv(ppvList);

    plist = new CListView32(hwnd, idChildCur);
    if (!plist)
        return(E_OUTOFMEMORY);

    hr = plist->QueryInterface(riid, ppvList);
    if (!SUCCEEDED(hr))
        delete plist;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CListView32::CListView32()
//
// --------------------------------------------------------------------------
CListView32::CListView32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  CListView32::SetupChildren()
//
// --------------------------------------------------------------------------
void CListView32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, LVM_GETITEMCOUNT, 0, 0L);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accName(VARIANT varChild, BSTR* pszName)
{
LVITEM* lplvShared;
LPTSTR  lpszShared;
LPTSTR  lpszLocal;
HANDLE  hProcess;
UINT    uSomeUint;
int     nSomeInt;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
    {
        if (InTheShell(m_hwnd, SHELL_DESKTOP))
            return(HrCreateString(STR_DESKTOP_NAME, pszName));
        else
            return(CClient::get_accName(varChild, pszName));
    }

    //
    // Try getting the item's text the easy way, by asking first. Since the
    // file system limits us to 255 character names, assume items aren't
    // bigger than that.
    //
    lplvShared = (LVITEM*)SharedAlloc((sizeof(LVITEM)+sizeof(TCHAR)*(MAX_NAME_TEXT+1)),
                                        m_hwnd,&hProcess);
    if (!lplvShared)
        return(E_OUTOFMEMORY);

    lpszShared = (LPTSTR)(lplvShared+1);

    //lplvShared->mask = LVIF_TEXT;
    uSomeUint = LVIF_TEXT;
    SharedWrite (&uSomeUint,&lplvShared->mask,sizeof(UINT),hProcess);
    //lplvShared->pszText = lpszShared;
    SharedWrite (&lpszShared,&lplvShared->pszText,sizeof(LPTSTR),hProcess);
    //lplvShared->cchTextMax = MAX_NAME_TEXT;
    nSomeInt = MAX_NAME_TEXT;
    SharedWrite (&nSomeInt,&lplvShared->cchTextMax,sizeof(int),hProcess);
    //lplvShared->iItem = varChild.lVal-1;
    nSomeInt = varChild.lVal -1;
    SharedWrite (&nSomeInt,&lplvShared->iItem,sizeof(int),hProcess);
    //lplvShared->iSubItem = 0;
    nSomeInt = 0;
    SharedWrite (&nSomeInt,&lplvShared->iSubItem,sizeof(int),hProcess);

    if (SendMessage(m_hwnd, LVM_GETITEM, varChild.lVal-1, (LPARAM)lplvShared))
    {
        lpszLocal = (LPTSTR)LocalAlloc (LPTR,sizeof(TCHAR)*(MAX_NAME_TEXT+1));
        if (!lpszLocal)
        {
            SharedFree (lplvShared,hProcess);
            return (E_OUTOFMEMORY);
        }
        
        SharedRead (lpszShared,lpszLocal,sizeof(TCHAR)*(MAX_NAME_TEXT+1),hProcess);
        if (*lpszLocal)
            *pszName = TCharSysAllocString(lpszLocal);
        LocalFree (lpszLocal);
    }

    SharedFree(lplvShared,hProcess);

    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accDescription(VARIANT varChild, BSTR* pszDesc)
{
    HWND    hwndHeader;
    int     cColumns;
    LPINT   lpColumnOrderShared;
    LPINT   lpColumnOrder;
    LPINT   lpDescOrder;
    int     iOrder;
    LVITEM* lplvShared;
    LPTSTR  lpszShared;
    LPTSTR  lpszLocal;
    LPTSTR  lpszTempLocal;
    UINT    cch;
    UINT    uSomeUint;
    HANDLE  hProcess1;
    HANDLE  hProcess2;

    InitPv(pszDesc);
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accDescription(varChild, pszDesc));

    //
    // Is there a header control?
    //
    hwndHeader = ListView_GetHeader(m_hwnd);
    if (!hwndHeader)
        return(E_NOT_APPLICABLE);

    //
    // Is there more than one column?
    //
    cColumns = SendMessageINT(hwndHeader, HDM_GETITEMCOUNT, 0, 0L);
    if (cColumns < 2)
        return(E_NOT_APPLICABLE);

    //
    // Get the order to traverse these columns in.
    //
    lpColumnOrderShared = (LPINT)SharedAlloc(2 * cColumns * sizeof(INT),
                                    m_hwnd,&hProcess1);
    if (!lpColumnOrderShared)
        return(E_OUTOFMEMORY);

    // Now allocate a local array twice as big, so we can do our sorting 
    // in the second half.    
    lpColumnOrder = (LPINT)LocalAlloc (LPTR,2 * cColumns * sizeof(INT));
    if (!lpColumnOrder)
    {
        SharedFree (lpColumnOrderShared,hProcess1);
        return(E_OUTOFMEMORY);
    }

    lpDescOrder = lpColumnOrder + cColumns;

    if (!SendMessage(m_hwnd, LVM_GETCOLUMNORDERARRAY, cColumns, (LPARAM)lpColumnOrderShared))
        goto OrderError;

    SharedRead (lpColumnOrderShared,lpColumnOrder,cColumns*sizeof(INT),hProcess1);

    //
    // lpColumnOrder is currently an array where index == iSubItem, value == order.
    // Change this into an array where index == order, value == iSubItem.
    // That way we can sit in a loop using the value as the iSubItem,
    // knowing we are composing the pieces of the description in the proper
    // order.
    //              

    for (iOrder = 0; iOrder < cColumns; iOrder++)
    {
        lpDescOrder[lpColumnOrder[iOrder]] = iOrder;
    }

    //
    // Yes.  Now allocate a TreeView Structure plus a string.
    //
    lplvShared = (LVITEM*)SharedAlloc(sizeof(LVITEM) + 
                                      (82 * sizeof(TCHAR)),
                                      m_hwnd,&hProcess2);
    if (!lplvShared)
    {
OrderError:
        SharedFree(lpColumnOrderShared,hProcess1);
        LocalFree (lpColumnOrder);
        return(E_OUTOFMEMORY);
    }

    // Now allocate a local string that is (80 + 2) * # of columns
    // 80 for the column data, 2 for ", " separating items.  Terminating
    // NULL doesn't have to be included since there are only (col count -1)
    // separators.
    //
    lpszLocal = (LPTSTR)LocalAlloc (LPTR,cColumns * 82 * sizeof(TCHAR));
    if (!lpszLocal)
    {
        SharedFree(lpColumnOrderShared,hProcess1);
        SharedFree(lplvShared,hProcess2);
        LocalFree (lpColumnOrder);
        return(E_OUTOFMEMORY);
    }

    lpszShared = (LPTSTR)(lplvShared+1);

    //lplvShared->mask = LVIF_TEXT;
    uSomeUint = LVIF_TEXT;
    SharedWrite (&uSomeUint,&lplvShared->mask,sizeof(UINT),hProcess2);
    //lplvShared->iItem = varChild.lVal - 1;
    uSomeUint = varChild.lVal - 1;
    SharedWrite (&uSomeUint,&lplvShared->iItem,sizeof(UINT),hProcess2);
    //lplvShared->cchTextMax = 80;
    uSomeUint = 80;
    SharedWrite (&uSomeUint,&lplvShared->cchTextMax,sizeof(UINT),hProcess2);
    //lplvShared->pszText = lpszShared;
    SharedWrite (&lpszShared,&lplvShared->pszText,sizeof(LPTSTR),hProcess2);

    lpszTempLocal = lpszLocal;
    //
    // Traverse the description order array sequentially.
    //
    for (iOrder = 0; iOrder < cColumns; iOrder++)
    {
        // Skip subitem 0, that is the 'name'
        if (lpDescOrder[iOrder] == 0)
            continue;

        //lplvShared->iSubItem = lpDescOrder[iOrder];
        SharedWrite (&lpDescOrder[iOrder],&lplvShared->iSubItem,sizeof(INT),hProcess2);

        if (SendMessage(m_hwnd, LVM_GETITEM, varChild.lVal-1, (LPARAM)lplvShared))
        {
            SharedRead (lpszShared,lpszTempLocal,80*sizeof(TCHAR),hProcess2);
            if (*lpszTempLocal)
            {
                cch = lstrlen(lpszTempLocal);
                lpszTempLocal += cch;
                *lpszTempLocal++ = ',';
                *lpszTempLocal++ = ' ';
            }
        }
    }

    // Strip final ', '
    if (lpszTempLocal != lpszLocal)
    {
        *(lpszTempLocal-2) = 0;
        *pszDesc = TCharSysAllocString(lpszLocal);
    }

    SharedFree(lpColumnOrderShared,hProcess1);
    SharedFree(lplvShared,hProcess2);
    LocalFree (lpszLocal);

    return(*pszDesc ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
    {
        DWORD dwRole;
        if( GetRoleFromStateImageMap( LVGetImageIndex, m_hwnd, varChild.lVal, & dwRole ) )
        {
            pvarRole->lVal = dwRole;
        }
        else
        {
            //
            //  Note that just because the listview has LVS_EX_CHECKBOXES
            //  doesn't mean that every item is itself a checkbox.  We
            //  need to sniff at the item, too, to see if it has a state
            //  image.
            //
            DWORD dwExStyle = SendMessageINT(m_hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
            if ((dwExStyle & LVS_EX_CHECKBOXES) &&
                ListView_GetItemState(m_hwnd, varChild.lVal-1, LVIS_STATEIMAGEMASK))
            {
                pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
            }
            else
            {
                pvarRole->lVal = ROLE_SYSTEM_LISTITEM;
            }
        }
    }
    else
        pvarRole->lVal = ROLE_SYSTEM_LIST;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
long    lState;
DWORD   dwStyle;
DWORD   dwExStyle;
RECT    *lprc;
HANDLE  hProcess;
RECT    rcLocal;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    lState = SendMessageINT(m_hwnd, LVM_GETITEMSTATE, varChild.lVal-1, 0xFFFFFFFF);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (MyGetFocus() == m_hwnd)
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        if (lState & LVIS_FOCUSED)
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    }

    pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

    dwStyle = GetWindowLong (m_hwnd,GWL_STYLE);
    if (!(dwStyle & LVS_SINGLESEL))
        pvarState->lVal |= STATE_SYSTEM_MULTISELECTABLE;

    if (lState & LVIS_SELECTED)
        pvarState->lVal |= STATE_SYSTEM_SELECTED;

    if (lState & LVIS_DROPHILITED)
        pvarState->lVal |= STATE_SYSTEM_HOTTRACKED;

    // If this is a checkbox listview, then look at the checkbox state.
    // State 0 = no checkbox, State 1 = unchecked, State 2 = checked
    dwExStyle = SendMessageINT(m_hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    if ((dwExStyle & LVS_EX_CHECKBOXES) &&
        (lState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
        pvarState->lVal |= STATE_SYSTEM_CHECKED;

    // Get the listview item rect.
	// If the item rect is completely outside the client rect,
	// then the state should be invisible.
	lprc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprc)
        return(E_OUTOFMEMORY);

    rcLocal.left = LVIR_BOUNDS;
    SharedWrite (&rcLocal,lprc,sizeof(RECT),hProcess);

    if (SendMessage(m_hwnd, LVM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprc))
    {
        SharedRead (lprc,&rcLocal,sizeof(RECT),hProcess);
        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

        RECT rcClient;
        GetClientRect (m_hwnd,&rcClient);
        MapWindowPoints (m_hwnd, HWND_DESKTOP, (POINT*)&rcClient, 2);
        if( Rect1IsOutsideRect2( rcLocal, rcClient ) )
			pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
    }

    DWORD dwState;
    if( GetStateFromStateImageMap( LVGetImageIndex, m_hwnd, varChild.lVal, & dwState ) )
    {
        pvarState->lVal |= dwState;
    }


    SharedFree(lprc,hProcess);
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accFocus(VARIANT* pvarFocus)
{
    long    lFocus;
    HRESULT hr;

    //
    // Do we have the focus?
    //
    hr = CClient::get_accFocus(pvarFocus);
    if (!SUCCEEDED(hr) || (pvarFocus->vt != VT_I4) || (pvarFocus->lVal != 0))
        return(hr);

    //
    // We do.  What item is focused?
    //
    lFocus = SendMessageINT(m_hwnd, LVM_GETNEXTITEM, 0xFFFFFFFF, LVNI_FOCUSED);

    if (lFocus != -1)
        pvarFocus->lVal = lFocus+1;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CListView32::get_accDefaultAction()
//
//  Since the default action for a listview item is really determined by the
//  creator of the listview control, the best we can do is double click on
//  the thing, and return "double click" as the default action string.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    InitPv(pszDefAction);

    //
    // Validate.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
        return (HrCreateString(STR_DOUBLE_CLICK, pszDefAction));

    return(E_NOT_APPLICABLE);
}

// --------------------------------------------------------------------------
//
//  CListView32::accDoDefaultAction()
//
//  As noted above, we really don't know what the default action for a list
//  view item is, so unless the parent overrides us, we'll just do a double
//  click on the thing.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accDoDefaultAction(VARIANT varChild)
{
	LPRECT		lprcLoc;
    RECT        rcLocal;
    HANDLE      hProcess;
	
    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
    {
        // Can't just use accLocation, since that gives back the rectangle
        // for the whole line in details view, but you can only click on 
        // a certain part - icon and text. So we'll just ask the control
        // for that rectangle.
        lprcLoc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
        if (!lprcLoc)
            return(E_OUTOFMEMORY);

        //lprcLoc->left = LVIR_ICON;
        rcLocal.left = LVIR_ICON;
        SharedWrite (&rcLocal,lprcLoc,sizeof(RECT),hProcess);

        if (SendMessage(m_hwnd, LVM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprcLoc))
        {
            SharedRead (lprcLoc,&rcLocal,sizeof(RECT),hProcess);
            MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);
            // convert to width and height
            rcLocal.right = rcLocal.right - rcLocal.left;
            rcLocal.bottom = rcLocal.bottom - rcLocal.top;
            // this will check if WindowFromPoint at the click point is the same
	        // as m_hwnd, and if not, it won't click. Cool!
	        if (ClickOnTheRect(&rcLocal,m_hwnd,TRUE))
            {
                SharedFree(lprcLoc,hProcess);
		        return (S_OK);
            }
        }
        SharedFree(lprcLoc,hProcess);
    }
    return(E_NOT_APPLICABLE);
}


// --------------------------------------------------------------------------
//
//  CListView32::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accSelection(VARIANT* pvarSelection)
{
    return(GetListViewSelection(m_hwnd, pvarSelection));
}



// --------------------------------------------------------------------------
//
//  CListView32::accSelect()
//
// Selection Flags can be OR'ed together, with certain limitations. So we 
// need to check each flag and do appropriate action.
//
//  Selection flags:
//  SELFLAG_TAKEFOCUS               
//  SELFLAG_TAKESELECTION           
//  SELFLAG_EXTENDSELECTION         
//  SELFLAG_ADDSELECTION            
//  SELFLAG_REMOVESELECTION         
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accSelect(long selFlags, VARIANT varChild)
{
LPLVITEM lpvi;
long     lState;
long     lStateMask;
long     lFocusedItem;
HANDLE   hProcess;

    if (!ValidateChild(&varChild) || !ValidateSelFlags(selFlags))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accSelect(selFlags, varChild));

    // get the thing with focus (anchor point)
    // if no focus, use first one
    // have to get it here because we might clear it b4 we need it.
    lFocusedItem = ListView_GetNextItem(m_hwnd, -1,LVNI_FOCUSED);
    if (lFocusedItem == -1)
        lFocusedItem = 0;
        
    varChild.lVal--;

    // First check if there can be more than one item selected.
	if ((selFlags & SELFLAG_ADDSELECTION) || 
        (selFlags & SELFLAG_REMOVESELECTION) ||
        (selFlags & SELFLAG_EXTENDSELECTION))
	{
		if (SendMessage(m_hwnd, LVM_GETITEMSTATE, varChild.lVal, 0xFFFFFFFF) & LVS_SINGLESEL)
			return (E_NOT_APPLICABLE);
	}

    lpvi = (LPLVITEM)SharedAlloc(sizeof(LVITEM),m_hwnd,&hProcess);
    if (!lpvi)
        return(E_OUTOFMEMORY);

    // If the take focus flag is set, check if it can get focus &
    // remove focus from other items
	if (selFlags & SELFLAG_TAKEFOCUS)
	{
        if (MyGetFocus() != m_hwnd)
        {
            SharedFree (lpvi,hProcess);
            return(S_FALSE);
        }
        RemoveCurrentSelFocus(SELFLAG_TAKEFOCUS);
	}

    // If the take selection flag is set, remove selection from other items
    if (selFlags & SELFLAG_TAKESELECTION)
        RemoveCurrentSelFocus(SELFLAG_TAKESELECTION);

	lState = 0;
    lStateMask = 0;

    // now is where the real work starts. If they are just taking
    // selection, adding a selection, or removing a selection, it is
    // pretty easy. But if they are extending the selection, we'll have
    // to loop through from where the focus is to this one and select or
    // deselect each one.
    if ((selFlags & SELFLAG_EXTENDSELECTION) == 0) // not extending (easy)
    {
        if (selFlags & SELFLAG_ADDSELECTION ||
            selFlags & SELFLAG_TAKESELECTION)
        {
            lState |= LVIS_SELECTED;
            lStateMask |= LVIS_SELECTED;
        }

        if (selFlags & SELFLAG_REMOVESELECTION)
            lStateMask |= LVIS_SELECTED;

        if (selFlags & SELFLAG_TAKEFOCUS)
        {
	        lState |= LVIS_FOCUSED;
            lStateMask |= LVIS_FOCUSED;
        }

        //lpvi->state = lState;
        SharedWrite (&lState,&lpvi->state,sizeof(LONG),hProcess);
        //lpvi->stateMask  = lStateMask;
        SharedWrite (&lStateMask,&lpvi->stateMask,sizeof(LONG),hProcess);

        SendMessage(m_hwnd, LVM_SETITEMSTATE, varChild.lVal, (LPARAM)lpvi);
    }
    else // we are extending the selection (hard work)
    {
    long        i;
    long        nIncrement;

        // we are always selecting or deselecting, so statemask
        // always has LVIS_SELECTED.
        lStateMask = LVIS_SELECTED;

        // if neither ADDSELECTION or REMOVESELECTION is set, then we are
        // supposed to do something based on the selection state of whatever
        // has the focus.
        if (selFlags & SELFLAG_ADDSELECTION)
            lState |= LVIS_SELECTED;
        
        if (((selFlags & SELFLAG_REMOVESELECTION) == 0) &&
            ((selFlags & SELFLAG_ADDSELECTION) == 0))
        {
            // if focused item is selected, lState to have selected also
    		if (SendMessage(m_hwnd, LVM_GETITEMSTATE, lFocusedItem, 0xFFFFFFFF) 
                & LVIS_SELECTED)
                lState |= LVIS_SELECTED;
        }

        //lpvi->state = lState;
        SharedWrite (&lState,&lpvi->state,sizeof(LONG),hProcess);
        //lpvi->stateMask  = lStateMask;
        SharedWrite (&lStateMask,&lpvi->stateMask,sizeof(LONG),hProcess);

        // Now walk through from focused to current, setting the state.
        // Set increment and last one depending on direction
        if (lFocusedItem > varChild.lVal)
        {
            nIncrement = -1;
            varChild.lVal--;
        }
        else
        {
            nIncrement = 1;
            varChild.lVal++;
        }

        for (i=lFocusedItem; i!=varChild.lVal; i+=nIncrement)
            SendMessage(m_hwnd, LVM_SETITEMSTATE, i, (LPARAM)lpvi);

        // focus the last one if needed
        if (selFlags & SELFLAG_TAKEFOCUS)
        {
            lStateMask |= LVIS_FOCUSED;
            lState |= LVIS_FOCUSED;
            //lpvi->state = lState;
            SharedWrite (&lState,&lpvi->state,sizeof(LONG),hProcess);
            //lpvi->stateMask  = lStateMask;
            SharedWrite (&lStateMask,&lpvi->stateMask,sizeof(LONG),hProcess);
            SendMessage(m_hwnd, LVM_SETITEMSTATE, i-nIncrement, (LPARAM)lpvi);
        }

    }
    
    SharedFree(lpvi,hProcess);
	return (S_OK);
}

// --------------------------------------------------------------------------
//
//  CListView32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    LPRECT  lprc;
    RECT    rcLocal;
    HANDLE  hProcess;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    // Get the listview item rect.
    lprc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprc)
        return(E_OUTOFMEMORY);

    rcLocal.left = LVIR_BOUNDS;
    SharedWrite (&rcLocal,lprc,sizeof(RECT),hProcess);

    if (SendMessage(m_hwnd, LVM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprc))
    {
        SharedRead (lprc,&rcLocal,sizeof(RECT),hProcess);
        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

        *pxLeft = rcLocal.left;
        *pyTop = rcLocal.top;
        *pcxWidth = rcLocal.right - rcLocal.left;
        *pcyHeight = rcLocal.bottom - rcLocal.top;
    }

    SharedFree(lprc,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListView32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    long    lEnd = 0;
    int     lvFlags;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir == NAVDIR_FIRSTCHILD)
        dwNavDir = NAVDIR_NEXT;
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        varStart.lVal = m_cChildren + 1;
        dwNavDir = NAVDIR_PREVIOUS;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    //
    // Gotta love those listview dudes!  They have all the messages we need
    // to do hittesting, location, and navigation easily.  And those are
    // by far the hardest things to manually implement.  
    //
    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
            lEnd = varStart.lVal + 1;
            if (lEnd > m_cChildren)
                lEnd = 0;
            break;

        case NAVDIR_PREVIOUS:
            lEnd = varStart.lVal - 1;
            break;

        case NAVDIR_LEFT:
            lvFlags = LVNI_TOLEFT;
            goto Navigate;

        case NAVDIR_RIGHT:
            lvFlags = LVNI_TORIGHT;
            goto Navigate;

        case NAVDIR_UP:
            lvFlags = LVNI_ABOVE;
            goto Navigate;

        case NAVDIR_DOWN:
            lvFlags = LVNI_BELOW;
Navigate:
            // Note that if nothing is there, COMCTL32 will return -1, and -1+1 is
            // zero, meaning nothing in our land also.
            lEnd = SendMessageINT(m_hwnd, LVM_GETNEXTITEM, varStart.lVal-1, lvFlags);
            ++lEnd;
            break;
    }

    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
        
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CListView32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    HRESULT     hr;
    HANDLE      hProcess;
    int         nSomeInt;
    POINT       ptLocal;
    LPLVHITTESTINFO lpht;

    SetupChildren();
    
    //
    // Is the point in the listview at all?
    //
    hr = CClient::accHitTest(x, y, pvarHit);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarHit->vt != VT_I4) || (pvarHit->lVal != 0))
        return(hr);

    //
    // Now find out what item this point is on.
    //
    lpht = (LPLVHITTESTINFO)SharedAlloc(sizeof(LVHITTESTINFO),m_hwnd,&hProcess);
    if (!lpht)
        return(E_OUTOFMEMORY);

    //lpht->iItem = -1;
    nSomeInt = -1;
    SharedWrite (&nSomeInt,&lpht->iItem,sizeof(int),hProcess);
    ptLocal.x = x;
    ptLocal.y = y;
    ScreenToClient(m_hwnd, &ptLocal);
    SharedWrite (&ptLocal,&lpht->pt,sizeof(POINT),hProcess);

    //
    // LVM_SUBHITTEST will return -1 if the point isn't over an item.  And -1
    // + 1 is zero, which is self.  So that works great for us.
    //
    SendMessage(m_hwnd, LVM_SUBITEMHITTEST, 0, (LPARAM)lpht);
    SharedRead (&lpht->iItem,&pvarHit->lVal,sizeof(int),hProcess);
    pvarHit->lVal++;

    SharedFree(lpht,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  RemoveCurrentSelFocus()
//
//  This removes all selected/focused items.
//
// -------------------------------------------------------------------------
void CListView32::RemoveCurrentSelFocus(long lState)
{
    LPLVITEM    lpvi;
    long        lNext;
    HANDLE      hProcess;
    int         nSomeInt;

    lpvi = (LPLVITEM)SharedAlloc(sizeof(LVITEM),m_hwnd,&hProcess);
    if (!lpvi)
        return;

    //lpvi->stateMask = lState;
    SharedWrite (&lState,&lpvi->stateMask,sizeof(LONG),hProcess);
    //lpvi->state = 0;
    nSomeInt = 0;
    SharedWrite (&nSomeInt,&lpvi->state,sizeof(int),hProcess);

    //
    // Loop through all focused/selected items.
    //
    lNext = ListView_GetNextItem(m_hwnd, -1,
        ((lState == LVIS_FOCUSED) ? LVNI_FOCUSED : LVNI_SELECTED));
    while (lNext != -1)
    {
        SendMessage(m_hwnd, LVM_SETITEMSTATE, lNext, (LPARAM)lpvi);
        lNext = ListView_GetNextItem(m_hwnd, lNext,
            ((lState == LVIS_FOCUSED) ? LVNI_FOCUSED : LVNI_SELECTED));
    }

    SharedFree(lpvi,hProcess);
}


/////////////////////////////////////////////////////////////////////////////
//
//  MULTIPLE SELECTION LISTVIEW SUPPORT
//
//  If a listview has more than one item selected, we create an object that
//  is a clone.  It supports merely IUnknown and IEnumVARIANT, and is a 
//  collection.  The caller should take the returned item IDs and pass them
//  in a VARIANT (VT_I4, ID as lVal) to the parent object.
//
/////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------------
//
//  GetListViewSelection()
//
// --------------------------------------------------------------------------
HRESULT GetListViewSelection(HWND hwnd, VARIANT* pvarSelection)
{
    int     cSelected;
    LPINT   lpSelected;
    long    lRet;
    int     iSelected;
    CListViewSelection * plvs;

    InitPvar(pvarSelection);

    cSelected = SendMessageINT(hwnd, LVM_GETSELECTEDCOUNT, 0, 0L);

    //
    // No selection.
    //
    if (!cSelected)
        return(S_FALSE);

    //
    // Single item.
    //
    if (cSelected == 1)
    {
        pvarSelection->vt = VT_I4;
        pvarSelection->lVal = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED) + 1;
        return(S_OK);
    }

    //
    // Multiple items, must make a collection object.
    //

    // Allocate the list.
    lpSelected = (LPINT)LocalAlloc(LPTR, cSelected*sizeof(INT));
    if (!lpSelected)
        return(E_OUTOFMEMORY);

    plvs = NULL;

    // Get the list of selected items.
    lRet = -1;
    for (iSelected = 0; iSelected < cSelected; iSelected++)
    {
        lRet = ListView_GetNextItem(hwnd, lRet, LVNI_SELECTED);
        if (lRet == -1)
            break;

        lpSelected[iSelected] = lRet;
    }

    //
    // Did something go wrong in the middle?
    //
    cSelected = iSelected;
    if (cSelected)
    {
        plvs = new CListViewSelection(0, cSelected, lpSelected);
        if (plvs)
        {
            pvarSelection->vt = VT_UNKNOWN;
            plvs->QueryInterface(IID_IUnknown, (void**)&(pvarSelection->punkVal));
        }
    }

    //
    // Free the list memory no matter what, the constructor will make a copy.
    //
    if (lpSelected)
        LocalFree((HANDLE)lpSelected);

    if (!plvs)
        return(E_OUTOFMEMORY);
    else
        return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::CListViewSelection()
//
// --------------------------------------------------------------------------
CListViewSelection::CListViewSelection(int iChildCur, int cTotal, LPINT lpItems)
{
    m_idChildCur = iChildCur;

    m_lpSelected = (LPINT)LocalAlloc(LPTR, cTotal*sizeof(int));
    if (!m_lpSelected)
        m_cSelected = 0;
    else
    {
        m_cSelected = cTotal;
        CopyMemory(m_lpSelected, lpItems, cTotal*sizeof(int));
    }
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::~CListViewSelection()
//
// --------------------------------------------------------------------------
CListViewSelection::~CListViewSelection()
{
    //
    // Free selection list
    //
    if (m_lpSelected)
    {
        LocalFree((HANDLE)m_lpSelected);
        m_cSelected = 0;
        m_lpSelected = NULL;
    }
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::QueryInterface()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::QueryInterface(REFIID riid, void** ppunk)
{
    InitPv(ppunk);

    if ((riid == IID_IUnknown)  ||
        (riid == IID_IEnumVARIANT))
    {
        *ppunk = this;
    }
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN) *ppunk)->AddRef();
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::AddRef()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListViewSelection::AddRef(void)
{
    return(++m_cRef);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Release()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListViewSelection::Release(void)
{
    if ((--m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return(m_cRef);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Next()
//
//  This returns a VT_I4 which is the child ID for the parent ListView that
//  returned this object for the selection collection.  The caller turns
//  around and passes this variant to the ListView object to get acc info
//  about it.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Next(ULONG celt, VARIANT* rgvar, ULONG *pceltFetched)
{
    VARIANT* pvar;
    long    cFetched;
    long    iCur;

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    //
    // Initialize VARIANTs
    // This is so bogus
    //
    pvar = rgvar;
    for (iCur = 0; iCur < (long)celt; iCur++, pvar++)
        VariantInit(pvar);

    pvar = rgvar;
    cFetched = 0;
    iCur = m_idChildCur;

    //
    // Loop through our items
    //
    while ((cFetched < (long)celt) && (iCur < m_cSelected))
    {
        pvar->vt = VT_I4;
        pvar->lVal = m_lpSelected[iCur] + 1;

        ++cFetched;
        ++iCur;
        ++pvar;
    }

    //
    // Advance the current position
    //
    m_idChildCur = iCur;

    //
    // Fill in the number fetched
    //
    if (pceltFetched)
        *pceltFetched = cFetched;

    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Skip()
//
// -------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Skip(ULONG celt)
{
    m_idChildCur += celt;
    if (m_idChildCur > m_cSelected)
        m_idChildCur = m_cSelected;

    //
    // We return S_FALSE if at the end.
    //
    return((m_idChildCur >= m_cSelected) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Clone(IEnumVARIANT **ppenum)
{
    CListViewSelection * plistselnew;

    InitPv(ppenum);

    plistselnew = new CListViewSelection(m_idChildCur, m_cSelected, m_lpSelected);
    if (!plistselnew)
        return(E_OUTOFMEMORY);

    return(plistselnew->QueryInterface(IID_IEnumVARIANT, (void**)ppenum));
}






int LVGetImageIndex( HWND hwnd, int id )
{
    LVITEM* lplvShared;
    
    HANDLE  hProcess;
    UINT    uSomeUint;

    // Try getting the item's text the easy way, by asking first. Since the
    // file system limits us to 255 character names, assume items aren't
    // bigger than that.
    //
    lplvShared = (LPLVITEM)SharedAlloc(sizeof(LVITEM),hwnd,&hProcess);
    if (!lplvShared)
        return -1;


    //lptvShared->mask = LVIF_IMAGE;
    uSomeUint = LVIF_IMAGE;
    SharedWrite (&uSomeUint,&lplvShared->mask,sizeof(UINT),hProcess);
    //lplvShared->iItem = id;
    SharedWrite (&id,&lplvShared->iItem,sizeof(lplvShared->iItem),hProcess);

    if (ListView_GetItem(hwnd, lplvShared))
    {
        SharedRead (&lplvShared->iImage,&uSomeUint,sizeof(UINT),hProcess);
    }

    SharedFree(lplvShared,hProcess);

    return uSomeUint;
}
