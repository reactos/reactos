/***************************************************************************/
/*                                                                         */
/*  ahglyph.h                                                              */
/*                                                                         */
/*    Routines used to load and analyze a given glyph before hinting       */
/*    (specification).                                                     */
/*                                                                         */
/*  Copyright 2000 Catharon Productions Inc.                               */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#ifndef AHGLYPH_H
#define AHGLYPH_H

#ifdef FT_FLAT_COMPILE

#include "ahtypes.h"

#else

#include <freetype/src/autohint/ahtypes.h>

#endif


  typedef enum  AH_UV_
  {
    ah_uv_fxy,
    ah_uv_fyx,
    ah_uv_oxy,
    ah_uv_oyx,
    ah_uv_ox,
    ah_uv_oy,
    ah_uv_yx,
    ah_uv_xy  /* should always be last! */

  } AH_UV;


  LOCAL_DEF
  void  ah_setup_uv( AH_Outline*  outline,
                     AH_UV        source );


  /* AH_Outline functions - they should be typically called in this order */

  LOCAL_DEF
  FT_Error  ah_outline_new( FT_Memory     memory,
                            AH_Outline**  aoutline );

  LOCAL_DEF
  FT_Error  ah_outline_load( AH_Outline*  outline,
                             FT_Face      face );

  LOCAL_DEF
  void  ah_outline_compute_segments( AH_Outline*  outline );

  LOCAL_DEF
  void  ah_outline_link_segments( AH_Outline*  outline );

  LOCAL_DEF
  void  ah_outline_detect_features( AH_Outline*  outline );

  LOCAL_DEF
  void  ah_outline_compute_blue_edges( AH_Outline*       outline,
                                       AH_Face_Globals*  globals );

  LOCAL_DEF
  void  ah_outline_scale_blue_edges( AH_Outline*       outline,
                                     AH_Face_Globals*  globals );

  LOCAL_DEF
  void  ah_outline_save( AH_Outline*  outline, AH_Loader*  loader );

  LOCAL_DEF
  void  ah_outline_done( AH_Outline*  outline );


#endif /* AHGLYPH_H */


/* END */
