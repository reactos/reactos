/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    localwin.c

Abstract:

    This module contains the routines to manipulate the Locals Window

Author:

    William J. Heaton (v-willhe) 20-Jul-1992
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32, User Mode

--*/


/*
 *  Preprocessor
 */

#include "precomp.h"
#pragma hdrstop

BOOL AcceptValueUpdate(PTRVIT pVit,PPANE p);


/*
 *  Global Memory (PROGRAM)
 */


extern CXF   CxfIp;  // The Current IP Context
extern LPSHF Lpshf;  // Pointer to SH function vector


/*
 *  Global Memory (FILE)
 */

HWND   hWndLocal;
PTRVIT pvitLocal;

/*
 *  Prototypes (Local)
 */

BOOL PASCAL NEAR GetLocalAtContext ( PTRVIT pvit, PCXF pcxf);
BOOL UpdateLocalsVit(PCXF pCxf, BOOL Expand1st);
/*
 *  Start of Code
 */


/***    GetLocalHWND
**
**  Synopsis:
**      hWnd = GetLocalHWND()
**
**  Entry:
**      None
**
**  Returns:
**      Pointer to the current watch window handle.
**
*/

HWND GetLocalHWND(VOID)
{
    return(hWndLocal);
}


/***    GetLocalAtContext Return a local linked list for the specified context
**
**  Purpose:
**    Make a locals linked list for a given context
**
**  Input:
**      pvit  - pointer to a vit packet to where the linked list is to start
**      pctxt - pointer to a ctxt of the context.
**
**  Output:
**      pvit - is filled in and the linked list is allocated and generated.
**
**  Returns
**      TRUE/FALSE has the context changed.
**
**
**  Exceptions:
**
**
*/

BOOL
PASCAL
GetLocalAtContext (
    PTRVIT pvit,
    PCXF pcxf
    )
{
    VIB      vibT;
    PTRVIB   pvibCur;
    int      RetStatus = FALSE;         // Default is context hasn't changed
    EESTATUS RetErr;
    CXF      cxf;
    HMEM     hsyml = 0;
    ULONG    strIndex;

    // If no Vit, we can't do anything (and the context has changed!)

    if ( !pvit ) {
        return(FALSE);
    }

    // If no Offset or Segment, same treatment

    if ((GetAddrOff (*SHpADDRFrompCXT (SHpCXTFrompCXF (pcxf))) == 0) &&
        (GetAddrOff (*SHpADDRFrompCXT (SHpCXTFrompCXF (pcxf))) == 0)) {
        return(FALSE);
    }

    // copy context and frame and check to see if the context can
    // handle locals.

    cxf = *pcxf;

    if (!ADDR_IS_LI (*SHpADDRFrompCXT (SHpCXTFrompCXF (&cxf))))
        SYUnFixupAddr (SHpADDRFrompCXT (SHpCXTFrompCXF (&cxf)));


    //
    //  If we don't have a context (no proc or no module) or we are
    //  still in the prolog -- we don't display anything
    //

    if (!( cxf.cxt.hProc && cxf.cxt.hMod) ||
        SHIsInProlog(SHpCXTFrompCXF(&cxf))) {
        pvit->cxf.cxt.hMod  = 0;
        pvit->cxf.cxt.hGrp  = 0;
        pvit->cxf.cxt.hProc = 0;
        pvit->cxf.cxt.hBlk  = 0;
        pvit->cln           = 0;
        FTFreeAllSib( pvit->pvibChild );
        pvit->pvibChild = NULL;
        return(TRUE);
    }


    if ( (pvit->cxf.cxt.hBlk == cxf.cxt.hBlk) &&
         (pvit->cxf.cxt.hProc == cxf.cxt.hProc)) {
        SHhFrameFrompCXF(&pvit->cxf) = SHhFrameFrompCXF(&cxf);
        return(FALSE);
    }


    // Release the Locals tree

    FTFreeAllSib( pvit->pvibChild );
    pvit->pvibChild = NULL;
    RetStatus = TRUE;

    // fill in the Variable Info block Top and clear line count

    pvit->cxf = cxf;
    pvit->cln = 0;

    // make a dummy Variable Info Block to start the search

    pvibCur = &vibT;
    pvibCur->pvibSib = pvit->pvibChild;

    // make the linked list

    EEGetHSYMList ( &hsyml, &cxf.cxt, HSYMR_lexical + HSYMR_function,
                                                            NULL, TRUE );

    if ( hsyml != (HMEM) NULL ) {
        PHSL_HEAD lphsymhead;
        PHSL_LIST lphsyml;
        WORD i = 0;
        WORD j = 0;

        // get the syms
        lphsymhead = (PHSL_HEAD) MMLpvLockMb ( hsyml );
        lphsyml = (PHSL_LIST)(lphsymhead + 1);

        for ( i = 0; i != lphsymhead->blockcnt; i++ ) {
            for ( j = 0; j != lphsyml->symbolcnt; j++ ) {

                // is this a displayable sym?
                if ( SHCanDisplay ( lphsyml->hSym[j] ) ) {

                    HSYM hSym = lphsyml->hSym[j];

                    // get the new Variable Info Block. To maintain the tree, the old
                    //   Variable Info Block is returned if it exists.

                    if ( !( pvibCur->pvibSib = FTvibGet ( pvibCur->pvibSib, (PTRVIB) pvit) ) ) {
                        goto nomoresyms;
                    }

                    // check to see if this Variable Info Block is an old vib that
                    // isn't correct anymore. We know offset is enough of a check
                    // because we checked the module above.  If it is clear the vib chain.

                    if( pvibCur->pvibSib->vibPtr != vibUndefined  &&
                        pvibCur->pvibSib->hSym != hSym ) {
                        FTvibInit(pvibCur->pvibSib, (PTRVIB) pvit);
                    }

                    // if we have a new tree, we must assign stuff

                    if ( pvibCur->pvibSib->vibPtr == vibUndefined ) {
                        DAssert (hSym != 0);
                        RetErr = EEGetTMFromHSYM(hSym,&pcxf->cxt,&pvibCur->pvibSib->hTMBd,
                                                 &strIndex, TRUE, TRUE);

                        if ( RetErr != EENOERROR && RetErr != EECATASTROPHIC ) {
                            pvibCur->pvibSib = NULL;
                            goto nextsym;
                        }

                        // assign this stuff only once

                        pvibCur->pvibSib->hSym   = hSym;
                        pvibCur->pvibSib->vibPtr = vibSymbol;

                    }

                    // update the parent's (Variable Info block Top) number count
                    // the vit is alway initialized to zero

                    pvibCur = pvibCur->pvibSib;
                    pvibCur->pvibParent->cln += pvibCur->cln;
nextsym:
                    ;   // must have a statement after a label
                }
            }
            lphsyml = (PHSL_LIST) &(lphsyml->hSym[j]);
        }

nomoresyms:

        MMbUnlockMb ( hsyml );
    }

    EEFreeHSYMList ( &hsyml );

    // free any extra Variable Info Blocks

    FTFreeAllSib ( pvibCur->pvibSib );
    pvibCur->pvibSib = NULL;

    // load in the child pointer from the dummy vit

    pvit->pvibChild = vibT.pvibSib;
    return(RetStatus);
}                   /* GetLocalAtContext() */




