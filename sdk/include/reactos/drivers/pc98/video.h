/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Graphics system header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/* Video memory ***************************************************************/

#define VRAM_NORMAL_PLANE_B      0xA8000 /* Blue */
#define VRAM_NORMAL_PLANE_G      0xB0000 /* Green */
#define VRAM_NORMAL_PLANE_R      0xB8000 /* Red */
#define VRAM_NORMAL_PLANE_I      0xE0000 /* Intensity */
#define VRAM_PLANE_SIZE          0x08000
#define VRAM_NORMAL_TEXT         0xA0000
#define VRAM_TEXT_ATTR_OFFSET    0x02000
#define VRAM_TEXT_SIZE           0x02000
#define VRAM_ATTR_SIZE           0x02000

#define PEGC_FRAMEBUFFER_PACKED  0xF00000
#define PEGC_FRAMEBUFFER_SIZE    0x080000

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

/* Operation type */
#define GDC_MOD_REPLACE           0x00
#define GDC_MOD_COMPLEMENT        0x01 /* XOR */
#define GDC_MOD_CLEAR             0x02 /* AND */
#define GDC_MOD_SET               0x03 /* OR */

#define GDC_GRAPHICS_DRAWING      0x40

#define GDC_COMMAND_RESET1        0x00
#define GDC_COMMAND_RESET2        0x01
#define GDC_COMMAND_STOP2         0x05
#define GDC_COMMAND_RESET3        0x09
#define GDC_COMMAND_BCTRL_STOP    0x0C
#define GDC_COMMAND_BCTRL_START   0x0D

#define GDC_COMMAND_SYNC_ON       0x0E
typedef struct _SYNCPARAM
{
    UCHAR Flags;
#define SYNC_DISPLAY_MODE_GRAPHICS_AND_CHARACTERS                 0x00
#define SYNC_DISPLAY_MODE_GRAPHICS                                0x02
#define SYNC_DISPLAY_MODE_CHARACTERS                              0x20

#define SYNC_VIDEO_FRAMING_NONINTERLACED                          0x00
#define SYNC_VIDEO_FRAMING_INTERLACED_REPEAT_FOR_CHARACTERS       0x08
#define SYNC_VIDEO_FRAMING_INTERLACED                             0x09

#define SYNC_DRAW_DURING_ACTIVE_DISPLAY_TIME_AND_RETRACE_BLANKING 0x00
#define SYNC_DRAW_ONLY_DURING_RETRACE_BLANKING                    0x10

#define SYNC_STATIC_RAM_NO_REFRESH                                0x00
#define SYNC_DYNAMIC_RAM_REFRESH                                  0x04

    UCHAR ScreenWidthChars;
    UCHAR HorizontalSyncWidth;
    UCHAR VerticalSyncWidth;
    UCHAR HorizontalFrontPorchWidth;
    UCHAR HorizontalBackPorchWidth;
    UCHAR VerticalFrontPorchWidth;
    USHORT ScreenWidthLines;
    UCHAR VerticalBackPorchWidth;
} SYNCPARAM, *PSYNCPARAM;

FORCEINLINE
VOID
WRITE_GDC_SYNC(PUCHAR Port, PSYNCPARAM SyncParameters)
{
    WRITE_PORT_UCHAR(Port, SyncParameters->Flags & 0x3F);
    WRITE_PORT_UCHAR(Port, SyncParameters->ScreenWidthChars - 2);
    WRITE_PORT_UCHAR(Port, (SyncParameters->VerticalSyncWidth & 0x07) << 5 |
                     (SyncParameters->HorizontalSyncWidth - 1));
    WRITE_PORT_UCHAR(Port, ((SyncParameters->HorizontalFrontPorchWidth - 1) << 2) |
                     ((SyncParameters->VerticalSyncWidth & 0x18) >> 3));
    WRITE_PORT_UCHAR(Port, SyncParameters->HorizontalBackPorchWidth - 1);
    WRITE_PORT_UCHAR(Port, SyncParameters->VerticalFrontPorchWidth);
    WRITE_PORT_UCHAR(Port, SyncParameters->ScreenWidthLines & 0xFF);
    WRITE_PORT_UCHAR(Port, (SyncParameters->VerticalBackPorchWidth << 2) |
                     ((SyncParameters->ScreenWidthLines & 0x300) >> 8));
}

