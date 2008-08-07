/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdieng.c
 * PURPOSE:         Syscall wrappers around win32k exports
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiEngAssociateSurface(IN HSURF hsurf,
                         IN HDEV hdev,
                         IN FLONG flHooks)
{
    UNIMPLEMENTED;
    return FALSE;
}

HBITMAP
APIENTRY
NtGdiEngCreateBitmap(IN SIZEL sizl,
                     IN LONG lWidth,
                     IN ULONG iFormat,
                     IN FLONG fl,
                     IN OPTIONAL PVOID pvBits)
{
    UNIMPLEMENTED;
    return NULL;
}

HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(IN DHSURF dhsurf,
                            IN SIZEL sizl,
                            IN ULONG iFormatCompat)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(IN DHSURF dhsurf,
                           IN SIZEL sizl,
                           IN ULONG iFormatCompat)
{
    UNIMPLEMENTED;
    return NULL;
}

HPALETTE
APIENTRY
NtGdiEngCreatePalette(IN ULONG iMode,
                      IN ULONG cColors,
                      IN ULONG *pulColors,
                      IN FLONG flRed,
                      IN FLONG flGreen,
                      IN FLONG flBlue)
{
    UNIMPLEMENTED;
    return NULL;
}

FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(IN INT nCodePage,
                        IN INT nFirstChar,
                        IN INT cChars)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiEngCopyBits(IN SURFOBJ *psoDst,
                 IN SURFOBJ *psoSrc,
                 IN OPTIONAL CLIPOBJ *pco,
                 IN XLATEOBJ *pxlo,
                 IN RECTL *prclDst,
                 IN POINTL *pptlSrc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngDeletePalette(IN HPALETTE hPal)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngDeleteSurface(IN HSURF hSurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngEraseSurface(IN SURFOBJ *pso,
                     IN RECTL *prcl,
                     IN ULONG iColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
NtGdiEngUnlockSurface(IN SURFOBJ *SurfObj)
{
    UNIMPLEMENTED;
}

SURFOBJ*
APIENTRY
NtGdiEngLockSurface(IN HSURF hSurf)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiEngBitBlt(IN SURFOBJ *psoDst,
               IN SURFOBJ *psoSrc,
               IN SURFOBJ *psoMask,
               IN CLIPOBJ *pco,
               IN XLATEOBJ *pxlo,
               IN RECTL *prclDst,
               IN POINTL *pptlSrc,
               IN POINTL *pptlMask,
               IN BRUSHOBJ *pbo,
               IN POINTL *pptlBrush,
               IN ROP4 rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStretchBlt(IN SURFOBJ *psoDest,
                   IN SURFOBJ *psoSrc,
                   IN SURFOBJ *psoMask,
                   IN CLIPOBJ *pco,
                   IN XLATEOBJ *pxlo,
                   IN COLORADJUSTMENT *pca,
                   IN POINTL *pptlHTOrg,
                   IN RECTL *prclDest,
                   IN RECTL *prclSrc,
                   IN POINTL *pptlMask,
                   IN ULONG iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngPlgBlt(IN SURFOBJ *psoTrg,
               IN SURFOBJ *psoSrc,
               IN OPTIONAL SURFOBJ *psoMsk,
               IN CLIPOBJ *pco,
               IN XLATEOBJ *pxlo,
               IN COLORADJUSTMENT *pca,
               IN POINTL *pptlBrushOrg,
               IN POINTFIX *pptfxDest,
               IN RECTL *prclSrc,
               IN OPTIONAL POINTL *pptlMask,
               IN ULONG iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngMarkBandingSurface(IN HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStrokePath(IN SURFOBJ *pso,
                   IN PATHOBJ *ppo,
                   IN CLIPOBJ *pco,
                   IN XFORMOBJ *pxo,
                   IN BRUSHOBJ *pbo,
                   IN POINTL *pptlBrushOrg,
                   IN LINEATTRS *plineattrs,
                   IN MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngFillPath(IN SURFOBJ *pso,
                 IN PATHOBJ *ppo,
                 IN CLIPOBJ *pco,
                 IN BRUSHOBJ *pbo,
                 IN POINTL *pptlBrushOrg,
                 IN MIX mix,
                 IN FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStrokeAndFillPath(IN SURFOBJ *pso,
                          IN PATHOBJ *ppo,
                          IN CLIPOBJ *pco,IN XFORMOBJ *pxo,
                          IN BRUSHOBJ *pboStroke,
                          IN LINEATTRS *plineattrs,
                          IN BRUSHOBJ *pboFill,
                          IN POINTL *pptlBrushOrg,
                          IN MIX mix,
                          IN FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngPaint(IN SURFOBJ *pso,
              IN CLIPOBJ *pco,
              IN BRUSHOBJ *pbo,
              IN POINTL *pptlBrushOrg,
              IN MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngLineTo(IN SURFOBJ *pso,
               IN CLIPOBJ *pco,
               IN BRUSHOBJ *pbo,
               IN LONG x1,
               IN LONG y1,
               IN LONG x2,
               IN LONG y2,
               IN RECTL *prclBounds,
               IN MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngAlphaBlend(IN SURFOBJ *psoDest,
                   IN SURFOBJ *psoSrc,
                   IN CLIPOBJ *pco,
                   IN XLATEOBJ *pxlo,
                   IN RECTL *prclDest,
                   IN RECTL *prclSrc,
                   IN BLENDOBJ *pBlendObj)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngGradientFill(IN SURFOBJ *psoDest,
                     IN CLIPOBJ *pco,
                     IN XLATEOBJ *pxlo,
                     IN TRIVERTEX *pVertex,
                     IN ULONG nVertex,
                     IN PVOID pMesh,
                     IN ULONG nMesh,
                     IN RECTL *prclExtents,
                     IN POINTL *pptlDitherOrg,
                     IN ULONG ulMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngTransparentBlt(IN SURFOBJ *psoDst,
                       IN SURFOBJ *psoSrc,
                       IN CLIPOBJ *pco,
                       IN XLATEOBJ *pxlo,
                       IN RECTL *prclDst,
                       IN RECTL *prclSrc,
                       IN ULONG iTransColor,
                       IN ULONG ulReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngTextOut(IN SURFOBJ *pso,
                IN STROBJ *pstro,
                IN FONTOBJ *pfo,
                IN CLIPOBJ *pco,
                IN RECTL *prclExtra,
                IN RECTL *prclOpaque,
                IN BRUSHOBJ *pboFore,
                IN BRUSHOBJ *pboOpaque,
                IN POINTL *pptlOrg,
                IN MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStretchBltROP(IN SURFOBJ *psoTrg,
                      IN SURFOBJ *psoSrc,
                      IN SURFOBJ *psoMask,
                      IN CLIPOBJ *pco,
                      IN XLATEOBJ *pxlo,
                      IN COLORADJUSTMENT *pca,
                      IN POINTL *pptlBrushOrg,
                      IN RECTL *prclTrg,
                      IN RECTL *prclSrc,
                      IN POINTL *pptlMask,
                      IN ULONG iMode,
                      IN BRUSHOBJ *pbo,
                      IN ROP4 rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(IN XLATEOBJ *pxlo,
                          IN ULONG iPal,
                          IN ULONG cPal,
                          OUT ULONG *pPal)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(IN XLATEOBJ *pxlo,
                     IN ULONG iColor)
{
    UNIMPLEMENTED;
    return 0;
}

HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(IN XLATEOBJ *pxlo)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(IN CLIPOBJ *pco,
                   IN ULONG cj,
                   OUT ULONG *pul)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(IN CLIPOBJ *pco,
                        IN BOOL bAll,
                        IN ULONG iType,
                        IN ULONG iDirection,
                        IN ULONG cLimit)
{
    UNIMPLEMENTED;
    return 0;
}

PATHOBJ*
APIENTRY
NtGdiCLIPOBJ_ppoGetPath(IN CLIPOBJ *pco)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
NtGdiEngDeletePath(IN PATHOBJ *ppo)
{
    UNIMPLEMENTED;
}

CLIPOBJ*
APIENTRY
NtGdiEngCreateClip(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
NtGdiEngDeleteClip(IN CLIPOBJ*pco)
{
    UNIMPLEMENTED;
}

ULONG
APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(IN BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return 0;
}

PVOID
APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ *pbo,
                            IN ULONG cj)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(IN BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return NULL;
}

HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(IN BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(IN XFORMOBJ *pxo,
                          IN ULONG iMode,
                          IN ULONG cPoints,
                          IN  PVOID pvIn,
                          OUT PVOID pvOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(IN XFORMOBJ *pxo,
                        OUT OPTIONAL XFORML *pxform)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
NtGdiFONTOBJ_vGetInfo(IN FONTOBJ *pfo,
                      IN ULONG cjSize,
                      OUT FONTINFO *pfi)
{
    UNIMPLEMENTED;
}

XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(IN FONTOBJ *pfo,
                        IN ULONG iMode,
                        IN ULONG cGlyph,
                        IN HGLYPH *phg,
                        OUT PVOID *ppvGlyph)
{
    UNIMPLEMENTED;
    return 0;
}

IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(IN FONTOBJ *pfo,
                              IN ULONG iMode)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
NtGdiFONTOBJ_pvTrueTypeFontFile(IN FONTOBJ *pfo,
                                OUT ULONG *pcjFile)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(IN FONTOBJ *pfo,
                                 OUT OPTIONAL HGLYPH *phg)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSTROBJ_bEnum(IN STROBJ *pstro,
                  OUT ULONG *pc,
                  OUT PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(IN STROBJ *pstro,
                               OUT ULONG *pc,
                               OUT PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(IN STROBJ *pstro,
                              IN ULONG iFirst,
                              IN ULONG c,
                              OUT POINTQF *pptqD)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
NtGdiSTROBJ_vEnumStart(IN STROBJ *pstro)
{
    UNIMPLEMENTED;
}

DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(IN STROBJ *pstro)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
NtGdiPATHOBJ_vGetBounds(IN PATHOBJ *ppo,
                        OUT PRECTFX prectfx)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(IN PATHOBJ *ppo,
                   OUT PATHDATA *ppd)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
NtGdiPATHOBJ_vEnumStart(IN PATHOBJ *ppo)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(IN PATHOBJ *ppo,
                                 IN CLIPOBJ *pco,
                                 IN SURFOBJ *pso,
                                 IN LINEATTRS *pla)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(IN PATHOBJ *ppo,
                            IN ULONG cb,
                            OUT CLIPLINE *pcl)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngCheckAbort(IN SURFOBJ *pso)
{
    UNIMPLEMENTED;
    return FALSE;
}

LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
                             IN USHORT RedGamma,
                             IN USHORT GreenGamma,
                             IN USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return 0;
}

LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
                           IN BOOL Use8BPPMaskPal,
                           IN BYTE CMYMask,
                           IN USHORT RedGamma,
                           IN USHORT GreenGamma,
                           IN USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return 0;
}
