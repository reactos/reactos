/*
 * GdiPlusColorMatrix.h
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

#ifndef _GDIPLUSCOLORMATRIX_H
#define _GDIPLUSCOLORMATRIX_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

typedef struct {
  Color oldColor;
  Color newColor;
} ColorMap;

struct ColorMatrix {
  REAL m[5][5];
};

typedef enum {
  ColorAdjustTypeDefault = 0,
  ColorAdjustTypeBitmap = 1,
  ColorAdjustTypeBrush = 2,
  ColorAdjustTypePen = 3,
  ColorAdjustTypeText = 4,
  ColorAdjustTypeCount = 5,
  ColorAdjustTypeAny = 6
} ColorAdjustType;

typedef enum {
  ColorMatrixFlagsDefault = 0,
  ColorMatrixFlagsSkipGrays = 1,
  ColorMatrixFlagsAltGray = 2
} ColorMatrixFlags;

#endif /* _GDIPLUSCOLORMATRIX_H */
