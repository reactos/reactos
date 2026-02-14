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
#include "../../vidfb.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

ULONG NvBase = 0xFD000000;
ULONG_PTR FrameBuffer;
ULONG FrameBufferSize;
PCM_FRAMEBUF_DEVICE_DATA FrameBufferData = NULL;
extern multiboot_info_t * MultibootInfoPtr;

#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

VOID
XboxVideoClearScreen(UCHAR Attr)
{
    FbConsClearScreen(Attr);
}

VOID
XboxVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    FbConsPutChar(Ch, Attr, X, Y);
}

static UCHAR
NvGetCrtc(UCHAR Index)
{
    WRITE_REGISTER_UCHAR(NvBase + NV2A_CRTC_REGISTER_INDEX, Index);
    return READ_REGISTER_UCHAR(NvBase + NV2A_CRTC_REGISTER_VALUE);
}

static ULONG
XboxGetFramebufferSize(
    _In_ ULONG_PTR Offset)
{
    memory_map_t * MemoryMap;
    INT Count, i;

    if (!MultibootInfoPtr)
        return 0;

    if (!(MultibootInfoPtr->flags & MB_INFO_FLAG_MEMORY_MAP))
        return 0;

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
        if (MemoryMap->base_addr_low == (Offset & 0x0FFFFFFF) && MemoryMap->base_addr_high == 0)
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
    ULONG ScreenWidth;
    ULONG ScreenHeight;
    ULONG BytesPerPixel;

    /* Reuse the framebuffer that was set up by firmware */
    FrameBuffer = (ULONG_PTR)READ_REGISTER_ULONG(NvBase + NV2A_CRTC_FRAMEBUFFER_START);
    TRACE("XBOX framebuffer at 0x%p\n", FrameBuffer);
    /* Verify that the framebuffer address is page-aligned */
    ASSERT(FrameBuffer % PAGE_SIZE == 0);

    /* Obtain framebuffer memory size from the multiboot memory map */
    if ((FrameBufferSize = XboxGetFramebufferSize(FrameBuffer)) == 0)
    {
        /* Fallback to Cromwell standard which reserves high 4 MB of RAM */
        FrameBufferSize = 4 * 1024 * 1024; // See FB_SIZE
        WARN("Could not detect framebuffer memory size, fallback to 4 MB\n");
    }

    ScreenWidth = READ_REGISTER_ULONG(NvBase + NV2A_RAMDAC_FP_HVALID_END) + 1;
    ScreenHeight = READ_REGISTER_ULONG(NvBase + NV2A_RAMDAC_FP_VVALID_END) + 1;

    /* Get BPP directly from NV2A CRTC (magic constants are from Cromwell) */
    BytesPerPixel = 8 * (((NvGetCrtc(0x19) & 0xE0) << 3) | (NvGetCrtc(0x13) & 0xFF)) / ScreenWidth;
    if (BytesPerPixel == 4)
        ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel - 1);
    else
        ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel);

    /* Verify screen resolution */
    ASSERT(ScreenWidth > 1);
    ASSERT(ScreenHeight > 1);
    ASSERT(BytesPerPixel >= 1 && BytesPerPixel <= 4);
    /* Verify that screen fits framebuffer size */
    ASSERT(ScreenWidth * ScreenHeight * BytesPerPixel <= FrameBufferSize);

    if (!VidFbInitializeVideo(&FrameBufferData,
                              FrameBuffer,
                              FrameBufferSize,
                              ScreenWidth,
                              ScreenHeight,
                              ScreenWidth,
                              BytesPerPixel * 8,
                              NULL))
    {
        ERR("Couldn't initialize video framebuffer\n");
        return;
    }

    VidFbClearScreenColor(MAKE_COLOR(0, 0, 0), TRUE);
}

VIDEODISPLAYMODE
XboxVideoSetDisplayMode(PCSTR DisplayMode, BOOLEAN Init)
{
    /* We only have one mode, semi-text */
    return VideoTextMode;
}

VOID
XboxVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    FbConsGetDisplaySize(Width, Height, Depth);
}

ULONG
XboxVideoGetBufferSize(VOID)
{
    return FbConsGetBufferSize();
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
    FbConsCopyOffScreenBufferToVRAM(Buffer);
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
    VidFbClearScreenColor(MAKE_COLOR(0, 0, 0), TRUE);
    XboxVideoHideShowTextCursor(FALSE);
}

/* EOF */
