#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <ddk/ntddvid.h>

HANDLE GdiHeap;

#define DS_SOLIDBRUSH  0x00000001
#define DS_GREYBRUSH   0x00000002
#define DS_BRUSH       0x00000004
#define DS_DIB         0x00000008

#define POW2(stride) (!((stride) & ((stride)-1)))   // TRUE if stride is power of 2
#define BROKEN_RASTERS(stride,cy) ((!(POW2(stride))) && ((stride*cy) > 0x10000))

#define DM_SPECVERSION 1 // FIXME: What is this really?
#define ENUM_RECT_LIMIT   50

typedef struct _RECT_ENUM
{
   ULONG c;
   RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;

// Cursor coordinates
typedef struct  _XYPAIR
{
   USHORT  x;
   USHORT  y;
} XYPAIR;

// Cursor states
#define CURSOR_DOWN      0x00000001
#define CURSOR_COLOR     0x00000004
#define CURSOR_HW        0x00000010
#define CURSOR_HW_ACTIVE 0x00000020
#define CURSOR_ANIMATE   0x00000040

typedef struct _PDEV
{
   ULONG fl; // driver flags

   // Handles
   HANDLE KMDriver;
   HDEV   GDIDevHandle;   // engine's handle to PDEV
   HSURF  SurfHandle;     // engine's handle to surface
   PVOID  AssociatedSurf; // associated surface

   // Cursor
   XYPAIR xyCursor;  // cursor position
   POINTL ptlExtent; // cursor extent
   ULONG  cExtent;   // effective cursor extent
   ULONG  flCursor;  // cursor status

   // Pointer
   PVIDEO_POINTER_ATTRIBUTES pPointerAttributes; // HW Pointer Attributes
   ULONG   XorMaskStartOffset;         // Start offset of hardware pointer
                                       //  XOR mask relative to AND mask for
                                       //  passing to HW pointer
   DWORD   PointerAttributes;          // Size of buffer allocated
   DWORD   flPreallocSSBBufferInUse;   // True if preallocated saved screen
                                       //  bits buffer is in use
   PUCHAR  pjPreallocSSBBuffer;        // Pointer to preallocated saved screen
                                       //  bits buffer, if there is one
   ULONG   ulPreallocSSBSize;          // Size of preallocated saved screen
                                       //  bits buffer
   VIDEO_POINTER_CAPABILITIES PointerCapabilities; // HW pointer abilities
   PUCHAR  pucDIB4ToVGAConvBuffer;     // DIB4->VGA conversion table buffer
   PUCHAR  pucDIB4ToVGAConvTables;     // Pointer to DIB4->VGA conversion

   // Misc

   ULONG  ModeNum;   // mode index for current VGA mode

   SIZEL  sizeSurf;  // displayed size of the surface

   PBYTE  fbScreen;  // pointer to the frame buffer

   RECTL  SavedBitsRight;  // invisible part right of screen
   RECTL  SavedBitsBottom; // invisible part at the bottom of the screen
   BOOL   BitsSaved;       // TRUE if bits are currently saved
   SIZEL  sizeMem;         // actual size (in pixels) of video memory
   LONG   NumScansUsedByPointer; // # scans of offscreen memory used by

} PDEV, *PPDEV;

typedef struct {
   RECTL BankBounds; // Pixels addressable in this bank
   ULONG BankOffset; // Offset of bank start from bitmap start if linearly addressable
} BANK_INFO, *PBANK_INFO;

typedef enum {
   JustifyTop = 0,
   JustifyBottom,
} BANK_JUST;

// bank control function vector
//typedef VOID (*PFN_BankControl)(PDEVSURF, ULONG, BANK_JUST);
typedef VOID (*PFN_BankControl)(PVOID, ULONG, BANK_JUST);

// descriptor for a saved screen bits block

typedef struct  _SAVED_SCREEN_BITS
{
   BOOL  bFlags;
   PBYTE pjBuffer;  // pointer to save buffer start
   ULONG ulSize;    // size of save buffer (per plane; display memory only)
   ULONG ulSaveWidthInBytes; // # of bytes across save area (including
                             //  partial edge bytes, if any)
   ULONG ulDelta;   // # of bytes from end of one saved scan's saved bits to
                    //  start of next (system memory only)
   PVOID pvNextSSB; // pointer to next saved screen bits block
                    // for system memory blocks, saved bits start immediately
                    //  after this structure
} SAVED_SCREEN_BITS, *PSAVED_SCREEN_BITS;

// DEVSURF -- definition of a surface as seen and used by the various VGA
// drivers

typedef struct _DEVSURF
{
   IDENT ident;  // Identifier for debugging ease
   ULONG flSurf; // DS_ flags as defined below
   BYTE  Color;  // Solid color surface if DS_SOLIDBRUSH

// If DS_SOLIDBRUSH, the following fields are undefined and not guaranteed to
// have been allocated!

   BYTE  Format;     // BMF_*, BMF_PHYSDEVICE
   BYTE  jReserved1; // Reserved
   BYTE  jReserved2; // Reserved
   PPDEV ppdev;      // Pointer to associated PDEV
   SIZEL sizeSurf;   // Size of the surface
   ULONG NextScan;   // Offset from scan  "n" to "n+1"
   ULONG NextPlane;  // Offset from plane "n" to "n+1"
   PVOID Scan0;      // Pointer to scan 0 of bitmap
                     // (actual address of start of bank, for banked VGA surface)
   PVOID StartBmp;   // Pointer to start of bitmap
   PVOID Conv;       // Pointer to DIB/Planer conversion buffer

// Banking variables; used only for banked VGA surfaces

   PVIDEO_BANK_SELECT BankSelectInfo;
   ULONG       Bank2RWSkip;            // Offset from one bank index to next to make two 32K banks appear to be
                                       // one seamless 64K bank
   PFN         pfnBankSwitchCode;
   VIDEO_BANK_TYPE BankingType;
   ULONG       BitmapSize;             // Length of bitmap if there were no banking, in CPU addressable bytes
   ULONG PtrBankScan;       // Last scan line in pointer work bank
   RECTL WindowClip1;       // Single-window banking clip rect
   RECTL WindowClip2[2];    // Double-window banking clip rects for
                            //  windows 0 & 1
   ULONG WindowBank[2];     // Current banks mapped into windows
                            //  0 & 1 (used in 2 window mode only)
   PBANK_INFO  BankInfo;         // Pointer to array of bank clip info
   ULONG       BankInfoLength;    // Length of pbiBankInfo, in entries
   PBANK_INFO  BankInfo2RW;      // Same as above, but for 2RW window
   ULONG       BankInfo2RWLength; // case
   PFN_BankControl pfnBankControl;     // Pointer to bank control function
   PFN_BankControl pfnBankControl2Window; // Pointer to double-window bank
                                       //  control function
   PVOID BitmapStart;   // Single-window bitmap start pointer (adjusted as
                        // necessary to make window map in at proper offset)
   PVOID BitmapStart2Window[2];   // Double-window window 0 and 1 bitmap start
   PVOID BankBufferPlane0;        // Pointer to temp buffer capable of
                                  // storing one full bank for plane 0 for 1
                                  // R/W case; capable of storing  one full
                                  // bank height of edge bytes for all four
                                  // planes for the 1R/1W case. Also used to
                                  // point to text building buffer in all
                                  // cases. This is the pointer used to
                                  // dealloc bank working storage for all
                                  // four planes

   // The following 3 pointers used by 1 R/W banked devices

   PVOID       BankBufferPlane1; // Like above, but for plane 1
   PVOID       BankBufferPlane2; // Like above, but for plane 2
   PVOID       BankBufferPlane3; // Like above, but for plane 3
   ULONG       TempBufferSize;   // Full size of temp buffer pointed to
                                 // by pvBankBufferPlane0

   ULONG       ajBits[1];           // Bits will start here for device bitmaps
   PSAVED_SCREEN_BITS ssbList;      // Pointer to start of linked list of
                                    //  saved screen bit blocks
} DEVSURF, *PDEVSURF;

typedef VOID (*PFN_ScreenToScreenBlt)(PDEVSURF, PRECTL, PPOINTL, INT);

// BMF_PHYSDEVICE format type

#define BMF_PHYSDEVICE  0xFF
#define BMF_DFB         0xFE

// Identifiers used in debugging (DEVSURF.ident)

#define PDEV_IDENT      ('V' + ('P' << 8) + ('D' << 16) + ('V' << 24))
#define DEVSURF_IDENT   ('V' + ('S' << 8) + ('R' << 16) + ('F' << 24))

BOOL InitVGA(PPDEV ppdev, BOOL bFirst); // screen.c: initialize VGA mode

#define DRIVER_EXTRA_SIZE 0
#define ALLOC_TAG 'agvD' // Dvga reversed
#define DLL_NAME  L"vga" // DLL name in Unicode

#define MAX_SCAN_WIDTH              2048  // pixels
#define DRIVER_OFFSCREEN_REFRESHED  0x04L // if not set, don't use offscreen memory
#define PLANAR_PELS_PER_CPU_ADDRESS 8
#define PACKED_PELS_PER_CPU_ADDRESS 2
