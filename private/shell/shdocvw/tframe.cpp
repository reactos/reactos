#include "priv.h"
#include <shguidp.h>
#include "hlframe.h"
#include "winlist.h"
#include "resource.h" //CLSID_SearchBand

// Locally defined FINDFRAME flag used to guarantee ITargetFrame vs ITargetFrame2 compatibility

#define FINDFRAME_OLDINTERFACE FINDFRAME_INTERNAL


STDAPI SafeGetItemObject(LPSHELLVIEW psv, UINT uItem, REFIID riid, LPVOID *ppv);
HRESULT TargetQueryService(LPUNKNOWN punk, REFIID riid, void **ppvObj);
HRESULT CoCreateNewIEWindow( DWORD dwClsContext, REFIID riid, void **ppvunk );

/******************************************************************

    NAME:       SetOleStrMember

    SYNOPSIS:   sets a new value for OleStr member
******************************************************************/
HRESULT SetOleStrMember(WCHAR **ppszMember, LPCOLESTR pszNewValue)
{
    HRESULT hr;

    if (*ppszMember) 
        LocalFree(*ppszMember);

    if (pszNewValue)
    {
        *ppszMember = StrDupW(pszNewValue);
        hr = *ppszMember ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        *ppszMember = NULL;
        hr = S_OK;
    }
    return hr;
}

/******************************************************************

    NAME:       GetOleStrMember

    SYNOPSIS:   gets a value for OleStr member as new CoTaskMemAlloc
    LPOLESTR
******************************************************************/
HRESULT GetOleStrMember(LPCOLESTR pszMember, WCHAR **ppszReturnValue)
{
    HRESULT hr;

    if (pszMember)
        hr = SHStrDupW(pszMember, ppszReturnValue);
    else
    {
        hr = S_OK;
        *ppszReturnValue = NULL;
    }
    return hr;
}

/*******************************************************************

    NAME:       CIEFrameAuto::SetFrameName

    SYNOPSIS:   Sets the Frame Name.  Frees current one if exists.

********************************************************************/

STDMETHODIMP CIEFrameAuto::SetFrameName(LPCOLESTR pszFrameName)
{
    //  AOL and other 3rd Party 3.0 compatibility.  The ITargetNotify
    //  object that sets off a window open operation via AOL container
    //  registers itself in ShellCallbacks list and wnsprintf's the
    //  coresponding cookie in the front of the frame name ("" if there
    //  is no frame name as in _blank).  we extract the cookie, notify
    //  all registered callbacks, then set the name MINUS the _[NNNNN..]
    //  that was prepending to the frame name.
    if (pszFrameName && pszFrameName[0] == '_' && pszFrameName[1] == '[')
    {
#define MAX_COOKIE 24
        WCHAR wszCookie[MAX_COOKIE + 1];   // big enough for "cbCookie"
        int i;
        long cbCookie;
        BOOL fNonDigit = FALSE;
        IShellWindows*   psw = NULL;

        for (i = 2; i < MAX_COOKIE && pszFrameName[i] && pszFrameName[i] != ']'; i++)
        {
            wszCookie[i-2] = pszFrameName[i];
            if (i != 2 || pszFrameName[2] != '-')
                fNonDigit = fNonDigit ||  pszFrameName[i] < '0' || pszFrameName[i] > '9';
        }
        wszCookie[i-2] = 0;
        if (i >= 3 && pszFrameName[i] == ']' && !fNonDigit)
        {
            cbCookie = StrToIntW(wszCookie);
            psw = WinList_GetShellWindows(TRUE);
            if (psw)
            {
                IUnknown *punkThis;

                if (SUCCEEDED(QueryInterface(IID_IUnknown, (LPVOID *) &punkThis)))
                {
                    psw->OnCreated(cbCookie, punkThis);
                    punkThis->Release();
                }
                psw->Release();
            }
            pszFrameName = pszFrameName + i + 1;
            if (pszFrameName[0] == 0) pszFrameName = NULL;
        }
    }
    return SetOleStrMember(&m_pszFrameName, pszFrameName);
}

    
/*******************************************************************

    NAME:       CIEFrameAuto::GetFrameName

    SYNOPSIS:   Gets the Frame Name.  Allocates a copy (this is an
    [OUT] parameter

********************************************************************/

STDMETHODIMP CIEFrameAuto::GetFrameName(WCHAR **ppszFrameName)
{
    return GetOleStrMember(m_pszFrameName, ppszFrameName);
}


/*******************************************************************

    NAME:       CIEFrameAuto::_GetParentFramePrivate

    SYNOPSIS:   Gets an the IUnknown pointer of the parent frame, or
    NULL if this is a top level frame. This pointer must be Released
    by Caller after use.

    IMPLEMENTATION:
    A Frame Container is required to implement ITargetFrame::GetParentFrame and
    implement GetParentFrame by returning the IUnknown pointer of the Browser that
    hosts it. A Browser implements GetParentFrame by returning NULL if it's
    top level or calling GetParentFrame on its Container if it is embedded.
    
    NOTE: THIS PRIVATE VERSION doesn't check for parent being desktopframe.

********************************************************************/
HRESULT CIEFrameAuto::_GetParentFramePrivate(LPUNKNOWN *ppunkParentFrame)
{
    LPOLEOBJECT pOleObject = NULL;
    LPOLECLIENTSITE pOleClientSite = NULL;
    HRESULT hr = S_OK;
    LPUNKNOWN punkParent = NULL;

    //  Start off with OleObject for this OCX embedding, it will
    //  be null if we're top level (a CIEFrameAuto, not a CVOCBrowser)
    _GetOleObject(&pOleObject);
    if (pOleObject != NULL)
    {

    //  Assumes GetClientSite succeeds and returns NULL if we
    //  are not embedded
        hr = pOleObject->GetClientSite( &pOleClientSite);
        if (FAILED(hr)) goto errExit;
        pOleObject->Release();
        pOleObject = NULL;

    //  If pOleClientSite is NULL, then we are at the top level
        if (pOleClientSite == NULL)
        {
            hr = S_OK;
            goto errExit;
        }
        else
        {
            hr = TargetQueryService(pOleClientSite, IID_IUnknown, (LPVOID*)&punkParent);
            if (FAILED(hr)) 
            {
                // if parent container does not support ITargetFrame, then
                // the parent container must be some other app, like VB.  In this
                // case, we've already found the outermost frame (us).  Return
                // S_OK and a NULL ptgfTargetFrame which indicates that we are the
                // outermost HTML frame.
                hr = S_OK;
            }
            SAFERELEASE(pOleClientSite);
        }
    }

errExit:
    SAFERELEASE(pOleObject);
    SAFERELEASE(pOleClientSite);
    *ppunkParentFrame = punkParent;
    return hr;
}

