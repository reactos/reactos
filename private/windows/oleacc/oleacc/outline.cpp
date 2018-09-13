// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  OUTLINE.CPP
//
//  Wrapper for COMCTL32's treeview control
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOTOOLBAR
#define NOHOTKEY
#define NOHEADER
#define NOPROGRESS
//#define NOLISTVIEW            // INDEXTOSTATEIMAGEMASK needs LISTVIEW
#define NOTABCONTROL
#define NOANIMATE
#include <commctrl.h>

#include "outline.h"



struct MSAASTATEIMAGEMAPENT
{
    DWORD   dwRole;
    DWORD   dwState;
};


typedef int (*PFNGetImageIndex)( HWND hwnd, int id );
int TVGetImageIndex( HWND hwnd, int id );

int GetStateImageMapEnt( PFNGetImageIndex, HWND hwnd, int id, MSAASTATEIMAGEMAPENT *pEnt );
extern "C" {
BOOL GetRoleFromStateImageMap( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, DWORD * pdwRole );
BOOL GetStateFromStateImageMap( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, DWORD * pdwState );
}





#ifdef _WIN64

    // 
    // Treeview support is currently not implemented on Win64.
    // (childIDs are 32bit, but win64's HTREEITEMs are 64bit.)
    // The following 'stubs' return FALSE/NULL/E_NOTIMPL.
    //

    HRESULT CreateTreeViewClient(HWND hwnd, long idChildCur, REFIID riid,
        void** ppvTreeView)
    { return E_NOTIMPL; }


    COutlineView32::COutlineView32(HWND hwnd, long idChildCur)
    { /* do nothing */ }

    void COutlineView32::SetupChildren(void)
    { /* do nothing */ }

    BOOL COutlineView32::ValidateChild(VARIANT* pvar)
    { return FALSE; }

    HTREEITEM COutlineView32::NextLogicalItem(HTREEITEM ht, BOOL fNext)
    { return NULL; }


    STDMETHODIMP COutlineView32::get_accName(VARIANT varChild, BSTR* pszName)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::get_accValue(VARIANT varChild, BSTR* pszValue)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::get_accState(VARIANT varChild, VARIANT* pvarState)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::get_accFocus(VARIANT* pvarFocus)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::get_accSelection(VARIANT* pvarSelection)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefA)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::accSelect(long selFlags, VARIANT varChild)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
        long* pcyHeight, VARIANT varChild)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::accNavigate(long dwNavDir, VARIANT varStart,
        VARIANT* pvarEnd)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::accHitTest(long x, long y, VARIANT* pvarHit)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::accDoDefaultAction(VARIANT varChild)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::Reset(void)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::Next(ULONG celt, VARIANT* rgvarFetch, ULONG* pceltFetch)
    { return E_NOTIMPL; }

    STDMETHODIMP COutlineView32::Skip(ULONG celtSkip)
    { return E_NOTIMPL; }

#else // _WIN64
    //
    // Normal treeview code resumes...
    //




#define MAX_NAME_SIZE   255

// these are in a newer version of comctl.h
#ifndef TVM_GETITEMSTATE

#define TVM_GETITEMSTATE        (TV_FIRST + 39)
#define TreeView_GetItemState(hwndTV, hti, mask) \
   (UINT)SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)hti, (LPARAM)mask)

#define TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)hti, TVIS_STATEIMAGEMASK))) >> 12) -1)

#endif // ifndef TVM_GETITEMSTATE


