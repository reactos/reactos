//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       mailprot.cxx
//
//  Contents:   Implementation of the mailto protocol
//
//  History:    04-26-97    Yin XIE     Created
//              06-20-97    Yin XIE     Added cc;bcc;body;to
//              06-29-97    Yin XIE     Added non MAPI mail client support
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef NO_MULTILANG

 #ifndef X_MLANG_H_
 #define X_MLANG_H_
 #include <mlang.h>
 #endif

 extern IMultiLanguage *g_pMultiLanguage;

 #ifndef X_CODEPAGE_H_
 #define X_CODEPAGE_H_
 #include <codepage.h>
 #endif

#endif // !NO_MULTILANG

#ifndef X_MAPI_H_
#define X_MAPI_H_
#include <mapi.h>
#endif

#ifndef X_MAILPROT_HXX_
#define X_MAILPROT_HXX_
#include "mailprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecuteA (for AXP)
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#define URL_KEYWL_MAILTO      7           // length of "mailto:"

MtDefine(CMailtoProtocol, Protocols, "CMailtoProtocol")
MtDefine(CMailtoProtocolPostData, CMailtoProtocol, "CMailtoProtocol::_postData")
MtDefine(CMailtoProtocolRecips, CMailtoProtocol, "CMailtoProtocol mailmsg.lpRecips")
MtDefine(CMailtoProtocolNoteText, CMailtoProtocol, "CMailtoProtocol mailmsg.lpszNoteText")
MtDefine(CMailtoProtocolFileDesc, CMailtoProtocol, "CMailtoProtocol MapiFileDesc")
MtDefine(CMailtoProtocolMbfwc, CMailtoProtocol, "CMailtoProtocol::MultiByteFromWideChar")
MtDefine(CMailtoFactory, Protocols, "CMailtoFactory")

//+---------------------------------------------------------------------------
//
//  Function:   CreateMailtoProtocol
//
//  Synopsis:   Creates a resource Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateMailtoProtocol(IUnknown *pUnkOuter)
{
    return new CMailtoProtocol(pUnkOuter);
}

const CBase::CLASSDESC CMailtoProtocol::s_classdesc =
{
    &CLSID_MailtoProtocol,          // _pclsid
};

//+---------------------------------------------------------------------------
//
//  Mailto Class Factory
//
//+---------------------------------------------------------------------------

CMailtoFactory g_cfMailtoProtocol   (CreateMailtoProtocol);

// IOInetProtocolInfo methods
HRESULT
CMailtoFactory::QueryInfo(LPCWSTR         pwzUrl,
                          QUERYOPTION     QueryOption,
                          DWORD           dwQueryFlags,
                          LPVOID          pBuffer,
                          DWORD           cbBuffer,
                          DWORD *         pcbBuf,
                          DWORD           dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    switch ( QueryOption )
    {
    case QUERY_CAN_NAVIGATE:

        // Need at least a DWORD
        if (cbBuffer < 4)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (pcbBuf)
        {
            *pcbBuf = 4; // 4 bytes needed for DWORD
        }

        *((DWORD *)pBuffer) = 0; // make sure we return false (0) for mailto prot.
        hr = S_OK;
        break;

    default:
        hr = THR(super::QueryInfo(
                pwzUrl,
                QueryOption,
                dwQueryFlags,
                pBuffer,
                cbBuffer,
                pcbBuf,
                dwReserved));
        break;
    }

Cleanup:
    RRETURN1(hr, INET_E_DEFAULT_ACTION);
}


//+---------------------------------------------------------------------------
//
//  Method:     CMailtoProtocol::CMailtoProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CMailtoProtocol::CMailtoProtocol(IUnknown *pUnkOuter) : super(pUnkOuter)
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CMailtoProtocol::~CMailtoProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CMailtoProtocol::~CMailtoProtocol()
{
}

