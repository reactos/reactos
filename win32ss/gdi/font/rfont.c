/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h">

#define NDEBUG
#include <debug.h>





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
