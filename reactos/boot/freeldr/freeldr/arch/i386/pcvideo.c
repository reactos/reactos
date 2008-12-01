/* $Id$
 *
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

/* non-standard specifier from windef.h -- please deprecate */
#undef PACKED
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#endif

#define VIDEOPORT_PALETTE_READ		0x03C7
#define VIDEOPORT_PALETTE_WRITE		0x03C8
#define VIDEOPORT_PALETTE_DATA		0x03C9
#define VIDEOPORT_VERTICAL_RETRACE	0x03DA

#define VIDEOVGA_MEM_ADDRESS  0xA0000
#define VIDEOTEXT_MEM_ADDRESS 0xB8000
#define VIDEOTEXT_MEM_SIZE    0x8000

#define	VIDEOCARD_CGA_OR_OTHER 0
#define VIDEOCARD_EGA          1
#define VIDEOCARD_VGA          2

#define VIDEOMODE_NORMAL_TEXT   0
#define VIDEOMODE_EXTENDED_TEXT 1
#define	VIDEOMODE_80X28         0x501C
#define	VIDEOMODE_80X30         0x501E
#define	VIDEOMODE_80X34         0x5022
#define	VIDEOMODE_80X43         0x502B
#define	VIDEOMODE_80X60         0x503C
#define	VIDEOMODE_132X25        0x8419
#define	VIDEOMODE_132X43        0x842B
#define	VIDEOMODE_132X50        0x8432
#define	VIDEOMODE_132X60        0x843C

#define VERTRES_200_SCANLINES		0x00
#define VERTRES_350_SCANLINES		0x01
#define VERTRES_400_SCANLINES		0x02

typedef struct
{
  USHORT ModeAttributes;             /* mode attributes (see #00080) */
  UCHAR  WindowAttributesA;          /* window attributes, window A (see #00081) */
  UCHAR  WindowsAttributesB;         /* window attributes, window B (see #00081) */
  USHORT WindowGranularity;          /* window granularity in KB */
  USHORT WindowSize;                 /* window size in KB */
  USHORT WindowAStartSegment;        /* start segment of window A (0000h if not supported) */
  USHORT WindowBStartSegment;        /* start segment of window B (0000h if not supported) */
  ULONG WindowPositioningFunction;  /* -> FAR window positioning function (equivalent to AX=4F05h) */
  USHORT BytesPerScanLine;           /* bytes per scan line */
  /* ---remainder is optional for VESA modes in v1.0/1.1, needed for OEM modes--- */
  USHORT WidthInPixels;              /* width in pixels (graphics) or characters (text) */
  USHORT HeightInPixels;             /* height in pixels (graphics) or characters (text) */
  UCHAR  CharacterWidthInPixels;     /* width of character cell in pixels */
  UCHAR  CharacterHeightInPixels;    /* height of character cell in pixels */
  UCHAR  NumberOfMemoryPlanes;       /* number of memory planes */
  UCHAR  BitsPerPixel;               /* number of bits per pixel */
  UCHAR  NumberOfBanks;              /* number of banks */
  UCHAR  MemoryModel;                /* memory model type (see #00082) */
  UCHAR  BankSize;                   /* size of bank in KB */
  UCHAR  NumberOfImagePanes;         /* number of image pages (less one) that will fit in video RAM */
  UCHAR  Reserved1;                  /* reserved (00h for VBE 1.0-2.0, 01h for VBE 3.0) */
  /* ---VBE v1.2+ --- */
  UCHAR  RedMaskSize;                /* red mask size */
  UCHAR  RedMaskPosition;            /* red field position */
  UCHAR  GreenMaskSize;              /* green mask size */
  UCHAR  GreenMaskPosition;          /* green field size */
  UCHAR  BlueMaskSize;               /* blue mask size */
  UCHAR  BlueMaskPosition;           /* blue field size */
  UCHAR  ReservedMaskSize;           /* reserved mask size */
  UCHAR  ReservedMaskPosition;       /* reserved mask position */
  UCHAR  DirectColorModeInfo;        /* direct color mode info */
                                  /* bit 0:Color ramp is programmable */
                                  /* bit 1:Bytes in reserved field may be used by application */
  /* ---VBE v2.0+ --- */
  ULONG LinearVideoBufferAddress;   /* physical address of linear video buffer */
  ULONG OffscreenMemoryPointer;     /* pointer to start of offscreen memory */
  USHORT OffscreenMemorySize;        /* KB of offscreen memory */
  /* ---VBE v3.0 --- */
  USHORT LinearBytesPerScanLine;     /* bytes per scan line in linear modes */
  UCHAR  BankedNumberOfImages;       /* number of images (less one) for banked video modes */
  UCHAR  LinearNumberOfImages;       /* number of images (less one) for linear video modes */
  UCHAR  LinearRedMaskSize;          /* linear modes:Size of direct color red mask (in bits) */
  UCHAR  LinearRedMaskPosition;      /* linear modes:Bit position of red mask LSB (e.g. shift count) */
  UCHAR  LinearGreenMaskSize;        /* linear modes:Size of direct color green mask (in bits) */
  UCHAR  LinearGreenMaskPosition;    /* linear modes:Bit position of green mask LSB (e.g. shift count) */
  UCHAR  LinearBlueMaskSize;         /* linear modes:Size of direct color blue mask (in bits) */
  UCHAR  LinearBlueMaskPosition;     /* linear modes:Bit position of blue mask LSB (e.g. shift count) */
  UCHAR  LinearReservedMaskSize;     /* linear modes:Size of direct color reserved mask (in bits) */
  UCHAR  LinearReservedMaskPosition; /* linear modes:Bit position of reserved mask LSB */
  ULONG MaximumPixelClock;          /* maximum pixel clock for graphics video mode, in Hz */
  UCHAR  Reserved2[190];             /* 190 BYTEs  reserved (0) */
} PACKED SVGA_MODE_INFORMATION, *PSVGA_MODE_INFORMATION;

