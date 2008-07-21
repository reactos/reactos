/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engpal.c
 * PURPOSE:         Path Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HPALETTE
APIENTRY
EngCreatePalette(
  IN ULONG  iMode,
  IN ULONG  cColors,
  IN ULONG  *pulColors,
  IN FLONG  flRed,
  IN FLONG  flGreen,
  IN FLONG  flBlue)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngDeletePalette(IN HPALETTE  hpal)
{
    UNIMPLEMENTED;
	return FALSE;
}

ULONG
APIENTRY
EngQueryPalette(
  IN HPALETTE  hPal,
  OUT ULONG  *piMode,
  IN ULONG  cColors,
  OUT ULONG  *pulColors)
{
    UNIMPLEMENTED;
	return 0;
}

ULONG
APIENTRY
EngDitherColor(
  IN HDEV  hdev,
  IN ULONG  iMode,
  IN ULONG  rgb,
  OUT PULONG pul)
{
    UNIMPLEMENTED;
	return 0;
}

LONG
APIENTRY
HT_ComputeRGBGammaTable(
  IN USHORT  GammaTableEntries,
  IN USHORT  GammaTableType,
  IN USHORT  RedGamma,
  IN USHORT  GreenGamma,
  IN USHORT  BlueGamma,
  OUT LPBYTE  pGammaTable)
{
    UNIMPLEMENTED;
	return 0;
}

LONG
APIENTRY
HT_Get8BPPFormatPalette(
  OUT LPPALETTEENTRY  pPaletteEntry,
  IN USHORT  RedGamma,
  IN USHORT  GreenGamma,
  IN USHORT  BlueGamma)
{
    UNIMPLEMENTED;
	return 0;
}

LONG
APIENTRY
HT_Get8BPPMaskPalette(
  IN OUT LPPALETTEENTRY  pPaletteEntry,
  IN BOOL  Use8BPPMaskPal,
  IN BYTE  CMYMask,
  IN USHORT  RedGamma,
  IN USHORT  GreenGamma,
  IN USHORT  BlueGamma)
{
    UNIMPLEMENTED;
	return 0;
}

ULONG
APIENTRY
PALOBJ_cGetColors(
  IN PALOBJ  *ppalo,
  IN ULONG  iStart,
  IN ULONG  cColors,
  OUT ULONG  *pulColors)
{
    UNIMPLEMENTED;
	return 0;
}
