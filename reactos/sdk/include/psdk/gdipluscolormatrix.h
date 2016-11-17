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

#ifndef _GDIPLUSCOLORMATRIX_H
#define _GDIPLUSCOLORMATRIX_H

struct ColorMatrix
{
    REAL m[5][5];
};

enum ColorMatrixFlags
{
    ColorMatrixFlagsDefault    = 0,
    ColorMatrixFlagsSkipGrays  = 1,
    ColorMatrixFlagsAltGray    = 2
};

enum ColorAdjustType
{
    ColorAdjustTypeDefault,
    ColorAdjustTypeBitmap,
    ColorAdjustTypeBrush,
    ColorAdjustTypePen,
    ColorAdjustTypeText,
    ColorAdjustTypeCount,
    ColorAdjustTypeAny
};

struct ColorMap
{
    Color oldColor;
    Color newColor;
};

enum HistogramFormat
{
    HistogramFormatARGB,
    HistogramFormatPARGB,
    HistogramFormatRGB,
    HistogramFormatGray,
    HistogramFormatB,
    HistogramFormatG,
    HistogramFormatR,
    HistogramFormatA,
};

#ifndef __cplusplus

typedef enum ColorAdjustType ColorAdjustType;
typedef enum ColorMatrixFlags ColorMatrixFlags;
typedef enum HistogramFormat HistogramFormat;
typedef struct ColorMatrix ColorMatrix;
typedef struct ColorMap ColorMap;

#endif  /* end of c typedefs */

#endif  /* _GDIPLUSCOLORMATRIX_H */
