/***********************************************************************/
/*                                                                     */
/*  RCFUTIL.C -                                                        */
/*                                                                     */
/*     Windows 3.0 Resource compiler - File utility functions          */
/*                                                                     */
/*                                                                     */
/***********************************************************************/

#include "rc.h"


/* IsTextUnicode has to be here so this will run on Chicago and NT 1.0. */

#define UNICODE_FFFF              0xFFFF
#define REVERSE_BYTE_ORDER_MARK   0xFFFE
#define BYTE_ORDER_MARK           0xFEFF

#define PARAGRAPH_SEPARATOR       0x2029
#define LINE_SEPARATOR            0x2028

#define UNICODE_TAB               0x0009
#define UNICODE_LF                0x000A
#define UNICODE_CR                0x000D
#define UNICODE_SPACE             0x0020
#define UNICODE_CJK_SPACE         0x3000

#define UNICODE_R_TAB             0x0900
#define UNICODE_R_LF              0x0A00
#define UNICODE_R_CR              0x0D00
#define UNICODE_R_SPACE           0x2000
#define UNICODE_R_CJK_SPACE       0x0030  /* Ambiguous - same as ASCII '0' */

#define ASCII_CRLF                0x0A0D

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))

#define ARGUMENT_PRESENT(a)     (a != NULL)

BOOL
WINAPI
LocalIsTextUnicode(
    CONST LPVOID Buffer,
    int Size,
    LPINT Result
    )

