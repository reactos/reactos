/*
 * PROJECT:     ReactOS Boot Video Driver for UEFI GOP systems
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     UEFI Graphics Output Protocol support
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* UEFI bootvid - standalone driver without VGA dependencies */
#include <ntifs.h>

/* Memory cache type for MmMapIoSpace */
#define MmNonCached 0

/* Font data */
#define BOOTCHAR_HEIGHT 13
extern UCHAR VidpFontData[];

/* Import framebuffer data from kernel */
__declspec(dllimport) PHYSICAL_ADDRESS VidpFrameBufferBase;
__declspec(dllimport) ULONG VidpFrameBufferSize;
__declspec(dllimport) ULONG VidpScreenWidth;
__declspec(dllimport) ULONG VidpScreenHeight;
__declspec(dllimport) ULONG VidpPixelsPerScanLine;

/* GLOBALS ********************************************************************/

static PHYSICAL_ADDRESS FrameBufferBase = {0};
static ULONG FrameBufferSize = 0;
static ULONG ScreenWidth = 0;
static ULONG ScreenHeight = 0;
static ULONG PixelsPerScanLine = 0;
static ULONG PixelFormat = 0;
static PULONG FrameBuffer = NULL;
static BOOLEAN DisplayInitialized = FALSE;

/* Font data is defined in precomp.h */

/* UEFI GOP Pixel Formats */
#define PixelRedGreenBlueReserved8BitPerColor  0
#define PixelBlueGreenRedReserved8BitPerColor  1
#define PixelBitMask                           2
#define PixelBltOnly                           3

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
UefiGopClearScreen(ULONG Color)
{
    ULONG x, y;
    PULONG Pixel;
    
    if (!FrameBuffer) return;
    
    for (y = 0; y < ScreenHeight; y++)
    {
        Pixel = (PULONG)((PUCHAR)FrameBuffer + y * PixelsPerScanLine * 4);
        for (x = 0; x < ScreenWidth; x++)
        {
            *Pixel++ = Color;
        }
    }
}

static VOID
UefiGopSetPixel(ULONG x, ULONG y, ULONG Color)
{
    PULONG Pixel;
    
    if (!FrameBuffer || x >= ScreenWidth || y >= ScreenHeight) return;
    
    Pixel = (PULONG)((PUCHAR)FrameBuffer + y * PixelsPerScanLine * 4 + x * 4);
    *Pixel = Color;
}

static VOID
UefiGopBitBlt(PULONG Buffer, ULONG x, ULONG y, ULONG Width, ULONG Height)
{
    ULONG i, j;
    PULONG Src = Buffer;
    PULONG Dst;
    
    if (!FrameBuffer) return;
    
    for (j = 0; j < Height && (y + j) < ScreenHeight; j++)
    {
        Dst = (PULONG)((PUCHAR)FrameBuffer + (y + j) * PixelsPerScanLine * 4 + x * 4);
        for (i = 0; i < Width && (x + i) < ScreenWidth; i++)
        {
            *Dst++ = *Src++;
        }
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN ResetMode)
{
    /* Check if already initialized */
    if (DisplayInitialized) return TRUE;
    
    /* Use values from loader if available, otherwise use defaults */
    if (VidpFrameBufferBase.QuadPart != 0)
    {
        FrameBufferBase = VidpFrameBufferBase;
        FrameBufferSize = VidpFrameBufferSize;
        ScreenWidth = VidpScreenWidth;
        ScreenHeight = VidpScreenHeight;
        PixelsPerScanLine = VidpPixelsPerScanLine;
    }
    else
    {
        /* Fallback to defaults */
        FrameBufferBase.QuadPart = 0xC0000000;
        FrameBufferSize = 1024 * 768 * 4;
        ScreenWidth = 1024;
        ScreenHeight = 768;
        PixelsPerScanLine = 1024;
    }
    
    PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
    
    /* Map the framebuffer */
    FrameBuffer = (PULONG)MmMapIoSpace(FrameBufferBase, FrameBufferSize, MmNonCached);
    if (!FrameBuffer)
    {
        return FALSE;
    }
    
    /* Clear the screen to black */
    UefiGopClearScreen(0x00000000);
    
    DisplayInitialized = TRUE;
    return TRUE;
}

VOID
NTAPI
VidCleanUp(VOID)
{
    if (FrameBuffer && DisplayInitialized)
    {
        MmUnmapIoSpace(FrameBuffer, FrameBufferSize);
        FrameBuffer = NULL;
        DisplayInitialized = FALSE;
    }
}

VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN HalReset)
{
    /* Clear screen to black */
    if (DisplayInitialized)
    {
        UefiGopClearScreen(0x00000000);
    }
}

