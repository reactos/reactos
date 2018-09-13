//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       resprot.cxx
//
//  Contents:   Implementation of the sysimage protocol
//
//  History:    06-15-98    dli     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_RESPROT_HXX_
#define X_RESPROT_HXX_
#include "simgprot.hxx"
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

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecuteA (for AXP)
#endif

#ifndef X_SHLWAPIP_H_
#define X_SHLWAPIP_H_
#include "shlwapip.h"  // for the definition of Shell_GetCachedImageIndexWrap
#endif

// The following data structures are standard icon file structures
typedef struct tagICONDIRENTRY
{
    BYTE    cx;
    BYTE    cy;
    BYTE    nColors;
    BYTE    iUnused;
    WORD    xHotSpot;
    WORD    yHotSpot;
    DWORD   cbDIB;
    DWORD   offsetDIB;
} ICONDIRENTRY;

typedef struct tagICONDIR
{
    WORD iReserved;
    WORD iResourceType;
    WORD cresIcons;
} ICONDIR;


MtDefine(CSysimageProtocol, Protocols, "CSysimageProtocol")
MtDefine(CSysimageProtocolDoParseAndBind_pb, Protocols, "CSysimageProtocol::DoParseAndBind pb")

//+---------------------------------------------------------------------------
//
//  Function:   CreateSysimageProtocol
//
//  Synopsis:   Creates a resource Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateSysimageProtocol(IUnknown *pUnkOuter)
{
    return new CSysimageProtocol(pUnkOuter);
}

CSysimageProtocolCF   g_cfSysimageProtocol(CreateSysimageProtocol);

//+---------------------------------------------------------------------------
//
//  Method:     CrackSysimageUrl
//
//  Synopsis:   Breaks res: URL into its component strings.
//              NOTE: pcstrSysimageType is allowed to be NULL.
//                    The caller needs only to pass in CStr*'s for the
//                    components they want.
//
//
//  Arguments:  pchURL          Url of the following syntax:
//
//              sysimage://<filename>[,icon index]>[/<small> | <large>]
//
//              pcstrModule     CStr to hold <dll name and location> or the sys_image_list_index
//                              The path may not contain '/' characters
//              pcstrSysimageType    CStr to hold <resource type>
//              pcstrRID        CStr to hold <resource id>
//
//----------------------------------------------------------------------------

