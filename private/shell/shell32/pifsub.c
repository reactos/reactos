/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFSUB.C
 *  Misc. subroutines for PIFMGR.DLL
 *
 *  History:
 *  Created 31-Jul-1992 3:30pm by Jeff Parsons
 */


#include "shellprv.h"
#pragma hdrstop

// shell priv can alter the definition of IsDBCSLeadByte!
#if defined(FE_SB)
#ifdef IsDBCSLeadByte
#undef IsDBCSLeadByte
#define IsDBCSLeadByte(x) IsDBCSLeadByteEx(CP_ACP,x)
#endif
#endif

/*
 * Most of the routines in this file will need to stay ANSI.  If a UNICODE
 * version is needed, it is supplied.
 *
 * This is because for the most part, the information in the PIF files
 * is ANSI and needs to stay that way.
 *
 * (RickTu)
 *
 */


/** lstrcpytrimA - copy fixed-length string, trimming trailing blanks
 *
 * INPUT
 *  lpszDst -> destination string
 *  lpszSrc -> fixed-length source string
 *  cchMax = size (in characters) of fixed-length source string
 *
 * OUTPUT
 *  Nothing
 */

void lstrcpytrimA(LPSTR lpszDst, LPCSTR lpszSrc, int cchMax)
{
    CHAR ch;
    int cchSave = 0;
    LPSTR lpszSave = lpszDst;
    FunctionName(lstrcpytrim);

    while (cchMax && *lpszSrc) {
        ch = *lpszSrc++;
        if (ch == ' ') {
            if (!cchSave) {
                cchSave = cchMax;
                lpszSave = lpszDst;
            }
        } else
            cchSave = 0;
        *lpszDst = ch;
        if (--cchMax) {
            lpszDst++;
#if (defined(DBCS) || defined(FE_SB))
            if (IsDBCSLeadByte(ch) && --cchMax) {
                *lpszDst++ = *lpszSrc++;
            }
#endif
        }
    }
    while (cchSave--)
        *lpszSave++ = '\0';
    *lpszDst = '\0';
}


/** lstrcpypadA - copy to fixed-length string, appending trailing blanks
 *
 * INPUT
 *  lpszDst -> fixed-length destination string
 *  lpszSrc -> source string
 *  cchMax = size of fixed-length destination string (count of characters)
 *
 * OUTPUT
 *  Nothing
 */

void lstrcpypadA(LPSTR lpszDst, LPCSTR lpszSrc, int cchMax)
{
    FunctionName(lstrcpypadA);
    while (cchMax && *lpszSrc) {
        cchMax--;
        *lpszDst++ = *lpszSrc++;
    }
    while (cchMax--) {
        *lpszDst++ = ' ';
    }
}


/** lstrcpyncharA - copy variable-length string, until char
 *
 * INPUT
 *  lpszDst -> fixed-length destination string
 *  lpszSrc -> source string
 *  cchMax = size of fixed-length destination string (count of characters)
 *  ch = character to stop copying at
 *
 * OUTPUT
 *  # of characters copied, excluding terminating NULL
 */

int lstrcpyncharA(LPSTR lpszDst, LPCSTR lpszSrc, int cchMax, CHAR ch)
{
    int cch = 0;
    FunctionName(lstrcpyncharA);

    while (--cchMax && *lpszSrc && *lpszSrc != ch) {
#if (defined(DBCS) || defined(FE_SB))
        if (IsDBCSLeadByte(*lpszSrc)) {
            cch++;
            *lpszDst++ = *lpszSrc++;
            if (!*lpszSrc) break;   /* Eek!  String ends in DBCS lead byte! */
        }
#endif
        cch++;
        *lpszDst++ = *lpszSrc++;
    }
    *lpszDst = '\0';
    return cch;
}


/** lstrskipcharA - skip char in variable-length string
 *
 * INPUT
 *  lpszSrc -> source string
 *  ch = character to skip
 *
 * OUTPUT
 *  # of characters skipped, 0 if none
 */

