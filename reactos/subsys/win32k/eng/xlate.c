/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI Color Translation Functions
 * FILE:             subsys/win32k/eng/xlate.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        8/20/1999: Created
 */

// TODO: Cache XLATEOBJs that are created by EngCreateXlate by checking if the given palettes match a cached list

#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <ddk/ntddvid.h>

#include <include/object.h>
#include "handle.h"

#define NDEBUG
#include <win32k/debug1.h>

ULONG CCMLastSourceColor = 0, CCMLastColorMatch = 0;

ULONG RGBtoULONG(BYTE Red, BYTE Green, BYTE Blue)
{
  return ((Red & 0xff) << 16) | ((Green & 0xff) << 8) | (Blue & 0xff);
}

ULONG BGRtoULONG(BYTE Blue, BYTE Green, BYTE Red)
{
  return ((Blue & 0xff) << 16) | ((Green & 0xff) << 8) | (Red & 0xff);
}

static ULONG ShiftAndMask(XLATEGDI *XlateGDI, ULONG Color)
{
  ULONG TranslatedColor;

  TranslatedColor = 0;
  if (XlateGDI->RedShift < 0)
  {
    TranslatedColor = (Color >> -(XlateGDI->RedShift)) & XlateGDI->RedMask;
  } else
    TranslatedColor = (Color << XlateGDI->RedShift) & XlateGDI->RedMask;
  if (XlateGDI->GreenShift < 0)
  {
    TranslatedColor |= (Color >> -(XlateGDI->GreenShift)) & XlateGDI->GreenMask;
  } else
    TranslatedColor |= (Color << XlateGDI->GreenShift) & XlateGDI->GreenMask;
  if (XlateGDI->BlueShift < 0)
  {
    TranslatedColor |= (Color >> -(XlateGDI->BlueShift)) & XlateGDI->BlueMask;
  } else
    TranslatedColor |= (Color << XlateGDI->BlueShift) & XlateGDI->BlueMask;

  return TranslatedColor;
}


// FIXME: If the caller knows that the destinations are indexed and not RGB
// then we should cache more than one value. Same with the source.

// Takes indexed palette and a
ULONG ClosestColorMatch(XLATEGDI *XlateGDI, ULONG SourceColor, ULONG *DestColors,
                        ULONG NumColors)
{
  PVIDEO_CLUTDATA cSourceColor;
  PVIDEO_CLUTDATA cDestColors;
  LONG idx = 0, i, rt;
  ULONG SourceRGB;
  ULONG SourceRed, SourceGreen, SourceBlue;
  ULONG cxRed, cxGreen, cxBlue, BestMatch = 16777215;

  // Simple cache -- only one value because we don't want to waste time
  // if the colors aren't very sequential

  if(SourceColor == CCMLastSourceColor)
  {
    return CCMLastColorMatch;
  }

  if (PAL_BITFIELDS == XlateGDI->XlateObj.iSrcType)
    {
    /* FIXME: must use bitfields */
    SourceRGB = ShiftAndMask(XlateGDI, SourceColor);
    cSourceColor = (PVIDEO_CLUTDATA) &SourceRGB;
/*
    SourceRed = (SourceColor >> 7) & 0xff;
    SourceGreen = (SourceColor >> 2) & 0xff;
    SourceBlue = (SourceColor << 3) & 0xff;
*/
    }
  else
    {
    cSourceColor = (PVIDEO_CLUTDATA)&SourceColor;
    } 
  SourceRed = cSourceColor->Red;
  SourceGreen = cSourceColor->Green;
  SourceBlue = cSourceColor->Blue;
  for (i=0; i<NumColors; i++)
  {
    cDestColors = (PVIDEO_CLUTDATA)&DestColors[i];

    cxRed = (SourceRed - cDestColors->Red);
	cxRed *= cxRed;  //compute cxRed squared
    cxGreen = (SourceGreen - cDestColors->Green);
	cxGreen *= cxGreen;
    cxBlue = (SourceBlue - cDestColors->Blue);
	cxBlue *= cxBlue;

    rt = /* sqrt */ (cxRed + cxGreen + cxBlue);

    if(rt<=BestMatch)
    {
      idx = i;
      BestMatch = rt;
    }
  }

  CCMLastSourceColor = SourceColor;
  CCMLastColorMatch  = idx;

  return idx;
}

