/***************************************************************************/
/*                                                                         */
/*  ftobjs.c                                                               */
/*                                                                         */
/*    The FreeType private base classes (body).                            */
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


#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftlist.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>

#include <freetype/tttables.h>

#include <string.h>     /* for strcmp() */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           M E M O R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_memory


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocates a new block of memory.  The returned area is always      */
  /*    zero-filled; this is a strong convention in many FreeType parts.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' which handles        */
  /*              allocation.                                              */
  /*                                                                       */
  /*    size   :: The size in bytes of the block to allocate.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    P      :: A pointer to the fresh new block.  It should be set to   */
  /*              NULL if `size' is 0, or in case of error.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  BASE_FUNC( FT_Error )  FT_Alloc( FT_Memory  memory,
                                   FT_Long    size,
                                   void**     P )
  {
    FT_Assert( P != 0 );

    if ( size > 0 )
    {
      *P = memory->alloc( memory, size );
      if ( !*P )
      {
        FT_ERROR(( "FT_Alloc:" ));
        FT_ERROR(( " Out of memory? (%ld requested)\n",
                   size ));

        return FT_Err_Out_Of_Memory;
      }
      MEM_Set( *P, 0, size );
    }
    else
      *P = NULL;

    FT_TRACE7(( "FT_Alloc:" ));
    FT_TRACE7(( " size = %ld, block = 0x%08p, ref = 0x%08p\n",
                size, *P, P ));

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Reallocates a block of memory pointed to by `*P' to `Size' bytes   */
  /*    from the heap, possibly changing `*P'.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A handle to a given `memory object' which handles       */
  /*               reallocation.                                           */
  /*                                                                       */
  /*    current :: The current block size in bytes.                        */
  /*                                                                       */
  /*    size    :: The new block size in bytes.                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    P       :: A pointer to the fresh new block.  It should be set to  */
  /*               NULL if `size' is 0, or in case of error.               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    All callers of FT_Realloc() _must_ provide the current block size  */
  /*    as well as the new one.                                            */
  /*                                                                       */
  BASE_FUNC( FT_Error )  FT_Realloc( FT_Memory  memory,
                                     FT_Long    current,
                                     FT_Long    size,
                                     void**     P )
  {
    void*  Q;


    FT_Assert( P != 0 );

    /* if the original pointer is NULL, call FT_Alloc() */
    if ( !*P )
      return FT_Alloc( memory, size, P );

    /* if the new block if zero-sized, clear the current one */
    if ( size <= 0 )
    {
      FT_Free( memory, P );
      return FT_Err_Ok;
    }

    Q = memory->realloc( memory, current, size, *P );
    if ( !Q )
      goto Fail;

    *P = Q;
    return FT_Err_Ok;

  Fail:
    FT_ERROR(( "FT_Realloc:" ));
    FT_ERROR(( " Failed (current %ld, requested %ld)\n",
               current, size ));
    return FT_Err_Out_Of_Memory;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases a given block of memory allocated through FT_Alloc().     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' which handles        */
  /*              memory deallocation                                      */
  /*                                                                       */
  /*    P      :: This is the _address_ of a _pointer_ which points to the */
  /*              allocated block.  It is always set to NULL on exit.      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If P or *P are NULL, this function should return successfully.     */
  /*    This is a strong convention within all of FreeType and its         */
  /*    drivers.                                                           */
  /*                                                                       */
  BASE_FUNC( void )  FT_Free( FT_Memory  memory,
                              void**     P )
  {
    FT_TRACE7(( "FT_Free:" ));
    FT_TRACE7(( " Freeing block 0x%08p, ref 0x%08p\n",
                P, P ? *P : (void*)0 ));

    if ( P && *P )
    {
      memory->free( memory, *P );
      *P = 0;
    }
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_new_input_stream                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new input stream object from an FT_Open_Args structure.  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function expects a valid `astream' parameter.                  */
  /*                                                                       */
  static
  FT_Error  ft_new_input_stream( FT_Library     library,
                                 FT_Open_Args*  args,
                                 FT_Stream*     astream )
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
    if ( ALLOC( stream, sizeof ( *stream ) ) )
      goto Exit;

    stream->memory = memory;

    /* now, look at the stream flags */
    if ( args->flags & ft_open_memory )
    {
      error = 0;
      FT_New_Memory_Stream( library,
                            args->memory_base,
                            args->memory_size,
                            stream );
    }
    else if ( args->flags & ft_open_pathname )
    {
      error = FT_New_Stream( args->pathname, stream );
      stream->pathname.pointer = args->pathname;
    }
    else if ( args->flags & ft_open_stream && args->stream )
    {
      *stream        = *(args->stream);
      stream->memory = memory;
    }
    else
      error = FT_Err_Invalid_Argument;

    if ( error )
      FREE( stream );

    *astream = stream;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Stream                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Closes and destroys a stream object.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: The stream to be closed and destroyed.                   */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Done_Stream( FT_Stream  stream )
  {
    if ( stream && stream->close )
      stream->close( stream );
  }


  static
  void  ft_done_stream( FT_Stream*  astream )
  {
    FT_Stream  stream = *astream;
    FT_Memory  memory = stream->memory;


    if ( stream->close )
      stream->close( stream );

    FREE( stream );
    *astream = 0;
  }


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_objs


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                     G L Y P H   L O A D E R                     ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The glyph loader is a simple object which is used to load a set of    */
  /* glyphs easily.  It is critical for the correct loading of composites. */
  /*                                                                       */
  /* Ideally, one can see it as a stack of abstract `glyph' objects.       */
  /*                                                                       */
  /*   loader.base     Is really the bottom of the stack.  It describes a  */
  /*                   single glyph image made of the juxtaposition of     */
  /*                   several glyphs (those `in the stack').              */
  /*                                                                       */
  /*   loader.current  Describes the top of the stack, on which a new      */
  /*                   glyph can be loaded.                                */
  /*                                                                       */
  /*   Rewind          Clears the stack.                                   */
  /*   Prepare         Set up `loader.current' for addition of a new glyph */
  /*                   image.                                              */
  /*   Add             Add the `current' glyph image to the `base' one,    */
  /*                   and prepare for another one.                        */
  /*                                                                       */
  /* The glyph loader is now a base object.  Each driver used to           */
  /* re-implement it in one way or the other, which wasted code and        */
  /* energy.                                                               */
  /*                                                                       */
  /*************************************************************************/


  /* create a new glyph loader */
  BASE_FUNC( FT_Error )  FT_GlyphLoader_New( FT_Memory         memory,
                                             FT_GlyphLoader**  aloader )
  {
    FT_GlyphLoader*  loader;
    FT_Error         error;


    if ( !ALLOC( loader, sizeof ( *loader ) ) )
    {
      loader->memory = memory;
      *aloader       = loader;
    }
    return error;
  }


  /* rewind the glyph loader - reset counters to 0 */
  BASE_FUNC( void )  FT_GlyphLoader_Rewind( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad*  base    = &loader->base;
    FT_GlyphLoad*  current = &loader->current;


    base->outline.n_points   = 0;
    base->outline.n_contours = 0;
    base->num_subglyphs      = 0;

    *current = *base;
  }


  /* reset the glyph loader, frees all allocated tables */
  /* and starts from zero                               */
  BASE_FUNC( void )  FT_GlyphLoader_Reset( FT_GlyphLoader*  loader )
  {
    FT_Memory memory = loader->memory;


    FREE( loader->base.outline.points );
    FREE( loader->base.outline.tags );
    FREE( loader->base.outline.contours );
    FREE( loader->base.extra_points );
    FREE( loader->base.subglyphs );

    loader->max_points    = 0;
    loader->max_contours  = 0;
    loader->max_subglyphs = 0;

    FT_GlyphLoader_Rewind( loader );
  }


  /* delete a glyph loader */
  BASE_FUNC( void )  FT_GlyphLoader_Done( FT_GlyphLoader*  loader )
  {
    if ( loader )
    {
      FT_Memory memory = loader->memory;


      FT_GlyphLoader_Reset( loader );
      FREE( loader );
    }
  }


  /* re-adjust the `current' outline fields */
  static
  void  FT_GlyphLoader_Adjust_Points( FT_GlyphLoader*  loader )
  {
    FT_Outline*  base    = &loader->base.outline;
    FT_Outline*  current = &loader->current.outline;


    current->points   = base->points   + base->n_points;
    current->tags     = base->tags     + base->n_points;
    current->contours = base->contours + base->n_contours;

    /* handle extra points table - if any */
    if ( loader->use_extra )
      loader->current.extra_points =
        loader->base.extra_points + base->n_points;
  }


  BASE_FUNC( FT_Error )  FT_GlyphLoader_Create_Extra(
                           FT_GlyphLoader*  loader )
  {
    FT_Error   error;
    FT_Memory  memory = loader->memory;


    if ( !ALLOC_ARRAY( loader->base.extra_points,
                       loader->max_points, FT_Vector ) )
    {
      loader->use_extra = 1;
      FT_GlyphLoader_Adjust_Points( loader );
    }
    return error;
  }


  /* re-adjust the `current' subglyphs field */
  static
  void  FT_GlyphLoader_Adjust_Subglyphs( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad* base    = &loader->base;
    FT_GlyphLoad* current = &loader->current;


    current->subglyphs = base->subglyphs + base->num_subglyphs;
  }


  /* Ensure that we can add `n_points' and `n_contours' to our glyph. this */
  /* function reallocates its outline tables if necessary.  Note that it   */
  /* DOESN'T change the number of points within the loader!                */
  /*                                                                       */
  BASE_FUNC( FT_Error ) FT_GlyphLoader_Check_Points(
                          FT_GlyphLoader*  loader,
                          FT_UInt          n_points,
                          FT_UInt          n_contours )
  {
    FT_Memory    memory  = loader->memory;
    FT_Error     error   = FT_Err_Ok;
    FT_Outline*  base    = &loader->base.outline;
    FT_Outline*  current = &loader->current.outline;
    FT_Bool      adjust  = 1;

    FT_UInt      new_max;


    /* check points & tags */
    new_max = base->n_points + current->n_points + n_points;
    if ( new_max > loader->max_points )
    {
      new_max = ( new_max + 7 ) & -8;
      if ( REALLOC_ARRAY( base->points, base->n_points,
                          new_max, FT_Vector )          ||
           REALLOC_ARRAY( base->tags, base->n_points,
                          new_max, FT_Byte   )          )
       goto Exit;

      if ( loader->use_extra &&
           REALLOC_ARRAY( loader->base.extra_points, base->n_points,
                          new_max, FT_Vector ) )
       goto Exit;

      adjust = 1;
      loader->max_points = new_max;
    }

    /* check contours */
    new_max = base->n_contours + current->n_contours +
              n_contours;
    if ( new_max > loader->max_contours )
    {
      new_max = ( new_max + 3 ) & -4;
      if ( REALLOC_ARRAY( base->contours, base->n_contours,
                          new_max, FT_Short ) )
        goto Exit;

      adjust = 1;
      loader->max_contours = new_max;
    }

    if ( adjust )
      FT_GlyphLoader_Adjust_Points( loader );

  Exit:
    return error;
  }


  /* Ensure that we can add `n_subglyphs' to our glyph. this function */
  /* reallocates its subglyphs table if necessary.  Note that it DOES */
  /* NOT change the number of subglyphs within the loader!            */
  /*                                                                  */
  BASE_FUNC( FT_Error )  FT_GlyphLoader_Check_Subglyphs(
                           FT_GlyphLoader*  loader,
                           FT_UInt          n_subs )
  {
    FT_Memory  memory = loader->memory;
    FT_Error   error  = FT_Err_Ok;
    FT_UInt    new_max;

    FT_GlyphLoad*  base    = &loader->base;
    FT_GlyphLoad*  current = &loader->current;


    new_max = base->num_subglyphs + current->num_subglyphs + n_subs;
    if ( new_max > loader->max_subglyphs )
    {
      new_max = ( new_max + 1 ) & -2;
      if ( REALLOC_ARRAY( base->subglyphs, base->num_subglyphs,
                          new_max, FT_SubGlyph ) )
        goto Exit;

      loader->max_subglyphs = new_max;

      FT_GlyphLoader_Adjust_Subglyphs( loader );
    }

  Exit:
    return error;
  }


  /* prepare loader for the addition of a new glyph on top of the base one */
  BASE_FUNC( void )  FT_GlyphLoader_Prepare( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad*  current = &loader->current;


    current->outline.n_points   = 0;
    current->outline.n_contours = 0;
    current->num_subglyphs      = 0;

    FT_GlyphLoader_Adjust_Points   ( loader );
    FT_GlyphLoader_Adjust_Subglyphs( loader );
  }


  /* add current glyph to the base image - and prepare for another */
  BASE_FUNC( void )  FT_GlyphLoader_Add( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad*  base    = &loader->base;
    FT_GlyphLoad*  current = &loader->current;

    FT_UInt        n_curr_contours = current->outline.n_contours;
    FT_UInt        n_base_points   = base->outline.n_points;
    FT_UInt        n;


    base->outline.n_points   += current->outline.n_points;
    base->outline.n_contours += current->outline.n_contours;
    base->num_subglyphs      += current->num_subglyphs;

    /* adjust contours count in newest outline */
    for ( n = 0; n < n_curr_contours; n++ )
      current->outline.contours[n] += n_base_points;

    /* prepare for another new glyph image */
    FT_GlyphLoader_Prepare( loader );
  }


  BASE_FUNC( FT_Error )  FT_GlyphLoader_Copy_Points( FT_GlyphLoader*  target,
                                                     FT_GlyphLoader*  source )
  {
    FT_Error  error;
    FT_UInt   num_points   = source->base.outline.n_points;
    FT_UInt   num_contours = source->base.outline.n_contours;


    error = FT_GlyphLoader_Check_Points( target, num_points, num_contours );
    if ( !error )
    {
      FT_Outline*  out = &target->base.outline;
      FT_Outline*  in  = &source->base.outline;


      MEM_Copy( out->points, in->points,
                num_points * sizeof ( FT_Vector ) );
      MEM_Copy( out->tags, in->tags,
                num_points * sizeof ( char ) );
      MEM_Copy( out->contours, in->contours,
                num_contours * sizeof ( short ) );

      /* do we need to copy the extra points? */
      if ( target->use_extra && source->use_extra )
        MEM_Copy( target->base.extra_points, source->base.extra_points,
                  num_points * sizeof ( FT_Vector ) );

      out->n_points   = num_points;
      out->n_contours = num_contours;

      FT_GlyphLoader_Adjust_Points( target );
    }

    return error;
  }


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


  static
  FT_Error  ft_glyphslot_init( FT_GlyphSlot  slot )
  {
    FT_Driver         driver = slot->face->driver;
    FT_Driver_Class*  clazz  = driver->clazz;
    FT_Memory         memory = driver->root.memory;
    FT_Error          error  = FT_Err_Ok;


    slot->library = driver->root.library;

    if ( FT_DRIVER_USES_OUTLINES( driver ) )
      error = FT_GlyphLoader_New( memory, &slot->loader );

    if ( !error && clazz->init_slot )
      error = clazz->init_slot( slot );

    return error;
  }


  static
  void  ft_glyphslot_clear( FT_GlyphSlot  slot )
  {
    /* free bitmap if needed */
    if ( slot->flags & ft_glyph_own_bitmap )
    {
      FT_Memory  memory = FT_FACE_MEMORY( slot->face );


      FREE( slot->bitmap.buffer );
      slot->flags &= ~ft_glyph_own_bitmap;
    }

    /* clear all public fields in the glyph slot */
    MEM_Set( &slot->metrics, 0, sizeof ( slot->metrics ) );
    MEM_Set( &slot->outline, 0, sizeof ( slot->outline ) );
    MEM_Set( &slot->bitmap,  0, sizeof ( slot->bitmap )  );

    slot->bitmap_left   = 0;
    slot->bitmap_top    = 0;
    slot->num_subglyphs = 0;
    slot->subglyphs     = 0;
    slot->control_data  = 0;
    slot->control_len   = 0;
    slot->other         = 0;
    slot->format        = ft_glyph_format_none;

    slot->linearHoriAdvance = 0;
    slot->linearVertAdvance = 0;
  }


  static
  void  ft_glyphslot_done( FT_GlyphSlot  slot )
  {
    FT_Driver         driver = slot->face->driver;
    FT_Driver_Class*  clazz  = driver->clazz;
    FT_Memory         memory = driver->root.memory;


    /* free bitmap buffer if needed */
    if ( slot->flags & ft_glyph_own_bitmap )
      FREE( slot->bitmap.buffer );

    /* free glyph loader */
    if ( FT_DRIVER_USES_OUTLINES( driver ) )
    {
      FT_GlyphLoader_Done( slot->loader );
      slot->loader = 0;
    }

    if ( clazz->done_slot )
      clazz->done_slot( slot );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_GlyphSlot                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    It is sometimes useful to have more than one glyph slot for a      */
  /*    given face object.  This function is used to create additional     */
  /*    slots.  All of them are automatically discarded when the face is   */
  /*    destroyed.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face  :: A handle to a parent face object.                         */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aslot :: A handle to a new glyph slot object.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_New_GlyphSlot( FT_Face        face,
                                                FT_GlyphSlot*  aslot )
  {
    FT_Error          error;
    FT_Driver         driver;
    FT_Driver_Class*  clazz;
    FT_Memory         memory;
    FT_GlyphSlot      slot;


    if ( !face || !aslot || !face->driver )
      return FT_Err_Invalid_Argument;

    *aslot = 0;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = driver->root.memory;

    FT_TRACE4(( "FT_New_GlyphSlot: Creating new slot object\n" ));
    if ( !ALLOC( slot, clazz->slot_object_size ) )
    {
      slot->face = face;

      error = ft_glyphslot_init( slot );
      if ( error )
      {
        ft_glyphslot_done( slot );
        FREE( slot );
        goto Exit;
      }

      *aslot = slot;
    }

  Exit:
    FT_TRACE4(( "FT_New_GlyphSlot: Return %d\n", error ));
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_GlyphSlot                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given glyph slot.  Remember however that all slots are  */
  /*    automatically destroyed with its parent.  Using this function is   */
  /*    not always mandatory.                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot :: A handle to a target glyph slot.                           */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Done_GlyphSlot( FT_GlyphSlot  slot )
  {
    if ( slot )
    {
      FT_Driver         driver = slot->face->driver;
      FT_Memory         memory = driver->root.memory;
      FT_GlyphSlot*     parent;
      FT_GlyphSlot      cur;


      /* Remove slot from its parent face's list */
      parent = &slot->face->glyph;
      cur    = *parent;

      while ( cur )
      {
        if ( cur == slot )
        {
          *parent = cur->next;
          ft_glyphslot_done( slot );
          FREE( slot );
          break;
        }
        cur = cur->next;
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Transform                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to set the transformation that is applied to glyph */
  /*    images just before they are converted to bitmaps in a glyph slot   */
  /*    when FT_Render_Glyph() is called.                                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the source face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    matrix :: A pointer to the transformation's 2x2 matrix.  Use 0 for */
  /*              the identity matrix.                                     */
  /*    delta  :: A pointer to the translation vector.  Use 0 for the null */
  /*              vector.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The transformation is only applied to scalable image formats after */
  /*    the glyph has been loaded.  It means that hinting is unaltered by  */
  /*    the transformation and is performed on the character size given in */
  /*    the last call to FT_Set_Char_Sizes() or FT_Set_Pixel_Sizes().      */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Set_Transform( FT_Face     face,
                                            FT_Matrix*  matrix,
                                            FT_Vector*  delta )
  {
    if ( !face )
      return;

    face->transform_flags = 0;

    if ( !matrix )
    {
      face->transform_matrix.xx = 0x10000L;
      face->transform_matrix.xy = 0;
      face->transform_matrix.yx = 0;
      face->transform_matrix.yy = 0x10000L;
      matrix = &face->transform_matrix;
    }
    else
      face->transform_matrix = *matrix;

    /* set transform_flags bit flag 0 if `matrix' isn't the identity */
    if ( ( matrix->xy | matrix->yx ) ||
         matrix->xx != 0x10000L      ||
         matrix->yy != 0x10000L      )
      face->transform_flags |= 1;

    if ( !delta )
    {
      face->transform_delta.x = 0;
      face->transform_delta.y = 0;
      delta = &face->transform_delta;
    }
    else
      face->transform_delta = *delta;

    /* set transform_flags bit flag 1 if `delta' isn't the null vector */
    if ( delta->x | delta->y )
      face->transform_flags |= 2;
  }


  static FT_Renderer  ft_lookup_glyph_renderer( FT_GlyphSlot  slot );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Load_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to the target face object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the glyph image is not a bitmap, and if the bit flag            */
  /*    FT_LOAD_IGNORE_TRANSFORM is unset, the glyph image will be         */
  /*    transformed with the information passed to a previous call to      */
  /*    FT_Set_Transform.                                                  */
  /*                                                                       */
  /*    Note that this also transforms the `face.glyph.advance' field, but */
  /*    *not* the values in `face.glyph.metrics'.                          */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Load_Glyph( FT_Face  face,
                                             FT_UInt  glyph_index,
                                             FT_Int   load_flags )
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

    /* when the flag NO_RECURSE is set, we disable hinting and scaling */
    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    /* do we need to load the glyph through the auto-hinter? */
    library  = driver->root.library;
    hinter   = library->auto_hinter;
    autohint = hinter &&
               !( load_flags & ( FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING ) );
    if ( autohint )
    {
      if ( FT_DRIVER_HAS_HINTER( driver ) &&
           !( load_flags & FT_LOAD_FORCE_AUTOHINT ) )
        autohint = 0;
    }

    if ( autohint )
    {
      FT_AutoHinter_Interface*  hinting;


      hinting = (FT_AutoHinter_Interface*)hinter->clazz->module_interface;
      error = hinting->load_glyph( (FT_AutoHinter)hinter, slot, face->size,
                                   glyph_index, load_flags );
    }
    else
      error = driver->clazz->load_glyph( slot,
                                         face->size,
                                         glyph_index,
                                         load_flags );
    if ( error )
      goto Exit;

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

    /* now, transform the glyph image when needed */
    if ( face->transform_flags )
    {
      /* get renderer */
      FT_Renderer  renderer = ft_lookup_glyph_renderer( slot );


      if ( renderer )
        error = renderer->clazz->transform_glyph( renderer, slot,
                                                  &face->transform_matrix,
                                                  &face->transform_delta );
      /* transform advance */
      FT_Vector_Transform( &slot->advance, &face->transform_matrix );
    }

    /* do we need to render the image now? */
    if ( !error                                    &&
         slot->format != ft_glyph_format_bitmap    &&
         slot->format != ft_glyph_format_composite &&
         load_flags & FT_LOAD_RENDER )
    {
      error = FT_Render_Glyph( slot,
                               ( load_flags & FT_LOAD_MONOCHROME )
                                  ? ft_render_mode_mono
                                  : ft_render_mode_normal );
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Load_Char                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size, according to its character code.                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a target face object where the glyph    */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    char_code   :: The glyph's character code, according to the        */
  /*                   current charmap used in the face.                   */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the face has no current charmap, or if the character code       */
  /*    is not defined in the charmap, this function will return an        */
  /*    error.                                                             */
  /*                                                                       */
  /*    If the glyph image is not a bitmap, and if the bit flag            */
  /*    FT_LOAD_IGNORE_TRANSFORM is unset, the glyph image will be         */
  /*    transformed with the information passed to a previous call to      */
  /*    FT_Set_Transform().                                                */
  /*                                                                       */
  /*    Note that this also transforms the `face.glyph.advance' field, but */
  /*    *not* the values in `face.glyph.metrics'.                          */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Load_Char( FT_Face   face,
                                            FT_ULong  char_code,
                                            FT_Int    load_flags )
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
  static
  void  destroy_size( FT_Memory  memory,
                      FT_Size    size,
                      FT_Driver  driver )
  {
    /* finalize client-specific data */
    if ( size->generic.finalizer )
      size->generic.finalizer( size );

    /* finalize format-specific stuff */
    if ( driver->clazz->done_size )
      driver->clazz->done_size( size );

    FREE( size );
  }


  /* destructor for faces list */
  static
  void  destroy_face( FT_Memory  memory,
                      FT_Face    face,
                      FT_Driver  driver )
  {
    FT_Driver_Class*  clazz = driver->clazz;


    /* discard auto-hinting data */
    if ( face->autohint.finalizer )
      face->autohint.finalizer( face->autohint.data );
      
    /* Discard glyph slots for this face                           */
    /* Beware!  FT_Done_GlyphSlot() changes the field `face->slot' */
    while ( face->glyph )
      FT_Done_GlyphSlot( face->glyph );

    /* Discard all sizes for this face */
    FT_List_Finalize( &face->sizes_list,
                     (FT_List_Destructor)destroy_size,
                     memory,
                     driver );
    face->size = 0;

    /* Now discard client data */
    if ( face->generic.finalizer )
      face->generic.finalizer( face );

    /* finalize format-specific stuff */
    if ( clazz->done_face )
      clazz->done_face( face );

    /* close the stream for this face if needed */
    if ( ( face->face_flags & FT_FACE_FLAG_EXTERNAL_STREAM ) == 0 )
      ft_done_stream( &face->stream );

    /* get rid of it */
    FREE( face );
  }


  static
  void  Destroy_Driver( FT_Driver  driver )
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
  /*    open_face                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function does some work for FT_Open_Face().                   */
  /*                                                                       */
  static
  FT_Error  open_face( FT_Driver      driver,
                       FT_Stream      stream,
                       FT_Long        face_index,
                       FT_Int         num_params,
                       FT_Parameter*  params,
                       FT_Face*       aface )
  {
    FT_Memory         memory;
    FT_Driver_Class*  clazz;
    FT_Face           face = 0;
    FT_Error          error;


    clazz  = driver->clazz;
    memory = driver->root.memory;

    /* allocate the face object and perform basic initialization */
    if ( ALLOC( face, clazz->face_object_size ) )
      goto Fail;

    face->driver = driver;
    face->memory = memory;
    face->stream = stream;

    error = clazz->init_face( stream,
                              face,
                              face_index,
                              num_params,
                              params );
    if ( error )
      goto Fail;

    *aface = face;

  Fail:
    if ( error )
    {
      clazz->done_face( face );
      FREE( face );
      *aface = 0;
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Face                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new face object from a given resource and typeface index */
  /*    using a pathname to the font file.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pathname   :: A path to the font file.                             */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    Note that additional slots can be added to each face with the      */
  /*    FT_New_GlyphSlot() API function.  Slots are linked in a single     */
  /*    list through their `next' field.                                   */
  /*                                                                       */
  /*    FT_New_Face() can be used to determine and/or check the font       */
  /*    format of a given font resource.  If the `face_index' field is     */
  /*    negative, the function will _not_ return any face handle in        */
  /*    `*face'.  Its return value should be 0 if the resource is          */
  /*    recognized, or non-zero if not.                                    */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_New_Face( FT_Library   library,
                                           const char*  pathname,
                                           FT_Long      face_index,
                                           FT_Face*     aface )
  {
    FT_Open_Args  args;


    /* test for valid `library' and `aface' delayed to FT_Open_Face() */
    if ( !pathname )
      return FT_Err_Invalid_Argument;

    args.flags    = ft_open_pathname;
    args.pathname = (char*)pathname;

    return FT_Open_Face( library, &args, face_index, aface );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Memory_Face                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new face object from a given resource and typeface index */
  /*    using a font file already loaded into memory.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    file_base  :: A pointer to the beginning of the font data.         */
  /*                                                                       */
  /*    file_size  :: The size of the memory chunk used by the font data.  */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /* <Output>                                                              */
  /*    face       :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    Note that additional slots can be added to each face with the      */
  /*    FT_New_GlyphSlot() API function.  Slots are linked in a single     */
  /*    list through their `next' field.                                   */
  /*                                                                       */
  /*    FT_New_Memory_Face() can be used to determine and/or check the     */
  /*    font format of a given font resource.  If the `face_index' field   */
  /*    is negative, the function will _not_ return any face handle in     */
  /*    `*face'.  Its return value should be 0 if the resource is          */
  /*    recognized, or non-zero if not.                                    */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_New_Memory_Face( FT_Library  library,
                                                  FT_Byte*    file_base,
                                                  FT_Long     file_size,
                                                  FT_Long     face_index,
                                                  FT_Face*    face )
  {
    FT_Open_Args  args;


    /* test for valid `library' and `face' delayed to FT_Open_Face() */
    if ( !file_base )
      return FT_Err_Invalid_Argument;

    args.flags       = ft_open_memory;
    args.memory_base = file_base;
    args.memory_size = file_size;

    return FT_Open_Face( library, &args, face_index, face );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Open_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Opens a face object from a given resource and typeface index using */
  /*    an `FT_Open_Args' structure.  If the face object doesn't exist, it */
  /*    will be created.                                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    args       :: A pointer to an `FT_Open_Args' structure which must  */
  /*                  be filled by the caller.                             */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    Note that additional slots can be added to each face with the      */
  /*    FT_New_GlyphSlot() API function.  Slots are linked in a single     */
  /*    list through their `next' field.                                   */
  /*                                                                       */
  /*    FT_Open_Face() can be used to determine and/or check the font      */
  /*    format of a given font resource.  If the `face_index' field is     */
  /*    negative, the function will _not_ return any face handle in        */
  /*    `*face'.  Its return value should be 0 if the resource is          */
  /*    recognized, or non-zero if not.                                    */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Open_Face( FT_Library     library,
                                            FT_Open_Args*  args,
                                            FT_Long        face_index,
                                            FT_Face*       aface )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_Stream    stream;
    FT_Face      face = 0;
    FT_ListNode  node = 0;


    /* test for valid `library' and `args' delayed to */
    /* ft_new_input_stream()                          */

    if ( !aface )
      return FT_Err_Invalid_Argument;

    *aface = 0;

    /* create input stream */
    error = ft_new_input_stream( library, args, &stream );
    if ( error )
      goto Exit;

    memory = library->memory;

    /* If the font driver is specified in the `args' structure, use */
    /* it.  Otherwise, we scan the list of registered drivers.      */
    if ( args->flags & ft_open_driver && args->driver )
    {
      driver = FT_DRIVER( args->driver );

      /* not all modules are drivers, so check... */
      if ( FT_MODULE_IS_DRIVER( driver ) )
      {
        FT_Int         num_params = 0;
        FT_Parameter*  params     = 0;


        if ( args->flags & ft_open_params )
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

      ft_done_stream( &stream );
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

          if ( args->flags & ft_open_params )
          {
            num_params = args->num_params;
            params     = args->params;
          }

          error = open_face( driver, stream, face_index,
                             num_params, params, &face );
          if ( !error )
            goto Success;

          if ( error != FT_Err_Unknown_File_Format )
            goto Fail;
        }
      }

      ft_done_stream( &stream );

      /* no driver is able to handle this format */
      error = FT_Err_Unknown_File_Format;
      goto Fail;
    }

  Success:
    FT_TRACE4(( "FT_New_Face: New face object, adding to list\n" ));

    /* set the FT_FACE_FLAG_EXTERNAL_STREAM bit for FT_Done_Face */
    if ( args->flags & ft_open_stream && args->stream )
      face->face_flags |= FT_FACE_FLAG_EXTERNAL_STREAM;

    /* add the face object to its driver's list */
    if ( ALLOC( node, sizeof ( *node ) ) )
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

    /* initialize transformation for convenience functions */
    face->transform_matrix.xx = 0x10000L;
    face->transform_matrix.xy = 0;
    face->transform_matrix.yx = 0;
    face->transform_matrix.yy = 0x10000L;

    face->transform_delta.x = 0;
    face->transform_delta.y = 0;

    *aface = face;
    goto Exit;

  Fail:
    FT_Done_Face( face );

  Exit:
    FT_TRACE4(( "FT_Open_Face: Return %d\n", error ));

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Attach_File                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    `Attaches' a given font file to an existing face.  This is usually */
  /*    to read additional information for a single face object.  For      */
  /*    example, it is used to read the AFM files that come with Type 1    */
  /*    fonts in order to add kerning data and other metrics.              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face         :: The target face object.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    filepathname :: An 8-bit pathname naming the `metrics' file.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If your font file is in memory, or if you want to provide your     */
  /*    own input stream object, use FT_Attach_Stream().                   */
  /*                                                                       */
  /*    The meaning of the `attach' action (i.e., what really happens when */
  /*    the new file is read) is not fixed by FreeType itself.  It really  */
  /*    depends on the font format (and thus the font driver).             */
  /*                                                                       */
  /*    Client applications are expected to know what they are doing       */
  /*    when invoking this function.  Most drivers simply do not implement */
  /*    file attachments.                                                  */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Attach_File( FT_Face      face,
                                              const char*  filepathname )
  {
    FT_Open_Args  open;


    /* test for valid `face' delayed to FT_Attach_Stream() */

    if ( !filepathname )
      return FT_Err_Invalid_Argument;

    open.flags    = ft_open_pathname;
    open.pathname = (char*)filepathname;

    return FT_Attach_Stream( face, &open );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Attach_Stream                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is similar to FT_Attach_File() with the exception    */
  /*    that it reads the attachment from an arbitrary stream.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: The target face object.                              */
  /*                                                                       */
  /*    parameters :: A pointer to an FT_Open_Args structure used to       */
  /*                  describe the input stream to FreeType.               */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The meaning of the `attach' (i.e. what really happens when the     */
  /*    new file is read) is not fixed by FreeType itself.  It really      */
  /*    depends on the font format (and thus the font driver).             */
  /*                                                                       */
  /*    Client applications are expected to know what they are doing       */
  /*    when invoking this function.  Most drivers simply do not implement */
  /*    file attachments.                                                  */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Attach_Stream( FT_Face        face,
                                                FT_Open_Args*  parameters )
  {
    FT_Stream  stream;
    FT_Error   error;
    FT_Driver  driver;

    FT_Driver_Class*  clazz;


    /* test for valid `parameters' delayed to ft_new_input_stream() */

    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    if ( !driver )
      return FT_Err_Invalid_Driver_Handle;

    error = ft_new_input_stream( driver->root.library, parameters, &stream );
    if ( error )
      goto Exit;

    /* we implement FT_Attach_Stream in each driver through the */
    /* `attach_file' interface                                  */

    error = FT_Err_Unimplemented_Feature;
    clazz = driver->clazz;
    if ( clazz->attach_file )
      error = clazz->attach_file( face, stream );

    /* close the attached stream */
    if ( !parameters->stream || ( parameters->flags & ft_open_stream ) )
      ft_done_stream( &stream );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given face object, as well as all of its child slots    */
  /*    and sizes.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to a target face object.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Done_Face( FT_Face  face )
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
        FREE( node );

        /* now destroy the object proper */
        destroy_face( memory, face, driver );
        error = FT_Err_Ok;
      }
    }
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Size                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new size object from a given face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to a parent face object.                          */
  /*                                                                       */
  /* <Output>                                                              */
  /*    asize :: A handle to a new size object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_New_Size( FT_Face   face,
                                           FT_Size*  asize )
  {
    FT_Error          error;
    FT_Memory         memory;
    FT_Driver         driver;
    FT_Driver_Class*  clazz;

    FT_Size           size = 0;
    FT_ListNode       node = 0;


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
    if ( ALLOC( size, clazz->size_object_size ) ||
         ALLOC( node, sizeof ( FT_ListNodeRec ) ) )
      goto Exit;

    size->face = face;

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
      FREE( node );
      FREE( size );
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Size                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given size object.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to a target size object.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Done_Size( FT_Size  size )
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
      FREE( node );

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

    return FT_Err_Ok;
  }


  static
  void  ft_recompute_scaled_metrics( FT_Face           face,
                                     FT_Size_Metrics*  metrics )
  {
    /* Compute root ascender, descender, test height, and max_advance */

    metrics->ascender    = ( FT_MulFix( face->ascender,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->descender   = ( FT_MulFix( face->descender,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->height      = ( FT_MulFix( face->height,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->max_advance = ( FT_MulFix( face->max_advance_width,
                                        metrics->x_scale ) + 32 ) & -64;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Char_Size                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the character dimensions of a given face object.  The         */
  /*    `char_width' and `char_height' values are used for the width and   */
  /*    height, respectively, expressed in 26.6 fractional points.         */
  /*                                                                       */
  /*    If the horizontal or vertical resolution values are zero, a        */
  /*    default value of 72dpi is used.  Similarly, if one of the          */
  /*    character dimensions is zero, its value is set equal to the other. */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size            :: A handle to a target size object.               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    char_width      :: The character width, in 26.6 fractional points. */
  /*                                                                       */
  /*    char_height     :: The character height, in 26.6 fractional        */
  /*                       points.                                         */
  /*                                                                       */
  /*    horz_resolution :: The horizontal resolution.                      */
  /*                                                                       */
  /*    vert_resolution :: The vertical resolution.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    When dealing with fixed-size faces (i.e., non-scalable formats),   */
  /*    use the function FT_Set_Pixel_Sizes().                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Set_Char_Size( FT_Face     face,
                                                FT_F26Dot6  char_width,
                                                FT_F26Dot6  char_height,
                                                FT_UInt     horz_resolution,
                                                FT_UInt     vert_resolution )
  {
    FT_Error          error = FT_Err_Ok;
    FT_Driver         driver;
    FT_Memory         memory;
    FT_Driver_Class*  clazz;
    FT_Size_Metrics*  metrics;
    FT_Long           dim_x, dim_y;


    if ( !face || !face->size || !face->driver )
      return FT_Err_Invalid_Face_Handle;

    driver  = face->driver;
    metrics = &face->size->metrics;

    if ( !char_width )
      char_width = char_height;

    else if ( !char_height )
      char_height = char_width;

    if ( !horz_resolution )
      horz_resolution = 72;

    if ( !vert_resolution )
      vert_resolution = 72;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = driver->root.memory;

    /* default processing -- this can be overridden by the driver */
    if ( char_width  < 1 * 64 )
      char_width  = 1 * 64;
    if ( char_height < 1 * 64 )
      char_height = 1 * 64;

    /* Compute pixel sizes in 26.6 units */
    dim_x = ( ( ( char_width  * horz_resolution ) / 72 ) + 32 ) & -64;
    dim_y = ( ( ( char_height * vert_resolution ) / 72 ) + 32 ) & -64;

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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Pixel_Sizes                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the character dimensions of a given face object.  The width   */
  /*    and height are expressed in integer pixels.                        */
  /*                                                                       */
  /*    If one of the character dimensions is zero, its value is set equal */
  /*    to the other.                                                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face         :: A handle to the target face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pixel_width  :: The character width, in integer pixels.            */
  /*                                                                       */
  /*    pixel_height :: The character height, in integer pixels.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Set_Pixel_Sizes( FT_Face  face,
                                                  FT_UInt  pixel_width,
                                                  FT_UInt  pixel_height )
  {
    FT_Error          error = FT_Err_Ok;
    FT_Driver         driver;
    FT_Memory         memory;
    FT_Driver_Class*  clazz;
    FT_Size_Metrics*  metrics = &face->size->metrics;


    if ( !face || !face->size || !face->driver )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = driver->root.memory;

    /* default processing -- this can be overridden by the driver */
    if ( pixel_width == 0 )
      pixel_width = pixel_height;

    else if ( pixel_height == 0 )
      pixel_height = pixel_width;

    if ( pixel_width  < 1 )
      pixel_width  = 1;
    if ( pixel_height < 1 )
      pixel_height = 1;

    metrics->x_ppem = pixel_width;
    metrics->y_ppem = pixel_height;

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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Kerning                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the kerning vector between two glyphs of a same face.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a source face object.                   */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /*    kern_mode   :: See FT_Kerning_Mode() for more information.         */
  /*                   Determines the scale/dimension of the returned      */
  /*                   kerning vector.                                     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    kerning     :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this method.  Other layouts, or more sophisticated    */
  /*    kernings, are out of the scope of this API function -- they can be */
  /*    implemented through format-specific interfaces.                    */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Get_Kerning( FT_Face     face,
                                              FT_UInt     left_glyph,
                                              FT_UInt     right_glyph,
                                              FT_UInt     kern_mode,
                                              FT_Vector*  kerning )
  {
    FT_Error   error = FT_Err_Ok;
    FT_Driver  driver;
    FT_Memory  memory;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    if ( !kerning )
      return FT_Err_Invalid_Argument;

    driver = face->driver;
    memory = driver->root.memory;

    kerning->x = 0;
    kerning->y = 0;

    if ( driver->clazz->get_kerning )
    {
      error = driver->clazz->get_kerning( face,
                                          left_glyph,
                                          right_glyph,
                                          kerning );
      if ( !error )
      {
        if ( kern_mode != ft_kerning_unscaled )
        {
          kerning->x = FT_MulFix( kerning->x, face->size->metrics.x_scale );
          kerning->y = FT_MulFix( kerning->y, face->size->metrics.y_scale );

          if ( kern_mode != ft_kerning_unfitted )
          {
            kerning->x = ( kerning->x + 32 ) & -64;
            kerning->y = ( kerning->y + 32 ) & -64;
          }
        }
      }
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Select_Charmap                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects a given charmap by its encoding tag (as listed in          */
  /*    `freetype.h').                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*                                                                       */
  /*    encoding :: A handle to the selected charmap.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will return an error if no charmap in the face       */
  /*    corresponds to the encoding queried here.                          */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Select_Charmap( FT_Face      face,
                                                 FT_Encoding  encoding )
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
      if ( cur[0]->encoding == encoding )
      {
        face->charmap = cur[0];
        return 0;
      }
    }

    return FT_Err_Invalid_Argument;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Charmap                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects a given charmap for character code to glyph index          */
  /*    decoding.                                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*    charmap  :: A handle to the selected charmap.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will return an error if the charmap is not part of   */
  /*    the face (i.e., if it is not listed in the face->charmaps[]        */
  /*    table).                                                            */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Set_Charmap( FT_Face     face,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Char_Index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the glyph index of a given character code.  This function  */
  /*    uses a charmap object to do the translation.                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*                                                                       */
  /*    charcode :: The character code.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph index.  0 means `undefined character code'.              */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_UInt )  FT_Get_Char_Index( FT_Face   face,
                                                FT_ULong  charcode )
  {
    FT_UInt    result;
    FT_Driver  driver;


    result = 0;
    if ( face && face->charmap )
    {
      driver = face->driver;
      result = driver->clazz->get_char_index( face->charmap, charcode );
    }
    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Glyph_Name                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the ASCII name of a given glyph in a face.  This only    */
  /*    works for those faces where FT_HAS_GLYPH_NAME(face) returns true.  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a source face object.                   */
  /*                                                                       */
  /*    glyph_index :: The glyph index.                                    */
  /*                                                                       */
  /*    buffer      :: A pointer to a target buffer where the name will be */
  /*                   copied to.                                          */
  /*                                                                       */
  /*    buffer_max  :: The maximal number of bytes available in the        */
  /*                   buffer.                                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error is returned if the face doesn't provide glyph names or if */
  /*    the glyph index is invalid.  In all cases of failure, the first    */
  /*    byte of `buffer' will be set to 0 to indicate an empty name.       */
  /*                                                                       */
  /*    The glyph name is truncated to fit within the buffer if it is too  */
  /*    long.  The returned string is always zero-terminated.              */
  /*                                                                       */
  /*    This function is not compiled within the library if the config     */
  /*    macro FT_CONFIG_OPTION_NO_GLYPH_NAMES is defined in                */
  /*    `include/freetype/config/ftoptions.h'                              */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Get_Glyph_Name( FT_Face     face,
                                                 FT_UInt     glyph_index,
                                                 FT_Pointer  buffer,
                                                 FT_UInt     buffer_max )
  {
    FT_Error  error = FT_Err_Invalid_Argument;
    

    /* clean up buffer */
    if ( buffer && buffer_max > 0 )
      ((FT_Byte*)buffer)[0] = 0;
      
    if ( face                                    &&
         glyph_index < (FT_UInt)face->num_glyphs &&
         FT_HAS_GLYPH_NAMES( face )              )
    {
      /* now, lookup for glyph name */
      FT_Driver        driver = face->driver;
      FT_Module_Class* clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_Glyph_Name_Requester  requester;
        

        requester = (FT_Glyph_Name_Requester)clazz->get_interface(
                      FT_MODULE( driver ), "glyph_name" );
        if ( requester )
          error = requester( face, glyph_index, buffer, buffer_max );
      }
    }

    return error;
  }                                                 


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Sfnt_Table                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a pointer to a given SFNT table within a face.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face object.                        */
  /*    tag  :: An index of an SFNT table.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A type-less pointer to the table.  This will be 0 in case of       */
  /*    error, or if the corresponding table was not found *OR* loaded     */
  /*    from the file.                                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The table is owned by the face object, and disappears with it.     */
  /*                                                                       */
  /*    This function is only useful to access SFNT tables that are loaded */
  /*    by the sfnt/truetype/opentype drivers.  See the FT_Sfnt_Tag        */
  /*    enumeration in `tttables.h' for a list.                            */
  /*                                                                       */
  /*    You can load any table with a different function.. XXX             */
  /*                                                                       */
  FT_EXPORT_FUNC( void* )  FT_Get_Sfnt_Table( FT_Face      face,
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
  BASE_FUNC( FT_Renderer )  FT_Lookup_Renderer( FT_Library       library,
                                                FT_Glyph_Format  format,
                                                FT_ListNode*     node )
  {
    FT_ListNode   cur;
    FT_Renderer   result = 0;


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


  static
  FT_Renderer  ft_lookup_glyph_renderer( FT_GlyphSlot  slot )
  {
    FT_Face      face    = slot->face;
    FT_Library   library = FT_FACE_LIBRARY( face );
    FT_Renderer  result  = library->cur_renderer;


    if ( !result || result->glyph_format != slot->format )
      result = FT_Lookup_Renderer( library, slot->format, 0 );

    return result;
  }


  static
  void  ft_set_current_renderer( FT_Library  library )
  {
    FT_Renderer  renderer;


    renderer = FT_Lookup_Renderer( library, ft_glyph_format_outline, 0 );
    library->cur_renderer = renderer;
  }


  static
  FT_Error  ft_add_renderer( FT_Module  module )
  {
    FT_Library   library = module->library;
    FT_Memory    memory  = library->memory;
    FT_Error     error;
    FT_ListNode  node;


    if ( ALLOC( node, sizeof ( *node ) ) )
      goto Exit;

    {
      FT_Renderer         render = FT_RENDERER( module );
      FT_Renderer_Class*  clazz  = (FT_Renderer_Class*)module->clazz;


      render->clazz        = clazz;
      render->glyph_format = clazz->glyph_format;

      /* allocate raster object if needed */
      if ( clazz->glyph_format == ft_glyph_format_outline &&
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
      FREE( node );

  Exit:
    return error;
  }


  static
  void  ft_remove_renderer( FT_Module  module )
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
      FREE( node );

      ft_set_current_renderer( library );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Renderer                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the current renderer for a given glyph format.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the library object.                         */
  /*                                                                       */
  /*    format  :: The glyph format.                                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A renderer handle.  0 if none found.                               */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error will be returned if a module already exists by that name, */
  /*    or if the module requires a version of FreeType that is too great. */
  /*                                                                       */
  /*    To add a new renderer, simply use FT_Add_Module().  To retrieve a  */
  /*    renderer by its name, use FT_Get_Module().                         */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Renderer )  FT_Get_Renderer( FT_Library       library,
                                                  FT_Glyph_Format  format )
  {
    /* test for valid `library' delayed to FT_Lookup_Renderer() */

    return  FT_Lookup_Renderer( library, format, 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Renderer                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the current renderer to use, and set additional mode.         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library    :: A handle to the library object.                      */
  /*                                                                       */
  /*    renderer   :: A handle to the renderer object.                     */
  /*                                                                       */
  /*    num_params :: The number of additional parameters.                 */
  /*                                                                       */
  /*    parameters :: Additional parameters.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    In case of success, the renderer will be used to convert glyph     */
  /*    images in the renderer's known format into bitmaps.                */
  /*                                                                       */
  /*    This doesn't change the current renderer for other formats.        */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Set_Renderer( FT_Library     library,
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

    if ( renderer->glyph_format == ft_glyph_format_outline )
      library->cur_renderer = renderer;

    if ( num_params > 0 )
    {
      FTRenderer_setMode  set_mode = renderer->clazz->set_mode;


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


  LOCAL_FUNC
  FT_Error  FT_Render_Glyph_Internal( FT_Library    library,
                                      FT_GlyphSlot  slot,
                                      FT_UInt       render_mode )
  {
    FT_Error     error = FT_Err_Ok;
    FT_Renderer  renderer;


    /* if it is already a bitmap, no need to do anything */
    switch ( slot->format )
    {
    case ft_glyph_format_bitmap:   /* already a bitmap, don't do anything */
      break;

    default:
      {
        FT_ListNode  node   = 0;
        FT_Bool      update = 0;


        /* small shortcut for the very common case */
        if ( slot->format == ft_glyph_format_outline )
        {
          renderer = library->cur_renderer;
          node     = library->renderers.head;
        }
        else
          renderer = FT_Lookup_Renderer( library, slot->format, &node );

        error = FT_Err_Unimplemented_Feature;
        while ( renderer )
        {
          error = renderer->render( renderer, slot, render_mode, 0 );
          if ( !error || error != FT_Err_Cannot_Render_Glyph )
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Render_Glyph                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts a given glyph image to a bitmap.  It does so by           */
  /*    inspecting the glyph image format, find the relevant renderer, and */
  /*    invoke it.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot        :: A handle to the glyph slot containing the image to  */
  /*                   convert.                                            */
  /*                                                                       */
  /*    render_mode :: This is the render mode used to render the glyph    */
  /*                   image into a bitmap.  See FT_Render_Mode for a list */
  /*                   of possible values.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Render_Glyph( FT_GlyphSlot  slot,
                                               FT_UInt       render_mode )
  {
    FT_Library   library;


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
  static
  void  Destroy_Module( FT_Module  module )
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
    FREE( module );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Module                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds a new module to a given library instance.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the library object.                         */
  /*                                                                       */
  /*    clazz   :: A pointer to class descriptor for the module.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error will be returned if a module already exists by that name, */
  /*    or if the module requires a version of FreeType that is too great. */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Add_Module( FT_Library              library,
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
      if ( strcmp( module->clazz->module_name, clazz->module_name ) == 0 )
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
    if ( ALLOC( module,clazz->module_size ) )
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
      FT_Driver   driver = FT_DRIVER( module );


      driver->clazz = (FT_Driver_Class*)module->clazz;
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

    FREE( module );
    goto Exit;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Module                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finds a module by its name.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: A handle to the library object.                     */
  /*                                                                       */
  /*    module_name :: The module's name (as an ASCII string).             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A module handle.  0 if none was found.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should better be familiar with FreeType internals to know      */
  /*    which module to look for :-)                                       */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Module ) FT_Get_Module( FT_Library   library,
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
      if ( strcmp( cur[0]->clazz->module_name, module_name ) == 0 )
      {
        result = cur[0];
        break;
      }

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Module_Interface                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finds a module and returns its specific interface as a typeless    */
  /*    pointer.                                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: A handle to the library object.                     */
  /*                                                                       */
  /*    module_name :: The module's name (as an ASCII string).             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A module-specific interface if available, 0 otherwise.             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should better be familiar with FreeType internals to know      */
  /*    which module to look for, and what its interface is :-)            */
  /*                                                                       */
  BASE_FUNC( const void* )  FT_Get_Module_Interface( FT_Library   library,
                                                     const char*  mod_name )
  {
    FT_Module  module;


    /* test for valid `library' delayed to FT_Get_Module() */

    module = FT_Get_Module( library, mod_name );

    return module ? module->clazz->module_interface : 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Remove_Module                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Removes a given module from a library instance.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to a library object.                           */
  /*                                                                       */
  /*    module  :: A handle to a module object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The module object is destroyed by the function in case of success. */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Remove_Module( FT_Library  library,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Library                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to create a new FreeType library instance    */
  /*    from a given memory object.  It is thus possible to use libraries  */
  /*    with distinct memory allocators within the same program.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory   :: A handle to the original memory object.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    alibrary :: A pointer to handle of a new library object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_New_Library( FT_Memory    memory,
                                              FT_Library*  alibrary )
  {
    FT_Library  library = 0;
    FT_Error    error;


    if ( !memory )
      return FT_Err_Invalid_Argument;

    /* first of all, allocate the library object */
    if ( ALLOC( library, sizeof ( *library ) ) )
      return error;

    library->memory = memory;

    /* allocate the render pool */
    library->raster_pool_size = FT_RENDER_POOL_SIZE;
    if ( ALLOC( library->raster_pool, FT_RENDER_POOL_SIZE ) )
      goto Fail;

    /* That's ok now */
    *alibrary = library;

    return FT_Err_Ok;

  Fail:
    FREE( library );
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Library                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given library object.  This closes all drivers and      */
  /*    discards all resource objects.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Done_Library( FT_Library  library )
  {
    FT_Memory  memory;
    FT_UInt    n;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    memory = library->memory;

    /* Discard client-data */
    if ( library->generic.finalizer )
      library->generic.finalizer( library );

    /* Close all modules in the library */
    for ( n = 0; n < library->num_modules; n++ )
    {
      FT_Module  module = library->modules[n];


      if ( module )
      {
        Destroy_Module( module );
        library->modules[n] = 0;
      }
    }

    /* Destroy raster objects */
    FREE( library->raster_pool );
    library->raster_pool_size = 0;

    FREE( library );
    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Debug_Hook                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets a debug hook function for debugging the interpreter of a font */
  /*    format.                                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library    :: A handle to the library object.                      */
  /*                                                                       */
  /*    hook_index :: The index of the debug hook.  You should use the     */
  /*                  values defined in ftobjs.h, e.g.                     */
  /*                  FT_DEBUG_HOOK_TRUETYPE                               */
  /*                                                                       */
  /*    debug_hook :: The function used to debug the interpreter.          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Currently, four debug hook slots are available, but only two (for  */
  /*    the TrueType and the Type 1 interpreter) are defined.              */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Set_Debug_Hook( FT_Library         library,
                                             FT_UInt            hook_index,
                                             FT_DebugHook_Func  debug_hook )
  {
    if ( library && debug_hook &&
         hook_index <
           ( sizeof ( library->debug_hooks ) / sizeof ( void* ) ) )
      library->debug_hooks[hook_index] = debug_hook;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_FreeType                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given FreeType library object and all of its childs,    */
  /*    including resources, drivers, faces, sizes, etc.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library object.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Done_FreeType( FT_Library  library )
  {
    /* test for valid `library' delayed to FT_Done_Library() */

    /* Discard the library object */
    FT_Done_Library( library );

    return FT_Err_Ok;
  }


/* END */
