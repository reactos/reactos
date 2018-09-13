/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    font.c

Abstract:

    This module implements the Win32 font dialog.

Revision History:

--*/

// ClaudeBe 10/12/98, to enable Multiple Master Axis selection in the
// font selection dialog, uncomment the following three lines and make sure
// you use the new font.dlg (from \nt\private\ntos\w32\ntgdi\test\fonttest.nt\comdlg32mm)
// as of 10/12/98, this is not a NT 5.0/IE 5.0 feature
// 

// arulk- MM_DESIGNVECTOR is not feature for windows 2000 so dont uncomment the following lines for
// windows 2000
//#ifdef WINNT
//#define  MM_DESIGNVECTOR_DEFINED
//#endif


//
//  Include Files.
//


#include "comdlg32.h"

#ifdef WINNT
  #include <nt.h>
  #include <ntrtl.h>
  #include <nturtl.h>
#endif

#include <imm.h>

#include "font.h"
#include "cdids.h"
#include <wingdip.h>              // for IS_ANY_DBCS_CHARSET macro

#ifdef MM_DESIGNVECTOR_DEFINED
#define MAX_NUM_AXES  6

VOID GetCannonicalNameW(
    WCHAR        *pwsz,  // in foo_XX_YY, out Cannonical and capitalized name FOO
    DESIGNVECTOR *pdv      // [XX,YY] on out
);

VOID AddDesignVectorToNameW(
    WCHAR        *pFaceName,  // in Cannonical name foo, out foo_XX_YY
    DESIGNVECTOR *pdv      // [XX,YY] in
);

VOID GetCannonicalNameA(
    char        *pasz,  // in foo_XX_YY, out Cannonical and capitalized name FOO
    DESIGNVECTOR *pdv      // [XX,YY] on out
);

VOID AddDesignVectorToNameA(
    char        *pFaceName,  // in Cannonical name foo, out foo_XX_YY
    DESIGNVECTOR *pdv      // [XX,YY] in
);

#ifdef UNICODE
#define GetCannonicalName(A,B) GetCannonicalNameW(A,B);
#define AddDesignVectorToName(A,B) AddDesignVectorToNameW(A,B);
#else
#define GetCannonicalName(A,B) GetCannonicalNameA(A,B);
#define AddDesignVectorToName(A,B) AddDesignVectorToNameA(A,B);
#endif // UNICODE

#ifdef UNICODE

#define IS_DIGITW(x)   (((x) >= L'0') && ((x) <= L'9'))
#define GET_DIGITW(X)  ((X) - L'0')
#define WC_UNDERSCORE L'_'

VOID GetCannonicalNameW(
    WCHAR        *pwsz,  // in foo_XX_YY, out Cannonical and capitalized name FOO
    DESIGNVECTOR *pdv      // [XX,YY] on out
)
{
// modified from GreGetCannonicalName
//
// The input is the zero terminated name of the form
//
// foo_XXaaaYYbbb...ZZccc
//
// where  XX,  YY,  ZZ are numerals (arbitrary number of them) and
//       aaa, bbb, ccc are not numerals, i.e. spaces, or another '_' signs or
// letters with abbreviated axes names.
//
// This face name will be considered equivalent to face name foo
// with DESIGNVECTOR [XX,YY, ...ZZ], number of axes being determined
// by number of numeral sequences.
//

    WCHAR *pwc;
    ULONG cAxes = 0;
    ULONG cwc;

    /* if we already have a DESIGNVECTOR information, we don't want to erase it */
    if (pdv->dvNumAxes != 0)
        return;

    for
    (
        pwc = pwsz ;
        (*pwc) && !((*pwc == WC_UNDERSCORE) && IS_DIGITW(pwc[1]));
        pwc++
    )
    {
        // do nothing;
    }

// copy out, zero terminate

// Sundown safe truncation
    cwc = (ULONG)(pwc - pwsz);

// If we found at least one WC_UNDERSCORE followed by the DIGIT
// we have to compute DV. Underscore followed by the DIGIT is Adobe's rule

    if ((*pwc == WC_UNDERSCORE) && IS_DIGITW(pwc[1]))
    {
    // step to the next character behind undescore

        pwc++;

        while (*pwc)
        {
        // go until you hit the first digit

            for ( ; *pwc && !IS_DIGITW(*pwc) ; pwc++)
            {
                // do nothing
            }


            if (*pwc)
            {
            // we have just hit the digit

                ULONG dvValue = GET_DIGITW(*pwc);

            // go until you hit first nondigit or the terminator

                pwc++;

                for ( ; *pwc && IS_DIGITW(*pwc); pwc++)
                {
                    dvValue = dvValue * 10 + GET_DIGITW(*pwc);
                }

                pdv->dvValues[cAxes] = (LONG)dvValue;

            // we have just parsed a string of numerals

                cAxes++;
            }
        }
    }

// record the findings

    pdv->dvNumAxes = cAxes;
    pdv->dvReserved = STAMP_DESIGNVECTOR;

    /* we do that at the end allowing to pass the same buffer as in and out */
    pwsz[cwc] = L'\0';

}

VOID AddDesignVectorToNameW(
    WCHAR        *pFaceName,  // in Cannonical name foo, out foo_XX_YY
    DESIGNVECTOR *pdv      // [XX,YY] in
)
{
    if (pdv->dvNumAxes != 0)
    {
        UINT i;
        PWCHAR endOfBaseName;
        WCHAR pszValue[20]; // temp buffer used to convert long to string

        for (i=0; i<pdv->dvNumAxes; i++)
        {
                    _itow(pdv->dvValues[i], pszValue, 10);

                    if ((wcslen(pFaceName) + wcslen(pszValue) + 1) >= (UINT)LF_FACESIZE)
                            return;

            endOfBaseName = pFaceName + wcslen(pFaceName);
                        *endOfBaseName = WC_UNDERSCORE;
                    endOfBaseName++;
                    *endOfBaseName = 0; 
                    wcscat(endOfBaseName, pszValue);
        }

    }

}
#else
#define IS_DIGITA(x)   (((x) >= '0') && ((x) <= '9'))
#define GET_DIGITA(X)  ((X) - '0')
#define AC_UNDERSCORE '_'

VOID GetCannonicalNameA(
    char        *pasz,  // in foo_XX_YY, out Cannonical and capitalized name FOO
    DESIGNVECTOR *pdv      // [XX,YY] on out
)
{
// modified from GreGetCannonicalName
//
// The input is the zero terminated name of the form
//
// foo_XXaaaYYbbb...ZZccc
//
// where  XX,  YY,  ZZ are numerals (arbitrary number of them) and
//       aaa, bbb, ccc are not numerals, i.e. spaces, or another '_' signs or
// letters with abbreviated axes names.
//
// This face name will be considered equivalent to face name foo
// with DESIGNVECTOR [XX,YY, ...ZZ], number of axes being determined
// by number of numeral sequences.
//

    char *pac;
    ULONG cAxes = 0;
    ULONG cac;

    /* if we already have a DESIGNVECTOR information, we don't want to erase it */
    if (pdv->dvNumAxes != 0)
        return;

    for
    (
        pac = pasz ;
        (*pac) && !((*pac == AC_UNDERSCORE) && IS_DIGITA(pac[1]));
        pac++
    )
    {
        // do nothing;
    }

// copy out, zero terminate

// Sundown safe truncation
    cac = (ULONG)(pac - pasz);

// If we found at least one AC_UNDERSCORE followed by the DIGIT
// we have to compute DV. Underscore followed by the DIGIT is Adobe's rule

    if ((*pac == AC_UNDERSCORE) && IS_DIGITA(pac[1]))
    {
    // step to the next character behind undescore

        pac++;

        while (*pac)
        {
        // go until you hit the first digit

            for ( ; *pac && !IS_DIGITA(*pac) ; pac++)
            {
                // do nothing
            }


            if (*pac)
            {
            // we have just hit the digit

                ULONG dvValue = GET_DIGITA(*pac);

            // go until you hit first nondigit or the terminator

                pac++;

                for ( ; *pac && IS_DIGITA(*pac); pac++)
                {
                    dvValue = dvValue * 10 + GET_DIGITA(*pac);
                }

                pdv->dvValues[cAxes] = (LONG)dvValue;

            // we have just parsed a string of numerals

                cAxes++;
            }
        }
    }

// record the findings

    pdv->dvNumAxes = cAxes;
    pdv->dvReserved = STAMP_DESIGNVECTOR;

    /* we do that at the end allowing to pass the same buffer as in and out */
    pasz[cac] = L'\0';

}

VOID AddDesignVectorToNameA(
    char        *pFaceName,  // in Cannonical name foo, out foo_XX_YY
    DESIGNVECTOR *pdv      // [XX,YY] in
)
{
    if (pdv->dvNumAxes != 0)
    {
        UINT i;
        char *endOfBaseName;
        char pszValue[20]; // temp buffer used to convert long to string

        for (i=0; i<pdv->dvNumAxes; i++)
        {
                    _itoa(pdv->dvValues[i], pszValue, 10);

                    if ((strlen(pFaceName) + strlen(pszValue) + 1) >= (UINT)LF_FACESIZE)
                            return;

            endOfBaseName = pFaceName + strlen(pFaceName);
                        *endOfBaseName = AC_UNDERSCORE;
                    endOfBaseName++;
                    *endOfBaseName = 0; 
                    strcat(endOfBaseName, pszValue);
        }

    }

}
#endif // UNICODE

#endif // MM_DESIGNVECTOR_DEFINED

BOOL IsSimplifiedChineseUI(void)
{
    BOOL bRet = FALSE;
    
    if (staticIsOS(OS_NT5))     // If NT5 or higher, we use system UI Language
    {
        static LANGID (CALLBACK* pfnGetUserDefaultUILanguage)(void) = NULL;

        if (pfnGetUserDefaultUILanguage == NULL)
        {
            HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

            if (hmod)
                pfnGetUserDefaultUILanguage = (LANGID (CALLBACK*)(void))GetProcAddress(hmod, "GetUserDefaultUILanguage");
        }
        if (pfnGetUserDefaultUILanguage)
        {
            LANGID LangID = pfnGetUserDefaultUILanguage();

            if (LangID == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
                bRet = TRUE;
        }
    }    
    else                        // If Win9x and NT4, we use CP_ACP
    {
        if (936 == GetACP())
            bRet = TRUE;
    }

    return bRet;
}

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ChooseFontA
//
//  ANSI entry point for ChooseFont when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL WINAPI ChooseFontA(
    LPCHOOSEFONTA pCFA)
{
    BOOL result;

    ENUMLOGFONTEXDVA LogFontDV;
    LPLOGFONTA lpLogFont = NULL;

    if (!pCFA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }


    if (pCFA->lpLogFont)
    {
        /* we ned space in lpLogFont for the DESIGNVECTOR */
        lpLogFont = pCFA->lpLogFont;

        pCFA->lpLogFont = (LPLOGFONTA)&LogFontDV; 

        *pCFA->lpLogFont = *lpLogFont;

        ((LPENUMLOGFONTEXDVA)pCFA->lpLogFont)->elfDesignVector.dvNumAxes = 0;  // set number of axis to zero
        ((LPENUMLOGFONTEXDVA)pCFA->lpLogFont)->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;  
    }

    result = ChooseFontExA(pCFA, 0);

    if (lpLogFont)
    {
        /* copy back lpLogFont */

        *lpLogFont = *pCFA->lpLogFont;

        pCFA->lpLogFont = lpLogFont;
    }

    return result;
}
#endif // MM_DESIGNVECTOR_DEFINED

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL WINAPI ChooseFontExA(
    LPCHOOSEFONTA pCFA, DWORD fl)
#else
BOOL WINAPI ChooseFontA(
    LPCHOOSEFONTA pCFA)
#endif // MM_DESIGNVECTOR_DEFINED
{
    LPCHOOSEFONTW pCFW;
    BOOL result;
    LPBYTE pStrMem;
    UNICODE_STRING usStyle;
    ANSI_STRING asStyle;
    int cchTemplateName = 0;
    FONTINFO FI;

    ZeroMemory(&FI, sizeof(FONTINFO));

    if (!pCFA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pCFA->lStructSize != sizeof(CHOOSEFONTA))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    //
    //  Setup and allocate CHOOSEFONTW structure.
    //
    if (!pCFA->lpLogFont && (pCFA->Flags & CF_INITTOLOGFONTSTRUCT))
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (!(pCFW = (LPCHOOSEFONTW)LocalAlloc(
                                LPTR,
#ifdef MM_DESIGNVECTOR_DEFINED
                                sizeof(CHOOSEFONTW) + sizeof(ENUMLOGFONTEXDVW) )))
#else
                                sizeof(CHOOSEFONTW) + sizeof(LOGFONTW) )))
#endif // MM_DESIGNVECTOR_DEFINED

    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    pCFW->lStructSize = sizeof(CHOOSEFONTW);

    pCFW->lpLogFont = (LPLOGFONTW)((LPCHOOSEFONTW)pCFW + 1);

    if (pCFA->Flags & CF_ENABLETEMPLATE)
    {
        if (!IS_INTRESOURCE(pCFA->lpTemplateName))
        {
            cchTemplateName = (lstrlenA(pCFA->lpTemplateName) + 1) *
                              sizeof(WCHAR);
            if (!(pCFW->lpTemplateName = (LPWSTR)LocalAlloc( LPTR,
                                                             cchTemplateName)))
            {
                LocalFree(pCFW);
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                return (FALSE);
            }
            else
            {
                SHAnsiToUnicode(pCFA->lpTemplateName,(LPWSTR)pCFW->lpTemplateName,cchTemplateName);
            }
        }
        else
        {
            (DWORD_PTR)pCFW->lpTemplateName = (DWORD_PTR)pCFA->lpTemplateName;
        }
    }
    else
    {
        pCFW->lpTemplateName = NULL;
    }

    if ((pCFA->Flags & CF_USESTYLE) && (!IS_INTRESOURCE(pCFA->lpszStyle)))
    {
        asStyle.MaximumLength = LF_FACESIZE;
        asStyle.Length = (USHORT) (lstrlenA(pCFA->lpszStyle));
        if (asStyle.Length >= asStyle.MaximumLength)
        {
            asStyle.MaximumLength = asStyle.Length;
        }
    }
    else
    {
        asStyle.Length = usStyle.Length = 0;
        asStyle.MaximumLength = LF_FACESIZE;
    }
    usStyle.MaximumLength = asStyle.MaximumLength * sizeof(WCHAR);
    usStyle.Length = asStyle.Length * sizeof(WCHAR);

    if (!(pStrMem = (LPBYTE)LocalAlloc( LPTR,
                                        asStyle.MaximumLength +
                                            usStyle.MaximumLength )))
    {
        if (cchTemplateName)
        {
            LocalFree((LPWSTR)(pCFW->lpTemplateName));
        }
        LocalFree(pCFW);
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    asStyle.Buffer = pStrMem;
    pCFW->lpszStyle = usStyle.Buffer =
        (LPWSTR)(asStyle.Buffer + asStyle.MaximumLength);

    if ((pCFA->Flags & CF_USESTYLE) && (!IS_INTRESOURCE(pCFA->lpszStyle)))
    {
        lstrcpyA(asStyle.Buffer, pCFA->lpszStyle);
    }

    FI.pCF = pCFW;
    FI.pCFA = pCFA;
    FI.ApiType = COMDLG_ANSI;
    FI.pasStyle = &asStyle;
    FI.pusStyle = &usStyle;

    ThunkChooseFontA2W(&FI);

#ifdef MM_DESIGNVECTOR_DEFINED
    if (result = ChooseFontX(&FI, fl))
#else
    if (result = ChooseFontX(&FI))
#endif // MM_DESIGNVECTOR_DEFINED
    {
        ThunkChooseFontW2A(&FI);

        //
        //  Doesn't say how many characters there are here.
        //
        if ((pCFA->Flags & CF_USESTYLE) && (!IS_INTRESOURCE(pCFA->lpszStyle)))
        {
            LPSTR psz = pCFA->lpszStyle;
            LPSTR pszT = asStyle.Buffer;

            try
            {
                while (*psz++ = *pszT++);
            }
            except (EXCEPTION_ACCESS_VIOLATION)
            {
                //
                //  Not enough space in the passed in string.
                //
                *--psz = '\0';
            }
        }
    }

    if (cchTemplateName)
    {
        LocalFree((LPWSTR)(pCFW->lpTemplateName));
    }
    LocalFree(pCFW);
    LocalFree(pStrMem);

    return (result);
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  ChooseFontW
//
//  Stub UNICODE function for ChooseFont when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseFontW(
   LPCHOOSEFONTW lpCFW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL WINAPI ChooseFontExW(
   LPCHOOSEFONTW lpCFW, DWORD fl)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}
#endif // MM_DESIGNVECTOR_DEFINED

#endif


////////////////////////////////////////////////////////////////////////////
//
//  ChooseFont
//
//  The ChooseFont function creates a system-defined dialog box from which
//  the user can select a font, a font style (such as bold or italic), a
//  point size, an effect (such as strikeout or underline), and a text
//  color.
//
////////////////////////////////////////////////////////////////////////////

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL WINAPI ChooseFont(
   LPCHOOSEFONT lpCF)
{
    BOOL result;

    ENUMLOGFONTEXDV LogFontDV;
    LPLOGFONT lpLogFont = NULL;

    if (!lpCF)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (lpCF->lpLogFont)
    {
        /* we ned space in lpLogFont for the DESIGNVECTOR */
        lpLogFont = lpCF->lpLogFont;

        lpCF->lpLogFont = (LPLOGFONT)&LogFontDV; 

        *lpCF->lpLogFont = *lpLogFont;

        ((LPENUMLOGFONTEXDV)lpCF->lpLogFont)->elfDesignVector.dvNumAxes = 0;  // set number of axis to zero
        ((LPENUMLOGFONTEXDV)lpCF->lpLogFont)->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;  
    }

    result = ChooseFontEx(lpCF, 0);


    if (lpLogFont)
    {
        /* copy back lpLogFont */

        *lpLogFont = *lpCF->lpLogFont;

        lpCF->lpLogFont = lpLogFont;
    }

    return result;
}
#endif // MM_DESIGNVECTOR_DEFINED

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL WINAPI ChooseFontEx(
   LPCHOOSEFONT lpCF, DWORD fl)
#else
BOOL WINAPI ChooseFont(
   LPCHOOSEFONT lpCF)
#endif // MM_DESIGNVECTOR_DEFINED
{
    FONTINFO FI;

    ZeroMemory(&FI, sizeof(FONTINFO));

    FI.pCF = lpCF;
    FI.ApiType = COMDLG_WIDE;

#ifdef MM_DESIGNVECTOR_DEFINED
    return ( ChooseFontX(&FI, fl) );
#else
    return ( ChooseFontX(&FI) );
#endif // MM_DESIGNVECTOR_DEFINED
}


////////////////////////////////////////////////////////////////////////////
//
//  ChooseFontX
//
//  Invokes the font picker dialog, which lets the user specify common
//  character format attributes: facename, point size, text color and
//  attributes (bold, italic, strikeout or underline).
//
//  lpCF    - ptr to structure that will hold character attributes
//  ApiType - api type (COMDLG_WIDE or COMDLG_ANSI) so that the dialog
//            can remember which message to send to the user.
//
//  Returns:   TRUE  - user pressed IDOK
//             FALSE - user pressed IDCANCEL
//
////////////////////////////////////////////////////////////////////////////

