//=-----------------------------------------------------------=
//
// File:        earea.cxx
//
// Contents:    Area element class
//
// Classes:     CHyperlink
//
//=-----------------------------------------------------------=


#include "headers.hxx"

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx" // AreRatingsEnabled()
#endif

MtDefine(CHyperlink, Elements, "CHyperlink")
MtDefine(CHyperlinkGetUrlComponent, Utilities, "CHyperlink::GetUrlComponent")


//+---------------------------------------------------------------------------
//
// Member: CHyperlink::SetStausText
//
//----------------------------------------------------------------------------

HRESULT
CHyperlink::SetStatusText()
{
    HRESULT     hr;
    CDoc *      pDoc    = Doc();
    TCHAR *     pchUrl  = NULL;

    hr = GetUrlComponent(NULL, URLCOMP_WHOLE, &pchUrl);
    if (!hr && pchUrl)
    {
        TCHAR *pchFriendlyUrl = GetFriendlyUrl(
                pchUrl,
                pDoc->_cstrUrl,
                pDoc->_pOptionSettings->fShowFriendlyUrl, TRUE);

        pDoc->SetStatusText(pchFriendlyUrl, STL_ROLLSTATUS);

        MemFreeString(pchFriendlyUrl);
        MemFreeString(pchUrl);
    }
    return hr;

}

//+------------------------------------------------------------------------
//
//  Member:     CHyperlink::CopyLinkToClipboard
//
//  Synopsis:   Copies the asssociated link to clipboard, which can then be
//              pasted onto dersktop as a URL shortcut, etc. 
//
//-------------------------------------------------------------------------
HRESULT CHyperlink::CopyLinkToClipboard(const TCHAR * pchDesc/*=NULL*/)
{
    HRESULT                     hr              = S_OK;
    IDataObject *               pDO             = NULL;
    IUniformResourceLocator *   pURLToDrag      = NULL;
    TCHAR                       cBuf[pdlUrlLen];
    TCHAR *                     pchExpandedUrl  = cBuf;
    CStr                        strUrlTitle;
    CDoc *                      pDoc = Doc();

    // fully resolve URL
    hr = THR(pDoc->ExpandUrl(GetUrl(), ARRAY_SIZE(cBuf), pchExpandedUrl, this));
    if (hr)
        goto Cleanup;

    if (!pchDesc && S_OK == GetUrlTitle(&strUrlTitle))
    {
        pchDesc = strUrlTitle;
    }

    hr = THR(CreateLinkDataObject(pchExpandedUrl, pchDesc, &pURLToDrag));
    if (hr)
        goto Cleanup;

    hr = THR(pURLToDrag->QueryInterface(IID_IDataObject, (void **)&pDO));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->SetClipboard(pDO));
    
Cleanup:
    ReleaseInterface(pURLToDrag);
    ReleaseInterface(pDO);
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
// Members:     URL componenet access helpers
//
// sysnopsis:  [Get/Set]UrlComponentHelper wraps InternetCrackURL
//              the fucntions below all call the helper with diffent
//              component requests:
//              Hash
//              Host
//              search
//              Hostname
//              pathname
//              port
//              protocol
//--------------------------------------------------------------------
#define URL_COMPONENT_FLAGS  (ICU_DECODE | URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE)

HRESULT
CHyperlink::get_host(BSTR *pstr)
{
    RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_HOST, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_host(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_HOST)));
}

HRESULT
CHyperlink::get_hostname(BSTR *pstr)
{
    RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_HOSTNAME, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_hostname(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_HOSTNAME)));
}

HRESULT
CHyperlink::get_pathname(BSTR *pstr)
{
    RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_PATHNAME, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_pathname(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_PATHNAME)));
}

HRESULT
CHyperlink::get_port(BSTR *pstr)
{
    RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_PORT, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_port(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_PORT)));
}

HRESULT
CHyperlink::get_protocol(BSTR *pstr)
{
     RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_PROTOCOL, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_protocol(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_PROTOCOL)));
}

HRESULT
CHyperlink::get_search(BSTR *pstr)
{
    RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_SEARCH, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_search(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_SEARCH)));
}

HRESULT
CHyperlink::get_hash(BSTR *pstr)
{
    RRETURN(SetErrorInfo(GetUrlComponent(pstr, URLCOMP_HASH, 
                NULL, URL_COMPONENT_FLAGS)));
}

