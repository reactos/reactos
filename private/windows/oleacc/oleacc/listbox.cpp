// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  LISTBOX.CPP
//
//  Listbox client class.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "window.h"
#include "listbox.h"

#define LBS_COMBOBOX    0x8000

const TCHAR szComboExName[] = TEXT("ComboBoxEx32");

STDAPI_(LPTSTR) MyPathFindFileName(LPCTSTR pPath);


BOOL IsTridentControl( HWND hWnd, BOOL fCombo, BOOL fComboList );


// --------------------------------------------------------------------------
//
//  CreateListBoxClient()
//
//  EXTERNAL for CClient.
//
// --------------------------------------------------------------------------
HRESULT CreateListBoxClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvListBox)
{
    CListBox * plist;
    HRESULT hr;

    InitPv(ppvListBox);

    plist = new CListBox(hwnd, idChildCur);
    if (!plist)
        return(E_OUTOFMEMORY);

    hr = plist->QueryInterface(riid, ppvListBox);
    if (!SUCCEEDED(hr))
        delete plist;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CListBox::CListBox()
//
// --------------------------------------------------------------------------
CListBox::CListBox(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);

    //
    // Check both the style and the CBOX data--SQL srvr creates controls
    // with bogus styles sometimes and could fool us into thinking this
    // was a combo.  USER's listbox creation code does the same check.
    //
    if (GetWindowLong(hwnd, GWL_STYLE) & LBS_COMBOBOX)
    {
        COMBOBOXINFO    cbi;

        if (MyGetComboBoxInfo(hwnd, &cbi))
        {
            m_fComboBox = TRUE;
            if (!(cbi.stateButton & STATE_SYSTEM_INVISIBLE))
                m_fDropDown = TRUE;
        }
    }

    m_fUseLabel = !m_fComboBox;
}


// --------------------------------------------------------------------------
//
//  CListBox::SetupChildren()
//
//  Sets the # of items we have.
//
// --------------------------------------------------------------------------
void CListBox::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, LB_GETCOUNT, 0, 0L);
}



