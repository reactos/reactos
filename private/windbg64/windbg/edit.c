/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    edit.c

Abstract:

    This file contains the routines which deal with the edit manager.

Author:

    Griffith Kadnier (v-griffk)

Environment:

    Win32 - User

Notes:

    This file also contains edit2.c

--*/


#include "precomp.h"
#pragma hdrstop


//Prototypes

int  NEAR PASCAL mini (int a, int b);
int  NEAR PASCAL maxi (int a, int b);
int  NEAR PASCAL Pos2Pix (int view, int x, long y);
int  NEAR PASCAL AdjustPosX (int x, BOOL goingRight);
VOID NEAR PASCAL CSyntax (WORD curWinColor, WORD status, WORD prevStatus);
BOOL NEAR PASCAL MarkStatusLine (int doc, WORD status, WORD prevStatus);
void NEAR PASCAL SelectLine (int view, int firstX, int lastX);
void NEAR PASCAL MarkSelectedLine (int view, int line);
void NEAR PASCAL PatchTabs (int view);
void NEAR PASCAL WriteLine (int view, int line, HDC hDC, int X, int Y, PRECT pRc, WORD prevStatus);
void NEAR PASCAL SelPosXY (int view, int X, int Y);
void NEAR PASCAL MoveRightWord (int view, BOOL Sel, int posX, int posY);
void NEAR PASCAL SortBlock (int view);
void NEAR PASCAL FindMatching (int view);



/***

a-seanl 3/1/94

            Notes On Color Coding For Pre-Processor Instructions.

We presume that the source file has successfully compiled (or else the
user couldn't even get this far).  Therefore, we can be casual in our
search for pre-processor directives.  Specifically, we assume that if the
first non-whitespace character on a line is '#', then we have a
pre-processor line, and that all words after that are either user
identifiers or pre-processor directives.

If the line begins with '#if', then 'defined' becomes a reserved word.

If the line begins with '#pragma', then all of the complier pragmas
become reserved words.

Otherwise, our handling of color information follows the same steps as the
original color-coding procedure, except that we must select which set
of reserved words to search.

Note that this all happens down in the body of CSyntax().

Note also that we now need to watch carefully for continuation lines.

In fact, this is going to get so hairy that I don't think it needs to
done yet.  There are lots of other brush fires that are considerably,
hotter, eh?

***/


//Editor : Table of C 2.00 Keywords-MUST remain in sorted order!!!
//
// [April 1994]
// Note: The second group is a single-underbar version of the first group.  The
//       compiler (v7.0) uses the double underbar for ANSI's sake, but allows the
//       single underbar for backwards compatibility.  The only discrepancy is the
//       '__declspec' keywaord, and that appears to nothave a single-underbar version.
//
//       We include the Win32 extended syntax keywords:
//
//            dllexport, dllimport, naked, thread
//
// [May 1994]
// Note: The list is now current with VC++ 2.0 sources, courtesy of richards (Rich Shupak).
//       They do something with '_declspec', so now we will, too.
//

char *(CKeywords[]) = {

        "__asm",
        "__based",
        "__cdecl",
        "__declspec",
        "__emit",
        "__except",
        "__export",
        "__far",
        "__fastcall",
        "__finally",
        "__fortran",
        "__huge",
        "__inline",
        "__int16",
        "__int32",
        "__int64",
        "__int8",
        "__interrupt",
        "__leave",
        "__loadds",
        "__multiple_inheritance",
        "__nctag",
        "__near",
        "__nounwind",
        "__novtordisp",
        "__pascal",
        "__resume",
        "__saveregs",
        "__segment",
        "__segname",
        "__self",
        "__single_inheritance",
        "__stdcall",
        "__try",
        "__virtual_inheritance",


        "_asm",
        "_based",
        "_cdecl",
        "_declspec",
        "_emit",
        "_except",
        "_export",
        "_far",
        "_fastcall",
        "_finally",
        "_fortran",
        "_huge",
        "_inline",
        "_int16",
        "_int32",
        "_int64",
        "_int8",
        "_interrupt",
        "_leave",
        "_loadds",
        "_multiple_inheritance",
        "_nctag",
        "_near",
        "_nounwind",
        "_novtordisp",
        "_pascal",
        "_resume",
        "_saveregs",
        "_segment",
        "_segname",
        "_self",
        "_single_inheritance",
        "_stdcall",
        "_try",


        "_virtual_inheritance",


        "auto",
        "break",
        "case",
        "catch",
        "cdecl",
        "char",
        "class",
        "const",
        "continue",
        "default",
        "delete",
        "dllexport",
        "dllimport",
        "do",
        "double",
        "else",
        "enum",
        "extern",
        "far",
        "float",
        "for",
        "fortran",
        "friend",
        "goto",
        "huge",
        "if",
        "inline",
        "int",
        "interrupt",
        "long",
        "naked",
        "near",
        "new",
        "operator",
        "pascal",
        "private",
        "protected",
        "public",
        "register",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "struct",
        "switch",
        "template",
        "this",
        "thread",
        "throw",
        "try",
        "typedef",
        "union",
        "unsigned",
        "virtual",
        "void",
        "volatile",
        "while"

};




// colors for Textout()

#define FORECOLOR(colorRef) ( StringColors[ colorRef ].FgndColor )
#define BACKCOLOR(colorRef) ( StringColors[ colorRef ].BkgndColor )


//To make the difference between a mouse and keyboard scroll

#define FROM_MOUSE 0x0
#define FROM_KEYBOARD 0x1
#define SELECTING 0x2


int NEAR PASCAL mini (int a, int b)
{
        if (a < b)
                return a;
        else
                return b;
}

int NEAR PASCAL maxi(int a, int b)
{
        if (a > b)
                return a;
        else
                return b;
}

//Pix2Pos - convert a position in pixel to its nearest position in char
int FAR PASCAL Pix2Pos(
        int view,
        int X,
        long Y)
{
        LPLINEREC pl;
        LPBLOCKDEF pb;
        register int pix, idx;
        int pix1;
        LPVIEWREC v = &Views[view];
        int blankWidth = v->charWidth[' '];
        BOOL bDBCS = FALSE;
        SIZE SizeDBCS;
        char szDBCS[3];
        HDC  hDCTmp = 0;

        //Normalize
        if (Y < 0)
                Y = 0;

        Assert(v->Doc >= 0);
        Assert(Y < Docs[v->Doc].NbLines);

        // Get text of line
        if (!FirstLine(v->Doc, &pl, &Y, &pb))
                return 0;
        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
            Dbg(hDCTmp = GetDC(v->hwndClient));
            Dbg(SelectObject(hDCTmp, v->font));
        }

        idx = 0;
        pix = 0;
        while (pix < X) {
                bDBCS = FALSE;
                if (idx < elLen - 1 && IsDBCSLeadByte((BYTE)el[idx])) {
                        bDBCS = TRUE;

                        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
                                szDBCS[0] = el[idx];
                                szDBCS[1] = el[idx+1];
                                szDBCS[2] = '\0';
                                GetTextExtentPoint(
                                        hDCTmp, szDBCS, 2, &SizeDBCS);
                                pix += (SizeDBCS.cx - v->Tmoverhang);
                        } else {
                            pix += v->charWidthDBCS;
                        }
                        idx += 2;
                } else if (idx >= elLen) {
                        pix += blankWidth;
                        idx++;
                } else {
                        pix += (v->charWidth[(BYTE)(el[idx])] - v->Tmoverhang);
                        idx++;
                }
        }

        if (pix != X) {
                pix1 = pix;
                if (bDBCS) {
                        idx -= 2;
                        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
                                szDBCS[0] = el[idx];
                                szDBCS[1] = el[idx+1];
                                szDBCS[2] = '\0';
                                GetTextExtentPoint(
                                        hDCTmp, szDBCS, 2, &SizeDBCS);
                                pix -= (SizeDBCS.cx - v->Tmoverhang);
                        } else {
                            pix -= v->charWidthDBCS;
                        }
                } else {
                        idx--;
                        if (idx >= elLen)
                                pix -= blankWidth;
                        else
                                pix -= v->charWidth[(BYTE)(el[idx])];
                }
                if ((pix + ((pix1 - pix) >> 1)) < X) {
                        if (bDBCS)
                                idx+=2;
                        else
                                idx++;
                }
        }

        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
                Dbg(ReleaseDC (v->hwndClient, hDCTmp));
        }
        CloseLine (v->Doc, &pl, Y, &pb);
        return idx;
}

//Pos2Pix - convert a position in character to its position in pixels
int NEAR PASCAL Pos2Pix(
        int view,
        int x,
        long y)
{
        LPLINEREC pl;
        LPBLOCKDEF pb;
        LPVIEWREC v = &Views[view];
        register int i, xPixel;
        int *charWidth = Views[view].charWidth;
        SIZE SizeDBCS;
        char szDBCS[3];
        HDC  hDCTmp = 0;

        //Normalize
        Assert(v->Doc >= 0);
        Assert(y < Docs[v->Doc].NbLines);
        if (x < 0)
                x = 0;
        if (x > MAX_USER_LINE)
                x = MAX_USER_LINE;
        if (y < 0)
                y = 0;

        // Get text of line
        if (!FirstLine(v->Doc, &pl, &y, &pb))
                return 0;
        CloseLine(v->Doc, &pl, y, &pb);

        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
                Dbg(hDCTmp = GetDC(v->hwndClient));
                Dbg(SelectObject(hDCTmp, v->font));
        }
        //We do not need to take care of the position beyond line,
        //"el" is allways filled with spaces
        xPixel = 0;
        for (i = 0; i < x; i++){
            if (IsDBCSLeadByte((BYTE)el[i])) {
                        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
                                szDBCS[0] = el[i];
                                szDBCS[1] = el[i+1];
                                szDBCS[2] = '\0';
                                GetTextExtentPoint(
                                        hDCTmp, szDBCS, 2, &SizeDBCS);
                                xPixel += (SizeDBCS.cx - v->Tmoverhang);
                        } else {
                            xPixel += Views[view].charWidthDBCS;
                        }
                        i++;
                } else {
                    xPixel += (charWidth[(BYTE)el[i]] - v->Tmoverhang);
                }
        }

        if (v->wViewPitch == VIEW_PITCH_VARIABLE) {
                Dbg(ReleaseDC (v->hwndClient, hDCTmp));
        }
        return xPixel;
}



void FAR PASCAL
SetCaret(
         int view,
         int x,
         int y,
         int pixelPos
         )
/*++

Routine Description:

    This routine is used to set the caret at a specfic X, Y position.

Arguments:

    view        - Supplies the index of the view to set caret in
    x           - Supplies the X pos (character) to set caret at
    y           - Supplies the Y pos (line) to set caret at
    pixelPos    - Supplies the pixel position to set cursor at.

Return Value:

    None.

--*/
{
    LPVIEWREC   v = &Views[view];
    int         yPos = v->iYTop;

    if (pixelPos == -1) {
        pixelPos = Pos2Pix(view, x, y);
    }

    if (yPos == -1) {
        yPos = GetScrollPos(v->hwndClient, SB_VERT);
    }

    SetCaretPos(pixelPos - v->maxCharWidth * GetScrollPos(v->hwndClient, SB_HORZ),
                (int)(((long)y - (long)yPos) * (long)v->charHeight));

    ImeMoveConvertWin(v->hwndClient,
                pixelPos - v->maxCharWidth
                        * GetScrollPos(v->hwndClient, SB_HORZ),
                (int)(((long)y - (long)yPos) * (long)v->charHeight));

    return;
}                               /* SetCaret() */


void FAR PASCAL
InvalidateLines(
                int actualView,
                int FromL,
                int ToL,
                BOOL lineAdded
                )
/*++

Routine Description:

    This function is called to invalidate lines in a window.  The
    lines to be invalided are passed in.   The routine will invalidate
    all views on the same document.

Arguments:

    actualView  - Supplies the view index to be invalidated
    FromL       - Starting line to invalidate
    ToL         - Ending line to invalidate
    lineAdded   - TRUE if lines were added to the view.

Return Value:

    None.

--*/

{
    RECT        rcl;
    int         minim;
    int         maxim;
    long        k;
    int         view = Docs[Views[actualView].Doc].FirstView;
    HWND        hwnd;
    int         charHeight;
    int         yPos;

    Assert(view >= 0 && view < MAX_VIEWS);
    Assert(Views[actualView].Doc >= 0);

    while (view != -1)  {
        yPos = Views[view].iYTop;
        hwnd = Views[view].hwndClient;
        if (yPos == -1) {
            yPos = GetScrollPos(hwnd, SB_VERT);
        }
        GetClientRect(hwnd, &rcl);
        charHeight = Views[view].charHeight;

        /*
         *      Beware of integer overflow
         */

        k = (long)(FromL - yPos) * charHeight;
        if (k > 32767) {
            minim = 32767;
        } else if (k < -32767) {
            minim = -32767;
        } else {
            minim = (int)k;
        }

        if ((ToL == LAST_LINE) || lineAdded) {
            maxim = rcl.bottom;
        } else {
            k = (long)(ToL - yPos + 1) * charHeight;

            if (k > 32767) {
                maxim = 32767;
            } else if (k < -32767) {
                maxim = -32767;
            } else {
                maxim = (int)k;
            }
        }

        if (minim < rcl.bottom && maxim > rcl.top) {
            if (minim > rcl.top) {
                rcl.top = minim;
            }

            if (maxim < rcl.bottom) {
                rcl.bottom = maxim;
            }

            InvalidateRect(hwnd, &rcl, TRUE);
        }
        view = Views[view].NextView;
    }
    return;
}                               /* InvalidateLines() */

