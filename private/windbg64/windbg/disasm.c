/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    disasm.c

Abstract:

    This file contains the code which controls the disassembly window.
    It is a normal MDI window.  The creation of each line of disassembly
    is done by OSDEBUG.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

/************************** INCLUDE FILES *******************************/

#include "precomp.h"
#pragma hdrstop

#include <ime.h>




/************************** Structs and Defines *************************/
/************************** Internal Prototypes *************************/

int DsmGetBackAddress(void);
void DisasmAlignInstr(ADDR *);

/************************** Data declaration    *************************/


ADDR    AddrDisasm;
UINT    CbDisasm;
int     StDisasm = DISASM_NONE;
int     CLinesDisasm = 0;
LONGLONG UlBaseOff;

extern  CXF     CxfIp;
extern  LPSHF   Lpshf;


/**********************************************************************/

void    DisasmSetVScroll(VOID);
void    DoVertScroll(HWND, UINT, WPARAM, LPARAM);


/**************************       Code          *************************/


void
PASCAL
ViewDisasm(
    LPADDR lpaddr,
    int cmd
    )

/*++

Routine Description:

    This function will cause the disassembly window to be voided and
    refreshed in the edit manager.  It reconstucts or adds to the existing
    buffer.

Arguments:

    lpaddr      - Supplies an address which is used in the disassembly

    cmd         - Supplies the operation to be preformed on the data

Return Value:

    None.

--*/

