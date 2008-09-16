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
PHYSICAL_ADDRESS VgaPhysical;
BOOLEAN NextLine = FALSE;
UCHAR VidpTextColor = 0xF;
ULONG VidpCurrentX = 0;
ULONG VidpCurrentY = 0;
ULONG VidpScrollRegion[4] =
{
    0,
    0,
    640 - 1,
    480 - 1
};

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
    return ((Red & 0x1F) << 11) | ((Green & 0x1F) << 6) | ((Blue & 0x1F));
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

VOID
NTAPI
DisplayCharacter(CHAR Character,
                 ULONG Left,
                 ULONG Top,
                 ULONG TextColor,
                 ULONG BackTextColor)
{
    PUCHAR FontChar;
    ULONG i, j, XOffset;
    
    /* Get the font line for this character */
    FontChar = &FontData[Character * 13 - Top];
    
    /* Loop each pixel height */
    i = 13;
    do
    {
        /* Loop each pixel width */
        j = 128;
        XOffset = Left;
        do
        {
            /* Check if we should draw this pixel */
            if (FontChar[Top] & (UCHAR)j)
            {
                /* We do, use the given Text Color */
                VidpSetPixel(XOffset, Top, (UCHAR)TextColor);
            }
            else if (BackTextColor < 16)
            {
                /* This is a background pixel. We're drawing it unless it's */
                /* transparent. */
                VidpSetPixel(XOffset, Top, (UCHAR)BackTextColor);
            }
            
            /* Increase X Offset */
            XOffset++;
        } while (j >>= 1);
        
        /* Move to the next Y ordinate */
        Top++;
    } while (--i);
}

VOID
NTAPI
VgaScroll(ULONG Scroll)
{
    ULONG Top, Offset;
    PUSHORT SourceOffset, DestOffset;
    PUSHORT i, j;
    
    /* Set memory positions of the scroll */
    SourceOffset = &VgaArmBase[(VidpScrollRegion[1] * 80) + (VidpScrollRegion[0] >> 3)];
    DestOffset = &SourceOffset[Scroll * 80];
    
    /* Save top and check if it's above the bottom */
    Top = VidpScrollRegion[1];
    if (Top > VidpScrollRegion[3]) return;
    
    /* Start loop */
    do
    {
        /* Set number of bytes to loop and start offset */
        Offset = VidpScrollRegion[0] >> 3;
        j = SourceOffset;
        
        /* Check if this is part of the scroll region */
        if (Offset <= (VidpScrollRegion[2] >> 3))
        {
            /* Update position */
            i = (PUSHORT)(DestOffset - SourceOffset);
            
            /* Loop the X axis */
            do
            {
                /* Write value in the new position so that we can do the scroll */
                WRITE_REGISTER_USHORT(j, READ_REGISTER_USHORT(j + (ULONG_PTR)i));
                
                /* Move to the next memory location to write to */
                j++;
                
                /* Move to the next byte in the region */
                Offset++;
                
                /* Make sure we don't go past the scroll region */
            } while (Offset <= (VidpScrollRegion[2] >> 3));
        }
        
        /* Move to the next line */
        SourceOffset += 80;
        DestOffset += 80;
        
        /* Increase top */
        Top++;
        
        /* Make sure we don't go past the scroll region */
    } while (Top <= VidpScrollRegion[3]);
}

VOID
NTAPI
PreserveRow(IN ULONG CurrentTop,
            IN ULONG TopDelta,
            IN BOOLEAN Direction)
{
    PUSHORT Position1, Position2;
    ULONG Count;
        
    /* Check which way we're preserving */
    if (Direction)
    {
        /* Calculate the position in memory for the row */
        Position1 = &VgaArmBase[CurrentTop * 80];
        Position2 = &VgaArmBase[0x9600];
    }
    else
    {
        /* Calculate the position in memory for the row */
        Position1 = &VgaArmBase[0x9600];
        Position2 = &VgaArmBase[CurrentTop * 80];
    }
    
    /* Set the count and make sure it's above 0 */
    Count = TopDelta * 80;
    if (Count)
    {
        /* Loop every pixel */
        do
        {
            /* Write the data back on the other position */
            WRITE_REGISTER_USHORT(Position1, READ_REGISTER_USHORT(Position2));
            
            /* Increase both positions */
            Position2++;
            Position1++;
        } while (--Count);
    }
}

