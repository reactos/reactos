#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct tagBITMAPINFOHEADER
{
    ULONG  biSize;
    LONG   biWidth;
    LONG   biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG  biCompression;
    ULONG  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    ULONG  biClrUsed;
    ULONG  biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

static PHYSICAL_ADDRESS GopFbPhys = {{0}};
static PUCHAR           GopFbBase = NULL;
static SIZE_T           GopFbSize = 0;

static ULONG GopWidth  = 0;
static ULONG GopHeight = 0;
static ULONG GopPsl    = 0;
static ULONG GopPitch  = 0;

static ULONG GopFormat = 0; /* 0=RGBX8, 1=BGRX8, 2=BitMask */
static ULONG GopBpp    = 4; /* bytes per pixel: 2 or 4 (never 3) */

static ULONG RMShift=0, GMShift=0, BMShift=0;
static ULONG RWidth=0,  GWidth=0,  BWidth=0;
static ULONG RMask=0,   GMask=0,   BMask=0;

static BOOLEAN   BgrtValid = FALSE;
static ULONG     BgrtX = 0, BgrtY = 0, BgrtW = 0, BgrtH = 0;
static ULONGLONG BgrtAddr = 0;
static ULONG     BgrtSize = 0;

static __inline VOID ComputeMaskInfo(ULONG Mask, PULONG Shift, PULONG Width)
{
    ULONG s = 0, w = 0;
    if (Mask)
    {
        while (((Mask >> s) & 1) == 0 && s < 32) s++;
        ULONG m = Mask >> s;
        while ((m & 1) && w < 32) { w++; m >>= 1; }
    }
    *Shift = s;
    *Width = w;
}

static __inline ULONG Scale8ToMask(UCHAR v8, ULONG width)
{
    if (!width) return 0;
    const ULONG maxv = (1u << width) - 1u;
    return (ULONG)((v8 * maxv + 127u) / 255u);
}

static __inline UCHAR ScaleMaskTo8(ULONG v, ULONG width)
{
    if (!width) return 0;
    const ULONG maxv = (1u << width) - 1u;
    return (UCHAR)((v * 255u + (maxv >> 1)) / maxv);
}

static __inline ULONG PackRGB888(UCHAR r, UCHAR g, UCHAR b)
{
    switch (GopFormat)
    {
        case 0: return ((ULONG)r) | ((ULONG)g << 8) | ((ULONG)b << 16);  /* RGBX: mem R,G, B, X */
        case 1: return ((ULONG)b) | ((ULONG)g << 8) | ((ULONG)r << 16);  /* BGRX: mem B,G, R, X */
        case 2:
        {
            ULONG out = 0;
            if (RWidth) out |= (Scale8ToMask(r, RWidth) << RMShift) & RMask;
            if (GWidth) out |= (Scale8ToMask(g, GWidth) << GMShift) & GMask;
            if (BWidth) out |= (Scale8ToMask(b, BWidth) << BMShift) & BMask;
            return out;
        }
        default: return 0;
    }
}

static __inline ULONG UnpackToRGB888(ULONG px)
{
    UCHAR r=0,g=0,b=0;
    switch (GopFormat)
    {
        case 0:
            r = (UCHAR)( px        & 0xFF);
            g = (UCHAR)((px >> 8)  & 0xFF);
            b = (UCHAR)((px >> 16) & 0xFF);
            break;
        case 1:
            b = (UCHAR)( px        & 0xFF);
            g = (UCHAR)((px >> 8)  & 0xFF);
            r = (UCHAR)((px >> 16) & 0xFF);
            break;
        case 2:
        {
            ULONG rv = (px & RMask) >> RMShift;
            ULONG gv = (px & GMask) >> GMShift;
            ULONG bv = (px & BMask) >> BMShift;
            r = ScaleMaskTo8(rv, RWidth);
            g = ScaleMaskTo8(gv, GWidth);
            b = ScaleMaskTo8(bv, BWidth);
            break;
        }
        default: break;
    }
    return ((ULONG)r << 16) | ((ULONG)g << 8) | (ULONG)b;
}

static __inline BOOLEAN RectEmpty(LONG l, LONG t, LONG r, LONG b)
{
    return (r < l) || (b < t);
}

static __inline BOOLEAN PointInBgrt(ULONG x, ULONG y)
{
    if (!BgrtValid || !BgrtW || !BgrtH) return FALSE;
    return (x >= BgrtX && x < (BgrtX + BgrtW) && y >= BgrtY && y < (BgrtY + BgrtH));
}

static __inline VOID WritePixel(ULONG x, ULONG y, ULONG rgb)
{
    if (!GopFbBase) return;
    if (x >= GopWidth || y >= GopHeight) return;
    if (PointInBgrt(x, y)) return;
    SIZE_T offset = (SIZE_T)y * GopPitch + (SIZE_T)x * GopBpp;
    if (offset + GopBpp > GopFbSize) return;
    PUCHAR p = GopFbBase + offset;
    ULONG packed = PackRGB888((UCHAR)((rgb >> 16) & 0xFF),
                              (UCHAR)((rgb >> 8)  & 0xFF),
                              (UCHAR)( rgb        & 0xFF));
    if (GopBpp == 4)      *(PULONG)p  = packed;
    else if (GopBpp == 2) *(PUSHORT)p = (USHORT)packed;
    else { p[0]=(UCHAR)(packed & 0xFF); p[1]=(UCHAR)((packed>>8)&0xFF); p[2]=(UCHAR)((packed>>16)&0xFF); }
}

BOOLEAN NTAPI GopVidInitialize(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) return FALSE;
    if (!LoaderBlock || !LoaderBlock->Extension) return FALSE;

    PLOADER_PARAMETER_EXTENSION Ext = LoaderBlock->Extension;
    if (Ext->GopFramebuffer.FrameBufferBase.QuadPart == 0 || Ext->GopFramebuffer.FrameBufferSize == 0) return FALSE;

    GopWidth  = Ext->GopFramebuffer.HorizontalResolution;
    GopHeight = Ext->GopFramebuffer.VerticalResolution;
    GopPsl    = Ext->GopFramebuffer.PixelsPerScanLine;
    GopFormat = Ext->GopFramebuffer.PixelFormat;
    RMask     = Ext->GopFramebuffer.RedMask;
    GMask     = Ext->GopFramebuffer.GreenMask;
    BMask     = Ext->GopFramebuffer.BlueMask;

    SIZE_T denom = (SIZE_T)GopPsl * (SIZE_T)GopHeight;
    ULONG bpp_from_size = (denom && (Ext->GopFramebuffer.FrameBufferSize % denom) == 0)
                          ? (ULONG)(Ext->GopFramebuffer.FrameBufferSize / denom)
                          : 0;

    switch (GopFormat)
    {
        case 0:
        case 1:
            GopBpp = 4;
            break;
        case 2:
        {
            if (bpp_from_size == 2 || bpp_from_size == 4) GopBpp = bpp_from_size;
            else
            {
                ULONG masks = RMask | GMask | BMask;
                GopBpp = (masks <= 0xFFFF) ? 2 : 4;
            }
            ComputeMaskInfo(RMask, &RMShift, &RWidth);
            ComputeMaskInfo(GMask, &GMShift, &GWidth);
            ComputeMaskInfo(BMask, &BMShift, &BWidth);
            break;
        }
        default:
            return FALSE;
    }

    if (GopBpp == 3) GopBpp = 4;

    GopPitch  = GopPsl * GopBpp;
    GopFbSize = (SIZE_T)Ext->GopFramebuffer.FrameBufferSize;
    GopFbPhys = Ext->GopFramebuffer.FrameBufferBase;

    GopFbBase = MmMapIoSpace(GopFbPhys, GopFbSize, MmWriteCombined);
    if (!GopFbBase) GopFbBase = MmMapIoSpace(GopFbPhys, GopFbSize, MmNonCached);
    if (!GopFbBase) return FALSE;

    SIZE_T total = (SIZE_T)GopPitch * (SIZE_T)GopHeight;
    if (total <= GopFbSize) RtlZeroMemory(GopFbBase, total); else RtlZeroMemory(GopFbBase, GopFbSize);

    if (Ext->BgrtInfo.Valid)
    {
        BgrtX    = Ext->BgrtInfo.ImageOffsetX;
        BgrtY    = Ext->BgrtInfo.ImageOffsetY;
        BgrtAddr = Ext->BgrtInfo.ImageAddress;
        BgrtSize = Ext->BgrtInfo.ImageSize;
        BgrtW = 0; BgrtH = 0;

#pragma pack(push,1)
        typedef struct { USHORT bfType; ULONG bfSize; USHORT r1; USHORT r2; ULONG bfOffBits; } BMPFILEHEADER;
#pragma pack(pop)
        if (BgrtAddr && BgrtSize >= (sizeof(BMPFILEHEADER) + sizeof(BITMAPINFOHEADER)))
        {
            PUCHAR p = (PUCHAR)(ULONG_PTR)BgrtAddr;
            BMPFILEHEADER* bfh = (BMPFILEHEADER*)p;
            BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(p + sizeof(BMPFILEHEADER));
            if (bfh->bfType == 0x4D42 && bih->biWidth > 0 && bih->biHeight != 0)
            {
                BgrtW = (ULONG)bih->biWidth;
                BgrtH = (ULONG)((bih->biHeight < 0) ? -bih->biHeight : bih->biHeight);
            }
        }
        BgrtValid = (BgrtW && BgrtH);
    }
    else BgrtValid = FALSE;

    DPRINT1("[GOP] %ux%u PSL=%u Bpp=%u Fmt=%lu\n", GopWidth, GopHeight, GopPsl, GopBpp, GopFormat);
    return TRUE;
}

