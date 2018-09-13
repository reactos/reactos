/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    watchwin.c

Abstract:

    This module contains the routines to manipulate the Watch Window

Author:

    William J. Heaton (v-willhe) 20-Jul-1992
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


/*
 *  Global Memory (PROGRAM)
 */

extern CXF      CxfIp;
char   output[512];
/*
 *  Global Memory (FILE)
 */

PTRVIT pvitWatch;
HWND   hWndWatch;

VOID WatchUpdate( PPANE p, BOOL fClear, WPARAM wParam);
BOOL AcceptValueUpdate(PTRVIT pVit,PPANE p);

extern LRESULT  SendMessageNZ (HWND,UINT,WPARAM,LPARAM);


/***    InitWatchVit
**
**  Synopsis:
**      pVit = InitWatchVit()
**
**  Entry:
**      None
**
**  Returns:
**     Returns a pointer to the current vit (Allocating one if needed).
**     return NULL if can't
**
**  Description:
**
**     Creates the Vit (Variable Information Top) block for the
**     watch window
**
*/

PTRVIT PASCAL InitWatchVit(void)
{
    if (pvitWatch == NULL) {
        pvitWatch = (PTRVIT) calloc(1, sizeof(VIT));
    }
    return (pvitWatch);
}                                       /* InitWatchVit() */

/***    GetWatchVit
**
**  Synopsis:
**      pVit = GetWatchVit()
**
**  Entry:
**      None
**
**  Returns:
**      Pointer to the current watch vit.
**
*/

PTRVIT GetWatchVit(VOID)
{
    return(pvitWatch);
}

/***    GetWatchHWND
**
**  Synopsis:
**      hWnd = GetWatchHWND()
**
**  Entry:
**      None
**
**  Returns:
**      Pointer to the current watch window handle.
**
*/

HWND GetWatchHWND(VOID)
{
    return(hWndWatch);
}

/***    UpdateCVWatchs
**
**  Synopsis:
**      UpdateCVWatchs()
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Forces the WatchWindow (If Present) to update ALL of its
**      Panels.
**
*/
VOID UpdateCVWatchs()
{
    PPANE p;

    if ( hWndWatch ) {
        p = (PPANE)GetWindowLongPtr(hWndWatch, GWW_EDIT );
        p->LeftOk = p->RightOk = FALSE;
    }

    UpdateDebuggerState( UPDATE_WATCH );
    return;
}

/***    AddCVWatch
**
**  Synopsis:
**      pVib =  AddCVWatch(PSTR szExpStr)
**
**  Entry:
**     szExpStr - A pointer to the string containing the entry to
**                be added.
**
**  Returns:
**     Return a pointer to the vib created or NULL;
**
**  Description:
**
**      Adds a expression to the watch window.  Returns the VIB created.
**
*/

PTRVIB AddCVWatch(PTRVIT pVit, PSTR szExpStr)
{
    VIB vibT = {0};
    PTRVIB pvib;

    // check to see if the watch vit is there yet or not
    // this is for the state file.

    if ( pVit == (PTRVIT)NULL) {
        FTError(UNABLETOADDWATCH);
        return(NULL);
    }

    // make a dummy vib
    vibT.pvibSib = pVit->pvibChild;
    pvib = &vibT;

    // find the end of the chain
    while( pvib->pvibSib != NULL) {
        pvib = pvib->pvibSib;
    }

    // set the current context
    if( FTMakeWatchEntry(&pvib->pvibSib, pVit, szExpStr) ) {
        return(NULL);
    }

    // add this entry to the vit
    FTclnUpdateParent(pvib->pvibSib->pvibParent, 1);

    // restore the vit
    pVit->pvibChild = vibT.pvibSib;
    return(pvib->pvibSib);
}                                       /* AddCVWatch() */


/***    DeleteCVWatch
**
**  Synopsis:
**      LTS = DeleveCVWatch(PTRVIT pVit, PTRVIB pvib)
**
**  Entry:
**      pVib    - Pointer to the VIB that contains the watch to
**                delete.
**
**  Returns:
**      Returns a LTS status code (defined in vib.h)
**
**  Description:
**      Given a pointer to a vib, find it and delete it from the
**      watch tree.
**
*/

