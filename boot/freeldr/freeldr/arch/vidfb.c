/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for linear framebuffers
 * COPYRIGHT:   Authors of uefivid.c and xboxvideo.c
 *              Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <freeldr.h>
#include "vidfb.h"
#include "vgafont.h"

#ifdef UEFIBOOT
#include "uefi_fb_font.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

/* This is used to introduce artificial symmetric borders at the top and bottom */
#define TOP_BOTTOM_LINES 0

#define FBCONS_FONT_MIN_PIXELS         16
#define FBCONS_FONT_MAX_PIXELS         32
#define FBCONS_FONT_TARGET_ROWS        38
#define FBCONS_FONT_MIN_COLUMNS        80
#define FBCONS_FONT_MIN_ROWS           25
#define FBCONS_FONT_X_PADDING          0
#define FBCONS_FONT_Y_PADDING          0
#define FBCONS_MAX_GLYPH_BITMAP_SIZE   4096


/* GLOBALS ********************************************************************/

typedef struct _FRAMEBUFFER_INFO
{
    ULONG_PTR BaseAddress;
    ULONG BufferSize;

    /* Horizontal and Vertical resolution in pixels */
    ULONG ScreenWidth;
    ULONG ScreenHeight;

    /* Number of pixel elements per video memory line */
    ULONG PixelsPerScanLine; // aka. "Pitch" or "ScreenStride", but Stride is in bytes or bits...
    ULONG BitsPerPixel;      // aka. "PixelStride".

    /* Physical format of the pixel for BPP > 8, specified by bit-mask */
    PIXEL_BITMASK PixelMasks;

/** Calculated values */

    ULONG BytesPerPixel;
    ULONG Delta;             // aka. "Pitch": actual size in bytes of a scanline.

    UCHAR RedShift;
    UCHAR GreenShift;
    UCHAR BlueShift;
    UCHAR ReservedShift;
    UCHAR RedBits;
    UCHAR GreenBits;
    UCHAR BlueBits;
    UCHAR ReservedBits;
} FRAMEBUFFER_INFO, *PFRAMEBUFFER_INFO;

typedef struct _FBCONS_FONT_STATE
{
    BOOLEAN Enabled;
#ifdef UEFIBOOT
    FT_Library Library;
    FT_Face Face;
    const UCHAR* CmapTable;
    const UCHAR* CmapLimit;
    USHORT CmapFormat;
#endif
    ULONG PixelHeight;
    ULONG CellWidth;
    ULONG CellHeight;
    ULONG Baseline;
} FBCONS_FONT_STATE, *PFBCONS_FONT_STATE;

static FRAMEBUFFER_INFO framebufInfo = {0};
static CM_FRAMEBUF_DEVICE_DATA FrameBufferData = {0};
static FBCONS_FONT_STATE FbConsFont = {0};

#ifdef UEFIBOOT
typedef struct _FBCONS_GLYPH
{
    BOOLEAN Valid;
    SHORT Left;
    SHORT Top;
    USHORT Width;
    USHORT Height;
    USHORT Pitch;
} FBCONS_GLYPH, *PFBCONS_GLYPH;

static FBCONS_GLYPH FbConsGlyphs[256] = {0};
static UCHAR FbConsGlyphBitmap[256][FBCONS_MAX_GLYPH_BITMAP_SIZE] = {{0}};

