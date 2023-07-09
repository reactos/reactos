#include "sol.h"
VSZASSERT




VOID FreeGm(GM *pgm)
{
    INT icol;
    COL *pcol;

    if(pgm != NULL)
    {
        for(icol = pgm->icolMac-1; icol >= 0; icol--)
            if((pcol = pgm->rgpcol[icol]) != NULL)
                SendColMsg(pcol, msgcEnd, 0, 0);
        if(pgm == pgmCur)
            pgmCur = NULL;
        FreeUndo(&pgm->udr);
        FreeP(pgm);
    }
}


BOOL FCreateDCBM(HDC hdc, HDC *phdc, HBITMAP *phbmOld, DY dyCol)
{
    HDC hdcT;
    HBITMAP hbm;

    if((hdcT = CreateCompatibleDC(hdc)) == NULL)
        return fFalse;

    if((hbm = CreateCompatibleBitmap(hdc, dxCrd, dyCol)) == NULL)
    {
        Error:
        DeleteDC(hdcT);
        return fFalse;
    }

    if((*phbmOld = SelectObject(hdcT, hbm)) == NULL)
    {
        /* Delete the bitmap */
        DeleteObject(hbm);
        goto Error;
    }
    *phdc = hdcT;

    return fTrue;
}


BOOL FSetDrag(BOOL fOutline)
    {
    HDC hdc;

    fOutlineDrag = fOutline;

    if(fOutline && move.fHdc)
    {
        Assert(move.hdcScreenSave);
        Assert(move.hdcCol);
        Assert(move.hbmScreenSaveOld);
        Assert(move.hbmColOld);
        Assert(move.hdcT);

        DeleteObject(SelectObject(move.hdcCol, move.hbmColOld));
        DeleteDC(move.hdcCol);

        DeleteObject(SelectObject(move.hdcScreenSave, move.hbmScreenSaveOld));
        DeleteDC(move.hdcScreenSave);

        DeleteObject(SelectObject(move.hdcT, move.hbmT));
        DeleteDC(move.hdcT);
        move.fHdc = fFalse;
    }


    if(!fOutline && !move.fHdc)
    {
        hdc = GetDC(hwndApp);
        if(hdc == NULL)
        {
            OOM:
            ErrorIds(idsNoFullDrag);
            fOutlineDrag = fFalse;
            move.fHdc = fFalse;
            return fFalse;
        }

        move.hdcScreen = NULL;

        if(!FCreateDCBM(hdc, &move.hdcScreenSave, &move.hbmScreenSaveOld, pgmCur->dyDragMax))
        {
            ReleaseDC(hwndApp,hdc);
            goto OOM;
        }

        if(!FCreateDCBM(hdc, &move.hdcT, &move.hbmT, pgmCur->dyDragMax))
        {
            OOM1:

            ReleaseDC(hwndApp,hdc);
            DeleteObject(SelectObject(move.hdcScreenSave, move.hbmScreenSaveOld));
            DeleteDC(move.hdcScreenSave);
            goto OOM;
        }

        if(!FCreateDCBM(hdc, &move.hdcCol, &move.hbmColOld, pgmCur->dyDragMax))
        {
            DeleteObject(SelectObject(move.hdcT, move.hbmT));
            DeleteDC(move.hdcT);
            goto OOM1;
        }

        move.fHdc = fTrue;
        ReleaseDC(hwndApp, hdc);
    }
    return fTrue;
}




BOOL FInitGm()
{
    BOOL FInitKlondGm();

    return FInitKlondGm();
}


#ifdef DEBUG
LRESULT SendGmMsg(GM *pgm, INT msgg, WPARAM wp1, LPARAM wp2)
{
    INT imdbg;
    LRESULT wResult;

    Assert(pgm != NULL);
    imdbg = ILogMsg(pgm, msgg, wp1, wp2, fTrue);

    wResult =(*(pgm->lpfnGmProc))(pgm, msgg, wp1, wp2);
    LogMsgResult(imdbg, wResult);
    return wResult;
}

#endif


BOOL DefGmInit(GM *pgm, BOOL fResetScore)
{

    pgm->fDealt = fFalse;
    if(fResetScore)
        pgm->sco = 0;
    pgm->iqsecScore = 0;
    pgm->irep = 0;
    pgm->icolHilight = pgm->icolSel = icolNil;
    pgm->icolKbd = 0;
    pgm->icrdKbd = 0;
    pgm->fInput = fFalse;
    pgm->fWon = fFalse;
    pgm->ccrdDeal = ccrdDeal;
    return fTrue;
}



