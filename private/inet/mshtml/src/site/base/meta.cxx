//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       meta.cxx
//
//  Contents:   META tag processing
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifdef WIN16
DeclareTag(tagShDocMetaRefresh, "ShDocVwSkel", "HTTP (and meta) Refresh");
#endif

DeclareTag(tagMeta, "Meta", "Trace meta tags");

MtDefine(CDoc_pvPics, CDoc, "CDoc::_pvPics buffer")

// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);

#ifdef WIN16
#define BSTRPAD _T("    ")
#else
#  ifdef UNIX
     // On Unix sizeof(WCHAR) == 4
#    define BSTRPAD _T(" ")
#  else
     // or can be 4 ansi spaces.
#    define BSTRPAD _T("  ")
#  endif // UNIX
#endif

BOOL ParseRefreshContent(LPCTSTR pchContent,
    UINT * puiDelay, LPTSTR pchUrlBuf, UINT cchUrlBuf)
{
    // We are parsing the following string:
    //
    //  [ws]* [0-9]+ { ws | ; }* url [ws]* = [ws]* { ' | " } [any]* { ' | " }
    //
    // Netscape insists that the string begins with a delay.  If not, it
    // ignores the entire directive.  There can be more than one URL mentioned,
    // and the last one wins.  An empty URL is treated the same as not having
    // a URL at all.  An empty URL which follows a non-empty URL resets
    // the previous URL.

    enum { PRC_START, PRC_DIG, PRC_DIG_WS_SEMI, PRC_DIG_DOT, PRC_SEMI_URL,
        PRC_SEMI_URL_EQL, PRC_SEMI_URL_EQL_ANY };
    #define ISSPACE(ch) (((ch) == 32) || ((unsigned)((ch) - 9)) <= 13 - 9)

    UINT uiState = PRC_START;
    UINT uiDelay = 0;
    LPCTSTR pch = pchContent;
    LPTSTR  pchUrl = NULL;
    UINT    cchUrl = 0;
    TCHAR   ch, chDel = 0;

    *pchUrlBuf = 0;

    do
    {
        ch = *pch;

        switch (uiState)
        {
            case PRC_START:
                if (ch >= '0' && ch <= '9')
                {
                    uiState = PRC_DIG;
                    uiDelay = ch - '0';
                }
                else if (ch == '.')
                    uiState = PRC_DIG_DOT;
                else if (!ISSPACE(ch))
                    goto done;
                break;

            case PRC_DIG:
                if (ch >= '0' && ch <= '9')
                    uiDelay = uiDelay * 10 + ch - '0';
                else if (ch == '.')
                    uiState = PRC_DIG_DOT;
                else if (ISSPACE(ch) || ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else
                    goto done;
                break;

            case PRC_DIG_DOT:
                if (ISSPACE(ch) || ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else if ((ch < '0' || ch > '9') && (ch != '.'))
                    goto done;
                break;

            case PRC_DIG_WS_SEMI:
                if (    (ch == 'u' || ch == 'U')
                    &&  (pch[1] == 'r' || pch[1] == 'R')
                    &&  (pch[2] == 'l' || pch[2] == 'L'))
                {
                    uiState = PRC_SEMI_URL;
                    pch += 2;
                }
                else if (!ISSPACE(ch) && ch != ';')
                    goto done;
                break;

            case PRC_SEMI_URL:
                if (ch == '=')
                {
                    uiState = PRC_SEMI_URL_EQL;
                    *pchUrlBuf = 0;
                }
                else if (ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else if (!ISSPACE(ch))
                    goto done;
                break;

            case PRC_SEMI_URL_EQL:
                if (ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else if (!ISSPACE(ch))
                {
                    uiState = PRC_SEMI_URL_EQL_ANY;

                    pchUrl = pchUrlBuf;
                    cchUrl = cchUrlBuf;

                    if (ch == '\''|| ch == '\"')
                        chDel = ch;
                    else
                    {
                        chDel = 0;
                        *pchUrl++ = ch;
                        cchUrl--;
                    }
                }
                break;
                        
            case PRC_SEMI_URL_EQL_ANY:
                if (    !ch
                    ||  ( chDel && ch == chDel)
                    ||  (!chDel && ch == ';'))
                {
                    *pchUrl = 0;
                    uiState = PRC_DIG_WS_SEMI;
                }
                else if (cchUrl > 1)
                {
                    *pchUrl++ = ch;
                    cchUrl--;
                }
                break;
        }

        ++pch;

    } while (ch);

done:

    *puiDelay = uiDelay;

    return(uiState >= PRC_DIG);
}

void
CDoc::ProcessHttpEquiv(LPCTSTR pchHttpEquiv, LPCTSTR pchContent)
{
    VARIANT var;
    TCHAR *pch;
    DWORD *plen;
    BOOL fRefresh;
    BOOL fExpireImmediate;
    HRESULT hr;

    TraceTag((tagMeta, "META http-equiv=\"%S\" content=\"%S\"", pchHttpEquiv, pchContent));

    // All http-equiv must be given to us before parsing is complete.  If
    // not, then we are probably parsing for paste and came across a <META>
    // tag.  Just ignore it.

    if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        TraceTag((tagMeta, "META _LoadStatus >= LOADSTATUS_PARSE_DONE, we're outta here"));
        return;
    }

    VariantInit(&var);
    var.vt = VT_BSTR;

    // Special case for "PICS-Label" until it gets merged with normal
    // command target mechanism in the shell.

    if (StrCmpIC(pchHttpEquiv, _T("PICS-Label")) == 0)
    {
        if (_pctPics)
        {
            // We've got a command target, just send up the string.
            if (Format(FMT_OUT_ALLOC, &pch, 0, BSTRPAD _T("<0s>"), pchContent) == S_OK)
            {
                plen = (DWORD *)pch;
                var.bstrVal = (BSTR) ((LPBYTE) pch + sizeof(DWORD));
                *plen = _tcslen(var.bstrVal) * sizeof(TCHAR);
                _pctPics->Exec(&CGID_ShellDocView, SHDVID_PICSLABELFOUND, 0, &var, NULL);
                delete (pch);
            }

        }
        else if (_pvPics != (void *)(LONG_PTR)(-1))
        {
            // No command target yet.  Save the string until it becomes
            // available later.

            UINT cbOld = _pvPics ? *(DWORD *)_pvPics : sizeof(DWORD);
            UINT cbNew = (_tcslen(pchContent) + 1) * sizeof(TCHAR);

            hr = MemRealloc(Mt(CDoc_pvPics), &_pvPics, cbOld + cbNew);

            if (hr == S_OK)
            {
                *(DWORD *)_pvPics = cbOld + cbNew;
                memcpy((BYTE *)_pvPics + cbOld, pchContent, cbNew);
            }
        }
    }

    fRefresh = StrCmpIC(pchHttpEquiv, _T("Refresh")) == 0;

    if (    _pClientSite
        &&  (   !(_dwLoadf & DLCTL_NO_CLIENTPULL)
            ||  !fRefresh))
    {
        TCHAR   ach[pdlUrlLen];
        UINT    uiDelay;
        TCHAR   cBuf[pdlUrlLen];
        TCHAR * pchUrl = cBuf;
        TCHAR * pchNew = NULL;

        // If this is a Refresh header, we have to get the URL and expand
        // it because it could be relative.

        if (    fRefresh
            &&  ParseRefreshContent(pchContent, &uiDelay, ach, ARRAY_SIZE(ach))
            &&  ach[0])
        {
            // Netscape ignores any BASE tags which come before the <META>
            // when expanding the relative URL.  So do we.

            hr = THR(ExpandUrl(ach, ARRAY_SIZE(cBuf), pchUrl, EXPANDIGNOREBASETAGS));

            // Need to check if the URL is the same as one of the parent CDoc
            //
            if (hr == S_OK)
            {
                CStr    cstrSpecialURL;

                if (WrapSpecialUrl(pchUrl, &cstrSpecialURL, _cstrUrl, FALSE, FALSE) != S_OK)
                {
                    cstrSpecialURL.Set(_T("about:blank"));     // Something bad happened, security spoof, so make URL empty.
                }
#if 0
//     This code checks for recursions, but unfortunately it causes problems with PowerSite 1.5
//    Powersite help system writes a code into its frame that does a refresh loading the parent
//    URL. We detect is as a recursion, and do not display the file. Actually there is no recursion, 
//    because the script detects is and solves the problem. The code here was not protecting from 
//    the obvious case when a frame directly was doing a refresh on itself.
//        Check IE5 bugs 48016 and 15720 for details
                CDoc  * pDoc = _pDocParent;
                DWORD   cchParentUrl;
                TCHAR   parentUrl[INTERNET_MAX_URL_LENGTH];
                LPTSTR  pchLocation;
                  while (pDoc)
                {
                    if (pDoc->_cstrUrl)
                    {
                        cchParentUrl = INTERNET_MAX_URL_LENGTH;
                        hr = THR(UrlCanonicalize(
                                    (LPTSTR) pDoc->_cstrUrl,
                                    (LPTSTR) parentUrl,
                                    &cchParentUrl,
                                    URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE));

                        if (hr)
                            break;

                        pchLocation = (LPTSTR) UrlGetLocation(parentUrl);

                        if (pchLocation)
                            * pchLocation = _T('\0');

                        if (!UrlCompare(parentUrl, pchUrl, TRUE))
                        {
                            cstrSpecialURL.Set(_T("about:blank"));
                            break;
                        }
                    }

                    pDoc = pDoc->_pDocParent;
                }
#endif

                hr = THR(Format(FMT_OUT_ALLOC, &pchNew, 0,
                            _T("<0d>;url=<1s>"), (long)uiDelay, (LPTSTR)cstrSpecialURL));

                if (hr == S_OK)
                {
                    pchContent = pchNew;
                }
            }
        }

        // Send to the command target of the client site.  Note that
        // the string needs to be in the format http-equiv:content.

        if (Format(FMT_OUT_ALLOC, &pch, 0, BSTRPAD _T("<0s>:<1s>"), pchHttpEquiv, pchContent) == S_OK)
        {
            plen = (DWORD *)pch;
            var.bstrVal = (BSTR) ((LPBYTE) pch + sizeof(DWORD));
            *plen = _tcslen(var.bstrVal) * sizeof(TCHAR);

#ifdef WIN16
            if (fRefresh)
            {
                // we handle the refresh method inside of trident,
                // at least for Beta 1.

                TraceTag((tagShDocMetaRefresh, "Got %s content = %s", pchHttpEquiv, pchContent));
                TraceTag((tagShDocMetaRefresh, "bstr = %s", var.bstrVal));

                // assume it's parsed all nice for us...
                long lMSec, lTimerId;
                lMSec = atol(pchContent) * 1000; // string must start w/an integer.

                hr = SetTimeout(var.bstrVal, lMSec, FALSE, &var, &lTimerId);

                TraceTag((tagShDocMetaRefresh, "SetTimeout timerid=%ld, hr=%hr", lTimerId, hr));

                // BUGWIN16: We need to store the lTimerId so that we can 
                // call FormsKillTimer on it when we deactivate.
            }
            else
#endif
            {
                CTExec(_pClientSite, NULL, OLECMDID_HTTPEQUIV, 0, &var, NULL);
                TraceTag((tagMeta, "META Invoking OLECMDID_HTTPEQUIV"));
            }

            MemFree(pch);
        }

        MemFree(pchNew);
    }

    fExpireImmediate = FALSE;

    // Special case for "Pragma: no-cache" http-equiv.

    if (    _cstrUrl
        &&  !StrCmpIC(pchHttpEquiv, _T("Pragma"))
        &&  !StrCmpIC(pchContent, _T("no-cache")))
    {
        if (GetUrlScheme(_cstrUrl) == URL_SCHEME_HTTPS)
        {
            // First try to delete the cache entry immediately.  If this fails, it's because the document
            // isn't completely loaded.  Set a flag to delete the cache entry upon LOADSTATUS_PARSE_DONE.

            if(!DeleteUrlCacheEntry(_cstrUrl))
            {
                _fDeleteUrlCacheEntry = TRUE;
            }
        }
        else
            fExpireImmediate = TRUE;
    }
    
    // Special case for "Expires" http-equiv.

    if (    fExpireImmediate
        || (!_fGotHttpExpires && _cstrUrl
            &&  !StrCmpIC(pchHttpEquiv, _T("Expires"))))
    {
        SYSTEMTIME stime;
        INTERNET_CACHE_ENTRY_INFO icei;

        if (fExpireImmediate || !InternetTimeToSystemTime(pchContent, &stime, 0))
        {
            // If the conversion failed, assume the document expires now.
            GetSystemTime(&stime);
        }

        SystemTimeToFileTime(&stime, &icei.ExpireTime);
        SetUrlCacheEntryInfo(_cstrUrl, &icei, CACHE_ENTRY_EXPTIME_FC);

        // We only care about the first one if there are many.

        _fGotHttpExpires = TRUE;
    }

}

void
CDoc::ProcessMetaName(LPCTSTR pchName, LPCTSTR pchContent)
{
    if (!StrCmpIC(pchName, _T("GENERATOR")))
    {
        // NOTE: This check should stop at the first zero of the major version number
        //       since HP98 generates documents with a number of different major/minor
        //       version numbers. (brendand)
        if (!StrCmpNIC(pchContent, _T("MMEditor Version 00.00.0"), 24) ||
            !StrCmpIC(pchContent, _T("IE4 Size and Overflow")) )    // For public consumption.
        {
            _fInHomePublisherDoc = TRUE;
        }
    }
}

HRESULT
CDoc::SetPicsCommandTarget(IOleCommandTarget *pctPics)
{
    ReplaceInterface(&_pctPics, pctPics);

    if (_pctPics == NULL)
    {
        // Setting _pvPics to this value tells ProcessHttpEquiv to ignore
        // any further PICS-Label directives and not attempt to queue them
        // up in _pvPics for later.

        if (_pvPics == NULL)
        {
            _pvPics = (void *)(LONG_PTR)(-1);
        }

        return(S_OK);
    }

    if (_pvPics && _pvPics != (void *)(LONG_PTR)(-1))
    {
        VARIANT var;
        BYTE *  pb    = (BYTE *)_pvPics + sizeof(DWORD);
        BYTE *  pbEnd = pb + *(DWORD *)_pvPics;
        UINT    cb;

        VariantInit(&var);
        var.vt = VT_BSTR;

        while (pb < pbEnd)
        {
            cb = (_tcslen((TCHAR *)pb) + 1) * sizeof(TCHAR);
            *(DWORD *)(pb - sizeof(DWORD)) = cb;
            var.bstrVal = (BSTR)pb;
            _pctPics->Exec(&CGID_ShellDocView, SHDVID_PICSLABELFOUND,
                0, &var, NULL);
            pb += cb;
        }

        MemFree(_pvPics);
        _pvPics = NULL;
    }

    if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        _pctPics->Exec(&CGID_ShellDocView, SHDVID_NOMOREPICSLABELS,
            0, NULL, NULL);
        ClearInterface(&_pctPics);
    }

    return(S_OK);
}
