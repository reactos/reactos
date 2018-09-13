//=================================================================
//
//   File:      optshold.cxx
//
//  Contents:   COptionsHolder class
//
//  Classes:    COptionsHolder
//              CFontNameOptions
//              CFontSizeOptions
//
//=================================================================

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include <uwininet.h>
#endif

#define _cxx_
#include "optshold.hdl"

MtDefine(COptionsHolder, ObjectModel, "COptionsHolder")
MtDefine(COptionsHolder_aryFontSizeObjects_pv, COptionsHolder, "COptionsHolder::_aryFontSizeObjects::_pv")
MtDefine(CFontNameOptions, ObjectModel, "CFontNameOptions")
MtDefine(CFontNameOptions_aryFontNames_pv, CFontNameOptions, "CFontNameOptions::_aryFontNames::_pv")
MtDefine(CFontSizeOptions, ObjectModel, "CFontSizeOptions")
MtDefine(CFontSizeOptions_aryFontSizes_pv, CFontSizeOptions, "CFontSizeOptions::_aryFontSize::_pv")

#define  START_OF_SAMPLE_STRINGS    0x0700

extern CCriticalSection    g_csFile;
extern TCHAR               g_achSavePath[];
extern BSTR                g_bstrFindText;

HRESULT
SetFindText(LPCTSTR bstr)
{
    LOCK_GLOBALS;

    FormsFreeString(g_bstrFindText);
    g_bstrFindText = NULL;
    RRETURN(FormsAllocString(bstr, &g_bstrFindText));
}


//+-------------------------------------------------------------------
//
//  Callbacks:   GetFont*Proc
//
//  These procedures are called by the EnumFontFamilies and EnumFont calls.
//  It fills the combobox with the font facename and the size
//
//--------------------------------------------------------------------

int CALLBACK
GetFontNameProc(LOGFONT FAR    * lplf,
                TEXTMETRIC FAR * lptm,
                int              iFontType,
                LPARAM           lParam)
{
    // Do not show vertical fonts
    if (lParam && lplf->lfFaceName[0] != _T('@'))
        ((CFontNameOptions *)lParam)->AddName(lplf->lfFaceName);

    return TRUE;
}

int CALLBACK
GetFontSizeProc(LOGFONT FAR *    lplf,
                TEXTMETRIC FAR * lptm,
                int              iFontType,
                LPARAM           lParam)
{
    if (lParam)
       ((CFontSizeOptions *)lParam)->AddSize(lplf->lfHeight);

    return TRUE;
}

//+----------------------------------------------------------------
//
//  member : classdesc
//
//  Synopsis : CBase Class Descriptor Structure
//
//+----------------------------------------------------------------

