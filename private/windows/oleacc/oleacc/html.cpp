// Copyright (c) 1996-1999 Microsoft Corporation


// Note - this file is not used on non-x86 builds.
// Since we can't easily conditionally ifdef it out at the sources/makefile
// level, we instead ifdef it out here.
#ifdef _X86_



// --------------------------------------------------------------------------
//
//  HTML.CPP
//
//  Wrapper for the HTML control.  It knows how to expose links, graphics,
//  and tables.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "html.h"

#ifdef _DEBUG
void PrintAXAInfo (LPAXAINFO lpaxaShared);
#define PRINTCHILD  PrintAXAInfo
#else
#define PRINTCHILD  1 ? (void)0 : (void)
#endif

// --------------------------------------------------------------------------
//
//  CreateHtmlClient()
//
// --------------------------------------------------------------------------
HRESULT CreateHtmlClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvHtml)
{
    CHtml32*    phtml;
    HRESULT     hr;

    InitPv(ppvHtml);

    phtml = new CHtml32(hwnd, idChildCur);
    if (!phtml)
        return(E_OUTOFMEMORY);

    hr = phtml->QueryInterface(riid, ppvHtml);
    if (!SUCCEEDED(hr))
        delete phtml;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CHtml32::CHtml32()
//
// --------------------------------------------------------------------------
CHtml32::CHtml32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = FALSE;
}



// --------------------------------------------------------------------------
//
//  CHtml32::SetupChildren()
//
// --------------------------------------------------------------------------
void CHtml32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, WM_HTML_GETAXACHILDCOUNT, 0, 0L);
}



// --------------------------------------------------------------------------
//
//  CHtml32::ValidateChild()
//
//  We do something special; only the LOW 24 bits of an index are the top
//  level element index in the HTML stream.  The HIGH 8 bits are the image
//  map index (for image maps).
//
//  Sometimes we are also given an hwnd as a child (for form elements) and
//  we need to validate that, but change it from an hwnd to an index. 
//
// --------------------------------------------------------------------------
BOOL CHtml32::ValidateChild(VARIANT* pvar)
{
    long    lRealVal;
    long    lMapVal;
    long    i;
    AXAINFO axa;

    while (pvar->vt == (VT_BYREF | VT_VARIANT))
        VariantCopy(pvar, pvar->pvarVal);

    if (pvar->vt != VT_I4)
        return(CAccessible::ValidateChild(pvar));

    SetupChildren();

    //
    // Is this a valid HTML item?
    // (just grab the LOW 24 bits)
    lRealVal = GET_AXA_ELEINDEX(pvar->lVal);
    if ((lRealVal < 0) || (lRealVal > m_cChildren))
    {
        if (IsWindow((HWND)lRealVal))
        {
            // This looks through all the elements until we find
            // the one that matches the window handle
            for (i=1; i <= m_cChildren; i++)
            {
                GetAxaInfo (i,&axa);
                if (axa.hwndEle == (HWND)lRealVal)
                {
                    pvar->lVal = i;
                    return (TRUE);
                }
            }
        }
        return(FALSE);
    }

    lMapVal = GET_AXA_MAPINDEX(pvar->lVal);
    if (lMapVal)
    {
        long    lMapItems;

        //
        // Get the number of items in the map.
        //
        lMapItems = SendMessageINT(m_hwnd, WM_HTML_GETAXACHILDCOUNT, lRealVal, 0);
        if ((lMapVal < 0) || (lMapVal > lMapItems))
            return(FALSE);
    }

    return(TRUE);

}


// --------------------------------------------------------------------------
//
//  CHtml32::GetAxaInfo()
//
//  Gets AXAINFO for a particular HTML item
//
// --------------------------------------------------------------------------
BOOL CHtml32::GetAxaInfo(long idChild, LPAXAINFO lpaxa)
{
LPAXAINFO   lpaxaShared;
BOOL        fReturn;
HANDLE      hProcess;
DWORD       dwSomething;    // since we can't just do assignment...

    fReturn = FALSE;

    lpaxaShared = (LPAXAINFO)SharedAlloc(sizeof(AXAINFO),m_hwnd,&hProcess);
    if (lpaxaShared)
    {
        //lpaxaShared->cbSize = sizeof(AXAINFO);
        dwSomething = sizeof(AXAINFO);
        SharedWrite (&dwSomething,&lpaxaShared->cbSize,sizeof(DWORD),hProcess);

        if (SendMessage(m_hwnd, WM_HTML_GETAXAINFO, idChild, (LPARAM)lpaxaShared))
        {
            SharedRead (lpaxaShared,lpaxa, sizeof(AXAINFO),hProcess);
            fReturn = TRUE;
        }

        SharedFree(lpaxaShared,hProcess);
    }

    return(fReturn);
}