HRESULT
CHyperlink::put_hash(BSTR str)
{
    RRETURN(SetErrorInfo(SetUrlComponent(str, URLCOMP_HASH)));
}

STDMETHODIMP
CHyperlink::get_href(BSTR * p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_WHOLE, 
                NULL, URL_COMPONENT_FLAGS)));
}

STDMETHODIMP
CHyperlink::put_href(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_WHOLE)));
}

//+-----------------------------------------------------------
//
//  Member  : GetUrlComponenet
//
//  Synopsis    : return a componenet of the href
//              the OM calls to this always fill in a BSTR and 
//                   a NULL ppchurl and require no processing, 
//                   merely the componenet spliting
//              internal calls to this are the opposite and 
//                   are processed (expamded, encoded and then split)
//              this functin is written so that it returns one
//                   or the other of these, but not both. I always
//                   test just one of the pair for consistency.
//              if you make changes to get/set/shortcut make sure to 
//                  make the changes to their clones in CAnchorElemnt
//-----------------------------------------------------------

HRESULT
CHyperlink::GetUrlComponent(BSTR     * pstrComp, 
                  URLCOMP_ID ucid, 
                  TCHAR   ** ppchUrl,
                  DWORD      dwFlags)
{
    HRESULT  hr = S_OK;
    TCHAR  * pchTheHref = (TCHAR*)GetUrl();
    TCHAR   cBuf[pdlUrlLen];
    TCHAR  * pchNewUrl  = cBuf;
    CDoc *   pDoc = Doc();

    // make sure we have at least one place to return a value
    Assert(!(pstrComp && ppchUrl));
    if (!pstrComp && !ppchUrl)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (ppchUrl)
        *ppchUrl = NULL;
    else
        *pstrComp = NULL;

    if (!pchTheHref)
        goto Cleanup;

    // get the expanded string 
    hr = THR(pDoc->ExpandUrl(pchTheHref, ARRAY_SIZE(cBuf), pchNewUrl, this, dwFlags));
    // don't bail out if OM set has occured and OM is asking for the component.
    if (hr && (!_fOMSetHasOccurred || ppchUrl))
        goto Cleanup;

    // if asking for whole thing, just set return param
    if (ucid == URLCOMP_WHOLE)
    {
        if (ppchUrl)
        {
            MemAllocString(Mt(CHyperlinkGetUrlComponent), pchNewUrl, ppchUrl);
        }
        else
        {
            *pstrComp = (_fOMSetHasOccurred) ? SysAllocString(pchTheHref) :
                               SysAllocString(pchNewUrl);
            hr = (!*pstrComp) ? E_OUTOFMEMORY : S_OK;
        }
    }
    else
    {
        // we want a piece, so split it up.
        CStr cstrComponent; 
        // we need to use TheHref when a set has happened. but when hash/search
        //  is requested we NEED to crack the url properly, so we need to use
        //  pchNewUrl.
        BOOL fUseTheHref = _fOMSetHasOccurred && ((ucid != URLCOMP_HASH) && 
                              (ucid != URLCOMP_SEARCH));

        // bail out if we have to use expanded Url, but the Combine failes and we are
        // here because an OM set has already occured and an OM get is happening now.
        if (!fUseTheHref && hr)
            goto Cleanup;

        if (!pchNewUrl || pchNewUrl[0]==NULL || 
            (_fOMSetHasOccurred && (!pchTheHref || pchTheHref[0]==NULL)))
            goto Cleanup;

        hr = THR(GetUrlComponentHelper((fUseTheHref ? pchTheHref: pchNewUrl), 
                        &cstrComponent, 
                        dwFlags, 
                        ucid));
        if (hr == E_FAIL)
        {
            hr = S_OK;
            goto Cleanup;
        }


        if (ppchUrl)
        {
            if (cstrComponent)
            {
                hr = THR(MemAllocString(Mt(CHyperlinkGetUrlComponent),
                            cstrComponent, ppchUrl));
            }
            else
                *ppchUrl = NULL;
        }
        else
        {
            hr = THR(cstrComponent.AllocBSTR(pstrComp));
        }

    }

Cleanup:

    RRETURN (hr);
}