LTS DeleteCVWatch(PTRVIT pVit, PTRVIB pvib)
{
    VIB         vibT;
    PTRVIB      pvibPrev;

    // make sure this is a watch to be deleted
    if (!(pvib && (pvib->vibPtr == vibWatch
        || pvib->vibPtr == vibType))) {

        return UNABLETODELETEWATCH;
    }

    // starting at the head of the list, find the previous vib
    vibT.pvibSib = pVit->pvibChild;
    pvibPrev = &vibT;

    while (pvibPrev->pvibSib != pvib) {
        pvibPrev = pvibPrev->pvibSib;
    }

    // remove this the deleted one from the chain
    pvibPrev->pvibSib = pvib->pvibSib;
    pvib->pvibSib = NULL;

    // update the the vit with the new chain
    pVit->pvibChild = vibT.pvibSib;

    // delete all members of this vib from the parent count
    FTclnUpdateParent(pvib->pvibParent, -pvib->cln );

    // delete this vib, and all children
    FTFreeAllSib(pvib);
    pvib = NULL;

    return OK;
}                                       /* DeleteCVWatch() */



/***    ReplaceCVWatch
**
**  Synopsis:
**      bool = ReplaceCVWatch( pvit, pvib, szStr)
**
**  Entry:
**      pvib  - Pointer to the VIB whos expression is changing
**      szStr - Pointer to the New expression string.
**
**  Returns:
**      TRUE or FALSE
**
**  Description:
**      Replaces the expression strings of the indicated watch
**      entry with the new string.
*/

BOOL ReplaceCVWatch(PTRVIT pVit, PTRVIB pvib, PSTR szStr)
{
    PTRVIB  *ppvib;

    //
    // Find The parent VIB
    //

    ppvib = &pVit->pvibChild;
    while ( *ppvib && *ppvib != pvib ) {
        ppvib = &(*ppvib)->pvibSib;
    }


    //
    // Make a New child for the parent
    //

    if ( ! FTMakeWatchEntry ( ppvib, pVit, szStr) ) {

        // New Child gets the Old Child's siblings
        FTVerify(&pVit->cxf,*ppvib);
        (*ppvib)->pvibSib = pvib->pvibSib;
        FTclnUpdateParent( (*ppvib)->pvibParent, (*ppvib)->cln );


        // Remove the Old Child
        FTclnUpdateParent(pvib->pvibParent, -pvib->cln);
        pvib->pvibSib = NULL;
        FTFreeAllSib(pvib);
        pvib = (PTRVIB) NULL;
        return(TRUE);
    } else {
        //
        // Couldn't add the new watch, make sure the old stays around
        *ppvib = pvib;
        return(FALSE);
    }
}


/***    AcceptWatchUpdate
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

BOOL AcceptWatchUpdate(PTRVIT pVit,PPANE p, WPARAM wParam)
{
    PSTR        pszBuffer;
    PTRVIB      EditVib;
    HWND        hWnd = p->hWndLeft;
    ULONG       Line = (LONG)p->CurIdx;
    BOOL        retVal = FALSE;


    //
    // Get the current line number, and contents, find its vib
    //

    pszBuffer = p->EditBuf;
    while ( isspace(*pszBuffer) ) {
        pszBuffer++;
    }

    EditVib = FTvibGetAtLine(pVit,Line);


    //
    // Does a Vib exist for this line, if so we're deleting or changing it
    //

    if ( EditVib ) {

        //
        //  It only makes sense to modify level zero things.
        //
        if (EditVib->level == 0 ) {

            if ( *pszBuffer ) {
                retVal = ReplaceCVWatch( pVit, EditVib, pszBuffer);

                if (p->bFlags.Expand1st)
                    EditVib->flags.ExpandMe = TRUE;

                p->LeftOk = p->RightOk = FALSE;
                WatchUpdate(p, FALSE, wParam);
            }
            else {

                // Blanking the name means remove the watch
                DeleteCVWatch(pVit, EditVib);

                // We Need to repaint, watch count changed
                p->LeftOk = p->RightOk = FALSE;
                WatchUpdate(p, FALSE, wParam);
                retVal = TRUE;
            }
        }
    } else {

        //
        // Vib doesn't exist for this line create it.
        //

        if (*pszBuffer && (EditVib = AddCVWatch(pVit, pszBuffer)) != NULL) {
            p->CurIdx++;

            if (p->bFlags.Expand1st) {
                EditVib->flags.ExpandMe = TRUE;
            }

            // We Need to repaint, watch count changed
            p->LeftOk = p->RightOk = FALSE;
            WatchUpdate(p, FALSE, wParam);
            retVal = TRUE;
        }
    }

    return retVal;
}                                       /* AcceptWatchUpdate() */