VOID IndexedToIndexedTranslationTable(XLATEGDI *XlateGDI, ULONG *TranslationTable,
                                      PALGDI *PalDest, PALGDI *PalSource)
{
  ULONG i;

  for(i=0; i<PalSource->NumColors; i++)
  {
    TranslationTable[i] = ClosestColorMatch(XlateGDI, PalSource->IndexedColors[i], PalDest->IndexedColors, PalDest->NumColors);
  }
}

static VOID BitMasksFromPal(USHORT PalType, PPALGDI Palette,
                            PULONG RedMask, PULONG BlueMask, PULONG GreenMask)
{
  switch(PalType)
  {
    case PAL_RGB:
      *RedMask = RGB(255, 0, 0);
      *GreenMask = RGB(0, 255, 0);
      *BlueMask = RGB(0, 0, 255);
      break;
    case PAL_BGR:
      *RedMask = RGB(0, 0, 255);
      *GreenMask = RGB(0, 255, 0);
      *BlueMask = RGB(255, 0, 0);
      break;
    case PAL_BITFIELDS:
      *RedMask = Palette->RedMask;
      *BlueMask = Palette->BlueMask;
      *GreenMask = Palette->GreenMask;
      break;
  }
}

/*
 * Calculate the number of bits Mask must be shift to the left to get a
 * 1 in the most significant bit position
 */
static INT CalculateShift(ULONG Mask)
{
   INT Shift = 0;
   ULONG LeftmostBit = 1 << (8 * sizeof(ULONG) - 1);

   while (0 == (Mask & LeftmostBit) && Shift < 8 * sizeof(ULONG))
     {
     Mask = Mask << 1;
     Shift++;
     }

   return Shift;
}