static const WCHAR FbConsCp437ControlMap[32] =
{
    0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

static const WCHAR FbConsCp437UpperHalfMap[128] =
{
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};
#endif


/* FUNCTIONS ******************************************************************/

#ifdef UEFIBOOT
static
__inline
WCHAR
FbConsCp437ToUnicode(
    _In_ UCHAR Char)
{
    if (Char < RTL_NUMBER_OF(FbConsCp437ControlMap))
        return FbConsCp437ControlMap[Char];

    if (Char < 0x80)
        return (Char == 0x7F) ? 0x2302 : Char;

    return FbConsCp437UpperHalfMap[Char - 0x80];
}

static
__inline
USHORT
FbConsReadUInt16BE(
    _In_reads_(2) const UCHAR* Value)
{
    return (USHORT)((Value[0] << 8) | Value[1]);
}

static
__inline
SHORT
FbConsReadInt16BE(
    _In_reads_(2) const UCHAR* Value)
{
    return (SHORT)FbConsReadUInt16BE(Value);
}

static
__inline
ULONG
FbConsReadUInt32BE(
    _In_reads_(4) const UCHAR* Value)
{
    return ((ULONG)Value[0] << 24) |
           ((ULONG)Value[1] << 16) |
           ((ULONG)Value[2] << 8)  |
           (ULONG)Value[3];
}

static
BOOLEAN
FbConsFindSfntTable(
    _In_reads_bytes_(FontSize) const UCHAR* FontData,
    _In_ ULONG FontSize,
    _In_ ULONG Tag,
    _Outptr_result_bytebuffer_(*TableSize) const UCHAR** TableData,
    _Out_ PULONG TableSize)
{
    ULONG NumTables;
    ULONG Table;

    if (!FontData || (FontSize < 12))
        return FALSE;

    NumTables = FbConsReadUInt16BE(FontData + 4);
    if (FontSize < 12 + NumTables * 16)
        return FALSE;

    for (Table = 0; Table < NumTables; ++Table)
    {
        const UCHAR* Record = FontData + 12 + Table * 16;
        ULONG TableOffset;
        ULONG Length;

        if (FbConsReadUInt32BE(Record) != Tag)
            continue;

        TableOffset = FbConsReadUInt32BE(Record + 8);
        Length = FbConsReadUInt32BE(Record + 12);

        if ((TableOffset > FontSize) || (Length > FontSize - TableOffset))
            return FALSE;

        *TableData = FontData + TableOffset;
        *TableSize = Length;
        return TRUE;
    }

    return FALSE;
}

static
ULONG
FbConsScoreCmapSubtable(
    _In_ USHORT PlatformId,
    _In_ USHORT EncodingId,
    _In_ USHORT Format)
{
    if (Format == 12)
    {
        if ((PlatformId == 3) && (EncodingId == 10))
            return 400;

        if (PlatformId == 0)
            return 300;

        if ((PlatformId == 3) && (EncodingId == 1))
            return 200;
    }

    if (Format == 4)
    {
        if ((PlatformId == 3) && (EncodingId == 1))
            return 190;

        if (PlatformId == 0)
            return 180;
    }

    return 0;
}

static
BOOLEAN
FbConsInitializeFontCmap(VOID)
{
    const UCHAR* CmapTable;
    const UCHAR* CmapLimit;
    ULONG CmapSize;
    USHORT NumSubtables;
    ULONG Index;
    ULONG BestScore = 0;

    FbConsFont.CmapTable = NULL;
    FbConsFont.CmapLimit = NULL;
    FbConsFont.CmapFormat = 0;

    if (!FbConsFindSfntTable(uefi_fb_font_data,
                             uefi_fb_font_data_SIZE,
                             0x636D6170UL, /* 'cmap' */
                             &CmapTable,
                             &CmapSize))
    {
        return FALSE;
    }

    if (CmapSize < 4)
        return FALSE;

    NumSubtables = FbConsReadUInt16BE(CmapTable + 2);
    if (CmapSize < 4 + NumSubtables * 8)
        return FALSE;

    CmapLimit = CmapTable + CmapSize;

    for (Index = 0; Index < NumSubtables; ++Index)
    {
        const UCHAR* EncodingRecord = CmapTable + 4 + Index * 8;
        const UCHAR* Subtable;
        USHORT PlatformId = FbConsReadUInt16BE(EncodingRecord);
        USHORT EncodingId = FbConsReadUInt16BE(EncodingRecord + 2);
        ULONG SubtableOffset = FbConsReadUInt32BE(EncodingRecord + 4);
        USHORT Format;
        ULONG SubtableLength;
        ULONG Score;

        if ((SubtableOffset > CmapSize) || (CmapSize - SubtableOffset < 2))
            continue;

        Subtable = CmapTable + SubtableOffset;
        Format = FbConsReadUInt16BE(Subtable);

        if (Format == 12)
        {
            if (CmapLimit - Subtable < 16)
                continue;

            SubtableLength = FbConsReadUInt32BE(Subtable + 4);
            if ((SubtableLength < 16) || (SubtableLength > (ULONG)(CmapLimit - Subtable)))
                continue;
        }
        else if (Format == 4)
        {
            if (CmapLimit - Subtable < 8)
                continue;

            SubtableLength = FbConsReadUInt16BE(Subtable + 2);
            if ((SubtableLength < 16) || (SubtableLength > (ULONG)(CmapLimit - Subtable)))
                continue;
        }
        else
        {
            continue;
        }

        Score = FbConsScoreCmapSubtable(PlatformId, EncodingId, Format);
        if (Score <= BestScore)
            continue;

        BestScore = Score;
        FbConsFont.CmapTable = Subtable;
        FbConsFont.CmapLimit = Subtable + SubtableLength;
        FbConsFont.CmapFormat = Format;
    }

    return (FbConsFont.CmapTable != NULL);
}

static
FT_UInt
FbConsMapUnicodeToGlyphIndexFormat12(
    _In_ ULONG Unicode)
{
    const UCHAR* Table = FbConsFont.CmapTable;
    ULONG NumGroups;
    ULONG MinIndex;
    ULONG MaxIndex;

    if (!Table || (FbConsFont.CmapFormat != 12))
        return 0;

    NumGroups = FbConsReadUInt32BE(Table + 12);
    if ((FbConsFont.CmapLimit - Table) < 16 + NumGroups * 12)
        return 0;

    MinIndex = 0;
    MaxIndex = NumGroups;
    while (MinIndex < MaxIndex)
    {
        ULONG MidIndex = (MinIndex + MaxIndex) / 2;
        const UCHAR* Group = Table + 16 + MidIndex * 12;
        ULONG StartCharCode = FbConsReadUInt32BE(Group);
        ULONG EndCharCode = FbConsReadUInt32BE(Group + 4);

        if (Unicode < StartCharCode)
        {
            MaxIndex = MidIndex;
        }
        else if (Unicode > EndCharCode)
        {
            MinIndex = MidIndex + 1;
        }
        else
        {
            ULONG StartGlyphId = FbConsReadUInt32BE(Group + 8);
            return (FT_UInt)(StartGlyphId + Unicode - StartCharCode);
        }
    }

    return 0;
}

static
FT_UInt
FbConsMapUnicodeToGlyphIndexFormat4(
    _In_ ULONG Unicode)
{
    const UCHAR* Table = FbConsFont.CmapTable;
    const UCHAR* TableLimit = FbConsFont.CmapLimit;
    USHORT SegCount;
    const UCHAR* EndCodes;
    const UCHAR* StartCodes;
    const UCHAR* IdDeltas;
    const UCHAR* IdRangeOffsets;
    USHORT Segment;

    if (!Table || (FbConsFont.CmapFormat != 4) || (Unicode > 0xFFFF))
        return 0;

    if (TableLimit - Table < 16)
        return 0;

    SegCount = FbConsReadUInt16BE(Table + 6) / 2;
    if ((SegCount == 0) || (TableLimit - Table < 16 + SegCount * 8))
        return 0;

    EndCodes = Table + 14;
    StartCodes = EndCodes + SegCount * 2 + 2;
    IdDeltas = StartCodes + SegCount * 2;
    IdRangeOffsets = IdDeltas + SegCount * 2;

    if (IdRangeOffsets + SegCount * 2 > TableLimit)
        return 0;

    for (Segment = 0; Segment < SegCount; ++Segment)
    {
        USHORT EndCharCode = FbConsReadUInt16BE(EndCodes + Segment * 2);
        USHORT StartCharCode = FbConsReadUInt16BE(StartCodes + Segment * 2);

        if ((Unicode < StartCharCode) || (Unicode > EndCharCode))
            continue;

        {
            SHORT IdDelta = FbConsReadInt16BE(IdDeltas + Segment * 2);
            USHORT IdRangeOffset = FbConsReadUInt16BE(IdRangeOffsets + Segment * 2);

            if (IdRangeOffset == 0)
                return (FT_UInt)((Unicode + IdDelta) & 0xFFFF);

            {
                const UCHAR* GlyphOffsetWord = IdRangeOffsets + Segment * 2;
                const UCHAR* GlyphEntry = GlyphOffsetWord + IdRangeOffset +
                                          (Unicode - StartCharCode) * 2;

                if ((GlyphEntry < Table) || (GlyphEntry + 2 > TableLimit))
                    return 0;

                {
                    USHORT GlyphIndex = FbConsReadUInt16BE(GlyphEntry);

                    if (GlyphIndex == 0)
                        return 0;

                    return (FT_UInt)((GlyphIndex + IdDelta) & 0xFFFF);
                }
            }
        }
    }

    return 0;
}

static
FT_UInt
FbConsMapUnicodeToGlyphIndex(
    _In_ ULONG Unicode)
{
    switch (FbConsFont.CmapFormat)
    {
        case 12:
            return FbConsMapUnicodeToGlyphIndexFormat12(Unicode);

        case 4:
            return FbConsMapUnicodeToGlyphIndexFormat4(Unicode);

        default:
            return 0;
    }
}

static
BOOLEAN
FbConsLoadGlyph(
    _In_ ULONG Unicode,
    _In_ FT_Int32 LoadFlags)
{
    FT_UInt GlyphIndex = FbConsMapUnicodeToGlyphIndex(Unicode);

    if ((GlyphIndex == 0) && (Unicode != 0))
        return FALSE;

    return (FT_Load_Glyph(FbConsFont.Face,
                          GlyphIndex,
                          LoadFlags | FT_LOAD_NO_AUTOHINT) == FT_Err_Ok);
}

static
__inline
UCHAR
VidFbMaskShift(
    _In_ ULONG Mask)
{
    UCHAR Shift = 0;

    if (!Mask)
        return 0;

    while ((Mask & 1) == 0)
    {
        ++Shift;
        Mask >>= 1;
    }

    return Shift;
}

static
__inline
ULONG
VidFbScaleColorComponentToMask(
    _In_ UCHAR Component,
    _In_ UCHAR Bits)
{
    ULONG MaxValue;

    if (Bits == 0)
        return 0;

    MaxValue = (1UL << Bits) - 1;
    return (Component * MaxValue + 127) / 255;
}

static
__inline
UCHAR
VidFbScaleColorComponentFromMask(
    _In_ ULONG Component,
    _In_ UCHAR Bits)
{
    ULONG MaxValue;

    if (Bits == 0)
        return 0;

    MaxValue = (1UL << Bits) - 1;
    return (UCHAR)((Component * 255 + MaxValue / 2) / MaxValue);
}

static
__inline
UINT32
VidFbComposePixel(
    _In_ UCHAR Red,
    _In_ UCHAR Green,
    _In_ UCHAR Blue)
{
    UINT32 Pixel = 0;

    Pixel |= (VidFbScaleColorComponentToMask(Red, framebufInfo.RedBits)
              << framebufInfo.RedShift) & framebufInfo.PixelMasks.RedMask;
    Pixel |= (VidFbScaleColorComponentToMask(Green, framebufInfo.GreenBits)
              << framebufInfo.GreenShift) & framebufInfo.PixelMasks.GreenMask;
    Pixel |= (VidFbScaleColorComponentToMask(Blue, framebufInfo.BlueBits)
              << framebufInfo.BlueShift) & framebufInfo.PixelMasks.BlueMask;

    if (framebufInfo.ReservedBits != 0)
    {
        Pixel |= (VidFbScaleColorComponentToMask(0xFF, framebufInfo.ReservedBits)
                  << framebufInfo.ReservedShift) & framebufInfo.PixelMasks.ReservedMask;
    }

    return Pixel;
}

static
__inline
VOID
VidFbExpandPixel(
    _In_ UINT32 Pixel,
    _Out_ PUCHAR Red,
    _Out_ PUCHAR Green,
    _Out_ PUCHAR Blue)
{
    *Red = VidFbScaleColorComponentFromMask(
        (Pixel & framebufInfo.PixelMasks.RedMask) >> framebufInfo.RedShift,
        framebufInfo.RedBits);
    *Green = VidFbScaleColorComponentFromMask(
        (Pixel & framebufInfo.PixelMasks.GreenMask) >> framebufInfo.GreenShift,
        framebufInfo.GreenBits);
    *Blue = VidFbScaleColorComponentFromMask(
        (Pixel & framebufInfo.PixelMasks.BlueMask) >> framebufInfo.BlueShift,
        framebufInfo.BlueBits);
}

static
__inline
UINT32
VidFbBlendPixels(
    _In_ UINT32 BackgroundPixel,
    _In_ UINT32 ForegroundPixel,
    _In_ UCHAR Alpha)
{
    UCHAR BgRed, BgGreen, BgBlue;
    UCHAR FgRed, FgGreen, FgBlue;
    UCHAR OutRed, OutGreen, OutBlue;

    if (Alpha == 0)
        return BackgroundPixel;

    if (Alpha == 0xFF)
        return ForegroundPixel;

    VidFbExpandPixel(BackgroundPixel, &BgRed, &BgGreen, &BgBlue);
    VidFbExpandPixel(ForegroundPixel, &FgRed, &FgGreen, &FgBlue);

    OutRed   = (UCHAR)((BgRed   * (255 - Alpha) + FgRed   * Alpha + 127) / 255);
    OutGreen = (UCHAR)((BgGreen * (255 - Alpha) + FgGreen * Alpha + 127) / 255);
    OutBlue  = (UCHAR)((BgBlue  * (255 - Alpha) + FgBlue  * Alpha + 127) / 255);

    return VidFbComposePixel(OutRed, OutGreen, OutBlue);
}

static
VOID
VidFbFillRect(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ UINT32 Color)
{
    ULONG Line, Col;
    PUINT32 Pixel;

    if ((Width == 0) || (Height == 0))
        return;

    if ((X >= framebufInfo.ScreenWidth) ||
        (Y >= (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES)))
    {
        return;
    }

    Width = min(Width, framebufInfo.ScreenWidth - X);
    Height = min(Height, (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) - Y);

    for (Line = 0; Line < Height; ++Line)
    {
        Pixel = (PUINT32)((PUCHAR)framebufInfo.BaseAddress +
                          (Y + TOP_BOTTOM_LINES + Line) * framebufInfo.Delta +
                          X * sizeof(UINT32));

        for (Col = 0; Col < Width; ++Col)
            Pixel[Col] = Color;
    }
}

static
VOID
VidFbOutputBitmapChar(
    _In_ UCHAR Char,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor)
{
    const UCHAR* FontPtr;
    PUINT32 Pixel;
    UCHAR Mask;
    ULONG Line, Col;

    if ((X + CHAR_WIDTH - 1 >= framebufInfo.ScreenWidth) ||
        (Y + CHAR_HEIGHT - 1 >= (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES)))
    {
        return;
    }

    FontPtr = BitmapFont8x16 + Char * CHAR_HEIGHT;
    Pixel = (PUINT32)((PUCHAR)framebufInfo.BaseAddress +
                      (Y + TOP_BOTTOM_LINES) * framebufInfo.Delta +
                      X * sizeof(UINT32));

    for (Line = 0; Line < CHAR_HEIGHT; ++Line)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; ++Col)
        {
            Pixel[Col] = ((FontPtr[Line] & Mask) != 0) ? FgColor : BgColor;
            Mask >>= 1;
        }
        Pixel = (PUINT32)((PUCHAR)Pixel + framebufInfo.Delta);
    }
}

