/* $Id: mbchars.c,v 1.3 2003/07/10 18:50:51 chorns Exp $
 *
 */
#include <windows.h>


/**********************************************************************
 * NAME                         PRIVATE
 *  IsInstalledCP@4
 *
 * RETURN VALUE
 *  TRUE if CodePage is installed in the system.
 */
static
BOOL
STDCALL
IsInstalledCP(UINT CodePage)
{
    /* FIXME */
    return TRUE;
}


/**********************************************************************
 * NAME                         EXPORTED
 *  MultiByteToWideChar@24
 *
 * ARGUMENTS
 *  CodePage
 *      CP_ACP      ANSI code page
 *      CP_MACCP    Macintosh code page
 *      CP_OEMCP    OEM code page
 *      (UINT)      Any installed code page
 *
 *  dwFlags
 *      MB_PRECOMPOSED
 *      MB_COMPOSITE
 *      MB_ERR_INVALID_CHARS
 *      MB_USEGLYPHCHARS
 *
 *  lpMultiByteStr
 *      Input buffer;
 *
 *  cchMultiByte
 *      Size of MultiByteStr, or -1 if MultiByteStr is
 *      NULL terminated;
 *
 *  lpWideCharStr
 *      Output buffer;
 *
 *  cchWideChar
 *      Size (in WCHAR unit) of WideCharStr, or 0
 *      if the caller just wants to know how large
 *      WideCharStr should be for a successful
 *      conversion.
 *
 * RETURN VALUE
 *  0 on error; otherwise the number of WCHAR written
 *  in the WideCharStr buffer.
 *
 * NOTE
 *  A raw converter for now. It assumes lpMultiByteStr is
 *  NEVER multi-byte (that is each input character is
 *  8-bit ASCII) and is ALWAYS NULL terminated.
 *  FIXME-FIXME-FIXME-FIXME
 *
 * @implemented
 */
INT
STDCALL
MultiByteToWideChar(
    UINT    CodePage,
    DWORD   dwFlags,
    LPCSTR  lpMultiByteStr,
    int     cchMultiByte,
    LPWSTR  lpWideCharStr,
    int     cchWideChar)
{
    int InStringLength = 0;
    PCHAR   r;
    PWCHAR  w;
    int cchConverted;

    /*
     * Check the parameters.
     */
    if (/* --- CODE PAGE --- */
        (  (CP_ACP != CodePage)
                && (CP_MACCP != CodePage)
                && (CP_OEMCP != CodePage)
                && (FALSE == IsInstalledCP(CodePage)) )
        /* --- FLAGS --- */
        || (dwFlags & ~(MB_PRECOMPOSED | MB_COMPOSITE |
                        MB_ERR_INVALID_CHARS | MB_USEGLYPHCHARS))
        /* --- INPUT BUFFER --- */
        || (NULL == lpMultiByteStr)  )
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*
     * Compute the input buffer length.
     */
    //if (-1 == cchMultiByte)
    if (cchMultiByte < 0)
    {
        InStringLength = lstrlen(lpMultiByteStr) + 1;
    }
    else
    {
        InStringLength = cchMultiByte;
    }
    /*
     * Does caller query for output
     * buffer size?
     */
    if (0 == cchWideChar)
    {
        //SetLastError(ERROR_SUCCESS); /* according to wine tests this shouldn't be touched on success.
        return InStringLength;
    }
    /*
     * Is space provided for the translated
     * string enough?
     */
    if (cchWideChar < InStringLength)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    /*
     * Raw 8- to 16-bit conversion.
     */
    for (cchConverted = 0,
        r = (PCHAR)lpMultiByteStr,
        w = (PWCHAR)lpWideCharStr;

        cchConverted < InStringLength;

        r++,
        w++,
        cchConverted++)
    {
        *w = (WCHAR)*r;
    }
    /*
     * Return how many characters we
     * wrote in the output buffer.
     */
    //SetLastError(ERROR_SUCCESS); /* according to wine tests this shouldn't be touched on success.
    return cchConverted;
}


