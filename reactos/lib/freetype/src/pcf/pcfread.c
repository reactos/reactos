/*  pcfread.c

    FreeType font driver for pcf fonts

  Copyright 2000-2001, 2002 by
  Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <ft2build.h>

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "pcf.h"
#include "pcfdriver.h"

#include "pcferror.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pcfread


#if defined( FT_DEBUG_LEVEL_TRACE )
  static const char*  tableNames[] =
  {
    "prop", "accl", "mtrcs", "bmps", "imtrcs",
    "enc", "swidth", "names", "accel"
  };
#endif


  static
  const FT_Frame_Field  pcf_toc_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_TocRec

    FT_FRAME_START( 8 ),
      FT_FRAME_ULONG_LE( version ),
      FT_FRAME_ULONG_LE( count ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_table_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_TableRec

    FT_FRAME_START( 16  ),
      FT_FRAME_ULONG_LE( type ),
      FT_FRAME_ULONG_LE( format ),
      FT_FRAME_ULONG_LE( size ),
      FT_FRAME_ULONG_LE( offset ),
    FT_FRAME_END
  };


  static FT_Error
  pcf_read_TOC( FT_Stream  stream,
                PCF_Face   face )
  {
    FT_Error   error;
    PCF_Toc    toc = &face->toc;
    PCF_Table  tables;

    FT_Memory  memory = FT_FACE(face)->memory;
    FT_UInt    n;


    if ( FT_STREAM_SEEK ( 0 )                          ||
         FT_STREAM_READ_FIELDS ( pcf_toc_header, toc ) )
      return PCF_Err_Cannot_Open_Resource;

    if ( toc->version != PCF_FILE_VERSION )
      return PCF_Err_Invalid_File_Format;

    if ( FT_NEW_ARRAY( face->toc.tables, toc->count ) )
      return PCF_Err_Out_Of_Memory;

    tables = face->toc.tables;
    for ( n = 0; n < toc->count; n++ )
    {
      if ( FT_STREAM_READ_FIELDS( pcf_table_header, tables ) )
        goto Exit;
      tables++;
    }

#if defined( FT_DEBUG_LEVEL_TRACE )

    {
      FT_UInt      i, j;
      const char*  name = "?";


      FT_TRACE4(( "Tables count: %ld\n", face->toc.count ));
      tables = face->toc.tables;
      for ( i = 0; i < toc->count; i++ )
      {
        for( j = 0; j < sizeof ( tableNames ) / sizeof ( tableNames[0] ); j++ )
          if ( tables[i].type == (FT_UInt)( 1 << j ) )
            name = tableNames[j];

        FT_TRACE4(( "Table %d: type=%-6s format=0x%04lX "
                    "size=0x%06lX (%8ld) offset=0x%04lX\n",
                    i, name,
                    tables[i].format,
                    tables[i].size, tables[i].size,
                    tables[i].offset ));
      }
    }

#endif

    return PCF_Err_Ok;

  Exit:
    FT_FREE( face->toc.tables );
    return error;
  }


  static
  const FT_Frame_Field  pcf_metric_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_MetricRec

    FT_FRAME_START( 12 ),
      FT_FRAME_SHORT_LE( leftSideBearing ),
      FT_FRAME_SHORT_LE( rightSideBearing ),
      FT_FRAME_SHORT_LE( characterWidth ),
      FT_FRAME_SHORT_LE( ascent ),
      FT_FRAME_SHORT_LE( descent ),
      FT_FRAME_SHORT_LE( attributes ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_metric_msb_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_MetricRec

    FT_FRAME_START( 12 ),
      FT_FRAME_SHORT( leftSideBearing ),
      FT_FRAME_SHORT( rightSideBearing ),
      FT_FRAME_SHORT( characterWidth ),
      FT_FRAME_SHORT( ascent ),
      FT_FRAME_SHORT( descent ),
      FT_FRAME_SHORT( attributes ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_compressed_metric_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_Compressed_MetricRec

    FT_FRAME_START( 5 ),
      FT_FRAME_BYTE( leftSideBearing ),
      FT_FRAME_BYTE( rightSideBearing ),
      FT_FRAME_BYTE( characterWidth ),
      FT_FRAME_BYTE( ascent ),
      FT_FRAME_BYTE( descent ),
    FT_FRAME_END
  };


  static FT_Error
  pcf_get_metric( FT_Stream   stream,
                  FT_ULong    format,
                  PCF_Metric  metric )
  {
    FT_Error  error = PCF_Err_Ok;


    if ( PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
    {
      const FT_Frame_Field*  fields;


      /* parsing normal metrics */
      fields = PCF_BYTE_ORDER( format ) == MSBFirst
               ? pcf_metric_msb_header
               : pcf_metric_header;

      /* the following sets 'error' but doesn't return in case of failure */
      (void)FT_STREAM_READ_FIELDS( fields, metric );
    }
    else
    {
      PCF_Compressed_MetricRec  compr;


      /* parsing compressed metrics */
      if ( FT_STREAM_READ_FIELDS( pcf_compressed_metric_header, &compr ) )
        goto Exit;

      metric->leftSideBearing  = (FT_Short)( compr.leftSideBearing  - 0x80 );
      metric->rightSideBearing = (FT_Short)( compr.rightSideBearing - 0x80 );
      metric->characterWidth   = (FT_Short)( compr.characterWidth   - 0x80 );
      metric->ascent           = (FT_Short)( compr.ascent           - 0x80 );
      metric->descent          = (FT_Short)( compr.descent          - 0x80 );
      metric->attributes       = 0;
    }

  Exit:
    return error;
  }


  static FT_Error
  pcf_seek_to_table_type( FT_Stream  stream,
                          PCF_Table  tables,
                          FT_Int     ntables,
                          FT_ULong   type,
                          FT_ULong  *aformat,
                          FT_ULong  *asize )
  {
    FT_Error  error = 0;
    FT_Int    i;


    for ( i = 0; i < ntables; i++ )
      if ( tables[i].type == type )
      {
        if ( stream->pos > tables[i].offset )
          return PCF_Err_Invalid_Stream_Skip;

        if ( FT_STREAM_SKIP( tables[i].offset - stream->pos ) )
          return PCF_Err_Invalid_Stream_Skip;

        *asize   = tables[i].size;  /* unused - to be removed */
        *aformat = tables[i].format;

        return PCF_Err_Ok;
      }

    return PCF_Err_Invalid_File_Format;
  }


  static FT_Bool
  pcf_has_table_type( PCF_Table  tables,
                      FT_Int     ntables,
                      FT_ULong   type )
  {
    FT_Int  i;


    for ( i = 0; i < ntables; i++ )
      if ( tables[i].type == type )
        return TRUE;

    return FALSE;
  }


  static
  const FT_Frame_Field  pcf_property_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_ParsePropertyRec

    FT_FRAME_START( 9 ),
      FT_FRAME_LONG_LE( name ),
      FT_FRAME_BYTE   ( isString ),
      FT_FRAME_LONG_LE( value ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_property_msb_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_ParsePropertyRec

    FT_FRAME_START( 9 ),
      FT_FRAME_LONG( name ),
      FT_FRAME_BYTE( isString ),
      FT_FRAME_LONG( value ),
    FT_FRAME_END
  };


  static PCF_Property
  pcf_find_property( PCF_Face          face,
                     const FT_String*  prop )
  {
    PCF_Property  properties = face->properties;
    FT_Bool       found      = 0;
    int           i;


    for ( i = 0 ; i < face->nprops && !found; i++ )
    {
      if ( !ft_strcmp( properties[i].name, prop ) )
        found = 1;
    }

    if ( found )
      return properties + i - 1;
    else
      return NULL;
  }


  static FT_Error
  pcf_get_properties( FT_Stream  stream,
                      PCF_Face   face )
  {
    PCF_ParseProperty  props      = 0;
    PCF_Property       properties = 0;
    FT_Int             nprops, i;
    FT_ULong           format, size;
    FT_Error           error;
    FT_Memory          memory     = FT_FACE(face)->memory;
    FT_ULong           string_size;
    FT_String*         strings    = 0;


    error = pcf_seek_to_table_type( stream,
                                    face->toc.tables,
                                    face->toc.count,
                                    PCF_PROPERTIES,
                                    &format,
                                    &size );
    if ( error )
      goto Bail;

    if ( FT_READ_ULONG_LE( format ) )
      goto Bail;

    FT_TRACE4(( "get_prop: format = %ld\n", format ));

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
      goto Bail;

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      (void)FT_READ_ULONG( nprops );
    else
      (void)FT_READ_ULONG_LE( nprops );
    if ( error )
      goto Bail;

    FT_TRACE4(( "get_prop: nprop = %d\n", nprops ));

    if ( FT_NEW_ARRAY( props, nprops ) )
      goto Bail;

    for ( i = 0; i < nprops; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      {
        if ( FT_STREAM_READ_FIELDS( pcf_property_msb_header, props + i ) )
          goto Bail;
      }
      else
      {
        if ( FT_STREAM_READ_FIELDS( pcf_property_header, props + i ) )
          goto Bail;
      }
    }

    /* pad the property array                                            */
    /*                                                                   */
    /* clever here - nprops is the same as the number of odd-units read, */
    /* as only isStringProp are odd length   (Keith Packard)             */
    /*                                                                   */
    if ( nprops & 3 )
    {
      i = 4 - ( nprops & 3 );
      FT_Stream_Skip( stream, i );
    }

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      (void)FT_READ_ULONG( string_size );
    else
      (void)FT_READ_ULONG_LE( string_size );
    if ( error )
      goto Bail;

    FT_TRACE4(( "get_prop: string_size = %ld\n", string_size ));

    if ( FT_NEW_ARRAY( strings, string_size ) )
      goto Bail;

    error = FT_Stream_Read( stream, (FT_Byte*)strings, string_size );
    if ( error )
      goto Bail;

    if ( FT_NEW_ARRAY( properties, nprops ) )
      goto Bail;

    for ( i = 0; i < nprops; i++ )
    {
      /* XXX: make atom */
      if ( FT_NEW_ARRAY( properties[i].name,
                         ft_strlen( strings + props[i].name ) + 1 ) )
        goto Bail;
      ft_strcpy( properties[i].name,strings + props[i].name );

      properties[i].isString = props[i].isString;

      if ( props[i].isString )
      {
        if ( FT_NEW_ARRAY( properties[i].value.atom,
                           ft_strlen( strings + props[i].value ) + 1 ) )
          goto Bail;
        ft_strcpy( properties[i].value.atom, strings + props[i].value );
      }
      else
        properties[i].value.integer = props[i].value;
    }

    face->properties = properties;
    face->nprops = nprops;

    FT_FREE( props );
    FT_FREE( strings );

    return PCF_Err_Ok;

  Bail:
    FT_FREE( props );
    FT_FREE( strings );

    return error;
  }


  static FT_Error
  pcf_get_metrics( FT_Stream  stream,
                   PCF_Face   face )
  {
    FT_Error    error    = PCF_Err_Ok;
    FT_Memory   memory   = FT_FACE(face)->memory;
    FT_ULong    format   = 0;
    FT_ULong    size     = 0;
    PCF_Metric  metrics  = 0;
    int         i;
    int         nmetrics = -1;


    error = pcf_seek_to_table_type( stream,
                                    face->toc.tables,
                                    face->toc.count,
                                    PCF_METRICS,
                                    &format,
                                    &size );
    if ( error )
      return error;

    error = FT_READ_ULONG_LE( format );

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT )     &&
         !PCF_FORMAT_MATCH( format, PCF_COMPRESSED_METRICS ) )
      return PCF_Err_Invalid_File_Format;

    if ( PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)FT_READ_ULONG( nmetrics );
      else
        (void)FT_READ_ULONG_LE( nmetrics );
    }
    else
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)FT_READ_USHORT( nmetrics );
      else
        (void)FT_READ_USHORT_LE( nmetrics );
    }
    if ( error || nmetrics == -1 )
      return PCF_Err_Invalid_File_Format;

    face->nmetrics = nmetrics;

    if ( FT_NEW_ARRAY( face->metrics, nmetrics ) )
      return PCF_Err_Out_Of_Memory;

    metrics = face->metrics;
    for ( i = 0; i < nmetrics; i++ )
    {
      pcf_get_metric( stream, format, metrics + i );

      metrics[i].bits = 0;

      FT_TRACE4(( "%d : width=%d, "
                  "lsb=%d, rsb=%d, ascent=%d, descent=%d, swidth=%d\n",
                  i,
                  ( metrics + i )->characterWidth,
                  ( metrics + i )->leftSideBearing,
                  ( metrics + i )->rightSideBearing,
                  ( metrics + i )->ascent,
                  ( metrics + i )->descent,
                  ( metrics + i )->attributes ));

      if ( error )
        break;
    }

    if ( error )
      FT_FREE( face->metrics );
    return error;
  }


  static FT_Error
  pcf_get_bitmaps( FT_Stream  stream,
                   PCF_Face   face )
  {
    FT_Error   error  = PCF_Err_Ok;
    FT_Memory  memory = FT_FACE(face)->memory;
    FT_Long*   offsets;
    FT_Long    bitmapSizes[GLYPHPADOPTIONS];
    FT_ULong   format, size;
    int        nbitmaps, i, sizebitmaps = 0;
    char*      bitmaps;


    error = pcf_seek_to_table_type( stream,
                                    face->toc.tables,
                                    face->toc.count,
                                    PCF_BITMAPS,
                                    &format,
                                    &size );
    if ( error )
      return error;

    error = FT_Stream_EnterFrame( stream, 8 );
    if ( error )
      return error;

    format = FT_GET_ULONG_LE();
    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      nbitmaps  = FT_GET_ULONG();
    else
      nbitmaps  = FT_GET_ULONG_LE();

    FT_Stream_ExitFrame( stream );

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
      return PCF_Err_Invalid_File_Format;

    if ( nbitmaps != face->nmetrics )
      return PCF_Err_Invalid_File_Format;

    if ( FT_NEW_ARRAY( offsets, nbitmaps ) )
      return error;

    for ( i = 0; i < nbitmaps; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)FT_READ_LONG( offsets[i] );
      else
        (void)FT_READ_LONG_LE( offsets[i] );

      FT_TRACE4(( "bitmap %d is at offset %ld\n", i, offsets[i] ));
    }
    if ( error )
      goto Bail;

    for ( i = 0; i < GLYPHPADOPTIONS; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)FT_READ_LONG( bitmapSizes[i] );
      else
        (void)FT_READ_LONG_LE( bitmapSizes[i] );
      if ( error )
        goto Bail;

      sizebitmaps = bitmapSizes[PCF_GLYPH_PAD_INDEX( format )];

      FT_TRACE4(( "padding %d implies a size of %ld\n", i, bitmapSizes[i] ));
    }

    FT_TRACE4(( "  %d bitmaps, padding index %ld\n",
                nbitmaps,
                PCF_GLYPH_PAD_INDEX( format ) ));
    FT_TRACE4(( "bitmap size = %d\n", sizebitmaps ));

    FT_UNUSED( sizebitmaps );       /* only used for debugging */

    for ( i = 0; i < nbitmaps; i++ )
      face->metrics[i].bits = stream->pos + offsets[i];

    face->bitmapsFormat = format;

    FT_FREE ( offsets );
    return error;

  Bail:
    FT_FREE ( offsets );
    FT_FREE ( bitmaps );
    return error;
  }


  static FT_Error
  pcf_get_encodings( FT_Stream  stream,
                     PCF_Face   face )
  {
    FT_Error      error   = PCF_Err_Ok;
    FT_Memory     memory  = FT_FACE(face)->memory;
    FT_ULong      format, size;
    int           firstCol, lastCol;
    int           firstRow, lastRow;
    int           nencoding, encodingOffset;
    int           i, j;
    PCF_Encoding  tmpEncoding, encoding = 0;


    error = pcf_seek_to_table_type( stream,
                                    face->toc.tables,
                                    face->toc.count,
                                    PCF_BDF_ENCODINGS,
                                    &format,
                                    &size );
    if ( error )
      return error;

    error = FT_Stream_EnterFrame( stream, 14 );
    if ( error )
      return error;

    format = FT_GET_ULONG_LE();

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
    {
      firstCol          = FT_GET_SHORT();
      lastCol           = FT_GET_SHORT();
      firstRow          = FT_GET_SHORT();
      lastRow           = FT_GET_SHORT();
      face->defaultChar = FT_GET_SHORT();
    }
    else
    {
      firstCol          = FT_GET_SHORT_LE();
      lastCol           = FT_GET_SHORT_LE();
      firstRow          = FT_GET_SHORT_LE();
      lastRow           = FT_GET_SHORT_LE();
      face->defaultChar = FT_GET_SHORT_LE();
    }

    FT_Stream_ExitFrame( stream );

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
      return PCF_Err_Invalid_File_Format;

    FT_TRACE4(( "enc: firstCol %d, lastCol %d, firstRow %d, lastRow %d\n",
                firstCol, lastCol, firstRow, lastRow ));

    nencoding = ( lastCol - firstCol + 1 ) * ( lastRow - firstRow + 1 );

    if ( FT_NEW_ARRAY( tmpEncoding, nencoding ) )
      return PCF_Err_Out_Of_Memory;

    error = FT_Stream_EnterFrame( stream, 2 * nencoding );
    if ( error )
      goto Bail;

    for ( i = 0, j = 0 ; i < nencoding; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        encodingOffset = FT_GET_SHORT();
      else
        encodingOffset = FT_GET_SHORT_LE();

      if ( encodingOffset != -1 )
      {
        tmpEncoding[j].enc = ( ( ( i / ( lastCol - firstCol + 1 ) ) +
                                 firstRow ) * 256 ) +
                               ( ( i % ( lastCol - firstCol + 1 ) ) +
                                 firstCol );

        tmpEncoding[j].glyph = (FT_Short)encodingOffset;
        j++;
      }

      FT_TRACE4(( "enc n. %d ; Uni %ld ; Glyph %d\n",
                  i, tmpEncoding[j - 1].enc, encodingOffset ));
    }
    FT_Stream_ExitFrame( stream );

    if ( FT_NEW_ARRAY( encoding, j ) )
      goto Bail;

    for ( i = 0; i < j; i++ )
    {
      encoding[i].enc   = tmpEncoding[i].enc;
      encoding[i].glyph = tmpEncoding[i].glyph;
    }

    face->nencodings = j;
    face->encodings  = encoding;
    FT_FREE( tmpEncoding );

    return error;

  Bail:
    FT_FREE( encoding );
    FT_FREE( tmpEncoding );
    return error;
  }


  static
  const FT_Frame_Field  pcf_accel_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_AccelRec

    FT_FRAME_START( 20 ),
      FT_FRAME_BYTE      ( noOverlap ),
      FT_FRAME_BYTE      ( constantMetrics ),
      FT_FRAME_BYTE      ( terminalFont ),
      FT_FRAME_BYTE      ( constantWidth ),
      FT_FRAME_BYTE      ( inkInside ),
      FT_FRAME_BYTE      ( inkMetrics ),
      FT_FRAME_BYTE      ( drawDirection ),
      FT_FRAME_SKIP_BYTES( 1 ),
      FT_FRAME_LONG_LE   ( fontAscent ),
      FT_FRAME_LONG_LE   ( fontDescent ),
      FT_FRAME_LONG_LE   ( maxOverlap ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_accel_msb_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_AccelRec

    FT_FRAME_START( 20 ),
      FT_FRAME_BYTE      ( noOverlap ),
      FT_FRAME_BYTE      ( constantMetrics ),
      FT_FRAME_BYTE      ( terminalFont ),
      FT_FRAME_BYTE      ( constantWidth ),
      FT_FRAME_BYTE      ( inkInside ),
      FT_FRAME_BYTE      ( inkMetrics ),
      FT_FRAME_BYTE      ( drawDirection ),
      FT_FRAME_SKIP_BYTES( 1 ),
      FT_FRAME_LONG      ( fontAscent ),
      FT_FRAME_LONG      ( fontDescent ),
      FT_FRAME_LONG      ( maxOverlap ),
    FT_FRAME_END
  };


  static FT_Error
  pcf_get_accel( FT_Stream  stream,
                 PCF_Face   face,
                 FT_ULong   type )
  {
    FT_ULong   format, size;
    FT_Error   error = PCF_Err_Ok;
    PCF_Accel  accel = &face->accel;


    error = pcf_seek_to_table_type( stream,
                                    face->toc.tables,
                                    face->toc.count,
                                    type,
                                    &format,
                                    &size );
    if ( error )
      goto Bail;

    error = FT_READ_ULONG_LE( format );

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT )    &&
         !PCF_FORMAT_MATCH( format, PCF_ACCEL_W_INKBOUNDS ) )
      goto Bail;

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
    {
      if ( FT_STREAM_READ_FIELDS( pcf_accel_msb_header, accel ) )
        goto Bail;
    }
    else
    {
      if ( FT_STREAM_READ_FIELDS( pcf_accel_header, accel ) )
        goto Bail;
    }

    error = pcf_get_metric( stream,
                            format & ( ~PCF_FORMAT_MASK ),
                            &(accel->minbounds) );
    if ( error )
      goto Bail;

    error = pcf_get_metric( stream,
                            format & ( ~PCF_FORMAT_MASK ),
                            &(accel->maxbounds) );
    if ( error )
      goto Bail;

    if ( PCF_FORMAT_MATCH( format, PCF_ACCEL_W_INKBOUNDS ) )
    {
      error = pcf_get_metric( stream,
                              format & ( ~PCF_FORMAT_MASK ),
                              &(accel->ink_minbounds) );
      if ( error )
        goto Bail;

      error = pcf_get_metric( stream,
                              format & ( ~PCF_FORMAT_MASK ),
                              &(accel->ink_maxbounds) );
      if ( error )
        goto Bail;
    }
    else
    {
      accel->ink_minbounds = accel->minbounds; /* I'm not sure about this */
      accel->ink_maxbounds = accel->maxbounds;
    }
    return error;

  Bail:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  pcf_load_font( FT_Stream  stream,
                 PCF_Face   face )
  {
    FT_Error   error  = PCF_Err_Ok;
    FT_Memory  memory = FT_FACE(face)->memory;
    FT_Bool    hasBDFAccelerators;


    error = pcf_read_TOC( stream, face );
    if ( error )
      goto Exit;

    error = pcf_get_properties( stream, face );
    if ( error )
      goto Exit;

    /* Use the old accelerators if no BDF accelerators are in the file. */
    hasBDFAccelerators = pcf_has_table_type( face->toc.tables,
                                             face->toc.count,
                                             PCF_BDF_ACCELERATORS );
    if ( !hasBDFAccelerators )
    {
      error = pcf_get_accel( stream, face, PCF_ACCELERATORS );
      if ( error )
        goto Exit;
    }

    /* metrics */
    error = pcf_get_metrics( stream, face );
    if ( error )
      goto Exit;

    /* bitmaps */
    error = pcf_get_bitmaps( stream, face );
    if ( error )
      goto Exit;

    /* encodings */
    error = pcf_get_encodings( stream, face );
    if ( error )
      goto Exit;

    /* BDF style accelerators (i.e. bounds based on encoded glyphs) */
    if ( hasBDFAccelerators )
    {
      error = pcf_get_accel( stream, face, PCF_BDF_ACCELERATORS );
      if ( error )
        goto Exit;
    }

    /* XXX: TO DO: inkmetrics and glyph_names are missing */

    /* now construct the face object */
    {
      FT_Face       root = FT_FACE( face );
      PCF_Property  prop;
      int           size_set = 0;


      root->num_faces = 1;
      root->face_index = 0;
      root->face_flags = FT_FACE_FLAG_FIXED_SIZES |
                         FT_FACE_FLAG_HORIZONTAL  |
                         FT_FACE_FLAG_FAST_GLYPHS;

      if ( face->accel.constantWidth )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      root->style_flags = 0;
      prop = pcf_find_property( face, "SLANT" );
      if ( prop != NULL )
        if ( prop->isString )
          if ( ( *(prop->value.atom) == 'O' ) ||
               ( *(prop->value.atom) == 'I' ) )
            root->style_flags |= FT_STYLE_FLAG_ITALIC;

      prop = pcf_find_property( face, "WEIGHT_NAME" );
      if ( prop != NULL )
        if ( prop->isString )
          if ( *(prop->value.atom) == 'B' )
            root->style_flags |= FT_STYLE_FLAG_BOLD;

      root->style_name = (char *)"Regular";

      if ( root->style_flags & FT_STYLE_FLAG_BOLD ) {
        if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
          root->style_name = (char *)"Bold Italic";
        else
          root->style_name = (char *)"Bold";
      }
      else if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
        root->style_name = (char *)"Italic";

      prop = pcf_find_property( face, "FAMILY_NAME" );
      if ( prop != NULL )
      {
        if ( prop->isString )
        {
          int  l = ft_strlen( prop->value.atom ) + 1;


          if ( FT_NEW_ARRAY( root->family_name, l ) )
            goto Exit;
          ft_strcpy( root->family_name, prop->value.atom );
        }
      }
      else
        root->family_name = 0;

      root->num_glyphs = face->nmetrics;

      root->num_fixed_sizes = 1;
      if ( FT_NEW_ARRAY( root->available_sizes, 1 ) )
        goto Exit;

      prop = pcf_find_property( face, "PIXEL_SIZE" );
      if ( prop != NULL )
      {
        root->available_sizes->height =
        root->available_sizes->width  = (FT_Short)( prop->value.integer );

        size_set = 1;
      }
      else
      {
        prop = pcf_find_property( face, "POINT_SIZE" );
        if ( prop != NULL )
        {
          PCF_Property  xres, yres, avgw;


          xres = pcf_find_property( face, "RESOLUTION_X" );
          yres = pcf_find_property( face, "RESOLUTION_Y" );
          avgw = pcf_find_property( face, "AVERAGE_WIDTH" );

          if ( ( yres != NULL ) && ( xres != NULL ) )
          {
            root->available_sizes->height =
              (FT_Short)( prop->value.integer *
                          yres->value.integer / 720 );

              root->available_sizes->width =
                (FT_Short)( prop->value.integer *
                            xres->value.integer / 720 );

            size_set = 1;
          }
        }
      }

      if (size_set == 0 )
      {
        root->available_sizes->width  = 12;
        root->available_sizes->height = 12;
      }

      /* set-up charset */
      {
        PCF_Property  charset_registry = 0, charset_encoding = 0;


        charset_registry = pcf_find_property( face, "CHARSET_REGISTRY" );
        charset_encoding = pcf_find_property( face, "CHARSET_ENCODING" );

        if ( ( charset_registry != NULL ) &&
             ( charset_encoding != NULL ) )
        {
          if ( ( charset_registry->isString ) &&
               ( charset_encoding->isString ) )
          {
            if ( FT_NEW_ARRAY( face->charset_encoding,
                               ft_strlen( charset_encoding->value.atom ) + 1 ) )
              goto Exit;

            if ( FT_NEW_ARRAY( face->charset_registry,
                               ft_strlen( charset_registry->value.atom ) + 1 ) )
              goto Exit;

            ft_strcpy( face->charset_registry, charset_registry->value.atom );
            ft_strcpy( face->charset_encoding, charset_encoding->value.atom );
          }
        }
      }
    }

  Exit:
    if ( error )
    {
      /* this is done to respect the behaviour of the original */
      /* PCF font driver.                                      */
      error = PCF_Err_Invalid_File_Format;
    }

    return error;
  }


/* END */