/*******************************************************************

    NAME:       CIEFrameAuto::GetParentFrame

    SYNOPSIS:   Gets an the IUnknown pointer of the parent frame, or
    NULL if this is a top level frame. This pointer must be Released
    by Caller after use.

    IMPLEMENTATION:
    A Frame Container is required to implement ITargetFrame::GetParentFrame and
    implement GetParentFrame by returning the IUnknown pointer of the Browser that
    hosts it. A Browser implements GetParentFrame by returning NULL if it's
    top level or calling GetParentFrame on its Container if it is embedded.

********************************************************************/
STDMETHODIMP CIEFrameAuto::GetParentFrame(LPUNKNOWN *ppunkParentFrame)
{
    HRESULT hr = _GetParentFramePrivate(ppunkParentFrame);
    
    //  Check if the parent is the desktop, if so, the frame chain stops
    //  at us
    if (SUCCEEDED(hr) && *ppunkParentFrame)
    {
       LPTARGETFRAME2 ptgfParent;
       DWORD dwOptions;

       if (SUCCEEDED((*ppunkParentFrame)->QueryInterface(IID_ITargetFrame2, (LPVOID *)&ptgfParent)))
       {
           ptgfParent->GetFrameOptions(&dwOptions);
           if (dwOptions & FRAMEOPTIONS_DESKTOP)
           {
               (*ppunkParentFrame)->Release();
               *ppunkParentFrame = NULL;
           }
           ptgfParent->Release();
       }
    }
    return hr;
}

// PLEASE PROPOGATE ANY CHANGES TO THESE ENUMS TO \mshtml\iextag\httpwfh.h
typedef enum _TARGET_TYPE {
TARGET_FRAMENAME,
TARGET_SELF,
TARGET_PARENT,
TARGET_BLANK,
TARGET_TOP,
TARGET_MAIN,
TARGET_SEARCH
} TARGET_TYPE;

typedef struct _TARGETENTRY {
    TARGET_TYPE targetType;
    const WCHAR *pTargetValue;
} TARGETENTRY;

static const TARGETENTRY targetTable[] =
{
    {TARGET_SELF, L"_self"},
    {TARGET_PARENT, L"_parent"},
    {TARGET_BLANK, L"_blank"},
    {TARGET_TOP, L"_top"},
    {TARGET_MAIN, L"_main"},
    {TARGET_SEARCH, L"_search"},
    {TARGET_SELF, NULL}
};


/*******************************************************************

    NAME:       ParseTargetType

    SYNOPSIS:   Maps pszTarget into a target class.

    IMPLEMENTATION:
    Treats unknown MAGIC targets as _self

********************************************************************/
// PLEASE PROPOGATE ANY CHANGES TO THIS FUNCTION TO \mshtml\iextag\httpwf.cxx
TARGET_TYPE ParseTargetType(LPCOLESTR pszTarget)
{
    const TARGETENTRY *pEntry = targetTable;

    if (pszTarget[0] != '_') return TARGET_FRAMENAME;
    while (pEntry->pTargetValue)
    {
        if (!StrCmpW(pszTarget, pEntry->pTargetValue)) return pEntry->targetType;
        pEntry++;
    }
    //  Treat unknown MAGIC targets as regular frame name! <<for NETSCAPE compatibility>>
    return TARGET_FRAMENAME;

}

/*******************************************************************

    NAME:       TargetQueryService

    SYNOPSIS:   Returns a pointer to containing Browser's ITargetFrame
                interface (or NULL if container does not support it)

    NOTES:      If we don't yet have this interface pointer yet,
                this function will QueryInterface to get it.

********************************************************************/
HRESULT TargetQueryService(LPUNKNOWN punk, REFIID riid, void **ppvObj)
{
    //  Get the ITargetFrame for the embedding.
    return IUnknown_QueryService(punk, IID_ITargetFrame2, riid, ppvObj);
}


/*******************************************************************

    NAME:       _TargetTopLevelWindows

    SYNOPSIS:   see FindFrame, does the named targets across windows

********************************************************************/
HRESULT _TargetTopLevelWindows(LPTARGETFRAMEPRIV ptgfpThis, LPCOLESTR pszTargetName, DWORD dwFlags, LPUNKNOWN *ppunkTargetFrame)
{
    IShellWindows*   psw = NULL;
    HRESULT hr = E_FAIL;

    *ppunkTargetFrame = NULL;
    psw = WinList_GetShellWindows(TRUE);
    if (psw != NULL)
    {
        IUnknown *punkEnum;
        IEnumVARIANT *penumVariant;
        VARIANT VarResult;
        BOOL fDone = FALSE;
        LPTARGETFRAMEPRIV ptgfpWindowFrame;

        hr = psw->_NewEnum(&punkEnum);
        if (SUCCEEDED(hr))
        {
            hr = punkEnum->QueryInterface(IID_IEnumVARIANT, (LPVOID *)&penumVariant);
            if (SUCCEEDED(hr))
            {
                while (!fDone)
                {
                    VariantInit(&VarResult);
                    hr = penumVariant->Next(1, &VarResult, NULL);
                    if (hr == NOERROR)
                    {
                        if (VarResult.vt == VT_DISPATCH && VarResult.pdispVal)
                        {
                            hr = VarResult.pdispVal->QueryInterface(IID_ITargetFramePriv, (LPVOID *)&ptgfpWindowFrame);
                            if (SUCCEEDED(hr))
                            {
                                if (ptgfpWindowFrame != ptgfpThis)
                                {
                                    hr = ptgfpWindowFrame->FindFrameDownwards(pszTargetName,
                                                                             dwFlags,
                                                                             ppunkTargetFrame);
                                    if (SUCCEEDED(hr) && *ppunkTargetFrame != NULL)
                                    {
                                        fDone = TRUE;
                                    }
                                }
                                ptgfpWindowFrame->Release();
                            }
                        }
                    }
                    else fDone = TRUE;
                    VariantClear(&VarResult);
                }
                penumVariant->Release();
            }
            punkEnum->Release();
        }
        psw->Release();
    }
    return hr;
}

