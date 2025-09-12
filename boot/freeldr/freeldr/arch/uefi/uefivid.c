/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define TOP_BOTTOM_LINES 0
#define LOWEST_SUPPORTED_RES 1

/* AGENT-MODIFIED: Added preferred resolution constants for better mode selection */
#define PREFERRED_WIDTH_MIN  800
#define PREFERRED_WIDTH_MAX  1920
#define PREFERRED_HEIGHT_MIN 600
#define PREFERRED_HEIGHT_MAX 1200

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern UCHAR BitmapFont8x16[256 * 16];

UCHAR MachDefaultTextColor = COLOR_GRAY;
REACTOS_INTERNAL_BGCONTEXT framebufferData;
EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/* AGENT-MODIFIED: Added variables for software text rendering */
static UINT32 ConsoleX = 0;
static UINT32 ConsoleY = 0;
static UINT32 MaxConsoleX = 0;
static UINT32 MaxConsoleY = 0;
static BOOLEAN GopConsoleInitialized = FALSE;

/* FUNCTIONS ******************************************************************/

/* AGENT-MODIFIED: Added function to select optimal GOP mode */
static UINT32
UefiFindOptimalGopMode(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop)
{
    EFI_STATUS Status;
    UINT32 BestMode = 0;
    UINT32 BestScore = 0;
    UINT32 CurrentMode;
    UINTN SizeOfInfo;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    
    TRACE("AGENT-MODIFIED: Searching for optimal GOP mode\n");
    TRACE("  Total modes available: %d\n", gop->Mode->MaxMode);
    
    for (CurrentMode = 0; CurrentMode < gop->Mode->MaxMode; CurrentMode++)
    {
        Status = gop->QueryMode(gop, CurrentMode, &SizeOfInfo, &Info);
        if (Status == EFI_SUCCESS)
        {
            UINT32 Width = Info->HorizontalResolution;
            UINT32 Height = Info->VerticalResolution;
            UINT32 Score = 0;
            
            TRACE("  Mode %d: %dx%d, PixelFormat=%d\n", 
                  CurrentMode, Width, Height, Info->PixelFormat);
            
            /* Skip modes without framebuffer support */
            if (Info->PixelFormat == PixelBltOnly)
            {
                TRACE("    Skipping - no framebuffer\n");
                continue;
            }
            
            /* Preferred resolutions (in order of preference) */
            if (Width == 1024 && Height == 768)
                Score = 100;  /* Most preferred */
            else if (Width == 1280 && Height == 1024)
                Score = 95;
            else if (Width == 1280 && Height == 800)
                Score = 90;
            else if (Width == 800 && Height == 600)
                Score = 85;
            else if (Width == 1366 && Height == 768)
                Score = 80;
            else if (Width == 1440 && Height == 900)
                Score = 75;
            else if (Width == 640 && Height == 480)
                Score = 50;  /* Fallback */
            else if (Width >= PREFERRED_WIDTH_MIN && Width <= PREFERRED_WIDTH_MAX &&
                     Height >= PREFERRED_HEIGHT_MIN && Height <= PREFERRED_HEIGHT_MAX)
            {
                /* Other acceptable resolutions */
                Score = 60;
            }
            
            if (Score > BestScore)
            {
                BestScore = Score;
                BestMode = CurrentMode;
                TRACE("    New best mode (score=%d)\n", Score);
            }
        }
    }
    
    TRACE("AGENT-MODIFIED: Selected mode %d with score %d\n", BestMode, BestScore);
    return BestMode;
}

