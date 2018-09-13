/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Memwin.c

Abstract:

    This module contains the main line code for display of multiple memory
    windows and the subclassed win proc to handle editing, display, etc.

Author:

    Griffith Wm. Kadnier (v-griffk) 26-Jul-1992

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop

#include <ime.h>



extern unsigned int InMemUpdate; // prevent multiple viemem() calls

BOOL IsDBCSCharSet(DWORD cs);
extern  CXF     CxfIp;

void FAR * PASCAL MMLpvLockMb(HDEP hmem);
void    PASCAL MMbUnlockMb(HDEP hmem);

static void NEAR GetMemText (void);

int     RgbFmts[] = MEM_FORMATS;

struct memWinDesc MemWinDesc[MAX_VIEWS];
struct memWinDesc TempMemWinDesc;
char   memText[MAX_MSG_TXT];
char   cMem[MAX_MSG_TXT];     //temp for edit/validation
char   cMemTemp[MAX_MSG_TXT];     //temp for edit/validation
char   cDoc[MAX_USER_LINE];
BOOL   fAscii = FALSE;        // is memwin edit taking place in an ascii field
                              // with MW_BYTE / twofield representation?
enum {
    GOTO_FIRST,
    GOTO_LAST,
    GOTO_FIRSTONLINE,
    GOTO_LASTONLINE,
    GOTO_NEXT,
    GOTO_PREVIOUS
};

enum {
    STARTED,
    INPROGRESS,
    FINISHED
};




/***    UnformatDataItem
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/


BOOL
UnformatDataItem (
    char * lpch,
    char * lpb
    )
{
    ULONG       cBits;
    FMTTYPE     fmtType;
    EERADIX     uradix;
    ULONG       fTwoFields;
    ULONG       cchMax;
    char        *lpIndx;
    EESTATUS    eeErr = EENOERROR;


  Dbg(CPFormatEnumerate(MemWinDesc[memView].iFormat,
                        &cBits,
                        &fmtType,
                        &uradix,
                        &fTwoFields,
                        &cchMax,
                        NULL) == EENOERROR);


    switch (MemWinDesc[memView].iFormat) {

      case MW_ASCII:
        if ((*lpch > 0x00) && (*lpch < 0x7F)) {
            *lpb = *lpch;
            return TRUE;
        } else {
            return FALSE;
        }

      case MW_BYTE:
        if (fAscii == TRUE) {
            if ((*lpch > 0x00) && (*lpch < 0x7F)) {
                *lpb = *lpch;
                return TRUE;
            } else {
                return FALSE;
            }
        }
      case MW_SHORT:
      case MW_SHORT_HEX:
      case MW_SHORT_UNSIGNED:
      case MW_LONG:
      case MW_LONG_HEX:
      case MW_LONG_UNSIGNED:
      case MW_QUAD:
      case MW_QUAD_HEX:
      case MW_QUAD_UNSIGNED:

        if ((eeErr = CPUnformatMemory ((PUCHAR) lpb, lpch, cBits,
                                fmtType | fmtOverRide, uradix)) == EENOERROR) {
            return TRUE;
        } else {
            return FALSE;
        }

      case MW_REAL:
      case MW_REAL_LONG:
      case MW_REAL_TEN:
        lpIndx = lpch;
        do {
            if (!(isdigit (*lpIndx))
                    && ((*lpIndx != '.')
                    && (*lpIndx != '+')
                    && (*lpIndx != '-')
                    && (*lpIndx != ' ')
                    && (*lpIndx != 'e')
                    && (*lpIndx != 'E')) )
            {
                return (FALSE);
            }
            lpIndx++;
        } while (*lpIndx != '\0');
        if ((eeErr = CPUnformatMemory ( (PUCHAR) lpb, lpch, cBits,
                                fmtType | fmtOverRide, uradix)) == EENOERROR) {
            return TRUE;
        }

      default:
        return FALSE;

    }

    return FALSE;
}                                       /* UnformatDataItem() */



/***    GotoField
**
**  Synopsis:
**      void = GotoField(action)
**
**  Entry:
**      action
**
**  Returns:
**
**  Description:
**
*/

static void NEAR
GotoField(
    WORD action
    )
{
    int n = 0;
    int startX = Views[memView].X;
    int startY = Views[memView].Y;


    if (!DebuggeeActive() || !DebuggeeAlive()) {
        return;
    }


    if (memView == -1) {
        return;
    }


    if (MemWinDesc[memView].lpMi == NULL) {
        PosXY(memView, 0, 0, FALSE);
    }

    if (startX < MemWinDesc[memView].lpMi[1].iStart) {
        PosXY(memView, MemWinDesc[memView].lpMi[1].iStart, startY, FALSE);
        GetMemText ();
        return;
    }

    switch (action) {

      case GOTO_FIRST:
        startY = 0;
        //fall through to set line element

      case GOTO_FIRSTONLINE:
        if ((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE)) {
            n = (MemWinDesc[memView].cMi / 2) + 1;
        } else {
            n = 1;
        }
        break;

      case GOTO_LAST:
        startY = (Docs[Views[memView].Doc].NbLines - 1);
        //fall through to set line element

      case GOTO_LASTONLINE:
        if ((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == FALSE)) {
            n = (MemWinDesc[memView].cMi / 2);
        } else {
            n = MemWinDesc[memView].cMi - 1;
        }
        break;

      case GOTO_NEXT:
        for (n =
                ((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE)) ?
                        ((int)((MemWinDesc[memView].cMi / 2) + 1))
                        : 1;

             // kcarlos - BUGBUG -> BUGCAST
             // n < (MemWinDesc[memView].iFormat == MW_BYTE)  ?
             n < (int) (MemWinDesc[memView].iFormat == MW_BYTE)  ?
                    ((fAscii == TRUE) ?
                        ((int)(MemWinDesc[memView].cMi)-1)
                        : ((int)(MemWinDesc[memView].cMi / 2)))
                    : ((int)(MemWinDesc[memView].cMi)-1);

             n++)

        {
            if (((int)(MemWinDesc[memView].lpMi[n].iStart) <= startX) &&
                            (startX < MemWinDesc[memView].lpMi[n+1].iStart)) {
                break;
            }
        }

        n += 1;

        // kcarlos
        // if ( (n >= ((MemWinDesc[memView].iFormat == MW_BYTE) ?
        if ( (n >= (int) ((MemWinDesc[memView].iFormat == MW_BYTE) ?
                ((fAscii == TRUE) ? ((int)MemWinDesc[memView].cMi)
                                    : ((int)(MemWinDesc[memView].cMi / 2) + 1))
                : (int)MemWinDesc[memView].cMi)
             )
              && ((startY + 1) < Docs[Views[memView].Doc].NbLines))
        {
            // user moving to next element (in next line of displayed data)
            startY += 1;
            if ((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE)) {
                n = (MemWinDesc[memView].cMi / 2) + 1;
            } else {
                n = 1;
            }

        } else if ((n >= ((MemWinDesc[memView].iFormat == MW_BYTE)
                        ? ((fAscii == TRUE)
                                ? ((int)MemWinDesc[memView].cMi)
                                : ((int)(MemWinDesc[memView].cMi / 2)))
                        : (int)MemWinDesc[memView].cMi))
                && ((startY + 1) >= Docs[Views[memView].Doc].NbLines))
        {
            n = (MemWinDesc[memView].iFormat == MW_BYTE)
                ? ((fAscii == TRUE)
                        ? MemWinDesc[memView].cMi - 1
                        : ((MemWinDesc[memView].cMi / 2)))
            //user trying to move beyond last element
                : MemWinDesc[memView].cMi - 1;
        }
        break;

      case GOTO_PREVIOUS:
        for (n = ((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE))
                    ? ((int)((MemWinDesc[memView].cMi / 2) + 1))
                    : 1;

             // kcarlos - BUGBUG -> BUGCAST
             // n < (MemWinDesc[memView].iFormat == MW_BYTE)
             n < (int) (MemWinDesc[memView].iFormat == MW_BYTE)
                    ? ((fAscii == TRUE)
                        ? ((int)MemWinDesc[memView].cMi)
                        : ((int)(MemWinDesc[memView].cMi / 2)))
                    : ((int)MemWinDesc[memView].cMi);
             n++)
        {
            if (((int)(MemWinDesc[memView].lpMi[n].iStart) <= startX) &&
                (startX < MemWinDesc[memView].lpMi[n].iStart +
                                       MemWinDesc[memView].lpMi[n].cch)) {
                break;
            }
        }
        n -= 1;

        if (n <  ((MemWinDesc[memView].iFormat == MW_BYTE)
                ? ((fAscii == TRUE)
                    ? (int)((MemWinDesc[memView].cMi / 2) + 1) : 1) : 1)) {
            if (startY == 0) {
                return;  // user trying to move to element before 1st
            }
            startY -= 1;
            n = (MemWinDesc[memView].iFormat == MW_BYTE)
                        ? ((fAscii == TRUE)
                            ? (MemWinDesc[memView].cMi - 1)
                            : ((MemWinDesc[memView].cMi / 2)))
                        : MemWinDesc[memView].cMi - 1;
        }
        break;

    }

    //Put the cursor at new location and save the text

    PosXY(memView, MemWinDesc[memView].lpMi[n].iStart, startY, FALSE);
    GetMemText ();

    return;
}                                       /* GotoField() */