/*******************************************************************

  NAME:         CreateTargetFrame

  SYNOPSIS:     Creates a new window, if pszTargetName is not special
                target, names it pszTargetName.  returns IUnknown for
                the object that implements ITargetFrame,IHlinkFrame and
                IWebBrowserApp.
********************************************************************/
// PLEASE PROPOGATE ANY CHANGES TO THIS FUNCTION TO \mshtml\iextag\httpwf.cxx
HRESULT CreateTargetFrame(LPCOLESTR pszTargetName, LPUNKNOWN /*IN,OUT*/ *ppunk)
{
    LPTARGETFRAME2 ptgfWindowFrame;
    HRESULT hr = S_OK;

    //  Launch a new window, set it's frame name to pszTargetName
    //  return it's IUnknown. If the new window is passed to us,
    //  just set the target name.

    if (NULL == *ppunk)
    {
#ifndef  UNIX
        hr = CoCreateNewIEWindow(CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IUnknown, (LPVOID*)ppunk);
#else
        hr = CoCreateInternetExplorer( IID_IUnknown, 
                                       CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                                       (LPVOID*) ppunk );
#endif
    }

    if (SUCCEEDED(hr))
    {
        //  Don't set frame name if target is special or missing
        if (pszTargetName && ParseTargetType(pszTargetName) == TARGET_FRAMENAME)
        {
            HRESULT hrLocal;
            hrLocal = (*ppunk)->QueryInterface(IID_ITargetFrame2, (LPVOID *)&ptgfWindowFrame);
            if (SUCCEEDED(hrLocal))
            {
                ptgfWindowFrame->SetFrameName(pszTargetName);
                ptgfWindowFrame->Release();
            }
        }

        // Even if we don't set the frame name, we still want to return
        // success, otherwise we'd have a blank window hanging around.
    }

    return hr;
}

/*******************************************************************

    NAME:       CIEFrameAuto::_DoNamedTarget

    SYNOPSIS:   see FindFrame, does the named targets.  checks itself
    then if that fails, all children except for punkContextFrame (if
    punkContextFrame != NULL).  if all of the above fail, will recurse
    upwards if necessary, if punkContextFrame != NULL.  if punkContextFrame
    is NULL, then this works out to checking ourself and all children
    then giving up.

********************************************************************/
HRESULT CIEFrameAuto::_DoNamedTarget(LPCOLESTR pszTargetName, LPUNKNOWN punkContextFrame, DWORD dwFlags, LPUNKNOWN *ppunkTargetFrame)
{
    //BUGBUG asserts
    HRESULT hr = S_OK;
    HRESULT hrLocal;
    LPUNKNOWN punkParent = NULL;
    LPUNKNOWN punkThisFrame = NULL;
    LPTARGETFRAMEPRIV ptgfpParent = NULL;
    LPUNKNOWN punkThis = NULL;
    LPUNKNOWN punkChildObj = NULL;
    LPUNKNOWN punkChildFrame = NULL;
    LPTARGETFRAMEPRIV ptgfpChild = NULL;
    LPOLECONTAINER pOleContainer = NULL;
    LPENUMUNKNOWN penumUnknown = NULL;
    LPUNKNOWN punkProxyContextFrame = NULL;
    LPTARGETFRAMEPRIV ptgfpTarget = NULL;

    *ppunkTargetFrame = NULL;
    hr = QueryInterface(IID_IUnknown, (LPVOID*)&punkThis);
    ASSERT(punkThis != NULL);
    if (FAILED(hr)) goto exitPoint;

    //  First check for match on US!
    if (m_pszFrameName && !StrCmpW(pszTargetName, m_pszFrameName))
    {
        *ppunkTargetFrame = punkThis;
        //  Set punkThis to NULL to prevent Release at exit
        punkThis = NULL;
        goto exitPoint;
    }
    //  we ask our ShellView's embedded DocObject if it supports ITargetFrame.  If it does,
    //  we first let it look for the target.
    hrLocal = GetFramesContainer(&pOleContainer);
    if (SUCCEEDED(hrLocal) && pOleContainer != NULL)
    {
        hr = pOleContainer->EnumObjects(OLECONTF_EMBEDDINGS, &penumUnknown);
        if (hr != S_OK || penumUnknown == NULL) goto exitPoint;

        while (TRUE)
        {
            hr = penumUnknown->Next(1, &punkChildObj, NULL);
            if (punkChildObj == NULL)
            {
                hr = S_OK;
                break;
            }
            hrLocal = punkChildObj->QueryInterface(IID_ITargetFramePriv, (LPVOID *) &ptgfpChild);
            if (SUCCEEDED(hrLocal))
            {
                hr = ptgfpChild->QueryInterface(IID_IUnknown, (LPVOID *) &punkChildFrame);
                if (FAILED(hr)) goto exitPoint;

               //  IF this isn't the punkContextFrame, see if embedding supports ITargetFrame
                if (punkChildFrame != punkContextFrame)
                {
                    hr = ptgfpChild->FindFrameDownwards(pszTargetName,
                                                        dwFlags,
                                                        ppunkTargetFrame);
                    if (hr != S_OK || *ppunkTargetFrame != NULL) goto exitPoint;
                }
            }
            punkChildObj->Release();
            punkChildObj = NULL;
            SAFERELEASE(punkChildFrame);
            SAFERELEASE(ptgfpChild);
        }
    }

    //  We don't recurse to parent in punkContextFrame is NULL
    if (punkContextFrame == NULL) goto exitPoint;
    hr = GetParentFrame(&punkParent);
    if (hr != S_OK) goto exitPoint;

    if (punkParent != NULL)
    {
        //  We have a parent, recurse upwards, with ourself as context frame
        hr = punkParent->QueryInterface(IID_ITargetFramePriv, (LPVOID*)&ptgfpParent);

        if (hr != S_OK) goto exitPoint;
        hr = ptgfpParent->FindFrameInContext(pszTargetName,
                                             punkThis,
                                             dwFlags,
                                             ppunkTargetFrame);
        goto exitPoint;
    }

    //  At this point we've come to the top level frame.
    //  Enumerate top level windows, unless we're a toolbar

    *ppunkTargetFrame = NULL;
    if (_psb != _psbProxy)
    {
        //  Webbar : Find in context of top frame of proxy
        hr = TargetQueryService(_psbProxy, IID_ITargetFramePriv, (LPVOID *) &ptgfpTarget);
        if (SUCCEEDED(hr) && ptgfpTarget)
        {
            hr = ptgfpTarget->QueryInterface(IID_IUnknown, (LPVOID *) &punkProxyContextFrame);
            if (SUCCEEDED(hr))
            {
                hr = ptgfpTarget->FindFrameInContext(pszTargetName,
                                                     punkProxyContextFrame,
                                                     dwFlags,
                                                     ppunkTargetFrame);
                if (*ppunkTargetFrame) goto exitPoint;
            }
        }
    }
    else if (!(FINDFRAME_OLDINTERFACE&dwFlags))
    {
        hr = _TargetTopLevelWindows((LPTARGETFRAMEPRIV)this, pszTargetName, dwFlags, ppunkTargetFrame);
        if (*ppunkTargetFrame) goto exitPoint;
    }

    //  Now we have exhausted all frames.  Unless FINDFRAME_JUSTTESTEXISTENCE
    //  is set in dwFlags, create a new window, set it's frame name and return it
    if (dwFlags & FINDFRAME_JUSTTESTEXISTENCE)
    {
        hr = S_OK;
    }
    else
    {
        //  CreateTargetFrame will not work with AOL 3.01 clients
        //  so we must return E_FAIL

        hr = E_FAIL;
    }


exitPoint:
    SAFERELEASE(punkProxyContextFrame);
    SAFERELEASE(ptgfpTarget);
    SAFERELEASE(punkThis);
    SAFERELEASE(punkThisFrame);
    SAFERELEASE(ptgfpParent);
    SAFERELEASE(punkParent);
    SAFERELEASE(punkChildFrame);
    SAFERELEASE(ptgfpChild);
    SAFERELEASE(penumUnknown);
    SAFERELEASE(pOleContainer);
    SAFERELEASE(punkChildObj);
    return hr;
}



