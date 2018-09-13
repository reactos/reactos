//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       resprot.cxx
//
//  Contents:   Implementation of the resource protocol
//
//  History:    02-12-97    AnandRa     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_RESPROT_HXX_
#define X_RESPROT_HXX_
#include "resprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecuteA (for AXP)
#endif

extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);

#if DBG==1
#define TID GetCurrentThreadId()
#endif

MtDefine(CResProtocol, Protocols, "CResProtocol")
MtDefine(CResProtocolDoParseAndBind_pb, Protocols, "CResProtocol::DoParseAndBind pb")
MtDefine(CViewSourceProtocol, Protocols, "CViewSourceProtocol")

//+---------------------------------------------------------------------------
//
//  Function:   CreateResProtocol
//
//  Synopsis:   Creates a resource Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateResProtocol(IUnknown *pUnkOuter)
{
    return new CResProtocol(pUnkOuter);
}

CResProtocolCF   g_cfResProtocol(CreateResProtocol);


//+---------------------------------------------------------------------------
//
//  Method:     CrackResUrl
//
//  Synopsis:   Breaks res: URL into its component strings.
//              NOTE: pcstrResName and pcstrRID are allowed to be NULL.
//                    The caller needs only to pass in CStr*'s for the
//                    components they want.
//
//
//  Arguments:  pchURL          Url of the following syntax:
//
//              res://<dll name and location>[/<resource type>]/<resource id>
//
//              pcstrResName    CStr to hold <dll name and location>
//                              The path may not contain '/' characters
//              pcstrResType    CStr to hold <resource type>
//              pcstrRID        CStr to hold <resource id>
//
//----------------------------------------------------------------------------

