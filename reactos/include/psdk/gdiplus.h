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

#ifndef _GDIPLUS_H
#define _GDIPLUS_H

#ifdef __cplusplus

namespace Gdiplus
{
    namespace DllExports
    {
#include "gdiplusmem.h"
    };

#include "gdiplusenums.h"
#include "gdiplustypes.h"
#include "gdiplusinit.h"
#include "gdipluspixelformats.h"
#include "gdiplusmetaheader.h"
#include "gdiplusimaging.h"
#include "gdipluscolor.h"
#include "gdipluscolormatrix.h"
#include "gdiplusgpstubs.h"

    namespace DllExports
    {
#include "gdiplusflat.h"
    };
};

#else /* end c++ includes */

#include "gdiplusmem.h"

#include "gdiplusenums.h"
#include "gdiplustypes.h"
#include "gdiplusinit.h"
#include "gdipluspixelformats.h"
#include "gdiplusmetaheader.h"
#include "gdiplusimaging.h"
#include "gdipluscolor.h"
#include "gdipluscolormatrix.h"
#include "gdiplusgpstubs.h"

#include "gdiplusflat.h"

#endif /* end c includes */

#endif /* _GDIPLUS_H_ */