{
    int         doc = Views[disasmView].Doc;
    RECT        rc;
    int         cLine;
    SDI         sds;
    int         x;
    int         y;
    int         i;
    int         cb;
    LPVIEWREC   v = &Views[disasmView];
    BOOL        fDisplay = FALSE;
    BOOL        fHighlight = FALSE;
    BOOL        fBreakpt = FALSE;
    HBPT        hbpt;
    BPSTATUS    bpstatus;
    ADDR        addrT;
    ADDR        addrPC;
    ADDR        addrT2;
    int         iLineFirst = v->iYTop;
    int         iPC = -1;
    int         iForce = -1;
    int         iLineCur = v->Y;
    OFFSET      offStart;
    static BOOL fRecurse = FALSE;

    /*
    *  Prevent recursive calls to this routine
    */

    if (fRecurse) {
        return;
    }
    fRecurse = TRUE;

    /*
    *  Assert that we really have a window to play with
    */

    Assert(disasmView != -1);

    /*
    *  Don't bother if the window is currently iconized
    */

    if (IsIconic(GetParent(Views[disasmView].hwndClient))) {
        fRecurse = FALSE;
        return;
    }

    /*
    *  If no child active then clean out the existing listing and
    *          return with out any thing else happening
    */

    if (!DebuggeeActive() || LptdCur == NULL) {
        v->Y = v->iYTop = 0;
        DeleteAll(doc);
        if (UlBaseOff != 0) {
            SetScrollRange(Views[disasmView].hwndClient, SB_VERT, 0, 0, FALSE);
            UlBaseOff = 0;
        }
        fRecurse = FALSE;
        return;
    }

    if (UlBaseOff == 0) {
        SetScrollRange(Views[disasmView].hwndClient, SB_VERT,
            0, 8*1024, FALSE);
        SetScrollPos(Views[disasmView].hwndClient, SB_VERT, 0, TRUE);
    }

    /*
    *  Compute the number of lines for display in the window
    */

    GetClientRect(Views[disasmView].hwndClient, &rc);
    cLine = rc.bottom / v->charHeight;

    /*
    *  Use some minimum size to keep small windows from going crazy
    */

    if (cLine == 0) {
        cLine = 1;
    }

    /*
    *  If the screen size if larger than the number of lines cached
    *  then force redisplay
    */

    if (cLine > CLinesDisasm) {
        fDisplay = TRUE;
    }

    /*
    *  Look at all of the paging commands to see if we have a suffiecnt
    *  amount of data cached in the edit buffers.
    */

    if ((cmd & disasmDownPage) == disasmDownPage) {
        if (iLineFirst +  2*cLine + 2 > CLinesDisasm) {
            fDisplay = TRUE;
            cLine = CLinesDisasm + cLine + 2;
        } else {
            fRecurse = FALSE;
            return;
        }
    } else if ((cmd & disasmDownLine) == disasmDownLine) {
        if (iLineFirst + cLine + 1 > CLinesDisasm) {
            fDisplay = TRUE;
            cLine = CLinesDisasm + cLine + 2;
        } else {
            fRecurse = FALSE;
            return;
        }
    } else if ((cmd & disasmUpPage) == disasmUpPage) {
        if (iLineFirst >= cLine) {
            fRecurse = FALSE;
            return;
        } else {
            i = DsmGetBackAddress();
            iLineFirst += i;
            iLineCur += i;
            cLine = CLinesDisasm + i;
            fDisplay = TRUE;
        }
    } else if ((cmd & disasmUpLine) == disasmUpLine) {
        if (iLineFirst > 0) {
            fRecurse = FALSE;
            return;
        } else {
            i = DsmGetBackAddress();
            iLineFirst += i;
            iLineCur += i;
            cLine = CLinesDisasm + i;
            fDisplay = TRUE;
        }
    } else if ((cmd & disasmHighlight) == disasmHighlight ) {
        cLine    = __max(cLine,CLinesDisasm);
        fDisplay = TRUE;
    }

    /*
    *  If we are suppose to have the PC in the current window then it
    *  was passed in as our parmeter.  Check to see if PC is
    *  in the current range if display
    */

    if (lpaddr != NULL) {
        SYFixupAddr(lpaddr);
    }

    if (cmd & disasmRefresh) {
        fDisplay = TRUE;
        for (i=iLineFirst; i< Docs[doc].NbLines ; i++) {
            if (DisasmGetAddrFromLine(&addrT2, i)) {
                StDisasm = -1;
                AddrDisasm = addrT2;
                break;
            }
        }
        iLineCur = iLineCur - iLineFirst;
        iLineFirst = 0;
    }


    {
        BOOL bSegAlwaysZero = FALSE;

        if ((StDisasm == DISASM_NONE) ||
            (cmd & disasmForce)) {
            fDisplay = TRUE;
            DisasmGetAddrFromLine(&AddrDisasm, iLineFirst, &bSegAlwaysZero);
            iLineFirst = iLineCur = 0;
        }

        if ((cmd & disasmForce) &&
            ((lpaddr->emi != AddrDisasm.emi) ||
            (!bSegAlwaysZero && AddrDisasm.addr.seg != lpaddr->addr.seg) ||
            (AddrDisasm.addr.off > lpaddr->addr.off) ||
            (AddrDisasm.addr.off + CbDisasm <= lpaddr->addr.off))) {

            AddrDisasm = *lpaddr;
            StDisasm = -1;
            UlBaseOff = ((AddrDisasm.addr.off + 2*1024 - 1) & ~(4*1024-1)) - 4*1024;
            iLineFirst = iLineCur = 0;

            // The entire contents of the disasm window have been blown
            // away, and the highlight is about to be placed at the top of
            // the window, but we want it to be more like the source
            // window, so the highlight will be placed 2 lines from the
            // top if the window is large enough.

            if (cLine > 5) {
                for (int i=0; i<2; i++) {
                    PostMessage(Views[disasmView].hwndClient, WM_VSCROLL,
                        MAKEWPARAM(SB_LINEUP, 0), (LPARAM) Views[disasmView].hwndClient);
                }
            }
        }
    }

    /*
    *  Get the set of parameters which control the display of the
    *  disassembly window
    */

    if (fDisplay) {
        v->Y = v->iYTop = 0;
        DeleteAll(doc);
        sds.dop = (g_contWorkspace_WkSp.m_dopDisAsmOpts & ~(0x800)) | dopAddr |
            dopOpcode | dopOperands;
    } else {
        sds.dop = 0;
    }

    if ((StDisasm == DISASM_NONE) &&
        !((cmd & disasmPC) || (cmd & disasmForce))) {
        fRecurse = FALSE;
        return;
    }

    /*
    *  Initialize the address and disassembler query structures
    */

    sds.addr = AddrDisasm;

    /*
    *  Grab a copy of the PC address so that we can check for a
    *  current PC highlight.
    */

    addrT2 = addrPC = *SHpADDRFrompCXT(&CxfIp.cxt);
    SYFixupAddr(&addrPC);

    /*
    *  Now start doing the dump into the edit window.  Put a label
    *  here in case when we finish doing the dump we have not managed
    *  to get a desired address in the window.
    */

again:

    offStart = sds.addr.addr.off;

    /*
    *  Dump the info into the buffer
    */

    for (i=0, y=0; i<cLine; i++, y++) {
    /*
    *      Check to see if the current display offset is the
    *      same as that of the program counter.
        */

        if (addrPC.addr.off == sds.addr.addr.off) {
            fHighlight = TRUE;
        }

        /*
        *      Check to see if the current display offset has a
        *      breakpoint set.
        */

        memcpy(&addrT, &sds.addr, sizeof(ADDR));
        bpstatus = BPHbptFromAddr(&addrT, &hbpt);
        fBreakpt = (bpstatus == BPNOERROR) || (bpstatus == BPAmbigous);

        /*
        *      Now check to see if we have a label on the current line.
        */

        if (fDisplay) {
            char rgchSymbol[512];
            ADDR        addrT = sds.addr;
            LPCH        lpchSymbol;
            ODR         odr;

            addrT.emi = 0;
            ADDR_IS_LI(addrT) = FALSE;
            SYUnFixupAddr(&addrT);

            odr.lszName = rgchSymbol;

            lpchSymbol = SHGetSymbol(&addrT, &addrT2, sopNone, &odr);
            // Check to see if SHGetSymbol screwed us over
            Assert(strlen(rgchSymbol) < sizeof(rgchSymbol));

            if ((lpchSymbol != NULL) && (odr.dwDeltaOff == 0)) {
                x = strlen(rgchSymbol);
                InsertBlock(doc, 0, y, x, rgchSymbol);
                InsertBlock(doc, x, y, 3, ":\r\n");
                y += 1;
                LineStatus(doc, y, DASM_LABEL_LINE, LINESTATUS_ON, FALSE, FALSE);
            }
        } else {
            if (QueryLineStatus(doc, y+1, DASM_LABEL_LINE)) {
                y += 1;
            }
        }

        /*
        * Check for force addresses
        */

        if (cmd & disasmForce) {
            if (lpaddr->addr.off == sds.addr.addr.off) {
                iForce = y;
            }
        }

        /*
        *  Now finally get down to doing the actual disassembly.  We
        *      call the EM to get most of the information and then
        *      just worry about the display
        */

        if (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sds) == xosdNone) {
            if (fDisplay) {
                x = 0;
                InsertBlock(doc, x, y, 2, (LPSTR) "\r\n");

                if (sds.ichAddr != -1) {
                    cb = strlen(&sds.lpch[sds.ichAddr]);
                    InsertBlock(doc, x, y, cb, &sds.lpch[sds.ichAddr]);
                    x = cb+1;
                }

                if (sds.ichBytes != -1) {
                    cb = strlen(&sds.lpch[sds.ichBytes]);
                    InsertBlock(doc, x, y, cb, &sds.lpch[sds.ichBytes]);
                    x += __max(17, cb+1); // M00TODO -- 32-bits should be 17
                }
                
                if ((LppdCur->mptProcessorType == mptia64) && (sds.ichPreg != -1)) { 
                    cb = strlen(&sds.lpch[sds.ichPreg]);
                    InsertBlock(doc, x, y, cb, &sds.lpch[sds.ichPreg]);
                    x += __max(5, cb+1);
                }

                if (sds.ichOpcode != -1) {
                    cb = strlen(&sds.lpch[sds.ichOpcode]);
                    InsertBlock(doc, x, y, cb, &sds.lpch[sds.ichOpcode]);
                    x += __max(12, cb+1);
                }
                else {
                    InsertBlock(doc, x, y, 3, "???");
                    x += 12;
                }

                if (sds.ichOperands != -1) {
                    cb = strlen(&sds.lpch[sds.ichOperands]);
                    InsertBlock(doc, x, y, cb, &sds.lpch[sds.ichOperands]);
                    x += __max(25, cb+1);
                }

                if (sds.ichComment != -1) {
                    cb = strlen(&sds.lpch[sds.ichComment]);
                    InsertBlock(doc, x, y, cb, &sds.lpch[sds.ichComment]);
                    x += __max(20, cb+1);
                }
            }

            /*
            *  Deal with highlighting
            */

            if (fHighlight) {
                LineStatus(doc, y+1, CURRENT_LINE, LINESTATUS_ON,
                    FALSE, !fDisplay);


                fHighlight = FALSE;
                iPC = y;
            } else if (QueryLineStatus(doc, y+1, CURRENT_LINE)) {
                LineStatus(doc, y+1, CURRENT_LINE, LINESTATUS_OFF,
                    FALSE, !fDisplay);

            }

            if (fBreakpt) {
                LineStatus(doc, y+1, BRKPOINT_LINE, LINESTATUS_ON,
                    FALSE, !fDisplay);
            } else if (QueryLineStatus(doc, y+1, BRKPOINT_LINE)) {
                LineStatus(doc, y+1, BRKPOINT_LINE, LINESTATUS_OFF,
                    FALSE, !fDisplay);
            }

        } else {
        }
    }

    /*
    *  If we are doing a pc update then we must have
    *  this address displayed on the screen
    */

    if ((cmd & disasmPC) && (iPC == -1) && fDisplay) {
        AddrDisasm = *lpaddr;
        sds.addr = AddrDisasm;
        cmd &= ~disasmPC;
        goto again;
    }

    /*
    *  Check to see if FORCE made it
    */

    if ((cmd & disasmForce) && (iForce == -1) && fDisplay) {
        AddrDisasm = *lpaddr;
        sds.addr = AddrDisasm;
        cmd &= ~disasmForce;
        goto again;
    }

    /*
    *  Deal with force and none redisplay
    */

    if ((cmd & DISASM_PC) == DISASM_PC) {
        if (iLineFirst > iPC) {
            iLineFirst = iPC;
        } else if (iLineFirst + cLine < iPC) {
            iLineFirst = iPC;
        }
        iLineCur = iPC;

        if (fDisplay) {
            ;
        } else {
            v->iYTop = iLineFirst;
            InvalidateLines(disasmView, 0, LAST_LINE, FALSE);
        }
    }

    //
    //  Force a full repaint of the screen
    //

    if (fDisplay) {
        v->iYTop = iLineFirst;
        InvalidateLines(disasmView, 0, LAST_LINE, FALSE);

        CbDisasm = (UINT) (sds.addr.addr.off - AddrDisasm.addr.off);
        CLinesDisasm = cLine;
    }

    if (iLineCur != -1) {
        iLineCur = min(iLineCur, Docs[v->Doc].NbLines-1);
        v->Y = iLineCur;
        if (curView == disasmView) {
            SetCaret(disasmView, v->X, v->Y, -1);
        }
    }

    DisasmSetVScroll();
    fRecurse = FALSE;
    return;
}                                       /* ViewDisasm() */


