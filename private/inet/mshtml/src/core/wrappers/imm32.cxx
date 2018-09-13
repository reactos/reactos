//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       imm32.cxx
//
//  Contents:   Dynamic wrappers for imm32.dll functions
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_IMM_H_
#define X_IMM_H_
#include "imm.h"
#endif

#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif

#ifndef NO_IME

DYNLIB g_dynlibIMM32 = { NULL, NULL, "IMM32.DLL" };

extern BOOL HasActiveIMM();
extern IActiveIMMApp * GetActiveIMM();

//----------------------------------------------------------------------------
// ActiveIMM wrappers
//----------------------------------------------------------------------------

HIMC ImmAssociateContextDIMM(HWND hWnd, HIMC hIMC)
{
    Assert(HasActiveIMM());

    HIMC hPrev;

    GetActiveIMM()->AssociateContext(hWnd, hIMC, &hPrev);

    return hPrev;
}

LRESULT ImmEscapeADIMM(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData)
{
    Assert(HasActiveIMM());

    LRESULT lResult;

    GetActiveIMM()->EscapeA(hKL, hIMC, uEscape, lpData, &lResult);

    return lResult;
}

LRESULT ImmEscapeWDIMM(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData)
{
    Assert(HasActiveIMM());

    LRESULT lResult;

    GetActiveIMM()->EscapeW(hKL, hIMC, uEscape, lpData, &lResult);

    return lResult;
}

BOOL ImmGetCandidateWindowDIMM(HIMC hIMC, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->GetCandidateWindow(hIMC, dwBufLen, lpCandidate));
}

LONG ImmGetCompositionStringADIMM(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    Assert(HasActiveIMM());
    
    LONG lCopied;

    GetActiveIMM()->GetCompositionStringA(hIMC, dwIndex, dwBufLen, &lCopied, lpBuf);

    return lCopied;
}

LONG ImmGetCompositionStringWDIMM(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    Assert(HasActiveIMM());
    
    LONG lCopied;

    GetActiveIMM()->GetCompositionStringW(hIMC, dwIndex, dwBufLen, &lCopied, lpBuf);

    return lCopied;
}

HIMC ImmGetContextDIMM(HWND hWnd)
{
    Assert(HasActiveIMM());

    HIMC hIMC;

    GetActiveIMM()->GetContext(hWnd, &hIMC);

    return hIMC;
}

BOOL ImmGetConversionStatusDIMM(HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence));
}

BOOL ImmGetOpenStatusDIMM(HIMC hIMC)
{
    Assert(HasActiveIMM());

    return (GetActiveIMM()->GetOpenStatus(hIMC) == S_OK);
}

BOOL ImmSetOpenStatusDIMM(HIMC hIMC, BOOL fOpen)
{
    Assert(HasActiveIMM());

    return (GetActiveIMM()->SetOpenStatus(hIMC, fOpen) == S_OK);
}

DWORD ImmGetPropertyDIMM(HKL hKL, DWORD dwFlags)
{
    Assert(HasActiveIMM());

    DWORD dwProperty;

    GetActiveIMM()->GetProperty(hKL, dwFlags, &dwProperty);

    return dwProperty;
}

UINT ImmGetVirtualKeyDIMM(HWND hWnd)
{
    Assert(HasActiveIMM());

    UINT uVirtualKey;

    GetActiveIMM()->GetVirtualKey(hWnd, &uVirtualKey);

    return uVirtualKey;
}

BOOL ImmNotifyIMEDIMM(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->NotifyIME(hIMC, dwAction, dwIndex, dwValue));
}

BOOL ImmReleaseContextDIMM(HWND hWnd, HIMC hIMC)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->ReleaseContext(hWnd, hIMC));
}

BOOL ImmSetCandidateWindowDIMM(HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->SetCandidateWindow(hIMC, lpCandidate));
}

BOOL ImmSetCompositionFontADIMM(HIMC hIMC, LPLOGFONTA lplf)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->SetCompositionFontA(hIMC, lplf));
}

BOOL ImmSetCompositionWindowDIMM(HIMC hIMC, LPCOMPOSITIONFORM lpcf)
{
    Assert(HasActiveIMM());

    return SUCCEEDED(GetActiveIMM()->SetCompositionWindow(hIMC, lpcf));
}

BOOL ImmIsIMEDIMM(HKL hklCurrent)
{
    if(HasActiveIMM())
    {
        return GetActiveIMM()->IsIME(hklCurrent);
    }
    return FALSE;
}

//----------------------------------------------------------------------------
// Active wrapper functions
//----------------------------------------------------------------------------

#define WRAPIT(fn, rettype, errret, a1, a2)\
rettype WINAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibIMM32, #fn };\
    \
    if (HasActiveIMM()) \
        return fn##DIMM a2; \
    \
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        return errret;\
    return (*(rettype (WINAPI *) a1)s_dynproc##fn.pfn) a2;\
}

WRAPIT(ImmAssociateContext,
       HIMC,
       NULL,
       (HWND hwnd, HIMC himc),
       (hwnd, himc))

