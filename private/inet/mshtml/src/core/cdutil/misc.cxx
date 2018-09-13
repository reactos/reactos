//+---------------------------------------------------------------------
//
//  File:       misc.cxx
//
//  Contents:   Useful OLE helper functions
//
//----------------------------------------------------------------------

#include "headers.hxx"

MAKE_UNALIGNED_TYPE( LONG, 4 );

#if DBG == 1 && !defined(WIN16)
//
// Global vars for use by the DYNCAST macro
//
char g_achDynCastMsg[200];
char *g_pszDynMsg = "Invalid Static Cast -- Attempt to cast object "
                    "of type %s to type %s.";
char *g_pszDynMsg2 = "Dynamic Cast Attempted ---  "
                     "Attempt to cast between two base classes of %s. "
                     "The cast was to class %s from some other base class "
                     "pointer. This cast will not succeed in a retail build.";
#endif

DeclareTag(tagAccess, "Accessibility", "Accessibility traces")

//+------------------------------------------------------------------------
//
//  Function:   GetLastWin32Error
//
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
GetLastWin32Error( )
{
#ifdef WIN16
    return E_FAIL;
#else
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
#endif
}


//+------------------------------------------------------------------------
//
//  Function:   _MemAllocString
//
//  Synopsis:   Allocates a string copy using MemAlloc.
//
//              The inline function MemFreeString is provided for symmetry.
//
//  Arguments:  pchSrc    String to copy
//              ppchDest  Copy of string is returned in *ppch
//                        NULL is stored on error
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
_MemAllocString(const TCHAR *pchSrc, TCHAR **ppchDest)
{
    TCHAR *pch;
    size_t cb;

    cb = (_tcsclen(pchSrc) + 1) * sizeof(TCHAR);
    *ppchDest = pch = (TCHAR *)_MemAlloc(cb);
    if (!pch)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pch, pchSrc, cb);
        return S_OK;
    }
}



//+------------------------------------------------------------------------
//
//  Function:   _MemAllocString
//
//  Synopsis:   Allocates a string copy using MemAlloc.  Doesn't require
//              null-terminated input string.
//
//              The inline function MemFreeString is provided for symmetry.
//
//  Arguments:  cch       number of characters in input string,
//                        not including any trailing null character
//              pchSrc    pointer to source string
//              ppchDest  Copy of string is returned in *ppch
//                        NULL is stored on error
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
_MemAllocString(ULONG cch, const TCHAR *pchSrc, TCHAR **ppchDest)
{
    TCHAR *pch;
    size_t cb = cch * sizeof(TCHAR);

    *ppchDest = pch = (TCHAR *)_MemAlloc(cb + sizeof(TCHAR));
    if (!pch)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pch, pchSrc, cb);
        pch[cch] = 0;
        return S_OK;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   _MemReplaceString
//
//  Synopsis:   Allocates a string using MemAlloc, replacing and freeing
//              another string on success.
//
//  Arguments:  pchSrc    String to copy. May be NULL.
//              ppchDest  On success, original string is freed and copy of
//                        source string is returned here
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
_MemReplaceString(const TCHAR *pchSrc, TCHAR **ppchDest)
{
    HRESULT hr;
    TCHAR *pch;

    if (pchSrc)
    {
        hr = THR(_MemAllocString(pchSrc, &pch));
        if (hr)
            RRETURN(hr);
    }
    else
    {
        pch = NULL;
    }

    _MemFreeString(*ppchDest);
    *ppchDest = pch;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   TaskAllocString
//
//  Synopsis:   Allocates a string copy that can be passed across an interface
//              boundary, using the standard memory allocation conventions.
//
//              The inline function TaskFreeString is provided for symmetry.
//
//  Arguments:  pstrSrc    String to copy
//              ppstrDest  Copy of string is returned in *ppstr
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

#if !defined(_MACUNICODE) || defined(_MAC) 
HRESULT
TaskAllocString(const TCHAR *pstrSrc, TCHAR **ppstrDest)
{
    TCHAR *pstr;
    size_t cb;

    cb = (_tcsclen(pstrSrc) + 1) * sizeof(TCHAR);
    *ppstrDest = pstr = (TCHAR *)CoTaskMemAlloc(cb);
    if (!pstr)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pstr, pstrSrc, cb);
        return S_OK;
    }
}
#else
HRESULT
TaskAllocString(const OLECHAR *pstrSrc, OLECHAR **ppstrDest)
{
    OLECHAR *pstr;
    size_t cb;

    cb = (strlen(pstrSrc) + 1) * sizeof(OLECHAR);
    *ppstrDest = pstr = (OLECHAR *)CoTaskMemAlloc(cb);
    if (!pstr)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pstr, pstrSrc, cb);
        return S_OK;
    }
}
#endif

