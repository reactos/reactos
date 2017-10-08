/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         GNU GPL, see COPYING in the top level directory
 * FILE:            win32ss/gdi/eng/drvdbg.c
 * PURPOSE:         Debug hooks for display driver callbacks
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(EngDev);

PPDEVOBJ
NTAPI
DbgLookupDHPDEV(DHPDEV dhpdev);

VOID
DbgDrvReserved(void)
{
    ASSERT(FALSE);
}

DHPDEV
APIENTRY
DbgDrvEnablePDEV(
    _In_ DEVMODEW *pdm,
    _In_ LPWSTR pwszLogAddress,
    ULONG cPat,
    _In_opt_ HSURF *phsurfPatterns,
    ULONG cjCaps,
    _Out_ ULONG *pdevcaps,
    ULONG cjDevInfo,
    _Out_ DEVINFO *pdi,
    HDEV hdev,
    _In_ LPWSTR pwszDeviceName,
    HANDLE hDriver)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hdev;

    ASSERT(pdm);
    ASSERT(hdev);

    return ppdev->pldev->pfn.EnablePDEV(pdm,
                                        pwszLogAddress,
                                        cPat,
                                        phsurfPatterns,
                                        cjCaps,
                                        pdevcaps,
                                        cjDevInfo,
                                        pdi,
                                        hdev,
                                        pwszDeviceName,
                                        hDriver);
}

VOID
APIENTRY
DbgDrvCompletePDEV(
    DHPDEV dhpdev,
    HDEV hdev)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hdev;

    ASSERT(ppdev);

    ppdev->pldev->pfn.CompletePDEV(dhpdev, hdev);
}

VOID
APIENTRY
DbgDrvDisablePDEV(
    DHPDEV dhpdev)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    ppdev->pldev->pfn.DisablePDEV(dhpdev);
}

HSURF
APIENTRY
DbgDrvEnableSurface(
    DHPDEV dhpdev)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.EnableSurface(dhpdev);
}

VOID
APIENTRY
DbgDrvDisableSurface(
    DHPDEV dhpdev)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    ppdev->pldev->pfn.DisableSurface(dhpdev);
}

BOOL
APIENTRY
DbgDrvAssertMode(
    _In_ DHPDEV dhpdev,
    _In_ BOOL bEnable)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.AssertMode(dhpdev, bEnable);
}

BOOL
APIENTRY
DbgDrvOffset(
    SURFOBJ* pso,
    LONG x1,
    LONG x2,
    FLONG fl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;

    ASSERT(FALSE);
    return 0;
}

ULONG
APIENTRY
DbgDrvResetPDEV(
    DHPDEV dhpdev,
    PVOID Reserved)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.ResetDevice(dhpdev, Reserved);
}

VOID
APIENTRY
DbgDrvDisableDriver(void)
{
    ASSERT(FALSE);
}

HBITMAP
APIENTRY
DbgDrvCreateDeviceBitmap(
    DHPDEV dhpdev,
    SIZEL sizl,
    ULONG iFormat)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.CreateDeviceBitmap(dhpdev, sizl, iFormat);
}

VOID
APIENTRY
DbgDrvDeleteDeviceBitmap(
    DHSURF dhsurf)
{
    ASSERT(FALSE);
}

BOOL
APIENTRY
DbgDrvRealizeBrush(
    _In_      BRUSHOBJ *pbo,
    _Inout_   SURFOBJ *psoTarget,
    _In_      SURFOBJ *psoPattern,
    _In_opt_  SURFOBJ *psoMask,
    _In_      XLATEOBJ *pxlo,
    _In_      ULONG iHatch)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)psoTarget->hdev;
    ASSERT(FALSE);
    return 0;
}

ULONG
APIENTRY
DbgDrvDitherColor(
    _In_     DHPDEV dhpdev,
    _In_     ULONG iMode,
    _In_     ULONG rgb,
    _Inout_  ULONG *pul)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.DitherColor(dhpdev, iMode, rgb, pul);
}

BOOL
APIENTRY
DbgDrvStrokePath(
    _Inout_   SURFOBJ *pso,
    _In_      PATHOBJ *ppo,
    _In_      CLIPOBJ *pco,
    _In_opt_  XFORMOBJ *pxo,
    _In_      BRUSHOBJ *pbo,
    _In_      POINTL *pptlBrushOrg,
    _In_      LINEATTRS *plineattrs,
    _In_      MIX mix)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    ASSERT(FALSE);
    return 0;
}

