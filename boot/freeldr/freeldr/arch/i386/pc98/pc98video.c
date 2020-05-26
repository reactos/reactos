/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <drivers/pc98/video.h>

extern UCHAR BitmapFont8x16[];
extern BOOLEAN HiResoMachine;

/* GLOBALS ********************************************************************/

#define VGA_CHAR_SIZE 2

#define TEXT_CHAR_SIZE 2
UCHAR TextCols;
UCHAR TextLines;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 400
#define BYTES_PER_SCANLINE (SCREEN_WIDTH / 8)

ULONG VramText;
static ULONG VramPlaneB;
static ULONG VramPlaneG;
static ULONG VramPlaneR;
static ULONG VramPlaneI;

static const PALETTE_ENTRY CgaPalette[] =
{
    {0x00, 0x00, 0x00},
    {0x00, 0x00, 0x0A},
    {0x00, 0x0A, 0x00},
    {0x00, 0x0A, 0x0A},
    {0x0A, 0x00, 0x00},
    {0x0A, 0x00, 0x0A},
    {0x0A, 0x05, 0x00},
    {0x0A, 0x0A, 0x0A},
    {0x05, 0x05, 0x05},
    {0x05, 0x05, 0x0F},
    {0x05, 0x0F, 0x05},
    {0x05, 0x0F, 0x0F},
    {0x0F, 0x05, 0x05},
    {0x0F, 0x05, 0x0F},
    {0x0F, 0x0F, 0x05},
    {0x0F, 0x0F, 0x0F}
};

/* FUNCTIONS ******************************************************************/

VOID
Pc98VideoInit(VOID)
{
    REGS Regs;
    USHORT i;

    if (HiResoMachine)
    {
        VramPlaneB = VRAM_HI_RESO_PLANE_B;
        VramPlaneG = VRAM_HI_RESO_PLANE_G;
        VramPlaneR = VRAM_HI_RESO_PLANE_R;
        VramPlaneI = VRAM_HI_RESO_PLANE_I;
        VramText = VRAM_HI_RESO_TEXT;
        TextCols = 80;
        TextLines = 31;
    }
    else
    {
        VramPlaneB = VRAM_NORMAL_PLANE_B;
        VramPlaneG = VRAM_NORMAL_PLANE_G;
        VramPlaneR = VRAM_NORMAL_PLANE_R;
        VramPlaneI = VRAM_NORMAL_PLANE_I;
        VramText = VRAM_NORMAL_TEXT;
        TextCols = 80;
        TextLines = 25;
    }

    for (i = 0; i < VRAM_ATTR_SIZE; i += TEXT_CHAR_SIZE)
        *(PUCHAR)(VramText + VRAM_TEXT_ATTR_OFFSET + i) = GDC_ATTR_WHITE | GDC_ATTR_VISIBLE;

    /* Int 18h AH=41h
     * CRT BIOS - Stop displaying graphics
     */
    Regs.b.ah = 0x41;
    Int386(0x18, &Regs, &Regs);

    /* Int 18h AH=42h
     * CRT BIOS - Set display area
     *
     * CH0-CH3 - always zero
     * CH4 - video page
     * CH5 - CRT display mode
     *    0 - color
     *    1 - monochrome
     * CH6-CH7 - VRAM area
     *    01 - Upper-half (16-32 kB), 640x200
     *    10 - Lower-half (0-16 kB), 640x200
     *    11 - All (0-32 kB), 640x400
     */
    Regs.b.ah = 0x42;
    Regs.b.ch = 0xC0;
    Int386(0x18, &Regs, &Regs); /* 640x400 */

    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_COLORS_16);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_VIDEO_PAGE, 0);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_VIDEO_PAGE_ACCESS, 0);

    Pc98VideoSync();
    for (i = 0; i < RTL_NUMBER_OF(CgaPalette); i++)
        Pc98VideoSetPaletteColor(i, CgaPalette[i].Red, CgaPalette[i].Green, CgaPalette[i].Blue);

    /* Int 18h AH=0Ah
     * CRT BIOS - Set text screen mode
     *
     * AL0 - text rows
     *    0 - 25
     *    1 - 20
     * AL1 - text cols
     *    0 - 80
     *    1 - 40
     * AL2 - text attribute
     *    0 - with vertical line
     *    1 - normal
     * AL3 - KCG access mode
     *    0 - code
     *    1 - bitmap
     * AL4-AL7 - always zero
     *
     * High-resolution machine:
     * AL4 - text rows, AL3 - KCG access mode
     */
    Regs.b.ah = 0x0A;
    Regs.b.al = HiResoMachine ? 0x10 : 0x00;
    Int386(0x18, &Regs, &Regs); /* 80x25(31) */

    /* Int 18h AH=0Ch
     * CRT BIOS - Start displaying text
     */
    Regs.b.ah = 0x0C;
    Int386(0x18, &Regs, &Regs);

    /* Int 18h AH=40h
     * CRT BIOS - Start displaying graphics
     */
    Regs.b.ah = 0x40;
    Int386(0x18, &Regs, &Regs);
}

