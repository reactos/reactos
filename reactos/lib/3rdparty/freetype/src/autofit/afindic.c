/***************************************************************************/
/*                                                                         */
/*  afindic.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines for Indic scripts (body).               */
/*                                                                         */
/*  Copyright 2007, 2011-2013 by                                           */
/*  Rahul Bhalerao <rahul.bhalerao@redhat.com>, <b.rahul.pm@gmail.com>.    */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_INDIC

#include "afindic.h"
#include "aferrors.h"
#include "afcjk.h"


#ifdef AF_CONFIG_OPTION_USE_WARPER
#include "afwarp.h"
#endif


  static FT_Error
  af_indic_metrics_init( AF_CJKMetrics  metrics,
                         FT_Face        face )
  {
    /* skip blue zone init in CJK routines */
    FT_CharMap  oldmap = face->charmap;


    metrics->units_per_em = face->units_per_EM;

    if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) )
      face->charmap = NULL;
    else
    {
      af_cjk_metrics_init_widths( metrics, face );
#if 0
      /* either need indic specific blue_chars[] or just skip blue zones */
      af_cjk_metrics_init_blues( metrics, face, af_cjk_blue_chars );
#endif
      af_cjk_metrics_check_digits( metrics, face );
    }

    FT_Set_Charmap( face, oldmap );

    return FT_Err_Ok;
  }


  static void
  af_indic_metrics_scale( AF_CJKMetrics  metrics,
                          AF_Scaler      scaler )
  {
    /* use CJK routines */
    af_cjk_metrics_scale( metrics, scaler );
  }


  static FT_Error
  af_indic_hints_init( AF_GlyphHints  hints,
                       AF_CJKMetrics  metrics )
  {
    /* use CJK routines */
    return af_cjk_hints_init( hints, metrics );
  }


  static FT_Error
  af_indic_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline,
                        AF_CJKMetrics  metrics )
  {
    /* use CJK routines */
    return af_cjk_hints_apply( hints, outline, metrics );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                I N D I C   S C R I P T   C L A S S            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  AF_DEFINE_WRITING_SYSTEM_CLASS(
    af_indic_writing_system_class,

    AF_WRITING_SYSTEM_INDIC,

    sizeof ( AF_CJKMetricsRec ),

    (AF_Script_InitMetricsFunc) af_indic_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_indic_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_indic_hints_init,
    (AF_Script_ApplyHintsFunc)  af_indic_hints_apply
  )

  /* XXX: this should probably fine tuned to differentiate better between */
  /*      scripts...                                                      */

  static const AF_Script_UniRangeRec  af_deva_uniranges[] =
  {
    AF_UNIRANGE_REC(  0x0900UL,  0x0DFFUL ),  /* Indic Range  */
    AF_UNIRANGE_REC(  0x0F00UL,  0x0FFFUL ),  /* Tibetan      */
    AF_UNIRANGE_REC(  0x1900UL,  0x194FUL ),  /* Limbu        */
    AF_UNIRANGE_REC(  0x1B80UL,  0x1BBFUL ),  /* Sundanese    */
    AF_UNIRANGE_REC(  0x1C80UL,  0x1CDFUL ),  /* Meetei Mayak */
    AF_UNIRANGE_REC(  0xA800UL,  0xA82FUL ),  /* Syloti Nagri */
    AF_UNIRANGE_REC( 0x11800UL, 0x118DFUL ),  /* Sharada      */
    AF_UNIRANGE_REC(       0UL,       0UL )
  };


#else /* !AF_CONFIG_OPTION_INDIC */

  AF_DEFINE_WRITING_SYSTEM_CLASS(
    af_indic_writing_system_class,

    AF_WRITING_SYSTEM_INDIC,

    sizeof ( AF_CJKMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  )


  static const AF_Script_UniRangeRec  af_deva_uniranges[] =
  {
    AF_UNIRANGE_REC( 0UL, 0UL )
  };

#endif /* !AF_CONFIG_OPTION_INDIC */


  AF_DEFINE_SCRIPT_CLASS(
    af_deva_script_class,

    AF_SCRIPT_DEVA,
    (AF_Blue_Stringset)0, /* XXX */
    AF_WRITING_SYSTEM_INDIC,

    af_deva_uniranges,
    'o' /* XXX */
  )


/* END */
