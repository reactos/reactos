#include "otljstf.h"
#include "otlcommn.h"
#include "otlgpos.h"

  static void
  otl_jstf_extender_validate( OTL_Bytes      table,
                              OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );

    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );
  }


  static void
  otl_jstf_gsub_mods_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( count*2 );

    /* XXX: check GSUB lookup indices */
  }


  static void
  otl_jstf_gpos_mods_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( count*2 );

    /* XXX: check GPOS lookup indices */
  }


  static void
  otl_jstf_max_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );

    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );
    for ( ; count > 0; count-- )
      otl_gpos_subtable_check( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_jstf_priority_validate( OTL_Bytes      table,
                              OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   offset;

    OTL_CHECK( 20 );

    /* shrinkage GSUB enable/disable */
    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gsub_mods_validate( table + val, valid );

    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gsub_mods_validate( table + val, valid );

    /* shrinkage GPOS enable/disable */
    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gpos_mods_validate( table + val, valid );

    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gpos_mods_validate( table + val, valid );

    /* shrinkage JSTF max */
    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_max_validate( table + val, valid );

    /* extension GSUB enable/disable */
    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gsub_mods_validate( table + val, valid );

    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gsub_mods_validate( table + val, valid );

    /* extension GPOS enable/disable */
    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gpos_mods_validate( table + val, valid );

    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_gpos_mods_validate( table + val, valid );

    /* extension JSTF max */
    val = OTL_NEXT_USHORT( p );
    if ( val )
      otl_jstf_max_validate( table + val, valid );
  }

  static void
  otl_jstf_lang_validate( OTL_Bytes      table,
                          OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );

    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( count*2 );
    for ( ; count > 0; count-- )
      otl_jstf_priority_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_jstf_script_validate( OTL_Bytes      table,
                            OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count, extender, default_lang;

    OTL_CHECK( 6 );
    extender     = OTL_NEXT_USHORT( p );
    default_lang = OTL_NEXT_USHORT( p );
    count        = OTL_NEXT_USHORT( p );

    if ( extender )
      otl_jstf_extender_validate( table + extender, valid );

    if ( default_lang )
      otl_jstf_lang_validate( table + default_lang, valid );

    OTL_CHECK( 6*count );

    for ( ; count > 0; count-- )
    {
      p += 4;  /* ignore tag */
      otl_jstf_lang_validate( table + OTL_NEXT_USHORT( p ), valid );
    }
  }


  OTL_LOCALDEF( void )
  otl_jstf_validate( OTL_Bytes      table,
                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 4 );

    if ( OTL_NEXT_ULONG( p ) != 0x10000UL )
      OTL_INVALID_DATA;

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( count*6 );

    for ( ; count > 0; count++ )
    {
      p += 4;  /* ignore tag */
      otl_jstf_script_validate( table + OTL_NEXT_USHORT( p ), valid );
    }
  }
  