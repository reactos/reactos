#include "afglobal.h"
#include "afdummy.h"
#include "aflatin.h"

 /* populate this list when you add new scripts
  */
  static AF_ScriptClass const   af_script_classes[] =
  {
    & af_dummy_script_class,
    & af_latin_script_class,

    NULL  /* do not remove */
  };

#define AF_SCRIPT_LIST_DEFAULT   1    /* index of default script in 'af_script_classes' */
#define AF_SCRIPT_LIST_NONE      255  /* indicates an uncovered glyph                      */

 /*
  *  note that glyph_scripts[] is used to map each glyph into
  *  an index into the 'af_script_classes' array.
  *
  */
  typedef struct AF_FaceGlobalsRec_
  {
    FT_Face            face;
    FT_UInt            glyph_count;    /* same as face->num_glyphs     */
    FT_Byte*           glyph_scripts;

    AF_ScriptMetrics   metrics[ AF_SCRIPT_MAX ];

  } AF_FaceGlobalsRec;




 /* this function is used to compute the script index of each glyph
  * within a given face
  */
  static FT_Error
  af_face_globals_compute_script_coverage( AF_FaceGlobals  globals )
  {
    FT_Error    error       = 0;
    FT_Face     face        = globals->face;
    FT_CharMap  old_charmap = face->charmap;
    FT_Byte*    gscripts    = globals->glyph_scripts;
    FT_UInt     ss;

   /* the value 255 means "uncovered glyph"
    */
    FT_MEM_SET( globals->glyph_scripts,
                AF_SCRIPT_LIST_NONE,
                globals->glyph_count );

    error = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
    if ( error )
    {
     /* ignore this error, we'll simply use Latin as the standard
      * script. XXX: Shouldn't we rather disable hinting ??
      */
      error = 0;
      goto Exit;
    }

   /* scan each script in a Unicode charmap
    */
    for ( ss = 0; af_script_classes[ss]; ss++ )
    {
      AF_ScriptClass      clazz = af_script_classes[ss];
      AF_Script_UniRange  range;

      if ( clazz->script_uni_ranges == NULL )
        continue;

     /* scan all unicode points in the range, and set the corresponding
      * glyph script index
      */
      for ( range = clazz->script_uni_ranges; range->first != 0; range++ )
      {
        FT_ULong  charcode = range->first;
        FT_UInt   gindex;

        gindex = FT_Get_Char_Index( face, charcode );

        if ( gindex != 0                               &&
             gindex < globals->glyph_count             &&
             gscripts[ gindex ] == AF_SCRIPT_LIST_NONE )
        {
          gscripts[ gindex ] = (FT_Byte) ss;
        }
        for (;;)
        {
          charcode = FT_Get_Next_Char( face, charcode, &gindex );

          if ( gindex == 0 || charcode > range->last )
            break;

          if ( gindex < globals->glyph_count       &&
               gscripts[ gindex ] == AF_SCRIPT_LIST_NONE )
          {
            gscripts[ gindex ] = (FT_Byte) ss;
          }
        }
      }
    }

  Exit:
   /* by default, all uncovered glyphs are set to the latin script
    * XXX: shouldnt' we disable hinting or do something similar ?
    */
    {
      FT_UInt  nn;

      for ( nn = 0; nn < globals->glyph_count; nn++ )
      {
        if ( gscripts[ nn ] == AF_SCRIPT_LIST_NONE )
          gscripts[ nn ] = AF_SCRIPT_LIST_DEFAULT;
      }
    }

    FT_Set_Charmap( face, old_charmap );
    return error;
  }



  FT_LOCAL_DEF( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals )
  {
    FT_Error        error;
    FT_Memory       memory;
    AF_FaceGlobals  globals;

    memory = face->memory;

    if ( !FT_ALLOC( globals, sizeof(*globals) +
                             face->num_glyphs*sizeof(FT_Byte) ) )
    {
      globals->face          = face;
      globals->glyph_count   = face->num_glyphs;
      globals->glyph_scripts = (FT_Byte*)( globals+1 );

      error = af_face_globals_compute_script_coverage( globals );
      if ( error )
      {
        af_face_globals_free( globals );
        globals = NULL;
      }
    }

    *aglobals = globals;
    return error;
  }


  FT_LOCAL_DEF( void )
  af_face_globals_free( AF_FaceGlobals  globals )
  {
    if ( globals )
    {
      FT_Memory  memory = globals->face->memory;
      FT_UInt    nn;

      for ( nn = 0; nn < AF_SCRIPT_MAX; nn++ )
      {
        if ( globals->metrics[nn] )
        {
          AF_ScriptClass  clazz = af_script_classes[nn];

          FT_ASSERT( globals->metrics[nn]->clazz == clazz );

          if ( clazz->script_metrics_done )
            clazz->script_metrics_done( globals->metrics[nn] );

          FT_FREE( globals->metrics[nn] );
        }
      }

      globals->glyph_count   = 0;
      globals->glyph_scripts = NULL;  /* no need to free this one !! */
      globals->face          = NULL;
      FT_FREE( globals );
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               AF_ScriptMetrics  *ametrics )
  {
    AF_ScriptMetrics  metrics = NULL;
    FT_UInt           index;
    AF_ScriptClass    clazz;
    FT_Error          error = 0;

    if ( gindex >= globals->glyph_count )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    index   = globals->glyph_scripts[ gindex ];
    clazz   = af_script_classes[ index ];
    metrics = globals->metrics[ clazz->script ];
    if ( metrics == NULL )
    {
     /* create the global metrics object when needed
      */
      FT_Memory  memory = globals->face->memory;

      if ( FT_ALLOC( metrics, clazz->script_metrics_size ) )
        goto Exit;

      metrics->clazz = clazz;

      if ( clazz->script_metrics_init )
      {
        error = clazz->script_metrics_init( metrics, globals->face );
        if ( error )
        {
          if ( clazz->script_metrics_done )
            clazz->script_metrics_done( metrics );

          FT_FREE( metrics );
          goto Exit;
        }
      }

      globals->metrics[ clazz->script ] = metrics;
    }

  Exit:
    *ametrics = metrics;
    return error;
  }