/*******************************************************************

    NAME:       CIEFrameAuto::SetFrameSrc

    SYNOPSIS:   Sets the Frame original SRC url.  Frees current one if exists.

********************************************************************/
HRESULT CIEFrameAuto::SetFrameSrc(LPCOLESTR pszFrameSrc)
{
    return SetOleStrMember(&m_pszFrameSrc, pszFrameSrc);
}

IShellView* CIEFrameAuto::_GetShellView(void)
{
    IShellView* psv = NULL;
    if (_psb) {
        _psb->QueryActiveShellView(&psv);
    }

    return psv;
}

/*******************************************************************

    NAME:       CIEFrameAuto::GetFrameSrc

    SYNOPSIS:   Gets the Frame original URL.  Allocates a copy (this is an
    [OUT] parameter

    NOTES:      If we are at top level, SRC is dynamic, so ask our
    contained DocObject to do it for us.

********************************************************************/
HRESULT CIEFrameAuto::GetFrameSrc(WCHAR **ppszFrameSrc)
{
    LPUNKNOWN punkParent = NULL;
    LPTARGETFRAME ptgfTargetFrame = NULL;
    LPTARGETCONTAINER ptgcTargetContainer = NULL;
    HRESULT hr;

    *ppszFrameSrc = NULL;
    hr = GetParentFrame(&punkParent);
    if (hr != S_OK) goto exitPoint;

    // If we're an embedding we have original src. If we're top level and
    // src has been set, return that, else defer to document
    if (punkParent != NULL || m_pszFrameSrc)
    {
        hr = GetOleStrMember(m_pszFrameSrc, ppszFrameSrc);
    }
    else // We're top level without an explicit SRC
    {
        *ppszFrameSrc = NULL;
        hr = S_OK;
        IShellView* psv = _GetShellView();
        if (psv)
        {
            HRESULT hrLocal = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_ITargetContainer, (LPVOID*) &ptgcTargetContainer);

            if (SUCCEEDED(hrLocal))
            {
                hr = ptgcTargetContainer->GetFrameUrl(ppszFrameSrc);
            }
            else
            {
                //  Backwards compatibility
                hrLocal = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_ITargetFrame, (LPVOID*) &ptgfTargetFrame);

                if (SUCCEEDED(hrLocal))
                {
                    hr = ptgfTargetFrame->GetFrameSrc(ppszFrameSrc);
                }
            }
            psv->Release();
        }
    }

exitPoint:
    SAFERELEASE(punkParent);
    SAFERELEASE(ptgfTargetFrame);
    SAFERELEASE(ptgcTargetContainer);
    return hr;
}



    
/*******************************************************************

    NAME:       CIEFrameAuto::GetFramesContainer

    SYNOPSIS:   Returns an addref'ed pointer to the LPOLECONTAINER
    for our nested frames.  Can be NULL.

********************************************************************/
HRESULT CIEFrameAuto::GetFramesContainer(LPOLECONTAINER *ppContainer)
{
    HRESULT hr;
    LPTARGETFRAME ptgfTargetFrame = NULL;
    LPTARGETCONTAINER ptgcTargetContainer = NULL;

    // BUGBUG REVIEW -- why is S_OK and NULL a valid return? E_FAIL makes way more sense...
    // (That's what will get returned if _psv->GetItemObject failes.)
    hr = S_OK;
    *ppContainer = NULL;
    IShellView* psv = _GetShellView();
    if (psv)
    {
        HRESULT hrLocal;

        hrLocal = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_ITargetContainer, (LPVOID*) &ptgcTargetContainer);

        if (SUCCEEDED(hrLocal))
        {
            hr = ptgcTargetContainer->GetFramesContainer(ppContainer);
        }
        else
        {
            //  Backwards compatibility
            hrLocal = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_ITargetFrame, (LPVOID*) &ptgfTargetFrame);

            if (SUCCEEDED(hrLocal))
            {
                hr = ptgfTargetFrame->GetFramesContainer(ppContainer);
            }
        }
        psv->Release();
    }
    SAFERELEASE(ptgcTargetContainer);
    SAFERELEASE(ptgfTargetFrame);

    return hr;
}


/*******************************************************************

    NAME:       CIEFrameAuto::SetFrameOptions

    SYNOPSIS:   Sets the Frame Options.

********************************************************************/
HRESULT CIEFrameAuto::SetFrameOptions(DWORD dwFlags)
{
    m_dwFrameOptions = dwFlags;
    return S_OK;
}

    
/*******************************************************************

    NAME:       CIEFrameAuto::GetFrameOptions

    SYNOPSIS:   Returns the frame options

********************************************************************/

HRESULT CIEFrameAuto::GetFrameOptions(LPDWORD pdwFlags)
{
    *pdwFlags = m_dwFrameOptions;

    // If we are full screen turn on a few extras...
    VARIANT_BOOL fFullScreen;
    if ((SUCCEEDED(get_FullScreen(&fFullScreen)) && fFullScreen == VARIANT_TRUE) ||
        (SUCCEEDED(get_TheaterMode(&fFullScreen)) && fFullScreen == VARIANT_TRUE)) {
        *pdwFlags |= FRAMEOPTIONS_SCROLL_AUTO | FRAMEOPTIONS_NO3DBORDER;
    } else if (_psbProxy != _psb) {
        // If we are in the WebBar, turn off the 3D border. 
        *pdwFlags |= FRAMEOPTIONS_NO3DBORDER;
    }
    // if we are desktop, turn on FRAMEOPTIONS_DESKTOP
    if (_fDesktopFrame)
    {
        *pdwFlags |= FRAMEOPTIONS_DESKTOP;
    }
    return S_OK;
}


/*******************************************************************

    NAME:       CIEFrameAuto::SetFrameMargins

    SYNOPSIS:   Sets the Frame margins.

********************************************************************/
HRESULT CIEFrameAuto::SetFrameMargins(DWORD dwWidth, DWORD dwHeight)
{
    m_dwFrameMarginWidth = dwWidth;
    m_dwFrameMarginHeight = dwHeight;
    return S_OK;
}

    
/*******************************************************************

    NAME:       CIEFrameAuto::GetFrameMargins

    SYNOPSIS:   Returns the frame margins

********************************************************************/

