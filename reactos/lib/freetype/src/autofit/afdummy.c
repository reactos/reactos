#include "afdummy.h"


  static FT_Error
  af_dummy_hints_init( AF_GlyphHints     hints,
                       FT_Outline*       outline,
                       AF_ScriptMetrics  metrics )
  {
    return af_glyph_hints_reset( hints,
                                 &metrics->scaler,
                                 metrics,
                                 outline );
  }

  static FT_Error
  af_dummy_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline )
  {
    af_glyph_hints_save( hints, outline );
  }


  FT_LOCAL_DEF( const AF_ScriptClassRec )  af_dummy_script_class =
  {
    AF_SCRIPT_NONE,
    NULL,

    sizeof( AF_ScriptMetricsRec ),
    (AF_Script_InitMetricsFunc)  NULL,
    (AF_Script_ScaleMetricsFunc) NULL,
    (AF_Script_DoneMetricsFunc)  NULL,

    (AF_Script_InitHintsFunc)    af_dummy_hints_init,
    (AF_Script_ApplyHintsFunc)   af_dummy_hints_apply
  };
