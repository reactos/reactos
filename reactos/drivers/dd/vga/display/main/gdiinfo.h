#include "../vgaddi.h"

GDIINFO gaulCap = {
  GDI_DRIVER_VERSION,
  DT_RASDISPLAY,         // ulTechnology
  0,                     // ulHorzSize
  0,                     // ulVertSize
  0,                     // ulHorzRes (filled in at initialization)
  0,                     // ulVertRes (filled in at initialization)
  4,                     // cBitsPixel
  1,                     // cPlanes
  16,                    // ulNumColors
  0,                     // flRaster (DDI reserved field)

  0,                     // ulLogPixelsX (filled in at initialization)
  0,                     // ulLogPixelsY (filled in at initialization)

  TC_RA_ABLE | TC_SCROLLBLT,  // flTextCaps

  6,                     // ulDACRed
  6,                     // ulDACGree
  6,                     // ulDACBlue

  0x0024,                // ulAspectX  (one-to-one aspect ratio)
  0x0024,                // ulAspectY
  0x0033,                // ulAspectXY

  1,                     // xStyleStep
  1,                     // yStyleSte;
  3,                     // denStyleStep

  { 0, 0 },              // ptlPhysOffset
  { 0, 0 },              // szlPhysSize

  0,                     // ulNumPalReg (win3.1 16 color drivers say 0 too)

// These fields are for halftone initialization.

  {                                         // ciDevice, ColorInfo
    { 6700, 3300, 0 },                      // Red
    { 2100, 7100, 0 },                      // Green
    { 1400,  800, 0 },                      // Blue
    { 1750, 3950, 0 },                      // Cyan
    { 4050, 2050, 0 },                      // Magenta
    { 4400, 5200, 0 },                      // Yellow
    { 3127, 3290, 0 },                      // AlignmentWhite
    20000,                                  // RedGamma
    20000,                                  // GreenGamma
    20000,                                  // BlueGamma
    0, 0, 0, 0, 0, 0
  },

  0,                                         // ulDevicePelsDPI
  PRIMARY_ORDER_CBA,                         // ulPrimaryOrder
  HT_PATSIZE_4x4_M,                          // ulHTPatternSize
  HT_FORMAT_4BPP_IRGB,                       // ulHTOutputFormat
  HT_FLAG_ADDITIVE_PRIMS,                    // flHTFlags

  0,                                         // ulVRefresh
  8,                                         // ulBltAlignment
  0,                                         // ulPanningHorzRes
  0,                                         // ulPanningVertRes
};

// Palette for VGA

typedef struct _VGALOGPALETTE
{
   USHORT ident;
   USHORT NumEntries;
   PALETTEENTRY PaletteEntry[16];
} VGALOGPALETTE;

const VGALOGPALETTE VGApalette =
{

0x400,  // driver version
16,     // num entries
{
  { 0x00, 0x00, 0x00, 0x00 }, // 0
  { 0x80, 0x00, 0x00, 0x00 }, // 1
  { 0x00, 0x80, 0x00, 0x00 }, // 2
  { 0x80, 0x80, 0x00, 0x00 }, // 3
  { 0x00, 0x00, 0x80, 0x00 }, // 4
  { 0x80, 0x00, 0x80, 0x00 }, // 5
  { 0x00, 0x80, 0x80, 0x00 }, // 6
  { 0x80, 0x80, 0x80, 0x00 }, // 7
  { 0xc0, 0xc0, 0xc0, 0x00 }, // 8
  { 0xff, 0x00, 0x00, 0x00 }, // 9
  { 0x00, 0xff, 0x00, 0x00 }, // 10
  { 0xff, 0xff, 0x00, 0x00 }, // 11
  { 0x00, 0x00, 0xff, 0x00 }, // 12
  { 0xff, 0x00, 0xff, 0x00 }, // 13
  { 0x00, 0xff, 0xff, 0x00 }, // 14
  { 0xff, 0xff, 0xff, 0x00 } // 15
}
};

// Devinfo structure passed back to the engine in DrvEnablePDEV

// FIXME: The names of these fonts should be L"..." (For Unicode).. but that
// just doesn't seem compatible with the def of LOGFONT

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE,  "MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE,    "Courier"}

DEVINFO devinfoVGA =
{
  (GCAPS_OPAQUERECT | GCAPS_HORIZSTRIKE | GCAPS_ALTERNATEFILL | GCAPS_MONO_DITHER | GCAPS_COLOR_DITHER |
   GCAPS_WINDINGFILL | GCAPS_DITHERONREALIZE
   ),       // Graphics capabilities

  SYSTM_LOGFONT,  // Default font description
  HELVE_LOGFONT,  // ANSI variable font description
  COURI_LOGFONT,  // ANSI fixed font description
  0,              // Count of device fonts
  BMF_4BPP,       // preferred DIB format
  8,              // Width of color dither
  8,              // Height of color dither
  0               // Default palette to use for this device
};