WRAPIT(ImmEscapeA,
       LRESULT,
       0,
       (HKL hkl, HIMC himc, UINT uEscape, LPVOID lpData),
       (hkl, himc, uEscape, lpData))

WRAPIT(ImmEscapeW,
       LRESULT,
       0,
       (HKL hkl, HIMC himc, UINT uEscape, LPVOID lpData),
       (hkl, himc, uEscape, lpData))

WRAPIT(ImmGetCandidateWindow,
       BOOL,
       0,
       (HIMC himc, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate),
       (himc, dwBufLen, lpCandidate))

WRAPIT(ImmGetCompositionStringA,
       LONG,
       IMM_ERROR_GENERAL,
       (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen),
       (hIMC, dwIndex, lpBuf, dwBufLen))

WRAPIT(ImmGetCompositionStringW,
       LONG,
       IMM_ERROR_GENERAL,
       (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen),
       (hIMC, dwIndex, lpBuf, dwBufLen))

WRAPIT(ImmGetContext,
       HIMC,
       NULL,
       (HWND hwnd),
       (hwnd))

WRAPIT(ImmGetConversionStatus,
       BOOL,
       0,
       (HIMC himc, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence),
       (himc, lpfdwConversion, lpfdwSentence))

WRAPIT(ImmGetOpenStatus,
       BOOL,
       FALSE,
       (HIMC himc),
       (himc))

WRAPIT(ImmSetOpenStatus,
       BOOL,
       FALSE,
       (HIMC himc, BOOL fOpen),
       (himc, fOpen))

WRAPIT(ImmGetProperty,
       DWORD,
       0,
       (HKL hkl, DWORD dwFlags),
       (hkl, dwFlags))

WRAPIT(ImmGetVirtualKey,
       UINT,
       0,
       (HWND hwnd),
       (hwnd))

WRAPIT(ImmNotifyIME,
       BOOL,
       0,
       (HIMC himc, DWORD dwAction, DWORD dwIndex, DWORD dwValue),
       (himc, dwAction, dwIndex, dwValue))

WRAPIT(ImmReleaseContext,
       BOOL,
       0,
       (HWND hwnd, HIMC himc),
       (hwnd, himc))

WRAPIT(ImmSetCandidateWindow,
       BOOL,
       0,
       (HIMC himc, LPCANDIDATEFORM lpCandidate),
       (himc, lpCandidate))

WRAPIT(ImmSetCompositionFontA,
       BOOL,
       0,
       (HIMC himc, LPLOGFONTA lplf),
       (himc, lplf))

WRAPIT(ImmSetCompositionWindow,
       BOOL,
       0,
       (HIMC himc, LPCOMPOSITIONFORM lpcf),
       (himc, lpcf))

WRAPIT(ImmIsIME,
       BOOL,
       FALSE,
       (HKL hklCurrent),
       (hklCurrent))

//----------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------

HRESULT GetKeyboardCodePageDIMM(UINT *puCodePage)
{
    HRESULT hr;

    if (!HasActiveIMM())
    {
        UINT ConvertLanguageIDtoCodePage(WORD langid);

        *puCodePage = ConvertLanguageIDtoCodePage((WORD)(DWORD_PTR)GetKeyboardLayout(0 /*idThread*/));
        hr = S_FALSE;
    }
    else
    {
        hr = THR( GetActiveIMM()->GetCodePageA(GetKeyboardLayout(0), puCodePage) );
    }

    RRETURN1(hr,S_FALSE);
}

//
// Actually returns an LCID with SortID == 0, matching the behavior
// of GetKeyboardLCID.
//

HRESULT GetKeyboardLCIDDIMM(LCID *plid)
{
    HKL hKL = GetKeyboardLayout(0 /*idThread*/);

    if (HasActiveIMM())
    {
        HRESULT hr;
        LANGID langid;

        hr = THR( GetActiveIMM()->GetLangId(hKL, &langid) );
        if (OK(hr))
        {
            *plid = LCID(langid);
            return S_OK;
        }
    }

    *plid = (WORD)(DWORD_PTR)hKL;

    return S_FALSE;
}

#endif // ndef NO_IME

