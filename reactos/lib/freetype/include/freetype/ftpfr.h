/***************************************************************************/
/*                                                                         */
/*  ftpfr.h                                                                */
/*                                                                         */
/*    FreeType API for accessing PFR-specific data                         */
/*                                                                         */
/*  Copyright 2002 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTPFR_H__
#define __FTPFR_H__

#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    pfr_fonts                                                          */
  /*                                                                       */
  /* <Title>                                                               */
  /*    PFR Fonts                                                          */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    PFR/TrueDoc specific APIs                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the declaration of PFR-specific functions.   */
  /*                                                                       */
  /*************************************************************************/


 /**********************************************************************
  *
  * @function:
  *    FT_Get_PFR_Metrics
  *
  * @description:
  *    returns the outline and metrics resolutions of a given PFR
  *    face.
  *
  * @input:
  *    face :: handle to input face. It can be a non-PFR face.
  *
  * @output:
  *    aoutline_resolution ::
  *      outline resolution. This is equivalent to "face->units_per_EM".
  *      optional (parameter can be NULL)
  *
  *    ametrics_resolution ::
  *      metrics_resolution. This is equivalent to "outline_resolution"
  *      for non-PFR fonts. can be NULL
  *      optional (parameter can be NULL)
  *
  *    ametrics_x_scale ::
  *      a 16.16 fixed-point number used to scale distance expressed
  *      in metrics units to device sub-pixels. This is equivalent to
  *      'face->size->x_scale', but for metrics only.
  *      optional (parameter can be NULL)
  *
  *    ametrics_y_scale ::
  *      same as 'ametrics_x_scale', but for the vertical direction.
  *      optional (parameter can be NULL)
  *
  * @note:
  *   if the input face is not a PFR, this function will return an error.
  *   However, in all cases, it will return valid values.
  */
  FT_EXPORT( FT_Error )
  FT_Get_PFR_Metrics( FT_Face     face,
                      FT_UInt    *aoutline_resolution,
                      FT_UInt    *ametrics_resolution,
                      FT_Fixed   *ametrics_x_scale,
                      FT_Fixed   *ametrics_y_scale );

 /**********************************************************************
  *
  * @function:
  *    FT_Get_PFR_Kerning
  *
  * @description:
  *    returns the kerning pair corresponding to two glyphs in
  *    a PFR face. The distance is expressed in metrics units, unlike
  *    the result of @FT_Get_Kerning.
  *
  * @input:
  *    face :: handle to input face.
  *    left  :: left glyph index
  *    right :: right glyph index
  *
  * @output:
  *    avector :: kerning vector
  *
  * @note:
  *    this function always return distances in original PFR metrics
  *    units. This is unlike @FT_Get_Kerning with the @FT_KERNING_UNSCALED
  *    mode, which always return distances converted to outline units.
  *
  *    you can use the value of the 'x_scale' and 'y_scale' parameters
  *    returned by @FT_Get_PFR_Metrics to scale these to device sub-pixels
  */
  FT_EXPORT( FT_Error )
  FT_Get_PFR_Kerning( FT_Face     face,
                      FT_UInt     left,
                      FT_UInt     right,
                      FT_Vector  *avector );

 /**********************************************************************
  *
  * @function:
  *    FT_Get_PFR_Advance
  *
  * @description:
  *    returns a given glyph advance, expressed in original metrics units,
  *    from a PFR font.
  *
  * @input:
  *    face   :: handle to input face.
  *    gindex :: glyph index
  *
  * @output:
  *    aadvance :: glyph advance in metrics units
  *
  * @return:
  *    error code. 0 means success
  *
  * @note:
  *    you can use the 'x_scale' or 'y_scale' results of @FT_Get_PFR_Metrics
  *    to convert the advance to device sub-pixels (i.e. 1/64th of pixels)
  */
  FT_EXPORT( FT_Error )
  FT_Get_PFR_Advance( FT_Face    face,
                      FT_UInt    gindex,
                      FT_Pos    *aadvance );

 /* */

FT_END_HEADER

#endif /* __FTBDF_H__ */


/* END */