BOOL
APIENTRY
DbgDrvFillPath(
    _Inout_  SURFOBJ *pso,
    _In_     PATHOBJ *ppo,
    _In_     CLIPOBJ *pco,
    _In_     BRUSHOBJ *pbo,
    _In_     POINTL *pptlBrushOrg,
    _In_     MIX mix,
    _In_     FLONG flOptions)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    ASSERT(FALSE);
    return 0;
}

BOOL
APIENTRY
DbgDrvStrokeAndFillPath(
    _Inout_   SURFOBJ *pso,
    _Inout_   PATHOBJ *ppo,
    _In_      CLIPOBJ *pco,
    _In_opt_  XFORMOBJ *pxo,
    _In_      BRUSHOBJ *pboStroke,
    _In_      LINEATTRS *plineattrs,
    _In_      BRUSHOBJ *pboFill,
    _In_      POINTL *pptlBrushOrg,
    _In_      MIX mixFill,
    _In_      FLONG flOptions)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    ASSERT(FALSE);
    return 0;
}

BOOL
APIENTRY
DbgDrvPaint(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX mix)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvBitBlt(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _When_(psoSrc, _In_) POINTL *pptlSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _When_(pbo, _In_) POINTL *pptlBrush,
    _In_ ROP4 rop4)
{
    PSURFACE psurfTrg = CONTAINING_RECORD(psoTrg, SURFACE, SurfObj);
    PSURFACE psurfSrc = CONTAINING_RECORD(psoSrc, SURFACE, SurfObj);
    PPDEVOBJ ppdev;

    /* Get the right BitBlt function */
    if (psurfTrg->flags & HOOK_BITBLT)
    {
        ppdev = (PPDEVOBJ)psoTrg->hdev;
    }
    else
    {
        ASSERT(ROP4_USES_SOURCE(rop4));
        ASSERT(psurfSrc->flags & HOOK_BITBLT);
        ppdev = (PPDEVOBJ)psoSrc->hdev;
    }

    /* Sanity checks */
    ASSERT(IS_VALID_ROP4(rop4));
    ASSERT(psoTrg);
    ASSERT(psoTrg->iBitmapFormat >= BMF_1BPP);
    ASSERT(psoTrg->iBitmapFormat <= BMF_32BPP);
    ASSERT(prclTrg);
    ASSERT(prclTrg->left >= 0);
    ASSERT(prclTrg->top >= 0);
    ASSERT(prclTrg->right <= psoTrg->sizlBitmap.cx);
    ASSERT(prclTrg->bottom <= psoTrg->sizlBitmap.cy);
    ASSERT(RECTL_bIsWellOrdered(prclTrg));
    ASSERT(pco);
    ASSERT(pco->iDComplexity != DC_RECT);

    if (ROP4_USES_SOURCE(rop4))
    {
        ASSERT(psoSrc);
        ASSERT(psoSrc->iBitmapFormat >= BMF_1BPP);
        ASSERT(psoSrc->iBitmapFormat <= BMF_8RLE);
        ASSERT(pptlSrc);
        ASSERT(pptlSrc->x >= 0);
        ASSERT(pptlSrc->y >= 0);
        ASSERT(pptlSrc->x <= psoTrg->sizlBitmap.cx);
        ASSERT(pptlSrc->y <= psoTrg->sizlBitmap.cy);
    }

    if (ROP4_USES_MASK(rop4))
    {
        ASSERT(psoMask);
        ASSERT(psoMask->iBitmapFormat == BMF_1BPP);
        ASSERT(pptlMask);
        ASSERT(pptlMask->x >= 0);
        ASSERT(pptlMask->y >= 0);
        ASSERT(pptlMask->x <= psoMask->sizlBitmap.cx);
        ASSERT(pptlMask->y <= psoMask->sizlBitmap.cy);

    }

    if (ROP4_USES_PATTERN(rop4))
    {
        ASSERT(pbo);
        ASSERT(pptlBrush);
    }


    return ppdev->pldev->pfn.BitBlt(psoTrg,
                                    psoSrc,
                                    psoMask,
                                    pco,
                                    pxlo,
                                    prclTrg,
                                    pptlSrc,
                                    pptlMask,
                                    pbo,
                                    pptlBrush,
                                    rop4);
}