static
__inline
ULONG
FbConsCellWidth(VOID)
{
    return FbConsFont.Enabled ? FbConsFont.CellWidth : CHAR_WIDTH;
}

static
__inline
ULONG
FbConsCellHeight(VOID)
{
    return FbConsFont.Enabled ? FbConsFont.CellHeight : CHAR_HEIGHT;
}

static
__inline
ULONG
FbConsWidth(VOID)
{
    return max(framebufInfo.ScreenWidth / FbConsCellWidth(), 1UL);
}

static
__inline
ULONG
FbConsHeight(VOID)
{
    ULONG VisibleHeight = framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES;
    return max(VisibleHeight / FbConsCellHeight(), 1UL);
}

static
ULONG
FbConsChooseFontPixelHeight(VOID)
{
    ULONG VisibleHeight = framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES;
    ULONG PixelHeight = max(VisibleHeight / FBCONS_FONT_TARGET_ROWS,
                            (ULONG)FBCONS_FONT_MIN_PIXELS);

    return min(PixelHeight, (ULONG)FBCONS_FONT_MAX_PIXELS);
}

static
BOOLEAN
FbConsMeasureFont(
    _In_ ULONG PixelHeight,
    _Out_ PULONG CellWidth,
    _Out_ PULONG CellHeight,
    _Out_ PULONG Baseline)
{
    FT_GlyphSlot Slot;
    FT_Error FtStatus;
    ULONG MaxGlyphWidth = 0;
    ULONG MaxAdvanceWidth = 0;
    LONG MaxTop = 0;
    LONG MaxBottom = 0;
    ULONG Char;
    BOOLEAN AnyGlyph = FALSE;

    FtStatus = FT_Set_Pixel_Sizes(FbConsFont.Face, 0, PixelHeight);
    if (FtStatus != FT_Err_Ok)
        return FALSE;

    for (Char = 0; Char < RTL_NUMBER_OF(FbConsGlyphs); ++Char)
    {
        if (!FbConsLoadGlyph(FbConsCp437ToUnicode((UCHAR)Char),
                             FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP))
            continue;

        FtStatus = FT_Render_Glyph(FbConsFont.Face->glyph, FT_RENDER_MODE_NORMAL);
        if (FtStatus != FT_Err_Ok)
            continue;

        Slot = FbConsFont.Face->glyph;
        if (Slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
            continue;

        AnyGlyph = TRUE;
        MaxGlyphWidth = max(MaxGlyphWidth, (ULONG)Slot->bitmap.width);
        MaxAdvanceWidth = max(MaxAdvanceWidth,
                             (ULONG)((Slot->advance.x + 32) >> 6));
        MaxTop = max(MaxTop, max((LONG)Slot->bitmap_top, 0L));
        MaxBottom = max(MaxBottom,
                        max((LONG)Slot->bitmap.rows - (LONG)Slot->bitmap_top, 0L));
    }

    if (!AnyGlyph)
        return FALSE;

    /* Use the font's advance width for cell width (correct for monospace
     * fonts like Lucida Console); fall back to bitmap width if needed */
    if (MaxAdvanceWidth == 0)
        MaxAdvanceWidth = MaxGlyphWidth;

    *Baseline = max((ULONG)MaxTop, PixelHeight - 2) + FBCONS_FONT_Y_PADDING;
    *CellHeight = max(*Baseline + (ULONG)MaxBottom + FBCONS_FONT_Y_PADDING,
                      PixelHeight + FBCONS_FONT_Y_PADDING);
    *CellWidth = max(MaxAdvanceWidth, (ULONG)CHAR_WIDTH);

    return TRUE;
}

static
BOOLEAN
FbConsCacheGlyph(
    _In_ UCHAR Char)
{
    FBCONS_GLYPH* Glyph = &FbConsGlyphs[Char];
    FT_GlyphSlot Slot;
    FT_Error FtStatus;
    SIZE_T BitmapSize;

    if (!FbConsLoadGlyph(FbConsCp437ToUnicode(Char),
                         FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP))
        return FALSE;

    FtStatus = FT_Render_Glyph(FbConsFont.Face->glyph, FT_RENDER_MODE_NORMAL);
    if (FtStatus != FT_Err_Ok)
        return FALSE;

    Slot = FbConsFont.Face->glyph;
    if (Slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
        return FALSE;

    Glyph->Left = (SHORT)Slot->bitmap_left;
    Glyph->Top = (SHORT)Slot->bitmap_top;
    Glyph->Width = (USHORT)Slot->bitmap.width;
    Glyph->Height = (USHORT)Slot->bitmap.rows;
    Glyph->Pitch = (USHORT)Slot->bitmap.pitch;

    BitmapSize = (SIZE_T)Glyph->Pitch * Glyph->Height;
    if (BitmapSize > sizeof(FbConsGlyphBitmap[Char]))
    {
        WARN("Glyph %u exceeds the static cache (%lu bytes)\n", Char, (ULONG)BitmapSize);
        return FALSE;
    }

    if (BitmapSize != 0)
    {
        RtlCopyMemory(FbConsGlyphBitmap[Char], Slot->bitmap.buffer, BitmapSize);
    }

    Glyph->Valid = TRUE;
    return TRUE;
}

static
VOID
FbConsInitializeFontRenderer(VOID)
{
    FT_Error FtStatus;
    ULONG PixelHeight;
    ULONG CellWidth = 0, CellHeight = 0, Baseline = 0;
    ULONG Char;

    RtlZeroMemory(&FbConsFont, sizeof(FbConsFont));
    RtlZeroMemory(FbConsGlyphs, sizeof(FbConsGlyphs));

    FtStatus = FT_Init_FreeType(&FbConsFont.Library);
    if (FtStatus != FT_Err_Ok)
    {
        TRACE("FreeType initialization failed: %d\n", FtStatus);
        return;
    }

    FtStatus = FT_New_Memory_Face(FbConsFont.Library,
                                  uefi_fb_font_data,
                                  uefi_fb_font_data_SIZE,
                                  0,
                                  &FbConsFont.Face);
    if (FtStatus != FT_Err_Ok)
    {
        TRACE("Loading the embedded console font failed: %d\n", FtStatus);
        return;
    }

    if (!FbConsInitializeFontCmap())
        return;

    PixelHeight = FbConsChooseFontPixelHeight();
    while (TRUE)
    {
        if (!FbConsMeasureFont(PixelHeight, &CellWidth, &CellHeight, &Baseline))
            return;

        if (((framebufInfo.ScreenWidth / CellWidth) >= FBCONS_FONT_MIN_COLUMNS &&
             ((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CellHeight) >= FBCONS_FONT_MIN_ROWS) ||
            (PixelHeight <= FBCONS_FONT_MIN_PIXELS))
        {
            break;
        }

        --PixelHeight;
    }

    FtStatus = FT_Set_Pixel_Sizes(FbConsFont.Face, 0, PixelHeight);
    if (FtStatus != FT_Err_Ok)
        return;

    FbConsFont.PixelHeight = PixelHeight;
    FbConsFont.CellWidth = CellWidth;
    FbConsFont.CellHeight = CellHeight;
    FbConsFont.Baseline = Baseline;

    for (Char = 0; Char < RTL_NUMBER_OF(FbConsGlyphs); ++Char)
    {
        FbConsCacheGlyph((UCHAR)Char);
    }

    FbConsFont.Enabled = TRUE;

    TRACE("Using FreeType framebuffer font at %lu px (%lux%lu cells, console %lux%lu)\n",
          FbConsFont.PixelHeight,
          FbConsFont.CellWidth,
          FbConsFont.CellHeight,
          FbConsWidth(),
          FbConsHeight());
}
#endif

#if DBG
static VOID
VidFbPrintFramebufferInfo(VOID)
{
    TRACE("Framebuffer format:\n");
    TRACE("    BaseAddress       : 0x%X\n", framebufInfo.BaseAddress);
    TRACE("    BufferSize        : %lu\n", framebufInfo.BufferSize);
    TRACE("    ScreenWidth       : %lu\n", framebufInfo.ScreenWidth);
    TRACE("    ScreenHeight      : %lu\n", framebufInfo.ScreenHeight);
    TRACE("    PixelsPerScanLine : %lu\n", framebufInfo.PixelsPerScanLine);
    TRACE("    BitsPerPixel      : %lu\n", framebufInfo.BitsPerPixel);
    TRACE("    BytesPerPixel     : %lu\n", framebufInfo.BytesPerPixel);
    TRACE("    Delta             : %lu\n", framebufInfo.Delta);
    TRACE("    ARGB masks:       : %08x/%08x/%08x/%08x\n",
          framebufInfo.PixelMasks.ReservedMask,
          framebufInfo.PixelMasks.RedMask,
          framebufInfo.PixelMasks.GreenMask,
          framebufInfo.PixelMasks.BlueMask);
}
#endif

/**
 * @brief
 * Initializes internal framebuffer information based on the given parameters.
 *
 * @param[in]   BaseAddress
 * The framebuffer physical base address.
 *
 * @param[in]   BufferSize
 * The framebuffer size, in bytes.
 *
 * @param[in]   ScreenWidth
 * @param[in]   ScreenHeight
 * The width and height of the visible framebuffer area, in pixels.
 *
 * @param[in]   PixelsPerScanLine
 * The size in number of pixels of a whole horizontal video memory scanline.
 *
 * @param[in]   BitsPerPixel
 * The number of usable bits (not counting the reserved ones) per pixel.
 *
 * @param[in]   PixelMasks
 * Optional pointer to a PIXEL_BITMASK structure describing the pixel
 * format used by the framebuffer.
 *
 * @return
 * TRUE if initialization is successful; FALSE if not.
 **/
BOOLEAN
VidFbInitializeVideo(
    _Out_opt_ PCM_FRAMEBUF_DEVICE_DATA* pFbData,
    _In_ ULONG_PTR BaseAddress,
    _In_ ULONG BufferSize,
    _In_ UINT32 ScreenWidth,
    _In_ UINT32 ScreenHeight,
    _In_ UINT32 PixelsPerScanLine,
    _In_ UINT32 BitsPerPixel,
    _In_opt_ PPIXEL_BITMASK PixelMasks)
{
    PPIXEL_BITMASK BitMasks = &framebufInfo.PixelMasks;

    if (pFbData)
        *pFbData = NULL;

    RtlZeroMemory(&framebufInfo, sizeof(framebufInfo));

    /* Verify framebuffer dimensions */
    if ((ScreenWidth < 1) || (ScreenHeight < 1))
    {
        ERR("Invalid framebuffer dimensions\n");
        return FALSE;
    }

    framebufInfo.BaseAddress  = BaseAddress;
    framebufInfo.BufferSize   = BufferSize;
    framebufInfo.ScreenWidth  = ScreenWidth;
    framebufInfo.ScreenHeight = ScreenHeight;
    framebufInfo.PixelsPerScanLine = PixelsPerScanLine;
    framebufInfo.BitsPerPixel = BitsPerPixel;

    framebufInfo.BytesPerPixel = (BitsPerPixel + 7) / 8; // Round up to nearest byte.
    framebufInfo.Delta = (PixelsPerScanLine * framebufInfo.BytesPerPixel + 3) & ~3;

    /* Verify that the framebuffer fits inside the video RAM */
    if (!(ScreenHeight * framebufInfo.Delta <= BufferSize))
    {
        ERR("Framebuffer doesn't fit inside the video RAM (FB size: %lu, VRAM size: %lu)\n",
            ScreenHeight * framebufInfo.Delta, BufferSize);
        return FALSE;
    }

    /* We currently only support 32bpp */
    if (BitsPerPixel != 32)
    {
        /* Unsupported BPP */
        ERR("Unsupported %lu bits per pixel format\n", BitsPerPixel);
        return FALSE;
    }

    //ASSERT((BitsPerPixel <= 8 && !PixelMasks) || (BitsPerPixel > 8));
    if (BitsPerPixel > 8)
    {
        if (!PixelMasks ||
            (PixelMasks->RedMask   == 0 &&
             PixelMasks->GreenMask == 0 &&
             PixelMasks->BlueMask  == 0 /* &&
             PixelMasks->ReservedMask == 0 */))
        {
            /* Determine pixel mask given color depth and color channel */
            switch (BitsPerPixel)
            {
                case 32:
                case 24: /* 8:8:8 */
                    BitMasks->RedMask   = 0x00FF0000; // 0x00FF0000;
                    BitMasks->GreenMask = 0x0000FF00; // 0x00FF0000 >> 8;
                    BitMasks->BlueMask  = 0x000000FF; // 0x00FF0000 >> 16;
                    BitMasks->ReservedMask = ((1 << (BitsPerPixel - 24)) - 1) << 24;
                    break;

                case 16: /* 5:6:5 */
                    BitMasks->RedMask   = 0xF800; // 0xF800;
                    BitMasks->GreenMask = 0x07E0; // (0xF800 >> 5) | 0x20;
                    BitMasks->BlueMask  = 0x001F; // 0xF800 >> 11;
                    BitMasks->ReservedMask = 0;
                    break;

                case 15: /* 5:5:5 */
                    BitMasks->RedMask   = 0x7C00; // 0x7C00;
                    BitMasks->GreenMask = 0x03E0; // 0x7C00 >> 5;
                    BitMasks->BlueMask  = 0x001F; // 0x7C00 >> 10;
                    BitMasks->ReservedMask = 0x8000;
                    break;

                default:
                    /* Unsupported BPP */
                    UNIMPLEMENTED;
                    RtlZeroMemory(BitMasks, sizeof(*BitMasks));
            }
        }
        else
        {
            /* Copy the pixel masks */
            RtlCopyMemory(BitMasks, PixelMasks, sizeof(*BitMasks));
        }
    }
    else
    {
        /* Palettized modes don't use masks */
        RtlZeroMemory(BitMasks, sizeof(*BitMasks));
    }

    framebufInfo.RedShift = VidFbMaskShift(BitMasks->RedMask);
    framebufInfo.GreenShift = VidFbMaskShift(BitMasks->GreenMask);
    framebufInfo.BlueShift = VidFbMaskShift(BitMasks->BlueMask);
    framebufInfo.ReservedShift = VidFbMaskShift(BitMasks->ReservedMask);

    framebufInfo.RedBits = (UCHAR)CountNumberOfBits(BitMasks->RedMask);
    framebufInfo.GreenBits = (UCHAR)CountNumberOfBits(BitMasks->GreenMask);
    framebufInfo.BlueBits = (UCHAR)CountNumberOfBits(BitMasks->BlueMask);
    framebufInfo.ReservedBits = (UCHAR)CountNumberOfBits(BitMasks->ReservedMask);

#if DBG
    VidFbPrintFramebufferInfo();
    {
    ULONG BppFromMasks =
        PixelBitmasksToBpp(BitMasks->RedMask,
                           BitMasks->GreenMask,
                           BitMasks->BlueMask,
                           BitMasks->ReservedMask);
        TRACE("BitsPerPixel = %lu , BppFromMasks = %lu\n", BitsPerPixel, BppFromMasks);
    //ASSERT(BitsPerPixel == BppFromMasks);
    }
#endif

    /* Initialize the hardware device configuration data if specified */
    if (pFbData)
    {
        FrameBufferData.FrameBufferOffset = 0;
        FrameBufferData.ScreenWidth  = framebufInfo.ScreenWidth;
        FrameBufferData.ScreenHeight = framebufInfo.ScreenHeight;
        FrameBufferData.PixelsPerScanLine = framebufInfo.PixelsPerScanLine;
        FrameBufferData.BitsPerPixel = framebufInfo.BitsPerPixel;

        RtlCopyMemory(&FrameBufferData.PixelMasks,
                      &framebufInfo.PixelMasks, sizeof(framebufInfo.PixelMasks));

        *pFbData = &FrameBufferData;
    }

#ifdef UEFIBOOT
    FbConsInitializeFontRenderer();

    if (!FbConsFont.Enabled)
    {
        TRACE("FreeType framebuffer font unavailable, falling back to the built-in VGA font\n");
    }
#endif

    return TRUE;
}

VOID
VidFbClearScreenColor(
    _In_ UINT32 Color,
    _In_ BOOLEAN FullScreen)
{
    ULONG Line, Col;
    PUINT32 p;

    for (Line = 0; Line < framebufInfo.ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (PUINT32)((PUCHAR)framebufInfo.BaseAddress + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * framebufInfo.Delta);
        for (Col = 0; Col < framebufInfo.ScreenWidth; Col++)
        {
            *p++ = Color;
        }
    }
}

/**
 * @brief
 * Displays a character at a given pixel position with specific foreground
 * and background colors.
 **/
VOID
VidFbOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor)
{
#ifdef UEFIBOOT
    const FBCONS_GLYPH* Glyph;
    ULONG GlyphX, GlyphY;
    ULONG Row, Col;
    PUINT32 Pixel;
    const UCHAR* Bitmap;

    if (!FbConsFont.Enabled)
    {
        VidFbOutputBitmapChar(Char, X, Y, FgColor, BgColor);
        return;
    }

    if ((X + FbConsCellWidth() - 1 >= framebufInfo.ScreenWidth) ||
        (Y + FbConsCellHeight() - 1 >= (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES)))
    {
        return;
    }

    VidFbFillRect(X, Y, FbConsCellWidth(), FbConsCellHeight(), BgColor);

    Glyph = &FbConsGlyphs[Char];
    if (!Glyph->Valid)
    {
        VidFbOutputBitmapChar(Char,
                              X + (FbConsCellWidth() - CHAR_WIDTH) / 2,
                              Y + (FbConsCellHeight() - CHAR_HEIGHT) / 2,
                              FgColor,
                              BgColor);
        return;
    }

    if ((Glyph->Width == 0) || (Glyph->Height == 0))
        return;

    GlyphX = X + (ULONG)max((LONG)Glyph->Left, 0L);
    GlyphY = Y + FbConsFont.Baseline - Glyph->Top;

    for (Row = 0; Row < Glyph->Height; ++Row)
    {
        if (GlyphY + Row >= (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES))
            break;

        Pixel = (PUINT32)((PUCHAR)framebufInfo.BaseAddress +
                          (GlyphY + TOP_BOTTOM_LINES + Row) * framebufInfo.Delta +
                          GlyphX * sizeof(UINT32));
        Bitmap = &FbConsGlyphBitmap[Char][Row * Glyph->Pitch];

        for (Col = 0; Col < Glyph->Width; ++Col)
        {
            if (GlyphX + Col >= framebufInfo.ScreenWidth)
                break;

            if (Bitmap[Col] != 0)
                Pixel[Col] = VidFbBlendPixels(Pixel[Col], FgColor, Bitmap[Col]);
        }
    }
#else
    VidFbOutputBitmapChar(Char, X, Y, FgColor, BgColor);
#endif
}

/**
 * @brief
 * Returns the width and height in pixels, of the whole visible area
 * of the graphics framebuffer.
 **/
VOID
VidFbGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth)
{
    *Width  = framebufInfo.ScreenWidth;
    *Height = framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES;
    *Depth  = framebufInfo.BitsPerPixel;
}

/**
 * @brief
 * Returns the size in bytes, of a full graphics pixel buffer rectangle
 * that can fill the whole visible area of the graphics framebuffer.
 **/
ULONG
VidFbGetBufferSize(VOID)
{
    return ((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) *
            framebufInfo.ScreenWidth * framebufInfo.BytesPerPixel);
}

VOID
VidFbScrollUp(
    _In_ UINT32 Color,
    _In_ ULONG Scroll)
{
    ULONG VisibleHeight = framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES;
    ULONG Line, Col;
    PUINT32 Pixel;
    PUCHAR Dst;
    PUCHAR Src;

    if (Scroll == 0)
        return;

    if (Scroll >= VisibleHeight)
    {
        VidFbClearScreenColor(Color, FALSE);
        return;
    }

    for (Line = 0; Line < VisibleHeight - Scroll; ++Line)
    {
        Dst = (PUCHAR)framebufInfo.BaseAddress +
              (TOP_BOTTOM_LINES + Line) * framebufInfo.Delta;
        Src = (PUCHAR)framebufInfo.BaseAddress +
              (TOP_BOTTOM_LINES + Line + Scroll) * framebufInfo.Delta;

        RtlMoveMemory(Dst, Src, framebufInfo.ScreenWidth * sizeof(UINT32));
    }

    for (; Line < VisibleHeight; ++Line)
    {
        Pixel = (PUINT32)((PUCHAR)framebufInfo.BaseAddress +
                          (TOP_BOTTOM_LINES + Line) * framebufInfo.Delta);

        for (Col = 0; Col < framebufInfo.ScreenWidth; ++Col)
            Pixel[Col] = Color;
    }
}

#if 0
VOID
VidFbSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* We don't have a cursor yet */
}