static ULONG BiosVideoMode;                              /* Current video mode as known by BIOS */
static ULONG ScreenWidth = 80;	                       /* Screen Width in characters */
static ULONG ScreenHeight = 25;                          /* Screen Height in characters */
static ULONG BytesPerScanLine = 160;                     /* Number of bytes per scanline (delta) */
static VIDEODISPLAYMODE DisplayMode = VideoTextMode;   /* Current display mode */
static BOOLEAN VesaVideoMode = FALSE;                     /* Are we using a VESA mode? */
static SVGA_MODE_INFORMATION VesaVideoModeInformation; /* Only valid when in VESA mode */
static ULONG CurrentMemoryBank = 0;                      /* Currently selected VESA bank */

static ULONG
PcVideoDetectVideoCard(VOID)
{
  REGS Regs;

  /* Int 10h AH=12h BL=10h
   * VIDEO - ALTERNATE FUNCTION SELECT (PS,EGA,VGA,MCGA) - GET EGA INFO
   *
   * AH = 12h
   * BL = 10h
   * Return:
   * BH = video state
   * 00h color mode in effect (I/O port 3Dxh)
   * 01h mono mode in effect (I/O port 3Bxh)
   * BL = installed memory (00h = 64K, 01h = 128K, 02h = 192K, 03h = 256K)
   * CH = feature connector bits
   * CL = switch settings
   * AH destroyed (at least by Tseng ET4000 BIOS v8.00n)
   *
   * Installation check;EGA
   */
  Regs.b.ah = 0x12;
  Regs.b.bl = 0x10;
  Int386(0x10, &Regs, &Regs);

  /* If BL is still equal to 0x10 then there is no EGA/VGA present */
  if (0x10 == Regs.b.bl)
    {
      return VIDEOCARD_CGA_OR_OTHER;
    }

  /* Int 10h AX=1A00h
   * VIDEO - GET DISPLAY COMBINATION CODE (PS,VGA/MCGA)
   *
   * AX = 1A00h
   * Return:
   * AL = 1Ah if function was supported
   * BL = active display code
   * BH = alternate display code
   *
   * This function is commonly used to check for the presence of a VGA.
   *
   * Installation check;VGA
   *
   * Values for display combination code:
   * 00h    no display
   * 01h    monochrome adapter w/ monochrome display
   * 02h    CGA w/ color display
   * 03h    reserved
   * 04h    EGA w/ color display
   * 05h    EGA w/ monochrome display
   * 06h    PGA w/ color display
   * 07h    VGA w/ monochrome analog display
   * 08h    VGA w/ color analog display
   * 09h    reserved
   * 0Ah    MCGA w/ digital color display
   * 0Bh    MCGA w/ monochrome analog display
   * 0Ch    MCGA w/ color analog display
   * FFh    unknown display type
   */
  Regs.b.ah = 0x12;
  Regs.b.bl = 0x10;
  Int386(0x10, &Regs, &Regs);

  if (0x1a == Regs.b.al)
    {
      return VIDEOCARD_VGA;
    }
  else
    {
      return VIDEOCARD_EGA;
    }
}

