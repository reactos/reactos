/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Graphics system header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/* Video memory ***************************************************************/

#define VRAM_NORMAL_PLANE_B      0xA8000
#define VRAM_NORMAL_PLANE_G      0xB0000
#define VRAM_NORMAL_PLANE_R      0xB8000
#define VRAM_NORMAL_PLANE_I      0xE0000
#define VRAM_PLANE_SIZE          0x08000
#define VRAM_NORMAL_TEXT         0xA0000
#define VRAM_TEXT_ATTR_OFFSET    0x02000
#define VRAM_TEXT_SIZE           0x02000
#define VRAM_ATTR_SIZE           0x02000

/* High-resolution machine */
#define VRAM_HI_RESO_PLANE_B     0xC0000
#define VRAM_HI_RESO_PLANE_G     0xC8000
#define VRAM_HI_RESO_PLANE_R     0xD0000
#define VRAM_HI_RESO_PLANE_I     0xD8000
#define VRAM_HI_RESO_TEXT        0xE0000

/* GDC definitions ************************************************************/

#define GDC_STATUS_DRDY           0x01
#define GDC_STATUS_FIFO_FULL      0x02
#define GDC_STATUS_FIFO_EMPTY     0x04
#define GDC_STATUS_DRAWING        0x08
#define GDC_STATUS_DMA_EXECUTE    0x10
#define GDC_STATUS_VSYNC          0x20
#define GDC_STATUS_HBLANK         0x40
#define GDC_STATUS_LPEN           0x80

#define GDC_ATTR_VISIBLE          0x01
#define GDC_ATTR_BLINK            0x02
#define GDC_ATTR_REVERSE          0x04
#define GDC_ATTR_UNDERLINE        0x08
#define GDC_ATTR_VERTICAL_LINE    0x10

#define GDC_ATTR_BLACK            0x00
#define GDC_ATTR_BLUE             0x20
#define GDC_ATTR_RED              0x40
#define GDC_ATTR_PURPLE           0x60
#define GDC_ATTR_GREEN            0x80
#define GDC_ATTR_LIGHTBLUE        0xA0
#define GDC_ATTR_YELLOW           0xC0
#define GDC_ATTR_WHITE            0xE0

#define GDC_COMMAND_RESET         0x00
#define GDC_COMMAND_BCTRL_STOP    0x0C
#define GDC_COMMAND_BCTRL_START   0x0D
#define GDC_COMMAND_SYNC_ON       0x0E
#define GDC_COMMAND_SYNC_OFF      0x0F
#define GDC_COMMAND_WRITE         0x20
#define GDC_COMMAND_SLAVE         0x6E
#define GDC_COMMAND_MASTER        0x6F

#define GDC_COMMAND_CSRFORM       0x4B
typedef struct _CSRFORMPARAM
{
    BOOLEAN Show;
    BOOLEAN Blink;
    UCHAR BlinkRate;
    UCHAR LinesPerRow;
    UCHAR StartScanLine;
    UCHAR EndScanLine;
} CSRFORMPARAM, *PCSRFORMPARAM;

FORCEINLINE
VOID
WRITE_GDC_CSRFORM(PUCHAR Port, PCSRFORMPARAM CursorParameters)
{
    WRITE_PORT_UCHAR(Port, ((CursorParameters->Show & 0x01) << 7) |
                     (CursorParameters->LinesPerRow - 1));
    WRITE_PORT_UCHAR(Port, ((CursorParameters->BlinkRate & 0x03) << 6) |
                     ((!CursorParameters->Blink & 0x01) << 5) | CursorParameters->StartScanLine);
    WRITE_PORT_UCHAR(Port, (CursorParameters->EndScanLine << 3) | ((CursorParameters->BlinkRate & 0x1C) >> 2));
}

#define GDC_COMMAND_START         0x6B
#define GDC_COMMAND_ZOOM          0x46

#define GDC_COMMAND_CSRW          0x49
typedef struct _CSRWPARAM
{
    ULONG CursorAddress;
    UCHAR DotAddress;
} CSRWPARAM, *PCSRWPARAM;