BOOL
APIENTRY
DbgDrvCopyBits(
    SURFOBJ *psoTrg,
    SURFOBJ *psoSrc,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclTrg,
    POINTL *pptlSrc)
{
    PSURFACE psurfTrg = CONTAINING_RECORD(psoTrg, SURFACE, SurfObj);
    PSURFACE psurfSrc = CONTAINING_RECORD(psoSrc, SURFACE, SurfObj);
    PPDEVOBJ ppdev;

    /* Get the right BitBlt function */
    if (psurfTrg->flags & HOOK_COPYBITS)
    {
        ppdev = (PPDEVOBJ)psoTrg->hdev;
    }
    else
    {
        ASSERT(psurfSrc->flags & HOOK_COPYBITS);
        ppdev = (PPDEVOBJ)psoSrc->hdev;
    }

    return ppdev->pldev->pfn.CopyBits(psoTrg,
                                      psoSrc,
                                      pco,
                                      pxlo,
                                      prclTrg,
                                      pptlSrc);

}

BOOL
APIENTRY
DbgDrvStretchBlt(
    _Inout_   SURFOBJ *psoTrg,
    _Inout_   SURFOBJ *psoSrc,
    _In_opt_  SURFOBJ *psoMask,
    _In_      CLIPOBJ *pco,
    _In_opt_  XLATEOBJ *pxlo,
    _In_opt_  COLORADJUSTMENT *pca,
    _In_      POINTL *pptlHTOrg,
    _In_      RECTL *prclDest,
    _In_      RECTL *prclSrc,
    _In_opt_  POINTL *pptlMask,
    _In_      ULONG iMode)
{
    PSURFACE psurfTrg = CONTAINING_RECORD(psoTrg, SURFACE, SurfObj);
    PSURFACE psurfSrc = CONTAINING_RECORD(psoSrc, SURFACE, SurfObj);

    return 0;
}

BOOL
APIENTRY
DbgDrvSetPalette(
    DHPDEV dhpdev,
    PALOBJ *ppalo,
    FLONG fl,
    ULONG iStart,
    ULONG cColors)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.SetPalette(dhpdev, ppalo, fl, iStart, cColors);
}

BOOL
APIENTRY
DbgDrvTextOut(
    SURFOBJ *pso,
    STROBJ *pstro,
    FONTOBJ *pfo,
    CLIPOBJ *pco,
    RECTL *prclExtra ,
    RECTL *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL *pptlOrg,
    MIX mix)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

ULONG
APIENTRY
DbgDrvEscape(
    _In_   SURFOBJ *pso,
    _In_   ULONG iEsc,
    _In_   ULONG cjIn,
    _In_   PVOID pvIn,
    _In_   ULONG cjOut,
    _Out_  PVOID pvOut)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

ULONG
APIENTRY
DbgDrvDrawEscape(
    _In_  SURFOBJ *pso,
    _In_  ULONG iEsc,
    _In_  CLIPOBJ *pco,
    _In_  RECTL *prcl,
    _In_  ULONG cjIn,
    _In_  PVOID pvIn)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

PIFIMETRICS
APIENTRY
DbgDrvQueryFont(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG_PTR *pid)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.QueryFont(dhpdev, iFile, iFace, pid);
}

PVOID
APIENTRY
DbgDrvQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid)
{
    return 0;
}

LONG
APIENTRY
DbgDrvQueryFontData(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    HGLYPH hg,
    GLYPHDATA *pgd,
    _Out_  PVOID pv,
    ULONG cjSize)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.QueryFontData(dhpdev, pfo, iMode, hg, pgd, pv, cjSize);
}

