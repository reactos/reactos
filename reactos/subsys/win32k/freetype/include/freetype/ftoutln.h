/***************************************************************************/
/*                                                                         */
/*  ftoutln.h                                                              */
/*                                                                         */
/*    Support for the FT_Outline type used to store glyph shapes of        */
/*    most scalable font formats (specification).                          */
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


#ifndef FTOUTLN_H
#define FTOUTLN_H


#include <freetype/freetype.h>

#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Decompose                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Walks over an outline's structure to decompose it into individual  */
  /*    segments and Bezier arcs.  This function is also able to emit      */
  /*    `move to' and `close to' operations to indicate the start and end  */
  /*    of new contours in the outline.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline   :: A pointer to the source target.                       */
  /*                                                                       */
  /*    interface :: A table of `emitters', i.e,. function pointers called */
  /*                 during decomposition to indicate path operations.     */
  /*                                                                       */
  /*    user      :: A typeless pointer which is passed to each emitter    */
  /*                 during the decomposition.  It can be used to store    */
  /*                 the state during the decomposition.                   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means sucess.                              */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Outline_Decompose(
                               FT_Outline*        outline,
                               FT_Outline_Funcs*  interface,
                               void*              user );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_New                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new outline of a given size.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: A handle to the library object from where the       */
  /*                   outline is allocated.  Note however that the new    */
  /*                   outline will NOT necessarily be FREED, when         */
  /*                   destroying the library, by FT_Done_FreeType().      */
  /*                                                                       */
  /*    numPoints   :: The maximal number of points within the outline.    */
  /*                                                                       */
  /*    numContours :: The maximal number of contours within the outline.  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    outline     :: A handle to the new outline.  NULL in case of       */
  /*                   error.                                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    No.                                                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The reason why this function takes a `library' parameter is simply */
  /*    to use the library's memory allocator.                             */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Outline_New( FT_Library   library,
                                             FT_UInt      numPoints,
                                             FT_Int       numContours,
                                             FT_Outline*  outline );


  FT_EXPORT_DEF( FT_Error )  FT_Outline_New_Internal(
                               FT_Memory    memory,
                               FT_UInt      numPoints,
                               FT_Int       numContours,
                               FT_Outline*  outline );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys an outline created with FT_Outline_New().                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle of the library object used to allocate the     */
  /*               outline.                                                */
  /*                                                                       */
  /*    outline :: A pointer to the outline object to be discarded.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    No.                                                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the outline's `owner' field is not set, only the outline        */
  /*    descriptor will be released.                                       */
  /*                                                                       */
  /*    The reason why this function takes an `outline' parameter is       */
  /*    simply to use FT_Free().                                           */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Outline_Done( FT_Library   library,
                                              FT_Outline*  outline );


  FT_EXPORT_DEF( FT_Error )  FT_Outline_Done_Internal( FT_Memory    memory,
                                                       FT_Outline*  outline );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Get_CBox                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns an outline's `control box'.  The control box encloses all  */
  /*    the outline's points, including Bezier control points.  Though it  */
  /*    coincides with the exact bounding box for most glyphs, it can be   */
  /*    slightly larger in some situations (like when rotating an outline  */
  /*    which contains Bezier outside arcs).                               */
  /*                                                                       */
  /*    Computing the control box is very fast, while getting the bounding */
  /*    box can take much more time as it needs to walk over all segments  */
  /*    and arcs in the outline.  To get the latter, you can use the       */
  /*    `ftbbox' component which is dedicated to this single task.         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline :: A pointer to the source outline descriptor.             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    cbox    :: The outline's control box.                              */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    Yes.                                                               */
  /*                                                                       */
  FT_EXPORT_DEF( void )  FT_Outline_Get_CBox( FT_Outline*  outline,
                                              FT_BBox*     cbox );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Translate                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Applies a simple translation to the points of an outline.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline :: A pointer to the target outline descriptor.             */
  /*                                                                       */
  /*    xOffset :: The horizontal offset.                                  */
  /*                                                                       */
  /*    yOffset :: The vertical offset.                                    */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    Yes.                                                               */
  /*                                                                       */
  FT_EXPORT_DEF( void )  FT_Outline_Translate( FT_Outline*  outline,
                                               FT_Pos       xOffset,
                                               FT_Pos       yOffset );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Copy                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Copies an outline into another one.  Both objects must have the    */
  /*    same sizes (number of points & number of contours) when this       */
  /*    function is called.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    source :: A handle to the source outline.                          */
  /*                                                                       */
  /* <Output>                                                              */
  /*    target :: A handle to the target outline.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Outline_Copy( FT_Outline*  source,
                                              FT_Outline*  target );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Vector_Transform                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Transforms a single vector through a 2x2 matrix.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    vector :: The target vector to transform.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    matrix :: A pointer to the source 2x2 matrix.                      */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    Yes.                                                               */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The result is undefined if either `vector' or `matrix' is invalid. */
  /*                                                                       */
  FT_EXPORT_DEF( void )  FT_Outline_Transform( FT_Outline*  outline,
                                               FT_Matrix*   matrix );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Reverse                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Reverses the drawing direction of an outline.  This is used to     */
  /*    ensure consistent fill conventions for mirrored glyphs.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline :: A pointer to the target outline descriptor.             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This functions toggles the bit flag `ft_outline_reverse_fill' in   */
  /*    the outline's `flags' field.                                       */
  /*                                                                       */
  /*    It shouldn't be used by a normal client application, unless it     */
  /*    knows what it is doing.                                            */
  /*                                                                       */
  FT_EXPORT_DEF( void )  FT_Outline_Reverse( FT_Outline*  outline );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Get_Bitmap                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Renders an outline within a bitmap.  The outline's image is simply */
  /*    OR-ed to the target bitmap.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to a FreeType library object.                  */
  /*                                                                       */
  /*    outline :: A pointer to the source outline descriptor.             */
  /*                                                                       */
  /*    map     :: A pointer to the target bitmap descriptor.              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    YES.  Rendering is synchronized, so that concurrent calls to the   */
  /*    scan-line converter will be serialized.                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT CREATE the bitmap, it only renders an       */
  /*    outline image within the one you pass to it!                       */
  /*                                                                       */
  /*    It will use the raster correponding to the default glyph format.   */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Outline_Get_Bitmap( FT_Library   library,
                                                    FT_Outline*  outline,
                                                    FT_Bitmap*   bitmap );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Render                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Renders an outline within a bitmap using the current scan-convert. */
  /*    This functions uses an FT_Raster_Params structure as an argument,  */
  /*    allowing advanced features like direct composition, translucency,  */
  /*    etc.                                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to a FreeType library object.                  */
  /*                                                                       */
  /*    outline :: A pointer to the source outline descriptor.             */
  /*                                                                       */
  /*    params  :: A pointer to a FT_Raster_Params structure used to       */
  /*               describe the rendering operation.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <MT-Note>                                                             */
  /*    YES.  Rendering is synchronized, so that concurrent calls to the   */
  /*    scan-line converter will be serialized.                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should know what you are doing and how FT_Raster_Params works  */
  /*    to use this function.                                              */
  /*                                                                       */
  /*    The field `params.source' will be set to `outline' before the scan */
  /*    converter is called, which means that the value you give to it is  */
  /*    actually ignored.                                                  */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )  FT_Outline_Render( FT_Library         library,
                                                FT_Outline*        outline,
                                                FT_Raster_Params*  params );


#ifdef __cplusplus
  }
#endif


#endif /* FTOUTLN_H */


/* END */