#define GDC_COMMAND_SYNC_OFF      0x0F
#define GDC_COMMAND_WRITE         0x20
#define GDC_COMMAND_DMAW          0x24

#define GDC_COMMAND_ZOOM          0x46
typedef struct _ZOOMPARAM
{
    UCHAR DisplayZoomFactor;
    UCHAR WritingZoomFactor;
} ZOOMPARAM, *PZOOMPARAM;

FORCEINLINE
VOID
WRITE_GDC_ZOOM(PUCHAR Port, PZOOMPARAM ZoomParameters)
{
    WRITE_PORT_UCHAR(Port, ZoomParameters->DisplayZoomFactor << 4 | ZoomParameters->WritingZoomFactor);
}

#define GDC_COMMAND_PITCH         0x47
typedef struct _PITCHPARAM
{
    ULONG WordsPerScanline;
} PITCHPARAM, *PPITCHPARAM;

FORCEINLINE
VOID
WRITE_GDC_PITCH(PUCHAR Port, PPITCHPARAM PitchParameters)
{
    WRITE_PORT_UCHAR(Port, PitchParameters->WordsPerScanline);
}

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

#define GDC_COMMAND_MASK          0x4A

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

#define GDC_COMMAND_FIGS          0x4C
#define GDC_COMMAND_GCHRD         0x68
#define GDC_COMMAND_START         0x6B
#define GDC_COMMAND_FIGD          0x6C
#define GDC_COMMAND_SLAVE         0x6E
#define GDC_COMMAND_MASTER        0x6F

#define GDC_COMMAND_PRAM          0x70
typedef struct _PRAMPARAM
{
    ULONG StartingAddress;
    USHORT Length;
    BOOLEAN ImageBit;
    BOOLEAN WideDisplay;
} PRAMPARAM, *PPRAMPARAM;

FORCEINLINE
VOID
WRITE_GDC_PRAM(PUCHAR Port, PPRAMPARAM RamParameters)
{
    WRITE_PORT_UCHAR(Port, RamParameters->StartingAddress & 0xFF);
    WRITE_PORT_UCHAR(Port, (RamParameters->StartingAddress >> 8) & 0xFF);
    WRITE_PORT_UCHAR(Port, ((RamParameters->Length & 0x0F) << 4) | ((RamParameters->StartingAddress >> 16) & 0x03));
    WRITE_PORT_UCHAR(Port, ((RamParameters->WideDisplay & 0x01) << 7) | ((RamParameters->ImageBit & 0x01) << 6) |
                            ((RamParameters->Length >> 4) & 0x3F));
}

#define GDC_COMMAND_TEXTW         0x78
#define GDC_COMMAND_READ          0xA0
#define GDC_COMMAND_DMAR          0xA4
#define GDC_COMMAND_LPRD          0xC0
#define GDC_COMMAND_CURD          0xE0

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
    #define GRAPH_MODE_COLORED         0x02
    #define GRAPH_MODE_MONOCHROME      0x03
    #define GDC1_MODE_COLS_80          0x04
    #define GDC1_MODE_COLS_40          0x05
    #define GDC1_MODE_ANK_6_8          0x06
    #define GDC1_MODE_ANK_7_13         0x07
    #define GDC2_MODE_ODD_RLINE_SHOW   0x08
    #define GDC2_MODE_ODD_RLINE_HIDE   0x09
    #define GDC1_MODE_KCG_CODE         0x0A /* CG access during V-SYNC */
    #define GDC1_MODE_KCG_BITMAP       0x0B
    #define GDC1_NVRAM_PROTECT         0x0C
    #define GDC1_NVRAM_UNPROTECT       0x0D /* Memory at TextVramSegment:(3FE2-3FFEh) */
    #define GRAPH_MODE_DISPLAY_DISABLE 0x0E
    #define GRAPH_MODE_DISPLAY_ENABLE  0x0F

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
    #define GDC2_MODE_PEGC_DISABLE     0x20
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
    // #define GDC2_MODE_VRAM_PLANAR   0x62 /* PC-H98 */
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