/***    RecreateAsciiStrings
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      This function will recreate ASCII and Shift JIS strings
**      in the right part of 'Byte Memory Window'.
*/

VOID RecreateAsciiPart(int doc, UINT cBits, BOOL fTwoFields)
{
    UINT       cb;
    int        nbLines;
    int        nbPerLine;
    int        x;
    long       y;
    BOOL       bDBCS;
    char FAR *  lpb;
    char FAR *  lpb2;
    char FAR *  lpbData;
    int        x1, x2, x3;
    LPLINEREC  pl = NULL;
    LPBLOCKDEF pb = NULL;
    HCURSOR    hOldCursor;
    int        nHead;
    DWORD      dwRet;

    hOldCursor = SetCursor(LoadCursor((HINSTANCE)NULL, IDC_WAIT));

    // Compute number of lines for MAX_CHUNK_TOREAD k of data
    nbPerLine = MemWinDesc[memView].cPerLine;
    nbLines = ((MAX_CHUNK_TOREAD / nbPerLine) / (cBits / 8));
    nHead = fTwoFields ? nbPerLine + 1 : 1;

    // Compute number of bytes to malloc for this space
    cb = ((nbPerLine * nbLines) * (cBits / 8));

    lpbData = (PSTR) malloc( cb );

    OSDReadMemory(LppdCur->hpid,
                  LptdCur->htid,
                  &MemWinDesc[memView].addr,
                  lpbData,
                  cb,
                  &dwRet
                  );

    bDBCS = FALSE;

    for (y = 0, lpb = lpbData; y < nbLines; y++) {
        lpb2 = lpb;

        //  Now deal with the line data
        for (x = 0; x < nbPerLine; x++) {
            if (bDBCS) {
                bDBCS = FALSE;
                if (x == 0) {
                    //This means that current *lpb is the 2nd byte
                    //of a splited DBCS
                    *lpb = '.';
                }
            } else if (IsDBCSLeadByte(*lpb)) {
                bDBCS = TRUE;
            }
            else if (!((BYTE)*lpb >= (BYTE)0x20 && (BYTE)*lpb <= (BYTE)0x7E)
            &&       !IsDBCSLeadByte(*lpb))
                //not ascii and 'Hankaku Kana' displayable
            {
                *lpb = '.'; // replace with .
            }
            lpb++;
        }
        x1 = MemWinDesc[memView].lpMi[nHead].iStart;
        x2 = MemWinDesc[memView].lpMi[nHead + x - 1].iStart;
        x3 = x2;

        if (FirstLine(doc, &pl, &y, &pb)) {
            if (elLen > x2 + 1) {
                //This means that 2nd byte of DBCS has been added.
                x2++;
            }
            CloseLine(doc, &pl, y, &pb);
            //FirstLine() incremented 'y', so we have to decrement it.
            y--;
            if (DeleteBlock(doc, x1, y, x2 + 1, y)) {
                if (bDBCS) {
                    //If DBC is separated by new line, add 2nd byte.
                    x3++;
                }
                InsertBlock(doc, x1, y, x3 - x1 + 1, lpb2);
            } else {
                MessageBeep(0);
            }
        }
    }
    //  Free up used space
    free(lpbData);
    InvalidateLines(memView, 0, y-1, FALSE);
    SetCursor (hOldCursor);
}


/***    FValidateEdit
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      This function will get the current character stream from the
**      memory window and attempt to convert it a memory pattern and
**      write it back to the users memory.  If it is not convertable
**      or is not writable then the function will return FALSE.
*/