int lstrskipcharA(LPCSTR lpszSrc, CHAR ch)
{
    int cch = 0;
    FunctionName(lstrskipcharA);

    while (*lpszSrc && *lpszSrc == ch) {
        cch++;
        lpszSrc++;
    }
    return cch;
}

/** lstrskiptocharA - skip *to* char in variable-length string
 *
 * INPUT
 *  lpszSrc -> source string
 *  ch = character to skip *to*
 *
 * OUTPUT
 *  # of characters skipped, 0 if none;  if char didn't exist, then all
 *  characters are skipped.
 */

int lstrskiptocharA(LPCSTR lpszSrc, CHAR ch)
{
    int cch = 0;
    FunctionName(lstrskiptocharA);

    while (*lpszSrc && *lpszSrc != ch) {
        cch++;
        lpszSrc++;
    }
    return cch;
}


#ifdef UNICODE
/** lstrrchrW - UNICODE-friendly version of strrchr
 *
 * INPUT
 *  lpszSrc -> source string
 *  ch = character to locate
 *
 * OUTPUT
 *  Pointer to last appearance of character ch in string,
 *  or NULL if ch does not appear.
 */

LPWSTR lstrrchrW(LPCWSTR lpszSrc, WCHAR ch)
{
    LPCWSTR lpszLast = 0;
    FunctionName(lstrrchrW);
    while (*lpszSrc) {
        if (*lpszSrc == ch)
            lpszLast = lpszSrc;
        lpszSrc++;
    }
    return (LPWSTR)lpszLast;
}
#endif


/** lstrrchrA - DBCS-friendly version of strrchr
 *
 * INPUT
 *  lpszSrc -> source string
 *  ch = character to locate
 *
 * OUTPUT
 *  Pointer to last appearance of character ch in string,
 *  or NULL if ch does not appear.
 */

LPSTR lstrrchrA(LPCSTR lpszSrc, CHAR ch)
{
    LPCSTR lpszLast = 0;
    FunctionName(lstrrchrA);

    while (*lpszSrc) {
        if (*lpszSrc == ch)
            lpszLast = lpszSrc;
#if (defined(DBCS) || defined(FE_SB))
        if (IsDBCSLeadByte(*lpszSrc++)) {
            if (!*lpszSrc++) break; /* Eek!  String ends in DBCS lead byte! */
        }
#else
        lpszSrc++;
#endif
    }
    return (LPSTR)lpszLast;
}


/** lstrcpyfnameA - copy filename appropriately
 *
 * INPUT
 *  lpszDst -> output buffer
 *  lpszSrc -> source filename
 *  cbMax = size of output buffer
 *
 * OUTPUT
 *  # of characters copied, including quotes if any, excluding terminating NULL
 */

int lstrcpyfnameA(LPSTR lpszDst, LPCSTR lpszSrc, int cchMax)
{
    int cch;
    CHAR ch;
    FunctionName(lstrcpyfnameA);

    if (TEXT('\0') != (ch = lpszSrc[lstrskiptocharA(lpszSrc, ' ')])) {
        cchMax -= 2;
        *lpszDst++ = '\"';
    }

    lstrcpynA(lpszDst, lpszSrc, cchMax);
    cch = lstrlenA(lpszDst);

    if (ch) {
        lpszDst[cch++] = '\"';
        lpszDst[cch] = 0;
    }
    return cch;
}


/** lstrunquotefnameA - unquote filename if it contains quotes
 *
 * INPUT
 *  lpszDst -> output buffer
 *  lpszSrc -> source filename (quoted or unquoted)
 *  cchMax = size of output buffer (count of characters)
 *  fShort = TRUE if filename should be converted to 8.3 (eg, for real-mode);
 *           -1 if the filename is known to not be quoted and should just be converted
 * OUTPUT
 *  # of characters copied, excluding terminating NULL
 */

