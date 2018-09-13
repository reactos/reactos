//+------------------------------------------------------------------------
//
//  File:       hlink.cxx
//
//  Contents:   CDoc hyperlinking support.
//              FollowHyperlink() for linking out
//              IHlinkTarget for linking in
//              ITargetFrame for frame targeting
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"    // for cbindtask
#endif

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include "hlink.h"        // for std hyperlink object
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"     // for ez hyperlink api
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"   // for url caching api
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"  // for _pcollectioncache
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for itargetframe, itargetembedding
#endif

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include "exdisp.h"     // for iwebbrowserapp
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include "urlhist.h"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_BOOKMARK_HXX_
#define X_BOOKMARK_HXX_
#include "bookmark.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx" // for IHtmlLoadOptions, et al..
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifdef WIN16
#ifndef X_REGMSG16_H_
#define X_REGMSG16_H_
#include <regmsg16.h> // for iexplore load
#endif

#ifndef X_URLMKI_H_
#define X_URLMKI_H_
#include "urlmki.h"
#endif
#endif

PerfDbgTag(tagNavigate, "Doc", "Measure FollowHyperlink");
MtDefine(HlinkBaseTarget, Locals, "Hlink (base target string)")
MtDefine(CTaskLookForBookmark, Utilities, "CTaskLookForBookmark")

#ifndef WIN16
EXTERN_C const IID IID_IWebBrowserApp;

extern IMultiLanguage *g_pMultiLanguage;

///////////////////////////////////////

DYNLIB g_dynlibSHDOCVW = { NULL, NULL, "SHDOCVW.DLL" };

#define WRAPIT_SHDOCVW(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibSHDOCVW, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

WRAPIT_SHDOCVW(HlinkFrameNavigate,
    (DWORD                   grfHLNF,
    LPBC                    pbc,
    IBindStatusCallback *   pibsc,
    IHlink *                pihlNavigate,
    IHlinkBrowseContext *   pihlbc),
    (grfHLNF, pbc, pibsc, pihlNavigate, pihlbc))

WRAPIT_SHDOCVW(HlinkFrameNavigateNHL,
    (DWORD grfHLNF,
    LPBC pbc,
    IBindStatusCallback *pibsc,
    LPCWSTR pszTargetFrame,
    LPCWSTR pszUrl,
    LPCWSTR pszLocation),
    (grfHLNF, pbc, pibsc, pszTargetFrame, pszUrl, pszLocation))

WRAPIT_SHDOCVW(HlinkFindFrame,
    (LPCWSTR pszFrameName,
    LPUNKNOWN *ppunk),
    (pszFrameName, ppunk))
#endif

#pragma warning(disable:4706)   // assignment within conditional expression.

BOOL
IsSpecialUrl(TCHAR *pchURL)
{
    UINT      uProt;
    uProt = GetUrlScheme(pchURL);
    return (URL_SCHEME_JAVASCRIPT == uProt || 
            URL_SCHEME_VBSCRIPT == uProt ||
            URL_SCHEME_ABOUT == uProt);
}

HRESULT
WrapSpecialUrl(TCHAR *pchURL, CStr *pcstrExpandedUrl, CStr &cstrDocUrl, BOOL fNonPrivate, BOOL fIgnoreUrlScheme)
{
    HRESULT   hr = S_OK;
    CStr      cstrSafeUrl;
    TCHAR   * pch, * pchPrev;
    TCHAR     achUrl[pdlUrlLen];
    DWORD     dwSize;

    if (IsSpecialUrl(pchURL) || fIgnoreUrlScheme)
    {
        //
        // If this is javascript:, vbscript: or about:, append the
        // url of this document so that on the other side we can
        // decide whether or not to allow script execution.
        //

        // QFE 2735 (Georgi XDomain): [alanau]
        //
        // If the special URL contains an %00 sequence, then it will be converted to a Null char when
        // encoded.  This will effectively truncate the Security ID.  For now, simply disallow this
        // sequence, and display a "Permission Denied" script error.
        //
        if (_tcsstr(pchURL, _T("%00")))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        // Copy the URL so we can munge it.
        //
        cstrSafeUrl.Set(pchURL);

        // someone could put in a string like this:
        //     %2501 OR %252501 OR %25252501
        // which, depending on the number of decoding steps, will bypass security
        // so, just keep decoding while there are %s and the string is getting shorter

        UINT uPreviousLen = 0;
        while ((uPreviousLen != cstrSafeUrl.Length()) && _tcschr(cstrSafeUrl, _T('%')))
        {
            uPreviousLen = cstrSafeUrl.Length();
            int nNumPercents;
            int nNumPrevPercents = 0;

            // Reduce the URL
            //
            for (;;)
            {
                // Count the % signs.
                //
                nNumPercents = 0;

                pch = cstrSafeUrl;
                while (pch = _tcschr(pch, _T('%')))
                {
                    pch++;
                    nNumPercents++;
                }

                // If the number of % signs has changed, we've reduced the URL one iteration.
                //
                if (nNumPercents != nNumPrevPercents)
                {
                    // Encode the URL 
                    hr = THR(CoInternetParseUrl(cstrSafeUrl, 
                        PARSE_ENCODE, 
                        0, 
                        achUrl, 
                        ARRAY_SIZE(achUrl), 
                        &dwSize,
                        0));

                    cstrSafeUrl.Set(achUrl);

                    nNumPrevPercents = nNumPercents;
                }
                else
                {
                    // The URL is fully reduced.  Break out of loop.
                    //
                    break;
                }
            }
        }

        // Now scan for '\1' characters.
        //
        if (_tcschr(cstrSafeUrl, _T('\1')))
        {
            // If there are '\1' characters, we can't guarantee the safety.  Put up "Permission Denied".
            //
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        hr = THR(pcstrExpandedUrl->Set(cstrSafeUrl));
        if (hr)
            goto Cleanup;

        hr = THR(pcstrExpandedUrl->Append( fNonPrivate ? _T("\1\1") : _T("\1")));
        if (hr)
            goto Cleanup;


        // Now copy the cstrDocUrl
        //
        cstrSafeUrl.Set(cstrDocUrl);


        // Scan the URL to ensure it appears un-spoofed.
        //
        // There may legitimately be multiple '\1' characters in the URL.  However, each one, except the last one
        // should be followed by a "special" URL (javascript:, vbscript: or about:).
        //
        pchPrev = cstrSafeUrl;
        pch = _tcschr(cstrSafeUrl, _T('\1'));
        while (pch)
        {
            pch++;                              // Bump past security marker
            if (*pch == _T('\1'))               // (Posibly two security markers)
                pch++;
                
            if (!IsSpecialUrl(pchPrev))         // If URL is not special
            {
                hr = E_ACCESSDENIED;            // then it's spoofed.
                goto Cleanup;
            }
            pchPrev = pch;
            pch = _tcschr(pch, _T('\1'));
        }

        // Look for escaped %01 strings in the Security Context.
        //
        pch = cstrSafeUrl;
        while (pch = _tcsstr(pch, _T("%01")))
        {
            pch[2] = _T('2');  // Just change the %01 to %02.
            pch += 3;          // and skip over
        }

        hr = THR(pcstrExpandedUrl->Append(cstrSafeUrl));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pcstrExpandedUrl->Set(pchURL));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

#pragma warning(default:4706)   // assignment within conditional expression.

#ifdef WIN16_NEVER
    // no longer used

// hacky new HlinkSimpleNavigateToMoniker.
// Who on earth designed this thing in the first place?
// Monikers are for data!

#undef HlinkSimpleNavigateToMoniker

HRESULT HlinkSimpleNavigateToMoniker(IMoniker *pmkTarget,
                                     LPCWSTR szLocation,
                                     LPCWSTR szAddParams,
                                     IUnknown *pUnk,
                                     IBindCtx *pbc,
                                     IBindStatusCallback *pbsc,
                                     DWORD grfHLNF,
                                     DWORD dwReserved,
                                     CDoc * pdoc)
{
    IMoniker        * pUrlMk = pmkTarget;
    IPersistMoniker * pPMk;
    IOleObject      * pObject = NULL;
    HRESULT         hr;
    BIND_OPTS       bindopts;
    DWORD           grfMode;


    if (!pbc)
    {
        hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pbc, 0));
        if (hr)
            goto Cleanup;
    }



    hr = THR(pdoc->QueryInterface(IID_IOleObject, (void **)&pObject));
    if (hr)
        goto Cleanup;

    hr = THR(pObject->QueryInterface(IID_IPersistMoniker, (void **)&pPMk));

    if (hr)
        goto Cleanup;

    bindopts.cbStruct = sizeof(BIND_OPTS);
    hr = pbc->GetBindOptions(&bindopts);
    if (SUCCEEDED(hr))
    {
        grfMode = bindopts.grfMode;
    }
    else
    {
        grfMode = STGM_READ;
    }

    hr = THR(pPMk->Load(FALSE, pUrlMk, pbc, grfMode));
    if (hr)
        goto Cleanup;

Cleanup:
    pbc->Release();
    //ReleaseInterface(pPMk);
    //ReleaseInterface(pObject);
    RRETURN(hr);
}


