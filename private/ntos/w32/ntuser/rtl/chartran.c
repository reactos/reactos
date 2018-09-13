/****************************** Module Header ******************************\
* Module Name: chartran.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the routines for translating ACP characters
* to Unicode and translating Unicode characters to ACP characters.
* NOTE: The ACP is the currently installed 8-bit code page.
*
*
* History:
* 08-01-91 GregoryW      Created.
* 05-14-92 GregoryW      Modified to use the Rtl translation routines.
\***************************************************************************/

extern __declspec(dllimport) USHORT NlsAnsiCodePage;

#ifndef THREAD_CODEPAGE
#ifdef _USERK_
#define THREAD_CODEPAGE() (PtiCurrent()->pClientInfo->CodePage)
#else
#define THREAD_CODEPAGE() (GetClientInfo()->CodePage)
#endif // _USERK_
#endif


#define IS_ACP(cp) (((cp) == NlsAnsiCodePage) || ((cp) == CP_ACP))

/***************************************************************************\
* WCSToMBEx (API)
*
* Convert a wide-character (Unicode) string to MBCS (ANSI) string.
*
* nAnsiChar > 0 indicates the number of bytes to allocate to store the
*    ANSI string (if bAllocateMem == TRUE) or the size of the buffer
*    pointed to by *pAnsiString (bAllocateMem == FALSE).
*
* nAnsiChar == -1 indicates that the necessary number of bytes be allocated
*    to hold the translated string.  bAllocateMem must be set to TRUE in
*    this case.
*
* Return value
*   Success: number of characters in the output string
*        If bAllocateMem was TRUE, then FreeAnsiString() may be
*        used to free the allocated memory at *ppAnsiString.
*   Failure: 0 means failure
*        (Any buffers allocated by this routine are freed)
*
* History:
*  1992-??-?? GregoryW   Created
*  1993-01-07 IanJa      fix memory leak on error case.
\***************************************************************************/

int
WCSToMBEx(
    WORD wCodePage,
    LPCWSTR pUnicodeString,
    int cchUnicodeString,
    LPSTR *ppAnsiString,
    int nAnsiChar,
    BOOL bAllocateMem)
{
    ULONG nCharsInAnsiString;
#ifdef _USERK_
    INT iCharsInAnsiString;
#endif // _USERK_

    if (nAnsiChar == 0 || cchUnicodeString == 0 || pUnicodeString == NULL) {
        return 0;      // nothing to translate or nowhere to put it
    }

    /*
     * Adjust the cchUnicodeString value.  If cchUnicodeString == -1 then the
     * string pointed to by pUnicodeString is NUL terminated so we
     * count the number of bytes.  If cchUnicodeString < -1 this is an
     * illegal value so we return FALSE.  Otherwise, cchUnicodeString is
     * set and requires no adjustment.
     */
    if (cchUnicodeString == -1) {
        cchUnicodeString = (wcslen(pUnicodeString) + 1);
    } else if (cchUnicodeString < -1) {
        return 0;     // illegal value
    }

    /*
     * Adjust the nAnsiChar value.  If nAnsiChar == -1 then we pick a
     * value based on cchUnicodeString to hold the converted string.  If
     * nAnsiChar < -1 this is an illegal value so we return FALSE.
     * Otherwise, nAnsiChar is set and requires no adjustment.
     */
    if (nAnsiChar == -1) {
        if (bAllocateMem == FALSE) {
            return 0;  // no destination
        }
        nAnsiChar = cchUnicodeString * DBCS_CHARSIZE;
    } else if (nAnsiChar < -1) {
        return 0;     // illegal value
    }

    if (bAllocateMem) {
        /*
         * We need to allocate memory to hold the translated string.
         */
        *ppAnsiString = (LPSTR)UserRtlAllocMem(nAnsiChar);
        if (*ppAnsiString == NULL) {
            return 0;
        }
    }

    /*
     * translate Unicode string pointed to by pUnicodeString into
     * ANSI and store in location pointed to by pAnsiString.  We
     * stop translating when we fill up the ANSI buffer or reach
     * the end of the Unicode string.
     */

    /*
     * if the target multibyte codepage is eqaul to ACP, Call faster Rtl function.
     */
    if (IS_ACP(wCodePage)) {

        NTSTATUS Status;

        Status = RtlUnicodeToMultiByteN(
                        (PCH)*ppAnsiString,
                        nAnsiChar,
                        &nCharsInAnsiString,
                        (PWCH)pUnicodeString,
                        cchUnicodeString * sizeof(WCHAR));
        /*
         * If the ansi buffer is too small, RtlUnicodeToMultiByteN()
         * returns STATUS_BUFFER_OVERFLOW. In this case, the function
         * put as many ansi characters as specified in the buffer and
         *  returns the number by chacacters(in bytes) written. We would
         * like to return the actual byte  count written in the ansi
         * buffer rather than returnning 0 since callers of this function
         * don't expect to be returned 0 in most case.
         */

        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
            if (bAllocateMem) {
                UserRtlFreeMem(*ppAnsiString);
            }
            return 0;   // translation failed
        }

        return (int)nCharsInAnsiString;

    } else {

#ifdef _USERK_
        /*
         * Call GRE to convert string to Unicode. (Kernel mode)
         */

        iCharsInAnsiString = EngWideCharToMultiByte(
                                 (UINT)wCodePage,
                                 (LPWSTR)pUnicodeString,
                                 cchUnicodeString * sizeof(WCHAR),
                                 (LPSTR)*ppAnsiString,
                                 nAnsiChar);

        nCharsInAnsiString = (iCharsInAnsiString == -1) ? 0 :
                                                          (ULONG) iCharsInAnsiString;

#else
        /*
         * Call NLS API (Kernel32) to convert string to Unicode. (User mode)
         */
        nCharsInAnsiString = WideCharToMultiByte(
                                 (UINT)wCodePage, 0,
                                 (LPCWSTR)pUnicodeString,
                                 cchUnicodeString,
                                 (LPSTR)*ppAnsiString,
                                 nAnsiChar,
                                 NULL, NULL);
#endif // _USERK_

        if (nCharsInAnsiString == 0) {
            if (bAllocateMem) {
                UserRtlFreeMem(*ppAnsiString);
            }
        }

        return (int)nCharsInAnsiString;
    }
}