/***    DisasmGetAddrFromLine
**
**  Synopsis:
**      bool = DisasmGetAddrFromLine( lpAddr, iLine )
**
**  Entry:
**      lpAddr  - pointer to address structure to fill in
**      iLIne   - line number to get address of
**      pbSegAlwaysZero - May be NULL.
**
**           This is dependent on the format of the
**           string passed into the function. With some formats, the segment
**           is always zero, with others, the segment may or may not be zero.
**
**           TRUE - Indicates that the segment of the address will always be
**                  0 regardless of the value passed in.
**           FALSE - The segment may or may not be zero.
**
**  Return:
**      TRUE if an address was retrived else FALSE
**
**  Description:
**      This function is used to get the address corresponding to
**      a line in the disassembler window.
*/

BOOL
DisasmGetAddrFromLine(
                      LPADDR lpAddr,
                      DWORD iLine,
                      PBOOL pbSegAlwaysZero
                      )
{
    int         doc = Views[disasmView].Doc;
    LPBLOCKDEF  pb;
    LPLINEREC   pl;
    long        first = iLine;
    char FAR *  lpch;
    char        ch;
    char        rgch[50];

    /*
    **     Check to see if there is assembler or source code on this line
    **         don't allow to goto source code only to assembler
    */

    FirstLine(doc, &pl, &first, &pb);

    lpch = &pl->Text[0];
    if ((*lpch < '0') || (*lpch > '9')) {
        return FALSE;
    }

    while ((*lpch != 0) &&
        ((('0' <= *lpch) && (*lpch <= '9')) ||
        (('a' <= *lpch) && (*lpch <= 'f')) ||
        (('A' <= *lpch) && (*lpch <= 'F')) ||
        ('x' == *lpch) ||
        (':' == *lpch) ||
        ('#' == *lpch))) {
        lpch++;
    }

    ch = *lpch;
    *lpch = 0;

    strcpy(rgch, &pl->Text[0]);
    *lpch = ch;


    if (CPUnformatAddr(lpAddr, rgch, pbSegAlwaysZero) != CPNOERROR) {
        return FALSE;
    }

    SYUnFixupAddr(lpAddr);
    SYFixupAddr(lpAddr);

    return TRUE;
}                                       /* DisasmGetAddrLine() */



