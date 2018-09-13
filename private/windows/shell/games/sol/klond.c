#include "sol.h"
VSZASSERT


/* Klondike init stuff */
LRESULT KlondGmProc(GM *pgm, INT msgg, WPARAM wp1, LPARAM wp2);
LRESULT DeckColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2);
LRESULT DiscardColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2);
LRESULT TabColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2);
LRESULT FoundColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2);



// Imported from Win3.1
BOOL FInitKlondGm()
{
    COLCLS *pcolcls;
    GM *pgm;
    DX dxCrdOffUp;
    DX dyCrdOffUp;
    DX dyCrdOffDn;
    int icol;
    int icrdMax;

    /* KLUDGE to get klondike going */
    FreeGm(pgmCur);

    if((pgm = pgmCur = PAlloc(sizeof(GM)+(13-1)*sizeof(COL *))) == NULL)
        return fFalse;

    pgm->lpfnGmProc = KlondGmProc;
    SendGmMsg(pgm, msggInit, fTrue, 0);
    pgm->icolMax = 13;
    pgm->dqsecScore = 10*4;

    if(!FInitUndo(&pgm->udr))
        goto OOM;

    /* Initialize all the column types but don't position yet */
    for(icol = 0; icol < pgm->icolMax; icol++)
    {
        switch(icol)
        {
        case icolDeck:
            pcolcls = PcolclsCreate(tclsDeck, (COLCLSCREATEFUNC)DeckColProc,
                            0, 0, 2, 1, 1, 10);
            icrdMax = icrdDeckMax;
            break;

        case icolDiscard:
            dxCrdOffUp = dxCrd / 5;
            pcolcls = PcolclsCreate(tclsDiscard, (COLCLSCREATEFUNC)DiscardColProc,
                            dxCrdOffUp, 1, 2, 1, 1, 10);
            icrdMax = icrdDiscardMax;
            break;

        case icolFoundFirst:
            pcolcls = PcolclsCreate(tclsFound, (COLCLSCREATEFUNC)FoundColProc,
                            2, 1, 0, 0, 4, 1);
            Assert(icol - 1 == icolDiscard);
            icrdMax = icrdFoundMax;
            break;

        case icolTabFirst:
            Assert(fHalfCards == 1 || fHalfCards == 0);
            dyCrdOffUp = dyCrd * 4 / 25 - fHalfCards;
            dyCrdOffDn = dyCrd / 25;
            pgm->dyDragMax = dyCrd + 12 * dyCrdOffUp;
            pcolcls = PcolclsCreate(tclsTab, (COLCLSCREATEFUNC)TabColProc,
                        0, dyCrdOffUp, 0, dyCrdOffDn, 1, 1);
            icrdMax = icrdTabMax;
            break;
        }

        if(pcolcls == NULL)
        {
OOM:
            OOM();
            FreeGm(pgmCur);
            Assert(pgmCur == NULL);
            return fFalse;
        }

        if((pgm->rgpcol[icol] = PcolCreate(pcolcls, 0, 0, 0, 0, icrdMax))
            == NULL)
        {
            FreeP(pcolcls);
            goto OOM;
        }
        pgm->icolMac++;
    }

    /* Return without positioning the cards.  This will be done at
     *  WM_SIZE message time.
     */
    return TRUE;
}


/*  PositionCols
 *      Positions the card columns.  Note that this has been revised to
 *      allow card positioning at times other than at the start of the
 *      game.
 */

