/***************************************************************************/
/*                                                                         */
/*  cidafm.h                                                               */
/*                                                                         */
/*    AFM support for CID-keyed fonts (specification).                     */
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


#ifndef CIDAFM_H
#define CIDAFM_H


#ifdef FT_FLAT_COMPILE

#include "cidobjs.h"

#else

#include <freetype/src/cid/cidobjs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  typedef struct  CID_Kern_Pair_
  {
    FT_UInt    glyph1;
    FT_UInt    glyph2;
    FT_Vector  kerning;

  } CID_Kern_Pair;

  typedef struct  CID_AFM_
  {
    FT_UInt         num_pairs;
    CID_Kern_Pair*  kern_pairs;

  } CID_AFM;


  LOCAL_DEF
  FT_Error  CID_Read_AFM( FT_Face    cid_face,
                          FT_Stream  stream );

  LOCAL_DEF
  void  CID_Done_AFM( FT_Memory  memory,
                      CID_AFM*   afm );

  LOCAL_DEF
  void  CID_Get_Kerning( CID_AFM*    afm,
                         FT_UInt     glyph1,
                         FT_UInt     glyph2,
                         FT_Vector*  kerning );


#ifdef __cplusplus
  }
#endif


#endif /* CIDAFM_H */


/* END */