BOOL DefGmMouseDown(GM *pgm, PT *ppt, INT icolFirst)
{
    INT icol;

    /* sel already in effect */
    if(FSelOfGm(pgm))
        return fFalse;
    if(!pgm->fDealt)
        return fFalse;
    pgm->fInput = fTrue;
    pgm->fButtonDown = fTrue;
    for(icol = icolFirst; icol < pgm->icolMac; icol++)
    {
        if(SendColMsg(pgm->rgpcol[icol], msgcHit, (INT_PTR) ppt, 0) != icrdNil)
        {
            pgm->icolSel = icol;
            pgm->ptMousePrev = ptNil;

            /* KLUDGE:  in col render, we redraw the column after a selection
               is made.  if the mouse isn't moved, no image of the selected
               card shows up.
            */
            if(!fOutlineDrag)
            {
                /* SendGmMsg(pgm, msggMouseMove, (INT_PTR) ppt, 0); */
                pgm->ptMousePrev = *ppt;
            }
            return fTrue;
        }
    }
    return fFalse;
}

BOOL DefGmMouseUp(GM *pgm, PT *pptBogus, BOOL fNoMove)
{
    COL *pcolSel, *pcolHilight;
    BOOL fResult = fFalse;

    pgm->fButtonDown = fFalse;
    if(FSelOfGm(pgm))
    {
        pcolSel = pgm->rgpcol[pgm->icolSel];
        if(FHilightOfGm(pgm))
        {
            pcolHilight = pgm->rgpcol[pgm->icolHilight];
            SendGmMsg(pgm, msggSaveUndo, pgm->icolHilight, pgm->icolSel);
            SendColMsg(pcolHilight, msgcDragInvert, 0, 0);
            if(fNoMove)
            {
                SendColMsg(pcolSel, msgcMouseUp, (INT_PTR) &pgm->ptMousePrev, fTrue);
                fResult = fTrue;
                goto Return;
            }
            SendColMsg(pcolSel, msgcMouseUp, (INT_PTR) &pgm->ptMousePrev, fFalse);
            fResult = SendColMsg(pcolHilight, msgcMove, (INT_PTR) pcolSel, icrdToEnd) &&
                SendGmMsg(pgm, msggScore, (INT_PTR) pcolHilight, (INT_PTR) pcolSel);
            pgm->icolHilight = icolNil;
            if(SendGmMsg(pgm, msggIsWinner, 0, 0))
                SendGmMsg(pgm, msggWinner, 0, 0);
        }
        else
            SendColMsg(pcolSel, msgcMouseUp, (INT_PTR) &pgm->ptMousePrev, fTrue);

        Return:
        SendColMsg(pcolSel, msgcEndSel, fFalse, 0);
        }
    pgm->icolSel = icolNil;
    return fResult;
}



BOOL DefGmMouseDblClk(GM *pgm, PT * ppt)
{
    INT icol;

    for(icol = 0; icol < pgm->icolMac; icol++)
        if(SendColMsg(pgm->rgpcol[icol], msgcDblClk, (INT_PTR) ppt, icol))
            return fTrue;
    return fFalse;
}



// This routine moves all the "playable" cards
// to the four suit stacks.
// It's invoked when the user right-clicks or
// presses Ctrl-A.


BOOL DefGmMouseRightClk(GM *pgm, PT * ppt)
{
    INT icol;
    CRD *pcrd;
    INT icolDest;
    COL *pcolDest;
    BOOL fResult;
    COL *pcol;
    INT  iContinue;

    fResult = fFalse;

    // Keep doing this as long as in every iteration
    // we move one card to the suit stack.
    do
    {
        iContinue = 0;
        for(icol = 0; icol < pgm->icolMac; icol++)
        {
            // We don't want to move cards from one suit stack
            // to another.
            if (icol >= icolFoundFirst && icol < icolFoundFirst+ccolFound)
                continue;

            // Now the column we have is one of the 7 columns
            // or the deck.
            pcol = pgm->rgpcol[icol];

            // If this column contains cards and the top one faces up
            if(pcol->icrdMac > 0 && (pcrd=&pcol->rgcrd[pcol->icrdMac-1])->fUp)
            {
                if(pcol->pmove == NULL)
                    SendColMsg(pcol, msgcSel, icrdEnd, ccrdToEnd);
                Assert(pcol->pmove != NULL);

                // Check if it can be moved to any of the suit stacks.
                for(icolDest = icolFoundFirst; icolDest < icolFoundFirst+ccolFound; icolDest++)
                {
                    pcolDest = pgmCur->rgpcol[icolDest];
                    if(SendColMsg(pcolDest, msgcValidMove, (INT_PTR)pcol, 0))
                    {
                        SendGmMsg(pgmCur, msggSaveUndo, icolDest, icol);
                        fResult = SendColMsg(pcolDest, msgcMove, (INT_PTR) pcol, icrdToEnd) &&
                        (fOutlineDrag || SendColMsg(pcol, msgcRender, pcol->icrdMac-1, icrdToEnd)) &&
                            SendGmMsg(pgmCur, msggScore, (INT_PTR) pcolDest, (INT_PTR) pcol);

                        iContinue ++;

                        if(SendGmMsg(pgmCur, msggIsWinner, 0, 0))
                            SendGmMsg(pgmCur, msggWinner, 0, 0);
                        break;
                    }
                }
            }

            SendColMsg(pcol, msgcEndSel, fFalse, 0);
        }
    } while (iContinue > 0);

    return fResult;
}





