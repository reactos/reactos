/***************************************************************************\
* editml.c - Edit controls rewrite. Version II of edit controls.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Multi-Line Support Routines
*
* Created: 24-Jul-88 davidds
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Number of lines to bump when reallocating index buffer
 */
#define LINEBUMP 32

/*
 * Used for ML scroll updates
 */
#define ML_REFRESH  0xffffffff

__inline void MLSanityCheck(PED ped)
{
    UNREFERENCED_PARAMETER(ped);    // For free build

    UserAssert(ped->cch >= ped->chLines[ped->cLines - 1]);
}


/***************************************************************************\
*
*  MLGetLineWidth()
*
*  Returns the max width in a line.  ECTabTheTextOut() ensures that max
*  width won't overflow.
*
\***************************************************************************/
UINT MLGetLineWidth(HDC hdc, LPSTR lpstr, int nCnt, PED ped)
{
    return(ECTabTheTextOut(hdc, 0, 0, 0, 0, lpstr, nCnt, 0, ped, 0, ECT_CALC, NULL));
}

/***************************************************************************\
*
*  MLSize()
*
*  Handles resizing of the edit control window and updating thereof.
*
*  Sets the edit field's formatting area given the passed in "client area".
*  We fudge it if it doesn't seem reasonable.
*
\***************************************************************************/

void   MLSize(PED ped, BOOL fRedraw)
{
    // Calculate the # of lines we can fit in our rectangle.
    ped->ichLinesOnScreen = (ped->rcFmt.bottom - ped->rcFmt.top) / ped->lineHeight;

    // Make the format rectangle height an integral number of lines
    ped->rcFmt.bottom = ped->rcFmt.top + ped->ichLinesOnScreen * ped->lineHeight;

    // Rebuild the line array
    if (ped->fWrap) {
        MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);
        MLUpdateiCaretLine(ped);
    } else {
        MLScroll(ped, TRUE,  ML_REFRESH, 0, fRedraw);
        MLScroll(ped, FALSE, ML_REFRESH, 0, fRedraw);
    }
}

/***************************************************************************\
* MLCalcXOffset AorW
*
* Calculates the horizontal offset (indent) required for centered
* and right justified lines.
*
* History:
*
* Not used if language pack loaded.
\***************************************************************************/

int MLCalcXOffset(
    PED ped,
    HDC hdc,
    int lineNumber)
{
    PSTR pText;
    ICH lineLength;
    ICH lineWidth;

    if (ped->format == ES_LEFT)
        return (0);

    lineLength = MLLine(ped, lineNumber);

    if (lineLength) {

        pText = ECLock(ped) + ped->chLines[lineNumber] * ped->cbChar;
        hdc = ECGetEditDC(ped, TRUE);
        lineWidth = MLGetLineWidth(hdc, pText, lineLength, ped);
        ECReleaseEditDC(ped, hdc, TRUE);
        ECUnlock(ped);
    } else {
        lineWidth = 0;
    }

    /*
     * If a SPACE or a TAB was eaten at the end of a line by MLBuildchLines
     * to prevent a delimiter appearing at the begining of a line, the
     * the following calculation will become negative causing this bug.
     * So, now, we take zero in such cases.
     * Fix for Bug #3566 --01/31/91-- SANKAR --
     */
    lineWidth = max(0, (int)(ped->rcFmt.right-ped->rcFmt.left-lineWidth));

    if (ped->format == ES_CENTER)
        return (lineWidth / 2);

    if (ped->format == ES_RIGHT) {

        /*
         * Subtract 1 so that the 1 pixel wide cursor will be in the visible
         * region on the very right side of the screen.
         */
        return max(0, (int)(lineWidth-1));
    }

    return 0;
}

/***************************************************************************\
* MLMoveSelection AorW
*
* Moves the selection character in the direction indicated. Assumes
* you are starting at a legal point, we decrement/increment the ich. Then,
* This decrements/increments it some more to get past CRLFs...
*
* History:
\***************************************************************************/

ICH MLMoveSelection(
    PED ped,
    ICH ich,
    BOOL fLeft)
{

    if (fLeft && ich > 0) {

        /*
         * Move left
         */
        ich = ECPrevIch( ped, NULL, ich );
        if (ich) {
            if (ped->fAnsi) {
                LPSTR pText;

                /*
                 * Check for CRLF or CRCRLF
                 */
                pText = ECLock(ped) + ich;

                /*
                 * Move before CRLF or CRCRLF
                 */
                if (*(WORD UNALIGNED *)(pText - 1) == 0x0A0D) {
                    ich--;
                    if (ich && *(pText - 2) == 0x0D)
                        ich--;
                }
                ECUnlock(ped);
            } else { // !fAnsi
                LPWSTR pwText;

                /*
                 * Check for CRLF or CRCRLF
                 */
                pwText = (LPWSTR)ECLock(ped) + ich;

                /*
                 * Move before CRLF or CRCRLF
                 */
                if (*(pwText - 1) == 0x0D && *pwText == 0x0A) {
                    ich--;
                    if (ich && *(pwText - 2) == 0x0D)
                        ich--;
                }
                ECUnlock(ped);
            }
        }
    } else if (!fLeft && ich < ped->cch) {
        /*
         * Move right.
         */
        ich = ECNextIch( ped, NULL, ich );
        if (ich < ped->cch) {
            if (ped->fAnsi) {
                LPSTR pText;
                pText = ECLock(ped) + ich;

                /*
                 * Move after CRLF
                 */
                if (*(WORD UNALIGNED *)(pText - 1) == 0x0A0D)
                    ich++;
                else {

                    /*
                     * Check for CRCRLF
                     */
                    if (ich && *(WORD UNALIGNED *)pText == 0x0A0D && *(pText - 1) == 0x0D)
                        ich += 2;
                }
                ECUnlock(ped);
            } else { // !fAnsi
                LPWSTR pwText;
                pwText = (LPWSTR)ECLock(ped) + ich;

                /*
                 * Move after CRLF
                 */
                if (*(pwText - 1) == 0x0D && *pwText == 0x0A)
                    ich++;
                else {

                    /*
                     * Check for CRCRLF
                     */
                    if (ich && *(pwText - 1) == 0x0D && *pwText == 0x0D &&
                            *(pwText + 1) == 0x0A)
                        ich += 2;
                }
                ECUnlock(ped);
            }
        }
    }
    return (ich);
}

/***************************************************************************\
* MLMoveSelectionRestricted AorW
*
* Moves the selection like MLMoveSelection, but also obeys limitations
* imposed by some languages such as Thai, where the cursor cannot stop
* between a character and it's attached vowel or tone marks.
*
* Only called if the language pack is loaded.
*
\***************************************************************************/

/***************************************************************************\
* MLMoveSelectionRestricted AorW
*
* Moves the selection like MLMoveSelection, but also obeys limitations
* imposed by some languages such as Thai, where the cursor cannot stop
* between a character and it's attached vowel or tone marks.
*
* Only called if the language pack is loaded.
*
\***************************************************************************/

ICH MLMoveSelectionRestricted(
    PED  ped,
    ICH  ich,
    BOOL fLeft)
{
    PSTR pText;
    HDC  hdc;
    ICH  ichResult;

    pText = ECLock(ped);
    hdc = ECGetEditDC(ped, TRUE);
    ichResult = ped->pLpkEditCallout->EditMoveSelection(ped, hdc, pText, ich, fLeft);
    ECReleaseEditDC(ped, hdc, TRUE);
    ECUnlock(ped);

    return ichResult;
}


/***************************************************************************\
* MLSetCaretPosition AorW
*
* If the window has the focus, find where the caret belongs and move
* it there.
*
* History:
\***************************************************************************/

void MLSetCaretPosition(
    PED ped,
    HDC hdc)
{
    POINT position;
    BOOL prevLine;
    int  x = -20000;
    int  y = -20000;

    /*
     * We will only position the caret if we have the focus since we don't want
     * to move the caret while another window could own it.
     */
    if (!ped->fFocus || !_IsWindowVisible(ped->pwnd))
         return;

    /*
     * Find the position of the caret
     */
    if (!ped->fCaretHidden &&
        ((ICH) ped->iCaretLine >= ped->ichScreenStart) &&
        ((ICH) ped->iCaretLine <  (ped->ichScreenStart + ped->ichLinesOnScreen))) {

        RECT    rcRealFmt;

        if (ped->f40Compat)
        {
            GetClientRect(ped->hwnd, &rcRealFmt);
            IntersectRect(&rcRealFmt, &rcRealFmt, &ped->rcFmt);
        } else {
            CopyRect(&rcRealFmt, &ped->rcFmt);
        }

        if (ped->cLines - 1 != ped->iCaretLine && ped->ichCaret == ped->chLines[ped->iCaretLine + 1]) {
            prevLine = TRUE;
        } else {
            prevLine = FALSE;
        }

        MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine, &position);

        if ( (position.y >= rcRealFmt.top) &&
             (position.y <= rcRealFmt.bottom - ped->lineHeight)) {
            int xPos = position.x;
            int cxCaret = ECGetCaretWidth();

            if (ped->fWrap ||
                ((xPos > (rcRealFmt.left - cxCaret)) &&
                 (xPos <= rcRealFmt.right))) {
                // Make sure the caret is in the visible region if word
                // wrapping. This is so that the caret will be visible if the
                // line ends with a space.
                x = max(xPos, rcRealFmt.left);
                x = min(x, rcRealFmt.right - cxCaret);
                y = position.y;
            }
        }
    }

    if (ped->pLpkEditCallout) {
        NtUserSetCaretPos(x + ped->iCaretOffset, y);
    } else {
        NtUserSetCaretPos(x, y);
    }

    // FE_IME : MLSetCaretPosition -- ImmSetCompositionWindow(CFS_RECT)
    if (fpImmIsIME(THREAD_HKL())) {
        if (x != -20000 && y != -20000) {
            ECImmSetCompositionWindow(ped, x, y);
        }
    }
}

/***************************************************************************\
* MLLine
*
* Returns the length of the line (cch) given by lineNumber ignoring any
* CRLFs in the line.
*
* History:
\***************************************************************************/

ICH MLLine(
    PED ped,
    ICH lineNumber)
{
    ICH result;

    UserAssert(lineNumber < ped->cLines);

    if (lineNumber >= ped->cLines)
        return (0);

    if (lineNumber == ped->cLines - 1) {

        /*
         * Since we can't have a CRLF on the last line
         */
        return (ped->cch - ped->chLines[ped->cLines - 1]);
    } else {
        result = ped->chLines[lineNumber + 1] - ped->chLines[lineNumber];
        RIPMSG1(RIP_VERBOSE, "MLLine result=%d\n", result);

        /*
         * Now check for CRLF or CRCRLF at end of line
         */
        if (result > 1) {
            if (ped->fAnsi) {
                LPSTR pText;

                pText = ECLock(ped) + ped->chLines[lineNumber + 1] - 2;
                if (*(WORD UNALIGNED *)pText == 0x0A0D) {
                    result -= 2;
                    if (result && *(--pText) == 0x0D)
                        /*
                         * In case there was a CRCRLF
                         */
                        result--;
                }
            } else { // !fAnsi
                LPWSTR pwText;

                pwText = (LPWSTR)ECLock(ped) +
                        (ped->chLines[lineNumber + 1] - 2);
                if (*(DWORD UNALIGNED *)pwText == 0x000A000D) {
                    result = result - 2;
                    if (result && *(--pwText) == 0x0D)
                        /*
                         * In case there was a CRCRLF
                         */
                        result--;
                }

            }
            ECUnlock(ped);
        }
    }
    return (result);
}


/***************************************************************************\
* MLIchToLine AorW
*
* Returns the line number (starting from 0) which contains the given
* character index. If ich is -1, return the line the first char in the
* selection is on (the caret if no selection)
*
* History:
\***************************************************************************/

int MLIchToLine(
    PED ped,
    ICH ich)
{
    int iLo, iHi, iLine;

    iLo = 0;
    iHi = ped->cLines;

    if (ich == (ICH)-1)
        ich = ped->ichMinSel;

    while (iLo < iHi - 1) {
        iLine = max((iHi - iLo)/2, 1) + iLo;

        if (ped->chLines[iLine] > ich) {
            iHi = iLine;
        } else {
            iLo = iLine;
        }
    }

    return iLo;
}

/***************************************************************************\
* MLIchToYPos
*
* Given an ich, return its y coordinate with respect to the top line
* displayed in the window. If prevLine is TRUE and if the ich is at the
* beginning of the line, return the y coordinate of the
* previous line (if it is not a CRLF).
*
* Added for the LPK (3Dec96) - with an LPK installed, calculating X position is
* a far more processor intensive job. Where only the Y position is required
* this routine should be called instead of MLIchToXYPos.
*
* Called only when LPK installed.
*
\***************************************************************************/


/***************************************************************************\
* MLIchToYPos
*
* Given an ich, return its y coordinate with respect to the top line
* displayed in the window. If prevLine is TRUE and if the ich is at the
* beginning of the line, return the y coordinate of the
* previous line (if it is not a CRLF).
*
* Added for the LPK (3Dec96) - with an LPK installed, calculating X position is
* a far more processor intensive job. Where only the Y position is required
* this routine should be called instead of MLIchToXYPos.
*
* Called only when LPK installed.
*
\***************************************************************************/


INT MLIchToYPos(
    PED  ped,
    ICH  ich,
    BOOL prevLine)
{
    int  iline;
    int  yPosition;
    PSTR pText;

    /*
     * Determine what line the character is on
     */
    iline = MLIchToLine(ped, ich);

    /*
     * Calc. the yPosition now. Note that this may change by the height of one
     * char if the prevLine flag is set and the ICH is at the beginning of a
     * line.
     */
    yPosition = (iline - ped->ichScreenStart) * ped->lineHeight + ped->rcFmt.top;

    pText = ECLock(ped);
    if (prevLine && iline && (ich == ped->chLines[iline]) &&
            (!AWCOMPARECHAR(ped, pText + (ich - 2) * ped->cbChar, 0x0D) ||
             !AWCOMPARECHAR(ped, pText + (ich - 1) * ped->cbChar, 0x0A))) {

        /*
         * First char in the line. We want Y position of the previous
         * line if we aren't at the 0th line.
         */
        iline--;

        yPosition = yPosition - ped->lineHeight;
    }
    ECUnlock(ped);

    return yPosition;
}

/***************************************************************************\
* MLIchToXYPos
*
* Given an ich, return its x,y coordinates with respect to the top
* left character displayed in the window. Returns the coordinates of the top
* left position of the char. If prevLine is TRUE then if the ich is at the
* beginning of the line, we will return the coordinates to the right of the
* last char on the previous line (if it is not a CRLF).
*
* History:
\***************************************************************************/