VOID
VidFbHideShowTextCursor(BOOLEAN Show)
{
    /* We don't have a cursor yet */
}

BOOLEAN
VidFbIsPaletteFixed(VOID)
{
    return FALSE;
}

VOID
VidFbSetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR Red, _In_ UCHAR Green, _In_ UCHAR Blue)
{
    /* Not supported */
}

VOID
VidFbGetPaletteColor(
    _In_ UCHAR Color,
    _Out_ PUCHAR Red, _Out_ PUCHAR Green, _Out_ PUCHAR Blue)
{
    /* Not supported */
}
#endif



/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Linear framebuffer based console support
 * COPYRIGHT:   Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#define VGA_CHAR_SIZE 2

static inline
UINT32
FbConsAttrToSingleColor(
    _In_ UCHAR Attr)
{
    UCHAR Intensity = ((Attr & 0x08) == 0) ? 127 : 255;

    return VidFbComposePixel((Attr & 0x04) ? Intensity : 0,
                             (Attr & 0x02) ? Intensity : 0,
                             (Attr & 0x01) ? Intensity : 0);
}

/**
 * @brief
 * Maps a text-mode CGA-style character attribute to separate
 * foreground and background colors in the framebuffer's native format.
 **/
static VOID
FbConsAttrToColors(
    _In_ UCHAR Attr,
    _Out_ PUINT32 FgColor,
    _Out_ PUINT32 BgColor)
{
    *FgColor = FbConsAttrToSingleColor(Attr & 0x0F);
    *BgColor = FbConsAttrToSingleColor((Attr >> 4) & 0x0F);
}

