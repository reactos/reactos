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
    GpStatus stat = Ok;
    struct color_remap_table remap_tables[ColorAdjustTypeCount] = {{0}};
    int i;

    TRACE("(%p, %p)\n", imageattr, cloneImageattr);

    if(!imageattr || !cloneImageattr)
        return InvalidParameter;

    for (i=0; i<ColorAdjustTypeCount; i++)
    {
        if (imageattr->colorremaptables[i].enabled)
        {
            remap_tables[i].enabled = TRUE;
            remap_tables[i].mapsize = imageattr->colorremaptables[i].mapsize;
            remap_tables[i].colormap = malloc(sizeof(ColorMap) * remap_tables[i].mapsize);

            if (remap_tables[i].colormap)
            {
                memcpy(remap_tables[i].colormap, imageattr->colorremaptables[i].colormap,
                    sizeof(ColorMap) * remap_tables[i].mapsize);
            }
            else
            {
                stat = OutOfMemory;
                break;
            }
        }
    }

    if (stat == Ok)
        stat = GdipCreateImageAttributes(cloneImageattr);

    if (stat == Ok)
    {
        **cloneImageattr = *imageattr;

        memcpy((*cloneImageattr)->colorremaptables, remap_tables, sizeof(remap_tables));
    }

    if (stat != Ok)
    {
        for (i=0; i<ColorAdjustTypeCount; i++)
            free(remap_tables[i].colormap);
    }

    return stat;
}

GpStatus WINGDIPAPI GdipCreateImageAttributes(GpImageAttributes **imageattr)
{
    if(!imageattr)
        return InvalidParameter;

    *imageattr = calloc(1, sizeof(GpImageAttributes));
    if(!*imageattr) return OutOfMemory;

    (*imageattr)->wrap = WrapModeClamp;

    TRACE("<-- %p\n", *imageattr);

    return Ok;
}

GpStatus WINGDIPAPI GdipDisposeImageAttributes(GpImageAttributes *imageattr)
{
    int i;

    TRACE("(%p)\n", imageattr);

    if(!imageattr)
        return InvalidParameter;

    for (i=0; i<ColorAdjustTypeCount; i++)
        free(imageattr->colorremaptables[i].colormap);

    free(imageattr);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageAttributesAdjustedPalette(GpImageAttributes *imageattr,
    ColorPalette *palette, ColorAdjustType type)
{
    TRACE("(%p,%p,%u)\n", imageattr, palette, type);

    if (!imageattr || !palette || !palette->Count ||
        type >= ColorAdjustTypeCount || type == ColorAdjustTypeDefault)
        return InvalidParameter;

    apply_image_attributes(imageattr, (LPBYTE)palette->Entries, palette->Count, 1, 0,
        type, PixelFormat32bppARGB);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetImageAttributesColorKeys(GpImageAttributes *imageattr,
    ColorAdjustType type, BOOL enableFlag, ARGB colorLow, ARGB colorHigh)
{
    TRACE("(%p,%u,%i,%08lx,%08lx)\n", imageattr, type, enableFlag, colorLow, colorHigh);

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
    TRACE("(%p,%u,%08lx,%i)\n", imageAttr, wrap, argb, clamp);

    if(!imageAttr || wrap > WrapModeClamp)
        return InvalidParameter;

    imageAttr->wrap = wrap;
    imageAttr->outside_color = argb;
    imageAttr->clamp = clamp;

    return Ok;
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
    TRACE("(%p,%u,%i)\n", imageAttr, type, enableFlag);

    if (type >= ColorAdjustTypeCount)
        return InvalidParameter;

    imageAttr->noop[type] = enableFlag ? IMAGEATTR_NOOP_SET : IMAGEATTR_NOOP_CLEAR;

    return Ok;
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
    ColorMap *new_map;

    TRACE("(%p,%u,%i,%u,%p)\n", imageAttr, type, enableFlag, mapSize, map);

    if(!imageAttr || type >= ColorAdjustTypeCount)
	return InvalidParameter;

    if (enableFlag)
    {
        if(!map || !mapSize)
	    return InvalidParameter;

        new_map = malloc(sizeof(*map) * mapSize);

        if (!new_map)
            return OutOfMemory;

        memcpy(new_map, map, sizeof(*map) * mapSize);

        free(imageAttr->colorremaptables[type].colormap);

        imageAttr->colorremaptables[type].mapsize = mapSize;
        imageAttr->colorremaptables[type].colormap = new_map;
    }
    else
    {
        free(imageAttr->colorremaptables[type].colormap);
        imageAttr->colorremaptables[type].colormap = NULL;
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

GpStatus WINGDIPAPI GdipResetImageAttributes(GpImageAttributes *imageAttr,
    ColorAdjustType type)
{
    TRACE("(%p,%u)\n", imageAttr, type);

    if(!imageAttr || type >= ColorAdjustTypeCount)
        return InvalidParameter;

    memset(&imageAttr->colormatrices[type], 0, sizeof(imageAttr->colormatrices[type]));
    GdipSetImageAttributesColorKeys(imageAttr, type, FALSE, 0, 0);
    GdipSetImageAttributesRemapTable(imageAttr, type, FALSE, 0, NULL);
    GdipSetImageAttributesGamma(imageAttr, type, FALSE, 0.0);
    imageAttr->noop[type] = IMAGEATTR_NOOP_UNDEFINED;

    return Ok;
}
