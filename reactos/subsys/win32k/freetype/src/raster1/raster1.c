/***************************************************************************/
/*                                                                         */
/*  raster1.c                                                              */
/*                                                                         */
/*    FreeType monochrome rasterer module component (body only).           */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#define FT_MAKE_OPTION_SINGLE_OBJECT


#ifdef FT_FLAT_COMPILE

#include "ftraster.c"
#include "ftrend1.c"

#else

#include <raster1/ftraster.c>
#include <raster1/ftrend1.c>

#endif


/* END */
