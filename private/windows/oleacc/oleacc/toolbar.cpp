// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TOOLBAR.CPP
//
//  This knows how to talk to COMCTL32's tool bar control.
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
#define NOPROGRESS
#define NOHOTKEY
#define NOHEADER
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE
#include <commctrl.h>

#include "toolbar.h"

#define MAX_NAME_SIZE   128 

// --------------------------------------------------------------------------
//
//  CreateToolBarClient()
//
//  EXTERNAL for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateToolBarClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvTool)
{
    HRESULT hr;
    CToolBar32* ptool;

    InitPv(ppvTool);

    ptool = new CToolBar32(hwnd, idChildCur);
    if (! ptool)
        return(E_OUTOFMEMORY);

    hr = ptool->QueryInterface(riid, ppvTool);
    if (!SUCCEEDED(hr))
        delete ptool;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CToolBar32::CToolBar32()
//
// --------------------------------------------------------------------------
CToolBar32::CToolBar32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  CToolBar32::SetupChildren()
//
//  We need the # of buttons, plus 1 if there is a window child.
//
// --------------------------------------------------------------------------
void CToolBar32::SetupChildren()
{
    m_cChildren = SendMessageINT(m_hwnd, TB_BUTTONCOUNT, 0, 0);

    if (::GetWindow(m_hwnd,GW_CHILD))
        m_cChildren++;
}



// --------------------------------------------------------------------------
//
//  CToolBar32::GetItemData()
//
//  This gets the data from a button in the toolbar, the command ID, the 
//  state, the style, etc.  We need the command ID for example to pass to
//  most TB_ messages instead of the index.
//
// --------------------------------------------------------------------------
BOOL CToolBar32::GetItemData(int itemID, LPTBBUTTON lptbResult)
{
LPTBBUTTON  lptbShared;
BOOL        fReturn;
HANDLE      hProcess;

    fReturn = FALSE;

    // Allocate a TBBUTTON struct from shared memory
    lptbShared = (LPTBBUTTON)SharedAlloc(sizeof(TBBUTTON),m_hwnd,&hProcess);
    if (lptbShared)
    {
        if (SendMessage(m_hwnd, TB_GETBUTTON, itemID-1, (LPARAM)lptbShared))
        {
            SharedRead (lptbShared,lptbResult,sizeof(TBBUTTON),hProcess);
            fReturn = TRUE;
        }

        SharedFree(lptbShared,hProcess);
    }

    return(fReturn);
}



// --------------------------------------------------------------------------
//
//  CToolBar32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::get_accName(VARIANT varChild, BSTR* pszName)
{
LPTSTR  lpszName = NULL;
HRESULT hr;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::get_accName(varChild, pszName));

    // if child id is not zero (CHILDID_SELF)...
    hr = GetToolbarString (varChild.lVal,&lpszName);
    if( ! lpszName )
        return (hr); // will be S_FALSE or an E_error_code

    if (*lpszName)
    {
        StripMnemonic(lpszName);
        *pszName = TCharSysAllocString(lpszName);
    }

	LocalFree (lpszName);
    
    return(*pszName ? S_OK : S_FALSE);


}


// --------------------------------------------------------------------------
//
//  CToolBar32::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
TCHAR   chMnemonic = 0;
LPTSTR  lpszName = NULL;
HRESULT hr;

    InitPv(pszShortcut);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));

    // if child id is not zero (CHILDID_SELF)...
    hr = GetToolbarString (varChild.lVal,&lpszName);
    if ( ! lpszName )
        return (hr); // will be S_FALSE or E_error_code...

    if (*lpszName)
		chMnemonic = StripMnemonic(lpszName);

	LocalFree (lpszName);

	//
	// Is there a mnemonic?
	//
	if (chMnemonic)
	{
		//
		// Make a string of the form "Alt+ch".
		//
		TCHAR   szKey[2];

		*szKey = chMnemonic;
		*(szKey+1) = 0;

		return(HrMakeShortcut(szKey, pszShortcut));
	}

    return(S_FALSE);

}