/*++

Routine Description:

    IsTextUnicode performs a series of inexpensive heuristic checks
    on a buffer in order to verify that it contains Unicode data.


    [[ need to fix this section, see at the end ]]

    Found            Return Result

    BOM              TRUE   BOM
    RBOM             FALSE  RBOM
    FFFF             FALSE  Binary
    NULL             FALSE  Binary
    null             TRUE   null bytes
    ASCII_CRLF       FALSE  CRLF
    UNICODE_TAB etc. TRUE   Zero Ext Controls
    UNICODE_TAB_R    FALSE  Reversed Controls
    UNICODE_ZW  etc. TRUE   Unicode specials

    1/3 as little variation in hi-byte as in lo byte: TRUE   Correl
    3/1 or worse   "                                  FALSE  AntiCorrel

Arguments:

    Buffer - pointer to buffer containing text to examine.

    Size - size of buffer in bytes.  At most 256 characters in this will
           be examined.  If the size is less than the size of a unicode
           character, then this function returns FALSE.

    Result - optional pointer to a flag word that contains additional information
             about the reason for the return value.  If specified, this value on
             input is a mask that is used to limit the factors this routine uses
             to make it decision.  On output, this flag word is set to contain
             those flags that were used to make its decision.

Return Value:

    Boolean value that is TRUE if Buffer contains unicode characters.

--*/
{
    CPINFO      cpinfo;
    UNALIGNED WCHAR *lpBuff = (UNALIGNED WCHAR *) Buffer;
    PCHAR lpb = (PCHAR) Buffer;
    ULONG iBOM = 0;
    ULONG iCR = 0;
    ULONG iLF = 0;
    ULONG iTAB = 0;
    ULONG iSPACE = 0;
    ULONG iCJK_SPACE = 0;
    ULONG iFFFF = 0;
    ULONG iPS = 0;
    ULONG iLS = 0;

    ULONG iRBOM = 0;
    ULONG iR_CR = 0;
    ULONG iR_LF = 0;
    ULONG iR_TAB = 0;
    ULONG iR_SPACE = 0;

    ULONG iNull = 0;
    ULONG iUNULL = 0;
    ULONG iCRLF = 0;
    ULONG iTmp;
    ULONG LastLo = 0;
    ULONG LastHi = 0;
    ULONG iHi, iLo;
    ULONG HiDiff = 0;
    ULONG LoDiff = 0;
    ULONG cLeadByte = 0;
    ULONG cWeird = 0;

    ULONG iResult = 0;

    ULONG iMaxTmp = __min(256, Size / sizeof(WCHAR));

    if (Size < 2 ) {
        if (ARGUMENT_PRESENT( Result )) {
            *Result = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
            }

        return FALSE;
        }


    // Check at most 256 wide character, collect various statistics
    for (iTmp = 0; iTmp < iMaxTmp; iTmp++) {
        switch (lpBuff[iTmp]) {
            case BYTE_ORDER_MARK:
                iBOM++;
                break;
            case PARAGRAPH_SEPARATOR:
                iPS++;
                break;
            case LINE_SEPARATOR:
                iLS++;
                break;
            case UNICODE_LF:
                iLF++;
                break;
            case UNICODE_TAB:
                iTAB++;
                break;
            case UNICODE_SPACE:
                iSPACE++;
                break;
            case UNICODE_CJK_SPACE:
                iCJK_SPACE++;
                break;
            case UNICODE_CR:
                iCR++;
                break;

            // The following codes are expected to show up in
            // byte reversed files
            case REVERSE_BYTE_ORDER_MARK:
                iRBOM++;
                break;
            case UNICODE_R_LF:
                iR_LF++;
                break;
            case UNICODE_R_TAB:
                iR_TAB++;
                break;
            case UNICODE_R_CR:
                iR_CR++;
                break;
            case UNICODE_R_SPACE:
                iR_SPACE++;
                break;

            // The following codes are illegal and should never occur
            case UNICODE_FFFF:
                iFFFF++;
                break;
            case UNICODE_NULL:
                iUNULL++;
                break;

            // The following is not currently a Unicode character
            // but is expected to show up accidentally when reading
            // in ASCII files which use CRLF on a little endian machine
            case ASCII_CRLF:
                iCRLF++;
                break;       /* little endian */
        }

        // Collect statistics on the fluctuations of high bytes
        // versus low bytes

        iHi = HIBYTE (lpBuff[iTmp]);
        iLo = LOBYTE (lpBuff[iTmp]);

        // Count cr/lf and lf/cr that cross two words
        if ((iLo == '\r' && LastHi == '\n') ||
            (iLo == '\n' && LastHi == '\r')) {
            cWeird++;
        }

        iNull += (iHi ? 0 : 1) + (iLo ? 0 : 1);   /* count Null bytes */

        HiDiff += __max( iHi, LastHi ) - __min( LastHi, iHi );
        LoDiff += __max( iLo, LastLo ) - __min( LastLo, iLo );

        LastLo = iLo;
        LastHi = iHi;
    }

    // Count cr/lf and lf/cr that cross two words
    if ((iLo == '\r' && LastHi == '\n') ||
        (iLo == '\n' && LastHi == '\r')) {
        cWeird++;
    }

    if (iHi == '\0')     /* don't count the last null */
        iNull--;
    if (iHi == 26)       /* count ^Z at end as weird */
        cWeird++;

    iMaxTmp = __min(256 * sizeof(WCHAR), Size);
    GetCPInfo(CP_ACP, &cpinfo);
    if (cpinfo.MaxCharSize != 1) {
        for (iTmp = 0; iTmp < iMaxTmp; iTmp++) {
            if (IsDBCSLeadByteEx(uiCodePage, lpb[iTmp])) {
                cLeadByte++;
                iTmp++;         /* should check for trailing-byte range */
            }
        }
    }

    // sift the statistical evidence
    if (LoDiff < 127 && HiDiff == 0) {
        iResult |= IS_TEXT_UNICODE_ASCII16;         /* likely 16-bit ASCII */
    }

    if (HiDiff && LoDiff == 0) {
        iResult |= IS_TEXT_UNICODE_REVERSE_ASCII16; /* reverse 16-bit ASCII */
    }

    // Use leadbyte info to weight statistics.
    if (!cpinfo.MaxCharSize != 1 || cLeadByte == 0 ||
        !ARGUMENT_PRESENT(Result) || !(*Result & IS_TEXT_UNICODE_DBCS_LEADBYTE)) {
        iHi = 3;
    } else {
        // A ratio of cLeadByte:cb of 1:2 ==> dbcs
        // Very crude - should have a nice eq.
        iHi = __min(256, Size/sizeof(WCHAR)) / 2;
        if (cLeadByte < (iHi-1) / 3) {
            iHi = 3;
        } else if (cLeadByte < (2 * (iHi-1)) / 3) {
            iHi = 2;
        } else {
            iHi = 1;
        }
        iResult |= IS_TEXT_UNICODE_DBCS_LEADBYTE;
    }

    if (iHi * HiDiff < LoDiff) {
        iResult |= IS_TEXT_UNICODE_STATISTICS;
    }

    if (iHi * LoDiff < HiDiff) {
        iResult |= IS_TEXT_UNICODE_REVERSE_STATISTICS;
    }

    //
    // Any control codes widened to 16 bits? Any Unicode character
    // which contain one byte in the control code range?
    //

    if (iCR + iLF + iTAB + iSPACE + iCJK_SPACE /*+iPS+iLS*/) {
        iResult |= IS_TEXT_UNICODE_CONTROLS;
    }

    if (iR_LF + iR_CR + iR_TAB + iR_SPACE) {
        iResult |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
    }

    //
    // Any characters that are illegal for Unicode?
    //

    if (((iRBOM + iFFFF + iUNULL + iCRLF) != 0) || ((cWeird != 0) && (cWeird >= iMaxTmp/40))) {
        iResult |= IS_TEXT_UNICODE_ILLEGAL_CHARS;
    }

    //
    // Odd buffer length cannot be Unicode
    //

    if (Size & 1) {
        iResult |= IS_TEXT_UNICODE_ODD_LENGTH;
    }

    //
    // Any NULL bytes? (Illegal in ANSI)
    //
    if (iNull) {
        iResult |= IS_TEXT_UNICODE_NULL_BYTES;
    }

    //
    // POSITIVE evidence, BOM or RBOM used as signature
    //

    if (*lpBuff == BYTE_ORDER_MARK) {
        iResult |= IS_TEXT_UNICODE_SIGNATURE;
    } else if (*lpBuff == REVERSE_BYTE_ORDER_MARK) {
        iResult |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
    }

    //
    // limit to desired categories if requested.
    //

    if (ARGUMENT_PRESENT( Result )) {
        iResult &= *Result;
        *Result = iResult;
    }

    //
    // There are four separate conclusions:
    //
    // 1: The file APPEARS to be Unicode     AU
    // 2: The file CANNOT be Unicode         CU
    // 3: The file CANNOT be ANSI            CA
    //
    //
    // This gives the following possible results
    //
    //      CU
    //      +        -
    //
    //      AU       AU
    //      +   -    +   -
    //      --------  --------
    //      CA +| 0   0    2   3
    //      |
    //      -| 1   1    4   5
    //
    //
    // Note that there are only 6 really different cases, not 8.
    //
    // 0 - This must be a binary file
    // 1 - ANSI file
    // 2 - Unicode file (High probability)
    // 3 - Unicode file (more than 50% chance)
    // 5 - No evidence for Unicode (ANSI is default)
    //
    // The whole thing is more complicated if we allow the assumption
    // of reverse polarity input. At this point we have a simplistic
    // model: some of the reverse Unicode evidence is very strong,
    // we ignore most weak evidence except statistics. If this kind of
    // strong evidence is found together with Unicode evidence, it means
    // its likely NOT Text at all. Furthermore if a REVERSE_BYTE_ORDER_MARK
    // is found, it precludes normal Unicode. If both byte order marks are
    // found it's not Unicode.
    //

    //
    // Unicode signature : uncontested signature outweighs reverse evidence
    //

    if ((iResult & IS_TEXT_UNICODE_SIGNATURE) &&
        !(iResult & (IS_TEXT_UNICODE_NOT_UNICODE_MASK&(~IS_TEXT_UNICODE_DBCS_LEADBYTE)))
       ) {
        return TRUE;
    }

    //
    // If we have conflicting evidence, it's not Unicode
    //

    if (iResult & IS_TEXT_UNICODE_REVERSE_MASK) {
        return FALSE;
    }

    //
    // Statistical and other results (cases 2 and 3)
    //

    if (!(iResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) &&
         ((iResult & IS_TEXT_UNICODE_NOT_ASCII_MASK) ||
          (iResult & IS_TEXT_UNICODE_UNICODE_MASK)
         )
       ) {
        return TRUE;
    }

    return FALSE;
}