VOID
NTAPI
VidpInitializeDisplay(VOID)
{
    //
    // Set framebuffer address
    //
    WRITE_REGISTER_ULONG(PL110_LCDUPBASE, VgaPhysical.LowPart);
    WRITE_REGISTER_ULONG(PL110_LCDLPBASE, VgaPhysical.LowPart);
    
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
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
VidInitialize(IN BOOLEAN SetMode)
{   
    DPRINT1("bv-arm v0.1\n");
    
    //
    // Allocate framebuffer
    // 600kb works out to 640x480@16bpp
    //
    VgaPhysical.QuadPart = -1;
    VgaArmBase = MmAllocateContiguousMemory(600 * 1024, VgaPhysical);
    if (!VgaArmBase) return FALSE;
    
    //
    // Get physical address
    //
    VgaPhysical = MmGetPhysicalAddress(VgaArmBase);
    if (!VgaPhysical.QuadPart) return FALSE;
    DPRINT1("[BV-ARM] Frame Buffer @ 0x%p 0p%p\n", VgaArmBase, VgaPhysical.LowPart);

    //
    // Setup the display
    //
    VidpInitializeDisplay();

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
    //
    // Clear the current position
    //
    VidpCurrentX = 0;
    VidpCurrentY = 0;
    
    //
    // Re-initialize the VGA Display
    //
    VidpInitializeDisplay();
    
    //
    // Re-initialize the palette and fill the screen black
    //
    //InitializePalette();
    VidSolidColorFill(0, 0, 639, 479, 0);
}

/*
 * @implemented
 */
ULONG
NTAPI
VidSetTextColor(ULONG Color)
{
    UCHAR OldColor;
    
    //
    // Save the old, set the new
    //
    OldColor = VidpTextColor;
    VidpTextColor = Color;
    
    //
    // Return the old text color
    //
    return OldColor;
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
    /* Assert alignment */
    ASSERT((x1 & 0x7) == 0);
    ASSERT((x2 & 0x7) == 7);
    
    /* Set Scroll Region */
    VidpScrollRegion[0] = x1;
    VidpScrollRegion[1] = y1;
    VidpScrollRegion[2] = x2;
    VidpScrollRegion[3] = y2;
    
    /* Set current X and Y */
    VidpCurrentX = x1;
    VidpCurrentY = y1;
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
    ULONG TopDelta = 14;
    
    /* Start looping the string */
    while (*String)
    {
        /* Treat new-line separately */
        if (*String == '\n')
        {
            /* Modify Y position */
            VidpCurrentY += TopDelta;
            if (VidpCurrentY >= VidpScrollRegion[3])
            {
                /* Scroll the view */
                VgaScroll(TopDelta);
                VidpCurrentY -= TopDelta;
                
                /* Preserve row */
                PreserveRow(VidpCurrentY, TopDelta, TRUE);
            }
            
            /* Update current X */
            VidpCurrentX = VidpScrollRegion[0];
            
            /* Preseve the current row */
            PreserveRow(VidpCurrentY, TopDelta, FALSE);
        }
        else if (*String == '\r')
        {
            /* Update current X */
            VidpCurrentX = VidpScrollRegion[0];
            
            /* Check if we're being followed by a new line */
            if (String[1] != '\n') NextLine = TRUE;
        }
        else
        {
            /* Check if we had a \n\r last time */
            if (NextLine)
            {
                /* We did, preserve the current row */
                PreserveRow(VidpCurrentY, TopDelta, TRUE);
                NextLine = FALSE;
            }
            
            /* Display this character */
            DisplayCharacter(*String,
                             VidpCurrentX,
                             VidpCurrentY,
                             VidpTextColor,
                             16);
            VidpCurrentX += 8;
            
            /* Check if we should scroll */
            if (VidpCurrentX > VidpScrollRegion[2])
            {
                /* Update Y position and check if we should scroll it */
                VidpCurrentY += TopDelta;
                if (VidpCurrentY > VidpScrollRegion[3])
                {
                    /* Do the scroll */
                    VgaScroll(TopDelta);
                    VidpCurrentY -= TopDelta;
                    
                    /* Save the row */
                    PreserveRow(VidpCurrentY, TopDelta, TRUE);
                }
                
                /* Update X */
                VidpCurrentX = VidpScrollRegion[0];
            }
        }
        
        /* Get the next character */
        String++;
    }    
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
    //while (TRUE);
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
    int y, x;
    
    //
    // Loop along the Y-axis
    //
	for (y = Top; y <= Bottom; y++)
	{
        //
        // Loop along the X-axis
        //
        for (x = Left; x <= Right; x++)
        {
            //
            // Draw the pixel
            //
            VidpSetPixel(x, y, Color);
        }
	}    
}
