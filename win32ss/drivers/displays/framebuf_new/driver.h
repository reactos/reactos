/*
 * PROJECT:         ReactOS Framebuffer Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/displays/framebuf_new/driver.h
 * PURPOSE:         Main Driver Header File
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */

#ifndef _FRAMEBUF_NEW_PCH_
#define _FRAMEBUF_NEW_PCH_

//#define DBG 1
#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>
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
} PDEV, *PPDEV;

DWORD NTAPI getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
BOOL NTAPI bInitPDEV(PPDEV, PDEVMODEW, GDIINFO *, DEVINFO *);
BOOL NTAPI bInitSURF(PPDEV, BOOL);
BOOL NTAPI bInitPaletteInfo(PPDEV, DEVINFO *);
BOOL NTAPI bInitPointer(PPDEV, DEVINFO *);
BOOL NTAPI bInit256ColorPalette(PPDEV);
VOID NTAPI vDisablePalette(PPDEV);
VOID NTAPI vDisableSURF(PPDEV);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0

#define DLL_NAME                L"framebuf"   // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "FRAMEBUF: "  // All debug output is prefixed
#define ALLOC_TAG               'bfDD'        // Four byte tag (characters in
                                              // reverse order) used for memory
                                              // allocations

#endif /* _FRAMEBUF_NEW_PCH_ */
