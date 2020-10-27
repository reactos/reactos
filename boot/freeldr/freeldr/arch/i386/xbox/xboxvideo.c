/*
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Note: much of this code was based on knowledge and/or code developed
 * by the Xbox Linux group: http://www.xbox-linux.org
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

ULONG NvBase = 0xFD000000;
PVOID FrameBuffer;
ULONG FrameBufferSize;
static ULONG ScreenWidth;
static ULONG ScreenHeight;
static ULONG BytesPerPixel;
static ULONG Delta;
extern multiboot_info_t * MultibootInfoPtr;

UCHAR MachDefaultTextColor = COLOR_GRAY;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define TOP_BOTTOM_LINES 0

#define FB_SIZE_MB 4

#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

static VOID
XboxVideoOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
  PUCHAR FontPtr;
  PULONG Pixel;
  UCHAR Mask;
  unsigned Line;
  unsigned Col;

  FontPtr = BitmapFont8x16 + Char * 16;
  Pixel = (PULONG) ((char *) FrameBuffer + (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta
                  + X * CHAR_WIDTH * BytesPerPixel);
  for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
      Mask = 0x80;
      for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
          Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
          Mask = Mask >> 1;
        }
      Pixel = (PULONG) ((char *) Pixel + Delta);
    }
}

static ULONG
XboxVideoAttrToSingleColor(UCHAR Attr)
{
  UCHAR Intensity;

  Intensity = (0 == (Attr & 0x08) ? 127 : 255);

  return 0xff000000 |
         (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
         (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
         (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
XboxVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
  *FgColor = XboxVideoAttrToSingleColor(Attr & 0xf);
  *BgColor = XboxVideoAttrToSingleColor((Attr >> 4) & 0xf);
}

static VOID
XboxVideoClearScreenColor(ULONG Color, BOOLEAN FullScreen)
{
  ULONG Line, Col;
  PULONG p;

  for (Line = 0; Line < ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
      p = (PULONG) ((char *) FrameBuffer + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
      for (Col = 0; Col < ScreenWidth; Col++)
        {
          *p++ = Color;
        }
    }
}

VOID
XboxVideoScrollUp(VOID)
{
    ULONG BgColor, Dummy;
    ULONG PixelCount = ScreenWidth * CHAR_HEIGHT *
                       (((ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    PULONG Src = (PULONG)((PUCHAR)FrameBuffer + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta);
    PULONG Dst = (PULONG)((PUCHAR)FrameBuffer + TOP_BOTTOM_LINES * Delta);

    XboxVideoAttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

VOID
XboxVideoClearScreen(UCHAR Attr)
{
  ULONG FgColor, BgColor;

  XboxVideoAttrToColors(Attr, &FgColor, &BgColor);

  XboxVideoClearScreenColor(BgColor, FALSE);
}

VOID
XboxVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
  ULONG FgColor, BgColor;

  XboxVideoAttrToColors(Attr, &FgColor, &BgColor);

  XboxVideoOutputChar(Ch, X, Y, FgColor, BgColor);
}

UCHAR
NvGetCrtc(UCHAR Index)
{
    WRITE_REGISTER_UCHAR(NvBase + NV2A_CRTC_REGISTER_INDEX, Index);
    return READ_REGISTER_UCHAR(NvBase + NV2A_CRTC_REGISTER_VALUE);
}

ULONG
XboxGetFramebufferSize(PVOID Offset)
{
    memory_map_t * MemoryMap;
    INT Count, i;

    if (!MultibootInfoPtr)
    {
        return 0;
    }

    if (!(MultibootInfoPtr->flags & MB_INFO_FLAG_MEMORY_MAP))
    {
        return 0;
    }

    MemoryMap = (memory_map_t *)MultibootInfoPtr->mmap_addr;

    if (!MemoryMap ||
        MultibootInfoPtr->mmap_length == 0 ||
        MultibootInfoPtr->mmap_length % sizeof(memory_map_t) != 0)
    {
        return 0;
    }

    Count = MultibootInfoPtr->mmap_length / sizeof(memory_map_t);
    for (i = 0; i < Count; i++, MemoryMap++)
    {
        TRACE("i = %d, base_addr_low = 0x%p, MemoryMap->length_low = 0x%p\n", i, MemoryMap->base_addr_low, MemoryMap->length_low);

        /* Framebuffer address offset value is coming from the GPU within
         * memory mapped I/O address space, so we're comparing only low
         * 28 bits of the address within actual RAM address space */
        if (MemoryMap->base_addr_low == ((ULONG)Offset & 0x0FFFFFFF) && MemoryMap->base_addr_high == 0)
        {
            TRACE("Video memory found\n");
            return MemoryMap->length_low;
        }
    }
    ERR("Video memory not found!\n");
    return 0;
}

