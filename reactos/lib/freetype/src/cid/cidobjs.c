/***************************************************************************/
/*                                                                         */
/*  cidobjs.c                                                              */
/*                                                                         */
/*    CID objects manager (body).                                          */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "cidgload.h"
#include "cidload.h"

#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidobjs


  /*************************************************************************/
  /*                                                                       */
  /*                            SLOT  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  cid_slot_done( CID_GlyphSlot  slot )
  {
    slot->root.internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  cid_slot_init( CID_GlyphSlot  slot )
  {
    CID_Face          face;
    PSHinter_Service  pshinter;


    face     = (CID_Face)slot->root.face;
    pshinter = (PSHinter_Service)face->pshinter;

    if ( pshinter )
    {
      FT_Module  module;


      module = FT_Get_Module( slot->root.face->driver->root.library,
                              "pshinter" );
      if ( module )
      {
        T1_Hints_Funcs  funcs;


        funcs = pshinter->get_t1_funcs( module );
        slot->root.internal->glyph_hints = (void*)funcs;
      }
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           SIZE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/


  static PSH_Globals_Funcs
  cid_size_get_globals_funcs( CID_Size  size )
  {
    CID_Face          face     = (CID_Face)size->root.face;
    PSHinter_Service  pshinter = (PSHinter_Service)face->pshinter;
    FT_Module         module;


    module = FT_Get_Module( size->root.face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0;
  }


  FT_LOCAL_DEF( void )
  cid_size_done( CID_Size  size )
  {
    if ( size->root.internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = cid_size_get_globals_funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)size->root.internal );

      size->root.internal = 0;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cid_size_init( CID_Size  size )
  {
    FT_Error           error = 0;
    PSH_Globals_Funcs  funcs = cid_size_get_globals_funcs( size );


    if ( funcs )
    {
      PSH_Globals   globals;
      CID_Face      face = (CID_Face)size->root.face;
      CID_FaceDict  dict = face->cid.font_dicts + face->root.face_index;
      PS_Private    priv = &dict->private_dict;


      error = funcs->create( size->root.face->memory, priv, &globals );
      if ( !error )
        size->root.internal = (FT_Size_Internal)(void*)globals;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  cid_size_reset( CID_Size  size )
  {
    PSH_Globals_Funcs  funcs = cid_size_get_globals_funcs( size );
    FT_Error           error = 0;


    if ( funcs )
      error = funcs->set_scale( (PSH_Globals)size->root.internal,
                                 size->root.metrics.x_scale,
                                 size->root.metrics.y_scale,
                                 0, 0 );
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_face_done                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given face object.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A pointer to the face object to destroy.                   */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  cid_face_done( CID_Face  face )
  {
    FT_Memory  memory;


    if ( face )
    {
      CID_FaceInfo  cid  = &face->cid;
      PS_FontInfo   info = &cid->font_info;


      memory = face->root.memory;

      /* release subrs */
      if ( face->subrs )
      {
        FT_Int  n;
        

        for ( n = 0; n < cid->num_dicts; n++ )
        {
          CID_Subrs  subr = face->subrs + n;
          

          if ( subr->code )
          {
            FT_FREE( subr->code[0] );
            FT_FREE( subr->code );
          }
        }

        FT_FREE( face->subrs );
      }

      /* release FontInfo strings */
      FT_FREE( info->version );
      FT_FREE( info->notice );
      FT_FREE( info->full_name );
      FT_FREE( info->family_name );
      FT_FREE( info->weight );

      /* release font dictionaries */
      FT_FREE( cid->font_dicts );
      cid->num_dicts = 0;

      /* release other strings */
      FT_FREE( cid->cid_font_name );
      FT_FREE( cid->registry );
      FT_FREE( cid->ordering );

      face->root.family_name = 0;
      face->root.style_name  = 0;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_face_init                                                      */
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
  FT_LOCAL_DEF( FT_Error )
  cid_face_init( FT_Stream      stream,
                 CID_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error          error;
    PSNames_Service   psnames;
    PSAux_Service     psaux;
    PSHinter_Service  pshinter;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );
    FT_UNUSED( stream );


    face->root.num_faces = 1;

    psnames = (PSNames_Service)face->psnames;
    if ( !psnames )
    {
      psnames = (PSNames_Service)FT_Get_Module_Interface(
                  FT_FACE_LIBRARY( face ), "psnames" );

      face->psnames = psnames;
    }

    psaux = (PSAux_Service)face->psaux;
    if ( !psaux )
    {
      psaux = (PSAux_Service)FT_Get_Module_Interface(
                FT_FACE_LIBRARY( face ), "psaux" );

      face->psaux = psaux;
    }

    pshinter = (PSHinter_Service)face->pshinter;
    if ( !pshinter )
    {
      pshinter = (PSHinter_Service)FT_Get_Module_Interface(
                   FT_FACE_LIBRARY( face ), "pshinter" );

      face->pshinter = pshinter;
    }

    /* open the tokenizer; this will also check the font format */
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    error = cid_face_open( face );
    if ( error )
      goto Exit;

    /* if we just wanted to check the format, leave successfully now */
    if ( face_index < 0 )
      goto Exit;

    /* check the face index */
    if ( face_index != 0 )
    {
      FT_ERROR(( "cid_face_init: invalid face index\n" ));
      error = CID_Err_Invalid_Argument;
      goto Exit;
    }

    /* now load the font program into the face object */

    /* initialize the face object fields */

    /* set up root face fields */
    {
      FT_Face       root = (FT_Face)&face->root;
      CID_FaceInfo  cid  = &face->cid;
      PS_FontInfo   info = &cid->font_info;


      root->num_glyphs   = cid->cid_count;
      root->num_charmaps = 0;

      root->face_index = face_index;
      root->face_flags = FT_FACE_FLAG_SCALABLE;

      root->face_flags |= FT_FACE_FLAG_HORIZONTAL;

      if ( info->is_fixed_pitch )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* XXX: TODO: add kerning with .afm support */

      /* get style name -- be careful, some broken fonts only */
      /* have a /FontName dictionary entry!                   */
      root->family_name = info->family_name;
      /* assume "Regular" style if we don't know better */
      root->style_name = (char *)"Regular";
      if ( root->family_name )
      {
        char*  full   = info->full_name;
        char*  family = root->family_name;


        if ( full )
        {
          while ( *full )
          {
            if ( *full == *family )
            {
              family++;
              full++;
            }
            else
            {
              if ( *full == ' ' || *full == '-' )
                full++;
              else if ( *family == ' ' || *family == '-' )
                family++;
              else
              {
                if ( !*family )
                  root->style_name = full;
                break;
              }
            }
          }
        }
      }
      else
      {
        /* do we have a `/FontName'? */
        if ( cid->cid_font_name )
          root->family_name = cid->cid_font_name;
      }

      /* compute style flags */
      root->style_flags = 0;
      if ( info->italic_angle )
        root->style_flags |= FT_STYLE_FLAG_ITALIC;
      if ( info->weight )
      {
        if ( !ft_strcmp( info->weight, "Bold"  ) ||
             !ft_strcmp( info->weight, "Black" ) )
          root->style_flags |= FT_STYLE_FLAG_BOLD;
      }

      /* no embedded bitmap support */
      root->num_fixed_sizes = 0;
      root->available_sizes = 0;

      root->bbox.xMin =   cid->font_bbox.xMin             >> 16;
      root->bbox.yMin =   cid->font_bbox.yMin             >> 16;
      root->bbox.xMax = ( cid->font_bbox.xMax + 0xFFFFU ) >> 16;
      root->bbox.yMax = ( cid->font_bbox.yMax + 0xFFFFU ) >> 16;

      if ( !root->units_per_EM )
        root->units_per_EM = 1000;

      root->ascender  = (FT_Short)( root->bbox.yMax );
      root->descender = (FT_Short)( root->bbox.yMin );
      root->height    = (FT_Short)(
        ( ( root->ascender - root->descender ) * 12 ) / 10 );

      root->underline_position  = info->underline_position >> 16;
      root->underline_thickness = info->underline_thickness >> 16;

      root->internal->max_points   = 0;
      root->internal->max_contours = 0;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_driver_init                                                    */
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
  FT_LOCAL_DEF( FT_Error )
  cid_driver_init( CID_Driver  driver )
  {
    FT_UNUSED( driver );

    return CID_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cid_driver_done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given CID driver.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target CID driver.                       */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  cid_driver_done( CID_Driver  driver )
  {
    FT_UNUSED( driver );
  }


/* END */