int NEAR PASCAL AdjustPosX(
        int x,
        BOOL goingRight)
{
        register int i = 0;
        register int j = 0;
        int k;
        int len = pcl->Length - LHD;

        while (i < len && j < x) {
                if (IsDBCSLeadByte((BYTE)pcl->Text[i])) {
                        if (x == j+1) {
                                if (goingRight)
                                        return j+2;
                                else
                                        return j;
                        }
                        j += 2;
                        i++;
                } else
                if (pcl->Text[i] == TAB) {
                        k = j;
                        j += g_contGlobalPreferences_WkSp.m_nTabSize - (j % g_contGlobalPreferences_WkSp.m_nTabSize);
                        if (x > k && x < j) {
                                if (goingRight)
                                        return j;
                                else
                                        return k;
                        }
                }
                else
                        j++;
                i++;
        }

        Assert(j <= MAX_USER_LINE);
        return x;
}


void FAR PASCAL
InternalPosXY(
      int view,
      int X,
      long Y,
      BOOL fDebugger,
      BOOL fCenter
      )

/*++

Routine Description:

    This function will cause the cursor to be placed at a specific
    location in a window.

Arguments:

    view        - Supplies the view index to postion in
    x           - Supplies the X (character) position
    y           - Supplies the Y (line) position
    fDebugger   - Supplies TRUE if called by the debugger placement routines

Return Value:

    None.

--*/

{
    int         firstL;
    int         firstLn;
    int         firstC,firstCol;
    int         winNbLines;
    int         XX;
    int         XC;
    RECT        rc;
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    LPLINEREC   pl;
    LPBLOCKDEF  pb;
    int         yPos;

    /*
     *  Normalize
     */

    Assert(Y <= Docs[v->Doc].NbLines - 1);
    Assert(v->Doc >= 0);

    if (X < 0) {
        X = 0;
    }
    if (X > MAX_USER_LINE) {
        X = MAX_USER_LINE;
    }
    if (Y < 0) {
        Y = 0;
    }

    /*
     *  Get text of line
     */

    if (!FirstLine(v->Doc, &pl, &Y, &pb)) {
        return;
    }
    CloseLine (v->Doc, &pl, Y, &pb);
    Y--;

    /*
     *  Convert to edit if coming from undo/redo
     */

    if (playingRecords) {
        X = ConvertPosX(X);
    }

    /*
     *  Skip tabs white space
     */

    X = AdjustPosX(X, (X >= v->X && Y == v->Y));
    v->X = X;
    v->Y = Y;

    if (view == curView) {
        SetLineColumn_StatusBar(Y + 1, X + 1);
    }

    /*
     *  See if we have to remove or put back scrollbars
     */

    EnsureScrollBars(view, FALSE);

    /*
     *  Get size of client data area.  This should wait until after scroll
     *  bars may have been added.
     */

    GetClientRect(hwnd, &rc);

    /*
     *  Take care of vertical scroll
     */

    yPos = v->iYTop;
    if (yPos == -1) {
        yPos = GetScrollPos(hwnd, SB_VERT);
    }
    firstLn = firstL = yPos;

    /*
     *  Compute nb of lines seen in window
     */

    winNbLines = rc.bottom / v->charHeight;

    if (rc.bottom % v->charHeight) {
        winNbLines++;
    }

    if (Y < firstL) {
        if (fDebugger) {
            if (winNbLines < 6) {
                firstL = Y;
            }
            else {
                firstL = Y - (winNbLines/5);
                if (firstL < 0) {
                    firstL = 0;
                }
            }
        }
        else {
            if (fCenter) {
                firstL = Y - (winNbLines / 2);
            }
            else {
                firstL = Y;
            }
        }
    }
    else
    if (Y >= (firstL + (winNbLines - 1))) {
        if (fDebugger) {
            if (winNbLines < 6) {
                firstL = Y;
            }
            else {
                firstL = Y - (winNbLines/5);
            }
        }
        else {
            if (fCenter) {
                firstL = Y - (winNbLines / 2);
            }
            else {
                firstL = Y - (winNbLines - 2);
            }
        }
    }

    /*
     *  Set scroll-bar positions before refreshing the screen.
     *  We use the scroll-bar positions during the PAINT event
     */

    if (firstL != firstLn) {
        if (v->iYTop == -1) {
            SetScrollPos(hwnd, SB_VERT, firstL, TRUE);
        } else {
            v->iYTop = firstL;
        }
        ScrollWindow(hwnd, 0, (firstLn - firstL) * v->charHeight, NULL, NULL);
                  UpdateWindow (hwnd);
    }

    /*
     *  Take care of horizontal scroll
     */

    firstCol = firstC = GetScrollPos(hwnd, SB_HORZ);
    XC = firstC * v->maxCharWidth;
    XX = Pos2Pix(view, X, Y);

    if (XX <= XC) {
        firstC = maxi((XX - (rc.right / SCROLL_RATIO)) / v->maxCharWidth, 0);
    } else {
        if (XX >= XC + rc.right) {
            firstC = (XX - rc.right + (rc.right / SCROLL_RATIO)) / v->maxCharWidth;
        }
    }

    /*
     *  Set scroll-bar positions before refreshing the screen.
     *  We use the scroll-bar positions during the PAINT event
     */

    if (firstC != firstCol) {
        SetScrollPos(hwnd, SB_HORZ, firstC, TRUE);
        ScrollWindow(hwnd, (firstCol - firstC) * v->maxCharWidth, 0, NULL, NULL);
        UpdateWindow (hwnd);
        //InvalidateRect (hwnd,NULL,TRUE);
    }

    if (view == curView) {
        SetCaret(view, X, Y, XX);
    }

    return;
}                                       /* InternalPosXY() */


void FAR PASCAL
PosXY(
      int view,
      int X,
      int Y,
      BOOL fDebugger
      )

/*++

Routine Description:

    This function will cause the cursor to be placed at a specific
    location in a window.

Arguments:

    view        - Supplies the view index to postion in
    x           - Supplies the X (character) position
    y           - Supplies the Y (line) position
    fDebugger   - Supplies TRUE if called by the debugger placement routines

Return Value:

    None.

--*/

{
    InternalPosXY( view, X, Y, fDebugger, FALSE );
}


void FAR PASCAL
PosXYCenter(
      int view,
      int X,
      int Y,
      BOOL fDebugger
      )

/*++

Routine Description:

    This function will cause the cursor to be placed at a specific
    location in a window.

Arguments:

    view        - Supplies the view index to postion in

    x           - Supplies the X (character) position

    y           - Supplies the Y (line) position

    fDebugger   - Supplies TRUE if called by the debugger placement routines

Return Value:

    None.

--*/

{
    InternalPosXY( view, X, Y, fDebugger, TRUE );
}

int
WINAPIV
Compare( //(const char **left, const char **right)
    const void* left,
    const void* right
    )
{
    return strcmp(*(LPTSTR*)(left), *(LPTSTR*)(right));
}

//Create attr table for a C source line
//This function is designed for speed, not to look nice...
VOID 
CSyntax(
    WORD curWinColor,
    WORD status,
    WORD prevStatus
    )
{
    register int i;
    ULONG fore = 0, back = 0;
    ULONG defaultFore;
    ULONG defaultBack;
    char ch, *pst;
    int start;
    BOOL wasInDefault = FALSE;
    BOOL isFrac = FALSE;


    colors[0].nbChars = 0;
    colors[0].fore = defaultFore = FORECOLOR(curWinColor);
    colors[0].back = defaultBack = BACKCOLOR(curWinColor);
    i = 0;

    if (elLen > 1 && !(status & COMMENT_LINE) && (prevStatus & COMMENT_LINE)) {
        start = 0;

        //Closing multiline comments
        while (i < elLen - 1) {
            if (IsDBCSLeadByte(el[i])) {
                i += 2;
            } else if (el[i] == '*' && el[i + 1] == '/') {
                break;
            } else {
                i++;
            }
        }
        Assert(i < elLen - 1);

        i += 2;

        colors[0].fore = FORECOLOR(Cols_Comment);
        colors[0].back = BACKCOLOR(Cols_Comment);
        colors[0].nbChars = i - start;
        nbColors = 1;
        colors[1].nbChars = 0;

    }

    while (i < elLen) {
        if (IsDBCSLeadByte(el[i])) {
            colors[nbColors].nbChars+=2;
            wasInDefault = TRUE;
            i += 2;
            continue;
        }

        start = i;
        ch = el[i++];

        if ((ch == ' ') && isFrac)
           {
            isFrac = FALSE;
           }

        if (IsCharAlpha(ch) || ch == '_' || ch == '#') {

            char savedChar;

            isFrac = FALSE;

            if (wasInDefault) {
                colors[nbColors].fore = defaultFore;
                colors[nbColors].back = defaultBack;
                nbColors++;
                wasInDefault = FALSE;
            }
            pst = el + i - 1;
            while (i < elLen && (CHARINALPHASET(el[i])))
              i++;
            savedChar = el[i];
            el[i] = 0;

            if (bsearch((char *)&pst, 
                        CKeywords, 
                        sizeof(CKeywords) / sizeof(CKeywords[0]), 
                        sizeof(CKeywords[0]), 
                        Compare
                        ) != NULL)  {

                fore = FORECOLOR(Cols_Keyword);
                back = BACKCOLOR(Cols_Keyword);
            }
            else {
                fore = FORECOLOR(Cols_Identifier);
                back = BACKCOLOR(Cols_Identifier);
            }
            el[i] = savedChar;

        }
        else
          switch (ch)     {

              //Number
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':

                if (wasInDefault) {
                    colors[nbColors].fore = defaultFore;
                    colors[nbColors].back = defaultBack;
                    nbColors++;
                    wasInDefault = FALSE;
                }

              fore = FORECOLOR(Cols_Number);
              back = BACKCOLOR(Cols_Number);

              if (i < elLen && ch == '0' && (el[i] == 'x' || el[i] == 'X')) {

                  //We have an hexadecimal number
                    while ((i < elLen) && ((el[i] >= '0' && el[i] <= '9') || (el[i] == 'x') || (el[i] == 'X') || ((el[i] >= 'a') && (el[i] <= 'f')) || ((el[i] >= 'A') && (el[i] <= 'F'))))
                      i++;
              }
              else {

                  BOOL isExponent = FALSE;

                  //We have a non hexa number
                    while ((i < elLen) && ((el[i] >= '0' && el[i] <= '9') || (!isFrac && (el[i] == '.'))
                           || el[i] == 'e' || el[i] == 'E'
                           || (isExponent && (el[i] == '-' || el[i] == '+')))) {
                        if (el[i] == 'e' || el[i] == 'E' || el[i] == '.') {

                            //Precisely, we have a real number
                              fore = FORECOLOR(Cols_Real);
                            back = BACKCOLOR(Cols_Real);

                            if (el[i] != '.')
                              isExponent = TRUE;
                               else
                                  isFrac = TRUE;
                        }
                        i++;
                    }

              }
              break;

          case '.':

              if (i < elLen && (el[i] >= '0' && el[i] <= '9') && !isFrac) {

                  BOOL isExponent = FALSE;

                  if (wasInDefault) {
                      colors[nbColors].fore = defaultFore;
                      colors[nbColors].back = defaultBack;
                      nbColors++;
                      wasInDefault = FALSE;
                  }

                  //We have a real number
                    while ((i < elLen) && ((el[i] >= '0' && el[i] <= '9')
                                         || el[i] == 'e' || el[i] == 'E'
                                         || (isExponent && (el[i] == '-' || el[i] == '+')))) {

                        if (el[i] == 'e' || el[i] == 'E')
                          isExponent = TRUE;
                        i++;
                    }

                  fore = FORECOLOR(Cols_Real);
                  back = BACKCOLOR(Cols_Real);
              }
              else

                //We have something else
                  goto defaut;

              break;

          case '"':

              if (wasInDefault) {
                  colors[nbColors].fore = defaultFore;
                  colors[nbColors].back = defaultBack;
                  nbColors++;
                  wasInDefault = FALSE;
              }

              fore = FORECOLOR(Cols_String);
              back = BACKCOLOR(Cols_String);

              //Now search for a second '"'
              {
                int iBackSlash1 = -2;
                int iBackSlash2 = -2;

                do {
                    //Handle possible double '\\' before a '"'
                    if (i < 1) {
                        break;
                    } else if (IsDBCSLeadByte(el[i])) {
                        i += 2;
                    } else {
                        if (el[i] == '\\') {
                            iBackSlash1 = iBackSlash2;
                            iBackSlash2 = i;
                        } else if (el[i] == '"') {
                            if (i != iBackSlash2 + 1
                            || (i == iBackSlash2 + 1 && i == iBackSlash1 + 2)) {
                                break;
                            }
                        }
                        i++;
                    }
                } while (i < elLen);
              }
              if (i >= elLen)
                goto done;

              i++;
              break;

          case '\'':

              if (wasInDefault) {
                  colors[nbColors].fore = defaultFore;
                  colors[nbColors].back = defaultBack;
                  nbColors++;
                  wasInDefault = FALSE;
              }


              fore = FORECOLOR(Cols_String);
              back = BACKCOLOR(Cols_String);

              //Now search for a second "'"
              {
                int iBackSlash1 = -2;
                int iBackSlash2 = -2;

                do {
                    //Handle possible double '\\' before a '"'
                    if (i < 1) {
                        break;
                    } else if (IsDBCSLeadByte(el[i])) {
                        i += 2;
                    } else {
                        if (el[i] == '\\') {
                            iBackSlash1 = iBackSlash2;
                            iBackSlash2 = i;
                        } else if (el[i] == '\'') {
                            if (i != iBackSlash2 + 1
                            || (i == iBackSlash2 + 1 && i == iBackSlash1 + 2)) {
                                break;
                            }
                        }
                        i++;
                    }
                } while (i < elLen);
              }

              if (i >= elLen)
                goto done;
              i++;
              break;

              //Opening Comments
            case '/':

                fore = FORECOLOR(Cols_Comment);
              back = BACKCOLOR(Cols_Comment);

              //Comment
                if (i < elLen && el[i] == '*') {

                    if (wasInDefault) {
                        colors[nbColors].fore = defaultFore;
                        colors[nbColors].back = defaultBack;
                        nbColors++;
                        wasInDefault = FALSE;
                    }

                    i++;

                    while (i < elLen - 1) {
                        if (IsDBCSLeadByte(el[i])) {
                            i += 2;
                        } else if (el[i] == '*' && el[i + 1] == '/') {
                            break;
                        } else {
                            i++;
                        }
                    }

                    //Begin of a multiline comment
                      if (i >= elLen - 1)
                        goto done;

                    i += 2;
                }
                else {
                    if (i < elLen && el[i] == '/') {
                        if (wasInDefault) {
                            colors[nbColors].fore = defaultFore;
                            colors[nbColors].back = defaultBack;
                            nbColors++;
                            wasInDefault = FALSE;
                        }
                        goto done;
                    }
                    else
                      goto defaut;
                }

              break;

          default:
          defaut:
              colors[nbColors].nbChars++;
              wasInDefault = TRUE;
              break;
          }

        if (!wasInDefault) {
            colors[nbColors].fore = fore;
            colors[nbColors].back = back;
            colors[nbColors].nbChars = i - start;
            nbColors++;
            colors[nbColors].nbChars = 0;
        }
    }

    colors[nbColors].fore = FORECOLOR(curWinColor);
    colors[nbColors].back = BACKCOLOR(curWinColor);
    colors[nbColors].nbChars = 2 * MAX_USER_LINE - i;
    return;

 done: {
     colors[nbColors].fore = fore;
     colors[nbColors].back = back;
     colors[nbColors].nbChars = 2 * MAX_USER_LINE - start;
 }

}