ULONG
APIENTRY
DbgDrvSetPointerShape(
    _In_  SURFOBJ *pso,
    _In_  SURFOBJ *psoMask,
    _In_  SURFOBJ *psoColor,
    _In_  XLATEOBJ *pxlo,
    _In_  LONG xHot,
    _In_  LONG yHot,
    _In_  LONG x,
    _In_  LONG y,
    _In_  RECTL *prcl,
    _In_  FLONG fl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

VOID
APIENTRY
DbgDrvMovePointer(
    _In_ SURFOBJ *pso,
    _In_ LONG x,
    _In_ LONG y,
    _In_ RECTL *prcl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
}

BOOL
APIENTRY
DbgDrvLineTo(
    SURFOBJ *pso,
    CLIPOBJ *pco,
    BRUSHOBJ *pbo,
    LONG x1,
    LONG y1,
    LONG x2,
    LONG y2,
    RECTL *prclBounds,
    MIX mix)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvSendPage(
    _In_  SURFOBJ *pso)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvStartPage(
    _In_  SURFOBJ *pso)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvEndDoc(
    _In_  SURFOBJ *pso,
    _In_  FLONG fl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvStartDoc(
    _In_ SURFOBJ *pso,
    _In_ LPWSTR pwszDocName,
    _In_ DWORD dwJobId)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

ULONG
APIENTRY
DbgDrvGetGlyphMode(
    _In_ DHPDEV dhpdev,
    _In_ FONTOBJ *pfo)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.GetGlyphMode(dhpdev, pfo);
}

VOID
APIENTRY
DbgDrvSynchronize(
    DHPDEV dhpdev,
    RECTL *prcl)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    ppdev->pldev->pfn.Synchronize(dhpdev, prcl);
}

ULONG_PTR
APIENTRY
DbgDrvSaveScreenBits(
    SURFOBJ *pso,
    ULONG iMode,
    ULONG_PTR ident,
    RECTL *prcl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

ULONG
APIENTRY
DbgDrvGetModes(
    _In_       HANDLE hDriver,
    ULONG cjSize,
    _Out_opt_  DEVMODEW *pdm)
{
    return 0;
}

VOID
APIENTRY
DbgDrvFree(
    PVOID pv,
    ULONG_PTR id)
{
}

VOID
APIENTRY
DbgDrvDestroyFont(
    FONTOBJ *pfo)
{
}

LONG
APIENTRY
DbgDrvQueryFontCaps(
    ULONG culCaps,
    ULONG *pulCaps)
{
    return 0;
}

ULONG_PTR
APIENTRY
DbgDrvLoadFontFile(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum)
{
    return 0;
}

BOOL
APIENTRY
DbgDrvUnloadFontFile(
    ULONG_PTR iFile)
{
    return 0;
}

ULONG
APIENTRY
DbgDrvFontManagement(
    _In_      SURFOBJ *pso,
    _In_opt_  FONTOBJ *pfo,
    _In_      ULONG iMode,
    _In_      ULONG cjIn,
    _In_      PVOID pvIn,
    _In_      ULONG cjOut,
    _Out_     PVOID pvOut)
{
    return 0;
}

LONG
APIENTRY
DbgDrvQueryTrueTypeTable(
    ULONG_PTR iFile,
    ULONG ulFont,
    ULONG ulTag,
    PTRDIFF dpStart,
    ULONG cjBuf,
    BYTE *pjBuf,
    PBYTE *ppjTable,
    ULONG *pcjTable)
{
    return 0;
}

LONG
APIENTRY
DbgDrvQueryTrueTypeOutline(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    HGLYPH hglyph,
    BOOL bMetricsOnly,
    GLYPHDATA *pgldt,
    ULONG cjBuf,
    TTPOLYGONHEADER *ppoly)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);
    return 0;
}

PVOID
APIENTRY
DbgDrvGetTrueTypeFile(
    ULONG_PTR iFile,
    ULONG *pcj)
{
    return 0;
}

LONG
APIENTRY
DbgDrvQueryFontFile(
    ULONG_PTR iFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf)
{
    return 0;
}

VOID
APIENTRY
DbgDrvMovePanning(
    LONG x,
    LONG y,
    FLONG fl)
{
    ERR("Obsolete driver function %s called!\n", __FUNCTION__);
    ASSERT(FALSE);
}

BOOL
APIENTRY
DbgDrvQueryAdvanceWidths(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    _In_   HGLYPH *phg,
    _Out_  PVOID pvWidths,
    ULONG cGlyphs)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);
    return 0;
}

BOOL
APIENTRY
DbgDrvSetPixelFormat(
    SURFOBJ *pso,
    LONG iPixelFormat,
    HWND hwnd)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

