/***************************************************************************/
/*                                                                         */
/*  t1objs.c                                                               */
/*                                                                         */
/*    Type 1 objects manager (body).                                       */
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


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>


#ifdef FT_FLAT_COMPILE

#include "t1gload.h"
#include "t1load.h"
#include "t1afm.h"

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
#include "t1hinter.h"
#endif

#else /* FT_FLAT_COMPILE */

#include <freetype/src/type1/t1gload.h>
#include <freetype/src/type1/t1load.h>
#include <freetype/src/type1/t1afm.h>

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
#include <freetype/src/type1/t1hinter.h>
#endif

#endif /* FT_FLAT_COMPILE */


#include <freetype/internal/psnames.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1objs


  /*************************************************************************/
  /*                                                                       */
  /*                           SIZE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Done_Size                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The Type 1 size object destructor.  Used to discard a given size   */
  /*    object.                                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Done_Size( T1_Size  size )
  {
    if ( size )
    {

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
      T1_Done_Size_Hinter( size );
#endif

      size->valid = 0;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Init_Size                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The size object initializer.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeTrue error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_DEF
  FT_Error  T1_Init_Size( T1_Size  size )
  {
    FT_Error  error;


    size->valid = 0;

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
    error = T1_New_Size_Hinter( size );

    return error;
#else

    FT_UNUSED( error );

    return T1_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Reset_Size                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Resets an instance to a new pointsize/transform.  This function is */
  /*    in charge of resetting the blue zones,a s well as the stem snap    */
  /*    tables for a given size.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: The target size object.                                    */
  /*                                                                       */
  /* <Output>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_Reset_Size( T1_Size  size )
  {
    /* recompute ascender, descender, etc. */
    T1_Face           face    = (T1_Face)size->root.face;
    FT_Size_Metrics*  metrics = &size->root.metrics;


    if ( metrics->x_ppem < 1 || metrics->y_ppem < 1 )
      return FT_Err_Invalid_Argument;

    /* Compute root ascender, descender, test height, and max_advance */
    metrics->ascender    = ( FT_MulFix( face->root.ascender,
                                        metrics->y_scale ) + 32 ) & -64;
    metrics->descender   = ( FT_MulFix( face->root.descender,
                                        metrics->y_scale ) + 32 ) & -64;
    metrics->height      = ( FT_MulFix( face->root.height,
                                        metrics->y_scale ) + 32 ) & -64;
    metrics->max_advance = ( FT_MulFix( face->root.max_advance_width,
                                        metrics->x_scale ) + 32 ) & -64;

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
    return T1_Reset_Size_Hinter( size );
#else
    return 0;
