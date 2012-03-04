/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include <include/font.h>
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
WIN32KAPI
XFORMOBJ*
APIENTRY
FONTOBJ_pxoGetXform(
    IN FONTOBJ *pfo)
{
    ASSERT(FALSE);
    return NULL;
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
