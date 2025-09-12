/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UEFI GOP Boot Video Driver support
 * COPYRIGHT:   Copyright 2024-2025 ReactOS
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/*
 * GOP framebuffer backend for INBV/boot video.
 * Notes:
 *  - We support GOP PixelFormat RGBX8 (0), BGRX8 (1), and BitMask (2).
 *  - Write-combined mapping for performance.
 *  - Safe fast paths (memcpy/fill) only when formats match and BGRT isn't preserved.
 *  - All coordinates are in pixels; colors passed as 0x00RRGGBB.
 */

/* ===== Globals ===== */

static PHYSICAL_ADDRESS GopFbPhys = {{0}};
static PUCHAR           GopFbBase = NULL;
static SIZE_T           GopFbSize = 0;

static ULONG GopWidth  = 0;
static ULONG GopHeight = 0;
static ULONG GopPsl    = 0;   /* PixelsPerScanLine from firmware */
static ULONG GopPitch  = 0;   /* bytes per scanline (Psl * Bpp) */

static ULONG GopFormat = 0;   /* 0: RGBX8, 1: BGRX8, 2: BitMask */
static ULONG GopBpp    = 4;   /* bytes per pixel (2/3/4) */

/* BitMask fields (for Format==2) */
static ULONG RMShift=0, GMShift=0, BMShift=0;
static ULONG RWidth=0,  GWidth=0,  BWidth=0;
static ULONG RMask=0,   GMask=0,   BMask=0;

/* BGRT (preserve logo region if precise dims known) */
static BOOLEAN   BgrtValid = FALSE;
static ULONG     BgrtX = 0, BgrtY = 0, BgrtW = 0, BgrtH = 0;
static ULONGLONG BgrtAddr = 0;
static ULONG     BgrtSize = 0;

/* ===== Helpers ===== */

static inline VOID
ComputeMaskInfo(_In_ ULONG Mask, _Out_ PULONG Shift, _Out_ PULONG Width)
{
    ULONG s = 0, w = 0;
    if (Mask != 0)
    {
        while (((Mask >> s) & 1) == 0 && s < 32) s++;
        ULONG m = Mask >> s;
        while ((m & 1) && w < 32) { w++; m >>= 1; }
    }
    *Shift = s;
    *Width = w;
}

static inline ULONG
Scale8ToMask(_In_ UCHAR v8, _In_ ULONG width)
{
    if (width == 0) return 0;
    /* Spread 0..255 into 0..(2^width-1) */
    const ULONG maxv = (1u << width) - 1u;
    return (ULONG)((v8 * maxv + 127u) / 255u);
}

static inline UCHAR
ScaleMaskTo8(_In_ ULONG v, _In_ ULONG width)
{
    if (width == 0) return 0;
    const ULONG maxv = (1u << width) - 1u;
    return (UCHAR)((v * 255u + (maxv >> 1)) / maxv);
}

/* Pack R,G,B (0..255) into destination pixel for current GOP format */
static inline ULONG
PackRGB888(_In_ UCHAR r, _In_ UCHAR g, _In_ UCHAR b)
{
    switch (GopFormat)
    {
        case 0: /* RGBX8 -> memory (LE): B,G,R,X */
            return ((ULONG)b) | ((ULONG)g << 8) | ((ULONG)r << 16);
        case 1: /* BGRX8 -> memory (LE): R,G,B,X */
            return ((ULONG)r) | ((ULONG)g << 8) | ((ULONG)b << 16);
        case 2: /* BitMask */
        {
            ULONG out = 0;
            if (RWidth) out |= (Scale8ToMask(r, RWidth) << RMShift) & RMask;
            if (GWidth) out |= (Scale8ToMask(g, GWidth) << GMShift) & GMask;
            if (BWidth) out |= (Scale8ToMask(b, BWidth) << BMShift) & BMask;
            return out;
        }
        default:
            return 0;
    }
}