static BOOL PASCAL NEAR
FValidateEdit(
    UINT iArea,
    UINT x,
    UINT y
    )
{
    int         doc = Views[memView].Doc;
    char        rgch[30];
    char        rgb[30];
    ULONG       off;
    ADDR        addr2;
    int         xs, xe, cnt;
    ULONG       cBits;
    FMTTYPE     fmtType;
    EERADIX     uradix;
    ULONG       fTwoFields;
    ULONG       cchMax;
    EESTATUS    eeErr = EENOERROR;
    char        cMemChar[5];
    DWORD       dwRet;



    if (!DebuggeeActive() || !DebuggeeAlive()) {
        return FALSE;
    }


    if (memView == -1) {
        return FALSE;
    }

    Dbg(CPFormatEnumerate(MemWinDesc[memView].iFormat,
                          &cBits,
                          &fmtType,
                          &uradix,
                          &fTwoFields,
                          &cchMax,
                          NULL) == EENOERROR);


    switch (MemWinDesc[memView].iFormat) {
      case MW_REAL:
      case MW_REAL_LONG:
        xs = x; /* MemWinDesc[memView].lpMi[iArea].iStart; */
        xe = MemWinDesc[memView].lpMi[iArea].iStart +
                                         MemWinDesc[memView].lpMi[iArea].cch;

        if ((xs == MemWinDesc[memView].lpMi[iArea].iStart) ||
             (xs == ((MemWinDesc[memView].lpMi[iArea].iStart +
                         MemWinDesc[memView].lpMi[iArea].cch) - 4))) {
            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] != '+' && cMemChar[0] != '-' &&
                                                         cMemChar[0] != ' ') {
                if (xs == MemWinDesc[memView].lpMi[iArea].iStart) {
                    ReplaceCharInBlock (doc, xs, y, cMem[0]);
                } else {
                    ReplaceCharInBlock (doc, xs, y,
                                cMem[MemWinDesc[memView].lpMi[iArea].cch - 4]);
                }
                PosXY(memView, x, y, FALSE);
                return(FALSE);
            }
        } else if (xs == ((MemWinDesc[memView].lpMi[iArea].iStart +
                                   MemWinDesc[memView].lpMi[iArea].cch) - 5)) {
            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] != 'E' && cMemChar[0] != 'e') {
                ReplaceCharInBlock (doc, xs, y,
                                cMem[MemWinDesc[memView].lpMi[iArea].cch - 5]);
                PosXY(memView, x, y, FALSE);
                return(FALSE);
            }
        } else if (xs == (MemWinDesc[memView].lpMi[iArea].iStart + 2)) {
            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] != '.') {
                ReplaceCharInBlock (doc, xs, y, cMem[2]);
            }
        } else {
            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] < '0' || cMemChar[0] > '9') {
                ReplaceCharInBlock (doc, xs, y,
                            cMem[xs - MemWinDesc[memView].lpMi[iArea].iStart]);
                PosXY(memView, x, y, FALSE);
                return(FALSE);
            }

        }

        break;

      case MW_REAL_TEN:

        xs = x; /* MemWinDesc[memView].lpMi[iArea].iStart; */
        xe = MemWinDesc[memView].lpMi[iArea].iStart +
                                           MemWinDesc[memView].lpMi[iArea].cch;

        if ((xs == MemWinDesc[memView].lpMi[iArea].iStart) ||
                 (xs == ((MemWinDesc[memView].lpMi[iArea].iStart +
                                 MemWinDesc[memView].lpMi[iArea].cch) - 5))) {

            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] != '+' && cMemChar[0] != '-' &&
                                                          cMemChar[0] != ' ') {
                if (xs == MemWinDesc[memView].lpMi[iArea].iStart) {
                    ReplaceCharInBlock (doc, xs, y, cMem[0]);
                } else {
                    ReplaceCharInBlock (doc, xs, y,
                                cMem[MemWinDesc[memView].lpMi[iArea].cch - 5]);
                }
                PosXY(memView, x, y, FALSE);
                return(FALSE);
            }
        } else if (xs == ((MemWinDesc[memView].lpMi[iArea].iStart +
                                  MemWinDesc[memView].lpMi[iArea].cch) - 6)) {

            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] != 'E' && cMemChar[0] != 'e') {
                ReplaceCharInBlock (doc, xs, y,
                             cMem[MemWinDesc[memView].lpMi[iArea].cch - 6]);
                PosXY(memView, x, y, FALSE);
                return(FALSE);
            }
        } else if (xs == (MemWinDesc[memView].lpMi[iArea].iStart + 2)) {
            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] != '.') {
                ReplaceCharInBlock (doc, xs, y, cMem[2]);
            }
        } else {

            GetTextAtLine(Views[memView].Doc, y, xs,xs+1, cMemChar);

            if (cMemChar[0] < '0' || cMemChar[0] > '9') {
                ReplaceCharInBlock (doc, xs, y,
                           cMem[xs - MemWinDesc[memView].lpMi[iArea].iStart]);
                PosXY(memView, x, y, FALSE);
                return(FALSE);
            }
        }

    }


    //  If no changes have been made then it is a valid edit
    if (MemWinDesc[memView].fEdit == FALSE) {
        return TRUE;
    }


    //  Get the new buffer from the window/editor

    Assert(MemWinDesc[memView].lpMi[iArea].cch < sizeof(rgch));
    GetTextAtLine(doc,
                  Views[memView].Y,
                  MemWinDesc[memView].lpMi[iArea].iStart,
                  MemWinDesc[memView].lpMi[iArea].iStart +
                                     MemWinDesc[memView].lpMi[iArea].cch,
                  rgch);
    rgch[MemWinDesc[memView].lpMi[iArea].cch] = 0;

    strcpy (cMemTemp, rgch); // make a copy

    //  Convert the character buffer into a byte buffer

    Assert(RgbFmts[MemWinDesc[memView].iFormat] <= sizeof(rgb));


    if (!UnformatDataItem(rgch, rgb)) {
        xs = x; /* MemWinDesc[memView].lpMi[iArea].iStart; */
        xe = MemWinDesc[memView].lpMi[iArea].iStart +
                                         MemWinDesc[memView].lpMi[iArea].cch;

        cnt = x - MemWinDesc[memView].lpMi[iArea].iStart; //initialize
        while (xs < xe) {
            ReplaceCharInBlock (doc, xs, Views[memView].Y, cMem[cnt++]);
            xs++;
        }

        PosXY(memView, x, y, FALSE);
        return FALSE;
    } else {
        //  Write the byte buffer back into users memory space

        //  Compute the address to write the memory to

        off = (Views[memView].Y * MemWinDesc[memView].cPerLine +
               (
                (MemWinDesc[memView].iFormat == MW_BYTE) ?
                    ((fAscii == TRUE)?
                             iArea - (MemWinDesc[memView].cMi / 2)
                             : iArea
                    )
                    : iArea
               ) - 1
              ) * RgbFmts[MemWinDesc[memView].iFormat];

        addr2 = MemWinDesc[memView].addr;
        addr2.addr.off += off;

        OSDWriteMemory( LppdCur->hpid,
                        LptdCur->htid,
                        &addr2,
                        rgb,
                        RgbFmts[MemWinDesc[memView].iFormat],
                        &dwRet
                        );


        // M00BUG -- highlight the changed area

        //  If a two character format then we need to repaint the
        //  window since there are "updates"

        if (MemWinDesc[memView].iFormat == MW_BYTE) {

            Dbg(CPFormatMemory(rgch, 3, (PUCHAR) rgb, 8, fmtZeroPad| fmtInt, 16) ==
                                                                    EENOERROR);

            if (!fAscii) {
                BYTE by;
                ULONG ul;
                LPSTR lpszStop = NULL;
                char sz[3];

                memset(sz, 0, sizeof(sz));
                memcpy(sz, rgch, 2);

                ul = strtoul(sz, &lpszStop, uradix);
                Assert(0 == errno);

                ReplaceCharInBlock (doc,
                              MemWinDesc[memView].
                                lpMi[iArea + ((MemWinDesc[memView].cMi / 2))].
                                  iStart,
                              Views[memView].Y,
                              LOBYTE(LOWORD(ul)) );
               //********************************************************
               //* In this case, we have to recreate ASCII(& Shift JIS) *
               //* string. But it takes long time to recreate whole     *
               //* string of ASCII side.                                *
               //********************************************************
               if (IsDBCSCharSet(Views[memView].wCharSet)) {
                   RecreateAsciiPart(doc, cBits, TRUE);
               }

            } else {

                ReplaceCharInBlock (doc,
                              MemWinDesc[memView].
                                lpMi[iArea - ((MemWinDesc[memView].cMi / 2))].
                                  iStart,
                              Views[memView].Y,
                              rgch[0]);
                ReplaceCharInBlock (doc,
                              MemWinDesc[memView].
                                lpMi[iArea - ((MemWinDesc[memView].cMi / 2))].
                                  iStart+1,
                              Views[memView].Y,
                              rgch[1]);
                if (IsDBCSCharSet(Views[memView].wCharSet)) {
                    RecreateAsciiPart(doc, cBits, TRUE);
                }
            }
        }
        else if (MemWinDesc[memView].iFormat == MW_ASCII &&
                 IsDBCSCharSet(Views[memView].wCharSet)) {
                RecreateAsciiPart(doc, cBits, FALSE);
        }
    }

    return TRUE;
}                                       /* FValidateEdit() */