FORCEINLINE
VOID
WRITE_GDC_CSRW(PUCHAR Port, PCSRWPARAM CursorParameters)
{
    ASSERT(CursorParameters->CursorAddress < 0xF00000);
    ASSERT(CursorParameters->DotAddress < 0x10);

    WRITE_PORT_UCHAR(Port, CursorParameters->CursorAddress & 0xFF);
    WRITE_PORT_UCHAR(Port, (CursorParameters->CursorAddress >> 8) & 0xFF);
    WRITE_PORT_UCHAR(Port, (CursorParameters->DotAddress << 4) |
                     ((CursorParameters->CursorAddress >> 16) & 0x03));
}

#define GDC_COMMAND_PRAM          0x70
#define GDC_COMMAND_PITCH         0x47
#define GDC_COMMAND_MASK          0x4A
#define GDC_COMMAND_FIGS          0x4C
#define GDC_COMMAND_FIGD          0x6C
#define GDC_COMMAND_GCHRD         0x68
#define GDC_COMMAND_READ          0xA0
#define GDC_COMMAND_CURD          0xE0
#define GDC_COMMAND_LPRD          0xC0
#define GDC_COMMAND_DMAR          0xA4
#define GDC_COMMAND_DMAW          0x24

/* Master GDC *****************************************************************/

#define GDC1_IO_i_STATUS          0x60
#define GDC1_IO_i_DATA            0x62
#define GDC1_IO_i_MODE_FLIPFLOP1  0x68

#define GDC1_IO_o_PARAM           0x60
#define GDC1_IO_o_COMMAND         0x62
#define GDC1_IO_o_VSYNC           0x64 /* CRT interrupt reset */

#define GDC1_IO_o_MODE_FLIPFLOP1  0x68
    #define GDC1_MODE_VERTICAL_LINE    0x00 /* Character attribute */
    #define GDC1_MODE_SIMPLE_GRAPHICS  0x01
    #define GDC1_MODE_COLORED          0x02
    #define GDC1_MODE_MONOCHROME       0x03
    #define GDC1_MODE_COLS_80          0x04
    #define GDC1_MODE_COLS_40          0x05
    #define GDC1_MODE_ANK_6_8          0x06
    #define GDC1_MODE_ANK_7_13         0x07
    #define GDC1_MODE_LINES_400        0x08
    #define GDC1_MODE_LINES_200        0x09 /* Hide odd raster line */
    #define GDC1_MODE_KCG_CODE         0x0A /* CG access during V-SYNC */
    #define GDC1_MODE_KCG_BITMAP       0x0B
    #define GDC1_NVMW_PROTECT          0x0C
    #define GDC1_NVMW_UNPROTECT        0x0D /* Memory at TextVramSegment:(3FE2-3FFEh) */
    #define GDC1_MODE_DISPLAY_DISABLE  0x0E
    #define GDC1_MODE_DISPLAY_ENABLE   0x0F

#define GDC1_IO_o_BORDER_COLOR    0x6C /* PC-H98 */

/* Slave GDC ******************************************************************/

#define GDC2_IO_i_STATUS               0xA0
#define GDC2_IO_i_DATA                 0xA2
#define GDC2_IO_i_VIDEO_PAGE           0xA4
#define GDC2_IO_i_VIDEO_PAGE_ACCESS    0xA6
#define GDC2_IO_i_PALETTE_INDEX        0xA8
#define GDC2_IO_i_GREEN                0xAA
#define GDC2_IO_i_RED                  0xAC
#define GDC2_IO_i_BLUE                 0xAE
#define GDC2_IO_i_MODE_FLIPFLOP2       0x6A
#define GDC2_IO_i_MODE_FLIPFLOP3       0x6E

#define GDC2_IO_o_PARAM                0xA0
#define GDC2_IO_o_COMMAND              0xA2
#define GDC2_IO_o_VIDEO_PAGE           0xA4 /* Video page to display (invalid in 480 height mode) */
#define GDC2_IO_o_VIDEO_PAGE_ACCESS    0xA6 /* Video page to CPU access */
#define GDC2_IO_o_PALETTE_INDEX        0xA8
#define GDC2_IO_o_GREEN                0xAA
#define GDC2_IO_o_RED                  0xAC
#define GDC2_IO_o_BLUE                 0xAE

