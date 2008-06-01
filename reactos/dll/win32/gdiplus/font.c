/*
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

GpStatus WINGDIPAPI GdipCreateFontFromLogfontW(HDC hdc,
    GDIPCONST LOGFONTW *logfont, GpFont **font)
{
    HFONT hfont, oldfont;
    TEXTMETRICW textmet;

    if(!logfont || !font)
        return InvalidParameter;

    *font = GdipAlloc(sizeof(GpFont));
    if(!*font)  return OutOfMemory;

    memcpy(&(*font)->lfw.lfFaceName, logfont->lfFaceName, LF_FACESIZE *
           sizeof(WCHAR));
    (*font)->lfw.lfHeight = logfont->lfHeight;
    (*font)->lfw.lfItalic = logfont->lfItalic;
    (*font)->lfw.lfUnderline = logfont->lfUnderline;
    (*font)->lfw.lfStrikeOut = logfont->lfStrikeOut;

    hfont = CreateFontIndirectW(&(*font)->lfw);
    oldfont = SelectObject(hdc, hfont);
    GetTextMetricsW(hdc, &textmet);

    (*font)->lfw.lfHeight = -textmet.tmHeight;
    (*font)->lfw.lfWeight = textmet.tmWeight;

    SelectObject(hdc, oldfont);
    DeleteObject(hfont);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromLogfontA(HDC hdc,
    GDIPCONST LOGFONTA *lfa, GpFont **font)
{
    LOGFONTW lfw;

    if(!lfa || !font)
        return InvalidParameter;

    memcpy(&lfw, lfa, FIELD_OFFSET(LOGFONTA,lfFaceName) );

    if(!MultiByteToWideChar(CP_ACP, 0, lfa->lfFaceName, -1, lfw.lfFaceName, LF_FACESIZE))
        return GenericError;

    GdipCreateFontFromLogfontW(hdc, &lfw, font);

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteFont(GpFont* font)
{
    if(!font)
        return InvalidParameter;

    GdipFree(font);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromDC(HDC hdc, GpFont **font)
{
    HFONT hfont;
    LOGFONTW lfw;

    if(!font)
        return InvalidParameter;

    hfont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    if(!hfont)
        return GenericError;

    if(!GetObjectW(hfont, sizeof(LOGFONTW), &lfw))
        return GenericError;

    return GdipCreateFontFromLogfontW(hdc, &lfw, font);
}

/* FIXME: use graphics */
GpStatus WINGDIPAPI GdipGetLogFontW(GpFont *font, GpGraphics *graphics,
    LOGFONTW *lfw)
{
    if(!font || !graphics || !lfw)
        return InvalidParameter;

    *lfw = font->lfw;

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneFont(GpFont *font, GpFont **cloneFont)
{
    if(!font || !cloneFont)
        return InvalidParameter;

    *cloneFont = GdipAlloc(sizeof(GpFont));
    if(!*cloneFont)    return OutOfMemory;

    **cloneFont = *font;

    return Ok;
}