BOOL PositionCols(void)
{
    DX dxMarg;
    DY dyMarg;
    DX dx;
    X xLeft;
    X xRight;
    Y yTop;
    Y yBot;
    int icol;
    DY dyCrdOffUp;
    DY dyCrdOffDn;
    COL *pcol;
    GM *pgm;
    WORD i;

    /* The game we're using is always the current one */
    pgm = pgmCur;

    /* Before doing the column classes, replace all card X coordinates with
     *  offsets from the column class.
     */
    for (icol = 0 ; icol < 13 ; ++icol)
    {
        /* Get a pointer to this COL structure */
        pcol = pgm->rgpcol[icol];

        /* Loop through all the cards in this column */
        for (i = 0 ; i < pcol->icrdMax ; ++i)
            pcol->rgcrd[i].pt.x -= pcol->rc.xLeft;
    }

    /* Set the card margins.  Note that xCardMargin is computed in SOL.C
     *  at the time the original window is created and is changed on
     *  WM_SIZE messages.
     */
    dxMarg = xCardMargin;
    dyMarg = MulDiv(dyCrd, 5, 100);

    /* Loop through all column types */
    for(icol = 0 ; icol < 13 ; icol++)
    {
        switch(icol)
        {
        case icolDeck:
            xLeft = dxMarg;
            yTop = dyMarg;
            xRight = xLeft + dxCrd + icrdDeckMax / 10 * 2;
            yBot = yTop + dyCrd + icrdDeckMax / 10;
            dx = 0;
            break;

        case icolDiscard:
            xLeft += dxMarg + dxCrd;
            xRight = xLeft + 7 * dxCrd / 5 + icrdDiscardMax / 10 * 2;
            break;

        case icolFoundFirst:
            xLeft = 4 * dxMarg + 3 * dxCrd;
            xRight = xLeft + dxCrd + icrdFoundMax / 4 * 2;
            dx = dxMarg + dxCrd;
            break;

        case icolTabFirst:
            dyCrdOffUp = dyCrd * 4 / 25 - fHalfCards;
            dyCrdOffDn = dyCrd / 25;
            xLeft = dxMarg;
            xRight = xLeft + dxCrd;
            yTop = yBot + 1;
            yBot = yTop + 12 * dyCrdOffUp + dyCrd + 6 * dyCrdOffDn;
            break;
        }

        /* Set this information into the structure */
        pcol = pgm->rgpcol[icol];
        pcol->rc.xLeft = xLeft;
        pcol->rc.yTop = yTop;
        pcol->rc.xRight = xRight;
        pcol->rc.yBot = yBot;

        /* Prepare for the next loop */
        xLeft += dx;
        xRight += dx;
    }

    /* Now that the column offsets are correct, move the cards back */
    for (icol = 0 ; icol < 13 ; ++icol)
    {
        /* Get a pointer to this COL structure */
        pcol = pgm->rgpcol[icol];

        /* Loop through all the cards in this column */
        for (i = 0 ; i < pcol->icrdMax ; ++i)
            pcol->rgcrd[i].pt.x += pcol->rc.xLeft;
    }

    return TRUE;
}




/* TABLEAU col Proc stuff */


BOOL FTabValidMove(COL *pcolDest, COL *pcolSrc)
{
    RA raSrc, raDest;
    SU suSrc, suDest;
    INT icrdSel;
    CD cd;

    Assert(pcolSrc->pmove != NULL);
    icrdSel = pcolSrc->pmove->icrdSel;

    Assert(icrdSel < pcolSrc->icrdMac);
    Assert(pcolSrc->icrdMac > 0);
    cd = pcolSrc->rgcrd[icrdSel].cd;
    raSrc = RaFromCd(cd);
    suSrc = SuFromCd(cd);
    if(raSrc == raKing)
        return (pcolDest->icrdMac == 0);
    if(pcolDest->icrdMac == 0)
        return fFalse;
    if(!pcolDest->rgcrd[pcolDest->icrdMac-1].fUp)
        return fFalse;
    cd = pcolDest->rgcrd[pcolDest->icrdMac-1].cd;
    raDest = RaFromCd(cd);
    suDest = SuFromCd(cd);
    /* invalid moves */
    Assert((suClub ^ suSpade) == 0x03);
    Assert((suHeart ^ suDiamond) == 0x03);
    /* valid moves */
    Assert((suClub ^ suDiamond) < 0x03);
    Assert((suClub ^ suHeart) < 0x03);
    Assert((suSpade ^ suDiamond) < 0x03);
    Assert((suSpade ^ suHeart) < 0x03);

    return (((suSrc ^ suDest) < 0x03) && suSrc != suDest && raSrc+1 == raDest);
}


INT TabHit(COL *pcol, PT *ppt, INT icrdMin)
{
    CRD *pcrd;

    if(pcol->icrdMac > 0 && !(pcrd=&pcol->rgcrd[pcol->icrdMac-1])->fUp && FPtInCrd(pcrd, *ppt))
    {
        SendGmMsg(pgmCur, msggKillUndo, 0, 0);
        SendColMsg(pcol, msgcSel, icrdEnd, 1);
        SendColMsg(pcol, msgcFlip, fTrue, 0);
        SendColMsg(pcol, msgcComputeCrdPos, pcol->icrdMac-1, fFalse);
        SendColMsg(pcol, msgcRender, pcol->icrdMac-1, icrdToEnd);
        SendGmMsg(pgmCur, msggChangeScore, csKlondTabFlip, 0);
        SendColMsg(pcol, msgcEndSel, fFalse, 0);
        /* should I return this? */
        return icrdNil;
    }
    return DefColProc(pcol, msgcHit, (INT_PTR) ppt, icrdMin);
}