int lstrunquotefnameA(LPSTR lpszDst, LPCSTR lpszSrc, int cchMax, BOOL fShort)
{
    int cch;
    FunctionName(lstrunquotefnameA);

    if (fShort != -1) {

        if (lpszSrc[0] == '\"') {
            cch = lstrcpyncharA(lpszDst, lpszSrc+1, cchMax, '\"');
        }
        else {
            cch = lstrcpyncharA(lpszDst, lpszSrc, cchMax, ' ');
        }
        lpszSrc = lpszDst;
    }
    if (fShort) {
        cch = 1;
        CharToOemA(lpszSrc, lpszDst);
        cch = GetShortPathNameA( lpszSrc, lpszDst, cchMax );
        if (cch) {                       // if no error...
            if (fShort == TRUE) {       // if conversion for real-mode...
                if ((int)GetFileAttributesA(lpszDst) == -1) {
                                        // if filename doesn't exist,
                                        // then just copy the 8.3 portion
                                        // and hope the user's real-mode PATH
                                        // ultimately finds it!

                    if (NULL != (lpszSrc = lstrrchrA(lpszDst, '\\'))) {
                        lstrcpyA(lpszDst, lpszSrc+1);
                    }
                }
            }
            cch = lstrlenA(lpszDst);      // recompute the length of the string
        }
    }
    return cch;
}


/** lstrskipfnameA - skip filename in string
 *
 * INPUT
 *  lpszSrc -> string beginning with filename (quoted or unquoted)
 *
 * OUTPUT
 *  # of characters skipped, 0 if none
 */

int lstrskipfnameA(LPCSTR lpszSrc)
{
    int cch = 0;
    FunctionName(lstrskipfname);

    if (lpszSrc[0] == '\"') {
        cch = lstrskiptocharA(lpszSrc+1, '\"') + 1;
        if (lpszSrc[cch] == '\"')
            cch++;
    }
    else
        cch = lstrskiptocharA(lpszSrc, ' ');
    return cch;
}


#ifdef UNICODE
int _fatoiW(LPCWSTR lpszw)
{
    CHAR szTemp[ 256 ];  // BUGBUG, 256 is just a guess!

    szTemp[0] = '\0';
    WideCharToMultiByte( CP_ACP, 0, lpszw, -1, szTemp, 256, NULL, NULL );
    return atoi( szTemp );
}
#endif

/*
 * NOTE! The careful definitions of achBuf and achFmt, so that
 * we can support total output of 2 * MAX_STRING_SIZE bytes.
 */
int cdecl Warning(HWND hwnd, WORD id, WORD type, ...)
{
    LPCTSTR lpchFmt;
    PPROPLINK ppl = NULL;
    TCHAR achBuf[2*MAX_STRING_SIZE];
#define achFmt (&achBuf[MAX_STRING_SIZE])
    va_list ArgList;
    FunctionName(Warning);

    lpchFmt = achFmt;

    // We never use MB_FOCUS to mean whatever it's really supposed
    // to mean;  we just use it as a kludge to support warning dialogs
    // when all we have is a ppl, not an hwnd.

    if (type & MB_NOFOCUS) {
        ppl = (PPROPLINK)hwnd;
        hwnd = NULL;
        type &= ~MB_NOFOCUS;
    }
    else if (hwnd)
        ppl = ((PPROPLINK)GetWindowLongPtr(hwnd, DWLP_USER))->ppl;

    if (id == IDS_ERROR + ERROR_NOT_ENOUGH_MEMORY)
        lpchFmt = TEXT("");
    else {
        if (!LoadString(g_hinst, id, achFmt, MAX_STRING_SIZE)) {
            ASSERTFAIL();
            lpchFmt = TEXT("");
        }
    }

    va_start(ArgList,type);
    wvnsprintf(achBuf, MAX_STRING_SIZE, lpchFmt, ArgList);
    va_end(ArgList);

    lpchFmt = NULL;
    if (ppl) {
        ASSERTTRUE(ppl->iSig == PROP_SIG);
        if (!(lpchFmt = ppl->lpszTitle))
            lpchFmt = ppl->szPathName+ppl->iFileName;
    }
    return MessageBox(hwnd, achBuf, lpchFmt, type);
}
#undef achFmt

int MemoryWarning(HWND hwnd)
{
    FunctionName(MemoryWarning);
    return Warning(hwnd, IDS_ERROR + ERROR_NOT_ENOUGH_MEMORY, MB_ICONEXCLAMATION | MB_OK);
}