/* Unpack a pixel from FB format to 0x00RRGGBB */
static inline ULONG
UnpackToRGB888(_In_ ULONG px)
{
    UCHAR r=0,g=0,b=0;
    switch (GopFormat)
    {
        case 0: /* RGBX8: memory B,G,R,X */
            b = (UCHAR)( px        & 0xFF);
            g = (UCHAR)((px >> 8)  & 0xFF);
            r = (UCHAR)((px >> 16) & 0xFF);
            break;
        case 1: /* BGRX8: memory R,G,B,X */
            r = (UCHAR)( px        & 0xFF);
            g = (UCHAR)((px >> 8)  & 0xFF);
            b = (UCHAR)((px >> 16) & 0xFF);
            break;
        case 2: /* BitMask */
        {
            ULONG rv = (px & RMask) >> RMShift;
            ULONG gv = (px & GMask) >> GMShift;
            ULONG bv = (px & BMask) >> BMShift;
            r = ScaleMaskTo8(rv, RWidth);
            g = ScaleMaskTo8(gv, GWidth);
            b = ScaleMaskTo8(bv, BWidth);
            break;
        }
        default:
            break;
    }
    return ((ULONG)r << 16) | ((ULONG)g << 8) | (ULONG)b;
}

static inline BOOLEAN
RectEmpty(_In_ LONG l, _In_ LONG t, _In_ LONG r, _In_ LONG b)
{
    return (r < l) || (b < t);
}

static inline BOOLEAN
PointInBgrt(_In_ ULONG x, _In_ ULONG y)
{
    if (!BgrtValid || BgrtW == 0 || BgrtH == 0) return FALSE;
    return (x >= BgrtX && x < (BgrtX + BgrtW) &&
            y >= BgrtY && y < (BgrtY + BgrtH));
}

/* ===== Public API ===== */

BOOLEAN
NTAPI
GopVidInitialize(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_EXTENSION Ext;

    DPRINT1("[GOP] Init\n");

    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        DPRINT1("[GOP] Init at IRQL %d is invalid\n", KeGetCurrentIrql());
        return FALSE;
    }

    Ext = LoaderBlock ? LoaderBlock->Extension : NULL;
    if (!Ext)
    {
        DPRINT1("[GOP] No loader extension\n");
        return FALSE;
    }

    if (Ext->GopFramebuffer.FrameBufferBase.QuadPart == 0 ||
        Ext->GopFramebuffer.FrameBufferSize == 0)
    {
        DPRINT1("[GOP] Missing framebuffer info\n");
        return FALSE;
    }

    /* Basic geometry */
    GopWidth  = Ext->GopFramebuffer.HorizontalResolution;
    GopHeight = Ext->GopFramebuffer.VerticalResolution;
    GopPsl    = Ext->GopFramebuffer.PixelsPerScanLine;
    GopFormat = Ext->GopFramebuffer.PixelFormat;
    RMask     = Ext->GopFramebuffer.RedMask;
    GMask     = Ext->GopFramebuffer.GreenMask;
    BMask     = Ext->GopFramebuffer.BlueMask;

    /* Compute Bpp */
    switch (GopFormat)
    {
        case 0: /* RGBX8 */
        case 1: /* BGRX8 */
            GopBpp = 4;
            break;
        case 2: /* BitMask (assume up to 32 bits; derive) */
        {
            ULONG masks = RMask | GMask | BMask;
            ULONG bits = 0;
            while (masks) { bits++; masks >>= 1; }
            GopBpp = (bits + 7) / 8;
            if (GopBpp < 2) GopBpp = 2;
            if (GopBpp > 4) GopBpp = 4;

            ComputeMaskInfo(RMask, &RMShift, &RWidth);
            ComputeMaskInfo(GMask, &GMShift, &GWidth);
            ComputeMaskInfo(BMask, &BMShift, &BWidth);
            break;
        }
        default:
            DPRINT1("[GOP] Unsupported PixelFormat=%lu\n", GopFormat);
            return FALSE; /* PixelBltOnly cannot be linear-mapped */
    }

    /* Pitch & size */
    GopPitch  = GopPsl * GopBpp;
    GopFbSize = (SIZE_T)Ext->GopFramebuffer.FrameBufferSize;
    GopFbPhys = Ext->GopFramebuffer.FrameBufferBase;

    DPRINT1("[GOP] %ux%u, PSL=%u, Pitch=%u, Format=%lu, Bpp=%u, FB=0x%llx, Size=%u\n",
            GopWidth, GopHeight, GopPsl, GopPitch, GopFormat, GopBpp,
            (unsigned long long)GopFbPhys.QuadPart, (unsigned)GopFbSize);

    /* Map FB as write-combined */
    GopFbBase = MmMapIoSpace(GopFbPhys, GopFbSize, MmWriteCombined);
    if (!GopFbBase)
    {
        DPRINT1("[GOP] MmMapIoSpace WC failed, trying NonCached\n");
        GopFbBase = MmMapIoSpace(GopFbPhys, GopFbSize, MmNonCached);
        if (!GopFbBase)
        {
            DPRINT1("[GOP] Map failed\n");
            return FALSE;
        }
    }

    /* BGRT (if precise dims are available in the loader ext, use them) */
    if (Ext->BgrtInfo.Valid)
    {
        BgrtX    = Ext->BgrtInfo.ImageOffsetX;
        BgrtY    = Ext->BgrtInfo.ImageOffsetY;
        BgrtAddr = Ext->BgrtInfo.ImageAddress;
        BgrtSize = Ext->BgrtInfo.ImageSize;

        /* If your loader provides width/height, set them here.
           Otherwise we do not preserve (leave W/H at 0). */
        BgrtW = 0;
        BgrtH = 0;

        BgrtValid = (BgrtW > 0 && BgrtH > 0);
        DPRINT1("[GOP] BGRT: off=(%u,%u) addr=0x%llx size=%u preserve=%d\n",
                BgrtX, BgrtY, (unsigned long long)BgrtAddr, BgrtSize, BgrtValid);
    }
    else
    {
        BgrtValid = FALSE;
    }

    return TRUE;
}

