/***************************************************************************/
/*                                                                         */
/*  ftmm.c                                                                 */
/*                                                                         */
/*    Multiple Master font support (body).                                 */
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


#include <freetype/ftmm.h>
#include <freetype/internal/ftobjs.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_mm


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Multi_Master                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the Multiple Master descriptor of a given font.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the source face.                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    master :: The Multiple Masters descriptor.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Get_Multi_Master( FT_Face           face,
                                                   FT_Multi_Master*  master )
  {
    FT_Error  error;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_Driver       driver = face->driver;
      FT_Get_MM_Func  func;


      func = (FT_Get_MM_Func)driver->root.clazz->get_interface(
                               FT_MODULE( driver ), "get_mm" );
      if ( func )
        error = func( face, master );
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_MM_Design_Coordinates                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    For Multiple Masters fonts, choose an interpolated font design     */
  /*    through design coordinates.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the source face.                         */
  /*                                                                       */
  /*    num_coords :: The number of design coordinates (must be equal to   */
  /*                  the number of axes in the font).                     */
  /*                                                                       */
  /*    coords     :: The design coordinates.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Set_MM_Design_Coordinates(
                                FT_Face   face,
                                FT_UInt   num_coords,
                                FT_Long*  coords )
  {
    FT_Error  error;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_Driver              driver = face->driver;
      FT_Set_MM_Design_Func  func;


      func = (FT_Set_MM_Design_Func)driver->root.clazz->get_interface(
                                      FT_MODULE( driver ), "set_mm_design" );
      if ( func )
        error = func( face, num_coords, coords );
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_MM_Blend_Coordinates                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    For Multiple Masters fonts, choose an interpolated font design     */
  /*    through normalized blend coordinates.                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the source face.                         */
  /*                                                                       */
  /*    num_coords :: The number of design coordinates (must be equal to   */
  /*                  the number of axes in the font).                     */
  /*                                                                       */
  /*    coords     :: The design coordinates (each one must be between 0   */
  /*                  and 1.0).                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Set_MM_Blend_Coordinates(
                                FT_Face    face,
                                FT_UInt    num_coords,
                                FT_Fixed*  coords )
  {
    FT_Error  error;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_Driver             driver = face->driver;
      FT_Set_MM_Blend_Func  func;


      func = (FT_Set_MM_Blend_Func)driver->root.clazz->get_interface(
                                     FT_MODULE( driver ), "set_mm_blend" );
      if ( func )
        error = func( face, num_coords, coords );
    }

    return error;
  }


/* END */