BOOL TabDiscardDblClk(COL *pcol, PT *ppt, INT icol)
{
    CRD *pcrd;
    INT icolDest;
    COL *pcolDest;
    BOOL fResult;

    fResult = fFalse;
    if(pcol->icrdMac > 0 && (pcrd=&pcol->rgcrd[pcol->icrdMac-1])->fUp && FPtInCrd(pcrd, *ppt))
    {
        if(pcol->pmove == NULL)
            SendColMsg(pcol, msgcSel, icrdEnd, ccrdToEnd);
        Assert(pcol->pmove != NULL);
        for(icolDest = icolFoundFirst; icolDest < icolFoundFirst+ccolFound; icolDest++)
        {
            pcolDest = pgmCur->rgpcol[icolDest];
            if(SendColMsg(pcolDest, msgcValidMove, (INT_PTR)pcol, 0))
            {
                SendGmMsg(pgmCur, msggSaveUndo, icolDest, icol);
                fResult = SendColMsg(pcolDest, msgcMove, (INT_PTR) pcol, icrdToEnd) &&
                    (fOutlineDrag || SendColMsg(pcol, msgcRender, pcol->icrdMac-1, icrdToEnd)) &&
                    SendGmMsg(pgmCur, msggScore, (INT_PTR) pcolDest, (INT_PTR) pcol);
                if(SendGmMsg(pgmCur, msggIsWinner, 0, 0))
                    SendGmMsg(pgmCur, msggWinner, 0, 0);
                goto Return;
            }
        }
        SendColMsg(pcol, msgcEndSel, fFalse, 0);
    }

Return:
    return fResult;
}



LRESULT TabColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2)
{

    switch(msgc)
    {
        case msgcHit:
        /* should this go in DefProc? */
            return TabHit(pcol, (PT *)wp1, (INT)wp2);

        case msgcDblClk:
            return TabDiscardDblClk(pcol, (PT *)wp1, (INT)wp2);

        case msgcValidMove:
            return FTabValidMove(pcol, (COL *) wp1);
    }
    return DefColProc(pcol, msgc, wp1, wp2);
}



BOOL FFoundRender(COL *pcol, INT icrdFirst, INT icrdLast)
{
#define dxFoundDn 2
#define dyFoundDn 1

    if(pcol->icrdMac == 0 || icrdLast == 0)
        {
        if(!FGetHdc())
            return fFalse;
        DrawCardExt((PT *)(&pcol->rc.xLeft), 0, GHOST);
        DrawBackExcl(pcol, (PT *) &pcol->rc);
        ReleaseHdc();
        return fTrue;
        }
    else
        return DefColProc(pcol, msgcRender, icrdFirst, icrdLast);
}





BOOL FFoundValidMove(COL *pcolDest, COL *pcolSrc)
{
    RA raSrc;
    SU suSrc;
    INT icrdSel;

    Assert(pcolSrc->pmove != NULL);
    icrdSel = pcolSrc->pmove->icrdSel;
    Assert(icrdSel < pcolSrc->icrdMac);
    Assert(pcolSrc->icrdMac > 0);
    if(pcolSrc->pmove->ccrdSel != 1)
        return fFalse;
    raSrc = RaFromCd(pcolSrc->rgcrd[icrdSel].cd);
    suSrc = SuFromCd(pcolSrc->rgcrd[icrdSel].cd);
    if(pcolDest->icrdMac == 0)
        return(raSrc == raAce);
    return (raSrc == RaFromCd(pcolDest->rgcrd[pcolDest->icrdMac-1].cd)+1 &&
                 suSrc == SuFromCd(pcolDest->rgcrd[pcolDest->icrdMac-1].cd));

}



/* Foundation stuff */
LRESULT FoundColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2)
{
        switch(msgc)
        {
        case msgcValidMove:
            return FFoundValidMove(pcol, (COL *) wp1);

        case msgcRender:
            return FFoundRender(pcol, (INT)wp1, (INT)wp2);
        }
    return DefColProc(pcol, msgc, wp1, wp2);
}




/* DeckStuff */


BOOL DeckInit(COL *pcol)
{
    CRD *pcrd;
    INT icrd;

    Assert(pcol->icrdMax == icrdDeckMax);
    for(icrd = 0; icrd < icrdDeckMax; icrd++)
        {
        pcrd = &pcol->rgcrd[icrd];
        pcrd->cd = (unsigned short)icrd;
        pcrd->pt = *(PT *)&pcol->rc;
        pcrd->fUp = fFalse;
        }
    pcol->icrdMac = icrdDeckMax;
    SendColMsg(pcol, msgcShuffle, 0, 0);
    SendColMsg(pcol, msgcComputeCrdPos, 0, fFalse);
    return fTrue;
}







INT DeckHit(COL *pcol, PT *ppt, INT icrdMin)
{
    RC rc;
    INT ccrd;

    if(pcol->icrdMac == 0)
    {
        CrdRcFromPt((PT *) &pcol->rc, &rc);
        if(PtInRect((LPRECT) &rc, *(POINT *)ppt))
            return icrdEmpty;
        else
            return icrdNil;
    }
    else
        if(!FPtInCrd(&pcol->rgcrd[pcol->icrdMac-1], *ppt))
            return icrdNil;

    ccrd = ((GetKeyState(VK_SHIFT) & GetKeyState(VK_CONTROL) & GetKeyState(VK_MENU)) < 0) ? 1 : pgmCur->ccrdDeal;

    move.icrdSel = WMax(pcol->icrdMac-ccrd, 0);
    move.ccrdSel = pcol->icrdMac - move.icrdSel;
    Assert(pcol->pmove == NULL);
    pcol->pmove = &move;
    return move.icrdSel;
}