void
DisasmSetVScroll(
    VOID
    )

/*++

Routine Description:

    This routine is used to set the vertical scroll bar position.  It
    will get the first line which has an address and then position
    the scroll bar according to that address.  Additionally it will
    adjust the scroll bar range if necessary according to the address.

Arguments:

    none

Return Value:

    None.

--*/

{
    LONGLONG    q;
    long        i;
    ADDR        addr;

    /*
    *  Check that there is something in the window first.
    */

    if ((disasmView == -1) ||
        (Docs[Views[disasmView].Doc].NbLines < 1)) {
        return;
    }

    /*
    *  Get the first address after the top line in the window
    */

    for (i=0; TRUE; i++) {
        if (Docs[Views[disasmView].Doc].NbLines <= i+Views[disasmView].iYTop) {
            return;
        }

        if (DisasmGetAddrFromLine( &addr, i+Views[disasmView].iYTop )) {
            break;
        }
    }

    /*
    *  Now set the position in the scroll bar
    */

    q = addr.addr.off - UlBaseOff;

    if ((q < 2*1024) || (q > 6*1024)) {
        UlBaseOff = ((addr.addr.off + 2*1024 - 1) & ~(4*1024-1)) - 4*1024;
        q = addr.addr.off - UlBaseOff;

    }


    SetScrollPos(Views[disasmView].hwndClient, SB_VERT, (long)q, TRUE);
}                               /* DisasmSetVScroll() */