HRESULT CIEFrameAuto::GetFrameMargins(LPDWORD pdwWidth, LPDWORD pdwHeight)
{
    *pdwWidth = m_dwFrameMarginWidth;
    *pdwHeight = m_dwFrameMarginHeight;
    return S_OK;
}

/*******************************************************************

    NAME:       CIEFrameAuto::_fDesktopComponent

    SYNOPSIS:   Returns TRUE if this frame is a desktop component
    top level frame or a the top frame of a browser band other than
    the search pane.  These panes need special treatment of FindFrame
    and navigate.

*******************************************************************/
BOOL CIEFrameAuto::_fDesktopComponent()
{
    BOOL fInDesktop = FALSE;
    LPUNKNOWN punkParent;

    //  Special interpretation for desktop components and non-search browser bands
    //  NULL pszTargetName at top level frames is defined as being targeted
    //  to the window whose frame is "_desktop".  this will create a new top level
    //  browser as necessary and return it's frame.
    if (SUCCEEDED(_GetParentFramePrivate(&punkParent)) && punkParent)
    {
        DWORD dwOptions;
        LPTARGETFRAME2 ptgfTop;

        
        //  not a top level frame unless our parent is desktop frame
        if (SUCCEEDED(punkParent->QueryInterface(IID_ITargetFrame2, (LPVOID *) &ptgfTop)))
        {
            if (SUCCEEDED(ptgfTop->GetFrameOptions(&dwOptions)))
            {
                fInDesktop = (dwOptions & FRAMEOPTIONS_DESKTOP) ? TRUE:FALSE;
            }
            ptgfTop->Release();
        }
        punkParent->Release();
    }
    else if (m_dwFrameOptions & FRAMEOPTIONS_BROWSERBAND)
    {
        //  a browser band - check for search band (proxied hlinkframe)
        fInDesktop = _psb == _psbProxy;
    }
    return fInDesktop;
}

//  ITargetFrame2 members
    
/*******************************************************************

    NAME:       CIEFrameAuto::GetTargetAlias

    SYNOPSIS:   Gets the Frame Name.  Allocates a copy (this is an
    [OUT] parameter

********************************************************************/

STDMETHODIMP CIEFrameAuto::GetTargetAlias(LPCOLESTR pszTargetName, WCHAR **ppszTargetAlias)
{
    BOOL fInDesktop = _fDesktopComponent();
    
    //  Special interpretation for desktop components and non-search browser bands
    //  NULL pszTargetName and "_top" at top level frames are defined as being targeted
    //  to the window whose frame is "_desktop".  this will create a new top level
    //  browser as necessary and return it's frame.

    if (pszTargetName == NULL && _fDesktopComponent())
        return GetOleStrMember(L"_desktop", ppszTargetAlias);
    *ppszTargetAlias = NULL;
    return E_FAIL;
}

/*******************************************************************

    NAME:       CIEFrameAuto::FindFrame

    SYNOPSIS:   Gets an the IUnknown pointer of the frame referenced
    by pszTarget. This pointer must be Released by Caller after use.
    punkContextFrame, if not NULL, is the IUnknown pointer for the immediate
    descendent frame in whose subtree the Target reference (eg anchor with a Target tag)
    resides.  dwFlags are flags which modify FindFrame's behaviour and
    can be any combination of FINDFRAME_FLAGS. In particular, SETTING
    FINDFRAME_JUSTTESTEXISTENCE allows the caller to defeat the default
    FindFrame behavior of creating a new top level frame named pszTarget,
    if pszTarget does not exist.

    IMPLEMENTATION:

    NOTE: In HTML all anchors and other TARGET tags can occur ONLY in
    leaf FRAMES!!

    punkContextFrame is significant only if pszTarget is not
    a MAGIC target name (_self, _top, _blank, _parent).

    Non-MAGIC target names:

    first off, this frame should check if it matches pszTarget and return
    it's own IUnknown pointer forthwith.

    if punkContextFrame is not NULL, all child Frames
    except punkContextFrame should be searched (depth first) for
    pszTarget with punkContextFrame == NULL.  on failure, the parent of this
    frame should be recursively called with this frame replacing punkContextFrame.
    if this is a top level Frame (so there is no parent), all top level frames
    should be called with punkContextFrame == NULL.  if this fails, then a new top level
    frame should be created (unless FINDFRAME_JUSTTESTEXISTENCE is set in
    dwFlags), named pszTarget and its IUnknown returned.

    if punkContextFrame is NULL, all child Frames should be searched
    depth first for pszTarget.  on failure, NULL should be returned.


    MAGIC target names:

    _self should return the IUnknown of this ITargetFrame
    _top should be recursively passed up to the top level ITargetFrame. if
    there is no FrameParent, this defaults to _self.
    _parent should return the IUnknown of the FrameParent ITargetFrame. if
    there is no FrameParent, this defaults to _self.
    _blank should be recursively passed up to the top level ITargetFrame,
    which should create a unnamed top level frame

********************************************************************/

STDMETHODIMP CIEFrameAuto::FindFrame(LPCOLESTR pszTargetName,
                                           DWORD dwFlags,
                                           LPUNKNOWN *ppunkTargetFrame)
{
    LPTARGETFRAMEPRIV ptgfpTarget = NULL;
    LPUNKNOWN punkContextFrame = NULL;
    HRESULT hr = E_FAIL;
    BOOL fInContext = TRUE;
    BOOL fWasMain = FALSE;
    TARGET_TYPE targetType;

    if (pszTargetName == NULL || *pszTargetName == 0)
        pszTargetName = L"_self";
    targetType  = ParseTargetType(pszTargetName);
    if (targetType == TARGET_MAIN)
    {
        fWasMain = TRUE;
        pszTargetName = L"_self";
    }

    *ppunkTargetFrame = NULL;

    // Default behavior:
    //  If this is a webbar and targeting _main, find frame in _psbProxy and return it
    //  If this is in browser, find frame relative to ourselves
    if (_psb != _psbProxy && fWasMain)
    {
        //  Webbar : Find in context of top frame of proxy
        hr = TargetQueryService(_psbProxy, IID_ITargetFramePriv, (LPVOID *) &ptgfpTarget);
    }
    else
    {
        //   Browser : A normal find in context in ourself

        hr = QueryInterface(IID_ITargetFramePriv, (LPVOID *) &ptgfpTarget);
    }

    if (SUCCEEDED(hr) && ptgfpTarget)
    {
        if (fInContext) 
        {
            hr = ptgfpTarget->QueryInterface(IID_IUnknown, (LPVOID *) &punkContextFrame);
            if (SUCCEEDED(hr))
            {
                hr = ptgfpTarget->FindFrameInContext(pszTargetName,
                                                     punkContextFrame,
                                                     dwFlags,
                                                     ppunkTargetFrame);

            }
        }
        else
        {
            hr = ptgfpTarget->FindFrameDownwards(pszTargetName,
                                                 dwFlags,
                                                 ppunkTargetFrame);
        }

    }

    SAFERELEASE(punkContextFrame);
    SAFERELEASE(ptgfpTarget);
    if (SUCCEEDED(hr) 
        && *ppunkTargetFrame == NULL
        && !(FINDFRAME_OLDINTERFACE&dwFlags)) hr = S_FALSE;
    return hr;
}