VOID
NTAPI
GopVidCleanUp(VOID)
{
    DPRINT1("[GOP] CleanUp\n");
    if (GopFbBase)
    {
        MmUnmapIoSpace(GopFbBase, GopFbSize);
        GopFbBase = NULL;
    }
}

VOID
NTAPI
GopVidResetDisplay(_In_ BOOLEAN HalReset)
{
    UNREFERENCED_PARAMETER(HalReset);
    DPRINT1("[GOP] ResetDisplay\n");

    if (!GopFbBase) return;

    if (!BgrtValid)
    {
        RtlZeroMemory(GopFbBase, GopPitch * GopHeight);
        return;
    }

    /* Clear everything except BGRT rect */
    for (ULONG y = 0; y < GopHeight; ++y)
    {
        PUCHAR row = GopFbBase + (SIZE_T)y * GopPitch;

        if (y < BgrtY || y >= (BgrtY + BgrtH))
        {
            RtlZeroMemory(row, (SIZE_T)GopPsl * GopBpp);
        }
        else
        {
            if (BgrtX > 0)
                RtlZeroMemory(row, (SIZE_T)BgrtX * GopBpp);

            ULONG after = BgrtX + BgrtW;
            if (after < GopWidth)
            {
                RtlZeroMemory(row + (SIZE_T)after * GopBpp,
                              ((SIZE_T)GopPsl - after) * GopBpp);
            }
        }
    }
}

/* Write a pixel (0x00RRGGBB) with clipping and BGRT preserve */
static inline VOID
WritePixel(_In_ ULONG x, _In_ ULONG y, _In_ ULONG rgb)
{
    PUCHAR p;
    ULONG packed;

    if (!GopFbBase) return;
    if (x >= GopWidth || y >= GopHeight) return;
    if (PointInBgrt(x, y)) return;

    p = GopFbBase + (SIZE_T)y * GopPitch + (SIZE_T)x * GopBpp;

    packed = PackRGB888((UCHAR)((rgb >> 16) & 0xFF),
                        (UCHAR)((rgb >> 8)  & 0xFF),
                        (UCHAR)( rgb        & 0xFF));

    switch (GopBpp)
    {
        case 2: *(PUSHORT)p = (USHORT)packed; break;
        case 3: p[0] = (UCHAR)( packed       & 0xFF);
                p[1] = (UCHAR)((packed >> 8) & 0xFF);
                p[2] = (UCHAR)((packed >>16) & 0xFF);
                break;
        case 4: *(PULONG)p = packed; break;
    }
}

