/***************************************************************************/
/*                                                                         */
/*  t2tokens.h                                                             */
/*                                                                         */
/*    OpenType token definitions (specification only).                     */
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


#undef  T2TYPE
#undef  T2CODE
#define T2TYPE  CFF_Font_Dict
#define T2CODE  T2CODE_TOPDICT

  T2_FIELD_STRING  ( 0, version )
  T2_FIELD_STRING  ( 1, notice )
  T2_FIELD_STRING  ( 0x100, copyright )
  T2_FIELD_STRING  ( 2, full_name )
  T2_FIELD_STRING  ( 3, family_name )
  T2_FIELD_STRING  ( 4, weight )
  T2_FIELD_BOOL    ( 0x101, is_fixed_pitch )
  T2_FIELD_FIXED   ( 0x102, italic_angle )
  T2_FIELD_NUM     ( 0x103, underline_position )
  T2_FIELD_NUM     ( 0x104, underline_thickness )
  T2_FIELD_NUM     ( 0x105, paint_type )
  T2_FIELD_NUM     ( 0x106, charstring_type )
  T2_FIELD_CALLBACK( 0x107, font_matrix )
  T2_FIELD_NUM     ( 13, unique_id )
  T2_FIELD_CALLBACK( 5, font_bbox )
  T2_FIELD_NUM     ( 0x108, stroke_width )
  T2_FIELD_NUM     ( 15, charset_offset )
  T2_FIELD_NUM     ( 16, encoding_offset )
  T2_FIELD_NUM     ( 17, charstrings_offset )
  T2_FIELD_CALLBACK( 18, private_dict )
  T2_FIELD_NUM     ( 0x114, synthetic_base )
  T2_FIELD_STRING  ( 0x115, postscript )
  T2_FIELD_STRING  ( 0x116, base_font_name )

#if 0
  T2_FIELD_DELTA   ( 0x117, base_font_blend, 16 )
  T2_FIELD_CALLBACK( 0x118, multiple_master )
  T2_FIELD_CALLBACK( 0x119, blend_axit_types )
#endif

  T2_FIELD_CALLBACK( 0x11E, cid_ros )
  T2_FIELD_NUM     ( 0x11F, cid_font_version )
  T2_FIELD_NUM     ( 0x120, cid_font_revision )
  T2_FIELD_NUM     ( 0x121, cid_font_type )
  T2_FIELD_NUM     ( 0x122, cid_count )
  T2_FIELD_NUM     ( 0x123, cid_uid_base )
  T2_FIELD_NUM     ( 0x124, cid_fd_array_offset )
  T2_FIELD_NUM     ( 0x125, cid_fd_select_offset )
  T2_FIELD_STRING  ( 0x126, cid_font_name )

#if 0
  T2_FIELD_NUM     ( 0x127, chameleon )
#endif


#undef  T2TYPE
#undef  T2CODE
#define T2TYPE  CFF_Private
#define T2CODE  T2CODE_PRIVATE

  T2_FIELD_DELTA( 6, blue_values, 14 )
  T2_FIELD_DELTA( 7, other_blues, 10 )
  T2_FIELD_DELTA( 8, family_blues, 14 )
  T2_FIELD_DELTA( 9, family_other_blues, 10 )
  T2_FIELD_FIXED( 0x109, blue_scale )
  T2_FIELD_NUM  ( 0x10A, blue_shift )
  T2_FIELD_NUM  ( 0x10B, blue_fuzz )
  T2_FIELD_NUM  ( 10, standard_width )
  T2_FIELD_NUM  ( 11, standard_height )
  T2_FIELD_DELTA( 0x10C, snap_widths, 13 )
  T2_FIELD_DELTA( 0x10D, snap_heights, 13 )
  T2_FIELD_BOOL ( 0x10E, force_bold )
  T2_FIELD_FIXED( 0x10F, force_bold_threshold )
  T2_FIELD_NUM  ( 0x110, lenIV )
  T2_FIELD_NUM  ( 0x111, language_group )
  T2_FIELD_FIXED( 0x112, expansion_factor )
  T2_FIELD_NUM  ( 0x113, initial_random_seed )
  T2_FIELD_NUM  ( 19, local_subrs_offset )
  T2_FIELD_NUM  ( 20, default_width )
  T2_FIELD_NUM  ( 21, nominal_width )


/* END */