// --------------------------------------------------------------------------
//
//  CHtml32::IsWhiteSpace()
//
//  Returns TRUE if a child elment of the HTML doc is noise/whitespace, not
//  something real.
//
// --------------------------------------------------------------------------
BOOL CHtml32::IsWhiteSpace(UINT eleType)
{
    switch (eleType)
    {
        case ELE_NOT:
        case ELE_ENDDOC:
        case ELE_VERTICALTAB:
        case ELE_HR:
        case ELE_NEWLINE:
        case ELE_BEGINLIST:
        case ELE_ENDLIST:
        case ELE_INDENT:
        case ELE_OUTDENT:
        case ELE_BEGINFORM:
        case ELE_ENDFORM:
        case ELE_HIDDEN:
        case ELE_TAB:
        case ELE_BEGINBLOCKQUOTE:
        case ELE_ENDBLOCKQUOTE:
        case ELE_PARAM:
        case ELE_EVENT:
        case ELE_STYLESHEET:
        case ELE_ALIAS:
        case ELE_SUBSCRIPT:
        case ELE_SUPSCRIPT:
        case ELE_OPENLISTITEM:
        case ELE_CLOSELISTITEM:
            return(TRUE);
    }

    return(FALSE);
}


// --------------------------------------------------------------------------
//
//  CHtml32::get_accChildCount()
//
//  NOTE:  An HTML control may have children (like buttons, edits, etc.).
//  But we wrap that ourselves, like a combobox wraps its components and
//  doesn't let the default client handler get in.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accChildCount(long* pcCount)
{
    return(CAccessible::get_accChildCount(pcCount));
}