VOID
XboxVideoInit(VOID)
{
  /* Reuse framebuffer that was set up by firmware */
  FrameBuffer = (PVOID)READ_REGISTER_ULONG(NvBase + NV2A_CRTC_FRAMEBUFFER_START);
  /* Verify that framebuffer address is page-aligned */
  ASSERT((ULONG_PTR)FrameBuffer % PAGE_SIZE == 0);

  /* Obtain framebuffer memory size from multiboot memory map */
  if ((FrameBufferSize = XboxGetFramebufferSize(FrameBuffer)) == 0)
  {
    /* Fallback to Cromwell standard which reserves high 4 MB of RAM */
    FrameBufferSize = 4 * 1024 * 1024;
    WARN("Could not detect framebuffer memory size, fallback to 4 MB\n");
  }

  ScreenWidth = READ_REGISTER_ULONG(NvBase + NV2A_RAMDAC_FP_HVALID_END) + 1;
  ScreenHeight = READ_REGISTER_ULONG(NvBase + NV2A_RAMDAC_FP_VVALID_END) + 1;
  /* Get BPP directly from NV2A CRTC (magic constants are from Cromwell) */
  BytesPerPixel = 8 * (((NvGetCrtc(0x19) & 0xE0) << 3) | (NvGetCrtc(0x13) & 0xFF)) / ScreenWidth;
  if (BytesPerPixel == 4)
  {
    ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel - 1);
  }
  else
  {
    ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel);
  }
  Delta = (ScreenWidth * BytesPerPixel + 3) & ~ 0x3;

  /* Verify screen resolution */
  ASSERT(ScreenWidth > 1);
  ASSERT(ScreenHeight > 1);
  ASSERT(BytesPerPixel >= 1 && BytesPerPixel <= 4);
  /* Verify that screen fits framebuffer size */
  ASSERT(ScreenWidth * ScreenHeight * BytesPerPixel <= FrameBufferSize);

  XboxVideoClearScreenColor(MAKE_COLOR(0, 0, 0), TRUE);
}

VIDEODISPLAYMODE
XboxVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
  /* We only have one mode, semi-text */
  return VideoTextMode;
}

VOID
XboxVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
  *Width = ScreenWidth / CHAR_WIDTH;
  *Height = (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
  *Depth = 0;
}

ULONG
XboxVideoGetBufferSize(VOID)
{
  return (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT * (ScreenWidth / CHAR_WIDTH) * 2;
}

VOID
XboxVideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    TRACE("XboxVideoGetFontsFromFirmware(): UNIMPLEMENTED\n");
}

VOID
XboxVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
  /* We don't have a cursor yet */
}

VOID
XboxVideoHideShowTextCursor(BOOLEAN Show)
{
  /* We don't have a cursor yet */
}

VOID
XboxVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  PUCHAR OffScreenBuffer = (PUCHAR) Buffer;
  ULONG Col, Line;

  for (Line = 0; Line < (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
      for (Col = 0; Col < ScreenWidth / CHAR_WIDTH; Col++)
        {
          XboxVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
          OffScreenBuffer += 2;
        }
    }
}

BOOLEAN
XboxVideoIsPaletteFixed(VOID)
{
  return FALSE;
}

VOID
XboxVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
  /* Not supported */
}

VOID
XboxVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
{
  /* Not supported */
}

VOID
XboxVideoSync(VOID)
{
  /* Not supported */
}

VOID
XboxVideoPrepareForReactOS(VOID)
{
    XboxVideoClearScreenColor(MAKE_COLOR(0, 0, 0), TRUE);
    XboxVideoHideShowTextCursor(FALSE);
}

/* EOF */