void MLIchToXYPos(
    PED ped,
    HDC hdc,
    ICH ich,
    BOOL prevLine,
    LPPOINT ppt)
{
    int iline;
    ICH cch;
    int xPosition, yPosition;
    int xOffset;

    /*
     * For horizontal scroll displacement on left justified text and
     * for indent on centered or right justified text
     */
    PSTR pText, pTextStart, pLineStart;

    /*
     * Determine what line the character is on
     */
    iline = MLIchToLine(ped, ich);

    /*
     * Calc. the yPosition now. Note that this may change by the height of one
     * char if the prevLine flag is set and the ICH is at the beginning of a
     * line.
     */
    yPosition = (iline - ped->ichScreenStart) * ped->lineHeight + ped->rcFmt.top;

    /*
     * Now determine the xPosition of the character
     */
    pTextStart = ECLock(ped);

    if (prevLine && iline && (ich == ped->chLines[iline]) &&
            (!AWCOMPARECHAR(ped, pTextStart + (ich - 2) * ped->cbChar, 0x0D) ||
            !AWCOMPARECHAR(ped, pTextStart + (ich - 1) * ped->cbChar, 0x0A))) {

        /*
         * First char in the line. We want text extent upto end of the previous
         * line if we aren't at the 0th line.
         */
        iline--;

        yPosition = yPosition - ped->lineHeight;
        pLineStart = pTextStart + ped->chLines[iline] * ped->cbChar;

        /*
         * Note that we are taking the position in front of any CRLFs in the
         * text.
         */
        cch = MLLine(ped, iline);

    } else {

        pLineStart = pTextStart + ped->chLines[iline] * ped->cbChar;
        pText = pTextStart + ich * ped->cbChar;

        /*
         * Strip off CRLF or CRCRLF. Note that we may be pointing to a CR but in
         * which case we just want to strip off a single CR or 2 CRs.
         */

        /*
         * We want pText to point to the first CR at the end of the line if
         * there is one. Thus, we will get an xPosition to the right of the last
         * visible char on the line otherwise we will be to the left of
         * character ich.
         */

        /*
         * Check if we at the end of text
         */
        if (ich < ped->cch) {
            if (ped->fAnsi) {
                if (ich && *(WORD UNALIGNED *)(pText - 1) == 0x0A0D) {
                    pText--;
                    if (ich > 2 && *(pText - 1) == 0x0D)
                        pText--;
                }
            } else {
                LPWSTR pwText = (LPWSTR)pText;

                if (ich && *(DWORD UNALIGNED *)(pwText - 1) == 0x000A000D) {
                    pwText--;
                    if (ich > 2 && *(pwText - 1) == 0x0D)
                        pwText--;
                }
                pText = (LPSTR)pwText;
            }
        }

        if (pText < pLineStart)
            pText = pLineStart;

        cch = (ICH)(pText - pLineStart)/ped->cbChar;
    }

    /*
     * Find out how many pixels we indent the line for funny formats
     */
    if (ped->pLpkEditCallout) {
        /*
         * Must find position at start of character offset cch from start of line.
         * This depends on the layout and the reading order
         */
        xPosition = ped->pLpkEditCallout->EditIchToXY(
                          ped, hdc, pLineStart, MLLine(ped, iline), cch);
    } else {
        if (ped->format != ES_LEFT) {
            xOffset = MLCalcXOffset(ped, hdc, iline);
        } else {
            xOffset = -(int)ped->xOffset;
        }

        xPosition = ped->rcFmt.left + xOffset +
                MLGetLineWidth(hdc, pLineStart, cch, ped);
    }

    ECUnlock(ped);
    ppt->x = xPosition;
    ppt->y = yPosition;
    return ;
}

/***************************************************************************\
* MLMouseToIch AorW
*
* Returns the closest cch to where the mouse point is.  Also optionally
* returns lineindex in pline (So that we can tell if we are at the beginning
* of the line or end of the previous line.)
*
* History:
\***************************************************************************/

ICH MLMouseToIch(
    PED ped,
    HDC hdc,
    LPPOINT mousePt,
    LPICH pline)
{
    int xOffset;
    LPSTR pLineStart;
    int height = mousePt->y;
    int line; //WASINT
    int width = mousePt->x;
    ICH cch;
    ICH cLineLength;
    ICH cLineLengthNew;
    ICH cLineLengthHigh;
    ICH cLineLengthLow;
    ICH cLineLengthTemp;
    int textWidth;
    int iCurWidth;
    int lastHighWidth, lastLowWidth;

    /*
     * First determine which line the mouse is pointing to.
     */
    line = ped->ichScreenStart;
    if (height <= ped->rcFmt.top) {

        /*
         * Either return 0 (the very first line, or one line before the top line
         * on the screen. Note that these are signed mins and maxes since we
         * don't expect (or allow) more than 32K lines.
         */
        line = max(0, line-1);
    } else if (height >= ped->rcFmt.bottom) {

        /*
         * Are we below the last line displayed
         */
        line = min(line+(int)ped->ichLinesOnScreen, (int)(ped->cLines-1));
    } else {

        /*
         * We are somewhere on a line visible on screen
         */
        line = min(line + (int)((height - ped->rcFmt.top) / ped->lineHeight),
                (int)(ped->cLines - 1));
    }

    /*
     * Now determine what horizontal character the mouse is pointing to.
     */
    pLineStart = ECLock(ped) + ped->chLines[line] * ped->cbChar;
    cLineLength = MLLine(ped, line); /* Length is sans CRLF or CRCRLF */
    RIPMSG3(RIP_VERBOSE, "MLLine(ped=%x, line=%d) returned %d\n", ped, line, cLineLength);
    UserAssert((int)cLineLength >= 0);

    /*
     * If the language pack is loaded, visual and logical character order
     * may differ.
     */
    if (ped->pLpkEditCallout) {
        /*
         * Use the language pack to find the character nearest the cursor.
         */
        cch = ped->chLines[line] + ped->pLpkEditCallout->EditMouseToIch
            (ped, hdc, pLineStart, cLineLength, width);
    } else {
        /*
         * xOffset will be a negative value for center and right justified lines.
         * ie. We will just displace the lines left by the amount of indent for
         * right and center justification. Note that ped->xOffset will be 0 for
         * these lines since we don't support horizontal scrolling with them.
         */
        if (ped->format != ES_LEFT) {
            xOffset = MLCalcXOffset(ped, hdc, line);
        } else {
            /*
             * So that we handle a horizontally scrolled window for left justified
             * text.
             */
            xOffset = 0;
        }

        width = width - xOffset;

        /*
         * The code below is tricky... I depend on the fact that ped->xOffset is 0
         * for right and center justified lines
         */

        /*
         * Now find out how many chars fit in the given width
         */
        if (width >= ped->rcFmt.right) {

            /*
             * Return 1+last char in line or one plus the last char visible
             */
            cch = ECCchInWidth(ped, hdc, pLineStart, cLineLength,
                    ped->rcFmt.right - ped->rcFmt.left + ped->xOffset, TRUE);
            //
            // Consider DBCS in case of width >= ped->rcFmt.right
            //
            // Since ECCchInWidth and MLLineLength takes care of DBCS, we only need to
            // worry about if the last character is a double byte character or not.
            //
            // cch = ped->chLines[line] + min( ECNextIch(ped, pLineStart, cch), cLineLength);
            //
            // we need to adjust the position. LiZ -- 5/5/93
            if (ped->fAnsi && ped->fDBCS) {
                ICH cch2 = min(cch+1,cLineLength);
                if (ECAdjustIch(ped, pLineStart, cch2) != cch2) {
                    /* Displayed character on the right edge is DBCS */
                    cch = min(cch+2,cLineLength);
                } else {
                    cch = cch2;
                }
                cch += ped->chLines[line];
            } else {
                cch = ped->chLines[line] + min(cch + 1, cLineLength);
            }
        } else if (width <= ped->rcFmt.left + ped->aveCharWidth / 2) {

            /*
             * Return first char in line or one minus first char visible. Note that
             * ped->xOffset is 0 for right and centered text so we will just return
             * the first char in the string for them. (Allow a avecharwidth/2
             * positioning border so that the user can be a little off...
             */
            cch = ECCchInWidth(ped, hdc, pLineStart, cLineLength,
                    ped->xOffset, TRUE);
            if (cch)
                cch--;

            cch = ECAdjustIch( ped, pLineStart, cch );
            cch += ped->chLines[line];
        } else {

            if (cLineLength == 0) {
                cch = ped->chLines[line];
                goto edUnlock;
            }

            iCurWidth = width + ped->xOffset - ped->rcFmt.left;
            /*
             * If the user clicked past the end of the text, return the last character
             */
            lastHighWidth = MLGetLineWidth(hdc, pLineStart, cLineLength, ped);
            if (lastHighWidth <= iCurWidth) {
                cLineLengthNew = cLineLength;
                goto edAdjust;
            }
            /*
             * Now the mouse is somewhere on the visible portion of the text
             * remember cch contains the length of the line.
             */
            cLineLengthLow = 0;
            cLineLengthHigh = cLineLength + 1;
            lastLowWidth = 0;

            while (cLineLengthLow < cLineLengthHigh - 1) {

                cLineLengthNew = (cLineLengthHigh + cLineLengthLow) / 2;

                if (ped->fAnsi && ped->fDBCS) {
                    /*
                     * MLGetLineWidth returns meaningless value for truncated DBCS.
                     */
                    cLineLengthTemp = ECAdjustIch(ped, pLineStart, cLineLengthNew);
                    textWidth = MLGetLineWidth(hdc, pLineStart, cLineLengthTemp, ped);

                } else {
                    textWidth = MLGetLineWidth(hdc, pLineStart, cLineLengthNew, ped);
                }

                if (textWidth > iCurWidth) {
                    cLineLengthHigh = cLineLengthNew;
                    lastHighWidth = textWidth;
                } else {
                    cLineLengthLow = cLineLengthNew;
                    lastLowWidth = textWidth;
                }
            }

            /*
             * When the while ends, you can't know the exact desired position.
             * Try to see if the mouse pointer was on the farest half
             * of the char we got and if so, adjust cch.
             */
            if (cLineLengthLow == cLineLengthNew) {
                /*
                 * Need to compare with lastHighWidth
                 */
                if ((lastHighWidth - iCurWidth) < (iCurWidth - textWidth)) {
                    cLineLengthNew++;
                }
            } else {
                /*
                 * Need to compare with lastLowHigh
                 */
                if ((iCurWidth - lastLowWidth) < (textWidth - iCurWidth)) {
                    cLineLengthNew--;
                }
            }
edAdjust:
            cLineLength = ECAdjustIch( ped, pLineStart, cLineLengthNew );

            cch = ped->chLines[line] + cLineLength;
        }
    }
edUnlock:
    ECUnlock(ped);

    if (pline) {
        *pline = line;
    }
    return cch;
}

/***************************************************************************\
* MLChangeSelection AorW
*
* Changes the current selection to have the specified starting and
* ending values. Properly highlights the new selection and unhighlights
* anything deselected. If NewMinSel and NewMaxSel are out of order, we swap
* them. Doesn't update the caret position.
*
* History:
\***************************************************************************/

void MLChangeSelection(
    PED ped,
    HDC hdc,
    ICH ichNewMinSel,
    ICH ichNewMaxSel)
{

    ICH temp;
    ICH ichOldMinSel, ichOldMaxSel;

    if (ichNewMinSel > ichNewMaxSel) {
        temp = ichNewMinSel;
        ichNewMinSel = ichNewMaxSel;
        ichNewMaxSel = temp;
    }
    ichNewMinSel = min(ichNewMinSel, ped->cch);
    ichNewMaxSel = min(ichNewMaxSel, ped->cch);

    /*
     * Save the current selection
     */
    ichOldMinSel = ped->ichMinSel;
    ichOldMaxSel = ped->ichMaxSel;

    /*
     * Set new selection
     */
    ped->ichMinSel = ichNewMinSel;
    ped->ichMaxSel = ichNewMaxSel;

    /*
     * This finds the XOR of the old and new selection regions and redraws it.
     * There is nothing to repaint if we aren't visible or our selection
     * is hidden.
     */
    if (_IsWindowVisible(ped->pwnd) && (ped->fFocus || ped->fNoHideSel)) {

        BLOCK Blk[2];
        int i;

        if (ped->fFocus) {
            NtUserHideCaret(ped->hwnd);
        }

        Blk[0].StPos = ichOldMinSel;
        Blk[0].EndPos = ichOldMaxSel;
        Blk[1].StPos = ped->ichMinSel;
        Blk[1].EndPos = ped->ichMaxSel;

        if (ECCalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel, (LPBLOCK)&Blk[0], (LPBLOCK)&Blk[1])) {

            /*
             * Paint both Blk[0] and Blk[1], if they exist
             */
            for (i = 0; i < 2; i++) {
                if (Blk[i].StPos != 0xFFFFFFFF)
                    MLDrawText(ped, hdc, Blk[i].StPos, Blk[i].EndPos, TRUE);
            }
        }

        /*
         * Update caret.
         */
        MLSetCaretPosition(ped, hdc);

        if (ped->fFocus) {
            NtUserShowCaret(ped->hwnd);
        }

    }
}


/**************************************************************************\
* MLUpdateiCaretLine AorW
*
* This updates the ped->iCaretLine field from the ped->ichCaret;
* Also, when the caret gets to the beginning of next line, pop it up to
* the end of current line when inserting text;
*
* History
* 4-18-91 Mikehar 31Merge
\**************************************************************************/

void MLUpdateiCaretLine(PED ped)
{
    PSTR pText;

    ped->iCaretLine = MLIchToLine(ped, ped->ichCaret);

    /*
     * If caret gets to beginning of next line, pop it up to end of current line
     * when inserting text.
     */
    pText = ECLock(ped) +
            (ped->ichCaret - 1) * ped->cbChar;
    if (ped->iCaretLine && ped->chLines[ped->iCaretLine] == ped->ichCaret &&
            (!AWCOMPARECHAR(ped, pText - ped->cbChar, 0x0D) ||
            !AWCOMPARECHAR(ped, pText, 0x0A)))
        ped->iCaretLine--;
    ECUnlock(ped);
}

/***************************************************************************\
* MLInsertText AorW
*
* Adds up to cchInsert characters from lpText to the ped starting at
* ichCaret. If the ped only allows a maximum number of characters, then we
* will only add that many characters to the ped. The number of characters
* actually added is return ed (could be 0). If we can't allocate the required
* space, we notify the parent with EN_ERRSPACE and no characters are added.
* We will rebuild the lines array as needed. fUserTyping is true if the
* input was the result of the user typing at the keyboard. This is so we can
* do some stuff faster since we will be getting only one or two chars of
* input.
*
* History:
* Created ???
* 4-18-91 Mikehar Win31 Merge
\***************************************************************************/