// --------------------------------------------------------------------------
//
//  CHtml32::get_accChild()
//
//  Succeeds for form controls.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accChild(VARIANT varChild, IDispatch** ppdispChild)
{
AXAINFO axaChild;
RECT    rcWindow;

    InitPv(ppdispChild);

    //
    // Validate
    //
    if (!ValidateChild(&varChild) || !varChild.lVal)
        return(E_INVALIDARG);

    if (GET_AXA_MAPINDEX(varChild.lVal))
        return(E_INVALIDARG);

    if (GetAxaInfo(varChild.lVal, &axaChild))
    {
        if (axaChild.hwndEle)
            return(AccessibleObjectFromWindow(axaChild.hwndEle, OBJID_WINDOW,
                IID_IDispatch, (void**)ppdispChild));
        else if (axaChild.lFlags & ELEFLAG_IMAGEMAP)
            return(CreateImageMap(m_hwnd, this, GET_AXA_ELEINDEX(varChild.lVal), 0, IID_IDispatch, (void**)ppdispChild));
        else
        {
        HWND    hwndChild;

            // problem here. With a document that has frames,
            // there are too many children returned. 
            // For each FRAMESET tag, there is 1 child.
            // for each FRAME within the FRAMESET, there are 2
            // children, 1 with ELE_FRAMESET, 1 with ELE_OBJECT. 
            // There is always 1 child that is the End of Document marker.
            // 
            if (axaChild.eleType == ELE_OBJECT)
            {
                // Since each frame is another HTML Window (and
                // therefore an HTML Object), we will create a new
                // one whenever we see ELE_OBJECT. To figure out
                // which window we are talking about, I am just going 
                // to see which child window of this window matches 
                // the coordinates best.
                MapWindowPoints(m_hwnd, NULL, (LPPOINT)&axaChild.rcItem, 2);
                hwndChild = ::GetWindow(m_hwnd,GW_CHILD);
                while (hwndChild)
                {
                    GetWindowRect (hwndChild,&rcWindow);
                    if (EqualRect (&axaChild.rcItem,&rcWindow))
                        return(AccessibleObjectFromWindow(hwndChild, OBJID_WINDOW,
                            IID_IDispatch, (void**)ppdispChild));

                    hwndChild = ::GetWindow(hwndChild,GW_HWNDNEXT);
                }
            }
        }
    }

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CHtml32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accName(VARIANT varChild, BSTR* pszName)
{
LPAXAINFO   lpaxaShared;
LPTSTR      lpNameShared;
LPTSTR      lpNameLocal;
HANDLE      hProcess1;
HANDLE      hProcess2;
DWORD       dwSomething;    // since we can't just do assignment...
UINT        nTextLen;
UINT        nSomeInt;

    InitPv(pszName);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Get the name length first
    //
    lpaxaShared = (LPAXAINFO)SharedAlloc(sizeof(AXAINFO),m_hwnd,&hProcess1);
    if (!lpaxaShared)
        return(E_OUTOFMEMORY);

    //lpaxaShared->cbSize = sizeof(AXAINFO);
    dwSomething = sizeof(AXAINFO);
    SharedWrite (&dwSomething,&lpaxaShared->cbSize,sizeof(DWORD),hProcess1);

    if (SendMessage(m_hwnd, WM_HTML_GETAXAINFO, varChild.lVal, (LPARAM)lpaxaShared))
    {
        SharedRead (&lpaxaShared->cchNameTextMax,&nTextLen,sizeof(UINT),hProcess1);
        if (nTextLen > 0)
        {
            lpNameShared = (LPTSTR)SharedAlloc(sizeof(TCHAR) * (nTextLen+1),m_hwnd,&hProcess2);
            if (lpNameShared)
            {
                lpNameLocal = (LPTSTR)LocalAlloc(LPTR,sizeof(TCHAR) * (nTextLen+1));
                if (!lpNameLocal)
                {
                    SharedFree (lpNameShared,hProcess2);
                    SharedFree (lpaxaShared,hProcess1);
                    return (E_OUTOFMEMORY);
                }
                // *lpNameShared = 0;
                nSomeInt = 0;
                SharedWrite (&nSomeInt,lpNameShared,sizeof(TCHAR),hProcess2);
                //lpaxaShared->lpNameText = lpNameShared;
                SharedWrite (&lpNameShared,&lpaxaShared->lpNameText,sizeof(LPTSTR),hProcess2);
                SendMessage(m_hwnd, WM_HTML_GETAXAINFO, varChild.lVal, (LPARAM)lpaxaShared);

                SharedRead (lpNameShared,lpNameLocal,sizeof(TCHAR) * (nTextLen+1),hProcess2);
                if (*lpNameLocal)
                    *pszName = TCharSysAllocString(lpNameLocal);

                LocalFree (lpNameLocal);
                SharedFree(lpNameShared,hProcess2);
            }
        }
    }

    SharedFree(lpaxaShared,hProcess1);
    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CHtml32::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accValue(VARIANT varChild, BSTR* pszValue)
{
LPAXAINFO   lpaxaShared;
LPTSTR      lpValueShared;
LPTSTR      lpValueLocal;
HANDLE      hProcess1;
HANDLE      hProcess2;
DWORD       dwSomething;    // since we can't just do assignment...
UINT        nTextLen;
UINT        nSomeInt;

    InitPv(pszValue);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Get the Value length first
    //
    lpaxaShared = (LPAXAINFO)SharedAlloc(sizeof(AXAINFO),m_hwnd,&hProcess1);
    if (!lpaxaShared)
        return(E_OUTOFMEMORY);

    //lpaxaShared->cbSize = sizeof(AXAINFO);
    dwSomething = sizeof(AXAINFO);
    SharedWrite (&dwSomething,&lpaxaShared->cbSize,sizeof(DWORD),hProcess1);

    if (SendMessage(m_hwnd, WM_HTML_GETAXAINFO, varChild.lVal, (LPARAM)lpaxaShared))
    {
        SharedRead (&lpaxaShared->cchValueTextMax,&nTextLen,sizeof(UINT),hProcess1);
        if (nTextLen > 0)
        {
            lpValueShared = (LPTSTR)SharedAlloc(sizeof(TCHAR) * (nTextLen+1),m_hwnd,&hProcess2);
            if (lpValueShared)
            {
                lpValueLocal = (LPTSTR)LocalAlloc(LPTR,sizeof(TCHAR) * (nTextLen+1));
                if (!lpValueLocal)
                {
                    SharedFree (lpValueShared,hProcess2);
                    SharedFree (lpaxaShared,hProcess1);
                    return (E_OUTOFMEMORY);
                }
                // *lpValueShared = 0;
                nSomeInt = 0;
                SharedWrite (&nSomeInt,lpValueShared,sizeof(TCHAR),hProcess2);
                //lpaxaShared->lpValueText = lpValueShared;
                SharedWrite (&lpValueShared,&lpaxaShared->lpValueText,sizeof(LPTSTR),hProcess2);
                SendMessage(m_hwnd, WM_HTML_GETAXAINFO, varChild.lVal, (LPARAM)lpaxaShared);

                SharedRead (lpValueShared,lpValueLocal,sizeof(TCHAR) * (nTextLen+1),hProcess2);
                if (*lpValueLocal)
                    *pszValue = TCharSysAllocString(lpValueLocal);

                LocalFree (lpValueLocal);
                SharedFree(lpValueShared,hProcess2);
            }
        }
    }

    SharedFree(lpaxaShared,hProcess1);
    return(*pszValue ? S_OK : S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CHtml32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    AXAINFO axa;

    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
    {
        pvarRole->vt = VT_I4;
        pvarRole->lVal = ROLE_SYSTEM_DOCUMENT;
    }
    else
    {
        if (!GetAxaInfo(varChild.lVal, &axa))
            return(S_FALSE);

        pvarRole->vt = VT_I4;

        switch (axa.eleType)
        {
            case ELE_TEXT:
            case ELE_TEXTAREA:
                pvarRole->lVal = ROLE_SYSTEM_TEXT;
                break;

            default:
                if (IsWhiteSpace(axa.eleType))
                    pvarRole->lVal = ROLE_SYSTEM_WHITESPACE;
                else
                    pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;
                break;

            case ELE_IMAGE:
            case ELE_FORMIMAGE:
                pvarRole->lVal = ROLE_SYSTEM_GRAPHIC;
                break;

            case ELE_LISTITEM:
                pvarRole->lVal = ROLE_SYSTEM_LISTITEM;
                break;

            case ELE_EDIT:
            case ELE_PASSWORD:
                pvarRole->lVal = ROLE_SYSTEM_TEXT;
                break;

            case ELE_CHECKBOX:
                pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
                break;

            case ELE_RADIO:
                pvarRole->lVal = ROLE_SYSTEM_RADIOBUTTON;
                break;

            case ELE_SUBMIT:
            case ELE_FETCH:
            case ELE_BUTTON:
                pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
                break;

            case ELE_COMBO:
                pvarRole->lVal = ROLE_SYSTEM_COMBOBOX;
                break;

            case ELE_LIST:
            case ELE_MULTILIST:
                pvarRole->lVal = ROLE_SYSTEM_LIST;
                break;

            case ELE_MARQUEE:
                pvarRole->lVal = ROLE_SYSTEM_ANIMATION;
                break;

            case ELE_BGSOUND:
                pvarRole->lVal = ROLE_SYSTEM_SOUND;
                break;

            case ELE_OBJECT:
                pvarRole->lVal = ROLE_SYSTEM_PANE;
                break;
            
            case ELE_EMBED:
                pvarRole->lVal = ROLE_SYSTEM_PANE;
                break;
            
            case ELE_FRAMESET:
                pvarRole->lVal = ROLE_SYSTEM_PANE;
                break;

            case ELE_FRAME:
                pvarRole->lVal = ROLE_SYSTEM_TABLE;
                break;
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHtml32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    AXAINFO axa;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (!GetAxaInfo(varChild.lVal, &axa))
    {
        pvarState->lVal = STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_UNAVAILABLE;
        return(S_FALSE);
    }

    if ((axa.lFlags & ELEFLAG_HIDDEN) ||
        (axa.hwndEle && !IsWindowVisible(axa.hwndEle)))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    if (axa.lFlags & ELEFLAG_ANCHOR)
    {
        pvarState->lVal |= STATE_SYSTEM_LINKED;

        if (axa.lFlags & ELEFLAG_VISITED)
            pvarState->lVal |= STATE_SYSTEM_TRAVERSED;

        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        //
        // No URL means the link is currently broken.
        //
        if (!axa.cchValueTextMax)
            pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;
    }
    else if (axa.hwndEle)
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

    if ((axa.eleType == ELE_TEXT) || (axa.eleType == ELE_TEXTAREA))
        pvarState->lVal |= STATE_SYSTEM_READONLY;

    if (axa.fAnimated)
        pvarState->lVal |= STATE_SYSTEM_ANIMATED;

    if (MyGetFocus() != m_hwnd)
        pvarState->lVal &= ~STATE_SYSTEM_FOCUSABLE;
    else if (SendMessage(m_hwnd, WM_HTML_GETAXAFOCUS, 0, 0) == varChild.lVal)
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHtml32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accFocus(VARIANT* pvarFocus)
{
    InitPvar(pvarFocus);

    if (MyGetFocus() == m_hwnd)
    {
        IDispatch*  pdispChild;

        pvarFocus->vt = VT_I4;
        pvarFocus->lVal = SendMessageINT(m_hwnd, WM_HTML_GETAXAFOCUS, 0, 0);

        //
        // Is this an object (image map?)
        //
        if (pvarFocus->lVal)
        {
            pdispChild = NULL;
            get_accChild(*pvarFocus, &pdispChild);

            if (pdispChild)
            {
                pvarFocus->vt = VT_DISPATCH;
                pvarFocus->pdispVal = pdispChild;
            }
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHtml32::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    VARIANT varState;

    InitPv(pszDefAction);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(E_NOT_APPLICABLE);

    //
    // Return "Jump" if this is a link.
    //
    VariantInit(&varState);
    get_accState(varChild, &varState);
    if ((varState.vt != VT_I4) || !(varState.lVal & STATE_SYSTEM_LINKED))
        return(S_FALSE);

    return(HrCreateString(STR_HTML_JUMP, pszDefAction));
}



// --------------------------------------------------------------------------
//
//  CHtml32::accSelect()
//
//  This will try to change the focus to the requested item.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::accSelect(long lFlags, VARIANT varChild)
{
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accSelect(lFlags, varChild));

    if (lFlags != SELFLAG_TAKEFOCUS)
        return(E_NOT_APPLICABLE);


    lFlags = SendMessageINT(m_hwnd, WM_HTML_DOAXAACTION, varChild.lVal, TRUE);
    return(lFlags ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CHtml32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    AXAINFO axa;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    if (GetAxaInfo(varChild.lVal, &axa))
    {
        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&axa.rcItem, 2);
        *pxLeft = axa.rcItem.left;
        *pyTop = axa.rcItem.top;
        *pcxWidth = axa.rcItem.right - axa.rcItem.left;
        *pcyHeight = axa.rcItem.bottom - axa.rcItem.top;

        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CHtml32::accNavigate()
//
//  We only handle FIRSTCHILD, LASTCHILD, NEXT, and PREVIOUS.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    int     iEl;
    AXAINFO axa;

    InitPvar(pvarEnd);

    //
    // Validate
    //
    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if ((dwNavDir < NAVDIR_FIRSTCHILD) && !varStart.lVal)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    if (dwNavDir == NAVDIR_FIRSTCHILD)
    {
        iEl = 1;
        dwNavDir = NAVDIR_NEXT;
        goto TryThisOne;
    }
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        iEl = m_cChildren;
        dwNavDir = NAVDIR_PREVIOUS;
        goto TryThisOne;
    }
    else
    {
        iEl = GET_AXA_ELEINDEX(varStart.lVal);
        if (GET_AXA_MAPINDEX(varStart.lVal))
            return(E_INVALIDARG);
    }

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
        case NAVDIR_PREVIOUS:
            while (iEl > 0)
            {
                iEl = SendMessageINT(m_hwnd, WM_HTML_GETAXANEXTITEM, iEl, (dwNavDir == NAVDIR_NEXT));
TryThisOne:
                if (GetAxaInfo(iEl, &axa) &&
                    !(axa.lFlags & ELEFLAG_HIDDEN) &&
                    !IsWhiteSpace(axa.eleType) &&
                    (!axa.hwndEle || IsWindowVisible(axa.hwndEle)))
                    break;
            }
            break;

        default:
            iEl = 0;
            break;
    }

    if (iEl)
    {
        // 
        // The contents of the axa will have our data.  If this is an image
        // map, return a real object.
        //
        if (axa.lFlags & ELEFLAG_USEMAP)
        {
            pvarEnd->vt = VT_DISPATCH;
            pvarEnd->pdispVal = NULL;
            return(CreateImageMap(m_hwnd, this, iEl, 0, IID_IDispatch,
                (void**)&pvarEnd->pdispVal));
        }

        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = iEl;

        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CHtml32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::accHitTest(long xLeft, long yTop, VARIANT* pvarHit)
{
    POINT   pt;
    RECT    rc;
    HWND    hwndChild;
    long    lResult;
    AXAINFO axa;

    InitPvar(pvarHit);

    //
    // Is the point in our client?
    //
    pt.x = xLeft;
    pt.y = yTop;
    ScreenToClient(m_hwnd, &pt);

    MyGetRect(m_hwnd, &rc, FALSE);
    if (!PtInRect(&rc, pt))
        return(S_FALSE);

    //
    // It is.  Is a child window underneath this point?
    //
    hwndChild = MyRealChildWindowFromPoint(m_hwnd, pt);
    if (hwndChild && (hwndChild != m_hwnd))
        return(GetWindowObject(hwndChild, pvarHit));

    //
    // The HTML window is here, return any subobjects (links, graphics, etc.)
    // that may be at this location.
    //
    lResult = SendMessageINT(m_hwnd, WM_HTML_GETAXAHITTEST, 0, MAKELPARAM(pt.x, pt.y));
    lResult = GET_AXA_ELEINDEX(lResult);
    if (GetAxaInfo(lResult, &axa) && (axa.lFlags & ELEFLAG_USEMAP))
    {
        pvarHit->vt = VT_DISPATCH;
        pvarHit->pdispVal = NULL;
        return(CreateImageMap(m_hwnd, this, lResult, 0, IID_IDispatch,
            (void**)&pvarHit->pdispVal));

    }

    pvarHit->vt = VT_I4;
    pvarHit->lVal = lResult;
    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  CHtml32::accDoDefaultAction()
//
//  This will jump to the link specified by the child 
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::accDoDefaultAction(VARIANT varChild)
{
    VARIANT varState;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(E_NOT_APPLICABLE);

    VariantInit(&varState);
    get_accState(varChild, &varState);
    if ((varState.vt != VT_I4) || !(varState.lVal & STATE_SYSTEM_LINKED))
        return(S_FALSE);

    //
    // Tell the HTML control to navigate.  We MUST do this via Post, or
    // it can fail since RPC will not do anything when in the middle of a
    // SendMessage().
    //
    PostMessage(m_hwnd, WM_HTML_DOAXAACTION, varChild.lVal, FALSE);
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHtml32::Next()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::Next(ULONG celt, VARIANT* rgvar, ULONG* pceltFetched)
{
    return(CAccessible::Next(celt, rgvar, pceltFetched));
}



// --------------------------------------------------------------------------
//
//  CHtml32::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::Skip(ULONG celt)
{
    return(CAccessible::Skip(celt));
}



// --------------------------------------------------------------------------
//
//  CHtml32::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHtml32::Reset(void)
{
    return(CAccessible::Reset());
}




/////////////////////////////////////////////////////////////////////////////
//
//  IMAGE MAP
//
//  The children of an image map (the clickable areas) are not in the top
//  level HTML stream.  So I need to treat the map like a container object
//  and expose the children.  The parent HTML control will still proxy
//  requests (except for HitTest) that are for map children.  Kind of like
//  how CWindow() will proxy requests for the frame elements.
//
/////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------------
//
//  CreateImageMap()
//
//  This creates a dummy image map object, for the purpose of hit testing
//  and child enumeration.  All other methods are passed through the parent.
//  Since we bump up the ref count, it's cool.
//
// --------------------------------------------------------------------------
HRESULT CreateImageMap(HWND hwnd, CHtml32* phtmlParent, int iElMap,
    long idChildCur, REFIID riid, void** ppvMap)
{
    CImageMap*  pImageMap;
    HRESULT     hr;

    InitPv(ppvMap);

    pImageMap = new CImageMap(hwnd, phtmlParent, iElMap, idChildCur);
    if (!pImageMap)
        return(E_OUTOFMEMORY);

    hr = pImageMap->QueryInterface(riid, ppvMap);
    if (!SUCCEEDED(hr))
        delete pImageMap;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CImageMap::CImageMap()
//
//  For the lifetime of a map, the # of items is constant.  Hence we only
//  need to initialize m_cChildren once.
//
// --------------------------------------------------------------------------
CImageMap::CImageMap(HWND hwnd, CHtml32* phtmlParent, int iElMap,
    long idChildCur)
{
    m_hwnd = hwnd;

    // Bump up the ref count so we don't go away.
    m_phtmlParent = phtmlParent;
    m_phtmlParent->AddRef();

    m_ielMap = iElMap;
    m_idChildCur = idChildCur;

    m_cChildren = SendMessageINT(m_hwnd, WM_HTML_GETAXACHILDCOUNT, iElMap, 0);
}



// --------------------------------------------------------------------------
//
//  CImageMap::~CImageMap()
//
//  We have to bump the ref count on our parent Html32 object on destruction
//  since we bumped it up on construction.
//
// --------------------------------------------------------------------------
CImageMap::~CImageMap()
{
    m_phtmlParent->Release();
}



// --------------------------------------------------------------------------
//
//  CImageMap::get_accParent()
//
//  This returns our HTML parent
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accParent(IDispatch** ppdispParent)
{
    InitPv(ppdispParent);

    return(m_phtmlParent->QueryInterface(IID_IDispatch, (void**)ppdispParent));
}



// --------------------------------------------------------------------------
//
//  CImageMap::get_accName()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accName(VARIANT varChild, BSTR* pszName)
{
    long    lFunkyIndex;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->get_accName(varChild, pszName));
}



// --------------------------------------------------------------------------
//
//  CImageMap::get_accValue()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    long    lFunkyIndex;

    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->get_accValue(varChild, pszValue));
}



// --------------------------------------------------------------------------
//
//  CImageMap::get_accRole()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    long    lFunkyIndex;

    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->get_accRole(varChild, pvarRole));
}




// --------------------------------------------------------------------------
//
//  CImageMap::get_accState()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    long    lFunkyIndex;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->get_accState(varChild, pvarState));
}



// --------------------------------------------------------------------------
//
//  CImageMap::get_accFocus()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accFocus(VARIANT* pvarFocus)
{
    long    lFunkyIndex;
    HRESULT hr;

    //
    // Ask our parent what item has the focus.
    //
    hr = m_phtmlParent->get_accFocus(pvarFocus);
    if (!SUCCEEDED(hr) || (pvarFocus->vt != VT_I4))
        return(hr);

    lFunkyIndex = GET_AXA_ELEINDEX(pvarFocus->lVal);
    if (lFunkyIndex != m_ielMap)
    {
        pvarFocus->vt = VT_EMPTY;
        return(S_FALSE);
    }

    lFunkyIndex = GET_AXA_MAPINDEX(pvarFocus->lVal);
    pvarFocus->lVal = lFunkyIndex;
    return(hr);
}



// --------------------------------------------------------------------------
//
//  CImageMap::get_accDefaultAction()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    long    lFunkyIndex;

    InitPv(pszDefAction);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->get_accDefaultAction(varChild, pszDefAction));
}



// --------------------------------------------------------------------------
//
//  CImageMap::accSelect()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::accSelect(long lFlags, VARIANT varChild)
{
    long    lFunkyIndex;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->accSelect(lFlags, varChild));
}



// --------------------------------------------------------------------------
//
//  CImageMap::accLocation()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    long    lFunkyIndex;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));
}



// --------------------------------------------------------------------------
//
//  CImageMap::accNavigate()
//
//  We only navigate among elements in the image map
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    long    lChild;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if ((dwNavDir < NAVDIR_FIRSTCHILD) && !varStart.lVal)
    {
        varStart.lVal = m_ielMap;
        return(m_phtmlParent->accNavigate(dwNavDir, varStart, pvarEnd));
    }

    switch (dwNavDir)
    {
        case NAVDIR_FIRSTCHILD:
            // ValidateNavDir() will make sure that lVal is 0 fo FIRST, LAST
            // child requests.  So this just works.

        case NAVDIR_NEXT:
            lChild = varStart.lVal+1;
            if (lChild > m_cChildren)
                lChild = 0;
            break;

        case NAVDIR_LASTCHILD:
            lChild = m_cChildren;
            break;

        case NAVDIR_PREVIOUS:
            lChild = varStart.lVal-1;
            break;

        default:
            lChild = 0;
            break;
    }

    if (lChild)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lChild;
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CImageMap::accHitTest()
//
//  We can't forward this to our parent, since it will create an object if
//  the point is over an image mapl
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::accHitTest(long x, long y, VARIANT* pvarHit)
{
    POINT   pt;
    long    lResult;

    InitPvar(pvarHit);

    pt.x = x;
    pt.y = y;
    ScreenToClient(m_hwnd, &pt);

    lResult = SendMessageINT(m_hwnd, WM_HTML_GETAXAHITTEST, 0, MAKELPARAM(pt.x, pt.y));
    if (GET_AXA_ELEINDEX(lResult) != m_ielMap)
        return(S_FALSE);

    pvarHit->vt = VT_I4;
    pvarHit->lVal = GET_AXA_MAPINDEX(lResult);
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CImageMap::accDoDefaultAction()
//
//  Forward request to the parent HTML object
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::accDoDefaultAction(VARIANT varChild)
{
    long    lFunkyIndex;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lFunkyIndex = MAKE_AXA_INDEX(m_ielMap, varChild.lVal);
    varChild.lVal = lFunkyIndex;
    return(m_phtmlParent->accDoDefaultAction(varChild));
}



// --------------------------------------------------------------------------
//
//  CImageMap::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CImageMap::Clone(IEnumVARIANT** ppenum)
{
    return(CreateImageMap(m_hwnd, m_phtmlParent, m_ielMap, m_idChildCur,
        IID_IEnumVARIANT, (void**)ppenum));
}

#ifdef _DEBUG
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void PrintAXAInfo (LPAXAINFO lpaxaShared)
{
    DBPRINTF (TEXT("AXAInfo:\r\n"));

    // lFlags
    DBPRINTF (TEXT(" lFlags:\r\n"));
    if (lpaxaShared->lFlags & ELEFLAG_VISITED)
        DBPRINTF (TEXT("  ELEFLAG_VISITED\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_ANCHOR)
        DBPRINTF (TEXT("  ELEFLAG_ANCHOR\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_NAME)
        DBPRINTF (TEXT("  ELEFLAG_NAME\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_IMAGEMAP)
        DBPRINTF (TEXT("  ELEFLAG_IMAGEMAP\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_USEMAP)
        DBPRINTF (TEXT("  ELEFLAG_USEMAP\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_CENTER)
        DBPRINTF (TEXT("  ELEFLAG_CENTER\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_HR_NOSHADE)
        DBPRINTF (TEXT("  ELEFLAG_HR_NOSHADE\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_HR_PERCENT)
        DBPRINTF (TEXT("  ELEFLAG_HR_PERCENT\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_PERCENT_WIDTH)
        DBPRINTF (TEXT("  ELEFLAG_PERCENT_WIDTH\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_NOBREAK)
        DBPRINTF (TEXT("  ELEFLAG_NOBREAK\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_BACKGROUND_IMAGE)
        DBPRINTF (TEXT("  ELEFLAG_BACKGROUND_IMAGE\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_WBR)
        DBPRINTF (TEXT("  ELEFLAG_WBR\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_MARQUEE_PERCENT)
        DBPRINTF (TEXT("  ELEFLAG_MARQUEE_PERCENT\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_PERCENT_HEIGHT)
        DBPRINTF (TEXT("  ELEFLAG_PERCENT_HEIGHT\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_HIDDEN)
        DBPRINTF (TEXT("  ELEFLAG_HIDDEN\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_CELLTEXT)
        DBPRINTF (TEXT("  ELEFLAG_CELLTEXT\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_ANCHORNOCACHE)
        DBPRINTF (TEXT("  ELEFLAG_ANCHORNOCACHE\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_LEFT)
        DBPRINTF (TEXT("  ELEFLAG_LEFT\r\n"));

    if (lpaxaShared->lFlags & ELEFLAG_RIGHT)
        DBPRINTF (TEXT("  ELEFLAG_RIGHT\r\n"));


    DBPRINTF (TEXT(" fAnimated:%s\r\n"),lpaxaShared->fAnimated?TEXT("Yes"):TEXT("No"));

    DBPRINTF (TEXT(" eleType: "));
    switch (lpaxaShared->eleType)
    {
        case ELE_NOT:
            DBPRINTF (TEXT("ELE_NOT\r\n"));
            break;
        case ELE_TEXT:
            DBPRINTF (TEXT("ELE_TEXT\r\n"));
            break;
        case ELE_IMAGE:
            DBPRINTF (TEXT("ELE_IMAGE\r\n"));
            break;
        case ELE_VERTICALTAB:
            DBPRINTF (TEXT("ELE_VERTICALTAB\r\n"));
            break;
        case ELE_HR:
            DBPRINTF (TEXT("ELE_HR\r\n"));
            break;
        case ELE_NEWLINE:
            DBPRINTF (TEXT("ELE_NEWLINE\r\n"));
            break;
        case ELE_BEGINLIST:
            DBPRINTF (TEXT("ELE_BEGINLIST\r\n"));
            break;
        case ELE_ENDLIST:
            DBPRINTF (TEXT("ELE_ENDLIST\r\n"));
            break;
        case ELE_LISTITEM:
            DBPRINTF (TEXT("ELE_LISTITEM\r\n"));
            break;
        case ELE_EDIT:
            DBPRINTF (TEXT("ELE_EDIT\r\n"));
            break;
        case ELE_PASSWORD:
            DBPRINTF (TEXT("ELE_PASSWORD\r\n"));
            break;
        case ELE_CHECKBOX:
            DBPRINTF (TEXT("ELE_CHECKBOX\r\n"));
            break;
        case ELE_RADIO:
            DBPRINTF (TEXT("ELE_RADIO\r\n"));
            break;
        case ELE_SUBMIT:
            DBPRINTF (TEXT("ELE_SUBMIT\r\n"));
            break;
        case ELE_RESET:
            DBPRINTF (TEXT("ELE_RESET\r\n"));
            break;
        case ELE_COMBO:
            DBPRINTF (TEXT("ELE_COMBO\r\n"));
            break;
        case ELE_LIST:
            DBPRINTF (TEXT("ELE_LIST\r\n"));
            break;
        case ELE_TEXTAREA:
            DBPRINTF (TEXT("ELE_TEXTAREA\r\n"));
            break;
        case ELE_INDENT:
            DBPRINTF (TEXT("ELE_INDENT\r\n"));
            break;
        case ELE_OUTDENT:
            DBPRINTF (TEXT("ELE_OUTDENT\r\n"));
            break;
        case ELE_BEGINFORM:
            DBPRINTF (TEXT("ELE_BEGINFORM\r\n"));
            break;
        case ELE_ENDFORM:
            DBPRINTF (TEXT("ELE_ENDFORM\r\n"));
            break;
        case ELE_MULTILIST:
            DBPRINTF (TEXT("ELE_MULTILIST\r\n"));
            break;
        case ELE_HIDDEN:
            DBPRINTF (TEXT("ELE_HIDDEN\r\n"));
            break;
        case ELE_TAB:
            DBPRINTF (TEXT("ELE_TAB\r\n"));
            break;
        case ELE_OPENLISTITEM:
            DBPRINTF (TEXT("ELE_OPENLISTITEM\r\n"));
            break;
        case ELE_CLOSELISTITEM:
            DBPRINTF (TEXT("ELE_CLOSELISTITEM\r\n"));
            break;
        case ELE_FORMIMAGE:
            DBPRINTF (TEXT("ELE_FORMIMAGE\r\n"));
            break;
        case ELE_ENDBLOCKQUOTE:
            DBPRINTF (TEXT("ELE_ENDBLOCKQUOTE\r\n"));
            break;
        case ELE_FETCH:
            DBPRINTF (TEXT("ELE_FETCH\r\n"));
            break;
        case ELE_MARQUEE:
            DBPRINTF (TEXT("ELE_MARQUEE\r\n"));
            break;
        case ELE_BGSOUND:
            DBPRINTF (TEXT("ELE_BGSOUND\r\n"));
            break;
        case ELE_FRAME:
            DBPRINTF (TEXT("ELE_FRAME\r\n"));
            break;
        case ELE_OBJECT:
            DBPRINTF (TEXT("ELE_OBJECT\r\n"));
            break;
        case ELE_SUBSCRIPT:
            DBPRINTF (TEXT("ELE_SUBSCRIPT\r\n"));
            break;
        case ELE_SUPSCRIPT:
            DBPRINTF (TEXT("ELE_SUPSCRIPT\r\n"));
            break;
        case ELE_ALIAS:
            DBPRINTF (TEXT("ELE_ALIAS\r\n"));
            break;
        case ELE_EMBED:
            DBPRINTF (TEXT("ELE_EMBED\r\n"));
            break;
        case ELE_PARAM:
            DBPRINTF (TEXT("ELE_PARAM\r\n"));
            break;
        case ELE_EVENT:
            DBPRINTF (TEXT("ELE_EVENT\r\n"));
            break;
        case ELE_FRAMESET:
            DBPRINTF (TEXT("ELE_FRAMESET\r\n"));
            break;
        case ELE_BUTTON:
            DBPRINTF (TEXT("ELE_BUTTON\r\n"));
            break;
        case ELE_STYLESHEET:
            DBPRINTF (TEXT("ELE_STYLESHEET\r\n"));
            break;
        case ELE_ENDDOC:
            DBPRINTF (TEXT("ELE_ENDDOC\r\n"));
            break;
        default:
            DBPRINTF (TEXT("unknown element type %lu\r\n"),lpaxaShared->eleType);
    } // end switch eleType

    DBPRINTF (TEXT(" hwndEle: %lX\r\n"),lpaxaShared->hwndEle);

    DBPRINTF (TEXT(" rcItem: (%ld,%ld) (%ld,%ld)\r\n"),
        lpaxaShared->rcItem.left,lpaxaShared->rcItem.top,
        lpaxaShared->rcItem.right,lpaxaShared->rcItem.bottom);

    if (lpaxaShared->lpNameText)
        DBPRINTF (TEXT(" NameText: '%s'\r\n"),lpaxaShared->lpNameText);
    else
        DBPRINTF (TEXT(" NameText: NUL\r\n"));
   
    if (lpaxaShared->lpValueText)
        DBPRINTF (TEXT(" ValueText: '%s'\r\n"),lpaxaShared->lpValueText);
    else
        DBPRINTF (TEXT(" ValueText: NUL\r\n"));
}

#endif // _DEBUG


#endif // _X86_