// --------------------------------------------------------------------------
//
//  CToolBar32::GetToolbarString()
//
// Get the name of the item on the toolbar. There are two ways to do this - 
// You can just ask using the standard messages, or if that fails, you
// can try to get it from a tooltip. Since we need to do this for both name 
// and keyboard shortcut, we'll write a private method to get the 
// unstripped name.
//
// Parameters:
//		int	ChildId	        - the Child ID (1 based) of the item we want to get
//		LPTSTR*	ppszName	- pointer that will be LocalAlloc'ed and filled
//							  in with the name. Caller must LocalFree it.
//
//	Returns:
//
//      On Success:
//        returns S_TRUE, *ppszName will be non-NULL, caller must LocalFree() it.
//
//      On Failure:
//		  returns S_FALSE - no name available. *ppszName set to NULL.
//		  ...or...
//		  returns COM Failure code (including E_OUTOFMEMORY) - com/memory error.
//		  *ppszName set to NULL.
//
// Note: caller should take care if using "FAILED( hr )" to examine the return
// value of this method, since it does treats both S_OK and S_FALSE as 'success'.
// It may be better to check that *ppszName is non-NULL.
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::GetToolbarString(int ChildId, LPTSTR* ppszName)
{
LPTSTR      lpszTextShared;
int         cchText;
int         nSomeInt;
TBBUTTON    tb;
HANDLE      hProcess;
LPTSTR      pszName;

	// Set this to NULL now, in case we return an error code (or S_FALSE) later...
    // (We'll set it to a valid return value later if we succeed...)
    *ppszName = NULL;

    // Get the button ID
    if (!GetItemData(ChildId, &tb))
        return(S_FALSE);

    //
    // Get the button text length.  NOTE:  If this is a separator item
    // then just return empty now.
    //
    if (tb.fsStyle & TBSTYLE_SEP)
        return(S_FALSE);

    cchText = SendMessageINT(m_hwnd, TB_GETBUTTONTEXT, tb.idCommand, 0);
    if (cchText && (cchText != -1))
    {
        // Allocate a buffer to hold it
        lpszTextShared = (LPTSTR)SharedAlloc((cchText+1)*sizeof(TCHAR),
                                        m_hwnd,&hProcess);

        if (! lpszTextShared)
            return(E_OUTOFMEMORY);

        pszName =  (LPTSTR)LocalAlloc(LPTR,(cchText+1)*sizeof(TCHAR));
        if (! pszName)
        {
            SharedFree (lpszTextShared,hProcess);
            return(E_OUTOFMEMORY);
        }

        // Get the button text
        nSomeInt = 0;
        SharedWrite (&nSomeInt,lpszTextShared,sizeof(int),hProcess);

        SendMessage(m_hwnd, TB_GETBUTTONTEXT, tb.idCommand, (LPARAM)lpszTextShared);

        SharedRead (lpszTextShared,pszName,(cchText+1)*sizeof(TCHAR),hProcess);
        SharedFree(lpszTextShared,hProcess);

		// At this stage, local var pszName points to a (possibly) empty string.
		// We deal with that after this else...

    }
    else // Button has no text, so use tooltip method.
    {
        HWND        hwndToolTip;
        LPTOOLINFO  lptiShared;
        LPTSTR      lpszTextShared;
        UINT        uSomeUint;

        //
        // Use tooltips, a hack but it was put there on purpose for
        // stuff like this.
        //
        hwndToolTip = (HWND)SendMessage(m_hwnd, TB_GETTOOLTIPS, 0, 0);
        if (!hwndToolTip)
            return(S_FALSE);

        //
        // Allocate a TOOLINFO buffer and a string buffer.  Note that
        // text is MAX_NAME_SIZE chars or less!
        //
        lptiShared = (LPTOOLINFO)SharedAlloc(sizeof(TOOLINFO) + 
                                             (MAX_NAME_SIZE+1)*sizeof(TCHAR),
                                             m_hwnd,&hProcess);
        if (!lptiShared)
            return(E_OUTOFMEMORY);

        //
        // Initialize tool info with window, button ID, and text.
        //
        lpszTextShared = (LPTSTR)(lptiShared + 1);

        SharedWrite (&lpszTextShared,&lptiShared->lpszText,sizeof(LPTSTR),hProcess);
        uSomeUint = sizeof(TOOLINFO);
        SharedWrite (&uSomeUint,&lptiShared->cbSize,sizeof(UINT),hProcess);
        SharedWrite (&m_hwnd,&(lptiShared->hwnd),sizeof(HWND),hProcess);
        SharedWrite (&tb.idCommand,&lptiShared->uId,sizeof(int),hProcess);

        SendMessage(hwndToolTip, TTM_GETTEXT, 0, (LPARAM)lptiShared);

        pszName = (LPTSTR)LocalAlloc (LPTR,(MAX_NAME_SIZE+1)*sizeof(TCHAR));
        if (!pszName)
        {
            SharedFree (lptiShared,hProcess);
            return (E_OUTOFMEMORY);
        }

        SharedRead (lpszTextShared,pszName,(MAX_NAME_SIZE+1)*sizeof(TCHAR),hProcess);
        SharedFree(lptiShared,hProcess);

		// At this stage, local var pszName points to a (possibly) empty string.
		// We deal with that next...
	}


    // do we have a non-empty string?
    if( *pszName )
	{
		*ppszName = pszName;
        return S_OK;
	}
	else
	{
		// *ppszName will still be NULL from the start of this method.
		// Free the 'empty' pszName...
		LocalFree( pszName );
		return S_FALSE;
	}
}