BOOL FDeckRender(COL *pcol, INT icrdFirst, INT icrdLast)
{
    INT mode;
    BOOL f;
    PT pt;

    /* to avoid redrawing the deck multiple times during dealing */
    if(!pgmCur->fDealt && pcol->icrdMac%10 != 9)
        return fTrue;
    if(!FGetHdc())
        return fFalse;
    if(pcol->icrdMac == 0)
        {
        mode = (smd == smdVegas && pgmCur->irep == ccrdDeal-1) ? DECKX : DECKO;
        DrawCardExt((PT *) &pcol->rc, 0, mode);
        DrawBackExcl(pcol, (PT *) &pcol->rc);
        f = fTrue;
        }
    else
        {
        f = DefColProc(pcol, msgcRender, icrdFirst, icrdLast);
        if((icrdLast == pcol->icrdMac || icrdLast == icrdToEnd) && !fHalfCards)
            {
            pt.x = pcol->rgcrd[pcol->icrdMac-1].pt.x+dxCrd-1;
            pt.y = pcol->rgcrd[pcol->icrdMac-1].pt.y+dyCrd-1;
            SetPixel(hdcCur, pt.x-xOrgCur, pt.y-yOrgCur, rgbTable);
            SetPixel(hdcCur, pt.x-1-xOrgCur, pt.y-yOrgCur, rgbTable);
            SetPixel(hdcCur, pt.x-xOrgCur, pt.y-1-yOrgCur, rgbTable);
            }
        }
    ReleaseHdc();
    return f;
}


VOID DrawAnimate(INT cd, PT *ppt, INT iani)
{

    if(!FGetHdc())
        return;
    cdtAnimate(hdcCur, cd, ppt->x, ppt->y, iani);
    ReleaseHdc();
}


BOOL DeckAnimate(COL *pcol, INT iqsec)
{
    INT iani;
    PT pt;


    if(pcol->icrdMac > 0 && !fHalfCards)
        {
        pt = pcol->rgcrd[pcol->icrdMac-1].pt;
        switch(modeFaceDown)
            {
        case IDFACEDOWN3:
            DrawAnimate(IDFACEDOWN3, &pt, iqsec % 4);
            break;
        case IDFACEDOWN10:  /* krazy kastle  */
            DrawAnimate(IDFACEDOWN10, &pt, iqsec % 2);
            break;

        case IDFACEDOWN11:  /* sanflipe */
            if((iani = (iqsec+4) % (50*4)) < 4)
                DrawAnimate(IDFACEDOWN11, &pt, iani);
            else
                /* if a menu overlapps an ani while it is ani'ing, leaves deck
                 bitmap in inconsistent state...  */
                if(iani % 6 == 0)
                    DrawAnimate(IDFACEDOWN11, &pt, 3);
            break;
        case IDFACEDOWN12:  /* SLIME  */
            if((iani = (iqsec+4) % (15*4)) < 4)
                DrawAnimate(IDFACEDOWN12, &pt, iani);
            else
                /* if a menu overlapps an ani while it is ani'ing, leaves deck
                 bitmap in inconsistent state... */
                if(iani % 6 == 0)
                    DrawAnimate(IDFACEDOWN12, &pt, 3);
            break;
            }
        }
    return fTrue;
}





LRESULT DeckColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2)
{

    switch(msgc)
        {
    case msgcInit:
        return DeckInit(pcol);
    case msgcValidMove:
    case msgcDrawOutline:
        return fFalse;
    case msgcValidMovePt:
        return icrdNil;
    case msgcHit:                    									
        return DeckHit(pcol, (PT *) wp1, (INT)wp2);
    case msgcRender:
        return FDeckRender(pcol, (INT) wp1, (INT) wp2);
    case msgcValidKbdColSel:
        return !wp1;
    case msgcValidKbdCrdSel:
        return pcol->icrdMac == 0 || wp1 == (WPARAM) pcol->icrdMac-1;
    case msgcAnimate:
        return DeckAnimate(pcol, (INT)wp1);
        }
    return DefColProc(pcol, msgc, wp1, wp2);
}


BOOL DiscardRemove(COL *pcol, COL *pcolDest, LPARAM wp2)
{
        return DefColProc(pcol, msgcRemove, (INT_PTR) pcolDest, wp2);
}


BOOL DiscardMove(COL *pcolDest, COL *pcolSrc, INT icrd)
{
    BOOL fResult;

    SendColMsg(pcolDest, msgcComputeCrdPos, WMax(0, pcolDest->icrdMac-3), fTrue);

    /* YUCK: Default ComputeCrdPos doesn't quite work for discard because
        up cards are handled specially for Discard piles.  To keep
        code size down we have this global hack variable which DefComputeCrdPos
        uses.
    */
    fMegaDiscardHack = fTrue;
    fResult = DefColProc(pcolDest, msgcMove, (INT_PTR) pcolSrc, icrd);
    fMegaDiscardHack = fFalse;
    return fResult;
}



