/***************************************************************************/
/*                                                                         */
/*  cffload.h                                                              */
/*                                                                         */
/*    OpenType & CFF data/program tables loader (specification).           */
/*                                                                         */
/*  Copyright 1996-2001, 2002 by                                           */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __CFFLOAD_H__
#define __CFFLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_CFF_TYPES_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H


FT_BEGIN_HEADER

  FT_LOCAL( FT_UShort )
  cff_get_standard_encoding( FT_UInt  charcode );


  FT_LOCAL( FT_String* )
  cff_index_get_name( CFF_Index  idx,
                      FT_UInt    element );

  FT_LOCAL( FT_String* )
  cff_index_get_sid_string( CFF_Index        idx,
                            FT_UInt          sid,
                            PSNames_Service  psnames_interface );


  FT_LOCAL( FT_Error )
  cff_index_access_element( CFF_Index  idx,
                            FT_UInt    element,
                            FT_Byte**  pbytes,
                            FT_ULong*  pbyte_len );

  FT_LOCAL( void )
  cff_index_forget_element( CFF_Index  idx,
                            FT_Byte**  pbytes );


  FT_LOCAL( FT_Error )
  cff_font_load( FT_Stream  stream,
                 FT_Int     face_index,
                 CFF_Font   font );

  FT_LOCAL( void )
  cff_font_done( CFF_Font  font );


  FT_LOCAL( FT_Byte )
  cff_fd_select_get( CFF_FDSelect  select,
                     FT_UInt       glyph_index );


FT_END_HEADER

#endif /* __CFFLOAD_H__ */


/* END */
