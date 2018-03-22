/*
 * Unit test suite for string format
 *
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(got == expected, "Expected %.2f, got %.2f\n", expected, got)

static void test_constructor(void)
{
    GpStringFormat *format;
    GpStatus stat;
    INT n, count;
    StringAlignment align, line_align;
    StringTrimming trimming;
    StringDigitSubstitute digitsub;
    LANGID digitlang;

    stat = GdipCreateStringFormat(0, LANG_NEUTRAL, &format);
    expect(Ok, stat);

    GdipGetStringFormatAlign(format, &align);
    GdipGetStringFormatLineAlign(format, &line_align);
    GdipGetStringFormatHotkeyPrefix(format, &n);
    GdipGetStringFormatTrimming(format, &trimming);
    GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    GdipGetStringFormatMeasurableCharacterRangeCount(format, &count);

    expect(HotkeyPrefixNone, n);
    expect(StringAlignmentNear, align);
    expect(StringAlignmentNear, line_align);
    expect(StringTrimmingCharacter, trimming);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);
    expect(0, count);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_characterrange(void)
{
    CharacterRange ranges[3];
    INT count;
    GpStringFormat* format;
    GpStatus stat;

    stat = GdipCreateStringFormat(0, LANG_NEUTRAL, &format);
    expect(Ok, stat);
    stat = GdipSetStringFormatMeasurableCharacterRanges(NULL, 3, ranges);
    expect(InvalidParameter, stat);
    stat = GdipSetStringFormatMeasurableCharacterRanges(format, 0, ranges);
    expect(Ok, stat);
    stat = GdipSetStringFormatMeasurableCharacterRanges(format, 3, NULL);
    expect(InvalidParameter, stat);

    stat = GdipSetStringFormatMeasurableCharacterRanges(format, 3, ranges);
    expect(Ok, stat);
    stat = GdipGetStringFormatMeasurableCharacterRangeCount(format, &count);
    expect(Ok, stat);
    if (stat == Ok) expect(3, count);

    stat= GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_digitsubstitution(void)
{
    GpStringFormat *format;
    GpStatus stat;
    StringDigitSubstitute digitsub;
    LANGID digitlang;

    stat = GdipCreateStringFormat(0, LANG_RUSSIAN, &format);
    expect(Ok, stat);

    /* NULL arguments */
    stat = GdipGetStringFormatDigitSubstitution(NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatDigitSubstitution(format, NULL, NULL);
    expect(Ok, stat);
    stat = GdipGetStringFormatDigitSubstitution(NULL, &digitlang, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatDigitSubstitution(NULL, NULL, &digitsub);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatDigitSubstitution(NULL, &digitlang, &digitsub);
    expect(InvalidParameter, stat);
    stat = GdipSetStringFormatDigitSubstitution(NULL, LANG_NEUTRAL, StringDigitSubstituteNone);
    expect(InvalidParameter, stat);

    /* try to get both and one by one */
    stat = GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    expect(Ok, stat);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);

    digitsub  = StringDigitSubstituteNone;
    stat = GdipGetStringFormatDigitSubstitution(format, NULL, &digitsub);
    expect(Ok, stat);
    expect(StringDigitSubstituteUser, digitsub);

    digitlang = LANG_RUSSIAN;
    stat = GdipGetStringFormatDigitSubstitution(format, &digitlang, NULL);
    expect(Ok, stat);
    expect(LANG_NEUTRAL, digitlang);

    /* set/get */
    stat = GdipSetStringFormatDigitSubstitution(format, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),
                                                        StringDigitSubstituteUser);
    expect(Ok, stat);
    digitsub  = StringDigitSubstituteNone;
    digitlang = LANG_RUSSIAN;
    stat = GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    expect(Ok, stat);
    expect(StringDigitSubstituteUser, digitsub);
    expect(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), digitlang);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_getgenerictypographic(void)
{
    GpStringFormat *format, *format2;
    GpStatus stat;
    INT flags;
    INT n;
    StringAlignment align, line_align;
    StringTrimming trimming;
    StringDigitSubstitute digitsub;
    LANGID digitlang;
    INT tabcount;

    /* NULL arg */
    stat = GdipStringFormatGetGenericTypographic(NULL);
    expect(InvalidParameter, stat);

    stat = GdipStringFormatGetGenericTypographic(&format);
    expect(Ok, stat);

    stat = GdipStringFormatGetGenericTypographic(&format2);
    expect(Ok, stat);
    ok(format == format2, "expected same format object\n");
    stat = GdipDeleteStringFormat(format2);
    expect(Ok, stat);

    GdipGetStringFormatFlags(format, &flags);
    GdipGetStringFormatAlign(format, &align);
    GdipGetStringFormatLineAlign(format, &line_align);
    GdipGetStringFormatHotkeyPrefix(format, &n);
    GdipGetStringFormatTrimming(format, &trimming);
    GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    GdipGetStringFormatTabStopCount(format, &tabcount);

    expect((StringFormatFlagsNoFitBlackBox |StringFormatFlagsLineLimit | StringFormatFlagsNoClip),
            flags);
    expect(HotkeyPrefixNone, n);
    expect(StringAlignmentNear, align);
    expect(StringAlignmentNear, line_align);
    expect(StringTrimmingNone, trimming);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);
    expect(0, tabcount);

    /* Change format parameters, release, get format object again. */
    stat = GdipSetStringFormatFlags(format, StringFormatFlagsNoWrap);
    expect(Ok, stat);

    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(StringFormatFlagsNoWrap, flags);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);

    stat = GdipStringFormatGetGenericTypographic(&format);
    expect(Ok, stat);

    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(StringFormatFlagsNoWrap, flags);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static REAL tabstops[] = {0.0, 10.0, 2.0};