HRESULT
CrackResUrl(const TCHAR* pchURL, CStr* pcstrResName, CStr* pcstrResType, CStr* pcstrRID)
{
    HRESULT         hr = S_OK;
    TCHAR *         pch = NULL;
    TCHAR *         pchNext = NULL;
    TCHAR           achURL[pdlUrlLen];
    DWORD           cchURL;

    Assert (pchURL && _tcsnipre(_T("res:"), 4, pchURL, -1));

    //
    // NOTENOTE: pchstrResName, pcstrResType, and pcstrRID are
    //           allowed to be NULL!
    //

    // Unescape the URL.
    hr = THR(CoInternetParseUrl(
            pchURL,
            PARSE_ENCODE,
            0,
            achURL,
            ARRAY_SIZE(achURL),
            &cchURL,
            0));
    if (hr)
        goto Cleanup;

    pch = _tcschr(achURL, _T(':'));
    if (!pch)
    {
        goto SyntaxError;
    }

    if (!(_tcsnipre(_T("://"), 3, pch, -1)))
    {
        goto SyntaxError;
    }

    pch += 3;

    //
    // Now look for the dll name.  Basically find the next '/'.
    // Everything to the left of this is the dll name.
    //

    pchNext = _tcschr(pch, _T('/'));
    if (!pchNext)
    {
        goto SyntaxError;
    }

    if (pcstrResName)
    {
        hr = THR(pcstrResName->Set(pch, pchNext - pch));
        if (hr)
            goto Cleanup;
    }

    pch = pchNext + 1;
    pchNext = _tcschr(pch, _T('/'));

    if (pchNext)
    {
        //
        // We've found a second '/', which means that the URL
        // contains both a resource-ID and resource type.
        //

        if (pcstrResType)
        {
            hr = THR(pcstrResType->Set(pch, pchNext - pch));
            if (hr)
                goto Cleanup;

            LONG lTemp;
            HRESULT hr2 = THR_NOTRACE(ttol_with_error(*pcstrResType, &lTemp));

            if (SUCCEEDED(hr2))
            {
                // 
                // Only accept numbers that are max 16 bits.  FindResource
                // can only accept such numbers.  
                //

                if (lTemp & 0xFFFF0000)
                    goto SyntaxError;
                    
                hr = pcstrResType->Set(_T("#"));
                if (hr)
                    goto Cleanup;

                hr = pcstrResType->Append(pch, pchNext - pch);
                if (hr)
                    goto Cleanup;
            }
            else if (hr2 != E_INVALIDARG)
            {
                hr = hr2;
                goto Cleanup;
            }
        }

        pch = pchNext + 1;
    }
    else if (pcstrResType)
    {
        hr = THR(pcstrResType->Set(_T("")));
    }

    if (pcstrRID)
    {
        LONG lTemp;
        HRESULT hr2 = THR_NOTRACE(ttol_with_error(pch, &lTemp));
        if (SUCCEEDED(hr2))
        {
            // 
            // Only accept numbers that are max 16 bits.  FindResource
            // can only accept such numbers.  
            //

            if (lTemp & 0xFFFF0000)
                goto SyntaxError;
                
            hr = THR(pcstrRID->Set(_T("#")));
            if (hr)
                goto Cleanup;
                
            hr = THR(pcstrRID->Append(pch));
            if (hr)
                goto Cleanup;
        }
        else if (hr2 == E_INVALIDARG)
        {
            hr = THR(pcstrRID->Set(pch));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = hr2;
            goto Cleanup;
        }
    }


Cleanup:
    RRETURN(hr);

SyntaxError:
    hr = MK_E_SYNTAX;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocolCF::ParseUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

#define FILEPROT_PREFIX_LEN 7

HRESULT
CResProtocolCF::ParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!pcchResult || !pwzResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (ParseAction == PARSE_SECURITY_URL)
    {
        CStr            cstrResName;
        DWORD           cchNewUrlLen;
        TCHAR           achFullResourceName [pdlUrlLen];

        *pcchResult = FILEPROT_PREFIX_LEN;

        hr = THR(CrackResUrl (pwzUrl, &cstrResName, NULL, NULL));
        if (hr)
            goto Cleanup;

#ifndef WIN16
        {
            CStrIn  strinResName ((LPWSTR)cstrResName);
            CStrOut stroutFile (achFullResourceName, pdlUrlLen);
            char *  pstrFileName;

            cchNewUrlLen = ::SearchPathA (NULL,
                                          strinResName,
                                          NULL,
                                          pdlUrlLen,
                                          stroutFile,
                                          &pstrFileName);
        }
#else
        {
            char *  pstrFileName;
            cchNewUrlLen = ::SearchPathA (NULL,
                                          cstrResName,
                                          NULL,
                                          pdlUrlLen,
                                          achFullResourceName,
                                          &pstrFileName);
        }
#endif // WIN16

        //
        // The returned path is now stored as a TCHAR in achFullResourceName.
        //

        if (!cchNewUrlLen)
        {
            hr = MK_E_SYNTAX;
            goto Cleanup;
        }

        *pcchResult += cchNewUrlLen + 1; // total mem needed includes \0 terminator

        if (*pcchResult > pdlUrlLen)
        {
            hr = MK_E_SYNTAX;
            goto Cleanup;
        }

        if (*pcchResult > cchResult)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        _tcsncpy(pwzResult, _T("file://"), FILEPROT_PREFIX_LEN);
        _tcscpy(pwzResult + FILEPROT_PREFIX_LEN, achFullResourceName);

        Assert (_tcslen(pwzResult) + 1 == *pcchResult);
    }
    else
    {
        hr = THR_NOTRACE(super::ParseUrl(
            pwzUrl,
            ParseAction,
            dwFlags,
            pwzResult,
            cchResult,
            pcchResult,
            dwReserved));
    }

Cleanup:
    RRETURN2(hr, INET_E_DEFAULT_ACTION, S_FALSE);
}


