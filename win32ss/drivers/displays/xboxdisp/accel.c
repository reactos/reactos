/*
 * PROJECT:     Xbox NV2A accelerated GDI display driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DrvBitBlt / DrvCopyBits / DrvEscape acceleration hooks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 * 
 * This might seem kind of limited but either it's because of the XEMU  emulation or the actual XBOX GPU core
 * lacks a lot of the gdi stuff. Like accelearted text!
 */

#include "xboxdisp.h"

/* ROP4 / ROP3 codes */
#define ROP4_PATCOPY  0xF0F0
#define ROP4_SRCCOPY  0xCCCC

static __inline BOOL
RectClip(RECTL *r, const RECTL *clip)
{
    if (r->left   < clip->left)   r->left   = clip->left;
    if (r->top    < clip->top)    r->top    = clip->top;
    if (r->right  > clip->right)  r->right  = clip->right;
    if (r->bottom > clip->bottom) r->bottom = clip->bottom;
    return (r->right > r->left) && (r->bottom > r->top);
}

static BOOL
NvFillRect(PPDEV ppdev, const RECTL *r, ULONG color)
{
    NV2A_FILL_RECT cmd;
    DWORD bytes;

    cmd.X      = r->left;
    cmd.Y      = r->top;
    cmd.Width  = r->right - r->left;
    cmd.Height = r->bottom - r->top;
    cmd.Color  = color;
    return !EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_FILL_RECT,
                               &cmd, sizeof(cmd), NULL, 0, &bytes);
}

static BOOL
NvScreenBlt(PPDEV ppdev, const RECTL *dst, const POINTL *src)
{
    NV2A_SCREEN_BLT cmd;
    DWORD bytes;

    cmd.SrcX   = src->x;
    cmd.SrcY   = src->y;
    cmd.DstX   = dst->left;
    cmd.DstY   = dst->top;
    cmd.Width  = dst->right - dst->left;
    cmd.Height = dst->bottom - dst->top;
    return !EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_SCREEN_BLT,
                               &cmd, sizeof(cmd), NULL, 0, &bytes);
}

static __inline BOOL
SurfaceIsScreen(PPDEV ppdev, SURFOBJ *pso)
{
    return pso != NULL && pso->dhpdev == (DHPDEV)ppdev;
}

/* Iterate the clip object's rectangles and call op() for each one, clipped
 * to the destination rect.  Returns FALSE if the engine path is needed. */
static BOOL
ForEachClipRect(CLIPOBJ *pco, const RECTL *dst, PPDEV ppdev,
                BOOL (*op)(PPDEV, const RECTL*, void*), void *ctx)
{
    RECTL r;
    if (pco == NULL || pco->iDComplexity == DC_TRIVIAL)
    {
        r = *dst;
        return op(ppdev, &r, ctx);
    }
    if (pco->iDComplexity == DC_RECT)
    {
        r = *dst;
        if (!RectClip(&r, &pco->rclBounds))
            return TRUE;
        return op(ppdev, &r, ctx);
    }

    /* DC_COMPLEX: enumerate the rectangles. */
    {
        ENUMRECTS  buf;
        BOOL       moreRects;
        ULONG      i;

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do
        {
            moreRects = CLIPOBJ_bEnum(pco, sizeof(buf), (ULONG*)&buf);
            for (i = 0; i < buf.c; i++)
            {
                r = *dst;
                if (RectClip(&r, &buf.arcl[i]))
                {
                    if (!op(ppdev, &r, ctx))
                        return FALSE;
                }
            }
        } while (moreRects);
    }
    return TRUE;
}

/* Op callbacks */
static BOOL OpFill(PPDEV ppdev, const RECTL *r, void *ctx)
{
    return NvFillRect(ppdev, r, *(ULONG*)ctx);
}

typedef struct { POINTL src; POINTL dstOrigin; } BltCtx;
static BOOL OpBlt(PPDEV ppdev, const RECTL *r, void *ctxRaw)
{
    BltCtx *c = (BltCtx*)ctxRaw;
    POINTL src;
    src.x = c->src.x + (r->left - c->dstOrigin.x);
    src.y = c->src.y + (r->top  - c->dstOrigin.y);
    return NvScreenBlt(ppdev, r, &src);
}

/* ------------------------------------------------------------------------- */
/* Offscreen device-bitmap heap + general VRAM blit ------------------------ */
/* ------------------------------------------------------------------------- */

