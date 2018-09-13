#include "sol.h"
#ifndef DLL
VSZASSERT
#else
#ifdef DEBUG
#undef Assert
#define Assert(f) { if (!(f)) { ExitWindows(0L); } }
#endif
#endif


#define COOLCARD


#ifdef COOLCARD
void SaveCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy);
void RestoreCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy);
#endif

VOID APIENTRY cdtTerm(VOID);
VOID MyDeleteHbm(HBITMAP hbm);
static HBITMAP HbmFromCd(INT, HDC);
static BOOL FLoadBack(INT);


typedef struct
        {
        INT id;
        DX  dx;
        DY       dy;
        } SPR;

#define isprMax 4

typedef struct
        {
        INT cdBase;
        DX dxspr;
        DY dyspr;
        INT isprMac;
        SPR rgspr[isprMax];
        } ANI;

#define ianiMax 4
static ANI rgani[ianiMax] =
        { IDFACEDOWN12, 32, 22, 4,
                {IDASLIME1, 32, 32,
                 IDASLIME2, 32, 32,
                 IDASLIME1, 32, 32,
                 IDFACEDOWN12, 32, 32
                },
          IDFACEDOWN10, 36, 12, 2,
                {IDAKASTL1, 42, 12,
                 IDFACEDOWN10, 42, 12,
                 0, 0, 0,
                 0, 0, 0
                },
          IDFACEDOWN11, 14, 12, 4,
                {
                IDAFLIPE1, 47, 1,
                IDAFLIPE2, 47, 1,
                IDAFLIPE1, 47, 1,
                IDFACEDOWN11, 47, 1
                },
          IDFACEDOWN3, 24, 7, 4,
                {
                IDABROBOT1, 24, 40,
                IDABROBOT2, 24, 40,
                IDABROBOT1, 24, 40,
                IDFACEDOWN3, 24, 40
                }
        /* remember to inc ianiMax */
        };



