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
    (*format)->trimming = StringTrimmingCharacter;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteStringFormat(GpStringFormat *format)
{
    if(!format)
        return InvalidParameter;

    GdipFree(format);

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

GpStatus WINGDIPAPI GdipSetStringFormatTrimming(GpStringFormat *format,
    StringTrimming trimming)
{
    if(!format)
        return InvalidParameter;

    format->trimming = trimming;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetStringFormatFlags(GDIPCONST GpStringFormat *format, INT flags)
{
    FIXME("format (%p) flags (%d)\n", format, flags);

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneStringFormat(GDIPCONST GpStringFormat *format, GpStringFormat **newFormat)
{
    if(!format || !newFormat)
        return InvalidParameter;

    *newFormat = GdipAlloc(sizeof(GpStringFormat));
    if(!*newFormat)    return OutOfMemory;

    **newFormat = *format;

    TRACE("%p %p\n",format,newFormat);

    return Ok;
}