static VOID PcVideoSetBiosMode(ULONG VideoMode)
{
  REGS Regs;

  /* Int 10h AH=00h
   * VIDEO - SET VIDEO MODE
   *
   * AH = 00h
   * AL = desired video mode
   * Return:
   * AL = video mode flag (Phoenix, AMI BIOS)
   * 20h mode > 7
   * 30h modes 0-5 and 7
   * 3Fh mode 6
   * AL = CRT controller mode byte (Phoenix 386 BIOS v1.10)
   */
  Regs.b.ah = 0x00;
  Regs.b.al = VideoMode;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoSetFont8x8(VOID)
{
  REGS Regs;

  /* Int 10h AX=1112h
   * VIDEO - TEXT-MODE CHARGEN - LOAD ROM 8x8 DBL-DOT PATTERNS (PS,EGA,VGA)
   *
   * AX = 1112h
   * BL = block to load
   * Return:
   * Nothing
   */
  Regs.w.ax = 0x1112;
  Regs.b.bl = 0x00;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoSetFont8x14(VOID)
{
  REGS Regs;

  /* Int 10h AX=1111h
   * VIDEO - TEXT-MODE CHARGEN - LOAD ROM MONOCHROME PATTERNS (PS,EGA,VGA)
   *
   * AX = 1111h
   * BL = block to load
   * Return:
   * Nothing
   */
  Regs.w.ax = 0x1111;
  Regs.b.bl = 0;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoSelectAlternatePrintScreen(VOID)
{
  REGS Regs;

  /* Int 10h AH=12h BL=20h
   * VIDEO - ALTERNATE FUNCTION SELECT (PS,EGA,VGA,MCGA) - ALTERNATE PRTSC
   *
   * AH = 12h
   * BL = 20h select alternate print screen routine
   * Return:
   * Nothing
   *
   * Installs a PrtSc routine from the video card's BIOS to replace the
   * default PrtSc handler from the ROM BIOS, which usually does not
   * understand screen heights other than 25 lines.
   *
   * Some adapters disable print-screen instead of enhancing it.
   */
  Regs.b.ah = 0x12;
  Regs.b.bl = 0x20;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoDisableCursorEmulation(VOID)
{
  REGS Regs;

  /* Int 10h AH=12h BL=34h
   * VIDEO - ALTERNATE FUNCTION SELECT (VGA) - CURSOR EMULATION
   *
   * AH = 12h
   * BL = 34h
   * AL = new state
   * 00h enable alphanumeric cursor emulation
   * 01h disable alphanumeric cursor emulation
   * Return:
   * AL = 12h if function supported
   *
   * Specify whether the BIOS should automatically remap cursor start/end
   * according to the current character height in text modes.
   */
  Regs.b.ah = 0x12;
  Regs.b.bl = 0x34;
  Regs.b.al = 0x01;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoDefineCursor(ULONG StartScanLine, ULONG EndScanLine)
{
  REGS Regs;

  /* Int 10h AH=01h
   * VIDEO - SET TEXT-MODE CURSOR SHAPE
   *
   * AH = 01h
   * CH = cursor start and options
   * CL = bottom scan line containing cursor (bits 0-4)
   * Return:
   * Nothing
   *
   * Specify the starting and ending scan lines to be occupied
   * by the hardware cursor in text modes.
   *
   * AMI 386 BIOS and AST Premier 386 BIOS will lock up the
   * system if AL is not equal to the current video mode.
   *
   * Bitfields for cursor start and options:
   *
   * Bit(s)    Description
   * 7         should be zero
   * 6,5       cursor blink
   * (00=normal, 01=invisible, 10=erratic, 11=slow).
   * (00=normal, other=invisible on EGA/VGA)
   * 4-0       topmost scan line containing cursor
   */
  Regs.b.ah = 0x01;
  Regs.b.al = 0x03;
  Regs.b.ch = StartScanLine;
  Regs.b.cl = EndScanLine;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoSetVerticalResolution(ULONG ScanLines)
{
  REGS Regs;

  /* Int 10h AH=12h BL=30h
   * VIDEO - ALTERNATE FUNCTION SELECT (VGA) - SELECT VERTICAL RESOLUTION
   *
   * AH = 12h
   * BL = 30h
   * AL = vertical resolution
   * 00h 200 scan lines
   * 01h 350 scan lines
   * 02h 400 scan lines
   * Return:
   * AL = 12h if function supported
   *
   * Specifiy the number of scan lines used to display text modes.
   *
   * The specified resolution will take effect on the next mode set.
   */
  Regs.b.ah = 0x12;
  Regs.b.bl = 0x30;
  Regs.b.al = ScanLines;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoSet480ScanLines(VOID)
{
  INT_PTR CRTC;

  /* Read CRTC port */
  CRTC = READ_PORT_UCHAR((PUCHAR)0x03CC);

  if (CRTC & 1)
    {
      CRTC = 0x3D4;
    }
  else
    {
      CRTC = 0x3B4;
    }

  /* Vertical sync end (also unlocks CR0-7) */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x11);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x0C);

  /* Vertical total */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x06);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x0B);

  /* (vertical) overflow */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x07);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x3E);

  /* Vertical sync start */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x10);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xEA);

  /* Vertical display end */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x12);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xDF);

  /* Vertical blank start */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x15);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xE7);

  /* Vertical blank end */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x16);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x04);

  /* Misc output register (read) */
  CRTC = READ_PORT_UCHAR((PUCHAR)0x03CC);

  /* Preserve clock select bits and color bit */
  CRTC = (CRTC & 0x0D);
  /* Set correct sync polarity */
  CRTC = (CRTC | 0xE2);

  /* (write) */
  WRITE_PORT_UCHAR((PUCHAR)0x03C2, CRTC);
}