LPTSTR LoadStringSafe(HWND hwnd, UINT id, LPTSTR lpsz, int cchsz)
{
    FunctionName(LoadStringSafe);
    if (!LoadString(g_hinst, id, lpsz, cchsz)) {
        ASSERTFAIL();
        if (hwnd) {
            MemoryWarning(hwnd);
            return NULL;
        }
        lpsz = TEXT("");
    }
    return lpsz;
}


/** SetDlgBits - Check various dialog checkboxes according to given flags
 *
 * INPUT
 *  hDlg   = HWND of dialog box
 *  pbinf -> array of bitinfo descriptors
 *  cbinf  = size of array
 *  wFlags = flags
 *
 * OUTPUT
 *  Returns NOTHING
 */

void SetDlgBits(HWND hDlg, PBINF pbinf, UINT cbinf, WORD wFlags)
{
    FunctionName(SetDlgBits);

    ASSERTTRUE(cbinf > 0);
    do {
        ASSERTTRUE((pbinf->bBit & 0x3F) < 16);
        CheckDlgButton(hDlg, pbinf->id,
                       !!(wFlags & (1 << (pbinf->bBit & 0x3F))) == !(pbinf->bBit & 0x80));
    } while (++pbinf, --cbinf);
}


/** GetDlgBits - Set various flags according to dialog checkboxes
 *
 * INPUT
 *  hDlg   = HWND of dialog box
 *  pbinf -> array of bitinfo descriptors
 *  cbinf  = size of array
 *  lpwFlags -> flags word
 *
 * OUTPUT
 *  Returns NOTHING
 */

void GetDlgBits(HWND hDlg, PBINF pbinf, UINT cbinf, LPWORD lpwFlags)
{
    WORD wFlags;
    FunctionName(GetDlgBits);

    ASSERTTRUE(cbinf > 0);
    wFlags = *lpwFlags;
    do {
        ASSERTTRUE((pbinf->bBit & 0x3F) < 16);

        if (pbinf->bBit & 0x40)         // 0x40 is a special bit mask
            continue;                   // that means "set but don't get
                                        // this control's value"
        wFlags &= ~(1 << (pbinf->bBit & 0x3F));
        if (!!IsDlgButtonChecked(hDlg, pbinf->id) == !(pbinf->bBit & 0x80))
            wFlags |= (1 << (pbinf->bBit & 0x3F));

    } while (++pbinf, --cbinf);
    *lpwFlags = wFlags;
}


/** SetDlgInts - Set various edit controls according to integer fields
 *
 * INPUT
 *  hDlg   = HWND of dialog box
 *  pvinf -> array of validation info descriptors
 *  cvinf  = size of array
 *  lp    -> structure of integers
 *
 * OUTPUT
 *  Returns NOTHING
 */

void SetDlgInts(HWND hDlg, PVINF pvinf, UINT cvinf, LPVOID lp)
{
    WORD wMin, wMax;
    FunctionName(SetDlgInts);

    ASSERTTRUE(cvinf > 0);
    do {
        wMin = wMax = *(WORD UNALIGNED *)((LPBYTE)lp + pvinf->off);

        if (pvinf->fbOpt & VINF_AUTO) {

            SendDlgItemMessage(hDlg, pvinf->id, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)g_szAuto);

            AddDlgIntValues(hDlg, pvinf->id, pvinf->iMax);

            if (wMin == 0) {
                SetDlgItemText(hDlg, pvinf->id, g_szAuto);
                continue;
            }
        }
        if (pvinf->fbOpt & VINF_AUTOMINMAX) {

            SendDlgItemMessage(hDlg, pvinf->id, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)g_szAuto);
            SendDlgItemMessage(hDlg, pvinf->id, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)g_szNone);

            AddDlgIntValues(hDlg, pvinf->id, pvinf->iMax);

            // When AUTOMINMAX is set, we assume that the field
            // we're validating is followed in its structure by a
            // corresponding max WORD.

            wMax = *(WORD UNALIGNED *)((LPBYTE)lp + pvinf->off + sizeof(WORD));

            if (wMin == 0 && wMax == 0) {
                SetDlgItemText(hDlg, pvinf->id, g_szNone);
                continue;
            }

            // Let's try to simplify things by mapping 0xFFFF (aka -1)
            // to settings that mean "Auto"

            if (wMin == 0xFFFF || wMax == 0xFFFF) {
                wMin = 0;
                wMax = (WORD)pvinf->iMax;
            }

            if (wMax == (WORD)pvinf->iMax) {
                SetDlgItemText(hDlg, pvinf->id, g_szAuto);
                continue;
            }

            if (wMin != wMax) {
                //
                // We're in a bit of a quandary here.  The settings show
                // explicit min and max values which are not equal, probably
                // due to settings inherited from a 3.1 PIF file.  We'll
                // just go with the wMax value.  Fortunately for us, we
                // don't actually have to *do* anything to make this happen.
                //
            }
        }
        SetDlgItemInt(hDlg, pvinf->id, wMin, pvinf->iMin < 0);

    } while (++pvinf, --cvinf);
}


