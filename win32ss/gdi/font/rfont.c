/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h">
DBG_DEFAULT_CHANNEL(GdiFont);

ULONG gulRFONTUnique = 0;
PRFONT gprfntSystemTT;

PRFONT
NTAPI
RFONT_AllocRFONT(void)
{
    PRFONT prfnt;

    /* Allocate the RFONT structure */
    prfnt = ExAllocatePoolWithTag(PagedPool, sizeof(RFONT), GDITAG_RFONT);
    if (!prfnt)
    {
        ERR("Not enough memory to allocate RFONT\n");
        return NULL;
    }

    /* Zero out the whole structure */
    RtlZeroMemory(prfnt, sizeof(RFONT));

    /* Set a unique number */
    prfnt->fobj.iUniq = InterlockedIncrementUL(&gulRFONTUnique);


    return prfnt;
}

VOID
NTAPI
RFONT_vDeleteRFONT(
    _Inout_ PRFONT prfnt)
{
    ASSERT(prfnt->cSelected == 0);

    /* Free the structure */
    ExFreePoolWithTag(prfnt, GDITAG_RFONT);

}

BOOL
NTAPI
RFONT_bQueryDeviceMetrics(
    PRFONT prfnt)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)prfnt->hdevProducer;
    PLDEVOBJ pldev = ppdev->pldev;
    ULONG ulResult;

    /* Preinitialize some fields */
    RtlZeroMemory(&prfnt->fddm, sizeof(FD_DEVICEMETRICS));
    prfnt->fddm.lNonLinearExtLeading = 0x80000000;
    prfnt->fddm.lNonLinearIntLeading = 0x80000000;
    prfnt->fddm.lNonLinearMaxCharWidth = 0x80000000;
    prfnt->fddm.lNonLinearAvgCharWidth = 0x80000000;
    prfnt->fddm.fdxQuantized = prfnt->fdx;

    /* Call the fontdriver */
    ulResult = pldev->pfn.QueryFontData(prfnt->ppff->dhpdev,
                                        &prfnt->fobj,
                                        QFD_MAXEXTENTS,
                                        -1,
                                        NULL,
                                        &prfnt->fddm,
                                        sizeof(FD_DEVICEMETRICS));

    /* Calculate max extents (This seems to be what Windows does) */
    prfnt->fxMaxExtent = max(prfnt->fddm.fxMaxAscender, 0) +
                         max(prfnt->fddm.fxMaxDescender, 0);

    return (ulResult != FD_ERROR);
}

#if 0
RFONT_bRealizeFont()
{

    if (!RFONT_bQueryDeviceMetrics(prfnt))
    {
        __debugbreak();
    }

    prfnt->fobj.iUniq;
    prfnt->fobj.iFace;
    prfnt->fobj.cxMax;
    prfnt->fobj.flFontType;
    prfnt->fobj.iTTUniq;
    prfnt->fobj.iFile;
    prfnt->fobj.sizLogResPpi;
    prfnt->fobj.ulStyleSize;
    prfnt->fobj.pvConsumer;
    prfnt->fobj.pvProducer;


    ULONG          iUnique;                // 02c
    FLONG          flType;                 // 030
    ULONG          ulContent;              // 034
    PVOID          hdevProducer;           // 038
    BOOL           bDeviceFont;            // 03c
    PVOID          hdevConsumer;           // 040
    DHPDEV         dhpdev;                 // 044
    PFE *          ppfe;                   // 048
    PFF *          pPFF;                   // 04c
    FD_XFORM       fdx;                    // 050
    ULONG          cBitsPerPel;            // 060
    MATRIX         mxWorldToDevice;        // 064
    ULONG          iGraphicsMode;          // 0a0
    FLOATOBJ       eptflNtoWScale_x_i;     // 0a4
    FLOATOBJ       eptflNtoWScale_y_i;     // 0ac
    ULONG          bNtoWIdent;             // 0b4
    EXFORMOBJ      xoForDDI;               // 0b8
    unsigned       unk_000;                // 0c0
    MATRIX         mxForDDI;               // 0c4
    ULONG          flRealizedType;         // 100
    POINTL         ptlUnderline1;          // 104
    POINTL         ptlStrikeOut;           // 10c
    POINTL         ptlULThickness;         // 114
    POINTL         ptlSOThickness;         // 11c
    LONG           lCharInc;               // 124
    FIX            fxMaxAscent;            // 128
    FIX            fxMaxDescent;           // 12c
    FIX            fxMaxExtent;            // 130
    POINTFX        ptfxMaxAscent;          // 134
    POINTFX        ptfxMaxDescent;         // 13c
    ULONG          cxMax;                  // 144
    LONG           lMaxAscent;             // 148
    LONG           lMaxHeight;             // 14c
    ULONG          cyMax;                  // 150
    ULONG          cjGlyphMax;             // 154
    FD_XFORM       fdxQuantized;           // 158
    LONG           lNonLinearExtLeading;   // 168
    LONG           lNonLinearIntLeading;   // 16c
    LONG           lNonLinearMaxCharWidth; // 170
    LONG           lNonLinearAvgCharWidth; // 174
    ULONG          ulOrientation;          // 178
    POINTEF        pteUnitBase;            // 17c
    FLOATOBJ       efWtoDBase;             // 18c
    FLOATOBJ       efDtoWBase;             // 194
    LONG           lAscent;                // 19c
    POINTEF        pteUnitAscent;          // 1A0
    FLOATOBJ       efWtoDAscent;           // 1B0
    FLOATOBJ       efDtoWAscent;           // 1B8
    LONG           lEscapement;            // 1c0
    FLOATOBJ       efWtoDEsc;              // 1c4
    FLOATOBJ       efDtoWEsc;              // 1cc
    ULONG          unknown[4];             // 1d4
    FLOATOBJ       efEscToBase;            // 1e4
    FLOATOBJ       efEscToAscent;          // 1ec
    HGLYPH *       phg;                    // 1f4
    HGLYPH         hgBreak;                // 1f8
    FIX            fxBreak;                // 1fc
    FD_GLYPHSET *  pfdg;                   // 200
    void *         wcgp;                   // 204
    FLONG          flInfo;                 // 208
    ULONG          cSelected;              // 20c
    RFONTLINK      rflPDEV;                // 210
    RFONTLINK      rflPFF;                 // 218
    void *         hsemCache;              // 220
    CACHE          cache;                  // 224
    POINTL         ptlSim;                 // 27c
    BOOL           bNeededPaths;           // 284
    FLOATOBJ       efDtoWBase_31;          // 288
    FLOATOBJ       efDtoWAscent_31;        // 290
    TEXTMETRICW *  ptmw;                   // 298
    LONG           lMaxNegA;               // 29c
    LONG           lMaxNegC;               // 2a0
    LONG           lMinWidthD;             // 2a4
    BOOL           bIsSystemFont;          // 2a8
    FLONG          flEUDCState;            // 2ac
    PRFONT         prfntSystemTT;          // 2b0
    PRFONT         prfntSysEUDC;           // 2b4
    PRFONT         prfntDefEUDC;           // 2b8
    PRFONT         paprfntFaceName;        // 2bc
    PRFONT         aprfntQuickBuff[8];     // 2c0
    BOOL           bFilledEudcArray;       // 2e0
    ULONG          ulTimeStamp;            // 2e4
    ULONG          uiNumLinks;             // 2e8
    BOOL           bVertical;              // 2ec
    HSEMAPHORE     hsemCache1;             // 2f0
    ULONG          unknown2                // 2f4
}
#endif