static VOID
PcVideoSetDisplayEnd(VOID)
{
  INT_PTR	CRTC;

  /* Read CRTC port */
  CRTC = READ_PORT_UCHAR((PUCHAR)0x03CC);

  if (CRTC & 1)
    {
      CRTC = 0x3D4;
    }
  else
    {
      CRTC = 0x3B4;
    }

  /* Vertical display end */
  WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x12);
  WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xDF);
}

static BOOLEAN
PcVideoVesaGetSVGAModeInformation(USHORT Mode, PSVGA_MODE_INFORMATION ModeInformation)
{
  REGS Regs;

  RtlZeroMemory((PVOID)BIOSCALLBUFFER, 256);

  /* VESA SuperVGA BIOS - GET SuperVGA MODE INFORMATION
   * AX = 4F01h
   * CX = SuperVGA video mode (see #04082 for bitfields)
   * ES:DI -> 256-byte buffer for mode information (see #00079)
   * Return:
   * AL = 4Fh if function supported
   * AH = status
   * 00h successful
   * ES:DI buffer filled
   * 01h failed
   *
   * Desc: Determine the attributes of the specified video mode
   *
   * Note: While VBE 1.1 and higher will zero out all unused bytes
   * of the buffer, v1.0 did not, so applications that want to be
   * backward compatible should clear the buffer before calling
   */
  Regs.w.ax = 0x4F01;
  Regs.w.cx = Mode;
  Regs.w.es = BIOSCALLBUFSEGMENT;
  Regs.w.di = BIOSCALLBUFOFFSET;
  Int386(0x10, &Regs, &Regs);

  if (Regs.w.ax != 0x004F)
    {
      return FALSE;
    }

  RtlCopyMemory(ModeInformation, (PVOID)BIOSCALLBUFFER, sizeof(SVGA_MODE_INFORMATION));

  DbgPrint((DPRINT_UI, "\n"));
  DbgPrint((DPRINT_UI, "BiosVesaGetSVGAModeInformation() mode 0x%x\n", Mode));
  DbgPrint((DPRINT_UI, "ModeAttributes = 0x%x\n", ModeInformation->ModeAttributes));
  DbgPrint((DPRINT_UI, "WindowAttributesA = 0x%x\n", ModeInformation->WindowAttributesA));
  DbgPrint((DPRINT_UI, "WindowAttributesB = 0x%x\n", ModeInformation->WindowsAttributesB));
  DbgPrint((DPRINT_UI, "WindowGranularity = %dKB\n", ModeInformation->WindowGranularity));
  DbgPrint((DPRINT_UI, "WindowSize = %dKB\n", ModeInformation->WindowSize));
  DbgPrint((DPRINT_UI, "WindowAStartSegment = 0x%x\n", ModeInformation->WindowAStartSegment));
  DbgPrint((DPRINT_UI, "WindowBStartSegment = 0x%x\n", ModeInformation->WindowBStartSegment));
  DbgPrint((DPRINT_UI, "WindowPositioningFunction = 0x%x\n", ModeInformation->WindowPositioningFunction));
  DbgPrint((DPRINT_UI, "BytesPerScanLine = %d\n", ModeInformation->BytesPerScanLine));
  DbgPrint((DPRINT_UI, "WidthInPixels = %d\n", ModeInformation->WidthInPixels));
  DbgPrint((DPRINT_UI, "HeightInPixels = %d\n", ModeInformation->HeightInPixels));
  DbgPrint((DPRINT_UI, "CharacterWidthInPixels = %d\n", ModeInformation->CharacterWidthInPixels));
  DbgPrint((DPRINT_UI, "CharacterHeightInPixels = %d\n", ModeInformation->CharacterHeightInPixels));
  DbgPrint((DPRINT_UI, "NumberOfMemoryPlanes = %d\n", ModeInformation->NumberOfMemoryPlanes));
  DbgPrint((DPRINT_UI, "BitsPerPixel = %d\n", ModeInformation->BitsPerPixel));
  DbgPrint((DPRINT_UI, "NumberOfBanks = %d\n", ModeInformation->NumberOfBanks));
  DbgPrint((DPRINT_UI, "MemoryModel = %d\n", ModeInformation->MemoryModel));
  DbgPrint((DPRINT_UI, "BankSize = %d\n", ModeInformation->BankSize));
  DbgPrint((DPRINT_UI, "NumberOfImagePlanes = %d\n", ModeInformation->NumberOfImagePanes));
  DbgPrint((DPRINT_UI, "---VBE v1.2+ ---\n"));
  DbgPrint((DPRINT_UI, "RedMaskSize = %d\n", ModeInformation->RedMaskSize));
  DbgPrint((DPRINT_UI, "RedMaskPosition = %d\n", ModeInformation->RedMaskPosition));
  DbgPrint((DPRINT_UI, "GreenMaskSize = %d\n", ModeInformation->GreenMaskSize));
  DbgPrint((DPRINT_UI, "GreenMaskPosition = %d\n", ModeInformation->GreenMaskPosition));
  DbgPrint((DPRINT_UI, "BlueMaskSize = %d\n", ModeInformation->BlueMaskSize));
  DbgPrint((DPRINT_UI, "BlueMaskPosition = %d\n", ModeInformation->BlueMaskPosition));
  DbgPrint((DPRINT_UI, "ReservedMaskSize = %d\n", ModeInformation->ReservedMaskSize));
  DbgPrint((DPRINT_UI, "ReservedMaskPosition = %d\n", ModeInformation->ReservedMaskPosition));
  DbgPrint((DPRINT_UI, "\n"));

  return TRUE;
}

