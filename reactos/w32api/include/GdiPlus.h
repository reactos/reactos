/*
 * GdiPlus.h
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

#ifndef _GDIPLUS_H
#define _GDIPLUS_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifndef __cplusplus
#error In order to use GDI+ headers use must use a C++ compiler
#else

struct IDirectDrawSurface7;

typedef signed short INT16;
typedef unsigned short UINT16;

#include <pshpack8.h>

namespace Gdiplus
{
  namespace DllExports
  {
    #include "GdiplusMem.h"
  };

  #include "GdiplusBase.h"
  
  #include "GdiplusEnums.h"
  #include "GdiplusTypes.h"
  #include "GdiplusInit.h"
  #include "GdiplusPixelFormats.h"
  #include "GdiplusColor.h"
  #include "GdiplusMetaHeader.h"
  #include "GdiplusImaging.h"
  #include "GdiplusColorMatrix.h"
  #include "GdiplusGpStubs.h"
  #include "GdiplusHeaders.h"

  namespace DllExports
  {
    #include "GdiPlusFlat.h"
  };

  #include "GdiplusImageAttributes.h"
  #include "GdiplusMatrix.h"
  #include "GdiplusBrush.h"
  #include "GdiplusPen.h"
  #include "GdiplusStringFormat.h"
  #include "GdiplusPath.h"
  #include "GdiplusLineCaps.h"
  #include "GdiplusMetaFile.h"
  #include "GdiplusGraphics.h"
  #include "GdiplusEffects.h"
}

#include <poppack.h>

#endif

#endif /* _GDIPLUS_H */