ICH MLInsertText(
    PED ped,
    LPSTR lpText,
    ICH cchInsert,
    BOOL fUserTyping)
{
    HDC hdc;
    ICH validCch = cchInsert;
    ICH oldCaret = ped->ichCaret;
    int oldCaretLine = ped->iCaretLine;
    BOOL fCRLF = FALSE;
    LONG ll, hl;
    POINT xyPosInitial;
    POINT xyPosFinal;
    HWND hwndSave = ped->hwnd;
    UNDO undo;
    ICH validCchTemp;

    xyPosInitial.x=0;
    xyPosInitial.y=0;
    xyPosFinal.x=0;
    xyPosFinal.y=0;

    if (validCch == 0)
        return 0;

    if (ped->cchTextMax <= ped->cch) {

        /*
         * When the max chars is reached already, notify parent
         * Fix for Bug #4183 -- 02/06/91 -- SANKAR --
         */
        ECNotifyParent(ped,EN_MAXTEXT);
        return 0;
    }

    /*
     * Limit the amount of text we add
     */
    validCch = min(validCch, ped->cchTextMax - ped->cch);

    /*
     * Make sure we don't split a CRLF in half
     */
    if (validCch) {
        if (ped->fAnsi) {
            if (*(WORD UNALIGNED *)(lpText + validCch - 1) == 0x0A0D)
                validCch--;
        } else {
            if (*(DWORD UNALIGNED *)(lpText + (validCch - 1) * ped->cbChar) == 0x000A000D)
                validCch--;
        }
    }
    if (!validCch) {
        /*
         * When the max chars is reached already, notify parent
         * Fix for Bug #4183 -- 02/06/91 -- SANKAR --
         */
        ECNotifyParent(ped,EN_MAXTEXT);
        return 0;
    }

    if (validCch == 2) {
        if (ped->fAnsi) {
            if (*(WORD UNALIGNED *)lpText == 0x0A0D)
                fCRLF = TRUE;
        } else {
            if (*(DWORD UNALIGNED *)lpText == 0x000A000D)
                fCRLF = TRUE;
        }
    }

    //
    // Save current undo state always, but clear it out only if !AutoVScroll
    //
    ECSaveUndo(Pundo(ped), (PUNDO)&undo, !ped->fAutoVScroll);

    hdc = ECGetEditDC(ped, FALSE);
    /*
     * We only need the y position. Since with an LPK loaded
     * calculating the x position is an intensive job, just
     * call MLIchToYPos.
     */
    if (ped->cch)
        if (ped->pLpkEditCallout)
            xyPosInitial.y = MLIchToYPos(ped, ped->cch-1, FALSE);
        else
            MLIchToXYPos(ped, hdc, ped->cch - 1, FALSE, &xyPosInitial);

    /*
     * Insert the text
     */
    validCchTemp = validCch;    // may not be needed, but just for precautions..
    if (!ECInsertText(ped, lpText, &validCchTemp)) {

        // Restore previous undo buffer if it was cleared
        if (!ped->fAutoVScroll)
            ECSaveUndo((PUNDO)&undo, Pundo(ped), FALSE);

        ECReleaseEditDC(ped, hdc, FALSE);
        ECNotifyParent(ped, EN_ERRSPACE);
        return (0);
    }

#if DBG
    if (validCch != validCchTemp) {
        /*
         * All characters in lpText has not been inserted to ped.
         * This could happen when cch is close to cchMax.
         * Better revisit this after NT5 ships.
         */
        RIPMSG2(RIP_WARNING, "MLInsertText: validCch is changed (%x -> %x) in ECInsertText.",
            validCch, validCchTemp);
    }
#endif

    /*
     * Note that ped->ichCaret is updated by ECInsertText
     */
    MLBuildchLines(ped, (ICH)oldCaretLine, (int)validCch, fCRLF?(BOOL)FALSE:fUserTyping, &ll, &hl);

    if (ped->cch)
       /*
        * We only need the y position. Since with an LPK loaded
        * calculating the x position is an intensive job, just
        * call MLIchToYPos.
        */
       if (ped->pLpkEditCallout)
           xyPosFinal.y = MLIchToYPos(ped, ped->cch-1, FALSE);
       else
           MLIchToXYPos(ped, hdc, ped->cch - 1, FALSE,&xyPosFinal);

    if (xyPosFinal.y < xyPosInitial.y && ((ICH)ped->ichScreenStart) + ped->ichLinesOnScreen >= ped->cLines - 1) {
        RECT rc;

        CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);
        rc.top = xyPosFinal.y + ped->lineHeight;
        if (ped->pLpkEditCallout) {
            int xFarOffset = ped->xOffset + ped->rcFmt.right - ped->rcFmt.left;
            // Include left or right margins in display unless clipped
            // by horizontal scrolling.
            if (ped->wLeftMargin) {
                if (!(   ped->format == ES_LEFT     // Only ES_LEFT (Nearside alignment) can get clipped
                      && (   (!ped->fRtoLReading && ped->xOffset > 0)  // LTR and first char not fully in view
                          || ( ped->fRtoLReading && xFarOffset < ped->maxPixelWidth)))) { //RTL and last char not fully in view
                    rc.left  -= ped->wLeftMargin;
                }
            }

            // Process right margin
            if (ped->wRightMargin) {
                if (!(   ped->format == ES_LEFT     // Only ES_LEFT (Nearside alignment) can get clipped
                      && (   ( ped->fRtoLReading && ped->xOffset > 0)  // RTL and first char not fully in view
                          || (!ped->fRtoLReading && xFarOffset < ped->maxPixelWidth)))) { // LTR and last char not fully in view
                    rc.right += ped->wRightMargin;
                }
            }
        }
        NtUserInvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
    }

    if (!ped->fAutoVScroll) {
        if (ped->ichLinesOnScreen < ped->cLines) {
            MLUndo(ped);
            ECEmptyUndo(Pundo(ped));

            ECSaveUndo(&undo, Pundo(ped), FALSE);

            NtUserMessageBeep(0);
            ECReleaseEditDC(ped, hdc, FALSE);

            /*
             * When the max lines is reached already, notify parent
             * Fix for Bug #7586 -- 10/14/91 -- SANKAR --
             */
            ECNotifyParent(ped,EN_MAXTEXT);
            return (0);
        } else {
            ECEmptyUndo(&undo);
        }
    }

    if (fUserTyping && ped->fWrap) {
        //
        // To avoid oldCaret points intermediate of DBCS character,
        // adjust oldCaret position if necessary.
        //
        // !!!CR If MLBuildchLines() returns reasonable value ( and I think
        //       it does), we don't probably need this. Check this out later.
        //
        if (ped->fDBCS && ped->fAnsi) {
            oldCaret = ECAdjustIch(ped,
                                   ECLock(ped),
                                   min((ICH)LOWORD(ll),oldCaret));
            /* ECUnlock(ped); */
        } else { // same as original code
            oldCaret = min((ICH)LOWORD(ll), oldCaret);
        }
    }

    // Update ped->iCaretLine properly.
    MLUpdateiCaretLine(ped);

    ECNotifyParent(ped, EN_UPDATE);

    /*
     * Make sure window still exists.
     */
    if (!IsWindow(hwndSave))
        return 0;

    if (_IsWindowVisible(ped->pwnd)) {

        //
        // If the current font has negative A widths, we may have to start
        // drawing a few characters before the oldCaret position.
        //
        if (ped->wMaxNegAcharPos) {
            int iLine = MLIchToLine(ped, oldCaret);
            oldCaret = max( ((int)(oldCaret - ped->wMaxNegAcharPos)),
                          ((int)(ped->chLines[iLine])));
        }

        // Redraw to end of screen/text if CRLF or large insert
        if (fCRLF || !fUserTyping) {

            /*
             * Redraw to end of screen/text if crlf or large insert.
             */
            MLDrawText(ped, hdc, (fUserTyping ? oldCaret : 0), ped->cch, FALSE);
        } else
            MLDrawText(ped, hdc, oldCaret, max(ped->ichCaret, (ICH)hl), FALSE);
    }

    ECReleaseEditDC(ped, hdc, FALSE);

    /*
     * Make sure we can see the cursor
     */
    MLEnsureCaretVisible(ped);

    ped->fDirty = TRUE;

    ECNotifyParent(ped, EN_CHANGE);

    if (validCch < cchInsert)
        ECNotifyParent(ped, EN_MAXTEXT);

    if (validCch && FWINABLE()) {
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
    }

    /*
     * Make sure the window still exists.
     */
    if (!IsWindow(hwndSave))
        return 0;
    else
        return validCch;
}

/***************************************************************************\
*
*  MLReplaceSel() -
*
*  Replaces currently selected text with the passed in text, WITH UNDO
*  CAPABILITIES.
*
\***************************************************************************/
void   MLReplaceSel(PED ped, LPSTR lpText)
{
    ICH  cchText;

    //
    // Delete text, which will put it into the clean undo buffer.
    //
    ECEmptyUndo(Pundo(ped));
    MLDeleteText(ped);

    //
    // B#3356
    // Some apps do "clear" by selecting all of the text, then replacing
    // it with "", in which case MLInsertText() will return 0.  But that
    // doesn't mean failure...
    //
    if ( ped->fAnsi )
        cchText = strlen(lpText);
    else
        cchText = wcslen((LPWSTR)lpText);

    if (cchText ) {
        BOOL fFailed;
        UNDO undo;
        HWND hwndSave;

        //
        // B#1385,1427
        // Save undo buffer, but DO NOT CLEAR IT.  We want to restore it
        // if insertion fails due to OOM.
        //
        ECSaveUndo(Pundo(ped), (PUNDO)&undo, FALSE);

        hwndSave = ped->hwnd;
        fFailed = (BOOL) !MLInsertText(ped, lpText, cchText, FALSE);
        if (!IsWindow(hwndSave))
            return;

        if (fFailed) {
            //
            // UNDO the previous edit
            //
            ECSaveUndo((PUNDO)&undo, Pundo(ped), FALSE);
            MLUndo(ped);
        }
    }
}


/***************************************************************************\
* MLDeleteText AorW
*
* Deletes the characters between ichMin and ichMax. Returns the
* number of characters we deleted.
*
* History:
\***************************************************************************/

ICH MLDeleteText(
    PED ped)
{
    ICH minSel = ped->ichMinSel;
    ICH maxSel = ped->ichMaxSel;
    ICH cchDelete;
    HDC hdc;
    int minSelLine;
    int maxSelLine;
    POINT xyPos;
    RECT rc;
    BOOL fFastDelete = FALSE;
    LONG hl;
    INT  cchcount = 0;

    /*
     * Get what line the min selection is on so that we can start rebuilding the
     * text from there if we delete anything.
     */
    minSelLine = MLIchToLine(ped, minSel);
    maxSelLine = MLIchToLine(ped, maxSel);
    //
    // Calculate fFastDelete and cchcount
    //
    if (ped->fAnsi && ped->fDBCS) {
        if ((ped->fAutoVScroll) &&
            (minSelLine == maxSelLine) &&
            (ped->chLines[minSelLine] != minSel)  &&
            (ECNextIch(ped,NULL,minSel) == maxSel)) {

                fFastDelete = TRUE;
                cchcount = ((maxSel - minSel) == 1) ? 0 : -1;
        }
    } else if (((maxSel - minSel) == 1) && (minSelLine == maxSelLine) && (ped->chLines[minSelLine] != minSel)) {
            if (!ped->fAutoVScroll)
                fFastDelete = FALSE;
            else
                fFastDelete = TRUE;
    }
    if (!(cchDelete = ECDeleteText(ped)))
        return (0);

    /*
     * Start building lines at minsel line since caretline may be at the max sel
     * point.
     */
    if (fFastDelete) {
        //
        // cchcount is (-1) if it's a double byte character
        //
        MLShiftchLines(ped, minSelLine + 1, -2 + cchcount);
        MLBuildchLines(ped, minSelLine, 1, TRUE, NULL, &hl);
    } else {
        MLBuildchLines(ped, max(minSelLine-1,0), -(int)cchDelete, FALSE, NULL, NULL);
    }

    MLUpdateiCaretLine(ped);

    ECNotifyParent(ped, EN_UPDATE);

    if (_IsWindowVisible(ped->pwnd)) {

        /*
         * Now update the screen to reflect the deletion
         */
        hdc = ECGetEditDC(ped, FALSE);

        /*
         * Otherwise just redraw starting at the line we just entered
         */
        minSelLine = max(minSelLine-1,0);
        MLDrawText(ped, hdc, ped->chLines[minSelLine],
                   fFastDelete ? hl : ped->cch, FALSE);

        CopyRect(&rc, &ped->rcFmt);
        rc.left  -= ped->wLeftMargin;
        rc.right += ped->wRightMargin;

        if (ped->cch) {

            /*
             * Clear from end of text to end of window.
             *
             * We only need the y position. Since with an LPK loaded
             * calculating the x position is an intensive job, just
             * call MLIchToYPos.
             */
            if (ped->pLpkEditCallout)
                xyPos.y = MLIchToYPos(ped, ped->cch, FALSE);
            else
                MLIchToXYPos(ped, hdc, ped->cch, FALSE, &xyPos);
            rc.top = xyPos.y + ped->lineHeight;
        }

        NtUserInvalidateRect(ped->hwnd, &rc, TRUE);
        ECReleaseEditDC(ped, hdc, FALSE);

        MLEnsureCaretVisible(ped);
    }

    ped->fDirty = TRUE;

    ECNotifyParent(ped, EN_CHANGE);

    if (cchDelete && FWINABLE())
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);

    return cchDelete;
}

/***************************************************************************\
* MLInsertchLine AorW
*
* Inserts the line iline and sets its starting character index to be
* ich. All the other line indices are moved up. Returns TRUE if successful
* else FALSE and notifies the parent that there was no memory.
*
* History:
\***************************************************************************/

BOOL MLInsertchLine(
    PED ped,
    ICH iLine,
    ICH ich,
    BOOL fUserTyping)
{
    DWORD dwSize;

    if (fUserTyping && iLine < ped->cLines) {
        ped->chLines[iLine] = ich;
        return (TRUE);
    }

    dwSize = (ped->cLines + 2) * sizeof(int);

    if (dwSize > UserLocalSize(ped->chLines)) {
        LPICH hResult;
        /*
         * Grow the line index buffer
         */
        dwSize += LINEBUMP * sizeof(int);
        hResult = (LPICH)UserLocalReAlloc(ped->chLines, dwSize, 0);

        if (!hResult) {
            ECNotifyParent(ped, EN_ERRSPACE);
            return FALSE;
        }
        ped->chLines = hResult;
    }

    /*
     * Move indices starting at iLine up
     */
    if (ped->cLines != iLine)
        RtlMoveMemory(&ped->chLines[iLine + 1], &ped->chLines[iLine],
                (ped->cLines - iLine) * sizeof(int));
    ped->cLines++;

    ped->chLines[iLine] = ich;
    return TRUE;
}

/***************************************************************************\
* MLShiftchLines AorW
*
* Move the starting index of all lines iLine or greater by delta
* bytes.
*
* History:
\***************************************************************************/

void MLShiftchLines(
    PED ped,
    ICH iLine,
    int delta)
{
    if (iLine >= ped->cLines)
        return;

    /*
     * Just add delta to the starting point of each line after iLine
     */
    for (; iLine < ped->cLines; iLine++)
        ped->chLines[iLine] += delta;
}

/***************************************************************************\
* MLBuildchLines AorW
*
* Rebuilds the start of line array (ped->chLines) starting at line
* number ichLine.
*
* History:
\***************************************************************************/