const CBase::CLASSDESC CResProtocol::s_classdesc =
{
    &CLSID_ResProtocol,             // _pclsid
};


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::QueryInfo
//
//  Synopsis:   for QUERY_IS_SAFE
//
//----------------------------------------------------------------------------
HRESULT
CResProtocolCF::QueryInfo(
    LPCWSTR       pwzUrl, 
    QUERYOPTION   QueryOption,
    DWORD         dwQueryFlags,
    LPVOID        pvBuffer,
    DWORD         cbBuffer,
    DWORD  *      pcbBuffer,
    DWORD         dwReserved)
{
    //
    // intercept is-safe
    //

    if (QueryOption == QUERY_IS_SAFE)
    {
        if (!pvBuffer || cbBuffer < sizeof(DWORD))
            return E_FAIL;

        if (pcbBuffer)
            *pcbBuffer = sizeof(DWORD);
        
        *(DWORD *)pvBuffer = TRUE;
        return S_OK;
    }
    else
    {
        RRETURN(super::QueryInfo(
                pwzUrl,
                QueryOption,
                dwQueryFlags,
                pvBuffer,
                cbBuffer,
                pcbBuffer,
                dwReserved));
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::CResProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CResProtocol::CResProtocol(IUnknown *pUnkOuter) : super(pUnkOuter)
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::~CResProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CResProtocol::~CResProtocol()
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::Start
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CResProtocol::Start(
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
        hr = THR(ParseAndBind());
    }


Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding & execution of script.
//
//----------------------------------------------------------------------------

HRESULT
CResProtocol::ParseAndBind()
{
    RRETURN(DoParseAndBind(_cstrURL, _cstrResName, _cstrResType, _cstrRID, &_pStm, this));
}


void
CResProtocol::_ReportData(ULONG cb)
{
    _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    _pProtSink->ReportData(_bscf, cb, cb);
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::DoParseAndBind
//
//  Synopsis:   Static helper to actually perform the binding of protocol.
//
//  Arguments:  pchURL          Url to bind to [needed]
//              cstrRID         Resource id extracted from url [needed]
//              cstrResName     Resource dll name extracted from url [needed]
//              ppStm           Stream of data from resource [needed]
//              pProt           Protocol object that's involved [optional]
//
//----------------------------------------------------------------------------

HRESULT
CResProtocol::DoParseAndBind(
    TCHAR *pchURL, 
    CStr &cstrResName,
    CStr &cstrResType,
    CStr &cstrRID,
    IStream **ppStm,
    CResProtocol *pProt)
{
    HRESULT         hr = S_OK;
    TCHAR *         pchBuf = NULL;
    ULONG           cb = 0;
    CROStmOnBuffer *prostm = NULL;
    HINSTANCE       hInst = NULL;
    BYTE *          pb = NULL;
    IBindCtx *      pBCtx = NULL;
    TCHAR *         pchMime = NULL;
    
    hr = THR(CrackResUrl (pchURL, &cstrResName, &cstrResType, &cstrRID));
    if (hr)
        goto Cleanup;

    //
    // Do the binding.
    //

    Assert((TCHAR *)cstrResName && (TCHAR *)cstrRID);

    if (cstrResName.Length() >= MAX_PATH)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hInst = LoadLibraryEx(
                cstrResName, 
                NULL, 
                DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    if (!hInst)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    if (cstrResType.Length())
    {
        pchBuf = (TCHAR *)GetResource(
            hInst,
            cstrRID,
            cstrResType,
            &cb);
    }
    else
    {
        pchBuf = (TCHAR *)GetResource(
            hInst,
            cstrRID,
            MAKEINTRESOURCE(RT_HTML),
            &cb);
        //
        //  dialog resource type may be RT_FILE
        //
        if (!pchBuf)
        {
            pchBuf = (TCHAR *)GetResource(
                hInst,
                cstrRID,
                MAKEINTRESOURCE(RT_FILE),
                &cb);
        }
    }

    if (!pchBuf)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
            
    }
    
    Assert(pchBuf);

    // BUGBUG - res: bitmap hack (t-chrisr)
    //
    // We want to be able to load bitmaps from resource files.  The problem is
    // that resource files don't store bitmaps quite like .bmp files store bitmaps
    // (resource files lack the BITMAPFILEHEADER header).  So we'll construct a
    // "real" bitmap here.
    //
    // <HACK>

    if (_tcsequal(cstrResType, _T("#2")))
    {
        BITMAPFILEHEADER  bmfh;
        PBITMAPINFOHEADER pbmih = NULL;
        PBITMAPCOREHEADER pbmch = NULL;
        DWORD             dwClrTblSize = 0;
        DWORD             dwClrUsed;
        DWORD             dwCompression;
        WORD              wBitCount;
        WORD              wPlanes;

        pbmih = (PBITMAPINFOHEADER)pchBuf;
        if (pbmih->biSize == sizeof(BITMAPINFOHEADER))
        {
            dwClrUsed     = pbmih->biClrUsed;
            dwCompression = pbmih->biCompression;
            wBitCount     = pbmih->biBitCount;
            wPlanes       = pbmih->biPlanes;
        }
        else if (pbmih->biSize == sizeof(BITMAPCOREHEADER))
        {
            pbmch = (PBITMAPCOREHEADER)pchBuf;

            dwClrUsed     = 0;
            dwCompression = BI_RGB;
            wBitCount     = pbmch->bcBitCount;
            wPlanes       = pbmch->bcPlanes;
        }
        else
        {
            // we don't have a bitmap that we understand
            goto EndHack;
        }

        //
        // Calculate the size of the bitmap's color table
        //

        if (wBitCount <= 8)
        {
            dwClrTblSize = dwClrUsed ? dwClrUsed : (1 << wBitCount);
            dwClrTblSize *= sizeof(RGBQUAD);
        }
        else if ((dwCompression == BI_BITFIELDS) &&
            ((wBitCount == 16) || (wBitCount == 32)))
        {
            dwClrTblSize = 3 * sizeof(DWORD);
        }


        //
        // Fabricate the missing header.  Remember, pbmih might
        // really be a PBITMAPCOREHEADER.  But pbmih->biSize will
        // always be the right size, either way.
        //

#ifdef BIG_ENDIAN
        bmfh.bfType = (WORD)0x424D;         // 0x42 = "B", 0x4D = "M"
#else
        bmfh.bfType = (WORD)0x4D42;         // 0x42 = "B", 0x4D = "M"
#endif
        bmfh.bfSize = sizeof(bmfh) + cb;
        bmfh.bfReserved1 = 0;
        bmfh.bfReserved2 = 0;
        bmfh.bfOffBits = sizeof(bmfh) + pbmih->biSize + dwClrTblSize;

        //
        // Put this sandwich together and make it be pchBuf
        //
        pb = new(Mt(CResProtocolDoParseAndBind_pb)) BYTE [bmfh.bfSize];
        if (!pb)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        MemSetName((pb, "Resprot Bitmap Buffer"));

        memcpy (pb, &bmfh, sizeof(bmfh));
        memcpy (pb + sizeof(bmfh), pchBuf, cb);

        pchBuf = (TCHAR *)pb;
        cb = bmfh.bfSize;
    }
    // </HACK>

EndHack:
    //
    // Try and retrieve a mime type from the resource name and
    // report that if available.
    //

    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBCtx, 0));
    if (hr)
        goto Cleanup;
        
    if (pProt &&
        OK(THR(FindMimeFromData(
            pBCtx, 
            pProt->_cstrURL, 
            NULL, 0, NULL, 0, &pchMime, 0))))
    {
        hr = THR(pProt->_pProtSink->ReportProgress(
                BINDSTATUS_MIMETYPEAVAILABLE,
                pchMime));
        if (hr)
            goto Cleanup;
    }
    
    // cb includes the null terminator

    prostm = new CROStmOnBuffer;
    if (!prostm)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(prostm->Init((BYTE *)pchBuf, cb));
    if (hr)
        goto Cleanup;

    *ppStm = (IStream *)prostm;
    (*ppStm)->AddRef();

Cleanup:
    if (pProt)
    {
        if (!pProt->_fAborted)
        {
            if (!hr)
            {
                pProt->_ReportData(cb);
            }
            if (pProt->_pProtSink)
            {
                pProt->_pProtSink->ReportResult(hr, 0, 0);
            }
        }
    }

    if (hInst)
    {
        FreeLibrary(hInst);
    }
    if (prostm)
    {
        prostm->Release();
    }
    ReleaseInterface(pBCtx);
    delete [] pb;
    CoTaskMemFree(pchMime);
    
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CreateResourceMoniker
//
//  Synopsis:   Creates a new moniker based off a resource file & rid
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT 
CreateResourceMoniker(
    HINSTANCE hInst, 
    TCHAR *pchRID, 
    IMoniker **ppmk)
{
    HRESULT         hr = S_OK;
    TCHAR           ach[pdlUrlLen];
    
    _tcscpy(ach, _T("res://"));

    if (!GetModuleFileName(
            hInst, 
            ach + _tcslen(ach), 
            pdlUrlLen - _tcslen(ach) - 1))
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

#ifdef UNIX
    {
        TCHAR* p = _tcsrchr(ach, _T('/'));
        if (p)
	{
            int iLen = _tcslen(++p);
            memmove(ach + 6, p, sizeof(TCHAR) * iLen);
            ach[6 + iLen] = _T('\0');
	}
    }
#endif

    _tcscat(ach, _T("/"));
    _tcscat(ach, pchRID);

    hr = THR(CreateURLMoniker(NULL, ach, ppmk));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateViewSourceProtocol
//
//  Synopsis:   Creates a view-source: Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateViewSourceProtocol(IUnknown *pUnkOuter)
{
    return new CViewSourceProtocol(pUnkOuter);
}


CViewSourceProtocolCF   g_cfViewSourceProtocol(CreateViewSourceProtocol);

const CBase::CLASSDESC CViewSourceProtocol::s_classdesc =
{
    &CLSID_ViewSourceProtocol,             // _pclsid
};

// IOInetProtocolInfo methods
HRESULT CViewSourceProtocolCF::QueryInfo(LPCWSTR         pwzUrl,
                                         QUERYOPTION     QueryOption,
                                         DWORD           dwQueryFlags,
                                         LPVOID          pBuffer,
                                         DWORD           cbBuffer,
                                         DWORD *         pcbBuf,
                                         DWORD           dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    switch (QueryOption)
    {
    case QUERY_CAN_NAVIGATE:

        // Need at least a DWORD
        if (cbBuffer < sizeof(DWORD))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (pcbBuf)
        {
            *pcbBuf = sizeof(DWORD); // 4 bytes needed for DWORD
        }

        *((DWORD *)pBuffer) = 0; // make sure we return false (0) for view-source prot.
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

HRESULT DisplaySource(LPCTSTR tszSourceName)
{
    // Attempt to read a registry key

    HKEY    hKeyEditor = NULL;
    long    lRet;
    TCHAR   tszEditorName[MAX_PATH];
    LPCTSTR tszDefaultEditorName = _T("notepad");
    long    dwSize = sizeof(tszEditorName);
    SHELLEXECUTEINFO ei = {0};
    BOOL    fSuccess;

    lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer\\View Source Editor", 0, KEY_READ, &hKeyEditor);
    if(lRet == ERROR_SUCCESS)
    {
        lRet = RegQueryValue(hKeyEditor, _T("Editor Name"), tszEditorName, &dwSize);
        RegCloseKey(hKeyEditor);
    }

    // If key doesn't exist, fails to open or query fails, open default editor

    if(lRet != ERROR_SUCCESS)
    {
        _tcscpy(tszEditorName, tszDefaultEditorName);
    }

    // If named editor fails to execute, open default editor   

    // BUGBUG#### (rodc) Disable leak tracking because ShellExecute leaks.

    ei.cbSize = sizeof(ei);
    ei.lpVerb = _T("open");
    ei.lpFile = tszEditorName;
    ei.lpParameters = tszSourceName;
    ei.nShow = SW_SHOWNORMAL;

    DbgMemoryTrackDisable(TRUE);
    fSuccess = ShellExecuteEx(&ei);
    DbgMemoryTrackDisable(FALSE);

    if (!fSuccess)
    {
        ei.lpFile = tszDefaultEditorName;

        DbgMemoryTrackDisable(TRUE);
        fSuccess = ShellExecuteEx(&ei);
        DbgMemoryTrackDisable(FALSE);

        if (!fSuccess)
            RRETURN( GetLastWin32Error());
    }

    return S_OK;
}

HRESULT CopySource(IStream *pStmSrc, IStream *pStmDest)
{
    STATSTG statStm;
    BYTE abBuf[4096];   // copy in chunks of 4K
    ULONG cb = 0;
    ULONG cbCopied = 0;
    LONG cbToCopy;
    BYTE *pbStart;
    BYTE *pbEnd;
    BYTE bLast = 0;
    HRESULT hr;

    hr = pStmSrc->Stat(&statStm, STATFLAG_NONAME);
    if (hr)
        goto Cleanup;

#ifdef UNIX
    cb = statStm.cbSize.LowPart;  // get size of bit stream
#else
    cb = statStm.cbSize.u.LowPart;  // get size of bit stream
#endif
    while (cbCopied < cb)
    {
        cbCopied += sizeof(abBuf);
        // bytes to be copied is 4K or remainder (if last chunk)
        if (cbCopied > cb)
            cbToCopy= cb % sizeof(abBuf);
        else
            cbToCopy= sizeof(abBuf);

        hr = pStmSrc->Read(abBuf, (ULONG)cbToCopy, NULL);
        if (hr)
            goto Cleanup;

        pbStart = pbEnd = abBuf;

        while(cbToCopy > 0)
        {
            // keep going until CR, LF (i.e end of line) or End of current chunk
            while (*pbEnd != '\r' && *pbEnd != '\n' && pbEnd-pbStart<cbToCopy)
                pbEnd++;

            // write out what we have so far
            hr = THR(pStmDest->Write(pbStart, pbEnd-pbStart, NULL));
            if (hr)
                goto Cleanup;

            // if first byte of new chunk is not LF, clear the last byte. so 
            // that there is no confusion if tht last byte of previous chunk
            // was a CR
            if (pbStart != pbEnd)
                bLast = 0;

            if (pbEnd-pbStart == cbToCopy)  // bailout if at end of 4K chunk
                break;

            // If LF is found and previous char is CR, 
            // then we don't write, Otherwise, write CR & LF to temp file.

            if (!(*pbEnd == '\n' && bLast == '\r')) 
            {
                hr = THR(pStmDest->Write("\r\n", 2, NULL));
                if (hr)
                    goto Cleanup;
            }

            bLast = *pbEnd; // remember last char (CR or LF)
            cbToCopy -= (LONG)(pbEnd - pbStart + 1); // adjust bytes to be copied
            pbStart = ++pbEnd;  // adjust pts to start of new line
        }
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Method:     CViewSourceProtocol::OnDwnChan
//
//-------------------------------------------------------------------------

void CViewSourceProtocol::OnDwnChan()
{
    ULONG       ulState = _pBitsCtx->GetState();
    IStream *   pStm = NULL;
    IStream *   pStmFile = NULL;
    TCHAR achFileName[MAX_PATH];
    HRESULT hr = S_OK;
    
    if (ulState & DWNLOAD_COMPLETE)
    {
        _pBitsCtx->GetStream(&pStm);
        if (pStm)
        {
            achFileName[0] = 0;
            if (!CreateUrlCacheEntry(_cstrURL, 0, NULL, achFileName, 0))
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            hr = THR(CreateStreamOnFile(
                     achFileName,
                     STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE,
                     &pStmFile));

            if (hr)
                goto Cleanup;

            hr = CopySource(pStm, pStmFile);    // insert CR\LF if reqd.
            if (hr)
                goto Cleanup;

            hr = CloseStreamOnFile(pStmFile);
            if (hr)
                goto Cleanup;

            FILETIME fileTime;
            fileTime.dwLowDateTime = 0;
            fileTime.dwHighDateTime = 0;
            if (!CommitUrlCacheEntry(_cstrURL,
                                     achFileName,
                                     fileTime,
                                     fileTime,
                                     NORMAL_CACHE_ENTRY,
                                     NULL,
                                     0,
                                     NULL,
                                     0))
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            hr = DisplaySource(achFileName);
        }
        goto Cleanup;
    }
    else if (ulState & DWNLOAD_ERROR)
    {
        goto Cleanup;
    }

    return;

Cleanup:
    if (!_fAborted)
    {
        if (_pBitsCtx)
        {
            // if aborted, kill the bits ctx if started
            _pBitsCtx->Disconnect();
            _pBitsCtx->Release();
            _pBitsCtx = NULL;
        }

        // tell urlmon that we are done
        if (!hr && _pProtSink)
        {
            _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
            _pProtSink->ReportData(_bscf, 0, 0);
        }
        if (_pProtSink)
        {
            _pProtSink->ReportResult(hr, 0, 0);
        }
    }

    ReleaseInterface(pStm);
    ReleaseInterface(pStmFile);
}

//+---------------------------------------------------------------------------
//
//  Method:     CViewSourceProtocol::ParseAndBind
//
//  Synopsis:   Actually get the source stream and display it in notepad.
//
//----------------------------------------------------------------------------

HRESULT CViewSourceProtocol::ParseAndBind()
{
    HRESULT         hr = S_OK;
    TCHAR *         pchSourceUrl = NULL;
    DWNLOADINFO     dwnloadinfo = {0};
    URL_COMPONENTS  uc;
    TCHAR           achPath[pdlUrlLen];

    // skip protocol part
    pchSourceUrl = _tcschr(_cstrURL, ':');
    if (!pchSourceUrl)
    {
        hr = MK_E_SYNTAX;
        goto Cleanup;
    }

    // Go past the :
    pchSourceUrl++;

    memset(&uc, 0, sizeof(uc));

    uc.dwStructSize = sizeof(uc);
    uc.lpszUrlPath = achPath;
    uc.dwUrlPathLength = ARRAY_SIZE(achPath);

    // Only file://, http:// and https:// supported
    if (pchSourceUrl && InternetCrackUrl(pchSourceUrl, _tcslen(pchSourceUrl), 0, &uc))
    {
        switch (uc.nScheme)
        {
            case INTERNET_SCHEME_HTTP:
            case INTERNET_SCHEME_HTTPS:
                dwnloadinfo.pDwnDoc = new CDwnDoc();
    
                if (dwnloadinfo.pDwnDoc)
                    dwnloadinfo.pDwnDoc->SetRefresh(IncrementLcl());
    
                dwnloadinfo.pchUrl = pchSourceUrl;

                Assert(!_pBitsCtx);

                // Get the bits context
                hr = THR(::NewDwnCtx(DWNCTX_BITS, TRUE, &dwnloadinfo, (CDwnCtx **)&_pBitsCtx));
                if (hr)
                    break;

                if (_pBitsCtx)
                {
                    // if bits already got, call the callback
                    if (_pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR))
                        OnDwnChan();
                    else    // register the callback
                    {
                        _pBitsCtx->SetCallback(OnDwnChanCallback, this);
                        _pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
                    }
                }

                if (dwnloadinfo.pDwnDoc)
                    dwnloadinfo.pDwnDoc->Release();
            
                RRETURN(hr);

            // just display file if file:// protocol
            case INTERNET_SCHEME_FILE:
                hr = DisplaySource(uc.lpszUrlPath);
                // fall through

            default:
                break;
        }
    }
        
Cleanup:
    // tell urlmon that we are done
    if (!_fAborted)
    {
        if (!hr && _pProtSink)
        {
            _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
            _pProtSink->ReportData(_bscf, 0, 0);
        }
        if (_pProtSink)
        {
            _pProtSink->ReportResult(hr, 0, 0);
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CViewSourceProtocol::Abort
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT CViewSourceProtocol::Abort(HRESULT hrReason, DWORD dwOptions)
{
    if (_pBitsCtx)
    {
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
        _pBitsCtx = NULL;
    }

    return super::Abort(hrReason, dwOptions);
}

//+---------------------------------------------------------------------------
//
//  Method:     CViewSourceProtocol::Passivate
//
//  Synopsis:   1st stage dtor. Need to override so that the BitsCtx can be
//              killed if started and then aborted before OnDwnChan CB is called
//
//----------------------------------------------------------------------------

void
CViewSourceProtocol::Passivate()
{
    if (_pBitsCtx)
    {
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
        _pBitsCtx = NULL;
    }
    
    super::Passivate();
}