VOID NTAPI GopVidCleanUp(VOID)
{
    if (GopFbBase) { MmUnmapIoSpace(GopFbBase, GopFbSize); GopFbBase = NULL; }
}

VOID NTAPI GopVidResetDisplay(BOOLEAN HalReset)
{
    UNREFERENCED_PARAMETER(HalReset);
    if (!GopFbBase) return;

    if (!BgrtValid)
    {
        SIZE_T total = (SIZE_T)GopPitch * (SIZE_T)GopHeight;
        if (total <= GopFbSize) RtlZeroMemory(GopFbBase, total); else RtlZeroMemory(GopFbBase, GopFbSize);
        return;
    }

    for (ULONG y = 0; y < GopHeight; ++y)
    {
        PUCHAR row = GopFbBase + (SIZE_T)y * GopPitch;
        if (y < BgrtY || y >= (BgrtY + BgrtH))
        {
            RtlZeroMemory(row, (SIZE_T)GopPsl * GopBpp);
        }
        else
        {
            if (BgrtX > 0) RtlZeroMemory(row, (SIZE_T)BgrtX * GopBpp);
            ULONG after = BgrtX + BgrtW;
            if (after < GopWidth)
            {
                RtlZeroMemory(row + (SIZE_T)after * GopBpp,
                              ((SIZE_T)GopPsl - after) * GopBpp);
            }
        }
    }
}

