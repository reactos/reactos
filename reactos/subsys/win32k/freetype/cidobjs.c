/***************************************************************************/
/*                                                                         */
/*  cidobjs.c                                                              */
/*                                                                         */
/*    CID objects manager (body).                                          */
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

#include "cidgload.h"
#include "cidload.h"

#else

#include <cid/cidgload.h>
#include <cid/cidload.h>

#endif


#include <freetype/internal/psnames.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef   FT_COMPONENT
#define  FT_COMPONENT  trace_cidobjs


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_Done_Face                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given face object.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A pointer to the face object to destroy.                   */
  /*                                                                       */
  LOCAL_FUNC
  void  CID_Done_Face( CID_Face  face )
  {
    FT_Memory  memory;


    if ( face )
    {
      CID_Info*     cid  = &face->cid;
      T1_FontInfo*  info = &cid->font_info;


      memory = face->root.memory;

      /* release FontInfo strings */
      FREE( info->version );
      FREE( info->notice );
      FREE( info->full_name );
      FREE( info->family_name );
      FREE( info->weight );

      /* release font dictionaries */
      FREE( cid->font_dicts );
      cid->num_dicts = 0;

      /* release other strings */
      FREE( cid->cid_font_name );
      FREE( cid->registry );
      FREE( cid->ordering );

      face->root.family_name = 0;
      face->root.style_name  = 0;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_Init_Face                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given CID face object.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The source font stream.                              */
  /*                                                                       */
  /*    face_index :: The index of the font face in the resource.          */
  /*                                                                       */
  /*    num_params :: Number of additional generic parameters.  Ignored.   */
  /*                                                                       */
  /*    params     :: Additional generic parameters.  Ignored.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The newly built face object.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  CID_Init_Face( FT_Stream      stream,
                           CID_Face       face,
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
      psnames = (PSNames_Interface*)FT_Get_Module_Interface(
                  FT_FACE_LIBRARY( face ), "psnames" );

      face->psnames = psnames;
    }

    /* open the tokenizer; this will also check the font format */
    if ( FILE_Seek( 0 ) )
      goto Exit;

    error = CID_Open_Face( face );
    if ( error )
      goto Exit;

    /* if we just wanted to check the format, leave successfully now */
    if ( face_index < 0 )
      goto Exit;

    /* check the face index */
    if ( face_index != 0 )
    {
      FT_ERROR(( "CID_Init_Face: invalid face index\n" ));
      error = T1_Err_Invalid_Argument;
      goto Exit;
    }

    /* Now, load the font program into the face object */
    {
      /* Init the face object fields */
      /* Now set up root face fields */
      {
        FT_Face  root = (FT_Face)&face->root;


        root->num_glyphs   = face->cid.cid_count;
        root->num_charmaps = 0;

        root->face_index = face_index;
        root->face_flags = FT_FACE_FLAG_SCALABLE;

        root->face_flags |= FT_FACE_FLAG_HORIZONTAL;

        if ( face->cid.font_info.is_fixed_pitch )
          root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

        /* XXX: TODO: add kerning with .afm support */

        /* get style name -- be careful, some broken fonts only */
        /* have a /FontName dictionary entry!                   */
        root->family_name = face->cid.font_info.family_name;
        if ( root->family_name )
        {
          char*  full   = face->cid.font_info.full_name;
          char*  family = root->family_name;

          while ( *family && *full == *family )
          {
            family++;
            full++;
          }

          root->style_name = ( *full == ' ' ) ? full + 1
                                              : (char *)"Regular";
        }
        else
        {
          /* do we have a `/FontName'? */
          if ( face->cid.cid_font_name )
          {
            root->family_name = face->cid.cid_font_name;
            root->style_name  = "Regular";
          }
        }

        /* no embedded bitmap support */
        root->num_fixed_sizes = 0;
        root->available_sizes = 0;

        root->bbox         = face->cid.font_bbox;
        root->units_per_EM = 1000;
        root->ascender     =  (FT_Short)face->cid.font_bbox.yMax;
        root->descender    = -(FT_Short)face->cid.font_bbox.yMin;
        root->height       = ( ( root->ascender + root->descender ) * 12 )
                             / 10;


#if 0

        /* now compute the maximum advance width */

        root->max_advance_width = face->type1.private_dict.standard_width[0];

        /* compute max advance width for proportional fonts */
        if ( !face->type1.font_info.is_fixed_pitch )
        {
          FT_Int  max_advance;


          error = CID_Compute_Max_Advance( face, &max_advance );

          /* in case of error, keep the standard width */
          if ( !error )
            root->max_advance_width = max_advance;
          else
            error = 0;   /* clear error */
        }

        root->max_advance_height = root->height;

#endif /* 0 */

        root->underline_position  = face->cid.font_info.underline_position;
        root->underline_thickness = face->cid.font_info.underline_thickness;

        root->max_points   = 0;
        root->max_contours = 0;
      }
    }

#if 0

    /* charmap support - synthetize unicode charmap when possible */
    {
      FT_Face      root    = &face->root;
      FT_CharMap   charmap = face->charmaprecs;


      /* synthesize a Unicode charmap if there is support in the `psnames' */
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

          /* simply clear the error in case of failure (which really */
          /* means that out of memory or no unicode glyph names)     */
          error = 0;
        }
      }

      /* now, support either the standard, expert, or custom encodings */
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

#endif /* 0 */

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_Init_Driver                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given CID driver object.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  CID_Init_Driver( CID_Driver  driver )
  {
    FT_UNUSED( driver );

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_Done_Driver                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given CID driver.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target CID driver.                       */
  /*                                                                       */
  LOCAL_DEF
  void  CID_Done_Driver( CID_Driver  driver )
  {
    FT_UNUSED( driver );
  }


/* END */
