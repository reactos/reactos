/***************************************************************************/
/*                                                                         */
/*  ttcmap.h                                                               */
/*                                                                         */
/*    TrueType character mapping table (cmap) support (specification).     */
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


#ifndef TTCMAP_H
#define TTCMAP_H

#include <freetype/internal/tttypes.h>

#ifdef __cplusplus
  extern "C" {
#endif


  LOCAL_DEF
  FT_Error  TT_CharMap_Load( TT_Face        face,
                             TT_CMapTable*  cmap,
                             FT_Stream      input );

  LOCAL_DEF
  FT_Error  TT_CharMap_Free( TT_Face        face,
                             TT_CMapTable*  cmap );

#ifdef __cplusplus
  }
#endif

#endif /* TTCMAP_H */


/* END */