/* First-fit free-list allocator over the offscreen VRAM heap.  Offsets are
 * absolute GPU offsets; returns 0 on failure (a real heap offset is never 0). */
static ULONG
HeapAlloc(PPDEV ppdev, ULONG size)
{
    ULONG i;
    size = (size + 63) & ~63UL;
    for (i = 0; i < ppdev->HeapSpanCount; i++)
    {
        if (ppdev->HeapFree[i].Size >= size)
        {
            ULONG off = ppdev->HeapFree[i].Off;
            ppdev->HeapFree[i].Off  += size;
            ppdev->HeapFree[i].Size -= size;
            return off;
        }
    }
    return 0;
}

static VOID
HeapReturn(PPDEV ppdev, ULONG off, ULONG size)
{
    ULONG i;
    size = (size + 63) & ~63UL;
    for (i = 0; i < ppdev->HeapSpanCount; i++)
    {
        if (ppdev->HeapFree[i].Off + ppdev->HeapFree[i].Size == off)
        {   /* coalesce with a span that ends where this one begins */
            ppdev->HeapFree[i].Size += size;
            return;
        }
        if (off + size == ppdev->HeapFree[i].Off)
        {   /* coalesce with a span that begins where this one ends */
            ppdev->HeapFree[i].Off   = off;
            ppdev->HeapFree[i].Size += size;
            return;
        }
    }
    if (ppdev->HeapSpanCount < XBOXDISP_HEAP_MAX_SPANS)
    {
        ppdev->HeapFree[ppdev->HeapSpanCount].Off  = off;
        ppdev->HeapFree[ppdev->HeapSpanCount].Size = size;
        ppdev->HeapSpanCount++;
    }
    /* else: span table full -> leak this span (bounded, never crashes). */
}

/* If pso's pixels live in VRAM (the screen or a device bitmap), return its
 * absolute GPU offset + byte pitch.  Top-down surfaces only (positive lDelta). */
static BOOL
GetVramSurf(PPDEV ppdev, SURFOBJ *pso, ULONG *gpuOff, ULONG *pitch)
{
    if (pso == NULL)
        return FALSE;

    if (pso->iType == STYPE_DEVBITMAP && pso->dhsurf != 0)
    {
        PXBOXDISP_DEVBMP d = (PXBOXDISP_DEVBMP)pso->dhsurf;
        *gpuOff = d->GpuOffset;
        *pitch  = d->Pitch;
        return TRUE;
    }

    if (ppdev->VramBase != NULL && pso->pvScan0 != NULL && pso->lDelta > 0 &&
        (PUCHAR)pso->pvScan0 >= (PUCHAR)ppdev->VramBase &&
        (PUCHAR)pso->pvScan0 <  (PUCHAR)ppdev->VramBase + ppdev->VramLen)
    {
        *gpuOff = ppdev->FbGpuOffset +
                  (ULONG)((PUCHAR)pso->pvScan0 - (PUCHAR)ppdev->VramBase);
        *pitch  = (ULONG)pso->lDelta;
        return TRUE;
    }
    return FALSE;
}

static BOOL
NvBltEx(PPDEV ppdev, ULONG sOff, ULONG sPitch, ULONG sx, ULONG sy,
        ULONG dOff, ULONG dPitch, ULONG dx, ULONG dy, ULONG w, ULONG h)
{
    NV2A_BLT_EX cmd;
    DWORD bytes;
    cmd.SrcOffset = sOff; cmd.SrcPitch = sPitch; cmd.SrcX = sx; cmd.SrcY = sy;
    cmd.DstOffset = dOff; cmd.DstPitch = dPitch; cmd.DstX = dx; cmd.DstY = dy;
    cmd.Width = w; cmd.Height = h;
    return !EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_BLT_EX,
                               &cmd, sizeof(cmd), NULL, 0, &bytes);
}

/* HW-blit a single clipped rect between two VRAM surfaces (src origin tracked). */
typedef struct { ULONG sOff, sPitch, dOff, dPitch; POINTL src; POINTL dstOrigin; } BltExCtx;
static BOOL OpBltEx(PPDEV ppdev, const RECTL *r, void *ctxRaw)
{
    BltExCtx *c = (BltExCtx*)ctxRaw;
    ULONG sx = (ULONG)(c->src.x + (r->left - c->dstOrigin.x));
    ULONG sy = (ULONG)(c->src.y + (r->top  - c->dstOrigin.y));
    return NvBltEx(ppdev, c->sOff, c->sPitch, sx, sy,
                   c->dOff, c->dPitch, (ULONG)r->left, (ULONG)r->top,
                   (ULONG)(r->right - r->left), (ULONG)(r->bottom - r->top));
}