const CBase::CLASSDESC COptionsHolder::s_classdesc =
{
    &CLSID_HTMLDocument,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLOptionsHolder,        // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+----------------------------------------------------------------
//
//  member : CTOR
//
//+----------------------------------------------------------------

COptionsHolder::COptionsHolder(CDoc * pDoc) : super(), _pDoc(pDoc)
{
    Assert(pDoc);

    _pDoc->AddRef();
    _pFontNameObj=NULL;

    VariantInit(&_execArg);
    _hParentWnd = NULL;
}

//+----------------------------------------------------------------
//
//  member : DTOR
//
//+----------------------------------------------------------------

COptionsHolder::~COptionsHolder()
{
    _aryFontSizeObjects.ReleaseAll();

    _pDoc->Release();
    ReleaseInterface(_pFontNameObj);

    VariantClear(&_execArg);
}

void
COptionsHolder::Passivate()
{
    IGNORE_HR(SetFindText(GetAAfindText()));

    super::Passivate();
}


//+---------------------------------------------------------------
//
//  Member  : COptionsHolder::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
COptionsHolder::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatch)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    default:
        if (iid == IID_IHTMLOptionsHolder)
        {
           *ppv = (IHTMLOptionsHolder *) this;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+----------------------------------------------------------------
//
//  member : get_document
//
//  Synopsis : IHTMLOptionsHolder property. returns the document
//      member
//
//+----------------------------------------------------------------

HRESULT
COptionsHolder::get_document(IHTMLDocument2 ** ppDocDisp)
{
    HRESULT hr = S_OK;

    if (!ppDocDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDocDisp = NULL;

    hr = THR_NOTRACE(_pDoc->QueryInterface(IID_IHTMLDocument2,
                                            (void**)ppDocDisp));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_unsecuredWindowOfDocument
//
//  Synopsis : IHTMLOptionsHolder property. returns the unsecured
//              window of the document
//
//+----------------------------------------------------------------

HRESULT
COptionsHolder::get_unsecuredWindowOfDocument(IHTMLWindow2 ** ppDocDisp)
{
    HRESULT hr = S_OK;
    COmWindow2 *    pWindow = NULL;

    if (!ppDocDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(_pDoc->EnsureOmWindow());
    if ( hr )
        goto Cleanup;


    pWindow = _pDoc->_pOmWindow->Window();

    Assert(pWindow);

    hr = THR_NOTRACE(pWindow->QueryInterface(IID_IHTMLWindow2,
                                            (void**)ppDocDisp));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : fonts
//
//  Synopsis : IHTMLOptionsHolder Property. returns an Ole collection of
//      BSTR of the available fonts
//
//+----------------------------------------------------------------

HRESULT
COptionsHolder::get_fonts(IHTMLFontNamesCollection ** ppFontCollection)
{
    HRESULT hr = S_OK;
    HWND    hWndInPlace;
    HDC     hdc;

    if (!ppFontCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppFontCollection = NULL;

    // make sure we've got a font options collection
    if (!_pFontNameObj)
    {
        _pFontNameObj = new CFontNameOptions();
        if (!_pFontNameObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        _pFontNameObj->SetSize(0);

        // load it with the current system fonts
        _pDoc->GetWindow(&hWndInPlace);

        hdc = GetDC(hWndInPlace);
        EnumFontFamilies(hdc,
                         NULL,
                         (FONTENUMPROC) GetFontNameProc,
                         (LPARAM)_pFontNameObj);
        ReleaseDC(hWndInPlace, hdc);
    }

    // QI for an interface to return
    hr = THR_NOTRACE(_pFontNameObj->QueryInterface(
                                    IID_IHTMLFontNamesCollection,
                                    (void**)ppFontCollection));

    // We keep an additional ref because we cache the name collection obj

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : secureProtocolInfo
//
//  Synopsis : IHTMLOptionsHolder Property. returns a BSTR which 
//      describes the secure connection info. 
//      Empty string if current connection is insecure
//+----------------------------------------------------------------

HRESULT
COptionsHolder::get_secureConnectionInfo(BSTR * p)
{
    HRESULT hr = S_OK;
    BOOL bSuccess = FALSE;

    if ( p == NULL)
    {
        return E_POINTER;
    }
                
    if (_pDoc->HtmCtx() != NULL)
    {
        INTERNET_SECURITY_CONNECTION_INFO * pSCI = NULL;
        _pDoc->HtmCtx()->GetSecConInfo(&pSCI);
        if (!pSCI)
        {
            // Without benefit of INTERNET_SECURITY_CONNECTION_INFO, report "Encrypted."
            // if we believe the original source of the page to be secure
            
            TCHAR achMessage[FORMS_BUFLEN + 1];
            SSL_SECURITY_STATE sss;
            SSL_PROMPT_STATE sps;

            _pDoc->GetRootSslState(&sss, &sps);

            if (sss > SSL_SECURITY_MIXED)
            {
                if (!LoadString(GetResourceHInst(), IDS_SECURESOURCE, achMessage, ARRAY_SIZE(achMessage)))
                    return E_FAIL;

                *p = SysAllocString(achMessage);
                if (*p == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    bSuccess = TRUE;
                }
            }
        }
        
        if (pSCI != NULL && pSCI->fSecure)
        {
            // These are way beyond the sizes required for protocol & algorithm names (SSL 2.0, RC4, ...)
            // Assert's added in case the strings ever exceed the limit. 
            TCHAR achProtocol[32]; 
            TCHAR achAlgCipher[64];
            TCHAR achAlgExch[64];
            DWORD dwProtocol = ARRAY_SIZE(achProtocol);;
            DWORD dwAlgCipher = ARRAY_SIZE(achAlgCipher);
            DWORD dwAlgExch = ARRAY_SIZE(achAlgExch);
             
            if ( InternetSecurityProtocolToString(pSCI->dwProtocol, achProtocol, &dwProtocol, 0) && 
                 InternetAlgIdToString(pSCI->aiCipher, achAlgCipher, &dwAlgCipher, 0) &&
                 InternetAlgIdToString(pSCI->aiExch, achAlgExch, &dwAlgExch, 0)
               )
            {
                int idCipherQuality;
                
                if ( pSCI->dwCipherStrength < 56 )
                    idCipherQuality = IDS_SECURE_LOW;
                else if ( pSCI->dwCipherStrength < 128 )
                    idCipherQuality = IDS_SECURE_MEDIUM;
                else
                    idCipherQuality = IDS_SECURE_HIGH;
                
                TCHAR achSecurityInfo[256]; // This is way beyond what we can handle. 
                                
                hr = THR(Format(0,
                                achSecurityInfo,
                                ARRAY_SIZE(achSecurityInfo),
                                MAKEINTRESOURCE(IDS_SECURECONNECTIONINFO),
                                achProtocol,
                                achAlgCipher,
                                pSCI->dwCipherStrength,
                                GetResourceHInst(), idCipherQuality,
                                achAlgExch,
                                pSCI->dwExchStrength
                            ));

                if (SUCCEEDED(hr))
                {
                    *p = SysAllocString(achSecurityInfo);
                    if (*p == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        bSuccess = TRUE;
                    }
                }
            }
            else
            {
                DWORD dwError = GetLastError();
                // This implies the stack buffers we have allocated are not big enough!!
                Assert(dwError != ERROR_INSUFFICIENT_BUFFER);
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }  /* if the connection was secure */
    } /* if we found a _pHtmCtx */

    // If we failed somewhere along the way. just allocate an empty string and return quietly. 
    if (!bSuccess)
    {
        *p = SysAllocString(_T(""));
        if (*p == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }                                                                                                                
                                         
    return hr;    
}



//+----------------------------------------------------------------
//
//  member : sizes
//
//  Synopsis : IHTMLOptionsHolder Property. returns an Ole Collecion of
//      longs of the available sizes available for the given font
//
//+----------------------------------------------------------------

HRESULT
COptionsHolder::sizes( BSTR bstrFontName,
                       IHTMLFontSizesCollection ** ppOptsCollection)
{
    HRESULT hr = S_OK;
    HWND    hWndInPlace;
    HDC     hdc;
    CStr    strName;
    long    lIndex;

    if (!ppOptsCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    if (!bstrFontName || !SysStringLen(bstrFontName))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    lIndex = GetObjectLocation(bstrFontName);
    if (lIndex<0)
    {
        // some error happened
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If we don't have the sizes cached for this font....
    if (lIndex == _aryFontSizeObjects.Size())
    {
        CFontSizeOptions * pfsObject=NULL;

        // add a new one to the list
        hr = THR(_aryFontSizeObjects.EnsureSize(lIndex+1));
        if (hr)
            goto Cleanup;

        pfsObject = new CFontSizeOptions();
        if (!pfsObject)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pfsObject->_cstrFont.SetBSTR(bstrFontName);
        pfsObject->SetSize(0);

        _pDoc->GetWindow(&hWndInPlace);
        hdc = GetDC(hWndInPlace);
#ifdef WIN16
        EnumFontFamilies( hdc, pfsObject->_cstrFont,
#else
        EnumFonts( hdc, pfsObject->_cstrFont,
#endif
                   (FONTENUMPROC) GetFontSizeProc,
                   (LPARAM)pfsObject);
        ReleaseDC(hWndInPlace, hdc);

        _aryFontSizeObjects[lIndex] = pfsObject;
    }

        // QI for an interface to return
    hr = THR_NOTRACE(_aryFontSizeObjects[lIndex]->QueryInterface(
                                    IID_IHTMLFontSizesCollection,
                                    (void**)ppOptsCollection));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : openfiledlg
//
//  Synopsis : IHTMLOptionsHolder method. bring up open file dialog and 
//              returns the selected filename
//
//+----------------------------------------------------------------

HRESULT
COptionsHolder::openfiledlg( VARIANTARG initFile, VARIANTARG initDir, VARIANTARG filter, VARIANTARG title, BSTR *pathName)
{
    RRETURN(SetErrorInfo(OpenSaveFileDlg(initFile, initDir, filter, title, pathName, FALSE)));
}

//+----------------------------------------------------------------
//
//  member : savefiledlg
//
//  Synopsis : IHTMLOptionsHolder method. bring up save file dialog and 
//              returns the selected filename
//
//+----------------------------------------------------------------

HRESULT
COptionsHolder::savefiledlg( VARIANTARG initFile, VARIANTARG initDir, VARIANTARG filter, VARIANTARG title, BSTR *pathName)
{
    RRETURN(SetErrorInfo(OpenSaveFileDlg(initFile, initDir, filter, title, pathName, TRUE)));
}

//+----------------------------------------------------------------
//
//  member : OpenSaveFileDlg
//
//  Synopsis : IHTMLOptionsHolder method. bring up open or save file dialog and 
//              returns the selected filename
//
//+----------------------------------------------------------------

#define VERIFY_VARARG_4BSTR(arg, var)           \
    switch(V_VT(&arg))                          \
    {                                           \
    case    VT_BSTR:                            \
        var = &arg;                             \
        break;                                  \
    case    VT_BYREF|VT_BSTR:                   \
        var = V_VARIANTREF(&arg);               \
        break;                                  \
    default:                                    \
        var = NULL;                             \
    }

HRESULT
COptionsHolder::OpenSaveFileDlg( VARIANTARG initFile, VARIANTARG initDir, VARIANTARG filter, VARIANTARG title, BSTR *pathName, BOOL fSaveFile)
{
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    VARIANT *       pvarInitFile;
    VARIANT *       pvarInitDir;
    VARIANT *       pvarFilter;
    VARIANT *       pvarTitle;
    OPENFILENAME    ofn;
    HRESULT         hr = E_INVALIDARG;
    HWND            hWndInPlace;
    BSTR            bstrFile = 0;
    TCHAR           *pstrExt;
    TCHAR           achPath[MAX_PATH];

    VERIFY_VARARG_4BSTR(initFile, pvarInitFile);
    VERIFY_VARARG_4BSTR(initDir, pvarInitDir);
    VERIFY_VARARG_4BSTR(filter, pvarFilter);
    VERIFY_VARARG_4BSTR(title, pvarTitle);

    hr = THR(FormsAllocStringLen(NULL, MAX_PATH, &bstrFile));
    if (hr)
    {
        goto Cleanup;
    }

    if (pvarInitFile)
    {
        _tcscpy(bstrFile, V_BSTR(pvarInitFile));
    }
    else
    {
        *bstrFile = _T('\0');
    }

    _pDoc->GetWindow(&hWndInPlace);

    Assert(_hParentWnd);
    // Initialize ofn struct
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    // ofn.hwndOwner       = hWndInPlace;
    ofn.hwndOwner       = _hParentWnd;
    ofn.Flags           =   OFN_FILEMUSTEXIST   |
                            OFN_PATHMUSTEXIST   |
                            OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY    |
                            OFN_NOCHANGEDIR     |
                            OFN_EXPLORER;
                            // no readonly checkbox, per request

    ofn.lpfnHook        = NULL;
    ofn.lpstrFile       = bstrFile;     // file name buffer
    ofn.nMaxFile        = MAX_PATH;     // file name buffer size
    
    if (pvarInitDir)
    {
        ofn.lpstrInitialDir = V_BSTR(pvarInitDir);
    }

    if (pvarFilter)
    {
        BSTR    bstrFilter = V_BSTR(pvarFilter);
        TCHAR   *cp;

        for ( cp = bstrFilter; *cp; cp++ )
        {
            if ( *cp == _T('|') )
            {
                *cp = _T('\0');
            }
        }
        ofn.lpstrFilter = bstrFilter;
    }

    if (pvarTitle)
    {
        ofn.lpstrTitle = V_BSTR(pvarTitle);
    }

    //
    // Find the extension and set the filter index based on what the
    // extension is.  After these loops pstrExt will either be NULL if
    // we didn't find an extension, or will point to the extension starting
    // at the '.'

    pstrExt = bstrFile;
    while (*pstrExt)
        pstrExt++;
    while ( pstrExt > bstrFile )
    {
        if( *pstrExt == _T('.') )
            break;
        pstrExt--;
    }

    if( pstrExt > bstrFile )
    {
        int    iIndex = 0;
        const TCHAR* pSearch = ofn.lpstrFilter;

        while( pSearch )
        {
            if( wcsstr ( pSearch, pstrExt ) )
            {
                ofn.nFilterIndex = (iIndex / 2) + 1;
                ofn.lpstrDefExt = pstrExt + 1;

                // Remove the extension from the file name we pass in
                *pstrExt = _T('\0');

                break;
            }
            pSearch += _tcslen(pSearch);
            if( pSearch[1] == 0 )
                break;

            pSearch++;
            iIndex++;
        }
    }

    {
        LOCK_SECTION(g_csFile);

        _tcscpy(achPath, g_achSavePath);
        ofn.lpstrInitialDir = *achPath ? achPath : NULL;
    }

    DbgMemoryTrackDisable(TRUE);

    // Call function
    fOK = (fSaveFile ? GetSaveFileName : GetOpenFileName)(&ofn);

    DbgMemoryTrackDisable(FALSE);

    if (!fOK)
    {
        FormsFreeString(bstrFile);
        bstrFile = NULL;
#ifndef WINCE
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
            goto Cleanup;
        }
        else
        {
            hr = S_OK;
        }
#else //WINCE
        hr = E_FAIL;
#endif //WINCE
    }
    else
    {
        LOCK_SECTION(g_csFile);

        _tcscpy(g_achSavePath, ofn.lpstrFile);
        
        TCHAR * pchShortName =_tcsrchr(g_achSavePath, _T('\\'));

        if (pchShortName)
        {
            *(pchShortName + 1) = 0;
        }
        else
        {
            *g_achSavePath = 0;
        }
        hr = S_OK;
    }

Cleanup:

    *pathName = bstrFile;

    return hr;
}

HRESULT
COptionsHolder::choosecolordlg( VARIANTARG initColor, long *rgbColor)
{
#ifdef WINCE
    return S_OK;
#else
    int             i;
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    CHOOSECOLOR     structCC;
    HRESULT         hr = E_INVALIDARG;
    HWND            hWndInPlace;
    COLORREF        aCColors[16];
    VARIANT *       pvarRGBColor;
    DWORD           dwResult;
    VARIANTARG      varArgTmp;

    hr = THR (VariantChangeTypeSpecial(&varArgTmp, &initColor, VT_I4));
    if (hr)
    {
        pvarRGBColor = NULL;
    }
    else
    {
        if (V_VT(&initColor) & VT_BYREF)
        {
            pvarRGBColor = V_VARIANTREF(&varArgTmp);
        }
        else
        {
            pvarRGBColor = &varArgTmp;
        }
    }

    _pDoc->GetWindow(&hWndInPlace);

    for (i = ARRAY_SIZE(aCColors) - 1; i >= 0; i--)
    {
        aCColors[i] = RGB(255, 255, 255);
    }

    // Initialize ofn struct
    memset(&structCC, 0, sizeof(structCC));
    structCC.lStructSize     = sizeof(structCC);
    structCC.hwndOwner       = _hParentWnd;
    structCC.lpCustColors    = aCColors;
    
    if (pvarRGBColor)
    {
        structCC.Flags          = CC_RGBINIT;
        structCC.rgbResult      = V_I4(pvarRGBColor);
        dwResult                = structCC.rgbResult;
    }
    else
    {
        dwResult = RGB(0,0,0);
    }

    // Call function
    fOK = ChooseColor(&structCC);

    if (fOK)
    {
        hr = S_OK;
        dwResult = structCC.rgbResult;
    }
    else
    {
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
            goto Cleanup;
        }
        else
        {
            hr = S_OK;
        }
    }

Cleanup:

    *rgbColor = dwResult;

    RRETURN(SetErrorInfo( hr ));
#endif // WINCE
}

HRESULT
COptionsHolder::showSecurityInfo()
{
    HWND    hwnd = _hParentWnd;

    if (!_hParentWnd)
        _pDoc->GetWindow(&hwnd);
        
    InternetShowSecurityInfoByURL(_pDoc->_cstrUrl, hwnd);

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
COptionsHolder::isApartmentModel( IHTMLObjectElement* object, 
                                 VARIANT_BOOL * fApartment)
{
    HRESULT             hr = S_OK;
    IClientSecurity    *pCL = NULL;
    VARIANT_BOOL        vbRetVal = VB_FALSE;
    VARIANT             var;
    IOleCommandTarget  *pCT = NULL;

    VariantInit(&var);

    hr = THR(object->QueryInterface(IID_IOleCommandTarget, (void **) &pCT));
    if (hr)
        goto Cleanup;

    hr = THR(pCT->Exec((GUID *)&CGID_MSHTML, 
                        IDM_GETPUNKCONTROL, 
                        0, 
                        NULL, 
                        &var));
    if (hr)
        goto Cleanup;

    if (!var.punkVal)
    {
        vbRetVal = VB_TRUE;
        goto Cleanup;
    }

    //
    //  QI IClientSecurity returns S_OK if the control is NOT 
    //  apartment model!
    //
    hr = var.punkVal->QueryInterface(IID_IClientSecurity, (void **) &pCL);
    if (!hr)
    {
        hr = S_OK;      // It really is OK.
        goto Cleanup;
    }

    vbRetVal = VB_TRUE;

Cleanup:
    ReleaseInterface(pCL);
    ReleaseInterface(pCT);

    *fApartment = vbRetVal;

    RRETURN( SetErrorInfo( S_OK ) );
}


HRESULT
COptionsHolder::getCharset(BSTR fontName, long * charset)
{
    HRESULT         hr = S_OK;
    LOGFONT         lf;
    UINT            uintResult = 0;
    HDC             hdc = NULL;
    HFONT           hfont = NULL, hfontOld = NULL;

    if (!charset)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *charset = 0;

    hdc = GetDC(NULL);
    if (!hdc)
        goto Cleanup;

    memset(&lf, 0, sizeof(lf));

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality  = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    // If fontName is too long, we have to shorten it.
    _tcsncpy(lf.lfFaceName, fontName, LF_FACESIZE - 1);
    
    hfont = CreateFontIndirect(&lf);
    if (!hfont)
        goto Cleanup;

    hfontOld = (HFONT) SelectObject(hdc, hfont);
    if (!hfontOld)
        goto Cleanup;

    uintResult = GetTextCharset(hdc);

    *charset = uintResult;

Cleanup:

    if (hfontOld)
    {
        SelectObject(hdc, hfontOld);
    }

    if (hfont)
    {
        DeleteObject(hfont);
    }
    
    if (hdc)
    {
        ReleaseDC(NULL, hdc);
    }

    RRETURN( SetErrorInfo( hr ) );
}



//+----------------------------------------------------------------
//
//  Member GetObjetLocation
//
//  Synopsis : helper function for sizes. this will search the
//      pointer array of size collections to see if this one
//      already exists. if not, size() is returned.
//
//-----------------------------------------------------------------

long
COptionsHolder::GetObjectLocation(BSTR strTargetFontName)
{
    long    lSize = _aryFontSizeObjects.Size();
    long    l;

    // Look for a free slot in the non-reserved part of the cache.
    for (l=0; l < lSize; ++l)
    {
        if (!FormsStringICmp(_aryFontSizeObjects[l]->_cstrFont,
                             strTargetFontName))
            break;
    }

    return l;
}


HRESULT
STDMETHODCALLTYPE COptionsHolder::put_execArg(VARIANT varExecArg)
{
    VariantClear(&_execArg);
    RRETURN(SetErrorInfo(VariantCopy(&_execArg, &varExecArg)));
}

HRESULT 
STDMETHODCALLTYPE COptionsHolder::get_execArg(VARIANT *pexecArg)
{
    HRESULT     hr;

    if (pexecArg == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = VariantCopy(pexecArg, &_execArg);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CFontNameOptions
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CFontNameOptions::s_classdesc =
{
    &CLSID_HTMLDocument,             // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLFontNamesCollection,  // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+----------------------------------------------------------------
//
//  member : DTOR
//
//+----------------------------------------------------------------

CFontNameOptions::~CFontNameOptions()
{
    CStr *  pcstr;
    long    i;

    for (i = _aryFontNames.Size(), pcstr = _aryFontNames;
         i > 0;
         i--, pcstr++)
    {
        pcstr->Free();
    }

    _aryFontNames.DeleteAll();
}


//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatch)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    default:
        if (iid == IID_IHTMLFontNamesCollection)
        {
           *ppv = (IHTMLFontNamesCollection *) this;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member  : AddName
//
//  Sysnopsis : Helper function that takes a font name from the font
//      callback and adds it to the cdataary.
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::AddName(TCHAR * strFontName)
{
    HRESULT hr = S_OK;
    long    lIndex;
    long    lSizeAry = _aryFontNames.Size();

    // does this name already exist in the list
    for (lIndex = 0; lIndex < lSizeAry ; lIndex++)
    {
        if (_tcsiequal(strFontName, _aryFontNames[lIndex]))
            break;
    }

    // Not found, so add element to array.
    if (lIndex == lSizeAry)
    {
        CStr *pcstr;

        hr = THR(_aryFontNames.AppendIndirect(NULL, &pcstr));
        if (hr)
            goto Cleanup;

        hr = pcstr->Set(strFontName);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::length
//
//  Sysnopsis : IHTMLFontNameCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aryFontNames.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));

}

//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::item
//
//  Sysnopsis : IHTMLFontNameCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::item(long lIndex, BSTR * pstrName)
{
    HRESULT   hr   = S_OK;

    if (!pstrName)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (lIndex < 0 || lIndex >= _aryFontNames.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = _aryFontNames[lIndex].AllocBSTR(pstrName);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::_newEnum
//
//  Sysnopsis :
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryFontNames.EnumVARIANT(VT_BSTR,
                                (IEnumVARIANT**)ppEnum,
                                FALSE,
                                FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CFontSizeOptions
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CFontSizeOptions::s_classdesc =
{
    &CLSID_HTMLDocument,             // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLFontSizesCollection,  // _piidDispinterface
    &s_apHdlDescs,                      // _apHdlDesc
};

//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CFontSizeOptions::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatch)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    default:
        if (iid == IID_IHTMLFontSizesCollection)
        {
           *ppv = (IHTMLFontSizesCollection *) this;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::AddSize
//
//  Sysnopsis : adds the given size to the aryFontSizes
//          in ascending order.
//
//----------------------------------------------------------------
HRESULT
CFontSizeOptions::AddSize(long lFSize)
{
    HRESULT hr = S_OK;
    int     i, value, iSizeAry = _aryFontSizes.Size();

    for (i=0; i<iSizeAry; i++)
    {
        value = _aryFontSizes[i];

        // is it already in the list
        if (lFSize == value)
            goto Cleanup;

        // is it smaller than the thing we're looking at
        if (lFSize < value)
            break;
    }

    // it is not in the list and smaller than the value at index i
    // or i = iSizeAry and we want to tack this at the end.
    hr = THR(_aryFontSizes.Insert(i, lFSize));

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member  : CFontSizeOptions::length
//
//  Sysnopsis : IHTMLFONTSizesCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontSizeOptions::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aryFontSizes.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));

}

//+---------------------------------------------------------------
//
//  Member  : CFontSizeOptions::item
//
//  Sysnopsis : IHTMLFONTSizesCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontSizeOptions::item(long lIndex, long *plSize)
{
    HRESULT hr = S_OK;

    if (!plSize)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (lIndex < 0 || lIndex >= _aryFontSizes.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plSize = _aryFontSizes[lIndex];

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------
//
//  Member  : CFontSizeOptions::_newEnum
//
//  Sysnopsis : IHTMLFONTSizesCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontSizeOptions::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryFontSizes.EnumVARIANT(VT_I4,
                                (IEnumVARIANT**)ppEnum,
                                FALSE,
                                FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