// --------------------------------------------------------------------------
//
//  CToolBar32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
    {
        pvarRole->vt = VT_I4;
        pvarRole->lVal = ROLE_SYSTEM_TOOLBAR;
    }
    else
    {
        TBBUTTON tb;

        // Get the button type (checkbox, radio, or push).
        if (!GetItemData(varChild.lVal, &tb))
            return(S_FALSE);

        pvarRole->vt = VT_I4;

        // If a separator, say so
		if (tb.fsStyle & TBSTYLE_SEP)
            pvarRole->lVal = ROLE_SYSTEM_SEPARATOR;
        else if (tb.fsStyle & TBSTYLE_CHECK)
        {
			// Check other possible styles
            if (tb.fsStyle & TBSTYLE_GROUP)
                pvarRole->lVal = ROLE_SYSTEM_RADIOBUTTON;
            else
                pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
        }
        else
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CToolBar32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    InitPvar(pvarState);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
	{
        pvarState->vt = VT_I4;
        pvarState->lVal = 0;
        return(S_OK);
	}
    else
    {
        TBBUTTON tb;

        if (! GetItemData(varChild.lVal, &tb))
        {
            pvarState->vt = VT_I4;
            pvarState->lVal = STATE_SYSTEM_INVISIBLE;
            return(S_OK);
        }

        pvarState->vt = VT_I4;
        pvarState->lVal = 0;

        if (tb.fsState & TBSTATE_CHECKED)
            pvarState->lVal |= STATE_SYSTEM_CHECKED;
        if (tb.fsState & TBSTATE_PRESSED)
            pvarState->lVal |= STATE_SYSTEM_PRESSED;
        if (!(tb.fsState & TBSTATE_ENABLED))
            pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;
        if (tb.fsState & TBSTATE_HIDDEN)
            pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        if (tb.fsState & TBSTATE_INDETERMINATE)
            pvarState->lVal |= STATE_SYSTEM_MIXED;

        if (tb.fsStyle & TBSTYLE_ALTDRAG)
            pvarState->lVal |= STATE_SYSTEM_MOVEABLE;

        // What about separators?
		// CWO, 4/22/97, Separators have a state of TBSTATE_ENABLED

        return(S_OK);
    }
}



// --------------------------------------------------------------------------
//
//  CToolBar32::get_accDefaultAction()
//
//  Default action is same as the name of the button.
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::get_accDefaultAction(VARIANT varChild, BSTR* pszDef)
{
    InitPv(pszDef);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::get_accDefaultAction(varChild, pszDef));
    else
    {
        TBBUTTON tb;

        // TBSTYLE_DROP has a different default action than the name
        if (GetItemData(varChild.lVal, &tb) && (tb.fsStyle & TBSTYLE_DROPDOWN))
            return(HrCreateString(STR_DROPDOWN_SHOW, pszDef));
        else
            return(HrCreateString(STR_BUTTON_PUSH, pszDef));
    }
}



// --------------------------------------------------------------------------
//
//  CToolBar32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));
    else
    {
        LPRECT  prcShared;
        HRESULT hr;
        RECT    rcLocal;
        HANDLE  hProcess;

        // Allocate a Shared RECT
        prcShared = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
        if (! prcShared)
            return(E_OUTOFMEMORY);

        // This returns FALSE if the button is hidden
        if (SendMessage(m_hwnd, TB_GETITEMRECT, varChild.lVal-1, (LPARAM)prcShared))
        {
            hr = S_OK;

            SharedRead (prcShared,&rcLocal,sizeof(RECT),hProcess);
            MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

            *pxLeft = rcLocal.left;
            *pyTop = rcLocal.top;
            *pcxWidth = rcLocal.right - rcLocal.left;
            *pcyHeight = rcLocal.bottom - rcLocal.top;
        }
        else
            hr = S_FALSE;

        SharedFree(prcShared,hProcess);

        return(hr);
    }

}