VOID
NTAPI
GopVidSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR Color)
{
    static const ULONG pal16[16] = {
        0x000000,0x0000AA,0x00AA00,0x00AAAA,0xAA0000,0xAA00AA,0xAA5500,0xAAAAAA,
        0x555555,0x5555FF,0x55FF55,0x55FFFF,0xFF5555,0xFF55FF,0xFFFF55,0xFFFFFF
    };

    if (!GopFbBase) return;

    /* Normalize & clip */
    LONG l = (LONG)Left, t = (LONG)Top, r = (LONG)Right, b = (LONG)Bottom;
    if (RectEmpty(l,t,r,b)) return;

    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if ((ULONG)r >= GopWidth)  r = (LONG)GopWidth  - 1;
    if ((ULONG)b >= GopHeight) b = (LONG)GopHeight - 1;
    if (RectEmpty(l,t,r,b)) return;

    const ULONG rgb = pal16[Color & 0x0F];

    /* Fast row fill (only if 32bpp RGBX/BGRX and no BGRT preservation) */
    if ((GopFormat == 0 || GopFormat == 1) && GopBpp == 4 && !BgrtValid)
    {
        const ULONG packed = PackRGB888((UCHAR)((rgb >> 16) & 0xFF),
                                        (UCHAR)((rgb >> 8)  & 0xFF),
                                        (UCHAR)( rgb        & 0xFF));
        for (ULONG y = (ULONG)t; y <= (ULONG)b; ++y)
        {
            PULONG row = (PULONG)(GopFbBase + (SIZE_T)y * GopPitch + (SIZE_T)l * 4);
            SIZE_T npx = (SIZE_T)(r - l + 1);

            /* align to 8 bytes then stream 64b if useful */
            while (((ULONG_PTR)row & 7) && npx) { *row++ = packed; --npx; }

            PULONGLONG q = (PULONGLONG)row;
            const ULONGLONG vv = ((ULONGLONG)packed) | ((ULONGLONG)packed << 32);
            while (npx >= 2) { *q++ = vv; npx -= 2; }

            row = (PULONG)q;
            while (npx) { *row++ = packed; --npx; }
        }
        return;
    }

    /* Slow path: per-pixel (handles BGRT) */
    for (ULONG y = (ULONG)t; y <= (ULONG)b; ++y)
    {
        for (ULONG x = (ULONG)l; x <= (ULONG)r; ++x)
            WritePixel(x, y, rgb);
    }
}

VOID
NTAPI
GopVidBufferToScreenBlt(
    _In_reads_bytes_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    if (!GopFbBase || !Buffer) return;
    if (Width == 0 || Height == 0) return;
    if (Left >= GopWidth || Top >= GopHeight) return;

    if (Left + Width  > GopWidth)  Width  = GopWidth  - Left;
    if (Top  + Height > GopHeight) Height = GopHeight - Top;

    /* Determine source bpp (heuristic based on Delta) */
    ULONG srcBpp = 0;
    if (Delta >= Width * 4) srcBpp = 4;
    else if (Delta >= Width * 3) srcBpp = 3;
    else srcBpp = 1;

    /* Safe fast path: if src is 32bpp and FB is BGRX8 and we assume src is already BGRX8
       (common for INBV logo buffers). Only when not preserving BGRT. */
    if (!BgrtValid && srcBpp == 4 && GopBpp == 4 && GopFormat == 1)
    {
        for (ULONG y = 0; y < Height; ++y)
        {
            PUCHAR dst = GopFbBase + (SIZE_T)(Top + y) * GopPitch + (SIZE_T)Left * 4;
            PUCHAR src = Buffer + (SIZE_T)y * Delta;
            RtlCopyMemory(dst, src, (SIZE_T)Width * 4);
        }
        return;
    }

    /* Convert per-pixel */
    for (ULONG y = 0; y < Height; ++y)
    {
        PUCHAR src = Buffer + (SIZE_T)y * Delta;
        for (ULONG x = 0; x < Width; ++x)
        {
            ULONG rgb;
            if (srcBpp == 4)
            {
                /* Treat as 0x00RRGGBB in memory (common) */
                rgb = (*(PULONG)(src + (SIZE_T)x * 4)) & 0x00FFFFFF;
            }
            else if (srcBpp == 3)
            {
                rgb =  ((ULONG)src[x*3 + 0] << 16) |
                       ((ULONG)src[x*3 + 1] << 8)  |
                        (ULONG)src[x*3 + 2];
            }
            else /* 8bpp palette (CGA 16 subset) */
            {
                static const ULONG pal16[16] = {
                    0x000000,0x0000AA,0x00AA00,0x00AAAA,0xAA0000,0xAA00AA,0xAA5500,0xAAAAAA,
                    0x555555,0x5555FF,0x55FF55,0x55FFFF,0xFF5555,0xFF55FF,0xFFFF55,0xFFFFFF
                };
                rgb = pal16[src[x] & 0x0F];
            }
            WritePixel(Left + x, Top + y, rgb);
        }
    }
}

