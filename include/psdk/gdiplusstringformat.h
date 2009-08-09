/*
 * GdiPlusStringFormat.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSSTRINGFORMAT_H
#define _GDIPLUSSTRINGFORMAT_H

class StringFormat : public GdiplusBase
{
public:
  StringFormat(INT formatFlags, LANGID language)
  {
  }

  StringFormat(const StringFormat *format)
  {
  }

  StringFormat *Clone(VOID)
  {
    return NULL;
  }

  static const StringFormat *GenericDefault(VOID)
  {
    return NULL;
  }

  static const StringFormat *GenericTypographic(VOID)
  {
    return NULL;
  }

  StringAlignment GetAlignment(VOID) const
  {
    return StringAlignmentNear;
  }

  LANGID GetDigitSubstitutionLanguage(VOID) const
  {
    return 0;
  }

  StringDigitSubstitute GetDigitSubstitutionMethod(VOID) const
  {
    return StringDigitSubstituteNone;
  }

  INT GetFormatFlags(VOID) const
  {
    return 0;
  }

  HotkeyPrefix GetHotkeyPrefix(VOID) const
  {
    return HotkeyPrefixNone;
  }

  Status GetLastStatus(VOID) const
  {
    return NotImplemented;
  }

  StringAlignment GetLineAlignment(VOID) const
  {
    return StringAlignmentNear;
  }

  INT GetMeasurableCharacterRangeCount(VOID) const
  {
    return 0;
  }

  INT GetTabStopCount(VOID) const
  {
    return 0;
  }

  Status GetTabStops(INT count, REAL *firstTabOffset, REAL *tabStops) const
  {
    return NotImplemented;
  }

  StringTrimming GetTrimming(VOID) const
  {
    return StringTrimmingNone;
  }

  Status SetAlignment(StringAlignment align)
  {
    return NotImplemented;
  }

  Status SetDigitSubstitution(LANGID language, StringDigitSubstitute substitute)
  {
    return NotImplemented;
  }

  Status SetFormatFlags(INT flags)
  {
    return NotImplemented;
  }

  Status SetHotkeyPrefix(HotkeyPrefix hotkeyPrefix)
  {
    return NotImplemented;
  }

  Status SetLineAlignment(StringAlignment align)
  {
    return NotImplemented;
  }

  Status SetMeasurableCharacterRanges(INT rangeCount, const CharacterRange *ranges)
  {
    return NotImplemented;
  }

  Status SetTabStops(REAL firstTabOffset, INT count, const REAL *tabStops)
  {
    return NotImplemented;
  }

  Status SetTrimming(StringTrimming trimming)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSSTRINGFORMAT_H */
