#include "otlparse.h"
#include "otlutils.h"

  static void
  otl_string_ensure( OTL_String  string,
                     OTL_UInt    count,
                     OTL_Parser  parser )
  {
    count += string->length;

    if ( count > string->capacity )
    {
      OTL_UInt    old_count = string->capacity;
      OTL_UInt    new_count = old_count;
      OTL_Memory  memory    = parser->memory;

      while ( new_count < count )
        new_count += (new_count >> 1) + 16;

      if ( OTL_MEM_RENEW_ARRAY( string->glyphs, old_count, new_count ) )
        otl_parser_error( parser, OTL_Parse_Err_Memory );

      string->capacity = new_count;
    }
  }

#define  OTL_STRING_ENSURE(str,count,parser)                   \
           OTL_BEGIN_STMNT                                     \
             if ( (str)->length + (count) > (str)>capacity )   \
               otl_string_ensure( str, count, parser );        \
           OTL_END_STMNT


  OTL_LOCALDEF( OTL_UInt )
  otl_parser_get_gindex( OTL_Parser  parser )
  {
    OTL_String  in = parser->str_in;

    if ( in->cursor >= in->num_glyphs )
      otl_parser_error( parser, OTL_Err_Parser_Internal );

    return in->str[ in->cursor ].gindex;
  }


  OTL_LOCALDEF( void )
  otl_parser_error( OTL_Parser      parser,
                    OTL_ParseError  error; )
  {
    parser->error = error;
    otl_longjmp( parser->jump_buffer, 1 );
  }

#define  OTL_PARSER_UNCOVERED(x)  otl_parser_error( x, OTL_Parse_Err_UncoveredGlyph );

  OTL_LOCAL( void )
  otl_parser_check_property( OTL_Parser  parser,
                             OTL_UInt    gindex,
                             OTL_UInt    flags,
                             OTL_UInt   *aproperty );

  OTL_LOCALDEF( void )
  otl_parser_replace_1( OTL_Parser  parser,
                        OTL_UInt    gindex )
  {
    OTL_String       in  = parser->str_in;
    OTL_String       out = parser->str_out;
    OTL_StringGlyph  glyph, in_glyph;

    /* sanity check */
    if ( in == NULL               ||
         out == NULL              ||
         in->length == 0          ||
         in->cursor >= in->length )
    {
      /* report as internal error, since these should */
      /* never happen !!                              */
      otl_parser_error( parser, OTL_Err_Parse_Internal );
    }

    OTL_STRING_ENSURE( out, 1, parser );
    glyph    = out->glyphs + out->length;
    in_glyph = in->glyphs  + in->cursor;

    glyph->gindex        = gindex;
    glyph->property      = in_glyph->property;
    glyph->lig_component = in_glyph->lig_component;
    glyph->lig_id        = in_glyph->lig_id;

    out->length += 1;
    out->cursor  = out->length;
    in->cursor  += 1;
  }

  OTL_LOCALDEF( void )
  otl_parser_replace_n( OTL_Parser  parser,
                        OTL_UInt    count,
                        OTL_Bytes   indices )
  {
    OTL_UInt         lig_component, lig_id, property;
    OTL_String       in  = parser->str_in;
    OTL_String       out = parser->str_out;
    OTL_StringGlyph  glyph, in_glyph;
    OTL_Bytes        p = indices;

    /* sanity check */
    if ( in == NULL               ||
         out == NULL              ||
         in->length == 0          ||
         in->cursor >= in->length )
    {
      /* report as internal error, since these should */
      /* never happen !!                              */
      otl_parser_error( parser, OTL_Err_Parse_Internal );
    }

    OTL_STRING_ENSURE( out, count, parser );
    glyph    = out->glyphs + out->length;
    in_glyph = in->glyphs  + in->cursor;

    glyph->gindex = gindex;

    lig_component = in_glyph->lig_component;
    lig_id        = in_glyph->lid_id;
    property      = in_glyph->property;

    for ( ; count > 0; count-- )
    {
      glyph->gindex        = OTL_NEXT_USHORT(p);
      glyph->property      = property;
      glyph->lig_component = lig_component;
      glyph->lig_id        = lig_id;

      out->length ++;
    }

    out->cursor  = out->length;
    in->cursor  += 1;
  }



