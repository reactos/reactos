/***************************************************************************/
/*                                                                         */
/*  ftglyph.h                                                              */
/*                                                                         */
/*    FreeType convenience functions to handle glyphs (specification).     */
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
  /* This file contains the definition of several convenience functions    */
  /* that can be used by client applications to easily retrieve glyph      */
  /* bitmaps and outlines from a given face.                               */
  /*                                                                       */
  /* These functions should be optional if you are writing a font server   */
  /* or text layout engine on top of FreeType.  However, they are pretty   */
  /* handy for many other simple uses of the library.                      */
  /*                                                                       */
  /*************************************************************************/


#ifndef FTGLYPH_H
#define FTGLYPH_H

#include <freetype/freetype.h>

#ifdef __cplusplus
  extern "C" {
#endif

  /* forward declaration to a private type */
  typedef struct FT_Glyph_Class_  FT_Glyph_Class;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_GlyphRec                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The root glyph structure contains a given glyph image plus its     */
  /*    advance width in 16.16 fixed float format.                         */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    library :: A handle to the FreeType library object.                */
  /*                                                                       */
  /*    clazz   :: A pointer to the glyph's class.  Private.               */
  /*                                                                       */
  /*    format  :: The format of the glyph's image.                        */
  /*                                                                       */
  /*    advance :: A 16.16 vector that gives the glyph's advance width.    */
  /*                                                                       */
  typedef struct  FT_GlyphRec_
  {
    FT_Library             library;
    const FT_Glyph_Class*  clazz;
    FT_Glyph_Format        format;
    FT_Vector              advance;

  } FT_GlyphRec, *FT_Glyph;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_BitmapGlyphRec                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used for bitmap glyph images.  This really is a        */
  /*    `sub-class' of `FT_GlyphRec'.                                      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    root   :: The root FT_Glyph fields.                                */
  /*                                                                       */
  /*    left   :: The left-side bearing, i.e., the horizontal distance     */
  /*              from the current pen position to the left border of the  */
  /*              glyph bitmap.                                            */
  /*                                                                       */
  /*    top    :: The top-side bearing, i.e., the vertical distance from   */
  /*              the current pen position to the top border of the glyph  */
  /*              bitmap.  This distance is positive for upwards-y!        */
  /*                                                                       */
  /*    bitmap :: A descriptor for the bitmap.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You can typecast FT_Glyph to FT_BitmapGlyph if you have            */
  /*    glyph->format == ft_glyph_format_bitmap.  This lets you access     */
  /*    the bitmap's contents easily.                                      */
  /*                                                                       */
  /*    The corresponding pixel buffer is always owned by the BitmapGlyph  */
  /*    and is thus created and destroyed with it.                         */
  /*                                                                       */
  typedef struct  FT_BitmapGlyphRec_
  {
    FT_GlyphRec  root;
    FT_Int       left;
    FT_Int       top;
    FT_Bitmap    bitmap;

  } FT_BitmapGlyphRec, *FT_BitmapGlyph;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_OutlineGlyphRec                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used for outline (vectorial) glyph images.  This       */
  /*    really is a `sub-class' of `FT_GlyphRec'.                          */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    root    :: The root FT_Glyph fields.                               */
  /*                                                                       */
  /*    outline :: A descriptor for the outline.                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You can typecast FT_Glyph to FT_OutlineGlyph if you have           */
  /*    glyph->format == ft_glyph_format_outline.  This lets you access    */
  /*    the outline's content easily.                                      */
  /*                                                                       */
  /*    As the outline is extracted from a glyph slot, its coordinates are */
  /*    expressed normally in 26.6 pixels, unless the flag                 */
  /*    FT_LOAD_NO_SCALE was used in FT_Load_Glyph() or FT_Load_Char().    */
  /*                                                                       */
  /*    The outline's tables are always owned by the object and are        */
  /*    destroyed with it.                                                 */
  /*                                                                       */
  typedef struct  FT_OutlineGlyphRec_
  {
    FT_GlyphRec  root;
    FT_Outline   outline;

  } FT_OutlineGlyphRec, *FT_OutlineGlyph;


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
  FT_EXPORT_DEF( FT_Error )  FT_Get_Glyph( FT_GlyphSlot  slot,
                                           FT_Glyph*     aglyph );


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
  FT_EXPORT_DEF( FT_Error )  FT_Glyph_Copy( FT_Glyph   source,
                                            FT_Glyph*  target );


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
  FT_EXPORT_DEF( FT_Error )  FT_Glyph_Transform( FT_Glyph    glyph,
                                                 FT_Matrix*  matrix,
                                                 FT_Vector*  delta );


  enum
  {
    ft_glyph_bbox_pixels    = 0,
    ft_glyph_bbox_subpixels = 1,
    ft_glyph_bbox_gridfit   = 2
  };


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
  FT_EXPORT_DEF( void )  FT_Glyph_Get_CBox( FT_Glyph  glyph,
                                            FT_UInt   bbox_mode,
                                            FT_BBox*  cbox );


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
  FT_EXPORT_DEF( FT_Error )  FT_Glyph_To_Bitmap( FT_Glyph*   the_glyph,
                                                 FT_ULong    render_mode,
                                                 FT_Vector*  origin,
                                                 FT_Bool     destroy );


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
  FT_EXPORT_DEF( void )  FT_Done_Glyph( FT_Glyph  glyph );


  /* other helpful functions */


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
  FT_EXPORT_DEF( void )  FT_Matrix_Multiply( FT_Matrix*  a,
                                             FT_Matrix*  b );


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
  FT_EXPORT_DEF( FT_Error )  FT_Matrix_Invert( FT_Matrix*  matrix );


#ifdef __cplusplus
  }
#endif

#endif /* FTGLYPH_H */


/* END */
