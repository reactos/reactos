#ifndef OLTYPES_H
#define OLTYPES_H

#include <ftobjs.h>
#include <tttypes.h>

 /*************************************************************
  *
  * <Struct> OTL_Table
  *
  * <Description>
  *    The base table of most OpenType Layout sub-tables.
  *    Provides a simple way to scan a table for script,
  *    languages, features and lookups..
  *
  * <Fields>
  *    num_scripts  :: number of scripts in table's script list
  *    script_tags  :: array of tags for each table script
  *
  *    max_languages :: max number of languages for any script in
  *                     the table.
  *    num_languages :: number of languages available for current script
  *    language_tags :: tags of all languages available for current script.
  *
  *    max_features  :: total number of features in table
  *    feature_tags  :: tags of all features for current script/language
  *    features      :: selection flags for all features in current script/lang
  *
  *    max_lookups   :: total number of lookups in table
  *    lookups       :: selection flags for all lookups for current
  *                     feature list.
  *
  ****************************************************************/

  typedef enum OTL_Type_
  {
    otl_type_none = 0,
    otl_type_base,
    otl_type_gdef,
    otl_type_gpos,
    otl_type_gsub,
    otl_type_jstf

  } OTL_Type;


  typedef struct OTL_Table_
  {
    FT_Memory memory;

    TT_Int    num_scripts;
    TT_Tag*   script_tags;

    TT_Int    max_languages;
    TT_Int    num_languages;
    TT_Tag*   language_tags;

    TT_Int    max_features;
    TT_Tag*   feature_tags;
    TT_Bool*  features;

    TT_Int    max_lookups;
    TT_Bool*  lookups;

    TT_Byte*  scripts_table;
    TT_Long   scripts_len;

    TT_Byte*  features_table;
    TT_Long*  features_len;

    TT_Byte*  lookups_table;
    TT_Byte*  lookups_len;

    TT_Byte*  cur_script;   /* current script   */
    TT_Byte*  cur_language; /* current language */

    TT_Byte*  cur_base_values;
    TT_Byte*  cur_min_max;

    OTL_Type  otl_type;

  } OTL_Table;


  typedef struct OTL_BaseCoord_
  {
    TT_UShort  format;
    TT_Short   coordinate;
    TT_UShort  ref_glyph;
    TT_UShort  ref_point;
    TT_Byte*   device;

  } OTL_BaseCoord;


  typedef struct OTL_ValueRecord_
  {
    TT_Vector  placement;
    TT_Vector  advance;

    TT_Byte*   device_pla_x;
    TT_Byte*   device_pla_y;
    TT_Byte*   device_adv_x;
    TT_Byte*   device_adv_y;

  } OTL_ValueRecord;


  typedef struct OTL_Anchor_
  {
    TT_UInt    format;
    TT_Vector  coord;
    TT_UInt    anchor_point;
    TT_Byte*   device_x;
    TT_Byte*   device_y;

  } OTL_Anchor;

  LOCAL_DEF
  TT_Error  OTL_Table_Init( OTL_Table*  table,
                            FT_Memory   memory );

  LOCAL_DEF
  TT_Error  OTL_Table_Set_Scripts( OTL_Table*  table,
                                   TT_Byte*    bytes,
                                   TT_Long     len,
                                   OTL_Type    otl_type );

  LOCAL_DEF
  TT_Error  OTL_Table_Set_Features( OTL_Table*  table,
                                    TT_Byte*    bytes,
                                    TT_Long     len );

  LOCAL_DEF
  TT_Error  OTL_Table_Set_Lookups( OTL_Table*  table,
                                   TT_Byte*    bytes,
                                   TT_Long     len );

  LOCAL_DEF
  void      OTL_Table_Done( OTL_Table*  table );



/*****************************************************
 *
 *  Typical uses:
 *
 *  - after OTL_Table_Set_Scripts have been called :
 *
 *      table->script_tags contains the list of tags of all
 *      scripts defined for this table.
 *
 *      table->num_scripts is the number of scripts
 *
 */

/********************************************************
 *
 *  - after calling OTL_Table_Set_Features:
 *
 *      table->max_features is the number of all features
 *      in the table
 *
 *      table->feature_tags is the list of tags of all
 *      features in the table
 *
 *      table->features[] is an array of boolean used to
 *      indicate which feature is active for a given script/language
 *      it is empty (zero-filled) by default.
 *
 */

