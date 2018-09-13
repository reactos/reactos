/****************************** Module Header ******************************\
* Module Name: drawtext.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains common text drawing functions.
*
* History:
* 02-12-92 mikeke   Moved Drawtext to the client side
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CR 13
#define LF 10

#define DT_HFMTMASK 0x03

/***************************************************************************\
* IsMetaFile
*
* History:
* 30-Nov-1992 mikeke    Created
\***************************************************************************/

BOOL IsMetaFile(
    HDC hdc)
{
    DWORD dwType = GetObjectType(hdc);
    return (dwType == OBJ_METAFILE ||
            dwType == OBJ_METADC ||
            dwType == OBJ_ENHMETAFILE ||
            dwType == OBJ_ENHMETADC);
}

/***************************************************************************\
* DrawTextA (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/


CONST WCHAR gwszNullStr[] = L"";

int DrawTextExA(
    HDC hdc,
    LPSTR lpchText,
    int cchText,
    LPRECT lprc,
    UINT format,
    LPDRAWTEXTPARAMS lpdtp)
{
    LPWSTR lpwstr;
    int iRet;
    int iUniString;
    WORD wCodePage = (WORD)GdiGetCodePage(hdc);

    if (cchText == -1) {
        // USER_AWCONV_COUNTSTRINGSZ does not count/convert trailing \0.
        cchText = USER_AWCONV_COUNTSTRINGSZ;
    } else if (cchText < -1) {
        return 0;
    }

    if ((iUniString = MBToWCSEx(wCodePage, lpchText, cchText, &lpwstr, -1, TRUE)) == 0) {
        if (cchText == USER_AWCONV_COUNTSTRINGSZ) {
            lpwstr = (LPWSTR)gwszNullStr;
            format &= ~DT_MODIFYSTRING;
        } else {
            return 0;
        }
    }

    /*
     * Grow the buffer to accomodate the ellipsis (see AddEllipsisAndDrawLine)
     */
    if (format & DT_MODIFYSTRING) {
        int iNewLen = (iUniString + CCHELLIPSIS + 1) * sizeof(*lpwstr);
        LPWSTR lpwstrNew = UserLocalReAlloc(lpwstr, iNewLen, HEAP_ZERO_MEMORY);
        if (lpwstrNew == NULL) {
            UserLocalFree((HANDLE)lpwstr);
            return FALSE;
        }
        lpwstr = lpwstrNew;
    }

    iRet = DrawTextExWorker(hdc, lpwstr, iUniString, lprc, format, lpdtp, GetTextCharset(hdc));

    if (format & DT_MODIFYSTRING) {
        /*
         * Note that if the buffer grew and the caller provided the string size,
         *  then we won't return the additional characters... fixing this
         *  might break some apps so let's leave it alone until some one complains
         */
        if (cchText < 0) {
            UserAssert(cchText == USER_AWCONV_COUNTSTRINGSZ);
            // Guess how many bytes we can put in the buffer...
            // We can safely assume the maximum bytes available.
            // At worst, even for DBCS, the buffer size required
            // will be smaller than or equal to the orignal size,
            // because some DBCS characters would be substituted
            // to SBC ".", which is one byte each.
            // On the other hand, the number of characters converted
            // is limited by both iUniString and cchText.
            //
            if (IS_DBCS_ENABLED()) {
                cchText = iUniString * DBCS_CHARSIZE;
            } else {
                cchText = iUniString * sizeof(CHAR);
            }
        }
        WCSToMBEx(wCodePage, lpwstr, iUniString, &lpchText, cchText, FALSE);
    }

    if (lpwstr != gwszNullStr) {
        UserLocalFree((HANDLE)lpwstr);
    }

    return iRet;
}