BOOL ChooseFontX(
#ifdef MM_DESIGNVECTOR_DEFINED
    PFONTINFO pFI, DWORD fl)
#else
    PFONTINFO pFI)
#endif // MM_DESIGNVECTOR_DEFINED
{
    INT_PTR iRet;                // font picker dialog return value
    HANDLE hDlgTemplate;         // handle to loaded dialog resource
    HANDLE hRes;                 // handle of res. block with dialog
    int id;
    LPCHOOSEFONT lpCF = pFI->pCF;
    BOOL fAllocLogFont = FALSE;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif
    LANGID LangID;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    StoreExtendedError(0);
    bUserPressedCancel = FALSE;

    if (!lpCF)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (lpCF->lStructSize != sizeof(CHOOSEFONT))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (!lpCF->lpLogFont)
    {
#ifdef MM_DESIGNVECTOR_DEFINED
        if (!(lpCF->lpLogFont = (LPLOGFONT)LocalAlloc(LPTR, sizeof(ENUMLOGFONTEXDV))))
#else
        if (!(lpCF->lpLogFont = (LPLOGFONT)LocalAlloc(LPTR, sizeof(LOGFONT))))
#endif // MM_DESIGNVECTOR_DEFINED
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            return (FALSE);
        }

        fAllocLogFont = TRUE;
    } 
#ifdef MM_DESIGNVECTOR_DEFINED
    else 
    {
        GetCannonicalName(lpCF->lpLogFont->lfFaceName, 
            &((LPENUMLOGFONTEXDVW)lpCF->lpLogFont)->elfDesignVector);
    }

    // GetProcAddress for CreateFontIndirectEx, if this function exist, it's safe
    // to access DESIGNVECTOR information

    
    pFI->pfnCreateFontIndirectEx = NULL;

    if ( (lpCF->Flags & CF_MM_DESIGNVECTOR) &&  
         !(lpCF->Flags & CF_ENABLETEMPLATE) && !(lpCF->Flags & CF_ENABLETEMPLATEHANDLE))
    {
        HINSTANCE hinst = GetModuleHandleA("GDI32.DLL");

        if (hinst)
        {
#ifdef UNICODE
            pFI->pfnCreateFontIndirectEx = (PFNCREATEFONTINDIRECTEX)GetProcAddress(hinst, "CreateFontIndirectExW");
#else
            pFI->pfnCreateFontIndirectEx = (PFNCREATEFONTINDIRECTEX)GetProcAddress(hinst, "CreateFontIndirectExA");
#endif
        }
    }
#endif // MM_DESIGNVECTOR_DEFINED
    //
    //  Get the process version of the app for later use.
    //
    pFI->ProcessVersion = GetProcessVersion(0);

    //
    //  Get the default user language id for later use.
    //
    g_bIsSimplifiedChineseUI = IsSimplifiedChineseUI();


    //
    //  Verify that lpfnHook is not null if CF_ENABLEHOOK is specified.
    //
    if (lpCF->Flags & CF_ENABLEHOOK)
    {
        if (!lpCF->lpfnHook)
        {
            if (fAllocLogFont)
            {
                LocalFree(lpCF->lpLogFont);
                lpCF->lpLogFont = NULL;
            }
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        lpCF->lpfnHook = NULL;
    }

    if (lpCF->Flags & CF_ENABLETEMPLATE)
    {
        //
        //  Both custom instance handle and the dialog template name are
        //  user specified. Locate the dialog resource in the specified
        //  instance block and load it.
        //
        if (!(hRes = FindResource(lpCF->hInstance, lpCF->lpTemplateName, RT_DIALOG)))
        {
            if (fAllocLogFont)
            {
                LocalFree(lpCF->lpLogFont);
                lpCF->lpLogFont = NULL;
            }
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if (!(hDlgTemplate = LoadResource(lpCF->hInstance, hRes)))
        {
            if (fAllocLogFont)
            {
                LocalFree(lpCF->lpLogFont);
                lpCF->lpLogFont = NULL;
            }
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
        LangID = GetDialogLanguage(lpCF->hwndOwner, hDlgTemplate);
    }
    else if (lpCF->Flags & CF_ENABLETEMPLATEHANDLE)
    {
        //
        //  A handle to the pre-loaded resource has been specified.
        //
        hDlgTemplate = lpCF->hInstance;
        LangID = GetDialogLanguage(lpCF->hwndOwner, hDlgTemplate);
    }
    else
    {

#ifdef MM_DESIGNVECTOR_DEFINED
        if (!(lpCF->Flags & CF_MM_DESIGNVECTOR))
        {
            id = FORMATDLGORD31;
        }
        else
        {
            id = FONTDLGMMAXES;
        }
#else
        id = FORMATDLGORD31;
#endif
        LangID = GetDialogLanguage(lpCF->hwndOwner, NULL);

        if (!(hRes = FindResourceEx(g_hinst, RT_DIALOG, MAKEINTRESOURCE(id), LangID)))
        {
            if (fAllocLogFont)
            {
                LocalFree(lpCF->lpLogFont);
                lpCF->lpLogFont = NULL;
            }
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if (!(hDlgTemplate = LoadResource(g_hinst, hRes)))
        {
            if (fAllocLogFont)
            {
                LocalFree(lpCF->lpLogFont);
                lpCF->lpLogFont = NULL;
            }
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }

    //
    // Warning! Warning! Warning!
    //
    // We have to set g_tlsLangID before any call for CDLoadString
    //
    TlsSetValue(g_tlsLangID, (LPVOID) LangID);

    if (LockResource(hDlgTemplate))
    {
        if (lpCF->Flags & CF_ENABLEHOOK)
        {
            glpfnFontHook = GETHOOKFN(lpCF);
        }

#ifdef UNICODE
        if (IS16BITWOWAPP(lpCF))
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        iRet = DialogBoxIndirectParamAorW( g_hinst,
                                           (LPDLGTEMPLATE)hDlgTemplate,
                                           lpCF->hwndOwner,
                                           FormatCharDlgProc,
                                           (LPARAM)pFI,
                                           uiWOWFlag );
#else
        iRet = DialogBoxIndirectParam( g_hinst,
                                       (LPDLGTEMPLATE)hDlgTemplate,
                                       lpCF->hwndOwner,
                                       FormatCharDlgProc,
                                       (LPARAM)pFI );
#endif

        glpfnFontHook = 0;

        if (iRet == -1 || ((iRet == 0) && (!bUserPressedCancel) && (!GetStoredExtendedError())))
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
    }

    if (fAllocLogFont)
    {
        LocalFree(lpCF->lpLogFont);
        lpCF->lpLogFont = NULL;
    } 
#ifdef MM_DESIGNVECTOR_DEFINED
    else
    {
        if (lpCF->lpLogFont && !(fl & CHF_DESIGNVECTOR))
        {
            UINT i;
            BOOL bDefaultAxis = TRUE;
            if (pFI->DefaultDesignVector.dvNumAxes != ((LPENUMLOGFONTEXDVW)lpCF->lpLogFont)->elfDesignVector.dvNumAxes)
            {
                bDefaultAxis = FALSE;
            }
            else
            {
                for (i=0; i<pFI->DefaultDesignVector.dvNumAxes; i++)
                {
                    if (pFI->DefaultDesignVector.dvValues[i] != ((LPENUMLOGFONTEXDVW)lpCF->lpLogFont)->elfDesignVector.dvValues[i])
                    {
                        bDefaultAxis = FALSE;
                    }
                }
            }

            if (!bDefaultAxis)
            {
                AddDesignVectorToName(lpCF->lpLogFont->lfFaceName, 
                    &((LPENUMLOGFONTEXDVW)lpCF->lpLogFont)->elfDesignVector);
            }
        }
    }
#endif // MM_DESIGNVECTOR_DEFINED

    return (iRet == IDOK);
}

#ifdef MM_DESIGNVECTOR_DEFINED
////////////////////////////////////////////////////////////////////////////
//
//  SetMMAxesSelection
//
////////////////////////////////////////////////////////////////////////////

VOID SetMMAxesSelection(
    HWND hDlg,
    LPCHOOSEFONT lpcf)
{
    UINT i;
    SCROLLINFO scri;

    if (MAX_NUM_AXES < ((LPENUMLOGFONTEXDV)lpcf->lpLogFont)->elfDesignVector.dvNumAxes)
    {
        ((LPENUMLOGFONTEXDV)lpcf->lpLogFont)->elfDesignVector.dvNumAxes = 0; // for safety, set NumAxis to zero
//        MessageBox (hDlg, TEXT("Cannot support so many axes"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    for (i=0; i<((LPENUMLOGFONTEXDV)lpcf->lpLogFont)->elfDesignVector.dvNumAxes; i++)
    {
            // setting the positions of scroll bars according to MM Axes values
            scri.nPos   = ((LPENUMLOGFONTEXDV)lpcf->lpLogFont)->elfDesignVector.dvValues[i];
            scri.fMask  = SIF_POS;
            SetScrollInfo(GetDlgItem(hDlg, scr1 + i), SB_CTL, &scri, TRUE);
            SetDlgItemInt ( hDlg, edt1 + i, scri.nPos, TRUE);
    }

}
#endif // MM_DESIGNVECTOR_DEFINED

////////////////////////////////////////////////////////////////////////////
//
//  SetStyleSelection
//
////////////////////////////////////////////////////////////////////////////

VOID SetStyleSelection(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    BOOL bInit)
{
    if (!(lpcf->Flags & CF_NOSTYLESEL))
    {
        if (bInit && (lpcf->Flags & CF_USESTYLE))
        {
            PLOGFONT plf;
            int iSel;

            iSel = CBSetSelFromText(GetDlgItem(hDlg, cmb2), lpcf->lpszStyle);
            if (iSel >= 0)
            {
                LPITEMDATA lpItemData =
                     (LPITEMDATA)SendDlgItemMessage( hDlg,
                                                     cmb2,
                                                     CB_GETITEMDATA,
                                                     iSel,
                                                     0L );
                if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
                {
                    plf = lpItemData->pLogFont;

                    lpcf->lpLogFont->lfWeight = plf->lfWeight;
                    lpcf->lpLogFont->lfItalic = plf->lfItalic;
                }
                else
                {
                    lpcf->lpLogFont->lfWeight = FW_NORMAL;
                    lpcf->lpLogFont->lfItalic = 0;
                }
            }
            else
            {
                lpcf->lpLogFont->lfWeight = FW_NORMAL;
                lpcf->lpLogFont->lfItalic = 0;
            }
        }
        else
        {
            SelectStyleFromLF(GetDlgItem(hDlg, cmb2), lpcf->lpLogFont);
        }

        CBSetTextFromSel(GetDlgItem(hDlg, cmb2));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HideDlgItem
//
////////////////////////////////////////////////////////////////////////////

VOID HideDlgItem(
    HWND hDlg,
    int id)
{
    EnableWindow(GetDlgItem(hDlg, id), FALSE);
    ShowWindow(GetDlgItem(hDlg, id), SW_HIDE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FixComboHeights
//
//  Fixes the ownerdraw combo boxes to match the height of the non
//  ownerdraw combo boxes.
//
////////////////////////////////////////////////////////////////////////////

VOID FixComboHeights(
    HWND hDlg)
{
    LPARAM height;

    height = SendDlgItemMessage(hDlg, cmb2, CB_GETITEMHEIGHT, (WPARAM)-1, 0L);
    SendDlgItemMessage(hDlg, cmb1, CB_SETITEMHEIGHT, (WPARAM)-1, height);
    SendDlgItemMessage(hDlg, cmb3, CB_SETITEMHEIGHT, (WPARAM)-1, height);
}


////////////////////////////////////////////////////////////////////////////
//
//  FormatCharDlgProc
//
//  Message handler for font dlg
//
//  chx1 - "underline" checkbox
//  chx2 - "strikeout" checkbox
//  psh4 - "help" pushbutton
//
//  On WM_INITDIALOG message, the choosefont is accessed via lParam,
//  and stored in the window's prop list.  If a hook function has been
//  specified, it is invoked AFTER the current function has processed
//  WM_INITDIALOG.
//
//  For all other messages, control is passed directly to the hook
//  function first.  Depending on the latter's return value, the message
//  is processed by this function.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK FormatCharDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PFONTINFO pFI;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;
    HDC hDC;                      // handle to screen DC
    LPCHOOSEFONT pCF = NULL;      // ptr to struct passed to ChooseFont()
    HWND hWndHelp;                // handle to Help... pushbutton
    short nIndex;                 // at init, see if color matches
    TCHAR szPoints[20];
    HDC hdc;
    HFONT hFont;
    DWORD dw;
    BOOL_PTR bRet;
    LPTSTR lpRealFontName, lpSubFontName;
    int iResult;
    BOOL bContinueChecking;

#ifdef MM_DESIGNVECTOR_DEFINED
    int i, nCode, nPos, oldPos;
    HWND hwndScr;
#endif // MM_DESIGNVECTOR_DEFINED

    //
    //  If CHOOSEFONT struct has already been accessed and if a hook
    //  function is specified, let it do the processing first.
    //
    if (pFI = (PFONTINFO)GetProp(hDlg, FONTPROP))
    {
        if ((pCF = (LPCHOOSEFONT)pFI->pCF) &&
            (pCF->lpfnHook))
        {
            LPCFHOOKPROC lpfnHook = GETHOOKFN(pCF);

            if ((bRet = (*lpfnHook)(hDlg, wMsg, wParam, lParam)))
            {
                if ((wMsg == WM_COMMAND) &&
                    (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL))
                {
                    //
                    //  Set global flag stating that the user pressed cancel.
                    //
                    bUserPressedCancel = TRUE;
                }
                return (bRet);
            }
        }
    }
    else
    {
        if (glpfnFontHook &&
            (wMsg != WM_INITDIALOG) &&
            (bRet = (* glpfnFontHook)(hDlg, wMsg, wParam, lParam)))
        {
            return (bRet);
        }
    }

    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            pFI = (PFONTINFO)lParam;

            if (!CDLoadString(g_hinst, iszRegular, (LPTSTR)szRegular, CCHSTYLE) ||
                !CDLoadString(g_hinst, iszBold, (LPTSTR)szBold, CCHSTYLE)       ||
                !CDLoadString(g_hinst, iszItalic, (LPTSTR)szItalic, CCHSTYLE)   ||
                !CDLoadString(g_hinst, iszBoldItalic, (LPTSTR)szBoldItalic, CCHSTYLE))
            {
                StoreExtendedError(CDERR_LOADSTRFAILURE);
                EndDialog(hDlg, FALSE);
                return (FALSE);
            }

            pCF = pFI->pCF;
            if ((pCF->Flags & CF_LIMITSIZE) &&
                (pCF->nSizeMax < pCF->nSizeMin))
            {
                StoreExtendedError(CFERR_MAXLESSTHANMIN);
                EndDialog(hDlg, FALSE);
                return (FALSE);
            }

            //
            //  Save ptr to CHOOSEFONT struct in the dialog's prop list.
            //  Alloc a temp LOGFONT struct to be used for the length of
            //  the dialog session, the contents of which will be copied
            //  over to the final LOGFONT (pointed to by CHOOSEFONT)
            //  only if <OK> is selected.
            //
            SetProp(hDlg, FONTPROP, (HANDLE)pFI);
            glpfnFontHook = 0;

            hDlgFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);

            if (!hbmFont)
            {
                hbmFont = LoadBitmaps(BMFONT);
            }

            if (!(pCF->Flags & CF_APPLY))
            {
                HideDlgItem(hDlg, psh3);
            }

            if (!(pCF->Flags & CF_EFFECTS))
            {
                HideDlgItem(hDlg, stc4);
                HideDlgItem(hDlg, cmb4);
            }
            else
            {
                //
                //  Fill color list.
                //
                FillColorCombo(hDlg);
                for (nIndex = CCHCOLORS - 1; nIndex > 0; nIndex--)
                {
                    dw = (DWORD) SendDlgItemMessage( hDlg,
                                                     cmb4,
                                                     CB_GETITEMDATA,
                                                     nIndex,
                                                     0L );
                    if (pCF->rgbColors == dw)
                    {
                        break;
                    }
                }
                SendDlgItemMessage(hDlg, cmb4, CB_SETCURSEL, nIndex, 0L);
            }

            GetWindowRect(GetDlgItem (hDlg, stc5), &pFI->rcText);
            MapWindowPoints(NULL, hDlg, (POINT *)(&pFI->rcText), 2);
            FixComboHeights(hDlg);

            //
            //  Init our LOGFONT.
            //
            if (!(pCF->Flags & CF_INITTOLOGFONTSTRUCT))
            {
                InitLF(pCF->lpLogFont);
            }

            //
            //  Init effects.
            //
            if (!(pCF->Flags & CF_EFFECTS))
            {
                HideDlgItem(hDlg, grp1);
                HideDlgItem(hDlg, chx1);
                HideDlgItem(hDlg, chx2);
            }
            else
            {
                CheckDlgButton(hDlg, chx1, pCF->lpLogFont->lfStrikeOut);
                CheckDlgButton(hDlg, chx2, pCF->lpLogFont->lfUnderline);
            }

            pFI->nLastFontType = 0;

            if (!GetFontFamily( hDlg,
                                pCF->hDC,
                                pCF->Flags,
                                pCF->lpLogFont->lfCharSet ))
            {
                StoreExtendedError(CFERR_NOFONTS);
                if (pCF->Flags & CF_ENABLEHOOK)
                {
                    glpfnFontHook = GETHOOKFN(pCF);
                }
                EndDialog(hDlg, FALSE);
                return (FALSE);
            }

            if (!(pCF->Flags & CF_NOFACESEL) && *pCF->lpLogFont->lfFaceName)
            {
                //
                //  We want to select the font the user has requested.
                //
                iResult = CBSetSelFromText( GetDlgItem(hDlg, cmb1),
                                            pCF->lpLogFont->lfFaceName );

                //
                //  If iResult == CB_ERR, then we could be working with a
                //  font subsitution name (eg: MS Shell Dlg).
                //
                if (iResult == CB_ERR)
                {
                    lpSubFontName = pCF->lpLogFont->lfFaceName;
                }

                //
                //  Allocate a buffer to store the real font name in.
                //
                lpRealFontName = GlobalAlloc(GPTR, MAX_PATH * sizeof(TCHAR));

                if (!lpRealFontName)
                {
                    StoreExtendedError(CDERR_MEMALLOCFAILURE);
                    EndDialog(hDlg, FALSE);
                    return (FALSE);
                }

                //
                //  The while loop is necessary in order to resolve
                //  substitions pointing to subsitutions.
                //     eg:  Helv->MS Shell Dlg->MS Sans Serif
                //
                bContinueChecking = TRUE;
                while ((iResult == CB_ERR) && bContinueChecking)
                {
                    bContinueChecking = LookUpFontSubs( lpSubFontName,
                                                        lpRealFontName );

                    //
                    //  If bContinueChecking is TRUE, then we have a font
                    //  name.  Try to select that in the list.
                    //
                    if (bContinueChecking)
                    {
                        iResult = CBSetSelFromText( GetDlgItem(hDlg, cmb1),
                                                    lpRealFontName );
                    }

                    lpSubFontName = lpRealFontName;
                }

                //
                //  Free our buffer.
                //
                GlobalFree(lpRealFontName);

                //
                //  Set the edit control text if appropriate.
                //
                if (iResult != CB_ERR)
                {
                    CBSetTextFromSel(GetDlgItem(hDlg, cmb1));
                }
            }

            hdc = GetDC(NULL);

            if (pCF->Flags & CF_NOSCRIPTSEL)
            {
                hWndHelp = GetDlgItem(hDlg, cmb5);
                if (hWndHelp)
                {
                    CDLoadString( g_hinst,
                                iszNoScript,
                                szPoints,
                                sizeof(szPoints) / sizeof(TCHAR));
                    CBAddScript(hWndHelp, szPoints, DEFAULT_CHARSET);
                    EnableWindow(hWndHelp, FALSE);
                }
                DefaultCharset = DEFAULT_CHARSET;
                pFI->iCharset = DEFAULT_CHARSET;
            }
            else if (pCF->Flags & (CF_SELECTSCRIPT | CF_INITTOLOGFONTSTRUCT))
            {
                //
                //  We could come in here with a bogus value, if the app is
                //  NOT 4.0, that would result in the bogus charset not
                //  being found for the facename, and the default would be
                //  put back again anyway.
                //
                pFI->iCharset = pCF->lpLogFont->lfCharSet;
            }
            else
            {
                DefaultCharset = GetTextCharset(hdc);
                pFI->iCharset = DefaultCharset;
            }

            GetFontStylesAndSizes(hDlg, pFI, pCF, TRUE);

            if (!(pCF->Flags & CF_NOSTYLESEL))
            {
                SetStyleSelection(hDlg, pCF, TRUE);
            }

            if (!(pCF->Flags & CF_NOSIZESEL) && pCF->lpLogFont->lfHeight)
            {
                GetPointString(szPoints, hdc, pCF->lpLogFont->lfHeight);
                CBSetSelFromText(GetDlgItem(hDlg, cmb3), szPoints);
                SetDlgItemText(hDlg, cmb3, szPoints);
            }

#ifdef MM_DESIGNVECTOR_DEFINED
            SetMMAxesSelection(hDlg, pCF);
#endif // MM_DESIGNVECTOR_DEFINED

            ReleaseDC(NULL, hdc);

            //
            //  Hide the help button if it isn't needed.
            //
            if (!(pCF->Flags & CF_SHOWHELP))
            {
                ShowWindow(hWndHelp = GetDlgItem(hDlg, pshHelp), SW_HIDE);
                EnableWindow(hWndHelp, FALSE);
            }

            SendDlgItemMessage(hDlg, cmb1, CB_LIMITTEXT, LF_FACESIZE - 1, 0L);
            SendDlgItemMessage(hDlg, cmb2, CB_LIMITTEXT, LF_FACESIZE - 1, 0L);
            SendDlgItemMessage(hDlg, cmb3, CB_LIMITTEXT, 5, 0L);

            //
            //  If hook function has been specified, let it do any additional
            //  processing of this message.
            //
            if (pCF->lpfnHook)
            {
                LPCFHOOKPROC lpfnHook = GETHOOKFN(pCF);
#ifdef UNICODE
                if (pFI->ApiType == COMDLG_ANSI)
                {
                    ThunkChooseFontW2A(pFI);
                    bRet = (*lpfnHook)( hDlg,
                                        wMsg,
                                        wParam,
                                        (LPARAM)pFI->pCFA );
                    ThunkChooseFontA2W(pFI);
                }
                else
#endif
                {
                    bRet = (*lpfnHook)( hDlg,
                                        wMsg,
                                        wParam,
                                        (LPARAM)pCF );
                }
                return (bRet);
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            break;
        }
        case ( WM_DESTROY ) :
        {
            if (pCF)
            {
                RemoveProp(hDlg, FONTPROP);
            }
            break;
        }
        case ( WM_PAINT ) :
        {
            if (!pFI)
            {
                return (FALSE);
            }

            if (BeginPaint(hDlg, &ps))
            {
                DrawSampleText(hDlg, pFI, pCF, ps.hdc);
                EndPaint(hDlg, &ps);
            }
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            hDC = GetDC(hDlg);
            hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);
            if (hFont)
            {
                hFont = SelectObject(hDC, hFont);
            }
            GetTextMetrics(hDC, &tm);
            if (hFont)
            {
                SelectObject(hDC, hFont);
            }
            ReleaseDC(hDlg, hDC);

            if (((LPMEASUREITEMSTRUCT)lParam)->itemID != -1)
            {
                ((LPMEASUREITEMSTRUCT)lParam)->itemHeight =
                       max(tm.tmHeight, DY_BITMAP);
            }
            else
            {
                //
                //  This is for 3.0 only.  In 3.1, the CB_SETITEMHEIGHT
                //  will fix this.  Note, this is off by one on 8514.
                //
                ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = tm.tmHeight + 1;
            }

            break;
        }
        case ( WM_DRAWITEM ) :
        {
#define lpdis ((LPDRAWITEMSTRUCT)lParam)

            if (lpdis->itemID == (UINT)-1)
            {
                break;
            }

            if (lpdis->CtlID == cmb4)
            {
                DrawColorComboItem(lpdis);
            }
            else if (lpdis->CtlID == cmb1)
            {
                DrawFamilyComboItem(lpdis);
            }
            else
            {
                DrawSizeComboItem(lpdis);
            }
            break;

#undef lpdis
        }
        case ( WM_SYSCOLORCHANGE ) :
        {
            DeleteObject(hbmFont);
            hbmFont = LoadBitmaps(BMFONT);
            break;
        }
        case ( WM_COMMAND ) :
        {
            if (!pFI)
            {
                return (FALSE);
            }

            return (ProcessDlgCtrlCommand(hDlg, pFI, wParam, lParam));
            break;
        }
        case ( WM_HELP ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPVOID)aFontHelpIDs );
            }
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)aFontHelpIDs );
            }
            break;
        }
        case ( WM_CHOOSEFONT_GETLOGFONT ) :
        {
Handle_WM_CHOOSEFONT_GETLOGFONT:
            if (!pFI)
            {
                return (FALSE);
            }

#ifdef UNICODE
            if (pFI->ApiType == COMDLG_ANSI)
            {
                BOOL bRet;
                LOGFONT lf;

                bRet = FillInFont(hDlg, pFI, pCF, &lf, TRUE);

                ThunkLogFontW2A(&lf, (LPLOGFONTA)lParam);

                return (bRet);
            }
            else
#endif
            {
                return (FillInFont(hDlg, pFI, pCF, (LPLOGFONT)lParam, TRUE));
            }
        }
        case ( WM_CHOOSEFONT_SETLOGFONT ) :
        {
            if (!pFI)
            {
                return (FALSE);
            }

#ifdef UNICODE
            if (pFI->ApiType == COMDLG_ANSI)
            {
                LOGFONT lf;

                ThunkLogFontA2W((LPLOGFONTA)lParam, &lf);

                return (SetLogFont(hDlg, pCF, &lf));
            }
            else
#endif
            {
                return (SetLogFont(hDlg, pCF, (LPLOGFONT)lParam));
            }
            break;
        }

        case ( WM_CHOOSEFONT_SETFLAGS ) :
        {
            DWORD dwFlags = pCF->Flags;

            pCF->Flags = (DWORD) lParam;
            SetDlgMsgResult(hDlg, WM_CHOOSEFONT_SETFLAGS, dwFlags);
            return (TRUE);
        }
#ifdef MM_DESIGNVECTOR_DEFINED
        case ( WM_HSCROLL ) :
        {
             nCode = (int) LOWORD(wParam);  // scroll bar value 
             nPos = (short int) HIWORD(wParam);   // scroll box position 
             hwndScr = (HWND) lParam;       // handle to scroll bar               

             i = 0;
             while ((GetDlgItem(hDlg, scr1 + i) != hwndScr) && (i < MAX_NUM_AXES))
                 i++;
             if (i >= MAX_NUM_AXES)
                 return (FALSE);

             if (nCode == SB_THUMBPOSITION)
             {
                 SetDlgItemInt ( hDlg, edt1 + i, nPos,  TRUE );
                 SendMessage(hwndScr, SBM_SETPOS, nPos, (LPARAM)TRUE);

                 // redraw sample text
                 GetWindowRect(GetDlgItem (hDlg, stc5), &pFI->rcText);
                 MapWindowPoints(NULL, hDlg, (POINT *)(&pFI->rcText), 2);
                 InvalidateRect(hDlg, &pFI->rcText, FALSE);
                 UpdateWindow(hDlg);

                 return (TRUE);
             }

             if (nCode == SB_THUMBTRACK)
             {
                 SetDlgItemInt ( hDlg, edt1 + i, nPos,  TRUE );
                 SendMessage(hwndScr, SBM_SETPOS, nPos, (LPARAM)TRUE);

                 // redraw sample text
                 GetWindowRect(GetDlgItem (hDlg, stc5), &pFI->rcText);
                 MapWindowPoints(NULL, hDlg, (POINT *)(&pFI->rcText), 2);
                 InvalidateRect(hDlg, &pFI->rcText, FALSE);
                 UpdateWindow(hDlg);

                 return (TRUE);
             }

             oldPos = (int)SendMessage(hwndScr, SBM_GETPOS, 0, 0);
             if (nCode == SB_LINELEFT)
                 oldPos -= 1;
             if (nCode == SB_LINERIGHT)
                 oldPos += 1;
             if (nCode == SB_PAGELEFT)
                 oldPos -= 10;
             if (nCode == SB_PAGERIGHT)
                 oldPos += 10;
             if (nCode == SB_LINELEFT || nCode == SB_LINERIGHT || nCode == SB_PAGELEFT || nCode == SB_PAGERIGHT)
             {
                 SetDlgItemInt ( hDlg, edt1 + i, oldPos,  TRUE );
                 SendMessage(hwndScr, SBM_SETPOS, oldPos, (LPARAM)TRUE);

                 // redraw the sample text
                 GetWindowRect(GetDlgItem (hDlg, stc5), &pFI->rcText);
                 MapWindowPoints(NULL, hDlg, (POINT *)(&pFI->rcText), 2);
                 InvalidateRect(hDlg, &pFI->rcText, FALSE);
                 UpdateWindow(hDlg);

                 return (TRUE);
             }
             return (TRUE);
        }
#endif // MM_DESIGNVECTOR_DEFINED
        default :
        {
            if (wMsg == msgWOWCHOOSEFONT_GETLOGFONT)
            {
                goto Handle_WM_CHOOSEFONT_GETLOGFONT;
            }
            return (FALSE);
        }
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelectStyleFromLF
//
//  Given a logfont, selects the closest match in the style list.
//
////////////////////////////////////////////////////////////////////////////

void SelectStyleFromLF(
    HWND hwnd,
    LPLOGFONT lplf)
{
    int ctr, count, iSel;
    PLOGFONT plf;
    int weight_delta, best_weight_delta = 1000;
    BOOL bIgnoreItalic;
    LPITEMDATA lpItemData;


    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
    iSel = 0;
    bIgnoreItalic = FALSE;

TryAgain:
    for (ctr = 0; ctr < count; ctr++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, ctr, 0L);

        if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            plf = lpItemData->pLogFont;

            if (bIgnoreItalic ||
                (plf->lfItalic && lplf->lfItalic) ||
                (!plf->lfItalic && !lplf->lfItalic))
            {
                weight_delta = lplf->lfWeight - plf->lfWeight;
                if (weight_delta < 0)
                {
                    weight_delta = -weight_delta;
                }

                if (weight_delta < best_weight_delta)
                {
                    best_weight_delta = weight_delta;
                    iSel = ctr;
                }
            }
        }
    }
    if (!bIgnoreItalic && iSel == 0)
    {
        bIgnoreItalic = TRUE;
        goto TryAgain;
    }

    SendMessage(hwnd, CB_SETCURSEL, iSel, 0L);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBSetTextFromSel
//
//  Makes the currently selected item the edit text for a combo box.
//
////////////////////////////////////////////////////////////////////////////

int CBSetTextFromSel(
    HWND hwnd)
{
    int iSel;
    TCHAR szFace[LF_FACESIZE];

    iSel = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0L);
    if (iSel >= 0)
    {
        SendMessage(hwnd, CB_GETLBTEXT, iSel, (LONG_PTR)(LPTSTR)szFace);
        SetWindowText(hwnd, szFace);
    }
    return (iSel);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBSetSelFromText
//
//  Sets the selection based on lpszString.  Sends notification messages
//  if bNotify is TRUE.
//
////////////////////////////////////////////////////////////////////////////

int CBSetSelFromText(
    HWND hwnd,
    LPTSTR lpszString)
{
    int iInd;

    iInd = CBFindString(hwnd, lpszString);

    if (iInd >= 0)
    {
        SendMessage(hwnd, CB_SETCURSEL, iInd, 0L);
    }
    return (iInd);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBGetTextAndData
//
//  Returns the text and item data for a combo box based on the current
//  edit text.  If the current edit text does not match anything in the
//  listbox, then CB_ERR is returned.
//
////////////////////////////////////////////////////////////////////////////

int CBGetTextAndData(
    HWND hwnd,
    LPTSTR lpszString,
    int iSize,
    PULONG_PTR lpdw)
{
    LRESULT Result;
    int iSel;

    if (lpszString == NULL)
    {
        if ((Result = SendMessage(hwnd, CB_GETITEMDATA, 0, 0L)) < 0)
        {
            return ((int) Result);
        }
        else
        {
            *lpdw = Result;
            return (0);
        }
    }

    GetWindowText(hwnd, lpszString, iSize);
    iSel = CBFindString(hwnd, lpszString);
    if (iSel < 0)
    {
        return (iSel);
    }

    *lpdw = SendMessage(hwnd, CB_GETITEMDATA, iSel, 0L);
    return (iSel);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBFindString
//
//  Does an exact string find and returns the index.
//
////////////////////////////////////////////////////////////////////////////

int CBFindString(
    HWND hwnd,
    LPTSTR lpszString)
{
    return ((int)SendMessage( hwnd,
                              CB_FINDSTRINGEXACT,
                              (WPARAM)-1,
                              (LPARAM)(LPCSTR)lpszString ));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPointSizeInRange
//
//  Ensures that the point size edit field is in range.
//
//  Returns:  Point Size - of the edit field limitted by MIN/MAX size
//            0          - if the field is empty
//
////////////////////////////////////////////////////////////////////////////

#define GPS_COMPLAIN    0x0001
#define GPS_SETDEFSIZE  0x0002

BOOL GetPointSizeInRange(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPINT pts,
    WORD wFlags)
{
    TCHAR szBuffer[90];
    TCHAR szTitle[90];
    int nTmp;
    int nTmpFr = 0;
    BOOL bOK;

    *pts = 0;

    if (GetDlgItemText(hDlg, cmb3, szBuffer, sizeof(szBuffer) / sizeof(TCHAR)))
    {
        nTmp = GetDlgItemInt(hDlg, cmb3, &bOK, TRUE);

        if (!bOK && g_bIsSimplifiedChineseUI)
        {
            int ctr;
            LPTSTR lpsz = szBuffer;

            //
            //  Skip leading white space.
            //
            while (*lpsz == TEXT(' '))
            {
                lpsz++;
            }
            for (ctr = 0; ctr < NUM_ZIHAO; ctr++)
            {
                if (!lstrcmpi(lpsz, stZihao[ctr].name))
                {
                    bOK = TRUE;
                    nTmp = stZihao[ctr].size;
                    nTmpFr = stZihao[ctr].sizeFr;
                    break;
                }
            }
        }

        if (!bOK)
        {
            nTmp = 0;
        }
    }
    else if (wFlags & GPS_SETDEFSIZE)
    {
        nTmp = DEF_POINT_SIZE;
        bOK = TRUE;
    }
    else
    {
        //
        //  We're just returning with 0 in *pts.
        //
        return (FALSE);
    }

    //
    //  Check that we got a number in range.
    //
    if (wFlags & GPS_COMPLAIN)
    {
        if ((lpcf->Flags & CF_LIMITSIZE) &&
            (!bOK || (nTmp > lpcf->nSizeMax) || (nTmp < lpcf->nSizeMin)))
        {
            bOK = FALSE;
            CDLoadString( g_hinst,
                        iszSizeRange,
                        szTitle,
                        sizeof(szTitle) / sizeof(TCHAR) );

            wsprintf( (LPTSTR)szBuffer,
                      (LPTSTR)szTitle,
                      lpcf->nSizeMin,
                      lpcf->nSizeMax );
        }
        else if (!bOK)
        {
            CDLoadString( g_hinst,
                        iszSizeNumber,
                        szBuffer,
                        sizeof(szBuffer) / sizeof(TCHAR) );
        }

        if (!bOK)
        {
            GetWindowText(hDlg, szTitle, sizeof(szTitle) / sizeof(TCHAR));
            MessageBox(hDlg, szBuffer, szTitle, MB_OK | MB_ICONINFORMATION);
            return (FALSE);
        }
    }

    *pts = nTmp * 10 + nTmpFr;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ResetSampleFromScript
//
////////////////////////////////////////////////////////////////////////////

BOOL ResetSampleFromScript(
    HWND hDlg,
    HWND hwndScript,
    PFONTINFO pFI)
{
    int iSel;
    TCHAR szScript[LF_FACESIZE];
    LPITEMDATA lpItemData;

    if (IsWindow(hwndScript) && IsWindowEnabled(hwndScript))
    {
        iSel = (int)SendMessage(hwndScript, CB_GETCURSEL, 0, 0L);
        if (iSel >= 0)
        {
            lpItemData = (LPITEMDATA)SendMessage( hwndScript,
                                                  CB_GETITEMDATA,
                                                  iSel,
                                                  0L );
            if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
            {
                pFI->iCharset = lpItemData->nFontType;
            }
        }
    }

    if (!CDLoadString( g_hinst,
                     pFI->iCharset + iszFontSample,
                     szScript,
                     LF_FACESIZE ))
    {
        return (FALSE);
    }

    SetDlgItemText(hDlg, stc5, szScript);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ProcessDlgCtrlCommand
//
//  Handles all WM_COMMAND messages for the font dialog.
//
//  cmb1 - ID of font facename combobox
//  cmb2 - style
//  cmb3 - size
//  chx1 - "Underline" checkbox
//  chx2 - "Strikeout" checkbox
//  stc5 - frame around text preview area
//  psh4 - button that invokes the Help application
//  IDOK - OK button to end dialog, retaining information
//  IDCANCEL - button to cancel dialog, not doing anything
//
//  Returns:   TRUE    - if message is processed successfully
//             FALSE   - otherwise
//
////////////////////////////////////////////////////////////////////////////

BOOL ProcessDlgCtrlCommand(
    HWND hDlg,
    PFONTINFO pFI,
    WPARAM wParam,
    LPARAM lParam)
{
    int iSel;
    LPCHOOSEFONT pCF = (pFI ? pFI->pCF : NULL);
    TCHAR szPoints[10];
    TCHAR szStyle[LF_FACESIZE];
    LPITEMDATA lpItemData;
    WORD wCmbId;
    TCHAR szMsg[160], szTitle[160];


    if (pCF)
    {
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case ( IDABORT ) :
            {
                //
                //  This is how a hook can cause the dialog to go away.
                //
                FreeAllItemData(hDlg, pFI);
                if (pCF->Flags & CF_ENABLEHOOK)
                {
                    glpfnFontHook = GETHOOKFN(pCF);
                }
                
                // BUGBUG ARULK Why are we returning an HWND anyway?  
                // The caller (ChooseFontX) expects us to return a BOOL

                EndDialog(hDlg, BOOLFROMPTR(GET_WM_COMMAND_HWND(wParam, lParam)));
                break;
            }
            case ( IDOK ) :
            {
                //
                //  Make sure the focus is set to the OK button.  Must do
                //  this so that when the user presses Enter from one of
                //  the combo boxes, the kill focus processing is done
                //  before the data is captured.
                //
                SetFocus(GetDlgItem(hDlg, IDOK));

                if (!GetPointSizeInRange( hDlg,
                                          pCF,
                                          &iSel,
                                          GPS_COMPLAIN | GPS_SETDEFSIZE ))
                {
                    PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, cmb3), 1L);
                    break;
                }
                pCF->iPointSize = iSel;

#ifdef MM_DESIGNVECTOR_DEFINED
                FillInFontEx(hDlg, pFI, pCF, (LPENUMLOGFONTEXDV)pCF->lpLogFont, TRUE);
#else
                FillInFont(hDlg, pFI, pCF, pCF->lpLogFont, TRUE);
#endif // MM_DESIGNVECTOR_DEFINED

                if (pCF->Flags & CF_FORCEFONTEXIST)
                {
                    if (pCF->Flags & CF_NOFACESEL)
                    {
                        wCmbId = cmb1;
                    }
                    else if (pCF->Flags & CF_NOSTYLESEL)
                    {
                        wCmbId = cmb2;
                    }
                    else
                    {
                        wCmbId = 0;
                    }

                    if (wCmbId)
                    {
                        //
                        //  Error found.
                        //
                        CDLoadString( g_hinst,
                                    (wCmbId == cmb1)
                                        ? iszNoFaceSel
                                        : iszNoStyleSel,
                                    szMsg,
                                    sizeof(szMsg) / sizeof(TCHAR) );

                        GetWindowText( hDlg,
                                       szTitle,
                                       sizeof(szTitle) / sizeof(TCHAR) );
                        MessageBox( hDlg,
                                    szMsg,
                                    szTitle,
                                    MB_OK | MB_ICONINFORMATION );
                        PostMessage( hDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hDlg, wCmbId),
                                     1L );
                        break;
                    }
                }

                if (pCF->Flags & CF_EFFECTS)
                {
                    //
                    //  Get currently selected item in color combo box and
                    //  the 32 bit color rgb value associated with it.
                    //
                    iSel = (int)SendDlgItemMessage( hDlg,
                                                    cmb4,
                                                    CB_GETCURSEL,
                                                    0,
                                                    0L );
                    pCF->rgbColors = (DWORD) SendDlgItemMessage( hDlg,
                                                                 cmb4,
                                                                 CB_GETITEMDATA,
                                                                 iSel,
                                                                 0L );
                }

                //
                //  Get a valid nFontType.
                //
                iSel = CBGetTextAndData( GetDlgItem(hDlg, cmb2),
                                         szStyle,
                                         sizeof(szStyle) / sizeof(TCHAR),
                                         (PULONG_PTR)&lpItemData );
                if (iSel < 0)
                {
                    lpItemData = 0;
                    iSel = CBGetTextAndData( GetDlgItem(hDlg, cmb2),
                                             (LPTSTR)NULL,
                                             0,
                                             (PULONG_PTR)&lpItemData);
                }

                if (iSel >= 0 && lpItemData)
                {
                    pCF->nFontType = (WORD)lpItemData->nFontType;
                }
                else
                {
                    pCF->nFontType = 0;
                }

                if (pCF->Flags & CF_USESTYLE)
                {
                    lstrcpy(pCF->lpszStyle, szStyle);
                }

                goto LeaveDialog;
            }
            case ( IDCANCEL ) :
            {
                bUserPressedCancel = TRUE;

LeaveDialog:
                FreeAllItemData(hDlg, pFI);
                if (pCF->Flags & CF_ENABLEHOOK)
                {
                    glpfnFontHook = GETHOOKFN(pCF);
                }
                EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
                break;
            }
            case ( cmb1 ) :                 // facenames combobox
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case ( CBN_SELCHANGE ) :
                    {
                        CBSetTextFromSel(GET_WM_COMMAND_HWND(wParam, lParam));
FillStyles:
                        //
                        //  Try to maintain the current point size and style.
                        //
                        GetDlgItemText( hDlg,
                                        cmb3,
                                        szPoints,
                                        sizeof(szPoints) / sizeof(TCHAR) );
                        GetFontStylesAndSizes(hDlg, pFI, pCF, FALSE);
                        SetStyleSelection(hDlg, pCF, FALSE);

                        //
                        //  Preserve the point size selection or put it in
                        //  the edit control if it is not in the list for
                        //  this font.
                        //
                        iSel = CBFindString(GetDlgItem(hDlg, cmb3), szPoints);
                        if (iSel < 0)
                        {
                            SetDlgItemText(hDlg, cmb3, szPoints);
                        }
                        else
                        {
                            SendDlgItemMessage( hDlg,
                                                cmb3,
                                                CB_SETCURSEL,
                                                iSel,
                                                0L );
                        }

                        goto DrawSample;
                        break;
                    }
                    case ( CBN_EDITUPDATE ) :
                    {
                        PostMessage( hDlg,
                                     WM_COMMAND,
                                     GET_WM_COMMAND_MPS(
                                         GET_WM_COMMAND_ID(wParam, lParam),
                                         GET_WM_COMMAND_HWND(wParam, lParam),
                                         CBN_MYEDITUPDATE ) );
                        break;
                    }
                    case ( CBN_MYEDITUPDATE ) :
                    {
                        GetWindowText( GET_WM_COMMAND_HWND(wParam, lParam),
                                       szStyle,
                                       sizeof(szStyle) / sizeof(TCHAR) );
                        iSel = CBFindString( GET_WM_COMMAND_HWND(wParam, lParam),
                                             szStyle );
                        if (iSel >= 0)
                        {
                            // HACK: This is only for Korean input. Because Korean Edit control has
                            //       level 3 implementation for DBCS input, we may have a problem if 
                            //       focus is moving like below with interim character.
                            //       0xE0000412 is Korean IME layout id.
                            if (((ULONG_PTR)GetKeyboardLayout(0L) & 0xF000FFFFL) == 0xE0000412L)
                            {
                                HIMC hIMC = ImmGetContext(GET_WM_COMMAND_HWND(wParam, lParam));
                                LONG cb = ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0);
                                ImmReleaseContext(GET_WM_COMMAND_HWND(wParam, lParam), hIMC);
                                if (cb > 0)
                                    break;
                            }

                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETCURSEL,
                                         (WPARAM)iSel,
                                         0L );
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETEDITSEL,
                                         0,
                                         0x0000FFFF );
                            goto FillStyles;
                        }
                        break;
                    }
                }
                break;
            }
            case ( cmb2 ) :                 // styles combobox
            case ( cmb3 ) :                 // point sizes combobox
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case ( CBN_EDITUPDATE ) :
                    {
                        PostMessage( hDlg,
                                     WM_COMMAND,
                                     GET_WM_COMMAND_MPS(
                                         GET_WM_COMMAND_ID(wParam, lParam),
                                         GET_WM_COMMAND_HWND(wParam, lParam),
                                         CBN_MYEDITUPDATE ) );
                        break;
                    }
                    case ( CBN_MYEDITUPDATE ) :
                    {
                        GetWindowText( GET_WM_COMMAND_HWND(wParam, lParam),
                                       szStyle,
                                       sizeof(szStyle) / sizeof(TCHAR) );
                        iSel = CBFindString( GET_WM_COMMAND_HWND(wParam, lParam),
                                             szStyle );
                        if (iSel >= 0)
                        {
                            // HACK: This is only for Korean input. Because Korean Edit control has
                            //       level 3 implementation for DBCS input, we may have a problem if 
                            //       focus is moving like below with interim character.
                            //       0xE0000412 is Korean IME layout id.
                            if (((ULONG_PTR)GetKeyboardLayout(0L) & 0xF000FFFFL) == 0xE0000412L)
                            {
                                HIMC hIMC = ImmGetContext(GET_WM_COMMAND_HWND(wParam, lParam));
                                LONG cb = ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0);
                                ImmReleaseContext(GET_WM_COMMAND_HWND(wParam, lParam), hIMC);
                                if (cb > 0)
                                    break;
                            }

                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETCURSEL,
                                         iSel,
                                         0L );
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETEDITSEL,
                                         0,
                                         0x0000FFFF );
                            goto DrawSample;
                        }
                        break;
                    }
                    case ( CBN_SELCHANGE ) :
                    {
                        iSel = CBSetTextFromSel(GET_WM_COMMAND_HWND(wParam, lParam));

                        //
                        //  Make the style selection stick.
                        //
                        if ((iSel >= 0) &&
                            (GET_WM_COMMAND_ID(wParam, lParam) == cmb2))
                        {
                            LPITEMDATA lpItemData;
                            PLOGFONT plf;

                            lpItemData = (LPITEMDATA)SendMessage(
                                            GET_WM_COMMAND_HWND(wParam, lParam),
                                            CB_GETITEMDATA,
                                            iSel,
                                            0L );

                            if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
                            {
                                plf = lpItemData->pLogFont;
                                pCF->lpLogFont->lfWeight = plf->lfWeight;
                                pCF->lpLogFont->lfItalic = plf->lfItalic;
                            }
                            else
                            {
                                pCF->lpLogFont->lfWeight = FW_NORMAL;
                                pCF->lpLogFont->lfItalic = 0;
                            }
                        }

                        goto DrawSample;
                    }
                    case ( CBN_KILLFOCUS ) :
                    {
DrawSample:
#ifdef UNICODE
                        if (pFI->ApiType == COMDLG_ANSI)
                        {
                            //
                            //  Send special WOW message to indicate the
                            //  font style has changed.
                            //
                            LOGFONT lf;

                            if (FillInFont(hDlg, pFI, pCF, &lf, TRUE))
                            {
                                memcpy(pCF->lpLogFont, &lf, sizeof(LOGFONT));
                                ThunkLogFontW2A( pCF->lpLogFont,
                                                 pFI->pCFA->lpLogFont );
                                SendMessage( hDlg,
                                             msgWOWLFCHANGE,
                                             0,
                                             (LPARAM)(LPLOGFONT)pFI->pCFA->lpLogFont );
                            }
                        }
#endif

                        //
                        //  Force redraw of preview text for any size change.
                        //
                        InvalidateRect(hDlg, &pFI->rcText, FALSE);
                        UpdateWindow(hDlg);
                    }
                }
                break;
            }
            case ( cmb5 ) :                 // script combobox
            {
                //
                //  Need to change the sample text to reflect the new script.
                //
                if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_SELCHANGE)
                {
                    break;
                }
                if (pFI->ProcessVersion < 0x40000)
                {
                    //
                    //  Enabled template also has a cmb5!
                    //
                    return (FALSE);
                }
                if (ResetSampleFromScript( hDlg,
                                           GET_WM_COMMAND_HWND(wParam, lParam),
                                           pFI ))
                {
                    goto FillStyles;
                }
                else
                {
                    break;
                }
            }
            case ( cmb4 ) :
            {
                if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_SELCHANGE)
                {
                    break;
                }

                // fall thru...
            }
            case ( chx1 ) :                 // bold
            case ( chx2 ) :                 // italic
            {
                goto DrawSample;
            }
            case ( pshHelp ) :              // help
            {
#ifdef UNICODE
                if (pFI->ApiType == COMDLG_ANSI)
                {
                    if (msgHELPA && pCF->hwndOwner)
                    {
                        SendMessage( pCF->hwndOwner,
                                     msgHELPA,
                                     (WPARAM)hDlg,
                                     (LPARAM)pCF );
                    }
                }
                else
#endif
                {
                    if (msgHELPW && pCF->hwndOwner)
                    {
                        SendMessage( pCF->hwndOwner,
                                     msgHELPW,
                                     (WPARAM)hDlg,
                                     (LPARAM)pCF );
                    }
                }
                break;
            }
            default :
            {
                return (FALSE);
            }
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CmpFontType
//
//  Compares two font types.  The values of the font type bits are
//  monotonic except the low bit (RASTER_FONTTYPE).  After flipping
//  that bit the words can be compared directly.
//
//  Returns the best of the two.
//
////////////////////////////////////////////////////////////////////////////