XLATEOBJ *IntEngCreateXlate(USHORT DestPalType, USHORT SourcePalType,
                            HPALETTE PaletteDest, HPALETTE PaletteSource)
{
  // FIXME: Add support for BGR conversions

  HPALETTE NewXlate;
  XLATEOBJ *XlateObj;
  XLATEGDI *XlateGDI;
  PALGDI   *SourcePalGDI, *DestPalGDI;
  ULONG    IndexedColors;
  ULONG    SourceRedMask, SourceGreenMask, SourceBlueMask;
  ULONG    DestRedMask, DestGreenMask, DestBlueMask;
  UINT     i;

  NewXlate = (HPALETTE)CreateGDIHandle(sizeof( XLATEGDI ), sizeof( XLATEOBJ ));
  if( !ValidEngHandle( NewXlate ) )
	return NULL;

  XlateObj = (XLATEOBJ*) AccessUserObject( (ULONG) NewXlate );
  XlateGDI = (XLATEGDI*) AccessInternalObject( (ULONG) NewXlate );
  ASSERT( XlateObj );
  ASSERT( XlateGDI );

  SourcePalGDI = (PALGDI*)AccessInternalObject((ULONG)PaletteSource);
  DestPalGDI = (PALGDI*)AccessInternalObject((ULONG)PaletteDest);

  XlateObj->iSrcType = SourcePalType;
  XlateObj->iDstType = DestPalType;

  // Store handles of palettes in internal Xlate GDI object (or NULLs)
  XlateGDI->DestPal   = PaletteDest;
  XlateGDI->SourcePal = PaletteSource;

  XlateObj->flXlate = 0;

  XlateGDI->UseShiftAndMask = FALSE;

  /* Compute bit fiddeling constants unless both palettes are indexed, then we don't need them */
  if (PAL_INDEXED != SourcePalType || PAL_INDEXED != DestPalType)
  {
    BitMasksFromPal(PAL_INDEXED == SourcePalType ? PAL_RGB : SourcePalType,
                    SourcePalGDI, &SourceRedMask, &SourceBlueMask, &SourceGreenMask);
    BitMasksFromPal(PAL_INDEXED == DestPalType ? PAL_RGB : DestPalType,
                    DestPalGDI, &DestRedMask, &DestBlueMask, &DestGreenMask);
    XlateGDI->RedShift = CalculateShift(SourceRedMask) - CalculateShift(DestRedMask);
    XlateGDI->RedMask = DestRedMask;
    XlateGDI->GreenShift = CalculateShift(SourceGreenMask) - CalculateShift(DestGreenMask);
    XlateGDI->GreenMask = DestGreenMask;
    XlateGDI->BlueShift = CalculateShift(SourceBlueMask) - CalculateShift(DestBlueMask);
    XlateGDI->BlueMask = DestBlueMask;
  }

  // If source and destination palettes are the same or if they're RGB/BGR
  if( (PaletteDest == PaletteSource) ||
      ((DestPalType == PAL_RGB) && (SourcePalType == PAL_RGB)) ||
      ((DestPalType == PAL_BGR) && (SourcePalType == PAL_BGR)) )
  {
    XlateObj->flXlate |= XO_TRIVIAL;
    return XlateObj;
  }

  /* If source and destination are bitfield based (RGB and BGR are just special bitfields) */
  if ((PAL_RGB == DestPalType || PAL_BGR == DestPalType || PAL_BITFIELDS == DestPalType) &&
      (PAL_RGB == SourcePalType || PAL_BGR == SourcePalType || PAL_BITFIELDS == SourcePalType))
  {
    if (SourceRedMask == DestRedMask &&
        SourceBlueMask == DestBlueMask &&
        SourceGreenMask == DestGreenMask)
      {
      XlateObj->flXlate |= XO_TRIVIAL;
      }
    XlateGDI->UseShiftAndMask = TRUE;
    return XlateObj;
  }

  // Prepare the translation table
  if( (SourcePalType == PAL_INDEXED) || (SourcePalType == PAL_RGB) )
  {
    XlateObj->flXlate |= XO_TABLE;
    if ((SourcePalType == PAL_INDEXED) && (DestPalType == PAL_INDEXED))
    {
      if(SourcePalGDI->NumColors > DestPalGDI->NumColors)
      {
        IndexedColors = SourcePalGDI->NumColors;
      } else
        IndexedColors = DestPalGDI->NumColors;
    }
    else if (SourcePalType == PAL_INDEXED) { IndexedColors = SourcePalGDI->NumColors; }
    else if (DestPalType   == PAL_INDEXED) { IndexedColors = DestPalGDI->NumColors; }

    XlateGDI->translationTable = EngAllocMem(FL_ZERO_MEMORY, sizeof(ULONG)*IndexedColors, 0);
  }

  // Source palette is indexed
  if(XlateObj->iSrcType == PAL_INDEXED)
  {
    if(XlateObj->iDstType == PAL_INDEXED)
    {
      // Converting from indexed to indexed
      IndexedToIndexedTranslationTable(XlateGDI, XlateGDI->translationTable, DestPalGDI, SourcePalGDI);
    } else
      if (PAL_RGB == XlateObj->iDstType || PAL_BITFIELDS == XlateObj->iDstType )
      {
        // FIXME: Is this necessary? I think the driver has to call this
        // function anyways if pulXlate is NULL and Source is PAL_INDEXED

        // Converting from indexed to RGB

        XLATEOBJ_cGetPalette(XlateObj, XO_SRCPALETTE,
                             SourcePalGDI->NumColors,
                             XlateGDI->translationTable);
	if (PAL_BITFIELDS == XlateObj->iDstType)
	{
	  for (i = 0; i < SourcePalGDI->NumColors; i++)
	  {
	  XlateGDI->translationTable[i] = ShiftAndMask(XlateGDI, XlateGDI->translationTable[i]);
	  }
	}
      }

    XlateObj->pulXlate = XlateGDI->translationTable;
  }

  // Source palette is RGB
  if(XlateObj->iSrcType == PAL_RGB)
  {
    if(XlateObj->iDstType == PAL_INDEXED)
    {
      // FIXME: Is this necessary? I think the driver has to call this
      // function anyways if pulXlate is NULL and Dest is PAL_INDEXED

      // Converting from RGB to indexed
      XLATEOBJ_cGetPalette(XlateObj, XO_DESTPALETTE, DestPalGDI->NumColors, XlateGDI->translationTable);
    }
  }

  // FIXME: Add support for XO_TO_MONO
  return XlateObj;
}

