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

//#define NDEBUG
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


// FIXME: If the caller knows that the destinations are indexed and not RGB
// then we should cache more than one value. Same with the source.

// Takes indexed palette and a
ULONG ClosestColorMatch(ULONG SourceColor, ULONG *DestColors,
                        ULONG NumColors)
{
  PVIDEO_CLUTDATA cSourceColor;
  PVIDEO_CLUTDATA cDestColors;
  LONG idx = 0, i, rt;
  ULONG cxRed, cxGreen, cxBlue, BestMatch = 16777215;

  // Simple cache -- only one value because we don't want to waste time
  // if the colors aren't very sequential

  if(SourceColor == CCMLastSourceColor)
  {
    return CCMLastColorMatch;
  }

  cSourceColor = (PVIDEO_CLUTDATA)&SourceColor;
  for (i=0; i<NumColors; i++)
  {
    cDestColors = (PVIDEO_CLUTDATA)&DestColors[i];

    cxRed = (cSourceColor->Red - cDestColors->Red);
	cxRed *= cxRed;  //compute cxRed squared
    cxGreen = (cSourceColor->Green - cDestColors->Green);
	cxGreen *= cxGreen;
    cxBlue = (cSourceColor->Blue - cDestColors->Blue);
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

VOID IndexedToIndexedTranslationTable(ULONG *TranslationTable,
                                      PALGDI *PalDest, PALGDI *PalSource)
{
  ULONG i;

  for(i=0; i<PalSource->NumColors; i++)
  {
    TranslationTable[i] = ClosestColorMatch(PalSource->IndexedColors[i], PalDest->IndexedColors, PalDest->NumColors);
  }
}

XLATEOBJ *EngCreateXlate(USHORT DestPalType, USHORT SourcePalType,
                         HPALETTE PaletteDest, HPALETTE PaletteSource)
{
  // FIXME: Add support for BGR conversions

  HPALETTE NewXlate;
  XLATEOBJ *XlateObj;
  XLATEGDI *XlateGDI;
  PALGDI   *SourcePalGDI, *DestPalGDI;
  ULONG    IndexedColors;

  NewXlate = (HPALETTE)CreateGDIHandle(sizeof( XLATEGDI ), sizeof( XLATEOBJ ));
  if( !ValidEngHandle( NewXlate ) )
	return NULL;

  XlateObj = (XLATEOBJ*) AccessUserObject( NewXlate );
  XlateGDI = (XLATEGDI*) AccessInternalObject( NewXlate );
  ASSERT( XlateObj );
  ASSERT( XlateGDI );

  if(SourcePalType == PAL_INDEXED)
    SourcePalGDI = (PALGDI*)AccessInternalObject((ULONG)PaletteSource);
  if(DestPalType == PAL_INDEXED)
    DestPalGDI = (PALGDI*)AccessInternalObject((ULONG)PaletteDest);

  XlateObj->iSrcType = SourcePalType;
  XlateObj->iDstType = DestPalType;

  // Store handles of palettes in internal Xlate GDI object (or NULLs)
  XlateGDI->DestPal   = PaletteDest;
  XlateGDI->SourcePal = PaletteSource;

  XlateObj->flXlate = 0;

  // If source and destination palettes are the same or if they're RGB/BGR
  if( (PaletteDest == PaletteSource) ||
      ((DestPalType == PAL_RGB) && (SourcePalType == PAL_RGB)) ||
      ((DestPalType == PAL_BGR) && (SourcePalType == PAL_BGR)) )
  {
    XlateObj->flXlate |= XO_TRIVIAL;
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
      IndexedToIndexedTranslationTable(XlateGDI->translationTable, DestPalGDI, SourcePalGDI);
    } else
      if(XlateObj->iDstType == PAL_RGB)
      {
        // FIXME: Is this necessary? I think the driver has to call this
        // function anyways if pulXlate is NULL and Source is PAL_INDEXED

        // Converting from indexed to RGB

        XLATEOBJ_cGetPalette(XlateObj, XO_SRCPALETTE,
                             SourcePalGDI->NumColors,
                             XlateGDI->translationTable);
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
  if(XlateObj->iSrcType == PAL_RGB)
  {
    // FIXME: should we cache colors used often?
    // FIXME: won't work if destination isn't indexed

    // Extract the destination palette
    PalGDI = (PALGDI*)AccessInternalObject((ULONG)XlateGDI->DestPal);

    // Return closest match for the given RGB color
    return ClosestColorMatch(Color, PalGDI->IndexedColors, PalGDI->NumColors);
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
