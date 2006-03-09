#include "otlgdef.h"
#include "otlcommn.h"

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                      ATTACHMENTS LIST                        *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_attach_point_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    if ( p + 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    count = OTL_NEXT_USHORT( p );
    if ( table + count*2 > valid->limit )
      OTL_INVALID_TOO_SHORT;
  }


  static void
  otl_attach_list_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_Bytes  coverage;
    OTL_UInt   count;

    if ( p + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    coverage = table + OTL_NEXT_USHORT( p );
    count    = OTL_NEXT_USHORT( p );

    otl_coverage_validate( coverage, valid );
    if ( count != otl_coverage_get_count( coverage ) )
      OTL_INVALID_DATA;

    if ( p + count*2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; count > 0; count-- )
      otl_attach_point_validate( table + OTL_NEXT_USHORT( p ) );
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                      LIGATURE CARETS                         *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_caret_value_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;

    if ( p + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
      case 2:
        break;

      case 3:
        {
          OTL_Bytes  device;

          p += 2;
          if ( p + 2 > valid->limit )
            OTL_INVALID_TOO_SHORT;

          otl_device_table_validate( table + OTL_PEEK_USHORT( p ) );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static void
  otl_ligature_glyph_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    if ( p + 2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    count = OTL_NEXT_USHORT( p );

    if ( p + count*2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; count > 0; count-- )
      otl_caret_value_validate( table + OTL_NEXT_USHORT( p ) );
  }


  static void
  otl_ligature_caret_list_validate( OTL_Bytes      table,
                                    OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_Bytes  coverage;
    OTL_UInt   count;

    if ( p + 4 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    coverage = table + OTL_NEXT_USHORT( p );
    count    = OTL_NEXT_USHORT( p );

    otl_coverage_validate( coverage, valid );
    if ( count != otl_coverage_get_count( coverage ) )
      OTL_INVALID_DATA;

    if ( p + count*2 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    for ( ; count > 0; count-- )
      otl_ligature_glyph_validate( table + OTL_NEXT_USHORT( p ) );
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                         GDEF TABLE                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  OTL_APIDEF( void )
  otl_gdef_validate( OTL_Bytes      table,
                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;

    if ( p + 12 > valid->limit )
      OTL_INVALID_TOO_SHORT;

    /* check format */
    if ( OTL_NEXT_ULONG( p ) != 0x00010000UL )
      OTL_INVALID_FORMAT;

    /* validate class definition table */
    otl_class_definition_validate( table + OTL_NEXT_USHORT( p ) );

    /* validate attachment point list */
    otl_attach_list_validate( table + OTL_NEXT_USHORT( p ) );

    /* validate ligature caret list */
    otl_ligature_caret_list_validate( table + OTL_NEXT_USHORT( p ) );

    /* validate mark attach class */
    otl_class_definition_validate( table + OTL_NEXT_USHORT( p ) );
  }

