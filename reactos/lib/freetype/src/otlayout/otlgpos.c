#include "otlgpos.h"
#include "otlcommn.h"

 /* forward declaration */
  static OTL_ValidateFunc  otl_gpos_validate_funcs[];


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                         VALUE RECORDS                        *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static OTL_UInt
  otl_value_length( OTL_UInt  format )
  {
    FT_UInt  count;

    count = (( format & 0xAA ) >> 1) + ( format & 0x55 );
    count = (( count  & 0xCC ) >> 2) + ( count  & 0x33 );
    count = (( count  & 0xF0 ) >> 4) + ( count  & 0x0F );

    return count;
  }


  static void
  otl_value_validate( OTL_Bytes      table,
                      OTL_Bytes      pos_table,
                      OTL_UInt       format,
                      OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count, device;

    if ( format >= 0x100U )
      OTL_INVALID_DATA;

    for ( count = 4; count > 0; count-- )
    {
      if ( format & 1 )
      {
        OTL_CHECK( 2 );
        p += 2;
      }

      format >>= 1;
    }

    for ( count = 4; count > 0; count-- )
    {
      if ( format & 1 )
      {
        OTL_CHECK( 2 );
        device = OTL_NEXT_USHORT( p );
        if ( device )
          otl_device_table_validate( pos_table + device, valid );
      }
      format >>= 1;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                          ANCHORS                             *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_anchor_validate( OTL_Bytes      table,
                       OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 6 );
    format = OTL_NEXT_USHORT( p );
    p += 4;

    switch ( format )
    {
      case 1:
        break;

      case 2:
        OTL_CHECK( 2 );  /* anchor point */
        break;

      case 3:
        {
          OTL_UInt  x_device, y_device;

          OTL_CHECK( 4 );
          x_device = OTL_NEXT_USHORT( p );
          y_device = OTL_NEXT_USHORT( p );

          if ( x_device )
            otl_device_table_validate( table + x_device, valid );

          if ( y_device )
            otl_device_table_validate( table + y_device, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                           MARK ARRAY                         *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_mark_array_validate( OTL_Bytes      table,
                           OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( count * 4 );
    for ( ; count > 0; count-- )
    {
      p += 2;  /* ignore class index */
      otl_anchor_validate( table + OTL_NEXT_USHORT( p ), valid );
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 1                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_gpos_lookup1_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          FT_UInt  coverage, value_format;

          OTL_CHECK( 4 );
          coverage     = OTL_NEXT_USHORT( p );
          value_format = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );
          otl_value_validate( p, table, value_format, valid );
        }
        break;

      case 2:
        {
          FT_UInt  coverage, value_format, count, len;

          OTL_CHECK( 6 );
          coverage     = OTL_NEXT_USHORT( p );
          value_format = OTL_NEXT_USHORT( p );
          count        = OTL_NEXT_USHORT( p );
          len          = otl_value_length( value_format );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( count * len );
          for ( ; count > 0; count-- )
          {
            otl_value_validate( p, table, value_format, valid );
            p += len;
          }
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 2                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static otl_gpos_pairset_validate( OTL_Bytes      table,
                                    OTL_Bytes      pos_table,
                                    OTL_UInt       format1,
                                    OTL_UInt       format2,
                                    OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   len1, len2, count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );
    len1  = otl_value_length( format1 );
    len2  = otl_value_length( format2 );

    OTL_CHECK( count * (len1+len2+2) );
    for ( ; count > 0; count-- )
    {
      p += 2;  /* ignore glyph id */
      otl_value_validate( p, pos_table, format1, valid );
      p += len1;

      otl_value_validate( p, pos_table, format2, valid );
      p += len2;
    }
  }

  static void
  otl_gpos_lookup2_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  coverage, value1, value2, count;

          OTL_CHECK( 8 );
          coverage = OTL_NEXT_USHORT( p );
          value1   = OTL_NEXT_USHORT( p );
          value2   = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( count*2 );
          for ( ; count > 0; count-- )
          {
            otl_gpos_pairset_validate( table + OTL_NEXT_USHORT( p ),
                                       table, value1, value2, valid );
          }
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, value1, value2, class1, class2, count1, count2;
          OTL_UInt  len1, len2;

          OTL_CHECK( 14 );
          coverage = OTL_NEXT_USHORT( p );
          value1   = OTL_NEXT_USHORT( p );
          value2   = OTL_NEXT_USHORT( p );
          class1   = OTL_NEXT_USHORT( p );
          class2   = OTL_NEXT_USHORT( p );
          count1   = OTL_NEXT_USHORT( p );
          count2   = OTL_NEXT_USHORT( p );

          len1 = otl_value_length( value1 );
          len2 = otl_value_length( value2 );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( count1*count2*(len1+len2) );
          for ( ; count1 > 0; count1-- )
          {
            for ( ; count2 > 0; count2-- )
            {
              otl_value_validate( p, table, value1, valid );
              p += len1;

              otl_value_validate( p, table, value2, valid );
              p += len2;
            }
          }
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 3                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_gpos_lookup3_validate( OTL_Bytes  table,
                             OTL_Valid  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  coverage, count, anchor1, anchor2;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( count*4 );
          for ( ; count > 0; count-- )
          {
            anchor1 = OTL_NEXT_USHORT( p );
            anchor2 = OTL_NEXT_USHORT( p );

            if ( anchor1 )
              otl_anchor_validate( table + anchor1, valid );

            if ( anchor2 )
              otl_anchor_validate( table + anchor2, valid );
          }
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 4                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_base_array_validate( OTL_Bytes      table,
                           OTL_UInt       class_count,
                           OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count, count2;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*class_count*2 );
    for ( ; count > 0; count-- )
      for ( count2 = class_count; count2 > 0; count2-- )
        otl_anchor_validate( table + OTL_NEXT_USHORT( p ) );
  }


  static void
  otl_gpos_lookup4_validate( OTL_Bytes  table,
                             OTL_Valid  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  mark_coverage, base_coverage, class_count;
          OTL_UInt  mark_array, base_array;

          OTL_CHECK( 10 );
          mark_coverage = OTL_NEXT_USHORT( p );
          base_coverage = OTL_NEXT_USHORT( p );
          class_count   = OTL_NEXT_USHORT( p );
          mark_array    = OTL_NEXT_USHORT( p );
          base_array    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + mark_coverage, valid );
          otl_coverage_validate( table + base_coverage, valid );

          otl_mark_array_validate( table + mark_array, valid );
          otl_base_array_validate( table, class_count, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 5                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_liga_attach_validate( OTL_Bytes      table,
                            OTL_UInt       class_count,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count, count2;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*class_count*2 );
    for ( ; count > 0; count-- )
      for ( count2 = class_count; class_count > 0; class_count-- )
        otl_anchor_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_liga_array_validate( OTL_Bytes      table,
                           OTL_UInt       class_count,
                           OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count, count2;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );
    for ( ; count > 0; count-- )
      otl_liga_attach_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gpos_lookup5_validate( OTL_Bytes  table,
                             OTL_Valid  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  mark_coverage, lig_coverage, class_count;
          OTL_UInt  mar_array, lig_array;

          OTL_CHECK( 10 );
          mark_coverage = OTL_NEXT_USHORT( p );
          liga_coverage = OTL_NEXT_USHORT( p );
          class_count   = OTL_NEXT_USHORT( p );
          mark_array    = OTL_NEXT_USHORT( p );
          liga_array    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + mark_coverage, valid );
          otl_coverage_validate( table + liga_coverage, valid );

          otl_mark_array_validate( table + mark_array, valid );
          otl_liga_array_validate( table + liga_array, class_count, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 6                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  static void
  otl_mark2_array_validate( OTL_Bytes      table,
                            OTL_UInt       class_count,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count, count2;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*class_count*2 );
    for ( ; count > 0; count-- )
      for ( count2 = class_count; class_count > 0; class_count-- )
        otl_anchor_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gpos_lookup6_validate( OTL_Bytes  table,
                             OTL_Valid  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  coverage1, coverage2, class_count, array1, array2;

          OTL_CHECK( 10 );
          coverage1   = OTL_NEXT_USHORT( p );
          coverage2   = OTL_NEXT_USHORT( p );
          class_count = OTL_NEXT_USHORT( p );
          array1      = OTL_NEXT_USHORT( p );
          array2      = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage1, valid );
          otl_coverage_validate( table + coverage2, valid );

          otl_mark_array_validate( table + array1, valid );
          otl_mark2_array_validate( table + array2, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 7                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_pos_rule_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   glyph_count, pos_count;

    OTL_CHECK( 4 );
    glyph_count = OTL_NEXT_USHORT( p );
    pos_count   = OTL_NEXT_USHORT( p );

    if ( glyph_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( (glyph_count-1)*2 + pos_count*4 );

    /* XXX: check glyph indices and pos lookups */
  }


  static void
  otl_pos_rule_set_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );
    for ( ; count > 0; count-- )
      otl_pos_rule_validate( table + OTL_NEXT_USHORT(p), valid );
  }



  static void
  otl_pos_class_rule_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   glyph_count, pos_count;

    OTL_CHECK( 4 );
    glyph_count = OTL_NEXT_USHORT( p );
    pos_count   = OTL_NEXT_USHORT( p );

    if ( glyph_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( (glyph_count-1)*2 + pos_count*4 );

    /* XXX: check glyph indices and pos lookups */
  }


  static void
  otl_pos_class_set_validate( OTL_Bytes      table,
                              OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );
    for ( ; count > 0; count-- )
      otl_pos_rule_validate( table + OTL_NEXT_USHORT(p), valid );
  }


  static void
  otl_gpos_lookup7_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( count*2 );
          for ( ; count > 0; count-- )
            otl_pos_rule_set_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, class_def, count;

          OTL_CHECK( 6 );
          coverage  = OTL_NEXT_USHORT( p );
          class_def = OTL_NEXT_USHORT( p );
          count     = OTL_NEXT_USHORT( p );

          otl_coverage_validate        ( table + coverage, valid );
          otl_class_definition_validate( table + class_def, valid );

          OTL_CHECK( count*2 );
          for ( ; count > 0; count-- )
            otl_
        }
        break;

      case 3:
        {
          OTL_UInt  glyph_count, pos_count;

          OTL_CHECK( 4 );
          glyph_count = OTL_NEXT_USHORT( p );
          pos_count   = OTL_NEXT_USHORT( p );

          OTL_CHECK( glyph_count*2 + pos_count*4 );
          for ( ; glyph_count > 0; glyph_count )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          /* XXX: check pos lookups */
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 8                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_chain_pos_rule_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   back_count, input_count, ahead_count, pos_count;

    OTL_CHECK( 2 );
    back_count = OTL_NEXT_USHORT( p );

    OTL_CHECK( back_count*2 + 2 );
    p += back_count*2;

    input_count = OTL_NEXT_USHORT( p );
    if ( input_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( input_count*2 );
    p += (input_count-1)*2;

    ahead_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( ahead_count*2 + 2 );
    p += ahead_count*2;

    pos_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( pos_count*4 );
  }


  static void
  otl_chain_pos_rule_set_validate( OTL_Bytes      table,
                                   OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_chain_pos_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }



  static void
  otl_chain_pos_class_rule_validate( OTL_Bytes      table,
                                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   back_count, input_count, ahead_count, pos_count;

    OTL_CHECK( 2 );
    back_count = OTL_NEXT_USHORT( p );

    OTL_CHECK( back_count*2 + 2 );
    p += back_count*2;

    input_count = OTL_NEXT_USHORT( p );
    if ( input_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( input_count*2 );
    p += (input_count-1)*2;

    ahead_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( ahead_count*2 + 2 );
    p += ahead_count*2;

    pos_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( pos_count*4 );
  }


  static void
  otl_chain_pos_class_set_validate( OTL_Bytes      table,
                                   OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_chain_pos_class_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gpos_lookup8_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( count*2 );
          for ( ; count > 0; count-- )
            otl_chain_pos_rule_set_validate( table + OTL_NEXT_USHORT( p ),
                                             valid );
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, back_class, input_class, ahead_class, count;

          OTL_CHECK( 10 );
          coverage    = OTL_NEXT_USHORT( p );
          back_class  = OTL_NEXT_USHORT( p );
          input_class = OTL_NEXT_USHORT( p );
          ahead_class = OTL_NEXT_USHORT( p );
          count       = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          otl_class_definition_validate( table + back_class,  valid );
          otl_class_definition_validate( table + input_class, valid );
          otl_class_definition_validate( table + ahead_class, valid );

          OTL_CHECK( count*2 );
          for ( ; count > 0; count-- )
            otl_chain_pos_class_set_validate( table + OTL_NEXT_USHORT( p ),
                                              valid );
        }
        break;

      case 3:
        {
          OTL_UInt  back_count, input_count, ahead_count, pos_count, count;

          OTL_CHECK( 2 );
          back_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*back_count+2 );
          for ( count = back_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          input_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*input_count+2 );
          for ( count = input_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          ahead_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*ahead_count+2 );
          for ( count = ahead_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          pos_count = OTL_NEXT_USHORT( p );
          OTL_CHECK( pos_count*4 );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GPOS LOOKUP TYPE 9                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_gpos_lookup9_validate( OTL_Bytes  table,
                             OTL_Valid  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch (format)
    {
      case 1:
        {
          OTL_UInt          lookup_type, lookup_offset;
          OTL_ValidateFunc  validate;

          OTL_CHECK( 6 );
          lookup_type   = OTL_NEXT_USHORT( p );
          lookup_offset = OTL_NEXT_ULONG( p );

          if ( lookup_type == 0 || lookup_type >= 9 )
            OTL_INVALID_DATA;

          validate = otl_gpos_validate_funcs[ lookup_type-1 ];
          validate( table + lookup_offset, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

  static OTL_ValidateFunc  otl_gpos_validate_funcs[ 9 ] =
  {
    otl_gpos_lookup1_validate,
    otl_gpos_lookup2_validate,
    otl_gpos_lookup3_validate,
    otl_gpos_lookup4_validate,
    otl_gpos_lookup5_validate,
    otl_gpos_lookup6_validate,
    otl_gpos_lookup7_validate,
    otl_gpos_lookup8_validate,
    otl_gpos_lookup9_validate,
  };


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                         GPOS TABLE                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  OTL_LOCALDEF( void )
  otl_gpos_validate( OTL_Bytes      table,
                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   scripts, features, lookups;

    OTL_CHECK( 10 );

    if ( OTL_NEXT_USHORT( p ) != 0x10000UL )
      OTL_INVALID_DATA;

    scripts  = OTL_NEXT_USHORT( p );
    features = OTL_NEXT_USHORT( p );
    lookups  = OTL_NEXT_USHORT( p );

    otl_script_list_validate ( table + scripts, valid );
    otl_feature_list_validate( table + features, valid );

    otl_lookup_list_validate( table + lookups, 9, otl_gpos_validate_funcs,
                              valid );
  }
  