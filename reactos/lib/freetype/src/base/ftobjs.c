/***************************************************************************/
/*                                                                         */
/*  ftobjs.c                                                               */
/*                                                                         */
/*    The FreeType private base classes (body).                            */
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
#include FT_LIST_H
#include FT_OUTLINE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H    /* for SFNT_Load_Table_Func */
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_IDS_H
#include FT_OUTLINE_H


  FT_BASE_DEF( void )
  ft_validator_init( FT_Validator        valid,
                     const FT_Byte*      base,
                     const FT_Byte*      limit,
                     FT_ValidationLevel  level )
  {
    valid->base  = base;
    valid->limit = limit;
    valid->level = level;
    valid->error = 0;
  }


  FT_BASE_DEF( FT_Int )
  ft_validator_run( FT_Validator  valid )
  {
    int  result;


    result = ft_setjmp( valid->jump_buffer );
    return result;
  }


  FT_BASE_DEF( void )
  ft_validator_error( FT_Validator  valid,
                      FT_Error      error )
  {
    valid->error = error;
    ft_longjmp( valid->jump_buffer, 1 );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           S T R E A M                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* create a new input stream from a FT_Open_Args structure */
  /*                                                         */
  static FT_Error
  ft_input_stream_new( FT_Library           library,
                       const FT_Open_Args*  args,
                       FT_Stream*           astream )
  {
    FT_Error   error;
    FT_Memory  memory;
    FT_Stream  stream;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !args )
      return FT_Err_Invalid_Argument;

    *astream = 0;
    memory   = library->memory;

    if ( FT_NEW( stream ) )
      goto Exit;

    stream->memory = memory;

    if ( args->flags & FT_OPEN_MEMORY )
    {
      /* create a memory-based stream */
      FT_Stream_OpenMemory( stream,
                            (const FT_Byte*)args->memory_base,
                            args->memory_size );
    }
    else if ( args->flags & FT_OPEN_PATHNAME )
    {
      /* create a normal system stream */
      error = FT_Stream_Open( stream, args->pathname );
      stream->pathname.pointer = args->pathname;
    }
    else if ( ( args->flags & FT_OPEN_STREAM ) && args->stream )
    {
      /* use an existing, user-provided stream */

      /* in this case, we do not need to allocate a new stream object */
      /* since the caller is responsible for closing it himself       */
      FT_FREE( stream );
      stream = args->stream;
    }
    else
      error = FT_Err_Invalid_Argument;

    if ( error )
      FT_FREE( stream );
    else
      stream->memory = memory;  /* just to be certain */

    *astream = stream;

  Exit:
    return error;
  }


  static void
  ft_input_stream_free( FT_Stream  stream,
                        FT_Int     external )
  {
    if ( stream )
    {
      FT_Memory  memory = stream->memory;


      FT_Stream_Close( stream );

      if ( !external )
        FT_FREE( stream );
    }
  }


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_objs


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****               FACE, SIZE & GLYPH SLOT OBJECTS                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  ft_glyphslot_init( FT_GlyphSlot  slot )
  {
    FT_Driver         driver = slot->face->driver;
    FT_Driver_Class   clazz  = driver->clazz;
    FT_Memory         memory = driver->root.memory;
    FT_Error          error  = FT_Err_Ok;
    FT_Slot_Internal  internal;


    slot->library = driver->root.library;

    if ( FT_NEW( internal ) )
      goto Exit;

    slot->internal = internal;

    if ( FT_DRIVER_USES_OUTLINES( driver ) )
      error = FT_GlyphLoader_New( memory, &internal->loader );

    if ( !error && clazz->init_slot )
      error = clazz->init_slot( slot );

  Exit:
    return error;
  }


  FT_BASE_DEF( void )
  ft_glyphslot_free_bitmap( FT_GlyphSlot  slot )
  {
    if ( slot->internal->flags & FT_GLYPH_OWN_BITMAP )
    {
      FT_Memory  memory = FT_FACE_MEMORY( slot->face );


      FT_FREE( slot->bitmap.buffer );
      slot->internal->flags &= ~FT_GLYPH_OWN_BITMAP;
    }
    else
    {
      /* assume that the bitmap buffer was stolen or not */
      /* allocated from the heap                         */
      slot->bitmap.buffer = NULL;
    }
  }


  FT_BASE_DEF( void )
  ft_glyphslot_set_bitmap( FT_GlyphSlot  slot,
                           FT_Byte*      buffer )
  {
    ft_glyphslot_free_bitmap( slot );

    slot->bitmap.buffer = buffer;

    FT_ASSERT( (slot->internal->flags & FT_GLYPH_OWN_BITMAP) == 0 );
  }


  FT_BASE_DEF( FT_Error )
  ft_glyphslot_alloc_bitmap( FT_GlyphSlot  slot,
                             FT_ULong      size )
  {
    FT_Memory  memory = FT_FACE_MEMORY( slot->face );


    if ( slot->internal->flags & FT_GLYPH_OWN_BITMAP )
      FT_FREE( slot->bitmap.buffer );
    else
      slot->internal->flags |= FT_GLYPH_OWN_BITMAP;

    return FT_MEM_ALLOC( slot->bitmap.buffer, size );
  }


  static void
  ft_glyphslot_clear( FT_GlyphSlot  slot )
  {
    /* free bitmap if needed */
    ft_glyphslot_free_bitmap( slot );

    /* clear all public fields in the glyph slot */
    FT_ZERO( &slot->metrics );
    FT_ZERO( &slot->outline );

    slot->bitmap.width = 0;
    slot->bitmap.rows  = 0;
    slot->bitmap.pitch = 0;
    slot->bitmap.pixel_mode = 0;
    /* don't touch 'slot->bitmap.buffer'! */

    slot->bitmap_left   = 0;
    slot->bitmap_top    = 0;
    slot->num_subglyphs = 0;
    slot->subglyphs     = 0;
    slot->control_data  = 0;
    slot->control_len   = 0;
    slot->other         = 0;
    slot->format        = FT_GLYPH_FORMAT_NONE;

    slot->linearHoriAdvance = 0;
    slot->linearVertAdvance = 0;
  }


  static void
  ft_glyphslot_done( FT_GlyphSlot  slot )
  {
    FT_Driver         driver = slot->face->driver;
    FT_Driver_Class   clazz  = driver->clazz;
    FT_Memory         memory = driver->root.memory;


    if ( clazz->done_slot )
      clazz->done_slot( slot );

    /* free bitmap buffer if needed */
    ft_glyphslot_free_bitmap( slot );

    /* free glyph loader */
    if ( FT_DRIVER_USES_OUTLINES( driver ) )
    {
      FT_GlyphLoader_Done( slot->internal->loader );
      slot->internal->loader = 0;
    }

    FT_FREE( slot->internal );
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( FT_Error )
  FT_New_GlyphSlot( FT_Face        face,
                    FT_GlyphSlot  *aslot )
  {
    FT_Error          error;
    FT_Driver         driver;
    FT_Driver_Class   clazz;
    FT_Memory         memory;
    FT_GlyphSlot      slot;


    if ( !face || !aslot || !face->driver )
      return FT_Err_Invalid_Argument;

    *aslot = 0;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = driver->root.memory;

    FT_TRACE4(( "FT_New_GlyphSlot: Creating new slot object\n" ));
    if ( !FT_ALLOC( slot, clazz->slot_object_size ) )
    {
      slot->face = face;

      error = ft_glyphslot_init( slot );
      if ( error )
      {
        ft_glyphslot_done( slot );
        FT_FREE( slot );
        goto Exit;
      }

      *aslot = slot;
    }

  Exit:
    FT_TRACE4(( "FT_New_GlyphSlot: Return %d\n", error ));
    return error;
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( void )
  FT_Done_GlyphSlot( FT_GlyphSlot  slot )
  {
    if ( slot )
    {
      FT_Driver      driver = slot->face->driver;
      FT_Memory      memory = driver->root.memory;
      FT_GlyphSlot*  parent;
      FT_GlyphSlot   cur;


      /* Remove slot from its parent face's list */
      parent = &slot->face->glyph;
      cur    = *parent;

      while ( cur )
      {
        if ( cur == slot )
        {
          *parent = cur->next;
          ft_glyphslot_done( slot );
          FT_FREE( slot );
          break;
        }
        cur = cur->next;
      }
    }
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( void )
  FT_Set_Transform( FT_Face     face,
                    FT_Matrix*  matrix,
                    FT_Vector*  delta )
  {
    FT_Face_Internal  internal;


    if ( !face )
      return;

    internal = face->internal;

    internal->transform_flags = 0;

    if ( !matrix )
    {
      internal->transform_matrix.xx = 0x10000L;
      internal->transform_matrix.xy = 0;
      internal->transform_matrix.yx = 0;
      internal->transform_matrix.yy = 0x10000L;
      matrix = &internal->transform_matrix;
    }
    else
      internal->transform_matrix = *matrix;

    /* set transform_flags bit flag 0 if `matrix' isn't the identity */
    if ( ( matrix->xy | matrix->yx ) ||
         matrix->xx != 0x10000L      ||
         matrix->yy != 0x10000L      )
      internal->transform_flags |= 1;

    if ( !delta )
    {
      internal->transform_delta.x = 0;
      internal->transform_delta.y = 0;
      delta = &internal->transform_delta;
    }
    else
      internal->transform_delta = *delta;

    /* set transform_flags bit flag 1 if `delta' isn't the null vector */
    if ( delta->x | delta->y )
      internal->transform_flags |= 2;
  }


  static FT_Renderer
  ft_lookup_glyph_renderer( FT_GlyphSlot  slot );


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Load_Glyph( FT_Face   face,
                 FT_UInt   glyph_index,
                 FT_Int32  load_flags )
  {
    FT_Error      error;
    FT_Driver     driver;
    FT_GlyphSlot  slot;
    FT_Library    library;
    FT_Bool       autohint;
    FT_Module     hinter;


    if ( !face || !face->size || !face->glyph )
      return FT_Err_Invalid_Face_Handle;

    if ( glyph_index >= (FT_UInt)face->num_glyphs )
      return FT_Err_Invalid_Argument;

    slot = face->glyph;
    ft_glyphslot_clear( slot );

    driver = face->driver;

    /* if the flag NO_RECURSE is set, we disable hinting and scaling */
    if ( load_flags & FT_LOAD_NO_RECURSE )
    {
      /* disable scaling, hinting, and transformation */
      load_flags |= FT_LOAD_NO_SCALE         |
                    FT_LOAD_NO_HINTING       |
                    FT_LOAD_NO_BITMAP        |
                    FT_LOAD_IGNORE_TRANSFORM;

      /* disable bitmap rendering */
      load_flags &= ~FT_LOAD_RENDER;
    }

    /* do we need to load the glyph through the auto-hinter? */
    library  = driver->root.library;
    hinter   = library->auto_hinter;
    autohint =
      FT_BOOL( hinter                                      &&
               !( load_flags & ( FT_LOAD_NO_SCALE    |
                                 FT_LOAD_NO_HINTING  |
                                 FT_LOAD_NO_AUTOHINT ) )   &&
               FT_DRIVER_IS_SCALABLE( driver )             &&
               FT_DRIVER_USES_OUTLINES( driver )           );
    if ( autohint )
    {
      if ( FT_DRIVER_HAS_HINTER( driver ) &&
           !( load_flags & FT_LOAD_FORCE_AUTOHINT ) )
        autohint = 0;
    }

    if ( autohint )
    {
      FT_AutoHinter_Service  hinting;


      /* try to load embedded bitmaps first if available            */
      /*                                                            */
      /* XXX: This is really a temporary hack that should disappear */
      /*      promptly with FreeType 2.1!                           */
      /*                                                            */
      if ( FT_HAS_FIXED_SIZES( face )             &&
          ( load_flags & FT_LOAD_NO_BITMAP ) == 0 )
      {
        error = driver->clazz->load_glyph( slot, face->size,
                                           glyph_index,
                                           load_flags | FT_LOAD_SBITS_ONLY );

        if ( !error && slot->format == FT_GLYPH_FORMAT_BITMAP )
          goto Load_Ok;
      }

      /* load auto-hinted outline */
      hinting = (FT_AutoHinter_Service)hinter->clazz->module_interface;

      error   = hinting->load_glyph( (FT_AutoHinter)hinter,
                                     slot, face->size,
                                     glyph_index, load_flags );
    }
    else
    {
      error = driver->clazz->load_glyph( slot,
                                         face->size,
                                         glyph_index,
                                         load_flags );
      if ( error )
        goto Exit;

      /* check that the loaded outline is correct */
      error = FT_Outline_Check( &slot->outline );
      if ( error )
        goto Exit;
    }

  Load_Ok:
    /* compute the advance */
    if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
    {
      slot->advance.x = 0;
      slot->advance.y = slot->metrics.vertAdvance;
    }
    else
    {
      slot->advance.x = slot->metrics.horiAdvance;
      slot->advance.y = 0;
    }

    /* compute the linear advance in 16.16 pixels */
    if ( ( load_flags & FT_LOAD_LINEAR_DESIGN ) == 0  &&
         ( face->face_flags & FT_FACE_FLAG_SCALABLE ) )
    {
      FT_UInt           EM      = face->units_per_EM;
      FT_Size_Metrics*  metrics = &face->size->metrics;


      slot->linearHoriAdvance = FT_MulDiv( slot->linearHoriAdvance,
                                           (FT_Long)metrics->x_ppem << 16, EM );

      slot->linearVertAdvance = FT_MulDiv( slot->linearVertAdvance,
                                           (FT_Long)metrics->y_ppem << 16, EM );
    }

    if ( ( load_flags & FT_LOAD_IGNORE_TRANSFORM ) == 0 )
    {
      FT_Face_Internal  internal = face->internal;


      /* now, transform the glyph image if needed */
      if ( internal->transform_flags )
      {
        /* get renderer */
        FT_Renderer  renderer = ft_lookup_glyph_renderer( slot );


        if ( renderer )
          error = renderer->clazz->transform_glyph(
                                     renderer, slot,
                                     &internal->transform_matrix,
                                     &internal->transform_delta );
        /* transform advance */
        FT_Vector_Transform( &slot->advance, &internal->transform_matrix );
      }
    }

    /* do we need to render the image now? */
    if ( !error                                    &&
         slot->format != FT_GLYPH_FORMAT_BITMAP    &&
         slot->format != FT_GLYPH_FORMAT_COMPOSITE &&
         load_flags & FT_LOAD_RENDER )
    {
      FT_Render_Mode  mode = FT_LOAD_TARGET_MODE( load_flags );


      if ( mode == FT_RENDER_MODE_NORMAL      &&
           (load_flags & FT_LOAD_MONOCHROME ) )
        mode = FT_RENDER_MODE_MONO;

      error = FT_Render_Glyph( slot, mode );
    }

  Exit:
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Load_Char( FT_Face   face,
                FT_ULong  char_code,
                FT_Int32  load_flags )
  {
    FT_UInt  glyph_index;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    glyph_index = (FT_UInt)char_code;
    if ( face->charmap )
      glyph_index = FT_Get_Char_Index( face, char_code );

    return FT_Load_Glyph( face, glyph_index, load_flags );
  }


  /* destructor for sizes list */
  static void
  destroy_size( FT_Memory  memory,
                FT_Size    size,
                FT_Driver  driver )
  {
    /* finalize client-specific data */
    if ( size->generic.finalizer )
      size->generic.finalizer( size );

    /* finalize format-specific stuff */
    if ( driver->clazz->done_size )
      driver->clazz->done_size( size );

    FT_FREE( size->internal );
    FT_FREE( size );
  }


  /* destructor for faces list */
  static void
  destroy_face( FT_Memory  memory,
                FT_Face    face,
                FT_Driver  driver )
  {
    FT_Driver_Class  clazz = driver->clazz;


    /* discard auto-hinting data */
    if ( face->autohint.finalizer )
      face->autohint.finalizer( face->autohint.data );

    /* Discard glyph slots for this face.                           */
    /* Beware!  FT_Done_GlyphSlot() changes the field `face->glyph' */
    while ( face->glyph )
      FT_Done_GlyphSlot( face->glyph );

    /* discard all sizes for this face */
    FT_List_Finalize( &face->sizes_list,
                     (FT_List_Destructor)destroy_size,
                     memory,
                     driver );
    face->size = 0;

    /* now discard client data */
    if ( face->generic.finalizer )
      face->generic.finalizer( face );

    /* discard charmaps */
    {
      FT_Int  n;


      for ( n = 0; n < face->num_charmaps; n++ )
      {
        FT_CMap  cmap = FT_CMAP( face->charmaps[n] );


        FT_CMap_Done( cmap );

        face->charmaps[n] = NULL;
      }

      FT_FREE( face->charmaps );
      face->num_charmaps = 0;
    }


    /* finalize format-specific stuff */
    if ( clazz->done_face )
      clazz->done_face( face );

    /* close the stream for this face if needed */
    ft_input_stream_free(
      face->stream,
      ( face->face_flags & FT_FACE_FLAG_EXTERNAL_STREAM ) != 0 );

    face->stream = 0;

    /* get rid of it */
    if ( face->internal )
    {
      FT_FREE( face->internal->postscript_name );
      FT_FREE( face->internal );
    }
    FT_FREE( face );
  }


  static void
  Destroy_Driver( FT_Driver  driver )
  {
    FT_List_Finalize( &driver->faces_list,
                      (FT_List_Destructor)destroy_face,
                      driver->root.memory,
                      driver );

    /* check whether we need to drop the driver's glyph loader */
    if ( FT_DRIVER_USES_OUTLINES( driver ) )
      FT_GlyphLoader_Done( driver->glyph_loader );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    find_unicode_charmap                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function finds a Unicode charmap, if there is one.            */
  /*    And if there is more than one, it tries to favour the more         */
  /*    extensive one, i.e., one that supports UCS-4 against those which   */
  /*    are limited to the BMP (said UCS-2 encoding.)                      */
  /*                                                                       */
  /*    This function is called from open_face() (just below), and also    */
  /*    from FT_Select_Charmap( ..., FT_ENCODING_UNICODE).                 */
  /*                                                                       */
  static FT_Error
  find_unicode_charmap( FT_Face  face )
  {
    FT_CharMap*  first;
    FT_CharMap*  cur;
    FT_CharMap*  unicmap = NULL;  /* some UCS-2 map, if we found it */


    /* caller should have already checked that `face' is valid */
    FT_ASSERT ( face );

    first = face->charmaps;

    if ( !first )
      return FT_Err_Invalid_CharMap_Handle;

    /*
     *  The original TrueType specification(s) only specified charmap
     *  formats that are capable of mapping 8 or 16 bit character codes to
     *  glyph indices.
     *
     *  However, recent updates to the Apple and OpenType specifications
     *  introduced new formats that are capable of mapping 32-bit character
     *  codes as well.  And these are already used on some fonts, mainly to
     *  map non-BMP Asian ideographs as defined in Unicode.
     *
     *  For compatibility purposes, these fonts generally come with
     *  *several* Unicode charmaps:
     *
     *   - One of them in the "old" 16-bit format, that cannot access
     *     all glyphs in the font.
     *
     *   - Another one in the "new" 32-bit format, that can access all
     *     the glyphs.
     *
     *  This function has been written to always favor a 32-bit charmap
     *  when found.  Otherwise, a 16-bit one is returned when found.
     */

    /* since the `interesting' table, with id's 3,10, is normally the */
    /* last one, we loop backwards. This looses with type1 fonts with */
    /* non-BMP characters (<.0001%), this wins with .ttf with non-BMP */
    /* chars (.01% ?), and this is the same about 99.99% of the time! */

    cur = first + face->num_charmaps;  /* points after the last one */

    for ( ; --cur >= first; )
    {
      if ( cur[0]->encoding == FT_ENCODING_UNICODE )
      {
        unicmap = cur;  /* record we found a Unicode charmap */

        /* XXX If some new encodings to represent UCS-4 are added,  */
        /*     they should be added here.                           */
        if ( ( cur[0]->platform_id == TT_PLATFORM_MICROSOFT &&
               cur[0]->encoding_id == TT_MS_ID_UCS_4        )          ||
             ( cur[0]->platform_id == TT_PLATFORM_APPLE_UNICODE &&
               cur[0]->encoding_id == TT_APPLE_ID_UNICODE_32    )      )

        /* Hurray! We found a UCS-4 charmap. We can stop the scan! */
        {
          face->charmap = cur[0];
          return 0;
        }
      }
    }

    /* We do not have any UCS-4 charmap. Sigh.                           */
    /* Let's see if we have  some other kind of Unicode charmap, though. */
    if ( unicmap != NULL )
    {
      face->charmap = unicmap[0];
      return 0;
    }

    /* Chou blanc! */
    return FT_Err_Invalid_CharMap_Handle;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    open_face                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function does some work for FT_Open_Face().                   */
  /*                                                                       */
  static FT_Error
  open_face( FT_Driver      driver,
             FT_Stream      stream,
             FT_Long        face_index,
             FT_Int         num_params,
             FT_Parameter*  params,
             FT_Face       *aface )
  {
    FT_Memory         memory;
    FT_Driver_Class   clazz;
    FT_Face           face = 0;
    FT_Error          error, error2;
    FT_Face_Internal  internal;


    clazz  = driver->clazz;
    memory = driver->root.memory;

    /* allocate the face object and perform basic initialization */
    if ( FT_ALLOC( face, clazz->face_object_size ) )
      goto Fail;

    if ( FT_NEW( internal ) )
      goto Fail;

    face->internal = internal;

    face->driver   = driver;
    face->memory   = memory;
    face->stream   = stream;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    {
      int  i;


      face->internal->incremental_interface = 0;
      for ( i = 0; i < num_params && !face->internal->incremental_interface;
            i++ )
        if ( params[i].tag == FT_PARAM_TAG_INCREMENTAL )
          face->internal->incremental_interface = params[i].data;
    }
#endif

    error = clazz->init_face( stream,
                              face,
                              (FT_Int)face_index,
                              num_params,
                              params );
    if ( error )
      goto Fail;

    /* select Unicode charmap by default */
    error2 = find_unicode_charmap( face );

    /* if no Unicode charmap can be found, FT_Err_Invalid_CharMap_Handle */
    /* is returned.                                                      */

    /* no error should happen, but we want to play safe */
    if ( error2 && error2 != FT_Err_Invalid_CharMap_Handle )
    {
      error = error2;
      goto Fail;
    }

    *aface = face;

  Fail:
    if ( error )
    {
      clazz->done_face( face );
      FT_FREE( internal );
      FT_FREE( face );
      *aface = 0;
    }

    return error;
  }


  /* there's a Mac-specific extended implementation of FT_New_Face() */
  /* in src/base/ftmac.c                                             */

#ifndef FT_MACINTOSH

  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  pathname,
               FT_Long      face_index,
               FT_Face     *aface )
  {
    FT_Open_Args  args;


    /* test for valid `library' and `aface' delayed to FT_Open_Face() */
    if ( !pathname )
      return FT_Err_Invalid_Argument;

    args.flags    = FT_OPEN_PATHNAME;
    args.pathname = (char*)pathname;

    return FT_Open_Face( library, &args, face_index, aface );
  }

#endif  /* !FT_MACINTOSH */


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Memory_Face( FT_Library      library,
                      const FT_Byte*  file_base,
                      FT_Long         file_size,
                      FT_Long         face_index,
                      FT_Face        *aface )
  {
    FT_Open_Args  args;


    /* test for valid `library' and `face' delayed to FT_Open_Face() */
    if ( !file_base )
      return FT_Err_Invalid_Argument;

    args.flags       = FT_OPEN_MEMORY;
    args.memory_base = file_base;
    args.memory_size = file_size;

    return FT_Open_Face( library, &args, face_index, aface );
  }


#if !defined( FT_MACINTOSH ) && defined( FT_CONFIG_OPTION_MAC_FONTS )

  /* The behavior here is very similar to that in base/ftmac.c, but it     */
  /* is designed to work on non-mac systems, so no mac specific calls.     */
  /*                                                                       */
  /* We look at the file and determine if it is a mac dfont file or a mac  */
  /* resource file, or a macbinary file containing a mac resource file.    */
  /*                                                                       */
  /* Unlike ftmac I'm not going to look at a `FOND'.  I don't really see   */
  /* the point, especially since there may be multiple `FOND' resources.   */
  /* Instead I'll just look for `sfnt' and `POST' resources, ordered as    */
  /* they occur in the file.                                               */
  /*                                                                       */
  /* Note that multiple `POST' resources do not mean multiple postscript   */
  /* fonts; they all get jammed together to make what is essentially a     */
  /* pfb file.                                                             */
  /*                                                                       */
  /* We aren't interested in `NFNT' or `FONT' bitmap resources.            */
  /*                                                                       */
  /* As soon as we get an `sfnt' load it into memory and pass it off to    */
  /* FT_Open_Face.                                                         */
  /*                                                                       */
  /* If we have a (set of) `POST' resources, massage them into a (memory)  */
  /* pfb file and pass that to FT_Open_Face.  (As with ftmac.c I'm not     */
  /* going to try to save the kerning info.  After all that lives in the   */
  /* `FOND' which isn't in the file containing the `POST' resources so     */
  /* we don't really have access to it.                                    */


  /* Finalizer for a memory stream; gets called by FT_Done_Face().
     It frees the memory it uses. */
  /* from ftmac.c */
  static void
  memory_stream_close( FT_Stream  stream )
  {
    FT_Memory  memory = stream->memory;


    FT_FREE( stream->base );

    stream->size  = 0;
    stream->base  = 0;
    stream->close = 0;
  }


  /* Create a new memory stream from a buffer and a size. */
  /* from ftmac.c */
  static FT_Error
  new_memory_stream( FT_Library           library,
                     FT_Byte*             base,
                     FT_ULong             size,
                     FT_Stream_CloseFunc  close,
                     FT_Stream           *astream )
  {
    FT_Error   error;
    FT_Memory  memory;
    FT_Stream  stream;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !base )
      return FT_Err_Invalid_Argument;

    *astream = 0;
    memory = library->memory;
    if ( FT_NEW( stream ) )
      goto Exit;

    FT_Stream_OpenMemory( stream, base, size );

    stream->close = close;

    *astream = stream;

  Exit:
    return error;
  }


  /* Create a new FT_Face given a buffer and a driver name. */
  /* from ftmac.c */
  static FT_Error
  open_face_from_buffer( FT_Library   library,
                         FT_Byte*     base,
                         FT_ULong     size,
                         FT_Long      face_index,
                         const char*  driver_name,
                         FT_Face     *aface )
  {
    FT_Open_Args  args;
    FT_Error      error;
    FT_Stream     stream;
    FT_Memory     memory = library->memory;


    error = new_memory_stream( library,
                               base,
                               size,
                               memory_stream_close,
                               &stream );
    if ( error )
    {
      FT_FREE( base );
      return error;
    }

    args.flags = FT_OPEN_STREAM;
    args.stream = stream;
    if ( driver_name )
    {
      args.flags = args.flags | FT_OPEN_DRIVER;
      args.driver = FT_Get_Module( library, driver_name );
    }

    error = FT_Open_Face( library, &args, face_index, aface );

    if ( error == FT_Err_Ok )
      (*aface)->face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;
    else
    {
      FT_Stream_Close( stream );
      FT_FREE( stream );
    }

    return error;
  }


  /* The resource header says we've got resource_cnt `POST' (type1) */
  /* resources in this file.  They all need to be coalesced into    */
  /* one lump which gets passed on to the type1 driver.             */
  /* Here can be only one PostScript font in a file so face_index   */
  /* must be 0 (or -1).                                             */
  /*                                                                */
  static FT_Error
  Mac_Read_POST_Resource( FT_Library  library,
                          FT_Stream   stream,
                          FT_Long     resource_listoffset,
                          FT_Long     resource_cnt,
                          FT_Long     resource_data,
                          FT_Long     face_index,
                          FT_Face    *aface )
  {
    FT_Error   error  = FT_Err_Cannot_Open_Resource;
    FT_Memory  memory = library->memory;
    FT_Byte*   pfb_data;
    int        i, type, flags, len;
    FT_Long    pfb_len, pfb_pos, pfb_lenpos;
    FT_Long    rlen, junk, temp;
    FT_Long   *offsets;


    if ( face_index == -1 )
      face_index = 0;
    if ( face_index != 0 )
      return error;

    if ( FT_ALLOC( offsets, (FT_Long)resource_cnt * sizeof ( FT_Long ) ) )
      return error;

    error = FT_Stream_Seek( stream, resource_listoffset );
    if ( error )
      goto Exit;

    /* Find all the POST resource offsets */
    for ( i = 0; i < resource_cnt; ++i )
    {
      (void)FT_READ_USHORT( junk ); /* resource id */
      (void)FT_READ_USHORT( junk ); /* rsource name */
      if ( FT_READ_LONG( temp ) )
        goto Exit;
      offsets[i] = resource_data + ( temp & 0xFFFFFFL );
      (void)FT_READ_LONG( junk );   /* mbz */
    }

    /* Find the length of all the POST resources, concatenated.  Assume */
    /* worst case (each resource in its own section).                   */
    pfb_len = 0;
    for ( i = 0; i < resource_cnt; ++i )
    {
      error = FT_Stream_Seek( stream, offsets[i] );
      if ( error )
        goto Exit;
      if ( FT_READ_LONG( temp ) )
        goto Exit;
      pfb_len += temp + 6;
    }

    if ( FT_ALLOC( pfb_data, (FT_Long)pfb_len + 2 ) )
      goto Exit;

    pfb_pos             = 0;
    pfb_data[pfb_pos++] = 0x80;
    pfb_data[pfb_pos++] = 1;            /* Ascii section */
    pfb_lenpos          = pfb_pos;
    pfb_data[pfb_pos++] = 0;            /* 4-byte length, fill in later */
    pfb_data[pfb_pos++] = 0;
    pfb_data[pfb_pos++] = 0;
    pfb_data[pfb_pos++] = 0;

    len = 0;
    type = 1;
    for ( i = 0; i < resource_cnt; ++i )
    {
      error = FT_Stream_Seek( stream, offsets[i] );
      if ( error )
        goto Exit2;
      if ( FT_READ_LONG( rlen ) )
        goto Exit;
      if ( FT_READ_USHORT( flags ) )
        goto Exit;
      rlen -= 2;                    /* the flags are part of the resource */
      if ( ( flags >> 8 ) == type )
        len += rlen;
      else
      {
        pfb_data[pfb_lenpos    ] =   len         & 0xFF;
        pfb_data[pfb_lenpos + 1] = ( len >>  8 ) & 0xFF;
        pfb_data[pfb_lenpos + 2] = ( len >> 16 ) & 0xFF;
        pfb_data[pfb_lenpos + 3] = ( len >> 24 ) & 0xFF;

        if ( ( flags >> 8 ) == 5 )      /* End of font mark */
          break;

        pfb_data[pfb_pos++] = 0x80;

        type = flags >> 8;
        len = rlen;

        pfb_data[pfb_pos++] = type;
        pfb_lenpos          = pfb_pos;
        pfb_data[pfb_pos++] = 0;        /* 4-byte length, fill in later */
        pfb_data[pfb_pos++] = 0;
        pfb_data[pfb_pos++] = 0;
        pfb_data[pfb_pos++] = 0;
      }

      error = FT_Stream_Read( stream, (FT_Byte *)pfb_data + pfb_pos, rlen );
      pfb_pos += rlen;
    }

    pfb_data[pfb_pos++] = 0x80;
    pfb_data[pfb_pos++] = 3;

    pfb_data[pfb_lenpos    ] =   len         & 0xFF;
    pfb_data[pfb_lenpos + 1] = ( len >>  8 ) & 0xFF;
    pfb_data[pfb_lenpos + 2] = ( len >> 16 ) & 0xFF;
    pfb_data[pfb_lenpos + 3] = ( len >> 24 ) & 0xFF;

    return open_face_from_buffer( library,
                                  pfb_data,
                                  pfb_pos,
                                  face_index,
                                  "type1",
                                  aface );

  Exit2:
    FT_FREE( pfb_data );

  Exit:
    FT_FREE( offsets );
    return error;
  }


  /* The resource header says we've got resource_cnt `sfnt'      */
  /* (TrueType/OpenType) resources in this file.  Look through   */
  /* them for the one indicated by face_index, load it into mem, */
  /* pass it on the the truetype driver and return it.           */
  /*                                                             */
  static FT_Error
  Mac_Read_sfnt_Resource( FT_Library  library,
                          FT_Stream   stream,
                          FT_Long     resource_listoffset,
                          FT_Long     resource_cnt,
                          FT_Long     resource_data,
                          FT_Long     face_index,
                          FT_Face    *aface )
  {
    FT_Memory  memory = library->memory;
    FT_Byte*   sfnt_data;
    FT_Error   error;
    int        i;
    FT_Long    flag_offset= 0xFFFFFFL;
    FT_Long    rlen, junk;
    int        is_cff;


    if ( face_index == -1 )
      face_index = 0;
    if ( face_index >= resource_cnt )
      return FT_Err_Cannot_Open_Resource;

    error = FT_Stream_Seek( stream, resource_listoffset );
    if ( error )
      goto Exit;

    for ( i = 0; i <= face_index; ++i )
    {
      (void)FT_READ_USHORT( junk );     /* resource id */
      (void)FT_READ_USHORT( junk );     /* rsource name */
      if ( FT_READ_LONG( flag_offset ) )
        goto Exit;
      flag_offset &= 0xFFFFFFL;
      (void)FT_READ_LONG( junk );       /* mbz */
    }

    if ( flag_offset == 0xFFFFFFL )
      return FT_Err_Cannot_Open_Resource;

    error = FT_Stream_Seek( stream, flag_offset + resource_data );
    if ( error )
      goto Exit;
    if ( FT_READ_LONG( rlen ) )
      goto Exit;
    if ( rlen == -1 )
      return FT_Err_Cannot_Open_Resource;

    if ( FT_ALLOC( sfnt_data, (FT_Long)rlen ) )
      return error;
    error = FT_Stream_Read( stream, (FT_Byte *)sfnt_data, rlen );
    if ( error )
      goto Exit;

    is_cff = rlen > 4 && sfnt_data[0] == 'O' &&
                         sfnt_data[1] == 'T' &&
                         sfnt_data[2] == 'T' &&
                         sfnt_data[3] == 'O';

    error = open_face_from_buffer( library,
                                   sfnt_data,
                                   rlen,
                                   face_index,
                                   is_cff ? "cff" : "truetype",
                                   aface );

  Exit:
    return error;
  }


  /* Check for a valid resource fork header, or a valid dfont    */
  /* header.  In a resource fork the first 16 bytes are repeated */
  /* at the location specified by bytes 4-7.  In a dfont bytes   */
  /* 4-7 point to 16 bytes of zeroes instead.                    */
  /*                                                             */
  static FT_Error
  IsMacResource( FT_Library  library,
                 FT_Stream   stream,
                 FT_Long     resource_offset,
                 FT_Long     face_index,
                 FT_Face    *aface )
  {
    FT_Error       error;
    unsigned char  head[16], head2[16];
    FT_Long        rdata_pos, map_pos, rdata_len, map_len;
    int            allzeros, allmatch, i, cnt, subcnt;
    FT_Long        type_list, tag, rpos, junk;


    error = FT_Stream_Seek( stream, resource_offset );
    if ( error )
      goto Exit;
    error = FT_Stream_Read( stream, (FT_Byte *)head, 16 );
    if ( error )
      goto Exit;

    rdata_pos = resource_offset + ( ( head[0] << 24 ) |
                                    ( head[1] << 16 ) |
                                    ( head[2] <<  8 ) |
                                      head[3]         );
    map_pos   = resource_offset + ( ( head[4] << 24 ) |
                                    ( head[5] << 16 ) |
                                    ( head[6] <<  8 ) |
                                      head[7]         );
    rdata_len = ( head[ 8] << 24 ) |
                ( head[ 9] << 16 ) |
                ( head[10] <<  8 ) |
                  head[11];
    map_len   = ( head[12] << 24 ) |
                ( head[13] << 16 ) |
                ( head[14] <<  8 ) |
                  head[15];

    if ( rdata_pos + rdata_len != map_pos || map_pos == resource_offset )
      return FT_Err_Unknown_File_Format;

    error = FT_Stream_Seek( stream, map_pos );
    if ( error )
      goto Exit;

    head2[15] = head[15] + 1;       /* make it be different */

    error = FT_Stream_Read( stream, (FT_Byte*)head2, 16 );
    if ( error )
      goto Exit;

    allzeros = 1;
    allmatch = 1;
    for ( i = 0; i < 16; ++i )
    {
      if ( head2[i] != 0 )
        allzeros = 0;
      if ( head2[i] != head[i] )
        allmatch = 0;
    }
    if ( !allzeros && !allmatch )
      return FT_Err_Unknown_File_Format;

    /* If we've gotten this far then it's probably a mac resource file. */
    /* Now, does it contain any interesting resources?                  */

    (void)FT_READ_LONG( junk );    /* skip handle to next resource map */
    (void)FT_READ_USHORT( junk );  /* skip file resource number */
    (void)FT_READ_USHORT( junk );  /* skip attributes */

    if ( FT_READ_USHORT( type_list ) )
      goto Exit;
    if ( type_list == -1 )
      return FT_Err_Unknown_File_Format;

    error = FT_Stream_Seek( stream, map_pos + type_list );
    if ( error )
      goto Exit;

    if ( FT_READ_USHORT( cnt ) )
      goto Exit;

    ++cnt;
    for ( i = 0; i < cnt; ++i )
    {
      if ( FT_READ_LONG( tag )      ||
           FT_READ_USHORT( subcnt ) ||
           FT_READ_USHORT( rpos )   )
        goto Exit;

      ++subcnt;
      rpos += map_pos + type_list;
      if ( tag == FT_MAKE_TAG( 'P', 'O', 'S', 'T' ) )
        return Mac_Read_POST_Resource( library,
                                       stream,
                                       rpos,
                                       subcnt,
                                       rdata_pos,
                                       face_index,
                                       aface );
      else if ( tag == FT_MAKE_TAG( 's', 'f', 'n', 't' ) )
        return Mac_Read_sfnt_Resource( library,
                                       stream,
                                       rpos,
                                       subcnt,
                                       rdata_pos,
                                       face_index,
                                       aface );
    }

    error = FT_Err_Cannot_Open_Resource; /* this file contains no
                                            interesting resources */
  Exit:
    return error;
  }


  /* Check for a valid macbinary header, and if we find one   */
  /* check that the (flattened) resource fork in it is valid. */
  /*                                                          */
  static FT_Error
  IsMacBinary( FT_Library  library,
               FT_Stream   stream,
               FT_Long     face_index,
               FT_Face    *aface )
  {
    unsigned char  header[128];
    FT_Error       error;
    FT_Long        dlen, offset;


    error = FT_Stream_Seek( stream, 0 );
    if ( error )
      goto Exit;

    error = FT_Stream_Read( stream, (FT_Byte*)header, 128 );
    if ( error )
      goto Exit;

    if (            header[ 0] !=  0 ||
                    header[74] !=  0 ||
                    header[82] !=  0 ||
                    header[ 1] ==  0 ||
                    header[ 1] >  33 ||
                    header[63] !=  0 ||
         header[2 + header[1]] !=  0 )
      return FT_Err_Unknown_File_Format;

    dlen = ( header[0x53] << 24 ) |
           ( header[0x54] << 16 ) |
           ( header[0x55] <<  8 ) |
             header[0x56];
#if 0
    rlen = ( header[0x57] << 24 ) |
           ( header[0x58] << 16 ) |
           ( header[0x59] <<  8 ) |
             header[0x5a];
#endif /* 0 */
    offset = 128 + ( ( dlen + 127 ) & ~127 );

    return IsMacResource( library, stream, offset, face_index, aface );

  Exit:
    return error;
  }


  /* Check for some macintosh formats                              */
  /* Is this a macbinary file?  If so look at the resource fork.   */
  /* Is this a mac dfont file?                                     */
  /* Is this an old style resource fork? (in data)                 */
  /* Else if we're on Mac OS/X, open the resource fork explicitly. */
  /*                                                               */
  static FT_Error
  load_mac_face( FT_Library           library,
                 FT_Stream            stream,
                 FT_Long              face_index,
                 FT_Face             *aface,
                 const FT_Open_Args  *args )
  {
    FT_Error error;
    FT_UNUSED( args );


    error = IsMacBinary( library, stream, face_index, aface );
    if ( FT_ERROR_BASE( error ) == FT_Err_Unknown_File_Format )
      error = IsMacResource( library, stream, 0, face_index, aface );

#ifdef FT_MACINTOSH
    /*
       I know this section is within code which is normally turned off 
       for the Mac.  It provides an alternative approach to reading the
       mac resource forks on OS/X in the event that a user does not wish
       to compile ftmac.c.
     */
         
    if ( ( FT_ERROR_BASE( error ) == FT_Err_Unknown_File_Format      ||
           FT_ERROR_BASE( error ) == FT_Err_Invalid_Stream_Operation )  &&
           ( args->flags & FT_OPEN_PATHNAME )                           )
    {
      FT_Open_Args  args2;
      char*         newpath;
      FT_Memory     memory;
      FT_Stream     stream2;


      memory = library->memory;

      FT_ALLOC( newpath,
                ft_strlen( args->pathname ) + ft_strlen( "/rsrc" ) + 1 );
      ft_strcpy( newpath, args->pathname );
      ft_strcat( newpath, "/rsrc" );

      args2.flags    = FT_OPEN_PATHNAME;
      args2.pathname = (char*)newpath;
      error = ft_input_stream_new( library, &args2, &stream2 );
      if ( !error )
      {
        error = IsMacResource( library, stream2, 0, face_index, aface );
        FT_Stream_Close( stream2 );
      }
      FT_FREE( newpath );
    }

#endif  /* FT_MACINTOSH */

    return error;
  }

#endif  /* !FT_MACINTOSH && FT_CONFIG_OPTION_MAC_FONTS */


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Open_Face( FT_Library           library,
                const FT_Open_Args*  args,
                FT_Long              face_index,
                FT_Face             *aface )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_Stream    stream;
    FT_Face      face = 0;
    FT_ListNode  node = 0;
    FT_Bool      external_stream;


    /* test for valid `library' delayed to */
    /* ft_input_stream_new()               */

    if ( !aface || !args )
      return FT_Err_Invalid_Argument;

    *aface = 0;

    external_stream = FT_BOOL( ( args->flags & FT_OPEN_STREAM ) &&
                               args->stream                     );

    /* create input stream */
    error = ft_input_stream_new( library, args, &stream );
    if ( error )
      goto Exit;

    memory = library->memory;

    /* If the font driver is specified in the `args' structure, use */
    /* it.  Otherwise, we scan the list of registered drivers.      */
    if ( ( args->flags & FT_OPEN_DRIVER ) && args->driver )
    {
      driver = FT_DRIVER( args->driver );

      /* not all modules are drivers, so check... */
      if ( FT_MODULE_IS_DRIVER( driver ) )
      {
        FT_Int         num_params = 0;
        FT_Parameter*  params     = 0;


        if ( args->flags & FT_OPEN_PARAMS )
        {
          num_params = args->num_params;
          params     = args->params;
        }

        error = open_face( driver, stream, face_index,
                           num_params, params, &face );
        if ( !error )
          goto Success;
      }
      else
        error = FT_Err_Invalid_Handle;

      ft_input_stream_free( stream, external_stream );
      goto Fail;
    }
    else
    {
      /* check each font driver for an appropriate format */
      FT_Module*  cur   = library->modules;
      FT_Module*  limit = cur + library->num_modules;


      for ( ; cur < limit; cur++ )
      {
        /* not all modules are font drivers, so check... */
        if ( FT_MODULE_IS_DRIVER( cur[0] ) )
        {
          FT_Int         num_params = 0;
          FT_Parameter*  params     = 0;


          driver = FT_DRIVER( cur[0] );

          if ( args->flags & FT_OPEN_PARAMS )
          {
            num_params = args->num_params;
            params     = args->params;
          }

          error = open_face( driver, stream, face_index,
                             num_params, params, &face );
          if ( !error )
            goto Success;

          if ( FT_ERROR_BASE( error ) != FT_Err_Unknown_File_Format )
            goto Fail3;
        }
      }

  Fail3:
    /* If we are on the mac, and we get an FT_Err_Invalid_Stream_Operation */
    /* it may be because we have an empty data fork, so we need to check   */
    /* the resource fork.                                                  */
    if ( FT_ERROR_BASE( error ) != FT_Err_Unknown_File_Format      &&
         FT_ERROR_BASE( error ) != FT_Err_Invalid_Stream_Operation )
      goto Fail2;

#ifndef FT_MACINTOSH
    error = load_mac_face( library, stream, face_index, aface, args );
    if ( !error )
    {
      /* We don't want to go to Success here.  We've already done that. */
      /* On the other hand, if we succeeded we still need to close this */
      /* stream (we opened a different stream which extracted the       */
      /* interesting information out of this stream here.  That stream  */
      /* will still be open and the face will point to it).             */
      ft_input_stream_free( stream, external_stream );
      return error;
    }

    if ( FT_ERROR_BASE( error ) != FT_Err_Unknown_File_Format )
      goto Fail2;
#endif  /* !FT_MACINTOSH */

      /* no driver is able to handle this format */
      error = FT_Err_Unknown_File_Format;

  Fail2:
      ft_input_stream_free( stream, external_stream );
      goto Fail;
    }

  Success:
    FT_TRACE4(( "FT_Open_Face: New face object, adding to list\n" ));

    /* set the FT_FACE_FLAG_EXTERNAL_STREAM bit for FT_Done_Face */
    if ( external_stream )
      face->face_flags |= FT_FACE_FLAG_EXTERNAL_STREAM;

    /* add the face object to its driver's list */
    if ( FT_NEW( node ) )
      goto Fail;

    node->data = face;
    /* don't assume driver is the same as face->driver, so use */
    /* face->driver instead.                                   */
    FT_List_Add( &face->driver->faces_list, node );

    /* now allocate a glyph slot object for the face */
    {
      FT_GlyphSlot  slot;


      FT_TRACE4(( "FT_Open_Face: Creating glyph slot\n" ));

      error = FT_New_GlyphSlot( face, &slot );
      if ( error )
        goto Fail;

      face->glyph = slot;
    }

    /* finally, allocate a size object for the face */
    {
      FT_Size  size;


      FT_TRACE4(( "FT_Open_Face: Creating size object\n" ));

      error = FT_New_Size( face, &size );
      if ( error )
        goto Fail;

      face->size = size;
    }

    /* initialize internal face data */
    {
      FT_Face_Internal  internal = face->internal;


      internal->transform_matrix.xx = 0x10000L;
      internal->transform_matrix.xy = 0;
      internal->transform_matrix.yx = 0;
      internal->transform_matrix.yy = 0x10000L;

      internal->transform_delta.x = 0;
      internal->transform_delta.y = 0;
    }

    *aface = face;
    goto Exit;

  Fail:
    FT_Done_Face( face );

  Exit:
    FT_TRACE4(( "FT_Open_Face: Return %d\n", error ));

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Attach_File( FT_Face      face,
                  const char*  filepathname )
  {
    FT_Open_Args  open;


    /* test for valid `face' delayed to FT_Attach_Stream() */

    if ( !filepathname )
      return FT_Err_Invalid_Argument;

    open.flags    = FT_OPEN_PATHNAME;
    open.pathname = (char*)filepathname;

    return FT_Attach_Stream( face, &open );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Attach_Stream( FT_Face        face,
                    FT_Open_Args*  parameters )
  {
    FT_Stream  stream;
    FT_Error   error;
    FT_Driver  driver;

    FT_Driver_Class  clazz;


    /* test for valid `parameters' delayed to ft_input_stream_new() */

    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    if ( !driver )
      return FT_Err_Invalid_Driver_Handle;

    error = ft_input_stream_new( driver->root.library, parameters, &stream );
    if ( error )
      goto Exit;

    /* we implement FT_Attach_Stream in each driver through the */
    /* `attach_file' interface                                  */

    error = FT_Err_Unimplemented_Feature;
    clazz = driver->clazz;
    if ( clazz->attach_file )
      error = clazz->attach_file( face, stream );

    /* close the attached stream */
    ft_input_stream_free( stream,
                    (FT_Bool)( parameters->stream &&
                               ( parameters->flags & FT_OPEN_STREAM ) ) );

  Exit:
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_Face( FT_Face  face )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_ListNode  node;


    error = FT_Err_Invalid_Face_Handle;
    if ( face && face->driver )
    {
      driver = face->driver;
      memory = driver->root.memory;

      /* find face in driver's list */
      node = FT_List_Find( &driver->faces_list, face );
      if ( node )
      {
        /* remove face object from the driver's list */
        FT_List_Remove( &driver->faces_list, node );
        FT_FREE( node );

        /* now destroy the object proper */
        destroy_face( memory, face, driver );
        error = FT_Err_Ok;
      }
    }
    return error;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Size( FT_Face   face,
               FT_Size  *asize )
  {
    FT_Error         error;
    FT_Memory        memory;
    FT_Driver        driver;
    FT_Driver_Class  clazz;

    FT_Size          size = 0;
    FT_ListNode      node = 0;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    if ( !asize )
      return FT_Err_Invalid_Size_Handle;

    if ( !face->driver )
      return FT_Err_Invalid_Driver_Handle;

    *asize = 0;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = face->memory;

    /* Allocate new size object and perform basic initialisation */
    if ( FT_ALLOC( size, clazz->size_object_size ) || FT_NEW( node ) )
      goto Exit;

    size->face = face;

    /* for now, do not use any internal fields in size objects */
    size->internal = 0;

    if ( clazz->init_size )
      error = clazz->init_size( size );

    /* in case of success, add to the face's list */
    if ( !error )
    {
      *asize     = size;
      node->data = size;
      FT_List_Add( &face->sizes_list, node );
    }

  Exit:
    if ( error )
    {
      FT_FREE( node );
      FT_FREE( size );
    }

    return error;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_Size( FT_Size  size )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_Face      face;
    FT_ListNode  node;


    if ( !size )
      return FT_Err_Invalid_Size_Handle;

    face = size->face;
    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    if ( !driver )
      return FT_Err_Invalid_Driver_Handle;

    memory = driver->root.memory;

    error = FT_Err_Ok;
    node  = FT_List_Find( &face->sizes_list, size );
    if ( node )
    {
      FT_List_Remove( &face->sizes_list, node );
      FT_FREE( node );

      if ( face->size == size )
      {
        face->size = 0;
        if ( face->sizes_list.head )
          face->size = (FT_Size)(face->sizes_list.head->data);
      }

      destroy_size( memory, size, driver );
    }
    else
      error = FT_Err_Invalid_Size_Handle;

    return error;
  }


  static void
  ft_recompute_scaled_metrics( FT_Face           face,
                               FT_Size_Metrics*  metrics )
  {
    /* Compute root ascender, descender, test height, and max_advance */

    metrics->ascender    = ( FT_MulFix( face->ascender,
                                        metrics->y_scale ) + 63 ) & -64;

    metrics->descender   = ( FT_MulFix( face->descender,
                                        metrics->y_scale ) + 0 ) & -64;

    metrics->height      = ( FT_MulFix( face->height,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->max_advance = ( FT_MulFix( face->max_advance_width,
                                        metrics->x_scale ) + 32 ) & -64;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Char_Size( FT_Face     face,
                    FT_F26Dot6  char_width,
                    FT_F26Dot6  char_height,
                    FT_UInt     horz_resolution,
                    FT_UInt     vert_resolution )
  {
    FT_Error          error = FT_Err_Ok;
    FT_Driver         driver;
    FT_Driver_Class   clazz;
    FT_Size_Metrics*  metrics;
    FT_Long           dim_x, dim_y;


    if ( !face || !face->size || !face->driver )
      return FT_Err_Invalid_Face_Handle;

    driver  = face->driver;
    metrics = &face->size->metrics;
    clazz   = driver->clazz;

    if ( !char_width )
      char_width = char_height;

    else if ( !char_height )
      char_height = char_width;

    if ( !horz_resolution )
      horz_resolution = 72;

    if ( !vert_resolution )
      vert_resolution = 72;

    /* default processing -- this can be overridden by the driver */
    if ( char_width  < 1 * 64 )
      char_width  = 1 * 64;
    if ( char_height < 1 * 64 )
      char_height = 1 * 64;

    /* Compute pixel sizes in 26.6 units with rounding */
    dim_x = ( ( char_width  * horz_resolution + (36+32*72) ) / 72 ) & -64;
    dim_y = ( ( char_height * vert_resolution + (36+32*72) ) / 72 ) & -64;

    metrics->x_ppem  = (FT_UShort)( dim_x >> 6 );
    metrics->y_ppem  = (FT_UShort)( dim_y >> 6 );

    metrics->x_scale = 0x10000L;
    metrics->y_scale = 0x10000L;

    if ( face->face_flags & FT_FACE_FLAG_SCALABLE )
    {
      metrics->x_scale = FT_DivFix( dim_x, face->units_per_EM );
      metrics->y_scale = FT_DivFix( dim_y, face->units_per_EM );

      ft_recompute_scaled_metrics( face, metrics );
    }

    if ( clazz->set_char_sizes )
      error = clazz->set_char_sizes( face->size,
                                     char_width,
                                     char_height,
                                     horz_resolution,
                                     vert_resolution );
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Pixel_Sizes( FT_Face  face,
                      FT_UInt  pixel_width,
                      FT_UInt  pixel_height )
  {
    FT_Error          error = FT_Err_Ok;
    FT_Driver         driver;
    FT_Driver_Class   clazz;
    FT_Size_Metrics*  metrics;


    if ( !face || !face->size || !face->driver )
      return FT_Err_Invalid_Face_Handle;

    driver  = face->driver;
    metrics = &face->size->metrics;
    clazz   = driver->clazz;

    /* default processing -- this can be overridden by the driver */
    if ( pixel_width == 0 )
      pixel_width = pixel_height;

    else if ( pixel_height == 0 )
      pixel_height = pixel_width;

    if ( pixel_width  < 1 )
      pixel_width  = 1;
    if ( pixel_height < 1 )
      pixel_height = 1;

    metrics->x_ppem = (FT_UShort)pixel_width;
    metrics->y_ppem = (FT_UShort)pixel_height;

    if ( face->face_flags & FT_FACE_FLAG_SCALABLE )
    {
      metrics->x_scale = FT_DivFix( metrics->x_ppem << 6,
                                    face->units_per_EM );

      metrics->y_scale = FT_DivFix( metrics->y_ppem << 6,
                                    face->units_per_EM );

      ft_recompute_scaled_metrics( face, metrics );
    }

    if ( clazz->set_pixel_sizes )
      error = clazz->set_pixel_sizes( face->size,
                                      pixel_width,
                                      pixel_height );
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Kerning( FT_Face     face,
                  FT_UInt     left_glyph,
                  FT_UInt     right_glyph,
                  FT_UInt     kern_mode,
                  FT_Vector  *akerning )
  {
    FT_Error   error = FT_Err_Ok;
    FT_Driver  driver;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    if ( !akerning )
      return FT_Err_Invalid_Argument;

    driver = face->driver;

    akerning->x = 0;
    akerning->y = 0;

    if ( driver->clazz->get_kerning )
    {
      error = driver->clazz->get_kerning( face,
                                          left_glyph,
                                          right_glyph,
                                          akerning );
      if ( !error )
      {
        if ( kern_mode != FT_KERNING_UNSCALED )
        {
          akerning->x = FT_MulFix( akerning->x, face->size->metrics.x_scale );
          akerning->y = FT_MulFix( akerning->y, face->size->metrics.y_scale );

          if ( kern_mode != FT_KERNING_UNFITTED )
          {
            akerning->x = ( akerning->x + 32 ) & -64;
            akerning->y = ( akerning->y + 32 ) & -64;
          }
        }
      }
    }

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Select_Charmap( FT_Face      face,
                     FT_Encoding  encoding )
  {
    FT_CharMap*  cur;
    FT_CharMap*  limit;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    /* FT_ENCODING_UNICODE is special.  We try to find the `best' Unicode */
    /* charmap available, i.e., one with UCS-4 characters, if possible.   */
    /*                                                                    */
    /* This is done by find_unicode_charmap() above, to share code.       */
    if ( encoding == FT_ENCODING_UNICODE )
      return find_unicode_charmap( face );

    cur = face->charmaps;
    if ( !cur )
      return FT_Err_Invalid_CharMap_Handle;

    limit = cur + face->num_charmaps;

    for ( ; cur < limit; cur++ )
    {
      if ( cur[0]->encoding == encoding )
      {
        face->charmap = cur[0];
        return 0;
      }
    }

    return FT_Err_Invalid_Argument;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Charmap( FT_Face     face,
                  FT_CharMap  charmap )
  {
    FT_CharMap*  cur;
    FT_CharMap*  limit;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    cur = face->charmaps;
    if ( !cur )
      return FT_Err_Invalid_CharMap_Handle;

    limit = cur + face->num_charmaps;

    for ( ; cur < limit; cur++ )
    {
      if ( cur[0] == charmap )
      {
        face->charmap = cur[0];
        return 0;
      }
    }
    return FT_Err_Invalid_Argument;
  }


  FT_BASE_DEF( void )
  FT_CMap_Done( FT_CMap  cmap )
  {
    if ( cmap )
    {
      FT_CMap_Class  clazz  = cmap->clazz;
      FT_Face        face   = cmap->charmap.face;
      FT_Memory      memory = FT_FACE_MEMORY(face);


      if ( clazz->done )
        clazz->done( cmap );

      FT_FREE( cmap );
    }
  }


  FT_BASE_DEF( FT_Error )
  FT_CMap_New( FT_CMap_Class   clazz,
               FT_Pointer      init_data,
               FT_CharMap      charmap,
               FT_CMap        *acmap )
  {
    FT_Error   error = 0;
    FT_Face    face;
    FT_Memory  memory;
    FT_CMap    cmap;


    if ( clazz == NULL || charmap == NULL || charmap->face == NULL )
      return FT_Err_Invalid_Argument;

    face   = charmap->face;
    memory = FT_FACE_MEMORY( face );

    if ( !FT_ALLOC( cmap, clazz->size ) )
    {
      cmap->charmap = *charmap;
      cmap->clazz   = clazz;

      if ( clazz->init )
      {
        error = clazz->init( cmap, init_data );
        if ( error )
          goto Fail;
      }

      /* add it to our list of charmaps */
      if ( FT_RENEW_ARRAY( face->charmaps,
                           face->num_charmaps,
                           face->num_charmaps+1 ) )
        goto Fail;

      face->charmaps[face->num_charmaps++] = (FT_CharMap)cmap;
    }

  Exit:
    if ( acmap )
      *acmap = cmap;

    return error;

  Fail:
    FT_CMap_Done( cmap );
    cmap = NULL;
    goto Exit;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_UInt )
  FT_Get_Char_Index( FT_Face   face,
                     FT_ULong  charcode )
  {
    FT_UInt  result = 0;


    if ( face && face->charmap )
    {
      FT_CMap  cmap = FT_CMAP( face->charmap );


      result = cmap->clazz->char_index( cmap, charcode );
    }
    return  result;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_ULong )
  FT_Get_First_Char( FT_Face   face,
                     FT_UInt  *agindex )
  {
    FT_ULong  result = 0;
    FT_UInt   gindex = 0;


    if ( face && face->charmap )
    {
      gindex = FT_Get_Char_Index( face, 0 );
      if ( gindex == 0 )
        result = FT_Get_Next_Char( face, 0, &gindex );
    }

    if ( agindex  )
      *agindex = gindex;

    return result;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_ULong )
  FT_Get_Next_Char( FT_Face   face,
                    FT_ULong  charcode,
                    FT_UInt  *agindex )
  {
    FT_ULong  result = 0;
    FT_UInt   gindex = 0;


    if ( face && face->charmap )
    {
      FT_UInt32  code = (FT_UInt32)charcode;
      FT_CMap    cmap = FT_CMAP( face->charmap );


      gindex = cmap->clazz->char_next( cmap, &code );
      result = ( gindex == 0 ) ? 0 : code;
    }

    if ( agindex )
      *agindex = gindex;

    return result;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_UInt )
  FT_Get_Name_Index( FT_Face     face,
                     FT_String*  glyph_name )
  {
    FT_UInt  result = 0;


    if ( face && FT_HAS_GLYPH_NAMES( face ) )
    {
      /* now, lookup for glyph name */
      FT_Driver         driver = face->driver;
      FT_Module_Class*  clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_Face_GetGlyphNameIndexFunc  requester;


        requester = (FT_Face_GetGlyphNameIndexFunc)clazz->get_interface(
                      FT_MODULE( driver ), "name_index" );
        if ( requester )
          result = requester( face, glyph_name );
      }
    }

    return result;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Glyph_Name( FT_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max )
  {
    FT_Error  error = FT_Err_Invalid_Argument;


    /* clean up buffer */
    if ( buffer && buffer_max > 0 )
      ((FT_Byte*)buffer)[0] = 0;

    if ( face                                     &&
         glyph_index <= (FT_UInt)face->num_glyphs &&
         FT_HAS_GLYPH_NAMES( face )               )
    {
      /* now, lookup for glyph name */
      FT_Driver         driver = face->driver;
      FT_Module_Class*  clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_Face_GetGlyphNameFunc  requester;


        requester = (FT_Face_GetGlyphNameFunc)clazz->get_interface(
                      FT_MODULE( driver ), "glyph_name" );
        if ( requester )
          error = requester( face, glyph_index, buffer, buffer_max );
      }
    }

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( const char* )
  FT_Get_Postscript_Name( FT_Face  face )
  {
    const char*  result = NULL;


    if ( !face )
      goto Exit;

    result = face->internal->postscript_name;
    if ( !result )
    {
      /* now, look up glyph name */
      FT_Driver         driver = face->driver;
      FT_Module_Class*  clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_Face_GetPostscriptNameFunc  requester;


        requester = (FT_Face_GetPostscriptNameFunc)clazz->get_interface(
                      FT_MODULE( driver ), "postscript_name" );
        if ( requester )
          result = requester( face );
      }
    }
  Exit:
    return result;
  }


  /* documentation is in tttables.h */

  FT_EXPORT_DEF( void* )
  FT_Get_Sfnt_Table( FT_Face      face,
                     FT_Sfnt_Tag  tag )
  {
    void*                   table = 0;
    FT_Get_Sfnt_Table_Func  func;
    FT_Driver               driver;


    if ( !face || !FT_IS_SFNT( face ) )
      goto Exit;

    driver = face->driver;
    func = (FT_Get_Sfnt_Table_Func)driver->root.clazz->get_interface(
                                     FT_MODULE( driver ), "get_sfnt" );
    if ( func )
      table = func( face, tag );

  Exit:
    return table;
  }


  /* documentation is in tttables.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Load_Sfnt_Table( FT_Face    face,
                      FT_ULong   tag,
                      FT_Long    offset,
                      FT_Byte*   buffer,
                      FT_ULong*  length )
  {
    SFNT_Load_Table_Func  func;
    FT_Driver             driver;


    if ( !face || !FT_IS_SFNT( face ) )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    func   = (SFNT_Load_Table_Func) driver->root.clazz->get_interface(
                                      FT_MODULE( driver ), "load_sfnt" );
    if ( !func )
      return FT_Err_Unimplemented_Feature;

    return func( face, tag, offset, buffer, length );
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Activate_Size( FT_Size  size )
  {
    FT_Face  face;


    if ( size == NULL )
      return FT_Err_Bad_Argument;

    face = size->face;
    if ( face == NULL || face->driver == NULL )
      return FT_Err_Bad_Argument;

    /* we don't need anything more complex than that; all size objects */
    /* are already listed by the face                                  */
    face->size = size;

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                        R E N D E R E R S                        ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /* lookup a renderer by glyph format in the library's list */
  FT_BASE_DEF( FT_Renderer )
  FT_Lookup_Renderer( FT_Library       library,
                      FT_Glyph_Format  format,
                      FT_ListNode*     node )
  {
    FT_ListNode  cur;
    FT_Renderer  result = 0;


    if ( !library )
      goto Exit;

    cur = library->renderers.head;

    if ( node )
    {
      if ( *node )
        cur = (*node)->next;
      *node = 0;
    }

    while ( cur )
    {
      FT_Renderer  renderer = FT_RENDERER( cur->data );


      if ( renderer->glyph_format == format )
      {
        if ( node )
          *node = cur;

        result = renderer;
        break;
      }
      cur = cur->next;
    }

  Exit:
    return result;
  }


  static FT_Renderer
  ft_lookup_glyph_renderer( FT_GlyphSlot  slot )
  {
    FT_Face      face    = slot->face;
    FT_Library   library = FT_FACE_LIBRARY( face );
    FT_Renderer  result  = library->cur_renderer;


    if ( !result || result->glyph_format != slot->format )
      result = FT_Lookup_Renderer( library, slot->format, 0 );

    return result;
  }


  static void
  ft_set_current_renderer( FT_Library  library )
  {
    FT_Renderer  renderer;


    renderer = FT_Lookup_Renderer( library, FT_GLYPH_FORMAT_OUTLINE, 0 );
    library->cur_renderer = renderer;
  }


  static FT_Error
  ft_add_renderer( FT_Module  module )
  {
    FT_Library   library = module->library;
    FT_Memory    memory  = library->memory;
    FT_Error     error;
    FT_ListNode  node;


    if ( FT_NEW( node ) )
      goto Exit;

    {
      FT_Renderer         render = FT_RENDERER( module );
      FT_Renderer_Class*  clazz  = (FT_Renderer_Class*)module->clazz;


      render->clazz        = clazz;
      render->glyph_format = clazz->glyph_format;

      /* allocate raster object if needed */
      if ( clazz->glyph_format == FT_GLYPH_FORMAT_OUTLINE &&
           clazz->raster_class->raster_new )
      {
        error = clazz->raster_class->raster_new( memory, &render->raster );
        if ( error )
          goto Fail;

        render->raster_render = clazz->raster_class->raster_render;
        render->render        = clazz->render_glyph;
      }

      /* add to list */
      node->data = module;
      FT_List_Add( &library->renderers, node );

      ft_set_current_renderer( library );
    }

  Fail:
    if ( error )
      FT_FREE( node );

  Exit:
    return error;
  }


  static void
  ft_remove_renderer( FT_Module  module )
  {
    FT_Library   library = module->library;
    FT_Memory    memory  = library->memory;
    FT_ListNode  node;


    node = FT_List_Find( &library->renderers, module );
    if ( node )
    {
      FT_Renderer  render = FT_RENDERER( module );


      /* release raster object, if any */
      if ( render->raster )
        render->clazz->raster_class->raster_done( render->raster );

      /* remove from list */
      FT_List_Remove( &library->renderers, node );
      FT_FREE( node );

      ft_set_current_renderer( library );
    }
  }


  /* documentation is in ftrender.h */

  FT_EXPORT_DEF( FT_Renderer )
  FT_Get_Renderer( FT_Library       library,
                   FT_Glyph_Format  format )
  {
    /* test for valid `library' delayed to FT_Lookup_Renderer() */

    return FT_Lookup_Renderer( library, format, 0 );
  }


  /* documentation is in ftrender.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Renderer( FT_Library     library,
                   FT_Renderer    renderer,
                   FT_UInt        num_params,
                   FT_Parameter*  parameters )
  {
    FT_ListNode  node;
    FT_Error     error = FT_Err_Ok;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !renderer )
      return FT_Err_Invalid_Argument;

    node = FT_List_Find( &library->renderers, renderer );
    if ( !node )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    FT_List_Up( &library->renderers, node );

    if ( renderer->glyph_format == FT_GLYPH_FORMAT_OUTLINE )
      library->cur_renderer = renderer;

    if ( num_params > 0 )
    {
      FT_Renderer_SetModeFunc  set_mode = renderer->clazz->set_mode;


      for ( ; num_params > 0; num_params-- )
      {
        error = set_mode( renderer, parameters->tag, parameters->data );
        if ( error )
          break;
      }
    }

  Exit:
    return error;
  }


  FT_BASE_DEF( FT_Error )
  FT_Render_Glyph_Internal( FT_Library      library,
                            FT_GlyphSlot    slot,
                            FT_Render_Mode  render_mode )
  {
    FT_Error     error = FT_Err_Ok;
    FT_Renderer  renderer;


    /* if it is already a bitmap, no need to do anything */
    switch ( slot->format )
    {
    case FT_GLYPH_FORMAT_BITMAP:   /* already a bitmap, don't do anything */
      break;

    default:
      {
        FT_ListNode  node   = 0;
        FT_Bool      update = 0;


        /* small shortcut for the very common case */
        if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
        {
          renderer = library->cur_renderer;
          node     = library->renderers.head;
        }
        else
          renderer = FT_Lookup_Renderer( library, slot->format, &node );

        error = FT_Err_Unimplemented_Feature;
        while ( renderer )
        {
          error = renderer->render( renderer, slot, render_mode, NULL );
          if ( !error ||
               FT_ERROR_BASE( error ) != FT_Err_Cannot_Render_Glyph )
            break;

          /* FT_Err_Cannot_Render_Glyph is returned if the render mode   */
          /* is unsupported by the current renderer for this glyph image */
          /* format.                                                     */

          /* now, look for another renderer that supports the same */
          /* format.                                               */
          renderer = FT_Lookup_Renderer( library, slot->format, &node );
          update   = 1;
        }

        /* if we changed the current renderer for the glyph image format */
        /* we need to select it as the next current one                  */
        if ( !error && update && renderer )
          FT_Set_Renderer( library, renderer, 0, 0 );
      }
    }

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Render_Glyph( FT_GlyphSlot    slot,
                   FT_Render_Mode  render_mode )
  {
    FT_Library  library;


    if ( !slot )
      return FT_Err_Invalid_Argument;

    library = FT_FACE_LIBRARY( slot->face );

    return FT_Render_Glyph_Internal( library, slot, render_mode );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                         M O D U L E S                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Destroy_Module                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given module object.  For drivers, this also destroys   */
  /*    all child faces.                                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*     module :: A handle to the target driver object.                   */
  /*                                                                       */
  /* <Note>                                                                */
  /*     The driver _must_ be LOCKED!                                      */
  /*                                                                       */
  static void
  Destroy_Module( FT_Module  module )
  {
    FT_Memory         memory  = module->memory;
    FT_Module_Class*  clazz   = module->clazz;
    FT_Library        library = module->library;


    /* finalize client-data - before anything else */
    if ( module->generic.finalizer )
      module->generic.finalizer( module );

    if ( library && library->auto_hinter == module )
      library->auto_hinter = 0;

    /* if the module is a renderer */
    if ( FT_MODULE_IS_RENDERER( module ) )
      ft_remove_renderer( module );

    /* if the module is a font driver, add some steps */
    if ( FT_MODULE_IS_DRIVER( module ) )
      Destroy_Driver( FT_DRIVER( module ) );

    /* finalize the module object */
    if ( clazz->module_done )
      clazz->module_done( module );

    /* discard it */
    FT_FREE( module );
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Add_Module( FT_Library              library,
                 const FT_Module_Class*  clazz )
  {
    FT_Error   error;
    FT_Memory  memory;
    FT_Module  module;
    FT_UInt    nn;


#define FREETYPE_VER_FIXED  ( ( (FT_Long)FREETYPE_MAJOR << 16 ) | \
                                FREETYPE_MINOR                  )

    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !clazz )
      return FT_Err_Invalid_Argument;

    /* check freetype version */
    if ( clazz->module_requires > FREETYPE_VER_FIXED )
      return FT_Err_Invalid_Version;

    /* look for a module with the same name in the library's table */
    for ( nn = 0; nn < library->num_modules; nn++ )
    {
      module = library->modules[nn];
      if ( ft_strcmp( module->clazz->module_name, clazz->module_name ) == 0 )
      {
        /* this installed module has the same name, compare their versions */
        if ( clazz->module_version <= module->clazz->module_version )
          return FT_Err_Lower_Module_Version;

        /* remove the module from our list, then exit the loop to replace */
        /* it by our new version..                                        */
        FT_Remove_Module( library, module );
        break;
      }
    }

    memory = library->memory;
    error  = FT_Err_Ok;

    if ( library->num_modules >= FT_MAX_MODULES )
    {
      error = FT_Err_Too_Many_Drivers;
      goto Exit;
    }

    /* allocate module object */
    if ( FT_ALLOC( module, clazz->module_size ) )
      goto Exit;

    /* base initialization */
    module->library = library;
    module->memory  = memory;
    module->clazz   = (FT_Module_Class*)clazz;

    /* check whether the module is a renderer - this must be performed */
    /* before the normal module initialization                         */
    if ( FT_MODULE_IS_RENDERER( module ) )
    {
      /* add to the renderers list */
      error = ft_add_renderer( module );
      if ( error )
        goto Fail;
    }

    /* is the module a auto-hinter? */
    if ( FT_MODULE_IS_HINTER( module ) )
      library->auto_hinter = module;

    /* if the module is a font driver */
    if ( FT_MODULE_IS_DRIVER( module ) )
    {
      /* allocate glyph loader if needed */
      FT_Driver  driver = FT_DRIVER( module );


      driver->clazz = (FT_Driver_Class)module->clazz;
      if ( FT_DRIVER_USES_OUTLINES( driver ) )
      {
        error = FT_GlyphLoader_New( memory, &driver->glyph_loader );
        if ( error )
          goto Fail;
      }
    }

    if ( clazz->module_init )
    {
      error = clazz->module_init( module );
      if ( error )
        goto Fail;
    }

    /* add module to the library's table */
    library->modules[library->num_modules++] = module;

  Exit:
    return error;

  Fail:
    if ( FT_MODULE_IS_DRIVER( module ) )
    {
      FT_Driver  driver = FT_DRIVER( module );


      if ( FT_DRIVER_USES_OUTLINES( driver ) )
        FT_GlyphLoader_Done( driver->glyph_loader );
    }

    if ( FT_MODULE_IS_RENDERER( module ) )
    {
      FT_Renderer  renderer = FT_RENDERER( module );


      if ( renderer->raster )
        renderer->clazz->raster_class->raster_done( renderer->raster );
    }

    FT_FREE( module );
    goto Exit;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Module )
  FT_Get_Module( FT_Library   library,
                 const char*  module_name )
  {
    FT_Module   result = 0;
    FT_Module*  cur;
    FT_Module*  limit;


    if ( !library || !module_name )
      return result;

    cur   = library->modules;
    limit = cur + library->num_modules;

    for ( ; cur < limit; cur++ )
      if ( ft_strcmp( cur[0]->clazz->module_name, module_name ) == 0 )
      {
        result = cur[0];
        break;
      }

    return result;
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( const void* )
  FT_Get_Module_Interface( FT_Library   library,
                           const char*  mod_name )
  {
    FT_Module  module;


    /* test for valid `library' delayed to FT_Get_Module() */

    module = FT_Get_Module( library, mod_name );

    return module ? module->clazz->module_interface : 0;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Remove_Module( FT_Library  library,
                    FT_Module   module )
  {
    /* try to find the module from the table, then remove it from there */

    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( module )
    {
      FT_Module*  cur   = library->modules;
      FT_Module*  limit = cur + library->num_modules;


      for ( ; cur < limit; cur++ )
      {
        if ( cur[0] == module )
        {
          /* remove it from the table */
          library->num_modules--;
          limit--;
          while ( cur < limit )
          {
            cur[0] = cur[1];
            cur++;
          }
          limit[0] = 0;

          /* destroy the module */
          Destroy_Module( module );

          return FT_Err_Ok;
        }
      }
    }
    return FT_Err_Invalid_Driver_Handle;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                         L I B R A R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Library( FT_Memory    memory,
                  FT_Library  *alibrary )
  {
    FT_Library  library = 0;
    FT_Error    error;


    if ( !memory )
      return FT_Err_Invalid_Argument;

#ifdef FT_DEBUG_LEVEL_ERROR
    /* init debugging support */
    ft_debug_init();
#endif

    /* first of all, allocate the library object */
    if ( FT_NEW( library ) )
      return error;

    library->memory = memory;

    /* allocate the render pool */
    library->raster_pool_size = FT_RENDER_POOL_SIZE;
    if ( FT_ALLOC( library->raster_pool, FT_RENDER_POOL_SIZE ) )
      goto Fail;

    /* That's ok now */
    *alibrary = library;

    return FT_Err_Ok;

  Fail:
    FT_FREE( library );
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( void )
  FT_Library_Version( FT_Library   library,
                      FT_Int      *amajor,
                      FT_Int      *aminor,
                      FT_Int      *apatch )
  {
    FT_Int  major = 0;
    FT_Int  minor = 0;
    FT_Int  patch = 0;


    if ( library )
    {
      major = library->version_major;
      minor = library->version_minor;
      patch = library->version_patch;
    }

    if ( amajor )
      *amajor = major;

    if ( aminor )
      *aminor = minor;

    if ( apatch )
      *apatch = patch;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_Library( FT_Library  library )
  {
    FT_Memory  memory;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    memory = library->memory;

    /* Discard client-data */
    if ( library->generic.finalizer )
      library->generic.finalizer( library );

    /* Close all modules in the library */
#if 1
    while ( library->num_modules > 0 )
      FT_Remove_Module( library, library->modules[0] );
#else
    {
      FT_UInt  n;


      for ( n = 0; n < library->num_modules; n++ )
      {
        FT_Module  module = library->modules[n];


        if ( module )
        {
          Destroy_Module( module );
          library->modules[n] = 0;
        }
      }
    }
#endif

    /* Destroy raster objects */
    FT_FREE( library->raster_pool );
    library->raster_pool_size = 0;

    FT_FREE( library );
    return FT_Err_Ok;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( void )
  FT_Set_Debug_Hook( FT_Library         library,
                     FT_UInt            hook_index,
                     FT_DebugHook_Func  debug_hook )
  {
    if ( library && debug_hook &&
         hook_index <
           ( sizeof ( library->debug_hooks ) / sizeof ( void* ) ) )
      library->debug_hooks[hook_index] = debug_hook;
  }


/* END */
