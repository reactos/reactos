#include "precomp.h"
#define NDEBUG
#include "debug.h"

#define LCDTIMING0_PPL(x) 		((((x) / 16 - 1) & 0x3f) << 2)
#define LCDTIMING1_LPP(x) 		(((x) & 0x3ff) - 1)
#define LCDCONTROL_LCDPWR		(1 << 11)
#define LCDCONTROL_LCDEN		(1)
#define LCDCONTROL_LCDBPP(x)	(((x) & 7) << 1)
#define LCDCONTROL_LCDTFT		(1 << 5)

#define PL110_LCDTIMING0	(PVOID)0xE0020000
#define PL110_LCDTIMING1	(PVOID)0xE0020004
#define PL110_LCDTIMING2	(PVOID)0xE0020008
#define PL110_LCDUPBASE		(PVOID)0xE0020010
#define PL110_LCDLPBASE		(PVOID)0xE0020014
#define PL110_LCDCONTROL	(PVOID)0xE0020018

#define READ_REGISTER_ULONG(r) (*(volatile ULONG * const)(r))
#define WRITE_REGISTER_ULONG(r, v) (*(volatile ULONG *)(r) = (v))

#define READ_REGISTER_USHORT(r) (*(volatile USHORT * const)(r))
#define WRITE_REGISTER_USHORT(r, v) (*(volatile USHORT *)(r) = (v))

PUSHORT VgaArmBase;

typedef struct _VGA_COLOR
{
    UCHAR Red;
    UCHAR Green;
    UCHAR Blue;
} VGA_COLOR;

VGA_COLOR VidpVga8To16BitTransform[16] =
{
    {0x00, 0x00, 0x00}, // Black
    {0x00, 0x00, 0x08}, // Blue
    {0x00, 0x08, 0x00}, // Green
    {0x00, 0x08, 0x08}, // Cyan
    {0x08, 0x00, 0x00}, // Red
    {0x08, 0x00, 0x08}, // Magenta
    {0x08, 0x08, 0x00}, // Brown
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

/* PRIVATE FUNCTIONS *********************************************************/

USHORT
FORCEINLINE
VidpBuildColor(IN UCHAR Color)
{
    UCHAR Red, Green, Blue;
    
    //
    // Extract color components
    //
    Red = VidpVga8To16BitTransform[Color].Red;
    Green = VidpVga8To16BitTransform[Color].Green;
    Blue = VidpVga8To16BitTransform[Color].Blue;
    
    //
    // Build the 16-bit color mask
    //
    return ((Blue & 0x1F) << 11) | ((Green & 0x1F) << 6) | ((Red & 0x1F));
}


VOID
FORCEINLINE
VidpSetPixel(IN ULONG Left,
             IN ULONG Top,
             IN UCHAR Color)
{
    PUSHORT PixelPosition;
    
    //
    // Calculate the pixel position
    //
    PixelPosition = &VgaArmBase[Left + (Top * 640)];

    //
    // Set our color
    //
    WRITE_REGISTER_USHORT(PixelPosition, VidpBuildColor(Color));
}


/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
VidInitialize(IN BOOLEAN SetMode)
{   
    PHYSICAL_ADDRESS Physical;
    DPRINT1("bv-arm v0.1\n");
    
    //
    // Allocate framebuffer
    // 600kb works out to 640x480@16bpp
    //
    Physical.QuadPart = -1;
    VgaArmBase = MmAllocateContiguousMemory(600 * 1024, Physical);
    if (!VgaArmBase) return FALSE;
    
    //
    // Get physical address
    //
    Physical = MmGetPhysicalAddress(VgaArmBase);
    if (!Physical.QuadPart) return FALSE;
    DPRINT1("[BV-ARM] Frame Buffer @ 0x%p 0p%p\n", VgaArmBase, Physical.LowPart);

    //
    // Set framebuffer address
    //
    WRITE_REGISTER_ULONG(PL110_LCDUPBASE, Physical.LowPart);
    WRITE_REGISTER_ULONG(PL110_LCDLPBASE, Physical.LowPart);
    
    //
    // Initialize timings to 640x480
    //
	WRITE_REGISTER_ULONG(PL110_LCDTIMING0, LCDTIMING0_PPL(640));
	WRITE_REGISTER_ULONG(PL110_LCDTIMING1, LCDTIMING1_LPP(480));
    
    //
    // Enable the LCD Display
    //
	WRITE_REGISTER_ULONG(PL110_LCDCONTROL,
                         LCDCONTROL_LCDEN |
                         LCDCONTROL_LCDTFT |
                         LCDCONTROL_LCDPWR |
                         LCDCONTROL_LCDBPP(4));

#if DBG
    //
    // Draw an RGB test pattern
    //
    int y, x;
	for (y = 0; y < 480; y += 40)
	{
        for (x = 0; x < 640; x++)
        {
            VidpSetPixel(x, y, 12);
            VidpSetPixel(x, y + 10, 9);
            VidpSetPixel(x, y + 20, 10);
            VidpSetPixel(x, y + 30, 15);
        }
	}
#endif
    
    //
    // We are done!
    //
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
VidResetDisplay(IN BOOLEAN HalReset)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
ULONG
NTAPI
VidSetTextColor(ULONG Color)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

/*
 * @implemented
 */
VOID
NTAPI
VidDisplayStringXY(PUCHAR String,
                   ULONG Left,
                   ULONG Top,
                   BOOLEAN Transparent)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidSetScrollRegion(ULONG x1,
                   ULONG y1,
                   ULONG x2,
                   ULONG y2)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidCleanUp(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidBufferToScreenBlt(IN PUCHAR Buffer,
                     IN ULONG Left,
                     IN ULONG Top,
                     IN ULONG Width,
                     IN ULONG Height,
                     IN ULONG Delta)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidDisplayString(PUCHAR String)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidBitBlt(PUCHAR Buffer,
          ULONG Left,
          ULONG Top)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidScreenToBufferBlt(PUCHAR Buffer,
                     ULONG Left,
                     ULONG Top,
                     ULONG Width,
                     ULONG Height,
                     ULONG Delta)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidSolidColorFill(IN ULONG Left,
                  IN ULONG Top,
                  IN ULONG Right,
                  IN ULONG Bottom,
                  IN UCHAR Color)
{
    UNIMPLEMENTED;
    while (TRUE);
}
