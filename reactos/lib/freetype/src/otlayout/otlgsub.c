#include "otlgsub.h"
#include "otlcommn.h"

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 1                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

 /*
  *  1: Single Substitution - Table format(s)
  *
  *  This table is used to substiture individual glyph indices
  *  with another one. There are only two sub-formats:
  *
  *   Name         Offset    Size       Description
  *   ------------------------------------------
  *   format       0         2          sub-table format (1)
  *   offset       2         2          offset to coverage table
  *   delta        4         2          16-bit delta to apply on all
  *                                     covered glyph indices
  *
  *   Name         Offset    Size       Description
  *   ------------------------------------------
  *   format       0         2          sub-table format (2)
  *   offset       2         2          offset to coverage table
  *   count        4         2          coverage table count
  *   substs[]     6         2*count    substituted glyph indices,
  *
  */

  static void
  otl_gsub_lookup1_validate( OTL_Bytes      table,
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
          OTL_UInt  coverage;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );

          /* NB: we don't check that there are at most 'count'   */
          /*     elements in the coverage table. This is delayed */
          /*     to the lookup function...                       */
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static OTL_Bool
  otl_gsub_lookup1_apply( OTL_Bytes   table,
                          OTL_Parser  parser )
  {
    OTL_Bytes  p = table;
    OTL_Bytes  coverage;
    OTL_UInt   format, gindex, property;
    OTL_Int    index;
    OTL_Bool   subst = 0;

    if ( parser->context_len != 0xFFFFU && parser->context_len < 1 )
      goto Exit;

    gindex = otl_parser_get_gindex( parser );

    if ( !otl_parser_check_property( parser, gindex, &property ) )
      goto Exit;

    format   = OTL_NEXT_USHORT(p);
    coverage = table + OTL_NEXT_USHORT(p);
    index    = otl_coverage_lookup( coverage, gindex );

    if ( index >= 0 )
    {
      switch ( format )
      {
        case 1:
          {
            OTL_Int  delta = OTL_NEXT_SHORT(p);

            gindex = ( gindex + delta ) & 0xFFFFU;
            otl_parser_replace_1( parser, gindex );
            subst = 1;
          }
          break;

        case 2:
          {
            OTL_UInt  count = OTL_NEXT_USHORT(p);

            if ( (OTL_UInt) index < count )
            {
              p += index*2;
              otl_parser_replace_1( parser, OTL_PEEK_USHORT(p) );
              subst = 1;
            }
          }
          break;

        default:
          ;
      }
    }
  Exit:
    return subst;
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 2                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

 /*
  *  2: Multiple Substitution - Table format(s)
  *
  *  Replaces a single glyph with one or more glyphs.
  *
  *   Name         Offset    Size       Description
  *   -----------------------------------------------------------
  *   format       0         2          sub-table format (1)
  *   offset       2         2          offset to coverage table
  *   count        4         2          coverage table count
  *   sequencess[] 6         2*count    offsets to sequence items
  *
  *   each sequence item has the following format:
  *
  *   Name         Offset    Size       Description
  *   -----------------------------------------------------------
  *   count        0         2          number of replacement glyphs
  *   gindices[]   2         2*count    string of glyph indices
  */

  static void
  otl_seq_validate( OTL_Bytes      table,
                    OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    /* XXX: according to the spec, 'count' should be > 0     */
    /*      we can deal with these cases pretty well however */

    OTL_CHECK( 2*count );
    /* check glyph indices */
  }


  static void
  otl_gsub_lookup2_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, seq_count;

          OTL_CHECK( 4 );
          coverage  = OTL_NEXT_USHORT( p );
          seq_count = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( seq_count*2 );
          for ( ; seq_count > 0; seq_count-- )
            otl_seq_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static OTL_Bool
  otl_gsub_lookup2_apply( OTL_Bytes    table,
                          OTL_Parser   parser )
  {
    OTL_Bytes  p = table;
    OTL_Bytes  coverage, sequence;
    OTL_UInt   format, gindex, index, property;
    OTL_Int    index;
    OTL_Bool   subst = 0;

    if ( context_len != 0xFFFFU && context_len < 1 )
      goto Exit;

    gindex = otl_parser_get_gindex( parser );

    if ( !otl_parser_check_property( parser, gindex, &property ) )
      goto Exit;

    p        += 2;  /* skip format */
    coverage  = table + OTL_NEXT_USHORT(p);
    seq_count = OTL_NEXT_USHORT(p);
    index     = otl_coverage_lookup( coverage, gindex );

    if ( (OTL_UInt) index >= seq_count )
      goto Exit;

    p       += index*2;
    sequence = table + OTL_PEEK_USHORT(p);
    p        = sequence;
    count    = OTL_NEXT_USHORT(p);

    otl_parser_replace_n( parser, count, p );
    subst = 1;

   Exit:
    return subst;
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 3                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

 /*
  *  3: Alternate Substitution - Table format(s)
  *
  *  Replaces a single glyph by another one taken liberally
  *  in a list of alternatives
  *
  *   Name         Offset    Size       Description
  *   -----------------------------------------------------------
  *   format       0         2          sub-table format (1)
  *   offset       2         2          offset to coverage table
  *   count        4         2          coverage table count
  *   alternates[] 6         2*count    offsets to alternate items
  *
  *   each alternate item has the following format:
  *
  *   Name         Offset    Size       Description
  *   -----------------------------------------------------------
  *   count        0         2          number of replacement glyphs
  *   gindices[]   2         2*count    string of glyph indices, each one
  *                                     is a valid alternative
  */

  static void
  otl_alternate_set_validate( OTL_Bytes      table,
                              OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    /* XXX: check glyph indices */
  }


  static void
  otl_gsub_lookup3_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_alternate_set_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static OTL_Bool
  otl_gsub_lookup3_apply( OTL_Bytes    table,
                          OTL_Parser   parser )
  {
    OTL_Bytes  p = table;
    OTL_Bytes  coverage, alternates;
    OTL_UInt   format, gindex, index, property;
    OTL_Int    index;
    OTL_Bool   subst = 0;

    OTL_GSUB_Alternate  alternate = parser->alternate;

    if ( context_len != 0xFFFFU && context_len < 1 )
      goto Exit;

    if ( alternate == NULL )
      goto Exit;

    gindex = otl_parser_get_gindex( parser );

    if ( !otl_parser_check_property( parser, gindex, &property ) )
      goto Exit;

    p        += 2;  /* skip format */
    coverage  = table + OTL_NEXT_USHORT(p);
    seq_count = OTL_NEXT_USHORT(p);
    index     = otl_coverage_lookup( coverage, gindex );

    if ( (OTL_UInt) index >= seq_count )
      goto Exit;

    p         += index*2;
    alternates = table + OTL_PEEK_USHORT(p);
    p          = alternates;
    count      = OTL_NEXT_USHORT(p);

    gindex = alternate->handler_func(
                 gindex, count, p, alternate->handler_data );

    otl_parser_replace_1( parser, gindex );
    subst = 1;

   Exit:
    return subst;
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 4                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_ligature_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_UInt  glyph_id, count;

    OTL_CHECK( 4 );
    glyph_id = OTL_NEXT_USHORT( p );
    count    = OTL_NEXT_USHORT( p );

    if ( count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( 2*(count-1) );
    /* XXX: check glyph indices */
  }


  static void
  otl_ligature_set_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_ligature_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gsub_lookup4_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_ligature_set_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 5                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  static void
  otl_sub_rule_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   glyph_count, subst_count;

    OTL_CHECK( 4 );
    glyph_count = OTL_NEXT_USHORT( p );
    subst_count = OTL_NEXT_USHORT( p );

    if ( glyph_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( (glyph_count-1)*2 + substcount*4 );

    /* XXX: check glyph indices and subst lookups */
  }


  static void
  otl_sub_rule_set_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_sub_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_sub_class_rule_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_UInt  glyph_count, subst_count;

    OTL_CHECK( 4 );
    glyph_count = OTL_NEXT_USHORT( p );
    subst_count = OTL_NEXT_USHORT( p );

    if ( glyph_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( (glyph_count-1)*2 + substcount*4 );

    /* XXX: check glyph indices and subst lookups */
  }


  static void
  otl_sub_class_rule_set_validate( OTL_Bytes      table,
                                   OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_sub_class_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gsub_lookup5_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_sub_rule_set_validate( table + coverage, valid );
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

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_sub_class_rule_set_validate( table + coveragen valid );
        }
        break;

      case 3:
        {
          OTL_UInt  glyph_count, subst_count, count;

          OTL_CHECK( 4 );
          glyph_count = OTL_NEXT_USHORT( p );
          subst_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*glyph_count + 4*subst_count );
          for ( count = glyph_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 6                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  static void
  otl_chain_sub_rule_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   back_count, input_count, ahead_count, subst_count, count;

    OTL_CHECK( 2 );
    back_count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*back_count+2 );
    p += 2*back_count;

    input_count = OTL_NEXT_USHORT( p );
    if ( input_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( 2*input_count );
    p += 2*(input_count-1);

    ahead_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 2*ahead_count + 2 );
    p += 2*ahead_count;

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 4*count );

    /* XXX: check glyph indices and subst lookups */
  }


  static void
  otl_chain_sub_rule_set_validate( OTL_Bytes      table,
                                   OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_chain_sub_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_chain_sub_class_rule_validate( OTL_Bytes      table,
                                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   back_count, input_count, ahead_count, subst_count, count;

    OTL_CHECK( 2 );
    back_count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*back_count+2 );
    p += 2*back_count;

    input_count = OTL_NEXT_USHORT( p );
    if ( input_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( 2*input_count );
    p += 2*(input_count-1);

    ahead_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 2*ahead_count + 2 );
    p += 2*ahead_count;

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 4*count );

    /* XXX: check class indices and subst lookups */
  }



  static void
  otl_chain_sub_class_set_validate( OTL_Bytes      table,
                                    OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_chain_sub_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gsub_lookup6_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_chain_sub_rule_set_validate( table + coverage, valid );
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

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_chain_sub_class_set( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      case 3:
        {
          OTL_UInt  back_count, input_count, ahead_count, subst_count, count;

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

          subst_count = OTL_NEXT_USHORT( p );
          OTL_CHECK( subst_count*4 );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 6                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_gsub_lookup7_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt          lookup_type, lookup_offset;
          OTL_ValidateFunc  validate;

          OTL_CHECK( 6 );
          lookup_type   = OTL_NEXT_USHORT( p );
          lookup_offset = OTL_NEXT_ULONG( p );

          if ( lookup_type == 0 || lookup_type >= 7 )
            OTL_INVALID_DATA;

          validate = otl_gsub_validate_funcs[ lookup_type-1 ];
          validate( table + lookup_offset, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static const OTL_ValidateFunc  otl_gsub_validate_funcs[ 7 ] =
  {
    otl_gsub_lookup1_validate,
    otl_gsub_lookup2_validate,
    otl_gsub_lookup3_validate,
    otl_gsub_lookup4_validate,
    otl_gsub_lookup5_validate,
    otl_gsub_lookup6_validate
  };

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                         GSUB TABLE                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  OTL_LOCALDEF( void )
  otl_gsub_validate( OTL_Bytes      table,
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

    otl_lookup_list_validate( table + lookups, 7, otl_gsub_validate_funcs,
                              valid );
  }
