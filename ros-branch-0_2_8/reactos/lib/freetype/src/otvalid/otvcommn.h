/***************************************************************************/
/*                                                                         */
/*  otvcommn.h                                                             */
/*                                                                         */
/*    OpenType common tables validation (specification).                   */
/*                                                                         */
/*  Copyright 2004 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __OTVCOMMN_H__
#define __OTVCOMMN_H__


#include <ft2build.h>
#include "otvalid.h"
#include FT_INTERNAL_DEBUG_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                         VALIDATION                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct OTV_ValidatorRec_*  OTV_Validator;

  typedef void  (*OTV_Validate_Func)( FT_Bytes       table,
                                      OTV_Validator  valid );

  typedef struct  OTV_ValidatorRec_
  {
    FT_Validator        root;
    FT_UInt             type_count;
    OTV_Validate_Func*  type_funcs;

    FT_UInt             lookup_count;
    FT_UInt             glyph_count;

    FT_UInt             nesting_level;

    OTV_Validate_Func   func[3];

    FT_UInt             extra1;     /* for passing parameters */
    FT_UInt             extra2;
    FT_Bytes            extra3;

#ifdef FT_DEBUG_LEVEL_TRACE
    FT_UInt             debug_indent;
    const FT_String*    debug_function_name[3];
#endif

  } OTV_ValidatorRec;