//+-----------------------------------------------------------
//
//  Member  : SetUrlComponenet
//
//  Synopsis    : field the various component setting requests
//
//-----------------------------------------------------------

HRESULT
CHyperlink::SetUrlComponent(const BSTR bstrComp, URLCOMP_ID ucid)
{
    HRESULT     hr=S_OK;
    TCHAR       achUrl[pdlUrlLen];
    TCHAR     * pchTheHref = NULL;

    // if set_href, just set it
    if (ucid == URLCOMP_WHOLE)
    {
        hr = THR(SetUrl(bstrComp));
    }
    else
    {
        // get the old url
        hr = THR(GetUrlComponent(NULL, URLCOMP_WHOLE, &pchTheHref, 
                     ICU_DECODE));
        if (hr || !pchTheHref)
            goto Cleanup;

        // expand it if necessary
        if ((ucid != URLCOMP_HASH) && (ucid != URLCOMP_SEARCH))
        {
            // and set the appropriate component
            hr = THR(SetUrlComponentHelper(pchTheHref,
                           achUrl,
                           ARRAY_SIZE(achUrl),
                           &bstrComp,
                           ucid));
        }
        else
        {
            hr = THR(ShortCutSetUrlHelper(pchTheHref,
                       achUrl,
                       ARRAY_SIZE(achUrl),
                       &bstrComp,
                       ucid));
        }
        if (hr)
            goto Cleanup;

        hr = THR(SetUrl((BSTR)achUrl));
    }
                        

Cleanup:
    if (pchTheHref)
        MemFreeString(pchTheHref);

    RRETURN(hr);
}

HRESULT CHyperlink::ClickAction(CMessage *pmsg)
{
    HRESULT         hr = S_OK;

    // Disable this feature, because this breaks compat. with IE$, where
    // Shift+Click causes the navigation to occur in a new browser window.
#ifdef NEVER
    // Shift+Click should do 'SaveAs'
    if (pmsg && pmsg->message == WM_LBUTTONUP && (pmsg->dwKeyState & FSHIFT))
    {
        MSOCMD cmd;

        cmd.cmdID = IDM_SAVETARGET;
        cmd.cmdf  = 0;

        hr = QueryStatusHelper((GUID *)&CGID_MSHTML, 1, &cmd, NULL, TRUE);
        if (hr == S_OK && cmd.cmdf != 0 && cmd.cmdf != MSOCMDSTATE_DISABLED)
        {
            hr = ExecHelper((GUID *)&CGID_MSHTML, cmd.cmdID, 0, NULL, NULL, FALSE);
        }

    }
    else
#endif
    {
        const TCHAR *   pchUrl = GetUrl();

        // This used to not hyperlink if the HREF was "".  For compatibility
        // with Navigator, we need to hyperlink even in that case.  The only
        // time we don't want to hyperlink is if the HREF is not supplied at all.

        if (pchUrl)
        {
            CDoc *pDoc = Doc();

            Assert(pDoc);
            hr = THR(pDoc->FollowHyperlink(pchUrl, GetTarget(), this, NULL,
                               FALSE,
                               (pmsg && pmsg->message != WM_MOUSEWHEEL)
                                       ? !!(pmsg->dwKeyState & MK_SHIFT)
                                       : FALSE));

        }
    }
    RRETURN1(hr, S_FALSE);
}

//+--------------------------------------------------------------------------
//
//  Method :    CHyperlink::GetHyperlinkCursor
//
//  Synopsis :  Get cursor based on offline state and cache
//
//---------------------------------------------------------------------------

extern BOOL IsGlobalOffline();