INT DiscardHit(COL *pcol, PT *ppt, INT icrdMin)
{
    return DefColProc(pcol, msgcHit, (INT_PTR) ppt, WMax(0, pcol->icrdMac-1));
}


BOOL DiscardRender(COL *pcol, INT icrdFirst, INT icrdLast)
{
    PT pt;
    INT icrd;
    COLCLS *pcolcls;

    if(DefColProc(pcol, msgcRender, icrdFirst, icrdLast))
        {
        if(FGetHdc())
            {
            pcolcls = pcol->pcolcls;
            for(icrd = pcol->icrdMac-1; icrd >= 0 && icrd >= pcol->icrdMac-2; icrd--)
                {
                pt = pcol->rgcrd[icrd].pt;
                /* 3 is a kludge value here */
                DrawBackground(pt.x+dxCrd-pcolcls->dxUp, pt.y-pcolcls->dyUp*3,
                        			pt.x+dxCrd, pt.y);
                }
            ReleaseHdc();
            }
        return fTrue;
        }
    return fFalse;
}



/* Discard Stuff */
LRESULT DiscardColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2)
{
    switch(msgc)
        {
    case msgcDblClk:
        return TabDiscardDblClk(pcol, (PT *)wp1, (INT)wp2);
    case msgcHit:
        return DiscardHit(pcol, (PT *)wp1, (INT)wp2);
    case msgcMove:
        return DiscardMove(pcol, (COL *) wp1, (INT)wp2);
    case msgcRemove:
        return DiscardRemove(pcol, (COL *) wp1, wp2);
    case msgcValidMovePt:
        return icrdNil;
    case msgcValidKbdColSel:
        return !wp1;
    case msgcRender:
        return DiscardRender(pcol, (INT)wp1, (INT)wp2);
    case msgcValidKbdCrdSel:
        return pcol->icrdMac == 0 || wp1 == (WPARAM) pcol->icrdMac-1;
        }

    return DefColProc(pcol, msgc, wp1, wp2);
}




/* GAME stuff */

BOOL KlondDeal(GM *pgm, BOOL fZeroScore)
{
    INT icrdSel;
    INT icol;
    INT irw;
    COL *pcolDeck;
    VOID StatString();

    if(!FGetHdc())
        {
        OOM();
        return fFalse;
        }

    EraseScreen();
    for(icol = 0; icol < pgm->icolMac; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcClearCol, 0, 0);

    pcolDeck = pgm->rgpcol[icolDeck];
    SendColMsg(pcolDeck, msgcInit, 0, 0);

    SendGmMsg(pgm, msggKillUndo, 0, 0);
    SendGmMsg(pgm, msggInit, !(smd == smdVegas && fKeepScore) || fZeroScore, 0);

    StatString(idsNil);
    pgm->fDealt = fTrue;
    SendGmMsg(pgm, msggChangeScore, csKlondDeal, 0);
    SendColMsg(pcolDeck, msgcRender, 0, icrdToEnd);
    pgm->fDealt = fFalse;
    for(icol = icolFoundFirst; icol < icolFoundFirst+ccolFound; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcRender, 0, icrdToEnd);

// BabakJ: What the %@!&$* is this?! irw is always less than irw + ccolTab!!!
//         Note: ccolTab if #ifdef'ed as 7
//    for(irw = 0; irw < irw+ccolTab; irw++)

    for(irw = 0; irw < ccolTab; irw++)
        for(icol = irw; icol < ccolTab; icol++)
            {
            icrdSel = SendColMsg(pcolDeck, msgcSel, icrdEnd, 0);
            if(icol == irw)
                SendColMsg(pcolDeck, msgcFlip, fTrue, 0);
            SendColMsg(pgm->rgpcol[icol+icolTabFirst], msgcMove, (INT_PTR) pcolDeck, icrdToEnd);
            SendColMsg(pcolDeck, msgcRender, icrdSel-1, icrdToEnd);
            }
    NewKbdColAbs(pgm, 0);
    pgm->fDealt = fTrue;
    ReleaseHdc();
    return fTrue;
}