//Mark status attributes
BOOL NEAR PASCAL MarkStatusLine(
        int doc,
        WORD status,
        WORD prevStatus)
{
        ULONG fore, back;

        //Colors for tags, current debug line, breakpoint and comment lines
        if (status > 0) {



                if ((status & UBP_LINE) && (!(status & BRKPOINT_LINE)))      {
                       fore = FORECOLOR(Cols_UnInstantiatedBreakpoint);
                       back = BACKCOLOR(Cols_UnInstantiatedBreakpoint);
                       goto markAllLine;
                }

                if (status & CURRENT_LINE)      {
                   if ((status & BRKPOINT_LINE) || (status & UBP_LINE))
                      {
                       fore = FORECOLOR(Cols_CurrentBreak);
                       back = BACKCOLOR(Cols_CurrentBreak);
                      }
                      else
                         {
                          fore = FORECOLOR(Cols_CurrentLine);
                          back = BACKCOLOR(Cols_CurrentLine);
                         }
                       goto markAllLine;
                }

                if (status & BRKPOINT_LINE)     {
                   if (status & CURRENT_LINE)
                      {
                       fore = FORECOLOR(Cols_CurrentBreak);
                       back = BACKCOLOR(Cols_CurrentBreak);
                      }
                      else
                         {
                          fore = FORECOLOR(Cols_BreakpointLine);
                          back = BACKCOLOR(Cols_BreakpointLine);
                         }
                       goto markAllLine;
                }

                if (status & TAGGED_LINE)       {
                        fore = FORECOLOR(Cols_TaggedLine);
                        back = BACKCOLOR(Cols_TaggedLine);
                        goto markAllLine;
                }

                if (syntaxColors && Docs[doc].language != NO_LANGUAGE
                         && (status & COMMENT_LINE)
                         && (prevStatus & COMMENT_LINE))        {
                        fore = FORECOLOR(Cols_Comment);
                        back = BACKCOLOR(Cols_Comment);
                        goto markAllLine;
                }

        }

        return FALSE;

        markAllLine : {
                colors[nbColors].nbChars = 2 * MAX_USER_LINE;
                colors[nbColors].fore = fore;
                colors[nbColors].back = back;
                return TRUE;
        }

}

void NEAR PASCAL SelectLine(
        int view,
        int firstX,
        int lastX)
{
        int firstTot = 0, firstColSel = 0;
        int lastTot, lastColSel;
        int lastRemain;
        int colNb, inSelect;
        COLORINFO c;

        Unused(view);
        Unused(colNb);
        Unused(inSelect);
        Unused(c);

        //Handle the case where all right part of line is selected
        if (lastX == MAX_USER_LINE) {

                if (firstX == 0) {
                        colors[0].fore = FORECOLOR(Cols_Selection);
                        colors[0].back = BACKCOLOR(Cols_Selection);
                        colors[0].nbChars = 2 * MAX_USER_LINE;
                        nbColors = 0;
                        return;
                }

                //Find the position of firstX in the color array
                while (firstTot <= firstX) {
                        firstTot += colors[firstColSel].nbChars;
                        if (firstTot <= firstX)
                                firstColSel++;
                }
                Assert(firstColSel <= nbColors);

                colors[firstColSel].nbChars -= (firstTot - firstX);
                nbColors = firstColSel + 1;
                colors[nbColors].fore = FORECOLOR(Cols_Selection);
                colors[nbColors].back = BACKCOLOR(Cols_Selection);
                colors[nbColors].nbChars = 2 * MAX_USER_LINE - firstX;
                return;
        }
        else {

                //Handle the case where all left part of line is selected
                if (firstX == 0) {

                        if (lastX == 0)
                                return;

                        if (lastX == MAX_USER_LINE) {
                                colors[0].fore = FORECOLOR(Cols_Selection);
                                colors[0].back = BACKCOLOR(Cols_Selection);
                                colors[0].nbChars = 2 * MAX_USER_LINE;
                                nbColors = 0;
                                return;
                        }

                        //Find the position of lastX in the color array
                        lastTot = lastColSel = 0;
                        while (lastTot < lastX) {
                                lastTot += colors[lastColSel].nbChars;
                                if (lastTot < lastX)
                                        lastColSel++;
                        }
                        Assert(lastColSel <= nbColors);
                        lastRemain = lastTot - lastX;

                        //Insert a new color
                        if (lastRemain != 0) {
                                nbColors++;
                                memmove(&colors[1], &colors[0], nbColors * sizeof(COLORINFO));
                        }

                        //Remove all colors before lastX
                        nbColors -= lastColSel;
                        memmove(&colors[0], &colors[lastColSel], (nbColors + 1) * sizeof(COLORINFO));

                        colors[0].fore = FORECOLOR(Cols_Selection);
                        colors[0].back = BACKCOLOR(Cols_Selection);
                        colors[0].nbChars = lastX;

                        if (lastRemain == 0)
                                return;

                        colors[1].nbChars = lastRemain;

                }
                else {

                        //Find the position of firstX in the color array
                        while (firstTot <= firstX) {
                                firstTot += colors[firstColSel].nbChars;
                                if (firstTot <= firstX)
                                        firstColSel++;
                        }
                        Assert(firstColSel <= nbColors);

                        //Find the position of lastX in the color array
                        lastTot = lastColSel = 0;
                        while (lastTot < lastX) {
                                lastTot += colors[lastColSel].nbChars;
                                if (lastTot < lastX)
                                        lastColSel++;
                        }
                        Assert(lastColSel <= nbColors);
                        lastRemain = lastTot - lastX;

                        //Selection inside the same color
                        if (firstColSel == lastColSel) {

                                //Duplicate the element and leave space for selection
                                nbColors++;
                                lastColSel += 2;
                                memmove(&colors[lastColSel], &colors[firstColSel], (nbColors - firstColSel) * sizeof(COLORINFO));
                                nbColors++;

                                //Align non selected left part
                                colors[firstColSel].nbChars -= (firstTot - firstX);

                                //Align non selected right part
                                colors[lastColSel].nbChars = lastRemain;

                                //Set selection
                                firstColSel++;
                                colors[firstColSel].fore = FORECOLOR(Cols_Selection);
                                colors[firstColSel].back = BACKCOLOR(Cols_Selection);
                                colors[firstColSel].nbChars = lastX - firstX;
                                return;
                        }

                        //Selection spreads over more than one color
                        nbColors++;
                        lastColSel++;
                        memmove(&colors[firstColSel + 1], &colors[firstColSel], (nbColors - firstColSel) * sizeof(COLORINFO));

                        //Align non selected left part
                        colors[firstColSel].nbChars -= (firstTot - firstX);

                        firstColSel++;
                        memmove(&colors[firstColSel + 1], &colors[lastColSel], (nbColors - lastColSel + 1) * sizeof(COLORINFO));
                        nbColors -= (lastColSel - firstColSel - 1);
                        lastColSel -=  (lastColSel - firstColSel - 1);

                        //Align non selected right part
                        colors[lastColSel].nbChars = lastRemain;

                        //Set selection
                        colors[firstColSel].fore = FORECOLOR(Cols_Selection);
                        colors[firstColSel].back = BACKCOLOR(Cols_Selection);
                        colors[firstColSel].nbChars = lastX - firstX;
                }
        }
}

//Mark Selected line
void NEAR PASCAL MarkSelectedLine(
        int view,
        int line)
{
        long firstY,lastY;
        long firstX,lastX;

        GetBlockCoord(view, &firstX, &firstY, &lastX, &lastY);

        if (line < firstY || line > lastY)
                return;

        if (line > firstY && line < lastY) {
                nbColors = 0;
                colors[0].nbChars = 2 * MAX_USER_LINE;
                colors[0].fore = FORECOLOR(Cols_Selection);
                colors[0].back = BACKCOLOR(Cols_Selection);
        }
        else {
                if (line == firstY && line != lastY)
                        SelectLine(view, firstX, MAX_USER_LINE);
                else {
                        if (line == lastY && line != firstY)
                                SelectLine(view, 0, lastX);
                        else {
                                if (firstX != lastX)
                                        SelectLine(view, firstX, lastX);
                        }
                }
        }
}

void NEAR PASCAL PatchTabs(
        int view)
{
        register int i = 0;
        register int j = 0;
        int len = pcl->Length - LHD;
        LPSTR pc = pcl->Text;
        uchar tabMark;

        if (Views[view].charSet == OEM_CHARSET)
                tabMark = 175;
        else
                tabMark = 187;
        while (i < len) {
                if (pc[i] == TAB) {
                        el[j] = tabMark;
                        j += g_contGlobalPreferences_WkSp.m_nTabSize - (j % g_contGlobalPreferences_WkSp.m_nTabSize);
                }
                else
                        j++;
                i++;
        }
        Assert(j <= MAX_USER_LINE);
}



/***    WriteLine
**
**  Synopsis:
**      void = WriteLine(view, line, hDC, X, Y, prevStatus)
**
**  Entry:
**      view - View for which to display a line
**      line - line number in view to display
**      hDC  - device context on which to display the line
**      X    - X coordinate of line to display
**      Y    - Y coordinate of line to display
**      prevStatus - The previous syntax parsing status
**
**  Returns:
**      Nothing
**
**  Description:
**      This function will cause a specified line to be displayed on the
**      screen.  Any syntax coloring will be done as needed as well as
**      the foreground/background coloration needed for the sepcific window
*/