void MLBuildchLines(
    PED ped,
    ICH iLine,
    int cchDelta, // Number of chars added or deleted
    BOOL fUserTyping,
    PLONG pll,
    PLONG phl)
{
    PSTR ptext; /* Starting address of the text */

    /*
     * We keep these ICH's so that we can Unlock ped->hText when we have to grow
     * the chlines array. With large text handles, it becomes a problem if we
     * have a locked block in the way.
     */
    ICH ichLineStart;
    ICH ichLineEnd;
    ICH ichLineEndBeforeCRLF;
    ICH ichCRLF;

    ICH cch;
    HDC hdc;

    BOOL fLineBroken = FALSE; /* Initially, no new line breaks are made */
    ICH minCchBreak;
    ICH maxCchBreak;
    BOOL fOnDelimiter;

    if (!ped->cch) {
        ped->maxPixelWidth = 0;
        ped->xOffset = 0;
        ped->ichScreenStart = 0;
        ped->cLines = 1;

        if (pll)
            *pll = 0;
        if (phl)
            *phl = 0;

        goto UpdateScroll;
    }

    if (fUserTyping && cchDelta)
        MLShiftchLines(ped, iLine + 1, cchDelta);

    hdc = ECGetEditDC(ped, TRUE);

    if (!iLine && !cchDelta && !fUserTyping) {

        /*
         * Reset maxpixelwidth only if we will be running through the whole
         * text. Better too long than too short.
         */
        ped->maxPixelWidth = 0;

        /*
         * Reset number of lines in text since we will be running through all
         * the text anyway...
         */
        ped->cLines = 1;
    }

    /*
     * Set min and max line built to be the starting line
     */
    minCchBreak = maxCchBreak = (cchDelta ? ped->chLines[iLine] : 0);

    ptext = ECLock(ped);

    ichCRLF = ichLineStart = ped->chLines[iLine];

    while (ichLineStart < ped->cch) {
        if (ichLineStart >= ichCRLF) {
            ichCRLF = ichLineStart;

            /*
             * Move ichCRLF ahead to either the first CR or to the end of text.
             */
            if (ped->fAnsi) {
                while (ichCRLF < ped->cch) {
                    if (*(ptext + ichCRLF) == 0x0D) {
                        if (*(ptext + ichCRLF + 1) == 0x0A ||
                                *(WORD UNALIGNED *)(ptext + ichCRLF + 1) == 0x0A0D)
                            break;
                    }
                    ichCRLF++;
                }
            } else {
                LPWSTR pwtext = (LPWSTR)ptext;

                while (ichCRLF < ped->cch) {
                    if (*(pwtext + ichCRLF) == 0x0D) {
                        if (*(pwtext + ichCRLF + 1) == 0x0A ||
                                *(DWORD UNALIGNED *)(pwtext + ichCRLF + 1) == 0x000A000D)
                            break;
                    }
                    ichCRLF++;
                }
            }
        }


        if (!ped->fWrap) {

            UINT  LineWidth;
            /*
             * If we are not word wrapping, line breaks are signified by CRLF.
             */

            //
            // If we cut off the line at MAXLINELENGTH, we should
            // adjust ichLineEnd.
            //
            if ((ichCRLF - ichLineStart) <= MAXLINELENGTH) {
                ichLineEnd = ichCRLF;
            } else {
                ichLineEnd = ichLineStart + MAXLINELENGTH;
                if (ped->fAnsi && ped->fDBCS) {
                    ichLineEnd = ECAdjustIch( ped, (PSTR)ptext, ichLineEnd);
                }
            }

            /*
             * We will keep track of what the longest line is for the horizontal
             * scroll bar thumb positioning.
             */
            if (ped->pLpkEditCallout) {
                LineWidth = ped->pLpkEditCallout->EditGetLineWidth(
                    ped, hdc, ptext + ichLineStart*ped->cbChar,
                    ichLineEnd - ichLineStart);
            } else {
                LineWidth = MLGetLineWidth(hdc, ptext + ichLineStart * ped->cbChar,
                                            ichLineEnd - ichLineStart,
                                            ped);
            }
            ped->maxPixelWidth = max(ped->maxPixelWidth,(int)LineWidth);

        } else {

            /*
             * Check if the width of the edit control is non-zero;
             * a part of the fix for Bug #7402 -- SANKAR -- 01/21/91 --
             */
            if(ped->rcFmt.right > ped->rcFmt.left) {

                /*
                 * Find the end of the line based solely on text extents
                 */
                if (ped->pLpkEditCallout) {
                    ichLineEnd = ichLineStart +
                        ped->pLpkEditCallout->EditCchInWidth(
                            ped, hdc, ptext + ped->cbChar*ichLineStart,
                            ichCRLF - ichLineStart,
                            ped->rcFmt.right - ped->rcFmt.left);
                } else {
                    if (ped->fAnsi) {
                        ichLineEnd = ichLineStart +
                                 ECCchInWidth(ped, hdc,
                                              ptext + ichLineStart,
                                              ichCRLF - ichLineStart,
                                              ped->rcFmt.right - ped->rcFmt.left,
                                              TRUE);
                    } else {
                        ichLineEnd = ichLineStart +
                                 ECCchInWidth(ped, hdc,
                                              (LPSTR)((LPWSTR)ptext + ichLineStart),
                                              ichCRLF - ichLineStart,
                                              ped->rcFmt.right - ped->rcFmt.left,
                                              TRUE);
                    }
                }
            } else {
                ichLineEnd = ichLineStart;
            }

            if (ichLineEnd == ichLineStart && ichCRLF - ichLineStart) {

                /*
                 * Maintain a minimum of one char per line
                 */
                //
                // Since it might be a double byte char, so calling ECNextIch.
                //
                ichLineEnd = ECNextIch(ped, NULL, ichLineEnd);
            }

            /*
             * Now starting from ichLineEnd, if we are not at a hard line break,
             * then if we are not at a space AND the char before us is
             * not a space,(OR if we are at a CR) we will look word left for the
             * start of the word to break at.
             * This change was done for TWO reasons:
             * 1. If we are on a delimiter, no need to look word left to break at.
             * 2. If the previous char is a delimter, we can break at current char.
             * Change done by -- SANKAR --01/31/91--
             */
            if (ichLineEnd != ichCRLF) {
                if(ped->lpfnNextWord) {
                     fOnDelimiter = (CALLWORDBREAKPROC(*ped->lpfnNextWord, ptext,
                            ichLineEnd, ped->cch, WB_ISDELIMITER) ||
                            CALLWORDBREAKPROC(*ped->lpfnNextWord, ptext, ichLineEnd - 1,
                            ped->cch, WB_ISDELIMITER));
                //
                // This change was done for FOUR reasons:
                //
                // 1. If we are on a delimiter, no need to look word left to break at.
                // 2. If we are on a double byte character, we can break at current char.
                // 3. If the previous char is a delimter, we can break at current char.
                // 4. If the previous char is a double byte character, we can break at current char.
                //
                } else if (ped->fAnsi) {
                    fOnDelimiter = (ISDELIMETERA(*(ptext + ichLineEnd)) ||
                                    ECIsDBCSLeadByte(ped, *(ptext + ichLineEnd)));
                    if (!fOnDelimiter) {
                        PSTR pPrev = ECAnsiPrev(ped,ptext,ptext+ichLineEnd);

                        fOnDelimiter = ISDELIMETERA(*pPrev) ||
                                       ECIsDBCSLeadByte(ped,*pPrev);
                    }
                } else { // Unicode
                    fOnDelimiter = (ISDELIMETERW(*((LPWSTR)ptext + ichLineEnd))     ||
                                    UserIsFullWidth(CP_ACP,*((LPWSTR)ptext + ichLineEnd))      ||
                                    ISDELIMETERW(*((LPWSTR)ptext + ichLineEnd - 1)) ||
                                    UserIsFullWidth(CP_ACP,*((LPWSTR)ptext + ichLineEnd - 1)));
                }
                if (!fOnDelimiter ||
                    (ped->fAnsi && *(ptext + ichLineEnd) == 0x0D) ||
                    (!ped->fAnsi && *((LPWSTR)ptext + ichLineEnd) == 0x0D)) {

                    if (ped->lpfnNextWord != NULL) {
                        cch = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)ptext, ichLineEnd,
                                ped->cch, WB_LEFT);
                    } else {
                        ped->fCalcLines = TRUE;
                        ECWord(ped, ichLineEnd, TRUE, &cch, NULL);
                        ped->fCalcLines = FALSE;
                    }
                    if (cch > ichLineStart) {
                        ichLineEnd = cch;
                    }

                    /*
                     * Now, if the above test fails, it means the word left goes
                     * back before the start of the line ie. a word is longer
                     * than a line on the screen. So, we just fit as much of
                     * the word on the line as possible. Thus, we use the
                     * pLineEnd we calculated solely on width at the beginning
                     * of this else block...
                     */
                }
            }
        }
#if 0
        if (!ISDELIMETERAW((*(ptext + (ichLineEnd - 1)*ped->cbChar))) && ISDELIMETERAW((*(ptext + ichLineEnd*ped->cbChar)))) #ERROR

            if ((*(ptext + ichLineEnd - 1) != ' ' &&
                        *(ptext + ichLineEnd - 1) != VK_TAB) &&
                        (*(ptext + ichLineEnd) == ' ' ||
                        *(ptext + ichLineEnd) == VK_TAB))
#endif
        if (AWCOMPARECHAR(ped,ptext + ichLineEnd * ped->cbChar, ' ') ||
                AWCOMPARECHAR(ped,ptext + ichLineEnd * ped->cbChar, VK_TAB)) {
            /*
             * Swallow the space at the end of a line.
             */
            if (ichLineEnd < ped->cch) {
                ichLineEnd++;
            }
        }

        /*
         * Skip over crlf or crcrlf if it exists. Thus, ichLineEnd is the first
         * character in the next line.
         */
        ichLineEndBeforeCRLF = ichLineEnd;

        if (ped->fAnsi) {
            if (ichLineEnd < ped->cch && *(ptext + ichLineEnd) == 0x0D)
                ichLineEnd += 2;

            /*
             * Skip over CRCRLF
             */
            if (ichLineEnd < ped->cch && *(ptext + ichLineEnd) == 0x0A)
                ichLineEnd++;
            UserAssert(ichLineEnd <= ped->cch);
        } else {
            if (ichLineEnd < ped->cch && *(((LPWSTR)ptext) + ichLineEnd) == 0x0D)
                ichLineEnd += 2;

            /*
             * Skip over CRCRLF
             */
            if (ichLineEnd < ped->cch && *(((LPWSTR)ptext) + ichLineEnd) == 0x0A) {
                ichLineEnd++;
                RIPMSG0(RIP_VERBOSE, "Skip over CRCRLF\n");
            }
            UserAssert(ichLineEnd <= ped->cch);
        }

        /*
         * Now, increment iLine, allocate space for the next line, and set its
         * starting point
         */
        iLine++;

        if (!fUserTyping || (iLine > ped->cLines - 1) || (ped->chLines[iLine] != ichLineEnd)) {

            /*
             * The line break occured in a different place than before.
             */
            if (!fLineBroken) {

                /*
                 * Since we haven't broken a line before, just set the min
                 * break line.
                 */
                fLineBroken = TRUE;
                if (ichLineEndBeforeCRLF == ichLineEnd)
                    minCchBreak = maxCchBreak = (ichLineEnd ? ichLineEnd - 1 : 0);
                else
                    minCchBreak = maxCchBreak = ichLineEndBeforeCRLF;
            }
            maxCchBreak = max(maxCchBreak, ichLineEnd);

            ECUnlock(ped);

            /*
             * Now insert the new line into the array
             */
            if (!MLInsertchLine(ped, iLine, ichLineEnd, (BOOL)(cchDelta != 0)))
                goto EndUp;

            ptext = ECLock(ped);
        } else {
            maxCchBreak = ped->chLines[iLine];

            /*
             * Quick escape
             */
            goto UnlockAndEndUp;
        }

        ichLineStart = ichLineEnd;
    } /* end while (ichLineStart < ped->cch) */


    if (iLine != ped->cLines) {
        RIPMSG1(RIP_VERBOSE, "chLines[%d] is being cleared.\n", iLine);
        ped->cLines = iLine;
        ped->chLines[ped->cLines] = 0;
    }

    /*
     * Note that we incremented iLine towards the end of the while loop so, the
     * index, iLine, is actually equal to the line count
     */
    if (ped->cch && AWCOMPARECHAR(ped, ptext + (ped->cch - 1)*ped->cbChar, 0x0A) &&
            ped->chLines[ped->cLines - 1] < ped->cch) {

        /*
         * Make sure last line has no crlf in it
         */
        if (!fLineBroken) {

            /*
             * Since we haven't broken a line before, just set the min break
             * line.
             */
            fLineBroken = TRUE;
            minCchBreak = ped->cch - 1;
        }
        maxCchBreak = max(maxCchBreak, ichLineEnd);
        ECUnlock(ped);
        MLInsertchLine(ped, iLine, ped->cch, FALSE);
        MLSanityCheck(ped);
    } else
UnlockAndEndUp:
        ECUnlock(ped);

EndUp:
    ECReleaseEditDC(ped, hdc, TRUE);
    if (pll)
        *pll = minCchBreak;
    if (phl)
        *phl = maxCchBreak;

UpdateScroll:
    MLScroll(ped, FALSE, ML_REFRESH, 0, TRUE);
    MLScroll(ped, TRUE,  ML_REFRESH, 0, TRUE);

    MLSanityCheck(ped);

    return;
}

/***************************************************************************\
*
*  MLPaint()
*
*  Response to WM_PAINT message.
*
\***************************************************************************/
void   MLPaint(PED ped, HDC hdc, LPRECT lprc)
{
    HFONT       hOldFont;
    ICH         imin;
    ICH         imax;

    //
    // Do we need to draw the border ourself for old apps?
    //
    if (ped->fFlatBorder)
    {
        RECT    rcT;

        _GetClientRect(ped->pwnd, &rcT);
        if (TestWF(ped->pwnd, WFSIZEBOX))
        {
            InflateRect(&rcT, SYSMET(CXBORDER) - SYSMET(CXFRAME),
                SYSMET(CYBORDER) - SYSMET(CYFRAME));
        }
        DrawFrame(hdc, &rcT, 1, DF_WINDOWFRAME);
    }

    ECSetEditClip(ped, hdc, (BOOL) (ped->xOffset == 0));

    if (ped->hFont)
        hOldFont = SelectObject(hdc, ped->hFont);

    if (!lprc) {
        // no partial rect given -- draw all text
        imin = 0;
        imax = ped->cch;
    } else {
        // only draw pertinent text
        imin = (ICH) MLMouseToIch(ped, hdc, ((LPPOINT) &lprc->left), NULL) - 1;
        if (imin == -1)
            imin = 0;

        // HACK_ALERT:
        // The 3 is required here because, MLMouseToIch() returns decremented
        // value; We must fix MLMouseToIch.
        imax = (ICH) MLMouseToIch(ped, hdc, ((LPPOINT) &lprc->right), NULL) + 3;
        if (imax > ped->cch)
            imax = ped->cch;
    }

    MLDrawText(ped, hdc, imin, imax, FALSE);

    if (ped->hFont)
        SelectObject(hdc, hOldFont);
}

/***************************************************************************\
* MLKeyDown AorW
*
* Handles cursor movement and other VIRT KEY stuff. keyMods allows
* us to make MLKeyDownHandler calls and specify if the modifier keys (shift
* and control) are up or down. If keyMods == 0, we get the keyboard state
* using GetKeyState(VK_SHIFT) etc. Otherwise, the bits in keyMods define the
* state of the shift and control keys.
*
* History:
\***************************************************************************/