EFI_STATUS
UefiInitializeVideo(VOID)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    UINT32 OptimalMode;

    RtlZeroMemory(&framebufferData, sizeof(framebufferData));
    Status = GlobalSystemTable->BootServices->LocateProtocol(&EfiGraphicsOutputProtocol, 0, (void**)&gop);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to find GOP with status %d\n", Status);
        return Status;
    }

    /* AGENT-MODIFIED: Add debug output for GOP modes */
    TRACE("AGENT-MODIFIED: GOP Protocol located successfully\n");
    TRACE("  MaxMode: %d\n", gop->Mode->MaxMode);
    TRACE("  Current Mode: %d\n", gop->Mode->Mode);
    
    /* AGENT-MODIFIED: Find and set optimal resolution instead of hardcoded low res */
    OptimalMode = UefiFindOptimalGopMode(gop);
    if (OptimalMode != gop->Mode->Mode)
    {
        Status = gop->SetMode(gop, OptimalMode);
        if (Status != EFI_SUCCESS)
        {
            TRACE("AGENT-MODIFIED: Failed to set optimal mode %d, using current mode\n", OptimalMode);
        }
        else
        {
            TRACE("AGENT-MODIFIED: Successfully set mode %d\n", OptimalMode);
        }
    }

    framebufferData.BaseAddress        = (ULONG_PTR)gop->Mode->FrameBufferBase;
    framebufferData.BufferSize         = gop->Mode->FrameBufferSize;
    framebufferData.ScreenWidth        = gop->Mode->Info->HorizontalResolution;
    framebufferData.ScreenHeight       = gop->Mode->Info->VerticalResolution;
    framebufferData.PixelsPerScanLine  = gop->Mode->Info->PixelsPerScanLine;
    framebufferData.PixelFormat        = gop->Mode->Info->PixelFormat;

    /* AGENT-MODIFIED: Print GOP framebuffer details */
    TRACE("AGENT-MODIFIED: GOP Framebuffer initialized:\n");
    TRACE("  BaseAddress: 0x%lx\n", framebufferData.BaseAddress);
    TRACE("  BufferSize: 0x%x\n", framebufferData.BufferSize);
    TRACE("  Resolution: %dx%d\n", framebufferData.ScreenWidth, framebufferData.ScreenHeight);
    TRACE("  PixelsPerScanLine: %d\n", framebufferData.PixelsPerScanLine);
    TRACE("  PixelFormat: %d\n", framebufferData.PixelFormat);
    
    /* AGENT-MODIFIED: Initialize console dimensions for software text rendering */
    MaxConsoleX = framebufferData.ScreenWidth / CHAR_WIDTH;
    MaxConsoleY = (framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
    ConsoleX = 0;
    ConsoleY = 0;
    GopConsoleInitialized = TRUE;
    TRACE("AGENT-MODIFIED: Console dimensions: %dx%d chars\n", MaxConsoleX, MaxConsoleY);

    return Status;
}

VOID
UefiPrintFramebufferData(VOID)
{
    TRACE("Framebuffer BaseAddress       : %X\n", framebufferData.BaseAddress);
    TRACE("Framebuffer BufferSize        : %X\n", framebufferData.BufferSize);
    TRACE("Framebuffer ScreenWidth       : %d\n", framebufferData.ScreenWidth);
    TRACE("Framebuffer ScreenHeight      : %d\n", framebufferData.ScreenHeight);
    TRACE("Framebuffer PixelsPerScanLine : %d\n", framebufferData.PixelsPerScanLine);
    TRACE("Framebuffer PixelFormat       : %d\n", framebufferData.PixelFormat);
}

static ULONG
UefiVideoAttrToSingleColor(UCHAR Attr)
{
    UCHAR Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
UefiVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
    *FgColor = UefiVideoAttrToSingleColor(Attr & 0xf);
    *BgColor = UefiVideoAttrToSingleColor((Attr >> 4) & 0xf);
}


static VOID
UefiVideoClearScreenColor(ULONG Color, BOOLEAN FullScreen)
{
    ULONG Delta;
    ULONG Line, Col;
    PULONG p;

    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    for (Line = 0; Line < framebufferData.ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (PULONG) ((char *) framebufferData.BaseAddress + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
        for (Col = 0; Col < framebufferData.ScreenWidth; Col++)
        {
            *p++ = Color;
        }
    }
}

VOID
UefiVideoClearScreen(UCHAR Attr)
{
    ULONG FgColor, BgColor;

    UefiVideoAttrToColors(Attr, &FgColor, &BgColor);
    UefiVideoClearScreenColor(BgColor, FALSE);
}

VOID
UefiVideoOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
    PUCHAR FontPtr;
    PULONG Pixel;
    UCHAR Mask;
    unsigned Line;
    unsigned Col;
    ULONG Delta;
    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    FontPtr = BitmapFont8x16 + Char * 16;
    Pixel = (PULONG) ((char *) framebufferData.BaseAddress +
            (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) *  Delta + X * CHAR_WIDTH * 4);

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

VOID
UefiVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    ULONG FgColor = 0;
    ULONG BgColor = 0;
    if (Ch != 0)
    {
        UefiVideoAttrToColors(Attr, &FgColor, &BgColor);
        UefiVideoOutputChar(Ch, X, Y, FgColor, BgColor);
    }
}

VOID
UefiVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    *Width =  framebufferData.ScreenWidth / CHAR_WIDTH;
    *Height = (framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
    *Depth =  0;
}