VOID
FbConsClearScreen(
    _In_ UCHAR Attr)
{
    UINT32 FgColor, BgColor;
    FbConsAttrToColors(Attr, &FgColor, &BgColor);
    VidFbClearScreenColor(BgColor, FALSE);
}

/**
 * @brief
 * Displays a character at a given position with specific foreground
 * and background colors.
 **/
VOID
FbConsOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG Column,
    _In_ ULONG Row,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor)
{
    /* Don't display outside of the screen */
    if ((Column >= FbConsWidth()) || (Row >= FbConsHeight()))
        return;

    VidFbOutputChar(Char,
                    Column * FbConsCellWidth(),
                    Row * FbConsCellHeight(),
                    FgColor,
                    BgColor);
}

/**
 * @brief
 * Displays a character with specific text attributes at a given position.
 **/
VOID
FbConsPutChar(
    _In_ UCHAR Char,
    _In_ UCHAR Attr,
    _In_ ULONG Column,
    _In_ ULONG Row)
{
    UINT32 FgColor, BgColor;

    FbConsAttrToColors(Attr, &FgColor, &BgColor);
    FbConsOutputChar(Char, Column, Row, FgColor, BgColor);
}

/**
 * @brief
 * Returns the width and height in number of CGA characters/attributes, of a
 * full text-mode CGA-style character buffer rectangle that can fill the whole console.
 **/
