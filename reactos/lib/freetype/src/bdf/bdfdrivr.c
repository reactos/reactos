/*  bdfdrivr.c

    FreeType font driver for bdf files

    Copyright (C) 2001-2002 by
    Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <ft2build.h>

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "bdf.h"
#include "bdfdrivr.h"

#include "bdferror.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_bdfdriver


  typedef struct  BDF_CMapRec_
  {
    FT_CMapRec        cmap;
    FT_UInt           num_encodings;
    BDF_encoding_el*  encodings;

  } BDF_CMapRec, *BDF_CMap;


  FT_CALLBACK_DEF( FT_Error )
  bdf_cmap_init( BDF_CMap  cmap )
  {
    BDF_Face  face = (BDF_Face)FT_CMAP_FACE( cmap );


    cmap->num_encodings = face->bdffont->glyphs_used;
    cmap->encodings     = face->en_table;

    return FT_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  bdf_cmap_done( BDF_CMap  cmap )
  {
    cmap->encodings     = NULL;
    cmap->num_encodings = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  bdf_cmap_char_index( BDF_CMap   cmap,
                       FT_UInt32  charcode )
  {
    BDF_encoding_el*  encodings = cmap->encodings;
    FT_UInt           min, max, mid;
    FT_UInt           result = 0;


    min = 0;
    max = cmap->num_encodings;

    while ( min < max )
    {
      FT_UInt32  code;


      mid  = ( min + max ) >> 1;
      code = encodings[mid].enc;

      if ( charcode == code )
      {
        result = encodings[mid].glyph + 1;
        break;
      }

      if ( charcode < code )
        max = mid;
      else
        min = mid + 1;
    }

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  bdf_cmap_char_next( BDF_CMap    cmap,
                      FT_UInt32  *acharcode )
  {
    BDF_encoding_el*  encodings = cmap->encodings;
    FT_UInt           min, max, mid;
    FT_UInt32         charcode = *acharcode + 1;
    FT_UInt           result   = 0;


    min = 0;
    max = cmap->num_encodings;

    while ( min < max )
    {
      FT_UInt32  code;


      mid  = ( min + max ) >> 1;
      code = encodings[mid].enc;

      if ( charcode == code )
      {
        result = encodings[mid].glyph + 1;
        goto Exit;
      }

      if ( charcode < code )
        max = mid;
      else
        min = mid + 1;
    }

    charcode = 0;
    if ( min < cmap->num_encodings )
    {
      charcode = encodings[min].enc;
      result   = encodings[min].glyph + 1;
    }

  Exit:
    *acharcode = charcode;
    return result;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec  bdf_cmap_class =
  {
    sizeof( BDF_CMapRec ),
    (FT_CMap_InitFunc)     bdf_cmap_init,
    (FT_CMap_DoneFunc)     bdf_cmap_done,
    (FT_CMap_CharIndexFunc)bdf_cmap_char_index,
    (FT_CMap_CharNextFunc) bdf_cmap_char_next
  };




  FT_CALLBACK_DEF( FT_Error )
  BDF_Face_Done( BDF_Face  face )
  {
    FT_Memory  memory = FT_FACE_MEMORY( face );


    bdf_free_font( face->bdffont );

    FT_FREE( face->en_table );

    FT_FREE( face->charset_encoding );
    FT_FREE( face->charset_registry );
    FT_FREE( face->root.family_name );

    FT_FREE( face->root.available_sizes );

    FT_FREE( face->bdffont );

    FT_TRACE4(( "BDF_Face_Done: done face\n" ));

    return BDF_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  BDF_Face_Init( FT_Stream      stream,
                 BDF_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error       error  = BDF_Err_Ok;
    FT_Memory      memory = FT_FACE_MEMORY( face );

    bdf_font_t*    font;
    bdf_options_t  options;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );


    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    options.correct_metrics = 1;   /* FZ XXX: options semantics */
    options.keep_unencoded  = 1;
    options.keep_comments   = 0;
    options.font_spacing    = BDF_PROPORTIONAL;

    error = bdf_load_font( stream, memory, &options, &font );
    if ( error == BDF_Err_Missing_Startfont_Field )
    {
      FT_TRACE2(( "[not a valid BDF file]\n" ));
      goto Fail;
    }
    else if ( error )
      goto Exit;

    /* we have a bdf font: let's construct the face object */
    face->bdffont = font;
    {
      FT_Face          root = FT_FACE( face );
      bdf_property_t*  prop = NULL;


      FT_TRACE4(( "number of glyphs: %d (%d)\n",
                  font->glyphs_size,
                  font->glyphs_used ));
      FT_TRACE4(( "number of unencoded glyphs: %d (%d)\n",
                  font->unencoded_size,
                  font->unencoded_used ));

      root->num_faces  = 1;
      root->face_index = 0;
      root->face_flags = FT_FACE_FLAG_FIXED_SIZES |
                         FT_FACE_FLAG_HORIZONTAL  |
                         FT_FACE_FLAG_FAST_GLYPHS;

      prop = bdf_get_font_property( font, (char *)"SPACING" );
      if ( prop != NULL )
        if ( prop->format == BDF_ATOM )
          if ( prop->value.atom != NULL )
            if ( ( *(prop->value.atom) == 'M' ) ||
                 ( *(prop->value.atom) == 'C' ) )
              root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* FZ XXX: TO DO: FT_FACE_FLAGS_VERTICAL   */
      /* FZ XXX: I need a font to implement this */

      root->style_flags = 0;
      prop = bdf_get_font_property( font, (char *)"SLANT" );
      if ( prop != NULL )
        if ( prop->format == BDF_ATOM )
          if ( prop->value.atom != NULL )
            if ( ( *(prop->value.atom) == 'O' ) ||
                 ( *(prop->value.atom) == 'I' ) )
              root->style_flags |= FT_STYLE_FLAG_ITALIC;

      prop = bdf_get_font_property( font, (char *)"WEIGHT_NAME" );
      if ( prop != NULL )
        if ( prop->format == BDF_ATOM )
          if ( prop->value.atom != NULL )
            if ( *(prop->value.atom) == 'B' )
              root->style_flags |= FT_STYLE_FLAG_BOLD;

      prop = bdf_get_font_property( font, (char *)"FAMILY_NAME" );
      if ( ( prop != NULL ) && ( prop->value.atom != NULL ) )
      {
        int  l = ft_strlen( prop->value.atom ) + 1;


        if ( FT_NEW_ARRAY( root->family_name, l ) )
          goto Exit;
        ft_strcpy( root->family_name, prop->value.atom );
      }
      else
        root->family_name = 0;

      root->style_name = (char *)"Regular";
      if ( root->style_flags & FT_STYLE_FLAG_BOLD )
      {
        if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
          root->style_name = (char *)"Bold Italic";
        else
          root->style_name = (char *)"Bold";
      }
      else if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
        root->style_name = (char *)"Italic";

      root->num_glyphs = font->glyphs_size;     /* unencoded included */

      root->num_fixed_sizes = 1;
      if ( FT_NEW_ARRAY( root->available_sizes, 1 ) )
        goto Exit;

      prop = bdf_get_font_property( font, (char *)"AVERAGE_WIDTH" );
      if ( ( prop != NULL ) && ( prop->value.int32 >= 10 ) )
        root->available_sizes->width = (short)( prop->value.int32 / 10 );

      prop = bdf_get_font_property( font, (char *)"PIXEL_SIZE" );
      if ( prop != NULL )
        root->available_sizes->height = (short) prop->value.int32;
      else
      {
        prop = bdf_get_font_property( font, (char *)"POINT_SIZE" );
        if ( prop != NULL )
        {
          bdf_property_t  *yres;


          yres = bdf_get_font_property( font, (char *)"RESOLUTION_Y" );
          if ( yres != NULL )
          {
            FT_TRACE4(( "POINT_SIZE: %d  RESOLUTION_Y: %d\n",
                        prop->value.int32, yres->value.int32 ));
            root->available_sizes->height =
              (FT_Short)( prop->value.int32 * yres->value.int32 / 720 );
          }
        }
      }

      if ( root->available_sizes->width == 0 )
      {
        if ( root->available_sizes->height == 0 )
        {
          /* some fonts have broken SIZE declaration (jiskan24.bdf) */
          FT_ERROR(( "BDF_Face_Init: reading size\n" ));
          root->available_sizes->width = (FT_Short)font->point_size;
        }
        else
          root->available_sizes->width = root->available_sizes->height;
      }
      if ( root->available_sizes->height == 0 )
          root->available_sizes->height = root->available_sizes->width;

      /* encoding table */
      {
        bdf_glyph_t*   cur = font->glyphs;
        unsigned long  n;


        if ( FT_NEW_ARRAY( face->en_table, font->glyphs_size ) )
          goto Exit;

        for ( n = 0; n < font->glyphs_size; n++ )
        {
          (face->en_table[n]).enc = cur[n].encoding;
          FT_TRACE4(( "idx %d, val 0x%lX\n", n, cur[n].encoding ));
          (face->en_table[n]).glyph = (FT_Short)n;
        }
      }

      /* charmaps */
      {
        bdf_property_t  *charset_registry = 0, *charset_encoding = 0;
        FT_Bool          unicode_charmap  = 0;


        charset_registry =
          bdf_get_font_property( font, (char *)"CHARSET_REGISTRY" );
        charset_encoding =
          bdf_get_font_property( font, (char *)"CHARSET_ENCODING" );
        if ( ( charset_registry != NULL ) && ( charset_encoding != NULL ) )
        {
          if ( ( charset_registry->format == BDF_ATOM ) &&
               ( charset_encoding->format == BDF_ATOM ) &&
               ( charset_registry->value.atom != NULL ) &&
               ( charset_encoding->value.atom != NULL ) )
          {
            if ( FT_NEW_ARRAY( face->charset_encoding,
                               strlen( charset_encoding->value.atom ) + 1 ) )
              goto Exit;
            if ( FT_NEW_ARRAY( face->charset_registry,
                               strlen( charset_registry->value.atom ) + 1 ) )
              goto Exit;
            ft_strcpy( face->charset_registry, charset_registry->value.atom );
            ft_strcpy( face->charset_encoding, charset_encoding->value.atom );
            if ( !ft_strcmp( face->charset_registry, "ISO10646" )     ||
                 ( !ft_strcmp( face->charset_registry, "ISO8859" ) &&
                   !ft_strcmp( face->charset_encoding, "1" )       )  )
              unicode_charmap = 1;

            {
              FT_CharMapRec  charmap;


              charmap.face        = FT_FACE( face );
              charmap.encoding    = FT_ENCODING_NONE;
              charmap.platform_id = 0;
              charmap.encoding_id = 0;

              if ( unicode_charmap )
              {
                charmap.encoding    = FT_ENCODING_UNICODE;
                charmap.platform_id = 3;
                charmap.encoding_id = 1;
              }

              error = FT_CMap_New( &bdf_cmap_class, NULL, &charmap, NULL );

#if 0
              /* Select default charmap */
              if (root->num_charmaps)
                root->charmap = root->charmaps[0];
#endif
            }

            goto Exit;
          }
        }

        /* otherwise assume Adobe standard encoding */

        {
          FT_CharMapRec  charmap;


          charmap.face        = FT_FACE( face );
          charmap.encoding    = FT_ENCODING_ADOBE_STANDARD;
          charmap.platform_id = 7;
          charmap.encoding_id = 0;

          error = FT_CMap_New( &bdf_cmap_class, NULL, &charmap, NULL );

          /* Select default charmap */
          if (root->num_charmaps)
            root->charmap = root->charmaps[0];
        }
      }
    }

  Exit:
    return error;

  Fail:
    BDF_Face_Done( face );
    return BDF_Err_Unknown_File_Format;
  }


  static FT_Error
  BDF_Set_Pixel_Size( FT_Size  size )
  {
    BDF_Face  face = (BDF_Face)FT_SIZE_FACE( size );
    FT_Face   root = FT_FACE( face );


    FT_TRACE4(( "rec %d - pres %d\n",
                size->metrics.y_ppem, root->available_sizes->height ));

    if ( size->metrics.y_ppem == root->available_sizes->height )
    {
      size->metrics.ascender  = face->bdffont->bbx.ascent << 6;
      size->metrics.descender = face->bdffont->bbx.descent * ( -64 );
      size->metrics.height    = face->bdffont->bbx.height << 6;

      return BDF_Err_Ok;
    }
    else
      return BDF_Err_Invalid_Pixel_Size;
  }


  static FT_Error
  BDF_Glyph_Load( FT_GlyphSlot  slot,
                  FT_Size       size,
                  FT_UInt       glyph_index,
                  FT_Int32      load_flags )
  {
    BDF_Face        face   = (BDF_Face)FT_SIZE_FACE( size );
    FT_Error        error  = BDF_Err_Ok;
    FT_Bitmap*      bitmap = &slot->bitmap;
    bdf_glyph_t     glyph;
    int             bpp    = face->bdffont->bpp;
    int             i, j, count;
    unsigned char   *p, *pp;

    FT_Memory       memory = face->bdffont->memory;

    FT_UNUSED( load_flags );


    if ( !face )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    if ( glyph_index > 0 )
      glyph_index--;

    /* slot, bitmap => freetype, glyph => bdflib */
    glyph = face->bdffont->glyphs[glyph_index];

    bitmap->rows  = glyph.bbx.height;
    bitmap->width = glyph.bbx.width;

    if ( bpp == 1 )
    {
      bitmap->pixel_mode = FT_PIXEL_MODE_MONO;
      bitmap->pitch      = glyph.bpr;

      if ( FT_NEW_ARRAY( bitmap->buffer, glyph.bytes ) )
        goto Exit;
      FT_MEM_COPY( bitmap->buffer, glyph.bitmap, glyph.bytes );
    }
    else
    {
      /* blow up pixmap to have 8 bits per pixel */
      bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
      bitmap->pitch      = bitmap->width;

      if ( FT_NEW_ARRAY( bitmap->buffer, bitmap->rows * bitmap->pitch ) )
        goto Exit;

      switch ( bpp )
      {
      case 2:
        bitmap->num_grays = 4;

        count = 0;
        p     = glyph.bitmap;

        for ( i = 0; i < bitmap->rows; i++ )
        {
          pp = p;

          /* get the full bytes */
          for ( j = 0; j < ( bitmap->width >> 2 ); j++ )
          {
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0xC0 ) >> 6 );
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0x30 ) >> 4 );
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0x0C ) >> 2 );
            bitmap->buffer[count++] = (FT_Byte)(   *pp & 0x03 );

            pp++;
          }

          /* get remaining pixels (if any) */
          switch ( bitmap->width & 3 )
          {
          case 3:
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0xC0 ) >> 6 );
            /* fall through */
          case 2:
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0x30 ) >> 4 );
            /* fall through */
          case 1:
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0x0C ) >> 2 );
            /* fall through */
          case 0:
            break;
          }

          p += glyph.bpr;
        }
        break;

      case 4:
        bitmap->num_grays = 16;

        count = 0;
        p     = glyph.bitmap;

        for ( i = 0; i < bitmap->rows; i++ )
        {
          pp = p;

          /* get the full bytes */
          for ( j = 0; j < ( bitmap->width >> 1 ); j++ )
          {
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0xF0 ) >> 4 );
            bitmap->buffer[count++] = (FT_Byte)(   *pp & 0x0F );

            pp++;
          }

          /* get remaining pixel (if any) */
          switch ( bitmap->width & 1 )
          {
          case 1:
            bitmap->buffer[count++] = (FT_Byte)( ( *pp & 0xF0 ) >> 4 );
            /* fall through */
          case 0:
            break;
          }

          p += glyph.bpr;
        }
        break;

      case 8:
        bitmap->num_grays = 256;

        FT_MEM_COPY( bitmap->buffer, glyph.bitmap,
                     bitmap->rows * bitmap->pitch );
        break;
      }
    }

    slot->bitmap_left = 0;
    slot->bitmap_top  = glyph.bbx.ascent;

    /* FZ XXX: TODO: vertical metrics */
    slot->metrics.horiAdvance  = glyph.dwidth << 6;
    slot->metrics.horiBearingX = glyph.bbx.x_offset << 6;
    slot->metrics.horiBearingY = ( glyph.bbx.y_offset +
                                   glyph.bbx.height ) << 6;
    slot->metrics.width        = bitmap->width << 6;
    slot->metrics.height       = bitmap->rows << 6;

    slot->linearHoriAdvance = (FT_Fixed)glyph.dwidth << 16;
    slot->format            = FT_GLYPH_FORMAT_BITMAP;
    slot->flags             = FT_GLYPH_OWN_BITMAP;

  Exit:
    return error;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  bdf_driver_class =
  {
    {
      ft_module_font_driver,
      sizeof ( FT_DriverRec ),

      "bdf",
      0x10000L,
      0x20000L,

      0,

      (FT_Module_Constructor)0,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    sizeof ( BDF_FaceRec ),
    sizeof ( FT_SizeRec ),
    sizeof ( FT_GlyphSlotRec ),

    (FT_Face_InitFunc)        BDF_Face_Init,
    (FT_Face_DoneFunc)        BDF_Face_Done,
    (FT_Size_InitFunc)        0,
    (FT_Size_DoneFunc)        0,
    (FT_Slot_InitFunc)        0,
    (FT_Slot_DoneFunc)        0,

    (FT_Size_ResetPointsFunc) BDF_Set_Pixel_Size,
    (FT_Size_ResetPixelsFunc) BDF_Set_Pixel_Size,

    (FT_Slot_LoadFunc)        BDF_Glyph_Load,

    (FT_Face_GetKerningFunc)  0,
    (FT_Face_AttachFunc)      0,
    (FT_Face_GetAdvancesFunc) 0
  };


/* END */