// --------------------------------------------------------------------------
//
//  CreateTreeViewClient()
//
// --------------------------------------------------------------------------
HRESULT CreateTreeViewClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvTreeView)
{
    COutlineView32 * poutline;
    HRESULT         hr;

    InitPv(ppvTreeView);

    poutline = new COutlineView32(hwnd, idChildCur);
    if (!poutline)
        return(E_OUTOFMEMORY);

    hr = poutline->QueryInterface(riid, ppvTreeView);
    if (!SUCCEEDED(hr))
        delete poutline;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::COutlineView32()
//
// --------------------------------------------------------------------------
COutlineView32::COutlineView32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::SetupChildren()
//
// --------------------------------------------------------------------------
void COutlineView32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, TVM_GETCOUNT, 0, 0);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::ValidateChild()
//
//  We have no index-ID support in tree view.  Hence, the HTREEITEM is the
//  child ID, only thing we can do.  We don't bother validating it except
//  to make sure it is less than 0x80000000.
//
// --------------------------------------------------------------------------
BOOL COutlineView32::ValidateChild(VARIANT* pvar)
{
TryAgain:
    switch (pvar->vt)
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy(pvar, pvar->pvarVal);
            goto TryAgain;

        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

        case VT_I4:
            if (pvar->lVal < 0)
                return(FALSE);

            //
            // Assume it's a valid HTREEITEM!
            //
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::NextLogicalItem()
//
// --------------------------------------------------------------------------
HTREEITEM COutlineView32::NextLogicalItem(HTREEITEM ht, BOOL fNext)
{
    HTREEITEM htNext;

    if (fNext)
    {
        //
        // We see if this item has a child.  If so, we are done.  If not,
        // we get the next sibling.  If that fails, we move back to the parent,
        // and try the next sibling thing again.  And so on until we reach the
        // root.
        //
        htNext = TreeView_GetChild(m_hwnd, ht);
        if (htNext)
            return(htNext);

        while (ht)
        {
            htNext = TreeView_GetNextSibling(m_hwnd, ht);
            if (htNext)
                return(htNext);

            ht = TreeView_GetParent(m_hwnd, ht);
        }
    }

    return(NULL);
}


// --------------------------------------------------------------------------
//
//  COutlineView32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accName(VARIANT varChild, BSTR* pszName)
{
    TVITEM* lptvShared;
    LPTSTR  lpszShared;
    HANDLE  hProcess;
    LPTSTR  lpszLocal;
    UINT    uSomeUint;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accName(varChild, pszName));

    //
    // Try getting the item's text the easy way, by asking first. Since the
    // file system limits us to 255 character names, assume items aren't
    // bigger than that.
    //
    lptvShared = (LPTVITEM)SharedAlloc(sizeof(TVITEM)+((MAX_NAME_SIZE+2)*sizeof(TCHAR)),
                                 m_hwnd,&hProcess);
    if (!lptvShared)
        return(E_OUTOFMEMORY);

    lpszLocal = (LPTSTR)LocalAlloc(LPTR,((MAX_NAME_SIZE+2)*sizeof(TCHAR)));
    if (!lpszLocal)
    {
        SharedFree (lptvShared,hProcess);
        return(E_OUTOFMEMORY);
    }

    lpszShared = (LPTSTR)(lptvShared+1);

    //lptvShared->mask = TVIF_TEXT;
    uSomeUint = TVIF_TEXT;
    SharedWrite (&uSomeUint,&lptvShared->mask,sizeof(UINT),hProcess);
    //lptvShared->hItem = (HTREEITEM)varChild.lVal;
    SharedWrite (&varChild.lVal,&lptvShared->hItem,sizeof(HTREEITEM),hProcess);
    //lptvShared->pszText = lpszShared;
    SharedWrite (&lpszShared,&lptvShared->pszText,sizeof(LPTSTR),hProcess);
    //lptvShared->cchTextMax = MAX_NAME_SIZE+1;
    uSomeUint = MAX_NAME_SIZE+1;
    SharedWrite (&uSomeUint,&lptvShared->cchTextMax,sizeof(UINT),hProcess);

    if (TreeView_GetItem(m_hwnd, lptvShared))
    {
        SharedRead (lpszShared,lpszLocal,((MAX_NAME_SIZE+2)*sizeof(TCHAR)),hProcess);
        if (*lpszLocal)
            *pszName = TCharSysAllocString(lpszLocal);
    }

    SharedFree(lptvShared,hProcess);
    LocalFree (lpszLocal);

    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accValue()
//
//  This returns back the indent level for a child item.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    HTREEITEM   htParent;
    long    lValue;

    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(E_NOT_APPLICABLE);

    lValue = 0;
    htParent = (HTREEITEM)varChild.lVal;

    while (htParent = TreeView_GetParent(m_hwnd, htParent))
        lValue++;

    return(VarBstrFromI4(lValue, 0, 0, pszValue));
}