VOID
FbConsGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth)
{
    // VidFbGetDisplaySize(Width, Height, Depth);
    // *Width  /= CHAR_WIDTH;
    // *Height /= CHAR_HEIGHT;
    *Width = FbConsWidth();
    *Height = FbConsHeight();
    *Depth = framebufInfo.BitsPerPixel;
}

/**
 * @brief
 * Draws a progress bar in framebuffer pixels using text-grid coordinates.
 **/
VOID
FbConsDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Right,
    _In_ ULONG Row,
    _In_ UCHAR FillAttr,
    _In_ UCHAR EmptyAttr,
    _In_ ULONG SubPercentTimes100)
{
    UINT32 FillColor, EmptyColor, Dummy;
    ULONG CellWidth, CellHeight;
    ULONG Margin, BarLeft, BarTop, BarWidth, BarHeight;
    ULONG FillWidth;

    if ((Left > Right) || (Row >= FbConsHeight()))
        return;

    CellWidth = FbConsCellWidth();
    CellHeight = FbConsCellHeight();
    Margin = (CellHeight > 2) ? 1 : 0;

    BarLeft = Left * CellWidth;
    BarTop = Row * CellHeight + Margin;
    BarWidth = (Right - Left + 1) * CellWidth;
    BarHeight = max(CellHeight - 2 * Margin, 1UL);
    FillWidth = BarWidth * min(SubPercentTimes100, 100UL * 100UL) / (100 * 100);

    FbConsAttrToColors(FillAttr, &Dummy, &FillColor);
    FbConsAttrToColors(EmptyAttr, &Dummy, &EmptyColor);

    if (FillWidth > 0)
    {
        VidFbFillRect(BarLeft, BarTop,
                      FillWidth, BarHeight,
                      FillColor);
    }
    if (FillWidth < BarWidth)
    {
        VidFbFillRect(BarLeft + FillWidth, BarTop,
                      BarWidth - FillWidth, BarHeight,
                      EmptyColor);
    }
}

/**
 * @brief
 * Returns the size in bytes, of a full text-mode CGA-style
 * character buffer rectangle that can fill the whole console.
 **/
ULONG
FbConsGetBufferSize(VOID)
{
    return FbConsHeight() * FbConsWidth() * VGA_CHAR_SIZE;
}

/**
 * @brief
 * Copies a full text-mode CGA-style character buffer rectangle to the console.
 **/
// TODO: Write a VidFb "BitBlt" equivalent.
VOID
FbConsCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;
    ULONG Row, Col;
    // ULONG Width, Height, Depth;
    // FbConsGetDisplaySize(&Width, &Height, &Depth);
    ULONG Width = FbConsWidth();
    ULONG Height = FbConsHeight();

    for (Row = 0; Row < Height; ++Row)
    {
        for (Col = 0; Col < Width; ++Col)
        {
            FbConsPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Row);
            OffScreenBuffer += VGA_CHAR_SIZE;
        }
    }
}

VOID
FbConsScrollUp(
    _In_ UCHAR Attr)
{
    UINT32 BgColor, Dummy;
    FbConsAttrToColors(Attr, &Dummy, &BgColor);
    VidFbScrollUp(BgColor, FbConsCellHeight());
}
