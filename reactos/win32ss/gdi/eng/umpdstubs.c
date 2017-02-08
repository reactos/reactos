#include <win32k.h>
#undef XFORMOBJ

#define UNIMPLEMENTED DbgPrint("(%s:%i) WIN32K: %s UNIMPLEMENTED\n", __FILE__, __LINE__, __FUNCTION__ )

__kernel_entry
BOOL
APIENTRY
NtGdiUMPDEngFreeUserMem(
    _In_ KERNEL_PVOID *ppv)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiSetPUMPDOBJ(
    _In_opt_ HUMPD humpd,
    _In_ BOOL bStoreID,
    _Inout_opt_ HUMPD *phumpd,
    _Out_opt_ BOOL *pbWOW64)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
HANDLE
APIENTRY
NtGdiBRUSHOBJ_hGetColorTransform(
    _In_ BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
PVOID
APIENTRY
NtGdiBRUSHOBJ_pvAllocRbrush(
    _In_ BRUSHOBJ *pbo,
    _In_ ULONG cj)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
PVOID
APIENTRY
NtGdiBRUSHOBJ_pvGetRbrush(
    _In_ BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
ULONG
APIENTRY
NtGdiBRUSHOBJ_ulGetBrushColor(
    _In_ BRUSHOBJ *pbo)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
BOOL
APIENTRY
NtGdiBRUSHOBJ_DeleteRbrush(
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ BRUSHOBJ *pboB)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
BOOL
APIENTRY
NtGdiCLIPOBJ_bEnum(
    _In_ CLIPOBJ *pco,
    _In_ ULONG cj,
    _Out_writes_bytes_(cj) ULONG *pul)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
ULONG
APIENTRY
NtGdiCLIPOBJ_cEnumStart(
    _In_ CLIPOBJ *pco,
    _In_ BOOL bAll,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cLimit)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
PATHOBJ*
APIENTRY
NtGdiCLIPOBJ_ppoGetPath(
    _In_ CLIPOBJ *pco)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngAssociateSurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngCheckAbort(
    _In_ SURFOBJ *pso)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
FD_GLYPHSET*
APIENTRY
NtGdiEngComputeGlyphSet(
    _In_ INT nCodePage,
    _In_ INT nFirstChar,
    _In_ INT cChars)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngCopyBits(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ POINTL *pptlSrc)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
HBITMAP
APIENTRY
NtGdiEngCreateBitmap(
    _In_ SIZEL sizl,
    _In_ LONG lWidth,
    _In_ ULONG iFormat,
    _In_ FLONG fl,
    _In_opt_ PVOID pvBits)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
CLIPOBJ*
APIENTRY
NtGdiEngCreateClip(
    VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
HBITMAP
APIENTRY
NtGdiEngCreateDeviceBitmap(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
HSURF
APIENTRY
NtGdiEngCreateDeviceSurface(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
HPALETTE
APIENTRY
NtGdiEngCreatePalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_ ULONG *pulColors,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiEngDeleteClip(
    _In_ CLIPOBJ*pco)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngDeletePalette(
    _In_ HPALETTE hPal)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiEngDeletePath(
    _In_ PATHOBJ *ppo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngDeleteSurface(
    _In_ HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngEraseSurface(
    _In_ SURFOBJ *pso,
    _In_ RECTL *prcl,
    _In_ ULONG iColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngGradientFill(
    _In_ SURFOBJ *psoDest,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_reads_(nVertex) TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    _In_ /* _In_reads_(nMesh) */ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ RECTL *prclExtents,
    _In_ POINTL *pptlDitherOrg,
    _In_ ULONG ulMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngLineTo(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_ RECTL *prclBounds,
    _In_ MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngMarkBandingSurface(
    _In_ HSURF hsurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngPlgBlt(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMsk,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ POINTFIX *pptfx,
    _In_ RECTL *prcl,
    _In_opt_ POINTL *pptl,
    _In_ ULONG iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngStretchBltROP(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ RECTL *prclTrg,
    _In_ RECTL *prclSrc,
    _In_ POINTL *pptlMask,
    _In_ ULONG iMode,
    _In_ BRUSHOBJ *pbo,
    _In_ ROP4 rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngStrokePath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX mix)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngStrokeAndFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,IN XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ *pboFill,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngTextOut(
    _In_ SURFOBJ *pso,
    _In_ STROBJ *pstro,
    _In_ FONTOBJ *pfo,
    _In_ CLIPOBJ *pco,
    _In_ RECTL *prclExtra,
    _In_ RECTL *prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ POINTL *pptlOrg,
    _In_ MIX mix)
{
     UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngTransparentBlt(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ RECTL *prclSrc,
    _In_ ULONG iTransColor,
    _In_ ULONG ulReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiFONTOBJ_vGetInfo(
    _In_ FONTOBJ *pfo,
    _In_ ULONG cjSize,
    _Out_writes_bytes_(cjSize) FONTINFO *pfi)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
XFORMOBJ*
APIENTRY
NtGdiFONTOBJ_pxoGetXform(
    _In_ FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
ULONG
APIENTRY
NtGdiFONTOBJ_cGetGlyphs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_ ULONG cGlyph,
    _In_ HGLYPH *phg,
    _At_((GLYPHDATA**)ppvGlyph, _Outptr_) PVOID *ppvGlyph)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
IFIMETRICS*
APIENTRY
NtGdiFONTOBJ_pifi(
    _In_ FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
FD_GLYPHSET*
APIENTRY
NtGdiFONTOBJ_pfdg(
    _In_ FONTOBJ *pfo)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
PFD_GLYPHATTR
APIENTRY
NtGdiFONTOBJ_pQueryGlyphAttrs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
PVOID
APIENTRY
NtGdiFONTOBJ_pvTrueTypeFontFile(
    _In_ FONTOBJ *pfo,
    _Out_ ULONG *pcjFile)
{
    UNIMPLEMENTED;
    return NULL;
}

__kernel_entry
ULONG
APIENTRY
NtGdiFONTOBJ_cGetAllGlyphHandles(
    _In_ FONTOBJ *pfo,
    _Out_opt_ _Post_count_(return) HGLYPH *phg)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
LONG
APIENTRY
NtGdiHT_Get8BPPMaskPalette(
    _Out_opt_ _Post_count_(return) LPPALETTEENTRY pPaletteEntry,
    _In_ BOOL Use8BPPMaskPal,
    _In_ BYTE CMYMask,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
LONG
APIENTRY
NtGdiHT_Get8BPPFormatPalette(
    _Out_opt_ _Post_count_(return) LPPALETTEENTRY pPaletteEntry,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiPATHOBJ_vGetBounds(
    _In_ PATHOBJ *ppo,
    _Out_ PRECTFX prectfx)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
BOOL
APIENTRY
NtGdiPATHOBJ_bEnum(
    _In_ PATHOBJ *ppo,
    _Out_ PATHDATA *ppd)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiPATHOBJ_vEnumStart(
    _In_ PATHOBJ *ppo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiPATHOBJ_vEnumStartClipLines(
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ SURFOBJ *pso,
    _In_ LINEATTRS *pla)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
BOOL
APIENTRY
NtGdiPATHOBJ_bEnumClipLines(
    _In_ PATHOBJ *ppo,
    _In_ ULONG cb,
    _Out_writes_bytes_(cb) CLIPLINE *pcl)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiSTROBJ_bEnum(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Outptr_result_buffer_(*pc) PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiSTROBJ_bEnumPositionsOnly(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Outptr_result_buffer_(*pc) PGLYPHPOS *ppgpos)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
BOOL
APIENTRY
NtGdiSTROBJ_bGetAdvanceWidths(
    _In_ STROBJ*pstro,
    _In_ ULONG iFirst,
    _In_ ULONG c,
    _Out_writes_(c) POINTQF*pptqD)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiSTROBJ_vEnumStart(
    _Inout_ STROBJ *pstro)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
DWORD
APIENTRY
NtGdiSTROBJ_dwGetCodePage(
    _In_ STROBJ *pstro)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
BOOL
APIENTRY
NtGdiXFORMOBJ_bApplyXform(
    _In_ XFORMOBJ *pxo,
    _In_ ULONG iMode,
    _In_ ULONG cPoints,
    _In_reads_(cPoints) PPOINTL pptIn,
    _Out_writes_(cPoints) PPOINTL pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
ULONG
APIENTRY
NtGdiXFORMOBJ_iGetXform(
    _In_ XFORMOBJ *pxo,
    _Out_opt_ XFORML *pxform)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
ULONG
APIENTRY
NtGdiXLATEOBJ_cGetPalette(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iPal,
    _In_ ULONG cPal,
    _Out_writes_(cPal) ULONG *pPal)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
ULONG
APIENTRY
NtGdiXLATEOBJ_iXlate(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iColor)
{
    UNIMPLEMENTED;
    return 0;
}

__kernel_entry
HANDLE
APIENTRY
NtGdiXLATEOBJ_hGetColorTransform(
    _In_ XLATEOBJ *pxlo)
{
    UNIMPLEMENTED;
    return 0;
}


//NtGdiEngAlphaBlend
//NtGdiEngUnlockSurface
//NtGdiEngLockSurface
//NtGdiEngBitBlt
//NtGdiEngStretchBlt