// Returns number of character converted

int MBToWCSEx(
    WORD wCodePage,
    LPCSTR pAnsiString,
    int nAnsiChar,
    LPWSTR *ppUnicodeString,
    int cchUnicodeString,
    BOOL bAllocateMem)
{
    ULONG nBytesInUnicodeString;

    if (nAnsiChar == 0 || cchUnicodeString == 0 || pAnsiString == NULL) {
        return 0;      // nothing to translate or nowhere to put it
    }

    /*
     * Adjust the nAnsiChar value.  If nAnsiChar == -1 then the
     * string pointed to by pAnsiString is NUL terminated so we
     * count the number of bytes.  If nAnsiChar < -1 this is an
     * illegal value so we return FALSE.  Otherwise, nAnsiChar is
     * set and requires no adjustment.
     */

#ifdef _USERK_
    UserAssert(nAnsiChar >= USER_AWCONV_COUNTSTRINGSZ);
#endif
    if (nAnsiChar < 0) {

        /*
         *  Bug 268035 - joejo
         *  Need to fail if the count is a negative number less than -2!
         */
        if (nAnsiChar < USER_AWCONV_COUNTSTRINGSZ) {
            return 0;
        }

#if (USER_AWCONV_COUNTSTRING != -1 || USER_AWCONV_COUNTSTRINGSZ != -2)
#error USER_AWCONV_COUNTSTRING or USER_AWCONV_COUNTSTRINGSZ has unexpected value.
#endif
        /* HACK HACK HACK
         * If nAnsiChar is -1 (USER_AWCONV_COUNTSTRING), nAnsiChar length will be strlen() + 1,
         * to allocate the memory including trailing \0: this is compatible to the original code.
         * If nAnsiCahr is -2 (USER_AWCONV_COUNTSTRINGSZ), memory for trailing \0 will not be needed,
         * so memory allocation is optimized and the return value would be same as strlen().
         */
        nAnsiChar = strlen(pAnsiString) + 2 + nAnsiChar;   // don't forget the NUL if nAnsiChar == -1

        if (nAnsiChar == 0) {
            return 0;
        }
    }

    /*
     * Adjust the cchUnicodeString value.  If cchUnicodeString == -1 then we
     * pick a value based on nAnsiChar to hold the converted string.  If
     * cchUnicodeString < -1 this is an illegal value so we return FALSE.
     * Otherwise, cchUnicodeString is set and requires no adjustment.
     */
    if (cchUnicodeString == -1) {
        if (bAllocateMem == FALSE) {
            return 0;    // no destination
        }
        cchUnicodeString = nAnsiChar;
    } else if (cchUnicodeString < -1) {
        return 0;     // illegal value
    }

    if (bAllocateMem) {
        *ppUnicodeString = (LPWSTR)UserRtlAllocMem(cchUnicodeString*sizeof(WCHAR));
        if (*ppUnicodeString == NULL) {
            return 0;    // allocation failed
        }
    }

    /*
     * if codepage is CP_ACP, We will call faster RtlXXX function.
     */
    if (IS_ACP(wCodePage)) {
        /*
         * translate ANSI string pointed to by pAnsiString into Unicode
         * and store in location pointed to by pUnicodeString.  We
         * stop translating when we fill up the Unicode buffer or reach
         * the end of the ANSI string.
         */
        if (!NT_SUCCESS(RtlMultiByteToUnicodeN(
                            (PWCH)*ppUnicodeString,
                            cchUnicodeString * sizeof(WCHAR),
                            &nBytesInUnicodeString,
                            (PCH)pAnsiString,
                            nAnsiChar
                            ))) {
            if (bAllocateMem) {
                UserRtlFreeMem(*ppUnicodeString);
            }
            return 0;   // translation failed
        }

        return (int)(nBytesInUnicodeString / sizeof(WCHAR));

    } else {
        /*
         * if wCodePage is not ACP, Call NLS API.
         */
        ULONG nCharsInUnicodeString;

#ifdef _USERK_

        /*
         * I believe we will never hit this code which is why I am
         * adding this assert.  [gerritv] 5-21-96
         */
#define SHOULD_NOT_REACH_HERE   0
        UserAssert(SHOULD_NOT_REACH_HERE);
#undef  SHOULD_NOT_REACH_HERE
        return 0;

#if 0   // FYI: old code
        INT   iCharsInUnicodeString;

        /*
         * Call GRE to convert string to Unicode. (Kernel mode)
         * I believe we will never hit this code which is why I am
         * adding this assert.  [gerritv] 5-21-96
         */

        UserAssert(0);

        iCharsInUnicodeString = EngMultiByteToWideChar(
                                    (UINT)wCodePage,
                                    (LPWSTR)*ppUnicodeString,
                                    (int)cchUnicodeString * sizeof(WCHAR),
                                    (LPSTR)pAnsiString,
                                    (int)nAnsiChar);

        nCharsInUnicodeString = (iCharsInUnicodeString == -1) ? 0 :
                                                          (ULONG) iCharsInUnicodeString;
#endif

#else
        /*
         * Call NLS API (Kernel32) to convert string to Unicode. (User mode)
         */
        nCharsInUnicodeString = MultiByteToWideChar(
                                    (UINT)wCodePage, 0,
                                    (LPCSTR)pAnsiString,
                                    (int)nAnsiChar,
                                    (LPWSTR)*ppUnicodeString,
                                    (int)cchUnicodeString);
#endif // _USERK_

        if (nCharsInUnicodeString == 0) {
            if (bAllocateMem) {
                UserRtlFreeMem(*ppUnicodeString);
            }
        }

        return (int)nCharsInUnicodeString;
    }

}