/* PUBLIC FUNCTIONS **********************************************************/

ULONG
APIENTRY
FONTOBJ_cGetAllGlyphHandles(
    IN FONTOBJ *pfo,
    IN HGLYPH *phg)
{
    ASSERT(FALSE);
    return 0;
}

ULONG
APIENTRY
FONTOBJ_cGetGlyphs(
    IN FONTOBJ *pfo,
    IN ULONG iMode,
    IN ULONG cGlyph,
    IN HGLYPH *phg,
    IN PVOID *ppvGlyph)
{
    ASSERT(FALSE);
    return 0;
}

IFIMETRICS*
APIENTRY
FONTOBJ_pifi(
    IN FONTOBJ *pfo)
{
    PRFONT prfnt = CONTAINING_RECORD(pfo, RFONT, fobj);

    return prfnt->ppfe->pifi;
}

PVOID
APIENTRY
FONTOBJ_pvTrueTypeFontFile(
    IN FONTOBJ  *FontObj,
    IN ULONG    *FileSize)
{
    ASSERT(FALSE);
    return NULL;
}

#undef XFORMOBJ
XFORMOBJ*
APIENTRY
FONTOBJ_pxoGetXform(
    IN FONTOBJ *pfo)
{
    PRFONT prfnt = (PRFONT)pfo;
    return (XFORMOBJ*)&prfnt->xoForDDI;
}


VOID
APIENTRY
FONTOBJ_vGetInfo(
    IN  FONTOBJ *pfo,
    IN  ULONG cjSize,
    OUT PFONTINFO pfi)
{
    PRFONT prfnt = CONTAINING_RECORD(pfo, RFONT, fobj);
    FLONG flInfo = prfnt->ppfe->pifi->flInfo;

    __debugbreak();

    pfi->cjThis = sizeof(FONTINFO);
    pfi->flCaps = 0;
    if (pfo->flFontType & FO_TYPE_DEVICE) pfi->flCaps |= FO_DEVICE_FONT;
    if (flInfo & FM_INFO_RETURNS_OUTLINES) pfi->flCaps |= FO_OUTLINE_CAPABLE;
    pfi->cGlyphsSupported = prfnt->pfdg->cGlyphsSupported;

    /* Reset all sizes to 0 */
    pfi->cjMaxGlyph1 = 0;
    pfi->cjMaxGlyph4 = 0;
    pfi->cjMaxGlyph8 = 0;
    pfi->cjMaxGlyph32 = 0;

    /* Set the appropriate field */
    switch (prfnt->cBitsPerPel)
    {
        case 1: pfi->cjMaxGlyph1 = prfnt->cache.cjGlyphMax; break;
        case 4: pfi->cjMaxGlyph4 = prfnt->cache.cjGlyphMax; break;
        case 8: pfi->cjMaxGlyph8 = prfnt->cache.cjGlyphMax; break;
        case 32: pfi->cjMaxGlyph32 = prfnt->cache.cjGlyphMax; break;
        default: ASSERT(FALSE); break;
    }

}

FD_GLYPHSET *
APIENTRY
FONTOBJ_pfdg(
   IN FONTOBJ *pfo)
{
   ASSERT(FALSE);
   return NULL;
}

PBYTE
APIENTRY
FONTOBJ_pjOpenTypeTablePointer(
   IN FONTOBJ *pfo,
   IN ULONG ulTag,
   OUT ULONG *pcjTable)
{
   ASSERT(FALSE);
   return NULL;
}

PFD_GLYPHATTR
APIENTRY
FONTOBJ_pQueryGlyphAttrs(
   IN FONTOBJ *pfo,
   IN ULONG iMode)
{
   ASSERT(FALSE);
   return NULL;
}

LPWSTR
APIENTRY
FONTOBJ_pwszFontFilePaths(
   IN FONTOBJ *pfo,
   OUT ULONG *pcwc)
{
   ASSERT(FALSE);
   return NULL;
}