VOID NTAPI GopVidSolidColorFill(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR Color)
{
    static const ULONG pal16[16] = {
        0x000000,0x0000AA,0x00AA00,0x00AAAA,0xAA0000,0xAA00AA,0xAA5500,0xAAAAAA,
        0x555555,0x5555FF,0x55FF55,0x55FFFF,0xFF5555,0xFF55FF,0xFFFF55,0xFFFFFF
    };
    if (!GopFbBase) return;

    LONG l = (LONG)Left, t = (LONG)Top, r = (LONG)Right, b = (LONG)Bottom;
    if (RectEmpty(l,t,r,b)) return;

    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if ((ULONG)r >= GopWidth)  r = (LONG)GopWidth  - 1;
    if ((ULONG)b >= GopHeight) b = (LONG)GopHeight - 1;
    if (RectEmpty(l,t,r,b)) return;

    const ULONG rgb = pal16[Color & 0x0F];

    if (!BgrtValid && GopBpp == 4 && (ULONG)l <= (ULONG)r)
    {
        const ULONG packed = PackRGB888((UCHAR)((rgb >> 16) & 0xFF),
                                        (UCHAR)((rgb >> 8)  & 0xFF),
                                        (UCHAR)( rgb        & 0xFF));
        SIZE_T npx = (SIZE_T)(r - l + 1);
        for (ULONG y = (ULONG)t; y <= (ULONG)b; ++y)
        {
            PULONG row = (PULONG)(GopFbBase + (SIZE_T)y * GopPitch + (SIZE_T)l * 4);
            SIZE_T bytes = npx * 4;
            RtlFillMemoryUlong(row, (ULONG)bytes, packed);
        }
        return;
    }

    for (ULONG y = (ULONG)t; y <= (ULONG)b; ++y)
        for (ULONG x = (ULONG)l; x <= (ULONG)r; ++x)
            WritePixel(x, y, rgb);
}