LPTSTR
CHyperlink::GetHyperlinkCursor()
{
    if (!IsGlobalOffline())
        return MAKEINTRESOURCE(IDC_HYPERLINK);

    if (!_fAvailableOfflineValid)
    {
        _fAvailableOffline = Doc()->IsAvailableOffline(GetUrl(), this);
        _fAvailableOfflineValid = TRUE;
    }

    return _fAvailableOffline ? MAKEINTRESOURCE(IDC_HYPERLINK) : MAKEINTRESOURCE(IDC_HYPERLINK_OFFLINE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CHyperlink::QueryStatusHelper
//
//  Synopsis:   Implements QueryStatus for CHyperlink
//
//----------------------------------------------------------------------------

HRESULT
CHyperlink::QueryStatusHelper(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    int idm;

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *        pCmd    = &rgCmds[0];
    HRESULT         hr      = S_OK;
    const TCHAR *   pchUrl  = GetUrl();

    Assert(!pCmd->cmdf);

    idm = CBase::IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
    switch (idm)
    {
    case IDM_FOLLOWLINKC:
    case IDM_FOLLOWLINKN:
    case IDM_PRINTTARGET:
    case IDM_SAVETARGET:

        // Plug a ratings security hole.
        if ((idm == IDM_PRINTTARGET || idm == IDM_SAVETARGET) &&
            S_OK == AreRatingsEnabled())
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;
        }

        // Enable "Open->In Current Window"; enable "Open->In New Window"
        //  if protocol is not "mailto:"
        // Note: We don't need to call ExpandUrl() here
        if (pchUrl && (idm == IDM_FOLLOWLINKC ||
                        !_tcsnipre(_T("mailto:"), 7, pchUrl, -1)))
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    case IDM_ADDFAVORITES:
    case IDM_COPYSHORTCUT:
        if (pchUrl)
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    case IDM_CUT:
        // Enable if script wants to handle it, otherwise leave it to default
        if (!Fire_onbeforecut())
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    case IDM_COPY:
        // Enable if script wants to handle it, otherwise leave it to default
        if (!Fire_onbeforecopy())
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    case IDM_PASTE:
        // Enable if script wants to handle it, otherwise leave it to default
        if (!Fire_onbeforepaste())
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    }

    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHyperlink::ExecHelper
//
//  Synopsis:   Executes a command on the CHyperlink
//
//----------------------------------------------------------------------------

HRESULT
CHyperlink::ExecHelper(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    int             idm             = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);
    HRESULT         hr              = MSOCMDERR_E_NOTSUPPORTED;
    const TCHAR *   pchUrl          = GetUrl();
    TCHAR   cBuf[pdlUrlLen];
    TCHAR *         pchExpandedUrl  = cBuf;

    switch (idm)
    {
    case IDM_FOLLOWLINKC:
    case IDM_FOLLOWLINKN:
    case IDM_SAVETARGET:
    case IDM_PRINTTARGET:
    {
        if (pchUrl && !_tcsnipre(_T("mailto:"), 7, pchUrl, -1))
        {
            CDoc *  pDoc = Doc();

            if ((idm == IDM_PRINTTARGET) || (idm == IDM_SAVETARGET))
            {
                // Plug a ratings security hole.
                if (S_OK == AreRatingsEnabled())
                {
                    Assert(hr == MSOCMDERR_E_NOTSUPPORTED);
                    break;
                }

                hr = THR(pDoc->ExpandUrl(pchUrl, ARRAY_SIZE(cBuf), pchExpandedUrl, this));
                if (hr == S_OK)
                {
                    if (idm == IDM_PRINTTARGET)
                        hr = THR(pDoc->DoPrint(pchExpandedUrl));
                    else    // IDM_SAVETARGET
                        hr = DoFileDownLoad(pchExpandedUrl);
                    if (hr == S_OK)
                    {
                        IGNORE_HR(SetVisited());
                    }

                }
            }
            else
            {
                hr = THR(pDoc->FollowHyperlink(pchUrl,
                            GetTarget(),
                            this, NULL, FALSE,
                            idm == IDM_FOLLOWLINKN));
            }
        }
        break;
    }

    case IDM_COPYSHORTCUT:
        if (pchUrl)
            hr = THR(CopyLinkToClipboard());
        break;

    case IDM_ADDFAVORITES:
        if (pchUrl)
        {
            CStr strUrlTitle;
            CDoc *  pDoc = Doc();

            hr = THR(pDoc->ExpandUrl(pchUrl, ARRAY_SIZE(cBuf), pchExpandedUrl, this));
            if (hr)
                goto Cleanup;
            IGNORE_HR(GetUrlTitle(&strUrlTitle));
            hr = pDoc->AddToFavorites(pchExpandedUrl, strUrlTitle);
        }
        break;
    case IDM_CUT:
        if (!Fire_oncut())
            hr = S_OK;
        break;
    case IDM_COPY:
        if (!Fire_oncopy())
            hr = S_OK;
        break;
    case IDM_PASTE:
        if (!Fire_onpaste())
            hr = S_OK;
        break;
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}
