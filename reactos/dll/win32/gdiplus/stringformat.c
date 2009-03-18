/*
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

GpStatus WINGDIPAPI GdipCreateStringFormat(INT attr, LANGID lang,
    GpStringFormat **format)
{
    if(!format)
        return InvalidParameter;

    *format = GdipAlloc(sizeof(GpStringFormat));
    if(!*format)   return OutOfMemory;

    (*format)->attr = attr;
    (*format)->lang = lang;
    (*format)->digitlang = LANG_NEUTRAL;
    (*format)->trimming = StringTrimmingCharacter;
    (*format)->digitsub = StringDigitSubstituteUser;
    /* tabstops */
    (*format)->tabcount = 0;
    (*format)->firsttab = 0.0;
    (*format)->tabs = NULL;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteStringFormat(GpStringFormat *format)
{
    if(!format)
        return InvalidParameter;

    GdipFree(format->tabs);
    GdipFree(format);

    return Ok;
}

GpStatus WINGDIPAPI GdipStringFormatGetGenericDefault(GpStringFormat **format)
{
    GpStatus stat;

    if (!format)
        return InvalidParameter;

    stat = GdipCreateStringFormat(0, LANG_NEUTRAL, format);
    if(stat != Ok)
        return stat;

    (*format)->align     = StringAlignmentNear;
    (*format)->vertalign = StringAlignmentNear;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatAlign(GpStringFormat *format,
    StringAlignment *align)
{
    if(!format || !align)
        return InvalidParameter;

    *align = format->align;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatDigitSubstitution(GDIPCONST GpStringFormat *format,
    LANGID *language, StringDigitSubstitute *substitute)
{
    if(!format)
        return InvalidParameter;

    if(language)    *language   = format->digitlang;
    if(substitute)  *substitute = format->digitsub;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatFlags(GDIPCONST GpStringFormat* format,
        INT* flags)
{
    if (!(format && flags))
        return InvalidParameter;

    *flags = format->attr;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatHotkeyPrefix(GDIPCONST GpStringFormat
    *format, INT *hkpx)
{
    if(!format || !hkpx)
        return InvalidParameter;

    *hkpx = (INT)format->hkprefix;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatLineAlign(GpStringFormat *format,
    StringAlignment *align)
{
    if(!format || !align)
        return InvalidParameter;

    *align = format->vertalign;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatMeasurableCharacterRangeCount(
        GDIPCONST GpStringFormat* format, INT* count)
{
    if (!(format && count))
        return InvalidParameter;

    FIXME("stub: %p %p\n", format, count);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetStringFormatTabStopCount(GDIPCONST GpStringFormat *format,
    INT *count)
{
    if(!format || !count)
        return InvalidParameter;

    *count = format->tabcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatTabStops(GDIPCONST GpStringFormat *format, INT count,
    REAL *firsttab, REAL *tabs)
{
    if(!format || !firsttab || !tabs)
        return InvalidParameter;

    /* native simply crashes on count < 0 */
    if(count != 0)
        memcpy(tabs, format->tabs, sizeof(REAL)*count);

    *firsttab = format->firsttab;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatTrimming(GpStringFormat *format,
    StringTrimming *trimming)
{
    if(!format || !trimming)
        return InvalidParameter;

    *trimming = format->trimming;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatAlign(GpStringFormat *format,
    StringAlignment align)
{
    if(!format)
        return InvalidParameter;

    format->align = align;

    return Ok;
}

/*FIXME: digit substitution actually not implemented, get/set only */
GpStatus WINGDIPAPI GdipSetStringFormatDigitSubstitution(GpStringFormat *format,
    LANGID language, StringDigitSubstitute substitute)
{
    if(!format)
        return InvalidParameter;

    format->digitlang = language;
    format->digitsub  = substitute;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatHotkeyPrefix(GpStringFormat *format,
    INT hkpx)
{
    if(!format || hkpx < 0 || hkpx > 2)
        return InvalidParameter;

    format->hkprefix = (HotkeyPrefix) hkpx;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatLineAlign(GpStringFormat *format,
    StringAlignment align)
{
    if(!format)
        return InvalidParameter;

    format->vertalign = align;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatMeasurableCharacterRanges(GpStringFormat*
        format, INT rangeCount, GDIPCONST CharacterRange* ranges)
{
    if (!(format && rangeCount && ranges))
        return InvalidParameter;

    FIXME("stub: %p, %d, %p\n", format, rangeCount, ranges);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetStringFormatTabStops(GpStringFormat *format, REAL firsttab,
    INT count, GDIPCONST REAL *tabs)
{
    if(!format || !tabs)
        return InvalidParameter;

    if(count > 0){
        if(firsttab < 0.0)  return NotImplemented;
        /* first time allocation */
        if(format->tabcount == 0){
            format->tabs = GdipAlloc(sizeof(REAL)*count);
            if(!format->tabs)
                return OutOfMemory;
        }
        /* reallocation */
        if((format->tabcount < count) && (format->tabcount > 0)){
            REAL *ptr;
            ptr = HeapReAlloc(GetProcessHeap(), 0, format->tabs, sizeof(REAL)*count);
            if(!ptr)
                return OutOfMemory;
            format->tabs = ptr;
        }
        format->firsttab = firsttab;
        format->tabcount = count;
        memcpy(format->tabs, tabs, sizeof(REAL)*count);
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatTrimming(GpStringFormat *format,
    StringTrimming trimming)
{
    if(!format)
        return InvalidParameter;

    format->trimming = trimming;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatFlags(GpStringFormat *format, INT flags)
{
    if(!format)
        return InvalidParameter;

    format->attr = flags;

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneStringFormat(GDIPCONST GpStringFormat *format, GpStringFormat **newFormat)
{
    if(!format || !newFormat)
        return InvalidParameter;

    *newFormat = GdipAlloc(sizeof(GpStringFormat));
    if(!*newFormat)    return OutOfMemory;

    **newFormat = *format;

    if(format->tabcount > 0){
        (*newFormat)->tabs = GdipAlloc(sizeof(REAL) * format->tabcount);
        if(!(*newFormat)->tabs){
            GdipFree(*newFormat);
            return OutOfMemory;
        }
        memcpy((*newFormat)->tabs, format->tabs, sizeof(REAL) * format->tabcount);
    }
    else
        (*newFormat)->tabs = NULL;

    TRACE("%p %p\n",format,newFormat);

    return Ok;
}

GpStatus WINGDIPAPI GdipStringFormatGetGenericTypographic(GpStringFormat **format)
{
    GpStatus stat;

    if(!format)
        return InvalidParameter;

    stat = GdipCreateStringFormat(StringFormatFlagsNoFitBlackBox |
                                  StringFormatFlagsLineLimit |
                                  StringFormatFlagsNoClip, LANG_NEUTRAL, format);
    if(stat != Ok)
        return stat;

    (*format)->digitlang = LANG_NEUTRAL;
    (*format)->digitsub  = StringDigitSubstituteUser;
    (*format)->trimming  = StringTrimmingNone;
    (*format)->hkprefix  = HotkeyPrefixNone;
    (*format)->align     = StringAlignmentNear;
    (*format)->vertalign = StringAlignmentNear;

    return Ok;
}