//+------------------------------------------------------------------------
//
//  Function:   TaskReplaceString
//
//  Synopsis:   Replaces a string copy that can be passed across an interface
//              boundary, using the standard memory allocation conventions.
//
//              The inline function TaskFreeString is provided for symmetry.
//
//  Arguments:  pstrSrc    String to copy. May be NULL.
//              ppstrDest  Copy of string is returned in *ppstrDest,
//                         previous string is freed on success
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT TaskReplaceString(const TCHAR * pstrSrc, TCHAR **ppstrDest)
{
    TCHAR *pstr;
    HRESULT hr;

    if (pstrSrc)
    {
        hr = THR(TaskAllocString(pstrSrc, &pstr));
        if (hr)
            RRETURN(hr);
    }
    else
    {
        pstr = NULL;
    }

    CoTaskMemFree(*ppstrDest);
    *ppstrDest = pstr;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   MulDivQuick
//
//  Synopsis:   Mutiply two signed 32-bit values and then divide the
//              64-bit result by a third unsigned 32-bit value.
//              The return value is rounded up or down to the nearest
//              integer.  If the multiplier and divisor are both zero,
//              this function returns 0.
//
//  Arguments:  nMultiplicand   Specifies the multiplicand.
//              nMultiplier     Specifies the multiplier.
//              nDivisor        Specifies the number by which the result of
//                              the multiplication (nMultiplicand * nMultiplier)
//                              is to be divided.
//
//----------------------------------------------------------------------------

// The OS routines are almost as fast (no inlining) and more robust, so
// this routine is no longer used...

#if 0

#pragma warning(disable: 4035)

int __declspec(naked) __stdcall
MulDivQuick(int nMultiplicand, int nMultiplier, int nDivisor)
{
    __asm
    {
        mov eax, 8[esp]
        test eax, eax
        je  L0
        mov ecx, 4[esp]
        imul ecx
        mov ecx, 12[esp]
        shr ecx, 1
        test edx, edx
        jl  L1
        add eax, ecx
        adc edx, 0
        mov ecx, 12[esp]
        idiv ecx
L0:
        ret 12
L1:
        sub eax, ecx
        sbb edx, 0
        mov ecx, 12[esp]
        idiv ecx
        ret 12
    }
}

#pragma warning(default: 4035)

#endif

#if !defined(_MAC) && !defined(WIN16)

//+---------------------------------------------------------------------------
//
//  Function:   _purecall
//
//  Synopsis:   Define our own implementation of this function so
//              that we don't suck in lots of junk from the CRT.
//
//----------------------------------------------------------------------------

extern "C" int _cdecl
_purecall(void)
{
    Assert(0 && "Pure virtual function called.");
    return 0;
}

#endif  // !defined(_MAC)


//+-------------------------------------------------------------------------
//
//  Function:   InitSystemMetricValues
//
//  Synopsis:   Initializes globals holding system metric values.
//
//--------------------------------------------------------------------------

LONG        g_cMetricChange = 0;

SIZE        g_sizeDragMin;
SIZE        g_sizeScrollbar;
SIZE        g_sizeScrollButton;
SIZEL       g_sizelScrollbar;
SIZEL       g_sizelScrollButton;
SIZEL       g_sizelScrollThumb;
SIZE        g_sizePixelsPerInch = {96, 96};
LONG        g_alHimetricFrom8Pixels[2] = {0, 0};
SIZE        g_sizeSystemChar;


#if NEVER // we should not change the unit in Forms3 96.

  // User specified unit in control panel
UNITS        g_unitsMeasure = UNITS_POINT;

#endif // NEVER

  // Locale Information
LCID        g_lcidUserDefault = 0;
BOOL        g_fUSSystem;
BOOL        g_fJapanSystem;
BOOL        g_fKoreaSystem;
BOOL        g_fCHTSystem;
BOOL        g_fCHSSystem;

UINT        g_cpDefault;

// hold for number shaping used by system: 0 = Context, 1 = None, 2 = Native
NUMSHAPE    g_iNumShape;
DWORD       g_uLangNationalDigits;

//  Accessibility information

BOOL g_fHighContrastMode = FALSE;
BOOL g_fScreenReader = FALSE;


HRESULT
InitSystemMetricValues(
    THREADSTATE *   pts)
{
#ifndef WIN16
    HIGHCONTRAST hc;
#endif // !WIN16
    HFONT       hfontOld;
    TEXTMETRIC  tm;

    InterlockedIncrement(&g_cMetricChange);

    if (!pts->hdcDesktop)
    {
        pts->hdcDesktop = CreateCompatibleDC(NULL);
        if (!pts->hdcDesktop)
            RRETURN(E_OUTOFMEMORY);
    }

    g_sizePixelsPerInch.cx = GetDeviceCaps(pts->hdcDesktop, LOGPIXELSX);
    g_sizePixelsPerInch.cy = GetDeviceCaps(pts->hdcDesktop, LOGPIXELSY);

#if defined(WIN16) || defined(_MAC)
    // bugbug: (Stevepro) Isn't there an ini file setting for this in win16 that is used
    //         for ole drag-drop?
    //
    // Width and height, in pixels, of a rectangle centered on a drag point
    // to allow for limited movement of the mouse pointer before a drag operation
    // begins. This allows the user to click and release the mouse button easily
    // without unintentionally starting a drag operation
    //
    g_sizeDragMin.cx = 3;
    g_sizeDragMin.cy = 3;
#else
    g_sizeDragMin.cx = GetSystemMetrics(SM_CXDRAG);
    g_sizeDragMin.cy = GetSystemMetrics(SM_CYDRAG);
#endif

    g_sizeScrollbar.cx = GetSystemMetrics(SM_CXVSCROLL);
    g_sizeScrollbar.cy = GetSystemMetrics(SM_CYHSCROLL);
    g_sizelScrollbar.cx = HimetricFromHPix(g_sizeScrollbar.cx);
    g_sizelScrollbar.cy = HimetricFromVPix(g_sizeScrollbar.cy);

    g_sizeScrollButton.cx  = GetSystemMetrics(SM_CXHSCROLL);
    g_sizeScrollButton.cy  = GetSystemMetrics(SM_CYVSCROLL);
    g_sizelScrollButton.cx = HimetricFromHPix(g_sizeScrollButton.cx);
    g_sizelScrollButton.cy = HimetricFromVPix(g_sizeScrollButton.cy);

    g_sizelScrollThumb.cx = HimetricFromHPix(GetSystemMetrics(SM_CXHTHUMB));
    g_sizelScrollThumb.cy = HimetricFromVPix(GetSystemMetrics(SM_CYVTHUMB));

    g_alHimetricFrom8Pixels[0] = HimetricFromHPix(8);
    g_alHimetricFrom8Pixels[1] = HimetricFromVPix(8);

    //
    // System font info
    //

    hfontOld = (HFONT)SelectObject(pts->hdcDesktop, GetStockObject(SYSTEM_FONT));
    if(hfontOld)
    {
        GetTextMetrics(pts->hdcDesktop, &tm);

        g_sizeSystemChar.cx = tm.tmAveCharWidth;
        g_sizeSystemChar.cy = tm.tmHeight;

        SelectObject(pts->hdcDesktop, hfontOld);
    }
    else
    {
        g_sizeSystemChar.cx =
        g_sizeSystemChar.cy = 10;
    }


    //
    // Locale info
    //

    g_cpDefault = GetACP();

    g_lcidUserDefault = GetSystemDefaultLCID(); //Set Global Locale ID

    g_fUSSystem = FALSE;
    g_fJapanSystem = FALSE;
    g_fKoreaSystem = FALSE;
    g_fCHTSystem = FALSE;
    g_fCHSSystem = FALSE;

    
    GetSystemNumberSettings(&g_iNumShape, &g_uLangNationalDigits);


    // COMPLEXSCRIPT
    // note - this appears to be used solely to determine if a system is FE to 
    //        use in ...\src\site\base\taborder.cxx or if it is Japanese to use
    //        in ...\src\core\cdutil\wndclass.cxx
    switch (PRIMARYLANGID(LANGIDFROMLCID(g_lcidUserDefault)))
    {
    case LANG_JAPANESE:
        g_fJapanSystem = TRUE;
        break;
    case LANG_KOREAN:
        g_fKoreaSystem = TRUE;
        break;
    case LANG_CHINESE:
        switch (SUBLANGID(LANGIDFROMLCID(g_lcidUserDefault)))
        {
        case SUBLANG_CHINESE_TRADITIONAL:
            g_fCHTSystem = TRUE;
            break;
        case SUBLANG_CHINESE_SIMPLIFIED:
            g_fCHSSystem = TRUE;
            break;
        }
        break;

    default:
        g_fUSSystem = TRUE;
        break;
    }

#if !defined(WIN16) && !defined(UNIX)
    //
    //  Accessibility info
    //

    SystemParametersInfo(SPI_GETSCREENREADER, 0, &g_fScreenReader, FALSE);

    memset(&hc, 0, sizeof(HIGHCONTRAST));
    hc.cbSize = sizeof(HIGHCONTRAST);
    if (SystemParametersInfo(
                SPI_GETHIGHCONTRAST,
                sizeof(HIGHCONTRAST),
                &hc,
                0))
    {
        g_fHighContrastMode = !!(hc.dwFlags & HCF_HIGHCONTRASTON);
    }
    else
    {
        TraceTag((tagAccess, "SPI failed with error %x", GetLastError()));
    }
#endif // !WIN16

     RRETURN(S_OK);
 }

//+-------------------------------------------------------------------------
//
//  Function:   DeinitSystemMetricValues
//
//  Synopsis:   Deinitializes globals holding system metric values.
//
//--------------------------------------------------------------------------

void
DeinitSystemMetricValues(
    THREADSTATE *   pts)
{
   if(pts->hdcDesktop)
   {
       Verify(DeleteDC(pts->hdcDesktop));
#ifdef _MAC
    // Mac note: We need to recreate the hdcDesktop on a WM_SYSCOLORCHANGE
       pts->hdcDesktop = NULL;
#endif
   }

}

#if !defined(NO_IME)
//+-------------------------------------------------------------------------
//  CancelUndeterminedIMEString(HWND hwnd)
//
//  Synopsis:   Cancel Undetremined IME Compositon String when Close Dialog
//
//  Parameter   hwndDlg[IN] Handle of Dialogue BOX
//
//+-------------------------------------------------------------------------

void CancelUndeterminedIMEString(HWND hwndDlg)
{
    HIMC hIMC = NULL;
    HWND hwnd;

    hwnd = GetWindow(hwndDlg,GW_OWNER);
    if (!hwnd) // Need valid hwnd to call IME API.
    {
        Assert(0 && "Rename Dialog: Could not get hwnd");
    }
    Verify(hIMC = ImmGetContext(hwnd));
    ImmNotifyIME(hIMC,NI_COMPOSITIONSTR,CPS_CANCEL,0); /*Call Ime Fuction to Cancel Undertermined*/
    if (hIMC)
        ImmReleaseContext(hwnd, hIMC);

}

#endif // NO_IME

//+-------------------------------------------------------------------------
//
//  Function:   GetNumberOfSize
//              SetNumberOfSize
//
//  Synopsis:   Helpers to get/set an integer value of given byte size
//              by dereferencing a pointer
//
//              pv - pointer to dereference
//              cb - size (1, 2 or 4)
//
//--------------------------------------------------------------------------

long
GetNumberOfSize (void * pv, int cb)
{
    switch(cb)
    {
        case 1:
            return *(BYTE*) pv;

        case 2:
            return *(SHORT*) pv;

        case 4:
            return *(LONG*) pv;

        default:
            Assert(FALSE);
            return 0;
    }
}

void
SetNumberOfSize (void * pv, int cb, long i)
{
    switch(cb)
    {
        case 1:
            Assert((char)i >= SCHAR_MIN && (char)i <= SCHAR_MAX);
            * (BYTE*) pv = BYTE(i);
            break;

        case 2:
            Assert(i >= SHRT_MIN && i <= SHRT_MAX);
            * (SHORT*) pv  = SHORT(i);
            break;

        case 4:
            * (LONG*) pv = i;
            break;

        default:
            Assert(FALSE);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   GetNumberOfType
//              SetNumberOfType
//
//  Synopsis:   Helpers to get/set an integer value of given variant type
//              by dereferencing a pointer
//
//              pv - pointer to dereference
//              vt - variant type
//
//--------------------------------------------------------------------------

// BUGBUG: The VC 5 compiler flags this as an error because VT_BOOL4 is not
// a valid VARENUM value.  Disable the warning for now.
#pragma warning(disable:4063)       // case '254' is not a valid value for switch of enum 'VARENUM'

long
GetNumberOfType (void * pv, VARENUM vt)
{
    switch(vt)
    {
        case VT_I2:
        case VT_BOOL:
            return * (SHORT*) pv;

        case VT_I4:
        case VT_BOOL4:
            return * (LONG*) pv;

        default:
            Assert(FALSE);
            return 0;
    }
}

void
SetNumberOfType (void * pv, VARENUM vt, long l)
{
    switch(vt)
    {
        case VT_BOOL:
            l = l ? VB_TRUE : VB_FALSE;
            //  vvvvvvvvvvv  FALL THROUGH vvvvvvvvvvvvv

        case VT_I2:
            Assert(l >= SHRT_MIN && l <= SHRT_MAX);
            * (SHORT*) pv = SHORT(l);
            break;

        case VT_BOOL4:
            l = l ? VB_TRUE : VB_FALSE;
            //  vvvvvvvvvvv  FALL THROUGH vvvvvvvvvvvvv

        case VT_I4:
            * (LONG_UNALIGNED *) pv = l;
            break;

        default:
            Assert(FALSE);
    }
}





//+---------------------------------------------------------------------------
//
//  Function:   TextConvert
//
//  Synopsis:   Converts the text for a multiline/singleline textbox
//              Parameters:
//                  pszTextIn: current/passed in text
//                  pBstrOut: out parameter allocated in method
//                          will only be used in cased of Glyph->CR/LF/FF
//                          otherwise, conversion in place
//                  fToGlyph: force conversion to glyph case (used
//                      when multiline property is changed
//                  returns S_FALSE when no conversion happened
//
//----------------------------------------------------------------------------
#pragma warning(disable:4706)   // assignment within conditional expression
HRESULT
TextConvert(LPTSTR pszTextIn, BSTR *pBstrOut, BOOL fToGlyph)
{

    HRESULT hr = S_FALSE;

    LPTSTR  pszWrite;
    BOOL    fCRAndLF;
    const   int k_GlyphChar = 182;
    int     iHowManyGlyphs=0;

    if (!pszTextIn)
        goto Cleanup;

    if (fToGlyph)
    {
        TCHAR   ch;

        // so we are multiline and need to convert to the glyph representation
        pszWrite = pszTextIn;
        while ((ch = *pszTextIn))
        {
            if (ch != '\n' && ch != '\r' && ch != '\f')
            {
                *pszWrite++ = *pszTextIn++;
                continue;
            }

            fCRAndLF = (ch) == '\n';
            hr = S_OK;
            *pszWrite++ = k_GlyphChar;
            pszTextIn++;

            // NOTE: If we encountered a <FF> then map the <FF> to a <CR/LF> and
            // continue.  If this isn't done we could get into a nasty condition
            // if a <CR> or <LF> followed the <FF> then we would treat the
            // <FF><LF> as one CR/LF combination.  We don't want to do that the
            // <FF> by itself is one CR/LF combination.
            if (ch == '\f')
                continue;

            // the following part of the code tries to cover all the cases
            // of multiple presentations of CR/LF, like CR/LF, LF/CR, or
            // just LF or just CR. This is done by advancing the input pointer
            // and testing the next character to be the companion.
            if (*pszTextIn)
            {
                if (*pszTextIn == '\n' && !fCRAndLF)
                {
                    pszTextIn++;
                }
                else if (*pszTextIn == '\r' && fCRAndLF)
                {
                    pszTextIn++;
                }
            }

        }
        while (pszWrite != pszTextIn)
        {
            *pszWrite++ = 0;
        }
    }
    else if (pszTextIn)
    {
#ifdef _MAC
        CStr str;
        int  cStr;
#endif
        // scan the text first to check if memory allocation
        // and conversion is needed.Count the number of glyphs
        // to make a smart allocation
        pszWrite = pszTextIn;
        while (*pszWrite)
        {
            if (*pszWrite==k_GlyphChar)
            {
                iHowManyGlyphs++;
            }
            pszWrite++;
        }

        if (!iHowManyGlyphs)
            goto Cleanup;

        // so we are singleline and need to get rid of the glyphs...
        // first allocate a new and bigger buffer
#ifdef _MAC
        *pBstrOut = NULL;

        cStr = _tcslen(pszTextIn) + iHowManyGlyphs;
        str.ReAlloc(cStr);
        pszWrite = (LPTSTR)str;
#else
        *pBstrOut = SysAllocStringLen(0, (_tcslen(pszTextIn)+iHowManyGlyphs));
        pszWrite = (TCHAR*)*pBstrOut;
#endif

        if (!pszWrite)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        while (*pszTextIn)
        {
            if (*pszTextIn == k_GlyphChar)
            {
                hr = S_OK;
                *pszWrite++ = '\r';
                *pszWrite++ = '\n';
                pszTextIn++;
            }
            else
            {
                *pszWrite++ = *pszTextIn++;
            }
        }
        *pszWrite = 0;

#ifdef _MAC
        str.SetLengthNoAlloc(cStr);
        hr = str.AllocBSTR(pBstrOut);
#endif
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}
#pragma warning(default:4706)   // assignment within conditional expression


// Coerce pArgFrom into this instance from anyvariant to a given type
HRESULT CVariant::CoerceVariantArg ( VARIANT *pArgFrom, WORD wCoerceToType)
{
    HRESULT hr = S_OK;
    VARIANT *pvar;

    if( V_VT(pArgFrom) == (VT_BYREF | VT_VARIANT) )
        pvar = V_VARIANTREF(pArgFrom);
    else
        pvar = pArgFrom;

    if ( !(pvar->vt == VT_EMPTY || pvar->vt == VT_ERROR ) )
    {
        hr = THR(VariantChangeTypeSpecial ( (VARIANT *)this, pvar,  wCoerceToType ));
    }
    else
    {
        return S_FALSE;
    }
    RRETURN(hr);
}

// Coerce current variant into itself
HRESULT CVariant::CoerceVariantArg (WORD wCoerceToType)
{
    HRESULT hr = S_OK;

    if ( !(vt == VT_EMPTY || vt == VT_ERROR ) )
    {
        hr = THR(VariantChangeTypeSpecial ( (VARIANT *)this, (VARIANT *)this, wCoerceToType ));
    }
    else
    {
        return S_FALSE;
    }
    RRETURN(hr);
}


// Coerce any numeric (VT_I* or  VT_UI*) into a VT_I4 in this instance
BOOL CVariant::CoerceNumericToI4 ()
{
    switch (vt)
    {
    case VT_I1:
    case VT_UI1:
        lVal = 0x000000FF & (DWORD)bVal;
        break;
    
    case VT_UI2:
    case VT_I2:
        lVal = 0x0000FFFF & (DWORD)iVal;
        break;
    
    case VT_UI4:
    case VT_I4:
    case VT_INT: 
    case VT_UINT:
        break;

    case VT_R8:
        lVal = (LONG)dblVal;
        break;

    case VT_R4:
        lVal = (LONG)fltVal;
        break;

    default:
        return FALSE;
    }

    vt = VT_I4;
    return TRUE;
}
//+------------------------------------------------------------------------
//
//  Binary search routine
//      pb - Pointer to array
//      c  - Number of entries in the array
//      l  - Value for which to search (expressed as a long)
//      cb - Size of an array entry (defaults to 4 bytes)
//      ob - Offset within entry of comparison value (defaults to 0)
//
//-------------------------------------------------------------------------
int BSearch(const BYTE * pb, const int c, const unsigned long l, const int cb,
            const int ob)
{
    int i = 0;

    if (c)
    {
        int iLow  = 0;
        int iHigh = c;

        while (iLow < iHigh)
        {
            i = (iLow + iHigh) >> 1;

            if ((*((unsigned long *)(pb + (cb * i) + ob))) < l)
                iLow = i + 1;

            else
                iHigh = i;
        }

        i = iLow;
    }

    return i;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetBStrFromStream
//
//  Synopsis:   Given a stream, this function allocates and returns a bstr
//              representing its contents.
//
//----------------------------------------------------------------------------

HRESULT
GetBStrFromStream(IStream * pIStream, BSTR * pbstr, BOOL fStripTrailingCRLF)
{
    HRESULT  hr;
    HGLOBAL  hHtmlText = 0;
    TCHAR *  pstrWide = NULL;

    *pbstr = NULL;
    
    hr = THR(GetHGlobalFromStream(pIStream, &hHtmlText));
    if (hr)
        goto Cleanup;

    pstrWide = (TCHAR *) GlobalLock( hHtmlText );

    if (!pstrWide)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (fStripTrailingCRLF)
    {
        // Remove trailing cr/lf's
        
        TCHAR * pstr = pstrWide + _tcslen(pstrWide);

        while (pstr-- > pstrWide && (*pstr == '\r' || *pstr == '\n'))
            ;

        *(pstr + 1) = 0;
    }
                
    hr = FormsAllocString(pstrWide, pbstr);

    GlobalUnlock(hHtmlText);
    
Cleanup:
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsRectInRegion
//
//  Synopsis:   Same as RectInRegion, except clips the passed in rect to
//              16-bit coordinates on Win95 only.
//
//----------------------------------------------------------------------------

BOOL
FormsRectInRegion(HRGN hrgn, RECT * prc)
{
    // BUGBUG (make this a wrapper?)
    
    //
    // NT can handle the truth about 32-bit coordinates.
    //
    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
        return RectInRegion(hrgn, prc);

    //
    // Clip the coordinates to the 16-bit world for Win95.
    //
    RECT rc = { SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX };

    IntersectRect(&rc, prc, &rc);
    return RectInRegion(hrgn, &rc);
}

BOOL MakeThisTopBrowserInProcess(HWND hwnd)
{
    HWND hwndThisBrowser = NULL;
    HWND hwndTopMostBrowser = NULL;
    DWORD pid, thisPid = 0;
    TCHAR achClassName[126];

    // return if a window in the called thread has the keyboard focus
    if (::GetFocus() != NULL)
        return FALSE;

    // get top-level browser window for which the alert was called
    while(hwnd)
    {
        hwndThisBrowser = hwnd;
        hwnd = ::GetParent(hwnd);
    }

    // if found, get the pid of this one and the next top-level window above this one
    if (hwndThisBrowser)
    {
        hwnd = ::GetNextWindow(hwndThisBrowser, GW_HWNDPREV);
        ::GetWindowThreadProcessId(hwndThisBrowser, &thisPid);
    }

    // see if any of the top-level windows above this one are browser windows
    while (hwnd && thisPid)
    {
        ::GetWindowThreadProcessId(hwnd, &pid);
        // if the pids are same then it could be a browser window
        if (pid == thisPid)
        {
            ::GetClassName(hwnd, achClassName, ARRAY_SIZE(achClassName));
            // IEFrame is the classname for normally launched browsers and CabinetWClass is the
            // classname for browsers launched with window.open(). Check for both!
            if(!_tcscmp(achClassName, _T("IEFrame")) || !_tcscmp(achClassName, _T("CabinetWClass")))
                hwndTopMostBrowser = hwnd;
        }

        hwnd = ::GetNextWindow(hwnd, GW_HWNDPREV);
    }

    // if top browser window found, get the next top-level window just above it in
    // the z-order, so that this browser window can be inserted just before it (so
    // that it now becomes the top browser window).
    if (hwndTopMostBrowser)
    {
        // if there is a top browser above us and it is the currently active window, then
        // make ourselves active instead
        if (::GetForegroundWindow() == hwndTopMostBrowser)
        {
            ::SetForegroundWindow(hwndThisBrowser);
            return FALSE;
        }
        else    // else, just insert ourselves above the top browser, w/o activating
        {
            hwndTopMostBrowser = ::GetNextWindow(hwndTopMostBrowser, GW_HWNDPREV);
            if (hwndTopMostBrowser)
            {
                ::SetWindowPos(hwndThisBrowser, hwndTopMostBrowser, 0, 0, 0, 0,
                               SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            }
        }
    }

    return TRUE;
}


#ifndef LANG_TIBETAN
#define LANG_TIBETAN     0x51
#endif
#ifndef LANG_LAO
#define LANG_LAO         0x54
#endif

void GetSystemNumberSettings(
    NUMSHAPE * piNumShape,
    DWORD * plangNationalDigits)
{
    NUMSHAPE iNumShape = NUMSHAPE_NONE;
    DWORD langDigits = LANG_NEUTRAL;
    HKEY hkey = NULL;
    DWORD dwType;
    DWORD cbData;
    char achBufferData[41]; // Max Size user can fit in our variables edit field
    WCHAR achDigits[16];

    Assert(piNumShape != NULL && plangNationalDigits != NULL);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\International"), 0L, KEY_READ, &hkey))
    {
        goto Cleanup;
    }

    Assert(hkey != NULL);

    cbData = sizeof(achBufferData);
    if (RegQueryValueExA(hkey, "NumShape",
        0L, &dwType, (LPBYTE) achBufferData, &cbData) == ERROR_SUCCESS &&
        achBufferData[0] != TEXT('\0') && (dwType & REG_SZ) && !(dwType & REG_NONE))
    {
        iNumShape = (NUMSHAPE) max(min(atoi(achBufferData), (int) NUMSHAPE_NATIVE), (int) NUMSHAPE_CONTEXT);
    }

    RegCloseKey(hkey);

    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SNATIVEDIGITS, achDigits, 16))
    {
        switch (achDigits[4])
        {
        case 0x664: // Arabic-Indic digits
            langDigits = LANG_ARABIC;
            break;
        case 0x6F4: // Eastern Arabic-Indic digits
            langDigits = LANG_FARSI;
            break;
        case 0x96A: // Devanagari digits
            langDigits = LANG_HINDI;
            break;
        case 0x9EA: // Bengali digits
            langDigits = LANG_BENGALI;
            break;
        case 0xA6A: // Gurmukhi digits
            langDigits = LANG_PUNJABI;
            break;
        case 0xAEA: // Gujarati digits
            langDigits = LANG_GUJARATI;
            break;
        case 0xB6A: // Oriya digits
            langDigits = LANG_ORIYA;
            break;
        case 0xBEA: // Tamil digits
            langDigits = LANG_TAMIL;
            break;
        case 0xC6A: // Telugu digits
            langDigits = LANG_TELUGU;
            break;
        case 0xCEA: // Kannada digits
            langDigits = LANG_KANNADA;
            break;
        case 0xD6A: // Malayalam digits
            langDigits = LANG_MALAYALAM;
            break;
        case 0xE54: // Thai digits
            langDigits = LANG_THAI;
            break;
        case 0xED4: // Lao digits
            langDigits = LANG_LAO;
            break;
        case 0xF24: // Tibetan digits
            langDigits = LANG_TIBETAN;
            break;
        default:
            langDigits = LANG_NEUTRAL;
            break;
        }
    }
    else
    {
        // Work from the platform's locale.
        langDigits = PRIMARYLANGID(GetUserDefaultLangID());
    }
    if (langDigits != LANG_ARABIC &&
        langDigits != LANG_FARSI &&
        langDigits != LANG_HINDI &&
        langDigits != LANG_BENGALI &&
        langDigits != LANG_PUNJABI &&
        langDigits != LANG_GUJARATI &&
        langDigits != LANG_ORIYA &&
        langDigits != LANG_TAMIL &&
        langDigits != LANG_TELUGU &&
        langDigits != LANG_KANNADA &&
        langDigits != LANG_MALAYALAM &&
        langDigits != LANG_THAI &&
        langDigits != LANG_LAO &&
        langDigits != LANG_TIBETAN)
    {
        langDigits = LANG_NEUTRAL;
        iNumShape = NUMSHAPE_NONE;
    }

Cleanup:
    *piNumShape = iNumShape;
    *plangNationalDigits = langDigits;
}

HINSTANCE EnsureMLLoadLibrary()
{
#ifndef UNIX
    HINSTANCE hInst = MLLoadLibrary(_T("shdoclc.dll"), g_hInstCore, ML_CROSSCODEPAGE);
#else
    HINSTANCE hInst = MLLoadLibrary(_T("shdocvw.dll"), g_hInstCore, ML_CROSSCODEPAGE);
#endif
    if (hInst)
    {
        LOCK_GLOBALS;

        if (!g_hInstResource)
        {
            g_hInstResource = hInst;
            hInst = NULL;
        }
    }

    if (hInst)
        MLFreeLibrary(hInst);

    Assert(g_hInstResource && "Resource DLL is not loaded!");

    return g_hInstResource;
}