static void test_tabstops(void)
{
    GpStringFormat *format;
    GpStatus stat;
    INT count;
    REAL tabs[3];
    REAL firsttab;

    stat = GdipCreateStringFormat(0, LANG_NEUTRAL, &format);
    expect(Ok, stat);

    /* NULL */
    stat = GdipGetStringFormatTabStopCount(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStopCount(NULL, &count);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStopCount(format, NULL);
    expect(InvalidParameter, stat);

    stat = GdipSetStringFormatTabStops(NULL, 0.0, 0, NULL);
    expect(InvalidParameter, stat);
    stat = GdipSetStringFormatTabStops(format, 0.0, 0, NULL);
    expect(InvalidParameter, stat);
    stat = GdipSetStringFormatTabStops(NULL, 0.0, 0, tabstops);
    expect(InvalidParameter, stat);

    stat = GdipGetStringFormatTabStops(NULL, 0, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStops(format, 0, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStops(NULL, 0, &firsttab, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStops(NULL, 0, NULL, tabs);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStops(format, 0, &firsttab, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatTabStops(format, 0, NULL, tabs);
    expect(InvalidParameter, stat);

    /* not NULL */
    stat = GdipGetStringFormatTabStopCount(format, &count);
    expect(Ok, stat);
    expect(0, count);
    /* negative tabcount */
    stat = GdipSetStringFormatTabStops(format, 0.0, -1, tabstops);
    expect(Ok, stat);
    count = -1;
    stat = GdipGetStringFormatTabStopCount(format, &count);
    expect(Ok, stat);
    expect(0, count);

    stat = GdipSetStringFormatTabStops(format, -10.0, 0, tabstops);
    expect(Ok, stat);
    stat = GdipSetStringFormatTabStops(format, -10.0, 1, tabstops);
    expect(NotImplemented, stat);

    firsttab = -1.0;
    tabs[0] = tabs[1] = tabs[2] = -1.0;
    stat = GdipGetStringFormatTabStops(format, 0, &firsttab, tabs);
    expect(Ok, stat);
    expectf(-1.0, tabs[0]);
    expectf(-1.0, tabs[1]);
    expectf(-1.0, tabs[2]);
    expectf(0.0, firsttab);

    stat = GdipSetStringFormatTabStops(format, +0.0, 3, tabstops);
    expect(Ok, stat);
    count = 0;
    stat = GdipGetStringFormatTabStopCount(format, &count);
    expect(Ok, stat);
    expect(3, count);

    firsttab = -1.0;
    tabs[0] = tabs[1] = tabs[2] = -1.0;
    stat = GdipGetStringFormatTabStops(format, 3, &firsttab, tabs);
    expect(Ok, stat);
    expectf(0.0,  tabs[0]);
    expectf(10.0, tabs[1]);
    expectf(2.0,  tabs[2]);
    expectf(0.0,  firsttab);

    stat = GdipSetStringFormatTabStops(format, 10.0, 3, tabstops);
    expect(Ok, stat);
    firsttab = -1.0;
    tabs[0] = tabs[1] = tabs[2] = -1.0;
    stat = GdipGetStringFormatTabStops(format, 0, &firsttab, tabs);
    expect(Ok, stat);
    expectf(-1.0, tabs[0]);
    expectf(-1.0, tabs[1]);
    expectf(-1.0, tabs[2]);
    expectf(10.0, firsttab);

    /* zero tabcount, after valid setting to 3 */
    stat = GdipSetStringFormatTabStops(format, 0.0, 0, tabstops);
    expect(Ok, stat);
    count = 0;
    stat = GdipGetStringFormatTabStopCount(format, &count);
    expect(Ok, stat);
    expect(3, count);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_getgenericdefault(void)
{
    GpStringFormat *format, *format2;
    GpStatus stat;

    INT flags;
    INT n;
    StringAlignment align, line_align;
    StringTrimming trimming;
    StringDigitSubstitute digitsub;
    LANGID digitlang;
    INT tabcount;

    /* NULL arg */
    stat = GdipStringFormatGetGenericDefault(NULL);
    expect(InvalidParameter, stat);

    stat = GdipStringFormatGetGenericDefault(&format);
    expect(Ok, stat);

    stat = GdipStringFormatGetGenericDefault(&format2);
    expect(Ok, stat);
    ok(format == format2, "expected same format object\n");
    stat = GdipDeleteStringFormat(format2);
    expect(Ok, stat);

    GdipGetStringFormatFlags(format, &flags);
    GdipGetStringFormatAlign(format, &align);
    GdipGetStringFormatLineAlign(format, &line_align);
    GdipGetStringFormatHotkeyPrefix(format, &n);
    GdipGetStringFormatTrimming(format, &trimming);
    GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    GdipGetStringFormatTabStopCount(format, &tabcount);

    expect(0, flags);
    expect(HotkeyPrefixNone, n);
    expect(StringAlignmentNear, align);
    expect(StringAlignmentNear, line_align);
    expect(StringTrimmingCharacter, trimming);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);
    expect(0, tabcount);

    /* Change default format parameters, release, get format object again. */
    stat = GdipSetStringFormatFlags(format, StringFormatFlagsNoWrap);
    expect(Ok, stat);

    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(StringFormatFlagsNoWrap, flags);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);

    stat = GdipStringFormatGetGenericDefault(&format);
    expect(Ok, stat);

    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(StringFormatFlagsNoWrap, flags);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_stringformatflags(void)
{
    GpStringFormat *format;
    GpStatus stat;

    INT flags;

    stat = GdipCreateStringFormat(0, LANG_NEUTRAL, &format);
    expect(Ok, stat);

    /* NULL args */
    stat = GdipSetStringFormatFlags(NULL, 0);
    expect(InvalidParameter, stat);

    stat = GdipSetStringFormatFlags(format, 0);
    expect(Ok, stat);
    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(0, flags);

    /* Check some valid flags */
    stat = GdipSetStringFormatFlags(format, StringFormatFlagsDirectionRightToLeft);
    expect(Ok, stat);
    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(StringFormatFlagsDirectionRightToLeft, flags);

    stat = GdipSetStringFormatFlags(format, StringFormatFlagsNoFontFallback);
    expect(Ok, stat);
    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(StringFormatFlagsNoFontFallback, flags);

    /* Check some flag combinations */
    stat = GdipSetStringFormatFlags(format, StringFormatFlagsDirectionVertical
        | StringFormatFlagsNoFitBlackBox);
    expect(Ok, stat);
    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect((StringFormatFlagsDirectionVertical
        | StringFormatFlagsNoFitBlackBox), flags);

    stat = GdipSetStringFormatFlags(format, StringFormatFlagsDisplayFormatControl
        | StringFormatFlagsMeasureTrailingSpaces);
    expect(Ok, stat);
    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect((StringFormatFlagsDisplayFormatControl
        | StringFormatFlagsMeasureTrailingSpaces), flags);

    /* Check invalid flags */
    stat = GdipSetStringFormatFlags(format, 0xdeadbeef);
    expect(Ok, stat);
    stat = GdipGetStringFormatFlags(format, &flags);
    expect(Ok, stat);
    expect(0xdeadbeef, flags);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

START_TEST(stringformat)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor();
    test_characterrange();
    test_digitsubstitution();
    test_getgenerictypographic();
    test_tabstops();
    test_getgenericdefault();
    test_stringformatflags();

    GdiplusShutdown(gdiplusToken);
}
