/***************************************************************************/
/*                                                                         */
/*  ftrend1.h                                                              */
/*                                                                         */
/*    The FreeType glyph rasterizer interface (specification).             */
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


#ifndef FTREND1_H
#define FTREND1_H

#include <freetype/ftrender.h>


  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_raster1_renderer_class;

  /* this renderer is _NOT_ part of the default modules, you'll need */
  /* to register it by hand in your application.  It should only be  */
  /* used for backwards-compatibility with FT 1.x anyway.            */
  /*                                                                 */
  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_raster5_renderer_class;


#endif /* FTREND1_H */


/* END */