HRESULT
CrackSysimageUrl(const TCHAR* pchURL, CStr* pcstrModule, CStr* pcstrSize, CStr* pcstrSelected)
{
    HRESULT         hr = S_OK;
    TCHAR *         pch = NULL;
    TCHAR *         pchNext = NULL;
    TCHAR           achURL[pdlUrlLen];
    DWORD           cchURL;
    //    BOOL            bGetIndex = FALSE;
            
    Assert (pchURL && _tcsnipre(_T("sysimage:"), 9, pchURL, -1));

    if (pcstrSize)
        hr = THR(pcstrSize->Set(_T("")));

    if (pcstrSelected)
        hr = THR(pcstrSelected->Set(_T("")));
    

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
    // Now look for the executable name
    //

    // we currently do not support index, yet
    //    pchNext = _tcschr(pch, _T(','));
    //    if (!pchNext)
    //    {
    pchNext = _tcschr(pch, _T('/'));
    if (!pchNext)
        goto SyntaxError;
    //    }
    //    else
    //        bGetIndex = TRUE;

    if (pcstrModule)
    {
        hr = THR(pcstrModule->Set(pch, pchNext - pch));
        if (hr)
            goto Cleanup;
    }


    // skip the '/'
    pchNext++;
    
    if (pcstrSize)
    {
        hr = THR(pcstrSize->Set(pchNext));
        if (hr)
            goto Cleanup;
    }

 /*  Add this in when we support index
  if (pchNext)
    {
        // Yes
        //
        // We've found a second '/', which means that the URL
        // contains the <small> | <large> indicator
        //
        if (pcstrSize)
        {
            hr = THR(pcstrSize->Set(pch, pchNext - pch));
            if (hr)
                goto Cleanup;
        }

        // Do we need the selected info?
        if (pcstrSelected)
        {
            // Yes, search for the next '/'
            pch = pchNext + 1;
            pchNext = _tcschr(pch, _T('/'));

            // Found one
            if (pchNext)
            {
                hr = THR(pcstrSize->Set(pch, pchNext - pch));
                if (hr)
                    goto Cleanup;
            }
        }
    }*/

Cleanup:
    RRETURN(hr);

SyntaxError:
    hr = MK_E_SYNTAX;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSysimageProtocolCF::ParseUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

#define FILEPROT_PREFIX_LEN 7

HRESULT
CSysimageProtocolCF::ParseUrl(
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
        CStr            cstrModule;
        DWORD           cchNewUrlLen;
        TCHAR           achFullResourceName [pdlUrlLen];
        
        *pcchResult = FILEPROT_PREFIX_LEN;

        hr = THR(CrackSysimageUrl (pwzUrl, &cstrModule, NULL, NULL));
        if (hr)
            goto Cleanup;
        
#ifndef WIN16
        {
            CStrIn  strinResName ((LPWSTR)cstrModule);
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
                                          cstrModule,
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


const CBase::CLASSDESC CSysimageProtocol::s_classdesc =
{
    &CLSID_SysimageProtocol,             // _pclsid
};


//+---------------------------------------------------------------------------
//
//  Method:     CSysimageProtocol::CSysimageProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CSysimageProtocol::CSysimageProtocol(IUnknown *pUnkOuter) : super(pUnkOuter)
{
    TCHAR szWindows[MAX_PATH];
    SHFILEINFO sfi = {0};
    GetWindowsDirectory(szWindows, ARRAY_SIZE(szWindows));
    _himglSmall = (HIMAGELIST)SHGetFileInfo(szWindows, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    _himglLarge = (HIMAGELIST)SHGetFileInfo(szWindows, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
}


//+---------------------------------------------------------------------------
//
//  Method:     CSysimageProtocol::~CSysimageProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CSysimageProtocol::~CSysimageProtocol()
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CSysimageProtocol::Start
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CSysimageProtocol::Start(
    LPCWSTR pchUrl, 
    IInternetProtocolSink *pTrans, 
    IInternetBindInfo *pOIBindInfo,
    DWORD grfSTI, 
    HANDLE_PTR dwReserved)
{
    HRESULT         hr = NOERROR;
    DWORD           dwSize;
    TCHAR *pchBuf = new TCHAR[pdlUrlLen];

    if (pchBuf == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

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

    hr = THR(CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0, pchBuf, pdlUrlLen, &dwSize, 0));
    if (hr)
        goto Cleanup;

    hr = THR(_cstrURL.Set(pchBuf));
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
    if (pchBuf != NULL)
        delete pchBuf;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSysimageProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding & execution of script.
//
//----------------------------------------------------------------------------

HRESULT
CSysimageProtocol::ParseAndBind()
{
    RRETURN(DoParseAndBind(_cstrURL, _cstrModule, _cstrSize, _cstrSelected, &_pStm, this));
}


void
CSysimageProtocol::_ReportData(ULONG cb)
{
    _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    _pProtSink->ReportData(_bscf, cb, cb);
}


//+---------------------------------------------------------------------------
//
//  Method:     CSysimageProtocol::DoParseAndBind
//
//  Synopsis:   Static helper to actually perform the binding of protocol.
//
//  Arguments:  pchURL          Url to bind to [needed]
//              cstrRID         Resource id extracted from url [needed]
//              cstrModule     Resource dll name extracted from url [needed]
//              ppStm           Stream of data from resource [needed]
//              pProt           Protocol object that's involved [optional]
//
//----------------------------------------------------------------------------

HRESULT
CSysimageProtocol::DoParseAndBind(
    TCHAR *pchURL, 
    CStr &cstrModule,
    CStr &cstrSize,
    CStr &cstrSelected,                                  
    IStream **ppStm,
    CSysimageProtocol *pProt)
{
    HRESULT         hr = S_OK;
    TCHAR *         pchBuf = NULL;
    CROStmOnBuffer *prostm = NULL;
    ULONG           cb=0;
    BYTE *          pb = NULL;
//    TCHAR *         pch = NULL;
    SHFILEINFO      sfi = {0};
    DWORD           dwShgfi = 0;
    LPVOID          pvdBitsXOR = NULL;
    LPVOID          pvdBitsAND = NULL;
    HDC             hdcT  = NULL;
    BITMAPINFO *    pbmi = NULL;
    BITMAPINFO      bmiMask = {0};
    ICONDIR *       pifh = NULL;
    ICONINFO        iconinfo = {0};
    BITMAP          bm = {0};
    BITMAP          bmMask = {0};
    DWORD           cbDib;
    DWORD           cbMask;
    ICONDIRENTRY *  pide;
    int             nColors = 0;
    TCHAR           achFullModuleName [MAX_PATH];
    TCHAR        *  pstrFileName;
    int             iIconIndex = 0;
    
    // for icon # feature
    // int             iModuleIndex;

    hr = THR(CrackSysimageUrl (pchURL, &cstrModule, &cstrSize, &cstrSelected));
    if (hr)
        goto Cleanup;

    // got to have a real module to load icon from
    Assert((TCHAR *)cstrModule);

    if (cstrModule.Length() >= MAX_PATH)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // determine the icon size to load 
    dwShgfi = SHGFI_ICON | SHGFI_SHELLICONSIZE;
    if (cstrSize.Length())
    {
        if (_tcsequal(cstrSize, _T("small")))
            dwShgfi |= SHGFI_SMALLICON;
        if (_tcsequal(cstrSize, _T("large")))
            dwShgfi |= SHGFI_LARGEICON;
    }

    iIconIndex = PathParseIconLocation(cstrModule);
    
    // get the full path of the module, in case of failure, use the windows directory
    if (0 == SearchPath(NULL, cstrModule, NULL, MAX_PATH, achFullModuleName, &pstrFileName))
    {
        GetWindowsDirectory(achFullModuleName, ARRAY_SIZE(achFullModuleName));
        iIconIndex = 0;
    }
    
    // get the icon
    if (iIconIndex != 0)
    {
        iIconIndex = Shell_GetCachedImageIndexWrap(achFullModuleName, iIconIndex, 0);
        sfi.hIcon = ImageList_GetIcon((dwShgfi & SHGFI_SMALLICON) ? pProt->_himglSmall : pProt->_himglLarge, iIconIndex, 0);
    }
    else
        SHGetFileInfo((TCHAR *)achFullModuleName, 0, &sfi, sizeof(sfi), dwShgfi);

    if (!sfi.hIcon)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // get icon info
    if (!GetIconInfo(sfi.hIcon, &iconinfo) && iconinfo.fIcon)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // get bitmap information on this icon 
    if (!GetObject(iconinfo.hbmColor, sizeof(bm), &bm) || !GetObject(iconinfo.hbmMask, sizeof(bmMask), &bmMask))
    {
    hr = E_FAIL;
        goto Cleanup;
    }

    // if this the most efficient way to round up to 4?
    cbDib = (bm.bmWidthBytes + 3) / 4 * 4 * bm.bmHeight;
    cbMask = (bmMask.bmWidthBytes + 3) / 4 * 4 * bm.bmHeight;

    // Compute the number of colors using BitsPixel only if BitsPixel is no
    // greater than 8 
    if (bm.bmBitsPixel <= 8)
        nColors = 1 << bm.bmBitsPixel;

    // Allocate the maximum possible memory 
    cb = cbDib + cbMask + sizeof(BITMAPINFOHEADER) + sizeof(ICONDIR) + sizeof(ICONDIRENTRY) + 
         nColors * sizeof(RGBQUAD);
    pb = new(Mt(CSysimageProtocolDoParseAndBind_pb)) BYTE [cb];
    if (!pb)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // zero initialize
    ZeroMemory(pb, cb);
    
    // Set up the icon file header
    pifh = (ICONDIR *)pb;
    pifh->iReserved = 0;
    pifh->iResourceType = 1;
    pifh->cresIcons = 1;

    // ICONDIRENTRY
    pide = (ICONDIRENTRY *)(pb + sizeof(ICONDIR)); 
    pide->cx = bm.bmWidth;
    pide->cy = bm.bmHeight;
    pide->nColors = (BYTE)nColors;
    pide->cbDIB = cbDib;
    pide->offsetDIB = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);

    // BITMAPINFOHEADER
    pbmi = (BITMAPINFO *)(pb + sizeof(ICONDIR) + sizeof(ICONDIRENTRY));
    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = bm.bmWidth;
    pbmi->bmiHeader.biHeight         = bm.bmHeight;
    pbmi->bmiHeader.biPlanes         = 1;
    pbmi->bmiHeader.biBitCount       = bm.bmBitsPixel;

    pvdBitsXOR = (LPVOID)(pb + sizeof(ICONDIR) + sizeof(ICONDIRENTRY) + sizeof(BITMAPINFOHEADER)
                                 + nColors * sizeof(RGBQUAD));
    hdcT = CreateCompatibleDC(NULL);
    if (!GetDIBits(hdcT, iconinfo.hbmColor, 0, bm.bmHeight, pvdBitsXOR, pbmi, DIB_RGB_COLORS))
        goto Cleanup;
    
    pvdBitsAND = (BYTE *)pvdBitsXOR + cbDib;
    bmiMask = *pbmi;
    bmiMask.bmiHeader.biBitCount = 1;
    if (!GetDIBits(hdcT, iconinfo.hbmMask, 0, bmMask.bmHeight, pvdBitsAND, &bmiMask, DIB_RGB_COLORS))
        goto Cleanup;

    // set the height to be twice the real image height, this is icon file standard
    pbmi->bmiHeader.biHeight = bm.bmHeight * 2;

    // tell URLMON we have an ICON image
    pchBuf = (TCHAR *)pb;
    hr = THR(pProt->_pProtSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, TEXT("image/x-icon")));
    if (hr)
        goto Cleanup;

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
            if (!hr && cb > 0)
            {   
                pProt->_ReportData(cb);
            }
            if (pProt->_pProtSink)
            {
                pProt->_pProtSink->ReportResult(hr, 0, 0);
            }
        }
    }

    if (hdcT)
        DeleteDC(hdcT);

    if (prostm)
    {
        prostm->Release();
    }

    delete [] pb;
    RRETURN(hr);
}

