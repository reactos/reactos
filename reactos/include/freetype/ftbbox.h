/***************************************************************************/
/*                                                                         */
/*  ftbbox.h                                                               */
/*                                                                         */
/*    FreeType bbox computation (specification).                           */
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
  /* This component has a _single_ role: to compute exact outline bounding */
  /* boxes.                                                                */
  /*                                                                       */
  /* It is separated from the rest of the engine for various technical     */
  /* reasons.  It may well be integrated in `ftoutln' later.               */
  /*                                                                       */
  /*************************************************************************/


#ifndef FTBBOX_H
#define FTBBOX_H

#include <freetype/freetype.h>

#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Raster_GetBBox                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the exact bounding box of an outline.  This is slower     */
  /*    than computing the control box.  However, it uses an advanced      */
  /*    algorithm which returns _very_ quickly when the two boxes          */
  /*    coincide.  Otherwise, the outline Bezier arcs are walked over to   */
  /*    extract their extrema.                                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline :: A pointer to the source outline.                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    bbox    :: The outline's exact bounding box.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  FT_EXPORT_DEF(FT_Error)  FT_Raster_GetBBox( FT_Outline*  outline,
                                              FT_BBox*     abbox );


#ifdef __cplusplus
  }
#endif

#endif /* FTBBOX_H */


/* END */
