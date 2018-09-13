// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CLIENT.CPP
//
//  Window client class.
//
//  This handles navigation to other frame elements, and does its best
//  to manage the client area.  We recognize special classes, like listboxes,
//  and those have their own classes to do stuff.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"


#define CH_PREFIX       ((TCHAR)'&')
#define CCH_WINDOW_SHORTCUTMAX  32
#define CCH_SHORTCUT            16


extern HRESULT  DirNavigate(HWND, long, VARIANT *);


// --------------------------------------------------------------------------
//
//  CreateClientObject()
//
//  EXTERNAL function for CreatStdOle...
//
// --------------------------------------------------------------------------
HRESULT CreateClientObject(HWND hwnd, long idObject, REFIID riid, void** ppvObject)
{
    UNUSED(idObject);

    InitPv(ppvObject);

    if (!IsWindow(hwnd))
        return(E_FAIL);

    // Look for (and create) a suitable proxy/handler if one
    // exists. Use CreateClient as default if none found.
    // (FALSE => use client, as opposed to window, classes)
    return FindAndCreateWindowClass( hwnd, FALSE, CreateClient,
                                     riid, 0, ppvObject );
}



// --------------------------------------------------------------------------
//
//  CreateClient()
//
//  INTERNAL function for CreateClientObject() and ::Clone()
//
// --------------------------------------------------------------------------
HRESULT CreateClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvObject)
{
    CClient * pclient;
    HRESULT hr;

    pclient = new CClient();
    if (!pclient)
        return(E_OUTOFMEMORY);

    pclient->Initialize(hwnd, idChildCur);

    hr = pclient->QueryInterface(riid, ppvObject);
    if (!SUCCEEDED(hr))
        delete pclient;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CClient::Initialize()
//
// --------------------------------------------------------------------------
void CClient::Initialize(HWND hwnd, long idChildCur)
{
    m_hwnd = hwnd;
    m_idChildCur = idChildCur;

    // If this is a comboex32, we want to pick up a preceeding
    // label, if one exists (just like we do for regular combos -
    // which set m_fUseLabel to TRUE in their own ::Initialize().
    // The combo will ask the parent comboex32 for its name, and
    // it in turn will look for a label.
    if( IsComboEx( m_hwnd ) )
    {
        m_fUseLabel = TRUE;
    }
}



// --------------------------------------------------------------------------
//
//  CClient::ValidateHwnd()
//
//  This will validate VARIANTs for both HWND-children clients and normal
//  clients.   If m_cChildren is non-zero, 
//
// --------------------------------------------------------------------------
BOOL CClient::ValidateHwnd(VARIANT* pvar)
{
    HWND    hwndChild;
    switch (pvar->vt)
    {
        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

#ifdef VT_I2_IS_VALID      // It should not be valid. That's why this is removed.
        case VT_I2:
            pvar->vt = VT_I4;
            pvar->lVal = (long)pvar->iVal;
            // FALL THROUGH
#endif

        case VT_I4:
            if (pvar->lVal == 0)
                break;

            hwndChild = HwndFromHWNDID(pvar->lVal);

            // This works for top-level AND child windows
            if (MyGetAncestor(hwndChild, GA_PARENT) != m_hwnd)
                return(FALSE);
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}




// --------------------------------------------------------------------------
//
//  CClient::get_accChildCount()
//
//  This handles both non-HWND and HWND children.
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::get_accChildCount(long *pcCount)
{
    HWND    hwndChild;
    HRESULT hr;

    hr = CAccessible::get_accChildCount(pcCount);
    if (!SUCCEEDED(hr))
        return(hr);

    for (hwndChild = ::GetWindow(m_hwnd, GW_CHILD); hwndChild; hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT))
        ++(*pcCount);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::get_accName(VARIANT varChild, BSTR *pszName)
{
    InitPv(pszName);

    //
    // Validate--this does NOT accept a child ID.
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrGetWindowName(m_hwnd, m_fUseLabel, pszName));
}



// --------------------------------------------------------------------------
//
//  CClient::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate--this does NOT accept a child ID.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_CLIENT;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    WINDOWINFO wi;
    HWND       hwndActive;

    InitPvar(pvarState);

    //
    // Validate--this does NOT accept a child ID.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    //
    // Are we the focus?  Are we enabled, visible, etc?
    //
    if (!MyGetWindowInfo(m_hwnd, &wi))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }

    if (!(wi.dwStyle & WS_VISIBLE))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    if (wi.dwStyle & WS_DISABLED)
        pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;

    if (MyGetFocus() == m_hwnd)
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;

    hwndActive = GetForegroundWindow();

    if (hwndActive == MyGetAncestor(m_hwnd, GA_ROOT))
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    //
    // Validate--this does NOT accept a child ID
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    // reject child elements - shortcut key only applies to the overall
    // control.
    if ( varChild.lVal != 0 )
        return(E_NOT_APPLICABLE);

    return(HrGetWindowShortcut(m_hwnd, m_fUseLabel, pszShortcut));
}