LONG
APIENTRY
DbgDrvDescribePixelFormat(
    DHPDEV dhpdev,
    LONG iPixelFormat,
    ULONG cjpfd,
    PIXELFORMATDESCRIPTOR *ppfd)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.DescribePixelFormat(dhpdev, iPixelFormat, cjpfd, ppfd);
}

BOOL
APIENTRY
DbgDrvSwapBuffers(
    SURFOBJ *pso,
    WNDOBJ *pwo)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvStartBanding(
    _In_  SURFOBJ *pso,
    _In_  POINTL *pptl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvNextBand(
    _In_  SURFOBJ *pso,
    _In_  POINTL *pptl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvGetDirectDrawInfo(
    DHPDEV dhpdev,
    DD_HALINFO *pHalInfo,
    DWORD *pdwNumHeaps,
    VIDEOMEMORY *pvmList,
    DWORD *pdwNumFourCCCodes,
    DWORD *pdwFourCC)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.GetDirectDrawInfo(dhpdev,
                                               pHalInfo,
                                               pdwNumHeaps,
                                               pvmList,
                                               pdwNumFourCCCodes,
                                               pdwFourCC);
}

BOOL
APIENTRY
DbgDrvEnableDirectDraw(
    DHPDEV dhpdev,
    DD_CALLBACKS *pCallBacks,
    DD_SURFACECALLBACKS *pSurfaceCallBacks,
    DD_PALETTECALLBACKS *pPaletteCallBacks)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.EnableDirectDraw(dhpdev,
                                              pCallBacks,
                                              pSurfaceCallBacks,
                                              pPaletteCallBacks);
}

VOID
APIENTRY
DbgDrvDisableDirectDraw(
    DHPDEV dhpdev)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    ppdev->pldev->pfn.DisableDirectDraw(dhpdev);
}

BOOL
APIENTRY
DbgDrvQuerySpoolType(DHPDEV PDev, LPWSTR SpoolType)
{
    ERR("Obsolete driver function %s called!\n", __FUNCTION__);
    ASSERT(FALSE);
    return 0;
}

HANDLE
APIENTRY
DbgDrvIcmCreateColorTransform(
    _In_      DHPDEV dhpdev,
    _In_      LPLOGCOLORSPACEW pLogColorSpace,
    _In_opt_  PVOID pvSourceProfile,
    _In_      ULONG cjSourceProfile,
    _In_      PVOID pvDestProfile,
    _In_      ULONG cjDestProfile,
    _In_opt_  PVOID pvTargetProfile,
    _In_      ULONG cjTargetProfile,
    _In_      DWORD dwReserved)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.IcmCreateColorTransform(dhpdev,
                                                     pLogColorSpace,
                                                     pvSourceProfile,
                                                     cjSourceProfile,
                                                     pvDestProfile,
                                                     cjDestProfile,
                                                     pvTargetProfile,
                                                     cjTargetProfile,
                                                     dwReserved);
}

BOOL
APIENTRY
DbgDrvIcmDeleteColorTransform(
    _In_  DHPDEV dhpdev,
    _In_  HANDLE hcmXform)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.IcmDeleteColorTransform(dhpdev, hcmXform);
}

BOOL
APIENTRY
DbgDrvIcmCheckBitmapBits(
    DHPDEV dhpdev,
    HANDLE hColorTransform,
    SURFOBJ *pso,
    PBYTE paResults)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.IcmCheckBitmapBits(dhpdev,
                                                hColorTransform,
                                                pso,
                                                paResults);
}

BOOL
APIENTRY
DbgDrvIcmSetDeviceGammaRamp(
    DHPDEV dhpdev,
    ULONG iFormat,
    LPVOID lpRamp)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);

    ASSERT(ppdev);

    return ppdev->pldev->pfn.IcmSetDeviceGammaRamp(dhpdev, iFormat, lpRamp);
}

