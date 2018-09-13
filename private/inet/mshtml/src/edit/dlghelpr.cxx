//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       DLGHELPR.CXX
//
//  Contents:   DlgHelpr OC which gets embedded in design time dialogs
//
//  Classes:    CHtmlDlgHelper
//
//  History:    12-Mar-98   raminh  Created
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_DLGHELPR_H_
#define X_DLGHELPR_H_
#include "dlghelpr.h"
#endif

MtDefine(CHtmlDlgHelper, Utilities, "CHtmlDlgHelper")

// Global variable used for retaining the save path
TCHAR       g_achSavePath[MAX_PATH];

MtDefine(CFontNameOptions, Utilities, "CFontNameOptions")
MtDefine(CFontNameOptions_aryFontNames_pv, CFontNameOptions, "CFontNameOptions::_aryFontNames::_pv")


//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::length
//
//  Sysnopsis : IHtmlFontNameCollection interface method
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
//  Sysnopsis : IHtmlFontNameCollection interface method
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

//+------------------------------------------------------------------------
//
//  Function:   get_document   
//
//  Synopsis:   Fetches the dialogs' document object, corresponds to document
//              property on the OC
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::get_document(LPDISPATCH * pVal)
{
    HRESULT hr;
    IHTMLDocument2 * pDoc = NULL;

    if (m_spClientSite)
    {
        hr = m_spClientSite->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc);
        *pVal = pDoc;
    }
    else
        *pVal = NULL;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   openfiledlg   
//
//  Synopsis:   Brings up the file open dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::openfiledlg(VARIANT initFile, VARIANT initDir, VARIANT filter, VARIANT title, BSTR * pathName)
{
    HWND            hwndDlg = NULL;
    HRESULT         hr = S_FALSE;

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hwndDlg );
    hr = THR(OpenSaveFileDlg( initFile, initDir, filter, title, pathName, FALSE, hwndDlg));
Cleanup:
    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Function:   savefiledlg   
//
//  Synopsis:   Brings up the file save dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::savefiledlg(VARIANT initFile, VARIANT initDir, VARIANT filter, VARIANT title, BSTR * pathName)
{
    HWND            hwndDlg = NULL;
    HRESULT         hr = S_FALSE;

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hwndDlg );
    hr = THR(OpenSaveFileDlg( initFile, initDir, filter, title, pathName, FALSE, hwndDlg));
Cleanup:
    RRETURN( hr );
}

// We don't want to include the CRuntime so we've built the routine here.

// IEEE format specifies these...
// +Infinity: 7FF00000 00000000
// -Infinity: FFF00000 00000000
//       NAN: 7FF***** ********
//       NAN: FFF***** ********

// We also test for these, because the MSVC 1.52 CRT produces them for things
// like log(0)...
// +Infinity: 7FEFFFFF FFFFFFFF
// -Infinity: FFEFFFFF FFFFFFFF


// returns true for non-infinite nans.
int isNAN(double dbl)
{
    union
    {
        USHORT rgw[4];
        ULONG  rglu[2];
        double dbl;
    } v;

    v.dbl = dbl;

#ifdef BIG_ENDIAN
    return 0 == (~v.rgw[0] & 0x7FF0) &&
        ((v.rgw[0] & 0x000F) || v.rgw[1] || v.rglu[1]);
#else
    return 0 == (~v.rgw[3] & 0x7FF0) &&
        ((v.rgw[3] & 0x000F) || v.rgw[2] || v.rglu[0]);
#endif
}


