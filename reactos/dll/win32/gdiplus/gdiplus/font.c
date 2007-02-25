#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFontFromDC(HDC hdc,
  GpFont **font)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFontFromLogfontA(HDC hdc,
  GDIPCONST LOGFONTA *logfont,
  GpFont **font)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFontFromLogfontW(HDC hdc,
  GDIPCONST LOGFONTW *logfont,
  GpFont **font)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFont(GDIPCONST GpFontFamily *fontFamily,
  REAL emSize,
  INT style,
  Unit unit,
  GpFont **font)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneFont(GpFont* font,
  GpFont** cloneFont)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteFont(GpFont* font)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFamily(GpFont *font,
  GpFontFamily **family)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontStyle(GpFont *font,
  INT *style)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontSize(GpFont *font,
  REAL *size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontUnit(GpFont *font,
  Unit *unit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontHeight(GDIPCONST GpFont *font,
  GDIPCONST GpGraphics *graphics,
  REAL *height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontHeightGivenDPI(GDIPCONST GpFont *font,
  REAL dpi,
  REAL *height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLogFontA(GpFont * font,
  GpGraphics *graphics,
  LOGFONTA * logfontA)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLogFontW(GpFont * font,
  GpGraphics *graphics,
  LOGFONTW * logfontW)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipNewInstalledFontCollection(GpFontCollection** fontCollection)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipNewPrivateFontCollection(GpFontCollection** fontCollection)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeletePrivateFontCollection(GpFontCollection** fontCollection)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontCollectionFamilyCount(GpFontCollection* fontCollection,
  INT * numFound)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFontCollectionFamilyList(GpFontCollection* fontCollection,
  INT numSought,
  GpFontFamily* gpfamilies[],
  INT* numFound)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPrivateAddFontFile(GpFontCollection* fontCollection,
  GDIPCONST WCHAR* filename)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPrivateAddMemoryFont(GpFontCollection* fontCollection,
  GDIPCONST void* memory,
  INT length)
{
  return NotImplemented;
}


/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFontFamilyFromName(GDIPCONST WCHAR *name,
  GpFontCollection *fontCollection,
  GpFontFamily **FontFamily)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteFontFamily(GpFontFamily *FontFamily)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneFontFamily(GpFontFamily *FontFamily,
  GpFontFamily **clonedFontFamily)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetGenericFontFamilySansSerif(GpFontFamily **nativeFamily)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetGenericFontFamilySerif(GpFontFamily **nativeFamily)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetGenericFontFamilyMonospace(GpFontFamily **nativeFamily)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetFamilyName(GDIPCONST GpFontFamily *family,
  WCHAR name[LF_FACESIZE],
  LANGID language)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsStyleAvailable(GDIPCONST GpFontFamily *family,
  INT style,
  BOOL * IsStyleAvailable)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipFontCollectionEnumerable(GpFontCollection* fontCollection,
  GpGraphics* graphics,
  INT * numFound)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipFontCollectionEnumerate(GpFontCollection* fontCollection,
  INT numSought,
  GpFontFamily* gpfamilies[],
  INT* numFound,
  GpGraphics* graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetEmHeight(GDIPCONST GpFontFamily *family,
  INT style,
  UINT16 * EmHeight)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetCellAscent(GDIPCONST GpFontFamily *family,
  INT style,
  UINT16 * CellAscent)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetCellDescent(GDIPCONST GpFontFamily *family,
  INT style,
  UINT16 * CellDescent)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineSpacing(GDIPCONST GpFontFamily *family,
  INT style,
  UINT16 * LineSpacing)
{
  return NotImplemented;
}