BOOL KlondMouseDown(GM *pgm, PT *ppt)
{
    INT icrdSel;
    INT icrd;
    COL *pcolDeck, *pcolDiscard;

    /* Kbd sel already in effect */
    if(FSelOfGm(pgm) || !pgm->fDealt)
        return fFalse;
    /* place the next cards on discard pile */
    if((icrd = SendColMsg(pgm->rgpcol[icolDeck], msgcHit, (INT_PTR) ppt, 0)) != icrdNil)
        {
        pgm->fInput = fTrue;
        pcolDeck = pgm->rgpcol[icolDeck];
        pcolDiscard = pgm->rgpcol[icolDiscard];
        if(icrd == icrdEmpty)
            {
            /* repeat */
            if(SendColMsg(pcolDiscard, msgcNumCards, 0, 0) == 0)
                {
                /* both deck and discard are empty */
                Assert(pcolDeck->pmove == NULL);
                return fFalse;
                }
            if(smd == smdVegas && pgm->irep == ccrdDeal-1)
                return fFalse;

            pgm->irep++;
            pgm->udr.fEndDeck = TRUE;

            return SendGmMsg(pgm, msggSaveUndo, icolDiscard, icolDeck) &&
                SendColMsg(pcolDiscard, msgcSel, 0, ccrdToEnd) != icrdNil &&
                SendColMsg(pcolDiscard, msgcFlip, fFalse, 0) &&
                SendColMsg(pcolDiscard, msgcInvert, 0, 0) &&
                SendGmMsg(pgm, msggScore, (INT_PTR) pcolDeck, (INT_PTR) pcolDiscard) &&
                SendColMsg(pcolDeck, msgcMove, (INT_PTR) pcolDiscard, icrdToEnd) &&
                SendColMsg(pcolDiscard, msgcRender, 0, icrdToEnd);
            }
        else
            {
            icrdSel = pcolDeck->pmove->icrdSel-1;
            /* deal next cards to discard */
            return SendGmMsg(pgm, msggSaveUndo, icolDiscard, icolDeck) &&
                SendColMsg(pcolDeck, msgcFlip, fTrue, 0) &&
                SendColMsg(pcolDeck, msgcInvert, 0, 0) &&
                SendColMsg(pcolDiscard, msgcMove, (INT_PTR)pcolDeck, icrdToEnd) &&
                SendColMsg(pcolDeck, msgcRender, icrdSel, icrdToEnd);
            }
        }
    return DefGmProc(pgm, msggMouseDown, (INT_PTR) ppt, icolDiscard);
}


BOOL KlondIsWinner(GM *pgm)
{
    INT icol;

    for(icol = icolFoundFirst; icol < icolFoundFirst+ccolFound; icol++)
        if(pgm->rgpcol[icol]->icrdMac != icrdFoundMax)
            return fFalse;
    return fTrue;
}


BOOL FAbort()
{
    MSG msg;

    if (MsgWaitForMultipleObjects(0, NULL, FALSE, 5, QS_ALLINPUT) != WAIT_OBJECT_0)
        return FALSE;

    if(PeekMessage(&msg, hwndApp, 0, 0, PM_NOREMOVE))
        {
        switch(msg.message)
            {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_MENUSELECT:
        case WM_NCLBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
            return fTrue;
            }

        PeekMessage(&msg, hwndApp, 0, 0, PM_REMOVE);
        TranslateMessage((LPMSG)&msg);
        DispatchMessage((LPMSG)&msg);
        }
    return fFalse;
}



// Hack for making winning animation faster:
// At cascading time we have: KlondWinner -> DrawCardPt ->cdtDrawExt
// so we set a flag so cdtDrawExt knows it is cascading and does not need
// to round up corners.
BOOL fKlondWinner = FALSE;

BOOL KlondWinner(GM *pgm)
{
    INT icol;
    INT icrd;
    CRD *pcrd;
    PT pt;
    PT ptV;
    INT dxp;
    INT dyp;
    RC rcT;
    INT dsco;
    TCHAR *pch;
    TCHAR szBonus[84];
    VOID StatString();

    fKlondWinner = TRUE;

    dsco = (INT)SendGmMsg(pgmCur, msggChangeScore, csKlondWin, 0);
    pgm->udr.fAvail = fFalse;
    pgm->fDealt = fFalse;
    pgm->fWon = fTrue;

    if(smd == smdStandard)
    {
        pch =    &szBonus[CchString(szBonus, idsBonus)];
        pch += CchDecodeInt(pch, dsco);
        *pch++ = TEXT(' ');
        *pch++ = TEXT(' ');
    }
    else
        pch = szBonus;
    CchString(pch, idsEndWinner);

    StatStringSz(szBonus);
    if(!FGetHdc())
        goto ByeNoRel;
    Assert(xOrgCur == 0);
    Assert(yOrgCur == 0);
    GetClientRect(hwndApp, (RECT *)&rcT);
    dxp = rcT.xRight;
    dyp = rcT.yBot - dyCrd;

    for(icrd = icrdFoundMax-1; icrd >= 0; icrd--)
    {
        for(icol = icolFoundFirst; icol < icolFoundFirst+ccolFound; icol++)
        {
            ptV.x = rand() % 110 - 65;  /* favor up and to left */
            if(abs(ptV.x) < 15)  /* kludge so doesn't bounce forever */
                ptV.x = -20;
            ptV.y = rand() % 110 - 75;
            pt = (pcrd = &pgm->rgpcol[icol]->rgcrd[icrd])->pt;

            while(pt.x > -dxCrd && pt.x < dxp)
            {
                DrawCardPt(pcrd, &pt);
                pt.x += ptV.x/10;
                pt.y += ptV.y/10;
                ptV.y+= 3;
                if(pt.y > dyp && ptV.y > 0)
                    ptV.y = -(ptV.y*8)/10;
                if(FAbort())
                    goto ByeBye;
            }

       }
    }
ByeBye:
    ReleaseHdc();
ByeNoRel:
    StatString(idsNil);
    EraseScreen();

    fKlondWinner = FALSE;

    return DefGmProc(pgm, msggWinner, 0, 0);
}