void NEAR PASCAL WriteLine(int view, int line, HDC hDC, int X, int Y, PRECT pRc, WORD prevStatus)
{
    register int        i, rX;
    register int        total = 0;
    LPVIEWREC           v = &Views[view];
    WORD                curColor = Docs[v->Doc].docType;
    int                 nb;
    char                elCopy[2 * MAX_USER_LINE + 1];
    DWORD               penPos = 0;
    int                 penPosX = 0;

    nbColors = 0;

    rX = pRc->right;  // get it into a register

    Assert(v->Doc >= 0);

    if (!MarkStatusLine(v->Doc, pcl->Status, prevStatus)) {

        switch (Docs[v->Doc].language) {
          case C_LANGUAGE:

            if (syntaxColors) {
                CSyntax(curColor, pcl->Status, prevStatus);
                break;
            }

            //Fall Thru if false

          case PASCAL_LANGUAGE:
          default:
            colors[0].nbChars = 2 * MAX_USER_LINE;
            colors[0].fore = FORECOLOR(curColor);
            colors[0].back = BACKCOLOR(curColor);
            break;
        }
    }

    if (v->BlockStatus)
     {
      MarkSelectedLine(view, line);
     }


    if (viewTabs) {
        memmove(elCopy, el, 2 * MAX_USER_LINE);
        PatchTabs(view);
    }

    //Optimize colors array

    i = 0;
    while (i < nbColors) {
        if (colors[i].fore == colors[i + 1].fore && colors[i].back == colors[i + 1].back) {
            colors[i + 1].nbChars += colors[i].nbChars;
            memmove(&colors[i], &colors[i + 1], (nbColors - i) * sizeof(COLORINFO));
            nbColors--;
        }
        else
            i++;
    }

    rX -= X;
    MoveToX(hDC, X, Y, NULL);
    for (i = 0; i <= nbColors; i++) {
       if (penPosX < rX)
           {
            SetTextColor(hDC, colors[i].fore);
            SetBkColor(hDC, colors[i].back);
            nb = colors[i].nbChars;

            TextOut(hDC, X, Y, (LPSTR)(el + total), nb);  // Don't clear the backgroud beforehand
           }
           else
              {
               break;
              }


#ifdef WIN32
        {
            POINT lppoint;
            GetCurrentPositionEx(hDC, &lppoint);
            penPosX = lppoint.x;
        }

#else
        penPos = GetCurrentPosition(hDC);
        penPosX = LOWORD(penPos);
#endif
        total += nb;
    }

    if (viewTabs)
          memmove(el, elCopy, elLen);
}                                       /* NWriteLine() */



void FAR PASCAL
PaintText(
          int view,
          HDC hDC,
          PRECT rcl
          )
/*++

Routine Description:

    This routine is called in response to a WM_PAINT message to
    do the transfer from the edit buffer to the display of data

Arguments:

    view        - Supplies the view index to be painted
    hDC         - Supplies the device context handle to paint in
    rcl         - Supplies pointer the rectangle to be painted

Return Value:

    None.

--*/

{
    int         xPos;
    int         yPos;
    long        first;
    int         last;
    int         stop;
    LPLINEREC   pl;
    LPBLOCKDEF  pb;
    int         Y;
    int         charHeight;
    HFONT       hOldFont;
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    WORD        prevStatus;


    Assert(v->Doc >= 0);


    /*
     *  Select the current view Font
     */

    Dbg(hOldFont = (HFONT) SelectObject(hDC, v->font));

    SetTextAlign(hDC, TA_UPDATECP);

    /*
     *  Get scroll-bars position
     */

    xPos = -GetScrollPos(hwnd, SB_HORZ) * v->maxCharWidth;
    yPos = v->iYTop;
    if (yPos == -1) {
        yPos = GetScrollPos(hwnd, SB_VERT);
    }

    charHeight = v->charHeight;

    /*
     *  Calc first & last line to display
     */

    first = yPos + rcl->top / charHeight;
    last = yPos + (rcl->bottom - 1) / charHeight;

    Y = rcl->top - (rcl->top % charHeight);

    stop = Docs[v->Doc].NbLines;
    Assert (stop != 0);

    if (first < stop) {

        Assert(first < Docs[v->Doc].NbLines);
        if (first == 0) {
            prevStatus = 0;
            if (!FirstLine(v->Doc, &pl, &first, &pb)) {
                return;
            }
        } else {
            first--;
            if (!FirstLine(v->Doc, &pl, &first, &pb)) {
                return;
            }
            prevStatus = pl->Status;
            if (!NextLine(v->Doc, &pl, &first, &pb))
              return;
        }

        while (TRUE) {

            WriteLine(view, first - 1, hDC, xPos, Y, rcl, prevStatus);
            Y += charHeight;
            prevStatus = pl->Status;
            if (first > last || first == stop)
              break;
            else
              if (!NextLine(v->Doc, &pl, &first, &pb))
                return;
        }

        CloseLine(v->Doc, &pl, first, &pb);
    }

    if (first <= last) {

        HBRUSH hBrush = CreateSolidBrush(BACKCOLOR(Docs[v->Doc].docType));

        if (hBrush) {
            rcl->top = Y;
            FillRect (hDC, rcl, hBrush);
            DeleteObject(hBrush);
        }
    }

    SelectObject(hDC, hOldFont);
    return;
}                               /* PaintText() */