// --------------------------------------------------------------------------
//
//  COutlineView32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
    {
        DWORD dwRole;
        if( GetRoleFromStateImageMap( TVGetImageIndex, m_hwnd, varChild.lVal, & dwRole ) )
        {
            pvarRole->lVal = dwRole;
        }
        else
        {

            //
            //  Note that just because the treeview has TVS_CHECKBOXES
            //  doesn't mean that every item is itself a checkbox.  We
            //  need to sniff at the item, too, to see if it has a state
            //  image.
            //
            if ((GetWindowLong (m_hwnd,GWL_STYLE) & TVS_CHECKBOXES) &&
                TreeView_GetItemState(m_hwnd, varChild.lVal, TVIS_STATEIMAGEMASK))
            {
                pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
            }
            else
            {
                pvarRole->lVal = ROLE_SYSTEM_OUTLINEITEM;
            }
        }
    }
    else
        pvarRole->lVal = ROLE_SYSTEM_OUTLINE;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    LPTVITEM    lptvShared;
    HANDLE      hProcess;
    TVITEM      tvLocal;
    UINT        uSomeUint;
    DWORD       dwStyle;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (MyGetFocus() == m_hwnd)
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;


    lptvShared = (LPTVITEM)SharedAlloc(sizeof(TVITEM),m_hwnd,&hProcess);
    if (!lptvShared)
        return(E_OUTOFMEMORY);

    //lptvShared->mask = TVIF_STATE | TVIF_CHILDREN;
    uSomeUint = TVIF_STATE | TVIF_CHILDREN;
    SharedWrite (&uSomeUint,&lptvShared->mask,sizeof(UINT),hProcess);
    //lptvShared->hItem = (HTREEITEM)varChild.lVal;
    SharedWrite(&varChild.lVal,&lptvShared->hItem,sizeof(HTREEITEM),hProcess);

    if (TreeView_GetItem(m_hwnd, lptvShared))
    {
        SharedRead (lptvShared,&tvLocal,sizeof(TVITEM),hProcess);
        if (tvLocal.state & TVIS_SELECTED)
        {
            pvarState->lVal |= STATE_SYSTEM_SELECTED;
            if (pvarState->lVal & STATE_SYSTEM_FOCUSABLE)
                pvarState->lVal |= STATE_SYSTEM_FOCUSED;
        }

        pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

        if (tvLocal.state & TVIS_DROPHILITED)
            pvarState->lVal |= STATE_SYSTEM_HOTTRACKED;

        //
        // If it isn't expanded and it has children, then it must be
        // collapsed.
        //
        if (tvLocal.state & (TVIS_EXPANDED | TVIS_EXPANDPARTIAL))
            pvarState->lVal |= STATE_SYSTEM_EXPANDED;
        else if (tvLocal.cChildren)
            pvarState->lVal |= STATE_SYSTEM_COLLAPSED;

        // If the treeview has checkboxes, then see if it's checked.
        // State 0 = no checkbox, State 1 = unchecked, State 2 = checked
        dwStyle = GetWindowLong (m_hwnd,GWL_STYLE);
        if ((dwStyle & TVS_CHECKBOXES) &&
            (tvLocal.state & TVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
            pvarState->lVal |= STATE_SYSTEM_CHECKED;

    
        DWORD dwState;
        if( GetStateFromStateImageMap( TVGetImageIndex, m_hwnd, varChild.lVal, & dwState ) )
        {
            pvarState->lVal |= dwState;
        }
   
    }

    SharedFree(lptvShared,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accFocus(VARIANT* pvarFocus)
{
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
    return(COutlineView32::get_accSelection(pvarFocus));
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accSelection(VARIANT* pvarSelection)
{
    HTREEITEM   ht;

    InitPvar(pvarSelection);

    ht = TreeView_GetSelection(m_hwnd);

    if (ht)
    {
        pvarSelection->vt = VT_I4;
        pvarSelection->lVal = (long)ht;

        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accDefaultAction()
//
//  The default action of a node with children is:
//      * Expand one level if it is fully collapsed
//      * Collapse if it is partly or completely expanded
//
//  The reason for not expanding fully is that it is slow and there is no
//  keyboard shortcut or mouse click that will do it.  You can use a menu
//  command to do so if you want.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefA)
{
    VARIANT varState;
    HRESULT hr;

    InitPv(pszDefA);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accDefaultAction(varChild, pszDefA));

    //
    // Get our state.  NOTE that we will not get back STATE_SYSTEM_COLLAPSED
    // if the item doesn't have children.
    //
    VariantInit(&varState);
    hr = get_accState(varChild, &varState);
    if (!SUCCEEDED(hr))
        return(hr);

    if (varState.lVal & STATE_SYSTEM_EXPANDED)
        return(HrCreateString(STR_TREE_COLLAPSE, pszDefA));
    else if (varState.lVal & STATE_SYSTEM_COLLAPSED)
        return(HrCreateString(STR_TREE_EXPAND, pszDefA));
    else
        return(E_NOT_APPLICABLE);
}


// --------------------------------------------------------------------------
//
//  COutlineView32::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accSelect(long selFlags, VARIANT varChild)
{
    if (!ValidateChild(&varChild) || !ValidateSelFlags(selFlags))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accSelect(selFlags, varChild));

	if (selFlags & SELFLAG_TAKEFOCUS) 
        if (MyGetFocus() != m_hwnd)
            return(S_FALSE);

	if ((selFlags & SELFLAG_TAKEFOCUS) || (selFlags & SELFLAG_TAKESELECTION))
	{
		TreeView_SelectItem(m_hwnd, (HTREEITEM)varChild.lVal);
		return (S_OK);
	}
	else
		return (E_NOT_APPLICABLE);

}



// --------------------------------------------------------------------------
//
//  COutlineView32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    LPRECT  lprcShared;
    RECT    rcLocal;
    HANDLE  hProcess;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    // Get the listview item rect.
    lprcShared = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprcShared)
        return(E_OUTOFMEMORY);

    // can't use the TreeView_GetItemRect macro, because it does a behind-the-scenes
    // assignment of the item id into the rect, which blows on shared memory.
    SharedWrite (&varChild.lVal,lprcShared,sizeof(HTREEITEM),hProcess);

    if (SendMessage (m_hwnd, TVM_GETITEMRECT, TRUE, (LPARAM)lprcShared))
    {
        SharedRead (lprcShared,&rcLocal,sizeof(RECT),hProcess);

        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

        *pxLeft = rcLocal.left;
        *pyTop = rcLocal.top;
        *pcxWidth = rcLocal.right - rcLocal.left;
        *pcyHeight = rcLocal.bottom - rcLocal.top;
    }

    SharedFree(lprcShared,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    HTREEITEM   htNewItem = 0;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir >= NAVDIR_FIRSTCHILD)
    {
        htNewItem = TreeView_GetRoot(m_hwnd);

        if ((dwNavDir == NAVDIR_LASTCHILD) && htNewItem)
        {
            HTREEITEM   htNext;

RecurseAgain:
            //
            // Keep recursing down all the way to the last ancestor of the
            // last item under the root.
            //
            htNext = TreeView_GetChild(m_hwnd, htNewItem);
            if (htNext)
            {
                while (htNext)
                {
                    htNewItem = htNext;
                    htNext = TreeView_GetNextSibling(m_hwnd, htNewItem);
                }

                goto RecurseAgain;
            }
        }

        goto AllDone;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
            // Next logical item, peer or child
            htNewItem = NextLogicalItem((HTREEITEM)varStart.lVal, TRUE);
            break;

        case NAVDIR_PREVIOUS:
#if 0
            // Previous logical item, peer or parent
            htNewItem = NextLogicalItem((HTREEITEM)varStart.lVal, FALSE);
            break;
#endif

        case NAVDIR_UP:
            // Previous sibling!
            htNewItem = TreeView_GetPrevSibling(m_hwnd, (HTREEITEM)varStart.lVal);
            break;

        case NAVDIR_DOWN:
            // Next sibling!
            htNewItem = TreeView_GetNextSibling(m_hwnd, (HTREEITEM)varStart.lVal);
            break;

        case NAVDIR_LEFT:
            // Get parent!
            htNewItem = TreeView_GetParent(m_hwnd, (HTREEITEM)varStart.lVal);
            break;

        case NAVDIR_RIGHT:
            // Get first child!
            htNewItem = TreeView_GetChild(m_hwnd, (HTREEITEM)varStart.lVal);
            break;
    }

AllDone:
    if (htNewItem)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = (long)htNewItem;
        
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    HRESULT         hr;
    LPTVHITTESTINFO lptvhtShared;
    HANDLE          hProcess;
    POINT           ptLocal;
    HTREEITEM       hItem;

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
    lptvhtShared = (LPTVHITTESTINFO)SharedAlloc(sizeof(TVHITTESTINFO),m_hwnd,&hProcess);
    if (!lptvhtShared)
        return(E_OUTOFMEMORY);

    //lptvhtShared->hItem = NULL;
    hItem = NULL;
    SharedWrite (&hItem,&lptvhtShared->hItem,sizeof(HTREEITEM),hProcess);
    
    ptLocal.x = x;
    ptLocal.y = y;
    ScreenToClient(m_hwnd, &ptLocal);

    SharedWrite (&ptLocal,&lptvhtShared->pt,sizeof(POINT),hProcess);

    //
    // Note:
    // TVM_HITTEST will return -1 if the point isn't over an item.  And -1
    // + 1 is zero, which is self.  So that works great for OLEACC
    //
    SendMessage(m_hwnd, TVM_HITTEST, 0, (LPARAM)lptvhtShared);
    SharedRead (&lptvhtShared->hItem,&pvarHit->lVal,sizeof(HTREEITEM),hProcess);

    SharedFree(lptvhtShared,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::accDoDefaultAction()
//
//  This expands collapsed items and collapses expanded items.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accDoDefaultAction(VARIANT varChild)
{
    VARIANT varState;
    HRESULT hr;
    UINT    tve;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accDoDefaultAction(varChild));

    //
    // Get the item's state.
    //
    VariantInit(&varState);
    hr = get_accState(varChild, &varState);
    if (!SUCCEEDED(hr))
        return(hr);

    if (varState.lVal & STATE_SYSTEM_COLLAPSED)
        tve = TVE_EXPAND;
    else if (varState.lVal & STATE_SYSTEM_EXPANDED)
        tve = TVE_COLLAPSE;
    else
        return(E_NOT_APPLICABLE);

    PostMessage(m_hwnd, TVM_EXPAND, tve, (LPARAM)varChild.lVal);
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::Reset()
//
//  Sets the "current" HTREEITEM to NULL so we know we are at the beginning.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::Next()
//
//  We descend into children, among siblings, and back up as necessary.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::Next(ULONG celt, VARIANT* rgvarFetch, ULONG* pceltFetch)
{
    HTREEITEM   htCur;
    HTREEITEM   htNext;
    VARIANT*    pvar;
    ULONG       cFetched;

    SetupChildren();

    if (pceltFetch)
        InitPv(pceltFetch);

    pvar = rgvarFetch;
    cFetched = 0;

    htCur = (HTREEITEM)m_idChildCur;

    if (!htCur)
        htNext = TreeView_GetRoot(m_hwnd);
    else
        htNext = NextLogicalItem(htCur, TRUE);

    while ((cFetched < celt) && htNext)
    {
        htCur = htNext;

        cFetched++;

        pvar->vt = VT_I4;
        pvar->lVal = (long)htCur;
        pvar++;

        htNext = NextLogicalItem(htCur, TRUE);
    }

    m_idChildCur = (long)htCur;

    if (pceltFetch)
        *pceltFetch = cFetched;

    return((cFetched < celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::Skip(ULONG celtSkip)
{
    HTREEITEM   htCur;
    HTREEITEM   htNext;

    SetupChildren();

    htCur = (HTREEITEM)m_idChildCur;

    if (!htCur)
        htNext = TreeView_GetRoot(m_hwnd);
    else
        htNext = NextLogicalItem(htCur, TRUE);

    while ((celtSkip > 0) && htNext)
    {
        --celtSkip;

        htCur = htNext;
        htNext = NextLogicalItem(htCur, TRUE);
    }

    m_idChildCur = (long)htCur;

    return(htNext ? S_OK : S_FALSE);
}




int TVGetImageIndex( HWND hwnd, int id )
{
    TVITEM* lptvShared;

    
    HANDLE  hProcess;
    UINT    uSomeUint;

    // Try getting the item's text the easy way, by asking first. Since the
    // file system limits us to 255 character names, assume items aren't
    // bigger than that.
    //
    lptvShared = (LPTVITEM)SharedAlloc(sizeof(TVITEM),hwnd,&hProcess);
    if (!lptvShared)
        return -1;


    //lptvShared->mask = TVIF_IMAGE;
    uSomeUint = TVIF_IMAGE;
    SharedWrite (&uSomeUint,&lptvShared->mask,sizeof(UINT),hProcess);
    //lptvShared->hItem = (HTREEITEM)id;
    SharedWrite (&id,&lptvShared->hItem,sizeof(HTREEITEM),hProcess);

    if (TreeView_GetItem(hwnd, lptvShared))
    {
        SharedRead (&lptvShared->iImage,&uSomeUint,sizeof(UINT),hProcess);
    }

    SharedFree(lptvShared,hProcess);

    return uSomeUint;
}


#endif // _WIN64




// This reads from the process associated with the given
// hwnd, and does the necessary OpenProcess/CloseHandle
// tidyup and checks....
BOOL ReadProcessMemoryHWND( HWND hwnd, void * pSrc, void * pDst, DWORD len )
{
    DWORD idProcess = 0;
    GetWindowThreadProcessId(hwnd, &idProcess);
    if( ! idProcess )
        return FALSE;

    HANDLE hProcess = OpenProcess( PROCESS_VM_READ, FALSE, idProcess );
    if( ! hProcess )
        return FALSE;

    DWORD cbActual = 0;
    BOOL retval = ReadProcessMemory( hProcess, pSrc, pDst, len, & cbActual )
            && len == cbActual;

    CloseHandle( hProcess );

    return retval;
}


BOOL GetStateImageMapEnt( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, MSAASTATEIMAGEMAPENT * pEnt )
{
    void * pAddress = (void *) GetProp( hwnd, TEXT("MSAAStateImageMapAddr") );
    if( ! pAddress )
        return FALSE;

    int NumStates = PtrToInt( GetProp( hwnd, TEXT("MSAAStateImageMapCount") ) );
    if( NumStates == 0 )
        return FALSE;

    int index = pfnGetImageIndex( hwnd, id );

    // <= used since number is a 1-based count, index is a 0-based index.
    // If index is 0, should be at least one state.
    if( NumStates <= index )
        return FALSE;

    // Adjust to index into array...
    pAddress = (void*)( (MSAASTATEIMAGEMAPENT*)pAddress + index );

    return ReadProcessMemoryHWND( hwnd, pAddress, pEnt, sizeof(MSAASTATEIMAGEMAPENT) );
}


BOOL GetRoleFromStateImageMap( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, DWORD * pdwRole )
{
    MSAASTATEIMAGEMAPENT MapEnt;
    if( GetStateImageMapEnt( pfnGetImageIndex, hwnd, id, & MapEnt ) )
    {
        *pdwRole = MapEnt.dwRole;
        return TRUE;
    }
    else
        return FALSE;
}

BOOL GetStateFromStateImageMap( PFNGetImageIndex pfnGetImageIndex, HWND hwnd, int id, DWORD * pdwState )
{
    MSAASTATEIMAGEMAPENT MapEnt;
    if( GetStateImageMapEnt( pfnGetImageIndex, hwnd, id, & MapEnt ) )
    {
        *pdwState = MapEnt.dwState;
        return TRUE;
    }
    else
        return FALSE;
}