BOOL KlondForceWin(GM *pgm)
{
    INT icol;
    CRD *pcrd;
    COL *pcol;
    RA ra;
    SU su;

    for(icol = 0; icol < pgm->icolMac; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcClearCol, 0, 0);
    for(su = suFirst, icol = icolFoundFirst; icol < icolFoundFirst+ccolFound; icol++, su++)
    {
        Assert(raFirst == 0);
        for(ra = raFirst; ra < raMax; ra++)
        {
            pcol = pgm->rgpcol[icol];
            pcrd = &pcol->rgcrd[ra];
            pcrd->cd = Cd(ra, su);
            pcrd->pt.x = pcol->rc.xLeft;
            pcrd->pt.y = pcol->rc.yTop;
            pcrd->fUp = fTrue;
        }
        pgm->rgpcol[icol]->icrdMac = icrdFoundMax;
    }
    Assert(SendGmMsg(pgm, msggIsWinner, 0, 0));
    return (BOOL)SendGmMsg(pgm, msggWinner, 0, 0);
}


/* Note: assumes is called once a second */
/* if pcolDest == pcolSrc == NULL, then is a timer msg */
BOOL KlondScore(GM *pgm, COL *pcolDest, COL *pcolSrc)
{
    INT cs;
    INT tclsSrc, tclsDest;


    if(smd == smdNone)
        return fTrue;
    cs = csNil;

    Assert(FValidCol(pcolSrc));
    Assert(FValidCol(pcolDest));


    tclsSrc = pcolSrc->pcolcls->tcls;
    tclsDest = pcolDest->pcolcls->tcls;

    switch(tclsDest)
        {
    default:
        return fTrue;
    case tclsDeck:
        if(tclsSrc == tclsDiscard)
            cs = csKlondDeckFlip;
        break;
    case tclsFound:
        switch(tclsSrc)
            {
        default:
            return fTrue;
        case tclsDiscard:
        case tclsTab:
            cs = csKlondFound;
            break;
            }
        break;
    case tclsTab:
        switch(tclsSrc)
            {
        default:
            return fTrue;
        case tclsDiscard:
            cs = csKlondTab;
            break;
        case tclsFound:
            cs = csKlondFoundTab;
            break;
            }
        break;
         }

    SendGmMsg(pgm, msggChangeScore, cs, 0);
    return fTrue;
}

INT mpcsdscoStd[] = { -2, -20, 10, 5, 5, -15,   0,  0};
INT mpcsdscoVegas[] = {0,   0,  5, 0, 0,  -5, -52,  0};


BOOL KlondChangeScore(GM *pgm, INT cs, INT sco)
{
    INT dsco;
    INT csNew;
    INT *pmpcsdsco;
    INT ret;

    if(cs < 0)
        return DefGmProc(pgm, msggChangeScore, cs, sco);
    Assert(FInRange(cs, 0, csKlondMax-1));
    switch(smd)
    {
        default:
            Assert(smd == smdNone);
            return fTrue;
        case smdVegas:
            pmpcsdsco = mpcsdscoVegas;
            break;

        case smdStandard:
            pmpcsdsco = mpcsdscoStd;
            if(cs == csKlondWin && fTimedGame)
            {
    #ifdef DEBUG
                pgm->iqsecScore = WMax(120, pgm->iqsecScore);
    #endif
                /* check if timer set properly */
                if(pgm->iqsecScore >= 120)
                    dsco = (20000/(pgm->iqsecScore>>2))*(350/10);
                else
                    dsco = 0;
                goto DoScore;
            }
            if(cs == csKlondDeckFlip)
            {
                if(ccrdDeal == 1 && pgm->irep >= 1)
                {
                    dsco = -100;
                    goto DoScore;
                }
                else if(ccrdDeal == 3 && pgm->irep > 3)
                    break;
                else
                    return fTrue;
            }
            break;
    }

    dsco = pmpcsdsco[cs];
DoScore:
    csNew = smd == smdVegas ? csDel : csDelPos;
    ret = DefGmProc(pgm, msggChangeScore, csNew, dsco);
    if(cs == csKlondWin)
        return dsco;
    else
        return ret;
}