void FAR PASCAL
VertScroll(
    int view,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This routine is called in response to a WM_VSCROLL message

Arguments:

    view        - Supplies the view index to be scrolled

    uMsg        - Message to be processed

    wParam      - Supplies wParam message for WM_VSCROLL

    lParam      - Supplies lParam message for WM_VSCROLL

Return Value:

    None.

--*/
{
    int         vScrollInc;
    int         vScrollPos;
    RECT        rcl;
    int         lines;
    int         vWinLines;
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    int         newY;

    /*
     *  We should never get here if we are in a disassembly window.
     *  The message must be handled in the edit proc for disasm.
     */

    Assert(v->Doc >= 0);
    Assert(Docs[v->Doc].docType != DISASM_WIN);
    Assert(v->iYTop == -1);

    /*
     *  Get info we need
     */

    vScrollPos = GetScrollPos(hwnd, SB_VERT);
    lines = Docs[v->Doc].NbLines - 1;
    GetClientRect(hwnd, &rcl);
    vWinLines = max( 1, rcl.bottom / v->charHeight);

    if (WM_MOUSEWHEEL == uMsg) {
        vScrollInc = -((short) HIWORD(wParam)) / WHEEL_DELTA;    // wheel rotation
    } else {
        switch (LOWORD(wParam)) {

        case SB_LINEUP:
            vScrollInc = -1;
            break;

        case SB_LINEDOWN:
            vScrollInc = 1;
            break;

        case SB_PAGEUP:
            vScrollInc = -vWinLines;
            break;

        case SB_PAGEDOWN:
            vScrollInc = vWinLines;
            break;

        case SB_THUMBTRACK:
            vScrollInc = ((int)HIWORD(wParam)) - vScrollPos;
            break;


        case SB_THUMBPOSITION:
            vScrollInc = HIWORD(wParam) - vScrollPos;
            Unreferenced( lParam );
            break;

        default:
            return;

        }
    }

    newY = mini(lines, v->Y + vScrollInc);

    if (vScrollInc < 0) {
        vScrollInc = maxi(vScrollInc, -vScrollPos);
    } else {
        lines -=(vScrollPos + vWinLines);
        if ((rcl.bottom + v->charHeight - 1) % v->charHeight) {
            lines++;
        }
        vScrollInc = maxi(mini(vScrollInc, lines), 0);
    }
    vScrollPos += vScrollInc;

    if (vScrollInc != 0) {

        /*
         * Set scroll-bar positions before refreshing the screen because
         * we use the scroll-bar positions during the PAINT event
         */

        if (v->iYTop == -1) {
            SetScrollPos(hwnd, SB_VERT, vScrollPos, TRUE);
        } else {
            v->iYTop = vScrollPos;
        }

        ScrollWindow(hwnd, 0, - v->charHeight * vScrollInc, NULL, NULL);
        UpdateWindow(hwnd);

    }

    if (scrollOrigin & FROM_KEYBOARD) {
        if (scrollOrigin & SELECTING) {
            int OldYR;

            if (!v->BlockStatus) {
                v->BlockStatus = TRUE;
                v->BlockXL = v->BlockXR = v->X;
                v->BlockYL = v->BlockYR = v->Y;
            }
            PosXY(view, v->X, newY, FALSE);
            OldYR = v->BlockYR;
            v->BlockXR = v->X;
            v->BlockYR = v->Y;
            InvalidateLines(view, mini(OldYR, v->BlockYR), maxi (OldYR, v->BlockYR), FALSE);
        } else {
            PosXY(view, v->X, newY, FALSE);
        }
    }
    else {
        // This is to change a position of IME conversion window
        SetCaret(view, v->X, v->Y, Pos2Pix(view, v->X, v->Y));
    }
    return;
}                               /* VertScroll() */

void FAR PASCAL
HorzScroll(
    int view,
    WPARAM wParam,
    LPARAM lParam
    )
{
    int       hScrollInc;
    int       hScrollPos;
    RECT      rcl;
    LPVIEWREC v = &Views[view];
    HWND hwnd = v->hwndClient;
    int  newX = v->X;

    // Get info we need

      Assert(v->Doc >= 0);

    hScrollPos = GetScrollPos(hwnd, SB_HORZ);
    GetClientRect(hwnd, &rcl);

    switch (LOWORD(wParam)) {

    case SB_LINEUP:
        hScrollInc = mini(-1, (-rcl.right / SCROLL_RATIO) / v->maxCharWidth);
        break;

    case SB_LINEDOWN:
        hScrollInc = maxi(1, (rcl.right / SCROLL_RATIO) / v->maxCharWidth);
        break;

    case SB_PAGEUP:
        hScrollInc = mini(-1, -rcl.right / v->maxCharWidth);
        break;

    case SB_PAGEDOWN:
        hScrollInc = maxi(1, rcl.right / v->maxCharWidth);
        break;

    case SB_THUMBTRACK:
        hScrollInc = ((int)HIWORD(wParam)) - hScrollPos;
         break;


    case SB_THUMBPOSITION:
#ifdef WIN32
        hScrollInc = HIWORD(wParam) - hScrollPos;
        Unreferenced( lParam );
#else
        hScrollInc = LOWORD(lParam) - hScrollPos;
#endif
        break;

    default:
        hScrollInc = 0;
    }

    if (hScrollInc = maxi(-hScrollPos, mini(hScrollInc, MAX_USER_LINE - hScrollPos)))
      {
          int XC, XX;

          XC = hScrollPos * v->maxCharWidth;
          XX = Pos2Pix(view, newX, v->Y);
          if (XX < XC) {
              newX = Pix2Pos(view, XC, v->Y);
              XX = Pos2Pix(view, newX, v->Y);
              if (XX < XC)
                newX++;
          }
          else
            if (XX >= (XC + rcl.right - (v->maxCharWidth + v->maxCharWidth))) {
                newX = Pix2Pos (view, XC + rcl.right - (v->maxCharWidth + v->maxCharWidth), v->Y);
                XX = Pos2Pix(view, newX, v->Y);
                if (XX >= (XC + rcl.right - (v->maxCharWidth + v->maxCharWidth)))
                  newX--;
            }
          Assert(v->Y < Docs[v->Doc].NbLines);

          //Set scroll-bar positions before refreshing the screen
            //we use the scroll-bar positions during the PAINT event
              if (newX < MAX_USER_LINE
                  || (newX >= MAX_USER_LINE && hScrollInc < 0)) {
                  hScrollPos += hScrollInc;
                  SetScrollPos(hwnd, SB_HORZ, hScrollPos, TRUE);
                  ScrollWindow(hwnd, -v->maxCharWidth * hScrollInc, 0, NULL, NULL);
                  UpdateWindow(hwnd);

              }

        // This is to change a position of IME conversion window
        SetCaret(view, v->X, v->Y, Pos2Pix(view, v->X, v->Y));
      }
}

void NEAR PASCAL SelPosXY(
        int view,
        int X,
        int Y)
{
        int OldYR;
        LPVIEWREC v = &Views[view];

        Assert(v->Doc >= 0);

        if (GetCapture() == v->hwndClient)
                return;

        //Normalize
        Assert(Y <= Docs[v->Doc].NbLines - 1);
        if (X < 0)
                X = 0;
        if (X > MAX_USER_LINE)
                X = MAX_USER_LINE;
        if (Y < 0)
                Y = 0;

        if (!v->BlockStatus)    {
                v->BlockStatus = TRUE;
                v->BlockXL = v->BlockXR = v->X;
                v->BlockYL = v->BlockYR = v->Y;
        }

        PosXY(view, X, Y, FALSE);
        OldYR = v->BlockYR;
        v->BlockXR = v->X;
        v->BlockYR = v->Y;

        InvalidateLines(view, mini(OldYR, v->BlockYR), maxi (OldYR, v->BlockYR), FALSE);
}

//Return length of line
int FAR PASCAL GetLineLength(
        int view,
        BOOL skipBlanks,
        long line)
{
        LPLINEREC pl;
        LPBLOCKDEF pb;
        register int X;
        LPVIEWREC v = &Views[view];

        Assert(v->Doc >= 0);

        if (!FirstLine(v->Doc, &pl, &line, &pb))
                return 0;
        CloseLine (v->Doc, &pl, line, &pb);
        X = elLen;
        if (skipBlanks) {
                register int i;

                X = 0;
                for (i = 0 ; i < elLen ; i++) {
                        if (IsDBCSLeadByte(el[i]) && i+1 < elLen) {
                                i++;
                                X = i+1;
                        } else if (' ' != el[i]) {
                                X = i+1;
                        }
                }
        }
        return X;
}


//Move cursor to prev word
void NEAR PASCAL MoveLeftWord(
        int view,
        BOOL Sel,
        int posX,
        int posY)
{
        LPLINEREC       pl;
        LPBLOCKDEF      pb;
        int                     x;
        long                    y;
        LPVIEWREC v = &Views[view];

        y = posY;

        Assert(v->Doc >= 0);
        Assert(y < Docs[v->Doc].NbLines);
        if (!FirstLine(v->Doc, &pl, &y, &pb))
                return;

        //y has been incremented. Adjust y to its previous position
        y--;

        x = mini(posX, elLen);
        do {
                while (x == 0) {
                        if (y == 0)

                                //Top of file
                                goto done;

                        else {

                                //FirstLine has incremented y. As we have decremented
                                //adjust it to its expected position before calling
                                //CloseLine
                                y++;
                                CloseLine(v->Doc, &pl, y, &pb);

                                //Decrements twice because we want to go to the previous line
                                y-= 2;

                                Assert(y < Docs[v->Doc].NbLines);
                                if (!FirstLine(v->Doc, &pl, &y, &pb))
                                        return;

                                y--; // adjust y to the right value
                                x = elLen;
                        }
                }
                {
                        int iLeftChar = -1;
                        int i;

                        for (i = 0 ; i < x ; i++) {
                                if (IsDBCSLeadByte((BYTE)el[i])) {
                                        iLeftChar = i;
                                        i++;
                                } else if (CHARINKANASET(el[i])) {
                                        iLeftChar = i;
                                } else if (CHARINALPHASET(el[i])) {
                                        iLeftChar = i;
                                }
                        }
                        x = (-1 != iLeftChar) ? iLeftChar : 0;
                }

        //Now either x is equal to zero or set after a word character
        } while (x == 0);

        {
                int iLeftChar = -1;
                int i;

                //make sure if the cursor isn't on middle of DBCS char
                for (i = 0 ; i < x ; i++) {
                        if (IsDBCSLeadByte((BYTE)el[i])) {
                                if (x == i + 1) {
                                        x = i;
                                        break;
                                }
                                i++;
                        }
                }
                for (i = 0 ; i <= x ; i++) {
                        if (IsDBCSLeadByte(el[i])) {
                                if (IsDBCSLeadByte(el[x])) {
#ifdef DBCS_WORD_MULTI
                                        if (-1 == iLeftChar) {
                                                iLeftChar = i;
                                        }
#else
                                        // Suppose one DBCS char is one word.
                                        iLeftChar = i;
#endif  // DBCS_WORD_MULTI
                                } else {
                                        iLeftChar = -1;
                                }
                                i++;
                        } else if (CHARINKANASET(el[i])) {
                                if (CHARINKANASET(el[x])) {
                                        if (-1 == iLeftChar) {
                                                iLeftChar = i;
                                        }
                                } else {
                                        iLeftChar = -1;
                                }
                        } else if (CHARINALPHASET(el[i])) {
                                if (CHARINALPHASET(el[x])) {
                                        if (-1 == iLeftChar) {
                                                iLeftChar = i;
                                        }
                                } else {
                                        iLeftChar = -1;
                                }
                        } else {
                                iLeftChar = -1;
                        }
                }
                x = (-1 != iLeftChar) ? iLeftChar : 0;
        }

done:
        y++;
        CloseLine(v->Doc, &pl, y, &pb);
        y--;

        if (Sel)
                SelPosXY(view, x, y);
        else
                PosXY(view, x, y, FALSE);
}


void NEAR PASCAL MoveRightWord(
        int view,
        BOOL Sel,
        int posX,
        int posY)
{
        LPLINEREC       pl;
        LPBLOCKDEF      pb;
        int                     x;
        long                    y;
        LPVIEWREC v = &Views[view];

        BYTE    chWord;

        y = posY;

        Assert (v->Doc >= 0);
        Assert (y < Docs[v->Doc].NbLines);

        if (!FirstLine(v->Doc, &pl, &y, &pb)) {
                return;
        }
        CloseLine(v->Doc, &pl, y, &pb);
        y--;

        posX = mini(posX, elLen);

        //make sure if the cursor isn't on middle of DBCS char
        for (x = 0 ; x < posX ; x++) {
                if (IsDBCSLeadByte((BYTE)el[x])) {
                        if (posX == x + 1) {
                                break;
                        }
                        x++;
                }
        }

        if (CHARINWORDCHAR(el[x])) {
                chWord = el[x];
        } else {

                /****************************/
                /* skip non-word characters */
                /****************************/

                while (y < Docs[v->Doc].NbLines) {
                        if (!FirstLine(v->Doc, &pl, &y, &pb)) {
                                return;
                        }
                        CloseLine(v->Doc, &pl, y, &pb);
                        y--;

                        while (x < elLen) {
                                if (CHARINWORDCHAR(el[x])) {
                                        chWord = el[x];
                                        break;
                                }
                                x++;
                        }

                        if (x < elLen) {
                                break;
                        }
                        x = 0;
                        y++;
                }

                if (y >= (Docs[v->Doc].NbLines)) {
                        PosXY(view, elLen, y - 1, FALSE);
                        return;
                }
                if (!Sel) {
                        PosXY(view, x, y, FALSE);
                        return;
                }
        }

        /**************************/
        /* search the end of word */
        /**************************/

        while (x < elLen) {
#ifdef DBCS_WORD_MULTI
#else
                if (IsDBCSLeadByte(chWord)) {
                        // Suppose one DBCS char is one word.
                        x += 2;
                        break;
                }
#endif
                if (IsDBCSLeadByte(el[x])) {
                        if (!IsDBCSLeadByte(chWord)) {
                                break;
                        }
                        x += 2;
                } else if (CHARINKANASET(el[x])) {
                        if (!CHARINKANASET(chWord)) {
                                break;
                        }
                        x++;
                } else if (CHARINALPHASET(el[x])) {
                        if (!CHARINALPHASET(chWord)) {
                                break;
                        }
                        x++;
                } else {
                        break;
                }
        }

        if (Sel) {
                SelPosXY(view, x, y);
                return;
        }

        /*******************************/
        /* search the top of next word */
        /*******************************/

        while (y < Docs[v->Doc].NbLines) {
                if (!FirstLine(v->Doc, &pl, &y, &pb)) {
                        return;
                }
                CloseLine(v->Doc, &pl, y, &pb);
                y--;

                while (x < elLen) {
                        if (CHARINWORDCHAR(el[x])) {
                                break;
                        }
                        x++;
                }

                if (x < elLen) {
                        break;
                }
                x = 0;
                y++;
        }
        if (y >= (Docs[v->Doc].NbLines)) {
                PosXY(view, elLen, y - 1, FALSE);
        } else {
                PosXY(view, x, y, FALSE);
        }
        return;

}

void FAR PASCAL ClearSelection(int view)
{
 LPVIEWREC v = &Views[view];

 Assert(v->Doc >= 0);
   if (v->BlockStatus)
    {
     v->BlockStatus = FALSE;
     InvalidateLines(view, mini(v->BlockYL, v->BlockYR), maxi(v->BlockYL, v->BlockYR), FALSE);
    }
}


void FAR PASCAL
MouseMove(
          int view,
          WPARAM wParam,
          int X,
          int Y
          )

/*++

Routine Description:

    This function is called in response to a mouse move message.

Arguments:

    view        - Supplies the view index for the mouse move
    wParam      - Supplies wParam for the WM_MOUSEMOVE
    X           - Supplies the X location of the ouse (LOWORD(lParam))
    Y           - Supplies the Y location of the ouse (HIWORD(lParam))

Return Value:

    None.

--*/
{
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    int         yPos = v->iYTop;

    Assert(v->Doc >= 0);

    Unused(wParam);

    if (GetCapture() == hwnd) {

        RECT rcl;
        int OldYR;

        GetClientRect(hwnd, &rcl);
        if (X < 0) {
            X = 0;
        } else if (X >= rcl.right) {
            X = rcl.right - 1;
        }

        if (Y < 0) {
            Y = 0;
        } else if (Y >= rcl.bottom) {
            Y = rcl.bottom - 1;
        }

        if (yPos == -1) {
            yPos = GetScrollPos( hwnd, SB_VERT);
        }
        Y = mini(yPos + Y / v->charHeight, Docs[v->Doc].NbLines - 1);
        X = mini(Pix2Pos(view,
                         GetScrollPos(hwnd, SB_HORZ) * v->maxCharWidth + X, Y),
                 MAX_USER_LINE);
        X = AdjustPosX(X, FALSE);
        OldYR = v->BlockYR;
        v->BlockXR = X;
        v->BlockYR = Y;
        PosXY(view, X, Y, FALSE);
        InvalidateLines(view, mini(OldYR, v->BlockYR), maxi(OldYR, v->BlockYR), FALSE);
    }
    return;
}                               /* MouseMove() */

void FAR PASCAL TimeOut(
        int view)
{
        POINT   point;
        RECT rcl;
        BOOL ok1,ok2;
                  int  scrl;
        HWND hwnd = Views[view].hwndClient;

        GetCursorPos (&point);
        ScreenToClient (hwnd, &point);

        GetClientRect(hwnd, &rcl);

        ok1 = ok2 = TRUE;
        if ((point.x < 0) && ((scrl = GetScrollPos(hwnd, SB_VERT)) > 0))
                SendMessage(hwnd, WM_HSCROLL, SB_LINEUP, 0L);
        else
                if (point.x >= rcl.right)
                        SendMessage(hwnd, WM_HSCROLL, SB_LINEDOWN, 0L);
                else
                        ok1 = FALSE;

        if ((point.y < 0) && ((scrl = GetScrollPos(hwnd, SB_VERT)) > 0))
                SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0L);
        else
                if (point.y >= rcl.bottom)
                        SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0L);
                else
                        ok2 = FALSE;

        if (ok1 || ok2)
                MouseMove(view, 0, point.x, point.y);
}

void NEAR PASCAL SortBlock(
        int view)
{
        LPVIEWREC v = &Views[view];

        Assert(v->Doc >= 0);

        if ((v->BlockYL > v->BlockYR)
                 || (v->BlockYL == v->BlockYR && v->BlockXL > v->BlockXR)) {

                int xx = v->BlockXR , yy = v->BlockYR;

                v->BlockXR = v->BlockXL;
                v->BlockYR = v->BlockYL;
                v->BlockXL = xx;
                v->BlockYL = yy;

        }
}


void FAR PASCAL
ButtonDown(
           int view,
           WPARAM wParam,
           int X,
           int Y
           )
/*++

Routine Description:

    This routine is called in response to a WM_LBUTTON message.

    We capture the mouse and setup a timer to send us regular messages
    so that we can still do page downs even if the mouse does not move.
    This is a normal windows program technique.

Arguments:

    view        - Supplies the view index for the window
    wParam      - Supplies the wParam for the WM_LBUTTON message
    X           - Supplies the X position of the mouse
    Y           - Supplies the Y position of the mouse

Return Value:

    None.

--*/

{
    RECT        rcl;
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    int         yPos = v->iYTop;

    Assert(v->Doc >= 0);

    GetClientRect(hwnd, &rcl);
    if (yPos == -1) {
        yPos = GetScrollPos(hwnd, SB_VERT);
    }

    Y = mini(yPos + Y / v->charHeight,
             Docs[v->Doc].NbLines - 1);
    X = mini(Pix2Pos(view, GetScrollPos(hwnd, SB_HORZ) * v->maxCharWidth + X,
                     Y), MAX_USER_LINE);

    X = AdjustPosX(X, FALSE);

    if (wParam & MK_SHIFT) {
        if (v->BlockStatus) {
            SortBlock(view);
            InvalidateLines(view, v->BlockYL, v->BlockYR, FALSE);
            if ((Y > v->BlockYL) || (Y == v->BlockYL && X > v->BlockXL)) {
                v->BlockXR = X;
                v->BlockYR = Y;
            } else {
                v->BlockXL = X;
                v->BlockYL = Y;
            }
        } else {
            v->BlockStatus = TRUE;
            v->BlockXL = v->X;
            v->BlockYL = v->Y;
            v->BlockXR = X;
            v->BlockYR = Y;
            SortBlock(view);
        }

        InvalidateLines(view, v->BlockYL, v->BlockYR, FALSE);
    } else {
        ClearSelection(view);
        v->BlockStatus = TRUE;
        v->BlockXL = X;
        v->BlockYL = Y;
        v->BlockXR = X;
        v->BlockYR = Y;
    }

    SetCaret(view, X, Y, -1);
    SetTimer(hwnd, 100, 50, (TIMERPROC)NULL);
    SetCursor(LoadCursor(NULL, IDC_IBEAM));
    SetCapture(hwnd);

    return;
}                               /* ButtonDown() */


void FAR PASCAL
ButtonUp(
         int view,
         WPARAM wParam,
         int X,
         int Y
         )

/*++

Routine Description:

    This routine is called in response to a WM_LBUTTONUP message.

    It will clean up what ever was done in ButtonDown about
    capturing the cursor.

Arguments:

    view        - Supplies the index to the view of the current window
    wParam      - Supplies the wParam for the message
    X           - Supplies the X position of the message
    Y           - Supplies the Y position of the messagen

Return Value:

    None.

--*/

{
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    int         yPos = v->iYTop;

    Unused(wParam);

    Assert(v->Doc >= 0);

    if (GetCapture() == hwnd) {

        RECT rcl;

        GetClientRect(hwnd, &rcl);
        if (X < 0) {
            X = 0;
        } else if (X >= rcl.right) {
            X = rcl.right-1;
        }

        if (Y < 0) {
            Y = 0;
        } else if (Y >= rcl.bottom) {
            Y = rcl.bottom-1;
        }

        if (yPos == -1) {
            yPos = GetScrollPos(hwnd, SB_VERT);
        }

        Y = mini(yPos + Y / v->charHeight, Docs[v->Doc].NbLines - 1);
        X = mini(Pix2Pos(view,
                         GetScrollPos(hwnd, SB_HORZ) * v->maxCharWidth + X,
                         Y), MAX_USER_LINE);
        X = AdjustPosX(X, FALSE);

        SortBlock(view);

        if ((Y > v->BlockYL) || (Y == v->BlockYL && X > v->BlockXL)) {
            v->BlockXR = X;
            v->BlockYR = Y;
        } else {
            v->BlockXL = X;
            v->BlockYL = Y;
        }

        InvalidateLines(view, v->BlockYL, v->BlockYR, FALSE);

        KillTimer(hwnd, 100);
        if (v->BlockXL == v->BlockXR && v->BlockYL == v->BlockYR) {
            v->BlockStatus = FALSE;
        }
        PosXY(view, X, Y, FALSE);
        ReleaseCapture();
    }

    return;
}                               /* ButtonUp() */

BOOL FAR PASCAL GetSelectedText(
        int view,
        BOOL *lookAround,
        LPSTR pText,
        int maxSize,
        LPINT leftCol,
        LPINT rightCol)
{

        LPVIEWREC v = &Views[view];
        LPLINEREC pl;
        LPBLOCKDEF pb;
        long XL,XR;
        long YL,YR;

        Assert(v->Doc >= 0);

        if (!v->BlockStatus)
                return FALSE;

        GetBlockCoord(view, &XL, &YL, &XR, &YR);

        //If selection is on 1 line, returns all selection
        if (YL == YR) {
                if (!FirstLine(v->Doc, &pl, &YL, &pb))
                        goto err;
                XR = mini(elLen, XR);

                maxSize = mini(maxSize, XR - XL);
                if (maxSize > 0) {
                        memmove(pText, (LPSTR)(el + XL), maxSize);
                        pText[maxSize] = '\0';
                }
                if (leftCol)
                        *leftCol = XL;
                if (rightCol)
                        *rightCol = XR;

                CloseLine (v->Doc, &pl, YL, &pb);
        }
        else {

                //Multiline selection, set line to line where cursor is
                if (v->Y != YL)
                        YL = YR;

                //Return word at or nearest to cursor
                return GetWordAtXY(view, v->X, YL, FALSE, lookAround, TRUE,
                                                                 (LPSTR)pText, maxSize, leftCol, rightCol);
        }
        return TRUE;

        err : {
                Assert(FALSE);
                return FALSE;
        }
}

/***    InsertStream
**
**  Synopsis:
**      bool = InsertStream(view, x, y, size, Buf, destroyRedo)
**
**  Entry:
**      view    - which view to do the insert on
**      x       - location to do the insert at
**      y       - location to do the insert at
**      size    - count of characters to insert
**      Buf     - pointer to buffer to be inserted
**      destroyRedo - TRUE if no redo is allowed now
**
**  Returns:
**      TRUE on success else FALSE
**
**  Description:
**
*/

BOOL
PASCAL
InsertStream (
    int view,
    int x,
    int y,
    int size,
    LPSTR Buf,
    BOOL destroyRedo
    )
{
    LPVIEWREC v = &Views[view];
    LPDOCREC d = &Docs[v->Doc];
    BOOL lineAdded;
    BOOL prevStopMark = stopMarkStatus;

    Assert(v->Doc >= 0);
    if (v->Doc < 0) {
        return FALSE;
    }

    //Destroy Redo Buffer if we start re-editing in a middle of a
    //undo/redo session

    if (destroyRedo)
          DestroyRecBuf(v->Doc, REC_REDO);

    if (v->BlockStatus) {
        long XR, YR;

        //More than 1 edit action for this, operation. Tell the
        //undo/redo engine that records have no stop marks after
        //this record

        stopMarkStatus = NEXT_HAS_NO_STOP;

        GetBlockCoord (view, &(v->X), &(v->Y), &XR, &YR);
        DeleteStream(view, v->X, v->Y, XR, YR, FALSE);
    } else {
        v->X = x;
        v->Y = y;
    }

    //Insert characters

    if (InsertBlock(v->Doc, v->X, v->Y, size, Buf)) {
        lineAdded =  (d->lineTop != d->lineBottom);
        InvalidateLines(view, d->lineTop, d->lineBottom, lineAdded);
        if (lineAdded)
              SetVerticalScrollBar(view, TRUE);

        //Put undo/redo engine back in normal state

        if (prevStopMark == HAS_STOP)
              stopMarkStatus = HAS_STOP;

        return TRUE;
    } else
        return FALSE;
}                                       /* InsertStream() */

BOOL FAR PASCAL ReplaceChar(
        int view,
        int x,
        int y,
        int ch,
        BOOL destroyRedo)
{

        LPVIEWREC v = &Views[view];
        BOOL prevStopMark = stopMarkStatus;

        Assert(v->Doc >= 0);

        //Destroy Redo Buffer if we start re-editing in a middle of a
        //undo/redo session
        if (destroyRedo)
                DestroyRecBuf(v->Doc, REC_REDO);

        if (v->BlockStatus) {
                long XR, YR;

                //More than 1 edit action for this, operation. Tell the
                //undo/redo engine that records have no stop marks after
                //this record
                stopMarkStatus = NEXT_HAS_NO_STOP;

                GetBlockCoord (view, &(v->X), &(v->Y), &XR, &YR);
                DeleteStream(view, v->X, v->Y, XR, YR, FALSE);
        }
        else {
                v->X = x;
                v->Y = y;
        }

        //Replace character and update line
        if (ReplaceCharInBlock(v->Doc, v->X, v->Y, ch)) {
                InvalidateLines(view, Docs[v->Doc].lineTop, Docs[v->Doc].lineBottom, FALSE);

                //Put undo/redo engine back in normal state
                if (prevStopMark == HAS_STOP)
                        stopMarkStatus = HAS_STOP;

                return TRUE;
        }
        else
                return FALSE;
}

//Delete stream
BOOL FAR PASCAL
DeleteStream(
    int view,
    int XL,
    int YL,
    int XR,
    int YR,
    BOOL destroyRedo
    )
{
    LPVIEWREC v = &Views[view];

    Assert(v->Doc >= 0);

    //Destroy Redo Buffer if we start re-editing in a middle of a
    //undo/redo session
    if (destroyRedo) {
        DestroyRecBuf(v->Doc, REC_REDO);
    }

    //Delete block
    if (!DeleteBlock(v->Doc, XL, YL, XR, YR)) {

        return FALSE;

    } else {

        InvalidateLines(view,
                 Docs[v->Doc].lineTop,
                 Docs[v->Doc].lineBottom,
                 YL != YR);

        if (YL != YR) {
            SetVerticalScrollBar(view, TRUE);
        }

        v->BlockStatus = FALSE;

        return TRUE;
    }
}

//Delete all stream
BOOL FAR PASCAL
DeleteAllStream(
    int view,
    BOOL destroyRedo
    )
{
    return DeleteStream(view,
                     0,
                     0,
                     MAX_USER_LINE,
                     Docs[Views[view].Doc].NbLines - 1,
                     destroyRedo);
}

//Paste stream
BOOL FAR PASCAL
PasteStream (
    int view,
    long XL,
    long YL,
    long XR,
    long YR
    )
{
    LPLINEREC pl;
    LPBLOCKDEF pb;
    long y;
    long size, sz;
    LPSTR p;
    HANDLE hData;
    LPVIEWREC v = &Views[view];
#ifdef DEBUGGING
    LPSTR p1;
#endif

    Assert(v->Doc >= 0);
    Assert(YL <= YR && YR < Docs[v->Doc].NbLines);

    //Get information to compute size of clipboard buffer
    y = YL;
    if (!FirstLine(v->Doc, &pl, &y, &pb)) {
        return FALSE;
    }
    XL = AlignToTabs(XL, pl->Length - LHD,  (LPSTR)pl->Text);

    if (YL == YR) {
        if (XL >= (pl->Length - LHD)) {
            size = 0;
        } else {
            XR = AlignToTabs(XR, pl->Length - LHD,  (LPSTR)pl->Text);
            size = mini(XR, (pl->Length - LHD)) - XL;
        }
    } else {

        //Compute size of first line
        size = pl->Length - LHD - mini(XL, (pl->Length - LHD));

        //Compute size of middle lines
        while (y < YR) {
            if (!NextLine(v->Doc, &pl, &y, &pb)) {
                return FALSE;
            }
            size += 2 + (pl->Length - LHD);
        }

        //Compute size of last line
        if (!NextLine(v->Doc, &pl, &y, &pb)){
            return FALSE;
        }
        XR = AlignToTabs(XR, pl->Length - LHD,  (LPSTR)pl->Text);
        size += 2 + mini((pl->Length - LHD), XR);

    }
    CloseLine (v->Doc, &pl, y, &pb);

    if (size >= MAX_CLIPBOARD_SIZE) {
        ErrorBox(ERR_Clipboard_Overflow);
        return FALSE;
    }

    Dbg(hData = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE, size + 1));
    if (!hData) {
        return FALSE;
    }

    Dbg((p = (PSTR) GlobalLock (hData)) != NULL);

    if (p == NULL) {
        return FALSE;
    }

#ifdef DEBUGGING
    p1 = p;
#endif
    y = YL;
    if (!FirstLine(v->Doc, &pl, &y, &pb)) {
        return FALSE;
    }

    if (YL == YR) {
        if (XL >= (pl->Length - LHD)) {
            sz = 0;
        } else {
            sz = mini(XR, (pl->Length - LHD)) - XL;
            memmove(p, (LPSTR)(pl->Text + XL), (unsigned)sz);
            p += sz;
        }
    } else {

        //Copy first line
        sz = (pl->Length - LHD) - XL;
        if (sz > 0) {
            memmove(p, (LPSTR)(pl->Text + XL), (unsigned)sz);
            p += sz;
        }

        //Copy middle lines (if any)
        while (y < YR) {
            if (!NextLine(v->Doc, &pl, &y, &pb)){
                return FALSE;
            }

            memmove(p, (LPSTR)CrLf, 2);
            p += 2;
            memmove(p, pl->Text, pl->Length - LHD);
            p += pl->Length - LHD;
        }

        memmove(p, (LPSTR)CrLf, 2);
        p += 2;

        //Copy last line
        if (!NextLine(v->Doc, &pl, &y, &pb)){
            return FALSE;
        }
        memmove((LPSTR)szTmp, pl->Text, pl->Length - LHD);
        szTmp[pl->Length - LHD] = '\0';
        memmove(p, pl->Text, mini((pl->Length - LHD), XR));
        p += mini((pl->Length - LHD), XR);
    }
    *p = '\0';
    CloseLine (v->Doc, &pl, y, &pb);

#ifdef DEBUGGING
    Assert(lstrlen(p1) == (int)size);
#endif

    DbgX(GlobalUnlock(hData) == 0);

    if (OpenClipboard (/*GetDesktopWindow())*/ hwndFrame)) {
        EmptyClipboard();
        hData = SetClipboardData(CF_TEXT, hData);
        CloseClipboard();
    }

    return TRUE;
}

//Find matching {[( ... )]}
void NEAR PASCAL FindMatching(
        int view)
{
        LPLINEREC pl;
        LPBLOCKDEF pb;
        LPVIEWREC v = &Views[view];
        long y = v->Y;
        char source, target, ch;
        int k, level = 0;
        BOOL found = FALSE, goDown;


        Assert(v->Doc >= 0);

        //Load line
        if (!FirstLine(v->Doc, &pl, &y, &pb))
                goto err;

        //See if X in range
        if (v->X < elLen) {

                source = el[v->X];

                //See if char at cursor is in '{' '[' '(' '}' ']' ')'
                switch (source) {
                        case '{':
                                target = '}';
                                goDown = TRUE;
                                break;
                        case '[':
                                target = ']';
                                goDown = TRUE;
                                break;
                        case '(':
                                target = ')';
                                goDown = TRUE;
                                break;
                        case '}':
                                target = '{';
                                goDown = FALSE;
                                break;
                        case ']':
                                target = '[';
                                goDown = FALSE;
                                break;
                        case ')':
                                target = '(';
                                goDown = FALSE;
                                break;

                        //No match char
                        default:
                                CloseLine(v->Doc, &pl, y, &pb);
                                MessageBeep(0);
                                return;
                }


                if (goDown) {

                        int n = y;
                        k = v->X + 1;

                        //Go down in text
                        while (n <= Docs[v->Doc].NbLines && !found) {

                                //Parse current line
                                while (k < elLen && !found) {
                                        if (IsDBCSLeadByte(el[k]) && k+1 < elLen) {
                                                k+=2;
                                                continue;
                                        }
                                        ch = el[k];
                                        found = (ch == target && level == 0);
                                        if (ch == source)
                                                level++;
                                        if (ch == target)
                                                level--;
                                        k++;
                                }

                                if (y < Docs[v->Doc].NbLines && !found) {
                                        if (!NextLine(v->Doc, &pl, &y, &pb))
                                                goto err;
                                        k = 0;
                                }
                                n++;
                        }

                        k--;
                }
                else {

                        //Adjust y
                        y--;
                        k = v->X - 1;

                        //Go up in text
                        while (y >= 0 && !found) {

                                //Parse current line
                                while (k >= 0 && !found) {
                                        ch = el[k];
                                        if (ch == source || ch == target) {
                                                register int i;
                                                BOOL bDBCS = FALSE;

                                                for (i = 0 ; i <= k ; i++) {
                                                        if (IsDBCSLeadByte(el[i]) && i+1 < elLen) {
                                                                if (i+1 == k){
                                                                        bDBCS = TRUE;
                                                                        break;
                                                                }
                                                        }
                                                }
                                                if (bDBCS) {
                                                        k -= 2;
                                                        continue;
                                                }
                                        }
                                        found = (ch == target && level == 0);
                                        if (ch == source)
                                                level++;
                                        if (ch == target)
                                                level--;
                                        k--;
                                }

                                if (y > 0 && !found) {
                                        if (!PreviousLine(v->Doc, &pl, y, &pb))
                                                goto err;
                                        k = elLen;
                                }
                                y--;
                        }
                        k++;

                        //Reajust y
                        y += 2;
                }

        }

        if (found)
                PosXY(view, k, y - 1, FALSE);
        else
                MessageBeep(0);
        CloseLine(v->Doc, &pl, y, &pb);
        return;

        err: {
                Assert(FALSE);
                return;
        }

}

// Get block coord
void FAR PASCAL GetBlockCoord(
        int  view,
        long *XL,
        long *YL,
        long *XR,
        long *YR)
{
        LPVIEWREC v = &Views[view];

        Assert(v->Doc >= 0);

        if (v->BlockYL < v->BlockYR) {
                *XL = v->BlockXL;
                *XR = v->BlockXR;
                *YL = v->BlockYL;
                *YR = v->BlockYR;
        }
        else
                if (v->BlockYL > v->BlockYR) {
                        *XL = v->BlockXR;
                        *XR = v->BlockXL;
                        *YL = v->BlockYR;
                        *YR = v->BlockYL;
                }
                else {
                        *XL = mini(v->BlockXL, v->BlockXR);
                        *XR = maxi(v->BlockXL, v->BlockXR);
                        *YL = *YR = v->BlockYL;
                }
}

void FAR PASCAL
DeleteKey(
    int view)
{
    long XL,XR;
    long YL,YR;
    LPVIEWREC v = &Views[view];
    BOOL fRet;

    Assert(v->Doc >= 0);

    GetBlockCoord (view, &XL, &YL, &XR, &YR);
    if (v->BlockStatus && (XL != XR || YL != YR)) {
        fRet = DeleteStream(view, XL,  YL, XR, YR, TRUE);
    } else {
        XL = v->X;
        YL = v->Y;

        if (XL < GetLineLength(view, TRUE, YL)) {
            LPLINEREC   pl;
            LPBLOCKDEF  pb;

            if (!FirstLine(v->Doc, &pl, &YL, &pb)) {
                return;
            }
            CloseLine(v->Doc, &pl, YL, &pb);
            YL--;
            if (IsDBCSLeadByte((BYTE)(el[XL]))) {
                if (fRet = DeleteStream(view, XL, YL, XL + 2, YL, TRUE)) {
                    SetReplaceDBCSFlag(&Docs[v->Doc], FALSE);
                }
            } else {
                fRet = DeleteStream(view, XL, YL, XL + 1, YL, TRUE);
            }
        } else if (YL < (Docs[v->Doc].NbLines - 1)) {
            fRet = DeleteStream(view, XL, YL, 0, YL + 1, TRUE);
        }
    }
    if (fRet && (YL <= (Docs[v->Doc].NbLines - 1))) {
        PosXY(view, XL, YL, FALSE);
    }
}

/***    KeyDown
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


void FAR PASCAL
KeyDown(
        int view,
        WPARAM wParam,
        BOOL shiftDown,
        BOOL ctrlDown
        )
/*++

Routine Description:

    This function is called in response to a WM_KEYDOWN message to
    do processing of some key sequences for the editor.

Arguments:

    view        - Supplies the view index of the current window
    wParam      - Supplies the wParam field for the WM_KEYDOWN message
    shiftDown   - Supplies TRUE if a shift key is presed
    ctrlDown    - Supplies TRUE if a control key is pressed

Return Value:

    None.

--*/

{
    int         posX;
    long        posY;
    int         nLine;
    LPVIEWREC   v = &Views[view];
    HWND        hwnd = v->hwndClient;
    int         yPos;

    Assert(v->Doc >= 0);

    posX = v->X;
    posY = v->Y;

    switch (wParam) {

    case VK_SHIFT:
    case VK_CONTROL:
        return;

    case VK_LEFT:
        if (shiftDown) {
            if (ctrlDown)
              MoveLeftWord(view, TRUE, posX, posY);
            else
              SelPosXY(view, posX - 1, posY);
            goto end;
        }
        break;

    case VK_RIGHT:
        if (shiftDown) {
            if (ctrlDown)
              MoveRightWord(view, TRUE, posX, posY);
            else
              SelPosXY(view, posX + 1, posY);
            goto end;
        }
        break;

    case VK_UP:
        if (shiftDown) {
            SelPosXY(view, Pix2Pos(view, Pos2Pix(view, posX, posY), posY - 1),
                     posY - 1);
            goto end;
        }
        break;

    case VK_DOWN:
        if (shiftDown) {
            if ((posY + 1) < (Docs[v->Doc].NbLines)) {
                SelPosXY(view, Pix2Pos(view, Pos2Pix(view, posX, posY), posY + 1),
                         posY + 1);
                //goto end;
            }
                        goto end;
        }
        break;

    case VK_PRIOR:
        if (shiftDown) {
            scrollOrigin = FROM_KEYBOARD | SELECTING;
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
            scrollOrigin = FROM_MOUSE;
            goto end;
        }
        break;

    case VK_NEXT:

        if (shiftDown) {
            scrollOrigin = FROM_KEYBOARD | SELECTING;
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
            scrollOrigin = FROM_MOUSE;
            goto end;
        }
        break;

    case VK_HOME:
        if (shiftDown) {
            if (ctrlDown)
              SelPosXY(view, 0, 0);
            else {
                int X = FirstNonBlank(v->Doc, posY);

                if (posX != X)
                  SelPosXY(view, X, posY);
                else
                  SelPosXY(view, 0, posY);
            }
            goto end;
        }
        break;

    case VK_END:
        if (shiftDown) {
            if (ctrlDown)
              SelPosXY(view,
                       GetLineLength(view, FALSE, Docs[v->Doc].NbLines - 1),
                       Docs[v->Doc].NbLines-1);
            else
              SelPosXY(view,
                       GetLineLength(view, TRUE, posY),
                       posY);
            goto end;
        }
        break;

    case VK_F1:
        {
            BOOL lookAround = TRUE;

            if (GetCurrentText(curView, &lookAround, (LPSTR)szTmp,
                               MAX_USER_LINE, NULL, NULL)) {

                Dbg(WinHelp(hwndFrame, szHelpFileName, HELP_PARTIALKEY,
                            (DWORD_PTR)(LPSTR)szTmp));


            }
            else
              MessageBeep(0);
            break;
        }

    case VK_F3:
        if (ctrlDown) {
            BOOL lookAround = TRUE;
            BOOL error = FALSE;

            if (GetCurrentText(curView, &lookAround,
                               (LPSTR)findReplace.findWhat,
                               MAX_USER_LINE, &frMem.leftCol,
                               &frMem.rightCol)) {

                //Cursor is not on a word, so start search from cursor
                  if (lookAround)
                    frMem.leftCol = frMem.rightCol = v->X;

                InsertInPickList(FIND_PICK);

                //Give control to modeless dialog box if active

                  if (frMem.hDlgFindNextWnd) {
                      SetFocus(frMem.hDlgFindNextWnd);
                      SendMessage(frMem.hDlgFindNextWnd, WM_COMMAND, IDOK, 0L);
                  }
                  else if (frMem.hDlgConfirmWnd) {

                      SetFocus(frMem.hDlgConfirmWnd);
                      SendMessage(frMem.hDlgConfirmWnd, WM_COMMAND, ID_CONFIRM_FINDNEXT, 0L);
                  } else {
                      FindNext(hwnd, v->Y, frMem.rightCol, v->BlockStatus, TRUE, TRUE);
                  }
            } else if (StartDialog(DLG_FIND, DlgFind)) {
                Find();
            }
        }
        break;
    }

    switch (wParam) {

    case VK_LEFT:
        ClearSelection(view);
        if (ctrlDown)
          MoveLeftWord(view, FALSE, posX, posY);
        else
          PosXY(view, posX - 1, posY, FALSE);
        goto end;

    case VK_RIGHT:
        ClearSelection(view);
        if (ctrlDown)
          MoveRightWord(view, FALSE, posX, posY);
        else
          PosXY(view, posX + 1, posY, FALSE);
        goto end;

    case VK_UP:
        ClearSelection(view);
        if (ctrlDown) {
            int         bottomLine;
            RECT        rc;
            int         yPos;

            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0L);
            GetClientRect(hwnd, &rc);
            bottomLine = ((rc.bottom - 1) / v->charHeight);

            yPos = v->iYTop;
            if (yPos == -1) {
                yPos = GetScrollPos(hwnd, SB_VERT);
            }
            if (posY >= yPos + bottomLine) {
                posY--;
            }
            PosXY(view, v->X, posY, FALSE);
        } else {
            PosXY(view, Pix2Pos(view, Pos2Pix(view, posX, posY), posY - 1),
                  posY - 1, FALSE);
        }
        goto end;

    case VK_DOWN:
        ClearSelection(view);
        if (ctrlDown) {
            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0L);
            yPos = v->iYTop;
            if (yPos == -1) {
                yPos = GetScrollPos(hwnd, SB_VERT);
            }
            posY = maxi(yPos, posY);
            PosXY(view, v->X, posY, FALSE);
        } else if (posY < (Docs[v->Doc].NbLines-1)) {
            PosXY(view, Pix2Pos(view, Pos2Pix(view, posX, posY), posY + 1),
                  posY + 1, FALSE);
        }
        goto end;

    case VK_PRIOR:
        ClearSelection(view);
        if (ctrlDown) {
            RECT rcl;

            GetClientRect (hwnd, &rcl);
            posX = Pix2Pos(view, Pos2Pix(view, posX, posY) - rcl.right, posY);
            PosXY(view, posX, v->Y, FALSE);
        } else {
            scrollOrigin = FROM_KEYBOARD;
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
            scrollOrigin = FROM_MOUSE;
        }
        goto end;

    case VK_NEXT:
        ClearSelection(view);
        if (ctrlDown) {
            RECT rcl;

            GetClientRect (hwnd, &rcl);
            posX = Pix2Pos(view, Pos2Pix(view, posX, posY) + rcl.right, posY);
            PosXY(view, posX, v->Y, FALSE);
        } else {
            scrollOrigin = FROM_KEYBOARD;
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
            scrollOrigin = FROM_MOUSE;
        }
        goto end;

    case VK_HOME:
        ClearSelection(view);
        if (ctrlDown) {
            PosXY(view, 0, 0, FALSE);
        } else {
            int X = FirstNonBlank(v->Doc, posY);

            if (posX != X) {
                PosXY(view, X, posY, FALSE);
            } else {
                PosXY(view, 0, posY, FALSE);
            }
        }
        goto end;

    case VK_END:
        ClearSelection(view);
        
        if (ctrlDown) {
            PosXY(view,
                  GetLineLength(view, FALSE, Docs[v->Doc].NbLines - 1),
                  Docs[v->Doc].NbLines - 1, 
                  FALSE
                  );
        } else if (((nLine = GetLineLength(view, TRUE, posY)) == 1) && 
            (Docs[Views[curView].Doc].docType == COMMAND_WIN)) {
            
            PosXY(view, GetLineLength(view, TRUE, posY) + 1, posY, FALSE);
        } else {
            PosXY(view, GetLineLength(view, TRUE, posY), posY, FALSE);
        }
        goto end;

    case VK_RETURN:
        {
            BOOL overtype;
            BOOL prevStopMark = stopMarkStatus;

            //Ctrl+Return is like return when overtyping


            overtype = GetOverType_StatusBar();
            SetOverType_StatusBar(FALSE);


            //Insert CR/LF at current cursor position if not Overtyping
              //or if cursor beyond file when overtyping

                if (( (GetOverType_StatusBar() == FALSE) &&
                     (Docs[v->Doc].forcedOvertype == FALSE))
                    || ((Docs[v->Doc].docType == DOC_WIN)
                        && (posY == Docs[v->Doc].NbLines - 1)
                        && (posX >= GetLineLength(view, TRUE, posY))))
                  {
                      if (InsertStream(view, posX, posY, 2, (LPSTR)CrLf, TRUE)) {
                          LPLINEREC pl;
                          LPBLOCKDEF pb;
                          int i = 0;

                          //Restore cursor to initial Y position (if some text was selected)

                            posY = v->Y;

                          //Copy blank chars from previous line

                            if (!FirstLine(v->Doc, &pl, &posY, &pb)) {
                                return;
                            }

                          posX = 0;
                          while (i < pl->Length - LHD)  {
                              if (IsDBCSLeadByte(pl->Text[i])) {
                                  break;
                              } else
                              if (pl->Text[i] == ' ') {
                                  posX++;
                              } else if (pl->Text[i] == TAB) {
                                  posX += (g_contGlobalPreferences_WkSp.m_nTabSize - posX % g_contGlobalPreferences_WkSp.m_nTabSize);
                              } else {
                                  break;
                              }
                              i++;
                          }
                          CloseLine(v->Doc, &pl, posY, &pb);

                          //If line starts with blanks, pad blanks to autoindent newly inserted line

                              if (i > 0) {
                                  //More than 1 edit action for this, operation. Tell the
                                    //undo/redo engine that this record have no stop marks

                                      stopMarkStatus = HAS_NO_STOP;

                                  InsertStream(view, 0, posY, i, (LPSTR)pl->Text, FALSE);

                                  //Put undo/redo engine back in normal state

                                    if (prevStopMark == HAS_STOP) {
                                        stopMarkStatus = HAS_STOP;
                                    }

                              }

                          //Only to avoid desynchonization between return key
                            //hold generating a bunch of vk_return and scroll bar
                              //messages processed less often

                                SetVerticalScrollBar(view, FALSE);

                          //Reposition cursor

                                PosXY(view, posX, posY, FALSE);
                      }
                  } else  {
                      //Avoid to move cursor below last line when overtyping

                        if (++posY >= Docs[v->Doc].NbLines) {
                            goto enough;
                        }

                      //Reposition cursor at 1st char of next line

                        PosXY(view, FirstNonBlank(v->Doc, posY), posY, FALSE);

                  }

        enough:
            // Ctrl+Return reset real overtype status

            SetOverType_StatusBar(overtype);

            goto end;
        }

    case VK_DELETE:
        DeleteKey(view);
        goto end;

    case VK_BACK:
        {
            long XL,XR;
            long YL,YR;

            GetBlockCoord(view, &XL, &YL, &XR, &YR);
            if (v->BlockStatus && (XL != XR || YL != YR)) {
                if (DeleteStream(view, XL, YL, XR, YR, TRUE)) {
                    PosXY(view, XL, YL, FALSE);
                }
            } else {
                if (posX > 0) {

                    LPLINEREC pl;
                    LPBLOCKDEF pb;
                    BOOL bDBCS;

                    if (!FirstLine(v->Doc, &pl, &posY, &pb))
                      goto end;
                    posY--;
                    XL = AdjustPosX(posX - 1, FALSE);
                    bDBCS = IsDBCSLeadByte((BYTE)(el[XL])) ? TRUE : FALSE;
                    if (DeleteStream(view, posX - 1, posY, posX, posY, TRUE)) {
                        if (bDBCS) {
                            SetReplaceDBCSFlag(&Docs[v->Doc], FALSE);
                        }
                        PosXY(view, XL, posY, FALSE);
                    }
                } else if (posY > 0) {

                    int X = GetLineLength(view, TRUE, posY - 1);

                    if (DeleteStream(view, X, posY - 1, posX, posY, TRUE)) {
                        PosXY(view, X, posY - 1, FALSE);
                    }
                }
            }
            goto end;
        }

    case VK_TAB:
        {
            int NewX;
            long XL = 0,XR = 0;
            long YL = 0,YR = 0;
            BOOL prevStopMark = stopMarkStatus;
            int len;

            //Do we have a selection ?

              if (v->BlockStatus) {

                  int startY;

                  GetBlockCoord(view, &XL, &YL, &XR, &YR);
                  startY = YL;

                  //If selection is on one line only, it's a normal TAB

                if (YL == YR && XR < MAX_USER_LINE)
                      goto normal;

                //Otherwise, we shift selection

                DestroyRecBuf(v->Doc, REC_REDO);

                //If we are at col 0 of last line, exclude it

                if (XR == 0)
                      YR--;

                //Resize selection to include all the lines moved

                v->BlockXL = 0;
                v->BlockYL = startY;
                v->BlockXR = MAX_USER_LINE;
                v->BlockYR = YR;

                //More than 1 edit action for this, operation. Tell the
                //undo/redo engine that records have no stop marks after
                //this record

                stopMarkStatus = NEXT_HAS_NO_STOP;

                if (shiftDown) {

                    //Unindent lines selected

                    while (startY <= YR) {

                        if (!DeleteBlock(v->Doc, 0, startY,
                              mini((int)g_contGlobalPreferences_WkSp.m_nTabSize,
                              FirstNonBlank(v->Doc, startY)),
                              startY))
                              goto err;
                        startY++;
                    }
                } else {

                    char tmp[MAX_TAB_WIDTH];

                    len = 1;
                    tmp[0] = TAB;

                    //Indent lines selected

                    while (startY <= YR) {
                        if (!InsertBlock(v->Doc, 0, startY, len, (LPSTR)tmp))
                                break;
                        startY++;
                    }
                }

                InvalidateLines(view, YL, YR, TRUE);

                //Put undo/redo engine back in normal state

                if (prevStopMark == HAS_STOP)
                      stopMarkStatus = HAS_STOP;
            } else {

              normal:

                //Handle Shift Tab

                if (shiftDown) {
                    if (posX > 0) {
                        ClearSelection(view);
                        PosXY(view, (posX - 1) - (posX - 1) % g_contGlobalPreferences_WkSp.m_nTabSize, posY, FALSE);
                    }
                    goto end;
                }

                if (v->BlockStatus) {

                    if (GetOverType_StatusBar())
                          ClearSelection(view);
                    else {

                        //More than 1 edit action for this, operation. Tell
                        //the undo/redo engine that records have no stop
                        //marks after this record

                        stopMarkStatus = NEXT_HAS_NO_STOP;

                        DeleteStream(view, XL, YL, XR, YR, TRUE);
                        posX = XL;
                        posY = YL;
                    }
                }

                //Compute new position

                NewX = (posX - (posX % g_contGlobalPreferences_WkSp.m_nTabSize)) + g_contGlobalPreferences_WkSp.m_nTabSize;
                if (NewX > MAX_USER_LINE)
                      NewX = MAX_USER_LINE;

                len = 1;
                szTmp[0] = TAB;

                if (NewX != posX) {

                    if (!GetOverType_StatusBar()
                          && InsertStream(view, posX, posY, len, (LPSTR)szTmp, TRUE))
                    {
                        PosXY(view, NewX, posY, FALSE);
                    }
                }

                //Put undo/redo engine back in normal state

                if (prevStopMark == HAS_STOP)
                      stopMarkStatus = HAS_STOP;
            }
        }
    }

  end:
    return;

  err:
    Assert(FALSE);
    return;
}                                       /* KeyDown() */


