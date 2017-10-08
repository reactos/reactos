/*
 * PROJECT:         ReactOS Framebuffer Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/displays/vga_new/driver.h
 * PURPOSE:         Main Driver Header File
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */
 
//#define DBG 1
#include "stddef.h"
#include <stdarg.h>
#include "windef.h"
#include "wingdi.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"
#include "debug.h"

typedef struct  _PDEV
{
    HANDLE  hDriver;                    // Handle to \Device\Screen
    HDEV    hdevEng;                    // Engine's handle to PDEV
    HSURF   hsurfEng;                   // Engine's handle to surface
    HPALETTE hpalDefault;               // Handle to the default palette for device.
    PBYTE   pjScreen;                   // This is pointer to base screen address
    ULONG   cxScreen;                   // Visible screen width
    ULONG   cyScreen;                   // Visible screen height
    ULONG   ulMode;                     // Mode the mini-port driver is in.
    LONG    lDeltaScreen;               // Distance from one scan to the next.
    ULONG   cScreenSize;                // size of video memory, including
                                        // offscreen memory.
    PVOID   pOffscreenList;             // linked list of DCI offscreen surfaces.
    FLONG   flRed;                      // For bitfields device, Red Mask
    FLONG   flGreen;                    // For bitfields device, Green Mask
    FLONG   flBlue;                     // For bitfields device, Blue Mask
    ULONG   cPaletteShift;              // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ULONG   ulBitCount;                 // # of bits per pel 8,16,24,32 are only supported.
    POINTL  ptlHotSpot;                 // adjustment for pointer hot spot
    VIDEO_POINTER_CAPABILITIES PointerCapabilities; // HW pointer abilities
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes; // hardware pointer attributes
    DWORD   cjPointerAttributes;        // Size of buffer allocated
    BOOL    fHwCursorActive;            // Are we currently using the hw cursor
    PALETTEENTRY *pPal;                 // If this is pal managed, this is the pal
    BOOL    bSupportDCI;                // Does the miniport support DCI?
// eVb: 3.1 [DDK Change] - Support new VGA Miniport behavior w.r.t updated framebuffer remapping
    LONG flHooks;
// eVb: 3.1 [END]
// eVb: 3.1 [VGARISC Change] - Add new fields for VGA support
    SURFOBJ* pso;
    UCHAR* pjBase;
// eVb: 3.1 [END]
} PDEV, *PPDEV;

DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
BOOL bInitPDEV(PPDEV, PDEVMODEW, GDIINFO *, DEVINFO *);
BOOL bInitSURF(PPDEV, BOOL);
BOOL bInitPaletteInfo(PPDEV, DEVINFO *);
BOOL bInitPointer(PPDEV, DEVINFO *);
BOOL bInit256ColorPalette(PPDEV);
VOID vDisablePalette(PPDEV);
VOID vDisableSURF(PPDEV);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0

// eVb: 3.2 [VGARISC Change] - Transform into VGA driver
#define DLL_NAME                L"vga"   // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "Vga risc: "  // All debug output is prefixed
#define ALLOC_TAG               'rgvD'        // Four byte tag (characters in
                                              // reverse order) used for memory
                                              // allocations
// eVb: 3.2 [END]

// eVb: 3.3 [VGARISC Change] - Add new macros for VGA usage
//
// Each pixel in 4BPP being a nibble, the color data for that pixel is thus
// located at the xth bit within the nibble, where x is the plane number [0-3].
// Each nibble being 4 bytes, the color data is thus at the (nibble * 4 + x).
// That color data is then taken from its linear position and shifted to the
// correct position within the 16-bit planar buffer word.
//
#define VAL(data, px, pl, pos)   ((data) >> (((px) * 4) + (pl)) & 1) << (pos)

//
// This figures out which pixel in the planar word data corresponds to which pixel
// in the 4BPP linear data.
//
#define SET_PLANE_DATA(x, y, a, b)  \
    (x) |= VAL(y, (((-1 + ((((b) % 8) % 2) << 1) - (((b) % 8) + 1) + 8))), a, b)

/* Alignment Macros */
#define ALIGN_DOWN_BY(size, align) \
    ((ULONG_PTR)(size) & ~((ULONG_PTR)(align) - 1))

#define ALIGN_UP_BY(size, align) \
    (ALIGN_DOWN_BY(((ULONG_PTR)(size) + align - 1), align))
// eVb: 3.3 [END]