/***    InEntryArea
**
**  Synopsis:
**      word = InEntryArea(pui)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

static BOOL NEAR
InEntryArea(
    UINT * pui
    )
{
    int startX = Views[memView].X;
    int n;

    if (!DebuggeeActive() || !DebuggeeAlive()) {
        return FALSE;
    }

    if (memView == -1) {
        return FALSE;
    }

    for (n=1; n < ((int)(MemWinDesc[memView].cMi)-1); n++) {
        if (((int)(MemWinDesc[memView].lpMi[n].iStart) <= startX) &&
                            (startX < MemWinDesc[memView].lpMi[n+1].iStart)) {
            break;
        }
    }

    if (startX >= MemWinDesc[memView].lpMi[n].iStart +
                                             MemWinDesc[memView].lpMi[n].cch) {
        return FALSE;
    }

    if (startX < MemWinDesc[memView].lpMi[1].iStart) {
        return FALSE;
    }

    *pui = n;
    return TRUE;
}                                       /* InEntryArea() */

/***
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
*/

BOOL RestoreDBCS(char *pszSrc, char *pszTgt, int x, BOOL bDBCS)
{
    if (bDBCS) {
        if (x == 0) {
            //This means that current *pszSrc is the 2nd byte
            //of a splited DBCS
            pszTgt[x] = '.';
        } else {
            //This DBC is changed to '.' by CPFormatMemory().
            //So I restore it.
            pszTgt[x - 1] = *(pszSrc - 1);
            pszTgt[x]     = *pszSrc;
            pszTgt[x + 1] = '\0';
        }
        bDBCS = FALSE;
    } else if (IsDBCSLeadByte(*pszSrc)) {
        bDBCS = TRUE;
    }
    else if (IsDBCSLeadByte(*pszSrc)) {
       //'Hankaku Kana' is changed to '.' by CPFormatMemory().
        pszTgt[x]     = *pszSrc;
        pszTgt[x + 1] = '\0';
    }
    return bDBCS;
}


/***    ViewMem
**
**  Synopsis:
**      void = ViewMem(view, fVoidCache)
**
**  Entry:
**      view    - Index of document to update the information in
**
**  Returns:
**      nothing
**
**  Description:
**      This function will update the contents of the window to reflect
**      the current memory patterns.
*/