void MLKeyDown(
    PED ped,
    UINT virtKeyCode,
    int keyMods)
{
    HDC hdc;
    BOOL prevLine;
    POINT mousePt;
    int defaultDlgId;
    int iScrollAmt;

    /*
     * Variables we will use for redrawing the updated text
     */

    /*
     * new selection is specified by newMinSel, newMaxSel
     */
    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;

    /*
     * Flags for drawing the updated text
     */
    BOOL changeSelection = FALSE;

    /*
     * Comparisons we do often
     */
    BOOL MinEqMax = (newMaxSel == newMinSel);
    BOOL MinEqCar = (ped->ichCaret == newMinSel);
    BOOL MaxEqCar = (ped->ichCaret == newMaxSel);

    /*
     * State of shift and control keys.
     */
    int scState;

    if (ped->fMouseDown) {

        /*
         * If we are in the middle of a mousedown command, don't do anything.
         */
        return ;
    }

    scState = ECGetModKeys(keyMods);

    switch (virtKeyCode) {
    case VK_ESCAPE:
        if (ped->fInDialogBox) {

            /*
             * This condition is removed because, if the dialogbox does not
             * have a CANCEL button and if ESC is hit when focus is on a
             * ML edit control the dialogbox must close whether it has cancel
             * button or not to be consistent with SL edit control;
             * DefDlgProc takes care of the disabled CANCEL button case.
             * Fix for Bug #4123 -- 02/07/91 -- SANKAR --
             */
#if 0
            if (GetDlgItem(ped->hwndParent, IDCANCEL))
#endif

                /*
                 * User hit ESC...Send a close message (which in turn sends a
                 * cancelID to the app in DefDialogProc...
                 */
                PostMessage(ped->hwndParent, WM_CLOSE, 0, 0L);
        }
        return ;

    case VK_RETURN:
        if (ped->fInDialogBox) {

            /*
             * If this multiline edit control is in a dialog box, then we want
             * the RETURN key to be sent to the default dialog button (if there
             * is one). CTRL-RETURN will insert a RETURN into the text. Note
             * that CTRL-RETURN automatically translates into a linefeed (0x0A)
             * and in the MLCharHandler, we handle this as if a return was
             * entered.
             */
            if (scState != CTRLDOWN) {

                if (TestWF(ped->pwnd, EFWANTRETURN)) {

                    /*
                     * This edit control wants cr to be inserted so break out of
                     * case.
                     */
                    return ;
                }

                defaultDlgId = (int)(DWORD)LOWORD(SendMessage(ped->hwndParent,
                        DM_GETDEFID, 0, 0L));
                if (defaultDlgId) {
                    HWND hwnd = GetDlgItem(ped->hwndParent, defaultDlgId);
                    if (hwnd) {
                        SendMessage(ped->hwndParent, WM_NEXTDLGCTL, (WPARAM)hwnd, 1L);
                        if (!ped->fFocus)
                            PostMessage(hwnd, WM_KEYDOWN, VK_RETURN, 0L);
                    }
                }
            }

            return ;
        }
        break;

    case VK_TAB:

        /*
         * If this multiline edit control is in a dialog box, then we want the
         * TAB key to take you to the next control, shift TAB to take you to the
         * previous control. We always want CTRL-TAB to insert a tab into the
         * edit control regardless of weather or not we're in a dialog box.
         */
        if (scState == CTRLDOWN)
            MLChar(ped, virtKeyCode, keyMods);
        else if (ped->fInDialogBox)
            SendMessage(ped->hwndParent, WM_NEXTDLGCTL, scState == SHFTDOWN, 0L);

        return ;

    case VK_LEFT:
        //
        // If the caret isn't at the beginning, we can move left
        //
        if (ped->ichCaret) {
            // Get new caret pos.
            if (scState & CTRLDOWN) {
                // Move caret word left
                ECWord(ped, ped->ichCaret, TRUE, &ped->ichCaret, NULL);
            } else {
                if (ped->pLpkEditCallout) {
                    ped->ichCaret = MLMoveSelectionRestricted(ped, ped->ichCaret, TRUE);
                } else {
                    // Move caret char left
                    ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, TRUE);
                }
            }

            // Get new selection
            if (scState & SHFTDOWN) {
                if (MaxEqCar && !MinEqMax) {
                    // Reduce selection
                    newMaxSel = ped->ichCaret;

                    UserAssert(newMinSel == ped->ichMinSel);
                } else {
                    // Extend selection
                    newMinSel = ped->ichCaret;
                }
            } else {
                // Clear selection
                newMaxSel = newMinSel = ped->ichCaret;
            }

            changeSelection = TRUE;
        } else {
            //
            // If the user tries to move left and we are at the 0th
            // character and there is a selection, then cancel the
            // selection.
            //
            if ( (ped->ichMaxSel != ped->ichMinSel) &&
                !(scState & SHFTDOWN) ) {
                changeSelection = TRUE;
                newMaxSel = newMinSel = ped->ichCaret;
            }
        }
        break;

    case VK_RIGHT:
        //
        // If the caret isn't at the end, we can move right.
        //
        if (ped->ichCaret < ped->cch) {
            //
            // Get new caret pos.
            //
            if (scState & CTRLDOWN) {
                // Move caret word right
                ECWord(ped, ped->ichCaret, FALSE, NULL, &ped->ichCaret);
            } else {
                // Move caret char right
                if (ped->pLpkEditCallout) {
                    ped->ichCaret = MLMoveSelectionRestricted(ped, ped->ichCaret, FALSE);
                } else {
                    ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
                }
            }

            //
            // Get new selection.
            //
            if (scState & SHFTDOWN) {
                if (MinEqCar && !MinEqMax) {
                    // Reduce selection
                    newMinSel = ped->ichCaret;

                    UserAssert(newMaxSel == ped->ichMaxSel);
                } else {
                    // Extend selection
                    newMaxSel = ped->ichCaret;
                }
            } else {
                // Clear selection
                newMaxSel = newMinSel = ped->ichCaret;
            }

            changeSelection = TRUE;
        } else {
            //
            // If the user tries to move right and we are at the last
            // character and there is a selection, then cancel the
            // selection.
            //
            if ( (ped->ichMaxSel != ped->ichMinSel) &&
                !(scState & SHFTDOWN) ) {
                newMaxSel = newMinSel = ped->ichCaret;
                changeSelection = TRUE;
            }
        }
        break;

    case VK_UP:
    case VK_DOWN:
        if (ped->cLines - 1 != ped->iCaretLine &&
                ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
            prevLine = TRUE;
        else
            prevLine = FALSE;

        hdc = ECGetEditDC(ped, TRUE);
        MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine, &mousePt);
        ECReleaseEditDC(ped, hdc, TRUE);
        mousePt.y += 1 + (virtKeyCode == VK_UP ? -ped->lineHeight : ped->lineHeight);

        if (!(scState & CTRLDOWN)) {
            //
            // Send fake mouse messages to handle this
            // If VK_SHIFT is down, extend selection & move caret up/down
            // 1 line.  Otherwise, clear selection & move caret.
            //
            MLMouseMotion(ped, WM_LBUTTONDOWN,
                            !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);
            MLMouseMotion(ped, WM_LBUTTONUP,
                            !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);
        }
        break;

    case VK_HOME:
        //
        // Update caret.
        //
        if (scState & CTRLDOWN) {
            // Move caret to beginning of text.
            ped->ichCaret = 0;
        } else {
            // Move caret to beginning of line.
            ped->ichCaret = ped->chLines[ped->iCaretLine];
        }

        //
        // Update selection.
        //
        newMinSel = ped->ichCaret;

        if (scState & SHFTDOWN) {
            if (MaxEqCar && !MinEqMax) {
                if (scState & CTRLDOWN)
                    newMaxSel = ped->ichMinSel;
                else {
                    newMinSel = ped->ichMinSel;
                    newMaxSel = ped->ichCaret;
                }
            }
        } else {
            // Clear selection
            newMaxSel = ped->ichCaret;
        }

        changeSelection = TRUE;
        break;

    case VK_END:
        //
        // Update caret.
        //
        if (scState & CTRLDOWN) {
            // Move caret to end of text.
            ped->ichCaret = ped->cch;
        } else {
            // Move caret to end of line.
            ped->ichCaret = ped->chLines[ped->iCaretLine] +
                MLLine(ped, ped->iCaretLine);
        }

        // Update selection.
        newMaxSel = ped->ichCaret;

        if (scState & SHFTDOWN) {
            if (MinEqCar && !MinEqMax) {
                // Reduce selection
                if (scState & CTRLDOWN) {
                    newMinSel = ped->ichMaxSel;
                } else {
                    newMinSel = ped->ichCaret;
                    newMaxSel = ped->ichMaxSel;
                }
            }
        } else {
            // Clear selection
            newMinSel = ped->ichCaret;
        }

        changeSelection = TRUE;
        break;

    // FE_IME // EC_INSERT_COMPOSITION_CHAR : MLKeyDown() : VK_HANJA support
    case VK_HANJA:
        if ( HanjaKeyHandler( ped ) ) {
            changeSelection = TRUE;
            newMinSel = ped->ichCaret;
            newMaxSel = ped->ichCaret + (ped->fAnsi ? 2 : 1);
        }
        break;

    case VK_PRIOR:
    case VK_NEXT:
        if (!(scState & CTRLDOWN)) {
            /*
             * Vertical scroll by one visual screen
             */
            hdc = ECGetEditDC(ped, TRUE);
            MLIchToXYPos(ped, hdc, ped->ichCaret, FALSE, &mousePt);
            ECReleaseEditDC(ped, hdc, TRUE);
            mousePt.y += 1;

            SendMessage(ped->hwnd, WM_VSCROLL, virtKeyCode == VK_PRIOR ? SB_PAGEUP : SB_PAGEDOWN, 0L);

            /*
             * Move the cursor there
             */
            MLMouseMotion(ped, WM_LBUTTONDOWN, !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);
            MLMouseMotion(ped, WM_LBUTTONUP,   !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);

        } else {
            /*
             * Horizontal scroll by one screenful minus one char
             */
            iScrollAmt = ((ped->rcFmt.right - ped->rcFmt.left) / ped->aveCharWidth) - 1;
            if (virtKeyCode == VK_PRIOR)
                iScrollAmt *= -1; /* For previous page */

            SendMessage(ped->hwnd, WM_HSCROLL, MAKELONG(EM_LINESCROLL, iScrollAmt), 0);
            break;
        }
        break;

    case VK_DELETE:
        if (ped->fReadOnly)
            break;

        switch (scState) {
        case NONEDOWN:

            /*
             * Clear selection. If no selection, delete (clear) character
             * right
             */
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) {

                /*
                 * Move cursor forwards and send a backspace message...
                 */
                if (ped->pLpkEditCallout) {
                    ped->ichMinSel = ped->ichCaret;
                    ped->ichMaxSel = MLMoveSelectionRestricted(ped, ped->ichCaret, FALSE);
                } else {
                    ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
                    ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
                }

                goto DeleteAnotherChar;
            }
            break;

        case SHFTDOWN:

            /*
             * CUT selection ie. remove and copy to clipboard, or if no
             * selection, delete (clear) character left.
             */
            if (ped->ichMinSel == ped->ichMaxSel) {
                goto DeleteAnotherChar;
            } else {
                SendMessage(ped->hwnd, WM_CUT, (UINT)0, 0L);
            }

            break;

        case CTRLDOWN:

            /*
             * Clear selection, or delete to end of line if no selection
             */
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) {
                ped->ichMaxSel = ped->ichCaret = ped->chLines[ped->iCaretLine] +
                                                 MLLine(ped, ped->iCaretLine);
            }
            break;
        }

        if (!(scState & SHFTDOWN) && (ped->ichMinSel != ped->ichMaxSel)) {

DeleteAnotherChar:
            if (GETAPPVER() >= VER40) {
                MLChar(ped, VK_BACK, 0);
            } else {
                SendMessageWorker(ped->pwnd, WM_CHAR, VK_BACK, 0, ped->fAnsi);
            }
        }

        /*
         * No need to update text or selection since BACKSPACE message does it
         * for us.
         */
        break;

    case VK_INSERT:
        if (scState == CTRLDOWN || scState == SHFTDOWN) {

            /*
             * if CTRLDOWN Copy current selection to clipboard
             */

            /*
             * if SHFTDOWN Paste clipboard
             */
            SendMessage(ped->hwnd, (UINT)(scState == CTRLDOWN ? WM_COPY : WM_PASTE), 0, 0);
        }
        break;
    }

    if (changeSelection) {
        hdc = ECGetEditDC(ped, FALSE);
        MLChangeSelection(ped, hdc, newMinSel, newMaxSel);

        /*
         * Set the caret's line
         */
        ped->iCaretLine = MLIchToLine(ped, ped->ichCaret);

        if (virtKeyCode == VK_END &&
                // Next line: Win95 Bug#11822, EditControl repaint (Sankar)
                (ped->ichCaret == ped->chLines[ped->iCaretLine]) &&
                ped->ichCaret < ped->cch &&
                ped->fWrap && ped->iCaretLine > 0) {
            LPSTR pText = ECLock(ped);

            /*
             * Handle moving to the end of a word wrapped line. This keeps the
             * cursor from falling to the start of the next line if we have word
             * wrapped and there is no CRLF.
             */
            if ( ped->fAnsi ) {
                if (*(WORD UNALIGNED *)(pText +
                        ped->chLines[ped->iCaretLine] - 2) != 0x0A0D) {
                    ped->iCaretLine--;
                }
            } else {
                if (*(DWORD UNALIGNED *)(pText +
                     (ped->chLines[ped->iCaretLine] - 2)*ped->cbChar) != 0x000A000D) {
                    ped->iCaretLine--;
                }
            }
            ECUnlock(ped);
        }

        /*
         * Since drawtext sets the caret position
         */
        MLSetCaretPosition(ped, hdc);
        ECReleaseEditDC(ped, hdc, FALSE);

        /*
         * Make sure we can see the cursor
         */
        MLEnsureCaretVisible(ped);
    }
}

/***************************************************************************\
* MLChar
*
* Handles character and virtual key input
*
* History:
\***************************************************************************/

void MLChar(
    PED ped,
    DWORD keyValue,
    int keyMods)
{
    WCHAR keyPress;
    BOOL updateText = FALSE;

    /*
     * keyValue is either:
     *    a Virtual Key (eg: VK_TAB, VK_ESCAPE, VK_BACK)
     *    a character (Unicode or "ANSI")
     */
    if (ped->fAnsi)
        keyPress = LOBYTE(keyValue);
    else
        keyPress = LOWORD(keyValue);

    if (ped->fMouseDown || keyPress == VK_ESCAPE) {

        /*
         * If we are in the middle of a mousedown command, don't do anything.
         * Also, just ignore it if we get a translated escape key which happens
         * with multiline edit controls in a dialog box.
         */
        return ;
    }

    ECInOutReconversionMode(ped, FALSE);

    {
        int scState;
        scState = ECGetModKeys(keyMods);

        if (ped->fInDialogBox && scState != CTRLDOWN) {

            /*
             * If this multiline edit control is in a dialog box, then we want the
             * TAB key to take you to the next control, shift TAB to take you to the
             * previous control, and CTRL-TAB to insert a tab into the edit control.
             * We moved the focus when we received the keydown message so we will
             * ignore the TAB key now unless the ctrl key is down. Also, we want
             * CTRL-RETURN to insert a return into the text and RETURN to be sent to
             * the default button.
             */
            if (keyPress == VK_TAB ||
                    (keyPress == VK_RETURN && !TestWF(ped->pwnd, EFWANTRETURN)))
                return ;
        }

        /*
         * Allow CTRL+C to copy from a read only edit control
         * Ignore all other keys in read only controls
         */
        if ((ped->fReadOnly) && !((keyPress == 3) && (scState == CTRLDOWN))) {
            return ;
        }
    }

    switch (keyPress) {
    case 0x0A: // linefeed
        keyPress = VK_RETURN;
        /*
         * FALL THRU
         */

    case VK_RETURN:
    case VK_TAB:
    case VK_BACK:
DeleteSelection:
        if (MLDeleteText(ped))
            updateText = TRUE;
        break;

    default:
        if (keyPress >= TEXT(' ')) {
            /*
             * If this is in [a-z],[A-Z] and we are an ES_NUMBER
             * edit field, bail.
             */
            if (ped->f40Compat && TestWF(ped->pwnd, EFNUMBER)) {
                if (!ECIsCharNumeric(ped, keyPress)) {
                    goto IllegalChar;
                }
            }

            goto DeleteSelection;
        }
        break;
    }

    /*
     * Handle key codes
     */
    switch(keyPress) {
    UINT msg;

    // Ctrl+Z == Undo
    case 26:
        msg = WM_UNDO;
        goto SendEditingMessage;
        break;

    // Ctrl+X == Cut
    case 24:
        if (ped->ichMinSel == ped->ichMaxSel)
            goto IllegalChar;
        else
        {
            msg = WM_CUT;
            goto SendEditingMessage;
        }
        break;

    // Ctrl+C == Copy
    case 3:
        msg = WM_COPY;
        goto SendEditingMessage;
        break;

    // Ctrl+V == Paste
    case 22:
        msg = WM_PASTE;
SendEditingMessage:
        SendMessage(ped->hwnd, msg, 0, 0L);
        break;

    case VK_BACK:
        //
        // Delete any selected text or delete character left if no sel
        //
        if (!updateText && ped->ichMinSel)
        {
            //
            // There was no selection to delete so we just delete
            // character left if available
            //
            ped->ichMinSel = MLMoveSelection(ped, ped->ichCaret, TRUE);
            MLDeleteText(ped);
        }
        break;

    default:
        if (keyPress == VK_RETURN)
            if (ped->fAnsi)
                keyValue = 0x0A0D;
            else
                keyValue = 0x000A000D;

        if (   keyPress >= TEXT(' ')
            || keyPress == VK_RETURN
            || keyPress == VK_TAB
            || keyPress == 0x1E     // RS - Unicode block separator
            || keyPress == 0x1F     // US - Unicode segment separator
            ) {

            NtUserCallNoParam(SFI_ZZZHIDECURSORNOCAPTURE);
            if (ped->fAnsi) {
                //
                // check if it's a leading byte of double byte character
                //
                if (ECIsDBCSLeadByte(ped,(BYTE)keyPress)) {
                    int DBCSkey;

                    if ((DBCSkey = DbcsCombine(ped->hwnd, keyPress)) != 0)
                        keyValue = DBCSkey;
                }
                MLInsertText(ped, (LPSTR)&keyValue, HIBYTE(keyValue) ? 2 : 1, TRUE);
            } else
                MLInsertText(ped, (LPSTR)&keyValue, HIWORD(keyValue) ? 2 : 1, TRUE);
        } else {
IllegalChar:
            NtUserMessageBeep(0);
        }
        break;
    }
}

/***************************************************************************\
* MLPasteText AorW
*
* Pastes a line of text from the clipboard into the edit control
* starting at ped->ichCaret. Updates ichMaxSel and ichMinSel to point to the
* end of the inserted text. Notifies the parent if space cannot be
* allocated. Returns how many characters were inserted.
*
* History:
\***************************************************************************/

ICH PASCAL NEAR MLPasteText(
    PED ped)
{
    HANDLE hData;
    LPSTR lpchClip;
    ICH cchAdded = 0;
    HCURSOR hCursorOld;

#ifdef UNDO_CLEANUP           // #ifdef Added in Chicago  - johnl
    if (!ped->fAutoVScroll) {

        /*
         * Empty the undo buffer if this edit control limits the amount of text
         * the user can add to the window rect. This is so that we can undo this
         * operation if doing in causes us to exceed the window boundaries.
         */
        ECEmptyUndo(ped);
    }
#endif

    hCursorOld = NtUserSetCursor(LoadCursor(NULL, IDC_WAIT));

    if (!OpenClipboard(ped->hwnd))
        goto PasteExitNoCloseClip;

    if (!(hData = GetClipboardData(ped->fAnsi ? CF_TEXT : CF_UNICODETEXT)) ||
            (GlobalFlags(hData) == GMEM_INVALID_HANDLE)) {
        RIPMSG1(RIP_WARNING, "MLPasteText(): couldn't get a valid handle(%x)", hData);
        goto PasteExit;
    }

    /*
     * See if any text should be deleted
     */
    MLDeleteText(ped);

    USERGLOBALLOCK(hData, lpchClip);
    if (lpchClip == NULL) {
        RIPMSG1(RIP_WARNING, "MLPasteText: USERGLOBALLOCK(%x) failed.", hData);
        goto PasteExit;
    }

    /*
     * Get the length of the addition.
     */
    if (ped->fAnsi)
        cchAdded = strlen(lpchClip);
    else
        cchAdded = wcslen((LPWSTR)lpchClip);

    /*
     * Insert the text (MLInsertText checks line length)
     */
    cchAdded = MLInsertText(ped, lpchClip, cchAdded, FALSE);

    USERGLOBALUNLOCK(hData);

PasteExit:
    NtUserCloseClipboard();

PasteExitNoCloseClip:
    NtUserSetCursor(hCursorOld);

    return (cchAdded);
}

/***************************************************************************\
* MLMouseMotion AorW
*
* History:
\***************************************************************************/