// returns false for infinities and nans.
int isFinite(double dbl)
{
    union
    {
        USHORT rgw[4];
        ULONG rglu[2];
        double dbl;
    } v;

    v.dbl = dbl;

#ifdef BIG_ENDIAN
    return (~v.rgw[0] & 0x7FE0) ||
        0 == (v.rgw[0] & 0x0010) &&
        (~v.rglu[1] || ~v.rgw[1] || (~v.rgw[0] & 0x000F));
#else
    return (~v.rgw[3] & 0x7FE0) ||
        0 == (v.rgw[3] & 0x0010) &&
        (~v.rglu[0] || ~v.rgw[2] || (~v.rgw[3] & 0x000F));
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   VARIANTARGChangeTypeSpecial
//
//  Synopsis:   Helper.
//              Converts a VARIANT of arbitrary type to a VARIANT of type VT,
//              using browswer specific conversion rules, which may differ from
//              standard OLE Automation conversion rules (usually because
//              Netscape does something wierd).
//
//              This was pulled out of VARIANTARGToCVar because its also called
//              from CheckBox databinding.
//  
//  Arguments:  [pVArgDest]     -- Destination VARIANT (should already be init'd).
//              [vt]            -- Type to convert to.
//              [pvarg]         -- Variant to convert.
//              [pv]            -- Location to place C-language variable.
//
//  Modifies:   [pv].
//
//  Returns:    HRESULT.
//
//  History:    1-7-96  cfranks pulled out from VARIANTARGToCVar.
//
//----------------------------------------------------------------------------

HRESULT
VariantChangeTypeSpecial(VARIANT *pVArgDest, VARIANT *pvarg, VARTYPE vt,IServiceProvider *pSrvProvider, DWORD dwFlags)
{
    HRESULT             hr;
    IVariantChangeType *pVarChangeType = NULL;

    if (pSrvProvider)
    {
        hr = THR(pSrvProvider->QueryService(SID_VariantConversion,
                                            IID_IVariantChangeType,
                                            (void **)&pVarChangeType));
        if (hr)
            goto OldWay;

        // Use script engine conversion routine.
        hr = pVarChangeType->ChangeType(pVArgDest, pvarg, 0, vt);

        //Assert(!hr && "IVariantChangeType::ChangeType failure");
        if (!hr)
            goto Cleanup;   // ChangeType suceeded we're done...
    }

    // Fall back to our tried & trusted type coercions
OldWay:

    hr = S_OK;

    if (vt == VT_BSTR && V_VT(pvarg) == VT_NULL)
    {
        // Converting a NULL to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( _T("null") );           
        goto Cleanup;
    }
    else if (vt == VT_BSTR && V_VT(pvarg) == VT_EMPTY)
    {
        // Converting "undefined" to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( _T("undefined") );
        goto Cleanup;
    }
    else if (vt == VT_BOOL && V_VT(pvarg) == VT_BSTR)
    {
        // Converting from BSTR to BOOL
        // To match Navigator compatibility empty strings implies false when
        // assigned to a boolean type any other string implies true.
        V_VT(pVArgDest) = VT_BOOL;
        V_BOOL(pVArgDest) = ( V_BSTR(pvarg) && SysStringLen( V_BSTR(pvarg) ) ) ? VB_TRUE : VB_FALSE ;
        goto Cleanup;
    }
    else if (  V_VT(pvarg) == VT_BOOL && vt == VT_BSTR )
    {
        // Converting from BOOL to BSTR
        // To match Nav we either get "true" or "false"
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( V_BOOL(pvarg) == VB_TRUE ? _T("true") : _T("false") );     
        goto Cleanup;
    }
    // If we're converting R4 or R8 to a string then we need special handling to
    // map Nan and +/-Inf.
    else if (vt == VT_BSTR && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4))
    {
        double  dblValue = V_VT(pvarg) == VT_R8 ? V_R8(pvarg) : (double)(V_R4(pvarg));

        // Infinity or NAN?
        if (!isFinite(dblValue))
        {
            if (isNAN(dblValue))
            {
                // NAN
                V_BSTR(pVArgDest) = SysAllocString(_T("NaN") );
            }
            else
            {
                // Infinity
                V_BSTR(pVArgDest) = SysAllocString((dblValue < 0) ? _T("-Infinity") : _T("Infinity"));
            }
        }
        else
            goto DefaultConvert;


        // Any error from allocating string?
        if (hr)
           goto Cleanup;

        V_VT(pVArgDest) = vt;
        goto Cleanup;
    }


DefaultConvert:
    //
    // Default VariantChangeTypeEx.
    //

    // VARIANT_NOUSEROVERRIDE flag is undocumented flag that tells OLEAUT to convert to the lcid
    // given. Without it the conversion is done to user localeid
    hr = THR(VariantChangeTypeEx(pVArgDest, pvarg, LCID_SCRIPTING, dwFlags|VARIANT_NOUSEROVERRIDE, vt));

    
    if (hr == DISP_E_TYPEMISMATCH  )
    {
        if ( V_VT(pvarg) == VT_NULL )
        {
            hr = S_OK;
            switch ( vt )
            {
            case VT_BOOL:
                V_BOOL(pVArgDest) = VB_FALSE;
                V_VT(pVArgDest) = VT_BOOL;
                break;

            // For NS compatability - NS treats NULL args as 0
            default:
                V_I4(pVArgDest)=0;
                break;
            }
        }
        else if (V_VT(pvarg) == VT_DISPATCH )
        {
            // Nav compatability - return the string [object] or null 
            V_VT(pVArgDest) = VT_BSTR;
            V_BSTR(pVArgDest) = SysAllocString ( (V_DISPATCH(pvarg)) ? _T("[object]") : _T("null"));
        }
        else if ( V_VT(pvarg) == VT_BSTR && V_BSTRREF(pvarg)  &&
            ( (V_BSTR(pvarg))[0] == _T('\0')) && (  vt == VT_I4 || vt == VT_I2 || vt == VT_UI2 || vt == VT_UI4 || vt == VT_I8 ||
                vt == VT_UI8 || vt == VT_INT || vt == VT_UINT ) )
        {
            // Converting empty string to integer => Zero
            hr = S_OK;
            V_VT(pVArgDest) = vt;
            V_I4(pVArgDest) = 0;
            goto Cleanup;
        }    
    }
    else if (hr == DISP_E_OVERFLOW && vt == VT_I4 && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4))
    {
        // Nav compatability - return MAXLONG on overflow
        V_VT(pVArgDest) = VT_I4;
        V_I4(pVArgDest) = MAXLONG;
        hr = S_OK;
        goto Cleanup;
    }

    // To match Navigator change any scientific notation E to e.
    if (!hr && (vt == VT_BSTR && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4)))
    {
        TCHAR *pENotation;

        pENotation = _tcschr(V_BSTR(pVArgDest), _T('E'));
        if (pENotation)
            *pENotation = _T('e');
    }

