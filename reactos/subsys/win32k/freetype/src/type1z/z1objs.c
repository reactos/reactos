/***************************************************************************/
/*                                                                         */
/*  z1objs.c                                                               */
/*                                                                         */
/*    Experimental Type 1 objects manager (body).                          */
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

#include "z1gload.h"
#include "z1load.h"
#include "z1afm.h"

#else

#include <type1z/z1gload.h>
#include <type1z/z1load.h>
#include <type1z/z1afm.h>

#endif


#include <freetype/internal/psnames.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_z1objs


  /*************************************************************************/
  /*                                                                       */
  /*                            FACE  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Done_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The face object destructor.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A typeless pointer to the face object to destroy.          */
  /*                                                                       */
  LOCAL_FUNC
  void  Z1_Done_Face( T1_Face  face )
  {
    FT_Memory  memory;
    T1_Font*   type1 = &face->type1;


    if ( face )
    {
      memory = face->root.memory;

#ifndef Z1_CONFIG_OPTION_NO_MM_SUPPORT
      /* release multiple masters information */
      Z1_Done_Blend( face );
      face->blend = 0;
#endif

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

#ifndef Z1_CONFIG_OPTION_NO_AFM
      /* release afm data if present */
      if ( face->afm_data )
        Z1_Done_AFM( memory, (Z1_AFM*)face->afm_data );
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
  /*    Z1_Init_Face                                                       */
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
  FT_Error  Z1_Init_Face( FT_Stream      stream,
                          T1_Face        face,
                          FT_Int         face_index,
                          FT_Int         num_params,
                          FT_Parameter*  params )
  {
    FT_Error            error;
    PSNames_Interface*  psnames;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );
    FT_UNUSED( stream );


    face->root.num_faces = 1;

    psnames = (PSNames_Interface*)face->psnames;
    if ( !psnames )
    {
      psnames = (PSNames_Interface*)
                FT_Get_Module_Interface( FT_FACE_LIBRARY( face ), "psnames" );

      face->psnames = psnames;
    }

    /* open the tokenizer, this will also check the font format */
    error = Z1_Open_Face( face );
    if ( error )
      goto Exit;

    /* if we just wanted to check the format, leave successfully now */
    if ( face_index < 0 )
      goto Exit;

    /* check the face index */
    if ( face_index != 0 )
    {
      FT_ERROR(( "Z1_Init_Face: invalid face index\n" ));
      error = T1_Err_Invalid_Argument;
      goto Exit;
    }

    /* Now, load the font program into the face object */

    /* Init the face object fields */
    /* Now set up root face fields */
    {
      FT_Face  root = (FT_Face)&face->root;


      root->num_glyphs   = face->type1.num_glyphs;
      root->num_charmaps = 1;

      root->face_index = face_index;
      root->face_flags = FT_FACE_FLAG_SCALABLE;

      root->face_flags |= FT_FACE_FLAG_HORIZONTAL;

      root->face_flags |= FT_FACE_FLAG_GLYPH_NAMES;

      if ( face->type1.font_info.is_fixed_pitch )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      if ( face->blend )
        root->face_flags |= FT_FACE_FLAG_MULTIPLE_MASTERS;

      /* XXX: TODO -- add kerning with .afm support */

      /* get style name -- be careful, some broken fonts only */
      /* have a `/FontName' dictionary entry!                 */
      root->family_name = face->type1.font_info.family_name;
      if ( root->family_name )
      {
        char*  full   = face->type1.font_info.full_name;
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
        if ( face->type1.font_name )
        {
          root->family_name = face->type1.font_name;
          root->style_name  = "Regular";
        }
      }

      /* no embedded bitmap support */
      root->num_fixed_sizes = 0;
      root->available_sizes = 0;

      root->bbox         = face->type1.font_bbox;
      root->units_per_EM = 1000;
      root->ascender     =  (FT_Short)face->type1.font_bbox.yMax;
      root->descender    = -(FT_Short)face->type1.font_bbox.yMin;
      root->height       = ( ( root->ascender + root->descender ) * 12 ) / 10;

      /* now compute the maximum advance width */

      root->max_advance_width = face->type1.private_dict.standard_width[0];

      /* compute max advance width for proportional fonts */
      if ( !face->type1.font_info.is_fixed_pitch )
      {
        FT_Int  max_advance;


        error = Z1_Compute_Max_Advance( face, &max_advance );

        /* in case of error, keep the standard width */
        if ( !error )
          root->max_advance_width = max_advance;
        else
          error = 0;   /* clear error */
      }

      root->max_advance_height = root->height;

      root->underline_position  = face->type1.font_info.underline_position;
      root->underline_thickness = face->type1.font_info.underline_thickness;

      root->max_points   = 0;
      root->max_contours = 0;
    }

    /* charmap support -- synthetize unicode charmap if possible */
    {
      FT_Face     root    = &face->root;
      FT_CharMap  charmap = face->charmaprecs;


      /* synthesize a Unicode charmap if there is support in the `PSNames' */
      /* module                                                            */
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
          error = FT_Err_Ok;
        }
      }

      /* now, support either the standard, expert, or custom encoding */
      charmap->face        = (FT_Face)face;
      charmap->platform_id = 7;  /* a new platform id for Adobe fonts? */

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

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Init_Driver                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given Type 1 driver object.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  Z1_Init_Driver( Z1_Driver  driver )
  {
    FT_UNUSED( driver );

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Done_Driver                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given Type 1 driver.                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver  :: A handle to the target Type 1 driver.                   */
  /*                                                                       */
  LOCAL_DEF
  void  Z1_Done_Driver( Z1_Driver  driver )
  {
    FT_UNUSED( driver );
  }


/* END */