#endif // WIN16

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FollowHyperlink
//
//  Synopsis:   Hyperlinks the specified target frame to the requested url
//              using ITargetFrame/IHyperlinkTarget of host if possible
//
//  Arguments:  pchURL:           the relative URL
//              pchTarget:        the target string
//              fOpenInNewWindow: TRUE to open in a new window
//              pUnkFrame:        the target frame's IUnknown, used when
//                                directly setting the SRC on a frame
//              dwBindOptions:    options to use while binding, used to
//                                control refresh for frames
//              dwSecurityCode    Used to give the appropriate alert when
//                                sending data over the network.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::FollowHyperlink(LPCTSTR           pchURL,
                      LPCTSTR           pchTarget,
                      CElement *        pElementContext,
                      CDwnPost *        pDwnPost,
                      BOOL              fSendAsPost,
                      BOOL              fOpenInNewWindow,
                      IUnknown *        pUnkFrame,
                      DWORD             dwBindf,
                      DWORD             dwSecurityCode)
{
    PerfDbgLog1(tagNavigate, this, "+CDoc::FollowHyperlink \"%ls\"", pchURL);

    TCHAR *         pchBaseTarget       = NULL;
    TCHAR *         pchTargetAlias      = NULL;
    IHlinkFrame *   pHlinkFrameTarget   = NULL;
    IBindCtx *      pBindCtx            = NULL;
    CDwnBindInfo *  pDwnBindInfo        = NULL;
    BOOL            fOpenWithFrameName  = FALSE;
    BOOL            fProtocolNavigates  = TRUE;
    CStr            cstrExpandedUrl;
    CStr            cstrLocation;
    HRESULT         hr;

    // these two arguments are exclusive
    Assert(!pUnkFrame || !pchTarget);

    // this flag is always set when binding through this API
    dwBindf |= BINDF_HYPERLINK;

    if (pDwnPost)
    {
        // this flag is set when posting through this API, no matter if
        // we ultimately do a POST or a GET
        dwBindf |= BINDF_FORMS_SUBMIT;
    }

    // Determine the expanded url and location.
    hr = THR( DetermineExpandedUrl(
        pchURL,
        pElementContext,
        pDwnPost,
        fSendAsPost,
        (pUnkFrame) ? (TRUE) : (FALSE),
        dwSecurityCode,
        &cstrExpandedUrl,
        &cstrLocation,
        &fProtocolNavigates) );

    if (hr)
        goto Cleanup;

    // Determine pHlinkFrameTarget.
    hr = THR( DetermineHyperlinkFrameTarget(
        pUnkFrame,
        pElementContext,
        &cstrExpandedUrl,
        fProtocolNavigates,
        &pHlinkFrameTarget,
        &pchTarget,
        &pchBaseTarget,
        &pchTargetAlias,
        &fOpenInNewWindow,
        &fOpenWithFrameName,
        &dwBindf) );

    if (hr)
        goto Cleanup;

    // Set up the bind info + context.
    hr = THR( SetupDwnBindInfoAndBindCtx(
        cstrExpandedUrl,
        pchTarget,
        pElementContext,
        pDwnPost,
        fSendAsPost,
        fProtocolNavigates,
        !!pUnkFrame,
        &dwBindf,
        &pDwnBindInfo,
        &pBindCtx) );

    if (hr)
        goto Cleanup;

    // Finally, navigate to the URL.
    hr = THR( DoNavigate(
        &cstrExpandedUrl,
        &cstrLocation,
        pHlinkFrameTarget,
        pDwnBindInfo,
        pBindCtx,
        pchURL,
        pchTarget,
        pUnkFrame,
        fOpenInNewWindow,
        fOpenWithFrameName,
        fProtocolNavigates) );

Cleanup:

    MemFreeString(pchBaseTarget);
    CoTaskMemFree(pchTargetAlias);

    ReleaseInterface(pHlinkFrameTarget);
    ReleaseInterface(pBindCtx);

    if (pDwnBindInfo)
        pDwnBindInfo->Release();

    if (S_FALSE == hr)
        hr = S_OK;

    PerfDbgLog(tagNavigate, this, "-CDoc::FollowHyperlink");

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DetermineExpandedUrl
//
//  Synopsis:   determines the extended url
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::DetermineExpandedUrl(LPCTSTR           pchURL,
                           CElement *        pElementContext,
                           CDwnPost *        pDwnPost,
                           BOOL              fSendAsPost,
                           BOOL              fFrameNavigate,
                           DWORD             dwSecurityCode,
                           CStr *            pcstrExpandedUrl,
                           CStr *            pcstrLocation,
                           BOOL *            pfProtocolNavigates)
{
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchExpandedUrl = cBuf;
    UINT    uProt;
    HRESULT hr;
    SSL_SECURITY_STATE sss;
    SSL_PROMPT_STATE sps;


    Assert(pcstrExpandedUrl && pcstrLocation && pfProtocolNavigates);

    // fully resolve URL
    hr = THR(ExpandUrl(pchURL, ARRAY_SIZE(cBuf), pchExpandedUrl, pElementContext));
    if (hr)
        goto Cleanup;

    // pass on security state
    GetRootSslState(&sss, &sps);
        
    // Opaque URL?
    if (!UrlIsOpaque(pchExpandedUrl))
    {
        LPTSTR pch = (LPTSTR)UrlGetLocation(pchExpandedUrl);
        // Bookmark?
        if (pch)
        {
            // Yes (But is it really a bookmark).
            Assert(*pch == _T('#'));

            hr = THR(pcstrLocation->Set(pch));
            if (hr)
                goto Cleanup;

            // So remove bookmark from expanded URL, remember bookmark has been
            // copied to cstrLocation.
            *pch = _T('\0');
        }

        // chop of '?' part if we are going to append '?'
        // TODO: use UrlGetQuery instead of searching for '?' ourselves
        if (!fSendAsPost && pDwnPost)
        {
            pch = _tcschr(pchExpandedUrl, _T('?'));
            if (pch)
                *pch = _T('\0');
        }

        hr = THR(pcstrExpandedUrl->Set(pchExpandedUrl));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(WrapSpecialUrl(
                (TCHAR *)pchExpandedUrl,
                pcstrExpandedUrl,
                _cstrUrl,
                sss <= SSL_SECURITY_MIXED,
                FALSE));
        if (hr)
            goto Cleanup;
    }

    // Check for security violation, of sending (POSTING) data
    // to a server without a secure channel protocol (SSL/PCT).

    uProt = GetUrlScheme(*pcstrExpandedUrl);
    if (pDwnPost && URL_SCHEME_HTTPS != uProt)
    {
        // warn when submitting over a nonsecure connection
        if (dwSecurityCode)
        {
            DWORD   dwPolicyTo;
            DWORD   dwPolicyFrom;
            BOOL    fAllow;

            // step 1: silently check if form submission is allowed or should be queried

            hr = THR(ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_TO,
                                      &fAllow, PUAF_NOUI | PUAF_WARN_IF_DENIED, &dwPolicyTo, *pcstrExpandedUrl));

            if (hr || GetUrlPolicyPermissions(dwPolicyTo) == URLPOLICY_DISALLOW)
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            hr = THR(ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_FROM,
                                      &fAllow, PUAF_NOUI | PUAF_WARN_IF_DENIED, &dwPolicyFrom));

            if (hr || GetUrlPolicyPermissions(dwPolicyFrom) == URLPOLICY_DISALLOW)
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            // step 2: if needed and allowed, query the user once, giving precedence to "To"
            // if this is a mailto, we ALWAYS pop up our security alert dialog

            if (URL_SCHEME_MAILTO == uProt)
            {
                int     nResult;

                hr = ShowMessage(&nResult,
                                        MB_OKCANCEL | MB_ICONWARNING, 0,
                                        IDS_MAILTO_SUBMITALERT);
                if (hr || nResult != IDOK)
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }
            }
            else if (GetUrlPolicyPermissions(dwPolicyTo) == URLPOLICY_QUERY)
            {
                hr = THR(ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_TO,
                                          &fAllow, 0, NULL, *pcstrExpandedUrl));

                if (hr || !fAllow)
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }
            }

            else if (GetUrlPolicyPermissions(dwPolicyFrom) == URLPOLICY_QUERY)
            {
                hr = THR(ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_FROM, &fAllow));

                if (hr || !fAllow)
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }
            }

            // If we make it to here, it's allowed
        }
    }

    // tack on the get data if needed
    if (pDwnPost && !fSendAsPost && pDwnPost->GetItemCount() > 0)
    {
        CPostItem * pPostItem = pDwnPost->GetItems();

        if (pPostItem->_ePostDataType == POSTDATA_LITERAL)
        {
            UINT    cp          = NavigatableCodePage(GetCodePage());
            int     cchPrefix   = pcstrExpandedUrl->Length() + 1; // len + '?'
            LPSTR   pszPost     = pPostItem->_pszAnsi;
            UINT    cbPost      = pszPost ? strlen(pszPost) : 0;
            UINT    cchPost;
            CStr    cstrT;

            Assert(g_pMultiLanguage);
            hr = THR(g_pMultiLanguage->ConvertStringToUnicode(
                        NULL,
                        cp,
                        pszPost,
                        &cbPost,
                        NULL,
                        &cchPost));
            if (FAILED(hr))
                goto Cleanup;

            // cchPost == 0 means the conversion failed
            Assert(cchPost > 0 || cbPost == 0);

            hr = THR(cstrT.Set(NULL, cchPrefix + cchPost + 1));
            if (FAILED(hr))
               goto Cleanup;

            _tcscpy(cstrT, *pcstrExpandedUrl);
            _tcscat(cstrT, _T("?"));
            hr = THR(g_pMultiLanguage->ConvertStringToUnicode(
                        NULL,
                        cp,
                        pszPost,
                        &cbPost,
                        (TCHAR *)cstrT + cchPrefix,
                        &cchPost));
            if (hr)
                goto Cleanup;

            cstrT[cchPrefix + cchPost] = 0;

            hr = THR(pcstrExpandedUrl->Set(cstrT));
            if (hr)
                goto Cleanup;
        }
    }

    // MHTML hook for Athena.
    if (_pHostUIHandler && !fFrameNavigate)
    {
        OLECHAR *pchURLOut = NULL;

        hr = _pHostUIHandler->TranslateUrl(0, *pcstrExpandedUrl, &pchURLOut);

        if (S_OK == hr
            && pchURLOut && _tcslen(pchURLOut))
        {
            // Replace the URL with the one we got back from Athena.
            pcstrExpandedUrl->Set(pchURLOut);
            CoTaskMemFree(pchURLOut);
        }
        else if (E_ABORT == hr)
        {
            // If we get back E_ABORT, it means Athena is taking over and we bail.
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    {
        DWORD dwNavigate, dwDummy;

        // Are we navigating? (mailto protocol)
        hr = THR( CoInternetQueryInfo(
            *pcstrExpandedUrl,
            QUERY_CAN_NAVIGATE,
            0,
            (LPVOID)&dwNavigate,
            4,
            &dwDummy,
            0) );

        if (!hr && !dwNavigate)
        {
            *pfProtocolNavigates = FALSE;
        }

        hr = S_OK;
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DetermineHyperlinkFrameTarget
//
//  Synopsis:   determines the hyperlink frame target
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::DetermineHyperlinkFrameTarget(IUnknown *        pUnkFrame,
                                    CElement *        pElementContext,
                                    CStr *            pcstrExpandedUrl,
                                    BOOL              fProtocolNavigates,
                                    IHlinkFrame **    ppHlinkFrameTarget,
                                    LPCTSTR *         ppchTarget,
                                    TCHAR **          ppchBaseTarget,
                                    TCHAR **          ppchTargetAlias,
                                    BOOL *            pfOpenInNewWindow,
                                    BOOL *            pfOpenWithFrameName,
                                    DWORD *           pdwBindf)
{
    IUnknown *      pUnkTarget = NULL;
    HRESULT         hr = S_OK;

    Assert(pcstrExpandedUrl && ppHlinkFrameTarget && ppchTarget && ppchBaseTarget && ppchTargetAlias && pfOpenInNewWindow && pfOpenWithFrameName);

    // Try to find an hlink target frame in the following order:

    // 1. If a target frame was passed in, use that one.
    if (pUnkFrame)
    {
        pUnkTarget = pUnkFrame;
        pUnkTarget->AddRef();

        hr = THR( HlinkFrameTargetFromUnkTarget(pUnkFrame, pUnkTarget, pcstrExpandedUrl, ppHlinkFrameTarget, pdwBindf) );

        if (hr == E_ABORT)
            goto Cleanup;
    }

    // 2. Try to find a frame using ITargetFrame2.
    if (!(*ppHlinkFrameTarget))
    {
        ITargetFrame2 *     pTargetFrameSource = NULL;

        //  If we don't have a specific target, check if one is defined in a base
        if (*ppchTarget == NULL || **ppchTarget == 0)
        {
            hr = THR(GetBaseTarget(ppchBaseTarget, pElementContext));
            if (hr)
                goto FrameSearchCleanup;
            *ppchTarget = *ppchBaseTarget;
        }

        hr = THR( QueryService(IID_ITargetFrame2, IID_ITargetFrame2, (void**)&pTargetFrameSource) );
        if (hr)
            goto FrameSearchCleanup;

        hr = pTargetFrameSource->GetTargetAlias(*ppchTarget, ppchTargetAlias);

        if (!hr && *ppchTargetAlias)
            *ppchTarget = *ppchTargetAlias;

        if (((GetRootDoc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_OPENNEWWIN) != 0)
             && (!(*ppchTarget) || !(**ppchTarget)))
        {
            MemFreeString(*ppchBaseTarget);
            *ppchBaseTarget = NULL;

            hr = THR(MemAllocString(Mt(HlinkBaseTarget), _T("_unspecifiedFrame"), ppchBaseTarget));
            if (hr)
                goto Cleanup;

            *ppchTarget = *ppchBaseTarget;
        }

        hr = TFAIL(-1,pTargetFrameSource->FindFrame(*ppchTarget && **ppchTarget ? *ppchTarget : NULL,
                                                 FINDFRAME_JUSTTESTEXISTENCE,
                                                 &pUnkTarget));
        if (!SUCCEEDED(hr))
            goto FrameSearchCleanup;

        // Perform a cross-domain check on the pUnkTarget we've received.
        //
        if (!_fTrustedDoc &&  *ppchTarget 
            &&  *ppchTarget[0]
            &&  StrCmpIC(*ppchTarget, _T("_self")) != 0
            &&  StrCmpIC(*ppchTarget, _T("_parent")) != 0
            &&  StrCmpIC(*ppchTarget, _T("_blank")) != 0
            &&  StrCmpIC(*ppchTarget, _T("_top")) != 0
            &&  StrCmpIC(*ppchTarget, _T("_main")) != 0
            &&  pUnkTarget)
        {
            IWebBrowser *       pWB = NULL;
            IDispatch *         pDispDoc = NULL;
            IHTMLDocument2 *    pDocParent = NULL;
            BSTR                bstrURL = NULL;

            // QI for IWebBrowser to determine frameness.
            //
            hr = THR(pUnkTarget->QueryInterface(IID_IWebBrowser, (void**) &pWB));
            if (hr)
                goto DomainCheckCleanup;

            // Get the IDispatch of the container.  This is the containing Trident if a frameset.
            //
            hr = THR(pWB->get_Container(&pDispDoc));
            if (hr || !pDispDoc)
                goto DomainCheckCleanup;
    
            // QI the container for IHTML2Document
            //
            hr = THR(pDispDoc->QueryInterface(IID_IHTMLDocument2, (void**) &pDocParent));
            if (hr)
                goto DomainCheckCleanup;

            // Get the URL
            //
            hr = THR(pDocParent->get_URL(&bstrURL));
            if (hr || !bstrURL)
                goto DomainCheckCleanup;

            // The target frame has a Trident parent.  Perform a cross-domain check on it.
            //
            if (!AccessAllowed(bstrURL))
            {
                // Domain check FAILS!  Reject this target frame.
                //
                BOOL fAllow;

                if (THR(ProcessURLAction(URLACTION_HTML_SUBFRAME_NAVIGATE, &fAllow)) || !fAllow)
                {
                    ReleaseInterface(pUnkTarget);
                    pUnkTarget = NULL;
                }
            }

DomainCheckCleanup:
            ReleaseInterface(pWB);
            ReleaseInterface(pDispDoc);
            ReleaseInterface(pDocParent);
            if (bstrURL)
                SysFreeString(bstrURL);
        }

        if (!pUnkTarget && *ppchTarget && **ppchTarget)
        {
            *pfOpenWithFrameName = TRUE;
            *pfOpenInNewWindow = TRUE;
        }

        if ( pUnkTarget )
        {
            hr = THR( HlinkFrameTargetFromUnkTarget(pUnkFrame, pUnkTarget, pcstrExpandedUrl, ppHlinkFrameTarget, pdwBindf) );
            if (hr == E_ABORT)
                goto Cleanup;
        }

    FrameSearchCleanup:
        ReleaseInterface(pTargetFrameSource);
    }

    // If we are not navigating, don't bother looking (further) for an hlinkframetarget.
    if (!fProtocolNavigates)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // 3. QS
    if (!(*ppHlinkFrameTarget))
    {
        IGNORE_HR(QueryService(SID_SHlinkFrame, IID_IHlinkFrame, (LPVOID*)ppHlinkFrameTarget));
    }

    // 4. Try the inplace frame.
    if (!(*ppHlinkFrameTarget))
    {
        if (_pInPlace && _pInPlace->_pFrame)
        {
            IGNORE_HR( _pInPlace->_pFrame->QueryInterface(IID_IHlinkFrame, (void **)ppHlinkFrameTarget) );
        }
    }

    // 5. Try magic ShDocVw function
    if (!(*ppHlinkFrameTarget))
    {
        // If we have no target frame name, come up with one, so all the links point to
        // the same frame.
        if (!(*ppchTarget) || !(**ppchTarget))
        {
            MemFreeString(*ppchBaseTarget);
            *ppchBaseTarget = NULL;

            hr = THR(MemAllocString(Mt(HlinkBaseTarget), _T("_unspecifiedFrame"), ppchBaseTarget));
            if (hr)
                goto Cleanup;

            *ppchTarget = *ppchBaseTarget;
        }

        *pfOpenWithFrameName = TRUE;

#ifdef WIN16
        Assert(0); // BUGWIN16: Need to implement in shdocvw16
#else
        hr = THR( HlinkFindFrame(*ppchTarget, &pUnkTarget) );
#endif

        if (!hr && pUnkTarget)
        {
            hr = THR( HlinkFrameTargetFromUnkTarget(pUnkFrame, pUnkTarget, pcstrExpandedUrl, ppHlinkFrameTarget, pdwBindf) );

            if (hr == E_ABORT)
                goto Cleanup;
        }
    }

    hr = S_OK;

Cleanup:

    ReleaseInterface(pUnkTarget);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::HlinkFrameTargetFromUnkTarget
//
//  Synopsis:   returns the IHlinkFrame given the pUnkTarget
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::HlinkFrameTargetFromUnkTarget(IUnknown *     pUnkFrame,
                                    IUnknown *     pUnkTarget,
                                    CStr *         pcstrExpandedUrl,
                                    IHlinkFrame ** ppHlinkFrameTarget,
                                    DWORD *        pdwBindf)
{
    IServiceProvider  * pServiceTarget = NULL;
    HRESULT hr = S_OK;

    Assert(pUnkTarget);

    if (pUnkFrame)
    {
        // If frame was passed in explicitly, we're in the subframe loading case
        if (AllowFrameUnsecureRedirect())
        {
            *pdwBindf |= BINDF_IGNORESECURITYPROBLEM;
        }
    }
    else
    {
        // validate security of targeted hyperlink (bug 20907) if pUnkFrame was not passed explicitly
        if (IsFrameInsideWindow(pUnkTarget))
        {
            // reprompt if user previously said no to mixed security
            if (!ValidateSecureUrl(*pcstrExpandedUrl, TRUE))
            {
                hr = E_ABORT;
                goto Cleanup;
            }
        }
    }

    hr = THR(pUnkTarget->QueryInterface(IID_IServiceProvider, (void**)&pServiceTarget));
    if (!hr)
    {
        // BUGBUG: SID should be SID_SHlinkFrame instead of IID_IShellBrowser
        hr = THR(pServiceTarget->QueryService(IID_IShellBrowser, IID_IHlinkFrame,(void**)ppHlinkFrameTarget));
    }

    //hr = S_OK;

Cleanup:

    ReleaseInterface(pServiceTarget);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetupDwnBindInfoAndBindCtx
//
//  Synopsis:   creates and sets up the bind task
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::SetupDwnBindInfoAndBindCtx(LPCTSTR           pchExpandedUrl,
                                 LPCTSTR           pchTarget,
                                 CElement *        pElementContext,
                                 CDwnPost *        pDwnPost,
                                 BOOL              fSendAsPost,
                                 BOOL              fProtocolNavigates,
                                 BOOL              fFrameLoad,
                                 DWORD *           pdwBindf,
                                 CDwnBindInfo **   ppDwnBindInfo,
                                 IBindCtx **       ppBindCtx)
{
    CDwnDoc * pDwnDoc = NULL;
    HRESULT hr = S_OK;
    DWORD dwOfflineFlag;

    Assert(pdwBindf && ppDwnBindInfo && ppBindCtx);

    *ppDwnBindInfo = new CDwnBindInfo;

    if (!*ppDwnBindInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    MemSetName((*ppDwnBindInfo, "DwnBindInfo %ls %ls", pchExpandedUrl,
        pchTarget ? pchTarget : _T("")));

    pDwnDoc = new CDwnDoc;

    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    (*ppDwnBindInfo)->SetDwnDoc(pDwnDoc);

    // The referer of the new document is the same as the sub referer
    // of this document.  Which should be the same as _cstrUrl, by the way.

    if (_pDwnDoc && _pDwnDoc->GetSubReferer())
    {
        hr = THR(pDwnDoc->SetDocReferer(_pDwnDoc->GetSubReferer()));
        if (hr)
            goto Cleanup;
    }

    // The referer of items within the new document is the fully expanded
    // URL which we are hyperlinking to.

    hr = THR(pDwnDoc->SetSubReferer(pchExpandedUrl));
    if (hr)
        goto Cleanup;

    // Tell CDwnBindInfo that this is going to be a document binding.  That
    // lets it pick the correct referer to send in the HTTP headers.

    (*ppDwnBindInfo)->SetIsDocBind();

    // Set the accept language header if one was specified.
    if (_pOptionSettings->fHaveAcceptLanguage)
    {
        hr = THR(pDwnDoc->SetAcceptLanguage(_pOptionSettings->cstrLang));
        if (hr)
            goto Cleanup;
    }

    hr = THR(pDwnDoc->SetUserAgent(_bstrUserAgent));
    if (hr)
        goto Cleanup;

    // Post data is passed in
    if (pDwnPost && fSendAsPost)
    {
        (*ppDwnBindInfo)->SetDwnPost(pDwnPost);
    }

    IsFrameOffline(&dwOfflineFlag);
    *pdwBindf |= dwOfflineFlag;

    // Now set up bind context.
    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, ppBindCtx, 0));
    if (hr)
        goto Cleanup;

    pDwnDoc->SetBindf(*pdwBindf);
    pDwnDoc->SetDocBindf(*pdwBindf);
    if ( IsCpAutoDetect() )
        pDwnDoc->SetDocCodePage(CP_AUTO);
    else
        pDwnDoc->SetDocCodePage(
            NavigatableCodePage(_pOptionSettings->codepageDefault));
    pDwnDoc->SetURLCodePage(
        NavigatableCodePage(_codepage));

    pDwnDoc->SetLoadf(_dwLoadf & (DLCTL_URL_ENCODING_DISABLE_UTF8 | DLCTL_URL_ENCODING_ENABLE_UTF8));
    // BUGBUG (lmollico): This is for IE5 #52877. Maybe we should just set _dwLoadf completely.


    // Set up HtmlLoadOptions
    if (!fFrameLoad || (_pShortcutUserData && *_cstrShortcutProfile))
    {
        COptionArray *phlo = new COptionArray(IID_IHtmlLoadOptions);

        if (SUCCEEDED(hr) && phlo)
        {
            if (!fFrameLoad)
            {
                BOOL fHyperlink = TRUE;
                
                phlo->SetOption(HTMLLOADOPTION_HYPERLINK, &fHyperlink, sizeof(fHyperlink));
            }
            
            if (_pShortcutUserData && *_cstrShortcutProfile)
            {
                VARIANT varName;

                V_VT(&varName) = VT_BSTR;
                _cstrShortcutProfile.AllocBSTR(&V_BSTR(&varName));

                // deliberately ignore failures here
                if (V_BSTR(&varName))
                {
                    phlo->SetOption(HTMLLOADOPTION_INETSHORTCUTPATH,
                                V_BSTR(&varName),
                                (lstrlenW(V_BSTR(&varName)) + 1)*sizeof(WCHAR));
                    VariantClear(&varName);
                }
            }

            hr = THR((*ppBindCtx)->RegisterObjectParam(SZ_HTMLLOADOPTIONS_OBJECTPARAM,
                                                   (IOptionArray *)phlo));
            phlo->Release();
        }

        if (hr)
            goto Cleanup;
    }


    hr = THR((*ppBindCtx)->RegisterObjectParam(SZ_DWNBINDINFO_OBJECTPARAM,
                (IBindStatusCallback *)*ppDwnBindInfo));
    if (hr)
        goto Cleanup;

Cleanup:

    if (pDwnDoc)
        pDwnDoc->Release();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DoNavigate
//
//  Synopsis:   navigates to a url for FollowHyperlink
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::DoNavigate(CStr *            pcstrInExpandedUrl,
                 CStr *            pcstrLocation,
                 IHlinkFrame *     pHlinkFrameTarget,
                 CDwnBindInfo *    pDwnBindInfo,
                 IBindCtx *        pBindCtx,
                 LPCTSTR           pchURL,
                 LPCTSTR           pchTarget,
                 IUnknown *        pUnkFrame,
                 BOOL              fOpenInNewWindow,
                 BOOL              fOpenWithFrameName,
                 BOOL              fProtocolNavigates)
{
    IHlink *                pHlink              = NULL;
    IMoniker *              pMoniker            = NULL;
    ITargetFramePriv *      pTargetFramePriv    = NULL;
    DWORD                   grfHLNF;
    HRESULT                 hr                  = S_OK;
    ULONG                   cDie                = _cDie;
    CStr                    cstrExpandedUrl;

    Assert(pcstrInExpandedUrl && pcstrLocation);

    // If the URL is a reference to a bookmark in this CDoc, and
    // we are following an OE hyperlink (pchTarget != NULL and
    // pchTarget == "_unspecifiedFrame")
    if (   pcstrLocation->Length() 
        && (  !pcstrInExpandedUrl->Length()
            || S_OK == CoInternetCompareUrl(_cstrUrl, *pcstrInExpandedUrl, 1 /* = CF_INGNORE_SLASH in urlmon */) 
           )
        && ( pchTarget && !_tcscmp(_T("_unspecifiedFrame"), pchTarget))
       )
    {
        hr = THR( NavigateHere(0, *pcstrLocation, 0, TRUE) );

        // if success, done
        if (!hr)
            goto Cleanup;
    }

// BUGBUG: ***TLL*** Temporarily disabled and reactivated 55100 this fix caused breakage HOT bug # 65764    
/*
    // Fix security bug #55100
    if (URL_SCHEME_RES == GetUrlScheme(*pcstrInExpandedUrl))
    {
        hr = THR(WrapSpecialUrl((TCHAR *)*pcstrInExpandedUrl,
                            &cstrExpandedUrl,
                            _cstrUrl,
                            FALSE,
                            TRUE));
        if (hr)
            goto Cleanup;
    }
    else
*/
    {
        hr = cstrExpandedUrl.Set(*pcstrInExpandedUrl);
        if (hr)
            goto Cleanup;
    }

    //  see if HLinkFrame supports ITargetFramePriv for HLink free navigation
    if (pHlinkFrameTarget)
    {
        IGNORE_HR(pHlinkFrameTarget->QueryInterface(IID_ITargetFramePriv, (LPVOID *)&pTargetFramePriv));
    }

    // hyperlink, either using the full or simple interface

    // create hlink, bindctx, and bindstatuscallback
    // BUGBUG: IE 3 implements an empty hlink site; we pass NULL.

    // BUGBUG: need to fill in grfHLNF as appropriate

    // don't put initial frameset loads in history folder
    // NOTE: this is not the same as navigation history
    grfHLNF = (pUnkFrame && pTargetFramePriv) ? SHHLNF_WRITENOHISTORY : 0;

    if (fOpenInNewWindow)
    {
        grfHLNF |= HLNF_OPENINNEWWINDOW;
    }

#ifndef WIN16
    // If the hlinkframe supports the private interface ITargetFramePriv, use it.
    if (pTargetFramePriv)
    {
        PerfDbgLog1(tagNavigate, this, "FollowHyperlink ITargetFramePriv::NavigateHack "
        " \"%ls\"", pchURL);
        // Have a ITargetFramePriv -> use it
        hr = THR(pTargetFramePriv->NavigateHack(grfHLNF, pBindCtx, pDwnBindInfo,
            fOpenWithFrameName ? pchTarget:NULL, cstrExpandedUrl, *pcstrLocation));
        if (hr)
            goto Cleanup;
    }
#endif // ndef WIN16
#ifndef WINCE
    // Otherwise use hlinkframetarget.
    else if (pHlinkFrameTarget)
    {
        // create moniker
        hr = THR(CreateURLMoniker(NULL, cstrExpandedUrl, &pMoniker));
        if (hr)
            goto Cleanup;

        hr = THR(HlinkCreateFromMoniker(pMoniker, *pcstrLocation, NULL,
                 NULL, 0, NULL, IID_IHlink, (LPVOID *)&pHlink));
        if (hr)
            goto Cleanup;

        //    Pass frame name so that navigate can set it on the new
        //    window
        if (fOpenWithFrameName)
            IGNORE_HR(pHlink->SetTargetFrameName(pchTarget));

        PerfDbgLog1(tagNavigate, this, "FollowHyperlink IHlinkFrame::Navigate "
        " \"%ls\"", pchURL);
        // Have a frame -> use it
        hr = THR(pHlinkFrameTarget->Navigate(grfHLNF, pBindCtx, pDwnBindInfo, pHlink));
        if (hr)
            goto Cleanup;
    }
#endif // WINCE
    // If we don't even have an hlinkframetarget, do this.
    else
    {
        // If the protocol doesn't navigate (such as mailto:), just bind to the moniker
        if (!fProtocolNavigates)
        {
            hr = THR(CreateURLMoniker(NULL, cstrExpandedUrl, &pMoniker));
            if (!hr)
            {
                IUnknown *pUnknown = NULL;
                IBindStatusCallback *pPreviousBindStatusCallback = NULL;

                hr = THR( RegisterBindStatusCallback( pBindCtx, pDwnBindInfo,
                    &pPreviousBindStatusCallback, 0) );
                if (!hr)
                {
                    ReleaseInterface(pPreviousBindStatusCallback);

                    hr = THR( pMoniker->BindToObject(pBindCtx, NULL, IID_IUnknown, (void**)&pUnknown) );
                    if (OK(hr))
                    {
                        ReleaseInterface(pUnknown);
                        hr = S_OK;
                        goto Cleanup;
                    }
                }
            }

            // If BindToObject fails, we default to what we were doing before.
        }

        PerfDbgLog1(tagNavigate, this, "FollowHyperlink "
            "HlinkFrameNavigateNHL \"%ls\"", pchURL);

#ifdef WIN16

#if 0 // should we still try this? It should never work.
        IWebBrowser * pBrowser;
        hr = THR(QueryInterface(IID_IWebBrowser, (void **)&pBrowser));
        if (pBrowser)
        {
            // BUGWIN16: Notice bogus arguments and bogus casts to VARIANT *.
            hr = THR(pBrowser->Navigate((char *)pchExpandedUrl, NULL, (VARIANT *)pchTarget, (VARIANT *)pbPostData, NULL));

            ReleaseInterface(pBrowser);
        }
#endif

        // This will either launch a new iexplore or open the URL
        // in the top window of an iexplore.

        char szStartBrowser[MAX_PATH+pdlUrlLen];
        char szURL[pdlUrlLen] ;
        wsprintf ( szURL, "%s%s", &cstrExpandedUrl,(LPTSTR)pcstrLocation ) ;
        wsprintf(szStartBrowser, "%s %s", "iexplore", szURL);
        int nResult = WinExec(szStartBrowser, SW_SHOW);

        // if iexplore is already running...
        if (nResult == 16)
        {
            // find the window for IExplore and send a registered to
            // open the selected URL
            HWND hBrowser = FindWindow(MAINWND_CLASSNAME, NULL);
            static UINT WM_BROWSER_OPENURL = 0;

            if (hBrowser)
            {
                // if the message hasn't been registered yet, do it now
                if (!WM_BROWSER_OPENURL)
                {
                    WM_BROWSER_OPENURL = RegisterWindowMessage(LOAD_MESSAGE);
                }
                SendMessage(hBrowser, WM_BROWSER_OPENURL,
                        0, (LPARAM)szURL);
            }
        }

#else

        DbgMemoryTrackDisable(TRUE); // (dbau) hlsntm leaks one string
        grfHLNF &= ~HLNF_OPENINNEWWINDOW;   // Implied by HlinkFrameNavigateNHL
        hr = THR(HlinkFrameNavigateNHL(grfHLNF, pBindCtx, pDwnBindInfo,
                    fOpenWithFrameName ? pchTarget:NULL, cstrExpandedUrl, *pcstrLocation));

        DbgMemoryTrackDisable(FALSE);
        if (hr)
            goto Cleanup;
#endif //
    }

Cleanup:

    // If the navigation was synchronous (e.g. as in Outlook9, see bug 31960),
    // the old document is destroyed at this point! All the callers in the
    // call stack above this function need to handle this gracefully. Set hr
    // to E_ABORT so that these callers abort furhter processing and return
    // immediately.
    if (_cDie != cDie)
        hr = E_ABORT;

    ReleaseInterface(pTargetFramePriv);
    ReleaseInterface(pMoniker);
    ReleaseInterface(pHlink);

    RRETURN1(hr, S_FALSE);
//        hr = THR(HlinkSimpleNavigateToMoniker(pMoniker, cstrLocation, pchTarget,
//                                              NULL, NULL, NULL, 0, 0));
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRootFrame
//
//  Synopsis:   Finds the root frame of the specified frame
//              by climing up parents.
//
//----------------------------------------------------------------------------
HRESULT
GetRootFrame(IUnknown *pUnkFrame, IUnknown **ppUnkRootFrame)
{
    IUnknown *pUnkScan = pUnkFrame;
    ITargetFrame2 *pTargetFrameRoot = NULL;
    HRESULT hr;

    pUnkScan->AddRef();

    // run up to target root
    for (;;)
    {
        hr = THR(pUnkScan->QueryInterface(IID_ITargetFrame2, (void**)&pTargetFrameRoot));
        if (hr)
            goto Cleanup;

        pUnkScan->Release();
        pUnkScan = NULL;

        hr = THR(pTargetFrameRoot->GetParentFrame(&pUnkScan));
        if (hr)
            pUnkScan = NULL;

        if (!pUnkScan)
            break;

        pTargetFrameRoot->Release();
        pTargetFrameRoot = NULL;
    }
    Assert(!pUnkScan);
    Assert(pTargetFrameRoot);

    hr = THR(pTargetFrameRoot->QueryInterface(IID_IUnknown, (void**)ppUnkRootFrame));

Cleanup:
    ReleaseInterface(pUnkScan);
    ReleaseInterface(pTargetFrameRoot);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::IsFrameInsideWindow
//
//  Synopsis:   Determines if the given target frame is strictly inside
//              the same window as the current doc.
//
//              Returns FALSE if the target frame is the top level frame,
//              or if it is a frame in a different window.
//
//----------------------------------------------------------------------------
BOOL
CDoc::IsFrameInsideWindow(IUnknown *pTargetFrame)
{
    IUnknown *pUnkTargetFrame = NULL;
    IUnknown *pUnkTargetRoot = NULL;
    IUnknown *pUnkThisRoot = NULL;
    ITargetFrame2 *pThisFrame = NULL;
    BOOL fResult = FALSE;
    HRESULT hr;

    hr = THR(pTargetFrame->QueryInterface(IID_IUnknown, (void**)&pUnkTargetFrame));
    if (hr)
        goto Cleanup;

    hr = THR(QueryService(IID_ITargetFrame2, IID_ITargetFrame2, (void**)&pThisFrame));
    if (hr)
        goto Cleanup;

    hr = THR(GetRootFrame(pTargetFrame, &pUnkTargetRoot));
    if (hr)
        goto Cleanup;

    hr = THR(GetRootFrame(pThisFrame, &pUnkThisRoot));
    if (hr)
        goto Cleanup;

    fResult = (pUnkTargetFrame != pUnkThisRoot && pUnkTargetRoot == pUnkThisRoot);

Cleanup:
    ReleaseInterface(pUnkTargetFrame);
    ReleaseInterface(pUnkTargetRoot);
    ReleaseInterface(pUnkThisRoot);
    ReleaseInterface(pThisFrame);

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FollowHistory
//
//  Synopsis:   Goes forward or backward in history via automation on the
//              hosting shdocvw
//
//----------------------------------------------------------------------------
HRESULT
CDoc::FollowHistory(BOOL fForward)
{
    HRESULT             hr;
    ITargetFrame      * pTargetFrameSource = NULL;
    IUnknown          * pUnkSource = NULL;
    IUnknown          * pUnkTarget = NULL;
    IServiceProvider  * pServiceTarget = NULL;
    IWebBrowserApp    * pWebBrowserApp = NULL;

    hr = THR(QueryService(IID_ITargetFrame, IID_ITargetFrame, (void**)&pTargetFrameSource));
    if (hr)
        goto Cleanup;

    hr = THR(pTargetFrameSource->QueryInterface(IID_IUnknown, (void**)&pUnkSource));
    if (hr)
        goto Cleanup;

    hr = THR(pTargetFrameSource->FindFrame(_T("_top"), pUnkSource, FINDFRAME_JUSTTESTEXISTENCE, &pUnkTarget));
    if (!pUnkTarget)
        hr = E_FAIL;
    if (hr)
        goto Cleanup;

    hr = THR(pUnkTarget->QueryInterface(IID_IServiceProvider, (void**)&pServiceTarget));
    if (hr)
        goto Cleanup;

    hr = THR(pServiceTarget->QueryService(IID_IWebBrowserApp, IID_IWebBrowserApp,(void**)&pWebBrowserApp));
    if (hr)
        goto Cleanup;

    if (fForward)
        pWebBrowserApp->GoForward();
    else
        pWebBrowserApp->GoBack();

Cleanup:
    ReleaseInterface(pTargetFrameSource);
    ReleaseInterface(pUnkSource);
    ReleaseInterface(pUnkTarget);
    ReleaseInterface(pServiceTarget);
    ReleaseInterface(pWebBrowserApp);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::IsVisitedHyperlink
//
//  Synopsis:   returns TRUE if the given url is in the Hisitory
//
//              Currently ignores #location information
//
//----------------------------------------------------------------------------

BOOL
CDoc::IsVisitedHyperlink(LPCTSTR pchURL, CElement *pElementContext)
{
    HRESULT     hr              = S_OK;
    TCHAR       cBuf[pdlUrlLen];
    TCHAR*      pchExpandedUrl  = cBuf;
    BOOL        result          = FALSE;
    TCHAR *     pch;

    // fully resolve URL
    hr = THR(ExpandUrl(pchURL, ARRAY_SIZE(cBuf), pchExpandedUrl, pElementContext));
    if (hr)
        goto Cleanup;

    pch = const_cast<TCHAR *>(UrlGetLocation(pchExpandedUrl));

    Assert(!pchExpandedUrl[0] || pchExpandedUrl[0] > _T(' '));

    // Use the history cache-container from wininet

    if(!_pUrlHistoryStg)
    {
        hr = QueryService(SID_SUrlHistory, IID_IUrlHistoryStg, (LPVOID*)&_pUrlHistoryStg);

        if((FAILED(hr)) || (!_pUrlHistoryStg))
        {
            result = FALSE;
            goto Cleanup;
        }
    }

    Assert(_pUrlHistoryStg);

    if (pchExpandedUrl)
    {
        hr = _pUrlHistoryStg->QueryUrl(pchExpandedUrl, 0, NULL);
        result = (SUCCEEDED(hr));
    }
    else
    {
        result = FALSE;
    }

Cleanup:

    return result;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetBrowseContext, IHlinkTarget
//
//  Synopsis:   Hosts calls this when we're being hyperlinked to in order
//              to supply us with the browse context.
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CDoc::SetBrowseContext(IHlinkBrowseContext *pihlbc)
{
    ReplaceInterface(&_phlbc, pihlbc);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetBrowseContext, IHlinkTarget
//
//  Synopsis:   Returns our browse context
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::GetBrowseContext(IHlinkBrowseContext **ppihlbc)
{
    *ppihlbc = _phlbc;
    if (_phlbc)
    {
        _phlbc->AddRef();
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NavigateOutside, helper
//
//  Synopsis:   called when there is no client site for the doc; e.g., if
//              the doc is created inside Word or Excel as doc supporting
//              a hyperlink. In this case we launch an IE instance through
//              call to shdocvw.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::NavigateOutside(DWORD grfHLNF, LPCWSTR pchLocation)
{
#if !defined(WINCE) && !defined(WIN16)
    HRESULT                 hr = E_FAIL;
    IHlink *                pHlink = NULL;

    // BUGBUG: IE 3/4 classic implements an empty hlink site; we pass NULL.
    // BUGWIN16: should be uncommented for win16 also.
    hr = THR(HlinkCreateFromMoniker(_pmkName, pchLocation, NULL,
            NULL, 0, NULL, IID_IHlink, (LPVOID*) &pHlink));
    if (hr)
        goto Cleanup;

    hr = THR(HlinkFrameNavigate(0, NULL, NULL, pHlink, _phlbc));

Cleanup:
    ReleaseInterface(pHlink);

    RRETURN (hr);
#else // !WINCE
    RRETURN (E_FAIL);
#endif // !WINCE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NavigateNow, helper
//
//  Synopsis:   Forces a recalc up until the position specified in the
//              current bookmark task, if any.
//
//----------------------------------------------------------------------------

void
CDoc::NavigateNow(BOOL fScrollBits)
{
    if (    _pTaskLookForBookmark
        &&  !_pTaskLookForBookmark->_cstrJumpLocation
        &&  LoadStatus() >= LOADSTATUS_INTERACTIVE
        &&  _view.IsActive()
        &&  GetPrimaryElementClient()
        &&  GetPrimaryElementClient()->Tag() == ETAG_BODY)
    {
        CFlowLayout *   pFlowLayout = GetPrimaryElementClient()->HasFlowLayout();
        RECT            rc;

        Assert(pFlowLayout);

        pFlowLayout->GetClientRect(&rc);
        pFlowLayout->GetDisplay()->WaitForRecalc(-1, _pTaskLookForBookmark->_dwScrollPos + rc.bottom - rc.top);

        NavigateHere(0, NULL, _pTaskLookForBookmark->_dwScrollPos, fScrollBits);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NavigateHere, helper
//
//  Synopsis:   Called when the document lives inside IE (shdocvw).
//              If wzJumpLocation is not NULL, treat it as a bookmark and jump
//              there. Otheriwse, scroll the document to dwScrollPos.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::NavigateHere(DWORD grfHLNF, LPCWSTR wzJumpLocation, DWORD dwScrollPos, BOOL fScrollBits)
{
    HRESULT     hr                  = S_OK;
    CElement *  pElemBookmark;
    TCHAR    *  pBookmarkName;
    LONG        iStart, iStartAll;
    LONG        iHeight;
    BOOL        fBookmark;
    BOOL        fCreateTask         = FALSE;
    BOOL        fPrinting           = GetRootDoc()->IsPrintDoc();
    BOOL        fDontAdvanceStart   = FALSE;

    CFlowLayout *   pFlowLayout     = NULL;
    CElement *      pElementFL      = NULL;

    CElement    * pElementClient = GetPrimaryElementClient();
    CCollectionCache *pCollectionCache;

    Assert(_pClientSite);
    Assert(!wzJumpLocation || dwScrollPos == 0);

    // Don't create bookmark task for print documents.
    if (fPrinting)
        goto Cleanup;

    fBookmark = (wzJumpLocation != NULL);

    if (!fBookmark)
    {
        if (!pElementClient) // not yet created
            fCreateTask = TRUE;
        else if (pElementClient->Tag() != ETAG_BODY) // don't bother
            goto Cleanup;
        else
        {
            pFlowLayout = GetPrimaryElementClient()->HasFlowLayout();

            if (pFlowLayout)
            {
                RECT rc;
                pFlowLayout->GetClientRect(&rc);
                iHeight = rc.bottom - rc.top;
            }
            else
            {
                iHeight = 0;
            }
        
            pElementFL  = pFlowLayout->ElementOwner();

            if (    State() < OS_INPLACE
                ||  (   (   LoadStatus() < LOADSTATUS_PARSE_DONE
                        ||  !pFlowLayout->FRecalcDone())
                    &&  pFlowLayout->GetContentHeight() < (long)dwScrollPos + iHeight))
            {
                fCreateTask = TRUE;
            }
            else
            {
                CDocInfo DCI(pElementFL);

                pFlowLayout->ScrollToY(dwScrollPos, fScrollBits ? 0 : -1);
                PrimaryMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);
            }
        }

        // Perf optimization:
        // We don't a need a task to scroll to top. The only time we need to scroll
        // AND dwScrollPos is 0 is when we are navigating upward within the same
        // document (bug 37614). However, in this case the doc (or at leats its top)
        // is already loaded.
        if (fCreateTask && dwScrollPos == 0)
            goto Cleanup;

        pCollectionCache = PrimaryMarkup()->CollectionCache();

        // Clear any existing OM string for the jump location
       _cstrCOMPAT_OMUrl.Set(_T(""));
    }
    else
    {
        if (_tcslen(wzJumpLocation) == 0)
            goto Cleanup;

        // and here is the magic that acutally sets the # information for the OM string
        // see CDoc:get_URL for a discussion on why this is here
        //-----------------------------------------------------------------------------
        if (wzJumpLocation[0] !=_T('#'))
        {
            _cstrCOMPAT_OMUrl.Set(_T("#"));
            _cstrCOMPAT_OMUrl.Append(wzJumpLocation);
        }
        else
            _cstrCOMPAT_OMUrl.Set(wzJumpLocation);

        // Now continue on with the effort of determining where to scroll to
        //prepare the anchors' collection
        hr = PrimaryMarkup()->EnsureCollectionCache(CMarkup::ANCHORS_COLLECTION);
        if (hr)
            goto Cleanup;

        pCollectionCache = PrimaryMarkup()->CollectionCache();
        pBookmarkName = (TCHAR *)wzJumpLocation;

        iStart = iStartAll = 0;

        // Is this the same location we had stashed away? If yes then
        // do the incremental search, else search from the beginning
        if (_pTaskLookForBookmark)
        {
            CStr cstrTemp;

            cstrTemp.Set(wzJumpLocation);
            if (_pTaskLookForBookmark->_cstrJumpLocation.Compare(&cstrTemp))
            {
                // Check the collections' cookie because they might been changed
                if(_pTaskLookForBookmark->_lColVer == pCollectionCache->GetVersion(CMarkup::ANCHORS_COLLECTION))
                {
                    // Continue the search
                    iStart = _pTaskLookForBookmark->_iStartSearchingAt;
                    iStartAll = _pTaskLookForBookmark->_iStartSearchingAtAll;
                }
                else
                {
                    // Restart the search from the beginning
                    iStart = iStartAll = 0;
                }
            }
        }

        // Find the element with given name attribute in the anchors' collection
        hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ANCHORS_COLLECTION,
            pBookmarkName, FALSE, &pElemBookmark, iStart));
        if(FAILED(hr) || !pElemBookmark)
        {
            // Try to search without the starting #
            if(pBookmarkName[0] == _T('#') && pBookmarkName[1])
            {
                pBookmarkName++;
                hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ANCHORS_COLLECTION,
                    pBookmarkName, FALSE, &pElemBookmark, iStart));

                // If not found try to search all element collection
                if(FAILED(hr) || !pElemBookmark)
                {
                    // Restore the bookmark name pointer
                    pBookmarkName--;

                    // prepare the elements' collection
                    hr = PrimaryMarkup()->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION);
                    if (hr)
                        goto Cleanup;

                    hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION,
                        pBookmarkName, FALSE, &pElemBookmark, iStartAll));
                    if(FAILED(hr) || !pElemBookmark)
                    {
                        // Try to search without the starting #
                        if(pBookmarkName[0] == _T('#') && pBookmarkName[1])
                        {
                            pBookmarkName++;
                            hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION,
                                pBookmarkName, FALSE, &pElemBookmark, iStartAll));
                        }
                    }
                }
            }
         }


        // treat mutliple anchors with the same name as okay
        if (FAILED(hr))
        {
            // DISP_E_MEMBERNOTFOUND is the only E_ hr that GetIntoAry may return
            Assert(hr == DISP_E_MEMBERNOTFOUND);

            // '#top' and the '#' bookmark are a special case.  '#' is necesary for 
            // ie4/NS compat
            if (    (StrCmpIC(pBookmarkName, _T("top")) == 0) 
                ||  (StrCmpIC(pBookmarkName, _T("#"))   == 0))
            {
                // Scroll to the top of the document
                if(GetPrimaryElementClient())
                {
                    hr = THR(GetPrimaryElementClient()->GetUpdatedLayout()->
                        ScrollElementIntoView(NULL, SP_TOPLEFT, SP_TOPLEFT, fScrollBits));
                }
            }
            else if (LoadStatus() < LOADSTATUS_PARSE_DONE)
            {
                fCreateTask = TRUE;
            }
        }
        else
        {
            // If for some reason we cannot show the element yet, then return.
            // Note: we keep _iStartSearchAt the same in this case and hence
            // we will be able to find this bookmark again.
            if (!pElemBookmark->CanShow())
            {
                fCreateTask = TRUE;
                fDontAdvanceStart = TRUE;
            }
            else
            {
                hr = THR(pElemBookmark->BubbleBecomeCurrent(0));
                if (hr)
                    goto Cleanup;

                hr = pElemBookmark->ScrollIntoView(SP_TOPLEFT, SP_MINIMAL, fScrollBits);
                if (hr)
                    goto Cleanup;

                if (pElemBookmark != _pElemCurrent && S_OK == GetCaret(NULL) && _pCaret)
                {
                    // Position the caret at the beginning of the bookmark, so that the
                    // next tab would go to the expected place (IE5 63832)
                    CMarkupPointer      ptrStart(this);
                    IMarkupPointer *    pIStartBookmark; 

                    hr = ptrStart.MoveToCp(pElemBookmark->GetFirstCp(), pElemBookmark->GetMarkup());
                    if (hr)
                        goto Cleanup;
                    Verify(S_OK == ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStartBookmark));
                    IGNORE_HR(_pCaret->MoveCaretToPointer(pIStartBookmark, TRUE, FALSE, CARET_DIRECTION_INDETERMINATE));
                    pIStartBookmark->Release();
                    _fFirstTimeTab = FALSE;

                }
            }
        }
    }

    if (fCreateTask)
    {

        // Start the task that periodically wakes up and checks if we
        // can jump to the desired location.
        if (!_pTaskLookForBookmark)
        {
            _pTaskLookForBookmark = new CTaskLookForBookmark(this);
            if (_pTaskLookForBookmark == NULL)
            {
                goto Ret;
            }
        }
        else
        {
            // Check if this is the last time this task needs to be tried.
            // For bookmarks, we can stop when the doc is fully loaded.
            // For setting scroll position, we need to also wait for the
            // doc to go in-place and the body site to get fully recalc'ed.
            if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
            {
                if (fBookmark)
                    goto Cleanup;

                if (State() >= OS_INPLACE &&
                    LoadStatus() >= LOADSTATUS_DONE &&
                    GetPrimaryElementClient() &&
                    GetPrimaryElementClient()->Tag() == ETAG_BODY)
                {
                    pFlowLayout = GetPrimaryElementClient()->HasFlowLayout();
                    if (pFlowLayout->FRecalcDone())
                        goto Cleanup;
                }
            }

        }


        if (fBookmark)
        {
            _pTaskLookForBookmark->_dwScrollPos = 0;
            _pTaskLookForBookmark->_cstrJumpLocation.Set(wzJumpLocation);

            // prepare the anchors' collection
            hr = PrimaryMarkup()->EnsureCollectionCache(CMarkup::ANCHORS_COLLECTION);
            if (hr)
                goto Cleanup;

            if (S_OK != pCollectionCache->GetLength (CMarkup::ANCHORS_COLLECTION, &iStart))
            {
                // Cannot find the length for some reason, we should
                // start searching from the beginning.
                iStart = 0;
            }
            
            if (!fDontAdvanceStart)
                _pTaskLookForBookmark->_iStartSearchingAt = iStart;
                
            if (S_OK != pCollectionCache->GetLength (CMarkup::ELEMENT_COLLECTION, &iStartAll))
            {
                // Cannot find the length for some reason, we should
                // start searching from the beginning.
                iStartAll = 0;
            }
            _pTaskLookForBookmark->_iStartSearchingAtAll = iStartAll;

            // Save the version of the collections
            _pTaskLookForBookmark->_lColVer = pCollectionCache->GetVersion(CMarkup::ANCHORS_COLLECTION);
        }
        else
        {
            _pTaskLookForBookmark->_dwScrollPos = dwScrollPos;
            _pTaskLookForBookmark->_cstrJumpLocation.Free();

            if (_pTaskLookForBookmark->_dwTimeGotBody)
            {
                // Delay interactivity for no more than five seconds after creating the body
                if (GetTickCount() - _pTaskLookForBookmark->_dwTimeGotBody > 5000)
                {
                    PrimaryMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);
                }
            }
            else if (pElementClient && pElementClient->Tag() == ETAG_BODY)
            {
                _pTaskLookForBookmark->_dwTimeGotBody = GetTickCount();
            }
        }
        goto Ret;
    }

Cleanup:
    TerminateLookForBookmarkTask();

Ret:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Navigate, IHlinkTarget
//
//  Synopsis:   Called to tell us which jump-location we need to navigate to.
//              We may not have loaded the anchor yet, so we need to save
//              the location and only jump once we find the anchor.
//              Note that when we support IHlinkTarget, containers simply
//              call Navigate instead of IOleObject::DoVerb(OLEIVERB_SHOW)
//
//----------------------------------------------------------------------------

HRESULT
CDoc::Navigate(DWORD grfHLNF, LPCWSTR wzJumpLocation)
{
    HRESULT hr;

    if (_pClientSite)
    {

        // BUGBUG - we should defer SHOW until there is something to show
        Assert(_pClientSite);
        hr = THR(DoVerb(OLEIVERB_SHOW, NULL, _pClientSite, 0, NULL, NULL));
        if (hr)
            goto Cleanup;

        if (wzJumpLocation)
            hr = THR(NavigateHere(grfHLNF, wzJumpLocation, 0, TRUE));
    }
    else
    {
        hr = THR(NavigateOutside(grfHLNF, wzJumpLocation));
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetMonikerHlink, IHlinkTarget
//
//  Synopsis:   Called to supply our moniker...
//              NOTE: this is renamed from GetMoniker to avoid multiple
//              inheritance problem with IOleObject::GetMoniker
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::GetMonikerHlink(LPCWSTR wzLocation, DWORD dwAssign, IMoniker **ppimkLocation)
{
    *ppimkLocation = _pmkName;
    if (_pmkName)
    {
        _pmkName->AddRef();
    }
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetFriendlyName, IHlinkTarget
//
//  Synopsis:   Returns a friendly name for the fragment
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::GetFriendlyName(LPCWSTR pchLocation, LPWSTR *pchFriendlyName)
{
    if (!pchLocation)
        RRETURN (E_POINTER);

    // to do: figure out where this string goes; perhaps fix it

    // for now: friendly-name = location-string
    RRETURN(TaskAllocString(pchLocation, pchFriendlyName));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetFramesContainer, ITargetContainer
//
//  Synopsis:   Provides access to our IOleContainer
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetFramesContainer(IOleContainer **ppContainer)
{
    RRETURN(QueryInterface(IID_IOleContainer, (void**)ppContainer));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetFrameUrl, ITargetContainer
//
//  Synopsis:   Provides access to our URL
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetFrameUrl(LPWSTR *ppszFrameSrc)
{
    Assert(!!_cstrUrl);

    RRETURN(TaskAllocString(_cstrUrl, ppszFrameSrc));
}

#if 0
//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetParentFrame, ITargetFrame
//
//  Synopsis:   Provides access to our IOleContainer
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetParentFrame(IUnknown **ppUnkParentFrame)
{
    HRESULT         hr;
    ITargetFrame  * pTargetFrame = NULL;

    hr = THR(QueryService(IID_ITargetFrame, IID_ITargetFrame, (void**)&pTargetFrame));
    if (hr)
        goto Cleanup;

    hr = THR(pTargetFrame->QueryInterface(IID_IUnknown, (void**)ppUnkParentFrame));

Cleanup:
    ReleaseInterface(pTargetFrame);
    RRETURN(hr);
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::TerminateLookForBookmarkTask
//
//  Synopsis:
//
//----------------------------------------------------------------------------
void
CDoc::TerminateLookForBookmarkTask()
{
    if (_pTaskLookForBookmark)
    {
        _pTaskLookForBookmark->Terminate();
        _pTaskLookForBookmark->Release();
        _pTaskLookForBookmark = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTaskLookForBookmark::OnRun
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CTaskLookForBookmark::OnRun(DWORD dwTimeOut)
{
    if (_cstrJumpLocation)
    {
        CStr cstrTemp;

        // The problem is that Navigate and its descendants will want
        // to call CStr::Set on _cstrJumpLocation, and hence we cannot
        // pass that in to Navigate (CStr::Set frees memory and then
        // allocates and copies -- you figure yet?)
        cstrTemp.Set(_cstrJumpLocation);
        _pDoc->Navigate(0, cstrTemp);
    }
    else
    {
        // Then try to scroll or continue waiting

        _pDoc->NavigateHere(0, NULL, _dwScrollPos, TRUE);
    }
}