#undef  FT_INVALID_
#define FT_INVALID_( _prefix, _error )                         \
          ft_validator_error( valid->root, _prefix ## _error )

#define OTV_OPTIONAL_TABLE( _table )  FT_UInt   _table;      \
                                      FT_Bytes  _table ## _p

#define OTV_OPTIONAL_OFFSET( _offset )           \
          FT_BEGIN_STMNT                         \
            _offset ## _p = p;                   \
            _offset       = FT_NEXT_USHORT( p ); \
          FT_END_STMNT

#define OTV_LIMIT_CHECK( _count )                    \
          FT_BEGIN_STMNT                             \
            if ( p + (_count) > valid->root->limit ) \
              FT_INVALID_TOO_SHORT;                  \
          FT_END_STMNT

#define OTV_SIZE_CHECK( _size )                                        \
          FT_BEGIN_STMNT                                               \
            if ( _size > 0 && _size < table_size )                     \
            {                                                          \
              if ( valid->root->level == FT_VALIDATE_PARANOID )        \
                FT_INVALID_OFFSET;                                     \
              else                                                     \
              {                                                        \
                /* strip off `const' */                                \
                FT_Byte*  pp = (FT_Byte*)_size ## _p;                  \
                                                                       \
                                                                       \
                FT_TRACE3(( "\n"                                       \
                            "Invalid offset to optional table `%s'!\n" \
                            "Set to zero.\n"                           \
                            "\n", #_size ));                           \
                                                                       \
                /* always assume 16bit entities */                     \
                _size = pp[0] = pp[1] = 0;                             \
              }                                                        \
            }                                                          \
          FT_END_STMNT


#ifdef FT_DEBUG_LEVEL_TRACE

  /* use preprocessor's argument prescan to expand one argument into two */
#define OTV_NEST1( x )  OTV_NEST1_( x )
#define OTV_NEST1_( func0, name0 )                 \
          FT_BEGIN_STMNT                           \
            valid->nesting_level          = 0;     \
            valid->func[0]                = func0; \
            valid->debug_function_name[0] = name0; \
          FT_END_STMNT

  /* use preprocessor's argument prescan to expand two arguments into four */
#define OTV_NEST2( x, y )  OTV_NEST2_( x, y )
#define OTV_NEST2_( func0, name0, func1, name1 )   \
          FT_BEGIN_STMNT                           \
            valid->nesting_level          = 0;     \
            valid->func[0]                = func0; \
            valid->func[1]                = func1; \
            valid->debug_function_name[0] = name0; \
            valid->debug_function_name[1] = name1; \
          FT_END_STMNT

  /* use preprocessor's argument prescan to expand three arguments into six */
#define OTV_NEST3( x, y, z )  OTV_NEST3_( x, y, z )
#define OTV_NEST3_( func0, name0, func1, name1, func2, name2 ) \
          FT_BEGIN_STMNT                                       \
            valid->nesting_level          = 0;                 \
            valid->func[0]                = func0;             \
            valid->func[1]                = func1;             \
            valid->func[2]                = func2;             \
            valid->debug_function_name[0] = name0;             \
            valid->debug_function_name[1] = name1;             \
            valid->debug_function_name[2] = name2;             \
          FT_END_STMNT

#define OTV_INIT  valid->debug_indent = 0

#define OTV_ENTER                                                            \
          FT_BEGIN_STMNT                                                     \
            valid->debug_indent += 2;                                        \
            FT_TRACE4(( "%*.s", valid->debug_indent, 0 ));                   \
            FT_TRACE4(( "%s table\n",                                        \
                        valid->debug_function_name[valid->nesting_level] )); \
          FT_END_STMNT

#define OTV_NAME_ENTER( name )                             \
          FT_BEGIN_STMNT                                   \
            valid->debug_indent += 2;                      \
            FT_TRACE4(( "%*.s", valid->debug_indent, 0 )); \
            FT_TRACE4(( "%s table\n", name ));             \
          FT_END_STMNT

#define OTV_EXIT  valid->debug_indent -= 2

#define OTV_TRACE( s )                                     \
          FT_BEGIN_STMNT                                   \
            FT_TRACE4(( "%*.s", valid->debug_indent, 0 )); \
            FT_TRACE4( s );                                \
          FT_END_STMNT

#else   /* !FT_DEBUG_LEVEL_TRACE */

  /* use preprocessor's argument prescan to expand one argument into two */
#define OTV_NEST1( x )  OTV_NEST1_( x )
#define OTV_NEST1_( func0, name0 )        \
          FT_BEGIN_STMNT                  \
            valid->nesting_level = 0;     \
            valid->func[0]       = func0; \
          FT_END_STMNT

  /* use preprocessor's argument prescan to expand two arguments into four */
#define OTV_NEST2( x, y )  OTV_NEST2_( x, y )
#define OTV_NEST2_( func0, name0, func1, name1 ) \
          FT_BEGIN_STMNT                         \
            valid->nesting_level = 0;            \
            valid->func[0]       = func0;        \
            valid->func[1]       = func1;        \
          FT_END_STMNT

  /* use preprocessor's argument prescan to expand three arguments into six */
#define OTV_NEST3( x, y, z )  OTV_NEST3_( x, y, z )
#define OTV_NEST3_( func0, name0, func1, name1, func2, name2 ) \
          FT_BEGIN_STMNT                                       \
            valid->nesting_level = 0;                          \
            valid->func[0]       = func0;                      \
            valid->func[1]       = func1;                      \
            valid->func[2]       = func2;                      \
          FT_END_STMNT

#define OTV_INIT                do ; while ( 0 )
#define OTV_ENTER               do ; while ( 0 )
#define OTV_NAME_ENTER( name )  do ; while ( 0 )
#define OTV_EXIT                do ; while ( 0 )

#define OTV_TRACE( s )          do ; while ( 0 )

#endif  /* !FT_DEBUG_LEVEL_TRACE */


#define OTV_RUN  valid->func[0]


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       COVERAGE TABLE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_Coverage_validate( FT_Bytes       table,
                         OTV_Validator  valid );

  /* return first covered glyph */
  FT_LOCAL( FT_UInt )
  otv_Coverage_get_first( FT_Bytes  table );

  /* return last covered glyph */
  FT_LOCAL( FT_UInt )
  otv_Coverage_get_last( FT_Bytes  table );

  /* return number of covered glyphs */
  FT_LOCAL( FT_UInt )
  otv_Coverage_get_count( FT_Bytes  table );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  CLASS DEFINITION TABLE                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_ClassDef_validate( FT_Bytes       table,
                         OTV_Validator  valid );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      DEVICE TABLE                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_Device_validate( FT_Bytes       table,
                       OTV_Validator  valid );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           LOOKUPS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_Lookup_validate( FT_Bytes       table,
                       OTV_Validator  valid );

  FT_LOCAL( void )
  otv_LookupList_validate( FT_Bytes       table,
                           OTV_Validator  valid );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        FEATURES                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_Feature_validate( FT_Bytes       table,
                        OTV_Validator  valid );

  /* lookups must already be validated */
  FT_LOCAL( void )
  otv_FeatureList_validate( FT_Bytes       table,
                            FT_Bytes       lookups,
                            OTV_Validator  valid );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       LANGUAGE SYSTEM                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_LangSys_validate( FT_Bytes       table,
                        OTV_Validator  valid );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                           SCRIPTS                             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL( void )
  otv_Script_validate( FT_Bytes       table,
                       OTV_Validator  valid );

  /* features must already be validated */
  FT_LOCAL( void )
  otv_ScriptList_validate( FT_Bytes       table,
                           FT_Bytes       features,
                           OTV_Validator  valid );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      UTILITY FUNCTIONS                        *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define ChainPosClassSet  otv_x_Ox, "ChainPosClassSet"
#define ChainPosRuleSet   otv_x_Ox, "ChainPosRuleSet"
#define ChainSubClassSet  otv_x_Ox, "ChainSubClassSet"
#define ChainSubRuleSet   otv_x_Ox, "ChainSubRuleSet"
#define JstfLangSys       otv_x_Ox, "JstfLangSys"
#define JstfMax           otv_x_Ox, "JstfMax"
#define LigGlyph          otv_x_Ox, "LigGlyph"
#define LigatureArray     otv_x_Ox, "LigatureArray"
#define LigatureSet       otv_x_Ox, "LigatureSet"
#define PosClassSet       otv_x_Ox, "PosClassSet"
#define PosRuleSet        otv_x_Ox, "PosRuleSet"
#define SubClassSet       otv_x_Ox, "SubClassSet"
#define SubRuleSet        otv_x_Ox, "SubRuleSet"

  FT_LOCAL( void )
  otv_x_Ox ( FT_Bytes       table,
             OTV_Validator  valid );

#define AlternateSubstFormat1     otv_u_C_x_Ox, "AlternateSubstFormat1"
#define ChainContextPosFormat1    otv_u_C_x_Ox, "ChainContextPosFormat1"
#define ChainContextSubstFormat1  otv_u_C_x_Ox, "ChainContextSubstFormat1"
#define ContextPosFormat1         otv_u_C_x_Ox, "ContextPosFormat1"
#define ContextSubstFormat1       otv_u_C_x_Ox, "ContextSubstFormat1"
#define LigatureSubstFormat1      otv_u_C_x_Ox, "LigatureSubstFormat1"
#define MultipleSubstFormat1      otv_u_C_x_Ox, "MultipleSubstFormat1"

  FT_LOCAL( void )
  otv_u_C_x_Ox( FT_Bytes       table,
                OTV_Validator  valid );

#define AlternateSet     otv_x_ux, "AlternateSet"
#define AttachPoint      otv_x_ux, "AttachPoint"
#define ExtenderGlyph    otv_x_ux, "ExtenderGlyph"
#define JstfGPOSModList  otv_x_ux, "JstfGPOSModList"
#define JstfGSUBModList  otv_x_ux, "JstfGSUBModList"
#define Sequence         otv_x_ux, "Sequence"

  FT_LOCAL( void )
  otv_x_ux( FT_Bytes       table,
            OTV_Validator  valid );

#define PosClassRule  otv_x_y_ux_sy, "PosClassRule"
#define PosRule       otv_x_y_ux_sy, "PosRule"
#define SubClassRule  otv_x_y_ux_sy, "SubClassRule"
#define SubRule       otv_x_y_ux_sy, "SubRule"

  FT_LOCAL( void )
  otv_x_y_ux_sy( FT_Bytes       table,
                 OTV_Validator  valid );

#define ChainPosClassRule  otv_x_ux_y_uy_z_uz_p_sp, "ChainPosClassRule"
#define ChainPosRule       otv_x_ux_y_uy_z_uz_p_sp, "ChainPosRule"
#define ChainSubClassRule  otv_x_ux_y_uy_z_uz_p_sp, "ChainSubClassRule"
#define ChainSubRule       otv_x_ux_y_uy_z_uz_p_sp, "ChainSubRule"

  FT_LOCAL( void )
  otv_x_ux_y_uy_z_uz_p_sp( FT_Bytes       table,
                           OTV_Validator  valid );

#define ContextPosFormat2    otv_u_O_O_x_Onx, "ContextPosFormat2"
#define ContextSubstFormat2  otv_u_O_O_x_Onx, "ContextSubstFormat2"

  FT_LOCAL( void )
  otv_u_O_O_x_Onx( FT_Bytes       table,
                   OTV_Validator  valid );

#define ContextPosFormat3    otv_u_x_y_Ox_sy, "ContextPosFormat3"
#define ContextSubstFormat3  otv_u_x_y_Ox_sy, "ContextSubstFormat3"

  FT_LOCAL( void )
  otv_u_x_y_Ox_sy( FT_Bytes       table,
                   OTV_Validator  valid );

#define ChainContextPosFormat2    otv_u_O_O_O_O_x_Onx, "ChainContextPosFormat2"
#define ChainContextSubstFormat2  otv_u_O_O_O_O_x_Onx, "ChainContextSubstFormat2"

  FT_LOCAL( void )
  otv_u_O_O_O_O_x_Onx( FT_Bytes       table,
                       OTV_Validator  valid );

#define ChainContextPosFormat3    otv_u_x_Ox_y_Oy_z_Oz_p_sp, "ChainContextPosFormat3"
#define ChainContextSubstFormat3  otv_u_x_Ox_y_Oy_z_Oz_p_sp, "ChainContextSubstFormat3"

  FT_LOCAL( void )
  otv_u_x_Ox_y_Oy_z_Oz_p_sp( FT_Bytes       table,
                             OTV_Validator  valid );


  FT_LOCAL( FT_UInt )
  otv_GSUBGPOS_get_Lookup_count( FT_Bytes  table );

  FT_LOCAL( FT_UInt )
  otv_GSUBGPOS_have_MarkAttachmentType_flag( FT_Bytes  table );

 /* */

FT_END_HEADER

#endif /* __OTVCOMMN_H__ */


/* END */