/***    AcceptValueUpdate
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

BOOL AcceptValueUpdate(PTRVIT pVit,PPANE p)
{
    PSTR        pszBuffer;
    PTRVIB      pVib;
    EESTATUS    eeErr;
    HTM         hTm;
    ULONG       strIndex;
    VPI         i;
    EEHSTR      hstr;
    DWORD       cb;
    char *      lpsz;
    EERADIX     uradix = radix;

    // If we're not editing, we're done (sucessfully)
    if ( !p->Edit ) {
        return(TRUE);
    }

    /*
    *  Get the expression represented by the left pane
    */

    pVib = FTvibGetAtLine(pVit,p->CurIdx);
    if (pVib->hTMBd == 0) {
        hTm = pVib->pvtext[pVib->vibIndex].htm;
    } else {
        hTm = pVib->hTMBd;
    }

    if (EEGetExprFromTM(&hTm, &uradix, &hstr, &cb) != EENOERROR) {
        return FALSE;
    }
    if (cb >= sizeof(output)-3) {
        return FALSE;
    }
    lpsz = (PSTR) MMLpvLockMb( hstr );
    output[0] = '(';
    strncpy(&output[1], lpsz, cb);
    strcpy(&output[cb+1], ")=");
    MMbUnlockMb( hstr );
    EEFreeStr( hstr );

    // Get the New values
    pszBuffer = p->EditBuf;
    DAssert(pszBuffer);

    while ( isspace(*pszBuffer) ) {
        pszBuffer++;
    }

    if ((strlen(pszBuffer) + cb + 3) > sizeof(output)) {
        return FALSE;
    }

    strcat(output, pszBuffer);

    // Parse, Bind, and Evaluate
    eeErr = EEParse(output, radix, fCaseSensitive, &hTm, &strIndex);
    if (eeErr == EENOERROR) {
        eeErr = EEBindTM(&hTm, SHpCXTFrompCXF(&CxfIp), TRUE, FALSE);
    }
    if (eeErr == EENOERROR) {
        eeErr = EEvaluateTM(&hTm, SHhFrameFrompCXF(&CxfIp), EEHORIZONTAL);
    }
    if (hTm) {
        EEFreeTM(  &hTm );
    }

    if (eeErr == EENOERROR) {
        i = pVib->vibIndex;

        FTEnsureTextExists(pVib);

        if(pVib->pvtext[i].pszValueP) {
            free(pVib->pvtext[i].pszValueP);
        }

        pVib->pvtext[i].pszValueP = pVib->pvtext[i].pszValueC;
        pVib->pvtext[i].pszValueC = NULL;
        return(TRUE);
    }

    MessageBeep(0);
    return(FALSE);
}                                       /* AcceptValueUpdate() */


/***    WatchEditProc
**
**  Synopsis:
**      lresult = WatchEditProc(hwnd, msg, wParam, lParam)
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
**      MDI function to handle Watch window messages
**
*/