VOID
NTAPI
GopVidScreenToBufferBlt(
    _Out_writes_bytes_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    if (!GopFbBase || !Buffer) return;
    if (Width == 0 || Height == 0) return;
    if (Left >= GopWidth || Top >= GopHeight) return;

    if (Left + Width  > GopWidth)  Width  = GopWidth  - Left;
    if (Top  + Height > GopHeight) Height = GopHeight - Top;

    ULONG dstBpp = 0;
    if (Delta >= Width * 4) dstBpp = 4;
    else if (Delta >= Width * 3) dstBpp = 3;
    else dstBpp = 1;

    for (ULONG y = 0; y < Height; ++y)
    {
        PUCHAR dst = Buffer + (SIZE_T)y * Delta;
        for (ULONG x = 0; x < Width; ++x)
        {
            /* Load FB pixel and convert to 0x00RRGGBB */
            PUCHAR p = GopFbBase + (SIZE_T)(Top + y) * GopPitch + (SIZE_T)(Left + x) * GopBpp;
            ULONG px = 0;

            switch (GopBpp)
            {
                case 2: px = *(PUSHORT)p; break;
                case 3: px = (ULONG)p[0] | ((ULONG)p[1] << 8) | ((ULONG)p[2] << 16); break;
                case 4: px = *(PULONG)p; break;
            }

            const ULONG rgb = UnpackToRGB888(px);

            if (dstBpp == 4)
            {
                *(PULONG)(dst + (SIZE_T)x * 4) = rgb;
            }
            else if (dstBpp == 3)
            {
                dst[x*3 + 0] = (UCHAR)((rgb >> 16) & 0xFF);
                dst[x*3 + 1] = (UCHAR)((rgb >>  8) & 0xFF);
                dst[x*3 + 2] = (UCHAR)( rgb        & 0xFF);
            }
            else
            {
                /* crude quantization to 16-color CGA */
                UCHAR r = (UCHAR)((rgb >> 16) & 0xFF);
                UCHAR g = (UCHAR)((rgb >>  8) & 0xFF);
                UCHAR b = (UCHAR)( rgb        & 0xFF);
                UCHAR idx =
                    (r > 128 && g > 128 && b > 128) ? 15 :
                    (r > 128) ? 4 :
                    (g > 128) ? 2 :
                    (b > 128) ? 1 : 0;
                dst[x] = idx;
            }
        }
    }
}

VOID
NTAPI
GopVidDisplayString(_In_z_ PUCHAR String)
{
    /* TODO: Implement 8x16 font rendering to display text on GOP framebuffer.
       For now, we skip logging to avoid cluttering the debug output since these
       strings already appear in the regular boot log. */
    UNREFERENCED_PARAMETER(String);
}

VOID
NTAPI
GopVidBitBlt(_In_ PUCHAR Buffer, _In_ ULONG Left, _In_ ULONG Top)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Left);
    UNREFERENCED_PARAMETER(Top);
    /* If your BitBlt format is known, you can decode here and call GopVidBufferToScreenBlt. */
    DPRINT1("[GOP] BitBlt at (%u,%u) [stub]\n", Left, Top);
}