STDAPI_ (DWORD) MailtoThreadProc(void *pmp)
{
    ((CMailtoProtocol *)pmp)->ParseAndBind();
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMailtoProtocol::Start
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CMailtoProtocol::Start(
    LPCWSTR pchUrl, 
    IInternetProtocolSink *pTrans, 
    IInternetBindInfo *pOIBindInfo,
    DWORD grfSTI, 
    HANDLE_PTR dwReserved)
{
    HRESULT         hr = NOERROR;
    TCHAR           ach[pdlUrlLen];
    DWORD           dwSize;
    
    Assert(!_pProtSink && pOIBindInfo && pTrans && !_cstrURL);

    if ( !(grfSTI & PI_PARSE_URL))
    {
        ReplaceInterface(&_pProtSink, pTrans);
        ReplaceInterface(&_pOIBindInfo, pOIBindInfo);
    }

    _bindinfo.cbSize = sizeof(BINDINFO);
    hr = THR(pOIBindInfo->GetBindInfo(&_grfBindF, &_bindinfo));

    if ((_grfBindF & BINDF_NO_UI) || (_grfBindF & BINDF_SILENTOPERATION))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pOIBindInfo->GetBindString(BINDSTRING_POST_DATA_MIME, &_pszMIMEType, 1, &dwSize));
    // It can fail, but _pszMIMEType will be NULL.
    Assert( !hr || (hr && _pszMIMEType == NULL));

    _cp = _bindinfo.dwCodePage ? _bindinfo.dwCodePage : g_cpDefault;

    //
    // First get the basic url.  Unescape it first.
    //
    hr = THR(CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0, ach, ARRAY_SIZE(ach), &dwSize, 0));
    if (hr)
        goto Cleanup;
    
    hr = THR(_cstrURL.Set(ach));
    if (hr)
        goto Cleanup;

    //
    // Now append any extra data if needed.
    //
    
    if (_bindinfo.szExtraInfo)
    {
        hr = THR(_cstrURL.Append(_bindinfo.szExtraInfo));
        if (hr)
            goto Cleanup;
    }
    
    switch (_bindinfo.dwBindVerb)
    {
    case    BINDVERB_POST:
        void *pData;
        _cPostData = _bindinfo.cbstgmedData;

        if (!_cPostData)
            break;

        _postData = (BYTE *)MemAlloc(Mt(CMailtoProtocolPostData), _cPostData);

        // BUGBUG (yinxie) need to add IStream thing when necessary
        Assert(_bindinfo.stgmedData.tymed == TYMED_HGLOBAL);

        pData = GlobalLock(_bindinfo.stgmedData.hGlobal);
        if (pData)
        {
            memcpy(_postData, pData, _cPostData);
            GlobalUnlock(_bindinfo.stgmedData.hGlobal);
        }
        else
        {
            _cPostData = 0;
        }
        break;
    default:
        _cPostData = 0;
    }

    _grfSTI = grfSTI;

    //
    // If forced to go async, return E_PENDING now, and
    // perform binding when we get the Continue.
    //
    
    if (grfSTI & PI_FORCE_ASYNC)
    {
        PROTOCOLDATA    protdata;

        hr = E_PENDING;
        protdata.grfFlags = PI_FORCE_ASYNC;
        protdata.dwState = BIND_ASYNC;
        protdata.pData = NULL;
        protdata.cbData = 0;

        _pProtSink->Switch(&protdata);
    }
    else
    {
        //hr = THR(ParseAndBind());
        DWORD   idThread;
        HANDLE  hThread  = CreateThread(NULL, 0, MailtoThreadProc, this, 0, &idThread);
        if (hThread)
        {
            CloseHandle(hThread);
            hr = S_OK;
        }
    }

Cleanup:
    RRETURN(hr);
}

// keep to,cc,bcc at begining, because we use this in loop
static const MAILTOURLSTRUCT   aURLInfo[]=
{
    {_T("to="),      3,    _T(" ")},
    {_T("cc="),      3,    _T(" ")},
    {_T("bcc="),     4,    _T(" ")},
    {_T("subject="), 8,    NULL},
    {_T("body="),    5,    NULL},
};

TCHAR *
CMailtoProtocol::GetNextRecipient(TCHAR * lptszRecipients)
{
    // get ride of all the spaces first,
    while (*lptszRecipients && *lptszRecipients == _T(' '))
        lptszRecipients++;

    while (*lptszRecipients && *lptszRecipients != _T(' ') && *lptszRecipients != _T(';'))
        lptszRecipients++;

    if (*lptszRecipients)
        return (lptszRecipients);

    return(NULL);
}

