#ifndef __OTL_PARSER_H__
#define __OTL_PARSER_H__

#include "otlayout.h"
#include "otlgdef.h"
#include "otlgsub.h"
#include "otlgpos.h"

OTL_BEGIN_HEADER

  typedef struct OTL_ParserRec_*   OTL_Parser;

  typedef struct OTL_StringRec_*   OTL_String;

  typedef struct OTL_StringGlyphRec_
  {
    OTL_UInt  gindex;
    OTL_UInt  properties;
    OTL_UInt  lig_component;
    OTL_UInt  lig_id;

  } OTL_StringGlyphRec, *OTL_StringGlyph;

  typedef struct OTL_StringRec_
  {
    OTL_StringGlyph  glyphs;
    OTL_UInt         cursor;
    OTL_UInt         length;
    OTL_UInt         capacity;

  } OTL_StringRec;

  typedef struct OTL_ParserRec_
  {
    OTL_Bytes      tab_gdef;
    OTL_Bytes      tab_gsub;
    OTL_Bytes      tab_gpos;
    OTL_Bytes      tab_base;
    OTL_Bytes      tab_jstf;

    OTL_Alternate  alternate;  /* external alternate handler */

    OTL_UInt       context_len;
    OTL_UInt       markup_flags;

    OTL_jmp_buf    jump_buffer;
    OTL_Memory     memory;
    OTL_Error      error;

    OTL_StringRec  strings[2];
    OTL_String     str_in;
    OTL_String     str_out;

  } OTL_ParserRec;

  typedef enum
  {
    OTL_Err_Parser_Ok = 0,
    OTL_Err_Parser_InvalidData,
    OTL_Err_Parser_UncoveredGlyph

  } OTL_ParseError;

  OTL_LOCAL( OTL_UInt )
  otl_parser_get_gindex( OTL_Parser  parser );


  OTL_LOCAL( void )
  otl_parser_error( OTL_Parser  parser, OTL_ParserError  error );

#define  OTL_PARSER_UNCOVERED(x)  \
           otl_parser_error( x, OTL_Err_Parser_UncoveredGlyph )

  OTL_LOCAL( void )
  otl_parser_check_property( OTL_Parser  parser,
                             OTL_UInt    gindex,
                             OTL_UInt    flags,
                             OTL_UInt   *aproperty );

 /* copy current input glyph to output */
  OTL_LOCAL( void )
  otl_parser_copy_1( OTL_Parser  parser );

 /* copy current input glyph to output, replacing its index */
  OTL_LOCAL( void )
  otl_parser_replace_1( OTL_Parser  parser,
                        OTL_UInt    gindex );

 /* copy current input glyph to output, replacing it by several indices */
 /* read directly from the table                                        */
  OTL_LOCAL( void )
  otl_parser_replace_n( OTL_Parser  parser,
                        OTL_UInt    count,
                        OTL_Bytes   indices );

OTL_END_HEADER

#endif /* __OTL_PARSER_H__ */

