//+------------------------------------------------------------------------
//
//  File:       docurl.cxx
//
//  Contents:   url helpers, etc.
//
//-------------------------------------------------------------------------

#include "headers.hxx"


#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifdef WIN16

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_URLMKI_H_
#define X_URLMKI_H_
#include "urlmki.h"
#endif

#endif

BOOL IsUrlOnNet(const TCHAR *pchUrl);

MtDefine(GetBaseTarget, Utilities, "CDoc::GetBaseTarget")
MtDefine(GetFriendlyUrl, Utilities, "CDoc::GetFriendlyUrl")
MtDefine(ExpandUrlWithBaseUrl, Utilities, "ExpandUrlWithBaseUrl")

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetBaseTarget
//
//              Get the base URL for the document.
//
//              If supplied with an element, gets the base TARGET in effect
//              at that element, based on the position of <BASE> tags.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetBaseTarget(TCHAR **ppchTarget, CElement *pElementContext)
{
    TCHAR       *pchTarget = NULL;
    TCHAR       *pchTargetTemp;
    HRESULT      hr = S_OK;
    CTreeNode   *pNode;

    if (pElementContext && pElementContext->GetFirstBranch())
    {
        //  Search the body upwards for first Base tag with target attribute
        for (pNode = pElementContext->GetFirstBranch()->Parent(); 
             pNode; 
             pNode=pNode->Parent())
        {
            if (pNode->Tag() == ETAG_BASE)
            {
                pchTargetTemp = (LPTSTR) DYNCAST(CBaseElement, pNode->Element())->GetAAtarget();
                if (pchTargetTemp && *pchTargetTemp)
                {
                    pchTarget = pchTargetTemp;
                    break;
                }
            }
        }
    }

    if (pchTarget == NULL)
    {
        CElement * pElementHead = _pPrimaryMarkup->GetHeadElement();
        
        if (pElementHead)
        {
            CChildIterator ci ( pElementHead );
            CTreeNode * pNode;

            ci.SetAfterEnd();

            while ( (pNode = ci.PreviousChild()) != NULL )
            {
                if (pNode->Tag() == ETAG_BASE)
                {
                    pchTargetTemp = LPTSTR( DYNCAST( CBaseElement, pNode->Element() )->GetAAtarget() );
                    
                                    
                    if (pchTargetTemp && *pchTargetTemp)
                    {
                        pchTarget = pchTargetTemp;
                        break;
                    }
                }
            }
        }
    }

    *ppchTarget = NULL;
    if (pchTarget)
    {
        hr = THR(MemAllocString(Mt(GetBaseTarget), pchTarget, ppchTarget));
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetBaseUrl
//
//              Get the base URL for the document.
//
//              If supplied with an element, gets the base URL in effect
//              at that element, based on the position of <BASE> tags.
//              Note that this is a pointer to an internal string, it can't
//              be modified. If you need to modify it, make a copy first.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetBaseUrl(
    TCHAR * *  ppchHref,
    CElement * pElementContext,
    BOOL *     pfDefault,
    TCHAR *    pszAlternateDocUrl )
{
    TCHAR *     pchHref = NULL;
    TCHAR    *  pchHrefTemp;
    BOOL        fDefault;
    HRESULT     hr=S_OK;
    CTreeNode * pNode;
    
    Assert(!!_cstrUrl);

    if (pElementContext != EXPANDIGNOREBASETAGS && _fHasBaseTag)
    {
        if (pElementContext)
        {
            CTreeNode * pNodeContext = pElementContext->GetFirstBranch();

            if (pNodeContext)
            {
                //  Search the body upwards for first Base tag with href attribute
                for ( pNode = pNodeContext->Parent() ; 
                      pNode ; 
                      pNode = pNode->Parent() )
                {
                    if (pNode->Tag() == ETAG_BASE)
                    {
                        pchHrefTemp = (LPTSTR) DYNCAST(CBaseElement, pNode->Element())->GetHref();

                        if (pchHrefTemp && *pchHrefTemp)
                        {
                            pchHref = pchHrefTemp;
                            break;
                        }
                    }
                }
            }
        }

        // BUGBUG: This searches the head nodes for the main tree.  We
        // probably want to search in a specific tree! Maybe we always
        // need a context element!

        if (pchHref == NULL)
        {
            CHeadElement * pHead = _pPrimaryMarkup->GetHeadElement();

            if( pHead )
            {
                CChildIterator ci ( pHead, NULL, CHILDITERATOR_DEEP );
                CTreeNode * pNode;

                ci.SetAfterEnd();

                while ( (pNode = ci.PreviousChild()) != NULL )
                {
                    if (pNode->Tag() == ETAG_BASE)
                    {
                        pchHrefTemp = LPTSTR (DYNCAST( CBaseElement, pNode->Element() )->GetHref() );
                    
                        if (pchHrefTemp && *pchHrefTemp)
                        {
                            pchHref = pchHrefTemp;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (pchHref == NULL)
    {
        *ppchHref = pszAlternateDocUrl ?
                    pszAlternateDocUrl :
                    _cstrUrl;

        fDefault = TRUE;
    }
    else
    {
        *ppchHref = pchHref;
        fDefault = FALSE;
    }

    if (pfDefault)
        *pfDefault = fDefault;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ExpandUrl
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ExpandUrl(
    LPCTSTR    pchRel, 
    LONG       dwUrlSize,
    TCHAR *    pchUrlOut,
    CElement * pElementContext,
    DWORD      dwFlags,
    TCHAR *    pszAlternateDocUrl )
{
    HRESULT hr=S_OK;
    DWORD cchBuf;
    BOOL fDefault;
    BOOL fCombine;
    DWORD dwSize;
    TCHAR *pchBaseUrl=NULL;

    AssertSz(dwUrlSize == pdlUrlLen, "Wrong size URL buffer!!");

    if (dwFlags == 0xFFFFFFFF)
        dwFlags = URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE;

    if (pchRel == NULL) // note: NULL is different from "", which is more similar to "."
    {
        *pchUrlOut = _T('\0');
        goto Cleanup;
    }

    hr = GetBaseUrl( & pchBaseUrl, pElementContext, & fDefault, pszAlternateDocUrl );
    if (hr)
        goto Cleanup;

    hr = THR(CoInternetCombineUrl(
        pchBaseUrl, pchRel, dwFlags, pchUrlOut, pdlUrlLen, &cchBuf, 0));
    
    if (hr)
        goto Cleanup;

    if (    !fDefault
        &&  S_OK == THR(CoInternetQueryInfo(
                        pszAlternateDocUrl ? pszAlternateDocUrl : _cstrUrl,
                        QUERY_RECOMBINE, 0, &fCombine,
                        sizeof(BOOL), &dwSize, 0))
        &&  fCombine)
    {
        TCHAR achBuf2[pdlUrlLen];
        DWORD cchBuf2;

        hr = THR(CoInternetCombineUrl(
            pszAlternateDocUrl ? pszAlternateDocUrl : _cstrUrl,
            pchUrlOut, dwFlags, achBuf2, pdlUrlLen, &cchBuf2, 0));
        
        if (hr)
            goto Cleanup;

        _tcsncpy (pchUrlOut, achBuf2, dwUrlSize);
        pchUrlOut[dwUrlSize-1] = _T('\0');    //If there wasn't room for the URL.
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetRootSslState
//
//----------------------------------------------------------------------------
extern void PostManLock();
extern void PostManUnlock();


void
CDoc::EnterRootSslPrompt()
{
    CDoc *pDoc = GetRootDoc();
    pDoc->_cInSslPrompt += 1;

    PostManLock();
}

BOOL
CDoc::InRootSslPrompt()
{
    CDoc *pDoc = GetRootDoc();
    return (!!pDoc->_cInSslPrompt);
}

void
CDoc::LeaveRootSslPrompt()
{
    CDoc *pDoc = GetRootDoc();
    pDoc->_cInSslPrompt -= 1;

    PostManUnlock();

    // Anything deferred because of InRootSslPrompt should be reexecuted now:
    if (_fNeedUrlImgCtxDeferredDownload)
    {
        _fNeedUrlImgCtxDeferredDownload = FALSE;
        OnUrlImgCtxDeferredDownload(NULL);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ValidateSecureUrl
//
//  Synopsis:   Given an absolute URL, returns TRUE if it is okay (in terms
//              of SSL security) to begin download of the URL.
//
//              May prompt the user (once per top-level frame).
//
//              If fReprompt is TRUE (default FALSE), the user is reprompted
//              even if s/he previously answered NO. The user is never
//              reprompted if s/he previously answered YES. (Once unsecure,
//              always unsecure.)
//
//----------------------------------------------------------------------------
BOOL
CDoc::ValidateSecureUrl(TCHAR *pchUrl, BOOL fReprompt, BOOL fSilent, BOOL fUnsecureSource)
{
    BOOL bResult = TRUE;
    DWORD error;
    SSL_SECURITY_STATE sslSecurity;
    SSL_PROMPT_STATE   sslPrompt;

    // grab the current security state
    GetRootSslState(&sslSecurity, &sslPrompt);

    // if everything is allowed, don't even look at URL (perf win)
    if (sslPrompt == SSL_PROMPT_ALLOW)
        return TRUE;

    // If the URL is https then it needs no validation
    if (!fUnsecureSource && IsUrlSecure(pchUrl))
        return TRUE;

    // Otherwise, we may need to prompt
    if (sslPrompt == SSL_PROMPT_QUERY ||
        (sslPrompt == SSL_PROMPT_DENY && fReprompt))
    {
        if (fSilent || _dwLoadf & DLCTL_SILENT)
        {
            // in silent mode, act as if the user ignores the warnings
            SetRootSslState(SSL_SECURITY_UNSECURE, SSL_PROMPT_ALLOW);
            sslPrompt = SSL_PROMPT_ALLOW;
        }
        else
        {
            {
                CDoEnableModeless   dem(this);
                HWND                hwnd = dem._hwnd;

                EnterRootSslPrompt();
                
                error = InternetErrorDlg(hwnd, 
                                         NULL,      // no request
                                         ERROR_INTERNET_MIXED_SECURITY,
                                         0,         // no flags
                                         NULL       // no extra data
                                         );

                LeaveRootSslPrompt();
            }
            
            // in case state has changed during message box,
            // grab the current security state again
            
            GetRootSslState(&sslSecurity, &sslPrompt);

            if (error == ERROR_SUCCESS)
            {
                // User says "okay" to mixed content: downgrade security
                if (sslSecurity >= SSL_SECURITY_SECURE)
                    sslSecurity = SSL_SECURITY_MIXED;

                sslPrompt = SSL_PROMPT_ALLOW;
            }
            else
            {
                sslPrompt = SSL_PROMPT_DENY;
            }
            
            SetRootSslState(sslSecurity, sslPrompt);
        }
    }
    
    bResult = (sslPrompt == SSL_PROMPT_ALLOW);
    
    return bResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetRootSslState
//
//----------------------------------------------------------------------------

void
CDoc::GetRootSslState(SSL_SECURITY_STATE *psss, SSL_PROMPT_STATE *psps)
{
    CDoc *pDoc = GetRootDoc();
    *psss = pDoc->_sslSecurity;
    *psps = pDoc->_sslPrompt;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetRootSslState
//
//----------------------------------------------------------------------------

void
CDoc::SetRootSslState(SSL_SECURITY_STATE sss, SSL_PROMPT_STATE sps, BOOL fInit)
{
    CDoc *pDoc = GetRootDoc();

    Assert(!(sss == SSL_SECURITY_UNSECURE && sps != SSL_PROMPT_ALLOW));
    Assert(!(sss >= SSL_SECURITY_SECURE && sps == SSL_PROMPT_ALLOW));

    pDoc->_sslPrompt = sps;
    
    if (pDoc->_sslSecurity != sss || fInit)
    {
        pDoc->_sslSecurity = sss;
        if (pDoc->_pClientSite)
        {
            VARIANT varIn;
        
            Assert(SECURELOCK_SET_UNSECURE          == SSL_SECURITY_UNSECURE);
            Assert(SECURELOCK_SET_MIXED             == SSL_SECURITY_MIXED);
            Assert(SECURELOCK_SET_SECUREUNKNOWNBIT  == SSL_SECURITY_SECURE);
            Assert(SECURELOCK_SET_SECURE40BIT       == SSL_SECURITY_SECURE_40);
            Assert(SECURELOCK_SET_SECURE56BIT       == SSL_SECURITY_SECURE_56);
            Assert(SECURELOCK_SET_FORTEZZA          == SSL_SECURITY_FORTEZZA);
            Assert(SECURELOCK_SET_SECURE128BIT      == SSL_SECURITY_SECURE_128);

            V_VT(&varIn) = VT_I4;
            V_I4(&varIn) = sss;

            CTExec(
                pDoc->_pClientSite,
                &CGID_ShellDocView,
                SHDVID_SETSECURELOCK,
                0,
                &varIn,   
                0);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::AllowFrameUnsecureRedirect
//
//----------------------------------------------------------------------------
BOOL
CDoc::AllowFrameUnsecureRedirect()
{
    SSL_SECURITY_STATE sslSecurity;
    SSL_PROMPT_STATE   sslPrompt;

    GetRootSslState(&sslSecurity, &sslPrompt);

    if (sslSecurity >= SSL_SECURITY_SECURE)
        return FALSE;

    if (sslPrompt != SSL_PROMPT_ALLOW)
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:  SecStateFromSecFlags
//
//----------------------------------------------------------------------------
SSL_SECURITY_STATE
SecStateFromSecFlags(DWORD dwFlags)
{
    if (!(dwFlags & SECURITY_FLAG_SECURE))
        return SSL_SECURITY_MIXED;

    if (dwFlags & SECURITY_FLAG_128BIT)
        return SSL_SECURITY_SECURE_128;

    if (dwFlags & SECURITY_FLAG_FORTEZZA)
        return SSL_SECURITY_FORTEZZA;

    if (dwFlags & SECURITY_FLAG_56BIT)
        return SSL_SECURITY_SECURE_56;
        
    if (dwFlags & SECURITY_FLAG_40BIT)
        return SSL_SECURITY_SECURE_40;

    return SSL_SECURITY_SECURE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnHtmDownloadSecFlags(dwFlags)
//
//----------------------------------------------------------------------------
void
CDoc::OnHtmDownloadSecFlags(DWORD dwFlags)
{
    if (IsRootDoc())
    {
        SSL_SECURITY_STATE sslSecurity;
        SSL_PROMPT_STATE   sslPrompt;

        GetRootSslState(&sslSecurity, &sslPrompt);

        sslSecurity = SecStateFromSecFlags(dwFlags);
                
        // we're a nonsecure top frame load...
        
        if (sslSecurity >= SSL_SECURITY_SECURE)
        {
            // We were potentially redirected to a secure URL
            
            if (sslPrompt == SSL_PROMPT_ALLOW)
                sslPrompt = SSL_PROMPT_QUERY;
                
            SetRootSslState(sslSecurity, sslPrompt);
        }
        else
        {
            // We were potentially redirected to an unsecure URL or to the cache

            SetRootSslState(SSL_SECURITY_UNSECURE, SSL_PROMPT_ALLOW);
        }
    }
    else
    {
        // Otherwise, we're a subframe load; behave like an image (except about:)
        
        if (!_fSslSuppressedLoad)
            OnSubDownloadSecFlags(_cstrUrl, dwFlags);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnSubDownloadSecFlags(dwFlags)
//
//----------------------------------------------------------------------------
void
CDoc::OnSubDownloadSecFlags(const TCHAR *pchUrl, DWORD dwFlags)
{
    SSL_SECURITY_STATE sslSecurity;
    SSL_SECURITY_STATE sslSecIn;
    SSL_SECURITY_STATE sslSecNew;
    SSL_PROMPT_STATE   sslPrompt;

    GetRootSslState(&sslSecurity, &sslPrompt);

    sslSecIn = SecStateFromSecFlags(dwFlags);

    sslSecNew = min(sslSecIn, sslSecurity);

    // note: SSL_SECURITY_SECURE (unknown bitness) should have been the
    // largest enum; but we can't change it because it is exported -
    // so fixup "min" by detecting the case where the smaller one was
    // SSL_SECURITY_SECURE and taking the other one
    
    if (sslSecNew == SSL_SECURITY_SECURE)
        sslSecNew = max(sslSecIn, sslSecurity);
        
    if (sslSecurity != sslSecNew && IsUrlOnNet(pchUrl))
        SetRootSslState(sslSecNew, sslPrompt);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnIgnoreFrameSslSecurity()
//
//  Synopsis:   BUGBUG: when a non-SSL frame load has been suppressed by
//              the user, we need to touch subframe's host with a CTExec
//              SDHVID_SETSECURELOCK in LoadFromInfo so that the shell
//              doesn't ask urlmon/wininet for its idea of the frame's
//              security.
//
//----------------------------------------------------------------------------
void
CDoc::OnIgnoreFrameSslSecurity()
{
    SSL_SECURITY_STATE sslSecurity;
    SSL_PROMPT_STATE   sslPrompt;

    GetRootSslState(&sslSecurity, &sslPrompt);
    
    if (_pClientSite)
    {

        VARIANT varIn;
    
        V_VT(&varIn) = VT_I4;
        V_I4(&varIn) = sslSecurity;

        CTExec(
            _pClientSite,
            &CGID_ShellDocView,
            SHDVID_SETSECURELOCK,
            0,
            &varIn,   
            0);
    }
}



// Implementation of the global GetFriendlyUrl

#define MAX_SUFFIX_LEN 256

#define PROTOCOL_FILE       _T("file")
#define PROTOCOL_MAILTO     _T("mailto")
#define PROTOCOL_GOPHER     _T("gopher")
#define PROTOCOL_FTP        _T("ftp")
#define PROTOCOL_HTTP       _T("http")
#define PROTOCOL_HTTPS      _T("https")
#define PROTOCOL_NEWS       _T("news")

static void Concat(TCHAR *pchDst, int sizeDst, const TCHAR *pchSrc, LONG x, LONG y)
{
    if (x >= 0)
    {
        TCHAR achTemp[pdlUrlLen + 1 + MAX_SUFFIX_LEN + 20];

        _tcscpy(achTemp, pchDst);
        Format(0, pchDst, sizeDst, _T("<0s><1s>?<2d>,<3d>"), achTemp, pchSrc, x, y);
    }
    else
        _tcscat(pchDst, pchSrc);
}

TCHAR *
GetFriendlyUrl(const TCHAR *pchUrl, const TCHAR *pchBaseUrl, BOOL fShowFriendlyUrl,
               BOOL fPreface, LONG x, LONG y)
{
    URL_COMPONENTS uc;
    TCHAR achScheme[INTERNET_MAX_SCHEME_LENGTH];
    TCHAR achHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR achPath[INTERNET_MAX_URL_LENGTH];
    TCHAR achUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR achFriendlyUrl[INTERNET_MAX_URL_LENGTH + 1 + MAX_SUFFIX_LEN + 20]; // 20 is ample extra space
    TCHAR *pchFriendlyUrl;
    DWORD cchUrl;

    memset(&uc, 0, sizeof(uc));

    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = achScheme;
    uc.dwSchemeLength = ARRAY_SIZE(achScheme);
    uc.lpszHostName = achHostName;
    uc.dwHostNameLength = ARRAY_SIZE(achHostName);
    uc.lpszUrlPath = achPath;
    uc.dwUrlPathLength = ARRAY_SIZE(achPath);

    if (S_OK != CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0, achUrl, ARRAY_SIZE(achUrl), &cchUrl, 0))
    {
        long    cch = min(INTERNET_MAX_URL_LENGTH - 1, _tcslen(pchUrl));
        
        _tcsncpy(achUrl, pchUrl, cch);
        achUrl[cch + 1] = 0;
    }

    if (InternetCrackUrl(achUrl, _tcslen(achUrl), 0, &uc))
    {
        BOOL fMail = !StrCmpIC(achScheme, PROTOCOL_MAILTO);

        achFriendlyUrl[0] = _T('\0');

        if (fMail)
        {
            if (fShowFriendlyUrl)
            {
                Format(0, achFriendlyUrl, ARRAY_SIZE(achFriendlyUrl), MAKEINTRESOURCE(IDS_FRIENDLYURL_SENDSMAILTO),
                       &achUrl[1 + _tcslen(PROTOCOL_MAILTO)]);
            }
            else
                _tcscat(achFriendlyUrl, achUrl);
        }
        else if (fShowFriendlyUrl)
        {
            TCHAR achSuffix[MAX_SUFFIX_LEN];
            BOOL fFriendlyString = TRUE;

            achSuffix[0] = _T('\0');

            if (!StrCmpIC(achScheme, PROTOCOL_FILE))
            {
                Format(0, achSuffix, ARRAY_SIZE(achSuffix), MAKEINTRESOURCE(IDS_FRIENDLYURL_LOCAL));
                fFriendlyString = FALSE;
            }
            else if (!StrCmpIC(achScheme, PROTOCOL_GOPHER))
                Format(0, achSuffix, ARRAY_SIZE(achSuffix), MAKEINTRESOURCE(IDS_FRIENDLYURL_GOPHER));
            else if (!StrCmpIC(achScheme, PROTOCOL_FTP))
                Format(0, achSuffix, ARRAY_SIZE(achSuffix), MAKEINTRESOURCE(IDS_FRIENDLYURL_FTP));
            else if (!StrCmpIC(achScheme, PROTOCOL_HTTPS))
                Format(0, achSuffix, ARRAY_SIZE(achSuffix), MAKEINTRESOURCE(IDS_FRIENDLYURL_SECUREWEBSITE));
            else if (StrCmpIC(achScheme, PROTOCOL_HTTP) && StrCmpIC(achScheme, PROTOCOL_NEWS))
                fFriendlyString = FALSE;
                    
            if (fFriendlyString)
            {
                // "http://sitename/path/filename.gif" -> "filename.gif at sitename"
                int length = _tcslen(achPath);
                TCHAR *pchShortName;
                BOOL fShowHostName = FALSE;

                if (length && (achPath[length - 1] == _T('/')))
                    achPath[length - 1] = _T('\0');

                if (*achHostName)
                {
                    URL_COMPONENTS ucBase;
                    TCHAR achHostNameBase[INTERNET_MAX_HOST_NAME_LENGTH];

                    memset(&ucBase, 0, sizeof(ucBase));

                    ucBase.dwStructSize = sizeof(ucBase);
                    ucBase.lpszHostName = achHostNameBase;
                    ucBase.dwHostNameLength = ARRAY_SIZE(achHostNameBase);

                    if (    !pchBaseUrl
                        ||  !InternetCrackUrl(pchBaseUrl, _tcslen(pchBaseUrl), 0, &ucBase)
                        ||  _tcsicmp(achHostName, achHostNameBase))
                        fShowHostName = TRUE;
                }

                pchShortName = _tcsrchr(achPath, _T('/'));
                if (fShowHostName && pchShortName && *(pchShortName + 1))
                {
                    TCHAR achShortName[INTERNET_MAX_URL_LENGTH];

                    achShortName[0] = _T('\0');
                    Concat(achShortName, ARRAY_SIZE(achShortName), pchShortName + 1, x, y);
                    Format(0, achFriendlyUrl, ARRAY_SIZE(achFriendlyUrl),
                           MAKEINTRESOURCE(IDS_FRIENDLYURL_AT), achShortName, achHostName);
                }
                else if (pchShortName && *(pchShortName + 1))
                    Concat(achFriendlyUrl, ARRAY_SIZE(achFriendlyUrl), pchShortName + 1, x, y);
                else if (*achHostName)
                    _tcscat(achFriendlyUrl, achHostName);
            }
            else
                Concat(achFriendlyUrl, ARRAY_SIZE(achFriendlyUrl), achUrl, x, y);

            if (fPreface && !fMail)
                Format(FMT_OUT_ALLOC, &pchFriendlyUrl, 0, MAKEINTRESOURCE(IDS_FRIENDLYURL_SHORTCUTTO),
                       achFriendlyUrl, achSuffix);
            else
                Format(FMT_OUT_ALLOC, &pchFriendlyUrl, 0, _T("<0s> <1s>"),
                       achFriendlyUrl, achSuffix);

            return pchFriendlyUrl;
        }
        else
            Concat(achFriendlyUrl, ARRAY_SIZE(achFriendlyUrl), achUrl, x, y);

        MemAllocString(Mt(GetFriendlyUrl), achFriendlyUrl, &pchFriendlyUrl);
    }
    else
        MemAllocString(Mt(GetFriendlyUrl), achUrl, &pchFriendlyUrl);

    return pchFriendlyUrl;
}


HRESULT
ExpandUrlWithBaseUrl(LPCTSTR pchBaseUrl, LPCTSTR pchRel, TCHAR ** ppchUrl)
{
    HRESULT hr;
    TCHAR achBuf[pdlUrlLen];
    DWORD cchBuf;
    BOOL fCombine;
    DWORD dwSize;

    *ppchUrl = NULL;

    if (pchRel == NULL) // note: NULL is different from "", which is more similar to "."
    {
        hr = MemAllocString(Mt(ExpandUrlWithBaseUrl), _T(""), ppchUrl);
        goto Cleanup;
    }

    if (!pchRel)
    {
        hr = MemAllocString(Mt(ExpandUrlWithBaseUrl), pchBaseUrl, ppchUrl);
    }
    else
    {
        hr = CoInternetCombineUrl(pchBaseUrl, pchRel, URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE, achBuf, ARRAY_SIZE(achBuf), &cchBuf, 0);
        if (hr)
            goto Cleanup;

        if (S_OK == CoInternetQueryInfo(pchBaseUrl, QUERY_RECOMBINE, 0, &fCombine, sizeof(BOOL), &dwSize, 0)  &&
            fCombine)
        {
            TCHAR achBuf2[pdlUrlLen];
            DWORD cchBuf2;

            hr = CoInternetCombineUrl(pchBaseUrl, achBuf, URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE, achBuf2, ARRAY_SIZE(achBuf2), &cchBuf2, 0);
            if (hr)
                goto Cleanup;

            hr = THR(MemAllocString(Mt(ExpandUrlWithBaseUrl), achBuf2, ppchUrl));
        }
        else
        {
            hr = THR(MemAllocString(Mt(ExpandUrlWithBaseUrl), achBuf, ppchUrl));
        }
    }

Cleanup:
    RRETURN(hr);
}

BOOL
IsUrlOnNet(const TCHAR *pchUrl)
{
    DWORD fOnNet;
    ULONG cb;
    
    switch (GetUrlScheme(pchUrl))
    {
    case URL_SCHEME_FILE:
    case URL_SCHEME_RES:
    case URL_SCHEME_JAVASCRIPT:
    case URL_SCHEME_VBSCRIPT:
        return FALSE;
        break;

    case URL_SCHEME_HTTP:
    case URL_SCHEME_HTTPS:
    case URL_SCHEME_FTP:
        return TRUE;
        break;

    default:
        if (!(CoInternetQueryInfo(pchUrl, QUERY_USES_NETWORK, 0, &fOnNet, sizeof(fOnNet), &cb, 0)) && cb == sizeof(fOnNet))
            return fOnNet;
        return FALSE;
        break;
    }
}