/* Fallback for any device-bitmap copy we don't accelerate: alias each device
 * bitmap's VRAM as a temporary engine bitmap (it has CPU-accessible bits) and let
 * the GDI engine do the copy/format-conversion straight into/out of the VRAM. */
static BOOL
CopyBitsViaEngine(PPDEV ppdev, SURFOBJ *psoDst, SURFOBJ *psoSrc, CLIPOBJ *pco,
                  XLATEOBJ *pxlo, RECTL *prclDst, POINTL *pptlSrc)
{
    HSURF hD = NULL, hS = NULL;
    SURFOBJ *pD = psoDst, *pS = psoSrc;
    BOOL ok = FALSE;

    if (psoDst != NULL && psoDst->iType == STYPE_DEVBITMAP && psoDst->dhsurf != 0)
    {
        PXBOXDISP_DEVBMP d = (PXBOXDISP_DEVBMP)psoDst->dhsurf;
        SIZEL sz; sz.cx = d->Width; sz.cy = d->Height;
        hD = (HSURF)EngCreateBitmap(sz, d->Pitch, ppdev->iDitherFormat, BMF_TOPDOWN, d->CpuPtr);
        if (hD == NULL) return FALSE;
        pD = EngLockSurface(hD);
        if (pD == NULL) { EngDeleteSurface(hD); return FALSE; }
    }
    if (psoSrc != NULL && psoSrc->iType == STYPE_DEVBITMAP && psoSrc->dhsurf != 0)
    {
        PXBOXDISP_DEVBMP d = (PXBOXDISP_DEVBMP)psoSrc->dhsurf;
        SIZEL sz; sz.cx = d->Width; sz.cy = d->Height;
        hS = (HSURF)EngCreateBitmap(sz, d->Pitch, ppdev->iDitherFormat, BMF_TOPDOWN, d->CpuPtr);
        if (hS == NULL) goto cleanup;
        pS = EngLockSurface(hS);
        if (pS == NULL) { EngDeleteSurface(hS); hS = NULL; goto cleanup; }
    }

    ok = EngCopyBits(pD, pS, pco, pxlo, prclDst, pptlSrc);

cleanup:
    if (hS != NULL) { EngUnlockSurface(pS); EngDeleteSurface(hS); }
    if (hD != NULL) { EngUnlockSurface(pD); EngDeleteSurface(hD); }
    return ok;
}

/* Same alias trick for BitBlt (engine can't touch a device bitmap's pixels). */
static BOOL
BitBltViaEngine(PPDEV ppdev, SURFOBJ *psoTrg, SURFOBJ *psoSrc, SURFOBJ *psoMask,
                CLIPOBJ *pco, XLATEOBJ *pxlo, RECTL *prclTrg, POINTL *pptlSrc,
                POINTL *pptlMask, BRUSHOBJ *pbo, POINTL *pptlBrush, ROP4 rop4)
{
    HSURF hT = NULL, hS = NULL;
    SURFOBJ *pT = psoTrg, *pS = psoSrc;
    BOOL ok = FALSE;

    if (psoTrg != NULL && psoTrg->iType == STYPE_DEVBITMAP && psoTrg->dhsurf != 0)
    {
        PXBOXDISP_DEVBMP d = (PXBOXDISP_DEVBMP)psoTrg->dhsurf;
        SIZEL sz; sz.cx = d->Width; sz.cy = d->Height;
        hT = (HSURF)EngCreateBitmap(sz, d->Pitch, ppdev->iDitherFormat, BMF_TOPDOWN, d->CpuPtr);
        if (hT == NULL) return FALSE;
        pT = EngLockSurface(hT);
        if (pT == NULL) { EngDeleteSurface(hT); return FALSE; }
    }
    if (psoSrc != NULL && psoSrc->iType == STYPE_DEVBITMAP && psoSrc->dhsurf != 0)
    {
        PXBOXDISP_DEVBMP d = (PXBOXDISP_DEVBMP)psoSrc->dhsurf;
        SIZEL sz; sz.cx = d->Width; sz.cy = d->Height;
        hS = (HSURF)EngCreateBitmap(sz, d->Pitch, ppdev->iDitherFormat, BMF_TOPDOWN, d->CpuPtr);
        if (hS == NULL) goto cleanup;
        pS = EngLockSurface(hS);
        if (pS == NULL) { EngDeleteSurface(hS); hS = NULL; goto cleanup; }
    }

    ok = EngBitBlt(pT, pS, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);

cleanup:
    if (hS != NULL) { EngUnlockSurface(pS); EngDeleteSurface(hS); }
    if (hT != NULL) { EngUnlockSurface(pT); EngDeleteSurface(hT); }
    return ok;
}

