/***************************************************************************/
/*                                                                         */
/*  ttload.c                                                               */
/*                                                                         */
/*    Load the basic TrueType tables, i.e., tables that can be either in   */
/*    TTF or OTF fonts (body).                                             */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/tterrors.h>
#include <freetype/tttags.h>


#ifdef FT_FLAT_COMPILE

#include "ttload.h"
#include "ttcmap.h"

#else

#include <freetype/src/sfnt/ttload.h>
#include <freetype/src/sfnt/ttcmap.h>

#endif


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttload


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_LookUp_Table                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks for a TrueType table by name.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A face object handle.                                      */
  /*    tag  :: The searched tag.                                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A pointer to the table directory entry.  0 if not found.           */
  /*                                                                       */
  LOCAL_FUNC
  TT_Table*  TT_LookUp_Table( TT_Face   face,
                              FT_ULong  tag  )
  {
    TT_Table*  entry;
    TT_Table*  limit;


    FT_TRACE3(( "TT_LookUp_Table: %08p, `%c%c%c%c'\n",
                  face,
                  (FT_Char)( tag >> 24 ),
                  (FT_Char)( tag >> 16 ),
                  (FT_Char)( tag >> 8  ),
                  (FT_Char)( tag       ) ));

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {
      if ( entry->Tag == tag )
        return entry;
    }

    FT_TRACE3(( "    Could not find table!\n" ));
    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Goto_Table                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks for a TrueType table by name, then seek a stream to it.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A face object handle.                                    */
  /*    tag    :: The searched tag.                                        */
  /*    stream :: The stream to seek when the table is found.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    length :: The length of the table if found, undefined otherwise.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Goto_Table( TT_Face    face,
                           FT_ULong   tag,
                           FT_Stream  stream,
                           FT_ULong*  length )
  {
    TT_Table*  table;
    FT_Error   error;


    table = TT_LookUp_Table( face, tag );
    if ( table )
    {
      if ( length )
        *length = table->Length;

      (void)FILE_Seek( table->Offset );
    }
    else
      error = TT_Err_Table_Missing;

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SFNT_Header                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the header of a SFNT font file.  Supports collections.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the target face object.                  */
  /*    stream     :: The input stream.                                    */
  /*    face_index :: If the font is a collection, the number of the font  */
  /*                  in the collection, ignored otherwise.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    sfnt       :: The SFNT header.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  /*    This function recognizes fonts embedded in a `TrueType collection' */
  /*                                                                       */
  /*    The header will be checked whether it is valid by looking at the   */
  /*    values of `search_range', `entry_selector', and `range_shift'.     */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_SFNT_Header( TT_Face       face,
                                 FT_Stream     stream,
                                 FT_Long       face_index,
                                 SFNT_Header*  sfnt )
  {
    FT_Error   error;
    FT_ULong   format_tag;
    FT_Memory  memory = stream->memory;

    const FT_Frame_Field  sfnt_header_fields[] =
    {
      FT_FRAME_START( 8 ),
        FT_FRAME_USHORT( SFNT_Header, num_tables ),
        FT_FRAME_USHORT( SFNT_Header, search_range ),
        FT_FRAME_USHORT( SFNT_Header, entry_selector ),
        FT_FRAME_USHORT( SFNT_Header, range_shift ),
      FT_FRAME_END
    };

    const FT_Frame_Field  ttc_header_fields[] =
    {
      FT_FRAME_START( 8 ),
        FT_FRAME_LONG( TTC_Header, version ),
        FT_FRAME_LONG( TTC_Header, count   ),
      FT_FRAME_END };


    FT_TRACE2(( "TT_Load_SFNT_Header: %08p, %ld\n",
                face, face_index ));

    face->ttc_header.tag      = 0;
    face->ttc_header.version  = 0;
    face->ttc_header.count    = 0;

    face->num_tables = 0;

    /* first of all, read the first 4 bytes.  If it is `ttcf', then the */
    /* file is a TrueType collection, otherwise it can be any other     */
    /* kind of font.                                                    */
    if ( READ_ULong( format_tag ) )
      goto Exit;

    if ( format_tag == TTAG_ttcf )
    {
      FT_Int  n;


      FT_TRACE3(( "TT_Load_SFNT_Header: file is a collection\n" ));

      /* it's a TrueType collection, i.e. a file containing several */
      /* font files.  Read the font directory now                   */
      if ( READ_Fields( ttc_header_fields, &face->ttc_header ) )
        goto Exit;

      /* now read the offsets of each font in the file */
      if ( ALLOC_ARRAY( face->ttc_header.offsets,
                        face->ttc_header.count,
                        FT_ULong )                     ||
           ACCESS_Frame( face->ttc_header.count * 4L ) )
        goto Exit;

      for ( n = 0; n < face->ttc_header.count; n++ )
        face->ttc_header.offsets[n] = GET_ULong();

      FORGET_Frame();

      /* check face index */
      if ( face_index >= face->ttc_header.count )
      {
        error = TT_Err_Bad_Argument;
        goto Exit;
      }

      /* seek to the appropriate TrueType file, then read tag */
      if ( FILE_Seek( face->ttc_header.offsets[face_index] ) ||
           READ_Long( format_tag )                           )
        goto Exit;
    }

    /* the format tag was read, now check the rest of the header */
    sfnt->format_tag = format_tag;
    if ( READ_Fields( sfnt_header_fields, sfnt ) )
      goto Exit;

    /* now, check the values of `num_tables', `seach_range', etc. */
    {
      FT_UInt   num_tables     = sfnt->num_tables;
      FT_ULong  entry_selector = 1L << sfnt->entry_selector;


      /* IMPORTANT: Many fonts have an incorrect `search_range' value, so */
      /*            we only check the `entry_selector' correctness here.  */
      /*                                                                  */
      if ( num_tables == 0                  ||
           entry_selector > num_tables      ||
           entry_selector * 2 <= num_tables )
      {
        FT_TRACE2(( "TT_Load_SFNT_Header: file is not SFNT!\n" ));
        error = FT_Err_Unknown_File_Format;
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Directory                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the table directory into a face object.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: The input stream.                                        */
  /*    sfnt   :: The SFNT directory header.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Directory( TT_Face       face,
                               FT_Stream     stream,
                               SFNT_Header*  sfnt )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    TT_Table *entry, *limit;


    FT_TRACE2(( "TT_Load_Directory: %08p\n", face ));

    FT_TRACE2(( "-- Tables count:   %12u\n",  sfnt->num_tables ));
    FT_TRACE2(( "-- Format version: %08lx\n", sfnt->format_tag ));

    face->num_tables = sfnt->num_tables;

    if ( ALLOC_ARRAY( face->dir_tables,
                      face->num_tables,
                      TT_Table ) )
      goto Exit;

    if ( ACCESS_Frame( face->num_tables * 16L ) )
      goto Exit;

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {                    /* loop through the tables and get all entries */
      entry->Tag      = GET_Tag4();
      entry->CheckSum = GET_ULong();
      entry->Offset   = GET_Long();
      entry->Length   = GET_Long();

      FT_TRACE2(( "  %c%c%c%c  -  %08lx  -  %08lx\n",
                  (FT_Char)( entry->Tag >> 24 ),
                  (FT_Char)( entry->Tag >> 16 ),
                  (FT_Char)( entry->Tag >> 8  ),
                  (FT_Char)( entry->Tag       ),
                  entry->Offset,
                  entry->Length ));
    }

    FORGET_Frame();

    FT_TRACE2(( "Directory loaded\n\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Any                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads any font table into client memory.                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: The face object to look for.                             */
  /*                                                                       */
  /*    tag    :: The tag of table to load.  Use the value 0 if you want   */
  /*              to access the whole font file, else set this parameter   */
  /*              to a valid TrueType table tag that you can forge with    */
  /*              the MAKE_TT_TAG macro.                                   */
  /*                                                                       */
  /*    offset :: The starting offset in the table (or the file if         */
  /*              tag == 0).                                               */
  /*                                                                       */
  /*    length :: The address of the decision variable:                    */
  /*                                                                       */
  /*                If length == NULL:                                     */
  /*                  Loads the whole table.  Returns an error if          */
  /*                  `offset' == 0!                                       */
  /*                                                                       */
  /*                If *length == 0:                                       */
  /*                  Exits immediately; returning the length of the given */
  /*                  table or of the font file, depending on the value of */
  /*                  `tag'.                                               */
  /*                                                                       */
  /*                If *length != 0:                                       */
  /*                  Loads the next `length' bytes of table or font,      */
  /*                  starting at offset `offset' (in table or font too).  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    buffer :: The address of target buffer.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Any( TT_Face    face,
                         FT_ULong   tag,
                         FT_Long    offset,
                         FT_Byte*   buffer,
                         FT_ULong*  length )
  {
    FT_Error   error;
    FT_Stream  stream;
    TT_Table*  table;
    FT_ULong   size;


    if ( tag != 0 )
    {
      /* look for tag in font directory */
      table = TT_LookUp_Table( face, tag );
      if ( !table )
      {
        error = TT_Err_Table_Missing;
        goto Exit;
      }

      offset += table->Offset;
      size    = table->Length;
    }
    else
      /* tag == 0 -- the user wants to access the font file directly */
      size = face->root.stream->size;

    if ( length && *length == 0 )
    {
      *length = size;

      return TT_Err_Ok;
    }

    if ( length )
      size = *length;

    stream = face->root.stream;
    (void)FILE_Read_At( offset, buffer, size );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Header                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the TrueType font header.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Header( TT_Face    face,
                            FT_Stream  stream )
  {
    FT_Error    error;
    TT_Header*  header;

    static const FT_Frame_Field  header_fields[] =
    {
      FT_FRAME_START( 54 ),
        FT_FRAME_ULONG(  TT_Header, Table_Version ),
        FT_FRAME_ULONG(  TT_Header, Font_Revision ),
        FT_FRAME_LONG(   TT_Header, CheckSum_Adjust ),
        FT_FRAME_LONG(   TT_Header, Magic_Number ),
        FT_FRAME_USHORT( TT_Header, Flags ),
        FT_FRAME_USHORT( TT_Header, Units_Per_EM ),
        FT_FRAME_LONG(   TT_Header, Created[0] ),
        FT_FRAME_LONG(   TT_Header, Created[1] ),
        FT_FRAME_LONG(   TT_Header, Modified[0] ),
        FT_FRAME_LONG(   TT_Header, Modified[1] ),
        FT_FRAME_SHORT(  TT_Header, xMin ),
        FT_FRAME_SHORT(  TT_Header, yMin ),
        FT_FRAME_SHORT(  TT_Header, xMax ),
        FT_FRAME_SHORT(  TT_Header, yMax ),
        FT_FRAME_USHORT( TT_Header, Mac_Style ),
        FT_FRAME_USHORT( TT_Header, Lowest_Rec_PPEM ),
        FT_FRAME_SHORT(  TT_Header, Font_Direction ),
        FT_FRAME_SHORT(  TT_Header, Index_To_Loc_Format ),
        FT_FRAME_SHORT(  TT_Header, Glyph_Data_Format ),
      FT_FRAME_END
    };


    FT_TRACE2(( "Load_TT_Header: %08p\n", face ));

    error = face->goto_table( face, TTAG_head, stream, 0 );
    if ( error )
    {
      FT_TRACE0(( "Font Header is missing!\n" ));
      goto Exit;
    }

    header = &face->header;

    if ( READ_Fields( header_fields, header ) )
      goto Exit;

    FT_TRACE2(( "    Units per EM: %8u\n", header->Units_Per_EM ));
    FT_TRACE2(( "    IndexToLoc:   %8d\n", header->Index_To_Loc_Format ));
    FT_TRACE2(( "Font Header Loaded.\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_MaxProfile                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the maximum profile into a face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_MaxProfile( TT_Face    face,
                                FT_Stream  stream )
  {
    FT_Error        error;
    TT_MaxProfile*  maxProfile = &face->max_profile;

    const FT_Frame_Field  maxp_fields[] =
    {
      FT_FRAME_START( 32 ),
        FT_FRAME_ULONG(  TT_MaxProfile, version ),
        FT_FRAME_USHORT( TT_MaxProfile, numGlyphs ),
        FT_FRAME_USHORT( TT_MaxProfile, maxPoints ),
        FT_FRAME_USHORT( TT_MaxProfile, maxContours ),
        FT_FRAME_USHORT( TT_MaxProfile, maxCompositePoints ),
        FT_FRAME_USHORT( TT_MaxProfile, maxCompositeContours ),
        FT_FRAME_USHORT( TT_MaxProfile, maxZones ),
        FT_FRAME_USHORT( TT_MaxProfile, maxTwilightPoints ),
        FT_FRAME_USHORT( TT_MaxProfile, maxStorage ),
        FT_FRAME_USHORT( TT_MaxProfile, maxFunctionDefs ),
        FT_FRAME_USHORT( TT_MaxProfile, maxInstructionDefs ),
        FT_FRAME_USHORT( TT_MaxProfile, maxStackElements ),
        FT_FRAME_USHORT( TT_MaxProfile, maxSizeOfInstructions ),
        FT_FRAME_USHORT( TT_MaxProfile, maxComponentElements ),
        FT_FRAME_USHORT( TT_MaxProfile, maxComponentDepth ),
      FT_FRAME_END };


    FT_TRACE2(( "Load_TT_MaxProfile: %08p\n", face ));

    error = face->goto_table( face, TTAG_maxp, stream, 0 );
    if ( error )
      goto Exit;

    if ( READ_Fields( maxp_fields, maxProfile ) )
      goto Exit;

    /* XXX: an adjustment that is necessary to load certain */
    /*      broken fonts like `Keystrokes MT' :-(           */
    /*                                                      */
    /*   We allocate 64 function entries by default when    */
    /*   the maxFunctionDefs field is null.                 */

    if ( maxProfile->maxFunctionDefs == 0 )
      maxProfile->maxFunctionDefs = 64;

    face->root.num_glyphs = maxProfile->numGlyphs;

    face->root.max_points = MAX( maxProfile->maxCompositePoints,
                                 maxProfile->maxPoints );

    face->root.max_contours = MAX( maxProfile->maxCompositeContours,
                                   maxProfile->maxContours );

    face->max_components = (FT_ULong)maxProfile->maxComponentElements +
                           maxProfile->maxComponentDepth;

    /* XXX: some fonts have maxComponents set to 0; we will */
    /*      then use 16 of them by default.                 */
    if ( face->max_components == 0 )
      face->max_components = 16;

    /* We also increase maxPoints and maxContours in order to support */
    /* some broken fonts.                                             */
    face->root.max_points   += 8;
    face->root.max_contours += 4;

    FT_TRACE2(( "MAXP loaded.\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Metrics                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal or vertical metrics table into a face object. */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*    stream   :: The input stream.                                      */
  /*    vertical :: A boolean flag.  If set, load vertical metrics.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  TT_Load_Metrics( TT_Face    face,
                             FT_Stream  stream,
                             FT_Bool    vertical )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_ULong   table_len;
    FT_Long    num_shorts, num_longs, num_shorts_checked;

    TT_LongMetrics**   longs;
    TT_ShortMetrics**  shorts;


    FT_TRACE2(( "TT_Load_%s_Metrics: %08p\n", vertical ? "Vertical"
                                                       : "Horizontal",
                                              face ));

    if ( vertical )
    {
      /* The table is optional, quit silently if it wasn't found       */
      /* XXX: Some fonts have a valid vertical header with a non-null  */
      /*      `number_of_VMetrics' fields, but no corresponding `vmtx' */
      /*      table to get the metrics from (e.g. mingliu).            */
      /*                                                               */
      /*      For safety, we set the field to 0!                       */
      /*                                                               */
      error = face->goto_table( face, TTAG_vmtx, stream, &table_len );
      if ( error )
      {
        /* Set number_Of_VMetrics to 0! */
        FT_TRACE2(( "  no vertical header in file.\n" ));
        face->vertical.number_Of_VMetrics = 0;
        error = TT_Err_Ok;
        goto Exit;
      }

      num_longs = face->vertical.number_Of_VMetrics;
      longs     = (TT_LongMetrics**)&face->vertical.long_metrics;
      shorts    = (TT_ShortMetrics**)&face->vertical.short_metrics;
    }
    else
    {
      error = face->goto_table( face, TTAG_hmtx, stream, &table_len );
      if ( error )
      {
        FT_ERROR(( " no horizontal metrics in file!\n" ));
        error = TT_Err_Hmtx_Table_Missing;
        goto Exit;
      }

      num_longs = face->horizontal.number_Of_HMetrics;
      longs     = (TT_LongMetrics**)&face->horizontal.long_metrics;
      shorts    = (TT_ShortMetrics**)&face->horizontal.short_metrics;
    }

    /* never trust derived values */

    num_shorts         = face->max_profile.numGlyphs - num_longs;
    num_shorts_checked = ( table_len - num_longs * 4L ) / 2;

    if ( num_shorts < 0 )
    {
      FT_ERROR(( "TT_Load_%s_Metrics: more metrics than glyphs!\n",
                 vertical ? "Vertical"
                          : "Horizontal" ));

      error = vertical ? TT_Err_Invalid_Vert_Metrics
                       : TT_Err_Invalid_Horiz_Metrics;
      goto Exit;
    }

    if ( ALLOC_ARRAY( *longs,  num_longs,  TT_LongMetrics )  ||
         ALLOC_ARRAY( *shorts, num_shorts, TT_ShortMetrics ) )
      goto Exit;

    if ( ACCESS_Frame( table_len ) )
      goto Exit;

    {
      TT_LongMetrics*  cur   = *longs;
      TT_LongMetrics*  limit = cur + num_longs;


      for ( ; cur < limit; cur++ )
      {
        cur->advance = GET_UShort();
        cur->bearing = GET_Short();
      }
    }

    /* do we have an inconsistent number of metric values? */
    {
      TT_ShortMetrics*  cur   = *shorts;
      TT_ShortMetrics*  limit = cur + MIN( num_shorts, num_shorts_checked );


      for ( ; cur < limit; cur++ )
        *cur = GET_Short();

      /* we fill up the missing left side bearings with the     */
      /* last valid value.  Since this will occur for buggy CJK */
      /* fonts usually only, nothing serious will happen        */
      if ( num_shorts > num_shorts_checked && num_shorts_checked > 0 )
      {
        FT_Short  val = *(shorts)[num_shorts_checked - 1];


        limit = *shorts + num_shorts;
        for ( ; cur < limit; cur++ )
          *cur = val;
      }
    }

    FORGET_Frame();

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Metrics_Header                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal or vertical header in a face object.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*    stream   :: The input stream.                                      */
  /*    vertical :: A boolean flag.  If set, load vertical metrics.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Metrics_Header( TT_Face    face,
                                    FT_Stream  stream,
                                    FT_Bool    vertical )
  {
    FT_Error        error;
    TT_HoriHeader*  header;

    const FT_Frame_Field  metrics_header_fields[] =
    {
      FT_FRAME_START( 36 ),
        FT_FRAME_ULONG(  TT_HoriHeader, Version ),
        FT_FRAME_SHORT(  TT_HoriHeader, Ascender ),
        FT_FRAME_SHORT(  TT_HoriHeader, Descender ),
        FT_FRAME_SHORT(  TT_HoriHeader, Line_Gap ),
        FT_FRAME_USHORT( TT_HoriHeader, advance_Width_Max ),
        FT_FRAME_SHORT(  TT_HoriHeader, min_Left_Side_Bearing ),
        FT_FRAME_SHORT(  TT_HoriHeader, min_Right_Side_Bearing ),
        FT_FRAME_SHORT(  TT_HoriHeader, xMax_Extent ),
        FT_FRAME_SHORT(  TT_HoriHeader, caret_Slope_Rise ),
        FT_FRAME_SHORT(  TT_HoriHeader, caret_Slope_Run ),
        FT_FRAME_SHORT(  TT_HoriHeader, Reserved[0] ),
        FT_FRAME_SHORT(  TT_HoriHeader, Reserved[1] ),
        FT_FRAME_SHORT(  TT_HoriHeader, Reserved[2] ),
        FT_FRAME_SHORT(  TT_HoriHeader, Reserved[3] ),
        FT_FRAME_SHORT(  TT_HoriHeader, Reserved[4] ),
        FT_FRAME_SHORT(  TT_HoriHeader, metric_Data_Format ),
        FT_FRAME_USHORT( TT_HoriHeader, number_Of_HMetrics ),
      FT_FRAME_END
    };


    FT_TRACE2(( vertical ? "Vertical header " : "Horizontal header " ));

    if ( vertical )
    {
      face->vertical_info = 0;

      /* The vertical header table is optional, so return quietly if */
      /* we don't find it.                                           */
      error = face->goto_table( face, TTAG_vhea, stream, 0 );
      if ( error )
      {
        error = TT_Err_Ok;
        goto Exit;
      }

      face->vertical_info = 1;
      header = (TT_HoriHeader*)&face->vertical;
    }
    else
    {
      /* The horizontal header is mandatory; return an error if we */
      /* don't find it.                                            */
      error = face->goto_table( face, TTAG_hhea, stream, 0 );
      if ( error )
      {
        error = TT_Err_Horiz_Header_Missing;
        goto Exit;
      }

      header = &face->horizontal;
    }

    if ( READ_Fields( metrics_header_fields, header ) )
      goto Exit;

    header->long_metrics  = NULL;
    header->short_metrics = NULL;

    FT_TRACE2(( "loaded\n" ));

    /* Now try to load the corresponding metrics */

    error = TT_Load_Metrics( face, stream, vertical );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Names                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the name records.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Names( TT_Face    face,
                           FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_ULong   table_pos, table_len;
    FT_ULong   storageSize;

    TT_NameTable*  names;

    const FT_Frame_Field  name_table_fields[] =
    {
      FT_FRAME_START( 6 ),
        FT_FRAME_USHORT( TT_NameTable, format ),
        FT_FRAME_USHORT( TT_NameTable, numNameRecords ),
        FT_FRAME_USHORT( TT_NameTable, storageOffset ),
      FT_FRAME_END
    };

    const FT_Frame_Field  name_record_fields[] =
    {
      /* no FT_FRAME_START */
        FT_FRAME_USHORT( TT_NameRec, platformID ),
        FT_FRAME_USHORT( TT_NameRec, encodingID ),
        FT_FRAME_USHORT( TT_NameRec, languageID ),
        FT_FRAME_USHORT( TT_NameRec, nameID ),
        FT_FRAME_USHORT( TT_NameRec, stringLength ),
        FT_FRAME_USHORT( TT_NameRec, stringOffset ),
      FT_FRAME_END
    };


    FT_TRACE2(( "Names " ));

    error = face->goto_table( face, TTAG_name, stream, &table_len );
    if ( error )
    {
      /* The name table is required so indicate failure. */
      FT_TRACE2(( "is missing!\n" ));
      error = TT_Err_Name_Table_Missing;
      goto Exit;
    }

    table_pos = FILE_Pos();

    names = &face->name_table;

    if ( READ_Fields( name_table_fields, names ) )
      goto Exit;

    /* Allocate the array of name records. */
    if ( ALLOC_ARRAY( names->names,
                      names->numNameRecords,
                      TT_NameRec )                   ||
         ACCESS_Frame( names->numNameRecords * 12L ) )
      goto Exit;

    /* Load the name records and determine how much storage is needed */
    /* to hold the strings themselves.                                */
    {
      TT_NameRec*  cur   = names->names;
      TT_NameRec*  limit = cur + names->numNameRecords;


      storageSize = 0;

      for ( ; cur < limit; cur ++ )
      {
        FT_ULong  upper;


        (void)READ_Fields( name_record_fields, cur );

        upper = (FT_ULong)( cur->stringOffset + cur->stringLength );
        if ( upper > storageSize )
          storageSize = upper;
      }
    }

    FORGET_Frame();

    if ( storageSize > 0 )
    {
      /* allocate the name storage area in memory, then read it */
      if ( ALLOC( names->storage, storageSize )               ||
           FILE_Read_At( table_pos + names->storageOffset,
                         names->storage, storageSize ) )
        goto Exit;

      /* Go through and assign the string pointers to the name records. */
      {
        TT_NameRec*  cur   = names->names;
        TT_NameRec*  limit = cur + names->numNameRecords;


        for ( ; cur < limit; cur++ )
          cur->string = names->storage + cur->stringOffset;
      }

#ifdef FT_DEBUG_LEVEL_TRACE

      /* Print Name Record Table in case of debugging */
      {
        TT_NameRec*  cur   = names->names;
        TT_NameRec*  limit = cur + names->numNameRecords;


        for ( ; cur < limit; cur++ )
        {
          FT_UInt  j;


          FT_TRACE3(( "%d %d %x %d\n  ",
                       cur->platformID,
                       cur->encodingID,
                       cur->languageID,
                       cur->nameID ));

          /* I know that M$ encoded strings are Unicode,            */
          /* but this works reasonable well for debugging purposes. */
          if ( cur->string )
            for ( j = 0; j < cur->stringLength; j++ )
            {
              FT_Char  c = *( cur->string + j );


              if ( (FT_Byte)c < 128 )
                FT_TRACE3(( "%c", c ));
            }
        }
      }
      FT_TRACE3(( "\n" ));

#endif /* FT_DEBUG_LEVEL_TRACE */

    }
    FT_TRACE2(( "loaded\n" ));

    /* everything went well, update face->num_names */
    face->num_names = names->numNameRecords;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Free_Names                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees the name records.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  LOCAL_FUNC
  void  TT_Free_Names( TT_Face  face )
  {
    FT_Memory      memory = face->root.driver->root.memory;
    TT_NameTable*  names  = &face->name_table;


    /* free strings table */
    FREE( names->names );

    /* free strings storage */
    FREE( names->storage );

    names->numNameRecords = 0;
    names->format         = 0;
    names->storageOffset  = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_CMap                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the cmap directory in a face object.  The cmaps itselves are */
  /*    loaded on demand in the `ttcmap.c' module.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_CMap( TT_Face    face,
                          FT_Stream  stream )
  {
    FT_Error    error;
    FT_Memory   memory = stream->memory;
    FT_Long     table_start;
    TT_CMapDir  cmap_dir;

    const FT_Frame_Field  cmap_fields[] =
    {
      FT_FRAME_START( 4 ),
        FT_FRAME_USHORT( TT_CMapDir, tableVersionNumber ),
        FT_FRAME_USHORT( TT_CMapDir, numCMaps ),
      FT_FRAME_END
    };

    const FT_Frame_Field  cmap_rec_fields[] =
    {
      FT_FRAME_START( 6 ),
        FT_FRAME_USHORT( TT_CMapTable, format ),
        FT_FRAME_USHORT( TT_CMapTable, length ),
        FT_FRAME_USHORT( TT_CMapTable, version ),
      FT_FRAME_END
    };


    FT_TRACE2(( "CMaps " ));

    error = face->goto_table( face, TTAG_cmap, stream, 0 );
    if ( error )
    {
      error = TT_Err_CMap_Table_Missing;
      goto Exit;
    }

    table_start = FILE_Pos();

    if ( READ_Fields( cmap_fields, &cmap_dir ) )
      goto Exit;

    /* reserve space in face table for cmap tables */
    if ( ALLOC_ARRAY( face->charmaps,
                      cmap_dir.numCMaps,
                      TT_CharMapRec ) )
      goto Exit;

    face->num_charmaps = cmap_dir.numCMaps;
    {
      TT_CharMap  charmap = face->charmaps;
      TT_CharMap  limit   = charmap + face->num_charmaps;


      /* read the header of each charmap first */
      if ( ACCESS_Frame( face->num_charmaps * 8L ) )
        goto Exit;

      for ( ; charmap < limit; charmap++ )
      {
        TT_CMapTable*  cmap;


        charmap->root.face = (FT_Face)face;
        cmap               = &charmap->cmap;

        cmap->loaded             = FALSE;
        cmap->platformID         = GET_UShort();
        cmap->platformEncodingID = GET_UShort();
        cmap->offset             = (FT_ULong)GET_Long();
      }

      FORGET_Frame();

      /* now read the rest of each table */
      for ( charmap = face->charmaps; charmap < limit; charmap++ )
      {
        TT_CMapTable* cmap = &charmap->cmap;


        if ( FILE_Seek( table_start + (FT_Long)cmap->offset ) ||
             READ_Fields( cmap_rec_fields, cmap )             )
          goto Exit;

        cmap->offset = FILE_Pos();
      }
    }

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_OS2                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the OS2 table.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_OS2( TT_Face    face,
                         FT_Stream  stream )
  {
    FT_Error  error;
    TT_OS2*   os2;

    const FT_Frame_Field  os2_fields[] =
    {
      FT_FRAME_START( 78 ),
        FT_FRAME_USHORT( TT_OS2, version ),
        FT_FRAME_SHORT(  TT_OS2, xAvgCharWidth ),
        FT_FRAME_USHORT( TT_OS2, usWeightClass ),
        FT_FRAME_USHORT( TT_OS2, usWidthClass ),
        FT_FRAME_SHORT(  TT_OS2, fsType ),
        FT_FRAME_SHORT(  TT_OS2, ySubscriptXSize ),
        FT_FRAME_SHORT(  TT_OS2, ySubscriptYSize ),
        FT_FRAME_SHORT(  TT_OS2, ySubscriptXOffset ),
        FT_FRAME_SHORT(  TT_OS2, ySubscriptYOffset ),
        FT_FRAME_SHORT(  TT_OS2, ySuperscriptXSize ),
        FT_FRAME_SHORT(  TT_OS2, ySuperscriptYSize ),
        FT_FRAME_SHORT(  TT_OS2, ySuperscriptXOffset ),
        FT_FRAME_SHORT(  TT_OS2, ySuperscriptYOffset ),
        FT_FRAME_SHORT(  TT_OS2, yStrikeoutSize ),
        FT_FRAME_SHORT(  TT_OS2, yStrikeoutPosition ),
        FT_FRAME_SHORT(  TT_OS2, sFamilyClass ),
        FT_FRAME_BYTE(   TT_OS2, panose[0] ),
        FT_FRAME_BYTE(   TT_OS2, panose[1] ),
        FT_FRAME_BYTE(   TT_OS2, panose[2] ),
        FT_FRAME_BYTE(   TT_OS2, panose[3] ),
        FT_FRAME_BYTE(   TT_OS2, panose[4] ),
        FT_FRAME_BYTE(   TT_OS2, panose[5] ),
        FT_FRAME_BYTE(   TT_OS2, panose[6] ),
        FT_FRAME_BYTE(   TT_OS2, panose[7] ),
        FT_FRAME_BYTE(   TT_OS2, panose[8] ),
        FT_FRAME_BYTE(   TT_OS2, panose[9] ),
        FT_FRAME_ULONG(  TT_OS2, ulUnicodeRange1 ),
        FT_FRAME_ULONG(  TT_OS2, ulUnicodeRange2 ),
        FT_FRAME_ULONG(  TT_OS2, ulUnicodeRange3 ),
        FT_FRAME_ULONG(  TT_OS2, ulUnicodeRange4 ),
        FT_FRAME_BYTE(   TT_OS2, achVendID[0] ),
        FT_FRAME_BYTE(   TT_OS2, achVendID[1] ),
        FT_FRAME_BYTE(   TT_OS2, achVendID[2] ),
        FT_FRAME_BYTE(   TT_OS2, achVendID[3] ),

        FT_FRAME_USHORT( TT_OS2, fsSelection ),
        FT_FRAME_USHORT( TT_OS2, usFirstCharIndex ),
        FT_FRAME_USHORT( TT_OS2, usLastCharIndex ),
        FT_FRAME_SHORT(  TT_OS2, sTypoAscender ),
        FT_FRAME_SHORT(  TT_OS2, sTypoDescender ),
        FT_FRAME_SHORT(  TT_OS2, sTypoLineGap ),
        FT_FRAME_USHORT( TT_OS2, usWinAscent ),
        FT_FRAME_USHORT( TT_OS2, usWinDescent ),
      FT_FRAME_END
    };

    const FT_Frame_Field  os2_fields_extra[] =
    {
      FT_FRAME_START( 8 ),
        FT_FRAME_ULONG( TT_OS2, ulCodePageRange1 ),
        FT_FRAME_ULONG( TT_OS2, ulCodePageRange2 ),
      FT_FRAME_END
    };

    const FT_Frame_Field  os2_fields_extra2[] =
    {
      FT_FRAME_START( 10 ),
        FT_FRAME_SHORT( TT_OS2,  sxHeight ),
        FT_FRAME_SHORT( TT_OS2,  sCapHeight ),
        FT_FRAME_USHORT( TT_OS2, usDefaultChar ),
        FT_FRAME_USHORT( TT_OS2, usBreakChar ),
        FT_FRAME_USHORT( TT_OS2, usMaxContext ),
      FT_FRAME_END
    };


    FT_TRACE2(( "OS/2 Table " ));

    /* We now support old Mac fonts where the OS/2 table doesn't  */
    /* exist.  Simply put, we set the `version' field to 0xFFFF   */
    /* and test this value each time we need to access the table. */
    error = face->goto_table( face, TTAG_OS2, stream, 0 );
    if ( error )
    {
      FT_TRACE2(( "is missing!\n" ));
      face->os2.version = 0xFFFF;
      error = TT_Err_Ok;
      goto Exit;
    }

    os2 = &face->os2;

    if ( READ_Fields( os2_fields, os2 ) )
      goto Exit;

    os2->ulCodePageRange1 = 0;
    os2->ulCodePageRange2 = 0;

    if ( os2->version >= 0x0001 )
    {
      /* only version 1 tables */
      if ( READ_Fields( os2_fields_extra, os2 ) )
        goto Exit;

      if ( os2->version >= 0x0002 )
      {
        /* only version 2 tables */
        if ( READ_Fields( os2_fields_extra2, os2 ) )
          goto Exit;
      }
    }

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Postscript                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the Postscript table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_PostScript( TT_Face    face,
                                FT_Stream  stream )
  {
    FT_Error        error;
    TT_Postscript*  post = &face->postscript;

    static const FT_Frame_Field  post_fields[] =
    {
      FT_FRAME_START( 32 ),
        FT_FRAME_ULONG( TT_Postscript, FormatType ),
        FT_FRAME_ULONG( TT_Postscript, italicAngle ),
        FT_FRAME_SHORT( TT_Postscript, underlinePosition ),
        FT_FRAME_SHORT( TT_Postscript, underlineThickness ),
        FT_FRAME_ULONG( TT_Postscript, isFixedPitch ),
        FT_FRAME_ULONG( TT_Postscript, minMemType42 ),
        FT_FRAME_ULONG( TT_Postscript, maxMemType42 ),
        FT_FRAME_ULONG( TT_Postscript, minMemType1 ),
        FT_FRAME_ULONG( TT_Postscript, maxMemType1 ),
      FT_FRAME_END
    };


    FT_TRACE2(( "PostScript " ));

    error = face->goto_table( face, TTAG_post, stream, 0 );
    if ( error )
      return TT_Err_Post_Table_Missing;

    if ( READ_Fields( post_fields, post ) )
      return error;

    /* we don't load the glyph names, we do that in another */
    /* module (ttpost).                                     */
    FT_TRACE2(( "loaded\n" ));

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_PCLT                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the PCL 5 Table.                                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_PCLT( TT_Face    face,
                          FT_Stream  stream )
  {
    static const FT_Frame_Field  pclt_fields[] =
    {
      FT_FRAME_START( 54 ),
        FT_FRAME_ULONG ( TT_PCLT, Version ),
        FT_FRAME_ULONG ( TT_PCLT, FontNumber ),
        FT_FRAME_USHORT( TT_PCLT, Pitch ),
        FT_FRAME_USHORT( TT_PCLT, xHeight ),
        FT_FRAME_USHORT( TT_PCLT, Style ),
        FT_FRAME_USHORT( TT_PCLT, TypeFamily ),
        FT_FRAME_USHORT( TT_PCLT, CapHeight ),
        FT_FRAME_BYTES ( TT_PCLT, TypeFace, 16 ),
        FT_FRAME_BYTES ( TT_PCLT, CharacterComplement, 8 ),
        FT_FRAME_BYTES ( TT_PCLT, FileName, 6 ),
        FT_FRAME_CHAR  ( TT_PCLT, StrokeWeight ),
        FT_FRAME_CHAR  ( TT_PCLT, WidthType ),
        FT_FRAME_BYTE  ( TT_PCLT, SerifStyle ),
        FT_FRAME_BYTE  ( TT_PCLT, Reserved ),
      FT_FRAME_END
    };

    FT_Error  error;
    TT_PCLT*  pclt = &face->pclt;


    FT_TRACE2(( "PCLT " ));

    /* optional table */
    error = face->goto_table( face, TTAG_PCLT, stream, 0 );
    if ( error )
    {
      FT_TRACE2(( "missing (optional)\n" ));
      pclt->Version = 0;
      return TT_Err_Ok;
    }

    if ( READ_Fields( pclt_fields, pclt ) )
      goto Exit;

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Gasp                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the `gasp' table into a face object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Gasp( TT_Face    face,
                          FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt        j,num_ranges;
    TT_GaspRange*  gaspranges;


    FT_TRACE2(( "TT_Load_Gasp: %08p\n", face ));

    /* the gasp table is optional */
    error = face->goto_table( face, TTAG_gasp, stream, 0 );
    if ( error )
      return TT_Err_Ok;

    if ( ACCESS_Frame( 4L ) )
      goto Exit;

    face->gasp.version   = GET_UShort();
    face->gasp.numRanges = GET_UShort();

    FORGET_Frame();

    num_ranges = face->gasp.numRanges;
    FT_TRACE3(( "number of ranges = %d\n", num_ranges ));

    if ( ALLOC_ARRAY( gaspranges, num_ranges, TT_GaspRange ) ||
         ACCESS_Frame( num_ranges * 4L )                     )
      goto Exit;

    face->gasp.gaspRanges = gaspranges;

    for ( j = 0; j < num_ranges; j++ )
    {
      gaspranges[j].maxPPEM  = GET_UShort();
      gaspranges[j].gaspFlag = GET_UShort();

      FT_TRACE3(( " [max:%d flag:%d]",
                    gaspranges[j].maxPPEM,
                    gaspranges[j].gaspFlag ));
    }
    FT_TRACE3(( "\n" ));

    FORGET_Frame();
    FT_TRACE2(( "GASP loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Kern                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the first kerning table with format 0 in the font.  Only     */
  /*    accepts the first horizontal kerning table.  Developers should use */
  /*    the `ftxkern' extension to access other kerning tables in the font */
  /*    file, if they really want to.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Kern( TT_Face    face,
                          FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt    n, num_tables, version;


    /* the kern table is optional; exit silently if it is missing */
    error = face->goto_table( face, TTAG_kern, stream, 0 );
    if ( error )
      return TT_Err_Ok;

    if ( ACCESS_Frame( 4L ) )
      goto Exit;

    version    = GET_UShort();
    num_tables = GET_UShort();

    FORGET_Frame();

    for ( n = 0; n < num_tables; n++ )
    {
      FT_UInt  coverage;
      FT_UInt  length;


      if ( ACCESS_Frame( 6L ) )
        goto Exit;

      version  = GET_UShort();      /* version                 */
      length   = GET_UShort() - 6;  /* substract header length */
      coverage = GET_UShort();

      FORGET_Frame();

      if ( coverage == 0x0001 )
      {
        FT_UInt          num_pairs;
        TT_Kern_0_Pair*  pair;
        TT_Kern_0_Pair*  limit;


        /* found a horizontal format 0 kerning table! */
        if ( ACCESS_Frame( 8L ) )
          goto Exit;

        num_pairs = GET_UShort();

        /* skip the rest */

        FORGET_Frame();

        /* allocate array of kerning pairs */
        if ( ALLOC_ARRAY( face->kern_pairs, num_pairs, TT_Kern_0_Pair ) ||
             ACCESS_Frame( 6L * num_pairs )                             )
          goto Exit;

        pair  = face->kern_pairs;
        limit = pair + num_pairs;
        for ( ; pair < limit; pair++ )
        {
          pair->left  = GET_UShort();
          pair->right = GET_UShort();
          pair->value = GET_UShort();
        }

        FORGET_Frame();

        face->num_kern_pairs   = num_pairs;
        face->kern_table_index = n;
        goto Exit;
      }

      if ( FILE_Skip( length ) )
        goto Exit;
    }

    /* no kern table found -- doesn't matter */
    face->kern_table_index = -1;
    face->num_kern_pairs   = 0;
    face->kern_pairs       = NULL;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Hdmx                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal device metrics table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  TT_Load_Hdmx( TT_Face    face,
                          FT_Stream  stream )
  {
    FT_Error  error;
    FT_Memory memory = stream->memory;

    TT_Hdmx*  hdmx = &face->hdmx;
    FT_Long   num_glyphs;
    FT_Long   record_size;


    hdmx->version     = 0;
    hdmx->num_records = 0;
    hdmx->records     = 0;

    /* this table is optional */
    error = face->goto_table( face, TTAG_hdmx, stream, 0 );
    if ( error )
      return TT_Err_Ok;

    if ( ACCESS_Frame( 8L ) )
      goto Exit;

    hdmx->version     = GET_UShort();
    hdmx->num_records = GET_Short();
    record_size       = GET_Long();

    FORGET_Frame();

    /* Only recognize format 0 */
    if ( hdmx->version != 0 )
      goto Exit;

    if ( ALLOC_ARRAY( hdmx->records, hdmx->num_records, TT_HdmxRec ) )
      goto Exit;

    num_glyphs   = face->root.num_glyphs;
    record_size -= num_glyphs + 2;

    {
      TT_HdmxRec*  cur   = hdmx->records;
      TT_HdmxRec*  limit = cur + hdmx->num_records;


      for ( ; cur < limit; cur++ )
      {
        /* read record */
        if ( READ_Byte( cur->ppem      ) ||
             READ_Byte( cur->max_width ) )
          goto Exit;

        if ( ALLOC( cur->widths, num_glyphs )     ||
             FILE_Read( cur->widths, num_glyphs ) )
          goto Exit;

        /* skip padding bytes */
        if ( record_size > 0 && FILE_Skip( record_size ) )
            goto Exit;
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Free_Hdmx                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees the horizontal device metrics table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  LOCAL_FUNC
  void  TT_Free_Hdmx( TT_Face  face )
  {
    if ( face )
    {
      FT_Int     n;
      FT_Memory  memory = face->root.driver->root.memory;


      for ( n = 0; n < face->hdmx.num_records; n++ )
        FREE( face->hdmx.records[n].widths );

      FREE( face->hdmx.records );
      face->hdmx.num_records = 0;
    }
  }


/* END */