void
DoVertScroll(
    HWND       hwnd,
    UINT       uMsg,
    WPARAM     wParam,
    LPARAM     lParam
    )

/*++

Routine Description:

    This routine is used to process the vertical scroll messages for the
    disassembly window

Arguments:

    hwnd        - Supplies the handle to the disassmbly window

    uMsg        - Original window message. Used to distinguish between the 
                    "mouse wheel" and the scroll bar

    wParam      - Supplies the wParam of the WM_VSCROLL message

    lparam      - Supplies the lParam of the WM_VSCROLL message

Return Value:

    None.

--*/

{
    int         vScrollInc;
    int         vScrollPos;
    RECT        rcl;
    int         lines;
    int         vWinLines;
    LPVIEWREC   v = &Views[disasmView];
    int         newY;
    ADDR        addr;

    /*
    *  Get info we need
    */

    vScrollPos = v->iYTop;
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
            if (vWinLines > 6) {
                vScrollInc = -vWinLines + 3;
            } else {
                vScrollInc = -vWinLines;
            }
            break;
    
        case SB_PAGEDOWN:
            if (vWinLines > 6) {
                vScrollInc = vWinLines - 3;
            } else {
                vScrollInc = vWinLines;
            }
            break;
    
        case SB_THUMBPOSITION:
            memcpy(&addr, &AddrDisasm, sizeof(ADDR));
            SYFixupAddr(&addr);
            // align the offset to 32 bit machine word as PPC/MIPS/ALPHA
            // instructions are 32 bits wide each
            // It does not matter to the x86 as it always disasm -0x100
            // ahead to get in sync.
            addr.addr.off = (UlBaseOff + HIWORD(wParam)) & ~0x3;
            DisasmAlignInstr(&addr); // realign starting address in x86 case
            Unreferenced( lParam );
            ViewDisasm(&addr, disasmForce);
            return;
    
        default:
            return;
    
        }
    }

    //
    // Some adjustments may be necessary to deal with the fact
    //  that the window is created on the fly.
    //
    //  Are we paging to some place which is prior to the current
    //  start of the window.  If so then we need to generate some
    //  more information at the start of the file.
    //

    if (vScrollPos + vScrollInc < 0) {
        do {
            ViewDisasm(NULL, disasmUpPage);
            vScrollPos = v->iYTop;
            if (AddrDisasm.addr.off == 0) {
                break;
            }
            if (lines == Docs[v->Doc].NbLines - 1) {
                break;
            }
            lines = Docs[v->Doc].NbLines - 1;
        } while (vScrollPos + vScrollInc < 0);
        lines = Docs[v->Doc].NbLines - 1;
    } else if (vScrollPos + vScrollInc + vWinLines > Docs[v->Doc].NbLines - 1) {
        do {
            ViewDisasm(NULL, disasmDownPage);
            DAssert(vScrollPos == v->iYTop);
            if (Docs[v->Doc].NbLines == 1) {
                return;
            }
            if (lines == Docs[v->Doc].NbLines - 1) {
                break;
            }
            lines = Docs[v->Doc].NbLines - 1;
        } while (vScrollPos + vScrollInc + vWinLines > Docs[v->Doc].NbLines - 1);
        lines = Docs[v->Doc].NbLines - 1;
    }


    //
    //  Continue with normal code VScroll code
    //

    newY = min(lines, v->Y + vScrollInc);

    if (vScrollInc < 0) {
        vScrollInc = max(vScrollInc, -vScrollPos);
    } else {
        lines -=(vScrollPos + vWinLines);
        if ((rcl.bottom + v->charHeight - 1) % v->charHeight) {
            lines++;
        }
        vScrollInc = max(min(vScrollInc, lines), 0);
    }
    vScrollPos += vScrollInc;

    if (vScrollInc != 0) {

        //
        //     Set scroll-bar positions before refreshing the screen because
        //      we use the scroll-bar positions during the PAINT event
        //

        v->iYTop = vScrollPos;

        ScrollWindow(hwnd, 0, - v->charHeight * vScrollInc, NULL, NULL);
        UpdateWindow(hwnd);

    }

