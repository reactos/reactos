/***************************************************************************/
/*                                                                         */
/*  otlcommn.h                                                             */
/*                                                                         */
/*    OpenType layout support, common tables (specification).              */
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


#ifndef __OTLCOMMN_H__
#define __OTLCOMMN_H__

#include "otlayout.h"

OTL_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       COVERAGE TABLE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate coverage table */
  OTL_LOCALDEF( void )
  otl_coverage_validate( OTL_Bytes      base,
                         OTL_Validator  valid );

  /* return number of covered glyphs */
  OTL_LOCALDEF( OTL_UInt )
  otl_coverage_get_count( OTL_Bytes  base );

  /* Return the coverage index corresponding to a glyph glyph index. */
  /* Return -1 if the glyph isn't covered.                           */
  OTL_LOCALDEF( OTL_Int )
  otl_coverage_get_index( OTL_Bytes  base,
                          OTL_UInt   glyph_index );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  CLASS DEFINITION TABLE                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate class definition table */
  OTL_LOCALDEF( void )
  otl_class_definition_validate( OTL_Bytes      table,
                                 OTL_Validator  valid );

  /* return class value for a given glyph index */
  OTL_LOCALDEF( OTL_UInt )
  otl_class_definition_get_value( OTL_Bytes  table,
                                  OTL_UInt   glyph_index );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      DEVICE TABLE                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate a device table */
  OTL_LOCALDEF( void )
  otl_device_table_validate( OTL_Bytes      table,
                             OTL_Validator  valid );

  /* return a device table's first size */
  OTL_LOCALDEF( OTL_UInt )
  otl_device_table_get_start( OTL_Bytes  table );

  /* return a device table's last size */
  OTL_LOCALDEF( OTL_UInt )
  otl_device_table_get_end( OTL_Bytes  table );

  /* return pixel adjustment for a given size */
  OTL_LOCALDEF( OTL_Int )
  otl_device_table_get_delta( OTL_Bytes  table,
                              OTL_UInt   size );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           LOOKUPS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate lookup table */
  OTL_LOCALDEF( void )
  otl_lookup_validate( OTL_Bytes      table,
                       OTL_Validator  valid );

  /* return number of sub-tables in a lookup */
  OTL_LOCALDEF( OTL_UInt )
  otl_lookup_get_count( OTL_Bytes  table );


  /* return lookup sub-table */
  OTL_LOCALDEF( OTL_Bytes )
  otl_lookup_get_table( OTL_Bytes  table,
                        OTL_UInt   idx );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      LOOKUP LISTS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate lookup list */
  OTL_LOCALDEF( void )
  otl_lookup_list_validate( OTL_Bytes      table,
                            OTL_Validator  valid );

  /* return number of lookups in list */
  OTL_LOCALDEF( OTL_UInt )
  otl_lookup_list_get_count( OTL_Bytes  table );

  /* return a given lookup from a list */
  OTL_LOCALDEF( OTL_Bytes )
  otl_lookup_list_get_lookup( OTL_Bytes  table,
                              OTL_UInt   idx );

  /* return lookup sub-table from a list */
  OTL_LOCALDEF( OTL_Bytes )
  otl_lookup_list_get_table( OTL_Bytes  table,
                             OTL_UInt   lookup_index,
                             OTL_UInt   table_index );

  /* iterate over lookup list */
  OTL_LOCALDEF( void )
  otl_lookup_list_foreach( OTL_Bytes        table,
                           OTL_ForeachFunc  func,
                           OTL_Pointer      func_data );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        FEATURES                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate feature table */
  OTL_LOCALDEF( void )
  otl_feature_validate( OTL_Bytes      table,
                        OTL_Validator  valid );

  /* return feature's lookup count */
  OTL_LOCALDEF( OTL_UInt )
  otl_feature_get_count( OTL_Bytes  table );

  /* get several lookups indices from a feature. returns the number of */
  /* lookups grabbed                                                   */
  OTL_LOCALDEF( OTL_UInt )
  otl_feature_get_lookups( OTL_Bytes  table,
                           OTL_UInt   start,
                           OTL_UInt   count,
                           OTL_UInt  *lookups );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        FEATURE LIST                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* validate a feature list */
  OTL_LOCALDEF( void )
  otl_feature_list_validate( OTL_Bytes      table,
                             OTL_Validator  valid );

  /* return number of features in list */
  OTL_LOCALDEF( OTL_UInt )
  otl_feature_list_get_count( OTL_Bytes  table );


  /* return a given feature from a list */
  OTL_LOCALDEF( OTL_Bytes )
  otl_feature_list_get_feature( OTL_Bytes  table,
                                OTL_UInt   idx );

  /* iterate over all features in a list */
  OTL_LOCALDEF( void )
  otl_feature_list_foreach( OTL_Bytes        table,
                            OTL_ForeachFunc  func,
                            OTL_Pointer      func_data );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       LANGUAGE SYSTEM                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCAL( void )
  otl_lang_validate( OTL_Bytes      table,
                     OTL_Validator  valid );


  OTL_LOCAL( OTL_UInt )
  otl_lang_get_req_feature( OTL_Bytes  table );


  OTL_LOCAL( OTL_UInt )
  otl_lang_get_count( OTL_Bytes  table );


  OTL_LOCAL( OTL_UInt )
  otl_lang_get_features( OTL_Bytes  table,
                         OTL_UInt   start,
                         OTL_UInt   count,
                         OTL_UInt  *features );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           SCRIPTS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCAL( void )
  otl_script_list_validate( OTL_Bytes          list,
                            OTL_Validator      valid );

  OTL_LOCAL( OTL_Bytes )
  otl_script_list_get_script( OTL_Bytes  table );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                         LOOKUP LISTS                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  OTL_LOCAL( void )
  otl_lookup_list_validate( OTL_Bytes          list,
                            OTL_UInt           type_count,
                            OTL_ValidateFunc*  type_funcs,
                            OTL_Validator      valid );

 /* */

OTL_END_HEADER

#endif /* __OTLCOMMN_H__ */


/* END */
