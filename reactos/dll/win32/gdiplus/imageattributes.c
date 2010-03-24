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
    GpStatus stat;

    TRACE("(%p, %p)\n", imageattr, cloneImageattr);

    if(!imageattr || !cloneImageattr)
        return InvalidParameter;

    stat = GdipCreateImageAttributes(cloneImageattr);

    if (stat == Ok)
        **cloneImageattr = *imageattr;

    return stat;
}

GpStatus WINGDIPAPI GdipCreateImageAttributes(GpImageAttributes **imageattr)
{
    if(!imageattr)
        return InvalidParameter;

    *imageattr = GdipAlloc(sizeof(GpImageAttributes));
    if(!*imageattr)    return OutOfMemory;

    TRACE("<-- %p\n", *imageattr);

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
    TRACE("(%p,%u,%i,%08x,%08x)\n", imageattr, type, enableFlag, colorLow, colorHigh);

    if(!imageattr || type >= ColorAdjustTypeCount)
        return InvalidParameter;

    imageattr->colorkeys[type].enabled = enableFlag;
    imageattr->colorkeys[type].low = colorLow;
    imageattr->colorkeys[type].high = colorHigh;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetImageAttributesColorMatrix(GpImageAttributes *imageattr,
    ColorAdjustType type, BOOL enableFlag, GDIPCONST ColorMatrix* colorMatrix,
    GDIPCONST ColorMatrix* grayMatrix, ColorMatrixFlags flags)
{
    TRACE("(%p,%u,%i,%p,%p,%u)\n", imageattr, type, enableFlag, colorMatrix,
        grayMatrix, flags);

    if(!imageattr || type >= ColorAdjustTypeCount || flags > ColorMatrixFlagsAltGray)
        return InvalidParameter;

    if (enableFlag)
    {
        if (!colorMatrix)
            return InvalidParameter;

        if (flags == ColorMatrixFlagsAltGray)
        {
            if (!grayMatrix)
                return InvalidParameter;

            imageattr->colormatrices[type].graymatrix = *grayMatrix;
        }

        imageattr->colormatrices[type].colormatrix = *colorMatrix;
        imageattr->colormatrices[type].flags = flags;
    }

    imageattr->colormatrices[type].enabled = enableFlag;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetImageAttributesWrapMode(GpImageAttributes *imageAttr,
    WrapMode wrap, ARGB argb, BOOL clamp)
{
    static int calls;

    TRACE("(%p,%u,%08x,%i)\n", imageAttr, wrap, argb, clamp);

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

    TRACE("(%p,%i)\n", imageAttr, enableFlag);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesGamma(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, REAL gamma)
{
    TRACE("(%p,%u,%i,%0.2f)\n", imageAttr, type, enableFlag, gamma);

    if (!imageAttr || (enableFlag && gamma <= 0.0) || type >= ColorAdjustTypeCount)
        return InvalidParameter;

    imageAttr->gamma_enabled[type] = enableFlag;
    imageAttr->gamma[type] = gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetImageAttributesNoOp(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag)
{
    static int calls;

    TRACE("(%p,%u,%i)\n", imageAttr, type, enableFlag);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesOutputChannel(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, ColorChannelFlags channelFlags)
{
    static int calls;

    TRACE("(%p,%u,%i,%x)\n", imageAttr, type, enableFlag, channelFlags);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesOutputChannelColorProfile(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag,
    GDIPCONST WCHAR *colorProfileFilename)
{
    static int calls;

    TRACE("(%p,%u,%i,%s)\n", imageAttr, type, enableFlag, debugstr_w(colorProfileFilename));

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesRemapTable(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, UINT mapSize,
    GDIPCONST ColorMap *map)
{
    TRACE("(%p,%u,%i,%u,%p)\n", imageAttr, type, enableFlag, mapSize, map);

    if(!imageAttr || type >= ColorAdjustTypeCount)
	return InvalidParameter;

    if (enableFlag)
    {
        if(!map || !mapSize)
	    return InvalidParameter;

        imageAttr->colorremaptables[type].mapsize = mapSize;
        imageAttr->colorremaptables[type].colormap = map;
    }

    imageAttr->colorremaptables[type].enabled = enableFlag;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetImageAttributesThreshold(GpImageAttributes *imageAttr,
    ColorAdjustType type, BOOL enableFlag, REAL threshold)
{
    static int calls;

    TRACE("(%p,%u,%i,%0.2f)\n", imageAttr, type, enableFlag, threshold);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetImageAttributesToIdentity(GpImageAttributes *imageAttr,
    ColorAdjustType type)
{
    static int calls;

    TRACE("(%p,%u)\n", imageAttr, type);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}
