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

const GpStringFormat default_drawstring_format =
{
    0,
    LANG_NEUTRAL,
    LANG_NEUTRAL,
    StringAlignmentNear,
    StringTrimmingCharacter,
    HotkeyPrefixNone,
    StringAlignmentNear,
    StringDigitSubstituteUser,
    0,
    0.0,
    NULL,
    NULL,
    0,
    FALSE
};

static GpStringFormat generic_default_format;
static GpStringFormat generic_typographic_format;

void init_generic_string_formats(void)
{
    memcpy(&generic_default_format, &default_drawstring_format, sizeof(generic_default_format));

    memcpy(&generic_typographic_format, &default_drawstring_format, sizeof(generic_typographic_format));
    generic_typographic_format.attr = StringFormatFlagsNoFitBlackBox | StringFormatFlagsLineLimit |
        StringFormatFlagsNoClip;
    generic_typographic_format.trimming = StringTrimmingNone;
    generic_typographic_format.generic_typographic = TRUE;
}

void free_generic_string_formats(void)
{
    free(generic_default_format.character_ranges);
    free(generic_default_format.tabs);

    free(generic_typographic_format.character_ranges);
    free(generic_typographic_format.tabs);
}

GpStatus WINGDIPAPI GdipCreateStringFormat(INT attr, LANGID lang,
    GpStringFormat **format)
{
    TRACE("(%i, %x, %p)\n", attr, lang, format);

    if(!format)
        return InvalidParameter;

    *format = calloc(1, sizeof(GpStringFormat));
    if(!*format) return OutOfMemory;

    (*format)->attr = attr;
    (*format)->lang = lang;
    (*format)->digitlang = LANG_NEUTRAL;
    (*format)->trimming = StringTrimmingCharacter;
    (*format)->digitsub = StringDigitSubstituteUser;
    (*format)->character_ranges = NULL;
    (*format)->range_count = 0;
    (*format)->generic_typographic = FALSE;
    /* tabstops */
    (*format)->tabcount = 0;
    (*format)->firsttab = 0.0;
    (*format)->tabs = NULL;

    TRACE("<-- %p\n", *format);

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteStringFormat(GpStringFormat *format)
{
    if(!format)
        return InvalidParameter;

    if (format == &generic_default_format || format == &generic_typographic_format)
        return Ok;

    free(format->character_ranges);
    free(format->tabs);
    free(format);

    return Ok;
}

GpStatus WINGDIPAPI GdipStringFormatGetGenericDefault(GpStringFormat **format)
{
    if (!format)
        return InvalidParameter;

    *format = &generic_default_format;

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

    *align = format->line_align;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetStringFormatMeasurableCharacterRangeCount(
    GDIPCONST GpStringFormat *format, INT *count)
{
    if (!(format && count))
        return InvalidParameter;

    TRACE("%p %p\n", format, count);

    *count = format->range_count;

    return Ok;
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
    TRACE("(%p, %i)\n", format, align);

    if(!format)
        return InvalidParameter;

    format->align = align;

    return Ok;
}

/*FIXME: digit substitution actually not implemented, get/set only */
GpStatus WINGDIPAPI GdipSetStringFormatDigitSubstitution(GpStringFormat *format,
    LANGID language, StringDigitSubstitute substitute)
{
    TRACE("(%p, %x, %i)\n", format, language, substitute);

    if(!format)
        return InvalidParameter;

    format->digitlang = language;
    format->digitsub  = substitute;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatHotkeyPrefix(GpStringFormat *format,
    INT hkpx)
{
    TRACE("(%p, %i)\n", format, hkpx);

    if(!format || hkpx < 0 || hkpx > 2)
        return InvalidParameter;

    format->hkprefix = (HotkeyPrefix) hkpx;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatLineAlign(GpStringFormat *format,
    StringAlignment align)
{
    TRACE("(%p, %i)\n", format, align);

    if(!format)
        return InvalidParameter;

    format->line_align = align;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatMeasurableCharacterRanges(
    GpStringFormat *format, INT rangeCount, GDIPCONST CharacterRange *ranges)
{
    CharacterRange *new_ranges;

    if (!(format && ranges))
        return InvalidParameter;

    TRACE("%p, %d, %p\n", format, rangeCount, ranges);

    new_ranges = malloc(rangeCount * sizeof(CharacterRange));
    if (!new_ranges)
        return OutOfMemory;

    free(format->character_ranges);
    format->character_ranges = new_ranges;
    memcpy(format->character_ranges, ranges, sizeof(CharacterRange) * rangeCount);
    format->range_count = rangeCount;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatTabStops(GpStringFormat *format, REAL firsttab,
    INT count, GDIPCONST REAL *tabs)
{
    TRACE("(%p, %0.2f, %i, %p)\n", format, firsttab, count, tabs);

    if(!format || !tabs)
        return InvalidParameter;

    if(count > 0){
        if(firsttab < 0.0)  return NotImplemented;
        if(format->tabcount < count){
            REAL *ptr;
            ptr = realloc(format->tabs, sizeof(REAL) * count);
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
    TRACE("(%p, %i)\n", format, trimming);

    if(!format)
        return InvalidParameter;

    format->trimming = trimming;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatFlags(GpStringFormat *format, INT flags)
{
    TRACE("(%p, %x)\n", format, flags);

    if(!format)
        return InvalidParameter;

    format->attr = flags;

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneStringFormat(GDIPCONST GpStringFormat *format, GpStringFormat **newFormat)
{
    if(!format || !newFormat)
        return InvalidParameter;

    *newFormat = malloc(sizeof(GpStringFormat));
    if(!*newFormat) return OutOfMemory;

    **newFormat = *format;

    if(format->tabcount > 0){
        (*newFormat)->tabs = malloc(sizeof(REAL) * format->tabcount);
        if(!(*newFormat)->tabs){
            free(*newFormat);
            return OutOfMemory;
        }
        memcpy((*newFormat)->tabs, format->tabs, sizeof(REAL) * format->tabcount);
    }
    else
        (*newFormat)->tabs = NULL;

    if(format->range_count > 0){
        (*newFormat)->character_ranges = malloc(sizeof(CharacterRange) * format->range_count);
        if(!(*newFormat)->character_ranges){
            free((*newFormat)->tabs);
            free(*newFormat);
            return OutOfMemory;
        }
        memcpy((*newFormat)->character_ranges, format->character_ranges,
               sizeof(CharacterRange) * format->range_count);
    }
    else
        (*newFormat)->character_ranges = NULL;

    TRACE("%p %p\n",format,newFormat);

    return Ok;
}

GpStatus WINGDIPAPI GdipStringFormatGetGenericTypographic(GpStringFormat **format)
{
    if(!format)
        return InvalidParameter;

    *format = &generic_typographic_format;

    TRACE("%p => %p\n", format, *format);

    return Ok;
}
