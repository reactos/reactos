/***************************************************************************/
/*                                                                         */
/*  z1tokens.h                                                             */
/*                                                                         */
/*    Experimental Type 1 tokenizer (specification).                       */
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
#define T1TYPE  T1_FontInfo

  Z1_FONTINFO_STRING( "version", version )
  Z1_FONTINFO_STRING( "Notice", notice )
  Z1_FONTINFO_STRING( "FullName", full_name )
  Z1_FONTINFO_STRING( "FamilyName", family_name )
  Z1_FONTINFO_STRING( "Weight", weight )

  Z1_FONTINFO_NUM   ( "ItalicAngle", italic_angle )
  Z1_FONTINFO_BOOL  ( "isFixedPitch", is_fixed_pitch )
  Z1_FONTINFO_NUM   ( "UnderlinePosition", underline_position )
  Z1_FONTINFO_NUM   ( "UnderlineThickness", underline_thickness )


#undef  T1TYPE
#define T1TYPE  T1_Private

  Z1_PRIVATE_NUM       ( "UniqueID", unique_id )
  Z1_PRIVATE_NUM       ( "lenIV", lenIV )
  Z1_PRIVATE_NUM       ( "LanguageGroup", language_group )
  Z1_PRIVATE_NUM       ( "password", password )

  Z1_PRIVATE_FIXED     ( "BlueScale", blue_scale )
  Z1_PRIVATE_NUM       ( "BlueShift", blue_shift )
  Z1_PRIVATE_NUM       ( "BlueFuzz",  blue_fuzz )

  Z1_PRIVATE_NUM_TABLE ( "BlueValues", blue_values, 14, num_blue_values )
  Z1_PRIVATE_NUM_TABLE ( "OtherBlues", other_blues, 10, num_other_blues )
  Z1_PRIVATE_NUM_TABLE ( "FamilyBlues", family_blues, 14, num_family_blues )
  Z1_PRIVATE_NUM_TABLE ( "FamilyOtherBlues", family_other_blues, 10, \
                                             num_family_other_blues )

  Z1_PRIVATE_NUM_TABLE2( "StdHW", standard_width,  1 )
  Z1_PRIVATE_NUM_TABLE2( "StdVW", standard_height, 1 )
  Z1_PRIVATE_NUM_TABLE2( "MinFeature", min_feature, 2 )

  Z1_PRIVATE_NUM_TABLE ( "StemSnapH", snap_widths, 12, num_snap_widths )
  Z1_PRIVATE_NUM_TABLE ( "StemSnapV", snap_heights, 12, num_snap_heights )


#undef  T1TYPE
#define T1TYPE  T1_Font

  Z1_TOPDICT_NUM( "PaintType", paint_type )
  Z1_TOPDICT_NUM( "FontType", font_type )
  Z1_TOPDICT_NUM( "StrokeWidth", stroke_width )


#if 0

 /* define the font info dictionary parsing callbacks */
#undef  FACE
#define FACE  (face->type1.font_info)

  PARSE_STRING( "version", version )
  PARSE_STRING( "Notice", notice )
  PARSE_STRING( "FullName", full_name )
  PARSE_STRING( "FamilyName", family_name )
  PARSE_STRING( "Weight", weight )

  PARSE_INT   ( "ItalicAngle", italic_angle )
  PARSE_BOOL  ( "isFixedPitch", is_fixed_pitch )
  PARSE_NUM   ( "UnderlinePosition", underline_position, FT_Short )
  PARSE_NUM   ( "UnderlineThickness", underline_thickness, FT_UShort )


  /* define the private dict parsing callbacks */
#undef  FACE
#define FACE  (face->type1.private_dict)

  PARSE_INT    ("UniqueID", unique_id )
  PARSE_INT    ("lenIV", lenIV )

  PARSE_COORDS ( "BlueValues", num_blues, 14, blue_values)
  PARSE_COORDS ( "OtherBlues", num_other_blues, 10, other_blues)

  PARSE_COORDS ( "FamilyBlues", num_family_blues, 14, family_blues )
  PARSE_COORDS ( "FamilyOtherBlues", num_family_other_blues, 10,
                                     family_other_blues )

  PARSE_FIXED  ( "BlueScale", blue_scale )
  PARSE_INT    ( "BlueShift", blue_shift )

  PARSE_INT    ( "BlueFuzz", blue_fuzz )

  PARSE_COORDS2( "StdHW", 1, standard_width )
  PARSE_COORDS2( "StdVW", 1, standard_height )

  PARSE_COORDS ( "StemSnapH", num_snap_widths, 12, stem_snap_widths )
  PARSE_COORDS ( "StemSnapV", num_snap_heights, 12, stem_snap_heights )

  PARSE_INT    ( "LanguageGroup", language_group )
  PARSE_INT    ( "password", password )
  PARSE_COORDS2( "MinFeature", 2, min_feature )


  /* define the top-level dictionary parsing callbacks */
#undef  FACE
#define FACE  (face->type1)

/*PARSE_STRING ( "FontName", font_name ) -- handled by special routine */
  PARSE_NUM    ( "PaintType", paint_type, FT_Byte )
  PARSE_NUM    ( "FontType", font_type, FT_Byte )
  PARSE_FIXEDS2( "FontMatrix", 4, font_matrix )
/*PARSE_COORDS2( "FontBBox", 4, font_bbox ) -- handled by special routine */
  PARSE_INT    ( "StrokeWidth", stroke_width )

#undef FACE

#endif /* 0 */


/* END */
