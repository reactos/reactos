/***************************************************************************/
/*                                                                         */
/*  ftwinfnt.h                                                             */
/*                                                                         */
/*    FreeType API for accessing Windows fnt-specific data.                */
/*                                                                         */
/*  Copyright 2002 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTWINFNT_H__
#define __FTWINFNT_H__

#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    winfnt_fonts                                                       */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Window FNT Fonts                                                   */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    Windows FNT specific APIs                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the declaration of Windows FNT specific      */
  /*    functions.                                                         */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_WinFNT_HeaderRec_                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Windows FNT Header info.                                           */
  /*                                                                       */
  typedef struct  FT_WinFNT_HeaderRec_
  {
    FT_UShort  version;
    FT_ULong   file_size;
    FT_Byte    copyright[60];
    FT_UShort  file_type;
    FT_UShort  nominal_point_size;
    FT_UShort  vertical_resolution;
    FT_UShort  horizontal_resolution;
    FT_UShort  ascent;
    FT_UShort  internal_leading;
    FT_UShort  external_leading;
    FT_Byte    italic;
    FT_Byte    underline;
    FT_Byte    strike_out;
    FT_UShort  weight;
    FT_Byte    charset;
    FT_UShort  pixel_width;
    FT_UShort  pixel_height;
    FT_Byte    pitch_and_family;
    FT_UShort  avg_width;
    FT_UShort  max_width;
    FT_Byte    first_char;
    FT_Byte    last_char;
    FT_Byte    default_char;
    FT_Byte    break_char;
    FT_UShort  bytes_per_row;
    FT_ULong   device_offset;
    FT_ULong   face_name_offset;
    FT_ULong   bits_pointer;
    FT_ULong   bits_offset;
    FT_Byte    reserved;
    FT_ULong   flags;
    FT_UShort  A_space;
    FT_UShort  B_space;
    FT_UShort  C_space;
    FT_UShort  color_table_offset;
    FT_ULong   reserved1[4];

  } FT_WinFNT_HeaderRec, *FT_WinFNT_Header;



 /**********************************************************************
  *
  * @function:
  *    FT_Get_WinFNT_Header
  *
  * @description:
  *    Retrieves a Windows FNT font info header.
  *
  * @input:
  *    face   :: handle to input face
  *
  * @output:
  *    header :: WinFNT header.
  *
  * @return:
  *   FreeType error code.  0 means success.
  *
  * @note:
  *   This function only works with Windows FNT faces, returning an erro
  *   otherwise.
  */
  FT_EXPORT( FT_Error )
  FT_Get_WinFNT_Header( FT_Face              face,
                        FT_WinFNT_HeaderRec *header );

 /* */

FT_END_HEADER

#endif /* __FTWINFNT_H__ */


/* END */