#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /*                            FACE  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Done_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The face object destructor.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A typeless pointer to the face object to destroy.          */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Done_Face( T1_Face  face )
  {
    FT_Memory  memory;
    T1_Font*   type1 = &face->type1;


    if ( face )
    {
      memory = face->root.memory;

      /* release font info strings */
      {
        T1_FontInfo*  info = &type1->font_info;


        FREE( info->version );
        FREE( info->notice );
        FREE( info->full_name );
        FREE( info->family_name );
        FREE( info->weight );
      }

      /* release top dictionary */
      FREE( type1->charstrings_len );
      FREE( type1->charstrings );
      FREE( type1->glyph_names );

      FREE( type1->subrs );
      FREE( type1->subrs_len );

      FREE( type1->subrs_block );
      FREE( type1->charstrings_block );
      FREE( type1->glyph_names_block );

      FREE( type1->encoding.char_index );
      FREE( type1->font_name );

#ifndef T1_CONFIG_OPTION_NO_AFM
      /* release afm data if present */
      if ( face->afm_data )
        T1_Done_AFM( memory, (T1_AFM*)face->afm_data );
#endif

      /* release unicode map, if any */
      FREE( face->unicode_map.maps );
      face->unicode_map.num_maps = 0;

      face->root.family_name = 0;
      face->root.style_name  = 0;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Init_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The face object constructor.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     ::  input stream where to load font data.               */
  /*                                                                       */
  /*    face_index :: The index of the font face in the resource.          */
  /*                                                                       */
  /*    num_params :: Number of additional generic parameters.  Ignored.   */
  /*                                                                       */
  /*    params     :: Additional generic parameters.  Ignored.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The face record to build.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_Init_Face( FT_Stream      stream,
                          T1_Face        face,
                          FT_Int         face_index,
                          FT_Int         num_params,
                          FT_Parameter*  params )
  {
    T1_Tokenizer        tokenizer;
    FT_Error            error;
    PSNames_Interface*  psnames;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );
    FT_UNUSED( face );


    face->root.num_faces = 1;

    psnames = (PSNames_Interface*)face->psnames;
    if ( !psnames )
    {
      psnames = (PSNames_Interface*)
                 FT_Get_Module_Interface( FT_FACE_LIBRARY( face ),
                                          "psnames" );

      face->psnames = psnames;
    }

    /* open the tokenizer, this will also check the font format */
    error = New_Tokenizer( stream, &tokenizer );
    if ( error )
      goto Fail;

    /* if we just wanted to check the format, leave successfully now */
    if ( face_index < 0 )
      goto Leave;

    /* check the face index */
    if ( face_index != 0 )
    {
      FT_ERROR(( "T1_Init_Face: invalid face index\n" ));
      error = T1_Err_Invalid_Argument;
      goto Leave;
    }

    /* Now, load the font program into the face object */
    {
      T1_Parser  parser;


      Init_T1_Parser( &parser, face, tokenizer );
      error = Parse_T1_FontProgram( &parser );
      if ( error )
        goto Leave;

      /* Init the face object fields */
      /* Now set up root face fields */
      {
        FT_Face   root  = (FT_Face)&face->root;
        T1_Font*  type1 = &face->type1;


        root->num_glyphs   = type1->num_glyphs;
        root->num_charmaps = 1;

        root->face_index = face_index;
        root->face_flags = FT_FACE_FLAG_SCALABLE;

        root->face_flags |= FT_FACE_FLAG_HORIZONTAL;

        root->face_flags |= FT_FACE_FLAG_GLYPH_NAMES;
        
        if ( type1->font_info.is_fixed_pitch )
          root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

        /* XXX: TODO -- add kerning with .afm support */

        /* get style name - be careful, some broken fonts only */
        /* have a `/FontName' dictionary entry!                */
        root->family_name = type1->font_info.family_name;
        if ( root->family_name )
        {
          char*  full   = type1->font_info.full_name;
          char*  family = root->family_name;


          while ( *family && *full == *family )
          {
            family++;
            full++;
          }

          root->style_name = ( *full == ' ' ? full + 1
                                            : (char *)"Regular" );
        }
        else
        {
          /* do we have a `/FontName'? */
          if (type1->font_name)
          {
            root->family_name = type1->font_name;
            root->style_name  = "Regular";
          }
        }

        /* no embedded bitmap support */
        root->num_fixed_sizes = 0;
        root->available_sizes = 0;

        root->bbox         = type1->font_bbox;
        root->units_per_EM = 1000;
        root->ascender     =  (FT_Short)type1->font_bbox.yMax;
        root->descender    = -(FT_Short)type1->font_bbox.yMin;
        root->height       = ( ( root->ascender + root->descender) * 12 )
                             / 10;

        /* now compute the maximum advance width */

        root->max_advance_width = type1->private_dict.standard_width[0];

        /* compute max advance width for proportional fonts */
        if ( !type1->font_info.is_fixed_pitch )
        {
          FT_Int  max_advance;


          error = T1_Compute_Max_Advance( face, &max_advance );

          /* in case of error, keep the standard width */
          if ( !error )
            root->max_advance_width = max_advance;
          else
            error = 0;   /* clear error */
        }

        root->max_advance_height = root->height;

        root->underline_position  = type1->font_info.underline_position;
        root->underline_thickness = type1->font_info.underline_thickness;

        root->max_points   = 0;
        root->max_contours = 0;
      }
    }

    /* charmap support - synthetize unicode charmap when possible */
    {
      FT_Face     root    = &face->root;
      FT_CharMap  charmap = face->charmaprecs;


      /* synthesize a Unicode charmap if there is support in the `PSNames' */
      /* module..                                                          */
      if ( face->psnames )
      {
        PSNames_Interface*  psnames = (PSNames_Interface*)face->psnames;


        if ( psnames->unicode_value )
        {
          error = psnames->build_unicodes(
                    root->memory,
                    face->type1.num_glyphs,
                    (const char**)face->type1.glyph_names,
                    &face->unicode_map );
          if ( !error )
          {
            root->charmap        = charmap;
            charmap->face        = (FT_Face)face;
            charmap->encoding    = ft_encoding_unicode;
            charmap->platform_id = 3;
            charmap->encoding_id = 1;
            charmap++;
          }

          /* simply clear the error in case of failure (which really) */
          /* means that out of memory or no unicode glyph names       */
          error = 0;
        }
      }

      /* now, support either the standard, expert, or custom encodings */
      charmap->face        = (FT_Face)face;
      charmap->platform_id = 7;  /* a new platform id for Adobe fonts ?? */

      switch ( face->type1.encoding_type )
      {
      case t1_encoding_standard:
        charmap->encoding    = ft_encoding_adobe_standard;
        charmap->encoding_id = 0;
        break;

      case t1_encoding_expert:
        charmap->encoding    = ft_encoding_adobe_expert;
        charmap->encoding_id = 1;
        break;

      default:
        charmap->encoding    = ft_encoding_adobe_custom;
        charmap->encoding_id = 2;
        break;
      }

      root->charmaps     = face->charmaps;
      root->num_charmaps = charmap - face->charmaprecs + 1;
      face->charmaps[0]  = &face->charmaprecs[0];
      face->charmaps[1]  = &face->charmaprecs[1];
    }

  Leave:
    Done_Tokenizer( tokenizer );

  Fail:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Done_GlyphSlot                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The glyph slot object destructor.                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph :: The glyph slot handle to destroy.                         */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Done_GlyphSlot( T1_GlyphSlot  glyph )
  {
#ifndef T1_CONFIG_OPTION_DISABLE_HINTER

    T1_Done_Glyph_Hinter( glyph );

#else

    FT_UNUSED( glyph );

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Init_GlyphSlot                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The glyph slot object constructor.                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph :: The glyph slot handle to initialize.                      */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_Init_GlyphSlot( T1_GlyphSlot  glyph )
  {
    FT_Error    error = FT_Err_Ok;


#ifndef T1_CONFIG_OPTION_DISABLE_HINTER

    error = T1_New_Glyph_Hinter( glyph );

#else

    FT_UNUSED( glyph );

#endif

    return error;
  }


/* END */