#if 0
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
            InvalidateLines(view, min(OldYR, v->BlockYR), max (OldYR, v->BlockYR), FALSE);
        } else {
            PosXY(view, v->X, newY, FALSE);
        }
    }
#endif // 0
    return;
}                               /* DoVertScroll() */

/***    DisasmEditProc
**
**  Synopsis:
**      long = DisasmEditProc(hwnd, msg, wParam, lParam)
**
**  Entry:
**      hwnd    - window handle to the disassembly window
**      msg     - Message to be processes
**      wParam  - info about the message
**      lParam  - info about the message
**
**  Returns:
**
**  Description:
**      This function is the window message processor for the disassembly
**      window class.  It processes those messages which are of interest
**      to this specific window class and passes all other messages on to
**      the default MDI window procedure handler
**
*/

LRESULT
FAR PASCAL EXPORT
DisasmEditProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch ( uMsg ) {
    case WU_INITDEBUGWIN:
        StDisasm = DISASM_NONE;
        memset(&AddrDisasm, 0, sizeof(AddrDisasm));
        UlBaseOff = 1;                 /* Force the scroll bar to be hidden */
        Views[disasmView].iYTop = 0;   /* Give use a line to start with     */
        if (DebuggeeActive()) {
            UlBaseOff = 0;
            ViewDisasm(SHpADDRFrompCXT(&CxfIp.cxt), DISASM_PC);
        }
        break;

    case WM_KEYDOWN:
        switch ( wParam ) {
        case VK_PRIOR:
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
            return FALSE;

        case VK_NEXT:
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
            return FALSE;

        case VK_UP:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
            return FALSE;

        case VK_DOWN:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
            return FALSE;

        case VK_HOME:
            ViewDisasm(SHpADDRFrompCXT(&CxfIp.cxt), DISASM_PC);
            return FALSE;
        }
        return FALSE;

        case WM_IME_REPORT:
            if (IR_STRING == wParam) {
                return TRUE;
            }
            break;

        case WM_CHAR:
            return FALSE;

        case WM_MOUSEWHEEL:
        case WM_VSCROLL:
            DoVertScroll(hwnd, uMsg, wParam, lParam);
            //SetFocus (hwnd);
            return FALSE;

        case WM_FONTCHANGE:
            ViewDisasm(&AddrDisasm, DISASM_OTHER);
            return FALSE;

        case WM_SIZE:
            CallWindowProc(lpfnEditProc, hwnd, uMsg, wParam, lParam);
            if (wParam != SIZEICONIC) {
                ViewDisasm(&AddrDisasm, DISASM_OTHER);
            }
            WindowTitle( disasmView, 0 );
            return FALSE;

    }
    return CallWindowProc(lpfnEditProc, hwnd, uMsg, wParam, lParam);
}                                       /* DisasmWndProc() */