static HBITMAP  hbmCard[52] =
        {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
         NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
         NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
         NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static HBITMAP hbmGhost = NULL;
static HBITMAP hbmBack = NULL;
static HBITMAP hbmDeckX = NULL;
static HBITMAP hbmDeckO = NULL;
static INT idback = 0;
static INT dxCard, dyCard;
static INT cInits = 0;


#ifdef DLL
HANDLE hinstApp;
#else
extern HANDLE  hinstApp;
#endif

BOOL APIENTRY cdtInit(INT FAR *pdxCard, INT FAR *pdyCard)
/*
 * Parameters:
 *      pdxCard, pdyCard
 *                      Far pointers to ints where card size will be placed
 *
 * Returns:
 *      True when successful, False when it can't find one of the standard
 *      bitmaps.
 */
        {

        BITMAP bmCard;
        HDC hdc = NULL;
        HBITMAP hbmDstBitmap;
        HANDLE hDstOld;
        HANDLE hSrcOld;
        HDC hdcDstMemory;
        HDC hdcSrcMemory;

#ifdef DLL
        if (cInits++ != 0)
                {
                *pdxCard = dxCard;
                *pdyCard = dyCard;
                return fTrue;
                }
#endif

        hbmGhost = LoadBitmap( hinstApp, MAKEINTRESOURCE(IDGHOST));
        hbmDeckX = LoadBitmap( hinstApp, MAKEINTRESOURCE(IDX));
        hbmDeckO = LoadBitmap( hinstApp, MAKEINTRESOURCE(IDO));
        if(hbmGhost == NULL || hbmDeckX == NULL || hbmDeckO == NULL) {
                goto Fail;
        }

        GetObject( hbmGhost, sizeof( BITMAP), (LPSTR)&bmCard);
        dxCard = *pdxCard = bmCard.bmWidth;
        dyCard = *pdyCard = bmCard.bmHeight;

        //
        // Create two compatible memory DCs for bitmap conversion.
        //

        hdc = GetDC(NULL);
        hdcSrcMemory = CreateCompatibleDC(hdc);
        hdcDstMemory = CreateCompatibleDC(hdc);
        if ((hdcSrcMemory == NULL) || (hdcDstMemory == NULL)) {
            goto Fail;
        }

        //
        // Create a compatible bitmap for the conversion of the Ghost
        // bitmap, blt the loaded bitmap to the compatible bitmap, and
        // delete the original bitmap.
        //

        hbmDstBitmap = CreateCompatibleBitmap(hdc, dxCard, dyCard);
        if (hbmDstBitmap == NULL) {
            goto Fail;
        }

        hSrcOld = SelectObject(hdcSrcMemory, hbmGhost);
        hDstOld = SelectObject(hdcDstMemory, hbmDstBitmap);
        BitBlt(hdcDstMemory, 0, 0, dxCard, dyCard, hdcSrcMemory, 0, 0, SRCCOPY);
        SelectObject(hdcSrcMemory, hSrcOld);
        SelectObject(hdcDstMemory, hDstOld);
        DeleteObject(hbmGhost);
        hbmGhost = hbmDstBitmap;

        //
        // Create a compatible bitmap for the conversion of the DeckX
        // bitmap, blt the loaded bitmap to the compatible bitmap, and
        // delete the original bitmap.
        //

        hbmDstBitmap = CreateCompatibleBitmap(hdc, dxCard, dyCard);
        if (hbmDstBitmap == NULL) {
            goto Fail;
        }

        hSrcOld = SelectObject(hdcSrcMemory, hbmDeckX);
        hDstOld = SelectObject(hdcDstMemory, hbmDstBitmap);
        BitBlt(hdcDstMemory, 0, 0, dxCard, dyCard, hdcSrcMemory, 0, 0, SRCCOPY);
        SelectObject(hdcSrcMemory, hSrcOld);
        SelectObject(hdcDstMemory, hDstOld);
        DeleteObject(hbmDeckX);
        hbmDeckX = hbmDstBitmap;

        //
        // Create a compatible bitmap for the conversion of the DeckO
        // bitmap, blt the loaded bitmap to the compatible bitmap, and
        // delete the original bitmap.
        //

        hbmDstBitmap = CreateCompatibleBitmap(hdc, dxCard, dyCard);
        if (hbmDstBitmap == NULL) {
        }

        hSrcOld = SelectObject(hdcSrcMemory, hbmDeckO);
        hDstOld = SelectObject(hdcDstMemory, hbmDstBitmap);
        BitBlt(hdcDstMemory, 0, 0, dxCard, dyCard, hdcSrcMemory, 0, 0, SRCCOPY);
        SelectObject(hdcSrcMemory, hSrcOld);
        SelectObject(hdcDstMemory, hDstOld);
        DeleteObject(hbmDeckO);
        hbmDeckO = hbmDstBitmap;

        //
        // Delete the compatible DCs.
        //

        DeleteDC(hdcDstMemory);
        DeleteDC(hdcSrcMemory);
        ReleaseDC(NULL, hdc);
        return fTrue;

Fail:
        if (hdc != NULL) {
            ReleaseDC(NULL, hdc);
        }

        cdtTerm();
        return fFalse;
        }




BOOL APIENTRY cdtDrawExt(HDC hdc, INT x, INT y, INT dx, INT dy, INT cd, INT mode, DWORD rgbBgnd)
/*
 * Parameters:
 *      hdc     HDC to window to draw cards on
 *      x, y    Where you'd like them
 * dx,dy card extents
 *      cd              Card to be drawn
 *      mode    Way you want it drawn
 *
 * Returns:
 *      True if card successfully drawn, False otherwise
 */
{

        HBITMAP  hbmSav;
        HDC      hdcMemory;
        DWORD    dwRop;
        HBRUSH   hbr;
#ifdef COOLCARD
        LONG     rgRGB[12];
#endif

        Assert(hdc != NULL);
                switch (mode)
                        {
                default:
                        Assert(fFalse);
                        break;
                case FACEUP:
                        hbmSav = HbmFromCd(cd, hdc);
                        dwRop = SRCCOPY;
                        break;
                case FACEDOWN:
                        if(!FLoadBack(cd))
                                return fFalse;
                        hbmSav = hbmBack;
                        dwRop = SRCCOPY;
                        break;
                case REMOVE:
                case GHOST:
                        hbr = CreateSolidBrush( rgbBgnd);
                        if(hbr == NULL)
                                return fFalse;

                        MUnrealizeObject( hbr);
                        if((hbr = SelectObject( hdc, hbr)) != NULL)
                                {
                        PatBlt(hdc, x, y, dx, dy, PATCOPY);
                                hbr = SelectObject( hdc, hbr);
                                }
                        DeleteObject( hbr);
                        if(mode == REMOVE)
                                return fTrue;
                        Assert(mode == GHOST);
                        /* default: fall thru */

                case INVISIBLEGHOST:
                        hbmSav = hbmGhost;
                        dwRop = SRCAND;
                        break;

                case DECKX:
                        hbmSav = hbmDeckX;
                        dwRop = SRCCOPY;
                        break;
                case DECKO:
                        hbmSav = hbmDeckO;
                        dwRop = SRCCOPY;
                        break;

                case HILITE:
                        hbmSav = HbmFromCd(cd, hdc);
                        dwRop = NOTSRCCOPY;
                        break;
                        }
        if (hbmSav == NULL)
                return fFalse;
        else
                {
        hdcMemory = CreateCompatibleDC( hdc);
                if(hdcMemory == NULL)
                        return fFalse;

                if((hbmSav = SelectObject( hdcMemory, hbmSav)) != NULL)
                        {
#ifdef COOLCARD
                        if( !fKlondWinner )
							SaveCorners(hdc, rgRGB, x, y, dx, dy);
#endif
                        if(dx != dxCard || dy != dyCard)
                                StretchBlt(hdc, x, y, dx, dy, hdcMemory, 0, 0, dxCard, dyCard, dwRop);
                        else
                                BitBlt( hdc, x, y, dxCard, dyCard, hdcMemory, 0, 0, dwRop);

                SelectObject( hdcMemory, hbmSav);
                        /* draw the border for the red cards */
                        if(mode == FACEUP)
                                {
                                INT icd;

                                icd = RaFromCd(cd) % 13 + SuFromCd(cd) * 13+1;
                                if((icd >= IDADIAMONDS && icd <= IDTDIAMONDS) ||
                                        (icd >= IDAHEARTS && icd <= IDTHEARTS))
                                        {
                                        PatBlt(hdc, x+2, y, dx-4, 1, BLACKNESS);  /* top */
                                        PatBlt(hdc, x+dx-1, y+2, 1, dy-4, BLACKNESS); /* right */
                                        PatBlt(hdc, x+2, y+dy-1, dx-4, 1, BLACKNESS); /* bottom */
                                        PatBlt(hdc, x, y+2, 1, dy-4, BLACKNESS); /* left */
                                        SetPixel(hdc, x+1, y+1, 0L); /* top left */
                                        SetPixel(hdc, x+dx-2, y+1, 0L); /*  top right */
                                        SetPixel(hdc, x+dx-2, y+dy-2, 0L); /* bot right */
                                        SetPixel(hdc, x+1, y+dy-2, 0L); /* bot left */
                                        }
                                }
#ifdef COOLCARD
                        if( !fKlondWinner )
							RestoreCorners(hdc, rgRGB, x, y, dx, dy);
#endif
                        }
        DeleteDC( hdcMemory);
                return fTrue;
                }
        }






BOOL APIENTRY cdtDraw(HDC hdc, INT x, INT y, INT cd, INT mode, DWORD rgbBgnd)
/*
 * Parameters:
 *      hdc             HDC to window to draw cards on
 *      x, y    Where you'd like them
 *      cd              Card to be drawn
 *      mode    Way you want it drawn
 *
 * Returns:
 *      True if card successfully drawn, False otherwise
 */
        {

        return cdtDrawExt(hdc, x, y, dxCard, dyCard, cd, mode, rgbBgnd);
        }


#ifdef COOLCARD

void SaveCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy)
        {
        if(dx != dxCard || dy != dyCard)
                return;

        /* Upper Left */
        rgRGB[0] = GetPixel(hdc, x, y);
        rgRGB[1] = GetPixel(hdc, x+1, y);
        rgRGB[2] = GetPixel(hdc, x, y+1);

        /* Upper Right */
        x += dx -1;
        rgRGB[3] = GetPixel(hdc, x, y);
        rgRGB[4] = GetPixel(hdc, x-1, y);
        rgRGB[5] = GetPixel(hdc, x, y+1);

        /* Lower Right */
        y += dy-1;
        rgRGB[6] = GetPixel(hdc, x, y);
        rgRGB[7] = GetPixel(hdc, x, y-1);
        rgRGB[8] = GetPixel(hdc, x-1, y);

        /* Lower Left */
        x -= dx-1;
        rgRGB[9] = GetPixel(hdc, x, y);
        rgRGB[10] = GetPixel(hdc, x+1, y);
        rgRGB[11] = GetPixel(hdc, x, y-1);

        }



void RestoreCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy)
        {
        if(dx != dxCard || dy != dyCard)
                return;

        /* Upper Left */
        SetPixel(hdc, x, y, rgRGB[0]);
        SetPixel(hdc, x+1, y, rgRGB[1]);
        SetPixel(hdc, x, y+1, rgRGB[2]);

        /* Upper Right */
        x += dx-1;
        SetPixel(hdc, x, y, rgRGB[3]);
        SetPixel(hdc, x-1, y, rgRGB[4]);
        SetPixel(hdc, x, y+1, rgRGB[5]);

        /* Lower Right */
        y += dy-1;
        SetPixel(hdc, x, y, rgRGB[6]);
        SetPixel(hdc, x, y-1, rgRGB[7]);
        SetPixel(hdc, x-1, y, rgRGB[8]);

        /* Lower Left */
        x -= dx-1;
        SetPixel(hdc, x, y, rgRGB[9]);
        SetPixel(hdc, x+1, y, rgRGB[10]);
        SetPixel(hdc, x, y-1, rgRGB[11]);
        }
#endif






BOOL APIENTRY cdtAnimate(HDC hdc, INT cd, INT x, INT y, INT ispr)
        {
        INT iani;
        ANI *pani;
        SPR *pspr;
        HBITMAP hbm;
        HDC hdcMem;
        X xSrc;
        Y ySrc;

        if(ispr < 0)
                return fFalse;
        Assert(hdc != NULL);
        for(iani = 0; iani < ianiMax; iani++)
                {
                if(cd == rgani[iani].cdBase)
                        {
                        pani = &rgani[iani];
                        if(ispr < pani->isprMac)
                                {
                                pspr = &pani->rgspr[ispr];
                                Assert(pspr->id != 0);
                                if(pspr->id == cd)
                                        {
                                        xSrc = pspr->dx;
                                        ySrc = pspr->dy;
                                        }
                                else
                                        xSrc = ySrc = 0;

                                hbm = LoadBitmap(hinstApp, MAKEINTRESOURCE(pspr->id));
                                if(hbm == NULL)
                                        return fFalse;

                        hdcMem = CreateCompatibleDC(hdc);
                                if(hdcMem == NULL)
                                        {
                                        DeleteObject(hbm);
                                        return fFalse;
                                        }

                                if((hbm = SelectObject(hdcMem, hbm)) != NULL)
                                        {
                                        BitBlt(hdc, x+pspr->dx, y+pspr->dy, pani->dxspr, pani->dyspr,
                                                hdcMem, xSrc, ySrc, SRCCOPY);
                                        DeleteObject(SelectObject(hdcMem, hbm));
                                        }
                                DeleteDC(hdcMem);
                                return fTrue;
                                }
                        }
                }
        return fFalse;
        }