int CmpFontType(
    DWORD ft1,
    DWORD ft2)
{
    ft1 &= ~(SCREEN_FONTTYPE | PRINTER_FONTTYPE);
    ft2 &= ~(SCREEN_FONTTYPE | PRINTER_FONTTYPE);

    //
    //  Flip the RASTER_FONTTYPE bit so we can compare.
    //
    ft1 ^= RASTER_FONTTYPE;
    ft2 ^= RASTER_FONTTYPE;

    return ( (int)ft1 - (int)ft2 );
}


////////////////////////////////////////////////////////////////////////////
//
//  FontFamilyEnumProc
//
//  nFontType bits
//
//  SCALABLE DEVICE RASTER
//     (TT)  (not GDI) (not scalable)
//      0       0       0       vector, ATM screen
//      0       0       1       GDI raster font
//      0       1       0       PS/LJ III, ATM printer, ATI/LaserMaster
//      0       1       1       non scalable device font
//      1       0       x       TT screen font
//      1       1       x       TT dev font
//
////////////////////////////////////////////////////////////////////////////

int FontFamilyEnumProc(
    LPENUMLOGFONTEX lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData)
{
    int iItem;
    DWORD nOldType, nNewType;
    LPITEMDATA lpItemData;
    LPITEMDATA lpOldItemData = NULL;

    //
    //  Bounce non TT fonts.
    //
    if ((lpData->dwFlags & CF_TTONLY) &&
        !(nFontType & TRUETYPE_FONTTYPE))
    {
        return (TRUE);
    }

    //
    //  Bounce non scalable fonts.
    //
    if ((lpData->dwFlags & CF_SCALABLEONLY) &&
        (nFontType & RASTER_FONTTYPE))
    {
        return (TRUE);
    }

    //
    //  Bounce non ANSI fonts.
    //
    if ((lpData->dwFlags & CF_SCRIPTSONLY) &&
        ((lplf->elfLogFont.lfCharSet == OEM_CHARSET) ||
         (lplf->elfLogFont.lfCharSet == SYMBOL_CHARSET)))
    {
        return (TRUE);
    }

    //
    //  Bounce vertical fonts.
    //
    if ((lpData->dwFlags & CF_NOVERTFONTS) &&
        (lplf->elfLogFont.lfFaceName[0] == TEXT('@'))
       )
    {
        return (TRUE);
    }

    //
    //  Bounce proportional fonts.
    //
    if ((lpData->dwFlags & CF_FIXEDPITCHONLY) &&
        (lplf->elfLogFont.lfPitchAndFamily & VARIABLE_PITCH))
    {
        return (TRUE);
    }

    //
    //  Bounce vector fonts.
    //
    if ((lpData->dwFlags & CF_NOVECTORFONTS) &&
        (lplf->elfLogFont.lfCharSet == OEM_CHARSET))
    {
        return (TRUE);
    }

    if (lpData->bPrinterFont)
    {
        nFontType |= PRINTER_FONTTYPE;
    }
    else
    {
        nFontType |= SCREEN_FONTTYPE;
    }

    //
    //  Test for a name collision.
    //
    iItem = CBFindString(lpData->hwndFamily, lplf->elfLogFont.lfFaceName);
    if (iItem >= 0)
    {
        lpItemData = (LPITEMDATA)SendMessage( lpData->hwndFamily,
                                              CB_GETITEMDATA,
                                              iItem,
                                              0L );
        if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            nOldType = lpItemData->nFontType;
            lpOldItemData = lpItemData;
        }
        else
        {
            nOldType = 0;
        }

        //
        //  If we don't want screen fonts, but do want printer fonts,
        //  the old font is a screen font and the new font is a
        //  printer font, take the new font regardless of other flags.
        //  Note that this means if a printer wants TRUETYPE fonts, it
        //  should enumerate them.
        //
        if (!(lpData->dwFlags & CF_SCREENFONTS)  &&
             (lpData->dwFlags & CF_PRINTERFONTS) &&
             (nFontType & PRINTER_FONTTYPE)      &&
             (nOldType & SCREEN_FONTTYPE))
        {
            nOldType = 0;                   // for setting nNewType below
            goto SetNewType;
        }

        if (CmpFontType(nFontType, nOldType) > 0)
        {
SetNewType:
            nNewType = nFontType;
            SendMessage( lpData->hwndFamily,
                         CB_INSERTSTRING,
                         iItem,
                         (LONG_PTR)(LPTSTR)lplf->elfLogFont.lfFaceName );
            SendMessage( lpData->hwndFamily,
                         CB_DELETESTRING,
                         iItem + 1,
                         0L );
        }
        else
        {
            nNewType = nOldType;
        }

        //
        //  Accumulate the printer/screen ness of these fonts.
        //
        nNewType |= (nFontType | nOldType) &
                    (SCREEN_FONTTYPE | PRINTER_FONTTYPE);

        lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
        if (!lpItemData)
        {
            return (FALSE);
        }
        lpItemData->pLogFont = 0L;

        lpItemData->nFontType = nNewType;
        SendMessage( lpData->hwndFamily,
                     CB_SETITEMDATA,
                     iItem,
                     (LONG_PTR)lpItemData );

        if (lpOldItemData)
        {
            LocalFree(lpOldItemData);
        }

        return (TRUE);
    }

    iItem = (int)SendMessage( lpData->hwndFamily,
                              CB_ADDSTRING,
                              0,
                              (LONG_PTR)(LPTSTR)lplf->elfLogFont.lfFaceName );
    if (iItem < 0)
    {
        return (FALSE);
    }

    lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
    if (!lpItemData)
    {
        return (FALSE);
    }
    lpItemData->pLogFont = 0L;

#ifdef WINNT
    if (lptm->ntmFlags & NTM_PS_OPENTYPE)
        nFontType |= PS_OPENTYPE_FONTTYPE;
    if (lptm->ntmFlags & NTM_TYPE1)
        nFontType |= TYPE1_FONTTYPE;
    if (lptm->ntmFlags & NTM_TT_OPENTYPE)
        nFontType |= TT_OPENTYPE_FONTTYPE;
#endif // WINNT

    lpItemData->nFontType = nFontType;

    SendMessage(lpData->hwndFamily, CB_SETITEMDATA, iItem, (LONG_PTR)lpItemData);

    lptm;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFontFamily
//
//  Fills the screen and/or printer font facenames into the font facenames
//  combobox depending on the CF_?? flags passed in.
//
//  cmb1 is the ID for the font facename combobox
//
//  Both screen and printer fonts are listed into the same combobox
//
//  Returns:   TRUE    if successful
//             FALSE   otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFontFamily(
    HWND hDlg,
    HDC hDC,
    DWORD dwEnumCode,
    UINT iCharset)
{
    ENUM_FONT_DATA data;
    int iItem, iCount;
    DWORD nFontType;
    TCHAR szMsg[120], szTitle[40];
    LPITEMDATA lpItemData;
    LOGFONT lf;

    data.hwndFamily = GetDlgItem(hDlg, cmb1);
    data.dwFlags = dwEnumCode;

    //
    //  This is a bit strange.  We have to get all the screen fonts
    //  so if they ask for the printer fonts we can tell which
    //  are really printer fonts.  This is so we don't list the
    //  vector and raster fonts as printer device fonts.
    //
    data.hDC = GetDC(NULL);
    data.bPrinterFont = FALSE;
    lf.lfFaceName[0] = CHAR_NULL;
    lf.lfCharSet = (dwEnumCode & CF_SELECTSCRIPT) ? iCharset : DEFAULT_CHARSET;
    EnumFontFamiliesEx( data.hDC,
                        &lf,
                        (FONTENUMPROC)FontFamilyEnumProc,
                        (LPARAM)&data,
                        0L );
    ReleaseDC(NULL, data.hDC);

    //
    //  List out printer font facenames.
    //
    if (dwEnumCode & CF_PRINTERFONTS)
    {
        data.hDC = hDC;
        data.bPrinterFont = TRUE;
        EnumFontFamiliesEx( hDC,
                            &lf,
                            (FONTENUMPROC)FontFamilyEnumProc,
                            (LPARAM)&data,
                            0L );
    }

    //
    //  Now we have to remove those screen fonts if they didn't
    //  ask for them.
    //
    if (!(dwEnumCode & CF_SCREENFONTS))
    {
        iCount = (int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L);

        for (iItem = iCount - 1; iItem >= 0; iItem--)
        {
            lpItemData = (LPITEMDATA)SendMessage( data.hwndFamily,
                                                  CB_GETITEMDATA,
                                                  iItem,
                                                  0L );
            if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
            {
                nFontType = lpItemData->nFontType;
            }
            else
            {
                nFontType = 0;
            }

            if ((nFontType & (SCREEN_FONTTYPE |
                              PRINTER_FONTTYPE)) == SCREEN_FONTTYPE)
            {
                SendMessage(data.hwndFamily, CB_DELETESTRING, iItem, 0L);
            }
        }
    }

    //
    //  For WYSIWYG mode we delete all the fonts that don't exist
    //  on the screen and the printer.
    //
    if (dwEnumCode & CF_WYSIWYG)
    {
        iCount = (int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L);

        for (iItem = iCount - 1; iItem >= 0; iItem--)
        {
            nFontType = ((LPITEMDATA)SendMessage( data.hwndFamily,
                                                  CB_GETITEMDATA,
                                                  iItem,
                                                  0L ))->nFontType;

            if ((nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE)) !=
                (SCREEN_FONTTYPE | PRINTER_FONTTYPE))
            {
                SendMessage(data.hwndFamily, CB_DELETESTRING, iItem, 0L);
            }
        }
    }

    if ((int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L) <= 0)
    {
        CDLoadString( g_hinst,
                    iszNoFontsTitle,
                    szTitle,
                    sizeof(szTitle) / sizeof(TCHAR));
        CDLoadString( g_hinst,
                    iszNoFontsMsg,
                    szMsg,
                    sizeof(szMsg) / sizeof(TCHAR));
        MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_ICONINFORMATION);

        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBAddSize
//
////////////////////////////////////////////////////////////////////////////