//  ITargetFramePriv members

/*******************************************************************

    NAME:       CIEFrameAuto::FindFrameDownwards

    SYNOPSIS:   

    IMPLEMENTATION:


********************************************************************/

STDMETHODIMP CIEFrameAuto::FindFrameDownwards(LPCOLESTR pszTargetName,
                                              DWORD dwFlags,
                                              LPUNKNOWN *ppunkTargetFrame)
{
    return FindFrameInContext(pszTargetName, 
                              NULL, 
                              dwFlags | FINDFRAME_JUSTTESTEXISTENCE,
                              ppunkTargetFrame);
}

/*******************************************************************

    NAME:       CIEFrameAuto::FindFrameInContext

    SYNOPSIS:   

    IMPLEMENTATION:


********************************************************************/

STDMETHODIMP CIEFrameAuto::FindFrameInContext(LPCOLESTR pszTargetName,
                                              LPUNKNOWN punkContextFrame,
                                              DWORD dwFlags,
                                              LPUNKNOWN *ppunkTargetFrame)
{
    //BUGBUG asserts
    TARGET_TYPE targetType;
    HRESULT hr = S_OK;
    LPUNKNOWN punkParent = NULL;
    LPUNKNOWN punkThisFrame = NULL;
    LPTARGETFRAMEPRIV ptgfpTargetFrame = NULL;
    LPUNKNOWN punkThis = NULL;

    targetType  = ParseTargetType(pszTargetName);
    if (targetType == TARGET_FRAMENAME)
    {
        hr = _DoNamedTarget(pszTargetName, punkContextFrame, dwFlags, ppunkTargetFrame);
        goto exitPoint;
    }

    //  Must be a Magic Target

    //for search, first show the search bar and then reach across to get it's TargetFrame
    if (targetType == TARGET_SEARCH)
    {
        SA_BSTRGUID  strGuid;
        VARIANT      vaGuid;
        VARIANT      vaPunkBand;

        StringFromGUID2(CLSID_SearchBand, strGuid.wsz, ARRAYSIZE(strGuid.wsz));
        strGuid.cb = lstrlenW(strGuid.wsz) * SIZEOF(WCHAR);
        vaGuid.vt = VT_BSTR;
        vaGuid.bstrVal = strGuid.wsz;

        //if we're in an explorer bar, use the proxy's pbs
        IBrowserService *pbs = _pbs;
        if (_psb != _psbProxy)
        {
            EVAL(SUCCEEDED(_psbProxy->QueryInterface(IID_IBrowserService, (void**)&pbs)));
        }

        IUnknown_Exec(pbs, &CGID_ShellDocView, SHDVID_SHOWBROWSERBAR, 1, &vaGuid, NULL);

        VariantInit(&vaPunkBand);
        hr = IUnknown_Exec(pbs, &CGID_ShellDocView, SHDVID_GETBROWSERBAR, NULL, NULL, &vaPunkBand);

        if (_psb != _psbProxy)
        {
            ATOMICRELEASE(pbs);
        }

        if (hr == S_OK)
        {
            IDeskBand* pband;
            
            pband = (IDeskBand*)vaPunkBand.punkVal;
            ASSERT(pband);
            
            if (pband)
            {
                IBrowserBand* pbb;
                
                hr = pband->QueryInterface(IID_IBrowserBand, (LPVOID*)&pbb);
                if (SUCCEEDED(hr))
                {
                    // now, get the pidl search pane is navigated to.
                    // if it's null we have to navigate it to something -- default search url (web search)
                    // this used to be in CSearchBand::_NavigateOC but caused problems
                    // if user had dial up networking set up and tried to get to the file search we would 
                    // first set pidl to web search url (_NavigateOC is called by _CreateOCHost) which would 
                    // cause AutoDial dialog to come up and then we would navigate the pane to the file search
                    // (nt5 bug#186970) reljai -- 6/22/98
                    VARIANT varPidl = {0};

                    if (SUCCEEDED(IUnknown_Exec(pbb, &CGID_SearchBand, SBID_GETPIDL, 0, NULL, &varPidl)))
                    {
                        ISearchItems* psi;
                        LPITEMIDLIST  pidl = VariantToIDList(&varPidl);
                        
                        VariantClear(&varPidl);
                        if (!pidl && SUCCEEDED(IUnknown_QueryService(pbb, SID_SExplorerToolbar, IID_ISearchItems, (void **)&psi)))
                        {
                            // get the default search url
                            WCHAR wszSearchUrl[INTERNET_MAX_URL_LENGTH];
        
                            if (SUCCEEDED(psi->GetDefaultSearchUrl(wszSearchUrl, ARRAYSIZE(wszSearchUrl))))
                            {
                                IBandNavigate* pbn;

                                if (SUCCEEDED(pbb->QueryInterface(IID_IBandNavigate, (void **)&pbn)))
                                {
                                    // reuse pidl
                                    IECreateFromPathW(wszSearchUrl, &pidl);
                                    pbn->Select(pidl);
                                    pbn->Release();
                                }
                            }
                            psi->Release();
                        }
                        ILFree(pidl);
                    }
                    
                    IWebBrowser2* pwb;

                    hr = pbb->GetObjectBB(IID_IWebBrowser2, (LPVOID*)&pwb);

                    //set the search pane's opener property
                    if (SUCCEEDED(hr))
                    {
                        IDispatch* pdisp;
                        
                        if (SUCCEEDED(pwb->get_Document(&pdisp)) && pdisp)
                        {
                            IHTMLDocument2* pDoc;
                            if (SUCCEEDED(pdisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
                            {
                                IHTMLWindow2* pWindow;
                            
                                if (SUCCEEDED(pDoc->get_parentWindow(&pWindow)) && pWindow)
                                {
                                    VARIANT var;
                                    VariantInit(&var);
                                    var.vt = VT_DISPATCH;
                                    _omwin.QueryInterface(IID_IUnknown, (void**)&var.pdispVal);
                                    
                                    pWindow->put_opener(var);

                                    VariantClear(&var);
                                    pWindow->Release();
                                }
                                pDoc->Release();
                            }
                            pdisp->Release();
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pwb->QueryInterface(IID_ITargetFramePriv, (void **)ppunkTargetFrame);
                        pwb->Release();
                    }

                    pbb->Release();
                }
                pband->Release();
            }
        }
        else
        {
            //maybe we're the search bar
            //hack to let search pane know to remember the next navigation
            IUnknown *punkThis;

            if (SUCCEEDED(_psb->QueryInterface(IID_IUnknown, (void**)&punkThis)))
            {
                hr = QueryInterface(IID_ITargetFramePriv, (void**)&ptgfpTargetFrame);

                if (SUCCEEDED(hr))
                    *ppunkTargetFrame = ptgfpTargetFrame;
                ptgfpTargetFrame = NULL;
                punkThis->Release();
            }
        }
        
        goto exitPoint;
    }

    hr = QueryInterface(IID_IUnknown, (LPVOID*)&punkThis);
    ASSERT(punkThis != NULL);
    if (targetType == TARGET_SELF)
    {
        *ppunkTargetFrame = punkThis;
    //  Set punkThis to NULL to prevent Release at exit
        punkThis = NULL;
    }
    else  // _blank, _parent, _top
    {
        hr = GetParentFrame(&punkParent);
        if (hr != S_OK) goto exitPoint;

        if (punkParent == NULL)
        {
            if (targetType == TARGET_PARENT || targetType == TARGET_TOP)
            {
                *ppunkTargetFrame = punkThis;
                //  Set punkThis to NULL to prevent Release at exit
                punkThis = NULL;
            }
            else // TARGET_BLANK
            {
                if (dwFlags & FINDFRAME_JUSTTESTEXISTENCE)
                {
                    //  It is the client's responsibility to handle "_blank"
                    hr = S_OK;
                }
                else
                {
                    //  CreateTargetFrame will not work with AOL 3.01 clients
                    //  so we must return E_FAIL

                    hr = E_FAIL;
                }
                *ppunkTargetFrame = NULL;
            }
        }
        else // punkParent != NULL
        {
            //  Handle parent ourself, defer _top and _blank to top level frame
            if (targetType == TARGET_PARENT)
            {
                *ppunkTargetFrame = punkParent;
                //  Set punkThisFrame to NULL to prevent Release at exit
                punkParent = NULL;
            }
            else
            {
                hr = punkParent->QueryInterface(IID_ITargetFramePriv, (LPVOID*)&ptgfpTargetFrame);
                if (hr != S_OK) goto exitPoint;
                hr = ptgfpTargetFrame->FindFrameInContext(pszTargetName,
                                             punkThis,
                                             dwFlags,
                                             ppunkTargetFrame);
            }
        }
    }

exitPoint:
    SAFERELEASE(punkThis);
    SAFERELEASE(punkThisFrame);
    SAFERELEASE(ptgfpTargetFrame);
    SAFERELEASE(punkParent);
    return hr;
}

HRESULT CIEFrameAuto::_GetOleObject(IOleObject** ppobj)
{
    HRESULT hres = E_UNEXPECTED;
    if (_pbs) {
        hres = _pbs->GetOleObject(ppobj);
    }

    return hres;
}

//  ITargetFrame implementation for backwards compatibility

HRESULT CIEFrameAuto::CTargetFrame::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->QueryInterface(riid, ppvObj);
}

ULONG CIEFrameAuto::CTargetFrame::AddRef(void) 
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->AddRef();
}

ULONG CIEFrameAuto::CTargetFrame::Release(void) 
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->Release();
}

