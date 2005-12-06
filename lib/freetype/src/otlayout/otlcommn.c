/***************************************************************************/
/*                                                                         */
/*  otlcommn.c                                                             */
/*                                                                         */
/*    OpenType layout support, common tables (body).                       */
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


#include "otlayout.h"


 /*************************************************************************/
 /*************************************************************************/
 /*****                                                               *****/
 /*****                       COVERAGE TABLE                          *****/
 /*****                                                               *****/
 /*************************************************************************/
 /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_coverage_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_Bytes  p;
    OTL_UInt   format;


    if ( table + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
    case 1:
      {
        OTL_UInt  count = OTL_NEXT_USHORT( p );


        if ( p + count * 2 >= valid->limit )
          OTL_INVALID_TOO_SHORT;

        /* XXX: check glyph indices */
      }
      break;

    case 2:
      {
        OTL_UInt  n, num_ranges = OTL_NEXT_USHORT( p );
        OTL_UInt  start, end, start_cover, total = 0, last = 0;


        if ( p + num_ranges * 6 >= valid->limit )
          OTL_INVALID_TOO_SHORT;

        for ( n = 0; n < num_ranges; n++ )
        {
          start       = OTL_NEXT_USHORT( p );
          end         = OTL_NEXT_USHORT( p );
          start_cover = OTL_NEXT_USHORT( p );

          if ( start > end || start_cover != total )
            OTL_INVALID_DATA;

          if ( n > 0 && start <= last )
            OTL_INVALID_DATA;

          total += end - start + 1;
          last   = end;
        }
      }
      break;

    default:
      OTL_INVALID_FORMAT;
    }
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_coverage_get_count( OTL_Bytes  table )
  {
    OTL_Bytes  p      = table;
    OTL_UInt   format = OTL_NEXT_USHORT( p );
    OTL_UInt   count  = OTL_NEXT_USHORT( p );
    OTL_UInt   result = 0;


    switch ( format )
    {
    case 1:
      return count;

    case 2:
      {
        OTL_UInt  start, end;


        for ( ; count > 0; count-- )
        {
          start = OTL_NEXT_USHORT( p );
          end   = OTL_NEXT_USHORT( p );
          p    += 2;                    /* skip start_index */

          result += end - start + 1;
        }
      }
      break;

    default:
      ;
    }

    return result;
  }


  OTL_LOCALDEF( OTL_Int )
  otl_coverage_get_index( OTL_Bytes  table,
                          OTL_UInt   glyph_index )
  {
    OTL_Bytes  p      = table;
    OTL_UInt   format = OTL_NEXT_USHORT( p );
    OTL_UInt   count  = OTL_NEXT_USHORT( p );


    switch ( format )
    {
    case 1:
      {
        OTL_UInt  min = 0, max = count, mid, gindex;


        table += 4;
        while ( min < max )
        {
          mid    = ( min + max ) >> 1;
          p      = table + 2 * mid;
          gindex = OTL_PEEK_USHORT( p );

          if ( glyph_index == gindex )
            return (OTL_Int)mid;

          if ( glyph_index < gindex )
            max = mid;
          else
            min = mid + 1;
        }
      }
      break;

    case 2:
      {
        OTL_UInt  min = 0, max = count, mid;
        OTL_UInt  start, end, delta, start_cover;


        table += 4;
        while ( min < max )
        {
          mid    = ( min + max ) >> 1;
          p      = table + 6 * mid;
          start  = OTL_NEXT_USHORT( p );
          end    = OTL_NEXT_USHORT( p );

          if ( glyph_index < start )
            max = mid;
          else if ( glyph_index > end )
            min = mid + 1;
          else
            return (OTL_Int)( glyph_index + OTL_NEXT_USHORT( p ) - start );
        }
      }
      break;

    default:
      ;
    }

    return -1;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  CLASS DEFINITION TABLE                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_class_definition_validate( OTL_Bytes      table,
                                 OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;


    if ( p + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
    case 1:
      {
        OTL_UInt  count, start = OTL_NEXT_USHORT( p );


        if ( p + 2 > valid->limit )
          OTL_INVALID_TOO_SHORT;

        count = OTL_NEXT_USHORT( p );

        if ( p + count * 2 > valid->limit )
          OTL_INVALID_TOO_SHORT;

        /* XXX: check glyph indices */
      }
      break;

    case 2:
      {
        OTL_UInt  n, num_ranges = OTL_NEXT_USHORT( p );
        OTL_UInt  start, end, value, last = 0;


        if ( p + num_ranges * 6 > valid->limit )
          OTL_INVALID_TOO_SHORT;

        for ( n = 0; n < num_ranges; n++ )
        {
          start = OTL_NEXT_USHORT( p );
          end   = OTL_NEXT_USHORT( p );
          value = OTL_NEXT_USHORT( p );  /* ignored */

          if ( start > end || ( n > 0 && start <= last ) )
            OTL_INVALID_DATA;

          last = end;
        }
      }
      break;

    default:
      OTL_INVALID_FORMAT;
    }
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_class_definition_get_value( OTL_Bytes  table,
                                  OTL_UInt   glyph_index )
  {
    OTL_Bytes  p      = table;
    OTL_UInt   format = OTL_NEXT_USHORT( p );


    switch ( format )
    {
    case 1:
      {
        OTL_UInt  start = OTL_NEXT_USHORT( p );
        OTL_UInt  count = OTL_NEXT_USHORT( p );
        OTL_UInt  idx   = (OTL_UInt)( glyph_index - start );


        if ( idx < count )
        {
          p += 2 * idx;
          return OTL_PEEK_USHORT( p );
        }
      }
      break;

    case 2:
      {
        OTL_UInt  count = OTL_NEXT_USHORT( p );
        OTL_UInt  min = 0, max = count, mid, gindex;


        table += 4;
        while ( min < max )
        {
          mid   = ( min + max ) >> 1;
          p     = table + 6 * mid;
          start = OTL_NEXT_USHORT( p );
          end   = OTL_NEXT_USHORT( p );

          if ( glyph_index < start )
            max = mid;
          else if ( glyph_index > end )
            min = mid + 1;
          else
            return OTL_PEEK_USHORT( p );
        }
      }
      break;

    default:
      ;
    }

    return 0;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      DEVICE TABLE                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_device_table_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   start, end, count, format, count;


    if ( p + 8 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    start  = OTL_NEXT_USHORT( p );
    end    = OTL_NEXT_USHORT( p );
    format = OTL_NEXT_USHORT( p );

    if ( format < 1 || format > 3 || end < start )
      OTL_INVALID_DATA;

    count = (OTL_UInt)( end - start + 1 );

    if ( p + ( ( 1 << format ) * count ) / 8 > valid->limit )
      OTL_INVALID_TOO_SHORT;
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_device_table_get_start( OTL_Bytes  table )
  {
    OTL_Bytes  p = table;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_device_table_get_end( OTL_Bytes  table )
  {
    OTL_Bytes  p = table + 2;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_Int )
  otl_device_table_get_delta( OTL_Bytes  table,
                              OTL_UInt   size )
  {
    OTL_Bytes  p = table;
    OTL_Int    result = 0;
    OTL_UInt   start, end, format, idx, value;


    start  = OTL_NEXT_USHORT( p );
    end    = OTL_NEXT_USHORT( p );
    format = OTL_NEXT_USHORT( p );

    if ( size >= start && size <= end )
    {
      /* we could do that with clever bit operations, but a switch is */
      /* much simpler to understand and maintain                      */
      /*                                                              */
      switch ( format )
      {
      case 1:
        idx    = (OTL_UInt)( ( size - start ) * 2 );
        p     += idx / 16;
        value  = OTL_PEEK_USHORT( p );
        shift  = idx & 15;
        result = (OTL_Short)( value << shift ) >> ( 14 - shift );

        break;

      case 2:
        idx    = (OTL_UInt)( ( size - start ) * 4 );
        p     += idx / 16;
        value  = OTL_PEEK_USHORT( p );
        shift  = idx & 15;
        result = (OTL_Short)( value << shift ) >> ( 12 - shift );

        break;

      case 3:
        idx    = (OTL_UInt)( ( size - start ) * 8 );
        p     += idx / 16;
        value  = OTL_PEEK_USHORT( p );
        shift  = idx & 15;
        result = (OTL_Short)( value << shift ) >> ( 8 - shift );

        break;

      default:
        ;
      }
    }

    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      LOOKUP LISTS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_lookup_validate( OTL_Bytes      table,
                       OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   num_tables;


    if ( table + 6 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    p += 4;
    num_tables = OTL_NEXT_USHORT( p );

    if ( p + num_tables * 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; num_tables > 0; num_tables-- )
    {
      offset = OTL_NEXT_USHORT( p );

      if ( table + offset >= valid->limit )
        OTL_INVALID_OFFSET;
    }

    /* XXX: check sub-tables? */
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_lookup_get_count( OTL_Bytes  table )
  {
    OTL_Bytes  p = table + 4;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_Bytes )
  otl_lookup_get_table( OTL_Bytes  table,
                        OTL_UInt   idx )
  {
    OTL_Bytes  p, result = NULL;
    OTL_UInt   count;


    p     = table + 4;
    count = OTL_NEXT_USHORT( p );
    if ( idx < count )
    {
      p     += idx * 2;
      result = table + OTL_PEEK_USHORT( p );
    }

    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      LOOKUP LISTS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_lookup_list_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table, q;
    OTL_UInt   num_lookups, offset;


    if ( p + 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    num_lookups = OTL_NEXT_USHORT( p );

    if ( p + num_lookups * 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; num_lookups > 0; num_lookups-- )
    {
      offset = OTL_NEXT_USHORT( p );

      otl_lookup_validate( table + offset, valid );
    }
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_lookup_list_get_count( OTL_Bytes  table )
  {
    OTL_Bytes  p = table;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_Bytes )
  otl_lookup_list_get_lookup( OTL_Bytes  table,
                              OTL_UInt   idx )
  {
    OTL_Bytes  p, result = 0;
    OTL_UInt   count;


    p     = table;
    count = OTL_NEXT_USHORT( p );
    if ( idx < count )
    {
      p     += idx * 2;
      result = table + OTL_PEEK_USHORT( p );
    }

    return result;
  }


  OTL_LOCALDEF( OTL_Bytes )
  otl_lookup_list_get_table( OTL_Bytes  table,
                             OTL_UInt   lookup_index,
                             OTL_UInt   table_index )
  {
    OTL_Bytes  result = NULL;


    result = otl_lookup_list_get_lookup( table, lookup_index );
    if ( result )
      result = otl_lookup_get_table( result, table_index );

    return result;
  }


  OTL_LOCALDEF( void )
  otl_lookup_list_foreach( OTL_Bytes        table,
                           OTL_ForeachFunc  func,
                           OTL_Pointer      func_data )
  {
    OTL_Bytes  p     = table;
    OTL_UInt   count = OTL_NEXT_USHORT( p );


    for ( ; count > 0; count-- )
      func( table + OTL_NEXT_USHORT( p ), func_data );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        FEATURES                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_feature_validate( OTL_Bytes      table,
                        OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   feat_params, num_lookups;


    if ( p + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    feat_params = OTL_NEXT_USHORT( p );  /* ignored */
    num_lookups = OTL_NEXT_USHORT( p );

    if ( p + num_lookups * 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    /* XXX: check lookup indices */
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_feature_get_count( OTL_Bytes  table )
  {
    OTL_Bytes  p = table + 4;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_feature_get_lookups( OTL_Bytes  table,
                           OTL_UInt   start,
                           OTL_UInt   count,
                           OTL_UInt  *lookups )
  {
    OTL_Bytes  p;
    OTL_UInt   num_features, result = 0;


    p            = table + 4;
    num_features = OTL_NEXT_USHORT( p );

    p += start * 2;

    for ( ; count > 0 && start < num_features; count--, start++ )
    {
      lookups[0] = OTL_NEXT_USHORT(p);
      lookups++;
      result++;
    }

    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        FEATURE LIST                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCALDEF( void )
  otl_feature_list_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   num_features, offset;


    if ( table + 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    num_features = OTL_NEXT_USHORT( p );

    if ( p + num_features * 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; num_features > 0; num_features-- )
    {
      p     += 4;                       /* skip tag */
      offset = OTL_NEXT_USHORT( p );

      otl_feature_table_validate( table + offset, valid );
    }
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_feature_list_get_count( OTL_Bytes  table )
  {
    OTL_Bytes  p = table;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_Bytes )
  otl_feature_list_get_feature( OTL_Bytes  table,
                                OTL_UInt   idx )
  {
    OTL_Bytes  p, result = NULL;
    OTL_UInt   count;


    p     = table;
    count = OTL_NEXT_USHORT( p );

    if ( idx < count )
    {
      p     += idx * 2;
      result = table + OTL_PEEK_USHORT( p );
    }

    return result;
  }


  OTL_LOCALDEF( void )
  otl_feature_list_foreach( OTL_Bytes        table,
                            OTL_ForeachFunc  func,
                            OTL_Pointer      func_data )
  {
    OTL_Bytes  p;
    OTL_UInt   count;


    p = table;
    count = OTL_NEXT_USHORT( p );

    for ( ; count > 0; count-- )
      func( table + OTL_NEXT_USHORT( p ), func_data );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       LANGUAGE SYSTEM                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  OTL_LOCALDEF( void )
  otl_lang_validate( OTL_Bytes      table,
                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   lookup_order;
    OTL_UInt   req_feature;
    OTL_UInt   num_features;


    if ( table + 6 >= valid->limit )
      OTL_INVALID_TOO_SHORT;

    lookup_order = OTL_NEXT_USHORT( p );
    req_feature  = OTL_NEXT_USHORT( p );
    num_features = OTL_NEXT_USHORT( p );

    /* XXX: check req_feature if not 0xFFFFU */

    if ( p + 2 * num_features >= valid->limit )
      OTL_INVALID_TOO_SHORT;

    /* XXX: check features indices! */
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_lang_get_count( OTL_Bytes  table )
  {
    OTL_Bytes  p = table + 4;

    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_lang_get_req_feature( OTL_Bytes  table )
  {
    OTL_Bytes  p = table + 2;


    return OTL_PEEK_USHORT( p );
  }


  OTL_LOCALDEF( OTL_UInt )
  otl_lang_get_features( OTL_Bytes  table,
                         OTL_UInt   start,
                         OTL_UInt   count,
                         OTL_UInt  *features )
  {
    OTL_Bytes  p            = table + 4;
    OTL_UInt   num_features = OTL_NEXT_USHORT( p );
    OTL_UInt   result       = 0;


    p += start * 2;

    for ( ; count > 0 && start < num_features; start++, count-- )
    {
      features[0] = OTL_NEXT_USHORT( p );
      features++;
      result++;
    }

    return result;
  }




  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           SCRIPTS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  OTL_LOCALDEF( void )
  otl_script_validate( OTL_Bytes      table,
                       OTL_Validator  valid )
  {
    OTL_UInt   default_lang;
    OTL_Bytes  p = table;


    if ( table + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    default_lang = OTL_NEXT_USHORT( p );
    num_langs    = OTL_NEXT_USHORT( p );

    if ( default_lang != 0 )
    {
      if ( table + default_lang >= valid->limit )
        OTL_INVALID_OFFSET;
    }

    if ( p + num_langs * 6 >= valid->limit )
      OTL_INVALID_OFFSET;

    for ( ; num_langs > 0; num_langs-- )
    {
      OTL_UInt  offset;


      p     += 4;  /* skip tag */
      offset = OTL_NEXT_USHORT( p );

      otl_lang_validate( table + offset, valid );
    }
  }


  OTL_LOCALDEF( void )
  otl_script_list_validate( OTL_Bytes      list,
                            OTL_Validator  valid )
  {
    OTL_UInt   num_scripts;
    OTL_Bytes  p = list;


    if ( list + 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    num_scripts = OTL_NEXT_USHORT( p );

    if ( p + num_scripts * 6 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; num_scripts > 0; num_scripts-- )
    {
      OTL_UInt  offset;


      p     += 4;                       /* skip tag */
      offset = OTL_NEXT_USHORT( p );

      otl_script_table_validate( list + offset, valid );
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                         LOOKUP LISTS                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  otl_lookup_table_validate( OTL_Bytes          table,
                             OTL_UInt           type_count,
                             OTL_ValidateFunc*  type_funcs,
                             OTL_Validator      valid )
  {
    OTL_Bytes         p = table;
    OTL_UInt          lookup_type, lookup_flag, count;
    OTL_ValidateFunc  validate;

    OTL_CHECK( 6 );
    lookup_type = OTL_NEXT_USHORT( p );
    lookup_flag = OTL_NEXT_USHORT( p );
    count       = OTL_NEXT_USHORT( p );

    if ( lookup_type == 0 || lookup_type >= type_count )
      OTL_INVALID_DATA;

    validate = type_funcs[ lookup_type - 1 ];

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  OTL_LOCALDEF( void )
  otl_lookup_list_validate( OTL_Bytes          table,
                            OTL_UInt           type_count,
                            OTL_ValidateFunc*  type_funcs,
                            OTL_Validator      valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_lookup_table_validate( table + OTL_NEXT_USHORT( p ),
                                 type_count, type_funcs, valid );
  }

/* END */