/** AddDlgIntValues - Fill integer combo-box with appropriate values
 *
 * INPUT
 *  hDlg   = HWND of dialog box
 *  id     = dialog control ID
 *  iMax   = maximum value
 *
 * OUTPUT
 *  Returns NOTHING
 */

void AddDlgIntValues(HWND hDlg, int id, int iMax)
{
    int iStart, iInc;
    TCHAR achValue[16];

    // HACK to make this do something sensible with the environment max;
    // they can still enter larger values (up to ENVSIZE_MAX) but I don't
    // see any sense in encouraging it. -JTP

    if ((WORD)iMax == ENVSIZE_MAX)
        iMax = 4096;

    if ((iMax < 0) || (iMax == 0xFFFF)) // HACK to make this do something sensible
        iMax = 16384;           // with fields that allow huge maximums -JTP

    iStart = iInc = iMax/16;    // arbitrarily chop the range up 16 times

    while (iStart <= iMax) {
        wsprintf(achValue, TEXT("%d"), iStart);
        SendDlgItemMessage(hDlg, id, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)achValue);
        iStart += iInc;
    }
}


/** GetDlgInts - Set various integer fields according to dialog edit controls
 *
 * INPUT
 *  hDlg   = HWND of dialog box
 *  pvinf -> array of validation info descriptors
 *  cvinf  = size of array
 *  lp    -> structure of integers
 *
 * OUTPUT
 *  Returns NOTHING
 */

void GetDlgInts(HWND hDlg, PVINF pvinf, int cvinf, LPVOID lp)
{
    WORD wMin, wMax;
    UINT uTemp;
    BOOL fSuccess;
    TCHAR achText[32];
    FunctionName(GetDlgInts);

    ASSERTTRUE(cvinf > 0);
    do {
        uTemp = GetDlgItemInt(hDlg, pvinf->id, &fSuccess, pvinf->iMin < 0);
        ASSERT(HIWORD(uTemp)==0);

        wMin = LOWORD(uTemp);

        // In case of error, make sure wMin doesn't actually change

        if (!fSuccess)
            wMin = *(WORD UNALIGNED *)((LPBYTE)lp + pvinf->off);

        if (pvinf->fbOpt & VINF_AUTO) {

            GetDlgItemText(hDlg, pvinf->id, achText, ARRAYSIZE(achText));

            if (lstrcmpi(achText, g_szAuto) == 0) {
                wMin = 0;
            }
        }

        if (pvinf->fbOpt & VINF_AUTOMINMAX) {

            // When AUTOMINMAX is set, we assume that the field
            // we're validating is followed in its structure by a
            // corresponding max WORD, which we will ZERO if the
            // user selects NONE, or set to its MAXIMUM if the user
            // selects AUTO, or otherwise set to match the specified
            // MINIMUM.

            wMax = wMin;

            GetDlgItemText(hDlg, pvinf->id, achText, ARRAYSIZE(achText));

            if (lstrcmpi(achText, g_szAuto) == 0) {
                wMin = 0;
                wMax = (WORD)pvinf->iMax;
            }
            else if (lstrcmpi(achText, g_szNone) == 0) {
                wMin = 0;
                wMax = 0;
            }

            *(WORD UNALIGNED *)((LPBYTE)lp + pvinf->off + sizeof(WORD)) = wMax;
        }

        *(WORD UNALIGNED *)((LPBYTE)lp + pvinf->off) = wMin;

    } while (++pvinf, --cvinf);
}


