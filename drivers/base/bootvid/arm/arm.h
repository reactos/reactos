
#pragma once

extern PUSHORT VgaArmBase;

#define LCDTIMING0_PPL(x)       ((((x) / 16 - 1) & 0x3f) << 2)
#define LCDTIMING1_LPP(x)       (((x) & 0x3ff) - 1)
#define LCDCONTROL_LCDPWR       (1 << 11)
#define LCDCONTROL_LCDEN        (1)
#define LCDCONTROL_LCDBPP(x)    (((x) & 7) << 1)
#define LCDCONTROL_LCDTFT       (1 << 5)

#define PL110_LCDTIMING0    (PVOID)0xE0020000
#define PL110_LCDTIMING1    (PVOID)0xE0020004
#define PL110_LCDTIMING2    (PVOID)0xE0020008
#define PL110_LCDUPBASE     (PVOID)0xE0020010
#define PL110_LCDLPBASE     (PVOID)0xE0020014
#define PL110_LCDCONTROL    (PVOID)0xE0020018

#define READ_REGISTER_ULONG(r) (*(volatile ULONG * const)(r))
#define WRITE_REGISTER_ULONG(r, v) (*(volatile ULONG *)(r) = (v))

#define READ_REGISTER_USHORT(r) (*(volatile USHORT * const)(r))
#define WRITE_REGISTER_USHORT(r, v) (*(volatile USHORT *)(r) = (v))

PALETTE_ENTRY VidpVga8To16BitTransform[16] =
{
    {0x00, 0x00, 0x00}, // Black
    {0x00, 0x00, 0x08}, // Blue
    {0x00, 0x08, 0x00}, // Green
    {0x00, 0x08, 0x08}, // Cyan
    {0x08, 0x00, 0x00}, // Red
    {0x08, 0x00, 0x08}, // Magenta
    {0x0B, 0x0D, 0x0F}, // Brown
    {0x10, 0x10, 0x10}, // Light Gray
    {0x08, 0x08, 0x08}, // Dark Gray
    {0x00, 0x00, 0x1F}, // Light Blue
    {0x00, 0x1F, 0x00}, // Light Green
    {0x00, 0x1F, 0x1F}, // Light Cyan
    {0x1F, 0x00, 0x00}, // Light Red
    {0x1F, 0x00, 0x1F}, // Light Magenta
    {0x1F, 0x1F, 0x00}, // Yellow
    {0x1F, 0x1F, 0x1F}, // White
};

FORCEINLINE
USHORT
VidpBuildColor(_In_ UCHAR Color)
{
    UCHAR Red, Green, Blue;

    /* Extract color components */
    Red   = VidpVga8To16BitTransform[Color].Red;
    Green = VidpVga8To16BitTransform[Color].Green;
    Blue  = VidpVga8To16BitTransform[Color].Blue;

    /* Build the 16-bit color mask */
    return ((Red & 0x1F) << 11) | ((Green & 0x1F) << 6) | ((Blue & 0x1F));
}

FORCEINLINE
VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color)
{
    PUSHORT PixelPosition;

    /* Calculate the pixel position */
    PixelPosition = &VgaArmBase[Left + (Top * SCREEN_WIDTH)];

    /* Set our color */
    WRITE_REGISTER_USHORT(PixelPosition, VidpBuildColor(Color));
}