void MLMouseMotion(
    PED ped,
    UINT message,
    UINT virtKeyDown,
    LPPOINT mousePt)
{
    BOOL fChangedSel = FALSE;

    HDC hdc = ECGetEditDC(ped, TRUE);

    ICH ichMaxSel = ped->ichMaxSel;
    ICH ichMinSel = ped->ichMinSel;

    ICH mouseCch;
    ICH mouseLine;
    int i, j;
    LONG  ll, lh;

    mouseCch = MLMouseToIch(ped, hdc, mousePt, &mouseLine);

    /*
     * Save for timer
     */
    ped->ptPrevMouse = *mousePt;
    ped->prevKeys = virtKeyDown;

    switch (message) {
    case WM_LBUTTONDBLCLK:
        /*
         * if shift key is down, extend selection to word we double clicked on
         * else clear current selection and select word.
         */
        // LiZ -- 5/5/93
        if (ped->fAnsi && ped->fDBCS) {
            LPSTR pText = ECLock(ped);
            ECWord(ped,ped->ichCaret,
                   ECIsDBCSLeadByte(ped, *(pText+(ped->ichCaret)))
                        ? FALSE :
                          (ped->ichCaret == ped->chLines[ped->iCaretLine]
                              ? FALSE : TRUE), &ll, &lh);
            ECUnlock(ped);
        } else {
            ECWord(ped, mouseCch, !(mouseCch == ped->chLines[mouseLine]), &ll, &lh);
        }
        if (!(virtKeyDown & MK_SHIFT)) {
            // If shift key isn't down, move caret to mouse point and clear
            // old selection
            ichMinSel = ll;
            ichMaxSel = ped->ichCaret = lh;
        } else {
            // Shiftkey is down so we want to maintain the current selection
            // (if any) and just extend or reduce it
            if (ped->ichMinSel == ped->ichCaret) {
                ichMinSel = ped->ichCaret = ll;
                ECWord(ped, ichMaxSel, TRUE, &ll, &lh);
            } else {
                ichMaxSel = ped->ichCaret = lh;
                ECWord(ped, ichMinSel, FALSE, &ll, &lh);
            }
        }

        ped->ichStartMinSel = ll;
        ped->ichStartMaxSel = lh;

        goto InitDragSelect;

    case WM_MOUSEMOVE:
        if (ped->fMouseDown) {

            /*
             * Set the system timer to automatically scroll when mouse is
             * outside of the client rectangle. Speed of scroll depends on
             * distance from window.
             */
            i = mousePt->y < 0 ? -mousePt->y : mousePt->y - ped->rcFmt.bottom;
            j = gpsi->dtScroll - ((UINT)i << 4);
            if (j < 1)
                j = 1;
            NtUserSetSystemTimer(ped->hwnd, IDSYS_SCROLL, (UINT)j, NULL);

            fChangedSel = TRUE;

            // Extend selection, move caret right
            if (ped->ichStartMinSel || ped->ichStartMaxSel) {
                // We're in WORD SELECT mode
                BOOL fReverse = (mouseCch <= ped->ichStartMinSel);
                ECWord(ped, mouseCch, !fReverse, &ll, &lh);
                if (fReverse) {
                    ichMinSel = ped->ichCaret = ll;
                    ichMaxSel = ped->ichStartMaxSel;
                } else {
                    ichMinSel = ped->ichStartMinSel;
                    ichMaxSel = ped->ichCaret = lh;
                }
            } else if ((ped->ichMinSel == ped->ichCaret) &&
                    (ped->ichMinSel != ped->ichMaxSel))
                // Reduce selection extent
                ichMinSel = ped->ichCaret = mouseCch;
            else
                // Extend selection extent
                ichMaxSel = ped->ichCaret = mouseCch;

            ped->iCaretLine = mouseLine;
        }
        break;

    case WM_LBUTTONDOWN:
        ll = lh = mouseCch;

        if (!(virtKeyDown & MK_SHIFT)) {
            // If shift key isn't down, move caret to mouse point and clear
            // old selection
            ichMinSel = ichMaxSel = ped->ichCaret = mouseCch;
        } else {
            // Shiftkey is down so we want to maintain the current selection
            // (if any) and just extend or reduce it
            if (ped->ichMinSel == ped->ichCaret)
                ichMinSel = ped->ichCaret = mouseCch;
            else
                ichMaxSel = ped->ichCaret = mouseCch;
        }

        ped->ichStartMinSel = ped->ichStartMaxSel = 0;

InitDragSelect:
        ped->iCaretLine = mouseLine;

        ped->fMouseDown = FALSE;
        NtUserSetCapture(ped->hwnd);
        ped->fMouseDown = TRUE;
        fChangedSel = TRUE;

        // Set the timer so that we can scroll automatically when the mouse
        // is moved outside the window rectangle.
        NtUserSetSystemTimer(ped->hwnd, IDSYS_SCROLL, gpsi->dtScroll, NULL);
        break;

    case WM_LBUTTONUP:
        if (ped->fMouseDown) {

            /*
             * Kill the timer so that we don't do auto mouse moves anymore
             */
            NtUserKillSystemTimer(ped->hwnd, IDSYS_SCROLL);
            NtUserReleaseCapture();
            MLSetCaretPosition(ped, hdc);
            ped->fMouseDown = FALSE;
        }
        break;
    }


    if (fChangedSel) {
        MLChangeSelection(ped, hdc, ichMinSel, ichMaxSel);
        MLEnsureCaretVisible(ped);
    }

    ECReleaseEditDC(ped, hdc, TRUE);

    if (!ped->fFocus && (message == WM_LBUTTONDOWN)) {

        /*
         * If we don't have the focus yet, get it
         */
        NtUserSetFocus(ped->hwnd);
    }
}

/***************************************************************************\
* MLScroll AorW
*
* History:
\***************************************************************************/

LONG MLScroll(
    PED  ped,
    BOOL fVertical,
    int  cmd,
    int  iAmt,
    BOOL fRedraw)
{
    SCROLLINFO  si;
    int         dx = 0;
    int         dy = 0;
    BOOL        fIncludeLeftMargin;
    int         newPos;
    int         oldPos;
    BOOL        fUp = FALSE;
    UINT        wFlag;
    DWORD       dwTime = 0;

    if (fRedraw && (cmd != ML_REFRESH)) {
        UpdateWindow(ped->hwnd);
    }

    if (ped->pLpkEditCallout && ped->fRtoLReading && !fVertical
        && ped->maxPixelWidth > ped->rcFmt.right - ped->rcFmt.left) {
        /*
         * Horizontal scoll of a right oriented window with a scrollbar.
         * Map the logical xOffset to visual coordinates.
         */
        oldPos = ped->maxPixelWidth
                 - ((int)ped->xOffset + ped->rcFmt.right - ped->rcFmt.left);
    } else
        oldPos = (int) (fVertical ? ped->ichScreenStart : ped->xOffset);

    fIncludeLeftMargin = (ped->xOffset == 0);

    switch (cmd) {
        case ML_REFRESH:
            newPos = oldPos;
            break;

        case EM_GETTHUMB:
            return(oldPos);

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:

            /*
             * If the edit contains more than 0xFFFF lines
             * it means that the scrolbar can return a position
             * that cannot fit in a WORD (16 bits), so use
             * GetScrollInfo (which is slower) in this case.
             */
            if (ped->cLines < 0xFFFF) {
                newPos = iAmt;
            } else {
                SCROLLINFO si;

                si.cbSize   = sizeof(SCROLLINFO);
                si.fMask    = SIF_TRACKPOS;

                GetScrollInfo( ped->hwnd, SB_VERT, &si);

                newPos = si.nTrackPos;
            }
            break;

        case SB_TOP:      // == SB_LEFT
            newPos = 0;
            break;

        case SB_BOTTOM:   // == SB_RIGHT
            if (fVertical)
                newPos = ped->cLines;
            else
                newPos = ped->maxPixelWidth;
            break;

        case SB_PAGEUP:   // == SB_PAGELEFT
            fUp = TRUE;
        case SB_PAGEDOWN: // == SB_PAGERIGHT

            if (fVertical)
                iAmt = ped->ichLinesOnScreen - 1;
            else
                iAmt = (ped->rcFmt.right - ped->rcFmt.left) - 1;

            if (iAmt == 0)
                iAmt++;

            if (fUp)
                iAmt = -iAmt;
            goto AddDelta;

        case SB_LINEUP:   // == SB_LINELEFT
            fUp = TRUE;
        case SB_LINEDOWN: // == SB_LINERIGHT

            dwTime = iAmt;

            iAmt = 1;

            if (fUp)
                iAmt = -iAmt;

            //   |             |
            //   |  FALL THRU  |
            //   V             V

        case EM_LINESCROLL:
            if (!fVertical)
                iAmt *= ped->aveCharWidth;

AddDelta:
            newPos = oldPos + iAmt;
            break;

        default:
            return(0L);
    }

    if (fVertical) {
        if (si.nMax = ped->cLines)
            si.nMax--;

        if (!ped->hwndParent ||
            TestWF(ValidateHwnd(ped->hwndParent), WFWIN40COMPAT))
            si.nPage = ped->ichLinesOnScreen;
        else
            si.nPage = 0;

        wFlag = WFVSCROLL;
    } else         {
        si.nMax  = ped->maxPixelWidth;
        si.nPage = ped->rcFmt.right - ped->rcFmt.left;
        wFlag = WFHSCROLL;
    }

    if (TestWF(ValidateHwnd(ped->hwnd), wFlag)) {
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        si.nMin  = 0;
        si.nPos = newPos;
        newPos = NtUserSetScrollInfo(ped->hwnd, fVertical ? SB_VERT : SB_HORZ,
                                     &si, fRedraw);
    } else {
        // BOGUS -- this is duped code from ScrollBar code
        // but it's for the case when we want to limit the position without
        // actually having the scroll bar
        int iMaxPos;

        // Clip page to 0, range + 1
        si.nPage = max(min((int)si.nPage, si.nMax + 1), 0);


        iMaxPos = si.nMax - (si.nPage ? si.nPage - 1 : 0);
        newPos = min(max(newPos, 0), iMaxPos);
    }

    oldPos -= newPos;

    if (!oldPos)
        return(0L);

    if (ped->pLpkEditCallout && ped->fRtoLReading && !fVertical
        && ped->maxPixelWidth > ped->rcFmt.right - ped->rcFmt.left) {
        // Map visual oldPos and newPos back to logical coordinates
        newPos = ped->maxPixelWidth
                 - (newPos + ped->rcFmt.right - ped->rcFmt.left);
        oldPos = -oldPos;
        if (newPos<0) {
            // Compensate for scroll bar returning pos > max-page
            oldPos += newPos;
            newPos=0;
        }
    }

    if (fVertical) {
        ped->ichScreenStart = newPos;
        dy = oldPos * ped->lineHeight;
    } else {
        ped->xOffset = newPos;
        dx = oldPos;
    }

    if (cmd != SB_THUMBTRACK)
        // We don't want to notify the parent of thumbtracking since they might
        // try to set the thumb position to something bogus.
        // NOTEPAD used to be guilty of this -- but I rewrote it so it's not.
        // The question is WHO ELSE does this? (jeffbog)
        ECNotifyParent(ped, fVertical ? EN_VSCROLL : EN_HSCROLL);

    if (fRedraw && _IsWindowVisible(ped->pwnd)) {
        RECT    rc;
        RECT    rcUpdate;
        RECT    rcClipRect;
        HDC     hdc;

        _GetClientRect(ped->pwnd, &rc);
        CopyRect(&rcClipRect, &ped->rcFmt);

        if (fVertical) { // Is this a vertical scroll?
            rcClipRect.left -= ped->wLeftMargin;
            rcClipRect.right += ped->wRightMargin;
        }

        IntersectRect(&rc, &rc, &rcClipRect);
        rc.bottom++;

        /*
         * Chicago has this HideCaret but there doesn't appear to be a
         * corresponding ShowCaret, so we lose the Caret under NT when the
         * EC scrolls - Johnl
         *
         * HideCaret(ped->hwnd);
         */

        hdc = ECGetEditDC(ped, FALSE);
        ECSetEditClip(ped, hdc, fIncludeLeftMargin);
        if (ped->hFont)
            SelectObject(hdc, ped->hFont);
        ECGetBrush(ped, hdc);

        if (ped->pLpkEditCallout && !fVertical) {
            // Horizontal scroll with complex script support
            int xFarOffset = ped->xOffset + ped->rcFmt.right - ped->rcFmt.left;

            rc = ped->rcFmt;
            if (dwTime != 0)
                ScrollWindowEx(ped->hwnd, ped->fRtoLReading ? -dx : dx, dy, NULL, NULL, NULL,
                        &rcUpdate, MAKELONG(SW_SMOOTHSCROLL | SW_SCROLLCHILDREN, dwTime));
            else
                NtUserScrollDC(hdc, ped->fRtoLReading ? -dx : dx, dy,
                               &rc, &rc, NULL, &rcUpdate);

            // Handle margins: Blank if clipped by horizontal scrolling,
            // display otherwise.
            if (ped->wLeftMargin) {
                rc.left  = ped->rcFmt.left - ped->wLeftMargin;
                rc.right = ped->rcFmt.left;
                if (   (ped->format != ES_LEFT)   // Always display margin for centred or far-aligned text
                    ||  // Display LTR left margin if first character fully visible
                        (!ped->fRtoLReading && ped->xOffset == 0)
                    ||  // Display RTL left margin if last character fully visible
                        (ped->fRtoLReading && xFarOffset >= ped->maxPixelWidth)) {
                    UnionRect(&rcUpdate, &rcUpdate, &rc);
                } else {
                    ExtTextOutW(hdc, rc.left, rc.top,
                        ETO_CLIPPED | ETO_OPAQUE | ETO_GLYPH_INDEX,
                        &rc, L"", 0, 0L);
                }
            }
            if (ped->wRightMargin) {
                rc.left  = ped->rcFmt.right;
                rc.right = ped->rcFmt.right + ped->wRightMargin;
                if (   (ped->format != ES_LEFT)   // Always display margin for centred or far-aligned text
                    ||  // Display RTL right margin if first character fully visible
                        (ped->fRtoLReading && ped->xOffset == 0)
                    ||  // Display LTR right margin if last character fully visible
                        (!ped->fRtoLReading && xFarOffset >= ped->maxPixelWidth)) {
                    UnionRect(&rcUpdate, &rcUpdate, &rc);
                } else {
                    ExtTextOutW(hdc, rc.left, rc.top,
                        ETO_CLIPPED | ETO_OPAQUE | ETO_GLYPH_INDEX,
                        &rc, L"", 0, 0L);
                }
            }
        } else {
            if (dwTime != 0)
                ScrollWindowEx(ped->hwnd, dx, dy, NULL, NULL, NULL,
                        &rcUpdate, MAKELONG(SW_SMOOTHSCROLL | SW_SCROLLCHILDREN, dwTime));
            else
                NtUserScrollDC(hdc, dx, dy, &rc, &rc, NULL, &rcUpdate);

            // If we need to wipe out the left margin area
            if (ped->wLeftMargin && !fVertical) {
                // Calculate the rectangle to be wiped out
                rc.right = rc.left;
                rc.left = max(0, ped->rcFmt.left - ped->wLeftMargin);
                if (rc.left < rc.right) {
                    if (fIncludeLeftMargin && (ped->xOffset != 0)) {

                        ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE,
                            &rc, L"", 0, 0L);
                    } else
                        if((!fIncludeLeftMargin) && (ped->xOffset == 0))
                            UnionRect(&rcUpdate, &rcUpdate, &rc);
                }
            }
        }
        MLSetCaretPosition(ped,hdc);

        ECReleaseEditDC(ped, hdc, FALSE);
        NtUserInvalidateRect(ped->hwnd, &rcUpdate,
                ((ped->ichLinesOnScreen + ped->ichScreenStart) >= ped->cLines));
        UpdateWindow(ped->hwnd);
    }

    return(MAKELONG(-oldPos, 1));
}

/***************************************************************************\
* MLSetFocus AorW
*
* Gives the edit control the focus and notifies the parent
* EN_SETFOCUS.
*
* History:
\***************************************************************************/

void MLSetFocus(
    PED ped)
{
    HDC hdc;

    if (!ped->fFocus) {
        ped->fFocus = 1; /* Set focus */

        hdc = ECGetEditDC(ped, TRUE);

        /*
         * Draw the caret. We need to do this even if the window is hidden
         * because in dlg box initialization time we may set the focus to a
         * hidden edit control window. If we don't create the caret etc, it will
         * never end up showing properly.
         */
        if (ped->pLpkEditCallout) {
            ped->pLpkEditCallout->EditCreateCaret (ped, hdc, ECGetCaretWidth(), ped->lineHeight, 0);
        }
        else {
            NtUserCreateCaret(ped->hwnd, (HBITMAP)NULL, ECGetCaretWidth(), ped->lineHeight);
        }
        NtUserShowCaret(ped->hwnd);
        MLSetCaretPosition(ped, hdc);

        /*
         * Show the current selection. Only if the selection was hidden when we
         * lost the focus, must we invert (show) it.
         */
        if (!ped->fNoHideSel && ped->ichMinSel != ped->ichMaxSel &&
                _IsWindowVisible(ped->pwnd))
            MLDrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel, TRUE);

        ECReleaseEditDC(ped, hdc, TRUE);

    }
