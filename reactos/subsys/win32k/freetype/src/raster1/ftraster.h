/***************************************************************************/
/*                                                                         */
/*  ftraster.h                                                             */
/*                                                                         */
/*    The FreeType glyph rasterizer (specification).                       */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTRASTER_H
#define FTRASTER_H

#ifdef __cplusplus
  extern "C" {
#endif

#include <freetype/ftimage.h>


  /*************************************************************************/
  /*                                                                       */
  /* Uncomment the following line if you are using ftraster.c as a         */
  /* standalone module, fully independent of FreeType.                     */
  /*                                                                       */
/* #define _STANDALONE_ */

#ifndef FT_EXPORT_VAR
#define FT_EXPORT_VAR( x )  extern x
#endif

  FT_EXPORT_VAR( FT_Raster_Funcs )  ft_standard_raster;

#ifdef __cplusplus
  }
#endif


#endif /* FTRASTER_H */


/* END */
