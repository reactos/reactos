#include "otlbase.h"
#include "otlcommn.h"

  static void
  otl_base_coord_validate( OTL_Bytes      table,
                           OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 4 );

    format = OTL_NEXT_USHORT( p );
    p += 2;

    switch ( format )
    {
      case 1:
        break;

      case 2:
        OTL_CHECK( 4 );
        break;

      case 3:
        OTL_CHECK( 2 );
        otl_device_table_validate( table + OTL_PEEK_USHORT( p ) );
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static void
  otl_base_tag_list_validate( OTL_Bytes      table,
                              OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK(2);

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( count*4 );
  }



  static void
  otl_base_values_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 4 );

    p    += 2;  /* skip default index */
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );

    for ( ; count > 0; count-- )
      otl_base_coord_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_base_minmax_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   min_coord, max_coord, count;

    OTL_CHECK(6);
    min_coord = OTL_NEXT_USHORT( p );
    max_coord = OTL_NEXT_USHORT( p );
    count     = OTL_NEXT_USHORT( p );

    if ( min_coord )
      otl_base_coord_validate( table + min_coord, valid );

    if ( max_coord )
      otl_base_coord_validate( table + max_coord, valid );

    OTL_CHECK( count*8 );
    for ( ; count > 0; count-- )
    {
      p += 4;  /* ignore tag */
      min_coord = OTL_NEXT_USHORT( p );
      max_coord = OTL_NEXT_USHORT( p );

      if ( min_coord )
        otl_base_coord_validate( table + min_coord, valid );

      if ( max_coord )
        otl_base_coord_validate( table + max_coord, valid );
    }
  }


  static void
  otl_base_script_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   values, default_minmax;

    OTL_CHECK(6);

    values         = OTL_NEXT_USHORT( p );
    default_minmax = OTL_NEXT_USHORT( p );
    count          = OTL_NEXT_USHORT( p );

    if ( values )
      otl_base_values_validate( table + values, valid );

    if ( default_minmax )
      otl_base_minmax_validate( table + default_minmax, valid );

    OTL_CHECK( count*6 );
    for ( ; count > 0; count-- )
    {
      p += 4;  /* ignore tag */
      otl_base_minmax_validate( table + OTL_NEXT_USHORT( p ), valid );
    }
  }


  static void
  otl_base_script_list_validate( OTL_Bytes      table,
                                 OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK(2);

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK(count*6);

    for ( ; count > 0; count-- )
    {
      p += 4;  /* ignore script tag */
      otl_base_script_validate( table + OTL_NEXT_USHORT( p ) );
    }
  }

  static void
  otl_axis_table_validate( OTL_Bytes      table,
                           OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   tags;

    OTL_CHECK(4);

    tags = OTL_NEXT_USHORT( p );
    if ( tags )
      otl_base_tag_list_validate   ( table + tags );

    otl_base_script_list_validate( table + OTL_NEXT_USHORT( p ) );
  }


  OTL_LOCALDEF( void )
  otl_base_validate( OTL_Bytes      table,
                     OTL_Validator  valid )
  {
    OTL_Bytes p = table;

    OTL_CHECK(6);

    if ( OTL_NEXT_ULONG( p ) != 0x10000UL )
      OTL_INVALID_DATA;

    otl_axis_table_validate( table + OTL_NEXT_USHORT( p ) );
    otl_axis_table_validate( table + OTL_NEXT_USHORT( p ) );
  }