VOID NTAPI GopVidBufferToScreenBlt(PUCHAR Buffer, ULONG Left, ULONG Top, ULONG Width, ULONG Height, ULONG Delta)
{
    if (!GopFbBase || !Buffer) return;
    if (!Width || !Height) return;
    if (Left >= GopWidth || Top >= GopHeight) return;

    if (Left + Width  > GopWidth)  Width  = GopWidth  - Left;
    if (Top  + Height > GopHeight) Height = GopHeight - Top;

    ULONG srcBpp = 0;
    if (Delta >= Width * 4) srcBpp = 4;
    else if (Delta >= Width * 3) srcBpp = 3;
    else srcBpp = 1;

    for (ULONG y = 0; y < Height; ++y)
    {
        PUCHAR src = Buffer + (SIZE_T)y * Delta;
        for (ULONG x = 0; x < Width; ++x)
        {
            ULONG rgb;
            if (srcBpp == 4)
            {
                ULONG spx = *(PULONG)(src + (SIZE_T)x * 4); /* BGRA/BGRX in memory */
                UCHAR b = (UCHAR)(spx & 0xFF);
                UCHAR g = (UCHAR)((spx >> 8) & 0xFF);
                UCHAR r = (UCHAR)((spx >> 16) & 0xFF);
                rgb = ((ULONG)r << 16) | ((ULONG)g << 8) | (ULONG)b;
            }
            else if (srcBpp == 3)
            {
                rgb = ((ULONG)src[x*3 + 2] << 16) |
                      ((ULONG)src[x*3 + 1] << 8)  |
                       (ULONG)src[x*3 + 0];
            }
            else
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

VOID NTAPI GopVidScreenToBufferBlt(PUCHAR Buffer, ULONG Left, ULONG Top, ULONG Width, ULONG Height, ULONG Delta)
{
    if (!GopFbBase || !Buffer) return;
    if (!Width || !Height) return;
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
            PUCHAR p = GopFbBase + (SIZE_T)(Top + y) * GopPitch + (SIZE_T)(Left + x) * GopBpp;
            ULONG px = 0;
            if (GopBpp == 4) px = *(PULONG)p;
            else if (GopBpp == 2) px = *(PUSHORT)p;
            else px = (ULONG)p[0] | ((ULONG)p[1] << 8) | ((ULONG)p[2] << 16);

            const ULONG rgb = UnpackToRGB888(px);

            if (dstBpp == 4)
            {
                *(PULONG)(dst + (SIZE_T)x * 4) = rgb; /* 0x00RRGGBB */
            }
            else if (dstBpp == 3)
            {
                dst[x*3 + 0] = (UCHAR)( rgb        & 0xFF);
                dst[x*3 + 1] = (UCHAR)((rgb >>  8) & 0xFF);
                dst[x*3 + 2] = (UCHAR)((rgb >> 16) & 0xFF);
            }
            else
            {
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

VOID NTAPI GopVidDisplayString(PUCHAR String)
{
    while (*String)
    {
        if (*String == '\r' || *String == '\n')
        {
            DbgPrint("\n");
            if (*String == '\r' && *(String + 1) == '\n') String++;
        }
        else DbgPrint("%c", *String);
        String++;
    }
}

VOID NTAPI GopVidBitBlt(PUCHAR Buffer, ULONG Left, ULONG Top)
{
    if (!Buffer || !GopFbBase) return;

    PBITMAPINFOHEADER bih = (PBITMAPINFOHEADER)Buffer;
    if (bih->biSize < sizeof(BITMAPINFOHEADER)) return;

    ULONG Width = (ULONG)bih->biWidth;
    ULONG Height = (ULONG)((bih->biHeight < 0) ? -bih->biHeight : bih->biHeight);
    ULONG BitCount = bih->biBitCount;
    ULONG Compression = bih->biCompression;

    ULONG PaletteCount = bih->biClrUsed ? bih->biClrUsed : ((BitCount <= 8) ? (1u << BitCount) : 0u);
    if (BitCount == 4 && PaletteCount < 16) PaletteCount = 16;
    if (PaletteCount > 256) PaletteCount = 256;

    PUCHAR afterHeader = Buffer + bih->biSize;
    PUCHAR paletteEndCandidate = afterHeader + (SIZE_T)PaletteCount * sizeof(ULONG);
    if (paletteEndCandidate < afterHeader) return; /* overflow guard */

    PULONG Palette = (PULONG)afterHeader;

    static ULONG VgaPalette[16];
    for (ULONG i = 0; i < PaletteCount && i < 16; i++)
    {
        ULONG bgra = Palette[i];
        UCHAR b = (UCHAR)(bgra & 0xFF);
        UCHAR g = (UCHAR)((bgra >> 8) & 0xFF);
        UCHAR r = (UCHAR)((bgra >> 16) & 0xFF);
        VgaPalette[i] = ((ULONG)r << 16) | ((ULONG)g << 8) | (ULONG)b;
    }

    LONG Delta = ((BitCount * Width + 31) / 32) * 4;
    PUCHAR DataStart = Buffer + sizeof(BITMAPINFOHEADER) + (SIZE_T)PaletteCount * sizeof(ULONG);

    SIZE_T expected;
    if (bih->biSizeImage) expected = (SIZE_T)bih->biSizeImage;
    else expected = (SIZE_T)((((BitCount * Width + 31) / 32) * 4) * Height);

    SIZE_T maxReasonable =
        (BitCount == 4) ? (SIZE_T)Delta * Height :
        (SIZE_T)Delta * Height;

    SIZE_T dataSize = (expected <= maxReasonable) ? expected : maxReasonable;
    PUCHAR DataEnd = DataStart + dataSize;
    if (DataEnd < DataStart) return; /* overflow guard */

    BOOLEAN IsTopDown = (bih->biHeight < 0);

    if (Compression == 2 && BitCount == 4)
    {
        ULONG CurrentY = IsTopDown ? Top : (Top + Height - 1);
        ULONG CurrentX = Left;
        LONG YInc = IsTopDown ? 1 : -1;
        PUCHAR BitmapOffset = DataStart;

        while (BitmapOffset < DataEnd)
        {
            if ((SIZE_T)(DataEnd - BitmapOffset) < 1) break;
            ULONG RleValue = *BitmapOffset++;
            if (RleValue)
            {
                if ((SIZE_T)(DataEnd - BitmapOffset) < 1) break;
                ULONG NewRleValue = *BitmapOffset++;
                ULONG Color1 = (NewRleValue >> 4) & 0x0F;
                ULONG Color2 = NewRleValue & 0x0F;
                for (ULONG i = 0; i < RleValue; i++)
                {
                    ULONG Color = (i & 1) ? Color2 : Color1;
                    if (CurrentX < GopWidth && CurrentY < GopHeight &&
                        CurrentX >= Left && CurrentX < (Left + Width) &&
                        CurrentY >= Top && CurrentY < (Top + Height))
                    {
                        WritePixel(CurrentX, CurrentY, VgaPalette[Color & 0x0F]);
                    }
                    CurrentX++;
                }
            }
            else
            {
                if ((SIZE_T)(DataEnd - BitmapOffset) < 1) break;
                ULONG Esc = *BitmapOffset++;
                if (Esc == 0)
                {
                    CurrentY += YInc;
                    CurrentX = Left;
                }
                else if (Esc == 1)
                {
                    break;
                }
                else if (Esc == 2)
                {
                    if ((SIZE_T)(DataEnd - BitmapOffset) < 2) break;
                    CurrentX += *BitmapOffset++;
                    CurrentY += ((ULONG)(*BitmapOffset++)) * YInc;
                }
                else
                {
                    ULONG j = Esc;
                    for (ULONG i = 0; i < j; i++)
                    {
                        if (BitmapOffset >= DataEnd) break;
                        ULONG Code;
                        if ((i & 1) == 0)
                        {
                            Code = (*BitmapOffset >> 4) & 0x0F;
                        }
                        else
                        {
                            Code = *BitmapOffset & 0x0F;
                            BitmapOffset++;
                        }
                        if (CurrentX < GopWidth && CurrentY < GopHeight &&
                            CurrentX >= Left && CurrentX < (Left + Width) &&
                            CurrentY >= Top && CurrentY < (Top + Height))
                        {
                            WritePixel(CurrentX, CurrentY, VgaPalette[Code & 0x0F]);
                        }
                        CurrentX++;
                    }
                    if ((j & 1) && BitmapOffset < DataEnd) BitmapOffset++;
                    if (((ULONG_PTR)BitmapOffset & 1) && BitmapOffset < DataEnd) BitmapOffset++;
                }
            }
        }
        return;
    }
    else if (BitCount == 4 && Compression == 0)
    {
        for (ULONG y = 0; y < Height; y++)
        {
            PUCHAR rowBase = IsTopDown
                ? (DataStart + (SIZE_T)y * Delta)
                : (DataStart + (SIZE_T)(Height - 1 - y) * Delta);

            PUCHAR InputBuffer = rowBase;
            UCHAR Colors = 0;
            for (ULONG x = 0; x < Width; x++)
            {
                ULONG Color;
                if ((x & 1) == 0)
                {
                    if (InputBuffer >= DataEnd) return;
                    Colors = *InputBuffer;
                    Color = VgaPalette[(Colors >> 4) & 0x0F];
                }
                else
                {
                    Color = VgaPalette[Colors & 0x0F];
                    InputBuffer++;
                }
                if ((Left + x) < GopWidth && (Top + y) < GopHeight)
                    WritePixel(Left + x, Top + y, Color);
            }
        }
        return;
    }
}