/***    PressChar
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
//A key has being pressed and has been translated to a valid character**
*/



void FAR PASCAL
PressChar(
          HWND hwnd,
          WPARAM wParam,
          LPARAM lParam
          )
/*++

Routine Description:

    This routine is called in response to a WM_CHAR message.  It will
    handle any keyboard messages not dealt with in KeyDown

Arguments:

    hwnd        - Supplies the handle of the window for the message
    wParam      - Supplies the wParam field of the message
    lParam      - Supplies the lParam field of the message

Return Value:

    None.

--*/

{
    int         view = GetWindowWord(hwnd, GWW_VIEW);
    LPVIEWREC   v = &Views[view];
    LPDOCREC    d = &Docs[v->Doc];
    int         yPos;
    static  BOOL bDBCS = FALSE;
    static  BYTE szBuf[4];

    Assert(v->Doc >= 0);

    if ((lParam & (1 << 31)) != 0 || IsIconic(GetParent(hwnd))) {
        return;
    }
    if (bDBCS) {
        bDBCS = FALSE;
        szBuf[1] = (BYTE)wParam;
        szBuf[2] = '\0';

        if ((GetOverType_StatusBar() || d->forcedOvertype)
        && !v->BlockStatus) {
            LPLINEREC   pl;
            LPBLOCKDEF  pb;

            /**********************************************/
            /* if over-write mode and no text is selected */
            /**********************************************/
            if (!FirstLine(v->Doc, &pl, &v->Y, &pb)) {
                return;
            }
            CloseLine(v->Doc, &pl, v->Y, &pb);
            v->Y--;

            v->BlockStatus = TRUE;
            v->BlockXL = v->X;
            v->BlockYL = v->Y;
            v->BlockYR = v->Y;
            if (IsDBCSLeadByte((BYTE)(el[v->X]))) {
                v->BlockXR = v->X + 2;
            } else if (v->bDBCSOverWrite) {
                if (v->X + 2 < elLen) {
                    if (IsDBCSLeadByte((BYTE)(el[v->X + 1]))) {
                        szBuf[2] = ' ';
                        szBuf[3] = '\0';
                        v->BlockXR = v->X + 3;
                    } else {
                        v->BlockXR = v->X + 2;
                    }
                } else {
                    v->BlockXR = v->X + 1;
                }
            } else {
                v->BlockXR = v->X + 1;
            }

            if (InsertStream(view, v->X, v->Y, lstrlen( (PSTR) szBuf), (PSTR) szBuf, TRUE)) {
                SetReplaceDBCSFlag(d, TRUE);
                PosXY(view, v->X + lstrlen( (PSTR) szBuf), v->Y, FALSE);
            }
        } else {
            if (InsertStream(view, v->X, v->Y, 2, (PSTR) szBuf, TRUE)) {
                PosXY(view, v->X + 2, v->Y, FALSE);
            }
        }
        return;
    }


    switch(wParam) {

    //Those are Handled in KeyDown
    case BS:
    case TAB:
    case CTRL_M:
    case CTRL_X:
    case CTRL_V:
        break;

        //Find matching {[( ... )]}
    case CTRL_RIGHTBRACKET:
        FindMatching(view);
        break;

    //Normal char or unknown CTRL chars
    default:
        if (IsDBCSLeadByte((BYTE)wParam)) {
            szBuf[0] = (BYTE)wParam;
            bDBCS = TRUE;
            break;
        }
        if (wParam >= ' ') {

            BOOL ok;

            //Replace or insert char at cursor location

            if (GetOverType_StatusBar() || d->forcedOvertype) {
                LPLINEREC   pl;
                LPBLOCKDEF  pb;
                BOOL        bDBCS;

                if (!FirstLine(v->Doc, &pl, &v->Y, &pb)) {
                    break;
                }
                CloseLine(v->Doc, &pl, v->Y, &pb);
                v->Y--;

                szBuf[0] = (BYTE)wParam;
                szBuf[1] = '\0';

                bDBCS = IsDBCSLeadByte((BYTE)(el[v->X])) ? TRUE : FALSE;
                if (bDBCS && !v->BlockStatus) {
                    if (v->bDBCSOverWrite) {
                        szBuf[1] = ' ';
                        szBuf[2] = '\0';
                    }
                    v->BlockStatus = TRUE;
                    v->BlockXL = v->X;
                    v->BlockXR = v->X + 2;
                    v->BlockYL = v->Y;
                    v->BlockYR = v->Y;
                }
                if (bDBCS || v->BlockStatus) {
                    if (ok = InsertStream(view, v->X, v->Y,
                        lstrlen( (PSTR) szBuf), (PSTR) szBuf, TRUE)) {
                        if (bDBCS) {
                            SetReplaceDBCSFlag(d, TRUE);
                        }
                    }
                } else {
                    ok = ReplaceChar(view, v->X, v->Y, (int) wParam, TRUE);
                }
            } else {
                ok = InsertStream(view, v->X, v->Y, 1, (LPSTR)&wParam, TRUE);
            }

            //Reposition Cursor

            if (ok) {
                PosXY(view, v->X + 1, v->Y, FALSE);
            }
        }
        else {
            MessageBeep(0);
        }
        break;

    }

    return;
}                                       /* PressChar() */



/***    SetRORegion
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


void SetRORegion (int view, int Xro, int Yro)
{
 LPVIEWREC v = &Views[view];
 LPDOCREC d = &Docs[v->Doc];

 Assert(v->Doc >= 0);

  //Set DOCREC structures

    if (Xro == 0 && Yro == 0)
     d->RORegionSet = FALSE;
      else
       d->RORegionSet = TRUE;


    d->RoX2 = Xro;
    d->RoY2 = Yro;

}   /* SetRORegion () */




/***    GetRORegion
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


BOOL GetRORegion (int view, LPINT Xro, LPINT Yro)
{
 LPVIEWREC v = &Views[view];
 LPDOCREC d = &Docs[v->Doc];

 Assert(v->Doc >= 0);

 if (d->RORegionSet == TRUE)
  {
   //Get DOCREC structures

    *Xro = d->RoX2;
    *Yro = d->RoY2;

   return TRUE;
  }
  else
   return FALSE;

}   /* GetRORegion () */

