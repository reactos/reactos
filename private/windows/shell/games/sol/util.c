#include "sol.h"
VSZASSERT




VOID *PAlloc(INT cb)
{
    TCHAR *p;

    // KLUDGE: solve overwriting memory by allocating more
    #define MEMORYPAD 200
    p = (TCHAR *)LocalAlloc(LPTR, cb+MEMORYPAD);
    Assert(p != NULL);
    return (VOID *)p;
}


VOID FreeP(VOID *p)
{
    LocalFree((HANDLE) p);
}



VOID InvertRc(RC *prc)
{
    Assert(xOrgCur == 0);
    Assert(yOrgCur == 0);
    AssertHdcCur();

    InvertRect(hdcCur, (LPRECT) prc);
}


VOID DrawCard(CRD *pcrd)
{
    AssertHdcCur();
    cdtDrawExt( hdcCur,
                pcrd->pt.x-xOrgCur,
                pcrd->pt.y-yOrgCur,
                dxCrd,
                dyCrd,
                pcrd->fUp ? pcrd->cd : modeFaceDown,
                pcrd->fUp ? FACEUP : FACEDOWN,
                rgbTable);
}


VOID DrawOutline(PT *ppt, INT ccrd, DX dx, DY dy)
{
    Y y;
    PT pt;
    INT rop2;
    if(!FGetHdc())
        return;

    pt = *ppt;

    rop2 = SetROP2(hdcCur, R2_NOT);
    MMoveTo(hdcCur, pt.x, pt.y);
    LineTo(hdcCur, pt.x+dxCrd, pt.y);
    LineTo(hdcCur, pt.x+dxCrd, y = pt.y+dyCrd+(ccrd-1) * dy);
    LineTo(hdcCur, pt.x, y);
    LineTo(hdcCur, pt.x, pt.y);
    y = pt.y;
    while(--ccrd)
    {
        y += dy;
        MMoveTo(hdcCur, pt.x, y);
        LineTo(hdcCur, pt.x+dxCrd, y);
    }
    SetROP2(hdcCur, rop2);
    ReleaseHdc();
}

VOID DrawCardPt(CRD *pcrd, PT *ppt)
{
    DWORD dwModeExt=0;     // turn on sign bit if moving fast
                           // cdtDrawExt must support this!

    if( fKlondWinner )
    {
        dwModeExt= MINLONG;
    }

    AssertHdcCur();
    cdtDrawExt(hdcCur,
               ppt->x-xOrgCur,
               ppt->y-yOrgCur,
               dxCrd,
               dyCrd,
               pcrd->fUp ? pcrd->cd : modeFaceDown,
               (pcrd->fUp ? FACEUP : FACEDOWN ) | dwModeExt,
               rgbTable);
}

VOID DrawCardExt(PT *ppt, INT cd, INT mode)
{
    VOID DrawBackground();

    AssertHdcCur();

    cdtDrawExt( hdcCur,
                ppt->x-xOrgCur,
                ppt->y-yOrgCur,
                dxCrd,
                dyCrd,
                cd,
                mode,
                rgbTable);
}


VOID DrawBackground(X xLeft, Y yTop, X xRight, Y yBot)
{
    HBRUSH hbr;


    AssertHdcCur();
    MSetBrushOrg(hdcCur, xOrgCur, yOrgCur);
    MUnrealizeObject(hbrTable);
    if((hbr = SelectObject(hdcCur, hbrTable)) != NULL)
    {
        Assert(xRight >= xLeft);
        Assert(yBot >= yTop);
        PatBlt( hdcCur,
                xLeft-xOrgCur,
                yTop-yOrgCur,
                xRight-xLeft,
                yBot-yTop,
                PATCOPY);
        SelectObject(hdcCur, hbr);
    }
}


VOID EraseScreen(VOID)
{
    RC rc;
    HDC HdcSet();

    if(!FGetHdc())
        return;
    GetClientRect(hwndApp, (LPRECT) &rc);
    DrawBackground(rc.xLeft, rc.yTop, rc.xRight, rc.yBot);
    ReleaseHdc();
}





BOOL FPtInCrd(CRD *pcrd, PT pt)
{

    return(pt.x >= pcrd->pt.x && pt.x < pcrd->pt.x+dxCrd &&
             pt.y >= pcrd->pt.y && pt.y < pcrd->pt.y+dyCrd);
}




BOOL FRectIsect(RC *prc1, RC *prc2)
{
    RC rcDummy;

    return(IntersectRect((LPRECT) &rcDummy, (LPRECT) prc1, (LPRECT) prc2));
}


VOID CrdRcFromPt(PT *ppt, RC *prc)
{
    prc->xRight = (prc->xLeft = ppt->x) + dxCrd;
    prc->yBot = (prc->yTop = ppt->y) + dyCrd;
}