BOOL DefGmMouseMove(GM *pgm, PT *ppt)
{
    COL *pcol;
    INT icol;

    if(FSelOfGm(pgm))
    {
        Assert(pgm->icolSel < pgm->icolMac);
        /* draw new outline */
        pcol = pgm->rgpcol[pgm->icolSel];
        SendColMsg(pcol, msgcDrawOutline, (INT_PTR) ppt, (INT_PTR) &pgm->ptMousePrev);
        pgm->ptMousePrev = *ppt;
        for(icol = 0; icol < pgm->icolMac; icol++)
            if(SendColMsg(pgm->rgpcol[icol], msgcValidMovePt, (INT_PTR)pgm->rgpcol[pgm->icolSel], (INT_PTR) ppt) != icrdNil)
            {
                 if(icol != pgm->icolHilight)
                 {
                    if(FHilightOfGm(pgm))
                        SendColMsg(pgm->rgpcol[pgm->icolHilight], msgcDragInvert, 0, 0);
                     pgm->icolHilight = icol;
                     return SendColMsg(pgm->rgpcol[icol], msgcDragInvert, 0, 0);
                 }
                 else
                     return fTrue;
            }
        /* nothing to hilight */
        if(FHilightOfGm(pgm))
        {
            SendColMsg(pgm->rgpcol[pgm->icolHilight], msgcDragInvert, 0, 0);
            pgm->icolHilight = icolNil;
            return fTrue;
        }
    }
    return fFalse;
}


BOOL DefGmPaint(GM *pgm, PAINTSTRUCT *ppaint)
{
    INT icol;
    HDC hdc;

    hdc = HdcSet(ppaint->hdc, 0, 0);

    if(!pgm->fDealt)
        goto Return;
    for(icol = 0; icol < pgm->icolMac; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcPaint, (INT_PTR) ppaint, 0);
Return:
    HdcSet(hdc, 0, 0);
    return fTrue;
}


BOOL DefGmUndo(GM *pgm)
{
    UDR *pudr;

    Assert(!FSelOfGm(pgm));
    pudr = &pgm->udr;
    if(!pudr->fAvail)
        return fFalse;
    Assert(pudr->icol1 != icolNil);
    Assert(pudr->icol2 != icolNil);

    Assert(pudr->icol1 < pgm->icolMax);
    Assert(pudr->icol2 < pgm->icolMax);

    pgm->sco  = pudr->sco;
    pgm->irep = pudr->irep;

    SendGmMsg(pgm, msggChangeScore, 0, 0);


    SendColMsg(pgm->rgpcol[pudr->icol1], msgcCopy, (INT_PTR) pudr->rgpcol[0], fTrue);
    SendColMsg(pgm->rgpcol[pudr->icol2], msgcCopy, (INT_PTR) pudr->rgpcol[1], fTrue);
    /* end any selectons if we had 'em */
    SendColMsg(pgm->rgpcol[pudr->icol1], msgcEndSel, 0, 0);
    SendColMsg(pgm->rgpcol[pudr->icol2], msgcEndSel, 0, 0);

    SendGmMsg(pgm, msggKillUndo, 0, 0);
    return fTrue;
}