#if 0
    MLEnsureCaretVisible(ped);
#endif

    /*
     * Notify parent we have the focus
     */
    ECNotifyParent(ped, EN_SETFOCUS);
}

/***************************************************************************\
* MLKillFocus AorW
*
* The edit control loses the focus and notifies the parent via
* EN_KILLFOCUS.
*
* History:
\***************************************************************************/

void MLKillFocus(
    PED ped)
{
    HDC hdc;

    /*
     * Reset the wheel delta count.
     */
    gcWheelDelta = 0;

    if (ped->fFocus) {
        ped->fFocus = 0; /* Clear focus */

        /*
         * Do this only if we still have the focus. But we always notify the
         * parent that we lost the focus whether or not we originally had the
         * focus.
         */

        /*
         * Hide the current selection if needed
         */
        if (!ped->fNoHideSel && ped->ichMinSel != ped->ichMaxSel &&
            _IsWindowVisible(ped->pwnd)) {
            hdc = ECGetEditDC(ped, FALSE);
            MLDrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel, TRUE);
            ECReleaseEditDC(ped, hdc, FALSE);
        }

        /*
         * Destroy the caret
         */
        NtUserDestroyCaret();
    }

    /*
     * Notify parent that we lost the focus.
     */
    ECNotifyParent(ped, EN_KILLFOCUS);
}

/***************************************************************************\
* MLEnsureCaretVisible AorW
*
* Scrolls the caret into the visible region.
* Returns TRUE if scrolling was done else return s FALSE.
*
* History:
\***************************************************************************/

BOOL MLEnsureCaretVisible(
    PED ped)
{
    UINT   iLineMax;
    int    xposition;
    BOOL   fPrevLine;
    HDC    hdc;
    BOOL   fVScroll = FALSE;
    BOOL   fHScroll = FALSE;

    if (_IsWindowVisible(ped->pwnd)) {
        int iAmt;
        int iFmtWidth = ped->rcFmt.right - ped->rcFmt.left;

        if (ped->fAutoVScroll) {
            iLineMax = ped->ichScreenStart + ped->ichLinesOnScreen - 1;

            if (fVScroll = (ped->iCaretLine > iLineMax))
                iAmt = iLineMax;
            else if (fVScroll = (ped->iCaretLine < ped->ichScreenStart))
                iAmt = ped->ichScreenStart;

            if (fVScroll)
                MLScroll(ped, TRUE, EM_LINESCROLL, ped->iCaretLine - iAmt, TRUE);
        }

        if (ped->fAutoHScroll && ((int) ped->maxPixelWidth > iFmtWidth)) {
            POINT pt;
            /* Get the current position of the caret in pixels */
            if ((UINT) (ped->cLines - 1) != ped->iCaretLine &&
                ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
                fPrevLine = TRUE;
            else
                fPrevLine = FALSE;

            hdc = ECGetEditDC(ped,TRUE);
            MLIchToXYPos(ped, hdc, ped->ichCaret, fPrevLine, &pt);
            ECReleaseEditDC(ped, hdc, TRUE);
            xposition = pt.x;

            // Remember, MLIchToXYPos returns coordinates with respect to the
            // top left pixel displayed on the screen.  Thus, if xPosition < 0,
            // it means xPosition is less than current ped->xOffset.

            iFmtWidth /= 3;
            if (fHScroll = (xposition < ped->rcFmt.left))
                // scroll to the left
                iAmt = ped->rcFmt.left + iFmtWidth;
            else if (fHScroll = (xposition > ped->rcFmt.right))
                // scroll to the right
                iAmt = ped->rcFmt.right - iFmtWidth;

            if (fHScroll)
                MLScroll(ped, FALSE, EM_LINESCROLL, (xposition - iAmt) / ped->aveCharWidth, TRUE);
        }
    }
    return(fVScroll);
}

/***************************************************************************\
* MLEditWndProc
*
* Class procedure for all multi line edit controls.
* Dispatches all messages to the appropriate handlers which are named
* as follows:
* SL (single line) prefixes all single line edit control procedures while
* EC (edit control) prefixes all common handlers.
*
* The MLEditWndProc only handles messages specific to multi line edit
* controls.
*
* WARNING: If you add a message here, add it to gawEditWndProc[] in
* kernel\server.c too, otherwise EditWndProcA/W will send it straight to
* DefWindowProcWorker
*
* History:
\***************************************************************************/

LRESULT MLEditWndProc(
    HWND hwnd,
    PED ped,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    LPRECT      lprc;
    POINT       pt;
    DWORD       windowstyle;

    switch (message) {

    case WM_INPUTLANGCHANGE:
        if (ped && ped->fFocus && ped->pLpkEditCallout) {
            NtUserHideCaret(hwnd);
            hdc = ECGetEditDC(ped, TRUE);
            NtUserDestroyCaret();
            ped->pLpkEditCallout->EditCreateCaret (ped, hdc, ECGetCaretWidth(), ped->lineHeight, (UINT)lParam);
            MLSetCaretPosition(ped, hdc);
            ECReleaseEditDC(ped, hdc, TRUE);
            NtUserShowCaret(hwnd);
        }
        goto PassToDefaultWindowProc;


    case WM_STYLECHANGED:
        if (ped && ped->pLpkEditCallout) {
            switch (wParam) {

                case GWL_STYLE:
                    ECUpdateFormat(ped,
                        ((LPSTYLESTRUCT)lParam)->styleNew,
                        GetWindowLong(ped->hwnd, GWL_EXSTYLE));
                    return 1L;

                case GWL_EXSTYLE:
                    ECUpdateFormat(ped,
                        GetWindowLong(ped->hwnd, GWL_STYLE),
                        ((LPSTYLESTRUCT)lParam)->styleNew);
                    return 1L;
            }
        }

        goto PassToDefaultWindowProc;

    case WM_CHAR:

        /*
         * wParam - the value of the key
         * lParam - modifiers, repeat count etc (not used)
         */
        MLChar(ped, (UINT)wParam, 0);
        break;

    case WM_ERASEBKGND:  {
            HBRUSH  hbr;

            // USE SAME RULES AS IN ECGetBrush()
            if (ped->f40Compat &&
                (ped->fReadOnly || ped->fDisabled))
                hbr = (HBRUSH) CTLCOLOR_STATIC;
            else
                hbr = (HBRUSH) CTLCOLOR_EDIT;

            FillWindow(ped->hwndParent, hwnd, (HDC)wParam, hbr);
        }
        return ((LONG)TRUE);

    case WM_GETDLGCODE: {
            LONG code = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS | DLGC_WANTALLKEYS;

            /*
             ** !!! JEFFBOG HACK !!!
             ** Only set Dialog Box Flag if GETDLGCODE message is generated by
             ** IsDialogMessage -- if so, the lParam will be a pointer to the
             ** message structure passed to IsDialogMessage; otherwise, lParam
             ** will be NULL. Reason for the HACK alert: the wParam & lParam
             ** for GETDLGCODE is still not clearly defined and may end up
             ** changing in a way that would throw this off
             **
             */
            if (lParam)
               ped->fInDialogBox = TRUE; // Mark ML edit ctrl as in a dialog box

            /*
             ** If this is a WM_SYSCHAR message generated by the UNDO keystroke
             ** we want this message so we can EAT IT in "case WM_SYSCHAR:"
             */
            if (lParam && (((LPMSG)lParam)->message == WM_SYSCHAR) &&
                    ((DWORD)((LPMSG)lParam)->lParam & SYS_ALTERNATE) &&
                    ((WORD)wParam == VK_BACK))
                 code |= DLGC_WANTMESSAGE;
            return code;
        }

    case EM_SCROLL:
        message = WM_VSCROLL;

        /*
         * FALL THROUGH
         */
    case WM_HSCROLL:
    case WM_VSCROLL:
        return MLScroll(ped, (message==WM_VSCROLL), LOWORD(wParam), HIWORD(wParam), TRUE);

    case WM_MOUSEWHEEL:
        /*
         * Don't handle zoom and datazoom.
         */
        if (wParam & (MK_SHIFT | MK_CONTROL)) {
            goto PassToDefaultWindowProc;
        }

        gcWheelDelta -= (short) HIWORD(wParam);
        windowstyle = ped->pwnd->style;
        if (    abs(gcWheelDelta) >= WHEEL_DELTA &&
                gpsi->ucWheelScrollLines > 0 &&
                (windowstyle & (WS_VSCROLL | WS_HSCROLL))) {

            int     cLineScroll;
            BOOL    fVert;
            int     cPage;

            if (windowstyle & WS_VSCROLL) {
                fVert = TRUE;
                cPage = ped->ichLinesOnScreen;
            } else {
                fVert = FALSE;
                cPage = (ped->rcFmt.right - ped->rcFmt.left) / ped->aveCharWidth;
            }

            /*
             * Limit a roll of one (1) WHEEL_DELTA to scroll one (1) page.
             */
            cLineScroll = (int) min(
                    (UINT) (max(1, (cPage - 1))),
                    gpsi->ucWheelScrollLines);

            cLineScroll *= (gcWheelDelta / WHEEL_DELTA);
            UserAssert(cLineScroll != 0);
            gcWheelDelta = gcWheelDelta % WHEEL_DELTA;
            MLScroll(ped, fVert, EM_LINESCROLL, cLineScroll, TRUE);
        }

        break;

    case WM_KEYDOWN:

        /*
         * wParam - virt keycode of the given key
         * lParam - modifiers such as repeat count etc. (not used)
         */
        MLKeyDown(ped, (UINT)wParam, 0);
        break;

    case WM_KILLFOCUS:

        /*
         * wParam - handle of the window that receives the input focus
         * lParam - not used
         */
        MLKillFocus(ped);
        break;

    case WM_CAPTURECHANGED:
        //
        // wParam -- unused
        // lParam -- hwnd of window gaining capture.
        //
        if (ped->fMouseDown) {
            //
            // We don't change the caret pos here.  If this is happening
            // due to button up, then we'll change the pos in the
            // handler after ReleaseCapture().  Otherwise, just end
            // gracefully because someone else has stolen capture out
            // from under us.
            //

            ped->fMouseDown = FALSE;
            NtUserKillSystemTimer(ped->hwnd, IDSYS_SCROLL);
        }
        break;

    case WM_SYSTIMER:

        /*
         * This allows us to automatically scroll if the user holds the mouse
         * outside the edit control window. We simulate mouse moves at timer
         * intervals set in MouseMotionHandler.
         */
        if (ped->fMouseDown)
            MLMouseMotion(ped, WM_MOUSEMOVE, ped->prevKeys, &ped->ptPrevMouse);
        break;

    case WM_MBUTTONDOWN:
        EnterReaderModeHelper(ped->hwnd);
        break;

    case WM_MOUSEMOVE:
        UserAssert(ped->fMouseDown);

        /*
         * FALL THROUGH
         */
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        /*
         * wParam - contains a value that indicates which virtual keys are down
           lParam - contains x and y coords of the mouse cursor
         */
        POINTSTOPOINT(pt, lParam);
        MLMouseMotion(ped, message, (UINT)wParam, &pt);
        break;

    case WM_CREATE:

        /*
         * wParam - handle to window being created
         * lParam - points to a CREATESTRUCT that contains copies of parameters
         * passed to the CreateWindow function.
         */
        return (MLCreate(ped, (LPCREATESTRUCT)lParam));

    case WM_PRINTCLIENT:
        MLPaint(ped, (HDC) wParam, NULL);
        break;

    case WM_PAINT:
        /*
         * wParam - can be hdc from subclassed paint
           lParam - not used
         */
        if (wParam) {
            hdc = (HDC) wParam;
            lprc = NULL;
        } else {
            hdc = NtUserBeginPaint(ped->hwnd, &ps);
            lprc = &ps.rcPaint;
        }

        if (_IsWindowVisible(ped->pwnd))
            MLPaint(ped, hdc, lprc);

        if (!wParam)
            NtUserEndPaint(ped->hwnd, &ps);
        break;

    case WM_PASTE:

        /*
         * wParam - not used
           lParam - not used
         */
        if (!ped->fReadOnly)
            MLPasteText(ped);
        break;

    case WM_SETFOCUS:

        /*
         * wParam - handle of window that loses the input focus (may be NULL)
           lParam - not used
         */
        MLSetFocus(ped);
        break;

    case WM_SIZE:

        /*
         * wParam - defines the type of resizing fullscreen, sizeiconic,
                    sizenormal etc.
           lParam - new width in LOWORD, new height in HIGHWORD of client area
         */
        ECSize(ped, NULL, TRUE);
        break;

    case EM_FMTLINES:

        /*
         * wParam - indicates disposition of end-of-line chars. If non
         * zero, the chars CR CR LF are placed at the end of a word
         * wrapped line. If wParam is zero, the end of line chars are
         * removed. This is only done when the user gets a handle (via
         * EM_GETHANDLE) to the text. lParam - not used.
         */
        if (wParam)
            MLInsertCrCrLf(ped);
        else
            MLStripCrCrLf(ped);
        MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);
        return (LONG)(wParam != 0);

    case EM_GETHANDLE:

        /*
         * wParam - not used
            lParam - not used
         */

        /*
         * Returns a handle to the edit control's text.
         */

        /*
         * Null terminate the string. Note that we are guaranteed to have the
         * memory for the NULL since ECInsertText allocates an extra
         * WCHAR for the NULL terminator.
         */

        if (ped->fAnsi)
            *(ECLock(ped) + ped->cch) = 0;
        else
            *((LPWSTR)ECLock(ped) + ped->cch) = 0;
        ECUnlock(ped);
        return ((LRESULT)ped->hText);

    case EM_GETLINE:

        /*
         * wParam - line number to copy (0 is first line)
         * lParam - buffer to copy text to. First WORD is max # of bytes to
         * copy
         */
        return MLGetLine(ped, (ICH)wParam, (ICH)*(WORD UNALIGNED *)lParam, (LPSTR)lParam);

    case EM_LINEFROMCHAR:

        /*
         * wParam - Contains the index value for the desired char in the text
         * of the edit control. These are 0 based.
         * lParam - not used
         */
        return (LRESULT)MLIchToLine(ped, (ICH)wParam);

    case EM_LINEINDEX:

        /*
         * wParam - specifies the desired line number where the number of the
         * first line is 0. If linenumber = 0, the line with the caret is used.
         * lParam - not used.
         * This function return s the number of character positions that occur
         * preceeding the first char in a given line.
         */
        return (LRESULT)MLLineIndex(ped, (ICH)wParam);

    case EM_LINELENGTH:

        /*
         * wParam - specifies the character index of a character in the
           specified line, where the first line is 0. If -1, the length
           of the current line (with the caret) is return ed not including the
           length of any selected text.
           lParam - not used
         */
        return (LRESULT)MLLineLength(ped, (ICH)wParam);

    case EM_LINESCROLL:

        /*
         * wParam - not used
           lParam - Contains the number of lines and char positions to scroll
         */
        MLScroll(ped, TRUE,  EM_LINESCROLL, (INT)lParam, TRUE);
        MLScroll(ped, FALSE, EM_LINESCROLL, (INT)wParam, TRUE);
        break;

    case EM_REPLACESEL:

        /*
         * wParam - flag for 4.0+ apps saying whether to clear undo
           lParam - Points to a null terminated replacement text.
         */
        MLReplaceSel(ped, (LPSTR)lParam);
        if (!ped->f40Compat || !wParam)
            ECEmptyUndo(Pundo(ped));
        break;

    case EM_SETHANDLE:

        /*
         * wParam - contains a handle to the text buffer
           lParam - not used
         */
        MLSetHandle(ped, (HANDLE)wParam);
        break;

    case EM_SETRECT:
    case EM_SETRECTNP:

        //
        // wParamLo --    not used
        // lParam --    LPRECT with new formatting area
        //
        ECSize(ped, (LPRECT) lParam, (message != EM_SETRECTNP));
        break;

    case EM_SETSEL:

        /*
         * wParam - Under 3.1, specifies if we should scroll caret into
         * view or not. 0 == scroll into view. 1 == don't scroll
         * lParam - starting pos in lowword ending pos in high word
         *
         * Under Win32, wParam is the starting pos, lParam is the
         * ending pos, and the caret is not scrolled into view.
         * The message EM_SCROLLCARET forces the caret to be scrolled
         * into view.
         */
        MLSetSelection(ped, TRUE, (ICH)wParam, (ICH)lParam);
        break;

    case EM_SCROLLCARET:

        /*
         * Scroll caret into view
         */
        MLEnsureCaretVisible(ped);
        break;

    case EM_GETFIRSTVISIBLELINE:

        /*
         * Returns the first visible line for multiline edit controls.
         */
        return (LONG)ped->ichScreenStart;

    case WM_SYSKEYDOWN:
        if (((WORD)wParam == VK_BACK) && ((DWORD)lParam & SYS_ALTERNATE)) {
            SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
            break;
        }
        goto PassToDefaultWindowProc;

    case WM_UNDO:
    case EM_UNDO:
        return MLUndo(ped);

    case EM_SETTABSTOPS:

        /*
         * This sets the tab stop positions for multiline edit controls.
         * wParam - Number of tab stops
         * lParam - Far ptr to a UINT array containing the Tab stop positions
         */
        return MLSetTabStops(ped, (int)wParam, (LPINT)lParam);

    case EM_POSFROMCHAR:
        //
        // wParam --    char index in text
        // lParam --    not used
        // This function returns the (x,y) position of the character
        //
    case EM_CHARFROMPOS:
        //
        // wParam --    unused
        // lParam --    pt in client coordinates
        // This function returns
        //      LOWORD: the position of the closest character
        //              to the passed in point.  Beware of
        //              points not actually in the edit client...
        //      HIWORD: the index of the line the char is on
        //
        {
            LONG  xyPos;
            LONG  line;

            hdc = ECGetEditDC(ped, TRUE);

            if (message == EM_POSFROMCHAR) {
                MLIchToXYPos(ped, hdc, (ICH)wParam, FALSE, &pt);
                xyPos = MAKELONG(pt.x, pt.y);
            } else {
                POINTSTOPOINT(pt, lParam);
                xyPos = MLMouseToIch(ped, hdc, &pt, &line);
                xyPos = MAKELONG(xyPos, line);
            }

            ECReleaseEditDC(ped, hdc, TRUE);
            return((LRESULT)xyPos);
            break;
        }

    case WM_SETREDRAW:
        DefWindowProcWorker(ped->pwnd, message, wParam, lParam, FALSE);
        if (wParam) {

            /*
             * Backwards compatability hack needed so that winraid's edit
             * controls work fine.
             */
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
        }
      break;

#if LATER
    case WM_IME_ENDCOMPOSITION:
        ECInOutReconversionMode(ped, FALSE);
        break;
#endif

    default:
PassToDefaultWindowProc:
        return DefWindowProcWorker(ped->pwnd, message, wParam, lParam, ped->fAnsi);
    }

    return 1L;
} /* MLEditWndProc */