// --------------------------------------------------------------------------
//
//  CClient::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::get_accFocus(VARIANT *pvarFocus)
{
    HWND    hwndFocus;

    InitPvar(pvarFocus);

    //
    // This RETURNS a child ID.
    //
    hwndFocus = MyGetFocus();

    //
    // Is the current focus a child of us?
    //
    if (m_hwnd == hwndFocus)
    {
        pvarFocus->vt = VT_I4;
        pvarFocus->lVal = 0;
    }
    else if (IsChild(m_hwnd, hwndFocus))
        return(GetWindowObject(hwndFocus, pvarFocus));

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    RECT    rc;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate--this does NOT take a child ID
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    MyGetRect(m_hwnd, &rc, FALSE);
    MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rc, 2);

    *pxLeft = rc.left;
    *pyTop = rc.top;
    *pcxWidth = rc.right - rc.left;
    *pcyHeight = rc.bottom - rc.top;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::accNavigate(long dwNavDir, VARIANT varStart, VARIANT * pvarEnd)
{
    HWND    hwndChild;
    int     gww;

    InitPvar(pvarEnd);

    //
    // Validate--this accepts an HWND id.
    //
    if (!ValidateHwnd(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir == NAVDIR_FIRSTCHILD)
    {
        gww = GW_HWNDNEXT;
        hwndChild = ::GetWindow(m_hwnd, GW_CHILD);
        if (!hwndChild)
            return(S_FALSE);

        goto NextPrevChild;
    }
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        gww = GW_HWNDPREV;

        hwndChild = ::GetWindow(m_hwnd, GW_CHILD);
        if (!hwndChild)
            return(S_FALSE);

        // Start at the end and work backwards
        hwndChild = ::GetWindow(hwndChild, GW_HWNDLAST);

        goto NextPrevChild;
    }
    else if (!varStart.lVal)
        return(GetParentToNavigate(OBJID_CLIENT, m_hwnd, OBJID_WINDOW,
            dwNavDir, pvarEnd));

    hwndChild = HwndFromHWNDID(varStart.lVal);

    if ((dwNavDir == NAVDIR_NEXT) || (dwNavDir == NAVDIR_PREVIOUS))
    {
        gww = ((dwNavDir == NAVDIR_NEXT) ? GW_HWNDNEXT : GW_HWNDPREV);

        while (hwndChild = ::GetWindow(hwndChild, gww))
        {
NextPrevChild:
            if (IsWindowVisible(hwndChild))
                return(GetWindowObject(hwndChild, pvarEnd));
        }
    }
    else
        return(DirNavigate(hwndChild, dwNavDir, pvarEnd));

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CClient::accHitTest()
//
//  This ALWAYS returns a real object.
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::accHitTest(long xLeft, long yTop, VARIANT *pvarHit)
{
    HWND    hwndChild;
    POINT   pt;

    InitPvar(pvarHit);

    pt.x = xLeft;
    pt.y = yTop;
    ScreenToClient(m_hwnd, &pt);

    hwndChild = MyRealChildWindowFromPoint(m_hwnd, pt);
    if (hwndChild)
    {
        if (hwndChild == m_hwnd)
        {
            pvarHit->vt = VT_I4;
            pvarHit->lVal = 0;
            return(S_OK);
        }
        else
            return(GetWindowObject(hwndChild, pvarHit));
    }
    else
    {
        // Null window means point isn't in us at all...
        return(S_FALSE);
    }
}



// --------------------------------------------------------------------------
//
//  CClient::Next()
//
//  This loops through non-HWND children first, then HWND children.
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::Next(ULONG celt, VARIANT *rgvar, ULONG* pceltFetched)
{
    HWND    hwndChild;
    VARIANT* pvar;
    long    cFetched;
    HRESULT hr;

    SetupChildren();

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    // Grab the non-HWND dudes first.
    if (!IsHWNDID(m_idChildCur) && (m_idChildCur < m_cChildren))
    {
        cFetched = 0;

        hr = CAccessible::Next(celt, rgvar, (ULONG*)&cFetched);
        if (!SUCCEEDED(hr))
            return(hr);

        celt -= cFetched;
        rgvar += cFetched;

        if (pceltFetched)
            *pceltFetched += cFetched;

        if (!celt)
            return(S_OK);
    }


    pvar = rgvar;
    cFetched = 0;

    if (!IsHWNDID(m_idChildCur))
    {
        Assert(m_idChildCur == m_cChildren);
        hwndChild = ::GetWindow(m_hwnd, GW_CHILD);
    }
    else
    {
        hwndChild = HwndFromHWNDID(m_idChildCur);
    }

    //
    // Loop through our HWND children now
    //
    while (hwndChild && (cFetched < (long)celt))
    {
        hr = GetWindowObject(hwndChild, pvar);
        if (SUCCEEDED(hr))
        {
            ++pvar;
            ++cFetched;
        }
        else
        {
            //
            // Clear everything on failure.
            //
            pvar = rgvar;
            for (cFetched = 0; cFetched < (long)celt; cFetched++, pvar++)
                VariantClear(pvar);

            if (pceltFetched)
                *pceltFetched = 0;

            return(hr);
         }

        hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
    }

    //
    // Advance the current position
    //
    m_idChildCur = HWNDIDFromHwnd(hwndChild);

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



// --------------------------------------------------------------------------
//
//  CClient::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::Skip(ULONG celt)
{
    HWND    hwndT;

    SetupChildren();

    // Skip non-HWND items
    if (!IsHWNDID(m_idChildCur) && (m_idChildCur < m_cChildren))
    {
        long    dAway;

        dAway = m_cChildren - m_idChildCur;
        if (celt >= (DWORD)dAway)
        {
            celt -= dAway;
            m_idChildCur = m_cChildren;
        }
        else
        {
            m_idChildCur += celt;
            return(S_OK);
        }
    }

    // Skip the HWND children next
    if (!IsHWNDID(m_idChildCur))
    {
        Assert(m_idChildCur == m_cChildren);
        hwndT = ::GetWindow(m_hwnd, GW_CHILD);
    }
    else
        hwndT = HwndFromHWNDID(m_idChildCur);

    while (hwndT && (celt-- > 0))
    {
        hwndT = ::GetWindow(hwndT, GW_HWNDNEXT);
    }

    m_idChildCur = HWNDIDFromHwnd(hwndT);

    return(celt ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CClient::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CClient::Clone(IEnumVARIANT** ppenum)
{
    InitPv(ppenum);

    // Look for (and create) a suitable proxy/handler if one
    // exists. Use CreateClient as default if none found.
    // (FALSE => use client, as opposed to window, classes)
    return FindAndCreateWindowClass( m_hwnd, FALSE, CreateClient,
                  IID_IEnumVARIANT, m_idChildCur, (void **)ppenum );
}




// --------------------------------------------------------------------------
//
//  GetTextString()
//
//  Parameters: hwnd of the window to get the text from, and a boolean
//  that indicates whether or not we should always allocate memory to
//  return. I.E., if the window says the size of the text is 0, and
//  fAllocIfEmpty is TRUE, then we'll still allocate 1 byte (size+1).
//
// This contains a bit of a hack. The way it was originally written, this
// will try to get the ENTIRE text of say, a RichEdit control, even if that
// document is HUGE. Eventually we want to support that, but we are going to 
// need to do better than LocalAlloc. With a big document, we would page
// fault sometimes, because even though the memory is allocated, it
// may not be able to be paged in. JeffBog suggested that the way to
// check is to try to read/write both ends of the allocated space, and
// assume that if that works everything in between is OK too.
//
// So here's the temporary hack (BOGUS!)
// I am putting an artificial limit of 4096 bytes on the allocation.
// I am also going to do IsBadWritePtr on the thing, instead of just 
// checking if the pointer returned by alloc is null. duh.
// --------------------------------------------------------------------------
LPTSTR GetTextString(HWND hwnd, BOOL fAllocIfEmpty)
{
    UINT    cchText;
    LPTSTR  lpText;
#define MAX_TEXT_SIZE   4096

    //
    // Look for a name property!
    //

    lpText = NULL;

    if (!IsWindow(hwnd))
        return (NULL);
    //
    // Barring that, use window text.
    // BOGUS!  Strip out the '&'.
    //
    cchText = SendMessageINT(hwnd, WM_GETTEXTLENGTH, 0, 0);

    // hack
    cchText = (cchText > MAX_TEXT_SIZE ? MAX_TEXT_SIZE : cchText);

    // Allocate a buffer
    if (cchText || fAllocIfEmpty)
    {
        lpText = (LPTSTR)LocalAlloc(LPTR, (cchText+1)*sizeof(TCHAR));
        if (IsBadWritePtr (lpText,cchText+1))
            return(NULL);


        if (cchText)
            SendMessage(hwnd, WM_GETTEXT, cchText+1, (LPARAM)lpText);
    }

    return(lpText);
}



// --------------------------------------------------------------------------
//
//  GetLabelString()
//
//  This walks backwards among peer windows to find a static field.  It stops
//  if it gets to the front or hits a group/tabstop, just like the dialog 
//  manager does.
//
// --------------------------------------------------------------------------
LPTSTR GetLabelString(HWND hwnd)
{
    HWND    hwndLabel;
    LONG    lStyle;
    LRESULT lResult;
    LPTSTR  lpszLabel;

    lpszLabel = NULL;

    if (!IsWindow(hwnd))
        return (NULL);

    hwndLabel = hwnd;

    while (hwndLabel = ::GetWindow(hwndLabel, GW_HWNDPREV))
    {
        lStyle = GetWindowLong(hwndLabel, GWL_STYLE);

        //
        // Is this a static dude?
        //
        lResult = SendMessage(hwndLabel, WM_GETDLGCODE, 0, 0L);
        if (lResult & DLGC_STATIC)
        {
            //
            // Great, we've found our label.
            //
            lpszLabel = GetTextString(hwndLabel, FALSE);
            break;
        }


        //
        // Skip if invisible
        // Note that we do this after checking if its a staic,
        // so that we give invisible statics a chance. Using invisible
        // statics is a easy workaround to add names to controls
        // without changing the visual UI.
        //
        if (!(lStyle & WS_VISIBLE))
            continue;

        
        //
        // Is this a tabstop or group?  If so, bail out now.
        //
        if (lStyle & (WS_GROUP | WS_TABSTOP))
            break;
    }

    return(lpszLabel);
}



// --------------------------------------------------------------------------
//
//  HrGetWindowName()
//
// --------------------------------------------------------------------------
HRESULT HrGetWindowName(HWND hwnd, BOOL fLookForLabel, BSTR* pszName)
{
    LPTSTR  lpText;

    lpText = NULL;
    if (!IsWindow(hwnd))
        return (E_INVALIDARG);

    //
    // Look for a name property!
    //

    //
    // If use a label, do that instead
    //
    if (!fLookForLabel)
    {
        // 
        // Try using a label anyway if this control has no window text
        // and the parent is a dialog.
        //
        lpText = GetTextString(hwnd, FALSE);
        if (!lpText)
        {
            HWND hwndParent = MyGetAncestor( hwnd, GA_PARENT );
            if( hwndParent && CompareWindowClass( hwndParent, CreateDialogClient ) )
            {
                fLookForLabel = TRUE;
            }
        }
    }

    if (fLookForLabel)
        lpText = GetLabelString(hwnd);

    if (! lpText)
        return(S_FALSE);

    //
    // Strip out the mnemonic.
    //
    StripMnemonic(lpText);

    // Get a BSTR
    *pszName = TCharSysAllocString(lpText);

    // Free our buffer
    LocalFree((HANDLE)lpText);

    // Did the BSTR succeed?
    if (! *pszName)
        return(E_OUTOFMEMORY);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  HrGetWindowShortcut()
//
// --------------------------------------------------------------------------
HRESULT HrGetWindowShortcut(HWND hwnd, BOOL fUseLabel, BSTR* pszShortcut)
{
    //
    // Get the window text, and see if the '&' character is in it.
    //
    LPTSTR  lpText;
    TCHAR   chMnemonic;

    if (!IsWindow(hwnd))
        return (E_INVALIDARG);

    lpText = NULL;

    if (! fUseLabel)
    {
        //
        // Try using a label anyway if this control has no window text
        // and the parent is a dialog.
        //
        lpText = GetTextString(hwnd, FALSE);
        if (!lpText)
        {
            HWND  hwndParent = MyGetAncestor( hwnd, GA_PARENT );
            if( hwndParent && CompareWindowClass( hwndParent, CreateDialogClient ) )
            {
                fUseLabel = TRUE;
            }
        }
    }

    if (fUseLabel)
        lpText = GetLabelString(hwnd);

    if (! lpText)
        return(S_FALSE);

    chMnemonic = StripMnemonic(lpText);
    LocalFree((HANDLE)lpText);

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
//  HrMakeShortcut()
//
//  This takes a string for the hotkey, then combines it with the "Alt+%s"
//  shortcut format to make the real string combination.  If asked, it will
//  free the hotkey string passed in.
//
// --------------------------------------------------------------------------
HRESULT HrMakeShortcut(LPTSTR lpszKey, BSTR* pszShortcut)
{
    TCHAR   szFormat[CCH_SHORTCUT];
    TCHAR   szResult[CCH_WINDOW_SHORTCUTMAX];

    // Get the format string
    LoadString(hinstResDll, STR_MENU_SHORTCUT_FORMAT, szFormat,
        ARRAYSIZE(szFormat));

    // Make the result
    wsprintf(szResult, szFormat, lpszKey);

    // Alloc a BSTR of the result
    *pszShortcut = TCharSysAllocString(szResult);

    // Should we free the key string?
    // Did the allocation fail?
    if (!*pszShortcut)
        return(E_OUTOFMEMORY);
    else
        return(S_OK);
}



// 'Slide' string along by one char, in-place, to effectively remove the
// char pointed to be pStr.
// eg. if pStr points to the 'd' of 'abcdefg', the string
// will be transformed to 'abcefg'.
// Note: the char pointed to by pStr is assumed to be a single-byte
// char (if compiled under ANSI - not an issue if compiled inder UNICODE)
// Note: Makes use of the fact that no DBCS char has NUL as the trail byte.
void SlideStrAndRemoveChar( LPTSTR pStr )
{
    LPTSTR pLead = pStr + 1;
    // Checking the trailing pStr ptr means that we continue until we've
    // copied (not just encountered) the terminating NUL.
    while( *pStr )
        *pStr++ = *pLead++;
}



// --------------------------------------------------------------------------
//
//  StripMnemonic()
//
//  This removes the mnemonic prefix.  However, if we see '&&', we keep
//  one '&'.
//
//
//  Modified to be DBCS 'aware' - uses CharNext() instead of ptr++ to
//  advance through the string. Will only return shortcut char if its a 
//  single byte char, though. (Would have to change all usages of this
//  function to allow return of a potentially DBCS char.) Will remove this
//  restriction in the planned-for-future fully-UNICODE OLEACC.
//  This restriction should not be much of a problem, because DBCS chars,
//  which typically require an IME to compose, are very unlikely to be
//  used as 'shortcut' chars. eg. Japanese Windows uses underlined roman
//  chars as shortcut chars.
//  (This will all be replaced by simpler code when we go UNICODE...)
// --------------------------------------------------------------------------
TCHAR StripMnemonic(LPTSTR lpszText)
{
    TCHAR   ch;
    TCHAR   chNext = 0;

    while( *lpszText == (TCHAR)' ' )
        lpszText = CharNext( lpszText );

    while( ch = *lpszText )
    {
        lpszText = CharNext( lpszText );

        if (ch == CH_PREFIX)
        {
            // Get the next character.
            chNext = *lpszText;

            // If it too is '&', then this isn't a mnemonic, it's the 
            // actual '&' character.  
            if (chNext == CH_PREFIX)
                chNext = 0;
            
            // Skip 'n' strip the '&' character
            SlideStrAndRemoveChar( lpszText - 1 );

#ifdef UNICODE
            CharLowerBuff(&chNext, 1);
#else
            if( IsDBCSLeadByte( chNext ) )
            {
                // We're ignoring DBCS chars as shortcut chars
                // - would need to change this func and all callers
                // to handle a returned DB char otherwise.
                // For the moment, we just ensure we don't return
                // an 'orphaned' lead byte...
                chNext = '\0';
            }
            else
            {
                CharLowerBuff(&chNext, 1);
            }
#endif
            break;
        }
    }

    return(chNext);
}



// --------------------------------------------------------------------------
//
//  DirNavigate()
//
//  Figures out which peer window is closest to us in the given direction.
//
// --------------------------------------------------------------------------
HRESULT DirNavigate(HWND hwndSelf, long dwNavDir, VARIANT* pvarEnd)
{
    HWND    hwndPeer;
    RECT    rcSelf;
    RECT    rcPeer;
    int     dwClosest;
    int     dwT;
    HWND    hwndClosest;

    if (!IsWindow(hwndSelf))
        return (E_INVALIDARG);

    MyGetRect(hwndSelf, &rcSelf, TRUE);

    dwClosest = 0x7FFFFFFF;
    hwndClosest = NULL;

    for (hwndPeer = ::GetWindow(hwndSelf, GW_HWNDFIRST); hwndPeer;
         hwndPeer = ::GetWindow(hwndPeer, GW_HWNDNEXT))
    {
        if ((hwndPeer == hwndSelf) || !IsWindowVisible(hwndPeer))
            continue;

        MyGetRect(hwndPeer, &rcPeer, TRUE);

        dwT = 0x7FFFFFFF;

        switch (dwNavDir)
        {
            case NAVDIR_LEFT:
                //
                // Bogus!  Only try this one if it intersects us vertically
                //
                if (rcPeer.left < rcSelf.left)
                    dwT = rcSelf.left - rcPeer.left;
                break;

            case NAVDIR_UP:
                //
                // Bogus!  Only try this one if it intersects us horizontally
                //
                if (rcPeer.top < rcSelf.top)
                    dwT = rcSelf.top - rcPeer.top;
                break;

            case NAVDIR_RIGHT:
                //
                // Bogus!  Only try this one if it intersects us vertically
                //
                if (rcPeer.right > rcSelf.right)
                    dwT = rcPeer.right - rcSelf.right;
                break;

            case NAVDIR_DOWN:
                //
                // Bogus!  Only try this one if it intersects us horizontally
                //
                if (rcPeer.bottom > rcSelf.bottom)
                    dwT = rcPeer.bottom - rcSelf.bottom;
                break;

            default:
                AssertStr( "INVALID NAVDIR" );
        }

        if (dwT < dwClosest)
        {
            dwClosest = dwT;
            hwndClosest = hwndPeer;
        }
    }

    if (hwndClosest)
        return(GetWindowObject(hwndClosest, pvarEnd));
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  InTheShell()
//
//  Returns TRUE if the object is on the shell tray, desktop, or process.
//
// --------------------------------------------------------------------------
BOOL InTheShell(HWND hwnd, int nPart)
{
    HWND    hwndShell;
    static  TCHAR szShellTray[] = TEXT("Shell_TrayWnd");
    DWORD   idProcessUs;
    DWORD   idProcessShell;

    hwndShell = GetShellWindow();

    switch (nPart)
    {
        case SHELL_TRAY:
            // Use the tray window instead.
            hwndShell = FindWindowEx(NULL, NULL, szShellTray, NULL);
            // Fall thru

        case SHELL_DESKTOP:
            if (!hwndShell)
                return(FALSE);
            return(MyGetAncestor(hwnd, GA_ROOT) == hwndShell);

        case SHELL_PROCESS:
            idProcessUs = NULL;
            idProcessShell = NULL;
            GetWindowThreadProcessId(hwnd, &idProcessUs);
            GetWindowThreadProcessId(hwndShell, &idProcessShell);
            return(idProcessUs && (idProcessUs == idProcessShell));
    }

    AssertStr( "GetShellWindow returned strange part" );
    return(FALSE);
}
