/***************************************************************************/
/*                                                                         */
/*  ftsmooth.h                                                             */
/*                                                                         */
/*    Anti-aliasing renderer interface (specification).                    */
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


#ifndef FTSMOOTH_H
#define FTSMOOTH_H

#include <freetype/ftrender.h>

#ifndef FT_CONFIG_OPTION_NO_STD_RASTER
  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_std_renderer_class;
#endif

#ifndef FT_CONFIG_OPTION_NO_SMOOTH_RASTER
  FT_EXPORT_VAR( const FT_Renderer_Class )  ft_smooth_renderer_class;
#endif

#endif /* FTSMOOTH_H */


/* END */