static const WORD s_CodePageTable[] =
{
// CodePage       PLID  primary language
// -------------------------------------
       0,       // 00 - undefined
    1256,       // 01 - Arabic
    1251,       // 02 - Bulgarian
    1252,       // 03 - Catalan
     950,       // 04 - Taiwan, Hong Kong (PRC and Singapore are 936)
    1250,       // 05 - Czech
    1252,       // 06 - Danish
    1252,       // 07 - German
    1253,       // 08 - Greek
    1252,       // 09 - English
    1252,       // 0a - Spanish
    1252,       // 0b - Finnish
    1252,       // 0c - French
    1255,       // 0d - Hebrew
    1250,       // 0e - Hungarian
    1252,       // 0f - Icelandic
    1252,       // 10 - Italian
     932,       // 11 - Japan
     949,       // 12 - Korea
    1252,       // 13 - Dutch
    1252,       // 14 - Norwegian
    1250,       // 15 - Polish
    1252,       // 16 - Portuguese
       0,       // 17 - Rhaeto-Romanic
    1250,       // 18 - Romanian
    1251,       // 19 - Russian
    1250,       // 1a - Croatian
    1250,       // 1b - Slovak
    1250,       // 1c - Albanian
    1252,       // 1d - Swedish
     874,       // 1e - Thai
    1254,       // 1f - Turkish
       0,       // 20 - Urdu
    1252,       // 21 - Indonesian
    1251,       // 22 - Ukranian
    1251,       // 23 - Byelorussian
    1250,       // 24 - Slovenian
    1257,       // 25 - Estonia
    1257,       // 26 - Latvian
    1257,       // 27 - Lithuanian
       0,       // 28 - undefined
    1256,       // 29 - Farsi
    1258,       // 2a - Vietnanese
       0,       // 2b - undefined
       0,       // 2c - undefined
    1252,       // 2d - Basque
       0,       // 2e - Sorbian
    1251,       // 2f - Macedonian
    1252,       // 30 - Sutu        *** use 1252 for the following ***
    1252,       // 31 - Tsonga
    1252,       // 32 - Tswana
    1252,       // 33 - Venda
    1252,       // 34 - Xhosa
    1252,       // 35 - Zulu
    1252,       // 36 - Africaans (uses 1252)
    1252,       // 38 - Faerose
    1252,       // 39 - Hindi
    1252,       // 3a - Maltese
    1252,       // 3b - Sami
    1252,       // 3c - Gaelic
    1252,       // 3e - Malaysian
       0,       // 3f -
       0,       // 40 -
    1252,       // 41 - Swahili
       0,       // 42 - 
       0,       // 43 - 
       0,       // 44 - 
    1252,       // 45 - Bengali *** The following languages use 1252, but
    1252,       // 46 - Gurmuki     actually have only Unicode characters ***
    1252,       // 47 - Gujarait
    1252,       // 48 - Oriya
    1252,       // 49 - Tamil
    1252,       // 4a - Telugu
    1252,       // 4b - Kannada
    1252,       // 4c - Malayalam
       0,       // 4d - 
       0,       // 4e - 
       0,       // 4f - 
    1252,       // 50 - Mongolian
    1252,       // 51 - Tibetan
       0,       // 52 - 
    1252,       // 53 - Khmer
    1252,       // 54 - Lao
    1252,       // 55 - Burmese
       0,       // 56 - LANG_MAX
};

#define ns_CodePageTable  (sizeof(s_CodePageTable)/sizeof(s_CodePageTable[0]))

#if !defined(lidSerbianCyrillic)
  #define lidSerbianCyrillic 0xc1a
#else
  #if lidSerbianCyrillic != 0xc1a
    #error "lidSerbianCyrillic macro value has changed"
  #endif // lidSerbianCyrillic
#endif

/*
 *  ConvertLanguageIDtoCodePage (lid)
 *
 *  @mfunc      Maps a language ID to a Code Page
 *
 *  @rdesc      returns Code Page
 *
 *  @devnote:
 *      This routine takes advantage of the fact that except for Chinese,
 *      the code page is determined uniquely by the primary language ID,
 *      which is given by the low-order 10 bits of the lcid.
 *
 *      The WORD s_CodePageTable could be replaced by a BYTE with the addition
 *      of a couple of if's and the BYTE table replaced by a nibble table
 *      with the addition of a shift and a mask.  Since the table is only
 *      92 bytes long, it seems that the simplicity of using actual code page
 *      values is worth the extra bytes.
 */

UINT
ConvertLanguageIDtoCodePage(WORD langid)
{
    WORD langidT = PRIMARYLANGID(langid);       // langidT = primary language (Plangid)
    CODEPAGE cp;

#if !defined(WIN16) && !defined(UNIX)
    if(langidT >= LANG_CROATIAN)                // Plangid = 0x1a
    {
        if(langid == lidSerbianCyrillic)        // Special case for langid = 0xc1a
            return 1251;                        // Use Cyrillic code page

        if(langidT >= ns_CodePageTable)         // Africans Plangid = 0x36, which
            return CP_ACP;                      //  is outside table
    }
#endif // !WIN16 && !UNIX

    cp = s_CodePageTable[langidT];              // Translate Plangid to code page

    if(cp != 950 || (langid & 0x400))           // All but Singapore, PRC, and Serbian
        return cp != 0 ? cp : CP_ACP;           // Remember there are holes in the array
                                                // that may not always be there.

    return 936;                                 // Singapore and PRC
}

/*
 *  GetKeyboardCodePage ()
 *
 *  @mfunc      Gets Code Page for keyboard active on current thread
 *
 *  @rdesc      returns Code Page
 */

UINT GetKeyboardCodePage()
{
#if !defined(MACPORT) && !defined(WIN16) && !defined(WINCE)
  #ifndef NO_IME
    UINT uCodePage;
    return SUCCEEDED(GetKeyboardCodePageDIMM(&uCodePage)) ? uCodePage : GetACP();
  #else
    return ConvertLanguageIDtoCodePage((WORD)(DWORD)GetKeyboardLayout(0 /*idThread*/));
  #endif  
#else
    return ConvertLanguageIDtoCodePage(GetUserDefaultLCID());
#endif
}