void
ViewMem(
    int view,
    BOOL fVoidCache
    )
{
    int         doc = Views[memView].Doc;
    int         n, x, y;
    char        rgch[MAX_MSG_TXT];
    char        rgchT[MAX_MSG_TXT];     // Temp formatting buffer
    char        rgchT2[MAX_MSG_TXT];    // Temp formatting buffer
    RECT        rc;
    int         cbWindowX;      // Number of characters across window
    int         cAddrWidth;     // # of characters in address display
    ADDR        addr;           // Address for current display
    ADDR FAR *  lpAddr = &addr;
    RTMI        ri;
    HTI         hti = (HTI) NULL;
    PTI         pTi = (PTI)NULL;
    HTM         hTm = (HTM) NULL;
    ULONG       us;
    char FAR *  lpb;
    char FAR *  lpbData;
    UINT        cbt;
    UINT        cb;
    long        cbRead;
    ULONG       cBits;
    FMTTYPE     fmtType;
    EERADIX     uradix;
    ULONG       fTwoFields;
    ULONG       cchMax;
    int         nbLines, nbPerLine, nNewbPerLine, nPower;
    LPDOCREC    d = &Docs[doc];
    HCURSOR     hOldCursor, hWaitCursor;
    EESTATUS    eeErr = EENOERROR;
    XOSD        xosd;
    BOOL        bDBCS = FALSE;
    char        *psz;




    Assert(memView != -1);

    // Keep using GetParent(hwndClient),
    // instead of hwndFrame (could be not assigned)

    if (IsIconic(GetParent(Views[memView].hwndClient))) {
        return;
    }



    //  First clean out all of the information currently in the window


    DeleteAll(doc);


    if (!DebuggeeActive() || !DebuggeeAlive()) {
        return;
    }


    // Set hourglass cursor
    hWaitCursor = LoadCursor ((HINSTANCE)NULL, IDC_WAIT);
    hOldCursor = SetCursor (hWaitCursor);


    //  Get the size of the memory window for sizing
    GetClientRect(Views[memView].hwndClient, &rc);
    cbWindowX = Pix2Pos(memView, rc.right, 0) - 1;

    if (cbWindowX > MAX_USER_LINE) {
        cbWindowX = (MAX_USER_LINE -1);
    }

    //  Get the format information
    Dbg(CPFormatEnumerate(MemWinDesc[memView].iFormat,
                          &cBits,
                          &fmtType,
                          &uradix,
                          &fTwoFields,
                          &cchMax,
                          NULL) == EENOERROR);


    //  If necessary compute the address of the expression

    if (!(MemWinDesc[memView].fLive) && (MemWinDesc[memView].fHaveAddr)) {

        addr = MemWinDesc[memView].addr;

    } else {

        GetAtomName(MemWinDesc[memView].atmAddress, rgch, sizeof(rgch));

        if ((EEParse(rgch, radix, fCaseSensitive, &hTm, &us) != EENOERROR) ||
            (EEBindTM(&hTm, SHpCXTFrompCXF( &CxfIp ), TRUE, FALSE) !=
                                                                 EENOERROR) ||
            (EEvaluateTM(&hTm, SHhFrameFrompCXF( &CxfIp ), EEHORIZONTAL) !=
                                                                 EENOERROR)) {

            LoadString (g_hInst,ERR_Memory_Context,cDoc,MAX_MSG_TXT);
            cbt = strlen (cDoc);
            InsertBlock(doc, 0, 0, cbt, cDoc);

            InvalidateRect(Views[memView].hwndClient, (LPRECT)NULL, TRUE);
            SendMessage(Views[memView].hwndClient, WM_PAINT, 0, 0L);
            return;
        }

        memset( &ri, 0, sizeof(ri));
        ri.fAddr = TRUE;


        eeErr = EEInfoFromTM(&hTm, &ri, &hti);

        // Extract the desired information

        if (eeErr == EENOERROR) {

            if ((hti == 0) || (pTi = (PTI) MMLpvLockMb( hti )) == 0) {

                return;

            } else {

                if (pTi->fResponse.fAddr) {
                    *lpAddr = pTi->AI;
                } else if (pTi->fResponse.fValue &&
                           pTi->fResponse.fSzBytes &&
                           pTi->cbValue >= sizeof(WORD)) {

                    switch( pTi->cbValue ) {
                      case sizeof(WORD):
                        SetAddrOff( lpAddr, *((WORD *) pTi->Value));
                        break;

                     case sizeof(DWORD):
                        SE_SetAddrOff( lpAddr, *((DWORD *) pTi->Value));
                        break;

                     case sizeof(DWORDLONG):
                        SetAddrOff( lpAddr, *((DWORDLONG *) pTi->Value));
                        break;
                    }

                    // set the segment

                    if ((pTi->SegType & EEDATA) == EEDATA) {
                        ADDR addrData = {0};

                        OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrData,
                                                                   &addrData );
                        SetAddrSeg( lpAddr, (SEGMENT) GetAddrSeg( addrData ));
                        ADDR_IS_FLAT(*lpAddr) = ADDR_IS_FLAT(addrData);
                        SYUnFixupAddr ( lpAddr );

                    } else if ((pTi->SegType & EECODE) == EECODE) {

                        ADDR addrPC = {0};

                        OSDGetAddr(LppdCur->hpid,LptdCur->htid, adrPC, &addrPC);
                        SetAddrSeg( lpAddr, (SEGMENT) GetAddrSeg( addrPC ));
                        ADDR_IS_FLAT(*lpAddr) = ADDR_IS_FLAT(addrPC);
                        SYUnFixupAddr( lpAddr );

                    } else {

                        ADDR addrData = {0};

                        OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrData,
                                                                   &addrData );
                        SetAddrSeg( lpAddr, (SEGMENT) GetAddrSeg( addrData ));
                        ADDR_IS_FLAT(*lpAddr) = ADDR_IS_FLAT(addrData);
                        SYUnFixupAddr ( lpAddr );
                    }
                }
                MMbUnlockMb( hti );
            }
        }


        //  Free up any handles
        if (hTm) {
            EEFreeTM (&hTm);
        }
        if (hti) {
            EEFreeTI (&hti);
        }

        SYFixupAddr(&addr);

        MemWinDesc[memView].addr = addr;

        MemWinDesc[memView].fHaveAddr = TRUE;

        // save orig address
        _fmemcpy (&MemWinDesc[memView].orig_addr, &addr, sizeof(ADDR));
    }


    //  Now compute how many memory locations will fit on one line
    EEFormatAddress(&MemWinDesc[memView].addr, rgchT, 20,
                    (g_contWorkspace_WkSp.m_bShowSegVal != FALSE) ? EEFMT_SEG : 0);
                    cAddrWidth = strlen(rgchT);


    if (fmtType != fmtAscii) {
        nbPerLine = (int)((cbWindowX - cAddrWidth) /
                                 (cchMax + 1 + (fTwoFields ? 1 : 0)));
    } else {
        nbPerLine = (int)((cbWindowX - cAddrWidth) / cchMax);
    }


    if (nbPerLine < 1) {
        //  Force at least one memory item to be displayed on the line
        return;
    }


    if (MemWinDesc[memView].fFill == FALSE) {
        // now we hold the number of items per line to a power of 2

        nNewbPerLine = nbPerLine / 2; // normalize and round

        nPower = 1;

        while (nNewbPerLine >= 1) {
            nNewbPerLine /= 2;
            nPower++;
        }

        nbPerLine = (int) pow (2.0, ((double)nPower - 1.0));

    }

    MemWinDesc[memView].cPerLine = nbPerLine;

    //  Now read in the new memory space
    Assert((cBits % 8) == 0);   // Only display evenly divisable items

    // Compute number of lines for MAX_CHUNK_TOREAD k of data
    nbLines = ((MAX_CHUNK_TOREAD / nbPerLine) / (cBits / 8));

    if ((nbPerLine * nbLines) < MAX_CHUNK_TOREAD) {
        nbLines++;
    }

    // Compute number of bytes to malloc for this space
    cb = ((nbPerLine * nbLines) * (cBits / 8));
    lpbData = (PSTR) malloc( cb );

    xosd = OSDReadMemory( LppdCur->hpid,
                          LptdCur->htid,
                          &addr,
                          lpbData,
                          cb,
                          (PULONG) &cbRead
                          );

    if (xosd == xosdNone && cbRead > 0) {
        MemWinDesc[memView].cbRead = cbRead; // save for VK_PRIOR;
        MemWinDesc[memView].fBadRead = FALSE;
    } else {
        MemWinDesc[memView].cbRead = cb;
        MemWinDesc[memView].fBadRead = TRUE;
    }

    //  If necessary clean up the formatted area and re-compute
    if (MemWinDesc[memView].lpMi)
        free(MemWinDesc[memView].lpMi);
    MemWinDesc[memView].cMi = nbPerLine + 1;

    if (MemWinDesc[memView].iFormat == MW_BYTE) {
        MemWinDesc[memView].cMi += nbPerLine;
    }

    MemWinDesc[memView].lpMi = (struct memItem *)
                 malloc( sizeof(struct memItem) * MemWinDesc[memView].cMi);

    /*
    **  Now, we know exactly the layout, display memory
    */

    if (fVoidCache) {
        DeleteAll (doc); // void the window first
    }

    //Initialize
    bDBCS = FALSE;
    for (y=0, lpb = lpbData; y < nbLines; y++) {
        x = 0;

        //  Place the current address out
        EEFormatAddress(&addr, rgchT, 20,
                (g_contWorkspace_WkSp.m_bShowSegVal != FALSE) ? EEFMT_SEG : 0);

        cb = strlen(rgchT);

        strcpy (cDoc,rgchT);

        if (y == 0) {
            MemWinDesc[memView].lpMi[0].iStart = (char)x;
            MemWinDesc[memView].lpMi[0].cch = (char) cb;
            MemWinDesc[memView].lpMi[0].iFmt = -1;
        }

        x += cb;

        if (fmtType == fmtAscii) {
            strcat (cDoc," ");
            x += 1;
        }
        psz = cDoc + x;


        //  Now deal with the line data
        for (n=0; n<nbPerLine; n++) {

            //  Format the data


            if ((cbRead >= (long)RgbFmts[MemWinDesc[memView].iFormat]) &&
                                    (MemWinDesc[memView].fBadRead == FALSE)) {
                Dbg(CPFormatMemory(rgchT, cchMax + 1, (PUCHAR) lpb, cBits,
                                     fmtType|fmtZeroPad, uradix) == EENOERROR);

                if (fTwoFields) {
                    Dbg(CPFormatMemory(&rgchT2[n], 2, (PUCHAR) lpb, 8, fmtAscii, 10) ==
                                                                     EENOERROR);
                    if (IsDBCSCharSet(Views[memView].wCharSet)) {
                        bDBCS = RestoreDBCS(lpb, rgchT2, n, bDBCS);
                    }
                }
              else if (MemWinDesc[memView].iFormat == MW_ASCII &&
                       IsDBCSCharSet(Views[memView].wCharSet)) {
                  psz[n] = rgchT[0];
                  psz[n+1] = '\0';
                  bDBCS = RestoreDBCS(lpb, psz, n, bDBCS);
                  rgchT[0] = psz[n];
                  psz[n] = '\0';
              }

              cb = strlen( rgchT );
            } else {
                memset(rgchT, '?', cchMax);
                rgchT[cchMax] = '\0';
                cb = cchMax;

                if (fTwoFields) {
                    rgchT2[n] = '.';
                    rgchT2[n+1] = '\0';
                }

            }

            if (cb < cchMax) {
                strncat (cDoc," ",(cchMax-cb));
                x += cchMax - cb;
            }

            if (fmtType != fmtAscii) {
                strcat (cDoc," ");
                x++;
            }

            strcat (cDoc,rgchT);

            if (y == 0) {
                MemWinDesc[memView].lpMi[1+n].iStart = (char)x;
                MemWinDesc[memView].lpMi[1+n].cch = (char) cb;
                MemWinDesc[memView].lpMi[1+n].iFmt = (char) fmtType;
            }

            //  Update the loop variables
            x += cb;
            lpb += RgbFmts[MemWinDesc[memView].iFormat];
            addr.addr.off += RgbFmts[MemWinDesc[memView].iFormat];
            cbRead -= (long)(min((UINT)RgbFmts[MemWinDesc[memView].iFormat],
                                 (UINT)cbRead));
        }

        if (fTwoFields) {
          if (bDBCS && IsDBCSCharSet(Views[memView].wCharSet)) {
             //If DBC is separated by new line, add 2nd byte.
             //This DBC is changed to '.' by CPFormatMemory().
             //So I restore it.
             rgchT2[n - 1] = *(lpb - 1);
             rgchT2[n]     = *lpb;
             rgchT2[n + 1] = '\0';
          }
            strcat (cDoc," ");
            x++;

            strcat (cDoc,rgchT2);

            if (y == 0) {
                for (n=0; n<nbPerLine; n++) {
                    MemWinDesc[memView].lpMi[1+n+nbPerLine].iStart =
                                                                   (char)(x+n);
                    MemWinDesc[memView].lpMi[1+n+nbPerLine].cch = 1;
                    MemWinDesc[memView].lpMi[1+n+nbPerLine].iFmt =
                                             (char)MemWinDesc[memView].iFormat;
                }
            }

            x += n;
        }
         else if (MemWinDesc[memView].iFormat == MW_ASCII &&
                  IsDBCSCharSet(Views[memView].wCharSet)) {
             if (bDBCS) {
                 //If DBC is separated by new line, add 2nd byte.
                 //This DBC is changed to '.' by CPFormatMemory().
                 //So I restore it.
                 psz[n-1] = *(lpb - 1);
                 psz[n  ] = *lpb;
                 psz[n+1] = '\0';
             }
         }


        //  Now add the CR/LF at the end of all lines except last line
        if (y < (nbLines - 1)) {
            strcat (cDoc,"\r\n");
        }
        cbt = strlen (cDoc);
        InsertBlock(doc, 0, y, cbt, cDoc);

    }

    // Set original cursor
    hOldCursor = SetCursor (hOldCursor);

    //Now everything is ready, so refresh the screen.
    //PosXY(memView, MemWinDesc[memView].lpMi[1].iStart, 0, FALSE);

    PosXY(memView, Views[memView].X, Views[memView].Y, FALSE);
    InvalidateLines(memView, 0, LAST_LINE, FALSE);
    EnsureScrollBars(memView,TRUE);
    SetVerticalScrollBar(memView, FALSE);

    // update address in struct
    _fmemcpy (&MemWinDesc[memView].old_addr, &addr, sizeof (ADDR));

    //  Free up used space
    free(lpbData);

    GetMemText ();

    return;
}                                       /* ViewMem() */


