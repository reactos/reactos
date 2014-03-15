/***************************************************************************/
/*                                                                         */
/*  afdummy.c                                                              */
/*                                                                         */
/*    Auto-fitter dummy routines to be used if no hinting should be        */
/*    performed (body).                                                    */
/*                                                                         */
/*  Copyright 2003-2005, 2011, 2013 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afdummy.h"
#include "afhints.h"
#include "aferrors.h"


  static FT_Error
  af_dummy_hints_init( AF_GlyphHints     hints,
                       AF_ScriptMetrics  metrics )
  {
    af_glyph_hints_rescale( hints, metrics );

    hints->x_scale = metrics->scaler.x_scale;
    hints->y_scale = metrics->scaler.y_scale;
    hints->x_delta = metrics->scaler.x_delta;
    hints->y_delta = metrics->scaler.y_delta;

    return FT_Err_Ok;
  }


  static FT_Error
  af_dummy_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline )
  {
    FT_Error  error;


    error = af_glyph_hints_reload( hints, outline );
    if ( !error )
      af_glyph_hints_save( hints, outline );

    return error;
  }


  AF_DEFINE_WRITING_SYSTEM_CLASS(
    af_dummy_writing_system_class,

    AF_WRITING_SYSTEM_DUMMY,

    sizeof ( AF_ScriptMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_dummy_hints_init,
    (AF_Script_ApplyHintsFunc)  af_dummy_hints_apply
  )


  AF_DEFINE_SCRIPT_CLASS(
    af_dflt_script_class,

    AF_SCRIPT_DFLT,
    (AF_Blue_Stringset)0,
    AF_WRITING_SYSTEM_DUMMY,

    NULL,
    '\0'
  )


/* END */
