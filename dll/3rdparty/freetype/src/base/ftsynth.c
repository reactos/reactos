/***************************************************************************/
/*                                                                         */
/*  ftsynth.c                                                              */
/*                                                                         */
/*    FreeType synthesizing code for emboldening and slanting (body).      */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005 by                         */
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
#include FT_SYNTHESIS_H
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include FT_BITMAP_H


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL OBLIQUING SUPPORT                                ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  /* documentation is in ftsynth.h */

  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Oblique( FT_GlyphSlot  slot )
  {
    FT_Matrix    transform;
    FT_Outline*  outline = &slot->outline;


    /* only oblique outline glyphs */
    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
      return;

    /* we don't touch the advance width */

    /* For italic, simply apply a shear transform, with an angle */
    /* of about 12 degrees.                                      */

    transform.xx = 0x10000L;
    transform.yx = 0x00000L;

    transform.xy = 0x06000L;
    transform.yy = 0x10000L;

    FT_Outline_Transform( outline, &transform );
  }


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL EMBOLDENING/OUTLINING SUPPORT                    ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftsynth.h */

  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Embolden( FT_GlyphSlot  slot )
  {
    FT_Library  library = slot->library;
    FT_Face     face    = FT_SLOT_FACE( slot );
    FT_Error    error   = FT_Err_Ok;
    FT_Pos      xstr, ystr;


    /* some reasonable strength */
    xstr = FT_MulFix( face->units_per_EM,
                      face->size->metrics.y_scale ) / 42;
    ystr = xstr;

    if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
    {
      error = FT_Outline_Embolden( &slot->outline, xstr );
      xstr = xstr * 4;  /* according to the documentation */
      ystr = xstr;
    }
    else if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
    {
      xstr = FT_PIX_FLOOR( xstr );
      if ( xstr == 0 )
        xstr = 1 << 6;
      ystr = FT_PIX_FLOOR( ystr );

      /* slot must be bitmap-owner */
      if ( !( slot->internal->flags & FT_GLYPH_OWN_BITMAP ) )
      {
        FT_Bitmap  bitmap;


        FT_Bitmap_New( &bitmap );
        error = FT_Bitmap_Copy( library, &slot->bitmap, &bitmap );

        if ( !error )
        {
          slot->bitmap = bitmap;
          slot->internal->flags |= FT_GLYPH_OWN_BITMAP;
        }
      }

      if ( !error )
        error = FT_Bitmap_Embolden( library, &slot->bitmap, xstr, ystr );
    }
    else
      error = FT_Err_Invalid_Argument;

    /* modify the metrics accordingly */
    if ( !error )
    {
      slot->advance.x += xstr;
      slot->advance.y += ystr;

      slot->metrics.width        += xstr;
      slot->metrics.height       += ystr;
      slot->metrics.horiBearingY += ystr;
      slot->metrics.horiAdvance  += xstr;
      slot->metrics.vertBearingX -= xstr / 2;
      slot->metrics.vertBearingY += ystr;
      slot->metrics.vertAdvance  += ystr;

      if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
        slot->bitmap_top += ystr >> 6;
    }
  }


/* END */
