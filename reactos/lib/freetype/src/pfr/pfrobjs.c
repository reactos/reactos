/***************************************************************************/
/*                                                                         */
/*  pfrobjs.c                                                              */
/*                                                                         */
/*    FreeType PFR object methods (body).                                  */
/*                                                                         */
/*  Copyright 2002 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "pfrobjs.h"
#include "pfrload.h"
#include "pfrgload.h"
#include "pfrcmap.h"
#include "pfrsbit.h"
#include FT_OUTLINE_H
#include FT_INTERNAL_DEBUG_H

#include "pfrerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pfr


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     FACE OBJECT METHODS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  pfr_face_done( PFR_Face  face )
  {
    /* finalize the physical font record */
    pfr_phy_font_done( &face->phy_font, FT_FACE_MEMORY( face ) );

    /* no need to finalize the logical font or the header */
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_face_init( FT_Stream      stream,
                 PFR_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error  error;

    FT_UNUSED( num_params );
    FT_UNUSED( params );


    /* load the header and check it */
    error = pfr_header_load( &face->header, stream );
    if ( error )
      goto Exit;

    if ( !pfr_header_check( &face->header ) )
    {
      FT_TRACE4(( "pfr_face_init: not a valid PFR font\n" ));
      error = PFR_Err_Unknown_File_Format;
      goto Exit;
    }

    /* check face index */
    {
      FT_UInt  num_faces;


      error = pfr_log_font_count( stream,
                                  face->header.log_dir_offset,
                                  &num_faces );
      if ( error )
        goto Exit;

      face->root.num_faces = num_faces;
    }

    if ( face_index < 0 )
      goto Exit;

    if ( face_index >= face->root.num_faces )
    {
      FT_ERROR(( "pfr_face_init: invalid face index\n" ));
      error = PFR_Err_Invalid_Argument;
      goto Exit;
    }

    /* load the face */
    error = pfr_log_font_load(
               &face->log_font, stream, face_index,
               face->header.log_dir_offset,
               FT_BOOL( face->header.phy_font_max_size_high != 0 ) );
    if ( error )
      goto Exit;

    /* now load the physical font descriptor */
     error = pfr_phy_font_load( &face->phy_font, stream,
                                 face->log_font.phys_offset,
                                 face->log_font.phys_size );
     if ( error )
       goto Exit;

     /* now, set-up all root face fields */
     {
       FT_Face      root     = FT_FACE( face );
       PFR_PhyFont  phy_font = &face->phy_font;


       root->face_index = face_index;
       root->num_glyphs = phy_font->num_chars;
       root->face_flags = FT_FACE_FLAG_SCALABLE;

       if ( (phy_font->flags & PFR_PHY_PROPORTIONAL) == 0 )
         root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

       if ( phy_font->flags & PFR_PHY_VERTICAL )
         root->face_flags |= FT_FACE_FLAG_VERTICAL;
       else
         root->face_flags |= FT_FACE_FLAG_HORIZONTAL;

       if ( phy_font->num_strikes > 0 )
         root->face_flags |= FT_FACE_FLAG_FIXED_SIZES;

       if ( phy_font->num_kern_pairs > 0 )
         root->face_flags |= FT_FACE_FLAG_KERNING;

       root->family_name = phy_font->font_id;
       root->style_name  = NULL;  /* no style name in font file */

       root->num_fixed_sizes = 0;
       root->available_sizes = 0;

       root->bbox         = phy_font->bbox;
       root->units_per_EM = (FT_UShort)phy_font->outline_resolution;
       root->ascender     = (FT_Short) phy_font->bbox.yMax;
       root->descender    = (FT_Short) phy_font->bbox.yMin;
       root->height       = (FT_Short)
                              ( ( ( root->ascender - root->descender ) * 12 )
                                / 10 );

       /* now compute maximum advance width */
       if ( ( phy_font->flags & PFR_PHY_PROPORTIONAL ) == 0 )
         root->max_advance_width = (FT_Short)phy_font->standard_advance;
       else
       {
         FT_Int    max = 0;
         FT_UInt   count = phy_font->num_chars;
         PFR_Char  gchar = phy_font->chars;


         for ( ; count > 0; count--, gchar++ )
         {
           if ( max < gchar->advance )
             max = gchar->advance;
         }

         root->max_advance_width = (FT_Short)max;
       }

       root->max_advance_height = root->height;

       root->underline_position  = (FT_Short)( - root->units_per_EM / 10 );
       root->underline_thickness = (FT_Short)(   root->units_per_EM / 30 );

       /* create charmap */
       {
         FT_CharMapRec  charmap;


         charmap.face        = root;
         charmap.platform_id = 3;
         charmap.encoding_id = 1;
         charmap.encoding    = FT_ENCODING_UNICODE;

         FT_CMap_New( &pfr_cmap_class_rec, NULL, &charmap, NULL );

#if 0
         /* Select default charmap */
         if (root->num_charmaps)
           root->charmap = root->charmaps[0];
#endif
       }

       /* check whether we've loaded any kerning pairs */
       if ( phy_font->num_kern_pairs )
         root->face_flags |= FT_FACE_FLAG_KERNING;
     }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    SLOT OBJECT METHOD                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( FT_Error )
  pfr_slot_init( PFR_Slot  slot )
  {
    FT_GlyphLoader  loader = slot->root.internal->loader;

    pfr_glyph_init( &slot->glyph, loader );

    return 0;
  }


  FT_LOCAL_DEF( void )
  pfr_slot_done( PFR_Slot  slot )
  {
    pfr_glyph_done( &slot->glyph );
  }


  FT_LOCAL_DEF( FT_Error )
  pfr_slot_load( PFR_Slot  slot,
                 PFR_Size  size,
                 FT_UInt   gindex,
                 FT_Int32  load_flags )
  {
    FT_Error     error;
    PFR_Face     face    = (PFR_Face)slot->root.face;
    PFR_Char     gchar;
    FT_Outline*  outline = &slot->root.outline;
    FT_ULong     gps_offset;

    if (gindex > 0)
      gindex--;

    /* check that the glyph index is correct */
    FT_ASSERT( gindex < face->phy_font.num_chars );

    /* try to load an embedded bitmap */
    if ( ( load_flags & ( FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) ) == 0 )
    {
      error = pfr_slot_load_bitmap( slot, size, gindex );
      if ( error == 0 )
        goto Exit;
    }

    gchar               = face->phy_font.chars + gindex;
    slot->root.format   = FT_GLYPH_FORMAT_OUTLINE;
    outline->n_points   = 0;
    outline->n_contours = 0;
    gps_offset          = face->header.gps_section_offset;

    /* load the glyph outline (FT_LOAD_NO_RECURSE isn't supported) */
    error = pfr_glyph_load( &slot->glyph, face->root.stream,
                            gps_offset, gchar->gps_offset, gchar->gps_size );

    if ( !error )
    {
      FT_BBox            cbox;
      FT_Glyph_Metrics*  metrics = &slot->root.metrics;
      FT_Pos             advance;
      FT_Int             em_metrics, em_outline;
      FT_Bool            scaling;


      scaling = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE ) == 0 );

      /* copy outline data */
      *outline = slot->glyph.loader->base.outline;

      outline->flags &= ~FT_OUTLINE_OWNER;
      outline->flags |= FT_OUTLINE_REVERSE_FILL;

      if ( size && size->root.metrics.y_ppem < 24 )
        outline->flags |= FT_OUTLINE_HIGH_PRECISION;

      /* compute the advance vector */
      metrics->horiAdvance = 0;
      metrics->vertAdvance = 0;

      advance    = gchar->advance;
      em_metrics = face->phy_font.metrics_resolution;
      em_outline = face->phy_font.outline_resolution;

      if ( em_metrics != em_outline )
        advance = FT_MulDiv( advance, em_outline, em_metrics );

      if ( face->phy_font.flags & PFR_PHY_VERTICAL )
        metrics->vertAdvance = advance;
      else
        metrics->horiAdvance = advance;

      slot->root.linearHoriAdvance = metrics->horiAdvance;
      slot->root.linearVertAdvance = metrics->vertAdvance;

      /* make-up vertical metrics(?) */
      metrics->vertBearingX = 0;
      metrics->vertBearingY = 0;

      /* scale when needed */
      if ( scaling )
      {
        FT_Int      n;
        FT_Fixed    x_scale = size->root.metrics.x_scale;
        FT_Fixed    y_scale = size->root.metrics.y_scale;
        FT_Vector*  vec     = outline->points;


        /* scale outline points */
        for ( n = 0; n < outline->n_points; n++, vec++ )
        {
          vec->x = FT_MulFix( vec->x, x_scale );
          vec->y = FT_MulFix( vec->y, y_scale );
        }

        /* scale the advance */
        metrics->horiAdvance = FT_MulFix( metrics->horiAdvance, x_scale );
        metrics->vertAdvance = FT_MulFix( metrics->vertAdvance, y_scale );
      }

      /* compute the rest of the metrics */
      FT_Outline_Get_CBox( outline, &cbox );

      metrics->width        = cbox.xMax - cbox.xMin;
      metrics->height       = cbox.yMax - cbox.yMin;
      metrics->horiBearingX = cbox.xMin;
      metrics->horiBearingY = cbox.yMax - metrics->height;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      KERNING METHOD                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( FT_Error )
  pfr_face_get_kerning( PFR_Face    face,
                        FT_UInt     glyph1,
                        FT_UInt     glyph2,
                        FT_Vector*  kerning )
  {
    FT_Error      error;
    PFR_PhyFont   phy_font = &face->phy_font;
    PFR_KernItem  item     = phy_font->kern_items;
    FT_UInt32     idx      = PFR_KERN_INDEX( glyph1, glyph2 );


    kerning->x = 0;
    kerning->y = 0;

    /* find the kerning item containing our pair */
    while ( item )
    {
      if ( item->pair1 <= idx && idx <= item->pair2 )
        goto Found_Item;

      item = item->next;
    }

    /* not found */
    goto Exit;

  Found_Item:
    {
      /* perform simply binary search within the item */
      FT_UInt    min, mid, max;
      FT_Stream  stream = face->root.stream;
      FT_Byte*   p;


      if ( FT_STREAM_SEEK( item->offset )                       ||
           FT_FRAME_ENTER( item->pair_count * item->pair_size ) )
        goto Exit;

      min = 0;
      max = item->pair_count;
      while ( min < max )
      {
        FT_UInt  char1, char2, charcode;


        mid = ( min + max ) >> 1;
        p   = stream->cursor + mid*item->pair_size;

        if ( item->flags & PFR_KERN_2BYTE_CHAR )
        {
          char1 = FT_NEXT_USHORT( p );
          char2 = FT_NEXT_USHORT( p );
        }
        else
        {
          char1 = FT_NEXT_USHORT( p );
          char2 = FT_NEXT_USHORT( p );
        }
        charcode = PFR_KERN_INDEX( char1, char2 );

        if ( idx == charcode )
        {
          if ( item->flags & PFR_KERN_2BYTE_ADJ )
            kerning->x = item->base_adj + FT_NEXT_SHORT( p );
          else
            kerning->x = item->base_adj + FT_NEXT_CHAR( p );

          break;
        }
        if ( idx > charcode )
          min = mid + 1;
        else
          max = mid;
      }

      FT_FRAME_EXIT();
    }

  Exit:
    return 0;
  }

/* END */