/*******************************************************************
 *
 *  - after calling OTL_Get_Languages_List(script_tag):
 *
 *      table->num_languages is the number of language systems
 *      available for the script, including the default
 *      langsys if there is one
 *
 *      table->language_tags contains the list of tags of all
 *      languages for the script. Note that the default langsys
 *      has tag "0" and is always placed first in "language_tags".
 *
 *
 *
 */
  LOCAL_DEF
  void  OTL_Get_Languages_List( OTL_Table*  table,
                                TT_ULong    script_tag );


/*******************************************************************
 *
 *  - after calling OTL_Get_Features_List(language_tag):
 *
 *      table->features[] is an array of booleans used to indicate
 *      which features are active for the current script/language
 *
 *      note that this function must be called after OTL_Get_Languages
 *      which remembers the last "script_tag" used..
 *
 *      A client application can change the table->features[] array
 *      to add or remove features from the list.
 *
 *
 *
 */
  LOCAL_DEF
  void OTL_Get_Features_List( OTL_Table*  table,
                              TT_ULong    language_tag );

  LOCAL_DEF
  void OTL_Get_Baseline_Values( OTL_Table*  table,
                                TT_ULong    language_tag );

  LOCAL_DEF
  void OTL_Get_Justification( OTL_Table*  table,
                              TT_ULong    language_tag );

/*******************************************************************
 *
 *  - after calling OTL_Get_Lookups_List():
 *
 *      The function uses the table->features[] array of boolean
 *      to determine which lookups must be processed.
 *
 *      It fills the table->lookups[] array accordingly. It is also
 *      an array of booleans (one for each lookup).
 *
 *
 */

  LOCAL_DEF
  void  OTL_Get_Lookups_List( OTL_Table*  table );


/***************************************************************
 *
 *  So the whole thing looks like:
 *
 *
 *  1.  A client specifies a given script and requests available
 *      language through OTL_Get_Languages_List()
 *
 *  2.  It selects the language tag it needs, then calls
 *      OTL_Get_Features_List()
 *
 *  3.  It updates the list of active features if it needs to
 *
 *  4.  It calls OTL_Get_Lookups_List()
 *      It now has a list of active lookups in "table->lookups[]"
 *
 *  5.  The lookups are processed according to the table's type..
 *
 */




  LOCAL_DEF
  TT_Long  OTL_Get_Coverage_Index( TT_Byte*  coverage,
                                   TT_UInt   glyph_id );

  LOCAL_DEF
  TT_UInt  OTL_Get_Glyph_Class( TT_Byte*  class_def,
                                TT_UInt   glyph_id );

  LOCAL_DEF
  TT_Int  OTL_Get_Device_Adjustment( TT_Byte* device,
                                     TT_UInt  size );

  LOCAL_DEF
  void    OTL_Get_Base_Coordinate( TT_Byte*        base_coord,
                                   OTL_BaseCoord*  coord );


  LOCAL_DEF
  TT_Int  OTL_ValueRecord_Size( TT_UShort  value_format );


  LOCAL_DEF
  void  OTL_Get_ValueRecord( TT_Byte*          value_record,
                             TT_UShort         value_format,
			     TT_Byte*          pos_table,
			     OTL_ValueRecord*  record );


  LOCAL_DEF
  void  OTL_Get_Anchor( TT_Byte*     anchor_table,
                        OTL_Anchor*  anchor );


  LOCAL_DEF
  void  OTL_Get_Mark( TT_Byte*     mark_array,
                      TT_UInt      index,
		      TT_UShort*   clazz,
		      OTL_Anchor*  anchor );



#define OTL_Byte(p)   (p++, p[-1])

#define OTL_UShort(p) (p+=2, ((TT_UShort)p[-2] << 8) | p[-1])

#define OTL_ULong(p)  (p+=4, ((TT_ULong)p[-4] << 24) |        \
                             ((TT_ULong)p[-3] << 16) |        \
                             ((TT_ULong)p[-2] << 8 ) | p[-1] )

#endif /* OLTYPES_H */