/***************************************************************************\
* DrawTextW (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

int DrawTextW(
    HDC hdc,
    LPCWSTR lpchText,
    int cchText,
    LPRECT lprc,
    UINT format)
{
    DRAWTEXTPARAMS      DTparams;
    LPDRAWTEXTPARAMS    lpDTparams = NULL;

    /* v-ronaar: fix bug #24985
     * Disallow negative string lengths, except -1 (which has special meaning).
     */
    if (cchText < -1)
        return(0);

    if (format & DT_TABSTOP)
    {
        DTparams.cbSize      = sizeof(DRAWTEXTPARAMS);
        DTparams.iLeftMargin = DTparams.iRightMargin = 0;
        DTparams.iTabLength  = (format & 0xff00) >> 8;
        lpDTparams           = &DTparams;
        format              &= 0xffff00ff;
    }

    return DrawTextExW(hdc, (LPWSTR)lpchText, cchText, lprc, format, lpDTparams);
}

/***************************************************************************\
* DrawTextA (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

int DrawTextA(
    HDC hdc,
    LPCSTR lpchText,
    int cchText,
    LPRECT lprc,
    UINT format)
{
    DRAWTEXTPARAMS   DTparams;
    LPDRAWTEXTPARAMS lpDTparams = NULL;

    /* v-ronaar: fix bug #24985
     * Disallow negative string lengths, except -1 (which has special meaning).
     */
    if (cchText < -1)
        return(0);

    if (format & DT_TABSTOP) {
        DTparams.cbSize      = sizeof(DRAWTEXTPARAMS);
        DTparams.iLeftMargin = DTparams.iRightMargin = 0;
        DTparams.iTabLength  = (format & 0xff00) >> 8;
        lpDTparams           = &DTparams;
        format              &= 0xffff00ff;
    }

    return DrawTextExA(hdc, (LPSTR)lpchText, cchText, lprc, format, lpDTparams);
}

/***************************************************************************\
* ClientTabTheTextOutForWimps
*
* effects: Outputs the tabbed text if fDrawTheText is TRUE and returns the
* textextent of the tabbed text.
*
* nCount                    Count of bytes in string
* nTabPositions             Count of tabstops in tabstop array
* lpintTabStopPositions     Tab stop positions in pixels
* iTabOrigin                Tab stops are with respect to this
*
* History:
* 19-Jan-1993 mikeke   Client side
* 13-Sep-1996 GregoryW This routine now calls the LPK(s) to handle text out.
*                      If no LPKs are installed, this defaults to calling
*                      UserLpkTabbedTextOut (identical behavior to what we
*                      had before supporting LPKs).
\***************************************************************************/

LONG TabTextOut(
    HDC hdc,
    int x,
    int y,
    LPCWSTR lpstring,
    int nCount,
    int nTabPositions,
    CONST INT *lpTabPositions,
    int iTabOrigin,
    BOOL fDrawTheText,
    int iCharset)
{
    int     cxCharWidth;
    int     cyCharHeight = 0;

    if (nCount == -1 && lpstring) {
        nCount = wcslen(lpstring);
    }
    if (!lpstring || nCount < 0 || nTabPositions < 0)
        return 0;


    // Check if it is SysFont AND the mapping mode is MM_TEXT;
    // Fix made in connection with Bug #8717 --02-01-90  --SANKAR--
    if (IsSysFontAndDefaultMode(hdc))
    {
        cxCharWidth  = gpsi->cxSysFontChar;
        cyCharHeight = gpsi->cySysFontChar;
    } else {
        cxCharWidth  = GdiGetCharDimensions(hdc, NULL, &cyCharHeight);
        if (cxCharWidth == 0) {
            RIPMSG0(RIP_WARNING, "TabTextOut: GdiGetCharDimensions failed");
            return 0;
        }
    }

    return (*fpLpkTabbedTextOut)(hdc, x, y, lpstring, nCount, nTabPositions,
                                 lpTabPositions, iTabOrigin, fDrawTheText,
                                 cxCharWidth, cyCharHeight, iCharset);
}

