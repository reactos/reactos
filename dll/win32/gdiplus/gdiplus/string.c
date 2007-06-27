#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateStringFormat(INT formatAttributes,
  LANGID language,
  GpStringFormat **format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipStringFormatGetGenericDefault(GpStringFormat **format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipStringFormatGetGenericTypographic(GpStringFormat **format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteStringFormat(GpStringFormat *format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneStringFormat(GDIPCONST GpStringFormat *format,
  GpStringFormat **newFormat)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatFlags(GpStringFormat *format,
  INT flags)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatFlags(GDIPCONST GpStringFormat *format,
  INT *flags)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatAlign(GpStringFormat *format,
  StringAlignment align)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatAlign(GDIPCONST GpStringFormat *format,
  StringAlignment *align)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatLineAlign(GpStringFormat *format,
  StringAlignment align)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatLineAlign(GDIPCONST GpStringFormat *format,
  StringAlignment *align)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatTrimming(GpStringFormat *format,
  StringTrimming trimming)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatTrimming(GDIPCONST GpStringFormat *format,
  StringTrimming *trimming)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatHotkeyPrefix(GpStringFormat *format,
  INT hotkeyPrefix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatHotkeyPrefix(GDIPCONST GpStringFormat *format,
  INT *hotkeyPrefix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatTabStops(GpStringFormat *format,
  REAL firstTabOffset,
  INT count,
  GDIPCONST REAL *tabStops)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatTabStops(GDIPCONST GpStringFormat *format,
  INT count,
  REAL *firstTabOffset,
  REAL *tabStops)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatTabStopCount(GDIPCONST GpStringFormat *format,
  INT * count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatDigitSubstitution(GpStringFormat *format,
  LANGID language,
  StringDigitSubstitute substitute)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatDigitSubstitution(GDIPCONST GpStringFormat *format,
  LANGID *language,
  StringDigitSubstitute *substitute)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetStringFormatMeasurableCharacterRangeCount(GDIPCONST GpStringFormat *format,
  INT *count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetStringFormatMeasurableCharacterRanges(GpStringFormat *format,
  INT rangeCount,
  GDIPCONST CharacterRange *ranges)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawString(GpGraphics *graphics,
  GDIPCONST WCHAR *string,
  INT length,
  GDIPCONST GpFont *font,
  GDIPCONST RectF *layoutRect,
  GDIPCONST GpStringFormat *stringFormat,
  GDIPCONST GpBrush *brush)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMeasureString(GpGraphics *graphics,
  GDIPCONST WCHAR *string,
  INT length,
  GDIPCONST GpFont *font,
  GDIPCONST RectF *layoutRect,
  GDIPCONST GpStringFormat *stringFormat,
  RectF *boundingBox,
  INT *codepointsFitted,
  INT *linesFilled)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMeasureCharacterRanges(GpGraphics *graphics,
  GDIPCONST WCHAR *string,
  INT length,
  GDIPCONST GpFont *font,
  GDIPCONST RectF *layoutRect,
  GDIPCONST GpStringFormat *stringFormat,
  INT regionCount,
  GpRegion **regions)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawDriverString(GpGraphics *graphics,
  GDIPCONST UINT16 *text,
  INT length,
  GDIPCONST GpFont *font,
  GDIPCONST GpBrush *brush,
  GDIPCONST PointF *positions,
  INT flags,
  GDIPCONST GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMeasureDriverString(GpGraphics *graphics,
  GDIPCONST UINT16 *text,
  INT length,
  GDIPCONST GpFont *font,
  GDIPCONST PointF *positions,
  INT flags,
  GDIPCONST GpMatrix *matrix,
  RectF *boundingBox)
{
  return NotImplemented;
}