VOID
Pc98VideoClearScreen(UCHAR Attr)
{
    USHORT i;
    USHORT B = (Attr & 0x10) ? 0xFFFF : 0;
    USHORT G = (Attr & 0x20) ? 0xFFFF : 0;
    USHORT R = (Attr & 0x40) ? 0xFFFF : 0;
    USHORT I = (Attr & 0x80) ? 0xFFFF : 0;

    for (i = 0; i < VRAM_TEXT_SIZE; i += TEXT_CHAR_SIZE)
        *(PUSHORT)(VramText + i) = ' ';

    for (i = 0; i < BYTES_PER_SCANLINE * SCREEN_HEIGHT; i += sizeof(USHORT))
    {
        *(PUSHORT)(VramPlaneB + i) = B;
        *(PUSHORT)(VramPlaneG + i) = G;
        *(PUSHORT)(VramPlaneR + i) = R;
        *(PUSHORT)(VramPlaneI + i) = I;
    }
}

VIDEODISPLAYMODE
Pc98VideoSetDisplayMode(char *DisplayModeName, BOOLEAN Init)
{
    /* Not supported by hardware */
    return VideoTextMode;
}

VOID
Pc98VideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    *Width = SCREEN_WIDTH / CHAR_WIDTH;
    *Height = SCREEN_HEIGHT / CHAR_HEIGHT;
    *Depth = 0;
}

ULONG
Pc98VideoGetBufferSize(VOID)
{
    return (SCREEN_WIDTH / CHAR_WIDTH) * (SCREEN_HEIGHT / CHAR_HEIGHT) * VGA_CHAR_SIZE;
}

VOID
Pc98VideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    *RomFontPointers = VramText + 0x4000;
}

VOID
Pc98VideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    CSRWPARAM CursorParameters;

    RtlZeroMemory(&CursorParameters, sizeof(CSRWPARAM));
    CursorParameters.CursorAddress = X + Y * TextCols;
    CursorParameters.DotAddress = 0;

    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_COMMAND, GDC_COMMAND_CSRW);
    WRITE_GDC_CSRW((PUCHAR)GDC1_IO_o_PARAM, &CursorParameters);
}

VOID
Pc98VideoHideShowTextCursor(BOOLEAN Show)
{
    CSRFORMPARAM CursorParameters;

    RtlZeroMemory(&CursorParameters, sizeof(CSRFORMPARAM));
    CursorParameters.Show = Show;
    CursorParameters.Blink = TRUE;
    CursorParameters.BlinkRate = 12;
    CursorParameters.LinesPerRow = 16;
    CursorParameters.StartScanLine = 12;
    CursorParameters.EndScanLine = 15;

    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_COMMAND, GDC_COMMAND_CSRFORM);
    WRITE_GDC_CSRFORM((PUCHAR)GDC1_IO_o_PARAM, &CursorParameters);
}