/*
**  void = UpdateLocalsVit(pCxf)
**
**  Entry:
**  pCxf    - pointer to context frame or Null if we are to use the
**            Current IP context frame.
**
**  Returns:
**  Nothine
**
**  Description:
**
*/

BOOL
UpdateLocalsVit(
    PCXF pCxf,
    BOOL Expand1st
    )
{
    PTRVIB pvib;

    if ( pCxf == NULL ) {
        pCxf = (PCXF)(&CxfIp);
    }

    if ( GetLocalAtContext(pvitLocal, pCxf) ) {

        // Mark all top-level if need b
        if ( Expand1st ) {
            pvib = pvitLocal->pvibChild;
            while ( pvib) {
                pvib->flags.ExpandMe = TRUE;
                pvib = pvib->pvibSib;
            }
        }
        return(TRUE);
    }

    return(FALSE);
}                   /* UpdateLocalsVit() */


/***    InitLocalsVits
**
**  Synopsis:
**  bool = InitLocalsVits()
**
**  Entry:
**  Nothing
**
**  Returns:
**  TRUE if the initialization was successful else FALSE
**
**  Description:
**  This routine will allocate the initial structures which are used
**  to contain information describing the contents of the locals window
*/


BOOL NEAR PASCAL InitLocalsVits(void)
{
    pvitLocal = (PTRVIT) calloc(1, sizeof(VIT) );
    return ( pvitLocal != NULL);

}


/***    FreeLocalsVits
**
**  Synopsis:
**  void = FreeLocalsVits()
**
**  Entry:
**  Nothing
**
**  Returns:
**  None
**
**  Description:
**  This function frees up the structures which describe the contents
**  of the locals window
**
*/

void NEAR PASCAL FreeLocalsVits(void)
{
    FTFreeAllSib(pvitLocal->pvibChild);
    pvitLocal->pvibChild = NULL;
    free(pvitLocal);
    pvitLocal = NULL;
}



/***    LocalEditProc
**
**  Synopsis:
**      long = LocalEditProc(hwnd, msg, wParam, lParam)
**
**  Entry:
**      hwnd    - handle to window to process message for
**      msg     - message to be processed
**      wParam  - information about message to be processed
**      lParam  - information about message to be processed
**
**  Returns:
**
**  Description:
**      MDI function to handle Localwindow messages
**
*/