/** ValidateDlgInts - Validate that integer fields are value
 *
 * INPUT
 *  hDlg   = HWND of dialog box
 *  pvinf -> array of validation descriptors
 *  cvinf  = size of array
 *
 * OUTPUT
 *  Returns TRUE if something is wrong; FALSE if all is okay.
 */

BOOL ValidateDlgInts(HWND hDlg, PVINF pvinf, int cvinf)
{
    DWORD dw;
    BOOL fSuccess;
    TCHAR achText[32];
    FunctionName(ValidateDlgInts);

    ASSERTTRUE(cvinf > 0);
    do {
        dw = GetDlgItemInt(hDlg, pvinf->id, &fSuccess, pvinf->iMin < 0);

        // NOTE: AUTO is for "Auto" only, whereas AUTOMINMAX is for
        // "Auto" and "None".  However, in the interest of simplicity, I
        // don't complain if either string is used in either case.

        if (pvinf->fbOpt & (VINF_AUTO | VINF_AUTOMINMAX)) {
            if (!fSuccess) {
                GetDlgItemText(hDlg, pvinf->id, achText, ARRAYSIZE(achText));
                if (lstrcmpi(achText, g_szNone) == 0 ||
                    lstrcmpi(achText, g_szAuto) == 0) {
                    continue;   // things be lookin' good, check next int...
                }
            }
        }
        if (!fSuccess || dw < (DWORD)pvinf->iMin || dw > (DWORD)pvinf->iMax) {
            Warning(hDlg, pvinf->idMsg, MB_ICONEXCLAMATION | MB_OK, pvinf->iMin, pvinf->iMax);
            SendDlgItemMessage(hDlg, pvinf->id, EM_SETSEL, 0, MAKELPARAM(0,-1));
            SetFocus(GetDlgItem(hDlg, pvinf->id));
            return TRUE;        // things be lookin' bad, bail out...
        }
    } while (++pvinf, --cvinf);
    return FALSE;
}


/*
 * NOTE -- The compiler emits really bad code for some of these guys.
 * In those cases, we are merely wrapping a call; there is no need to save BP.
 */


/** LimitDlgItemText - Sets the limit for a dialog edit control
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  iCtl = ID of control
 *  uiLimit = text limit
 *
 * OUTPUT
 *  None.
 */
void LimitDlgItemText(HWND hDlg, int iCtl, UINT uiLimit)
{
    FunctionName(LimitDlgItemText);

    SendDlgItemMessage(hDlg, iCtl, EM_LIMITTEXT, uiLimit, 0);
}


/** SetDlgItemPosRange - Sets the pos and range for a dialog slider control
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  iCtl = ID of control
 *  uiPos = Current position
 *  dwRange = Range (min in low word, max in high word)
 *
 * OUTPUT
 *  None.
 */
void SetDlgItemPosRange(HWND hDlg, int iCtl, UINT uiPos, DWORD dwRange)
{
    FunctionName(SetDlgItemPosRange);

    SendDlgItemMessage(hDlg, iCtl, TBM_SETRANGE, 0, dwRange);
    SendDlgItemMessage(hDlg, iCtl, TBM_SETPOS, TRUE, uiPos);
}


/** GetDlgItemPos - Gets the pos of a dialog slider control
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  iCtl = ID of control
 *
 * OUTPUT
 *  Trackbar position.
 */
UINT GetDlgItemPos(HWND hDlg, int iCtl)
{
    FunctionName(GetDlgItemPos);

    return (UINT)SendDlgItemMessage(hDlg, iCtl, TBM_GETPOS, 0, 0);
}


/** SetDlgItemPct - Sets the pos for a dialog slider control that measures %
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  iCtl = ID of control
 *  uiPct = Current position (range 0 .. 100)
 *
 * OUTPUT
 *  None.
 */
