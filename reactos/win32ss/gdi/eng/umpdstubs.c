#include <win32k.h>
#undef XFORMOBJ

#define UNIMPLEMENTED DbgPrint("(%s:%i) WIN32K: %s UNIMPLEMENTED\n", __FILE__, __LINE__, __FUNCTION__ )

BOOL
APIENTRY
NtGdiUMPDEngFreeUserMem(
    IN KERNEL_PVOID *ppv)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetPUMPDOBJ(
    IN HUMPD humpd,
    IN BOOL bStoreID,
    OUT HUMPD *phumpd,
    OUT BOOL *pbWOW64)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(
   IN BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(
    IN BRUSHOBJ *pbo,
    IN ULONG cj)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(
    IN BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(
    BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiBRUSHOBJ_DeleteRbrush(
    IN BRUSHOBJ *pbo,
    IN BRUSHOBJ *pboB)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(
    IN CLIPOBJ *pco,
    IN ULONG cj,
    OUT ULONG *pv)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(
    IN CLIPOBJ *pco,
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
NtGdiCLIPOBJ_ppoGetPath(
    CLIPOBJ *pco)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiEngAssociateSurface(
    IN HSURF hsurf,
    IN HDEV hdev,
    IN ULONG flHooks)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngCheckAbort(
    IN SURFOBJ *pso)
{
    UNIMPLEMENTED;
    return FALSE;
}

FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(
    INT nCodePage,
    INT nFirstChar,
    INT cChars)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiEngCopyBits(
    SURFOBJ *psoDest,
    SURFOBJ *psoSrc,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclDest,
    POINTL *pptlSrc)
{
    UNIMPLEMENTED;
    return FALSE;
}

HBITMAP
APIENTRY
NtGdiEngCreateBitmap(
    IN SIZEL sizl,
    IN LONG lWidth,
    IN ULONG iFormat,
    IN ULONG fl,
    IN PVOID pvBits)
{
    UNIMPLEMENTED;
    return NULL;
}

CLIPOBJ*
APIENTRY
NtGdiEngCreateClip(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat)
{
    UNIMPLEMENTED;
    return NULL;
}

HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormatCompat)
{
    UNIMPLEMENTED;
    return NULL;
}

HPALETTE
APIENTRY
NtGdiEngCreatePalette(
    IN ULONG iMode,
    IN ULONG cColors,
    IN ULONG *pulColors,
    IN ULONG flRed,
    IN ULONG flGreen,
    IN ULONG flBlue)
{
    UNIMPLEMENTED;
    return NULL;
}

NTSTATUS
APIENTRY
NtGdiEngDeleteClip(
    CLIPOBJ *pco)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
APIENTRY
NtGdiEngDeletePalette(
    IN HPALETTE hpal)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiEngDeletePath(
    IN PATHOBJ *ppo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
APIENTRY
NtGdiEngDeleteSurface(
    IN HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngEraseSurface(
    SURFOBJ *pso,
    RECTL *prcl,
    ULONG iColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngFillPath(
    SURFOBJ *pso,
    PATHOBJ *ppo,
    CLIPOBJ *pco,
    BRUSHOBJ *pbo,
    POINTL *pptlBrushOrg,
    MIX mix,
    FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngGradientFill(
    SURFOBJ *psoDest,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    TRIVERTEX *pVertex,
    ULONG nVertex,
    PVOID pMesh,
    ULONG nMesh,
    RECTL *prclExtents,
    POINTL *pptlDitherOrg,
    ULONG ulMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngLineTo(
    IN SURFOBJ *pso,
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
NtGdiEngMarkBandingSurface(
    HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngPaint(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX  mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngPlgBlt(
    SURFOBJ *psoTrg,
    SURFOBJ *psoSrc,
    SURFOBJ *psoMsk,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    COLORADJUSTMENT *pca,
    POINTL *pptlBrushOrg,
    POINTFIX *pptfx,
    RECTL *prcl,
    POINTL *pptl,
    ULONG iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStretchBltROP(
    SURFOBJ *psoDest,
    SURFOBJ *psoSrc,
    SURFOBJ *psoMask,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    COLORADJUSTMENT *pca,
    POINTL *pptlHTOrg,
    RECTL *prclDest,
    RECTL *prclSrc,
    POINTL *pptlMask,
    ULONG iMode,
    BRUSHOBJ *pbo,
    DWORD rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStrokePath(
    SURFOBJ *pso,
    PATHOBJ *ppo,
    CLIPOBJ *pco,
    XFORMOBJ *pxo,
    BRUSHOBJ *pbo,
    POINTL *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngStrokeAndFillPath(
    SURFOBJ *pso,
    PATHOBJ *ppo,
    CLIPOBJ *pco,
    XFORMOBJ *pxo,
    BRUSHOBJ *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ *pboFill,
    POINTL *pptlBrushOrg,
    MIX mixFill,
    FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngTextOut(
    SURFOBJ *pso,
    STROBJ *pstro,
    FONTOBJ *pfo,
    CLIPOBJ *pco,
    RECTL *prclExtra,
    RECTL *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL *pptlOrg,
    MIX mix)
{
     UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEngTransparentBlt(
    IN SURFOBJ *psoDst,
    IN SURFOBJ *psoSrc,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN PRECTL prclDst,
    IN PRECTL prclSrc,
    IN ULONG iTransColor,
    IN ULONG ulReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiFONTOBJ_vGetInfo(
    IN FONTOBJ *pfo,
    IN ULONG cjSize,
    OUT FONTINFO *pfi)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(
    IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(
    IN FONTOBJ *pfo,
    IN ULONG    iMode,
    IN ULONG    cGlyph,
    IN HGLYPH  *phg,
    IN PVOID   *ppvGlyph)
{
    UNIMPLEMENTED;
    return 0;
}

IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(
    IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(
    IN FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(
    IN FONTOBJ *pfo,
    IN ULONG iMode)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
NtGdiFONTOBJ_pvTrueTypeFontFile(
    IN FONTOBJ *pfo,
    OUT ULONG *pcjFile)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(
    IN FONTOBJ *pfo,
    IN HGLYPH  *phg)
{
    UNIMPLEMENTED;
    return 0;
}

LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN BOOL Use8BPPMaskPal,
    IN BYTE CMYMask,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return FALSE;
}

LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(
    OUT OPTIONAL LPPALETTEENTRY pPaletteEntry,
    IN USHORT RedGamma,
    IN USHORT GreenGamma,
    IN USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiPATHOBJ_vGetBounds(
    IN PATHOBJ *ppo,
    OUT PRECTFX prectfx)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(
    IN PATHOBJ *ppo,
    OUT PATHDATA *ppd)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiPATHOBJ_vEnumStart(
    IN PATHOBJ *ppo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(
    IN PATHOBJ *ppo,
    IN CLIPOBJ *pco,
    IN SURFOBJ *pso,
    IN LINEATTRS *pla)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(
    IN PATHOBJ *ppo,
    IN ULONG cb,
    OUT CLIPLINE *pcl)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSTROBJ_bEnum(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(
    IN STROBJ *pstro,
    OUT ULONG *pc,
    OUT PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(
    IN STROBJ *pstro,
    IN ULONG iFirst,
    IN ULONG c,
    OUT POINTQF *pptqD)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiSTROBJ_vEnumStart(
    IN STROBJ *pstro)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(
    IN STROBJ *pstro)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(
    IN XFORMOBJ *pxo,
    IN ULONG iMode,
    IN ULONG cPoints,
    _In_reads_(cPoints) PPOINTL pptIn,
    _Out_writes_(cPoints) PPOINTL pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(
    IN XFORMOBJ *pxo,
    OUT OPTIONAL XFORML *pxform)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    IN XLATEOBJ *pxlo,
    IN ULONG iPal,
    IN ULONG cPal,
    OUT ULONG *pPal)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    IN XLATEOBJ *pxlo,
    IN ULONG iColor)
{
    UNIMPLEMENTED;
    return 0;
}

HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(
    IN XLATEOBJ *pxlo)
{
    UNIMPLEMENTED;
    return 0;
}


//NtGdiEngAlphaBlend
//NtGdiEngUnlockSurface
//NtGdiEngLockSurface
//NtGdiEngBitBlt
//NtGdiEngStretchBlt