VOID
Pc98VideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    UCHAR Line;
    UCHAR B = (Attr & 0x10) ? 0xFF : 0;
    UCHAR G = (Attr & 0x20) ? 0xFF : 0;
    UCHAR R = (Attr & 0x40) ? 0xFF : 0;
    UCHAR I = (Attr & 0x80) ? 0xFF : 0;
    ULONG VramOffset = X + (Y * CHAR_HEIGHT) * BYTES_PER_SCANLINE;
    PUCHAR FontPtr = BitmapFont8x16 + Ch * 16;

    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        if (Attr & 0x0F)
        {
            *(PUCHAR)(VramPlaneB + VramOffset + Line * BYTES_PER_SCANLINE) = B | ((Attr & 0x01) ? FontPtr[Line] : 0);
            *(PUCHAR)(VramPlaneG + VramOffset + Line * BYTES_PER_SCANLINE) = G | ((Attr & 0x02) ? FontPtr[Line] : 0);
            *(PUCHAR)(VramPlaneR + VramOffset + Line * BYTES_PER_SCANLINE) = R | ((Attr & 0x04) ? FontPtr[Line] : 0);
            *(PUCHAR)(VramPlaneI + VramOffset + Line * BYTES_PER_SCANLINE) = I | ((Attr & 0x08) ? FontPtr[Line] : 0);
        }
        else
        {
            *(PUCHAR)(VramPlaneB + VramOffset + Line * BYTES_PER_SCANLINE) = B & ~FontPtr[Line];
            *(PUCHAR)(VramPlaneG + VramOffset + Line * BYTES_PER_SCANLINE) = G & ~FontPtr[Line];
            *(PUCHAR)(VramPlaneR + VramOffset + Line * BYTES_PER_SCANLINE) = R & ~FontPtr[Line];
            *(PUCHAR)(VramPlaneI + VramOffset + Line * BYTES_PER_SCANLINE) = I & ~FontPtr[Line];
        }
    }
}

VOID
Pc98VideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;
    USHORT X, Y;

    for (Y = 0; Y < SCREEN_HEIGHT / CHAR_HEIGHT; Y++)
    {
        for (X = 0; X < SCREEN_WIDTH / CHAR_WIDTH; X++)
        {
            Pc98VideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], X, Y);
            OffScreenBuffer += VGA_CHAR_SIZE;
        }
    }
}

BOOLEAN
Pc98VideoIsPaletteFixed(VOID)
{
    return FALSE;
}

VOID
Pc98VideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
    if (Color < 16)
    {
        WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_PALETTE_INDEX, Color);
        WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_RED, Red);
        WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_GREEN, Green);
        WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_BLUE, Blue);
    }
}

VOID
Pc98VideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
{
    if (Color < 16)
    {
        WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_PALETTE_INDEX, Color);
        *Red = READ_PORT_UCHAR((PUCHAR)GDC2_IO_i_RED);
        *Green = READ_PORT_UCHAR((PUCHAR)GDC2_IO_i_GREEN);
        *Blue = READ_PORT_UCHAR((PUCHAR)GDC2_IO_i_BLUE);
    }
    else
    {
        *Red = 0;
        *Green = 0;
        *Blue = 0;
    }
}

VOID
Pc98VideoSync(VOID)
{
    while (READ_PORT_UCHAR((PUCHAR)GDC2_IO_i_STATUS) & GDC_STATUS_VSYNC)
        NOTHING;

    while (!(READ_PORT_UCHAR((PUCHAR)GDC2_IO_i_STATUS) & GDC_STATUS_VSYNC))
        NOTHING;
}

VOID
Pc98VideoPrepareForReactOS(VOID)
{
    REGS Regs;

    /* Int 18h AH=41h
     * CRT BIOS - Stop displaying graphics
     */
    Regs.b.ah = 0x41;
    Int386(0x18, &Regs, &Regs);

    Pc98VideoHideShowTextCursor(FALSE);
}