// --------------------------------------------------------------------------
//
//  CListBox::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accName(VARIANT varChild, BSTR *pszName)
{
    InitPv(pszName);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == CHILDID_SELF)
    {
        if (m_fComboBox)
        {
            IAccessible* pacc;
            HRESULT hr;
            COMBOBOXINFO    cbi;

            //
            // Forward request up to combobox to get its name.
            //
            if (!MyGetComboBoxInfo(m_hwnd, &cbi))
                return(S_FALSE);

            pacc = NULL;
            hr = AccessibleObjectFromWindow(cbi.hwndCombo, OBJID_CLIENT,
                IID_IAccessible, (void**)&pacc);
            if (!SUCCEEDED(hr) || !pacc)
                return(S_FALSE);

            Assert(varChild.lVal == 0);
            hr = pacc->get_accName(varChild, pszName);
            pacc->Release();

            return(hr);
        }
        else
            return(CClient::get_accName(varChild, pszName));
    }
    else
    {
        UINT    cch;
        COMBOBOXINFO    cbi;
        UINT    msgLen;
        UINT    msgText;
        HWND    hwndAsk;
        TCHAR   szModuleName[MAX_PATH];
        LPTSTR  lpszModuleName;

        //
        // For a combobox, ask the COMBO for its text.  A lot of apps have
        // ownerdraw items, but actually subclass combos and return real
        // text for the items.
        //
        if (m_fComboBox && MyGetComboBoxInfo(m_hwnd, &cbi))
        {
            HWND    hwndT;

            hwndAsk = cbi.hwndCombo;
            if (hwndT = IsInComboEx(cbi.hwndCombo))
                hwndAsk = hwndT;

            msgLen = CB_GETLBTEXTLEN;
            msgText = CB_GETLBTEXT;
        }
        else
        {
            hwndAsk = m_hwnd;
            msgLen = LB_GETTEXTLEN;
            msgText = LB_GETTEXT;
        }

        //
        // Get the item text.
        //
        cch = SendMessageINT(hwndAsk, msgLen, varChild.lVal-1, 0);

        // Some apps do not handle LB_GETTEXTLEN correctly, and
        // always return a small number, like 2.
		if (cch < 1024)
			cch = 1024;

        if (cch)
        {
            // HACK ALERT
            // The IE4 listbox is a superclassed standard listbox,
            // but if I use SendMessageA (which I do) to get the
            // text, I just get back one character. They keep everything
            // in Unicode. It is a bug in the Trident MSHTML 
            // implementation, but even if they fixed it and gave me
            // back an ANSI string, I wouldn't know what code page to
            // use to convert the ANSI string to Unicode - web pages
            // can be in a different language than the one the user's
            // computer uses! Since they already have everything in 
            // Unicode, we decided on a private message that will fill
            // in the Unicode string, and I use that just like I would 
            // normally use LB_GETTEXT.
            // I was going to base this on the classname of the listbox
            // window, which is "Internet Explorer_TridentLstBox", but
            // the list part of a combo doesn't have a special class
            // name, so instead I am going to base the special case on
            // the file name of the module that owns the window.

            // GetWindowModuleFileName(hwndAsk,szModuleName,ARRAYSIZE(szModuleName));
            // lpszModuleName = MyPathFindFileName (szModuleName);
            // if (0 == lstrcmp(lpszModuleName,TEXT("MSHTML.DLL")))

            // Update: (BrendanM)
            // GetWindowModuleFilename is broken on Win2k...
            // IsTridentControl goes back to using classnames, and knows
            // how to cope with ComboLBoxes...

            if( IsTridentControl( m_hwnd, m_fComboBox, m_fComboBox ) )
            {
                OLECHAR*    lpszUnicodeText = NULL;
                OLECHAR*    lpszLocalText = NULL;
                HANDLE      hProcess;

                if (msgText == LB_GETTEXT)
                    msgText = WM_USER+LB_GETTEXT;
                else if (msgText == CB_GETLBTEXT)
                    msgText = WM_USER+CB_GETLBTEXT;

                lpszUnicodeText = (OLECHAR *)SharedAlloc((cch+1)*sizeof(OLECHAR),
                                                         hwndAsk,
                                                         &hProcess);
                lpszLocalText = (OLECHAR*)LocalAlloc(LPTR,(cch+1)*sizeof(OLECHAR));

                if (!lpszUnicodeText || !lpszLocalText)
                    return(E_OUTOFMEMORY);

                cch = SendMessageINT(hwndAsk, msgText, varChild.lVal-1, (LPARAM)lpszUnicodeText);
                SharedRead (lpszUnicodeText,lpszLocalText,(cch+1)*sizeof(OLECHAR),hProcess);

                *pszName = SysAllocString(lpszLocalText);

                SharedFree(lpszUnicodeText,hProcess);
                LocalFree(lpszLocalText);
            }
            else // normal, non IE4 code here:
            {
                LPTSTR lpszText;

                lpszText = (LPTSTR)LocalAlloc(LPTR, (cch+1)*sizeof(TCHAR));
                if (!lpszText)
                    return(E_OUTOFMEMORY);

                SendMessage(hwndAsk, msgText, varChild.lVal-1, (LPARAM)lpszText);
                *pszName = TCharSysAllocString(lpszText);

                LocalFree((HANDLE)lpszText);
            }
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBox::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
        pvarRole->lVal = ROLE_SYSTEM_LISTITEM;
    else
        pvarRole->lVal = ROLE_SYSTEM_LIST;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBox::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    RECT    rcItem;
    long    lStyle;

    InitPvar(pvarState);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == CHILDID_SELF)
        return(CClient::get_accState(varChild, pvarState));


    --varChild.lVal;

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    //
    // Is this item selected?
    //
    if (SendMessage(m_hwnd, LB_GETSEL, varChild.lVal, 0))
        pvarState->lVal |= STATE_SYSTEM_SELECTED;

    //
    // Does it have the focus?  Remember that we decremented the lVal so it
    // is zero-based like listbox indeces.
    //
    if (MyGetFocus() == m_hwnd)
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        if (SendMessage(m_hwnd, LB_GETCARETINDEX, 0, 0) == varChild.lVal)
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    }
    else if (m_fComboBox)
    {
    COMBOBOXINFO    cbi;
        if (MyGetComboBoxInfo(m_hwnd, &cbi))
        {
            // if this list is part of a combo box, AND the list
            // is showing (m_fDropdown is true), then say we are
            // focusable.
            if (m_fDropDown)
            {
                pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
                if (MyGetFocus() == cbi.hwndCombo)
                {
                    if (SendMessage(m_hwnd, LB_GETCARETINDEX, 0, 0) == varChild.lVal)
                        pvarState->lVal |= STATE_SYSTEM_FOCUSED;
                }
            } // end if it is dropped
        } // end if we got combo box info
    } // end if this is a combox box list

    //
    // Is the listbox read-only?
    //
    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);

    if (lStyle & LBS_NOSEL)
        pvarState->lVal |= STATE_SYSTEM_READONLY;
    else
    {
        pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

        //
        // Is the listbox multiple and/or extended sel?  NOTE:  We have
        // no way to implement accSelect() EXTENDSELECTION so don't.
        //
        if (lStyle & LBS_MULTIPLESEL)
            pvarState->lVal |= STATE_SYSTEM_MULTISELECTABLE;
    }

    //
    // Is the item in view?
    //
	// SMD 09/16/97 Offscreen things are things never on the screen,
	// and that doesn't apply to this. Changed from OFFSCREEN to
	// INVISIBLE.
    if (!SendMessage(m_hwnd, LB_GETITEMRECT, varChild.lVal, (LPARAM)&rcItem))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBox::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    //
    // Validate
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if ((varChild.lVal == 0) && !m_fComboBox)
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CListBox::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accFocus(VARIANT *pvarChild)
{
    InitPvar(pvarChild);

    //
    // Are we the focus?
    //
    if (MyGetFocus() == m_hwnd)
    {
        long    lCaret;

        pvarChild->vt = VT_I4;

        lCaret = SendMessageINT(m_hwnd, LB_GETCARETINDEX, 0, 0L);
        if (lCaret != LB_ERR)
            pvarChild->lVal = lCaret+1;
        else
            pvarChild->lVal = 0;

        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CListBox::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accSelection(VARIANT *pvarSelection)
{
    return(GetListBoxSelection(m_hwnd, pvarSelection));
}


// --------------------------------------------------------------------------
//
//  CListBox::get_accDefaultAction()
//
//  Since the default action for a listbox item is really determined by the
//  creator of the listbox control, the best we can do is double click on
//  the thing, and return "double click" as the default action string.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
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
//  CListBox::accDoDefaultAction()
//
//  As noted above, we really don't know what the default action for a list
//  box item is, so unless the parent overrides us, we'll just do a double
//  click on the thing.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::accDoDefaultAction(VARIANT varChild)
{
	RECT		rcLoc;
	HRESULT		hr;

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
    {
	    hr = accLocation(&rcLoc.left,&rcLoc.top,&rcLoc.right,&rcLoc.bottom,varChild);
	    if (!SUCCEEDED (hr))
		    return (hr);

        // this will check if WindowFromPoint at the click point is the same
	    // as m_hwnd, and if not, it won't click. Cool!
	    if (ClickOnTheRect(&rcLoc,m_hwnd,TRUE))
		    return (S_OK);
    }
    return(E_NOT_APPLICABLE);
}


// --------------------------------------------------------------------------
//
//  CListBox::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::accSelect(long selFlags, VARIANT varChild)
{
    long    lStyle;
    int     nFocusedItem;

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild)   ||
        ! ValidateSelFlags(selFlags))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accSelect(selFlags, varChild));

    varChild.lVal--;

    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if (lStyle & LBS_NOSEL)
        return(E_NOT_APPLICABLE);

    // note that LB_SETCURSEL doesn't work for extended or multi-select 
    // listboxes, have to use LB_SELITEMRANGE or LB_SETSEL. 
    if ((lStyle & LBS_MULTIPLESEL) ||
        (lStyle & LBS_EXTENDEDSEL))
    {
        // get the focused item here in case we change it. 
        nFocusedItem = SendMessageINT(m_hwnd,LB_GETCARETINDEX,0,0);

	    if (selFlags & SELFLAG_TAKEFOCUS) 
        {
            if (MyGetFocus() != m_hwnd)
                return(S_FALSE);
            SendMessage (m_hwnd, LB_SETCARETINDEX,varChild.lVal,0);
        }

        // These seem to be weird - when you tell it to set the selection, it 
        // also sets the focus. So we remember focus and reset it at the end.
	    if (selFlags & SELFLAG_TAKESELECTION)
        {
            // deselect the whole range of items
            SendMessage(m_hwnd, LB_SETSEL,FALSE,-1);
            // Select this one
            SendMessage(m_hwnd, LB_SETSEL, TRUE, varChild.lVal);
        }

        if (selFlags & SELFLAG_EXTENDSELECTION)
        {
        BOOL    bSelected;

            if ((selFlags & SELFLAG_ADDSELECTION) || (selFlags & SELFLAG_REMOVESELECTION))
                SendMessage (m_hwnd,LB_SELITEMRANGE,(selFlags & SELFLAG_ADDSELECTION),MAKELPARAM(nFocusedItem,varChild.lVal));
            else
            {
                bSelected = SendMessageINT(m_hwnd,LB_GETSEL,nFocusedItem,0);
                SendMessage (m_hwnd,LB_SELITEMRANGE,bSelected,MAKELPARAM(nFocusedItem,varChild.lVal));
            }
        }
        else // not extending, check add/remove
        {
            if ((selFlags & SELFLAG_ADDSELECTION) || (selFlags & SELFLAG_REMOVESELECTION))
                SendMessage(m_hwnd, LB_SETSEL, (selFlags & SELFLAG_ADDSELECTION),varChild.lVal);
        }
        // set focus to where it was before if SELFLAG_TAKEFOCUS not set
        if ((selFlags & SELFLAG_TAKEFOCUS) == 0)
            SendMessage (m_hwnd, LB_SETCARETINDEX,nFocusedItem,0);
    }
    else // listbox is single select
    {
        if (selFlags & (SELFLAG_ADDSELECTION | 
                        SELFLAG_REMOVESELECTION | 
                        SELFLAG_EXTENDSELECTION))
            return (E_INVALIDARG);

        // single select listboxes do not allow you to set the
        // focus independently of the selection, so we send a 
        // LB_SETCURSEL for both TAKESELECTION and TAKEFOCUS
	    if ((selFlags & SELFLAG_TAKESELECTION) ||
            (selFlags & SELFLAG_TAKEFOCUS))
            SendMessage(m_hwnd, LB_SETCURSEL, varChild.lVal, 0);
    } // end if listbox is single select
	
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBox::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::accLocation(long* pxLeft, long *pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    RECT    rc;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate params
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    //
    // Get item rect.
    //
    if (SendMessage(m_hwnd, LB_GETITEMRECT, varChild.lVal-1, (LPARAM)&rc))
    {
        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rc, 2);

        *pxLeft = rc.left;
        *pyTop = rc.top;
        *pcxWidth = rc.right - rc.left;
        *pcyHeight = rc.bottom - rc.top;
    }

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CListBox::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::accNavigate(long dwNavDir, VARIANT varStart, VARIANT *pvarEnd)
{
    long lEnd;
    long lRows;

    InitPvar(pvarEnd);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varStart)   ||
        ! ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    //
    // Is this something for the client (or combobox) to handle?
    //
    if (dwNavDir == NAVDIR_FIRSTCHILD)
    {
        lEnd = 1;
        if (lEnd > m_cChildren)
            lEnd = 0;
    }
    else if (dwNavDir == NAVDIR_LASTCHILD)
        lEnd = m_cChildren;
    else if (varStart.lVal == CHILDID_SELF)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));
    else
    {
        long    lT;

        lRows = MyGetListBoxInfo(m_hwnd);
        if (!lRows)
            return(S_FALSE);

        lEnd = 0;

        lT = varStart.lVal - 1;

        switch (dwNavDir)
        {
            case NAVDIR_LEFT:
                //
                // Are there any items to the left of us?
                //
                if (lT >= lRows)
                    lEnd = varStart.lVal - lRows;
                break;

            case NAVDIR_RIGHT:
                //
                // Are there are any items to the right of us?
                //
                if (lT + lRows < m_cChildren)
                    lEnd = varStart.lVal + lRows;
                break;

            case NAVDIR_UP:
                //
                // Are we in the top-most row?
                //
                if ((lT % lRows) != 0)
                    lEnd = varStart.lVal - 1;
                break;

            case NAVDIR_DOWN:
                //
                // Are we the last item or in the bottom-most row?
                //
                if (((lT+1) % lRows) != 0)
                {
                    lEnd = varStart.lVal + 1;
                    if (lEnd > m_cChildren)
                        lEnd = 0;
                }
                break;

            case NAVDIR_PREVIOUS:
                lEnd = varStart.lVal - 1;
                break;

            case NAVDIR_NEXT:
                lEnd = varStart.lVal + 1;
                if (lEnd > m_cChildren)
                    lEnd = 0;
                break;
        }
    }

    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
    }

    return(lEnd ? S_OK : S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CListBox::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBox::accHitTest(long xLeft, long yTop, VARIANT *pvarHit)
{
    POINT   pt;
    RECT    rc;
    long    l;
    
    InitPvar(pvarHit);

    //
    // Is the point in our client area?
    //
    pt.x = xLeft;
    pt.y = yTop;
    ScreenToClient(m_hwnd, &pt);

    MyGetRect(m_hwnd, &rc, FALSE);

    if (!PtInRect(&rc, pt))
        return(S_FALSE);

    //
    // What item is here?
    //
    l = SendMessageINT(m_hwnd, LB_ITEMFROMPOINT, 0, MAKELONG(pt.x, pt.y));

    pvarHit->vt = VT_I4;

    if (HIWORD(l))
    {
        // Outside bounds, in white space.
        pvarHit->lVal = 0;
    }
    else
    {
        pvarHit->lVal = (int)(short)LOWORD(l) + 1;
    }


    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  CreateListBoxWindow()
//
// --------------------------------------------------------------------------
HRESULT CreateListBoxWindow(HWND hwnd, long idChildCur, REFIID riid, void** ppvListBoxW)
{
    HRESULT hr;

    CListBoxFrame * plframe;

    InitPv(ppvListBoxW);

    plframe = new CListBoxFrame(hwnd, idChildCur);
    if (!plframe)
        return(E_OUTOFMEMORY);

    hr = plframe->QueryInterface(riid, ppvListBoxW);
    if (!SUCCEEDED(hr))
        delete plframe;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CListBoxFrame::CListBoxFrame()
//
// --------------------------------------------------------------------------
CListBoxFrame::CListBoxFrame(HWND hwnd, long iChildCur)
{
    Initialize(hwnd, iChildCur);

    if (GetWindowLong(hwnd, GWL_STYLE) & LBS_COMBOBOX)
    {
        COMBOBOXINFO    cbi;

        if (MyGetComboBoxInfo(hwnd, &cbi))
        {
            m_fComboBox = TRUE;
            if (!(cbi.stateButton & STATE_SYSTEM_INVISIBLE))
                m_fDropDown = TRUE;
        }
    }
}



// --------------------------------------------------------------------------
//
//  CListBoxFrame::get_accParent()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxFrame::get_accParent(IDispatch** ppdispParent)
{
    InitPv(ppdispParent);

    //
    // We need to handle combo dropdowns specially, since they are made
    // children of the desktop for free floating.
    //
    if (m_fComboBox && m_fDropDown)
    {
        COMBOBOXINFO    cbi;

        if (!MyGetComboBoxInfo(m_hwnd, &cbi))
            return(S_FALSE);

        //
        // Get the combo info and create our combobox parent.
        //
        return(AccessibleObjectFromWindow(cbi.hwndCombo, OBJID_CLIENT,
            IID_IDispatch, (void**)ppdispParent));
    }
    else
        return(CWindow::get_accParent(ppdispParent));
}



// --------------------------------------------------------------------------
//
//  CListBoxFrame::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxFrame::get_accState(VARIANT varStart, VARIANT *pvarState)
{
    HRESULT hr;

    InitPvar(pvarState);

    if (! ValidateChild(&varStart))
        return(E_INVALIDARG);

    //
    // Get the window's state
    //
    hr = CWindow::get_accState(varStart, pvarState);
    if (SUCCEEDED(hr) && m_fComboBox && m_fDropDown && (varStart.lVal == 0))
    {
        pvarState->lVal |= STATE_SYSTEM_FLOATING;
    }
    return(hr);

}




// --------------------------------------------------------------------------
//
//  CListBoxFrame::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxFrame::accNavigate(VARIANT varStart, long dwNavDir,
    VARIANT* pvarEnd)
{
    COMBOBOXINFO    cbi;

    InitPvar(pvarEnd);

    //
    // Validate.
    //
    if (! ValidateChild(&varStart)   ||
        ! ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    //
    // Hand off to CWindow if (1) first child, (2) non-zero start.
    //
    Assert(NAVDIR_LASTCHILD > NAVDIR_FIRSTCHILD);

    if (!m_fComboBox || (dwNavDir >= NAVDIR_FIRSTCHILD) || varStart.lVal)
        return(CWindow::accNavigate(dwNavDir, varStart, pvarEnd));

    //
    // Get our parent window
    //
    if (! MyGetComboBoxInfo(m_hwnd, &cbi))
        return(S_FALSE);

    return(GetParentToNavigate(INDEX_COMBOBOX_LIST, cbi.hwndCombo,
        OBJID_CLIENT, dwNavDir, pvarEnd));
}




/////////////////////////////////////////////////////////////////////////////
//
//  MULTIPLE SELECTION LISTBOX SUPPORT
//
//  If a listbox has more than one item selected, we create an object
//  that is a clone.  It keeps a list of the selected items.  Its sole
//  purpose is to respond to IEnumVARIANT, a collection.  The caller should
//  either 
//      (a) Pass the child ID to the parent object to get acc info
//      (b) Call the child directly if VT_DISPATCH.
//
/////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------------
//
//  GetListBoxSelection()
//
// --------------------------------------------------------------------------
HRESULT GetListBoxSelection(HWND hwnd, VARIANT* pvarSelection)
{
    int cSelected;
    LPINT lpSelected;
    long lRet;
    CListBoxSelection * plbs;

    InitPvar(pvarSelection);

    cSelected = SendMessageINT(hwnd, LB_GETSELCOUNT, 0, 0);

    if (cSelected <= 1)
    {
        // 
        // cSelected is -1, 0, or 1.  
        //      -1 means this is a single sel listbox.  
        //      0 or 1 means this is multisel
        //
        lRet = SendMessageINT(hwnd, LB_GETCURSEL, 0, 0);
        if (lRet == -1)
            return(S_FALSE);

        pvarSelection->vt = VT_I4;
        pvarSelection->lVal = lRet+1;
        return(S_OK);
    }

    //
    // Multiple items; must make a collection
    //

    //
    // Allocate memory for the list of item IDs
    //
    lpSelected = (LPINT)LocalAlloc(LPTR, cSelected*sizeof(INT));
    if (!lpSelected)
        return(E_OUTOFMEMORY);

    //
    // Get the list of selected item IDs
    //
    plbs = NULL;

    lRet = SendMessageINT(hwnd, LB_GETSELITEMS, cSelected, (LPARAM)lpSelected);
    if (lRet != LB_ERR)
    {
        plbs = new CListBoxSelection(0, lRet, lpSelected);
        if (plbs)
        {
            pvarSelection->vt = VT_UNKNOWN;
            plbs->QueryInterface(IID_IUnknown, (void**)&(pvarSelection->punkVal));
        }
    }

    //
    // Free the list memory; the constructor will make a copy.  This is
    // because the constructor is called both from create and clone.
    //
    LocalFree((HANDLE)lpSelected);

    if (!plbs)
        return(E_OUTOFMEMORY);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBoxSelection::CListBoxSelection()
//
//  We AddRef() once plistFrom so that it won't go away out from us.  When
//  we are destroyed, we will Release() it.
//
// --------------------------------------------------------------------------
CListBoxSelection::CListBoxSelection(int iChildCur, int cSelected, LPINT lpSelection)
{
    m_idChildCur = iChildCur;

    m_lpSelected = (LPINT)LocalAlloc(LPTR, cSelected*sizeof(int));
    if (!m_lpSelected)
        m_cSelected = 0;
    else
    {
        m_cSelected = cSelected;
        CopyMemory(m_lpSelected, lpSelection, cSelected*sizeof(int));
    }
}




// --------------------------------------------------------------------------
//
//  CListBoxSelection::~CListBoxSelection()
//
// --------------------------------------------------------------------------
CListBoxSelection::~CListBoxSelection()
{
    //
    // Free item memory
    //
    if (m_lpSelected)
    {
        LocalFree((HANDLE)m_lpSelected);
        m_lpSelected = NULL;
    }
}




// --------------------------------------------------------------------------
//
//  CListBoxSelection::QueryInterface()
//
//  We only respond to IUnknown and IEnumVARIANT!  It is the responsibility
//  of the caller to loop through the items using IEnumVARIANT interfaces,
//  and get the child IDs to then pass to the parent object (or call 
//  directly if VT_DISPATCH--not in this case they aren't though).
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::QueryInterface(REFIID riid, void** ppunk)
{
    *ppunk = NULL;

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
//  CListBoxSelection::AddRef()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListBoxSelection::AddRef(void)
{
    return(++m_cRef);
}



// --------------------------------------------------------------------------
//
//  CListBoxSelection::Release()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListBoxSelection::Release(void)
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
//  CListBoxSelection::Next()
//
//  This returns a VT_I4 which is the child ID for the parent listbox that
//  returned this object for the selection collection.  The caller turns
//  around and passes this variant to the listbox object to get acc info
//  about it.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Next(ULONG celt, VARIANT* rgvar, ULONG *pceltFetched)
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
//  CListBoxSelection::Skip()
//
// -------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Skip(ULONG celt)
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
//  CListBoxSelection::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBoxSelection::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Clone(IEnumVARIANT **ppenum)
{
    CListBoxSelection * plistselnew;

    InitPv(ppenum);

    plistselnew = new CListBoxSelection(m_idChildCur, m_cSelected, m_lpSelected);
    if (!plistselnew)
        return(E_OUTOFMEMORY);

    return(plistselnew->QueryInterface(IID_IEnumVARIANT, (void**)ppenum));
}



// --------------------------------------------------------------------------
//
//  IsComboEx()
//
//  Returns TRUE if this window is a comboex32
//
// --------------------------------------------------------------------------
BOOL IsComboEx(HWND hwnd)
{
    TCHAR   szClass[128];

    return MyGetWindowClass(hwnd, szClass, ARRAYSIZE(szClass) ) &&
                ! lstrcmpi(szClass, szComboExName);
}


// --------------------------------------------------------------------------
//
//  IsInComboEx()
//
//  Returns the COMBOEX window if the combo is embedded in a COMBOEX (like
//  on the toolbar).
//
// --------------------------------------------------------------------------
HWND IsInComboEx(HWND hwnd)
{
    HWND hwndParent = MyGetAncestor(hwnd, GA_PARENT);
    if( hwndParent && IsComboEx(hwndParent) )
        return hwndParent;
    else
        return NULL;
}


// --------------------------------------------------------------------------
// Copied from shlwapi\path.c
//
// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo
// --------------------------------------------------------------------------

STDAPI_(LPTSTR)
MyPathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
            && pPath[1] &&  pPath[1] != TEXT('\\')  &&   pPath[1] != TEXT('/'))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}


/*
 *  IsTridentControl
 *
 *  HWND hWnd
 *      window to test against
 *  BOOL fCombo
 *      TRUE if this is a combo or a combolbox
 *  BOOL fComboList
 *      TRUE if this is a combolbox (the drop-down list box associated with a combo)
 *
 *  This works by comparing class names - "Internet Explorer_TridentCmboBx"
 *  for combos, and "Internet Explorer_TridentLstBox" for listboxes.
 *  The drop-lists of combos don't have a special class, so instead we get
 *  the 'parent' combo, and check it against "Internet Explorer_TridentCmboBx".
 *
 */
BOOL IsTridentControl( HWND hWnd, BOOL fCombo, BOOL fComboList )
{
    // If this is a drop-list, get the associated combo...
    if( fComboList )
    {
        COMBOBOXINFO cbi;
        if( ! MyGetComboBoxInfo( hWnd, & cbi ) || cbi.hwndCombo == NULL )
        {
            return FALSE;
        }

        hWnd = cbi.hwndCombo;
    }

    // Get class name...
    TCHAR szClass[64];
    szClass[0] = '\0';
    GetClassName( hWnd, szClass, ARRAYSIZE( szClass ) );

    // Compare against expected string...
    TCHAR * pszCompare;
    if( fCombo )
        pszCompare = TEXT("Internet Explorer_TridentCmboBx");
    else
        pszCompare = TEXT("Internet Explorer_TridentLstBox");


    return lstrcmp( szClass, pszCompare ) == 0;
}