LONG UserLpkTabbedTextOut(
    HDC hdc,
    int x,
    int y,
    LPCWSTR lpstring,
    int nCount,
    int nTabPositions,
    CONST INT *lpTabPositions,
    int iTabOrigin,
    BOOL fDrawTheText,
    int cxCharWidth,
    int cyCharHeight,
    int iCharset)
{
    SIZE textextent, viewextent, windowextent;
    int     initialx = x;
    int     cch;
    LPCWSTR  lp;
    int     iOneTab = 0;
    RECT rc;
    UINT uOpaque = (GetBkMode(hdc) == OPAQUE) ? ETO_OPAQUE : 0;
    BOOL    fStrStart = TRUE;
    int     ySign = 1; //Assume y increases in down direction.

    UNREFERENCED_PARAMETER(iCharset);   //Needed by lpk, but not us
    /*
     * If no tabstop positions are specified, then use a default of 8 system
     * font ave char widths or use the single fixed tab stop.
     */
    if (!lpTabPositions) {
       // no tab stops specified -- default to a tab stop every 8 characters
        iOneTab = 8 * cxCharWidth;
    } else if (nTabPositions == 1) {
        // one tab stop specified -- treat value as the tab increment, one
        // tab stop every increment
            iOneTab = lpTabPositions[0];

        if (!iOneTab)
             iOneTab = 1;
    }

    // Calculate if the y increases or decreases in the down direction using
    // the ViewPortExtent and WindowExtents.
    // If this call fails, hdc must be invalid
    if (!GetViewportExtEx(hdc, &viewextent))
        return 0;
    GetWindowExtEx(hdc, &windowextent);
    if ((viewextent.cy ^ windowextent.cy) & 0x80000000)
         ySign = -1;

    rc.left = initialx;
    rc.top = y;
    rc.bottom = rc.top + (ySign * cyCharHeight);

    while (TRUE) {
        // count the number of characters until the next tab character
        // this set of characters (substring) will be the working set for
        // each iteration of this loop
        for (cch = nCount, lp = lpstring; cch && (*lp != TEXT('\t')); lp++, cch--)
        {
        }

        // Compute the number of characters to be drawn with textout.
        cch = nCount - cch;

        // Compute the number of characters remaining.
        nCount -= cch + 1;

        // get height and width of substring
        if (cch == 0) {
            textextent.cx = 0;
            textextent.cy = cyCharHeight;
        } else
            GetTextExtentPointW(hdc, lpstring, cch, &textextent);

        if (fStrStart)
            // first iteration should just spit out the first substring
            // no tabbing occurs until the first tab character is encountered
            fStrStart = FALSE;
        else
        {
           // not the first iteration -- tab accordingly

            int xTab;
            int i;

            if (!iOneTab)
            {
                // look thru tab stop array for next tab stop after existing
                // text to put this substring
                for (i = 0; i < nTabPositions; i++)
                {
                    xTab = lpTabPositions[i];

                    if (xTab < 0)
                        // calc length needed to use this right justified tab
                        xTab = (iTabOrigin - xTab) - textextent.cx;
                    else
                        // calc length needed to use this left  justified tab
                        xTab = iTabOrigin + xTab;

                    if (x < xTab)
                    {
                        // we found a tab with enough room -- let's use it
                        x = xTab;
                        break;
                    }
                }

                if (i == nTabPositions)
                    // we've exhausted all of the given tab positions
                    // go back to default of a tab stop every 8 characters
                    iOneTab = 8 * cxCharWidth;
            }

            // we have to recheck iOneTab here (instead of just saying "else")
            // because iOneTab will be set if we've run out of tab stops
            if (iOneTab)
            {
                if (iOneTab < 0)
                {
                    // calc next available right justified tab stop
                    xTab = x + textextent.cx - iTabOrigin;
                    xTab = ((xTab / iOneTab) * iOneTab) - iOneTab - textextent.cx + iTabOrigin;
                }
                else
                {
                    // calc next available left justified tab stop
                    xTab = x - iTabOrigin;
                    xTab = ((xTab / iOneTab) * iOneTab) + iOneTab + iTabOrigin;
                }
                x = xTab;
            }
        }

        if (fDrawTheText) {

            /*
             * Output all text up to the tab (or end of string) and get its
             * extent.
             */
            rc.right = x + textextent.cx;
            ExtTextOutW(
                    hdc, x, y, uOpaque, &rc, (LPWSTR)lpstring,
                    cch, NULL);
            rc.left = rc.right;
        }

        // Skip over the tab and the characters we just drew.
        x += textextent.cx;

        // Skip over the characters we just drew.
        lpstring += cch;

        // See if we have more to draw OR see if this string ends in
        // a tab character that needs to be drawn.
        if((nCount > 0) || ((nCount == 0) && (*lpstring == TEXT('\t'))))
        {

            lpstring++;  // Skip over the tab
            continue;
        }
        else
            break;        // Break from the loop.
    }
    return MAKELONG((x - initialx), (short)textextent.cy);
}



