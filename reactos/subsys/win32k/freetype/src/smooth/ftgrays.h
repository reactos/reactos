/***************************************************************************/
/*                                                                         */
/*  ftgrays.h                                                              */
/*                                                                         */
/*    FreeType smooth renderer declaration                                 */
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

#ifndef FTGRAYS_H
#define FTGRAYS_H

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef _STANDALONE_
#include "ftimage.h"
#else
#include <freetype/ftimage.h>
#endif

  /*************************************************************************/
  /*                                                                       */
  /* To make ftgrays.h independent from configuration files we check       */
  /* whether FT_EXPORT_DEF has been defined already.                       */
  /*                                                                       */
  /* On some systems and compilers (Win32 mostly), an extra keyword is     */
  /* necessary to compile the library as a DLL.                            */
  /*                                                                       */
#ifndef FT_EXPORT_VAR
#define FT_EXPORT_VAR( x )  extern  x
#endif

  FT_EXPORT_VAR( FT_Raster_Funcs )  ft_grays_raster;

#ifdef __cplusplus
  }
#endif

#endif /* FTGRAYS_H */


/* END */