/* PDEV from whichever surface is ours (the screen / a device bitmap carries our
 * dhpdev; a system DIB carries NULL). */
static __inline PPDEV
PdevOf(SURFOBJ *a, SURFOBJ *b)
{
    if (a != NULL && a->dhpdev != NULL) return (PPDEV)a->dhpdev;
    if (b != NULL && b->dhpdev != NULL) return (PPDEV)b->dhpdev;
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* DDI entry points -------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

HBITMAP APIENTRY
DrvCreateDeviceBitmap(DHPDEV dhpdev, SIZEL sizl, ULONG iFormat)
{
    PPDEV ppdev = (PPDEV)dhpdev;
    PXBOXDISP_DEVBMP d;
    ULONG pitch, size, gpuOff;
    HSURF hsurf;

    /* Only cache screen-format (32bpp) bitmaps in VRAM; decline everything else
     * so GDI keeps it as a system DIB.  Declining is always safe. */
    if (!ppdev->AccelAvailable || ppdev->VramBase == NULL ||
        iFormat != ppdev->iDitherFormat || iFormat != BMF_32BPP ||
        sizl.cx <= 0 || sizl.cy <= 0 || sizl.cx > 4096 || sizl.cy > 4096)
        return NULL;

    pitch = ((ULONG)sizl.cx * 4 + 63) & ~63UL;
    size  = pitch * (ULONG)sizl.cy;

    gpuOff = HeapAlloc(ppdev, size);
    if (gpuOff == 0)
        return NULL;   /* heap exhausted -> GDI uses a system DIB */

    d = EngAllocMem(FL_ZERO_MEMORY, sizeof(*d), ALLOC_TAG);
    if (d == NULL) { HeapReturn(ppdev, gpuOff, size); return NULL; }

    d->ppdev     = ppdev;
    d->GpuOffset = gpuOff;
    d->Pitch     = pitch;
    d->HeapOff   = gpuOff;
    d->HeapSize  = size;
    d->Width     = sizl.cx;
    d->Height    = sizl.cy;
    d->CpuPtr    = (PUCHAR)ppdev->VramBase + (gpuOff - ppdev->FbGpuOffset);

    hsurf = EngCreateDeviceSurface((DHSURF)d, sizl, iFormat);
    if (hsurf == NULL)
    {
        EngFreeMem(d);
        HeapReturn(ppdev, gpuOff, size);
        return NULL;
    }
    d->hsurf = hsurf;

    /* Hook COPYBITS only: GDI routes every other op on the device bitmap through
     * DrvCopyBits (to/from a temp DIB), which we handle. */
    if (!EngAssociateSurface(hsurf, ppdev->hDevEng, HOOK_COPYBITS))
    {
        EngDeleteSurface(hsurf);
        EngFreeMem(d);
        HeapReturn(ppdev, gpuOff, size);
        return NULL;
    }

    return (HBITMAP)hsurf;
}

VOID APIENTRY
DrvDeleteDeviceBitmap(DHSURF dhsurf)
{
    PXBOXDISP_DEVBMP d = (PXBOXDISP_DEVBMP)dhsurf;
    if (d == NULL)
        return;
    if (d->ppdev != NULL)
        HeapReturn(d->ppdev, d->HeapOff, d->HeapSize);
    if (d->hsurf != NULL)
        EngDeleteSurface(d->hsurf);
    EngFreeMem(d);
}

BOOL APIENTRY
DrvBitBlt(SURFOBJ *psoTrg, SURFOBJ *psoSrc, SURFOBJ *psoMask,
          CLIPOBJ *pco, XLATEOBJ *pxlo,
          RECTL *prclTrg, POINTL *pptlSrc, POINTL *pptlMask,
          BRUSHOBJ *pbo, POINTL *pptlBrush, ROP4 rop4)
{
    PPDEV ppdev = PdevOf(psoTrg, psoSrc);
    ULONG sOff, sPitch, dOff, dPitch;

    if (ppdev == NULL || psoMask != NULL)
        goto fallback;

    /* Solid colour PATCOPY fill into the screen. */
    if (SurfaceIsScreen(ppdev, psoTrg) && psoSrc == NULL && pbo != NULL &&
        pbo->iSolidColor != 0xFFFFFFFF && rop4 == ROP4_PATCOPY)
    {
        ULONG color = pbo->iSolidColor;
        if (ForEachClipRect(pco, prclTrg, ppdev, OpFill, &color))
            return TRUE;
        goto fallback;
    }

    /* SRCCOPY between two VRAM surfaces (screen<->screen, or a device bitmap). */
    if (psoSrc != NULL && rop4 == ROP4_SRCCOPY && pxlo == NULL &&
        GetVramSurf(ppdev, psoTrg, &dOff, &dPitch) &&
        GetVramSurf(ppdev, psoSrc, &sOff, &sPitch))
    {
        if (psoSrc == psoTrg)   /* same surface: overlap-safe screen blt */
        {
            BltCtx c;
            c.src         = *pptlSrc;
            c.dstOrigin.x = prclTrg->left;
            c.dstOrigin.y = prclTrg->top;
            if (ForEachClipRect(pco, prclTrg, ppdev, OpBlt, &c))
                return TRUE;
        }
        else                    /* different surfaces (no overlap): general blt */
        {
            BltExCtx c;
            c.sOff = sOff; c.sPitch = sPitch; c.dOff = dOff; c.dPitch = dPitch;
            c.src         = *pptlSrc;
            c.dstOrigin.x = prclTrg->left;
            c.dstOrigin.y = prclTrg->top;
            if (ForEachClipRect(pco, prclTrg, ppdev, OpBltEx, &c))
                return TRUE;
        }
        goto fallback;
    }

fallback:
    /* The engine cannot touch a device bitmap's pixels (no engine bits) — alias
     * it first so EngBitBlt operates on the VRAM directly. */
    if ((psoSrc != NULL && psoSrc->iType == STYPE_DEVBITMAP) ||
        (psoTrg != NULL && psoTrg->iType == STYPE_DEVBITMAP))
        return BitBltViaEngine(ppdev, psoTrg, psoSrc, psoMask, pco, pxlo,
                               prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);
    return EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo,
                     prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);
}