HRESULT
CMailtoProtocol::ParseMailToAttr()
{
    HRESULT hr = S_OK;
    int     i = 0;
    int     l = 0;
    TCHAR * pch = NULL;
    TCHAR * pch2 = NULL;

    //
    // The url is of the following syntax:
    // mailto:<email address>?subject=<subject>
    //
    
    Assert(_tcsnipre(TEXT("mailto:"), URL_KEYWL_MAILTO, _cstrURL, -1));

    for (i=0; i<mailtoAttrNUM; i++)
    {
        hr = THR(_aCStrAttr[i].Set(0));
        if (hr)
            goto Cleanup;
    }

    // parse recipients

    pch  = _tcschr(_cstrURL, _T('?'));
    if (pch)
    {
        hr = THR(_aCStrAttr[mailtoAttrTO].Set(_cstrURL+URL_KEYWL_MAILTO, pch-_cstrURL-URL_KEYWL_MAILTO));
    }
    else
    {
        hr = THR(_aCStrAttr[mailtoAttrTO].Set(_cstrURL+URL_KEYWL_MAILTO));
    }
    if (hr)
        goto Cleanup;

    if (!_aCStrAttr[mailtoAttrTO].IsNull() && _aCStrAttr[mailtoAttrTO].Length())
    {
        // append seperator to allow "to:"
        hr  = THR(_aCStrAttr[mailtoAttrTO].Append(aURLInfo[mailtoAttrTO].lptszSep, 1));
        if (hr)
            goto Cleanup;
    }

    // parse attributes

    while (pch)
    {
        pch++;
        for (i = 0; i < mailtoAttrNUM; i ++)
        {
            if (_tcsnipre(aURLInfo[i].lptszAttr, aURLInfo[i].attrLen, pch, -1))
            {
                pch     = _tcschr(pch, _T('='));
                if (!pch)
                    break;

                pch2    = _tcschr(pch, _T('&'));
                if (!pch2)
                {
                    // if no more subject, the next item is the end of URL
                    l = _tcslen(pch) - 1;
                }
                else
                {
                    l = pch2 - pch - 1;    
                }

                if (l <= 0)
                    break;

                hr  = THR(_aCStrAttr[i].Append(++pch, l));
                if (hr)
                    goto Cleanup;

                // append the seperator, if needed

                if (aURLInfo[i].lptszSep)
                {
                    hr  = THR(_aCStrAttr[i].Append(aURLInfo[i].lptszSep, 1));
                    if (hr)
                        goto Cleanup;
                }
                break;
            }
        }

        if (!pch || !*pch)
        {
            // we are at the end of URL
            // let's break
            break;
        }
        pch = _tcschr(pch, _T('&'));
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CMailtoProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding & execution of script.
//
//----------------------------------------------------------------------------

HRESULT
CMailtoProtocol::ParseAndBind()
{
    CLock           Lock(this);

    HRESULT         hr = S_OK;

    TCHAR *         pch = NULL;

#ifndef UNIX
    HMODULE         hMail = 0;

    TCHAR           aSubject[pdlUrlLen];
#endif

    TCHAR           aRecips[pdlUrlLen];
    TCHAR *         pRecipsPtr[] = {NULL, NULL, NULL};
    int             pRecipsNum[] = {0, 0, 0};
    char            *aBuffer=NULL;
    LPTSTR          pEnd = 0;

    UINT            nRecips = 0;
    UINT            cUnicode = 0;
    UINT            cAnsi = 0;
    UINT            l = 0;

    int             i;

    LPMAPISENDMAIL pfnSendMail = 0;

#ifndef UNIX
    MapiMessage	    mapimsg;
    memset(&mapimsg, 0 , sizeof(mapimsg));

    // speed up mail client launch
    if (!_cPostData)
    {
        hr = RunMailClient();
        if (!_fAborted && _pProtSink)
        {
            _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
            _pProtSink->ReportData(_bscf, 1, 1);
            _pProtSink->ReportResult(hr, 0, 0);
        }
        goto Cleanup;
    }
#endif

    hr = THR(ParseMailToAttr());
    if (hr)
        goto Cleanup;

    l = pdlUrlLen; 
    pch = aRecips;
    for (i = mailtoAttrTO; i <= mailtoAttrBCC; i++)
    {
        pRecipsPtr[i] = pch;
        cUnicode = _aCStrAttr[i].Length();
        if (cUnicode > l)
        {
            // if too big, truncate
            _tcsncpy(pch, (LPTSTR)_aCStrAttr[i], l-1);
            pch[l-1] = 0;
            pEnd = pch + l;
        }
        else if (cUnicode > 0)
        {
            _tcscpy(pch, (LPTSTR)_aCStrAttr[i]);
            pEnd = pch + cUnicode;
        }
        else
        {
            break;
        }

        while (pch < pEnd)
        {
            pch = GetNextRecipient(pch);
            if (!pch)
                break;
            // if found seperator, and the mail address is good
            *pch = 0;
            pch ++;
            nRecips ++;
            pRecipsNum[i] ++;
        }

        pch = pEnd;
    }

#ifndef UNIX
    if (nRecips)
    {
        mapimsg.lpRecips = (MapiRecipDesc *)MemAlloc(Mt(CMailtoProtocolRecips), nRecips * sizeof(MapiRecipDesc));
        memset(mapimsg.lpRecips, 0 , nRecips * sizeof(MapiRecipDesc));
        if (!mapimsg.lpRecips)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    for (i = mailtoAttrTO; i <= mailtoAttrBCC; i++)
    {
        if (pRecipsNum[i])
        {
            hr = THR(SetMAPIRecipients(&mapimsg, pRecipsPtr[i],
                                                pRecipsNum[i], i));

            if (FAILED(hr))
                goto Cleanup;
            mapimsg.nRecipCount += pRecipsNum[i];
        }
    }

    if (_pszMIMEType && StrCmpIC(_pszMIMEType, CFSTR_MIME_TEXT ) == 0)
    {   
        // This is always 8-bit ASCII because that is what
        // text/plain MIME encoding is!
        LPSTR pszText = (LPSTR) MemAlloc(Mt(CMailtoProtocolNoteText), _cPostData + 1);
        memcpy(pszText, _postData, _cPostData);
        pszText[_cPostData] = 0; // terminate.
        mapimsg.lpszNoteText = pszText;
    }
    else
    {
        hr = THR(SetMAPIAttachement(&mapimsg));

        if (FAILED(hr))
            goto Cleanup;
    }

    cUnicode = _aCStrAttr[mailtoAttrSUBJECT].Length();
    if (!cUnicode && _postData)
    {
        // there is no subject, let's add the default one
        LoadString(GetResourceHInst(), IDS_MAILTO_DEFAULTSUBJECT,
                                aSubject, ARRAY_SIZE(aSubject));
        hr  = THR(_aCStrAttr[mailtoAttrSUBJECT].Set(aSubject));
        if (hr)
            goto Cleanup;
        cUnicode = _aCStrAttr[mailtoAttrSUBJECT].Length();
    }
    if (cUnicode)
    {
        hr = THR(MultiByteFromWideChar((WCHAR *)_aCStrAttr[mailtoAttrSUBJECT],
                                        cUnicode, &aBuffer, &cAnsi));

        if (FAILED(hr))
            goto Cleanup;

        mapimsg.lpszSubject = aBuffer;
        mapimsg.lpszConversationID = aBuffer;
    }

    // KB: Netscape ignore body attribute when there is post data
    // 
    if (!_cPostData || 
       (_pszMIMEType && StrCmpIC(_pszMIMEType, CFSTR_MIME_TEXT ) != 0) )
    {   
        cUnicode = _aCStrAttr[mailtoAttrBODY].Length();
        if (cUnicode)
        {
            hr = THR(MultiByteFromWideChar((WCHAR *)_aCStrAttr[mailtoAttrBODY],
                                            cUnicode, &aBuffer, &cAnsi));

            if (FAILED(hr))
                goto Cleanup;

            mapimsg.lpszNoteText = aBuffer;
        }
    }

    if (_cPostData)
    {
        // for performance reason, we prefer command line
        // we only load the DLLs when this is a form submission
        // and there is a postdata
        hr = LoadMailProvider(&hMail);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (hMail)
    {
        pfnSendMail = (LPMAPISENDMAIL)GetProcAddress(hMail, "MAPISendMail");

        if (!pfnSendMail)
            goto Cleanup;
    }
#endif // !UNIX

    if (!_fAborted && _pProtSink)
    {
        _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
        _pProtSink->ReportData(_bscf, 1, 1);
        _pProtSink->ReportResult(hr, 0, 0);
    }

#ifndef UNIX
    if (hMail)
    {
        // this is going to generate memory leak
        DbgMemoryTrackDisable(TRUE);

        pfnSendMail(0,
                    (ULONG)HandleToLong(GetActiveWindow()),
                    &mapimsg,
                    _cPostData ? MAPI_LOGON_UI : (MAPI_LOGON_UI | MAPI_DIALOG),
                    0);
        DbgMemoryTrackDisable(FALSE);
    }
    else
    {
        // need to execute Shell command
        // no form submission supported
        hr = RunMailClient();
    }
#else
    hr = LaunchUnixClient(aRecips, nRecips);
#endif // !UNIX

Cleanup:
    for (i=0; i<mailtoAttrNUM; i++)
    {
        _aCStrAttr[i].Free();
    }

    _cstrURL.Free();

#ifndef UNIX
    ReleaseMAPIMessage(&mapimsg);
    if (hMail)
    {
        FreeLibrary(hMail);
    }
#endif // !UNIX

    if (_postData)
    {
        MemFree(_postData);
        _postData=NULL;
        _cPostData = 0;
    }

    if (_pszMIMEType)
    {
        CoTaskMemFree(_pszMIMEType);
        _pszMIMEType = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CMailtoProtocol::RunMailClient
//
//  Synopsis:   Load mail client in case we can not support form submission
//
//  Returns:    HMODULE
//
//-------------------------------------------------------------------------

HRESULT CMailtoProtocol::RunMailClient()
{
    HRESULT     hr = E_FAIL;
    char        *lpszShellCmd=0;
    UINT        cAnsi = 0;
    HINSTANCE   hinst=0;
    TCHAR       aCaption[64];
    TCHAR       aText[128];

    hr = THR(MultiByteFromWideChar((WCHAR *)_cstrURL, _tcslen(_cstrURL),
                                    &lpszShellCmd, &cAnsi));
    if (FAILED(hr))
        goto Cleanup;
    hinst = ShellExecuteA(  GetActiveWindow(),
                            NULL,
                            lpszShellCmd,
                            NULL,
                            NULL,
                            SW_SHOWNORMAL);

    if ((DWORD_PTR) hinst <= 32)
    {
        hr = GetLastWin32Error();

        if (hr)
        {
            LoadString(GetResourceHInst(), IDS_MESSAGE_BOX_TITLE, aCaption, 64);
            LoadString(GetResourceHInst(), IDS_MAILTO_MAILCLIENTNOTFOUND,
                                            aText, 128);
            MessageBox(
                    GetActiveWindow(),
                    aText,
                    aCaption,  
                    MB_OK | MB_TASKMODAL
                    );
        }
    }
    else
    {
        hr = S_OK;
    }

Cleanup:

    if (lpszShellCmd)
        MemFree(lpszShellCmd);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CMailtoProtocol::LoadMailProvider
//
//  Synopsis:   Load mail provider MAPI DLL
//
//  Returns:    HMODULE
//
//-------------------------------------------------------------------------

HRESULT CMailtoProtocol::LoadMailProvider(HMODULE *pHMod)
{
    HRESULT hr = S_OK;
    TCHAR   szMAPIDLL[MAX_PATH];
    TCHAR   aBuffer[MAX_PATH];
    DWORD   cb = MAX_PATH;
    HKEY    hkey;
    LONG    lSuccess;
    BOOL    fReadFromHKCU = TRUE;

    *pHMod = NULL;

    lSuccess = RegQueryValue(HKEY_CURRENT_USER,
                            TEXT("Software\\Clients\\Mail"),
                            szMAPIDLL,
                            (LONG *)&cb);

    if (lSuccess != ERROR_SUCCESS || cb <= sizeof(TCHAR))
    {
        fReadFromHKCU = FALSE;
        cb = MAX_PATH;
        lSuccess = RegQueryValue(HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Clients\\Mail"),
                            szMAPIDLL,
                            (LONG *)&cb);
    }

    if (lSuccess != ERROR_SUCCESS)
        goto Cleanup;

    _tcscpy(aBuffer, TEXT("Software\\Clients\\Mail\\"));
    _tcscat(aBuffer, szMAPIDLL);

    lSuccess = RegOpenKeyEx(fReadFromHKCU
                                    ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
                                aBuffer, NULL, KEY_QUERY_VALUE, &hkey);

    if (lSuccess != ERROR_SUCCESS)
        goto Cleanup;

    cb = MAX_PATH;
    lSuccess = RegQueryValueEx(hkey, TEXT("DLLPath"), 0, NULL, (LPBYTE)szMAPIDLL, &cb);

    RegCloseKey(hkey);
    if (lSuccess != ERROR_SUCCESS)
        goto Cleanup;

    cb = ExpandEnvironmentStrings(szMAPIDLL, aBuffer, MAX_PATH);
    if (!cb || (cb > MAX_PATH))
        goto Cleanup;

    *pHMod = LoadLibraryEx(aBuffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CMailtoProtocol::SetMAPIRecipients
//
//  Synopsis:   set recipients in MAPIMessage
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT CMailtoProtocol::SetMAPIRecipients(MapiMessage *pmm, LPTSTR lpszRecips, UINT uiNumRecips, int iClass)
{
    HRESULT         hr = S_OK;
    TCHAR           *pNextRecip=0;
    int             i=0, l= uiNumRecips + pmm->nRecipCount;
    UINT            cAnsi = MAX_PATH;

    Assert(l);

    Assert(     iClass==mailtoAttrTO
            ||  iClass==mailtoAttrBCC
            ||  iClass==mailtoAttrCC);

    pNextRecip = lpszRecips;

    for (i = pmm->nRecipCount; i < l; i++)
    {
        hr = THR(MultiByteFromWideChar((WCHAR *)pNextRecip,
                    _tcslen(pNextRecip), &pmm->lpRecips[i].lpszName, &cAnsi));
        if (FAILED(hr))
            goto Cleanup;

        // BUGBUG (yinxie)
        // Setting address seems to cause MAPI32 to put quote around the
        // well formed adress which causes mail provider to fail the delivery
        // BUG 43627 - need to understand better how to set this field.
        /* comment this out, outlook does not work with address set
        hr = THR(MultiByteFromWideChar((WCHAR *)pNextRecip,
                    _tcslen(pNextRecip), &pmm->lpRecips[i].lpszAddress,&cAnsi));
        if (FAILED(hr))
            goto Cleanup;
        */

        switch(iClass)
        {
        case    mailtoAttrTO:
            pmm->lpRecips[i].ulRecipClass = MAPI_TO;
            break;
        case    mailtoAttrCC:
            pmm->lpRecips[i].ulRecipClass = MAPI_CC;
            break;
        case    mailtoAttrBCC:
            pmm->lpRecips[i].ulRecipClass = MAPI_BCC;
            break;
        }

        pNextRecip = pNextRecip + _tcslen(pNextRecip) + 1;
    }

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CMailtoProtocol::SetMAPIAttachement
//
//  Synopsis:   set attachement in MAPIMessage
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT CMailtoProtocol::SetMAPIAttachement(MapiMessage *pmm)
{
    HRESULT     hr = E_FAIL;
    HANDLE      hFile=INVALID_HANDLE_VALUE;
    TCHAR       achTempPath[MAX_PATH];
    UINT        cb = 0;
    ULONG       lcb = 0;
    MapiFileDesc    *pMFD=0;

    if (!_cPostData)
    {
        hr = S_OK;
        goto Cleanup;
    }
    if (!GetTempPath( MAX_PATH, achTempPath ))
        goto Cleanup;
    if (!GetTempFileName( achTempPath, TEXT("ATT"), 0, _achTempFileName ))
        goto Cleanup;

#ifdef  NEVER
    // this is not very safe, let's hold on this.
    TCHAR       *pch=0;
    pch = _tcschr(_achTempFileName, _T('.'));
    _tcscpy(pch, TEXT(".ATT"));
#endif

    hFile = CreateFile(
                        _achTempFileName,
                        GENERIC_READ|GENERIC_WRITE,     // access rights
                        0,                              // share mode
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile==INVALID_HANDLE_VALUE)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    if (!WriteFile(hFile, _postData, _cPostData, &lcb, NULL))
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }
    Assert(_cPostData == lcb);

    pMFD = (MapiFileDesc *)MemAlloc(Mt(CMailtoProtocolFileDesc), sizeof(MapiFileDesc));
    if (!pMFD)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    memset(pMFD, 0 , sizeof(MapiRecipDesc));

    pmm->lpFiles = pMFD;
    pmm->nFileCount = 1;

    hr = THR(MultiByteFromWideChar((WCHAR *)_achTempFileName,
                                    _tcslen(_achTempFileName),
                                    &pMFD->lpszPathName,
                                    &cb));
    if (FAILED(hr))
        goto Cleanup;

    pMFD->lpszFileName = "POSTDATA.ATT";
    pMFD->nPosition = (UINT) -1;                // OS will figure out where to start

Cleanup:
    if (hFile!=INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(hFile))
        {
            hr = E_FAIL;
        }
    }
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CMailtoProtocol::ReleaseMAPIMessage
//
//  Synopsis:   release memory allocations in MAPIMessage
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

void CMailtoProtocol::ReleaseMAPIMessage(MapiMessage *pmm)
{
    if (!pmm)
        return;
    if (pmm->lpFiles)
    {
        if (pmm->lpFiles->lpszPathName)
        {
            DeleteFile(_achTempFileName);
            MemFree(pmm->lpFiles->lpszPathName);
        }
        MemFree(pmm->lpFiles);
    }
    if (pmm->lpRecips)
    {
        for (int i=0; i < (int)pmm->nRecipCount; i++)
        {
            if (pmm->lpRecips[i].lpszAddress)
            {
                MemFree(pmm->lpRecips[i].lpszAddress);
            }
            if (pmm->lpRecips[i].lpszName)
            {
                MemFree(pmm->lpRecips[i].lpszName);
            }
        }
        MemFree(pmm->lpRecips);
    }
    if (pmm->lpszSubject)
        MemFree(pmm->lpszSubject);
    if (pmm->lpszNoteText)
        MemFree(pmm->lpszNoteText);
}


//+------------------------------------------------------------------------
//
//  Function:   CMailtoProtocol::MultiByteFromWideChar
//
//  Synopsis:   unicode to multibyte conversion
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CMailtoProtocol::MultiByteFromWideChar(WCHAR *lpwcsz, UINT cwc, LPSTR *lppsz, UINT *pcb)
{
    HRESULT hr = S_OK;

    // use mlang only if necessary (when mlang is loaded)
    // when mlang is loaded, we use BINDINFO passed codepage
    // otherwise, we use the system CP
#ifndef NO_MULTILANG
    if (g_pMultiLanguage)
    {
        DWORD dwState = 0;
        CODEPAGE cp = (_cp == CP_UTF_7) ? CP_UTF_8 : _cp;

        // allocate a buffer which is twice as big as the unicode one (should always enough)
        *pcb = (cwc << 1) + 2;
        *lppsz = (LPSTR)MemAlloc(Mt(CMailtoProtocolMbfwc), *pcb);
        if (!*lppsz)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(g_pMultiLanguage->ConvertStringFromUnicode(&dwState, cp, lpwcsz, &cwc, *lppsz, pcb));
        goto Cleanup;
    }
#endif

    // allocate a buffer which is twice as big as the unicode one (should always enough)
    *pcb = (cwc << 1) + 2;
    *lppsz = (LPSTR)MemAlloc(Mt(CMailtoProtocolMbfwc), *pcb);
    if (!*lppsz)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *pcb = WideCharToMultiByte( CP_ACP, 0, lpwcsz, cwc, *lppsz, *pcb, NULL, NULL );
    if (!*pcb)
    {
        MemFree(*lppsz);
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:

    if (FAILED(hr))
    {
        *lppsz = NULL;
    }
    else
    {
        hr = S_OK;
        (*lppsz)[*pcb] = 0;
    }
    RRETURN(hr);
}