/**********************************************************************
 * NAME                         EXPORTED
 *  WideCharToMultiByte@32
 * 
 *  Not yet implemented complete (without NLS so far)
 *
 * ARGUMENTS
 *  CodePage
 *      CP_ACP ANSI code page 
 *      CP_MACCP Macintosh code page 
 *      CP_OEMCP OEM code page 
 *      CP_SYMBOL Symbol code page (42) 
 *      CP_THREAD_ACP Current thread's ANSI code page 
 *      CP_UTF7 Translate using UTF-7 
 *      CP_UTF8 Translate using UTF-8 
 *      (UINT)      Any installed code page
 *
 *  dwFlags
 *      WC_NO_BEST_FIT_CHARS    
 *      WC_COMPOSITECHECK Convert composite characters to precomposed characters. 
 *      WC_DISCARDNS Discard nonspacing characters during conversion. 
 *      WC_SEPCHARS Generate separate characters during conversion. This is the default conversion behavior. 
 *      WC_DEFAULTCHAR Replace exceptions with the default character during conversion. 
 *
 *  lpWideCharStr 
 *      Points to the wide-character string to be converted. 
 *
 *  cchWideChar
 *      Size (in WCHAR unit) of WideCharStr, or 0
 *      if the caller just wants to know how large
 *      WideCharStr should be for a successful
 *      conversion.
 *  lpMultiByteStr 
 *      Points to the buffer to receive the translated string. 
 *  cchMultiByte 
 *      Specifies the size in bytes of the buffer pointed to by the 
 *      lpMultiByteStr parameter. If this value is zero, the function 
 *      returns the number of bytes required for the buffer. 
 *  lpDefaultChar 
 *      Points to the character used if a wide character cannot be 
 *      represented in the specified code page. If this parameter is 
 *      NULL, a system default value is used. 
        FIXME: ignored
 *  lpUsedDefaultChar 
 *      Points to a flag that indicates whether a default character was used. 
 *      This parameter may be NULL. 
        FIXME: allways set to FALSE.
 *
 *
 *
 * RETURN VALUE
 *  0 on error; otherwise the number of bytes written
 *  in the lpMultiByteStr buffer. Or the number of
 *  bytes needed for the lpMultiByteStr buffer if cchMultiByte is zero.
 *
 * NOTE
 *  A raw converter for now. It just cuts off the upper 9 Bit.
 *  So the MBCS-string does not contain any LeadCharacters
 *  FIXME - FIXME - FIXME - FIXME
 *
 * @implemented
 */
int
STDCALL
WideCharToMultiByte(
    UINT    CodePage,
    DWORD   dwFlags,
    LPCWSTR lpWideCharStr,
    int     cchWideChar,
    LPSTR   lpMultiByteStr,
    int     cchMultiByte,
    LPCSTR  lpDefaultChar,
    LPBOOL  lpUsedDefaultChar
    )
{
    int wi, di;  // wide counter, dbcs byte count

    /*
     * Check the parameters.
     */
    if (    /* --- CODE PAGE --- */
        (   (CP_ACP != CodePage)
            && (CP_MACCP != CodePage)
            && (CP_OEMCP != CodePage)
            && (CP_SYMBOL != CodePage)
            && (CP_THREAD_ACP != CodePage)
            && (CP_UTF7 != CodePage)
            && (CP_UTF8 != CodePage)
            && (FALSE == IsInstalledCP (CodePage))
            )
        /* --- FLAGS --- */
        || (dwFlags & ~(/*WC_NO_BEST_FIT_CHARS
                |*/ WC_COMPOSITECHECK
                | WC_DISCARDNS
                | WC_SEPCHARS
                | WC_DEFAULTCHAR
                )
            )
        /* --- INPUT BUFFER --- */
        || (NULL == lpWideCharStr)
        )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // for now, make no difference but only convert cut the characters to 7Bit
    //if (cchWideChar == -1) // assume its a 0-terminated str
    if (cchWideChar < 0) // assume its a 0-terminated str
    {           // and determine its length
//        for (cchWideChar=0; lpWideCharStr[cchWideChar]!=0; cchWideChar++)
//            cchWideChar++;
        for (cchWideChar = 0; lpWideCharStr[cchWideChar] != 0; cchWideChar++) {
        }
        cchWideChar++; // length includes the null terminator
    }

    // user wants to determine needed space
    if (cchMultiByte == 0) 
    {
        //SetLastError(ERROR_SUCCESS); /* according to wine tests this shouldn't be touched on success.
        return cchWideChar;         // FIXME: determine correct.
    }
    // the lpWideCharStr is cchWideChar characters long.
    for (wi=0, di=0; wi<cchWideChar && di<cchMultiByte; ++wi, ++di)
    {
        // Flag and a not displayable char    FIXME
        /*if( (dwFlags&WC_NO_BEST_FIT_CHARS) && (lpWideCharStr[wi] >127) ) 
        {
            lpMultiByteStr[di]=
            *lpUsedDefaultChar = TRUE;

        }*/
        // FIXME
        // just cut off the upper 9 Bit, since vals>=128 mean LeadByte.
        lpMultiByteStr[di] = lpWideCharStr[wi] & 0x007F;
    }
    // has MultiByte exceeded but Wide is still in the string?
    if (wi < cchWideChar && di >= cchMultiByte)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    // else return # of bytes wirtten to MBCSbuffer (di)
    //SetLastError(ERROR_SUCCESS); /* according to wine tests this shouldn't be touched on success.
    // FIXME: move that elsewhere
    if (lpUsedDefaultChar != NULL) *lpUsedDefaultChar = FALSE; 
    return di;
}

/* EOF */