VIDEODISPLAYMODE
UefiVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
    /* We only have one mode, semi-text */
    return VideoTextMode;
}

ULONG
UefiVideoGetBufferSize(VOID)
{
    return ((framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT * (framebufferData.ScreenWidth / CHAR_WIDTH) * 2);
}

VOID
UefiVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;

    ULONG Col, Line;
    for (Line = 0; Line < (framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
        for (Col = 0; Col < framebufferData.ScreenWidth / CHAR_WIDTH; Col++)
        {
            UefiVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
            OffScreenBuffer += 2;
        }
    }
}

VOID
UefiVideoScrollUp(VOID)
{
    ULONG BgColor, Dummy;
    ULONG Delta;
    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    ULONG PixelCount = framebufferData.ScreenWidth * CHAR_HEIGHT *
                       (((framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    PULONG Src = (PULONG)((PUCHAR)framebufferData.BaseAddress + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta);
    PULONG Dst = (PULONG)((PUCHAR)framebufferData.BaseAddress + TOP_BOTTOM_LINES * Delta);

    UefiVideoAttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < framebufferData.ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

VOID
UefiVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* We don't have a cursor yet */
}

VOID
UefiVideoHideShowTextCursor(BOOLEAN Show)
{
    /* We don't have a cursor yet */
}

BOOLEAN
UefiVideoIsPaletteFixed(VOID)
{
    return 0;
}

VOID
UefiVideoSetPaletteColor(UCHAR Color, UCHAR Red,
                         UCHAR Green, UCHAR Blue)
{
    /* Not supported */
}

VOID
UefiVideoGetPaletteColor(UCHAR Color, UCHAR* Red,
                         UCHAR* Green, UCHAR* Blue)
{
    /* Not supported */
}

/* AGENT-MODIFIED: Added software text rendering functions for GOP console */

/* AGENT-MODIFIED: Direct GOP console output function */
VOID
UefiGopConsolePutChar(CHAR Ch)
{
    ULONG FgColor, BgColor;
    
    if (!GopConsoleInitialized || framebufferData.BaseAddress == 0)
        return;
    
    /* Get current colors */
    UefiVideoAttrToColors(MachDefaultTextColor, &FgColor, &BgColor);
    
    /* Handle special characters */
    if (Ch == '\r')
    {
        ConsoleX = 0;
        return;
    }
    else if (Ch == '\n')
    {
        ConsoleX = 0;
        ConsoleY++;
    }
    else if (Ch == '\t')
    {
        ConsoleX = (ConsoleX + 8) & ~7;
    }
    else if (Ch == '\b')
    {
        if (ConsoleX > 0)
        {
            ConsoleX--;
            UefiVideoOutputChar(' ', ConsoleX, ConsoleY, FgColor, BgColor);
        }
    }
    else
    {
        /* Output normal character */
        UefiVideoOutputChar(Ch, ConsoleX, ConsoleY, FgColor, BgColor);
        ConsoleX++;
    }
    
    /* Handle line wrap */
    if (ConsoleX >= MaxConsoleX)
    {
        ConsoleX = 0;
        ConsoleY++;
    }
    
    /* Handle scrolling */
    if (ConsoleY >= MaxConsoleY)
    {
        UefiVideoScrollUp();
        ConsoleY = MaxConsoleY - 1;
    }
}

/* AGENT-MODIFIED: GOP console string output */
VOID
UefiGopConsolePutString(PCSTR String)
{
    if (!String)
        return;
        
    while (*String)
    {
        UefiGopConsolePutChar(*String);
        String++;
    }
}

/* AGENT-MODIFIED: Clear GOP console screen */
VOID
UefiGopConsoleClear(VOID)
{
    if (!GopConsoleInitialized || framebufferData.BaseAddress == 0)
        return;
    
    UefiVideoClearScreen(MachDefaultTextColor);
    ConsoleX = 0;
    ConsoleY = 0;
}

/* AGENT-MODIFIED: Set GOP console cursor position */
VOID
UefiGopConsoleSetCursor(UINT32 X, UINT32 Y)
{
    if (!GopConsoleInitialized)
        return;
        
    if (X < MaxConsoleX)
        ConsoleX = X;
    
    if (Y < MaxConsoleY)
        ConsoleY = Y;
}

/* AGENT-MODIFIED: Get GOP console status */
BOOLEAN
UefiGopConsoleIsInitialized(VOID)
{
    return GopConsoleInitialized && (framebufferData.BaseAddress != 0);
}