/* in future: may want to alloc columns */
BOOL DefGmSaveUndo(GM *pgm, INT icol1, INT icol2)
{
    Assert(icol1 != icolNil);
    Assert(icol2 != icolNil);
    Assert(icol1 < pgm->icolMac);
    Assert(icol2 < pgm->icolMac);
    Assert(icol1 != icol2);

    /* should use msgcCopy, but undo colcls's may not be set correctly */
    bltb(pgm->rgpcol[icol1], pgm->udr.rgpcol[0], sizeof(COL)+(pgm->rgpcol[icol1]->icrdMac-1)*sizeof(CRD));
    bltb(pgm->rgpcol[icol2], pgm->udr.rgpcol[1], sizeof(COL)+(pgm->rgpcol[icol2]->icrdMac-1)*sizeof(CRD));
    pgm->udr.icol1  = icol1;
    pgm->udr.icol2  = icol2;
    pgm->udr.fAvail = fTrue;
    pgm->udr.sco    = pgm->sco;
    pgm->udr.irep   = pgm->irep;

    if(pgm->udr.fEndDeck)
    {
        pgm->udr.fEndDeck = FALSE;
        pgm->udr.irep--;
    }

    return fTrue;
}



#ifdef DEBUG
VOID DisplayKbdSel(GM *pgm)
{
    HDC hdc;
    TCHAR sz[20];
    INT cch;

    hdc = GetDC(hwndApp);
    PszCopy(TEXT("      "), sz);
    cch = CchDecodeInt(sz, pgm->icolKbd);
    TextOut(hdc, 0, 10, sz, 5);
    PszCopy(TEXT("      "), sz);
    cch = CchDecodeInt(sz, pgm->icrdKbd);
    TextOut(hdc, 0, 20, sz, 5);
    PszCopy(TEXT("      "), sz);
    cch = CchDecodeInt(sz, pgm->icolSel);
    TextOut(hdc, 0, 30, sz, 5);
    ReleaseDC(hwndApp, hdc);
}
#endif



VOID NewKbdColAbs(GM *pgm, INT icol)
{
    Assert(icol >= 0);
    Assert(icol < pgm->icolMac);

    if(!SendColMsg(pgm->rgpcol[icol], msgcValidKbdColSel, FSelOfGm(pgm), 0))
        /* beep? */
        return;

    pgm->icolKbd = icol;
    pgm->icrdKbd = SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fFalse, 0)-1;
    if(pgm->icrdKbd < 0)
        pgm->icrdKbd = 0;
}


VOID NewKbdCol(GM *pgm, INT dcol, BOOL fNextGroup)
{
    INT icolNew;

    icolNew = pgm->icolKbd;
    if(icolNew == icolNil)
        icolNew = 0;
    if(dcol != 0)
    {
        do
        {
            icolNew += dcol;
            if(icolNew < 0)
                icolNew = pgm->icolMac-1;
            else if(icolNew >= pgm->icolMac)
                icolNew = 0;

            /* only one col class and looped through all col's */
            if(icolNew == pgm->icolKbd)
                break;
        }
        while (!SendColMsg(pgm->rgpcol[icolNew], msgcValidKbdColSel, FSelOfGm(pgm), 0) ||
                (fNextGroup &&
                    pgm->rgpcol[icolNew]->pcolcls->tcls ==
                       pgm->rgpcol[pgm->icolKbd]->pcolcls->tcls));

    }

    NewKbdColAbs(pgm, icolNew);
}




VOID NewKbdCrd(GM *pgm, INT dcrd)
{
    INT icrdUpMac, icrdMac;
    INT icrdKbdNew;

    icrdUpMac = SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fTrue, 0);
    icrdMac = SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fFalse, 0);

    if(icrdMac == 0)
        icrdKbdNew = 0;
    else
    {
        if(icrdUpMac == 0)
            icrdKbdNew = icrdMac-1;
        else
            icrdKbdNew = PegRange(pgm->icrdKbd+dcrd, icrdMac-icrdUpMac, icrdMac-1);
    }
    if(SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcValidKbdCrdSel, icrdKbdNew, 0))
        pgm->icrdKbd = icrdKbdNew;
}