/* Miscellaneous **************************************************************/

#define GRAPH_IO_i_STATUS                 0x9A0
    #define GRAPH_STATUS_SET                   0x01
    #define GRAPH_GDC_CLOCK2_5MHZ              0x02

#define GRAPH_IO_o_STATUS_SELECT          0x9A0
    #define GRAPH_STATUS_GDC_CLOCK1_5MHZ       0x09
    #define GRAPH_STATUS_PEGC                  0x0A

#define GRAPH_IO_i_DPMS                   0x9A2
#define GRAPH_IO_o_DPMS                   0x9A2

#define GRAPH_IO_i_HORIZONTAL_SCAN_RATE   0x9A8
#define GRAPH_IO_o_HORIZONTAL_SCAN_RATE   0x9A8
    #define GRAPH_HF_24KHZ                     0x00
    #define GRAPH_HF_31KHZ                     0x01

#define GRAPH_IO_i_RELAY                  0xFAC
    #define GRAPH_RELAY_0                      0x01
    #define GRAPH_RELAY_1                      0x02

#define GRAPH_IO_o_RELAY                  0xFAC
    /* Relay 0 */
    #define GRAPH_VID_SRC_INTERNAL             0x00
    #define GRAPH_VID_SRC_EXTERNAL             0x01
    /* Relay 1 */
    #define GRAPH_SRC_GDC                      0x00
    #define GRAPH_SRC_WAB                      0x02

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
    #define GRCG_DISABLE                   0x00
    #define GRCG_ENABLE                    0x80
    #define GRCG_MODE_TILE_DIRECT_WRITE    0x80
    #define GRCG_MODE_TILE_COMPARE_READ    0x80
    #define GRCG_MODE_READ_MODIFY_WRITE    0xC0

#define GRCG_IO_o_TILE_PATTERN         0x7E

/* CG Window ******************************************************************/

#define KCG_IO_o_CHARCODE_HIGH      0xA1
#define KCG_IO_o_CHARCODE_LOW       0xA3
#define KCG_IO_o_LINE               0xA5
#define KCG_IO_o_PATTERN            0xA9

#define KCG_IO_i_PATTERN            0xA9

/* EGC blitter ****************************************************************/

#define EGC_IO_o_PLANE_ACCESS            0x4A0
#define EGC_IO_o_PATTERN_DATA_PLANE_READ 0x4A2
#define EGC_IO_o_READ_WRITE_MODE         0x4A4
#define EGC_IO_o_FG_COLOR                0x4A6
#define EGC_IO_o_MASK                    0x4A8
#define EGC_IO_o_BG_COLOR                0x4AA
#define EGC_IO_o_BIT_ADDRESS             0x4AC
#define EGC_IO_o_BIT_LENGTH              0x4AE

#define PEGC_MMIO_BANK_0          0x004
#define PEGC_MMIO_BANK_1          0x006

#define PEGC_MMIO_MODE            0x100
    #define PEGC_MODE_PACKED           0x00
    #define PEGC_MODE_PLANAR           0x01

#define PEGC_MMIO_FRAMEBUFFER     0x102
    #define PEGC_FB_UNMAP              0x00
    #define PEGC_FB_MAP                0x01
    #define PEGC_FB_UNKNOWN1           0x02
    #define PEGC_FB_UNKNOWN2           0x03

#define PEGC_MMIO_PLANE_ACCESS    0x104
#define PEGC_MMIO_ROP             0x108
#define PEGC_MMIO_DATA_SELECT     0x10A
#define PEGC_MMIO_MASK            0x10C
#define PEGC_MMIO_BIT_LENGTH      0x110
#define PEGC_MMIO_BIT_ADDRESS     0x112
#define PEGC_MMIO_FG_COLOR        0x114
#define PEGC_MMIO_BG_COLOR        0x118
#define PEGC_MMIO_ROP_PATTERN     0x120
