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

#include "windef.h"
#include "wingdi.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

GpStatus WINGDIPAPI GdipCloneImageAttributes(GDIPCONST GpImageAttributes *imageattr,
    GpImageAttributes **cloneImageattr)
{
    TRACE("(%p, %p)\n", imageattr, cloneImageattr);

    if(!imageattr || !cloneImageattr)
        return InvalidParameter;

    **cloneImageattr = *imageattr;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateImageAttributes(GpImageAttributes **imageattr)
{
    TRACE("(%p)\n", imageattr);

    if(!imageattr)
        return InvalidParameter;

    *imageattr = GdipAlloc(sizeof(GpImageAttributes));
    if(!*imageattr)    return OutOfMemory;

    return Ok;
}

GpStatus WINGDIPAPI GdipDisposeImageAttributes(GpImageAttributes *imageattr)
{
    TRACE("(%p)\n", imageattr);

    if(!imageattr)
        return InvalidParameter;

    GdipFree(imageattr);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetImageAttributesColorKeys(GpImageAttributes *imageattr,
    ColorAdjustType type, BOOL enableFlag, ARGB colorLow, ARGB colorHigh)
{
    static int calls;

    if(!imageattr)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesColorMatrix(GpImageAttributes *imageattr,
    ColorAdjustType type, BOOL enableFlag, GDIPCONST ColorMatrix* colorMatrix,
    GDIPCONST ColorMatrix* grayMatrix, ColorMatrixFlags flags)
{
    static int calls;

    if(!imageattr || !colorMatrix || !grayMatrix)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesWrapMode(GpImageAttributes *imageAttr,
    WrapMode wrap, ARGB argb, BOOL clamp)
{
    static int calls;

    if(!imageAttr)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesCachedBackground(GpImageAttributes *imageAttr,
    BOOL enableFlag)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesGamma(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, REAL gamma)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesNoOp(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesOutputChannel(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, ColorChannelFlags channelFlags)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesOutputChannelColorProfile(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag,
    GDIPCONST WCHAR *colorProfileFilename)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesRemapTable(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, UINT mapSize,
    GDIPCONST ColorMap *map)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesThreshold(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, REAL threshold)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesToIdentity(GpImageAttributes *imageAttr,
    ColorAdjustType type)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}
