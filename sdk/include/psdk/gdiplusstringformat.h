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
    StringFormat(INT formatFlags = 0, LANGID language = LANG_NEUTRAL) : nativeFormat(NULL)
    {
        lastStatus = DllExports::GdipCreateStringFormat(formatFlags, language, &nativeFormat);
    }

    StringFormat(const StringFormat *format) : nativeFormat(NULL)
    {
        lastStatus = DllExports::GdipCloneStringFormat(format ? format->nativeFormat : NULL, &nativeFormat);
    }

    StringFormat *
    Clone() const
    {
        GpStringFormat *cloneFormat = NULL;

        lastStatus = DllExports::GdipCloneStringFormat(nativeFormat, &cloneFormat);
        if (lastStatus != Ok)
            return NULL;

        StringFormat *newFormat = new StringFormat(cloneFormat, lastStatus);
        if (!newFormat)
            DllExports::GdipDeleteStringFormat(cloneFormat);
        return newFormat;
    }

    ~StringFormat()
    {
        DllExports::GdipDeleteStringFormat(nativeFormat);
    }

    static const StringFormat *
    GenericDefault()
    {
        return NULL; // FIXME
    }

    static const StringFormat *
    GenericTypographic()
    {
        return NULL; // FIXME
    }

    StringAlignment
    GetAlignment() const
    {
        StringAlignment alignment;
        SetStatus(DllExports::GdipGetStringFormatAlign(nativeFormat, &alignment));
        return alignment;
    }

    LANGID
    GetDigitSubstitutionLanguage() const
    {
        LANGID language;
        SetStatus(DllExports::GdipGetStringFormatDigitSubstitution(nativeFormat, &language, NULL));
        return language;
    }

    StringDigitSubstitute
    GetDigitSubstitutionMethod() const
    {
        StringDigitSubstitute substitute;
        SetStatus(DllExports::GdipGetStringFormatDigitSubstitution(nativeFormat, NULL, &substitute));
        return substitute;
    }

    INT
    GetFormatFlags() const
    {
        INT flags;
        SetStatus(DllExports::GdipGetStringFormatFlags(nativeFormat, &flags));
        return flags;
    }

    HotkeyPrefix
    GetHotkeyPrefix() const
    {
        HotkeyPrefix hotkeyPrefix;
        SetStatus(DllExports::GdipGetStringFormatHotkeyPrefix(nativeFormat, reinterpret_cast<INT *>(&hotkeyPrefix)));
        return hotkeyPrefix;
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    StringAlignment
    GetLineAlignment() const
    {
        StringAlignment alignment;
        SetStatus(DllExports::GdipGetStringFormatLineAlign(nativeFormat, &alignment));
        return alignment;
    }

    INT
    GetMeasurableCharacterRangeCount() const
    {
        INT count;
        SetStatus(DllExports::GdipGetStringFormatMeasurableCharacterRangeCount(nativeFormat, &count));
        return count;
    }

    INT
    GetTabStopCount() const
    {
        INT count;
        SetStatus(DllExports::GdipGetStringFormatTabStopCount(nativeFormat, &count));
        return count;
    }

    Status
    GetTabStops(INT count, REAL *firstTabOffset, REAL *tabStops) const
    {
        return SetStatus(DllExports::GdipGetStringFormatTabStops(nativeFormat, count, firstTabOffset, tabStops));
    }

    StringTrimming
    GetTrimming() const
    {
        StringTrimming trimming;
        SetStatus(DllExports::GdipGetStringFormatTrimming(nativeFormat, &trimming));
        return trimming;
    }

    Status
    SetAlignment(StringAlignment align)
    {
        return SetStatus(DllExports::GdipSetStringFormatAlign(nativeFormat, align));
    }

    Status
    SetDigitSubstitution(LANGID language, StringDigitSubstitute substitute)
    {
        return SetStatus(DllExports::GdipSetStringFormatDigitSubstitution(nativeFormat, language, substitute));
    }

    Status
    SetFormatFlags(INT flags)
    {
        return SetStatus(DllExports::GdipSetStringFormatFlags(nativeFormat, flags));
    }

    Status
    SetHotkeyPrefix(HotkeyPrefix hotkeyPrefix)
    {
        return SetStatus(DllExports::GdipSetStringFormatHotkeyPrefix(nativeFormat, INT(hotkeyPrefix)));
    }

    Status
    SetLineAlignment(StringAlignment align)
    {
        return SetStatus(DllExports::GdipSetStringFormatLineAlign(nativeFormat, align));
    }

    Status
    SetMeasurableCharacterRanges(INT rangeCount, const CharacterRange *ranges)
    {
        return SetStatus(DllExports::GdipSetStringFormatMeasurableCharacterRanges(nativeFormat, rangeCount, ranges));
    }

    Status
    SetTabStops(REAL firstTabOffset, INT count, const REAL *tabStops)
    {
        return SetStatus(DllExports::GdipSetStringFormatTabStops(nativeFormat, firstTabOffset, count, tabStops));
    }

    Status
    SetTrimming(StringTrimming trimming)
    {
        return SetStatus(DllExports::GdipSetStringFormatTrimming(nativeFormat, trimming));
    }

  protected:
    GpStringFormat *nativeFormat;
    mutable Status lastStatus;

    StringFormat(GpStringFormat *format, Status status) : nativeFormat(format), lastStatus(status)
    {
    }

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    // get native
    friend inline GpStringFormat *&
    getNat(const StringFormat *sf)
    {
        return const_cast<StringFormat *>(sf)->nativeFormat;
    }
};

#endif /* _GDIPLUSSTRINGFORMAT_H */