/* loads global bitmap hbmBack */
BOOL FLoadBack(INT idbackNew)
        {

        Assert(FInRange(idbackNew, IDFACEDOWNFIRST, IDFACEDOWNLAST));

        if(idback != idbackNew)
                {
                MyDeleteHbm(hbmBack);
                if((hbmBack = LoadBitmap(hinstApp, MAKEINTRESOURCE(idbackNew))) != NULL)
                        idback = idbackNew;
                else
                        idback = 0;
                }
        return idback != 0;
        }



static HBITMAP HbmFromCd(INT cd, HDC hdc)

{

    INT icd;
    HBITMAP hbmDstBitmap;
    HANDLE hDstOld;
    HANDLE hSrcOld;
    HDC hdcDstMemory;
    HDC hdcSrcMemory;

    if (hbmCard[cd] == NULL) {
        icd = RaFromCd(cd) % 13 + SuFromCd(cd) * 13;
        if ((hbmCard[cd] = LoadBitmap(hinstApp,MAKEINTRESOURCE(icd+1))) == NULL) {
            return NULL;
        }

        //
        // Create two compatible memory DCs for bitmap conversion.
        //

        hdcSrcMemory = CreateCompatibleDC(hdc);
        hdcDstMemory = CreateCompatibleDC(hdc);
        if ((hdcSrcMemory == NULL) || (hdcDstMemory == NULL)) {
            goto Finish;
        }

        //
        // Create a compatible bitmap for the conversion of the card
        // bitmap, blt the loaded bitmap to the compatible bitmap, and
        // delete the original bitmap.
        //

        hbmDstBitmap = CreateCompatibleBitmap(hdc, dxCard, dyCard);
        if (hbmDstBitmap == NULL) {
            goto Finish;
        }

        hSrcOld = SelectObject(hdcSrcMemory, hbmCard[cd]);
        hDstOld = SelectObject(hdcDstMemory, hbmDstBitmap);
        BitBlt(hdcDstMemory, 0, 0, dxCard, dyCard, hdcSrcMemory, 0, 0, SRCCOPY);
        SelectObject(hdcSrcMemory, hSrcOld);
        SelectObject(hdcDstMemory, hDstOld);
        DeleteObject(hbmCard[cd]);
        hbmCard[cd] = hbmDstBitmap;

        //
        // Delete the compatible DCs.
        //

        DeleteDC(hdcDstMemory);
        DeleteDC(hdcSrcMemory);
    }

Finish:
    return hbmCard[cd];
}


VOID MyDeleteHbm(HBITMAP hbm)
        {
        if(hbm != NULL)
                DeleteObject(hbm);
        }

VOID APIENTRY cdtTerm()
/*
 * Free up space if it's time to do so.
 *
 * Parameters:
 *      none
 *
 * Returns:
 *      nothing
 */
        {
        INT     i;
#ifdef DLL
        if (--cInits > 0) return;
#endif

        for (i = 0; i < 52; i++) {
            MyDeleteHbm(hbmCard[i]);
            hbmCard[i] = NULL;
        }

        MyDeleteHbm(hbmGhost);
        MyDeleteHbm(hbmBack);
        MyDeleteHbm(hbmDeckX);
        MyDeleteHbm(hbmDeckO);
        }