/***    DsmGetBackAddress
**
**  Synopsis:
**      int = DsmGetBackAddress()
**
**  Entry:
**      None
**
**  Returns:
**      count of lines in the added space
**
**  Description:
**
*/

int
DsmGetBackAddress()
{
    SDI         sdi;
    int         i;
    ADDR        addr = {0};
    ADDR        addr2 = {0};
    ADDR        addrT2 = {0};
    char        rgchSymbol[512];
    ODR         odr;
    char *      lpchSymbol;

    sdi.addr = AddrDisasm;
    sdi.dop = 0;
    if (sdi.addr.addr.off < 0x100) {
        sdi.addr.addr.off = 0;
    } else {
        sdi.addr.addr.off -= 0x100;

        //
        //  disassemble 10 instructions in an attempt to syncronize with
        //  the true code stream
        //

        for (i=0; i<10; i++) {
            if (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sdi) == xosdNone) {
            } else {
                Assert(FALSE);
            }
        }
    }

    addr = sdi.addr;
    odr.lszName = rgchSymbol;
    for (i=0; TRUE; i++) {
        if (sdi.addr.addr.off >= AddrDisasm.addr.off) {
            AddrDisasm = addr;
            return( i );
        }
        addr2 = sdi.addr;
        addr2.emi = 0;
        ADDR_IS_LI(addr2) = FALSE;
        SYUnFixupAddr(&addr2);

        lpchSymbol = SHGetSymbol(&addr2, &addrT2, sopNone, &odr);
        // Check to see if SHGetSymbol screwed us over
        Assert(strlen(rgchSymbol) < sizeof(rgchSymbol));

        if ((lpchSymbol != NULL) && (odr.dwDeltaOff == 0)) {
            i += 1;
        }

        if (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sdi) != xosdNone) {
            Assert(FALSE);
        }
    }
    Assert(FALSE);
    return( 0 );
}                                       /* DsmGetBackAddress() */

/***    DisasmAlignInstr
**
**  Synopsis:
**      DisasmAlignInstr
**
**  Entry:
**      Starting address
**
**  Returns:
**      None
**
**  Description:
**      Modify address offset to match instructions boundary if necessary.
*/

void
DisasmAlignInstr(
    ADDR *curraddr
    )
{
    SDI         sdi;
    int         i;

    sdi.addr = *curraddr;
    sdi.dop = 0;
    if (sdi.addr.addr.off < 0x100) {
        curraddr->addr.off = 0;
        return;
    } else {
        sdi.addr.addr.off -= 0x100;

        //
        //  disassemble 10 instructions in an attempt to syncronize with
        //  the true code stream
        //

        for (i=0;i<0x100;i++) {
            if (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sdi) == xosdNone) {
            } else {
                Assert(FALSE);
            }
            if (sdi.addr.addr.off >= curraddr->addr.off) {
                *curraddr = sdi.addr;
                return;
            }
        }
    }
}

