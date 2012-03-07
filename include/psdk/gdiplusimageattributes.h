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
  ImageAttributes(VOID)
  {
  }

  Status ClearBrushRemapTable(VOID)
  {
    return NotImplemented;
  }

  Status ClearColorKey(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearColorMatrices(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearColorMatrix(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearGamma(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearNoOp(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearOutputChannel(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearOutputChannelColorProfile(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearRemapTable(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status ClearThreshold(ColorAdjustType type)
  {
    return NotImplemented;
  }

  ImageAttributes *Clone(VOID)
  {
    return NULL;
  }

  Status GetAdjustedPalette(ColorPalette *colorPalette, ColorPalette colorAdjustType)
  {
    return NotImplemented;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  Status Reset(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetBrushRemapTable(UINT mapSize, ColorMap *map)
  {
    return NotImplemented;
  }

  Status SetColorKey(const Color &colorLow, const Color &colorHigh, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetColorMatrices(const ColorMatrix *colorMatrix, const ColorMatrix *grayMatrix, ColorMatrixFlags mode, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetColorMatrix(const ColorMatrix *colorMatrix, ColorMatrixFlags mode, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetGamma(REAL gamma, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetNoOp(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetOutputChannel(ColorChannelFlags channelFlags, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetOutputChannelColorProfile(const WCHAR *colorProfileFilename, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetRemapTable(UINT mapSize, const ColorMap *map, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetThreshold(REAL threshold, ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetToIdentity(ColorAdjustType type)
  {
    return NotImplemented;
  }

  Status SetWrapMode(WrapMode wrap, const Color &color, BOOL clamp)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSIMAGEATTRIBUTES_H */
