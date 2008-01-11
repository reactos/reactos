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
    #include "gdiplusmem.h"
  };

  #include "gdiplusbase.h"

  #include "gdiplusenums.h"
  #include "gdiplustypes.h"
  #include "gdiplusinit.h"
  #include "gdipluspixelformats.h"
  #include "gdipluscolor.h"
  #include "gdiplusmetaheader.h"
  #include "gdiplusimaging.h"
  #include "gdipluscolormatrix.h"
  #include "gdiplusgpstubs.h"
  #include "gdiplusheaders.h"

  namespace DllExports
  {
    #include "gdiplusflat.h"
  };

  #include "gdiplusimageattributes.h"
  #include "gdiplusmatrix.h"
  #include "gdiplusbrush.h"
  #include "gdipluspen.h"
  #include "gdiplusstringformat.h"
  #include "gdipluspath.h"
  #include "gdipluslinecaps.h"
  #include "gdiplusmetafile.h"
  #include "gdiplusgraphics.h"
  #include "gdipluseffects.h"
}

#include <poppack.h>

#endif

#endif /* _GDIPLUS_H */