BOOL APIENTRY
DrvCopyBits(SURFOBJ *psoDst, SURFOBJ *psoSrc, CLIPOBJ *pco, XLATEOBJ *pxlo,
            RECTL *prclDst, POINTL *pptlSrc)
{
    PPDEV ppdev = PdevOf(psoDst, psoSrc);
    ULONG sOff, sPitch, dOff, dPitch;

    /* Fast path: VRAM -> VRAM (screen and/or device bitmap), no translation. */
    if (ppdev != NULL && pxlo == NULL &&
        GetVramSurf(ppdev, psoDst, &dOff, &dPitch) &&
        GetVramSurf(ppdev, psoSrc, &sOff, &sPitch))
    {
        if (psoSrc == psoDst)   /* same surface: overlap-safe screen blt */
        {
            BltCtx c;
            c.src         = *pptlSrc;
            c.dstOrigin.x = prclDst->left;
            c.dstOrigin.y = prclDst->top;
            if (ForEachClipRect(pco, prclDst, ppdev, OpBlt, &c))
                return TRUE;
        }
        else
        {
            BltExCtx c;
            c.sOff = sOff; c.sPitch = sPitch; c.dOff = dOff; c.dPitch = dPitch;
            c.src         = *pptlSrc;
            c.dstOrigin.x = prclDst->left;
            c.dstOrigin.y = prclDst->top;
            if (ForEachClipRect(pco, prclDst, ppdev, OpBltEx, &c))
                return TRUE;
        }
        /* fall through to engine on a partial (clip) miss */
    }

    /* A device bitmap has no engine-accessible bits — alias it for EngCopyBits. */
    if ((psoDst != NULL && psoDst->iType == STYPE_DEVBITMAP) ||
        (psoSrc != NULL && psoSrc->iType == STYPE_DEVBITMAP))
    {
        if (ppdev == NULL)
            return FALSE;
        return CopyBitsViaEngine(ppdev, psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);
    }

    return EngCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);
}

/* ------------------------------------------------------------------------- */
/* OpenGL discovery escape -------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* opengl32.dll calls ExtEscape(hdc, QUERYESCSUPPORT) to ask if we support
 * OPENGL_GETINFO, then ExtEscape(hdc, OPENGL_GETINFO) to fetch the ICD
 * registration name.  We respond with "xboxopengl" — opengl32 then opens
 * HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\xboxopengl
 * to find the DLL path. */