LRESULT
CALLBACK
WatchEditProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PPANE     p     = (PPANE)lParam;
    PPANEINFO pInfo = (PPANEINFO)wParam;
    PTRVIB    pVib  = NULL;
    PVTEXT    pvtext;


    __try {

        switch (msg) {
        case WU_INITDEBUGWIN:
            Dbg(InitWatchVit());
            hWndWatch = hwnd;

            SendMessage(p->hWndLeft, LB_SETCOUNT, 1, 0);
            SendMessage(p->hWndButton, LB_SETCOUNT, 1, 0);
            SendMessage(p->hWndRight, LB_SETCOUNT, 1, 0);
            p->MaxIdx = (WORD)1;
            WatchUpdate(p, FALSE, wParam);
            break;

        case WM_DESTROY:
            hWndWatch = NULL;
            //
            // Fall thru...
            //

        case WU_DBG_UNLOADEE:
        case WU_DBG_UNLOADEM:
            //
            // Lose the Watch Tree
            //
            if ( pvitWatch && pvitWatch->pvibChild ) {
                FTFreeAllSib(pvitWatch->pvibChild);
                pvitWatch->pvibChild = NULL;
                free(pvitWatch);
                pvitWatch = NULL;
                SendMessage(p->hWndLeft, LB_SETCOUNT, 1, 0);
                SendMessage(p->hWndButton, LB_SETCOUNT, 1, 0);
                SendMessage(p->hWndRight, LB_SETCOUNT, 1, 0);
                p->MaxIdx = (WORD)1;
                WatchUpdate(p, FALSE, wParam);
            }
            break;

        case WU_DBG_LOADEM:
        case WU_DBG_LOADEE:
            if (!pvitWatch) {
                Dbg(InitWatchVit());
                hWndWatch = hwnd;

                SendMessage(p->hWndLeft, LB_SETCOUNT, 1, 0);
                SendMessage(p->hWndButton, LB_SETCOUNT, 1, 0);
                SendMessage(p->hWndRight, LB_SETCOUNT, 1, 0);
                p->MaxIdx = (WORD)1;
                WatchUpdate(p, FALSE, wParam);
            }
            break;

        case WU_INVALIDATE:
            if (p == (PPANE)NULL) {
                p = (PPANE)GetWindowLongPtr(GetWatchHWND(), GWW_EDIT );
            }

            SendMessage(p->hWndLeft, LB_SETCOUNT, 0, 0);
            SendMessage(p->hWndButton, LB_SETCOUNT, 0, 0);
            SendMessage(p->hWndRight, LB_SETCOUNT, 0, 0);
            p->MaxIdx = 0;

            WatchUpdate(p, FALSE, wParam);

            InvalidateRect(p->hWndButton, NULL, TRUE);
            InvalidateRect(p->hWndLeft, NULL, TRUE);
            InvalidateRect(p->hWndRight, NULL, TRUE);
            UpdateWindow (p->hWndButton);
            UpdateWindow (p->hWndLeft);
            UpdateWindow (p->hWndRight);
            break;

        case WU_OPTIONS:
            pVib = FTvibGetAtLine( pvitWatch, pInfo->ItemId);
            if ( pVib == NULL) {
                return(FALSE);
            }

            pvtext = &pVib->pvtext[pVib->vibIndex];

            if ( pInfo->pFormat && (*(pvtext->pszValueC) != '{') ) {
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

                if (pvtext->pszFormat) {
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
            pInfo->pBuffer  = pInfo->pFormat = NULL;
            pInfo->NewText  = FALSE;
            pInfo->ReadOnly = (pInfo->CtrlId == ID_PANE_LEFT) ? FALSE : TRUE;

            pVib = FTvibGetAtLine( pvitWatch, pInfo->ItemId);

            if ( pVib == NULL) {
                return(FALSE);
            }

            FTEnsureTextExists(pVib);
            pvtext = &pVib->pvtext[pVib->vibIndex];
            pInfo->pFormat = pvtext->pszFormat;

            pInfo->pBuffer  = FTGetPanelString( pvitWatch, pVib, pInfo->CtrlId);
            pInfo->ReadOnly = (pInfo->ItemId == (UINT) p->MaxIdx);

            if ( pInfo->CtrlId == ID_PANE_LEFT ) {
                pInfo->NewText  = FALSE;
            } else if ( pInfo->CtrlId == ID_PANE_RIGHT) {
                pInfo->NewText  = FTGetPanelStatus( pVib, pInfo->CtrlId);
            } else {
                pInfo->ReadOnly = TRUE;
                pInfo->NewText  = FALSE;
            }
            return TRUE;

        case WU_SETWATCH:
            if (pvitWatch == NULL) {
                return(FALSE);
            }

            if (p->nCtrlId == ID_PANE_LEFT) {
                return(AcceptWatchUpdate( pvitWatch, p, wParam));
            } else {
                BOOL retval;
                retval = (AcceptValueUpdate( pvitWatch, p));
                if (retval == TRUE) {
                    UpdateDebuggerState(UPDATE_DATAWINS);
                }
                return retval;
            }
            break;

        case WU_EXPANDWATCH:
            if (pvitWatch == NULL) {
                return(FALSE);
            }
            if ( FTExpand(pvitWatch, (ULONG)(wParam)) == OK) {
                p->LeftOk = p->RightOk = FALSE;
                WatchUpdate(p, FALSE, wParam);   // Watch Count changed
            }
            break;

        case WU_UPDATE:
            WatchUpdate(p, (BOOL)wParam, wParam);
            break;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;

    }

    return FALSE;
}


VOID
WatchUpdate(
    PPANE p,
    BOOL fClear,
    WPARAM wParam
    )
{
    LONG        Len = 0;
    LRESULT     lLen = 0;
    WORD        i;
    RECT        Rect, tRect;
    HWND        hFoc;
    HCURSOR     hOldCursor, hWaitCursor;


    if ( pvitWatch == NULL) {
        return;
    }

    // Set hourglass cursor
    hWaitCursor = LoadCursor (NULL, IDC_WAIT);
    hOldCursor = SetCursor (hWaitCursor);

    hFoc = GetFocus();


    if ( FTVerify(&CxfIp, pvitWatch->pvibChild) ) {
        p->LeftOk = FALSE;
    }

    pvitWatch->cxf = CxfIp;


    // Always repaint the watch window.
    if ( p->bFlags.Expand1st) {
        FTExpandOne(pvitWatch->pvibChild);
    }

    Len = (LONG)pvitWatch->cln + 1;


    lLen = SendMessage(p->hWndLeft, LB_GETCOUNT, 0, 0L);
    if ((lLen < Len) || (lLen == 0)) {
        SendMessage(p->hWndLeft, LB_SETCOUNT, Len, 0);
        SendMessage(p->hWndButton, LB_SETCOUNT, Len, 0);
        SendMessage(p->hWndRight, LB_SETCOUNT, Len, 0);
    } else {
        SendMessage(p->hWndLeft, LB_SETCOUNT, (WPARAM) ((int)lLen), 0L);
        SendMessage(p->hWndButton, LB_SETCOUNT, (WPARAM) ((int)lLen), 0L);
        SendMessage(p->hWndRight, LB_SETCOUNT, (WPARAM) ((int)lLen), 0L);
    }


    p->MaxIdx = (WORD)Len;

    //  Reseting the count, lost where we were so put us back

    if (p->MaxIdx > 0) {
        PaneResetIdx(p,p->CurIdx);
    }

    p->LeftOk = TRUE;


    //  Reset the right pane

    FTAgeVibValues(pvitWatch->pvibChild);


    for ( i= p->TopIdx; i < (p->TopIdx + p->PaneLines) ; i++) {
        if ( FTVerifyNew(pvitWatch, i) ) {
            PaneInvalidateItem( p->hWndRight, p, i);
        }
    }

    p->RightOk = TRUE;


    PaneCaretNum(p);


    if ((hFoc == p->hWndButton) || (hFoc == p->hWndLeft) || (hFoc == p->hWndRight)) {
        SendMessage(p->hWndButton , LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect (p->hWndButton, &tRect);
        tRect.top = Rect.top;
        InvalidateRect(p->hWndButton, &tRect, TRUE);


        SendMessage(p->hWndLeft , LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect (p->hWndLeft, &tRect);
        tRect.top = Rect.top;
        InvalidateRect(p->hWndLeft, &tRect, TRUE);


        SendMessage(p->hWndRight , LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect (p->hWndRight, &tRect);
        tRect.top = Rect.top;
        InvalidateRect(p->hWndRight, &tRect, TRUE);
    }


    CheckPaneScrollBar( p, (WORD)Len);

    // Set original cursor
    hOldCursor = SetCursor (hOldCursor);

}


void
ReloadAllWatchVariables()
/*++
Routine Desciption:
    Deletes and readds all of the variables to the watch window.
    This is basically just a cog in a bigger hack to force the
    watch window to work correctly.
--*/
{
    PTRVIT pVit = NULL;
    PTRVIB pVib = NULL;

    LPSTR * alpszExpr = NULL;   // Tmp holder for watch expressions. Array of LPSTR
    const int nGrowBy = 2;      // When the array is full, grow it by this amount.
    int nStrArraySize = 0;      // Size of alpszExpr
    int nNumStringItems = 0;    // Current number of strings stored in alpszExpr

    int i;

    if (!pvitWatch) {
        return;
    }

    // Save all of the strings
    for (pVib = pvitWatch->pvibChild; pVib; pVib = pVib->pvibSib) {
        if (pVib->pwoj && pVib->pwoj->szExpStr && *pVib->pwoj->szExpStr) {

            // Do we need to resize the array
            if (nNumStringItems == nStrArraySize) {
                PSTR * p = (PSTR *) realloc(alpszExpr, sizeof(LPSTR) * (nStrArraySize + nGrowBy));

                if (!p) {
                    // Array has not been resized. Bail out & lose the rest of the watch expresions.
                    break;
                } else {
                    // Array has been successfully reallocated. Initialize the new region by
                    // zeroing it out.
                    alpszExpr = p;
                    nStrArraySize += nGrowBy;
                    memset(&alpszExpr[nNumStringItems], 0, sizeof(LPSTR) * nGrowBy);
                }
            }

            alpszExpr[nNumStringItems] = _strdup(pVib->pwoj->szExpStr);
            if (!alpszExpr[nNumStringItems]) {
                // Weren't able to allocate mem. Bail out & lose the rest of the watch expressions.
                break;
            } else {
                // String successfully added
                nNumStringItems++;
            }
        }
    }

    // Delete all the watch variables
    SendMessageNZ( GetWatchHWND(), WU_DBG_UNLOADEE, 0, 0L);

    // Re-add all of the watch variables
    for (i=0; i<nNumStringItems; i++) {
        FTAddWatchVariable(&pVit, &pVib, alpszExpr[i]);
        free(alpszExpr[i]);
        alpszExpr[i] = NULL;
    }

    if (alpszExpr) {
        free(alpszExpr);
    }
}