VOID EngDeleteXlate(XLATEOBJ *XlateObj)
{
  HPALETTE HXlate    = (HPALETTE)AccessHandleFromUserObject(XlateObj);
  XLATEGDI *XlateGDI = (XLATEGDI*)AccessInternalObject((ULONG)HXlate);

  if(XlateGDI->translationTable!=NULL)
  {
    EngFreeMem(XlateGDI->translationTable);
  }

  FreeGDIHandle((ULONG)HXlate);
}

ULONG * STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
  XLATEGDI *XlateGDI = (XLATEGDI*)AccessInternalObjectFromUserObject(XlateObj);

  if(XlateObj->iSrcType == PAL_INDEXED)
  {
    return XlateGDI->translationTable;
  }

  return NULL;
}

ULONG STDCALL
XLATEOBJ_iXlate(XLATEOBJ *XlateObj,
		ULONG Color)
{
  PALGDI   *PalGDI;
  XLATEGDI *XlateGDI = (XLATEGDI*)AccessInternalObjectFromUserObject(XlateObj);

  // Return the original color if there's no color translation object
  if(!XlateObj) return Color;

  if(XlateObj->flXlate & XO_TRIVIAL)
  {
    return Color;
  } else
  if(XlateGDI->UseShiftAndMask)
  {
    return ShiftAndMask(XlateGDI, Color);
  } else
  if(PAL_RGB == XlateObj->iSrcType || PAL_BITFIELDS == XlateObj->iSrcType)
  {
    // FIXME: should we cache colors used often?
    // FIXME: won't work if destination isn't indexed

    // Extract the destination palette
    PalGDI = (PALGDI*)AccessInternalObject((ULONG)XlateGDI->DestPal);

    // Return closest match for the given color
    return ClosestColorMatch(XlateGDI, Color, PalGDI->IndexedColors, PalGDI->NumColors);
  } else
  if(XlateObj->iSrcType == PAL_INDEXED)
  {
    return XlateGDI->translationTable[Color];
  }

  return 0;
}

ULONG STDCALL
XLATEOBJ_cGetPalette(XLATEOBJ *XlateObj,
		     ULONG PalOutType,
		     ULONG cPal,
		     ULONG *OutPal)
{
  ULONG i;
  HPALETTE HPal;
  XLATEGDI *XlateGDI;
  PALGDI *PalGDI;

  XlateGDI = (XLATEGDI*)AccessInternalObjectFromUserObject(XlateObj);

  if(PalOutType == XO_SRCPALETTE)
  {
    HPal = XlateGDI->SourcePal;
  } else
  if(PalOutType == XO_DESTPALETTE)
  {
    HPal = XlateGDI->DestPal;
  }

  PalGDI = (PALGDI*)AccessInternalObject((ULONG)HPal);
  RtlCopyMemory(OutPal, PalGDI->IndexedColors, sizeof(ULONG)*cPal);

  return i;
}