/***************************************************************************\
* MLDrawText AorW
*
*  This function draws all the characters between ichStart and ichEnd for
*  the given Multiline Edit Control.
*
*  This function divides the block of text between ichStart and ichEnd
*  into lines and each line into strips of text based on the selection
*  attributes. It calls ECTabTheTextOut() to draw each strip.
*  This takes care of the Negative A anc C widths of the current font, if
*  it has any, on either side of each strip of text.
*
*  NOTE: If the language pack is loaded the text is not divided into strips,
*  nor is selection highlighting performed here. Whole lines are passed
*  to the language pack to display with tab expansion and selection
*  highlighting. (Since the language pack supports scripts with complex
*  character re-ordering rules, only it can do this).
*
* History:
\***************************************************************************/

void MLDrawText(
    PED ped,
    HDC hdc,
    ICH ichStart,
    ICH ichEnd,
    BOOL fSelChange)
{
    DWORD   textColorSave;
    DWORD   bkColorSave;
    PSTR    pText;
    UINT    wCurLine;
    UINT    wEndLine;
    int     xOffset;
    ICH     LengthToDraw;
    ICH     CurStripLength;
    ICH     ichAttrib, ichNewStart;
    ICH     ExtraLengthForNegA;
    ICH     ichT;
    int     iRemainingLengthInLine;
    int     xStPos, xClipStPos, xClipEndPos, yPos;
    BOOL    fFirstLineOfBlock   = TRUE;
    BOOL    fDrawEndOfLineStrip = FALSE;
    BOOL    fDrawOnSameLine     = FALSE;
    BOOL    fSelected                = FALSE;
    BOOL    fLineBegins      = FALSE;
    STRIPINFO   NegCInfo;
    POINT   pt;

    //
    // Just return if nothing to draw
    if (!ped->ichLinesOnScreen)
        return;

    ECGetBrush(ped, hdc);

    //
    // Adjust the value of ichStart such that we need to draw only those lines
    // visible on the screen.
    //
    if ((UINT)ichStart < (UINT)ped->chLines[ped->ichScreenStart]) {
        ichStart = ped->chLines[ped->ichScreenStart];
        if (ichStart > ichEnd)
            return;
    }

    // Adjust the value of ichEnd such that we need to draw only those lines
    // visible on the screen.
    wCurLine = min(ped->ichScreenStart+ped->ichLinesOnScreen,ped->cLines-1);
    ichT = ped->chLines[wCurLine] + MLLine(ped, wCurLine);
    ichEnd = min(ichEnd, ichT);

    wCurLine = MLIchToLine(ped, ichStart);    // Starting line.
    wEndLine = MLIchToLine(ped, ichEnd);           // Ending line.

    UserAssert(ped->chLines[wCurLine] <= ped->cch + 1);
    UserAssert(ped->chLines[wEndLine] <= ped->cch + 1);

    if (fSelChange && (GetBkMode(hdc) != OPAQUE))
    {
        /*
         * if changing selection on a transparent edit control, just
         * draw those lines from scratch
         */
        RECT rcStrip;
        CopyRect(&rcStrip, &ped->rcFmt);
        rcStrip.left -= ped->wLeftMargin;
        if (ped->pLpkEditCallout) {
            rcStrip.right += ped->wRightMargin;
        }
        rcStrip.top += (wCurLine - ped->ichScreenStart) * ped->lineHeight;
        rcStrip.bottom = rcStrip.top + ((wEndLine - wCurLine) + 1) * ped->lineHeight;
        NtUserInvalidateRect(ped->hwnd, &rcStrip, TRUE);
        return;
    }

    // If it is either centered or right-justified, then draw the whole lines.
    // Also draw whole lines if the language pack is handling line layout.
    if ((ped->format != ES_LEFT) || (ped->pLpkEditCallout)) {
        ichStart = ped->chLines[wCurLine];
        ichEnd = ped->chLines[wEndLine] + MLLine(ped, wEndLine);
    }

    pText = ECLock(ped);

    NtUserHideCaret(ped->hwnd);

    //
    // If ichStart stays on Second byte of DBCS, we have to
    // adjust it. LiZ -- 5/5/93
    //
    if (ped->fAnsi && ped->fDBCS) {
        ichStart = ECAdjustIch( ped, pText, ichStart );
    }
    UserAssert(ichStart <= ped->cch);
    UserAssert(ichEnd <= ped->cch);

    while (ichStart <= ichEnd) {
        // Pass whole lines to the language pack to display with selection
        // marking and tab expansion.
        if (ped->pLpkEditCallout) {
            ped->pLpkEditCallout->EditDrawText(
                ped, hdc, pText + ped->cbChar*ichStart,
                MLLine(ped, wCurLine),
                (INT)ped->ichMinSel - (INT)ichStart, (INT)ped->ichMaxSel - (INT)ichStart,
                MLIchToYPos(ped, ichStart, FALSE));
        } else {
        // xStPos:      The starting Position where the string must be drawn.
        // xClipStPos:  The starting position for the clipping rect for the block.
        // xClipEndPos: The ending position for the clipping rect for the block.

        // Calculate the xyPos of starting point of the block.
        MLIchToXYPos(ped, hdc, ichStart, FALSE, &pt);
        xClipStPos = xStPos = pt.x;
        yPos = pt.y;

        // The attributes of the block is the same as that of ichStart.
        ichAttrib = ichStart;

        // If the current font has some negative C widths and if this is the
        // begining of a block, we must start drawing some characters before the
        // block to account for the negative C widths of the strip before the
        // current strip; In this case, reset ichStart and xStPos.

        if (fFirstLineOfBlock && ped->wMaxNegC) {
            fFirstLineOfBlock = FALSE;
            ichNewStart = max(((int)(ichStart - ped->wMaxNegCcharPos)), ((int)ped->chLines[wCurLine]));

            // If ichStart needs to be changed, then change xStPos also accordingly.
            if (ichNewStart != ichStart) {
                if (ped->fAnsi && ped->fDBCS) {
                    //
                    // Adjust DBCS alignment...
                    //
                    ichNewStart = ECAdjustIchNext( ped, pText, ichNewStart );
                }
                MLIchToXYPos(ped, hdc, ichStart = ichNewStart, FALSE, &pt);
                xStPos = pt.x;
            }
        }

        // Calc the number of characters remaining to be drawn in the current line.
        iRemainingLengthInLine = MLLine(ped, wCurLine) -
                                (ichStart - ped->chLines[wCurLine]);

        // If this is the last line of a block, we may not have to draw all the
        // remaining lines; We must draw only upto ichEnd.
        if (wCurLine == wEndLine)
            LengthToDraw = ichEnd - ichStart;
        else
            LengthToDraw = iRemainingLengthInLine;

        // Find out how many pixels we indent the line for non-left-justified
        // formats
        if (ped->format != ES_LEFT)
            xOffset = MLCalcXOffset(ped, hdc, wCurLine);
        else
            xOffset = -((int)(ped->xOffset));

        // Check if this is the begining of a line.
        if (ichAttrib == ped->chLines[wCurLine]) {
            fLineBegins = TRUE;
            xClipStPos = ped->rcFmt.left - ped->wLeftMargin;
        }

        //
        // The following loop divides this 'wCurLine' into strips based on the
        // selection attributes and draw them strip by strip.
        do  {
            //
            // If ichStart is pointing at CRLF or CRCRLF, then iRemainingLength
            // could have become negative because MLLine does not include
            // CR and LF at the end of a line.
            //
            if (iRemainingLengthInLine < 0)  // If Current line is completed,
                break;                   // go on to the next line.

            //
            // Check if a part of the block is selected and if we need to
            // show it with a different attribute.
            //
            if (!(ped->ichMinSel == ped->ichMaxSel ||
                        ichAttrib >= ped->ichMaxSel ||
                        ichEnd   <  ped->ichMinSel ||
                        (!ped->fNoHideSel && !ped->fFocus))) {
                //
                // OK! There is a selection somewhere in this block!
                // Check if this strip has selection attribute.
                //
                if (ichAttrib < ped->ichMinSel) {
                    fSelected = FALSE;  // This strip is not selected

                    // Calculate the length of this strip with normal attribute.
                    CurStripLength = min(ichStart+LengthToDraw, ped->ichMinSel)-ichStart;
                    fLineBegins = FALSE;
                } else {
                    // The current strip has the selection attribute.
                    if (fLineBegins) {  // Is it the first part of a line?
                        // Then, draw the left margin area with normal attribute.
                        fSelected = FALSE;
                        CurStripLength = 0;
                        xClipStPos = ped->rcFmt.left - ped->wLeftMargin;
                        fLineBegins = FALSE;
                    } else {
                        // Else, draw the strip with selection attribute.
                        fSelected = TRUE;
                        CurStripLength = min(ichStart+LengthToDraw, ped->ichMaxSel)-ichStart;

                        // Select in the highlight colors.
                        bkColorSave = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                        if (!ped->fDisabled)
                            textColorSave = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    }
                }
            } else {
                // The whole strip has no selection attributes.
                CurStripLength = LengthToDraw;
            }

            //
            // Other than the current strip, do we still have anything
            // left to be drawn in the current line?
            //
            fDrawOnSameLine = (LengthToDraw != CurStripLength);

            //
            // When we draw this strip, we need to draw some more characters
            // beyond the end of this strip to account for the negative A
            // widths of the characters that follow this strip.
            //
            ExtraLengthForNegA = min(iRemainingLengthInLine-CurStripLength, ped->wMaxNegAcharPos);

            //
            // The blank strip at the end of the line needs to be drawn with
            // normal attribute irrespective of whether the line has selection
            // attribute or not. Hence, if the last strip of the line has selection
            // attribute, then this blank strip needs to be drawn separately.
            // Else, we can draw the blank strip along with the last strip.
            //

            // Is this the last strip of the current line?
            if (iRemainingLengthInLine == (int)CurStripLength) {
                if (fSelected) { // Does this strip have selection attribute?
                    // Then we need to draw the end of line strip separately.
                    fDrawEndOfLineStrip = TRUE;  // Draw the end of line strip.
                    MLIchToXYPos(ped, hdc, ichStart+CurStripLength, TRUE, &pt);
                    xClipEndPos = pt.x;
                } else {
                    //
                    // Set the xClipEndPos to a big value sothat the blank
                    // strip will be drawn automatically when the last strip
                    // is drawn.
                    //
                    xClipEndPos = MAXCLIPENDPOS;
                }
            } else {
                //
                // This is not the last strip of this line; So, set the ending
                // clip position accurately.
                //
                MLIchToXYPos(ped, hdc, ichStart+CurStripLength, FALSE, &pt);
                xClipEndPos = pt.x;
            }

            //
            // Draw the current strip starting from xStPos, clipped to the area
            // between xClipStPos and xClipEndPos. Obtain "NegCInfo" and use it
            // in drawing the next strip.
            //
            ECTabTheTextOut(hdc, xClipStPos, xClipEndPos,
                    xStPos, yPos, (LPSTR)(pText+ichStart*ped->cbChar),
                CurStripLength+ExtraLengthForNegA, ichStart, ped,
                ped->rcFmt.left+xOffset, fSelected ? ECT_SELECTED : ECT_NORMAL, &NegCInfo);

            if (fSelected) {
                //
                // If this strip was selected, then the next strip won't have
                // selection attribute
                //
                fSelected = FALSE;
                SetBkColor(hdc, bkColorSave);
                if (!ped->fDisabled)
                    SetTextColor(hdc, textColorSave);
            }

            // Do we have one more strip to draw on the current line?
            if (fDrawOnSameLine || fDrawEndOfLineStrip) {
                int  iLastDrawnLength;

                //
                // Next strip's attribute is decided based on the char at ichAttrib
                //
                ichAttrib = ichStart + CurStripLength;

                //
                // When drawing the next strip, start at a few chars before
                // the actual start to account for the Neg 'C' of the strip
                // just drawn.
                //
                iLastDrawnLength = CurStripLength +ExtraLengthForNegA - NegCInfo.nCount;
                //
                // Adjust DBCS alignment...
                //
                if (ped->fAnsi && ped->fDBCS) {
                    ichNewStart = ECAdjustIch(ped,pText,ichStart+iLastDrawnLength);
                    iLastDrawnLength = ichNewStart - ichStart;
                    ichStart = ichNewStart;
                } else {
                    ichStart += iLastDrawnLength;
                }
                LengthToDraw -= iLastDrawnLength;
                iRemainingLengthInLine -= iLastDrawnLength;

                //
                // The start of clip rect for the next strip.
                //
                xStPos = NegCInfo.XStartPos;
                xClipStPos = xClipEndPos;
            }

            // Draw the blank strip at the end of line seperately, if required.
            if (fDrawEndOfLineStrip) {
                ECTabTheTextOut(hdc, xClipStPos, MAXCLIPENDPOS, xStPos, yPos,
                    (LPSTR)(pText+ichStart*ped->cbChar), LengthToDraw, ichStart,
                    ped, ped->rcFmt.left+xOffset, ECT_NORMAL, &NegCInfo);

                fDrawEndOfLineStrip = FALSE;
            }
        }
        while(fDrawOnSameLine);   // do while loop ends here.
        }

        // Let us move on to the next line of this block to be drawn.
        wCurLine++;
        if (ped->cLines > wCurLine)
            ichStart = ped->chLines[wCurLine];
        else
            ichStart = ichEnd+1;   // We have reached the end of the text.
    }  // while loop ends here

    ECUnlock(ped);

    NtUserShowCaret(ped->hwnd);
    MLSetCaretPosition(ped, hdc);
}