static BOOLEAN
PcVideoSetBiosVesaMode(USHORT Mode)
{
  REGS Regs;

  /* Int 10h AX=4F02h
   * VESA SuperVGA BIOS - SET SuperVGA VIDEO MODE
   *
   * AX = 4F02h
   * BX = new video mode
   * ES:DI -> (VBE 3.0+) CRTC information block, bit mode bit 11 set
   * Return:
   * AL = 4Fh if function supported
   * AH = status
   * 00h successful
   * 01h failed
   *
   * Values for VESA video mode:
   * 00h-FFh OEM video modes (see #00010 at AH=00h)
   * 100h   640x400x256
   * 101h   640x480x256
   * 102h   800x600x16
   * 103h   800x600x256
   * 104h   1024x768x16
   * 105h   1024x768x256
   * 106h   1280x1024x16
   * 107h   1280x1024x256
   * 108h   80x60 text
   * 109h   132x25 text
   * 10Ah   132x43 text
   * 10Bh   132x50 text
   * 10Ch   132x60 text
   * ---VBE v1.2+ ---
   * 10Dh   320x200x32K
   * 10Eh   320x200x64K
   * 10Fh   320x200x16M
   * 110h   640x480x32K
   * 111h   640x480x64K
   * 112h   640x480x16M
   * 113h   800x600x32K
   * 114h   800x600x64K
   * 115h   800x600x16M
   * 116h   1024x768x32K
   * 117h   1024x768x64K
   * 118h   1024x768x16M
   * 119h   1280x1024x32K (1:5:5:5)
   * 11Ah   1280x1024x64K (5:6:5)
   * 11Bh   1280x1024x16M
   * ---VBE 2.0+ ---
   * 120h   1600x1200x256
   * 121h   1600x1200x32K
   * 122h   1600x1200x64K
   * 81FFh   special full-memory access mode
   *
   * Notes: The special mode 81FFh preserves the contents of the video memory and gives
   * access to all of the memory; VESA recommends that the special mode be a packed-pixel
   * mode. For VBE 2.0+, it is required that the VBE implement the mode, but not place it
   * in the list of available modes (mode information for this mode can be queried
   * directly, however).. As of VBE 2.0, VESA will no longer define video mode numbers
   */
  Regs.w.ax = 0x4F02;
  Regs.w.bx = Mode;
  Int386(0x10, &Regs, &Regs);

  if (0x004F != Regs.w.ax)
    {
      return FALSE;
    }

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x25(VOID)
{
  PcVideoSetBiosMode(0x03);
  ScreenWidth = 80;
  ScreenHeight = 25;

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x50_80x43(VOID)
{
  if (VIDEOCARD_VGA == PcVideoDetectVideoCard())
    {
      PcVideoSetBiosMode(0x12);
      PcVideoSetFont8x8();
      PcVideoSelectAlternatePrintScreen();
      PcVideoDisableCursorEmulation();
      PcVideoDefineCursor(6, 7);
      ScreenWidth = 80;
      ScreenHeight = 50;
    }
  else if (VIDEOCARD_EGA == PcVideoDetectVideoCard())
    {
      PcVideoSetBiosMode(0x03);
      PcVideoSetFont8x8();
      PcVideoSelectAlternatePrintScreen();
      PcVideoDisableCursorEmulation();
      PcVideoDefineCursor(6, 7);
      ScreenWidth = 80;
      ScreenHeight = 43;
    }
  else /* VIDEOCARD_CGA_OR_OTHER */
    {
      return FALSE;
    }

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x28(VOID)
{
  /* FIXME: Is this VGA-only? */
  PcVideoSetMode80x25();
  PcVideoSetFont8x14();
  PcVideoDefineCursor(11, 12);
  ScreenWidth = 80;
  ScreenHeight = 28;

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x30(VOID)
{
  /* FIXME: Is this VGA-only? */
  PcVideoSetMode80x25();
  PcVideoSet480ScanLines();
  ScreenWidth = 80;
  ScreenHeight = 30;

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x34(VOID)
{
  /* FIXME: Is this VGA-only? */
  PcVideoSetMode80x25();
  PcVideoSet480ScanLines();
  PcVideoSetFont8x14();
  PcVideoDefineCursor(11, 12);
  PcVideoSetDisplayEnd();
  ScreenWidth = 80;
  ScreenHeight = 34;

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x43(VOID)
{
  /* FIXME: Is this VGA-only? */
  PcVideoSetVerticalResolution(VERTRES_350_SCANLINES);
  PcVideoSetMode80x25();
  PcVideoSetFont8x8();
  PcVideoSelectAlternatePrintScreen();
  PcVideoDisableCursorEmulation();
  PcVideoDefineCursor(6, 7);
  ScreenWidth = 80;
  ScreenHeight = 43;

  return TRUE;
}

static BOOLEAN
PcVideoSetMode80x60(VOID)
{
  /* FIXME: Is this VGA-only? */
  PcVideoSetMode80x25();
  PcVideoSet480ScanLines();
  PcVideoSetFont8x8();
  PcVideoSelectAlternatePrintScreen();
  PcVideoDisableCursorEmulation();
  PcVideoDefineCursor(6, 7);
  PcVideoSetDisplayEnd();
  ScreenWidth = 80;
  ScreenHeight = 60;

  return TRUE;
}

static BOOLEAN
PcVideoSetMode(ULONG NewMode)
{
  CurrentMemoryBank = 0;

  /* Set the values for the default text modes
   * If they are setting a graphics mode then
   * these values will be changed.
   */
  BiosVideoMode = NewMode;
  ScreenWidth = 80;
  ScreenHeight = 25;
  BytesPerScanLine = 160;
  DisplayMode = VideoTextMode;
  VesaVideoMode = FALSE;

  switch (NewMode)
    {
      case VIDEOMODE_NORMAL_TEXT:
      case 0x03: /* BIOS 80x25 text mode number */
        return PcVideoSetMode80x25();
      case VIDEOMODE_EXTENDED_TEXT:
        return PcVideoSetMode80x50_80x43();
      case VIDEOMODE_80X28:
        return PcVideoSetMode80x28();
      case VIDEOMODE_80X30:
        return PcVideoSetMode80x30();
      case VIDEOMODE_80X34:
        return PcVideoSetMode80x34();
      case VIDEOMODE_80X43:
        return PcVideoSetMode80x43();
      case VIDEOMODE_80X60:
        return PcVideoSetMode80x60();
   }

  if (0x12 == NewMode)
    {
      /* 640x480x16 */
      PcVideoSetBiosMode(NewMode);
      WRITE_PORT_USHORT((USHORT*)0x03CE, 0x0F01); /* For some reason this is necessary? */
      ScreenWidth = 640;
      ScreenHeight = 480;
      BytesPerScanLine = 80;
      BiosVideoMode = NewMode;
      DisplayMode = VideoGraphicsMode;

      return TRUE;
    }
  else if (0x13 == NewMode)
    {
      /* 320x200x256 */
      PcVideoSetBiosMode(NewMode);
      ScreenWidth = 320;
      ScreenHeight = 200;
      BytesPerScanLine = 320;
      BiosVideoMode = NewMode;
      DisplayMode = VideoGraphicsMode;

      return TRUE;
    }
  else if (0x0108 <= NewMode && NewMode <= 0x010C)
    {
      /* VESA Text Mode */
      if (! PcVideoVesaGetSVGAModeInformation(NewMode, &VesaVideoModeInformation))
        {
          return FALSE;
        }

      if (! PcVideoSetBiosVesaMode(NewMode))
        {
          return FALSE;
        }

      ScreenWidth = VesaVideoModeInformation.WidthInPixels;
      ScreenHeight = VesaVideoModeInformation.HeightInPixels;
      BytesPerScanLine = VesaVideoModeInformation.BytesPerScanLine;
      BiosVideoMode = NewMode;
      DisplayMode = VideoTextMode;
      VesaVideoMode = TRUE;

      return TRUE;
    }
  else
    {
      /* VESA Graphics Mode */
      if (! PcVideoVesaGetSVGAModeInformation(NewMode, &VesaVideoModeInformation))
        {
          return FALSE;
        }

      if (! PcVideoSetBiosVesaMode(NewMode))
        {
          return FALSE;
        }

      ScreenWidth = VesaVideoModeInformation.WidthInPixels;
      ScreenHeight = VesaVideoModeInformation.HeightInPixels;
      BytesPerScanLine = VesaVideoModeInformation.BytesPerScanLine;
      BiosVideoMode = NewMode;
      DisplayMode = VideoTextMode;
      VesaVideoMode = TRUE;

      return TRUE;
    }

  return FALSE;
}

static VOID
PcVideoSetBlinkBit(BOOLEAN Enable)
{
  REGS Regs;

  /* Int 10h AX=1003h
   * VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA)
   *
   * AX = 1003h
   * BL = new state
   * 00h background intensity enabled
   * 01h blink enabled
   * BH = 00h to avoid problems on some adapters
   * Return:
   * Nothing
   *
   * Note: although there is no function to get
   * the current status, bit 5 of 0040h:0065h
   * indicates the state.
   */
  Regs.w.ax = 0x1003;
  Regs.w.bx = Enable ? 0x0001 : 0x0000;
  Int386(0x10, &Regs, &Regs);
}

static VOID
PcVideoSetMemoryBank(USHORT BankNumber)
{
  REGS Regs;

  if (CurrentMemoryBank != BankNumber)
    {
      /* Int 10h AX=4F05h
       * VESA SuperVGA BIOS - CPU VIDEO MEMORY CONTROL
       *
       * AX = 4F05h
       * BH = subfunction
       * 00h select video memory window
       * 01h get video memory window
       * DX = window address in video memory (in granularity units)
       * Return:
       * DX = window address in video memory (in gran. units)
       * BL = window number
       * 00h window A
       * 01h window B.
       * Return:
       * AL = 4Fh if function supported
       * AH = status
       * 00h successful
       * 01h failed
       */
      Regs.w.ax = 0x4F05;
      Regs.w.bx = 0x0000;
      Regs.w.dx = BankNumber;
      Int386(0x10, &Regs, &Regs);

      if (0x004F == Regs.w.ax)
        {
          CurrentMemoryBank = BankNumber;
	}
    }
}

VIDEODISPLAYMODE
PcVideoSetDisplayMode(char *DisplayModeName, BOOLEAN Init)
{
  ULONG VideoMode = VIDEOMODE_NORMAL_TEXT;

  if (NULL == DisplayModeName || '\0' == *DisplayModeName)
    {
      PcVideoSetBlinkBit(! Init);
      return DisplayMode;
    }

  if (VIDEOCARD_CGA_OR_OTHER == PcVideoDetectVideoCard())
    {
      DbgPrint((DPRINT_UI, "CGA or other display adapter detected.\n"));
      printf("CGA or other display adapter detected.\n");
      printf("Using 80x25 text mode.\n");
      VideoMode = VIDEOMODE_NORMAL_TEXT;
    }
  else if (VIDEOCARD_EGA == PcVideoDetectVideoCard())
    {
      DbgPrint((DPRINT_UI, "EGA display adapter detected.\n"));
      printf("EGA display adapter detected.\n");
      printf("Using 80x25 text mode.\n");
      VideoMode = VIDEOMODE_NORMAL_TEXT;
    }
  else /* if (VIDEOCARD_VGA == PcVideoDetectVideoCard()) */
    {
      DbgPrint((DPRINT_UI, "VGA display adapter detected.\n"));

      if (0 == _stricmp(DisplayModeName, "NORMAL_VGA"))
        {
          VideoMode = VIDEOMODE_NORMAL_TEXT;
        }
      else if (0 == _stricmp(DisplayModeName, "EXTENDED_VGA"))
        {
          VideoMode = VIDEOMODE_EXTENDED_TEXT;
        }
      else
        {
          VideoMode = atoi(DisplayModeName);
        }
    }

  if (! PcVideoSetMode(VideoMode))
    {
      printf("Error: unable to set video display mode 0x%x\n", (int) VideoMode);
      printf("Defaulting to 80x25 text mode.\n");
      printf("Press any key to continue.\n");
      PcConsGetCh();

      PcVideoSetMode(VIDEOMODE_NORMAL_TEXT);
    }

  PcVideoSetBlinkBit(! Init);


  return DisplayMode;
}

VOID
PcVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
  *Width = ScreenWidth;
  *Height = ScreenHeight;
  if (VideoGraphicsMode == DisplayMode && VesaVideoMode)
    {
      if (16 == VesaVideoModeInformation.BitsPerPixel)
        {
          /* 16-bit color modes give green an extra bit (5:6:5)
           * 15-bit color modes have just 5:5:5 for R:G:B */
          *Depth = (6 == VesaVideoModeInformation.GreenMaskSize ? 16 : 15);
        }
      else
        {
          *Depth = VesaVideoModeInformation.BitsPerPixel;
        }
    }
  else
    {
      *Depth = 0;
    }
}

ULONG
PcVideoGetBufferSize(VOID)
{
  return ScreenHeight * BytesPerScanLine;
}

VOID
PcVideoSetTextCursorPosition(ULONG X, ULONG Y)
{
  REGS Regs;

  /* Int 10h AH=02h
   * VIDEO - SET CURSOR POSITION
   *
   * AH = 02h
   * BH = page number
   * 0-3 in modes 2&3
   * 0-7 in modes 0&1
   * 0 in graphics modes
   * DH = row (00h is top)
   * DL = column (00h is left)
   * Return:
   * Nothing
   */
  Regs.b.ah = 0x02;
  Regs.b.bh = 0x00;
  Regs.b.dh = Y;
  Regs.b.dl = X;
  Int386(0x10, &Regs, &Regs);
}

VOID
PcVideoHideShowTextCursor(BOOLEAN Show)
{
  if (Show)
    {
      PcVideoDefineCursor(0x0D, 0x0E);
    }
  else
    {
      PcVideoDefineCursor(0x20, 0x00);
    }
}

VOID
PcVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  ULONG BanksToCopy;
  ULONG BytesInLastBank;
  ULONG CurrentBank;
  ULONG BankSize;

  /* PcVideoWaitForVerticalRetrace(); */

  /* Text mode (BIOS or VESA) */
  if (VideoTextMode == DisplayMode)
    {
      RtlCopyMemory((PVOID) VIDEOTEXT_MEM_ADDRESS, Buffer, PcVideoGetBufferSize());
    }
  /* VESA graphics mode */
  else if (VideoGraphicsMode == DisplayMode && VesaVideoMode)
    {
      BankSize = VesaVideoModeInformation.WindowGranularity << 10;
      BanksToCopy = (VesaVideoModeInformation.HeightInPixels * VesaVideoModeInformation.BytesPerScanLine) / BankSize;
      BytesInLastBank = (VesaVideoModeInformation.HeightInPixels * VesaVideoModeInformation.BytesPerScanLine) % BankSize;

      /* Copy all the banks but the last one because
       * it is probably a partial bank */
      for (CurrentBank = 0; CurrentBank < BanksToCopy; CurrentBank++)
        {
          PcVideoSetMemoryBank(CurrentBank);
          RtlCopyMemory((PVOID) VIDEOVGA_MEM_ADDRESS, (char *) Buffer + CurrentBank * BankSize, BankSize);
        }

      /* Copy the remaining bytes into the last bank */
      PcVideoSetMemoryBank(CurrentBank);
      RtlCopyMemory((PVOID)VIDEOVGA_MEM_ADDRESS, (char *) Buffer + CurrentBank * BankSize, BytesInLastBank);
    }
  /* BIOS graphics mode */
  else
    {
      UNIMPLEMENTED;
    }
}

VOID
PcVideoClearScreen(UCHAR Attr)
{
  USHORT AttrChar;
  USHORT *BufPtr;

  AttrChar = ((USHORT) Attr << 8) | ' ';
  for (BufPtr = (USHORT *) VIDEOTEXT_MEM_ADDRESS;
       BufPtr < (USHORT *) (VIDEOTEXT_MEM_ADDRESS + VIDEOTEXT_MEM_SIZE);
       BufPtr++)
    {
      *BufPtr = AttrChar;
    }
}

VOID
PcVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
  USHORT *BufPtr;

  BufPtr = (USHORT *) (ULONG_PTR)(VIDEOTEXT_MEM_ADDRESS + Y * BytesPerScanLine + X * 2);
  *BufPtr = ((USHORT) Attr << 8) | (Ch & 0xff);
}

BOOLEAN
PcVideoIsPaletteFixed(VOID)
{
  return FALSE;
}

VOID
PcVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
  WRITE_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_WRITE, Color);
  WRITE_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_DATA, Red);
  WRITE_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_DATA, Green);
  WRITE_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_DATA, Blue);
}

VOID
PcVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
{
  WRITE_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_READ, Color);
  *Red = READ_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_DATA);
  *Green = READ_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_DATA);
  *Blue = READ_PORT_UCHAR((UCHAR*) VIDEOPORT_PALETTE_DATA);
}

VOID
PcVideoSync(VOID)
{
  while (1 == (READ_PORT_UCHAR((UCHAR*)VIDEOPORT_VERTICAL_RETRACE) & 0x08))
    {
      /*
       * Keep reading the port until bit 3 is clear
       * This waits for the current retrace to end and
       * we can catch the next one so we know we are
       * getting a full retrace.
       */
    }

  while (0 == (READ_PORT_UCHAR((UCHAR*)VIDEOPORT_VERTICAL_RETRACE) & 0x08))
    {
      /*
       * Keep reading the port until bit 3 is set
       * Now that we know we aren't doing a vertical
       * retrace we need to wait for the next one.
       */
    }
}

VOID
PcVideoPrepareForReactOS(IN BOOLEAN Setup)
{
    if (Setup)
    {
        PcVideoSetMode80x50_80x43();
    }
    else
    {
        PcVideoSetBiosMode(0x12);
    }
    PcVideoHideShowTextCursor(FALSE);
}

/* EOF */