/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::SetFrameName

    SYNOPSIS:   Sets the Frame Name.  Frees current one if exists.

********************************************************************/

STDMETHODIMP CIEFrameAuto::CTargetFrame::SetFrameName(LPCOLESTR pszFrameName)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->SetFrameName(pszFrameName);
}

    
/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::GetFrameName

    SYNOPSIS:   Gets the Frame Name.  Allocates a copy (this is an
    [OUT] parameter

********************************************************************/

STDMETHODIMP CIEFrameAuto::CTargetFrame::GetFrameName(WCHAR **ppszFrameName)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->GetFrameName(ppszFrameName);
}


/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::GetParentFrame

    SYNOPSIS:   Gets an the IUnknown pointer of the parent frame, or
    NULL if this is a top level frame. This pointer must be Released
    by Caller after use.

    IMPLEMENTATION:
    A Frame Container is required to implement ITargetFrame::GetParentFrame and
    implement GetParentFrame by returning the IUnknown pointer of the Browser that
    hosts it. A Browser implements GetParentFrame by returning NULL if it's
    top level or calling GetParentFrame on its Container if it is embedded.

********************************************************************/
STDMETHODIMP CIEFrameAuto::CTargetFrame::GetParentFrame(LPUNKNOWN *ppunkParentFrame)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->GetParentFrame(ppunkParentFrame);
}

/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::FindFrame

    SYNOPSIS:   Gets an the IUnknown pointer of the frame referenced
    by pszTarget. This pointer must be Released by Caller after use.
    punkContextFrame, if not NULL, is the IUnknown pointer for the immediate
    descendent frame in whose subtree the Target reference (eg anchor with a Target tag)
    resides.  dwFlags are flags which modify FindFrame's behaviour and
    can be any combination of FINDFRAME_FLAGS. In particular, SETTING
    FINDFRAME_JUSTTESTEXISTENCE allows the caller to defeat the default
    FindFrame behavior of creating a new top level frame named pszTarget,
    if pszTarget does not exist.

    IMPLEMENTATION:

    NOTE: In HTML all anchors and other TARGET tags can occur ONLY in
    leaf FRAMES!!

    punkContextFrame is significant only if pszTarget is not
    a MAGIC target name (_self, _top, _blank, _parent).

    Non-MAGIC target names:

    first off, this frame should check if it matches pszTarget and return
    it's own IUnknown pointer forthwith.

    if punkContextFrame is not NULL, all child Frames
    except punkContextFrame should be searched (depth first) for
    pszTarget with punkContextFrame == NULL.  on failure, the parent of this
    frame should be recursively called with this frame replacing punkContextFrame.
    if this is a top level Frame (so there is no parent), all top level frames
    should be called with punkContextFrame == NULL.  if this fails, then a new top level
    frame should be created (unless FINDFRAME_JUSTTESTEXISTENCE is set in
    dwFlags), named pszTarget and its IUnknown returned.

    if punkContextFrame is NULL, all child Frames should be searched
    depth first for pszTarget.  on failure, NULL should be returned.


    MAGIC target names:

    _self should return the IUnknown of this ITargetFrame
    _top should be recursively passed up to the top level ITargetFrame. if
    there is no FrameParent, this defaults to _self.
    _parent should return the IUnknown of the FrameParent ITargetFrame. if
    there is no FrameParent, this defaults to _self.
    _blank should be recursively passed up to the top level ITargetFrame,
    which should create a unnamed top level frame