BOOL
APIENTRY
DbgDrvGradientFill(
    _Inout_   SURFOBJ *psoTrg,
    _In_      CLIPOBJ *pco,
    _In_opt_  XLATEOBJ *pxlo,
    _In_      TRIVERTEX *pVertex,
    _In_      ULONG nVertex,
    _In_      PVOID pMesh,
    _In_      ULONG nMesh,
    _In_      RECTL *prclExtents,
    _In_      POINTL *pptlDitherOrg,
    _In_      ULONG ulMode)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)psoTrg->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvStretchBltROP(
    _Inout_   SURFOBJ *psoTrg,
    _Inout_   SURFOBJ *psoSrc,
    _In_opt_  SURFOBJ *psoMask,
    _In_      CLIPOBJ *pco,
    _In_opt_  XLATEOBJ *pxlo,
    _In_opt_  COLORADJUSTMENT *pca,
    _In_      POINTL *pptlHTOrg,
    _In_      RECTL *prclDest,
    _In_      RECTL *prclSrc,
    _In_opt_  POINTL *pptlMask,
    _In_      ULONG iMode,
    _In_      BRUSHOBJ *pbo,
    _In_      DWORD rop4)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)psoTrg->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvPlgBlt(
    _Inout_   SURFOBJ *psoTrg,
    _Inout_   SURFOBJ *psoSrc,
    _In_opt_  SURFOBJ *psoMsk,
    _In_      CLIPOBJ *pco,
    _In_opt_  XLATEOBJ *pxlo,
    _In_opt_  COLORADJUSTMENT *pca,
    _In_opt_  POINTL *pptlBrushOrg,
    _In_      POINTFIX *pptfx,
    _In_      RECTL *prcl,
    _In_opt_  POINTL *pptl,
    _In_      ULONG iMode)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)psoTrg->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvAlphaBlend(
    _Inout_   SURFOBJ *psoDest,
    _In_      SURFOBJ *psoSrc,
    _In_      CLIPOBJ *pco,
    _In_opt_  XLATEOBJ *pxlo,
    _In_      RECTL *prclDest,
    _In_      RECTL *prclSrc,
    _In_      BLENDOBJ *pBlendObj)
{
    return 0;
}

VOID
APIENTRY
DbgSynthesizeFont(void)
{
    ASSERT(FALSE);
}

VOID
APIENTRY
DbgGetSynthesizedFontFiles(void)
{
    ASSERT(FALSE);
}

BOOL
APIENTRY
DbgDrvTransparentBlt(
    _Inout_   SURFOBJ *psoTrg,
    _In_      SURFOBJ *psoSrc,
    _In_      CLIPOBJ *pco,
    _In_opt_  XLATEOBJ *pxlo,
    _In_      RECTL *prclDst,
    _In_      RECTL *prclSrc,
    _In_      ULONG iTransColor,
    _In_      ULONG ulReserved)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)psoTrg->hdev;
    return 0;
}

ULONG
APIENTRY
DbgDrvQueryPerBandInfo(
    _In_     SURFOBJ *pso,
    _Inout_  PERBANDINFO *pbi)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    return 0;
}

BOOL
APIENTRY
DbgDrvQueryDeviceSupport(
    SURFOBJ *pso,
    XLATEOBJ *pxlo,
    XFORMOBJ *pxo,
    ULONG iType,
    ULONG cjIn,
    _In_ PVOID pvIn,
    ULONG cjOut,
    _Out_ PVOID pvOut)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    ASSERT(ppdev);

    return ppdev->pldev->pfn.QueryDeviceSupport(pso,
                                                pxlo,
                                                (PVOID)pxo, // FIXME!!!
                                                iType,
                                                cjIn,
                                                pvIn,
                                                cjOut,
                                                pvOut);
}

HBITMAP
APIENTRY
DbgDrvDeriveSurface(
    DD_DIRECTDRAW_GLOBAL *pDirectDraw,
    DD_SURFACE_LOCAL *pSurface)
{
    return 0;
}

PFD_GLYPHATTR
APIENTRY
DbgDrvQueryGlyphAttrs(
    _In_  FONTOBJ *pfo,
    _In_  ULONG iMode)
{
    return 0;
}

VOID
APIENTRY
DbgDrvNotify(
    SURFOBJ *pso,
    ULONG iType,
    PVOID pvData)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    ASSERT(ppdev);

    ppdev->pldev->pfn.Notify(pso, iType, pvData);
}

VOID
APIENTRY
DbgDrvSynchronizeSurface(
    SURFOBJ *pso,
    RECTL *prcl,
    FLONG fl)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;
    ASSERT(ppdev);

    ppdev->pldev->pfn.SynchronizeSurface(pso, prcl, fl);
}

