#include "afdummy.h"
#include "afhints.h"


  static FT_Error
  af_dummy_hints_init( AF_GlyphHints     hints,
                       AF_ScriptMetrics  metrics )
  {
    af_glyph_hints_rescale( hints,
                            metrics );
    return 0;
  }

  static FT_Error
  af_dummy_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline )
  {
    FT_UNUSED( hints );
    FT_UNUSED( outline );

    return 0;
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