Cleanup:
    ReleaseInterface(pVarChangeType);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   choosecolordlg   
//
//  Synopsis:   Brings up the color picker dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::choosecolordlg(VARIANT initColor, long * rgbColor)
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
    DWORD           dwResult = 0; // BUGBUG: raminh needs to review this
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

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hWndInPlace );

    for (i = ARRAY_SIZE(aCColors) - 1; i >= 0; i--)
    {
        aCColors[i] = RGB(255, 255, 255);
    }

    // Initialize ofn struct
    memset(&structCC, 0, sizeof(structCC));
    structCC.lStructSize     = sizeof(structCC);
    structCC.hwndOwner       = hWndInPlace;
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
    EnsureWrappersLoaded();
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

    //RRETURN(SetErrorInfo( hr ));
    RRETURN( hr );
#endif // WINCE}
}


//+------------------------------------------------------------------------
//
//  Function:   InterfaceSupportsErrorInfo   
//
//  Synopsis:   Rich error support per ATL
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IHtmlDlgHelper,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}


//+----------------------------------------------------------------
//
//  Function : OpenSaveFileDlg
//
//  Synopsis : IHTMLOptionsHolder method. bring up open or save file dialog and 
//             returns the selected filename
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
CHtmlDlgHelper::OpenSaveFileDlg( VARIANTARG initFile, VARIANTARG initDir, VARIANTARG filter, VARIANTARG title, BSTR *pathName, BOOL fSaveFile, HWND hwndInPlace)
{
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    VARIANT *       pvarInitFile;
    VARIANT *       pvarInitDir;
    VARIANT *       pvarFilter;
    VARIANT *       pvarTitle;
    OPENFILENAME    ofn;
    HRESULT         hr = E_INVALIDARG;
    BSTR            bstrFile = 0;
    TCHAR           *pstrExt;
    TCHAR           achPath[MAX_PATH];

    VERIFY_VARARG_4BSTR(initFile, pvarInitFile);
    VERIFY_VARARG_4BSTR(initDir, pvarInitDir);
    VERIFY_VARARG_4BSTR(filter, pvarFilter);
    VERIFY_VARARG_4BSTR(title, pvarTitle);

    bstrFile = SysAllocStringLen(NULL, MAX_PATH);
    if (bstrFile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    else
    {
        hr = S_OK;
    }

    if (pvarInitFile)
    {
        _tcscpy(bstrFile, V_BSTR(pvarInitFile));
    }
    else
    {
        *bstrFile = _T('\0');
    }

    Assert(hwndInPlace);
    // Initialize ofn struct
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hwndInPlace;
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

    _tcscpy(achPath, g_achSavePath);
    ofn.lpstrInitialDir = *achPath ? achPath : NULL;

    DbgMemoryTrackDisable(TRUE);

    // Call function
    fOK = (fSaveFile ? GetSaveFileName : GetOpenFileName)(&ofn);

    DbgMemoryTrackDisable(FALSE);

    if (!fOK)
    {
        SysFreeString(bstrFile);
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


//+------------------------------------------------------------------------
//
//  Function:   getCharset
//
//  Synopsis:   Obtains a character-set identifier for the font that 
//              is currently selected 
//
//-------------------------------------------------------------------------

HRESULT
CHtmlDlgHelper::getCharset(BSTR fontName, long * charset)
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

    hdc = ::GetDC(NULL);
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
        ::ReleaseDC(NULL, hdc);
    }
    RRETURN( SetErrorInfo( hr ) );
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

//+----------------------------------------------------------------
//
//  member : fonts
//
//  Synopsis : IHTMLOptionsHolder Property. returns an Ole collection of
//      BSTR of the available fonts
//
//+----------------------------------------------------------------

HRESULT
CHtmlDlgHelper::get_fonts(IHtmlFontNamesCollection ** ppFontCollection)
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
        _pFontNameObj = new CComObject<CFontNameOptions>;
        if (!_pFontNameObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _pFontNameObj->AddRef();
        _pFontNameObj->SetSize(0);

        // load it with the current system fonts
        m_spInPlaceSite->GetWindow(&hWndInPlace);

        hdc = ::GetDC(hWndInPlace);
        EnumFontFamilies(hdc,
                         NULL,
                         (FONTENUMPROC) GetFontNameProc,
                         (LPARAM)_pFontNameObj);
        ::ReleaseDC(hWndInPlace, hdc);
    }

    // QI for an interface to return
    hr = THR_NOTRACE(_pFontNameObj->QueryInterface(
                                    IID_IHtmlFontNamesCollection,
                                    (void**)ppFontCollection));

    // We keep an additional ref because we cache the name collection obj

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

VOID            
CHtmlDlgHelper::EnsureWrappersLoaded()
{
    static fLoaded = FALSE;

    if (!fLoaded)
    {
        InitUnicodeWrappers();
        fLoaded = TRUE;
    }
}