ULONG
APIENTRY
DbgDrvResetDevice(
    DHPDEV dhpdev,
    PVOID Reserved)
{
    PPDEVOBJ ppdev = DbgLookupDHPDEV(dhpdev);
    ASSERT(ppdev);

    return ppdev->pldev->pfn.ResetDevice(dhpdev, Reserved);
}

PVOID
apfnDbgDrvFunctions[] =
{
    DbgDrvEnablePDEV,
    DbgDrvCompletePDEV,
    DbgDrvDisablePDEV,
    DbgDrvEnableSurface,
    DbgDrvDisableSurface,
    DbgDrvAssertMode,
    DbgDrvOffset,
    DbgDrvResetPDEV,
    NULL, //DbgDrvDisableDriver,
    DbgDrvReserved, // Unknown1
    DbgDrvCreateDeviceBitmap,
    NULL, //DbgDrvDeleteDeviceBitmap,
    DbgDrvRealizeBrush,
    DbgDrvDitherColor,
    DbgDrvStrokePath,
    DbgDrvFillPath,
    DbgDrvStrokeAndFillPath,
    DbgDrvPaint,
    DbgDrvBitBlt,
    NULL, //DbgDrvCopyBits,
    NULL, //DbgDrvStretchBlt,
    DbgDrvReserved,
    DbgDrvSetPalette,
    NULL, //DbgDrvTextOut,
    NULL, //DbgDrvEscape,
    NULL, //DbgDrvDrawEscape,
    DbgDrvQueryFont,
    NULL, //DbgDrvQueryFontTree,
    DbgDrvQueryFontData,
    NULL, //DbgDrvSetPointerShape,
    NULL, //DbgDrvMovePointer,
    NULL, //DbgDrvLineTo,
    NULL, //DbgDrvSendPage,
    NULL, //DbgDrvStartPage,
    NULL, //DbgDrvEndDoc,
    NULL, //DbgDrvStartDoc,
    DbgDrvReserved,
    DbgDrvGetGlyphMode,
    DbgDrvSynchronize,
    DbgDrvReserved,
    NULL, //DbgDrvSaveScreenBits,
    NULL, //DbgDrvGetModes,
    NULL, //DbgDrvFree,
    NULL, //DbgDrvDestroyFont,
    NULL, //DbgDrvQueryFontCaps,
    NULL, //DbgDrvLoadFontFile,
    NULL, //DbgDrvUnloadFontFile,
    NULL, //DbgDrvFontManagement,
    NULL, //DbgDrvQueryTrueTypeTable,
    NULL, //DbgDrvQueryTrueTypeOutline,
    NULL, //DbgDrvGetTrueTypeFile,
    NULL, //DbgDrvQueryFontFile,
    DbgDrvMovePanning,
    NULL, //DbgDrvQueryAdvanceWidths,
    NULL, //DbgDrvSetPixelFormat,
    DbgDrvDescribePixelFormat,
    NULL, //DbgDrvSwapBuffers,
    NULL, //DbgDrvStartBanding,
    NULL, //DbgDrvNextBand,
    DbgDrvGetDirectDrawInfo,
    DbgDrvEnableDirectDraw,
    DbgDrvDisableDirectDraw,
    DbgDrvQuerySpoolType,
    DbgDrvReserved,
    DbgDrvIcmCreateColorTransform,
    DbgDrvIcmDeleteColorTransform,
    DbgDrvIcmCheckBitmapBits,
    DbgDrvIcmSetDeviceGammaRamp,
    NULL, //DbgDrvGradientFill,
    NULL, //DbgDrvStretchBltROP,
    NULL, //DbgDrvPlgBlt,
    NULL, //DbgDrvAlphaBlend,
    NULL, //DbgSynthesizeFont,
    NULL, //DbgGetSynthesizedFontFiles,
    NULL, //DbgDrvTransparentBlt,
    NULL, //DbgDrvQueryPerBandInfo,
    DbgDrvQueryDeviceSupport,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved,
    NULL, //DbgDrvDeriveSurface,
    NULL, //DbgDrvQueryGlyphAttrs,
    DbgDrvNotify,
    DbgDrvSynchronizeSurface,
    DbgDrvResetDevice,
    DbgDrvReserved,
    DbgDrvReserved,
    DbgDrvReserved
};