/***    CheckByteFields
**
**  Synopsis:
**      void CheckByteFields (void)
**
**  Entry:
**
**  Returns:
**
**  Description:
**              Makes sure of which field the caret is in when MW_BYTE
**
*/


static void NEAR
CheckByteFields (
    void
    )
{
    UINT        iArea2;
    BOOL        fInEntry;


    if (!DebuggeeActive() || !DebuggeeAlive() || (memView == -1)) {
        return;
    }

    if (MemWinDesc[memView].iFormat == MW_BYTE) {
        if ((fInEntry = InEntryArea(&iArea2)) == FALSE) {
            while ((fInEntry = InEntryArea(&iArea2)) == FALSE) {
                if (Views[memView].X > MemWinDesc[memView].lpMi[1].iStart) {
                    PosXY(memView, (Views[memView].X)-1, Views[memView].Y,
                                                                         FALSE);
                    GetMemText ();
                } else {
                    GotoField  ((WORD) GOTO_NEXT);
                    break;
                }
            }
        }

        fInEntry = InEntryArea(&iArea2);

        if ((fAscii == TRUE) && (iArea2 < (MemWinDesc[memView].cMi / 2) + 1)) {
            fAscii = FALSE;
        } else if ((fAscii == FALSE) &&
                     (iArea2 > (MemWinDesc[memView].cMi / 2))) {
            fAscii = TRUE;
        }
    }
}

/***    GetMemText
**
**  Synopsis:
**      void GetMemText (void)
**
**  Entry:
**
**  Returns:
**
**  Description:
**              Gets text of memory item
**
*/


static void NEAR
GetMemText (
    void
    )
{
    UINT        iArea2;
    BOOL        fInEntry;
    int memY =  Views[memView].Y;


    if (!DebuggeeActive() || !DebuggeeAlive() || (memView == -1)) {
        return;
    }

    fInEntry = InEntryArea(&iArea2); //got the area

    if (fInEntry) {
        GetTextAtLine(Views[memView].Doc,
                      memY,
                      MemWinDesc[memView].lpMi[iArea2].iStart,
                      MemWinDesc[memView].lpMi[iArea2].iStart +
                                        MemWinDesc[memView].lpMi[iArea2].cch,
                      cMem);
    }

}


