/***************************************************************************/
/*                                                                         */
/*  ftglyph.c                                                              */
/*                                                                         */
/*    FreeType convenience functions to handle glyphs (body).              */
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

  /*************************************************************************/
  /*                                                                       */
  /*  This file contains the definition of several convenience functions   */
  /*  that can be used by client applications to easily retrieve glyph     */
  /*  bitmaps and outlines from a given face.                              */
  /*                                                                       */
  /*  These functions should be optional if you are writing a font server  */
  /*  or text layout engine on top of FreeType.  However, they are pretty  */
  /*  handy for many other simple uses of the library.                     */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/internal/ftobjs.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_glyph


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   Convenience functions                                         ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Matrix_Multiply                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Performs the matrix operation `b = a*b'.                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: A pointer to matrix `a'.                                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    b :: A pointer to matrix `b'.                                      */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    Yes.                                                               */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The result is undefined if either `a' or `b' is zero.              */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Matrix_Multiply( FT_Matrix*  a,
                                              FT_Matrix*  b )
  {
    FT_Fixed  xx, xy, yx, yy;


    if ( !a || !b )
      return;

    xx = FT_MulFix( a->xx, b->xx ) + FT_MulFix( a->xy, b->yx );
    xy = FT_MulFix( a->xx, b->xy ) + FT_MulFix( a->xy, b->yy );
    yx = FT_MulFix( a->yx, b->xx ) + FT_MulFix( a->yy, b->yx );
    yy = FT_MulFix( a->yx, b->xy ) + FT_MulFix( a->yy, b->yy );

    b->xx = xx;  b->xy = xy;
    b->yx = yx;  b->yy = yy;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Matrix_Invert                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Inverts a 2x2 matrix.  Returns an error if it can't be inverted.   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    matrix :: A pointer to the target matrix.  Remains untouched in    */
  /*              case of error.                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    Yes.                                                               */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Matrix_Invert( FT_Matrix*  matrix )
  {
    FT_Pos  delta, xx, yy;


    if ( !matrix )
      return FT_Err_Invalid_Argument;

    /* compute discriminant */
    delta = FT_MulFix( matrix->xx, matrix->yy ) -
            FT_MulFix( matrix->xy, matrix->yx );

    if ( !delta )
      return FT_Err_Invalid_Argument;  /* matrix can't be inverted */

    matrix->xy = - FT_DivFix( matrix->xy, delta );
    matrix->yx = - FT_DivFix( matrix->yx, delta );

    xx = matrix->xx;
    yy = matrix->yy;

    matrix->xx = FT_DivFix( yy, delta );
    matrix->yy = FT_DivFix( xx, delta );

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   FT_BitmapGlyph support                                        ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  static
  FT_Error  ft_bitmap_copy( FT_Memory   memory,
                            FT_Bitmap*  source,
                            FT_Bitmap*  target )
  {
    FT_Error  error;
    FT_Int    pitch = source->pitch;
    FT_ULong  size;


    *target = *source;

    if ( pitch < 0 )
      pitch = -pitch;

    size = (FT_ULong)( pitch * source->rows );

    if ( !ALLOC( target->buffer, size ) )
      MEM_Copy( source->buffer, target->buffer, size );

    return error;
  }


  static
  FT_Error  ft_bitmap_glyph_init( FT_BitmapGlyph  glyph,
                                  FT_GlyphSlot    slot )
  {
    FT_Error    error   = FT_Err_Ok;
    FT_Library  library = FT_GLYPH(glyph)->library;
    FT_Memory   memory  = library->memory;


    if ( slot->format != ft_glyph_format_bitmap )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    /* grab the bitmap in the slot - do lazy copying whenever possible */
    glyph->bitmap = slot->bitmap;
    glyph->left   = slot->bitmap_left;
    glyph->top    = slot->bitmap_top;

    if ( slot->flags & ft_glyph_own_bitmap )
      slot->flags &= ~ft_glyph_own_bitmap;
    else
    {
      /* copy the bitmap into a new buffer */
      error = ft_bitmap_copy( memory, &slot->bitmap, &glyph->bitmap );
    }

  Exit:
    return error;
  }


  static
  FT_Error  ft_bitmap_glyph_copy( FT_BitmapGlyph  source,
                                  FT_BitmapGlyph  target )
  {
    FT_Memory  memory = source->root.library->memory;


    target->left = source->left;
    target->top  = source->top;

    return ft_bitmap_copy( memory, &source->bitmap, &target->bitmap );
  }


  static
  void  ft_bitmap_glyph_done( FT_BitmapGlyph  glyph )
  {
    FT_Memory  memory = FT_GLYPH(glyph)->library->memory;


    FREE( glyph->bitmap.buffer );
  }


  static
  void  ft_bitmap_glyph_bbox( FT_BitmapGlyph  glyph,
                              FT_BBox*        cbox )
  {
    cbox->xMin = glyph->left << 6;
    cbox->xMax = cbox->xMin + ( glyph->bitmap.width << 6 );
    cbox->yMax = glyph->top << 6;
    cbox->yMin = cbox->xMax - ( glyph->bitmap.rows << 6 );
  }


  const FT_Glyph_Class  ft_bitmap_glyph_class =
  {
    sizeof( FT_BitmapGlyphRec ),
    ft_glyph_format_bitmap,

    (FT_Glyph_Init_Func)     ft_bitmap_glyph_init,
    (FT_Glyph_Done_Func)     ft_bitmap_glyph_done,
    (FT_Glyph_Copy_Func)     ft_bitmap_glyph_copy,
    (FT_Glyph_Transform_Func)0,
    (FT_Glyph_BBox_Func)     ft_bitmap_glyph_bbox,
    (FT_Glyph_Prepare_Func)  0
  };


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   FT_OutlineGlyph support                                       ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  static
  FT_Error  ft_outline_glyph_init( FT_OutlineGlyph  glyph,
                                   FT_GlyphSlot     slot )
  {
    FT_Error     error   = FT_Err_Ok;
    FT_Library   library = FT_GLYPH(glyph)->library;
    FT_Outline*  source  = &slot->outline;
    FT_Outline*  target  = &glyph->outline;


    /* check format in glyph slot */
    if ( slot->format != ft_glyph_format_outline )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    /* allocate new outline */
    error = FT_Outline_New( library, source->n_points, source->n_contours,
                            &glyph->outline );
    if ( error )
      goto Exit;

    /* copy it */
    MEM_Copy( target->points, source->points,
              source->n_points * sizeof ( FT_Vector ) );

    MEM_Copy( target->tags, source->tags,
              source->n_points * sizeof ( FT_Byte ) );

    MEM_Copy( target->contours, source->contours,
              source->n_contours * sizeof ( FT_Short ) );

    /* copy all flags, except the `ft_outline_owner' one */
    target->flags = source->flags | ft_outline_owner;

  Exit:
    return error;
  }


  static
  void  ft_outline_glyph_done( FT_OutlineGlyph  glyph )
  {
    FT_Outline_Done( FT_GLYPH( glyph )->library, &glyph->outline );
  }


  static
  FT_Error  ft_outline_glyph_copy( FT_OutlineGlyph  source,
                                   FT_OutlineGlyph  target )
  {
    FT_Error    error;
    FT_Library  library = FT_GLYPH( source )->library;


    error = FT_Outline_New( library, source->outline.n_points,
                            source->outline.n_contours, &target->outline );
    if ( !error )
      FT_Outline_Copy( &source->outline, &target->outline );

    return error;
  }


  static
  void  ft_outline_glyph_transform( FT_OutlineGlyph  glyph,
                                    FT_Matrix*       matrix,
                                    FT_Vector*       delta )
  {
    if ( matrix )
      FT_Outline_Transform( &glyph->outline, matrix );

    if ( delta )
      FT_Outline_Translate( &glyph->outline, delta->x, delta->y );
  }


  static
  void  ft_outline_glyph_bbox( FT_OutlineGlyph  glyph,
                               FT_BBox*         bbox )
  {
    FT_Outline_Get_CBox( &glyph->outline, bbox );
  }


  static
  FT_Error  ft_outline_glyph_prepare( FT_OutlineGlyph  glyph,
                                      FT_GlyphSlot     slot )
  {
    slot->format         = ft_glyph_format_outline;
    slot->outline        = glyph->outline;
    slot->outline.flags &= ~ft_outline_owner;

    return FT_Err_Ok;
  }


  const FT_Glyph_Class  ft_outline_glyph_class =
  {
    sizeof( FT_OutlineGlyphRec ),
    ft_glyph_format_outline,

    (FT_Glyph_Init_Func)     ft_outline_glyph_init,
    (FT_Glyph_Done_Func)     ft_outline_glyph_done,
    (FT_Glyph_Copy_Func)     ft_outline_glyph_copy,
    (FT_Glyph_Transform_Func)ft_outline_glyph_transform,
    (FT_Glyph_BBox_Func)     ft_outline_glyph_bbox,
    (FT_Glyph_Prepare_Func)  ft_outline_glyph_prepare
  };


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   FT_Glyph class and API                                        ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

   static
   FT_Error  ft_new_glyph( FT_Library             library,
                           const FT_Glyph_Class*  clazz,
                           FT_Glyph*              aglyph )
   {
     FT_Memory  memory = library->memory;
     FT_Error   error;
     FT_Glyph   glyph;


     *aglyph = 0;

     if ( !ALLOC( glyph, clazz->glyph_size ) )
     {
       glyph->library = library;
       glyph->clazz   = clazz;
       glyph->format  = clazz->glyph_format;

       *aglyph = glyph;
     }

     return error;
   }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Glyph_Copy                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to copy a glyph image.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    source :: A handle to the source glyph object.                     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    target :: A handle to the target glyph object.  0 in case of       */
  /*              error.                                                   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Glyph_Copy( FT_Glyph   source,
                                             FT_Glyph*  target )
  {
    FT_Glyph               copy;
    FT_Error               error;
    const FT_Glyph_Class*  clazz;


    /* check arguments */
    if ( !target || !source || !source->clazz )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    *target = 0;

    clazz = source->clazz;
    error = ft_new_glyph( source->library, clazz, &copy );
    if ( error )
      goto Exit;

    if ( clazz->glyph_copy )
      error = clazz->glyph_copy( source, copy );

    if ( error )
      FT_Done_Glyph( copy );
    else
      *target = copy;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Glyph                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to extract a glyph image from a slot.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot   :: A handle to the source glyph slot.                       */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aglyph :: A handle to the glyph object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Get_Glyph( FT_GlyphSlot  slot,
                                            FT_Glyph*     aglyph )
  {
    FT_Library  library = slot->library;
    FT_Error    error;
    FT_Glyph    glyph;

    const FT_Glyph_Class*  clazz = 0;


    if ( !slot )
      return FT_Err_Invalid_Slot_Handle;

    if ( !aglyph )
      return FT_Err_Invalid_Argument;

    /* if it is a bitmap, that's easy :-) */
    if ( slot->format == ft_glyph_format_bitmap )
      clazz = &ft_bitmap_glyph_class;

    /* it it is an outline too */
    else if ( slot->format == ft_glyph_format_outline )
      clazz = &ft_outline_glyph_class;

    else
    {
      /* try to find a renderer that supports the glyph image format */
      FT_Renderer  render = FT_Lookup_Renderer( library, slot->format, 0 );


      if ( render )
        clazz = &render->glyph_class;
    }

    if ( !clazz )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    /* create FT_Glyph object */
    error = ft_new_glyph( library, clazz, &glyph );
    if ( error )
      goto Exit;

    /* copy advance while converting it to 16.16 format */
    glyph->advance.x = slot->advance.x << 10;
    glyph->advance.y = slot->advance.y << 10;

    /* now import the image from the glyph slot */
    error = clazz->glyph_init( glyph, slot );

    /* if an error occurred, destroy the glyph */
    if ( error )
      FT_Done_Glyph( glyph );
    else
      *aglyph = glyph;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Glyph_Transform                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Transforms a glyph image if its format is scalable.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph  :: A handle to the target glyph object.                     */
  /*                                                                       */
  /*    matrix :: A pointer to a 2x2 matrix to apply.                      */
  /*                                                                       */
  /*    delta  :: A pointer to a 2d vector to apply.  Coordinates are      */
  /*              expressed in 1/64th of a pixel.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code (the glyph format is not scalable if it is     */
  /*    not zero).                                                         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The 2x2 transformation matrix is also applied to the glyph's       */
  /*    advance vector.                                                    */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Glyph_Transform( FT_Glyph    glyph,
                                                  FT_Matrix*  matrix,
                                                  FT_Vector*  delta )
  {
    const FT_Glyph_Class*  clazz;
    FT_Error               error = FT_Err_Ok;


    if ( !glyph || !glyph->clazz )
      error = FT_Err_Invalid_Argument;
    else
    {
      clazz = glyph->clazz;
      if ( clazz->glyph_transform )
      {
        /* transform glyph image */
        clazz->glyph_transform( glyph, matrix, delta );

        /* transform advance vector */
        if ( matrix )
          FT_Vector_Transform( &glyph->advance, matrix );
      }
      else
        error = FT_Err_Invalid_Glyph_Format;
    }
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Glyph_Get_CBox                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the glyph image's bounding box.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph :: A handle to the source glyph object.                      */
  /*                                                                       */
  /*    mode  :: A set of bit flags that indicate how to interpret the     */
  /*             returned bounding box values.                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    box   :: The glyph bounding box.  Coordinates are expressed in     */
  /*             1/64th of pixels if it is grid-fitted.                    */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Coordinates are relative to the glyph origin, using the Y-upwards  */
  /*    convention.                                                        */
  /*                                                                       */
  /*    If `ft_glyph_bbox_subpixels' is set in `mode', the bbox            */
  /*    coordinates are returned in 26.6 pixels (i.e. 1/64th of pixels).   */
  /*    Otherwise, coordinates are expressed in integer pixels.            */
  /*                                                                       */
  /*    Note that the maximum coordinates are exclusive, which means that  */
  /*    one can compute the width and height of the glyph image (be it in  */
  /*    integer or 26.6 pixels) as:                                        */
  /*                                                                       */
  /*      width  = bbox.xMax - bbox.xMin;                                  */
  /*      height = bbox.yMax - bbox.yMin;                                  */
  /*                                                                       */
  /*    Note also that for 26.6 coordinates, if the                        */
  /*    `ft_glyph_bbox_gridfit' flag is set in `mode;, the coordinates     */
  /*    will also be grid-fitted, which corresponds to:                    */
  /*                                                                       */
  /*      bbox.xMin = FLOOR(bbox.xMin);                                    */
  /*      bbox.yMin = FLOOR(bbox.yMin);                                    */
  /*      bbox.xMax = CEILING(bbox.xMax);                                  */
  /*      bbox.yMax = CEILING(bbox.yMax);                                  */
  /*                                                                       */
  /*    The default value (0) for `bbox_mode' is `ft_glyph_bbox_pixels'.   */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Glyph_Get_CBox( FT_Glyph  glyph,
                                             FT_UInt   bbox_mode,
                                             FT_BBox*  cbox )
  {
    const FT_Glyph_Class*  clazz;
    FT_Error               error = FT_Err_Ok;


    if ( !cbox || !glyph || !glyph->clazz )
      error = FT_Err_Invalid_Argument;
    else
    {
      clazz = glyph->clazz;
      if ( !clazz->glyph_bbox )
        error = FT_Err_Invalid_Glyph_Format;
      else
      {
        /* retrieve bbox in 26.6 coordinates */
        clazz->glyph_bbox( glyph, cbox );

        /* perform grid fitting if needed */
        if ( bbox_mode & ft_glyph_bbox_gridfit )
        {
          cbox->xMin &= -64;
          cbox->yMin &= -64;
          cbox->xMax  = ( cbox->xMax + 63 ) & -64;
          cbox->yMax  = ( cbox->yMax + 63 ) & -64;
        }

        /* convert to integer pixels if needed */
        if ( !( bbox_mode & ft_glyph_bbox_subpixels ) )
        {
          cbox->xMin >>= 6;
          cbox->yMin >>= 6;
          cbox->xMax >>= 6;
          cbox->yMax >>= 6;
        }
      }
    }
    return;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Glyph_To_Bitmap                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts a given glyph object to a bitmap glyph object.            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    glyph       :: A pointer to a handle to the target glyph.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    render_mode :: A set of bit flags that describe how the data is    */
  /*                                                                       */
  /*                                                                       */
  /*    origin      :: A pointer to a vector used to translate the glyph   */
  /*                   image before rendering.  Can be 0 (if no            */
  /*                   translation).  The origin is expressed in           */
  /*                   26.6 pixels.                                        */
  /*                                                                       */
  /*    destroy     :: A boolean that indicates that the original glyph    */
  /*                   image should be destroyed by this function.  It is  */
  /*                   never destroyed in case of error.                   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The glyph image is translated with the `origin' vector before      */
  /*    rendering.  In case of error, it it translated back to its         */
  /*    original position and the glyph is left untouched.                 */
  /*                                                                       */
  /*    The first parameter is a pointer to a FT_Glyph handle, that will   */
  /*    be replaced by this function.  Typically, you would use (omitting  */
  /*    error handling):                                                   */
  /*                                                                       */
  /*                                                                       */
  /*      {                                                                */
  /*        FT_Glyph        glyph;                                         */
  /*        FT_BitmapGlyph  glyph_bitmap;                                  */
  /*                                                                       */
  /*                                                                       */
  /*        // load glyph                                                  */
  /*        error = FT_Load_Char( face, glyph_index, FT_LOAD_DEFAUT );     */
  /*                                                                       */
  /*        // extract glyph image                                         */
  /*        error = FT_Get_Glyph( face->glyph, &glyph );                   */
  /*                                                                       */
  /*        // convert to a bitmap (default render mode + destroy old)     */
  /*        if ( glyph->format != ft_glyph_format_bitmap )                 */
  /*        {                                                              */
  /*          error = FT_Glyph_To_Bitmap( &glyph, ft_render_mode_default,  */
  /*                                      0, 1 );                          */
  /*          if ( error ) // glyph unchanged                              */
  /*            ...                                                        */
  /*        }                                                              */
  /*                                                                       */
  /*        // access bitmap content by typecasting                        */
  /*        glyph_bitmap = (FT_BitmapGlyph)glyph;                          */
  /*                                                                       */
  /*        // do funny stuff with it, like blitting/drawing               */
  /*        ...                                                            */
  /*                                                                       */
  /*        // discard glyph image (bitmap or not)                         */
  /*        FT_Done_Glyph( glyph );                                        */
  /*      }                                                                */
  /*                                                                       */
  /*                                                                       */
  /*    This function will always fail if the glyph's format isn't         */
  /*    scalable.                                                          */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Glyph_To_Bitmap( FT_Glyph*   the_glyph,
                                                  FT_ULong    render_mode,
                                                  FT_Vector*  origin,
                                                  FT_Bool     destroy )
  {
    FT_GlyphSlotRec  dummy;
    FT_Error         error;
    FT_Glyph         glyph;
    FT_BitmapGlyph   bitmap;

    const FT_Glyph_Class*  clazz;


    /* check argument */
    if ( !the_glyph )
      goto Bad;

    /* we render the glyph into a glyph bitmap using a `dummy' glyph slot */
    /* then calling FT_Render_Glyph_Internal()                            */

    glyph = *the_glyph;
    if ( !glyph )
      goto Bad;

    clazz = glyph->clazz;
    if ( !clazz || !clazz->glyph_prepare )
      goto Bad;

    MEM_Set( &dummy, 0, sizeof ( dummy ) );
    dummy.library = glyph->library;
    dummy.format  = clazz->glyph_format;

    /* if `origin' is set, translate the glyph image */
    if ( origin )
      FT_Glyph_Transform( glyph, 0, origin );

    /* create result bitmap glyph */
    error = ft_new_glyph( glyph->library, &ft_bitmap_glyph_class,
                          (FT_Glyph*)&bitmap );
    if ( error )
      goto Exit;

    /* prepare dummy slot for rendering */
    error = clazz->glyph_prepare( glyph, &dummy ) ||
            FT_Render_Glyph_Internal( glyph->library, &dummy, render_mode );

    if ( !destroy && origin )
    {
      FT_Vector  v;


      v.x = -origin->x;
      v.y = -origin->y;
      FT_Glyph_Transform( glyph, 0, &v );
    }

    /* in case of succes, copy the bitmap to the glyph bitmap */
    if ( !error )
    {
      error = ft_bitmap_glyph_init( bitmap, &dummy );
      if ( error )
      {
        /* this should never happen, but let's be safe */
        FT_Done_Glyph( FT_GLYPH( bitmap ) );
        goto Exit;
      }

      if ( destroy )
        FT_Done_Glyph( glyph );

      *the_glyph = FT_GLYPH( bitmap );
    }

  Exit:
    return error;

  Bad:
    error = FT_Err_Invalid_Argument;
    goto Exit;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given glyph.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph :: A handle to the target glyph object.                      */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Done_Glyph( FT_Glyph  glyph )
  {
    if ( glyph )
    {
      FT_Memory              memory = glyph->library->memory;
      const FT_Glyph_Class*  clazz  = glyph->clazz;


      if ( clazz->glyph_done )
        clazz->glyph_done( glyph );

      FREE( glyph );
    }
  }


#if 0

  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL EMBOLDENING/OUTLINING SUPPORT                    ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  /* Compute the norm of a vector */

#ifdef FT_CONFIG_OPTION_OLD_CALCS

  static
  FT_Pos  ft_norm( FT_Vector*  vec )
  {
    FT_Int64  t1, t2;


    MUL_64( vec->x, vec->x, t1 );
    MUL_64( vec->y, vec->y, t2 );
    ADD_64( t1, t2, t1 );

    return (FT_Pos)SQRT_64( t1 );
  }

#else /* FT_CONFIG_OPTION_OLD_CALCS */

  static
  FT_Pos  ft_norm( FT_Vector*  vec )
  {
    FT_F26Dot6  u, v, d;
    FT_Int      shift;
    FT_ULong    H, L, L2, hi, lo, med;


    u = vec->x; if ( u < 0 ) u = -u;
    v = vec->y; if ( v < 0 ) v = -v;

    if ( u < v )
    {
      d = u;
      u = v;
      v = d;
    }

    /* check that we are not trying to normalize zero! */
    if ( u == 0 )
      return 0;

    /* compute (u*u + v*v) on 64 bits with two 32-bit registers [H:L] */
    hi  = (FT_ULong)u >> 16;
    lo  = (FT_ULong)u & 0xFFFF;
    med = hi * lo;

    H     = hi * hi + ( med >> 15 );
    med <<= 17;
    L     = lo * lo + med;
    if ( L < med )
      H++;

    hi  = (FT_ULong)v >> 16;
    lo  = (FT_ULong)v & 0xFFFF;
    med = hi * lo;

    H    += hi * hi + ( med >> 15 );
    med <<= 17;
    L2    = lo * lo + med;
    if ( L2 < med )
      H++;

    L += L2;
    if ( L < L2 )
      H++;

    /* if the value is smaller than 32 bits */
    shift = 0;
    if ( H == 0 )
    {
      while ( ( L & 0xC0000000UL ) == 0 )
      {
        L <<= 2;
        shift++;
      }
      return ( FT_Sqrt32( L ) >> shift );
    }
    else
    {
      while ( H )
      {
        L   = ( L >> 2 ) | ( H << 30 );
        H >>= 2;
        shift++;
      }
      return ( FT_Sqrt32( L ) << shift );
    }
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


  static
  int  ft_test_extrema( FT_Outline*  outline,
                        int          n )
  {
    FT_Vector  *prev, *cur, *next;
    FT_Pos      product;
    FT_Int      first, last;


    /* we need to compute the `previous' and `next' point */
    /* for these extrema.                                 */
    cur   = outline->points + n;
    prev  = cur - 1;
    next  = cur + 1;

    first = 0;
    for ( c = 0; c < outline->n_contours; c++ )
    {
      last  = outline->contours[c];

      if ( n == first )
        prev = outline->points + last;

      if ( n == last )
        next = outline->points + first;

      first = last + 1;
    }

    product = FT_MulDiv( cur->x - prev->x,   /* in.x  */
                         next->y - cur->y,   /* out.y */
                         0x40 )
              -
              FT_MulDiv( cur->y - prev->y,   /* in.y  */
                         next->x - cur->x,   /* out.x */
                         0x40 );

    if ( product )
      product = product > 0 ? 1 : -1;

    return product;
  }


  /* Compute the orientation of path filling.  It differs between TrueType */
  /* and Type1 formats.  We could use the `ft_outline_reverse_fill' flag,  */
  /* but it is better to re-compute it directly (it seems that this flag   */
  /* isn't correctly set for some weird composite glyphs currently).       */
  /*                                                                       */
  /* We do this by computing bounding box points, and computing their      */
  /* curvature.                                                            */
  /*                                                                       */
  /* The function returns either 1 or -1.                                  */
  /*                                                                       */
  static
  int  ft_get_orientation( FT_Outline*  outline )
  {
    FT_BBox  box;
    FT_BBox  indices;
    int      n, last;


    indices.xMin = -1;
    indices.yMin = -1;
    indices.xMax = -1;
    indices.yMax = -1;

    box.xMin = box.yMin = 32767;
    box.xMax = box.yMax = -32768;

    /* is it empty ? */
    if ( outline->n_contours < 1 )
      return 1;

    last = outline->contours[outline->n_contours - 1];

    for ( n = 0; n <= last; n++ )
    {
      FT_Pos  x, y;


      x = outline->points[n].x;
      if ( x < box.xMin )
      {
        box.xMin     = x;
        indices.xMin = n;
      }
      if ( x > box.xMax )
      {
        box.xMax     = x;
        indices.xMax = n;
      }

      y = outline->points[n].y;
      if ( y < box.yMin )
      {
        box.yMin     = y;
        indices.yMin = n;
      }
      if ( y > box.yMax )
      {
        box.yMax     = y;
        indices.yMax = n;
      }
    }

    /* test orientation of the xmin */
    return ft_test_extrema( outline, indices.xMin ) ||
           ft_test_extrema( outline, indices.yMin ) ||
           ft_test_extrema( outline, indices.xMax ) ||
           ft_test_extrema( outline, indices.yMax ) ||
           1;  /* this is an empty glyph? */
  }


  static
  FT_Error  ft_embolden( FT_Face      original,
                         FT_Outline*  outline,
                         FT_Pos*      advance )
  {
    FT_Vector  u, v;
    FT_Vector* points;
    FT_Vector  cur, prev, next;
    FT_Pos     distance;
    int        c, n, first, orientation;

    FT_UNUSED( advance );


    /* compute control distance */
    distance = FT_MulFix( original->em_size / 60,
                          original->size->metrics.y_scale );

    orientation = ft_get_orientation( &original->glyph->outline );

    points = original->glyph->outline.points;

    first = 0;
    for ( c = 0; c < outline->n_contours; c++ )
    {
      int  last = outline->contours[c];


      prev = points[last];

      for ( n = first; n <= last; n++ )
      {
        FT_Pos     norm, delta, d;
        FT_Vector  in, out;


        cur = points[n];
        if ( n < last ) next = points[n + 1];
        else            next = points[first];

        /* compute the in and out vectors */
        in.x  = cur.x - prev.x;
        in.y  = cur.y - prev.y;

        out.x = next.x - cur.x;
        out.y = next.y - cur.y;

        /* compute U and V */
        norm = ft_norm( &in );
        u.x = orientation *  FT_DivFix( in.y, norm );
        u.y = orientation * -FT_DivFix( in.x, norm );

        norm = ft_norm( &out );
        v.x = orientation *  FT_DivFix( out.y, norm );
        v.y = orientation * -FT_DivFix( out.x, norm );

        d = distance;

        if ( ( outline->flags[n] & FT_Curve_Tag_On ) == 0 )
          d *= 2;

        /* Check discriminant for parallel vectors */
        delta = FT_MulFix( u.x, v.y ) - FT_MulFix( u.y, v.x );
        if ( delta > FT_BOLD_THRESHOLD || delta < -FT_BOLD_THRESHOLD )
        {
          /* Move point -- compute A and B */
          FT_Pos  x, y, A, B;


          A = d + FT_MulFix( cur.x, u.x ) + FT_MulFix( cur.y, u.y );
          B = d + FT_MulFix( cur.x, v.x ) + FT_MulFix( cur.y, v.y );

          x = FT_MulFix( A, v.y ) - FT_MulFix( B, u.y );
          y = FT_MulFix( B, u.x ) - FT_MulFix( A, v.x );

          outline->points[n].x = distance + FT_DivFix( x, delta );
          outline->points[n].y = distance + FT_DivFix( y, delta );
        }
        else
        {
          /* Vectors are nearly parallel */
          FT_Pos  x, y;


          x = distance + cur.x + FT_MulFix( d, u.x + v.x ) / 2;
          y = distance + cur.y + FT_MulFix( d, u.y + v.y ) / 2;

          outline->points[n].x = x;
          outline->points[n].y = y;
        }

        prev = cur;
      }

      first = last + 1;
    }

    if ( advance )
      *advance = ( *advance + distance * 4 ) & -64;

    return 0;
  }

#endif /* 0 -- EXPERIMENTAL STUFF! */


/* END */
