/***************************************************************************/
/*                                                                         */
/*  ttload.h                                                               */
/*                                                                         */
/*    Load the basic TrueType tables, i.e., tables that can be either in   */
/*    TTF or OTF fonts (specification).                                    */
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


#ifndef TTLOAD_H
#define TTLOAD_H


#include <freetype/internal/ftstream.h>
#include <freetype/internal/tttypes.h>


#ifdef __cplusplus
  extern "C" {
#endif


  LOCAL_DEF
  TT_Table*  TT_LookUp_Table( TT_Face   face,
                              FT_ULong  tag );

  LOCAL_DEF
  FT_Error  TT_Goto_Table( TT_Face    face,
                           FT_ULong   tag,
                           FT_Stream  stream,
                           FT_ULong*  length );


  LOCAL_DEF
  FT_Error  TT_Load_SFNT_Header( TT_Face       face,
                                 FT_Stream     stream,
                                 FT_Long       face_index,
                                 SFNT_Header*  sfnt );
  LOCAL_DEF
  FT_Error  TT_Load_Directory( TT_Face       face,
                               FT_Stream     stream,
                               SFNT_Header*  sfnt );

  LOCAL_DEF
  FT_Error  TT_Load_Any( TT_Face    face,
                         FT_ULong   tag,
                         FT_Long    offset,
                         FT_Byte*   buffer,
                         FT_ULong*  length );


  LOCAL_DEF
  FT_Error  TT_Load_Header( TT_Face    face,
                            FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_Metrics_Header( TT_Face    face,
                                    FT_Stream  stream,
                                    FT_Bool    vertical );


  LOCAL_DEF
  FT_Error  TT_Load_CMap( TT_Face    face,
                          FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_MaxProfile( TT_Face    face,
                                FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_Names( TT_Face    face,
                           FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_OS2( TT_Face    face,
                         FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_PostScript( TT_Face    face,
                                FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_Hdmx( TT_Face    face,
                          FT_Stream  stream );

  LOCAL_DEF
  FT_Error  TT_Load_PCLT( TT_Face    face,
                          FT_Stream  stream );

  LOCAL_DEF
  void  TT_Free_Names( TT_Face  face );


  LOCAL_DEF
  void  TT_Free_Hdmx ( TT_Face  face );


  LOCAL_DEF
  FT_Error  TT_Load_Kern( TT_Face    face,
                          FT_Stream  stream );


  LOCAL_DEF
  FT_Error  TT_Load_Gasp( TT_Face    face,
                          FT_Stream  stream );


#ifdef __cplusplus
  }
#endif


#endif /* TTLOAD_H */


/* END */