VOID CBAddSize(
    HWND hwnd,
    int pts,
    LPCHOOSEFONT lpcf)
{
    int iInd;
    TCHAR szSize[10];
    int count, test_size;
    LPITEMDATA lpItemData;

    //
    //  See if the size is limited.
    //
    if ((lpcf->Flags & CF_LIMITSIZE) &&
        ((pts > lpcf->nSizeMax) || (pts < lpcf->nSizeMin)))
    {
        return;
    }

    //
    //  Convert the point size to a string.
    //
    wsprintf(szSize, szPtFormat, pts);

    //
    //  Figure out where in the list the item should be added.
    //  All values should be in increasing order in the list box.
    //
    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
    test_size = -1;
    for (iInd = 0; iInd < count; iInd++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, iInd, 0L);
        if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            test_size = (int)lpItemData->nFontType;
        }
        else
        {
            test_size = 0;
        }

        if (pts <= test_size)
        {
            break;
        }
    }

    //
    //  Don't add duplicates.
    //
    if (pts == test_size)
    {
        return;
    }

    //
    //  Add the string and the associated item data to the list box.
    //
    iInd = (int) SendMessage(hwnd, CB_INSERTSTRING, iInd, (LPARAM)szSize);
    if (iInd >= 0)
    {
        lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
        if (!lpItemData)
        {
            return;
        }

        lpItemData->pLogFont = 0L;
        lpItemData->nFontType = (DWORD)pts;
        SendMessage(hwnd, CB_SETITEMDATA, iInd, (LONG_PTR)lpItemData);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CBAddChineseSize
//
////////////////////////////////////////////////////////////////////////////

VOID CBAddChineseSize(
    HWND hwnd,
    LPCHOOSEFONT lpcf)
{
    int ctr, iInd = 0;
    TCHAR szSize[10];
    LPITEMDATA lpItemData;

    //
    //  Look at each item in the Zihao structure to see if it should be
    //  added.
    //
    for (ctr = 0; ctr < NUM_ZIHAO; ctr++)
    {
        //
        //  See if the size is limited.
        //
        if ((lpcf->Flags & CF_LIMITSIZE) &&
            ((stZihao[ctr].size > lpcf->nSizeMax) ||
             (stZihao[ctr].size < lpcf->nSizeMin)))
        {
            continue;
        }

        //
        //  Convert the point size to a string.
        //
        wsprintf(szSize, TEXT("%s"), stZihao[ctr].name);

        //
        //  Add the string and the associated item data to the list box.
        //
        iInd = (int) SendMessage(hwnd, CB_INSERTSTRING, iInd, (LPARAM)szSize);
        if (iInd >= 0)
        {
            lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
            if (!lpItemData)
            {
                return;
            }

            lpItemData->pLogFont = 0L;
            lpItemData->nFontType = (DWORD)(stZihao[ctr].size * 10 +
                                            stZihao[ctr].sizeFr);
            SendMessage(hwnd, CB_SETITEMDATA, iInd, (LONG_PTR)lpItemData);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InsertStyleSorted
//
//  Sort styles by weight first, then by italics.
//
//  Returns the index of the place this was inserted.
//
////////////////////////////////////////////////////////////////////////////

int InsertStyleSorted(
    HWND hwnd,
    LPTSTR lpszStyle,
    LPLOGFONT lplf)
{
    int count, ctr;
    PLOGFONT plf;
    LPITEMDATA lpItemData;

    count = (int) SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

    for (ctr = 0; ctr < count; ctr++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, ctr, 0L);
        if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            plf = lpItemData->pLogFont;

            if (lplf->lfWeight < plf->lfWeight)
            {
                break;
            }
            else if (lplf->lfWeight == plf->lfWeight)
            {
                if (lplf->lfItalic && !plf->lfItalic)
                {
                    ctr++;
                }
                break;
            }
        }
    }

    return ((int)SendMessage(hwnd, CB_INSERTSTRING, ctr, (LONG_PTR)lpszStyle));
}


////////////////////////////////////////////////////////////////////////////
//
//  CBAddStyle
//
////////////////////////////////////////////////////////////////////////////

PLOGFONT CBAddStyle(
    HWND hwnd,
    LPTSTR lpszStyle,
    DWORD nFontType,
    LPLOGFONT lplf)
{
    int iItem;
    PLOGFONT plf;
    LPITEMDATA lpItemData;

    //
    //  Don't add duplicates.
    //
    if (CBFindString(hwnd, lpszStyle) >= 0)
    {
        return (NULL);
    }

    iItem = (int)InsertStyleSorted(hwnd, lpszStyle, lplf);
    if (iItem < 0)
    {
        return (NULL);
    }

    plf = (PLOGFONT)LocalAlloc(LMEM_FIXED, sizeof(LOGFONT));
    if (!plf)
    {
        SendMessage(hwnd, CB_DELETESTRING, iItem, 0L);
        return (NULL);
    }

    *plf = *lplf;

    lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
    if (!lpItemData)
    {
        LocalFree(plf);
        SendMessage(hwnd, CB_DELETESTRING, iItem, 0L);
        return (NULL);
    }

    lpItemData->pLogFont = plf;
    lpItemData->nFontType = nFontType;
    SendMessage(hwnd, CB_SETITEMDATA, iItem, (LONG_PTR)lpItemData);

    return (plf);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBAddScript
//
////////////////////////////////////////////////////////////////////////////

int CBAddScript(
    HWND hwnd,
    LPTSTR lpszScript,
    UINT iCharset)
{
    int iItem;
    LPITEMDATA lpItemData;

    //
    //  Don't add duplicates or empty strings.
    //
    if (!IsWindow(hwnd) || !IsWindowEnabled(hwnd) || (!*lpszScript) ||
        (CBFindString(hwnd, lpszScript) >= 0))
    {
        return (-1);
    }

    iItem = (int)SendMessage(hwnd, CB_ADDSTRING, 0, (LONG_PTR)(LPTSTR)lpszScript);
    if (iItem < 0)
    {
        return (-1);
    }

    lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
    if (!lpItemData)
    {
        SendMessage(hwnd, CB_DELETESTRING, iItem, 0L);
        return (-1);
    }

    lpItemData->pLogFont = 0L;
    lpItemData->nFontType = (DWORD)iCharset;
    SendMessage(hwnd, CB_SETITEMDATA, iItem, (LONG_PTR)lpItemData);

    return (iItem);
}


////////////////////////////////////////////////////////////////////////////
//
//  FillInMissingStyles
//
//  Generates simulated forms from those that we have.
//
//  reg -> bold
//  reg -> italic
//  bold || italic || reg -> bold italic
//
////////////////////////////////////////////////////////////////////////////

VOID FillInMissingStyles(
    HWND hwnd)
{
    PLOGFONT plf, plf_reg, plf_bold, plf_italic;
    DWORD nFontType;
    int ctr, count;
    BOOL bBold, bItalic, bBoldItalic;
    LPITEMDATA lpItemData;
    LOGFONT lf;

    bBold = bItalic = bBoldItalic = FALSE;
    plf_reg = plf_bold = plf_italic = NULL;

    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
    for (ctr = 0; ctr < count; ctr++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, ctr, 0L);
        if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            plf = lpItemData->pLogFont;
            nFontType = lpItemData->nFontType;
        }
        else
        {
            plf = NULL;
            nFontType = 0;
        }

        if ((nFontType & BOLD_FONTTYPE) && (nFontType & ITALIC_FONTTYPE))
        {
            bBoldItalic = TRUE;
        }
        else if (nFontType & BOLD_FONTTYPE)
        {
            bBold = TRUE;
            plf_bold = plf;
        }
        else if (nFontType & ITALIC_FONTTYPE)
        {
            bItalic = TRUE;
            plf_italic = plf;
        }
        else
        {
            plf_reg = plf;
        }
    }

    nFontType |= SIMULATED_FONTTYPE;

    if (!bBold && plf_reg)
    {
        lf = *plf_reg;
        lf.lfWeight = FW_BOLD;
        CBAddStyle(hwnd, szBold, (nFontType | BOLD_FONTTYPE), &lf);
    }

    if (!bItalic && plf_reg)
    {
        lf = *plf_reg;
        lf.lfItalic = TRUE;
        CBAddStyle(hwnd, szItalic, (nFontType | ITALIC_FONTTYPE), &lf);
    }
    if (!bBoldItalic && (plf_bold || plf_italic || plf_reg))
    {
        if (plf_italic)
        {
            plf = plf_italic;
        }
        else if (plf_bold)
        {
            plf = plf_bold;
        }
        else
        {
            plf = plf_reg;
        }

        lf = *plf;
        lf.lfItalic = (BYTE)TRUE;
        lf.lfWeight = FW_BOLD;
        CBAddStyle(hwnd, szBoldItalic, (nFontType | BOLD_FONTTYPE | ITALIC_FONTTYPE), &lf);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FillScalableSizes
//
////////////////////////////////////////////////////////////////////////////

VOID FillScalableSizes(
    HWND hwnd,
    LPCHOOSEFONT lpcf)
{
    if (g_bIsSimplifiedChineseUI)
    {
        CBAddChineseSize(hwnd, lpcf);
    }

    CBAddSize(hwnd, 8,  lpcf);
    CBAddSize(hwnd, 9,  lpcf);
    CBAddSize(hwnd, 10, lpcf);
    CBAddSize(hwnd, 11, lpcf);
    CBAddSize(hwnd, 12, lpcf);
    CBAddSize(hwnd, 14, lpcf);
    CBAddSize(hwnd, 16, lpcf);
    CBAddSize(hwnd, 18, lpcf);
    CBAddSize(hwnd, 20, lpcf);
    CBAddSize(hwnd, 22, lpcf);
    CBAddSize(hwnd, 24, lpcf);
    CBAddSize(hwnd, 26, lpcf);
    CBAddSize(hwnd, 28, lpcf);
    CBAddSize(hwnd, 36, lpcf);
    CBAddSize(hwnd, 48, lpcf);
    CBAddSize(hwnd, 72, lpcf);
}


////////////////////////////////////////////////////////////////////////////
//
//  FontStyleEnumProc
//
////////////////////////////////////////////////////////////////////////////
int FontStyleEnumProc(
    LPENUMLOGFONTEX lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData)
{
    int height, pts;
    TCHAR buf[10];


    if (!(nFontType & RASTER_FONTTYPE))
    {
        //
        //  Vector or TT font.
        //
        if (lpData->bFillSize &&
            (int)SendMessage(lpData->hwndSizes, CB_GETCOUNT, 0, 0L) == 0)
        {
            FillScalableSizes(lpData->hwndSizes, lpData->lpcf);
        }
    }
    else
    {
        height = lptm->tmHeight - lptm->tmInternalLeading;
        pts = GetPointString(buf, lpData->hDC, height);

        //
        //  Filter devices same size of multiple styles.
        //
        if (CBFindString(lpData->hwndSizes, buf) < 0)
        {
            CBAddSize(lpData->hwndSizes, pts, lpData->lpcf);
        }
    }

    //
    //  Keep the printer/screen bits from the family list here too.
    //
    nFontType |= (lpData->nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE));

#ifdef WINNT
    if (lptm->ntmFlags & NTM_PS_OPENTYPE)
        nFontType |= PS_OPENTYPE_FONTTYPE;
    if (lptm->ntmFlags & NTM_TYPE1)
        nFontType |= TYPE1_FONTTYPE;
    if (lptm->ntmFlags & NTM_TT_OPENTYPE)
        nFontType |= TT_OPENTYPE_FONTTYPE;
#endif // WINNT

    if (nFontType & TRUETYPE_FONTTYPE)
    {
        //
        //  If (lptm->ntmFlags & NTM_REGULAR)
        //
        if (!(lptm->ntmFlags & (NTM_BOLD | NTM_ITALIC)))
        {
            nFontType |= REGULAR_FONTTYPE;
        }

        if (lptm->ntmFlags & NTM_ITALIC)
        {
            nFontType |= ITALIC_FONTTYPE;
        }

        if (lptm->ntmFlags & NTM_BOLD)
        {
            nFontType |= BOLD_FONTTYPE;
        }

        //
        //  After the LOGFONT.lfFaceName there are 2 more names
        //     lfFullName[LF_FACESIZE * 2]
        //     lfStyle[LF_FACESIZE]
        //
        //  If the font has one of the standard style strings in English,
        //  use the localized string instead.
        //
        if (!lstrcmp(c_szBoldItalic, lplf->elfStyle) ||
            ((nFontType & BOLD_FONTTYPE) && (nFontType & ITALIC_FONTTYPE)))
        {
            CBAddStyle( lpData->hwndStyle,
                        szBoldItalic,
                        nFontType,
                        &lplf->elfLogFont);
        }
        else if (!lstrcmp(c_szRegular, lplf->elfStyle) ||
                 (nFontType & REGULAR_FONTTYPE))
        {
            CBAddStyle( lpData->hwndStyle,
                        szRegular,
                        nFontType,
                        &lplf->elfLogFont );
        }
        else if (!lstrcmp(c_szBold, lplf->elfStyle) ||
                  (nFontType & BOLD_FONTTYPE))
        {
            CBAddStyle( lpData->hwndStyle,
                        szBold,
                        nFontType,
                        &lplf->elfLogFont );
        }
        else if (!lstrcmp(c_szItalic, lplf->elfStyle) ||
                  (nFontType & ITALIC_FONTTYPE))
        {
            CBAddStyle( lpData->hwndStyle,
                        szItalic,
                        nFontType,
                        &lplf->elfLogFont);
        }
    }
    else
    {
        if ((lplf->elfLogFont.lfWeight >= FW_BOLD) && lplf->elfLogFont.lfItalic)
        {
            CBAddStyle( lpData->hwndStyle,
                        szBoldItalic,
                        (nFontType | BOLD_FONTTYPE | ITALIC_FONTTYPE),
                        &lplf->elfLogFont );
        }
        else if (lplf->elfLogFont.lfWeight >= FW_BOLD)
        {
            CBAddStyle( lpData->hwndStyle,
                        szBold,
                        (nFontType | BOLD_FONTTYPE),
                        &lplf->elfLogFont );
        }
        else if (lplf->elfLogFont.lfItalic)
        {
            CBAddStyle( lpData->hwndStyle,
                        szItalic,
                        (nFontType | ITALIC_FONTTYPE),
                        &lplf->elfLogFont );
        }
        else
        {
            CBAddStyle( lpData->hwndStyle,
                        szRegular,
                        (nFontType | REGULAR_FONTTYPE),
                        &lplf->elfLogFont );
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeFonts
//
////////////////////////////////////////////////////////////////////////////

VOID FreeFonts(
    HWND hwnd)
{
    int ctr, count;
    LPITEMDATA lpItemData;

    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

    for (ctr = 0; ctr < count; ctr++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, ctr, 0L);
        if (!IS_INTRESOURCE(lpItemData) && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            if (!IS_INTRESOURCE(lpItemData->pLogFont))
            {
                LocalFree((HANDLE)lpItemData->pLogFont);
            }
            LocalFree((HANDLE)lpItemData);
        }
        SendMessage(hwnd, CB_SETITEMDATA, ctr, 0L);
    }

    SendMessage(hwnd, CB_RESETCONTENT, 0, 0L);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeAllItemData
//
////////////////////////////////////////////////////////////////////////////

VOID FreeAllItemData(
    HWND hDlg,
    PFONTINFO pFI)
{
    HWND hwndTemp;

    if (hwndTemp = GetDlgItem(hDlg, cmb1))
    {
        FreeFonts(hwndTemp);
    }
    if (hwndTemp = GetDlgItem(hDlg, cmb2))
    {
        FreeFonts(hwndTemp);
    }
    if (hwndTemp = GetDlgItem(hDlg, cmb3))
    {
        FreeFonts(hwndTemp);
    }
    if (((pFI->ProcessVersion >= 0x40000) ||
         (pFI->pCF->Flags & CF_NOSCRIPTSEL)) &&
        (hwndTemp = GetDlgItem(hDlg, cmb5)))
    {
        FreeFonts(hwndTemp);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InitLF
//
//  Initalize a LOGFONT structure to some base generic regular type font.
//
////////////////////////////////////////////////////////////////////////////

VOID InitLF(
    LPLOGFONT lplf)
{
    HDC hdc;

    hdc = GetDC(NULL);
    lplf->lfEscapement = 0;
    lplf->lfOrientation = 0;
    lplf->lfCharSet = (BYTE) GetTextCharset(hdc);
    lplf->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lplf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lplf->lfQuality = DEFAULT_QUALITY;
    lplf->lfPitchAndFamily = DEFAULT_PITCH;
    lplf->lfItalic = 0;
    lplf->lfWeight = FW_NORMAL;
    lplf->lfStrikeOut = 0;
    lplf->lfUnderline = 0;
    lplf->lfWidth = 0;            // otherwise we get independant x-y scaling
    lplf->lfFaceName[0] = 0;
    lplf->lfHeight = -MulDiv( DEF_POINT_SIZE,
                              GetDeviceCaps(hdc, LOGPIXELSY),
                              POINTS_PER_INCH );
    ReleaseDC(NULL, hdc);
}

#ifdef MM_DESIGNVECTOR_DEFINED
////////////////////////////////////////////////////////////////////////////
//
//  FontMMAxesEnumProc
//
//  Gets all of the lower and upper bounds and default values of MM axes 
//  for the face we are enumerating. Sets the scrollbar parameters as well as
//  static controls and edit boxes accordingly.
//
////////////////////////////////////////////////////////////////////////////

int FontMMAxesEnumProc(
    LPENUMLOGFONTEXDV lplf,
    LPENUMTEXTMETRIC lpetm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData)
{
    UINT i;
    SCROLLINFO scri;
    RECT rcCtl;

        lpData->pDefaultDesignVector->dvNumAxes = (lpetm)->etmAxesList.axlNumAxes;
        lpData->pDefaultDesignVector->dvReserved = STAMP_DESIGNVECTOR;

    if (MAX_NUM_AXES < lpData->pDefaultDesignVector->dvNumAxes)
    {
//        MessageBox(lpData->hwndParent, TEXT("Cannot support so many axes"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
        lpData->pDefaultDesignVector->dvNumAxes = 0;  // this will get rid of all scroll bars
    }


    scri.cbSize = sizeof(SCROLLINFO);
    scri.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE;
    scri.nTrackPos = 0;

    rcCtl.left   = 0;
    rcCtl.top    = 0;
    rcCtl.right  = 287;
    rcCtl.bottom = (lpData->pDefaultDesignVector->dvNumAxes == 0)? 216 : 225+20+20*lpData->pDefaultDesignVector->dvNumAxes;
    MapDialogRect( lpData->hwndParent, &rcCtl);
    SetWindowPos ( lpData->hwndParent, NULL, 
                   0, 0, 
                   rcCtl.right, rcCtl.bottom,
                   SWP_NOMOVE | SWP_NOZORDER );

    rcCtl.left   = 0;
    rcCtl.top    = 0;
    rcCtl.right  = 205;
    rcCtl.bottom = 15+20*(lpetm)->etmAxesList.axlNumAxes;
    MapDialogRect( lpData->hwndParent, &rcCtl);
    SetWindowPos ( GetDlgItem(lpData->hwndParent, grp3), NULL, 
                   0, 0, 
                   rcCtl.right, rcCtl.bottom,
                   SWP_NOMOVE | SWP_NOZORDER );

    for (i=0; i<lpData->pDefaultDesignVector->dvNumAxes; i++)
    {
            ShowWindow      ( GetDlgItem(lpData->hwndParent, stc11 + i), SW_SHOW); // axName
            ShowWindow      ( GetDlgItem(lpData->hwndParent, stc18 + i), SW_SHOW); // axMin
            ShowWindow      ( GetDlgItem(lpData->hwndParent, stc25 + i), SW_SHOW); // axMax
            ShowWindow      ( GetDlgItem(lpData->hwndParent,  edt1 + i), SW_SHOW); // dvVal
            ShowWindow      ( GetDlgItem(lpData->hwndParent,  scr1 + i), SW_SHOW); // scrollbar
            //EnableScrollBar ( GetDlgItem(lpData->hwndParent,  scr1 + i), SB_CTL, ESB_ENABLE_BOTH);

            lpData->pDefaultDesignVector->dvValues[i] = (lplf)->elfDesignVector.dvValues[i];

            scri.nMin   = (lpetm)->etmAxesList.axlAxisInfo[i].axMinValue;
            scri.nPage  = 10;
            scri.nMax   = (lpetm)->etmAxesList.axlAxisInfo[i].axMaxValue + scri.nPage - 1;
            scri.nPos   = (lplf)->elfDesignVector.dvValues[i];
            SetScrollInfo(GetDlgItem(lpData->hwndParent, scr1 + i), SB_CTL, &scri, TRUE);

            SetDlgItemText( lpData->hwndParent, 
                            stc11 + i, 
                            (lpetm)->etmAxesList.axlAxisInfo[i].axAxisName);
            SetDlgItemInt ( lpData->hwndParent, 
                            stc18 + i, 
                            (lpetm)->etmAxesList.axlAxisInfo[i].axMinValue, 
                            TRUE);
            SetDlgItemInt ( lpData->hwndParent, 
                            stc25 + i, 
                            (lpetm)->etmAxesList.axlAxisInfo[i].axMaxValue, 
                            TRUE);
            SetDlgItemInt ( lpData->hwndParent, 
                            edt1 + i, 
                            (lplf)->elfDesignVector.dvValues[i], 
                            TRUE);
    }

    for (i=lpData->pDefaultDesignVector->dvNumAxes; i<MAX_NUM_AXES; i++)
    {
        HideDlgItem(lpData->hwndParent, stc11 + i); // axName
        HideDlgItem(lpData->hwndParent, stc18 + i); // axMin
        HideDlgItem(lpData->hwndParent, stc25 + i); // axMax
        HideDlgItem(lpData->hwndParent, edt1 + i);  // dvVal
        HideDlgItem(lpData->hwndParent, scr1 + i);  // scrollbar
        // EnableScrollBar ( GetDlgItem(lpData->hwndParent, scr1 + i), SB_CTL, ESB_DISABLE_BOTH);
    }       

    return TRUE;
}
#endif // MM_DESIGNVECTOR_DEFINED

////////////////////////////////////////////////////////////////////////////
//
//  FontScriptEnumProc
//
//  Gets all of the charsets for the face we are enumerating.
//
//  Fills in the script window if any, and sets the script property to
//  the correct charset.  If there is no window, then the first value
//  enumerated is set into the script, and contol returned.  If there is a
//  window, then the scripts will all be filled in.  If the correct value
//  is found, then that will be filled in. If its not found, such as when
//  the user changes from TimesNewRoman to WingDings, then the caller will
//  fill in the property to be the first one.
//
////////////////////////////////////////////////////////////////////////////

int FontScriptEnumProc(
    LPENUMLOGFONTEX lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData)
{
    int script = -1;

    //
    //  Need to check the charsets again as we have a face and are checking
    //  the family.
    //
    //  Bounce non WANSI fonts.
    //
    if ( (lpData->dwFlags & CF_SCRIPTSONLY) &&
         ((lplf->elfLogFont.lfCharSet == OEM_CHARSET) ||
          (lplf->elfLogFont.lfCharSet == SYMBOL_CHARSET)) )
    {
        return (TRUE);
    }

    if (lpData->hwndScript)
    {
        script = CBAddScript( lpData->hwndScript,
                              lplf->elfScript,
                              lplf->elfLogFont.lfCharSet );
    }
    else if (lpData->iCharset == FONT_INVALID_CHARSET)
    {
        lpData->iCharset = lplf->elfLogFont.lfCharSet;
    }

    if (lplf->elfLogFont.lfCharSet == lpData->cfdCharset)
    {
        lpData->iCharset = lplf->elfLogFont.lfCharSet;
        if (script >= 0)
        {
            SendMessage(lpData->hwndScript, CB_SETCURSEL, script, 0L);
        }
        else if (!(lpData->hwndScript))
        {
            return (FALSE);
        }
    }

    if (lpData->lpcf->Flags & CF_SELECTSCRIPT)
    {
        //
        //  We just wanted the first one to fill in the script box, now stop.
        //
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFontStylesAndSizes
//
//  Fills the point sizes combo box with the point sizes for the current
//  selection in the facenames combobox.
//
//  cmb1 is the ID for the font facename combobox.
//
//  Returns:   TRUE    if successful
//             FALSE   otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFontStylesAndSizes(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    BOOL bForceSizeFill)
{
    ENUM_FONT_DATA data;
    TCHAR szFace[LF_FACESIZE];
    int iSel;
    int iMapMode;
    SIZE ViewportExt, WindowExt;
    LOGFONT lf;
    LPITEMDATA lpItemData;

    FreeFonts(GetDlgItem(hDlg, cmb2));

    data.hwndStyle  = GetDlgItem(hDlg, cmb2);
    data.hwndSizes  = GetDlgItem(hDlg, cmb3);
    data.hwndScript = (pFI->ProcessVersion >= 0x40000)
                          ? GetDlgItem(hDlg, cmb5)
                          : NULL;
    data.dwFlags    = lpcf->Flags;
    data.lpcf       = lpcf;

    if (!IsWindow(data.hwndScript) || !IsWindowEnabled(data.hwndScript))
    {
        data.hwndScript = NULL;
    }

    iSel = (int)SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);
    if (iSel < 0)
    {
        //
        //  If we don't have a face name selected we will synthisize
        //  the standard font styles...
        //
        InitLF(&lf);
        CBAddStyle(data.hwndStyle, szRegular, REGULAR_FONTTYPE, &lf);
        lf.lfWeight = FW_BOLD;
        CBAddStyle(data.hwndStyle, szBold, BOLD_FONTTYPE, &lf);
        lf.lfWeight = FW_NORMAL;
        lf.lfItalic = TRUE;
        CBAddStyle(data.hwndStyle, szItalic, ITALIC_FONTTYPE, &lf);
        lf.lfWeight = FW_BOLD;
        CBAddStyle(data.hwndStyle, szBoldItalic, BOLD_FONTTYPE | ITALIC_FONTTYPE, &lf);
        FillScalableSizes(data.hwndSizes, lpcf);

        return (TRUE);
    }

    lpItemData = (LPITEMDATA)SendDlgItemMessage( hDlg,
                                                 cmb1,
                                                 CB_GETITEMDATA,
                                                 iSel,
                                                 0L );
    if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
    {
        data.nFontType  = lpItemData->nFontType;
    }
    else
    {
        data.nFontType  = 0;
    }

    data.bFillSize = TRUE;

    //
    // Free existing contents of font size combo box.
    // Also sends CB_RESETCONTENT to control.
    //
    FreeFonts(data.hwndSizes);

    SendMessage(data.hwndStyle, WM_SETREDRAW, FALSE, 0L);

    GetDlgItemText(hDlg, cmb1, szFace, sizeof(szFace) / sizeof(TCHAR));
    lstrcpy(lf.lfFaceName, szFace);

    //
    //  Fill in the script box FIRST. That way we have something to play with.
    //
    if (data.hwndScript)
    {
        SendMessage(data.hwndScript, CB_RESETCONTENT, 0, 0L);
    }
    data.iCharset   = FONT_INVALID_CHARSET;      // impossible charset value.
    data.cfdCharset = pFI->iCharset;             // pass into enum procs

    //
    //  If no script box exists, then we must get the appropriate charset
    //  based on the default ansi code page.
    //
    if (!data.hwndScript)
    {
        CHARSETINFO csi;
        DWORD dwCodePage = GetACP();

        if (TranslateCharsetInfo((DWORD*)dwCodePage, &csi, TCI_SRCCODEPAGE))
        {
            data.cfdCharset = csi.ciCharset;
        }
    }

    lf.lfCharSet = (lpcf->Flags & CF_SELECTSCRIPT)
                       ? pFI->iCharset
                       : DEFAULT_CHARSET;

    if (lpcf->Flags & CF_SCREENFONTS)
    {
        data.hDC = GetDC(NULL);
        data.bPrinterFont = FALSE;
        EnumFontFamiliesEx( data.hDC,
                            &lf,
                            (FONTENUMPROC)FontScriptEnumProc,
                            (LPARAM)&data,
                            0L );
        ReleaseDC(NULL, data.hDC);
    }

    if (lpcf->Flags & CF_PRINTERFONTS)
    {
        data.hDC = lpcf->hDC;
        data.bPrinterFont = TRUE;
        EnumFontFamiliesEx( lpcf->hDC,
                            &lf,
                            (FONTENUMPROC)FontScriptEnumProc,
                            (LPARAM)&data,
                            0L );
    }

    //
    //  Put it back into the main structure.
    //
    if ((data.iCharset == FONT_INVALID_CHARSET) && (data.hwndScript))
    {
        //
        //  There MUST be a script window, and we didn't find the charset
        //  we were looking for.
        //
        SendMessage(data.hwndScript, CB_SETCURSEL, 0, 0L);
        lpItemData = (LPITEMDATA)SendMessage( data.hwndScript,
                                              CB_GETITEMDATA,
                                              0,
                                              0L );
        if (lpItemData && (lpItemData != (LPITEMDATA)CB_ERR))
        {
            data.iCharset = lpItemData->nFontType;
        }
        else
        {
            data.iCharset = DEFAULT_CHARSET;
        }
    }
    lf.lfCharSet = pFI->iCharset = data.iCharset;

    if (lpcf->Flags & CF_SCREENFONTS)
    {
        data.hDC = GetDC(NULL);
        data.bPrinterFont = FALSE;
        EnumFontFamiliesEx( data.hDC,
                            &lf,
                            (FONTENUMPROC)FontStyleEnumProc,
                            (LPARAM)&data,
                            0L );
        ReleaseDC(NULL, data.hDC);
    }

    if (lpcf->Flags & CF_PRINTERFONTS)
    {
        //
        //  Save and restore the DC's mapping mode (and extents if needed)
        //  if it's been set by the app to something other than MM_TEXT.
        //
        if ((iMapMode = GetMapMode(lpcf->hDC)) != MM_TEXT)
        {
            if ((iMapMode == MM_ISOTROPIC) || (iMapMode == MM_ANISOTROPIC))
            {
                GetViewportExtEx(lpcf->hDC, &ViewportExt);
                GetWindowExtEx(lpcf->hDC, &WindowExt);
            }
            SetMapMode(lpcf->hDC, MM_TEXT);
        }

        data.hDC = lpcf->hDC;
        data.bPrinterFont = TRUE;
        EnumFontFamiliesEx( lpcf->hDC,
                            &lf,
                            (FONTENUMPROC)FontStyleEnumProc,
                            (LPARAM)&data,
                            0L );

        if (iMapMode != MM_TEXT)
        {
            SetMapMode(lpcf->hDC, iMapMode);
            if ((iMapMode == MM_ISOTROPIC) || (iMapMode == MM_ANISOTROPIC))
            {
                SetWindowExtEx( lpcf->hDC,
                                WindowExt.cx,
                                WindowExt.cy,
                                &WindowExt );
                SetViewportExtEx( lpcf->hDC,
                                  ViewportExt.cx,
                                  ViewportExt.cy,
                                  &ViewportExt );
            }
        }
    }

    if (!(lpcf->Flags & CF_NOSIMULATIONS))
    {
        FillInMissingStyles(data.hwndStyle);
    }

    SendMessage(data.hwndStyle, WM_SETREDRAW, TRUE, 0L);
    if (wWinVer < 0x030A)
    {
        InvalidateRect(data.hwndStyle, NULL, TRUE);
    }

    if (data.bFillSize)
    {
        SendMessage(data.hwndSizes, WM_SETREDRAW, TRUE, 0L);
        if (wWinVer < 0x030A)
        {
            InvalidateRect(data.hwndSizes, NULL, TRUE);
        }
    }

#ifdef MM_DESIGNVECTOR_DEFINED
    if (pFI->pfnCreateFontIndirectEx !=NULL)
    /* it's not safe to access the MM Axis information on a system where CreateFontIndirectEx is not defined */
    {
        if (lpcf->Flags & CF_SCREENFONTS)
        {
            data.hDC = GetDC(NULL);
            data.bPrinterFont = FALSE;
            data.hwndParent = hDlg;
            data.pDefaultDesignVector = &pFI->DefaultDesignVector;
            EnumFontFamiliesEx( data.hDC,
                                &lf,
                                (FONTENUMPROC)FontMMAxesEnumProc,
                                (LPARAM)&data,
                                0L );
            ReleaseDC(NULL, data.hDC);
        }

        if (lpcf->Flags & CF_PRINTERFONTS)
        {
            data.hDC = lpcf->hDC;
            data.bPrinterFont = TRUE;
            EnumFontFamiliesEx( lpcf->hDC,
                                &lf,
                                (FONTENUMPROC)FontMMAxesEnumProc,
                                (LPARAM)&data,
                                0L );
        }
    }
#endif // MM_DESIGNVECTOR_DEFINED

    ResetSampleFromScript(hDlg, data.hwndScript, pFI);

    if (lpcf->Flags & CF_NOSCRIPTSEL)
    {
        pFI->iCharset = DEFAULT_CHARSET;
    }

    bForceSizeFill;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FillColorCombo
//
//  Adds the color name strings to the colors combobox.
//
//  cmb4 is the ID for the color combobox.
//
//  The color rectangles are drawn later in response to a WM_DRAWITEM msg.
//
////////////////////////////////////////////////////////////////////////////

VOID FillColorCombo(
    HWND hDlg)
{
    int iT, item;
    TCHAR szT[CCHCOLORNAMEMAX];

    for (iT = 0; iT < CCHCOLORS; ++iT)
    {
        *szT = 0;
        CDLoadString(g_hinst, iszBlack + iT, szT, sizeof(szT) / sizeof(TCHAR));
        item = (int) SendDlgItemMessage( hDlg,
                                         cmb4,
                                         CB_INSERTSTRING,
                                         iT,
                                         (LPARAM)szT );
        if (item >= 0)
        {
            SendDlgItemMessage(hDlg, cmb4, CB_SETITEMDATA, item, rgbColors[iT]);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawSizeComboItem
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawSizeComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC;
    DWORD rgbBack, rgbText;
    TCHAR szFace[LF_FACESIZE + 10];
    HFONT hFont;

    hDC = lpdis->hDC;

    //
    //  We must first select the dialog control font.
    //
    if (hDlgFont)
    {
        hFont = SelectObject(hDC, hDlgFont);
    }

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LONG_PTR)(LPTSTR)szFace );

    ExtTextOut( hDC,
                lpdis->rcItem.left + GetSystemMetrics(SM_CXBORDER),
                lpdis->rcItem.top,
                ETO_OPAQUE,
                &lpdis->rcItem,
                szFace,
                lstrlen(szFace),
                NULL );
    //
    //  Reset font.
    //
    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawFamilyComboItem
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawFamilyComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC, hdcMem;
    DWORD rgbBack, rgbText;
    TCHAR szFace[LF_FACESIZE + 10];
    HBITMAP hOld;
    int dy, x;
    HFONT hFont;

    hDC = lpdis->hDC;

    //
    //  We must first select the dialog control font.
    //
    if (hDlgFont)
    {
        hFont = SelectObject(hDC, hDlgFont);
    }

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    // wsprintf(szFace, "%4.4X", LOWORD(lpdis->itemData));

    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LONG_PTR)(LPTSTR)szFace );
    ExtTextOut( hDC,
                lpdis->rcItem.left + DX_BITMAP,
                lpdis->rcItem.top,
                ETO_OPAQUE,
                &lpdis->rcItem,
                szFace,
                lstrlen(szFace),
                NULL );
    //
    //  Reset font.
    //
    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem)
    {
        if (hbmFont)
        {
            LPITEMDATA lpItemData = (LPITEMDATA)lpdis->itemData;

            hOld = SelectObject(hdcMem, hbmFont);

            if (!lpItemData)
            {
                goto SkipBlt;
            }

            if (lpItemData->nFontType & TRUETYPE_FONTTYPE)
            {
#ifdef WINNT
                if (lpItemData->nFontType & TT_OPENTYPE_FONTTYPE)
                    x = 2 * DX_BITMAP;
                else
#endif
                    x = 0;
            }
#ifdef WINNT
            else if (lpItemData->nFontType & PS_OPENTYPE_FONTTYPE)
            {
                x = 3 * DX_BITMAP;
            }
            else if (lpItemData->nFontType & TYPE1_FONTTYPE)
            {
                x = 4 * DX_BITMAP;
            }
#endif
            else
            {
                if ((lpItemData->nFontType & (PRINTER_FONTTYPE |
                                              DEVICE_FONTTYPE))
                  == (PRINTER_FONTTYPE | DEVICE_FONTTYPE))
                {
                    //
                    //  This may be a screen and printer font but
                    //  we will call it a printer font here.
                    //
                    x = DX_BITMAP;
                }
                else
                {
                    goto SkipBlt;
                }
            }

            //If it a mirrored DC then the bitmaps are order from right to left.
            if (IS_DC_RTL_MIRRORED(hdcMem)) {
                x = ((NUM_OF_BITMAP - 1) - (x / DX_BITMAP)) * DX_BITMAP;
            }

            dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - DY_BITMAP) / 2;

            BitBlt( hDC,
                    lpdis->rcItem.left,
                    lpdis->rcItem.top + dy,
                    DX_BITMAP,
                    DY_BITMAP,
                    hdcMem,
                    x,
                    lpdis->itemState & ODS_SELECTED ? DY_BITMAP : 0,
                    SRCCOPY );

SkipBlt:
            SelectObject(hdcMem, hOld);
        }
        DeleteDC(hdcMem);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawColorComboItem
//
//  Computes and draws the color combo items.
//  Called by main dialog function in response to a WM_DRAWITEM msg.
//
//  All color name strings have already been loaded and filled into
//  the combobox.
//
//  Returns:   TRUE    if succesful
//             FALSE   otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawColorComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC;
    HBRUSH hbr;
    int dx, dy;
    RECT rc;
    TCHAR szColor[CCHCOLORNAMEMAX];
    DWORD rgbBack, rgbText, dw;
    HFONT hFont;

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }
    ExtTextOut( hDC,
                lpdis->rcItem.left,
                lpdis->rcItem.top,
                ETO_OPAQUE,
                &lpdis->rcItem,
                NULL,
                0,
                NULL );

    //
    //  Compute coordinates of color rectangle and draw it.
    //
    dx = GetSystemMetrics(SM_CXBORDER);
    dy = GetSystemMetrics(SM_CYBORDER);
    rc.top    = lpdis->rcItem.top + dy;
    rc.bottom = lpdis->rcItem.bottom - dy;
    rc.left   = lpdis->rcItem.left + dx;
    rc.right  = rc.left + 2 * (rc.bottom - rc.top);

    dw = (DWORD) SendMessage(lpdis->hwndItem, CB_GETITEMDATA, lpdis->itemID, 0L);

    hbr = CreateSolidBrush(dw);
    if (!hbr)
    {
        return (FALSE);
    }

    hbr = SelectObject(hDC, hbr);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    DeleteObject(SelectObject(hDC, hbr));

    //
    //  Shift the color text right by the width of the color rectangle.
    //
    *szColor = 0;
    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LONG_PTR)(LPTSTR)szColor );

    //
    //  We must first select the dialog control font.
    //
    if (hDlgFont)
    {
        hFont = SelectObject(hDC, hDlgFont);
    }

    TextOut( hDC,
             2 * dx + rc.right,
             lpdis->rcItem.top,
             szColor,
             lstrlen(szColor) );

    //
    //  Reset font.
    //
    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawSampleText
//
//  Displays sample text with given attributes.  Assumes rcText holds the
//  coordinates of the area within the frame (relative to dialog client)
//  which text should be drawn in.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSampleText(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    HDC hDC)
{
    DWORD rgbText;
    DWORD rgbBack;
    int iItem;
    HFONT hFont, hTemp;
    TCHAR szSample[50];
#ifdef MM_DESIGNVECTOR_DEFINED
    ENUMLOGFONTEXDV elfdv;
#else
    LOGFONT lf;
#endif // MM_DESIGNVECTOR_DEFINED
    SIZE TextExtent;
    int len, x, y;
    TEXTMETRIC tm;
    BOOL bCompleteFont;
    RECT rcText;

#ifdef MM_DESIGNVECTOR_DEFINED
    bCompleteFont = FillInFontEx(hDlg, pFI, lpcf, &elfdv, FALSE);
    elfdv.elfEnumLogfontEx.elfLogFont.lfEscapement = 0;
    elfdv.elfEnumLogfontEx.elfLogFont.lfOrientation = 0;

    if (pFI->pfnCreateFontIndirectEx != NULL)
    {
        hFont = pFI->pfnCreateFontIndirectEx(&elfdv);
    }
    else
    {
        hFont = CreateFontIndirect((LPLOGFONT)&elfdv);
    }
#else
    bCompleteFont = FillInFont(hDlg, pFI, lpcf, &lf, FALSE);
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;

    hFont = CreateFontIndirect(&lf);
#endif // MM_DESIGNVECTOR_DEFINED
    if (!hFont)
    {
        return;
    }

    hTemp = SelectObject(hDC, hFont);

    rgbBack = SetBkColor(hDC, GetSysColor((pFI->ProcessVersion < 0x40000)
                                          ? COLOR_WINDOW
                                          : COLOR_3DFACE));

    if (lpcf->Flags & CF_EFFECTS)
    {
        iItem = (int)SendDlgItemMessage(hDlg, cmb4, CB_GETCURSEL, 0, 0L);
        if (iItem != CB_ERR)
        {
            rgbText = (DWORD) SendDlgItemMessage(hDlg, cmb4, CB_GETITEMDATA, iItem, 0L);
        }
        else
        {
            goto GetWindowTextColor;
        }
    }
    else
    {
GetWindowTextColor:
        rgbText = GetSysColor(COLOR_WINDOWTEXT);
    }

    rgbText = SetTextColor(hDC, rgbText);
 
    if (bCompleteFont)
    {
        if (GetUnicodeSampleText(hDC,szSample, sizeof(szSample) / sizeof(TCHAR)))           
        {
            //Empty Body
        }
        else
        {
            GetDlgItemText(hDlg, stc5, szSample, sizeof(szSample) / sizeof(TCHAR));
        }
    }
    else
    {
        szSample[0] = 0;
    }

    GetTextMetrics(hDC, &tm);

    len = lstrlen(szSample);
    GetTextExtentPoint(hDC, szSample, len, &TextExtent);
    TextExtent.cy = tm.tmAscent - tm.tmInternalLeading;

    rcText = pFI->rcText;

    if (pFI->ProcessVersion >= 0x40000)
    {
#ifdef UNICODE
        if (!IS16BITWOWAPP(lpcf) || !(lpcf->Flags & CF_ENABLEHOOK))
#endif
        {
            DrawEdge(hDC, &rcText, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        }
    }
#ifndef WINNT
    else
    {
        //
        //  We only care about conforming if we have no border.
        //
        FORWARD_WM_CTLCOLORSTATIC(hDlg, hDC, NULL, SendMessage);
    }
#endif

    if ((TextExtent.cx >= (rcText.right - rcText.left)) ||
        (TextExtent.cx <= 0))
    {
        x = rcText.left;
    }
    else
    {
        x = rcText.left + ((rcText.right - rcText.left) - TextExtent.cx) / 2;
    }

    y = min( rcText.bottom,
             rcText.bottom - ((rcText.bottom - rcText.top) - TextExtent.cy) / 2);

    ExtTextOut( hDC,
                x,
                y - (tm.tmAscent),
                ETO_OPAQUE | ETO_CLIPPED,
                &rcText,
                szSample,
                len,
                NULL );

    SetBkColor(hDC, rgbBack);
    SetTextColor(hDC, rgbText);

    if (hTemp)
    {
        DeleteObject(SelectObject(hDC, hTemp));
    }
}

#ifdef MM_DESIGNVECTOR_DEFINED
////////////////////////////////////////////////////////////////////////////
//
//  FillInFontEx
//
//  Fills in the ENUMLOGFONTEXDV structure based on the current selection.
//
//  bSetBits - if TRUE the Flags fields in the lpcf are set to indicate
//             what parts (face, style, size) are not selected
//
//  lpelfdv  - ENUMLOGFONTEXDV filled in upon return
//
//  Returns:   TRUE    if there was an unambiguous selection
//                     (the ENUMLOGFONTEXDV is filled in as per the enumeration)
//             FALSE   there was not a complete selection
//                     (fields set in the ENUMLOGFONTEXDV with default values)
//
////////////////////////////////////////////////////////////////////////////

BOOL FillInFontEx(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    LPENUMLOGFONTEXDV lpelfdv,
    BOOL bSetBits)
{
    UINT i;
    BOOL bTranslated;

    ZeroMemory(lpelfdv, sizeof(ENUMLOGFONTEXDV));

    lpelfdv->elfDesignVector.dvNumAxes = pFI->DefaultDesignVector.dvNumAxes;
    lpelfdv->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;  

    for (i=0; i<lpelfdv->elfDesignVector.dvNumAxes; i++)
           lpelfdv->elfDesignVector.dvValues[i] = GetDlgItemInt(hDlg, edt1 + i, &bTranslated, TRUE);

    return FillInFont(hDlg, pFI, lpcf, &lpelfdv->elfEnumLogfontEx.elfLogFont, bSetBits);
    // memcpy (&lpelfdv->elfEnumLogfontEx.elfLogFont, &lf, sizeof(LOGFONT));
}
#endif // MM_DESIGNVECTOR_DEFINED

////////////////////////////////////////////////////////////////////////////
//
//  FillInFont
//
//  Fills in the LOGFONT structure based on the current selection.
//
//  bSetBits - if TRUE the Flags fields in the lpcf are set to indicate
//             what parts (face, style, size) are not selected
//
//  lplf     - LOGFONT filled in upon return
//
//  Returns:   TRUE    if there was an unambiguous selection
//                     (the LOGFONT is filled in as per the enumeration)
//             FALSE   there was not a complete selection
//                     (fields set in the LOGFONT with default values)
//
////////////////////////////////////////////////////////////////////////////

BOOL FillInFont(
    HWND hDlg,
    PFONTINFO pFI,
    LPCHOOSEFONT lpcf,
    LPLOGFONT lplf,
    BOOL bSetBits)
{
    HDC hdc;
    int iSel, id, pts;
    LPITEMDATA lpItemData;
    DWORD nFontType;
    PLOGFONT plf;
    TCHAR szStyle[LF_FACESIZE];
    TCHAR szMessage[128];
    BOOL bFontComplete = TRUE;
    CHARSETINFO csi;
    DWORD dwCodePage = GetACP();

    if (!TranslateCharsetInfo((DWORD*)dwCodePage, &csi, TCI_SRCCODEPAGE))
    {
        csi.ciCharset = ANSI_CHARSET;
    }

    InitLF(lplf);

    GetDlgItemText( hDlg,
                    cmb1,
                    lplf->lfFaceName,
                    sizeof(lplf->lfFaceName) / sizeof(TCHAR) );
    if (CBFindString(GetDlgItem(hDlg, cmb1), lplf->lfFaceName) >= 0)
    {
        if (bSetBits)
        {
            lpcf->Flags &= ~CF_NOFACESEL;
        }
    }
    else
    {
        bFontComplete = FALSE;
        if (bSetBits)
        {
            lpcf->Flags |= CF_NOFACESEL;
        }
    }

    iSel = CBGetTextAndData( GetDlgItem(hDlg, cmb2),
                             szStyle,
                             sizeof(szStyle) / sizeof(TCHAR),
                             (PULONG_PTR)&lpItemData );
    if ((iSel >= 0) && lpItemData)
    {
        nFontType = lpItemData->nFontType;
        plf = lpItemData->pLogFont;
        *lplf = *plf;                       // copy the LOGFONT
        lplf->lfWidth = 0;                  // 1:1 x-y scaling
        if (!lstrcmp(lplf->lfFaceName, TEXT("Small Fonts")))
        {
            lplf->lfCharSet = (BYTE) csi.ciCharset;
        }
        if (bSetBits)
        {
            lpcf->Flags &= ~CF_NOSTYLESEL;
        }
    }
    else
    {
        //
        //  Even if the style is invalid, we still need the charset.
        //
        iSel = CBGetTextAndData( GetDlgItem(hDlg, cmb2),
                                 (LPTSTR)NULL,
                                 0,
                                 (PULONG_PTR)&lpItemData );
        if ((iSel >= 0) && lpItemData)
        {
            nFontType = lpItemData->nFontType;
            plf = lpItemData->pLogFont;
            *lplf = *plf;                   // copy the LOGFONT
            lplf->lfWidth = 0;              // 1:1 x-y scaling
            if (!lstrcmp(lplf->lfFaceName, TEXT("Small Fonts")) ||
                !lstrcmp(lplf->lfFaceName, TEXT("Lucida Sans Unicode")))
            {
                lplf->lfCharSet = (BYTE) csi.ciCharset;
            }
        }

        bFontComplete = FALSE;
        if (bSetBits)
        {
            lpcf->Flags |= CF_NOSTYLESEL;
        }
        nFontType = 0;
    }

    //
    //  Now make sure the size is in range; pts will be 0 if not.
    //
    GetPointSizeInRange(hDlg, lpcf, &pts, 0);

    hdc = GetDC(NULL);
    if (pts)
    {
        if (g_bIsSimplifiedChineseUI)
        {
            UINT iHeight;
            int iLogPixY = GetDeviceCaps(hdc, LOGPIXELSY);
            int ptsfr = pts % 10;          // fractional point size

            pts /= 10;                     // real point size
            iHeight = pts * iLogPixY;
            if (ptsfr)
            {
                iHeight += MulDiv(ptsfr, iLogPixY, 10);
            }
            lplf->lfHeight = -((int)((iHeight + POINTS_PER_INCH / 2) /
                                     POINTS_PER_INCH));
        }
        else
        {
            pts /= 10;
            lplf->lfHeight = -MulDiv( pts,
                                      GetDeviceCaps(hdc, LOGPIXELSY),
                                      POINTS_PER_INCH );
        }
        if (bSetBits)
        {
            lpcf->Flags &= ~CF_NOSIZESEL;
        }
    }
    else
    {
        lplf->lfHeight = -MulDiv( DEF_POINT_SIZE,
                                  GetDeviceCaps(hdc, LOGPIXELSY),
                                  POINTS_PER_INCH );
        bFontComplete = FALSE;
        if (bSetBits)
        {
            lpcf->Flags |= CF_NOSIZESEL;
        }
    }
    ReleaseDC(NULL, hdc);

    //
    //  And the attributes we control.
    //
    lplf->lfStrikeOut = (BYTE)IsDlgButtonChecked(hDlg, chx1);
    lplf->lfUnderline = (BYTE)IsDlgButtonChecked(hDlg, chx2);
    lplf->lfCharSet   = (BYTE) pFI->iCharset;

    if (nFontType != pFI->nLastFontType)
    {
        if (lpcf->Flags & CF_PRINTERFONTS)
        {
            if (nFontType & SIMULATED_FONTTYPE)
            {
                id = iszSynth;
            }
#ifdef WINNT
            else if (nFontType & TT_OPENTYPE_FONTTYPE)
            {
                id = iszTTOpenType;
            }
            else if (nFontType & PS_OPENTYPE_FONTTYPE)
            {
                id = iszPSOpenType;
            }
            else if (nFontType & TYPE1_FONTTYPE)
            {
                id = iszType1;
            }
#endif
            else if (nFontType & TRUETYPE_FONTTYPE)
            {
                id = iszTrueType;
            }
            else if ((nFontType & (PRINTER_FONTTYPE | DEVICE_FONTTYPE)) ==
                     (PRINTER_FONTTYPE | DEVICE_FONTTYPE))
            {
                //
                //  May be both screen and printer (ATM) but we'll just
                //  call this a printer font.
                //
                id = iszPrinterFont;
            }
            else if ((nFontType & (PRINTER_FONTTYPE | SCREEN_FONTTYPE)) ==
                     SCREEN_FONTTYPE)
            {
                id = iszGDIFont;
            }
            else
            {
                szMessage[0] = 0;
                goto SetText;
            }
            CDLoadString( g_hinst,
                        id,
                        szMessage,
                        sizeof(szMessage) / sizeof(TCHAR) );
SetText:
            SetDlgItemText(hDlg, stc6, szMessage);
        }
    }

    pFI->nLastFontType = nFontType;

    return (bFontComplete);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetLogFont
//
//  Sets the current selection based on the LOGFONT structure passed in.
//
//  lpcf     - CHOOSEFONT structure for the current dialog
//  lplf     - LOGFONT filled in upon return
//
//  Returns:   TRUE    if there was an unambiguous selection
//                     (the LOGFONT is filled in as per the enumeration)
//             FALSE   there was not a complete selection
//                     (fields set in the LOGFONT with default values)
//
////////////////////////////////////////////////////////////////////////////

BOOL SetLogFont(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPLOGFONT lplf)
{
    *(lpcf->lpLogFont) = *lplf;        // Copies data & FaceName

    FORWARD_WM_COMMAND( hDlg,
                        cmb1,
                        GetDlgItem(hDlg, cmb1),
                        CBN_SELCHANGE,
                        SendMessage );
    return (TRUE);
}

#ifdef MM_DESIGNVECTOR_DEFINED
BOOL SetLogFontEx(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPENUMLOGFONTEXDV lplf)
{
    memcpy(lpcf->lpLogFont, lplf, sizeof(ENUMLOGFONTEXDV));

    FORWARD_WM_COMMAND( hDlg,
                        cmb1,
                        GetDlgItem(hDlg, cmb1),
                        CBN_SELCHANGE,
                        SendMessage );
    return (TRUE);
}
#endif // MM_DESIGNVECTOR_DEFINED

////////////////////////////////////////////////////////////////////////////
//
//  TermFont
//
//  Release any data required by functions in this module.
//
////////////////////////////////////////////////////////////////////////////

VOID TermFont()
{
    if (hbmFont)
    {
        DeleteObject(hbmFont);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPointString
//
//  Converts font height into a string of digits representing point size.
//
//  Returns:   Size in points and fills in buffer with string
//
////////////////////////////////////////////////////////////////////////////

int GetPointString(
    LPTSTR buf,
    HDC hDC,
    int height)
{
    int pts;

    if (g_bIsSimplifiedChineseUI)
    {
        int ptsfr, iLogPixY, ctr;
        long lpts;
        BOOL IsZihao = FALSE;

        lpts = ((height < 0) ? -height : height) * 72;

        //
        //  Get real point size.
        //
        pts = (int)(lpts / (iLogPixY = GetDeviceCaps(hDC, LOGPIXELSY)));

        //
        //  Get fractional point size.
        //
        ptsfr = MulDiv((int)(lpts % iLogPixY), 10, iLogPixY);

        //
        //  See if it's Zihao.
        //
        for (ctr = 0; ctr < NUM_ZIHAO; ctr++)
        {
            if ((pts == stZihao[ctr].size) &&
                (abs(ptsfr - stZihao[ctr].sizeFr) <= 3))
            {
                IsZihao = TRUE;
                wsprintf(buf, TEXT("%s"), stZihao[ctr].name);
                break;
            }
        }
        if (!IsZihao)
        {
            pts = MulDiv((height < 0) ? -height : height, 72, iLogPixY);
            for (ctr = 0; ctr < NUM_ZIHAO; ctr++)
            {
                if ((pts == stZihao[ctr].size) && (!stZihao[ctr].sizeFr))
                {
                    IsZihao = TRUE;
                    wsprintf(buf, TEXT("%s"), stZihao[ctr].name);
                    break;
                }
            }
        }
        if (!IsZihao)
        {
            wsprintf(buf, szPtFormat, pts);
        }
    }
    else
    {
        pts = MulDiv( (height < 0) ? -height : height,
                      72,
                      GetDeviceCaps(hDC, LOGPIXELSY) );
        wsprintf(buf, szPtFormat, pts);
    }

    return (pts);
}


////////////////////////////////////////////////////////////////////////////
//
//  FlipColor
//
////////////////////////////////////////////////////////////////////////////

DWORD FlipColor(
    DWORD rgb)
{
    return ( RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)) );
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadBitmaps
//
//  This routine loads DIB bitmaps, and "fixes up" their color tables
//  so that we get the desired result for the device we are on.
//
//  This routine requires:
//      the DIB is a 16 color DIB authored with the standard windows colors
//      bright blue (00 00 FF) is converted to the background color
//      light grey  (C0 C0 C0) is replaced with the button face color
//      dark grey   (80 80 80) is replaced with the button shadow color
//
//  This means you can't have any of these colors in your bitmap.
//
////////////////////////////////////////////////////////////////////////////

#define BACKGROUND      0x000000FF          // bright blue
#define BACKGROUNDSEL   0x00FF00FF          // bright blue
#define BUTTONFACE      0x00C0C0C0          // bright grey
#define BUTTONSHADOW    0x00808080          // dark grey

HBITMAP LoadBitmaps(
    int id)
{
    HDC hdc;
    HANDLE h;
    DWORD *p;
    BYTE *lpBits;
    HANDLE hRes;
    LPBITMAPINFOHEADER lpBitmapInfo;
    int numcolors;
    DWORD rgbSelected;
    DWORD rgbUnselected;
    HBITMAP hbm;
    UINT cbBitmapSize;
    LPBITMAPINFOHEADER lpBitmapData;

    rgbSelected = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
    rgbUnselected = FlipColor(GetSysColor(COLOR_WINDOW));

    h = FindResource(g_hinst, MAKEINTRESOURCE(id), RT_BITMAP);
    hRes = LoadResource(g_hinst, h);

    //
    //  Lock the bitmap and get a pointer to the color table.
    //
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
    {
        return (FALSE);
    }

    //
    //  Lock the bitmap data and make a copy of it for the mask and the
    //  bitmap.
    //
    cbBitmapSize = SizeofResource(g_hinst, h);

    lpBitmapData = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, cbBitmapSize);

    if (!lpBitmapData)
    {
        return (NULL);
    }

    memcpy((TCHAR *)lpBitmapData, (TCHAR *)lpBitmapInfo, cbBitmapSize);

    p = (DWORD *)((LPTSTR)(lpBitmapData) + lpBitmapData->biSize);

    //
    //  Search for the Solid Blue entry and replace it with the current
    //  background RGB.
    //
    numcolors = 16;

    while (numcolors-- > 0)
    {
        if (*p == BACKGROUND)
        {
            *p = rgbUnselected;
        }
        else if (*p == BACKGROUNDSEL)
        {
            *p = rgbSelected;
        }
#if 0
        else if (*p == BUTTONFACE)
        {
            *p = FlipColor(GetSysColor(COLOR_BTNFACE));
        }
        else if (*p == BUTTONSHADOW)
        {
            *p = FlipColor(GetSysColor(COLOR_BTNSHADOW));
        }
#endif
        p++;
    }

    //
    //  First skip over the header structure.
    //
    lpBits = (BYTE *)(lpBitmapData + 1);

    //
    //  Skip the color table entries, if any.
    //
    lpBits += (1 << (lpBitmapData->biBitCount)) * sizeof(RGBQUAD);

    //
    //  Create a color bitmap compatible with the display device.
    //
    hdc = GetDC(NULL);
    hbm = CreateDIBitmap( hdc,
                          lpBitmapData,
                          (DWORD)CBM_INIT,
                          lpBits,
                          (LPBITMAPINFO)lpBitmapData,
                          DIB_RGB_COLORS );
    ReleaseDC(NULL, hdc);

    LocalFree(lpBitmapData);

    return (hbm);
}


////////////////////////////////////////////////////////////////////////////
//
//  LookUpFontSubs
//
//  Looks in the font substitute list for a real font name.
//
//  lpSubFontName  - substitute font name
//  lpRealFontName - real font name buffer
//
//  Returns:   TRUE    if lpRealFontName is filled in
//             FALSE   if not
//
////////////////////////////////////////////////////////////////////////////

BOOL LookUpFontSubs(
    LPTSTR lpSubFontName,
    LPTSTR lpRealFontName)
{
    LONG lResult;
    HKEY hKey;
    TCHAR szValueName[MAX_PATH];
    TCHAR szValueData[MAX_PATH];
    DWORD dwValueSize;
    DWORD dwIndex = 0;
    DWORD dwType, dwSize;


    //
    //  Open the font substitution's key.
    //
    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            KEY_FONT_SUBS,
                            0,
                            KEY_READ,
                            &hKey );

    if (lResult != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Loop through the values in the key
    //
    dwValueSize = MAX_PATH;
    dwSize = MAX_PATH;
    while (RegEnumValue( hKey,
                         dwIndex,
                         szValueName,
                         &dwValueSize,
                         NULL,
                         &dwType,
                         (LPBYTE)szValueData,
                         &dwSize ) == ERROR_SUCCESS)
    {
        //
        //  If the value name matches the requested font name, then
        //  copy the real font name to the output buffer.
        //
        if (!lstrcmpi(szValueName, lpSubFontName))
        {
            lstrcpy(lpRealFontName, szValueData);
            RegCloseKey(hKey);
            return (TRUE);
        }

        //
        //  Re-initialize for the next time through the loop.
        //
        dwValueSize = MAX_PATH;
        dwSize = MAX_PATH;
        dwIndex++;
    }

    //
    //  Clean up.
    //
    *lpRealFontName = CHAR_NULL;
    RegCloseKey(hKey);
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUnicodeSampleText
//
//  Gets the sample text for the font selected in the HDC
//
////////////////////////////////////////////////////////////////////////////
BOOL GetUnicodeSampleText(HDC hdc, LPTSTR lpString, int nMaxCount)
{

    FONTSIGNATURE sig;
    int i, j;
    int iLang = 0;
    int base = 0;
    int mask;


    if (!lpString || !nMaxCount)
    {
        return FALSE;
    }

    //Make sure return value is nulled
    lpString[0] = 0;


    //First Get the Font Signature
    GetTextCharsetInfo(hdc, &sig, 0);

    //Select the first unicode range supported by this font

    //For each of Unicode dwords
    for (i=0; i < 4; i++)
    {
        // See if a particular bit is set
        for (j=0; j < sizeof(DWORD) * 8 ; j++)
        {
             mask =  1 << j;

            if (sig.fsUsb[i] & mask)
            {
                //if set the get the language id for that bit
                iLang = base + j;
                goto LoadString;
            }
        }    
        base +=32;
    }

LoadString:
    //Do we have lang id and  string for that language ?
    if (iLang && LoadString(g_hinst, iszUnicode + iLang, lpString, nMaxCount))
    {
        return TRUE;
    }

    return FALSE;
}

/*========================================================================*/
/*                 Ansi->Unicode Thunk routines                           */
/*========================================================================*/

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ThunkChooseFontA2W
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkChooseFontA2W(
    PFONTINFO pFI)
{
    BOOL bRet;
    LPCHOOSEFONTW pCFW = pFI->pCF;
    LPCHOOSEFONTA pCFA = pFI->pCFA;

    pCFW->hwndOwner = pCFA->hwndOwner;
    pCFW->lCustData = pCFA->lCustData;

    pCFW->Flags = pCFA->Flags;

    //
    //  !!! hack, should not be based on flag value, since this could happen
    //  at any time.
    //
    if (pCFA->Flags & CF_INITTOLOGFONTSTRUCT)
    {
#ifdef MM_DESIGNVECTOR_DEFINED
        if (pFI->pfnCreateFontIndirectEx !=NULL)
        {
            ThunkEnumLogFontExDvA2W( (LPENUMLOGFONTEXDVA)pCFA->lpLogFont, 
                                    (LPENUMLOGFONTEXDVW)pCFW->lpLogFont);
        }
        else
        {
            ThunkLogFontA2W( pCFA->lpLogFont, pCFW->lpLogFont);
        }
#else
        ThunkLogFontA2W( pCFA->lpLogFont, pCFW->lpLogFont);
#endif // MM_DESIGNVECTOR_DEFINED
    }

    pCFW->hInstance = pCFA->hInstance;
    pCFW->lpfnHook = pCFA->lpfnHook;

    if (pCFW->Flags & CF_PRINTERFONTS)
    {
        pCFW->hDC = pCFA->hDC;
    }

    if (pCFW->Flags & CF_USESTYLE)
    {
        bRet = RtlAnsiStringToUnicodeString(pFI->pusStyle, pFI->pasStyle, FALSE);
    }

    pCFW->nSizeMin = pCFA->nSizeMin;
    pCFW->nSizeMax = pCFA->nSizeMax;
    pCFW->rgbColors = pCFA->rgbColors;

    pCFW->iPointSize = pCFA->iPointSize;
    pCFW->nFontType = pCFA->nFontType;

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkChooseFontW2A
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkChooseFontW2A(
    PFONTINFO pFI)
{
    BOOL bRet;
    LPCHOOSEFONTA pCFA = pFI->pCFA;
    LPCHOOSEFONTW pCFW = pFI->pCF;

#ifdef MM_DESIGNVECTOR_DEFINED
    if (pFI->pfnCreateFontIndirectEx !=NULL)
    {
        ThunkEnumLogFontExDvW2A( (LPENUMLOGFONTEXDVW)pCFW->lpLogFont, 
                                (LPENUMLOGFONTEXDVA)pCFA->lpLogFont);
    } 
    else
    {
        ThunkLogFontW2A( pCFW->lpLogFont, pCFA->lpLogFont);
    }
#else
    ThunkLogFontW2A( pCFW->lpLogFont, pCFA->lpLogFont);
#endif // MM_DESIGNVECTOR_DEFINED

    pCFA->hInstance = pCFW->hInstance;
    pCFA->lpfnHook = pCFW->lpfnHook;

    if (pCFA->Flags & CF_USESTYLE)
    {
        pFI->pusStyle->Length = (USHORT)((lstrlen(pFI->pusStyle->Buffer) + 1) * sizeof(WCHAR));
        bRet = RtlUnicodeStringToAnsiString(pFI->pasStyle, pFI->pusStyle, FALSE);
    }

    pCFA->Flags = pCFW->Flags;
    pCFA->nSizeMin = pCFW->nSizeMin;
    pCFA->nSizeMax = pCFW->nSizeMax;
    pCFA->rgbColors = pCFW->rgbColors;

    pCFA->iPointSize = pCFW->iPointSize;
    pCFA->nFontType = pCFW->nFontType;
    pCFA->lCustData = pCFW->lCustData;

    return (bRet);
}

#ifdef MM_DESIGNVECTOR_DEFINED
////////////////////////////////////////////////////////////////////////////
//
//  ThunkEnumLogFontExDvA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkEnumLogFontExDvA2W(
    LPENUMLOGFONTEXDVA lpELFDVA,
    LPENUMLOGFONTEXDVW lpELFDVW)
{
    UINT i;

    ThunkLogFontA2W(&lpELFDVA->elfEnumLogfontEx.elfLogFont,
                    &lpELFDVW->elfEnumLogfontEx.elfLogFont);

    MultiByteToWideChar( CP_ACP,
                         0,
                         lpELFDVA->elfEnumLogfontEx.elfFullName,
                         -1,
                         lpELFDVW->elfEnumLogfontEx.elfFullName,
                         LF_FULLFACESIZE );

    MultiByteToWideChar( CP_ACP,
                         0,
                         lpELFDVA->elfEnumLogfontEx.elfScript,
                         -1,
                         lpELFDVW->elfEnumLogfontEx.elfScript,
                         LF_FACESIZE );

    MultiByteToWideChar( CP_ACP,
                         0,
                         lpELFDVA->elfEnumLogfontEx.elfStyle,
                         -1,
                         lpELFDVW->elfEnumLogfontEx.elfStyle,
                         LF_FACESIZE );

    lpELFDVW->elfDesignVector.dvNumAxes = lpELFDVA->elfDesignVector.dvNumAxes;
    lpELFDVW->elfDesignVector.dvReserved = lpELFDVA->elfDesignVector.dvReserved;

    for (i=0; i<lpELFDVA->elfDesignVector.dvNumAxes; i++)
         lpELFDVW->elfDesignVector.dvValues[i] = lpELFDVA->elfDesignVector.dvValues[i];

}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkEnumLogFontExDvW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkEnumLogFontExDvW2A(
    LPENUMLOGFONTEXDVW lpELFDVW,
    LPENUMLOGFONTEXDVA lpELFDVA)
{
    BOOL fDefCharUsed;
    UINT i;

    if (lpELFDVW && lpELFDVA)
    {
        ThunkLogFontW2A(&lpELFDVW->elfEnumLogfontEx.elfLogFont,
                        &lpELFDVA->elfEnumLogfontEx.elfLogFont);

        WideCharToMultiByte( CP_ACP,
                             0,
                             lpELFDVW->elfEnumLogfontEx.elfFullName,
                             -1,
                             lpELFDVA->elfEnumLogfontEx.elfFullName,
                             LF_FULLFACESIZE,
                             NULL,
                             &fDefCharUsed );

        WideCharToMultiByte( CP_ACP,
                             0,
                             lpELFDVW->elfEnumLogfontEx.elfScript,
                             -1,
                             lpELFDVA->elfEnumLogfontEx.elfScript,
                             LF_FACESIZE,
                             NULL,
                             &fDefCharUsed );

        WideCharToMultiByte( CP_ACP,
                             0,
                             lpELFDVW->elfEnumLogfontEx.elfStyle,
                             -1,
                             lpELFDVA->elfEnumLogfontEx.elfStyle,
                             LF_FACESIZE,
                             NULL,
                             &fDefCharUsed );

        lpELFDVA->elfDesignVector.dvNumAxes = lpELFDVW->elfDesignVector.dvNumAxes;
        lpELFDVA->elfDesignVector.dvReserved = lpELFDVW->elfDesignVector.dvReserved;

        for (i=0; i<lpELFDVW->elfDesignVector.dvNumAxes; i++)
             lpELFDVA->elfDesignVector.dvValues[i] = lpELFDVW->elfDesignVector.dvValues[i];

    }
}
#endif // MM_DESIGNVECTOR_DEFINED

////////////////////////////////////////////////////////////////////////////
//
//  ThunkLogFontA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkLogFontA2W(
    LPLOGFONTA lpLFA,
    LPLOGFONTW lpLFW)
{
    lpLFW->lfHeight = lpLFA->lfHeight;
    lpLFW->lfWidth = lpLFA->lfWidth;
    lpLFW->lfEscapement = lpLFA->lfEscapement;
    lpLFW->lfOrientation = lpLFA->lfOrientation;
    lpLFW->lfWeight = lpLFA->lfWeight;
    lpLFW->lfItalic = lpLFA->lfItalic;
    lpLFW->lfUnderline = lpLFA->lfUnderline;
    lpLFW->lfStrikeOut = lpLFA->lfStrikeOut;
    lpLFW->lfCharSet = lpLFA->lfCharSet;
    lpLFW->lfOutPrecision = lpLFA->lfOutPrecision;
    lpLFW->lfClipPrecision = lpLFA->lfClipPrecision;
    lpLFW->lfQuality = lpLFA->lfQuality;
    lpLFW->lfPitchAndFamily = lpLFA->lfPitchAndFamily;

    SHAnsiToUnicode(lpLFA->lfFaceName,lpLFW->lfFaceName,LF_FACESIZE );
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkLogFontW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkLogFontW2A(
    LPLOGFONTW lpLFW,
    LPLOGFONTA lpLFA)
{

    if (lpLFW && lpLFA)
    {
        lpLFA->lfHeight = lpLFW->lfHeight;
        lpLFA->lfWidth = lpLFW->lfWidth;
        lpLFA->lfEscapement = lpLFW->lfEscapement;
        lpLFA->lfOrientation = lpLFW->lfOrientation;
        lpLFA->lfWeight = lpLFW->lfWeight;
        lpLFA->lfItalic = lpLFW->lfItalic;
        lpLFA->lfUnderline = lpLFW->lfUnderline;
        lpLFA->lfStrikeOut = lpLFW->lfStrikeOut;
        lpLFA->lfCharSet = lpLFW->lfCharSet;
        lpLFA->lfOutPrecision = lpLFW->lfOutPrecision;
        lpLFA->lfClipPrecision = lpLFW->lfClipPrecision;
        lpLFA->lfQuality = lpLFW->lfQuality;
        lpLFA->lfPitchAndFamily = lpLFW->lfPitchAndFamily;

        SHUnicodeToAnsi(lpLFW->lfFaceName,lpLFA->lfFaceName,LF_FACESIZE);
    }
}


#ifdef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  Ssync_ANSI_UNICODE_CF_For_WOW
//
//  Function to allow NT WOW to keep the ANSI & UNICODE versions of
//  the CHOOSEFONT structure in ssync as required by many 16-bit apps.
//  See notes for Ssync_ANSI_UNICODE_Struct_For_WOW() in dlgs.c.
//
////////////////////////////////////////////////////////////////////////////

VOID Ssync_ANSI_UNICODE_CF_For_WOW(
    HWND hDlg,
    BOOL f_ANSI_to_UNICODE)
{
    PFONTINFO pFI;

    if (pFI = (PFONTINFO)GetProp(hDlg, FONTPROP))
    {
        if (pFI->pCF && pFI->pCFA)
        {
            if (f_ANSI_to_UNICODE)
            {
                ThunkChooseFontA2W(pFI);
            }
            else
            {
                ThunkChooseFontW2A(pFI);
            }
        }
    }
}

#endif

#endif