/**************************************************************************\
* RtlWCSMessageWParmCharToMB
*
* Converts a Wide Character to a Multibyte character; in place
* Returns the number of characters converted or zero if failure
*
* 11-Feb-1992  JohnC    Created
\**************************************************************************/

BOOL RtlWCSMessageWParamCharToMB(DWORD msg, WPARAM *pWParam)
{
    DWORD dwAnsi;
    NTSTATUS Status;
    WORD CodePage;
    int nbWch;

#ifdef FE_SB // RtlWCSMessageWParamCharToMB()
    //
    // Format of *pWParam here...
    //
    // LOWORD(*pWParam) = Unicode CodePoint...
    // HIWORD(*pWParam) = Has some information for DBCS messaging
    //                    (ex. WPARAM_IR_DBCSCHAR)
    //
    // Then we need to convert ONLY loword of wParam to Unicode...
    //
#endif // FE_SB
#ifndef FE_SB
    // NtBug #3135 (Closed 02/04/93)
    // Publisher Posts WM_CHAR messages with wParam > 0xFF (not a valid ANSI char)!
    //
    // It does this to disable TranslateAccelerator for that char.
    // MSPub's winproc must get the non-ANSI 'character' value, so PostMessage must
    // translate *two* characters of wParam for character messages, and PeekMessage
    // must translate *two* Unicode chars of wParam for ANSI app.
#endif

    /*
     * Only these messages have CHARs: others are passed through
     */

    switch(msg) {
#ifdef FE_IME // RtlWCSMessageWParamCharToMB()
    case WM_IME_CHAR:
    case WM_IME_COMPOSITION:
#endif // FE_IME
    case WM_CHAR:
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:

        CodePage = THREAD_CODEPAGE();
        dwAnsi = 0;

        nbWch = IS_DBCS_ENABLED() ? 1 * sizeof(WCHAR) : 2 * sizeof(WCHAR);

        if (IS_ACP(CodePage)) {
            // HACK HACK HACK HACK (for NtBug #3135)
            // to allow applications that store data in high word of wParam
            // Jan/06/96 hiroyama
            Status = RtlUnicodeToMultiByteN((LPSTR)&dwAnsi, sizeof(dwAnsi),
                    NULL, (LPWSTR)pWParam, nbWch);
            if (!NT_SUCCESS(Status)) {
                // LATER IanJa: returning FALSE makes GetMessage fail, which
                // terminates the app.  We should use some default 'bad character'
                // I use 0x00 for now.
                *pWParam = 0x00;
                return TRUE;
            }
        } else {
            int cwch;
            // assuming little endian
#ifdef _USERK_
            cwch = EngWideCharToMultiByte(CodePage,
                    (LPWSTR)pWParam, nbWch,
                    (LPSTR)&dwAnsi, sizeof(dwAnsi));
#else
            cwch = WideCharToMultiByte(CodePage, 0,
                    (LPCWSTR)pWParam, nbWch / sizeof(WCHAR),
                    (LPSTR)&dwAnsi, sizeof(dwAnsi), NULL, NULL);
#endif // _USERK_
            // KdPrint(("0x%04x -> 0x%02x (%d)\n", *pWParam, dwAnsi, CodePage));
            if (cwch == 0) {
                *pWParam = 0x00;
                return TRUE;
            }
        }
        if (IS_DBCS_ENABLED()) {
            WORD wAnsi = LOWORD(dwAnsi);
            //
            // From:
            //   HIBYTE(wAnsi)            = Dbcs TrailingByte.
            //   LOBYTE(wAnsi)            = Dbcs LeadingByte or Sbcs character.
            //
            // To:
            //   HIWORD(*pWParam)         = Original Data (information for DBCS messgaing).
            //   HIBYTE(LOWORD(*pWParam)) = Dbcs LeadingByte Byte.
            //   LOBYTE(LOWORD(*pWParam)) = Dbcs TrailingByte or Sbcs character.
            //
            if (IS_DBCS_MESSAGE(wAnsi)) {
                //
                // It's a DBCS character.
                //
                *pWParam = MAKEWPARAM(MAKEWORD(HIBYTE(wAnsi),LOBYTE(wAnsi)),HIWORD(*pWParam));
            } else {
                //
                // It's a SBCS character.
                //
                *pWParam = MAKEWPARAM(MAKEWORD(LOBYTE(wAnsi),0),0);
            }
        } else {
#if DBG
            if ((dwAnsi == 0) || (dwAnsi > 0xFF)) {
                RIPMSG1(RIP_VERBOSE, "msgW -> msgA: char = 0x%.4lX\n", dwAnsi);
            }
#endif
            *pWParam = dwAnsi;
        }
        break;
    }

    return TRUE;
}