#define GDC2_IO_o_MODE_FLIPFLOP2  0x6A
    #define GDC2_MODE_COLORS_8         0x00
    #define GDC2_MODE_COLORS_16        0x01
    #define GDC2_MODE_GRCG             0x04
    #define GDC2_MODE_EGC              0x05
    #define GDC2_EGC_FF_PROTECT        0x06
    #define GDC2_EGC_FF_UNPROTECT      0x07 /* Unprotect the EGC F/F registers */
    #define GDC2_MODE_PEGS_DISABLE     0x20
    #define GDC2_MODE_PEGC_ENABLE      0x21
    // #define GDC2_MODE_              0x26
    // #define GDC2_MODE_              0x27
    // #define GDC2_MODE_              0x28
    // #define GDC2_MODE_              0x29
    // #define GDC2_MODE_              0x2A
    // #define GDC2_MODE_              0x2B
    // #define GDC2_MODE_              0x2C
    // #define GDC2_MODE_              0x2D
    #define GDC2_MODE_CRT              0x40
    #define GDC2_MODE_LCD              0x41
    // #define GDC2_MODE_VRAM_PLAIN    0x62 /* PC-H98 */
    // #define GDC2_MODE_VRAM_PACKED   0x63
    #define GDC2_MODE_LINES_400        0x68 /* 128 kB VRAM boundary */
    #define GDC2_MODE_LINES_800        0x69 /* 256 kB VRAM boundary */
    // #define GDC2_MODE_              0x6C
    // #define GDC2_MODE_              0x6D
    #define GDC2_CLOCK1_2_5MHZ         0x82
    #define GDC2_CLOCK1_5MHZ           0x83
    #define GDC2_CLOCK2_2_5MHZ         0x84
    #define GDC2_CLOCK2_5MHZ           0x85

#define GDC2_IO_o_MODE_FLIPFLOP3  0x6E
    // #define GDC2_MODE_              0x02
    // #define GDC2_MODE_              0x03
    // #define GDC2_MODE_              0x08
    // #define GDC2_MODE_              0x09
    // #define GDC2_MODE_              0x0A
    // #define GDC2_MODE_              0x0B
    // #define GDC2_MODE_              0x0C
    // #define GDC2_MODE_              0x0D
    // #define GDC2_MODE_              0x0E
    // #define GDC2_MODE_              0x0F
    // #define GDC2_MODE_              0x14
    // #define GDC2_MODE_              0x15

FORCEINLINE
VOID
WRITE_GDC1_COMMAND(UCHAR Command)
{
    while (!(READ_PORT_UCHAR((PUCHAR)GDC1_IO_i_STATUS) & GDC_STATUS_FIFO_EMPTY))
        NOTHING;

    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_COMMAND, Command);
}

FORCEINLINE
VOID
WRITE_GDC2_COMMAND(UCHAR Command)
{
    while (!(READ_PORT_UCHAR((PUCHAR)GDC2_IO_i_STATUS) & GDC_STATUS_FIFO_EMPTY))
        NOTHING;

    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_COMMAND, Command);
}

/* CRT Controller *************************************************************/

#define CRTC_IO_o_SCANLINE_START       0x70
#define CRTC_IO_o_SCANLINE_END         0x72
#define CRTC_IO_o_SCANLINE_BLANK_AT    0x74
#define CRTC_IO_o_SCANLINES            0x76
#define CRTC_IO_o_SUR                  0x78
#define CRTC_IO_o_SDR                  0x7A

/* GRCG (Graphic Charger) *****************************************************/

#define GRCG_IO_i_MODE                 0x7C
#define GRCG_IO_o_MODE                 0x7C
typedef union _GRCG_MODE_REGISTER
{
    struct
    {
        UCHAR DisablePlaneB:1;
        UCHAR DisablePlaneR:1;
        UCHAR DisablePlaneG:1;
        UCHAR DisablePlaneI:1;
        UCHAR Unused:2;

        UCHAR Mode:1;
#define GRCG_MODE_TILE_DIRECT_WRITE 0
#define GRCG_MODE_TILE_COMPARE_READ 0
#define GRCG_MODE_READ_MODIFY_WRITE 1

        UCHAR Enable:1;
    };
    UCHAR Bits;
} GRCG_MODE_REGISTER, *PGRCG_MODE_REGISTER;

#define GRCG_IO_o_TILE_PATTERN         0x7E

/* CG Window ******************************************************************/

#define KCG_IO_o_CHARCODE_HIGH      0xA1
#define KCG_IO_o_CHARCODE_LOW       0xA3
#define KCG_IO_o_LINE               0xA5
#define KCG_IO_o_PATTERN            0xA9

#define KCG_IO_i_PATTERN            0xA9