VOID
NTAPI
VidScreenToBufferBlt(
    _Out_writes_bytes_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    ULONG x, y;
    PULONG Src, Dst;
    
    if (!FrameBuffer) return;
    
    for (y = 0; y < Height && (Top + y) < ScreenHeight; y++)
    {
        Src = (PULONG)((PUCHAR)FrameBuffer + (Top + y) * PixelsPerScanLine * 4 + Left * 4);
        Dst = (PULONG)(Buffer + y * Delta);
        for (x = 0; x < Width && (Left + x) < ScreenWidth; x++)
        {
            *Dst++ = *Src++;
        }
    }
}

VOID
NTAPI
VidBufferToScreenBlt(
    _In_reads_bytes_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    ULONG x, y;
    PULONG Src, Dst;
    
    if (!FrameBuffer) return;
    
    for (y = 0; y < Height && (Top + y) < ScreenHeight; y++)
    {
        Src = (PULONG)(Buffer + y * Delta);
        Dst = (PULONG)((PUCHAR)FrameBuffer + (Top + y) * PixelsPerScanLine * 4 + Left * 4);
        for (x = 0; x < Width && (Left + x) < ScreenWidth; x++)
        {
            *Dst++ = *Src++;
        }
    }
}

VOID
NTAPI
VidDisplayString(
    _In_z_ PUCHAR String)
{
    static ULONG CurrentX = 0;
    static ULONG CurrentY = 0;
    ULONG CharWidth = 8;
    ULONG CharHeight = BOOTCHAR_HEIGHT;
    UCHAR Ch;
    PUCHAR FontChar;
    ULONG i, j, k;
    ULONG Color;
    
    if (!FrameBuffer) return;
    
    while ((Ch = *String++))
    {
        if (Ch == '\r')
        {
            CurrentX = 0;
            continue;
        }
        else if (Ch == '\n')
        {
            CurrentX = 0;
            CurrentY += CharHeight;
            if (CurrentY + CharHeight > ScreenHeight)
            {
                /* Scroll up */
                CurrentY = ScreenHeight - CharHeight;
                /* TODO: Implement scrolling */
            }
            continue;
        }
        
        /* Get font data for this character */
        FontChar = &VidpFontData[Ch * CharHeight];
        
        /* Draw the character */
        for (j = 0; j < CharHeight; j++)
        {
            for (i = 0; i < CharWidth; i++)
            {
                if (FontChar[j] & (0x80 >> i))
                {
                    /* White pixel */
                    Color = 0xFFFFFFFF;
                }
                else
                {
                    /* Black pixel */
                    Color = 0x00000000;
                }
                UefiGopSetPixel(CurrentX + i, CurrentY + j, Color);
            }
        }
        
        /* Move to next character position */
        CurrentX += CharWidth;
        if (CurrentX + CharWidth > ScreenWidth)
        {
            CurrentX = 0;
            CurrentY += CharHeight;
        }
    }
}

ULONG
NTAPI
VidSetTextColor(
    _In_ ULONG Color)
{
    /* Store text color for later use */
    /* For now, we just use white on black */
    return Color; /* Return the color that was set */
}

VOID
NTAPI
VidSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR Color)
{
    ULONG x, y;
    ULONG RgbColor;
    
    if (!FrameBuffer) return;
    
    /* Convert VGA color to RGB */
    /* Simple mapping: use white for any non-black color */
    RgbColor = (Color == 0) ? 0x00000000 : 0xFFFFFFFF;
    
    for (y = Top; y <= Bottom && y < ScreenHeight; y++)
    {
        for (x = Left; x <= Right && x < ScreenWidth; x++)
        {
            UefiGopSetPixel(x, y, RgbColor);
        }
    }
}

VOID
NTAPI
VidSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom)
{
    /* TODO: Implement scrolling region */
}

VOID
NTAPI
VidDisplayStringXY(
    _In_z_ PUCHAR String,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ BOOLEAN Transparent)
{
    /* TODO: Implement positioned string display */
    VidDisplayString(String);
}

VOID
NTAPI
VidBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top)
{
    /* Assume buffer contains bitmap data */
    /* TODO: Implement proper bitmap blitting */
    UefiGopBitBlt((PULONG)Buffer, Left, Top, 100, 100);
}