/*
 * GdiPlusImageAttributes.h
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

#ifndef _GDIPLUSIMAGEATTRIBUTES_H
#define _GDIPLUSIMAGEATTRIBUTES_H

class ImageAttributes : public GdiplusBase
{
  public:
    friend class TextureBrush;

    ImageAttributes() : nativeImageAttr(NULL)
    {
        lastStatus = DllExports::GdipCreateImageAttributes(&nativeImageAttr);
    }

    ~ImageAttributes()
    {
        DllExports::GdipDisposeImageAttributes(nativeImageAttr);
    }

    Status
    ClearBrushRemapTable()
    {
        return ClearRemapTable(ColorAdjustTypeBrush);
    }

    Status
    ClearColorKey(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesColorKeys(nativeImageAttr, type, FALSE, NULL, NULL));
    }

    Status
    ClearColorMatrices(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesColorMatrix(
            nativeImageAttr, type, FALSE, NULL, NULL, ColorMatrixFlagsDefault));
    }

    Status
    ClearColorMatrix(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesColorMatrix(
            nativeImageAttr, type, FALSE, NULL, NULL, ColorMatrixFlagsDefault));
    }

    Status
    ClearGamma(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesGamma(nativeImageAttr, type, FALSE, 0.0));
    }

    Status
    ClearNoOp(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesNoOp(nativeImageAttr, type, FALSE));
    }

    Status
    ClearOutputChannel(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(
            DllExports::GdipSetImageAttributesOutputChannel(nativeImageAttr, type, FALSE, ColorChannelFlagsLast));
    }

    Status
    ClearOutputChannelColorProfile(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(
            DllExports::GdipSetImageAttributesOutputChannelColorProfile(nativeImageAttr, type, FALSE, NULL));
    }

    Status
    ClearRemapTable(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesRemapTable(nativeImageAttr, type, FALSE, 0, NULL));
    }

    Status
    ClearThreshold(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesThreshold(nativeImageAttr, type, FALSE, 0.0));
    }

    ImageAttributes *
    Clone()
    {
        GpImageAttributes *clone = NULL;
        SetStatus(DllExports::GdipCloneImageAttributes(nativeImageAttr, &clone));
        if (lastStatus != Ok)
            return NULL;

        ImageAttributes *newImageAttr = new ImageAttributes(clone, lastStatus);
        if (newImageAttr == NULL)
            SetStatus(DllExports::GdipDisposeImageAttributes(clone));

        return newImageAttr;
    }

    Status
    GetAdjustedPalette(ColorPalette *colorPalette, ColorAdjustType colorAdjustType)
    {
        return SetStatus(
            DllExports::GdipGetImageAttributesAdjustedPalette(nativeImageAttr, colorPalette, colorAdjustType));
    }

    Status
    GetLastStatus()
    {
        return lastStatus;
    }

    Status
    Reset(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipResetImageAttributes(nativeImageAttr, type));
    }

    Status
    SetBrushRemapTable(UINT mapSize, ColorMap *map)
    {
        return SetRemapTable(mapSize, map, ColorAdjustTypeBrush);
    }

    Status
    SetColorKey(const Color &colorLow, const Color &colorHigh, ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesColorKeys(
            nativeImageAttr, type, TRUE, colorLow.GetValue(), colorHigh.GetValue()));
    }

    Status
    SetColorMatrices(
        const ColorMatrix *colorMatrix,
        const ColorMatrix *grayMatrix,
        ColorMatrixFlags mode = ColorMatrixFlagsDefault,
        ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(
            DllExports::GdipSetImageAttributesColorMatrix(nativeImageAttr, type, TRUE, colorMatrix, grayMatrix, mode));
    }

    Status
    SetColorMatrix(
        const ColorMatrix *colorMatrix,
        ColorMatrixFlags mode = ColorMatrixFlagsDefault,
        ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(
            DllExports::GdipSetImageAttributesColorMatrix(nativeImageAttr, type, TRUE, colorMatrix, NULL, mode));
    }

    Status
    SetGamma(REAL gamma, ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesGamma(nativeImageAttr, type, TRUE, gamma));
    }

    Status
    SetNoOp(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesNoOp(nativeImageAttr, type, TRUE));
    }

    Status
    SetOutputChannel(ColorChannelFlags channelFlags, ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesOutputChannel(nativeImageAttr, type, TRUE, channelFlags));
    }

    Status
    SetOutputChannelColorProfile(const WCHAR *colorProfileFilename, ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesOutputChannelColorProfile(
            nativeImageAttr, type, TRUE, colorProfileFilename));
    }

    Status
    SetRemapTable(UINT mapSize, const ColorMap *map, ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesRemapTable(nativeImageAttr, type, TRUE, mapSize, map));
    }

    Status
    SetThreshold(REAL threshold, ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesThreshold(nativeImageAttr, type, TRUE, threshold));
    }

    Status
    SetToIdentity(ColorAdjustType type = ColorAdjustTypeDefault)
    {
        return SetStatus(DllExports::GdipSetImageAttributesToIdentity(nativeImageAttr, type));
    }

    Status
    SetWrapMode(WrapMode wrap, const Color &color = Color(), BOOL clamp = FALSE)
    {
        ARGB argb = color.GetValue();
        return SetStatus(DllExports::GdipSetImageAttributesWrapMode(nativeImageAttr, wrap, argb, clamp));
    }

  protected:
    GpImageAttributes *nativeImageAttr;
    mutable Status lastStatus;

    ImageAttributes(GpImageAttributes *imageAttr, Status status) : nativeImageAttr(imageAttr), lastStatus(status)
    {
    }

    VOID
    SetNativeImageAttr(GpImageAttributes *imageAttr)
    {
        nativeImageAttr = imageAttr;
    }

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

  private:
    // ImageAttributes is not copyable
    ImageAttributes(const ImageAttributes &);
    ImageAttributes &
    operator=(const ImageAttributes &);

    // get native
    friend inline GpImageAttributes *&
    getNat(const ImageAttributes *ia)
    {
        return const_cast<ImageAttributes *>(ia)->nativeImageAttr;
    }
};

#endif /* _GDIPLUSIMAGEATTRIBUTES_H */