********************************************************************/

STDMETHODIMP CIEFrameAuto::CTargetFrame::FindFrame(LPCOLESTR pszTargetName,
                                      LPUNKNOWN punkContextFrame,
                                      DWORD dwFlags,
                                      LPUNKNOWN *ppunkTargetFrame)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);
    
    return pie->FindFrame(pszTargetName, dwFlags|FINDFRAME_OLDINTERFACE, ppunkTargetFrame);
}

/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::RemoteNavigate

    SYNOPSIS:   Used in response to WM_COPYDATA message with dwData
                equal to TF_NAVIGATE.  Does a FindFrame (named
                target only) and if frame is not found returns
                S_FALSE.  If found, returns S_OK and fires off the
                navigate. cLength is number of ULONGs in pulData

    TODO:       Relies on RemoteNavigate member of top level MSHTML
                docobject host.  Need to write the equivalent code
                to work if top level frame contains some other DocObject.
                Post,etc require help from bindstatuscallback.

********************************************************************/
HRESULT CIEFrameAuto::CTargetFrame::RemoteNavigate(ULONG cLength,ULONG *pulData)
{
    //  BUGBUG chrisfra 10/22/96 - this is now here purely for backwards compatibility and
    //  should be removed for ie4.0
    return E_FAIL;
}

/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::SetFrameSrc

    SYNOPSIS:   Sets the Frame original SRC url.  Frees current one if exists.

********************************************************************/
HRESULT CIEFrameAuto::CTargetFrame::SetFrameSrc(LPCOLESTR pszFrameSrc)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->SetFrameSrc(pszFrameSrc);
}
    
/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::GetFrameSrc

    SYNOPSIS:   Gets the Frame original URL.  Allocates a copy (this is an
    [OUT] parameter

    NOTES:      If we are at top level, SRC is dynamic, so ask our
    contained DocObject to do it for us.

********************************************************************/
HRESULT CIEFrameAuto::CTargetFrame::GetFrameSrc(WCHAR **ppszFrameSrc)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->GetFrameSrc(ppszFrameSrc);
}



    
/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::GetFramesContainer

    SYNOPSIS:   Returns an addref'ed pointer to the LPOLECONTAINER
    for our nested frames.  Can be NULL.

********************************************************************/
HRESULT CIEFrameAuto::CTargetFrame::GetFramesContainer(LPOLECONTAINER *ppContainer)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->GetFramesContainer(ppContainer);
}


/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::SetFrameOptions

    SYNOPSIS:   Sets the Frame Options.

********************************************************************/
HRESULT CIEFrameAuto::CTargetFrame::SetFrameOptions(DWORD dwFlags)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->SetFrameOptions(dwFlags);
}

    
/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::GetFrameOptions

    SYNOPSIS:   Returns the frame options

********************************************************************/

HRESULT CIEFrameAuto::CTargetFrame::GetFrameOptions(LPDWORD pdwFlags)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->GetFrameOptions(pdwFlags);
}


/*******************************************************************

    NAME:       CIEFrameAuto::CTargetFrame::SetFrameMargins

    SYNOPSIS:   Sets the Frame margins.

********************************************************************/
HRESULT CIEFrameAuto::CTargetFrame::SetFrameMargins(DWORD dwWidth, DWORD dwHeight)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->SetFrameMargins(dwWidth, dwHeight);
}

    
/*******************************************************************

  NAME:       CIEFrameAuto::CTargetFrame::GetFrameMargins

    SYNOPSIS:   Returns the frame margins

********************************************************************/

HRESULT CIEFrameAuto::CTargetFrame::GetFrameMargins(LPDWORD pdwWidth, LPDWORD pdwHeight)
{
    CIEFrameAuto* pie = IToClass(CIEFrameAuto, _TargetFrame, this);

    return pie->GetFrameMargins(pdwWidth, pdwHeight);
}


/*******************************************************************

  NAME:       CIEFrameAuto::FindBrowserByIndex

    SYNOPSIS:   Returns an IUnknown that points to a Browser that
                has the requested index

********************************************************************/

HRESULT CIEFrameAuto::FindBrowserByIndex(DWORD dwID,IUnknown **ppunkBrowser)
{
    HRESULT hr = S_OK;
    IOleContainer *poc;
    IBrowserService *pbs;
    ASSERT(ppunkBrowser);
    *ppunkBrowser = NULL;

    if (!_psb)
        return E_FAIL;

    // first check self
    if(SUCCEEDED(_psb->QueryInterface(IID_IBrowserService, (LPVOID *) &pbs)))
    {
        ASSERT(pbs);
        if(dwID == pbs->GetBrowserIndex())
        {
            //  this is the one...
            *ppunkBrowser = (IUnknown *)pbs;
            goto exitPoint;
        }
        SAFERELEASE(pbs);
    }

    hr = GetFramesContainer(&poc);

    if (SUCCEEDED(hr) && poc)
    {
        IEnumUnknown *penum = NULL;
        IUnknown *punk;
        ASSERT(poc);

        hr = E_FAIL;
        
        if (S_OK != poc->EnumObjects(OLECONTF_EMBEDDINGS, &penum) || penum == NULL) 
            goto exitPoint;

        while (S_OK == penum->Next(1, &punk, NULL))
        {
            ITargetFramePriv *ptf;
            if (punk == NULL)
               break;

            if(SUCCEEDED(punk->QueryInterface(IID_ITargetFramePriv, (LPVOID *) &ptf)))
            {
                ASSERT(ptf);

                hr = ptf->FindBrowserByIndex(dwID, ppunkBrowser);
                SAFERELEASE(ptf);

            }

            SAFERELEASE(punk);
        
            if(SUCCEEDED(hr))  //foundit!
                break;

        }
        SAFERELEASE(penum);
        SAFERELEASE(poc);
    }
    else 
        hr = E_FAIL;

exitPoint:

    return hr;
}

//  External helper function for TRIDENT when it stands alone w/o the steely thews of
//  shdocvw CIEFrameAuto to shield it's pityfull body.
#ifdef UNIX
extern "C" 
#endif
HRESULT HlinkFindFrame(LPCWSTR pszFrameName, LPUNKNOWN *ppunk)
{
    HRESULT hres = E_FAIL;

    *ppunk = NULL;
    if (pszFrameName)
    {
        switch (ParseTargetType(pszFrameName))
        {
        case TARGET_FRAMENAME:
            hres = _TargetTopLevelWindows(NULL, pszFrameName, FINDFRAME_JUSTTESTEXISTENCE, ppunk);
            break;
        case TARGET_BLANK:
            hres = S_FALSE;
            break;
        }
    }
    return hres;
}