/***************************************************************************\
*  TabbedTextOutW
*
* effects: Outputs the tabbed text and returns the
* textextent of the tabbed text.
*
* nCount                    Count of bytes in string
* nTabPositions             Count of tabstops in tabstop array
* lpintTabStopPositions     Tab stop positions in pixels
* iTabOrigin                Tab stops are with respect to this
*
* History:
* 19-Jan-1993 mikeke   Client side
\***************************************************************************/

LONG TabbedTextOutW(
    HDC hdc,
    int x,
    int y,
    LPCWSTR lpstring,
    int cchChars,
    int nTabPositions,
    CONST INT *lpintTabStopPositions,
    int iTabOrigin)
{
    return TabTextOut(hdc, x, y, lpstring, cchChars,
        nTabPositions, lpintTabStopPositions, iTabOrigin, TRUE, -1);
}

/***************************************************************************\
* TabbedTextOutA (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

LONG TabbedTextOutA(
    HDC hdc,
    int x,
    int y,
    LPCSTR pString,
    int chCount,
    int nTabPositions,
    CONST INT *pnTabStopPositions,
    int nTabOrigin)
{
    LPWSTR lpwstr;
    BOOL bRet;
    WORD wCodePage = (WORD)GdiGetCodePage(hdc);
    int  iUniString;

    if (chCount == -1) {
        chCount = USER_AWCONV_COUNTSTRINGSZ;
    }

    if ((iUniString = MBToWCSEx(wCodePage, pString, chCount, &lpwstr, -1, TRUE)) == 0) {
        if (chCount == USER_AWCONV_COUNTSTRINGSZ) {
            lpwstr = (LPWSTR)gwszNullStr;
        } else {
            return FALSE;
        }
    }

    bRet = TabTextOut(
            hdc, x, y, lpwstr, iUniString, nTabPositions,
            pnTabStopPositions, nTabOrigin, TRUE, GetTextCharset(hdc));

    if (lpwstr != gwszNullStr) {
        UserLocalFree((HANDLE)lpwstr);
    }

    return bRet;
}

DWORD GetTabbedTextExtentW(
    HDC hdc,
    LPCWSTR pString,
    int chCount,
    int nTabPositions,
    CONST INT *pnTabStopPositions)
{
    return TabTextOut(hdc, 0, 0, pString, chCount,
        nTabPositions, pnTabStopPositions, 0, FALSE, -1);
}

DWORD GetTabbedTextExtentA(
    HDC hdc,
    LPCSTR pString,
    int chCount,
    int nTabPositions,
    CONST INT *pnTabStopPositions)
{
    LPWSTR lpwstr;
    BOOL bRet;
    WORD wCodePage = (WORD)GdiGetCodePage(hdc);
    int iUniString;

    if (chCount == -1) {
        chCount = USER_AWCONV_COUNTSTRINGSZ;
    }
    if ((iUniString = MBToWCSEx(wCodePage, pString, chCount, &lpwstr, -1, TRUE)) == 0) {
        if (chCount == USER_AWCONV_COUNTSTRINGSZ) {
            lpwstr = (LPWSTR)gwszNullStr;
        } else {
            return FALSE;
        }
    }

    bRet = TabTextOut(hdc, 0, 0, lpwstr, iUniString,
        nTabPositions, pnTabStopPositions, 0, FALSE, GetTextCharset(hdc));

    if (lpwstr != gwszNullStr) {
        UserLocalFree((HANDLE)lpwstr);
    }

    return bRet;
}


/***************************************************************************\
* PSMTextOut
*
* Outputs the text and puts and _ below the character with an &
* before it. Note that this routine isn't used for menus since menus
* have their own special one so that it is specialized and faster...
*
* History:
* 11-13-90 JimA         Ported to NT.
* 30-Nov-1992 mikeke    Client side version
* 7-Apr-1998 MCostea    Added dwFlags
\***************************************************************************/