BOOL KlondTimer(GM *pgm, INT wp1, INT wp2)
{

    if(fTimedGame && pgm->fDealt && pgm->fInput && !fIconic)
    {
        pgm->iqsecScore = WMin(pgm->iqsecScore+1, 0x7ffe);
        if(pgm->icolSel == icolNil)
            SendColMsg(pgm->rgpcol[icolDeck], msgcAnimate, pgm->iqsecScore, 0);
        if(pgm->dqsecScore != 0 && (pgm->iqsecScore)%pgm->dqsecScore == 0)
        {
            SendGmMsg(pgm, msggChangeScore, csKlondTime, 0);
        }
        else
        {
            /* update status bar once as second */
            if(~(pgm->iqsecScore & 0x03))
                StatUpdate();
            return fTrue;
        }
   }
   return fFalse;
}

BOOL KlondDrawStatus(GM *pgm, RC *prc)
{
    TCHAR *pch;
    TCHAR sz[80];
    RC rc;
    LONG rgb;
    BOOL fNegSco;
    SIZE   iSize;
    extern INT iCurrency;
    extern TCHAR szCurrency[];

    pch = sz;
    if(fTimedGame)
    {
        pch += CchString(pch, idsTime);
        pch += CchDecodeInt(pch, (pgm->iqsecScore>>2));
    }
#ifdef DEBUG
    if(!fScreenShots)
    {
        *pch++ = TEXT(' ');
        pch = PszCopy(TEXT("Game # "), pch);
        pch += CchDecodeInt(pch, igmCur);
    }
#endif
    if(pch != sz)
    {
        DrawText(hdcCur, sz, (INT)(pch-sz), (LPRECT) prc, DT_RIGHT|DT_NOCLIP|DT_SINGLELINE);
    }

    if(smd != smdNone)
    {
        rc = *prc;
        GetTextExtentPoint32(hdcCur, sz, (INT)(pch-sz), &iSize);
        rc.xRight -= iSize.cx;
        pch = sz;
        if(fNegSco = pgm->sco < 0)
            *pch++ = TEXT('-');
        if(smd == smdVegas)
        {
            if(!(iCurrency&1))
            {
                pch = PszCopy(szCurrency, pch);
                if(iCurrency == 2)
                    *pch++ = TEXT(' ');
            }
        }
        pch += CchDecodeInt(pch, fNegSco ? -pgm->sco : pgm->sco);
        if(smd == smdVegas)
        {
            if(iCurrency&1)
            {
                if(iCurrency == 3)
                    *pch++ = TEXT(' ');
                pch = PszCopy(szCurrency, pch);
            }
        }
        *pch++ = TEXT(' ');
        rgb = SetTextColor(hdcCur, (!fBW && fNegSco) ? RGB(0xff, 0, 0) : RGB(0, 0, 0));
        DrawText(hdcCur, sz, (INT)(pch-sz), (LPRECT) &rc, DT_RIGHT|DT_NOCLIP|DT_SINGLELINE);
        SetTextColor(hdcCur, rgb);

        GetTextExtentPoint32(hdcCur, sz, (INT)(pch-sz), &iSize);
        rc.xRight -= iSize.cx;
        pch = PszCopy(szScore, sz);
        DrawText(hdcCur, sz, (INT)(pch-sz), (LPRECT) &rc, DT_RIGHT|DT_NOCLIP|DT_SINGLELINE);
        GetTextExtentPoint32(hdcCur, sz, (INT)(pch-sz), &iSize);
        rc.xRight -= iSize.cx;
        rc.xLeft = rc.xRight - 4 * dxChar;
        PatBlt(hdcCur, rc.xLeft, rc.yTop, rc.xRight-rc.xLeft, rc.yBot-rc.yTop, PATCOPY);
        }
    return fTrue;
}




LRESULT KlondGmProc(GM *pgm, INT msgg, WPARAM wp1, LPARAM wp2)
{
    switch(msgg)
        {
    case msggMouseDblClk:
        if(DefGmProc(pgm, msggMouseDblClk, wp1, wp2))
            return fTrue;
        /* fall thru so works for deck double clicks */
    case msggMouseDown:
        return KlondMouseDown(pgm, (PT *)wp1);

    case msggDeal:
        return KlondDeal(pgm, (BOOL)wp1);

    case msggIsWinner:
        return KlondIsWinner(pgm);

    case msggWinner:
        return KlondWinner(pgm);

    case msggForceWin:
        return KlondForceWin(pgm);

    case msggScore:
        return KlondScore(pgm, (COL *) wp1, (COL *)wp2);

    case msggChangeScore:
        return KlondChangeScore(pgm, (INT)wp1, (INT)wp2);

    case msggTimer:
        return KlondTimer(pgm, (INT)wp1, (INT)wp2);


    case msggDrawStatus:
        return KlondDrawStatus(pgm, (RC *) wp1);
        }
    return DefGmProc(pgm, msgg, wp1, wp2);
}