void SetDlgItemPct(HWND hDlg, int iCtl, UINT uiPct)
{
    FunctionName(SetDlgItemPct);

    SetDlgItemPosRange(hDlg, iCtl, uiPct / (100/NUM_TICKS), MAKELONG(0, NUM_TICKS));
}


/** GetDlgItemPct - Gets the pos of a dialog slider control that measures %
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  iCtl = ID of control
 *
 * OUTPUT
 *  Slider position in the range 0 .. 100.
 */
UINT GetDlgItemPct(HWND hDlg, int iCtl)
{
    FunctionName(GetDlgItemPct);

    return GetDlgItemPos(hDlg, iCtl) * (100/NUM_TICKS);
}


/** EnableDlgItems - Enables or disables a collection of dialog controls
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  pbinf -> array of bitinfo descriptors
 *  cbinf  = size of array
 *  fEnable = whether the control should be enabled or disabled
 *
 * OUTPUT
 *  Dialog items have changed state.
 */
void EnableDlgItems(HWND hDlg, PBINF pbinf, int cbinf, BOOL fEnable)
{
    FunctionName(EnableDlgItems);

    ASSERTTRUE(cbinf > 0);
    do {
        EnableWindow(GetDlgItem(hDlg, pbinf->id), fEnable);
    } while (++pbinf, --cbinf);
}


/** DisableDlgItems - Disables a collection of dialog controls
 *
 * Most of the time, we call EnableDlgItems with fEnable = TRUE, so
 * this is a handy wrapper.  (Too bad the compiler emits awful code.)
 *
 * INPUT
 *  hDlg = HWND of dialog box
 *  pbinf -> array of bitinfo descriptors
 *  cbinf  = size of array
 *
 * OUTPUT
 *  Dialog items have been disabled.
 */
void DisableDlgItems(HWND hDlg, PBINF pbinf, int cbinf)
{
    FunctionName(DisableDlgItems);

    EnableDlgItems(hDlg, pbinf, cbinf, 0);
}


/** AdjustRealModeControls - Disables selected items if single-app mode
 *
 *  If the proplink says that "single-application mode" is enabled,
 *  then hide all controls whose IDs are less than 4000 and show all
 *  controls whose IDs are greater than or equal to 5000.  Controls whose
 *  IDs are in the 4000's are immune to all this hiding/showing.  Controls
 *  in the 3000's are actually disabled rather than hidden.  Controls in
 *  the 6000's are actually disabled rather than hidden as well.
 *
 *  RST: Ok, this is nice in theory, but now that we've pulled over this
 *       stuff into shell32.dll, we'll have to go off the actual IDC_
 *       defines instead of the magic #'s of 3000, 4000 and 5000.
 *
 *       IDC_ICONBMP        == 3001
 *       IDC_PIF_STATIC     == 4000
 *       IDC_REALMODEISABLE == 5001
 *
 *       So, when adding things to shell232.rc or ids.h, plan
 *       accordingly.
 *
 * INPUT
 *  ppl = proplink
 *  hDlg = HWND of dialog box
 *
 * OUTPUT
 *  Dialog items have been disabled/enabled shown/hidden.
 *  Returns nonzero if we are in normal (not single-app) mode.
 */

BOOL CALLBACK EnableEnumProc(HWND hwnd, LPARAM lp)
{
    int f;
    LONG l;

    f = SW_SHOW;
    l = GetWindowLong(hwnd, GWL_ID);

    if (!LOWORD(lp) && l < IDC_PIF_STATIC || LOWORD(lp) && l >= IDC_REALMODEDISABLE)
        f = SW_HIDE;

    if (l < IDC_ICONBMP || l >= IDC_PIF_STATIC && l < IDC_CONFIGLBL)
        ShowWindow(hwnd, f);
    else
        EnableWindow(hwnd, f == SW_SHOW);

    return TRUE;
}


BOOL AdjustRealModeControls(PPROPLINK ppl, HWND hDlg)
{
    BOOL fNormal;
    FunctionName(AdjustRealModeControls);

    fNormal = !(ppl->flProp & PROP_REALMODE);
    EnumChildWindows(hDlg, EnableEnumProc, fNormal);
    return fNormal;
}