/*------------------------------------------------------------------*/
/*                                                                  */
/* fgetl() -                                                        */
/*                                                                  */
/*------------------------------------------------------------------*/

/* fgetl expands tabs and return lines w/o separators */
/* returns line from file (no CRLFs); returns NULL if EOF */

int
fgetl (
    PWCHAR wbuf,
    int len,
    BOOL bUnicode,
    PFILE fh
    )
{
    int c = 0;
    int second;

    *wbuf = 0;

    if (bUnicode) {
        PWCHAR p;

        /* remember NUL at end */
        len--;
        p = wbuf;


        /* fill buffer from the file until EOF or EOLN or no space in buffer */
        while (len) {
            c = fgetc (fh);
            if (c == EOF)
                break;
            second = fgetc (fh);
            c = MAKEWORD (c, second);
            if (c == L'\n')
                break;

            if (c != L'\r') {
                if (c != L'\t') {
                    *p++ = (WCHAR)c;
                    len--;
                } else {

                    /* tabs: expand to spaces */
                    c = (int)(min (8 - ((p - wbuf) & 0x0007), len));
                    len -= c;
                    while (c) {
                        *p++ = L' ';
                        c--;
                    }
                }
            }
        }

        /* null terminate string */
        *p = 0;
    } else {
        PCHAR p;
        PCHAR lpbuf;

        p = lpbuf = (PCHAR) LocalAlloc (LPTR, len);

        /* remember NUL at end */
        len--;

        /* fill buffer from the file until EOF or EOLN or no space in buffer */
        while (len) {
            c = fgetc (fh);
            if (c == EOF || c == '\n')
                break;

            if (c != '\r') {
                if (c != '\t') {
                    *p++ = (CHAR)c;
                    len--;
                } else {

                    /* tabs: expand to spaces */
                    c = (int)(min (8 - ((p - lpbuf) & 0x0007), len));
                    len -= c;
                    while (c) {
                        *p++ = ' ';
                        c--;
                    }
                }
            }
        }

        /* null terminate string and translate to Unicode */
        *p = 0;
        MultiByteToWideChar (uiCodePage, MB_PRECOMPOSED, lpbuf, -1, wbuf, (int)(p - lpbuf + 1));

        LocalFree (lpbuf);
    }

    /* return false if EOF with no chars read */
    return !(c == EOF && !*wbuf);
}

/*----------------------------------------------------------*/
/*                                                          */
/* myfwrite() -                                             */
/*                                                          */
/*  Wrapper for fwrite to ensure data gets to the disk.     */
/*      returns if ok, calls quit if write fails            */
/*----------------------------------------------------------*/

void
myfwrite(
    const void *pv,
    size_t s,
    size_t n,
    PFILE fp
    )
{
    if (fwrite(pv, s, n, fp) == n)
        return;
    else
        quit(GET_MSG(1122));
}