// --------------------------------------------------------------------------
//
//  CToolBar32::accNavigate()
//
//  Toolbar clients can only set an indent on the left side.  Hence all
//  child window objects are on the left, buttons are on the right.
//
//  BOGUS!  Doesn't deal with wrapped toolbars yet.
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    int lEnd = 0;
    TBBUTTON tb;

    InitPvar(pvarEnd);

    if ((!ValidateChild(&varStart) && !ValidateHwnd(&varStart)) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    // Peer-to-peer navigation in nonclient
    if (!varStart.lVal && (dwNavDir < NAVDIR_FIRSTCHILD))
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    //
    // Nav is strange, the first items on the left have the highest IDs.
    //
    if (dwNavDir == NAVDIR_FIRSTCHILD)
        dwNavDir = NAVDIR_NEXT;
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        dwNavDir = NAVDIR_PREVIOUS;
        varStart.lVal = m_cChildren + 1;
    }

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
        case NAVDIR_RIGHT:
            lEnd = varStart.lVal;

            // Toolbars can't really have windows as children - it is
            // a hack that explorer does. Because it is a hack, we know
            // that when we get starting point that is a window, it is 
            // the first child, so to naviagte next, we just move to the
            // first 'real' child. 
            // If we are trying to navigate next from 0 (the toolbar itself) 
            // we just check if the toolbar window has a child window, and 
            // return a dispatch interface to that child.
            if (lEnd == CHILDID_SELF)
            {
            HWND    hwndChild;

                if (hwndChild = ::GetWindow(m_hwnd,GW_CHILD))
                {
			        pvarEnd->vt=VT_DISPATCH;
                    return (AccessibleObjectFromWindow(hwndChild,OBJID_WINDOW,
                        IID_IDispatch, (void**)&pvarEnd->pdispVal));
                }
            }
            // just set lEnd to 0 so we get the first 'real' child
            // of the toolbar - the first button.
            if (IsHWNDID(lEnd))
                lEnd = 0;

            while (++lEnd <= m_cChildren)
            {
                //
                // Is this a visible child?
				// CWO, 4/22/97, removed separate clause
                //
                if (GetItemData(lEnd, &tb) && !(tb.fsState & TBSTATE_HIDDEN))
                    break; // out of while loop
            }

            if (lEnd > m_cChildren)
                lEnd = 0;
            break; // out of switch 

        case NAVDIR_PREVIOUS:
        case NAVDIR_LEFT:
            lEnd = varStart.lVal;

            // Navigating previous is similar to next when dealing with 
            // children that are windows. If the start point is a child
            // window, then the end point is 0, the toolbar itself. If
            // the end point (after doing normal children) is 0, then 
            // check if the toolbar has a child window and if so, return
            // a dispatch interface to that object.
            if (IsHWNDID(lEnd))
            {
                lEnd = 0;
                break; // out of switch
            }
            while (--lEnd > 0)
            {
                //
                // Is this a visible child?
                // CWO, 4/22/97, removed separate clause
				//
                if (GetItemData(lEnd, &tb) && !(tb.fsState & TBSTATE_HIDDEN))
                    break; // out of while
            }
            if (lEnd == CHILDID_SELF)
            {
            HWND    hwndChild;

                if (hwndChild = ::GetWindow(m_hwnd,GW_CHILD))
                {
			        pvarEnd->vt=VT_DISPATCH;
                    return (AccessibleObjectFromWindow(hwndChild,OBJID_WINDOW,
                        IID_IDispatch, (void**)&pvarEnd->pdispVal));
                }
            }
            break; // out of switch

        case NAVDIR_UP:
        case NAVDIR_DOWN:
            lEnd = 0;
            // Don't handle wrapping toolbars yet.
            break; // out of switch
    }

    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
        return(S_OK);
    }

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CToolBar32::accHitTest()
//
//  First, ask the client window what is here.  If itself, then try the
//  buttons.  If nothing/child window, return that thing.
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    POINT   pt;
    LPRECT  lprcShared;
    int     iButton;
    HRESULT hr;
    RECT    rcLocal;
    HANDLE  hProcess;

    SetupChildren();

    //
    // Is this point in our client, not in any child window?
    //
    hr = CClient::accHitTest(x, y, pvarHit);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarHit->vt != VT_I4) || (pvarHit->lVal != 0))
        return(hr);

    pt.x = x;
    pt.y = y;
    ScreenToClient(m_hwnd, &pt);

    //
    // Figure out what button this point is over.  We have to do this the
    // SUCKY way, by looping through the buttons asking for location.
    //
    lprcShared = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprcShared)
        return(E_OUTOFMEMORY);

    for (iButton = 0; iButton < m_cChildren; iButton++)
    {
        if (SendMessage(m_hwnd, TB_GETITEMRECT, iButton, (LPARAM)lprcShared))
        {
            SharedRead (lprcShared,&rcLocal,sizeof(RECT),hProcess);
            if (PtInRect(&rcLocal, pt))
            {
                pvarHit->vt = VT_I4;
                pvarHit->lVal = iButton+1;

                SharedFree(lprcShared,hProcess);
                return(S_OK);
            }
        }
    }

    //
    // If we got here, the point is not over any toolbar item.  It must be
    // over ourself.
    //

    SharedFree(lprcShared,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CToolBar32::accDoDefaultAction()
//
//  This sends the command that the button represents.  We can't fake a click
//  because that won't work if the window isn't active.  
//
//  We have to send a WM_COMMAND, BN_CLICKED to the toolbar parent.  Problem
//  is, no easy way to get the parent.  So we set it (which returns the old
//  one, then set it back).
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::accDoDefaultAction(VARIANT varChild)
{
    HWND    hwndToolBarParent;
    TBBUTTON tb;

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::accDoDefaultAction(varChild));

    //
    // Get the toolbar parent in a hacky way, by setting it then setting
    // it back.  THIS CODE ASSUMES THAT THE HANDLING IS MINIMAL IN COMCTL32.
    //
    hwndToolBarParent = (HWND)SendMessage(m_hwnd, TB_SETPARENT, 0, 0);
    SendMessage(m_hwnd, TB_SETPARENT, (WPARAM)hwndToolBarParent, 0);

    if (! hwndToolBarParent)
        return(S_FALSE);

    //
    // Get the command ID of this button, and generate a BN_CLICK if it
    // isn't a separator.
    //
    if (GetItemData(varChild.lVal, &tb) &&
        !(tb.fsStyle & TBSTYLE_SEP) &&
        (tb.fsState & TBSTATE_ENABLED) &&
        !(tb.fsState & TBSTATE_HIDDEN))
    {
        PostMessage(hwndToolBarParent, WM_COMMAND, MAKEWPARAM(tb.idCommand, BN_CLICKED), (LPARAM)m_hwnd);
        return(S_OK);
    }
    else
        return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CToolBar32::Next()
//
//  This knows that the first child might be an HWND.
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolBar32::Next(ULONG celt, VARIANT *rgvar, ULONG* pceltFetched)
{
HWND        hwndChild;
VARIANT*    pvar;
long        cFetched;
HRESULT     hr;
long        iCur;
long        cChildTemp;

    SetupChildren();

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    cFetched = 0;

    // check for window handle child first
    if (m_idChildCur == CHILDID_SELF)
    {
        if (hwndChild = ::GetWindow(m_hwnd,GW_CHILD))
        {
			rgvar->vt=VT_DISPATCH;
            hr = AccessibleObjectFromWindow(hwndChild,OBJID_WINDOW,
                IID_IDispatch, (void**)&rgvar->pdispVal);

            if (!SUCCEEDED(hr))
                return(hr);

            // decrement how many left to get
            celt--;
            cFetched = 1;
            // increment to next variant in array
            rgvar++;

            // increment count of fetched
            if (pceltFetched)
                (*pceltFetched)++;

            // remember current child
            m_idChildCur = HWNDIDFromHwnd(hwndChild);
            
            // if no more to get, return
            if (!celt)
                return(S_OK);
        } // end if there is a child window
    } // end if (started at 0)


    // now get any non-window children
    pvar = rgvar;
    iCur = m_idChildCur;
    if (IsHWNDID(iCur))
        iCur = 0;

    //
    // Loop through our items. Need to do different if there is a 
    // window child, because m_cChildren will be +1.
    //
    cChildTemp = m_cChildren;
    if (::GetWindow(m_hwnd,GW_CHILD))
        cChildTemp--;
    while ((cFetched < (long)celt) && (iCur < cChildTemp))
    {
        cFetched++;
        iCur++;

        //
        // Note this gives us (index)+1 because we incremented iCur
        //
        pvar->vt = VT_I4;
        pvar->lVal = iCur;
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
        *pceltFetched += cFetched;
    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}