/**************************************************************************\
* RtlMBMessageCharToWCS
*
* Converts a Multibyte character to a Wide character; in place
* Returns the number of characters converted or zero if failure
*
* 11-Feb-1992  JohnC    Created
* 13-Jan-1993  IanJa    Translate 2 characters (Publisher posts these!)
\**************************************************************************/

BOOL RtlMBMessageWParamCharToWCS(DWORD msg, WPARAM *pWParam)
{
    DWORD dwUni;
    NTSTATUS Status;
    // FE_SB    (RtlMBMessageWParamCharToWCS)
    BOOL  bWmCrIrDbcsChar = FALSE;
    WORD  wAnsi = LOWORD(*pWParam);
    // end FE_SB    (RtlMBMessageWParamCharToWCS)
    WORD CodePage = THREAD_CODEPAGE();

    /*
     * Only these messages have CHARs: others are passed through
     */

    switch(msg) {
    // FE_SB    (RtlMBMessageWParamCharToWCS)
    case WM_CHAR:
        //
        // WM_CHAR's wParam format for WM_IME_REPORT:IR_DBCSCHAR
        //
        if (IS_DBCS_ENABLED() && (*pWParam & WMCR_IR_DBCSCHAR)) {
            //
            // Mark this message is sent as IR_DBCSCHAR format.
            //
            bWmCrIrDbcsChar = TRUE;
        }

        //
        // Fall through....
        //
#ifdef FE_IME
    case WM_IME_CHAR:
    case WM_IME_COMPOSITION:
        //
        // We need to re-align for Unicode convertsion..
        // WM_CHAR/WM_IME_CHAR/WM_IME_COMPOSITION's wParam format :
        //
        // ReAlign IR_DBCS char format to regular sequence.
        //
        // From:
        //
        //  HIWORD(wParam)         = 0;
        //  HIBYTE(LOWORD(wParam)) = DBCS LeadingByte.
        //  LOBYTE(LOWORD(wParan)) = DBCS TrailingByte or SBCS character.
        //
        // To:
        //  HIWORD(wParam)         = 0;
        //  HIBYTE(LOWORD(wParam)) = DBCS TrailingByte.
        //  LOBYTE(LOWORD(wParam)) = DBCS LeadingByte or SBCS character.
        //
        if (IS_DBCS_ENABLED()) {
            *pWParam = MAKE_WPARAM_DBCSCHAR(wAnsi);
        }
#endif
        //
        // Fall through...
        //
        // end FE_SB    (RtlMBMessageWParamCharToWCS)
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:

        dwUni = 0;

        if (IS_ACP(CodePage)) {
            Status = RtlMultiByteToUnicodeN((LPWSTR)&dwUni, sizeof(dwUni),
                    NULL, (LPSTR)pWParam, 2 * sizeof(CHAR));
            if (!NT_SUCCESS(Status))
                return FALSE;
        } else {
            int cwch;
#ifdef _USERK_
            cwch = EngMultiByteToWideChar(CodePage,
                    (LPWSTR)&dwUni, sizeof(dwUni),
                    (LPSTR)pWParam, 2);
#else
            cwch = MultiByteToWideChar(CodePage, 0,
                    (LPSTR)pWParam, 2,
                    (LPWSTR)&dwUni, sizeof(dwUni) / sizeof(WCHAR));
#endif // _USERK_
            // KdPrint(("0x%02x -> 0x%04x (%d)\n", *pWParam, dwUni, CodePage));
            if (cwch == 0) {
                return FALSE;
            }
        }

        // FE_SB    (RtlMBMessageWParamCharToWCS)
        //
        // if this character is sent for WM_IME_REPORT:IR_DBCSCHAR, we mark it.
        //
        if (bWmCrIrDbcsChar)
            dwUni |= WMCR_IR_DBCSCHAR;
        // else FE_SB (RtlMBMessageWParamCharToWCS)
#if DBG
        if ((dwUni == 0) || (dwUni > 0xFF)) {
            RIPMSG1(RIP_VERBOSE, "msgA -> msgW: wchar = 0x%lX\n", dwUni);
        }
#endif
        // end FE_SB
        *pWParam = dwUni;
        break;
    }

    return TRUE;
}