typedef struct
{
    ULONG Version;
    ULONG DriverVersion;
    WCHAR DriverName[260];
} OpenGlDrvInfo;

ULONG APIENTRY
DrvEscape(SURFOBJ *pso, ULONG iEsc, ULONG cjIn, PVOID pvIn,
          ULONG cjOut, PVOID pvOut)
{
    static const WCHAR kIcdName[] = L"xboxopengl";

    if (iEsc == QUERYESCSUPPORT)
    {
        if (cjIn >= sizeof(DWORD) &&
            (*(DWORD*)pvIn == OPENGL_GETINFO ||
             *(DWORD*)pvIn == XBOX_ESC_NV2A_DRAW3D ||
             *(DWORD*)pvIn == XBOX_ESC_NV2A_TEXUPLOAD ||
             *(DWORD*)pvIn == XBOX_ESC_NV2A_READBACK ||
             *(DWORD*)pvIn == XBOX_ESC_NV2A_WRITEBACK))
            return 1;
        return 0;
    }

    /* Texture upload: forward the NV2A_TEX_UPLOAD payload to the miniport, which
     * copies it into the VRAM texture heap and returns the GPU offset in pvOut. */
    if (iEsc == XBOX_ESC_NV2A_TEXUPLOAD)
    {
        PPDEV ppdev;
        DWORD bytes;

        if (pso == NULL || pvIn == NULL || cjIn < sizeof(NV2A_TEX_UPLOAD) ||
            pvOut == NULL || cjOut < sizeof(ULONG))
            return 0;
        ppdev = (PPDEV)pso->dhpdev;
        if (ppdev == NULL)
            return 0;

        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_TEX_UPLOAD,
                               pvIn, cjIn, pvOut, cjOut, &bytes) == 0)
            return 1;
        return 0;
    }

    /* Read back a rect of the rendered offscreen surface (glReadPixels/CopyTex*). */
    if (iEsc == XBOX_ESC_NV2A_READBACK)
    {
        PPDEV ppdev;
        DWORD bytes;

        if (pso == NULL || pvIn == NULL || cjIn < sizeof(NV2A_READBACK) ||
            pvOut == NULL || cjOut == 0)
            return 0;
        ppdev = (PPDEV)pso->dhpdev;
        if (ppdev == NULL)
            return 0;
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_READBACK,
                               pvIn, cjIn, pvOut, cjOut, &bytes) == 0)
            return 1;
        return 0;
    }

    /* Write a rect of ARGB pixels into the offscreen surface (glDrawPixels/Bitmap). */
    if (iEsc == XBOX_ESC_NV2A_WRITEBACK)
    {
        PPDEV ppdev;
        DWORD bytes;

        if (pso == NULL || pvIn == NULL || cjIn < sizeof(NV2A_WRITEBACK))
            return 0;
        ppdev = (PPDEV)pso->dhpdev;
        if (ppdev == NULL)
            return 0;
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_WRITEBACK,
                               pvIn, cjIn, NULL, 0, &bytes) == 0)
            return 1;
        return 0;
    }

    /* Private channel: the user-mode xboxogl ICD can't call the video IOCTLs,
     * so it ships a clip-space triangle batch (NV2A_DRAW_3D) here; forward it
     * to the miniport's NV2A 3D (Kelvin) engine. */
    if (iEsc == XBOX_ESC_NV2A_DRAW3D)
    {
        PPDEV ppdev;
        DWORD bytes;

        if (pso == NULL || pvIn == NULL || cjIn < sizeof(unsigned long))
            return 0;
        ppdev = (PPDEV)pso->dhpdev;
        if (ppdev == NULL)
            return 0;

        /* EngDeviceIoControl returns 0 on success. */
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_DRAW_3D,
                               pvIn, cjIn, NULL, 0, &bytes) == 0)
            return 1;
        return 0;
    }

    if (iEsc == OPENGL_GETINFO)
    {
        OpenGlDrvInfo *info;

        if (cjOut < sizeof(OpenGlDrvInfo) || pvOut == NULL)
            return 0;

        info = (OpenGlDrvInfo*)pvOut;
        RtlZeroMemory(info, sizeof(*info));
        info->Version       = 0x00010000; /* protocol v1 */
        info->DriverVersion = 0x00010000;
        memcpy(info->DriverName, kIcdName, sizeof(kIcdName));
        return sizeof(OpenGlDrvInfo);
    }

    return 0;
}