void PSMTextOut(
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR lpsz,
    int cch,
    DWORD dwFlags)
{
    /*
     * By default this is just a call to UserLpkPSMTextOut.  If an
     * LPK is installed, this calls out to the LPK.  The LPK calls
     * UserLpkPSMTextOut, if necessary.
     */
    (*fpLpkPSMTextOut)(hdc, xLeft, yTop, lpsz, cch, dwFlags);
    return;
}

/***************************************************************************\
* UserLpkPSMTextOut
*
* NOTE: A very similar routine (xxxPSMTextOut) exists on the kernel
*       side in text.c.  Any changes to this routine most likely need
*       to be made in xxxPSMTextOut as well.
*
\***************************************************************************/
void UserLpkPSMTextOut(
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR lpsz,
    int cch,
    DWORD dwFlags)
{
   int cx;
   LONG textsize, result;
   WCHAR achWorkBuffer[255];
   WCHAR *pchOut = achWorkBuffer;
   TEXTMETRICW textMetric;
   SIZE size;
   RECT rc;
   COLORREF color;

   if (cch > sizeof(achWorkBuffer)/sizeof(WCHAR)) {
       pchOut = (WCHAR*)UserLocalAlloc(HEAP_ZERO_MEMORY, (cch+1) * sizeof(WCHAR));
       if (pchOut == NULL)
           return;
   }

   result = GetPrefixCount(lpsz, cch, pchOut, cch);
   /*
    * DT_PREFIXONLY is a new 5.0 option used when switching from keyboard cues off
    *  to on.
    */
   if (!(dwFlags & DT_PREFIXONLY)) {
       TextOutW(hdc, xLeft, yTop, pchOut, cch - HIWORD(result));
   }

   /*
    * Any true prefix characters to underline?
    */
   if (LOWORD(result) == 0xFFFF || dwFlags & DT_HIDEPREFIX) {
       if (pchOut != achWorkBuffer)
           UserLocalFree(pchOut);
       return;
   }

   if (!GetTextMetricsW(hdc, &textMetric)) {
       textMetric.tmOverhang = 0;
       textMetric.tmAscent = 0;
   }

   /*
    * For proportional fonts, find starting point of underline.
    */
   if (LOWORD(result) != 0) {

       /*
        * How far in does underline start (if not at 0th byte.).
        */
       GetTextExtentPointW(hdc, pchOut, LOWORD(result), &size);
       xLeft += size.cx;

       /*
        * Adjust starting point of underline if not at first char and there is
        * an overhang.  (Italics or bold fonts.)
        */
       xLeft = xLeft - textMetric.tmOverhang;
   }

   /*
    * Adjust for proportional font when setting the length of the underline and
    * height of text.
    */
   GetTextExtentPointW(hdc, pchOut + LOWORD(result), 1, &size);
   textsize = size.cx;

   /*
    * Find the width of the underline character.  Just subtract out the overhang
    * divided by two so that we look better with italic fonts.  This is not
    * going to effect embolded fonts since their overhang is 1.
    */
   cx = LOWORD(textsize) - textMetric.tmOverhang / 2;

   /*
    * Get height of text so that underline is at bottom.
    */
   yTop += textMetric.tmAscent + 1;

   /*
    * Draw the underline using the foreground color.
    */
   SetRect(&rc, xLeft, yTop, xLeft+cx, yTop+1);
   color = SetBkColor(hdc, GetTextColor(hdc));
   ExtTextOutW(hdc, xLeft, yTop, ETO_OPAQUE, &rc, TEXT(""), 0, NULL);
   SetBkColor(hdc, color);

   if (pchOut != achWorkBuffer) {
       UserLocalFree(pchOut);
   }
}