BOOL DefGmKeyHit(GM *pgm, INT vk)
{
    PT pt, ptCurs;
    COLCLS *pcolcls;

    /* cancel any mouse selections */

    switch(vk)
    {
    case VK_SPACE:
    case VK_RETURN:
        if(!FSelOfGm(pgm))
            {
            /* begin a selection */
            NewKbdCrd(pgm, 0);  /* !!! */
            SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcGetPtInCrd, pgm->icrdKbd, (INT_PTR) &pt);
            if(!SendGmMsg(pgm, msggMouseDown, (INT_PTR) &pt, 0))
                return fFalse;
            NewKbdCol(pgm, 0, fFalse);
            goto Display;
            }
        else
            {
            /* possibly make a move */
            SendGmMsg(pgm, msggMouseUp, 0, fFalse);
            NewKbdCol(pgm, 0, fFalse);
            return fTrue;
            }

    case VK_ESCAPE:
        SendGmMsg(pgm, msggMouseUp, 0, fTrue);
        return fTrue;

    case VK_A:
        if (GetKeyState(VK_CONTROL) < 0)
            SendGmMsg(pgm, msggMouseRightClk, 0, fTrue);
        return fTrue;

    case VK_LEFT:
        /* Should these be VK_CONTROL??? */
        NewKbdCol(pgm, -1, GetKeyState(VK_SHIFT) < 0);
        goto Display;

    case VK_RIGHT:
        NewKbdCol(pgm, 1, GetKeyState(VK_SHIFT) < 0);
        goto Display;

    case VK_UP:
        NewKbdCrd(pgm, -1);
        goto Display;

    case VK_DOWN:
        NewKbdCrd(pgm, 1);
        goto Display;

    case VK_HOME:
        NewKbdColAbs(pgm, 0);
        goto Display;

    case VK_END:
        NewKbdColAbs(pgm, pgm->icolMac-1);
        goto Display;

    case VK_TAB:
        NewKbdCol(pgm, GetKeyState(VK_SHIFT) < 0 ? -1 : 1, fTrue);
Display:
        SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcGetPtInCrd, pgm->icrdKbd, (INT_PTR) &pt);
        ptCurs = pt;
        ClientToScreen(hwndApp, (LPPOINT) &ptCurs);
        if(FSelOfGm(pgm))
        {
            if(SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fFalse, 0) > 0)
            {
                pcolcls = pgm->rgpcol[pgm->icolKbd]->pcolcls;
                ptCurs.y += pcolcls->dyUp;
                /* dxUp ? */
            }
        }

        /* SetCursorPos will cause WM_MOUSEMOVE to be sent */
        SetCursorPos(ptCurs.x, ptCurs.y);
        return fTrue;
    }
    return fFalse;
}



BOOL DefGmChangeScore(GM *pgm, INT cs, INT sco)
{

    if(smd == smdNone)
        return fTrue;
    switch(cs)
        {
    default:
        return fTrue;
    case csAbs:
        pgm->sco = sco;
        break;
    case csDel:
        pgm->sco += sco;
        break;
    case csDelPos:
        pgm->sco = WMax(pgm->sco+sco, 0);
        break;
        }
    StatUpdate();

    return fTrue;
}


BOOL DefGmWinner(GM *pgm)
{
    pgm->fWon = fFalse;
    if(FYesNoAlert(idsDealAgain))
        PostMessage(hwndApp, WM_COMMAND, idsInitiate, 0L);
    return fTrue;
}


INT DefGmProc(GM *pgm, INT msgg, WPARAM wp1, LPARAM wp2)
{

    switch(msgg)
    {
    case msggInit:
        return DefGmInit(pgm, (BOOL)wp1);
    case msggEnd:
        FreeGm(pgm);
        break;

    case msggKeyHit:
        return DefGmKeyHit(pgm, (INT)wp1);

    case msggMouseRightClk:
        return DefGmMouseRightClk(pgm, (PT *)wp1);

    case msggMouseDown: /* wp1 == ppt, wp2 = icolFirst (normally 0) */
        return DefGmMouseDown(pgm, (PT *)wp1, (INT)wp2);

    case msggMouseUp:
        return DefGmMouseUp(pgm, (PT *)wp1, (BOOL)wp2);

    case msggMouseMove:
        return DefGmMouseMove(pgm, (PT *)wp1);

    case msggMouseDblClk:
        return DefGmMouseDblClk(pgm, (PT *)wp1);

    case msggPaint:
        return DefGmPaint(pgm, (PAINTSTRUCT *)wp1);

    case msggDeal:
        Assert(fFalse);
        break;

    case msggUndo:
        return DefGmUndo(pgm);

    case msggSaveUndo:

        return DefGmSaveUndo(pgm, (INT)wp1, (INT)wp2);

    case msggKillUndo:
        /* in future may want to free columns */
        pgm->udr.fAvail = fFalse;
        break;
    case msggIsWinner:
        return fFalse;
    case msggWinner:
        return DefGmWinner(pgm);
    case msggForceWin:
        NYI();
        break;
    case msggTimer:
        return fFalse;
    case msggScore:
        return fTrue;
    case msggChangeScore:
        return DefGmChangeScore(pgm, (INT)wp1, (INT)wp2);

    }

    return fFalse;
}