LRESULT
WINAPI
MemoryEditProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:


Arguments:

    hwnd    - window handle to CPU window

    message - message to be processed

    wParam  - info about message

    lParam  - info about message

Return Value:


--*/
{
    UINT        iArea;
    UINT        x, y;
    BOOL        fInEntry = FALSE;
    SHORT       isShiftDown;
    SHORT       isCtrlDown;
    static  BOOL    bOldImeStatus;


    switch (message) {

    case WU_INITDEBUGWIN:

        // WARNING : lParam is NOT a pointer to a Valid CREATESTRUCT,
        // but holds the view number.

        Assert(lParam >= 0 && lParam < MAX_VIEWS);


        //      Set Doc to forced overtype

        Docs[Views[(WORD)lParam].Doc].forcedOvertype = TRUE;

        //      Set cursor on First mem item

        _fmemcpy (&MemWinDesc[(WORD)lParam],
                  &TempMemWinDesc,
                  sizeof(struct memWinDesc));

        PostMessage(hwnd, WM_KEYDOWN, VK_HOME, 0L);

        return FALSE; //Never call original proc or you are Dead...

    case WM_KEYDOWN:


        if (!DebuggeeActive() ||
             !DebuggeeAlive() ||
             !(MemWinDesc[memView].fHaveAddr)) {

            //  First clean out all of the information currently in the window

            InvalidateRect(hwnd, (LPRECT)NULL, TRUE);
            SendMessage(hwnd, WM_PAINT, 0, 0L);
            return CallWindowProc(lpfnEditProc, hwnd, message, wParam, lParam);
        }

        isShiftDown = (GetKeyState(VK_SHIFT) < 0);
        isCtrlDown = (GetKeyState(VK_CONTROL) < 0);


        if ((memView != -1) && (MemWinDesc[memView].fHaveAddr)) {
            GetMemText ();

            if (!isShiftDown) {
                CheckByteFields ();
                fInEntry = InEntryArea(&iArea);
            }

            switch (wParam) {
              case VK_F6:
                if (MemWinDesc[memView].iFormat == MW_BYTE) {
                    if (fAscii == FALSE) {
                        fAscii = TRUE;
                    } else {
                        fAscii = FALSE;
                    }
                    GotoField(GOTO_FIRSTONLINE);
                }
                return FALSE;


              case VK_NEXT:

                    //if we had a bad read, return now-no mem movement allowed
                if (MemWinDesc[memView].fBadRead == TRUE) {
                    return FALSE;
                }


                if (hwnd == Views[memView].hwndClient) {
                    MemWinDesc[memView].addr.addr.seg =
                                         MemWinDesc[memView].old_addr.addr.seg;
                    // set for NEXT
                    MemWinDesc[memView].addr.addr.off =
                                         MemWinDesc[memView].old_addr.addr.off;
                    ViewMem(memView, FALSE);
                }

                InvalidateRect(hwnd, (LPRECT)NULL, TRUE);
                return FALSE;


              case VK_PRIOR:

                    //if we had a bad read, return now-no mem movement allowed
                if (MemWinDesc[memView].fBadRead == TRUE) {
                    return FALSE;
                }

                if (hwnd == Views[memView].hwndClient) {
                    if ((MemWinDesc[memView].addr.addr.off -
                                                MemWinDesc[memView].cbRead)
                                 >= MemWinDesc[memView].orig_addr.addr.off)
                    {
                        MemWinDesc[memView].addr.addr.off -=
                                                    MemWinDesc[memView].cbRead;
                        ViewMem(memView, FALSE);
                    }

                }

                InvalidateRect(hwnd, (LPRECT)NULL, TRUE);
                return FALSE;


              case VK_RETURN:
              case VK_TAB:
                if (fInEntry) {
                    GotoField  ((WORD)((GetKeyState (VK_SHIFT) >= 0) ?
                                                 GOTO_NEXT : GOTO_PREVIOUS));
                }
                return FALSE;


              case VK_BACK:
                if (fInEntry &&
                     (Views[memView].X > MemWinDesc[memView].lpMi[1].iStart)) {
                    PosXY(memView,
                          (Views[memView].X)-1,
                          Views[memView].Y, FALSE);
                    GetMemText ();
                } else if (fInEntry &&
                     (Views[memView].X <= MemWinDesc[memView].lpMi[1].iStart)) {
                    if (Views[memView].Y <= 0) {
                        PosXY(memView,
                              (MemWinDesc[memView].lpMi[1].iStart),
                              (Views[memView].Y),
                              FALSE);
                        GetMemText ();
                    } else {

                        PosXY(memView,
                             (MemWinDesc[memView].iFormat == MW_BYTE) ?
                                 (fAscii == TRUE) ?
                                     ((MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi].iStart) + MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi].cch)
                                     : ((MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi / 2].iStart) + MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi / 2].cch)
                                 : ((MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi - 1].iStart) + MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi - 1].cch),
                            (Views[memView].Y) - 1,
                            FALSE);

                        GetMemText ();
                    }
                }

                if ((fInEntry = InEntryArea(&iArea)) == FALSE) {
                    while ((fInEntry = InEntryArea(&iArea)) == FALSE) {
                        if (Views[memView].X >
                                         MemWinDesc[memView].lpMi[1].iStart) {
                            PosXY(memView,
                                  (Views[memView].X)-1,
                                  Views[memView].Y,
                                  FALSE);
                            GetMemText ();
                        } else {
                            GotoField  ((WORD) GOTO_NEXT);
                            break;
                        }
                    }
                }

                return FALSE;



            case VK_END:
                if (fInEntry) {
                    if (GetKeyState(VK_CONTROL) < 0) {
                        GotoField(GOTO_LAST);
                    } else {
                        GotoField(GOTO_LASTONLINE);
                    }
                }
                return FALSE;

            case VK_HOME:
                if (!fInEntry) {
                    GotoField(GOTO_FIRST);
                } else if (GetKeyState(VK_CONTROL) < 0) {
                    GotoField(GOTO_FIRST);
                } else {
                    GotoField(GOTO_FIRSTONLINE);
                }
                return FALSE;

            case VK_RIGHT:
                if ((wParam == VK_RIGHT) && isShiftDown) {
                    KeyDown((WORD) GetWindowWord(hwnd, GWW_VIEW), wParam,
                                                isShiftDown, isCtrlDown);
                    return(FALSE);
                }

                if (fInEntry) {
                    PosXY(memView,
                          (Views[memView].X)+1,
                          Views[memView].Y,
                          FALSE);
                    GetMemText ();
                }

                fInEntry = InEntryArea(&iArea);

                if (!fInEntry) {
                    ClearSelection(memView);
                    GotoField  ((WORD) GOTO_NEXT);
                }

                return FALSE;


            case VK_LEFT:
                if (isShiftDown) {
                    KeyDown((WORD) GetWindowWord(hwnd, GWW_VIEW), wParam,
                                                      isShiftDown, isCtrlDown);
                    return(FALSE);
                }


                if (fInEntry &&
                    (Views[memView].X >
                          (((MemWinDesc[memView].iFormat == MW_BYTE) &&
                                                          (fAscii == TRUE)) ?
                               MemWinDesc[memView].lpMi[(MemWinDesc[memView].cMi / 2) + 1].iStart
                               : MemWinDesc[memView].lpMi[1].iStart)))
                {
                    PosXY(memView,
                          (Views[memView].X)-1,
                          Views[memView].Y,
                          FALSE);
                    GetMemText ();
                } else {
                    if (fInEntry && (Views[memView].X <= (((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE)) ? MemWinDesc[memView].lpMi[(MemWinDesc[memView].cMi / 2) + 1].iStart : MemWinDesc[memView].lpMi[1].iStart)))
                    {
                        if (Views[memView].Y > 0) {
                            PosXY(memView, (MemWinDesc[memView].iFormat == MW_BYTE)
                                    ? (fAscii == TRUE)
                                        ? ((MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi - 1].iStart) + MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi - 1].cch)
                                        : ((MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi / 2].iStart) + MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi / 2].cch)
                                    : ((MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi - 1].iStart) + MemWinDesc[memView].lpMi[MemWinDesc[memView].cMi - 1].cch)
                                            , (Views[memView].Y) - 1, FALSE);

                            GetMemText ();

                        } else {

                            PosXY(memView,
                                  (((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE)) ? MemWinDesc[memView].lpMi[(MemWinDesc[memView].cMi / 2) + 1].iStart : MemWinDesc[memView].lpMi[1].iStart),
                                  (Views[memView].Y),
                                  FALSE);

                            GetMemText ();
                        }
                    }
                }

                if ((fInEntry = InEntryArea(&iArea)) == FALSE) {
                    ClearSelection(memView);
                    while ((fInEntry = InEntryArea(&iArea)) == FALSE) {

                        if (Views[memView].X > (((MemWinDesc[memView].iFormat == MW_BYTE) && (fAscii == TRUE)) ? MemWinDesc[memView].lpMi[(MemWinDesc[memView].cMi / 2) + 1].iStart : MemWinDesc[memView].lpMi[1].iStart))
                        {
                            PosXY(memView, (Views[memView].X)-1, Views[memView].Y, FALSE);
                            GetMemText ();

                        } else {
                            GotoField  ((WORD) GOTO_NEXT);
                            break;
                        }
                    }
                }
                return FALSE;

            case VK_DELETE:
            case VK_INSERT:
                return (FALSE);

            }
        }
        break;

      case WM_IME_REPORT:
         return TRUE;
         break;

      case WM_SETFOCUS:
         bOldImeStatus = ImeWINNLSEnableIME(NULL, FALSE);
         break;

      case WM_KILLFOCUS:
         ImeWINNLSEnableIME(NULL, bOldImeStatus);
         break;

    case WM_CHAR:

        if (!DebuggeeActive() ||
                 !DebuggeeAlive() ||
                 !(MemWinDesc[memView].fHaveAddr)) {
            return FALSE;
        }

        //if we had a bad read, return now-no edits allowed
        if (MemWinDesc[memView].fBadRead == TRUE) {
            return FALSE;
        }

        CheckByteFields ();

        if (Views[memView].X < MemWinDesc[memView].lpMi[1].iStart) {
            // keep user out of mem address display
            return FALSE;
        }


        switch (wParam) {
          //case VK_F6:
          case VK_RETURN:
          case VK_TAB:
          case VK_BACK:
            return FALSE;
        case VK_SPACE:
            if (!fAscii) {
                return FALSE;
            }
            break;
          default:
            break;
        }

        // See if we are at a cursor position where entry is allowed

        if (InEntryArea(&iArea)) {
            //  Now perform validation on the character about to be replaced
            //  based upon what is present and type of field

            x = Views[memView].X;
            y = Views[memView].Y;

            Docs[Views[memView].Doc].ismodified = FALSE;

            CallWindowProc(lpfnEditProc, hwnd, message, wParam, lParam);

            if (Docs[Views[memView].Doc].ismodified) {
                MemWinDesc[memView].fEdit = TRUE;
            }

            if (FValidateEdit(iArea, x, y)) {
/////           if (MemWinDesc[memView].iFormat != MW_ASCII) {
                    if (!InEntryArea(&iArea)) {
                        GotoField  ((WORD) GOTO_NEXT);
                    }
/////           }
            }

        }
        return FALSE;


    case WM_LBUTTONDBLCLK:
        CallWindowProc(lpfnEditProc, hwnd, message, wParam, lParam);

        fInEntry = InEntryArea(&iArea);

        if (fInEntry) {
            BOOL        lookAround = FALSE;
            char        memField[MAX_MSG_TXT];

            *memField = '\0';

            if (GetSelectedText (curView, &lookAround, (LPSTR)&memField,
                                                         MAX_MSG_TXT, 0, 0)) {
                // hmm.  there's nothing here...
            }

        }
        return FALSE;

    case WM_FONTCHANGE:

        if (!DebuggeeActive() || !DebuggeeAlive()) {
            return FALSE;
        }
        ViewMem(memView, TRUE);

        return FALSE;

    case WM_SIZE:
        if (wParam != SIZEICONIC) {
            if (!DebuggeeActive() || !DebuggeeAlive()) {
                return FALSE;
            }

            if ((hwnd == Views[memView].hwndClient) &&
                                                    (InMemUpdate == STARTED)) {
                InMemUpdate = INPROGRESS;
                ViewMem(memView, TRUE);
            }

            InvalidateRect(hwnd, (LPRECT)NULL, TRUE);
        }
        WindowTitle( memView, 0 );
        return FALSE;

    case WM_DESTROY:
        MemWinDesc[memView].fHaveAddr = FALSE;

        //Destroy the instance of this window proc
        FreeProcInstance((FARPROC)SetWindowLongPtr(hwnd,
                                                   GWLP_WNDPROC,
                                                   (DWORD_PTR)lpfnEditProc));
        break;
    }

    return CallWindowProc(lpfnEditProc, hwnd, message, wParam, lParam);
}                                       /* MemoryEditProc() */



BOOL IsDBCSCharSet ( DWORD cs )
/* cs is charset to check for dbcs-ness */
{
    switch(cs) {
        case SHIFTJIS_CHARSET:
        case HANGEUL_CHARSET:
        case GB2312_CHARSET:
        case CHINESEBIG5_CHARSET:
        case JOHAB_CHARSET:
            return TRUE;
        default:
            return FALSE;
    }
}