LRESULT
WINAPI
LocalEditProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PPANE         p     = (PPANE)lParam;
    PCXF          pCxf  = (PCXF)wParam;
    PPANEINFO     pInfo = (PPANEINFO)wParam;
    LONG          Len = 0;
    LONG          lLen = 0;
    PTRVIB        pVib = NULL;
    PVTEXT        pvtext;
    ULONG         i;
    RECT          Rect, tRect;
    HWND          hFoc;
    HCURSOR       hOldCursor, hWaitCursor;


    hFoc = GetFocus();

    __try {

        switch (msg) {

            case WU_INITDEBUGWIN:
                Dbg(InitLocalsVits());
                hWndLocal = hwnd;
                PostMessage(hwnd, WU_UPDATE, 0, 0L);
                break;

            case WM_DESTROY:
                hWndLocal = NULL;   //  Lose the Local Window Handle
                //
                // fall thru...
                //

            case WU_DBG_UNLOADEE:
            case WU_DBG_UNLOADEM:
                if ( pvitLocal) {
                    FreeLocalsVits();      // Lose the Local Trees
                    SendMessage(p->hWndLeft, LB_SETCOUNT, 1, 0);
                    SendMessage(p->hWndButton, LB_SETCOUNT, 1, 0);
                    SendMessage(p->hWndRight, LB_SETCOUNT, 1, 0);
                    p->MaxIdx = (WORD)1;
                    PostMessage(hwnd, WU_UPDATE, 0, 0L);
                }
                break;

            case WU_DBG_LOADEM:
            case WU_DBG_LOADEE:
                if (!pvitLocal) {
                    Dbg(InitLocalsVits());
                    hWndLocal = hwnd;
                    PostMessage(hwnd, WU_UPDATE, 0, 0L);
                }
                break;

            case WU_OPTIONS:
                pVib = FTvibGetAtLine( pvitLocal, pInfo->ItemId);
                if ( pVib == NULL) {
                    return FALSE;
                }

                pvtext = &pVib->pvtext[pVib->vibIndex];

                if ( pInfo->pFormat &&  (*(pvtext->pszValueC) != '{')) {
                    FTEnsureTextExists(pVib);

                    if ( pvtext->pszFormat) {
                        free(pvtext->pszFormat);
                    }
                    pvtext->pszFormat = _strdup(pInfo->pFormat);

                    if ( pvtext->pszValueC) {
                        free(pvtext->pszValueC);
                    }
                    pvtext->pszValueC = NULL;

                    PaneInvalidateRow(p);

                } else {

                    FTEnsureTextExists(pVib);

                    if ( pvtext->pszFormat) {
                        free(pvtext->pszFormat);
                    }
                    pvtext->pszFormat = '\0';

                    if ( pvtext->pszValueC) {
                        free(pvtext->pszValueC);
                    }
                    pvtext->pszValueC = NULL;

                    PaneInvalidateRow(p);
                }
                return TRUE;

            case WU_INFO:
                //
                // Default to a reasonable baseline
                //
                pInfo->pBuffer  = pInfo->pFormat = NULL;
                pInfo->NewText  = FALSE;
                pInfo->ReadOnly = TRUE;

                pVib = FTvibGetAtLine( pvitLocal, pInfo->ItemId);
                if ( pVib == NULL) {
                    return(FALSE);
                }

                FTEnsureTextExists(pVib);
                pvtext = &pVib->pvtext[pVib->vibIndex];

                pInfo->pFormat = pvtext->pszFormat;
                pInfo->pBuffer  = FTGetPanelString( pvitLocal, pVib, pInfo->CtrlId);

                if ( pInfo->CtrlId == ID_PANE_RIGHT) {
                    pInfo->ReadOnly = FALSE;
                    pInfo->NewText  = FTGetPanelStatus( pVib, pInfo->CtrlId);
                } else {
                    pInfo->ReadOnly = TRUE;
                    pInfo->NewText  = FALSE;
                }
                return TRUE;

            case WU_SETWATCH:
                if ( pvitLocal == NULL) {
                    return(FALSE);
                }
                if ( p->nCtrlId == ID_PANE_RIGHT) {
                    BOOL retval;
                    fUseFrameContext = TRUE;
                    retval = (AcceptValueUpdate( pvitLocal, p));
                    if (retval == TRUE) {
                         UpdateDebuggerState(UPDATE_DATAWINS);
                    }
                    fUseFrameContext = FALSE;
                    return retval;
                }
                break;


            case WU_INVALIDATE:
                if (p == (PPANE)NULL) {
                    p = (PPANE)GetWindowLongPtr(GetLocalHWND(), GWW_EDIT);
                }

                SendMessage(p->hWndLeft, LB_SETCOUNT, 0, 0);
                SendMessage(p->hWndButton, LB_SETCOUNT, 0, 0);
                SendMessage(p->hWndRight, LB_SETCOUNT, 0, 0);
                p->MaxIdx = 0;
                PostMessage(hwnd, WU_UPDATE, 0, 0L);

                InvalidateRect(p->hWndButton, NULL, TRUE);
                InvalidateRect(p->hWndLeft, NULL, TRUE);
                InvalidateRect(p->hWndRight, NULL, TRUE);
                UpdateWindow (p->hWndButton);
                UpdateWindow (p->hWndLeft);
                UpdateWindow (p->hWndRight);
                break;

            case WU_EXPANDWATCH:
                if ( pvitLocal == NULL) {
                    return(FALSE);
                }
                if ( FTExpand(pvitLocal, (ULONG)(wParam)) != OK) {
                    return(FALSE);
                }

                p->LeftOk = p->RightOk = FALSE;
                pCxf = &pvitLocal->cxf;            // Update in vit context
                //
                // fall thru...
                //

            case WU_UPDATE:

                if ( pvitLocal == NULL) {
                    return(FALSE);
                }

                if (!pCxf) {
                    pCxf = &CxfIp;
                }

                //
                //  Has the Context Changed?
                //
                if ( UpdateLocalsVit(pCxf, p->bFlags.Expand1st) || p->LeftOk == FALSE ) {

                    hWaitCursor = LoadCursor ( (HINSTANCE) NULL, IDC_WAIT);
                    hOldCursor = SetCursor (hWaitCursor);

                    //
                    // Do we need to expand first level?
                    //
                    if ( p->bFlags.Expand1st) {
                        FTExpandOne(pvitLocal->pvibChild);
                    }

                    Len = (LONG)pvitLocal->cln;
                    p->MaxIdx = (WORD)Len;
                    lLen = (long) SendMessage(p->hWndLeft, LB_GETCOUNT, 0, 0L);
                    lLen = (lLen < Len || lLen == 0) ? Len : lLen;

                    SendMessage( p->hWndLeft,   LB_SETCOUNT, lLen, 0L );
                    SendMessage( p->hWndButton, LB_SETCOUNT, lLen, 0L );
                    SendMessage( p->hWndRight,  LB_SETCOUNT, lLen, 0L );

                    //
                    //  Resetting the count, lost where we were so put us back
                    //
                    if (p->MaxIdx > 0) {
                        PaneResetIdx(p, p->CurIdx);
                    }

                    p->LeftOk = TRUE;

                } else {

                    // Set hourglass cursor
                    hWaitCursor = LoadCursor ( (HINSTANCE) NULL, IDC_WAIT);
                    hOldCursor = SetCursor (hWaitCursor);

                }

                //  Reset the right pane

                FTAgeVibValues(pvitLocal->pvibChild);

                for ( i= (ULONG)p->TopIdx;
                           i < (ULONG)(p->TopIdx + p->PaneLines) ; i++) {
                    if (FTVerifyNew(pvitLocal,i) ) {
                        PaneInvalidateItem( p->hWndRight, (PPANE)p, (SHORT)i);
                    }
                }

                p->RightOk = TRUE;

                PaneCaretNum(p);

                if ((hFoc == p->hWndButton) ||
                         (hFoc == p->hWndLeft) || (hFoc == p->hWndRight)) {
                    SendMessage(p->hWndButton ,
                                LB_GETITEMRECT,
                                (WPARAM)p->CurIdx,
                                (LPARAM)&Rect);
                    GetClientRect (p->hWndButton, &tRect);
                    tRect.top = Rect.top;
                    InvalidateRect(p->hWndButton, &tRect, TRUE);

                    SendMessage(p->hWndLeft ,
                                LB_GETITEMRECT,
                                (WPARAM)p->CurIdx,
                                (LPARAM)&Rect);
                    GetClientRect (p->hWndLeft, &tRect);
                    tRect.top = Rect.top;
                    InvalidateRect(p->hWndLeft, &tRect, TRUE);

                    SendMessage(p->hWndRight ,
                                LB_GETITEMRECT,
                                (WPARAM)p->CurIdx,
                                (LPARAM)&Rect);
                    GetClientRect (p->hWndRight, &tRect);
                    tRect.top = Rect.top;
                    InvalidateRect(p->hWndRight, &tRect, TRUE);
                }


            CheckPaneScrollBar( p, (WORD)Len);

            // Set original cursor
            hOldCursor = SetCursor (hOldCursor);

            break;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;

    }

    return FALSE;
}                                       /* LocalEditProc() */