BOOL FCrdRectIsect(CRD *pcrd, RC *prc)
{
    RC rcDummy;
    RC rcCrd;

    CrdRcFromPt(&pcrd->pt, &rcCrd);
    return(IntersectRect((LPRECT) &rcDummy, (LPRECT) &rcCrd, (LPRECT) prc));
}

/* BUG: only considers upper left and lower right corners */
/* this is ok for my purposes now, but beware... */
BOOL FRectAllVisible(HDC hdc, RC *prc)
{
    return PtVisible(hdc, prc->xLeft, prc->yTop) && PtVisible(hdc, prc->xRight, prc->yBot);
}


VOID OffsetPt(PT *ppt, DEL *pdel, PT *pptDest)
{
    pptDest->x = ppt->x + pdel->dx;
    pptDest->y = ppt->y + pdel->dy;
}


VOID SwapCards(CRD *pcrd1, CRD *pcrd2)
{
    CRD crdT;

    crdT = *pcrd1;
    *pcrd1 = *pcrd2;
    *pcrd2 = crdT;
}





TCHAR *PszCopy(TCHAR *pszFrom, TCHAR *rgchTo)
{
    while (*rgchTo++ = *pszFrom++)
        ;
    return(rgchTo-1);
}



INT CchDecodeInt(TCHAR *rgch, INT_PTR w)
{
    INT fNeg;
    TCHAR *pch, *pchT;
    TCHAR rgchT[20];

    if (fNeg = w<0)
        w = -w;

    pchT = rgchT;
    do
    {
        *pchT++ = (TCHAR)(TEXT('0') + (TCHAR) (w % 10));
        w /= 10;
    }
    while (w);
    pch = rgch;
    if (fNeg)
        *pch++ = TEXT('-');
    do
        *pch++ = *--pchT;
    while (pchT > rgchT);
    *pch = TEXT('\000');
    return((INT)(pch - rgch));
}

VOID Error(TCHAR *sz)
{

    MessageBox(hwndApp, (LPTSTR)sz, (LPTSTR)szAppName, MB_OK|MB_ICONEXCLAMATION);
}

/* returns fTrue if yes is clicked  */
BOOL FYesNoAlert( INT ids )
{
    TCHAR sz[128];
    INT id;

    CchString(sz, ids);
    id = MessageBox(hwndApp, sz, szAppName, MB_YESNO|MB_ICONEXCLAMATION);
    return id == IDYES || id == IDOK;
}


VOID ErrorIds(INT ids)
{
    TCHAR sz[128];

    CchString(sz, ids);
    Error(sz);
}

INT WMin(INT w1, INT w2)
{
    return(w1 < w2 ? w1 : w2);
}


INT WMax(INT w1, INT w2)
{
    return(w1 > w2 ? w1 : w2);
}


BOOL FInRange(INT w, INT wFirst, INT wLast)
{
    Assert(wFirst <= wLast);
    return(w >= wFirst && w <= wLast);
}


INT PegRange(INT w, INT wFirst, INT wLast)
{
    Assert(wFirst <= wLast);
    if(w < wFirst)
        return wFirst;
    else if(w > wLast)
        return wLast;
    else
        return w;
}


VOID OOM()
{
    Error(szOOM);
}

VOID NYI()
{
    Error(TEXT("Not Yet Implemented"));
}

INT CchString(TCHAR *sz, INT ids)
{
    return LoadString(hinstApp, (WORD)ids, (LPTSTR)sz, 255);
}



BOOL FWriteIniString(INT idsTopic, INT idsItem, TCHAR *szValue)
{
    TCHAR szItem[32];
    TCHAR szTopic[32];

    CchString(szTopic, idsTopic);
    CchString(szItem, idsItem);
    return WriteProfileString(szTopic, szItem, szValue);
}

// BabakJ: w was retyped from int to WORD2DWORD, casted to INT when calling
// CchDecodeInt()
BOOL FWriteIniInt(INT idsTopic, INT idsItem, WORD2DWORD w)
{
    TCHAR sz[32];

    CchDecodeInt(sz, (INT)w);
    return FWriteIniString(idsTopic, idsItem, sz);
}


BOOL FGetIniString(INT idsTopic, INT idsItem, TCHAR *sz, TCHAR *szDefault, INT cchMax)
{
    TCHAR szItem[32];
    TCHAR szTopic[32];

    CchString(szTopic, idsTopic);
    CchString(szItem, idsItem);

    GetProfileString(szTopic, szItem, szDefault, sz, cchMax);
    return fTrue;
}


// BabakJ: replaced int ret type with WORD2DWORD
WORD2DWORD GetIniInt(INT idsTopic, INT idsItem, INT wDefault)
{
    TCHAR szItem[32];
    TCHAR szTopic[32];

    CchString(szTopic, idsTopic);
    CchString(szItem, idsItem);

    return GetProfileInt(szTopic, szItem, wDefault);
}
