/***************************************************************************/
/*                                                                         */
/*  cidtokens.h                                                            */
/*                                                                         */
/*    CID token definitions (specification only).                          */
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


#undef  T1TYPE
#undef  T1CODE
#define T1TYPE  CID_Info
#define T1CODE  t1_field_cid_info

  CID_FIELD_STRING  ( "CIDFontName", cid_font_name )
  CID_FIELD_NUM     ( "CIDFontVersion", cid_version )
  CID_FIELD_NUM     ( "CIDFontType", cid_font_type )
  CID_FIELD_STRING  ( "Registry", registry )
  CID_FIELD_STRING  ( "Ordering", ordering )
  CID_FIELD_NUM     ( "Supplement", supplement )
  CID_FIELD_CALLBACK( "FontBBox", font_bbox )
  CID_FIELD_NUM     ( "UIDBase", uid_base )
  CID_FIELD_CALLBACK( "FDArray", fd_array )
  CID_FIELD_NUM     ( "CIDMapOffset", cidmap_offset )
  CID_FIELD_NUM     ( "FDBytes", fd_bytes )
  CID_FIELD_NUM     ( "GDBytes", gd_bytes )
  CID_FIELD_NUM     ( "CIDCount", cid_count )


#undef  T1TYPE
#undef  T1CODE
#define T1TYPE  T1_FontInfo
#define T1CODE  t1_field_font_info

  CID_FIELD_STRING( "version", version )
  CID_FIELD_STRING( "Notice", notice )
  CID_FIELD_STRING( "FullName", full_name )
  CID_FIELD_STRING( "FamilyName", family_name )
  CID_FIELD_STRING( "Weight", weight )
  CID_FIELD_FIXED ( "ItalicAngle", italic_angle )
  CID_FIELD_BOOL  ( "isFixedPitch", is_fixed_pitch )
  CID_FIELD_NUM   ( "UnderlinePosition", underline_position )
  CID_FIELD_NUM   ( "UnderlineThickness", underline_thickness )


#undef  T1TYPE
#undef  T1CODE
#define T1TYPE CID_FontDict
#define T1CODE t1_field_font_dict

  CID_FIELD_CALLBACK( "FontMatrix", font_matrix )
  CID_FIELD_NUM     ( "PaintType", paint_type )
  CID_FIELD_NUM     ( "FontType", font_type )
  CID_FIELD_NUM     ( "SubrMapOffset", subrmap_offset )
  CID_FIELD_NUM     ( "SDBytes", sd_bytes )
  CID_FIELD_NUM     ( "SubrCount", num_subrs )
  CID_FIELD_NUM     ( "lenBuildCharArray", len_buildchar )
  CID_FIELD_FIXED   ( "ForceBoldThreshold", forcebold_threshold )
  CID_FIELD_FIXED   ( "ExpansionFactor", expansion_factor )
  CID_FIELD_NUM     ( "StrokeWidth", stroke_width )


#undef  T1TYPE
#undef  T1CODE
#define T1TYPE  T1_Private
#define T1CODE  t1_field_private

  CID_FIELD_NUM       ( "UniqueID", unique_id )
  CID_FIELD_NUM       ( "lenIV", lenIV )
  CID_FIELD_NUM       ( "LanguageGroup", language_group )
  CID_FIELD_NUM       ( "password", password )

  CID_FIELD_FIXED     ( "BlueScale", blue_scale )
  CID_FIELD_NUM       ( "BlueShift", blue_shift )
  CID_FIELD_NUM       ( "BlueFuzz",  blue_fuzz )

  CID_FIELD_NUM_TABLE ( "BlueValues", blue_values, 14 )
  CID_FIELD_NUM_TABLE ( "OtherBlues", other_blues, 10 )
  CID_FIELD_NUM_TABLE ( "FamilyBlues", family_blues, 14 )
  CID_FIELD_NUM_TABLE ( "FamilyOtherBlues", family_other_blues, 10 )

  CID_FIELD_NUM_TABLE2( "StdHW", standard_width,  1 )
  CID_FIELD_NUM_TABLE2( "StdVW", standard_height, 1 )
  CID_FIELD_NUM_TABLE2( "MinFeature", min_feature, 2 )

  CID_FIELD_NUM_TABLE ( "StemSnapH", snap_widths, 12 )
  CID_FIELD_NUM_TABLE ( "StemSnapV", snap_heights, 12 )


/* END */
