/***************************************************************************/
/*                                                                         */
/*  cffobjs.c                                                              */
/*                                                                         */
/*    OpenType objects manager (body).                                     */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005 by                         */
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
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_ERRORS_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_SFNT_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include "cffobjs.h"
#include "cffload.h"
#include "cffcmap.h"
#include "cfferrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffobjs


  /*************************************************************************/
  /*                                                                       */
  /*                            SIZE FUNCTIONS                             */
  /*                                                                       */
  /*  Note that we store the global hints in the size's `internal' root    */
  /*  field.                                                               */
  /*                                                                       */
  /*************************************************************************/


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  static FT_Error
  sbit_size_reset( CFF_Size  size )
  {
    CFF_Face          face;
    FT_Error          error = CFF_Err_Ok;

    FT_ULong          strike_index;
    FT_Size_Metrics*  metrics;
    FT_Size_Metrics*  sbit_metrics;
    SFNT_Service      sfnt;


    metrics = &size->root.metrics;

    face = (CFF_Face)size->root.face;
    sfnt = (SFNT_Service)face->sfnt;

    sbit_metrics = &size->strike_metrics;

    error = sfnt->set_sbit_strike( face,
                                   metrics->x_ppem,
                                   metrics->y_ppem,
                                   &strike_index );

    if ( !error )
    {
      /* XXX: TODO: move this code to the SFNT module where it belongs */

#ifdef FT_OPTIMIZE_MEMORY

      FT_Byte*    strike = face->sbit_table + 8 + strike_index*48;

      sbit_metrics->ascender  = (FT_Char)strike[16] << 6;  /* hori.ascender  */
      sbit_metrics->descender = (FT_Char)strike[17] << 6;  /* hori.descender */

      /* XXX: Is this correct? */
      sbit_metrics->max_advance = ( (FT_Char)strike[22] + /* min_origin_SB  */
                                             strike[18] + /* max_width      */
                                    (FT_Char)strike[23]   /* min_advance_SB */
                                                        ) << 6;

#else /* !OPTIMIZE_MEMORY */

      TT_SBit_Strike  strike = face->sbit_strikes + strike_index;


      sbit_metrics->ascender  = strike->hori.ascender << 6;
      sbit_metrics->descender = strike->hori.descender << 6;

      /* XXX: Is this correct? */
      sbit_metrics->max_advance = ( strike->hori.min_origin_SB  +
                                    strike->hori.max_width      +
                                    strike->hori.min_advance_SB ) << 6;

#endif /* !OPTIMIZE_MEMORY */

      /* XXX: Is this correct? */
      sbit_metrics->height = sbit_metrics->ascender -
                             sbit_metrics->descender;

      sbit_metrics->x_ppem = metrics->x_ppem;
      sbit_metrics->y_ppem = metrics->y_ppem;
      size->strike_index   = (FT_UInt)strike_index;
    }
    else
    {
      size->strike_index = 0xFFFFU;

      sbit_metrics->x_ppem      = 0;
      sbit_metrics->y_ppem      = 0;
      sbit_metrics->ascender    = 0;
      sbit_metrics->descender   = 0;
      sbit_metrics->height      = 0;
      sbit_metrics->max_advance = 0;
    }

    return error;
  }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


  static PSH_Globals_Funcs
  cff_size_get_globals_funcs( CFF_Size  size )
  {
    CFF_Face          face     = (CFF_Face)size->root.face;
    CFF_Font          font     = (CFF_FontRec *)face->extra.data;
    PSHinter_Service  pshinter = (PSHinter_Service)font->pshinter;
    FT_Module         module;


    module = FT_Get_Module( size->root.face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0;
  }


  FT_LOCAL_DEF( void )
  cff_size_done( FT_Size  cffsize )        /* CFF_Size */
  {
    CFF_Size  size = (CFF_Size)cffsize;


    if ( cffsize->internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = cff_size_get_globals_funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)cffsize->internal );

      cffsize->internal = 0;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cff_size_init( FT_Size  cffsize )         /* CFF_Size */
  {
    CFF_Size           size  = (CFF_Size)cffsize;
    FT_Error           error = CFF_Err_Ok;
    PSH_Globals_Funcs  funcs = cff_size_get_globals_funcs( size );


    if ( funcs )
    {
      PSH_Globals    globals;
      CFF_Face       face    = (CFF_Face)cffsize->face;
      CFF_Font       font    = (CFF_FontRec *)face->extra.data;
      CFF_SubFont    subfont = &font->top_font;

      CFF_Private    cpriv   = &subfont->private_dict;
      PS_PrivateRec  priv;


      /* IMPORTANT: The CFF and Type1 private dictionaries have    */
      /*            slightly different structures; we need to      */
      /*            synthetize a type1 dictionary on the fly here. */

      {
        FT_UInt  n, count;


        FT_MEM_ZERO( &priv, sizeof ( priv ) );

        count = priv.num_blue_values = cpriv->num_blue_values;
        for ( n = 0; n < count; n++ )
          priv.blue_values[n] = (FT_Short)cpriv->blue_values[n];

        count = priv.num_other_blues = cpriv->num_other_blues;
        for ( n = 0; n < count; n++ )
          priv.other_blues[n] = (FT_Short)cpriv->other_blues[n];

        count = priv.num_family_blues = cpriv->num_family_blues;
        for ( n = 0; n < count; n++ )
          priv.family_blues[n] = (FT_Short)cpriv->family_blues[n];

        count = priv.num_family_other_blues = cpriv->num_family_other_blues;
        for ( n = 0; n < count; n++ )
          priv.family_other_blues[n] = (FT_Short)cpriv->family_other_blues[n];

        priv.blue_scale = cpriv->blue_scale;
        priv.blue_shift = (FT_Int)cpriv->blue_shift;
        priv.blue_fuzz  = (FT_Int)cpriv->blue_fuzz;

        priv.standard_width[0]  = (FT_UShort)cpriv->standard_width;
        priv.standard_height[0] = (FT_UShort)cpriv->standard_height;

        count = priv.num_snap_widths = cpriv->num_snap_widths;
        for ( n = 0; n < count; n++ )
          priv.snap_widths[n] = (FT_Short)cpriv->snap_widths[n];

        count = priv.num_snap_heights = cpriv->num_snap_heights;
        for ( n = 0; n < count; n++ )
          priv.snap_heights[n] = (FT_Short)cpriv->snap_heights[n];

        priv.force_bold     = cpriv->force_bold;
        priv.language_group = cpriv->language_group;
        priv.lenIV          = cpriv->lenIV;
      }

      error = funcs->create( cffsize->face->memory, &priv, &globals );
      if ( !error )
        cffsize->internal = (FT_Size_Internal)(void*)globals;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_size_reset( FT_Size  cffsize,         /* CFF_Size */
                  FT_UInt  char_width,
                  FT_UInt  char_height )
  {
    CFF_Size           size  = (CFF_Size)cffsize;
    PSH_Globals_Funcs  funcs = cff_size_get_globals_funcs( size );
    FT_Error           error = CFF_Err_Ok;
    FT_Face            face  = cffsize->face;

    FT_UNUSED( char_width );
    FT_UNUSED( char_height );


    if ( funcs )
      error = funcs->set_scale( (PSH_Globals)cffsize->internal,
                                 cffsize->metrics.x_scale,
                                 cffsize->metrics.y_scale,
                                 0, 0 );

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    if ( face->face_flags & FT_FACE_FLAG_FIXED_SIZES )
    {
      error = sbit_size_reset( size );

      if ( !error && !( face->face_flags & FT_FACE_FLAG_SCALABLE ) )
        cffsize->metrics = size->strike_metrics;
    }

#endif

    if ( face->face_flags & FT_FACE_FLAG_SCALABLE )
      return CFF_Err_Ok;
    else
      return error;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_point_size_reset( FT_Size     cffsize,
                        FT_F26Dot6  char_width,
                        FT_F26Dot6  char_height,
                        FT_UInt     horz_resolution,
                        FT_UInt     vert_resolution )
  {
    FT_UNUSED( char_width );
    FT_UNUSED( char_height );
    FT_UNUSED( horz_resolution );
    FT_UNUSED( vert_resolution );

    return cff_size_reset( cffsize, 0, 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /*                            SLOT  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  cff_slot_done( FT_GlyphSlot  slot )
  {
    slot->internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_slot_init( FT_GlyphSlot  slot )
  {
    CFF_Face          face     = (CFF_Face)slot->face;
    CFF_Font          font     = (CFF_FontRec *)face->extra.data;
    PSHinter_Service  pshinter = (PSHinter_Service)font->pshinter;


    if ( pshinter )
    {
      FT_Module  module;


      module = FT_Get_Module( slot->face->driver->root.library,
                              "pshinter" );
      if ( module )
      {
        T2_Hints_Funcs  funcs;


        funcs = pshinter->get_t2_funcs( module );
        slot->internal->glyph_hints = (void*)funcs;
      }
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

  static FT_String*
  cff_strcpy( FT_Memory         memory,
              const FT_String*  source )
  {
    FT_Error    error;
    FT_String*  result = 0;
    FT_Int      len = (FT_Int)ft_strlen( source );


    if ( !FT_ALLOC( result, len + 1 ) )
    {
      FT_MEM_COPY( result, source, len );
      result[len] = 0;
    }

    FT_UNUSED( error );

    return result;
  }


  FT_LOCAL_DEF( FT_Error )
  cff_face_init( FT_Stream      stream,
                 FT_Face        cffface,        /* CFF_Face */
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    CFF_Face            face = (CFF_Face)cffface;
    FT_Error            error;
    SFNT_Service        sfnt;
    FT_Service_PsCMaps  psnames;
    PSHinter_Service    pshinter;
    FT_Bool             pure_cff    = 1;
    FT_Bool             sfnt_format = 0;


#if 0
    FT_FACE_FIND_GLOBAL_SERVICE( face, sfnt,     SFNT );
    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames,  POSTSCRIPT_NAMES );
    FT_FACE_FIND_GLOBAL_SERVICE( face, pshinter, POSTSCRIPT_HINTER );

    if ( !sfnt )
      goto Bad_Format;
#else
    sfnt = (SFNT_Service)FT_Get_Module_Interface(
             cffface->driver->root.library, "sfnt" );
    if ( !sfnt )
      goto Bad_Format;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );

    pshinter = (PSHinter_Service)FT_Get_Module_Interface(
                 cffface->driver->root.library, "pshinter" );
#endif

    /* create input stream from resource */
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    /* check whether we have a valid OpenType file */
    error = sfnt->init_face( stream, face, face_index, num_params, params );
    if ( !error )
    {
      if ( face->format_tag != 0x4F54544FL )  /* `OTTO'; OpenType/CFF font */
      {
        FT_TRACE2(( "[not a valid OpenType/CFF font]\n" ));
        goto Bad_Format;
      }

      /* if we are performing a simple font format check, exit immediately */
      if ( face_index < 0 )
        return CFF_Err_Ok;

      /* UNDOCUMENTED!  A CFF in an SFNT can have only a single font. */
      if ( face_index > 0 )
      {
        FT_ERROR(( "cff_face_init: invalid face index\n" ));
        error = CFF_Err_Invalid_Argument;
        goto Exit;
      }

      sfnt_format = 1;

      /* now, the font can be either an OpenType/CFF font, or an SVG CEF */
      /* font; in the latter case it doesn't have a `head' table         */
      error = face->goto_table( face, TTAG_head, stream, 0 );
      if ( !error )
      {
        pure_cff = 0;

        /* load font directory */
        error = sfnt->load_face( stream, face,
                                 face_index, num_params, params );
        if ( error )
          goto Exit;
      }
      else
      {
        /* load the `cmap' table explicitly */
        error = sfnt->load_charmaps( face, stream );
        if ( error )
          goto Exit;

        /* XXX: we don't load the GPOS table, as OpenType Layout     */
        /* support will be added later to a layout library on top of */
        /* FreeType 2                                                */
      }

      /* now load the CFF part of the file */
      error = face->goto_table( face, TTAG_CFF, stream, 0 );
      if ( error )
        goto Exit;
    }
    else
    {
      /* rewind to start of file; we are going to load a pure-CFF font */
      if ( FT_STREAM_SEEK( 0 ) )
        goto Exit;
      error = CFF_Err_Ok;
    }

    /* now load and parse the CFF table in the file */
    {
      CFF_Font         cff;
      CFF_FontRecDict  dict;
      FT_Memory        memory = cffface->memory;
      FT_Int32         flags;
      FT_UInt          i;


      if ( FT_NEW( cff ) )
        goto Exit;

      face->extra.data = cff;
      error = cff_font_load( stream, face_index, cff );
      if ( error )
        goto Exit;

      cff->pshinter = pshinter;
      cff->psnames  = (void*)psnames;

      /* Complement the root flags with some interesting information. */
      /* Note that this is only necessary for pure CFF and CEF fonts; */
      /* SFNT based fonts use the `name' table instead.               */

      cffface->num_glyphs = cff->num_glyphs;

      dict = &cff->top_font.font_dict;

      /* we need the `PSNames' module for CFF and CEF formats */
      /* which aren't CID-keyed                               */
      if ( dict->cid_registry == 0xFFFFU && !psnames )
      {
        FT_ERROR(( "cff_face_init:" ));
        FT_ERROR(( " cannot open CFF & CEF fonts\n" ));
        FT_ERROR(( "              " ));
        FT_ERROR(( " without the `PSNames' module\n" ));
        goto Bad_Format;
      }

      if ( pure_cff )
      {
        char*  style_name = NULL;


        /* set up num_faces */
        cffface->num_faces = cff->num_faces;

        /* compute number of glyphs */
        if ( dict->cid_registry != 0xFFFFU )
          cffface->num_glyphs = dict->cid_count;
        else
          cffface->num_glyphs = cff->charstrings_index.count;

        /* set global bbox, as well as EM size */
        cffface->bbox.xMin =   dict->font_bbox.xMin             >> 16;
        cffface->bbox.yMin =   dict->font_bbox.yMin             >> 16;
        cffface->bbox.xMax = ( dict->font_bbox.xMax + 0xFFFFU ) >> 16;
        cffface->bbox.yMax = ( dict->font_bbox.yMax + 0xFFFFU ) >> 16;

        cffface->ascender  = (FT_Short)( cffface->bbox.yMax );
        cffface->descender = (FT_Short)( cffface->bbox.yMin );
        cffface->height    = (FT_Short)(
          ( ( cffface->ascender - cffface->descender ) * 12 ) / 10 );

        if ( !dict->units_per_em )
          dict->units_per_em = 1000;

        cffface->units_per_EM = dict->units_per_em;

        cffface->underline_position  =
          (FT_Short)( dict->underline_position >> 16 );
        cffface->underline_thickness =
          (FT_Short)( dict->underline_thickness >> 16 );

        /* retrieve font family & style name */
        cffface->family_name = cff_index_get_name( &cff->name_index,
                                                   face_index );

        if ( cffface->family_name )
        {
          char*  full   = cff_index_get_sid_string( &cff->string_index,
                                                    dict->full_name,
                                                    psnames );
          char*  fullp  = full;
          char*  family = cffface->family_name;
          char*  family_name = 0;


          if ( dict->family_name )
          {
            family_name = cff_index_get_sid_string( &cff->string_index,
                                                    dict->family_name,
                                                    psnames);
            if ( family_name )
              family = family_name;
          }

          /* We try to extract the style name from the full name.   */
          /* We need to ignore spaces and dashes during the search. */
          if ( full && family )
          {
            while ( *fullp )
            {
              /* skip common characters at the start of both strings */
              if ( *fullp == *family )
              {
                family++;
                fullp++;
                continue;
              }

              /* ignore spaces and dashes in full name during comparison */
              if ( *fullp == ' ' || *fullp == '-' )
              {
                fullp++;
                continue;
              }

              /* ignore spaces and dashes in family name during comparison */
              if ( *family == ' ' || *family == '-' )
              {
                family++;
                continue;
              }

              if ( !*family && *fullp )
              {
                /* The full name begins with the same characters as the  */
                /* family name, with spaces and dashes removed.  In this */
                /* case, the remaining string in `fullp' will be used as */
                /* the style name.                                       */
                style_name = cff_strcpy( memory, fullp );
              }
              break;
            }

            if ( family_name )
              FT_FREE( family_name );
            FT_FREE( full );
          }
        }
        else
        {
          char  *cid_font_name =
                   cff_index_get_sid_string( &cff->string_index,
                                             dict->cid_font_name,
                                             psnames );


          /* do we have a `/FontName' for a CID-keyed font? */
          if ( cid_font_name )
            cffface->family_name = cid_font_name;
        }

        if ( style_name )
          cffface->style_name = style_name;
        else
          /* assume "Regular" style if we don't know better */
          cffface->style_name = cff_strcpy( memory, (char *)"Regular" );

        /*******************************************************************/
        /*                                                                 */
        /* Compute face flags.                                             */
        /*                                                                 */
        flags = FT_FACE_FLAG_SCALABLE  |    /* scalable outlines */
                FT_FACE_FLAG_HORIZONTAL;    /* horizontal data   */

        if ( sfnt_format )
          flags |= FT_FACE_FLAG_SFNT;

        /* fixed width font? */
        if ( dict->is_fixed_pitch )
          flags |= FT_FACE_FLAG_FIXED_WIDTH;

  /* XXX: WE DO NOT SUPPORT KERNING METRICS IN THE GPOS TABLE FOR NOW */
#if 0
        /* kerning available? */
        if ( face->kern_pairs )
          flags |= FT_FACE_FLAG_KERNING;
#endif

        cffface->face_flags = flags;

        /*******************************************************************/
        /*                                                                 */
        /* Compute style flags.                                            */
        /*                                                                 */
        flags = 0;

        if ( dict->italic_angle )
          flags |= FT_STYLE_FLAG_ITALIC;

        {
          char  *weight = cff_index_get_sid_string( &cff->string_index,
                                                    dict->weight,
                                                    psnames );


          if ( weight )
            if ( !ft_strcmp( weight, "Bold"  ) ||
                 !ft_strcmp( weight, "Black" ) )
              flags |= FT_STYLE_FLAG_BOLD;
          FT_FREE( weight );
        }

        /* double check */
        if ( !(flags & FT_STYLE_FLAG_BOLD) && cffface->style_name )
          if ( !strncmp( cffface->style_name, "Bold", 4 )  ||
               !strncmp( cffface->style_name, "Black", 5 ) )
            flags |= FT_STYLE_FLAG_BOLD;

        cffface->style_flags = flags;
      }
      else
      {
        if ( !dict->units_per_em )
          dict->units_per_em = face->root.units_per_EM;
      }

      /* handle font matrix settings in subfonts (if any) */
      for ( i = cff->num_subfonts; i > 0; i-- )
      {
        CFF_FontRecDict  sub = &cff->subfonts[i - 1]->font_dict;
        CFF_FontRecDict  top = &cff->top_font.font_dict;


        if ( sub->units_per_em )
        {
          FT_Matrix  scale;


          scale.xx = scale.yy = (FT_Fixed)FT_DivFix( top->units_per_em,
                                                     sub->units_per_em );
          scale.xy = scale.yx = 0;

          FT_Matrix_Multiply( &scale, &sub->font_matrix );
          FT_Vector_Transform( &sub->font_offset, &scale );
        }
        else
        {
          sub->font_matrix = top->font_matrix;
          sub->font_offset = top->font_offset;
        }
      }

#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES
      /* CID-keyed CFF fonts don't have glyph names -- the SFNT loader */
      /* has unset this flag because of the 3.0 `post' table           */
      if ( dict->cid_registry == 0xFFFFU )
        cffface->face_flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

      /*******************************************************************/
      /*                                                                 */
      /* Compute char maps.                                              */
      /*                                                                 */

      /* Try to synthetize a Unicode charmap if there is none available */
      /* already.  If an OpenType font contains a Unicode "cmap", we    */
      /* will use it, whatever be in the CFF part of the file.          */
      {
        FT_CharMapRec  cmaprec;
        FT_CharMap     cmap;
        FT_UInt        nn;
        CFF_Encoding   encoding = &cff->encoding;


        for ( nn = 0; nn < (FT_UInt)cffface->num_charmaps; nn++ )
        {
          cmap = cffface->charmaps[nn];

          /* Windows Unicode (3,1)? */
          if ( cmap->platform_id == 3 && cmap->encoding_id == 1 )
            goto Skip_Unicode;

          /* Deprecated Unicode platform id? */
          if ( cmap->platform_id == 0 )
            goto Skip_Unicode; /* Standard Unicode (deprecated) */
        }

        /* since CID-keyed fonts don't contain glyph names, we can't */
        /* construct a cmap                                          */
        if ( pure_cff && cff->top_font.font_dict.cid_registry != 0xFFFFU )
          goto Exit;

        /* we didn't find a Unicode charmap -- synthetize one */
        cmaprec.face        = cffface;
        cmaprec.platform_id = 3;
        cmaprec.encoding_id = 1;
        cmaprec.encoding    = FT_ENCODING_UNICODE;

        nn = (FT_UInt)cffface->num_charmaps;

        FT_CMap_New( &cff_cmap_unicode_class_rec, NULL, &cmaprec, NULL );

        /* if no Unicode charmap was previously selected, select this one */
        if ( cffface->charmap == NULL && nn != (FT_UInt)cffface->num_charmaps )
          cffface->charmap = cffface->charmaps[nn];

      Skip_Unicode:
        if ( encoding->count > 0 )
        {
          FT_CMap_Class  clazz;


          cmaprec.face        = cffface;
          cmaprec.platform_id = 7;  /* Adobe platform id */

          if ( encoding->offset == 0 )
          {
            cmaprec.encoding_id = TT_ADOBE_ID_STANDARD;
            cmaprec.encoding    = FT_ENCODING_ADOBE_STANDARD;
            clazz               = &cff_cmap_encoding_class_rec;
          }
          else if ( encoding->offset == 1 )
          {
            cmaprec.encoding_id = TT_ADOBE_ID_EXPERT;
            cmaprec.encoding    = FT_ENCODING_ADOBE_EXPERT;
            clazz               = &cff_cmap_encoding_class_rec;
          }
          else
          {
            cmaprec.encoding_id = TT_ADOBE_ID_CUSTOM;
            cmaprec.encoding    = FT_ENCODING_ADOBE_CUSTOM;
            clazz               = &cff_cmap_encoding_class_rec;
          }

          FT_CMap_New( clazz, NULL, &cmaprec, NULL );
        }
      }
    }

  Exit:
    return error;

  Bad_Format:
    error = CFF_Err_Unknown_File_Format;
    goto Exit;
  }


  FT_LOCAL_DEF( void )
  cff_face_done( FT_Face  cffface )         /* CFF_Face */
  {
    CFF_Face      face   = (CFF_Face)cffface;
    FT_Memory     memory = cffface->memory;
    SFNT_Service  sfnt   = (SFNT_Service)face->sfnt;


    if ( sfnt )
      sfnt->done_face( face );

    {
      CFF_Font  cff = (CFF_Font)face->extra.data;


      if ( cff )
      {
        cff_font_done( cff );
        FT_FREE( face->extra.data );
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cff_driver_init( FT_Module  module )
  {
    FT_UNUSED( module );

    return CFF_Err_Ok;
  }


  FT_LOCAL_DEF( void )
  cff_driver_done( FT_Module  module )
  {
    FT_UNUSED( module );
  }


/* END */