/**************************************************************************\
* RtlInitLargeAnsiString
*
* Captures a large ANSI string in the same manner as
* RtlInitAnsiString.
*
* 03-22-95 JimA         Created.
\**************************************************************************/

VOID RtlInitLargeAnsiString(
    PLARGE_ANSI_STRING plstr,
    LPCSTR psz,
    UINT cchLimit)
{
    ULONG Length;

    plstr->Buffer = (PSTR)psz;
    plstr->bAnsi = TRUE;
    if (ARGUMENT_PRESENT( psz )) {
        Length = strlen( psz );
        plstr->Length = min(Length, cchLimit);
        plstr->MaximumLength = min((Length + 1), cchLimit);
    } else {
        plstr->MaximumLength = 0;
        plstr->Length = 0;
    }
}

/**************************************************************************\
* RtlInitLargeUnicodeString
*
* Captures a large unicode string in the same manner as
* RtlInitUnicodeString.
*
* 03-22-95 JimA         Created.
\**************************************************************************/

VOID RtlInitLargeUnicodeString(
    PLARGE_UNICODE_STRING plstr,
    LPCWSTR psz,
    UINT cchLimit)
{
    ULONG Length;

    plstr->Buffer = (PWSTR)psz;
    plstr->bAnsi = FALSE;
    if (ARGUMENT_PRESENT( psz )) {
        Length = wcslen( psz ) * sizeof( WCHAR );
        plstr->Length = min(Length, cchLimit);
        plstr->MaximumLength = min((Length + sizeof(UNICODE_NULL)), cchLimit);
    } else {
        plstr->MaximumLength = 0;
        plstr->Length = 0;
    }
}