/** OnWmHelp - Handle a WM_HELP message
 *
 *  This is called whenever the user presses F1 or clicks the help
 *  button in the title bar.  We forward the call on to the help engine.
 *
 * INPUT
 *  lparam  = LPARAM from WM_HELP message (LPHELPINFO)
 *  pdwHelp = array of DWORDs of help info
 *
 * OUTPUT
 *
 *  None.
 */

void OnWmHelp(LPARAM lparam, const DWORD *pdwHelp)
{
    FunctionName(OnWmHelp);

    WinHelp((HWND) ((LPHELPINFO) lparam)->hItemHandle, NULL,
            HELP_WM_HELP, (DWORD_PTR) (LPTSTR) pdwHelp);
}

/** OnWmContextMenu - Handle a WM_CONTEXTMENU message
 *
 *  This is called whenever the user right-clicks on a control.
 *  We forward the call on to the help engine.
 *
 * INPUT
 *  wparam  = WPARAM from WM_HELP message (HWND)
 *  pdwHelp = array of DWORDs of help info
 *
 * OUTPUT
 *
 *  None.
 */

void OnWmContextMenu(WPARAM wparam, const DWORD *pdwHelp)
{
    FunctionName(OnWmContextMenu);

    WinHelp((HWND) wparam, NULL, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPTSTR) pdwHelp);
}

#ifdef UNICODE
/** PifMgr_WCtoMBPath - Converts UNICODE path to it's ANSI representation
 *
 *  This is called whenever we need to convert a UNICODE path to it's
 *  best approximation in ANSI.  Sometimes this will be a direct mapping,
 *  but sometimes not.  We may have to use the short name, etc.
 *
 * INPUT
 *  lpUniPath  -> pointer UNICODE path (NULL terminated)
 *  lpAnsiPath -> pointer to buffer to hold ANSI path
 *  cchBuf     -> size of ANSI buffer, in characters
 *
 * OUTPUT
 *
 *  lpAnsiPath buffer contains ANSI representation of lpUniPath
 */

void PifMgr_WCtoMBPath(LPWSTR lpUniPath, LPSTR lpAnsiPath, UINT cchBuf )
{
    WCHAR awchPath[ MAX_PATH ]; // Should be bigger than any PIF string
    CHAR  achPath[ MAX_PATH ];  // Should be bigger than any PIF string
    UINT  cchAnsi = 0;

    FunctionName(PifMgr_WCtoMBPath);

    // Try converting to Ansi and then converting back and comparing.
    // If we get back exactly what we started with, this is the "simple"
    // case.

    cchAnsi = WideCharToMultiByte( CP_ACP, 0,
                                   lpUniPath, -1,
                                   achPath, MAX_PATH,
                                   NULL, NULL );

    if (cchAnsi && (cchAnsi<=cchBuf)) {

        // Now try converting back
        MultiByteToWideChar( CP_ACP, 0,
                             achPath, -1,
                             awchPath, MAX_PATH
                            );

        if (lstrcmp(lpUniPath,awchPath)==0) {

            // We're done...copy over the string.
            lstrcpynA( lpAnsiPath, achPath, cchBuf );
            *(BYTE UNALIGNED *)(lpAnsiPath+cchBuf-1) = '\0';
            return;

        }

        // Well, the string has some unmappable UNICODE
        // character in it, so try option #2 -- using the
        // short path name.
        goto TryShortPathName;

    } else {

TryShortPathName:
        // Hmmm, the best we can do is to use the short path name and map
        // it to ANSI.

        GetShortPathName( lpUniPath, awchPath, MAX_PATH );
        awchPath[ MAX_PATH-1 ] = TEXT('\0');
        WideCharToMultiByte( CP_ACP, 0,
                             awchPath, -1,
                             lpAnsiPath, cchBuf,
                             NULL, NULL
                            );

        // Make sure we're NULL terminated
        *(BYTE UNALIGNED *)(lpAnsiPath+cchBuf-1) = '\0';

    }

}
#endif
