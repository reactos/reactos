/***************************************************************************/
/*                                                                         */
/*  ttload.c                                                               */
/*                                                                         */
/*    Load the basic TrueType tables, i.e., tables that can be either in   */
/*    TTF or OTF fonts (body).                                             */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004 by                               */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttload.h"

#include "sferrors.h"


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
  /*    tt_face_lookup_table                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks for a TrueType table by name.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A face object handle.                                      */
  /*                                                                       */
  /*    tag  :: The searched tag.                                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A pointer to the table directory entry.  0 if not found.           */
  /*                                                                       */
  FT_LOCAL_DEF( TT_Table  )
  tt_face_lookup_table( TT_Face   face,
                        FT_ULong  tag  )
  {
    TT_Table  entry;
    TT_Table  limit;


    FT_TRACE3(( "tt_face_lookup_table: %08p, `%c%c%c%c' -- ",
                face,
                (FT_Char)( tag >> 24 ),
                (FT_Char)( tag >> 16 ),
                (FT_Char)( tag >> 8  ),
                (FT_Char)( tag       ) ));

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {
      /* For compatibility with Windows, we consider 0-length */
      /* tables the same as missing tables.                   */
      if ( entry->Tag == tag && entry->Length != 0 )
      {
        FT_TRACE3(( "found table.\n" ));
        return entry;
      }
    }

    FT_TRACE3(( "could not find table!\n" ));
    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_goto_table                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks for a TrueType table by name, then seek a stream to it.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A face object handle.                                    */
  /*                                                                       */
  /*    tag    :: The searched tag.                                        */
  /*                                                                       */
  /*    stream :: The stream to seek when the table is found.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    length :: The length of the table if found, undefined otherwise.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_goto_table( TT_Face    face,
                      FT_ULong   tag,
                      FT_Stream  stream,
                      FT_ULong*  length )
  {
    TT_Table  table;
    FT_Error  error;


    table = tt_face_lookup_table( face, tag );
    if ( table )
    {
      if ( length )
        *length = table->Length;

      if ( FT_STREAM_SEEK( table->Offset ) )
       goto Exit;
    }
    else
      error = SFNT_Err_Table_Missing;

  Exit:
    return error;
  }


 /* In theory, we should check the values of `search_range',               */
 /* `entry_selector', and `range_shift' to detect non-SFNT based files     */
 /* whose header might also start with 0x100000L (yes, these exist).       */
 /*                                                                        */
 /* Very unfortunately, many TrueType fonts don't have these fields        */
 /* set correctly and we must ignore them to support them.  An alternative */
 /* way to check the font file is thus to:                                 */
 /*                                                                        */
 /* - check that `num_tables' is valid                                     */
 /* - look for a "head" table, check its size, and parse it to             */
 /*   see if its "magic" field is correctly set                            */
 /*                                                                        */
 /* When checking directory entries, ignore the tables `glyx' and `locx'   */
 /* which are hacked-out versions of `glyf' and `loca' in some PostScript  */
 /* Type 42 fonts, and will generally be invalid.                          */
 /*                                                                        */
  static FT_Error
  sfnt_dir_check( FT_Stream  stream,
                  FT_ULong   offset,
                  FT_UInt    num_tables )
   {
    FT_Error        error;
    FT_UInt         nn, has_head = 0;

    const FT_ULong  glyx_tag = FT_MAKE_TAG( 'g', 'l', 'y', 'x' );
    const FT_ULong  locx_tag = FT_MAKE_TAG( 'l', 'o', 'c', 'x' );

    static const FT_Frame_Field  sfnt_dir_entry_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_TableRec

      FT_FRAME_START( 16 ),
        FT_FRAME_ULONG( Tag ),
        FT_FRAME_ULONG( CheckSum ),
        FT_FRAME_ULONG( Offset ),
        FT_FRAME_ULONG( Length ),
      FT_FRAME_END
    };


    /* if 'num_tables' is 0, read the table count from the file */
    if ( num_tables == 0 )
    {
      if ( FT_STREAM_SEEK( offset )     ||
           FT_STREAM_SKIP( 4 )          ||
           FT_READ_USHORT( num_tables ) ||
           FT_STREAM_SKIP( 6 )          )
        goto Bad_Format;

      if ( offset + 12 + num_tables*16 > stream->size )
        goto Bad_Format;
    }
    else if ( FT_STREAM_SEEK( offset + 12 ) )
      goto Bad_Format;

    for ( nn = 0; nn < num_tables; nn++ )
    {
      TT_TableRec  table;


      if ( FT_STREAM_READ_FIELDS( sfnt_dir_entry_fields, &table ) )
        goto Bad_Format;

      if ( table.Offset + table.Length > stream->size     &&
           table.Tag != glyx_tag && table.Tag != locx_tag )
        goto Bad_Format;

      if ( table.Tag == TTAG_head )
      {
        FT_UInt32  magic;


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
      head_retry:
#endif  /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

        has_head = 1;

        /* The table length should be 0x36, but certain font tools
         * make it 0x38, so we will just check that it is greater.
         *
         * Note that according to the specification,
         * the table must be padded to 32-bit lengths, but this doesn't
         * apply to the value of its "Length" field!
         */
        if ( table.Length < 0x36                 ||
             FT_STREAM_SEEK( table.Offset + 12 ) ||
             FT_READ_ULONG( magic )              ||
             magic != 0x5F0F3CF5UL               )
          goto Bad_Format;

        if ( FT_STREAM_SEEK( offset + 28 + 16*nn ) )
          goto Bad_Format;
      }
#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
      else if ( table.Tag == TTAG_bhed )
        goto head_retry;
#endif  /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */      
    }

    if ( has_head == 0 )
      goto Bad_Format;

  Exit:
    return  error;

  Bad_Format:
    error = SFNT_Err_Unknown_File_Format;
    goto Exit;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_sfnt_header                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the header of a SFNT font file.  Supports collections.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face_index :: If the font is a collection, the number of the font  */
  /*                  in the collection.  Must be zero otherwise.          */
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
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_sfnt_header( TT_Face      face,
                            FT_Stream    stream,
                            FT_Long      face_index,
                            SFNT_Header  sfnt )
  {
    FT_Error   error;
    FT_ULong   font_format_tag, format_tag, offset;
    FT_Memory  memory = stream->memory;

    static const FT_Frame_Field  sfnt_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  SFNT_HeaderRec

      FT_FRAME_START( 8 ),
        FT_FRAME_USHORT( num_tables ),
        FT_FRAME_USHORT( search_range ),
        FT_FRAME_USHORT( entry_selector ),
        FT_FRAME_USHORT( range_shift ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  ttc_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TTC_HeaderRec

      FT_FRAME_START( 8 ),
        FT_FRAME_LONG( version ),
        FT_FRAME_LONG( count   ),
      FT_FRAME_END
    };


    FT_TRACE2(( "tt_face_load_sfnt_header: %08p, %ld\n",
                face, face_index ));

    face->ttc_header.tag     = 0;
    face->ttc_header.version = 0;
    face->ttc_header.count   = 0;

    face->num_tables = 0;

    /* First of all, read the first 4 bytes.  If it is `ttcf', then the   */
    /* file is a TrueType collection, otherwise it is a single-face font. */
    /*                                                                    */
    offset = FT_STREAM_POS();

    if ( FT_READ_ULONG( font_format_tag ) )
      goto Exit;

    format_tag = font_format_tag;

    if ( font_format_tag == TTAG_ttcf )
    {
      FT_Int  n;


      FT_TRACE3(( "tt_face_load_sfnt_header: file is a collection\n" ));

      /* It is a TrueType collection, i.e. a file containing several */
      /* font files.  Read the font directory now                    */
      if ( FT_STREAM_READ_FIELDS( ttc_header_fields, &face->ttc_header ) )
        goto Exit;

      /* now read the offsets of each font in the file */
      if ( FT_NEW_ARRAY( face->ttc_header.offsets, face->ttc_header.count ) ||
           FT_FRAME_ENTER( face->ttc_header.count * 4L )                    )
        goto Exit;

      for ( n = 0; n < face->ttc_header.count; n++ )
        face->ttc_header.offsets[n] = FT_GET_ULONG();

      FT_FRAME_EXIT();

      /* check face index */
      if ( face_index >= face->ttc_header.count )
      {
        error = SFNT_Err_Bad_Argument;
        goto Exit;
      }

      /* seek to the appropriate TrueType file, then read tag */
      offset = face->ttc_header.offsets[face_index];

      if ( FT_STREAM_SEEK( offset )   ||
           FT_READ_LONG( format_tag ) )
        goto Exit;
    }

    /* the format tag was read, now check the rest of the header */
    sfnt->format_tag = format_tag;
    sfnt->offset     = offset;

    if ( FT_STREAM_READ_FIELDS( sfnt_header_fields, sfnt ) )
      goto Exit;

    /* now check the sfnt directory */
    error = sfnt_dir_check( stream, offset, sfnt->num_tables );
    if ( error )
    {
      FT_TRACE2(( "tt_face_load_sfnt_header: file is not SFNT!\n" ));
      error = SFNT_Err_Unknown_File_Format;
      goto Exit;
    }

    /* disallow face index values > 0 for non-TTC files */
    if ( font_format_tag != TTAG_ttcf && face_index > 0 )
      error = SFNT_Err_Bad_Argument;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_directory                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the table directory into a face object.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /*    sfnt   :: The SFNT directory header.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_directory( TT_Face      face,
                          FT_Stream    stream,
                          SFNT_Header  sfnt )
  {
    FT_Error     error;
    FT_Memory    memory = stream->memory;

    TT_TableRec  *entry, *limit;


    FT_TRACE2(( "tt_face_load_directory: %08p\n", face ));

    FT_TRACE2(( "-- Tables count:   %12u\n",  sfnt->num_tables ));
    FT_TRACE2(( "-- Format version: %08lx\n", sfnt->format_tag ));

    face->num_tables = sfnt->num_tables;

    if ( FT_QNEW_ARRAY( face->dir_tables, face->num_tables ) )
      goto Exit;

    if ( FT_STREAM_SEEK( sfnt->offset + 12 )      ||
         FT_FRAME_ENTER( face->num_tables * 16L ) )
      goto Exit;

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {                    /* loop through the tables and get all entries */
      entry->Tag      = FT_GET_TAG4();
      entry->CheckSum = FT_GET_ULONG();
      entry->Offset   = FT_GET_LONG();
      entry->Length   = FT_GET_LONG();

      FT_TRACE2(( "  %c%c%c%c  -  %08lx  -  %08lx\n",
                  (FT_Char)( entry->Tag >> 24 ),
                  (FT_Char)( entry->Tag >> 16 ),
                  (FT_Char)( entry->Tag >> 8  ),
                  (FT_Char)( entry->Tag       ),
                  entry->Offset,
                  entry->Length ));
    }

    FT_FRAME_EXIT();

    FT_TRACE2(( "Directory loaded\n\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_any                                                   */
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
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_any( TT_Face    face,
                    FT_ULong   tag,
                    FT_Long    offset,
                    FT_Byte*   buffer,
                    FT_ULong*  length )
  {
    FT_Error   error;
    FT_Stream  stream;
    TT_Table   table;
    FT_ULong   size;


    if ( tag != 0 )
    {
      /* look for tag in font directory */
      table = tt_face_lookup_table( face, tag );
      if ( !table )
      {
        error = SFNT_Err_Table_Missing;
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

      return SFNT_Err_Ok;
    }

    if ( length )
      size = *length;

    stream = face->root.stream;
    /* the `if' is syntactic sugar for picky compilers */
    if ( FT_STREAM_READ_AT( offset, buffer, size ) )
      goto Exit;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_generic_header                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the TrueType table `head' or `bhed'.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  tt_face_load_generic_header( TT_Face    face,
                               FT_Stream  stream,
                               FT_ULong   tag )
  {
    FT_Error    error;
    TT_Header*  header;

    static const FT_Frame_Field  header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_Header

      FT_FRAME_START( 54 ),
        FT_FRAME_ULONG ( Table_Version ),
        FT_FRAME_ULONG ( Font_Revision ),
        FT_FRAME_LONG  ( CheckSum_Adjust ),
        FT_FRAME_LONG  ( Magic_Number ),
        FT_FRAME_USHORT( Flags ),
        FT_FRAME_USHORT( Units_Per_EM ),
        FT_FRAME_LONG  ( Created[0] ),
        FT_FRAME_LONG  ( Created[1] ),
        FT_FRAME_LONG  ( Modified[0] ),
        FT_FRAME_LONG  ( Modified[1] ),
        FT_FRAME_SHORT ( xMin ),
        FT_FRAME_SHORT ( yMin ),
        FT_FRAME_SHORT ( xMax ),
        FT_FRAME_SHORT ( yMax ),
        FT_FRAME_USHORT( Mac_Style ),
        FT_FRAME_USHORT( Lowest_Rec_PPEM ),
        FT_FRAME_SHORT ( Font_Direction ),
        FT_FRAME_SHORT ( Index_To_Loc_Format ),
        FT_FRAME_SHORT ( Glyph_Data_Format ),
      FT_FRAME_END
    };


    FT_TRACE2(( "tt_face_load_generic_header: "
                "%08p, looking up font table `%c%c%c%c'.\n",
                face,
                (FT_Char)( tag >> 24 ),
                (FT_Char)( tag >> 16 ),
                (FT_Char)( tag >> 8  ),
                (FT_Char)( tag       ) ));

    error = face->goto_table( face, tag, stream, 0 );
    if ( error )
    {
      FT_TRACE2(( "tt_face_load_generic_header: Font table is missing!\n" ));
      goto Exit;
    }

    header = &face->header;

    if ( FT_STREAM_READ_FIELDS( header_fields, header ) )
      goto Exit;

    FT_TRACE2(( "    Units per EM: %8u\n", header->Units_Per_EM ));
    FT_TRACE2(( "    IndexToLoc:   %8d\n", header->Index_To_Loc_Format ));
    FT_TRACE2(( "tt_face_load_generic_header: Font table loaded.\n" ));

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_header( TT_Face    face,
                       FT_Stream  stream )
  {
    return tt_face_load_generic_header( face, stream, TTAG_head );
  }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_bitmap_header( TT_Face    face,
                              FT_Stream  stream )
  {
    return tt_face_load_generic_header( face, stream, TTAG_bhed );
  }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_max_profile                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the maximum profile into a face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_max_profile( TT_Face    face,
                            FT_Stream  stream )
  {
    FT_Error        error;
    TT_MaxProfile*  maxProfile = &face->max_profile;

    const FT_Frame_Field  maxp_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_MaxProfile

      FT_FRAME_START( 6 ),
        FT_FRAME_LONG  ( version ),
        FT_FRAME_USHORT( numGlyphs ),
      FT_FRAME_END
    };

    const FT_Frame_Field  maxp_fields_extra[] =
    {
      FT_FRAME_START( 26 ),
        FT_FRAME_USHORT( maxPoints ),
        FT_FRAME_USHORT( maxContours ),
        FT_FRAME_USHORT( maxCompositePoints ),
        FT_FRAME_USHORT( maxCompositeContours ),
        FT_FRAME_USHORT( maxZones ),
        FT_FRAME_USHORT( maxTwilightPoints ),
        FT_FRAME_USHORT( maxStorage ),
        FT_FRAME_USHORT( maxFunctionDefs ),
        FT_FRAME_USHORT( maxInstructionDefs ),
        FT_FRAME_USHORT( maxStackElements ),
        FT_FRAME_USHORT( maxSizeOfInstructions ),
        FT_FRAME_USHORT( maxComponentElements ),
        FT_FRAME_USHORT( maxComponentDepth ),
      FT_FRAME_END
    };


    FT_TRACE2(( "Load_TT_MaxProfile: %08p\n", face ));

    error = face->goto_table( face, TTAG_maxp, stream, 0 );
    if ( error )
      goto Exit;

    if ( FT_STREAM_READ_FIELDS( maxp_fields, maxProfile ) )
      goto Exit;

    face->root.num_glyphs = maxProfile->numGlyphs;

    maxProfile->maxPoints             = 0;
    maxProfile->maxContours           = 0;
    maxProfile->maxCompositePoints    = 0;
    maxProfile->maxCompositeContours  = 0;
    maxProfile->maxZones              = 0;
    maxProfile->maxTwilightPoints     = 0;
    maxProfile->maxStorage            = 0;
    maxProfile->maxFunctionDefs       = 0;
    maxProfile->maxInstructionDefs    = 0;
    maxProfile->maxStackElements      = 0;
    maxProfile->maxSizeOfInstructions = 0;
    maxProfile->maxComponentElements  = 0;
    maxProfile->maxComponentDepth     = 0;

    if ( maxProfile->version >= 0x10000L )
    {
      if ( FT_STREAM_READ_FIELDS( maxp_fields_extra, maxProfile ) )
        goto Exit;

      /* XXX: an adjustment that is necessary to load certain */
      /*      broken fonts like `Keystrokes MT' :-(           */
      /*                                                      */
      /*   We allocate 64 function entries by default when    */
      /*   the maxFunctionDefs field is null.                 */

      if ( maxProfile->maxFunctionDefs == 0 )
        maxProfile->maxFunctionDefs = 64;

      face->root.internal->max_points =
        (FT_UShort)FT_MAX( maxProfile->maxCompositePoints,
                           maxProfile->maxPoints );

      face->root.internal->max_contours =
        (FT_Short)FT_MAX( maxProfile->maxCompositeContours,
                          maxProfile->maxContours );

      face->max_components = (FT_ULong)maxProfile->maxComponentElements +
                             maxProfile->maxComponentDepth;

      /* XXX: some fonts have maxComponents set to 0; we will */
      /*      then use 16 of them by default.                 */
      if ( face->max_components == 0 )
        face->max_components = 16;

      /* We also increase maxPoints and maxContours in order to support */
      /* some broken fonts.                                             */
      face->root.internal->max_points   += (FT_UShort)8;
      face->root.internal->max_contours += (FT_Short) 4;
    }

    FT_TRACE2(( "MAXP loaded.\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_metrics                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal or vertical metrics table into a face object. */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load vertical metrics.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  tt_face_load_metrics( TT_Face    face,
                        FT_Stream  stream,
                        FT_Bool    vertical )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_ULong   table_len;
    FT_Long    num_shorts, num_longs, num_shorts_checked;

    TT_LongMetrics *   longs;
    TT_ShortMetrics**  shorts;


    FT_TRACE2(( "TT_Load_%s_Metrics: %08p\n", vertical ? "Vertical"
                                                       : "Horizontal",
                                              face ));

    if ( vertical )
    {
      /* The table is optional, quit silently if it wasn't found       */
      /*                                                               */
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
        error = SFNT_Err_Ok;
        goto Exit;
      }

      num_longs = face->vertical.number_Of_VMetrics;
      longs     = (TT_LongMetrics *)&face->vertical.long_metrics;
      shorts    = (TT_ShortMetrics**)&face->vertical.short_metrics;
    }
    else
    {
      error = face->goto_table( face, TTAG_hmtx, stream, &table_len );
      if ( error )
      {

#ifdef FT_CONFIG_OPTION_INCREMENTAL
        /* If this is an incrementally loaded font and there are */
        /* overriding metrics tolerate a missing 'hmtx' table.   */
        if ( face->root.internal->incremental_interface &&
             face->root.internal->incremental_interface->funcs->
               get_glyph_metrics )
        {
          face->horizontal.number_Of_HMetrics = 0;
          error = SFNT_Err_Ok;
          goto Exit;
        }
#endif

        FT_ERROR(( " no horizontal metrics in file!\n" ));
        error = SFNT_Err_Hmtx_Table_Missing;
        goto Exit;
      }

      num_longs = face->horizontal.number_Of_HMetrics;
      longs     = (TT_LongMetrics *)&face->horizontal.long_metrics;
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

      error = vertical ? SFNT_Err_Invalid_Vert_Metrics
                       : SFNT_Err_Invalid_Horiz_Metrics;
      goto Exit;
    }

    if ( FT_QNEW_ARRAY( *longs,  num_longs  ) ||
         FT_QNEW_ARRAY( *shorts, num_shorts ) )
      goto Exit;

    if ( FT_FRAME_ENTER( table_len ) )
      goto Exit;

    {
      TT_LongMetrics  cur   = *longs;
      TT_LongMetrics  limit = cur + num_longs;


      for ( ; cur < limit; cur++ )
      {
        cur->advance = FT_GET_USHORT();
        cur->bearing = FT_GET_SHORT();
      }
    }

    /* do we have an inconsistent number of metric values? */
    {
      TT_ShortMetrics*  cur   = *shorts;
      TT_ShortMetrics*  limit = cur +
                                FT_MIN( num_shorts, num_shorts_checked );


      for ( ; cur < limit; cur++ )
        *cur = FT_GET_SHORT();

      /* we fill up the missing left side bearings with the     */
      /* last valid value.  Since this will occur for buggy CJK */
      /* fonts usually only, nothing serious will happen        */
      if ( num_shorts > num_shorts_checked && num_shorts_checked > 0 )
      {
        FT_Short  val = (*shorts)[num_shorts_checked - 1];


        limit = *shorts + num_shorts;
        for ( ; cur < limit; cur++ )
          *cur = val;
      }
    }

    FT_FRAME_EXIT();

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_metrics_header                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal or vertical header in a face object.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load vertical metrics.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_metrics_header( TT_Face    face,
                               FT_Stream  stream,
                               FT_Bool    vertical )
  {
    FT_Error        error;
    TT_HoriHeader*  header;

    const FT_Frame_Field  metrics_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_HoriHeader

      FT_FRAME_START( 36 ),
        FT_FRAME_ULONG ( Version ),
        FT_FRAME_SHORT ( Ascender ),
        FT_FRAME_SHORT ( Descender ),
        FT_FRAME_SHORT ( Line_Gap ),
        FT_FRAME_USHORT( advance_Width_Max ),
        FT_FRAME_SHORT ( min_Left_Side_Bearing ),
        FT_FRAME_SHORT ( min_Right_Side_Bearing ),
        FT_FRAME_SHORT ( xMax_Extent ),
        FT_FRAME_SHORT ( caret_Slope_Rise ),
        FT_FRAME_SHORT ( caret_Slope_Run ),
        FT_FRAME_SHORT ( caret_Offset ),
        FT_FRAME_SHORT ( Reserved[0] ),
        FT_FRAME_SHORT ( Reserved[1] ),
        FT_FRAME_SHORT ( Reserved[2] ),
        FT_FRAME_SHORT ( Reserved[3] ),
        FT_FRAME_SHORT ( metric_Data_Format ),
        FT_FRAME_USHORT( number_Of_HMetrics ),
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
        error = SFNT_Err_Ok;
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
        error = SFNT_Err_Horiz_Header_Missing;
        goto Exit;
      }

      header = &face->horizontal;
    }

    if ( FT_STREAM_READ_FIELDS( metrics_header_fields, header ) )
      goto Exit;

    header->long_metrics  = NULL;
    header->short_metrics = NULL;

    FT_TRACE2(( "loaded\n" ));

    /* Now try to load the corresponding metrics */

    error = tt_face_load_metrics( face, stream, vertical );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_names                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the name records.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_names( TT_Face    face,
                      FT_Stream  stream )
  {
    FT_Error      error;
    FT_Memory     memory = stream->memory;
    FT_ULong      table_pos, table_len;
    FT_ULong      storage_start, storage_limit;
    FT_UInt       count;
    TT_NameTable  table;

    static const FT_Frame_Field  name_table_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_NameTableRec

      FT_FRAME_START( 6 ),
        FT_FRAME_USHORT( format ),
        FT_FRAME_USHORT( numNameRecords ),
        FT_FRAME_USHORT( storageOffset ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  name_record_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_NameEntryRec

      /* no FT_FRAME_START */
        FT_FRAME_USHORT( platformID ),
        FT_FRAME_USHORT( encodingID ),
        FT_FRAME_USHORT( languageID ),
        FT_FRAME_USHORT( nameID ),
        FT_FRAME_USHORT( stringLength ),
        FT_FRAME_USHORT( stringOffset ),
      FT_FRAME_END
    };


    table         = &face->name_table;
    table->stream = stream;

    FT_TRACE2(( "Names " ));

    error = face->goto_table( face, TTAG_name, stream, &table_len );
    if ( error )
    {
      /* The name table is required so indicate failure. */
      FT_TRACE2(( "is missing!\n" ));
      error = SFNT_Err_Name_Table_Missing;
      goto Exit;
    }

    table_pos = FT_STREAM_POS();


    if ( FT_STREAM_READ_FIELDS( name_table_fields, table ) )
      goto Exit;

    /* Some popular Asian fonts have an invalid `storageOffset' value   */
    /* (it should be at least "6 + 12*num_names").  However, the string */
    /* offsets, computed as "storageOffset + entry->stringOffset", are  */
    /* valid pointers within the name table...                          */
    /*                                                                  */
    /* We thus can't check `storageOffset' right now.                   */
    /*                                                                  */
    storage_start = table_pos + 6 + 12*table->numNameRecords;
    storage_limit = table_pos + table_len;

    if ( storage_start > storage_limit )
    {
      FT_ERROR(( "tt_face_load_names: invalid `name' table\n" ));
      error = SFNT_Err_Name_Table_Missing;
      goto Exit;
    }

    /* Allocate the array of name records. */
    count                 = table->numNameRecords;
    table->numNameRecords = 0;

    if ( FT_NEW_ARRAY( table->names, count ) ||
         FT_FRAME_ENTER( count * 12 )        )
      goto Exit;

    /* Load the name records and determine how much storage is needed */
    /* to hold the strings themselves.                                */
    {
      TT_NameEntryRec*  entry = table->names;


      for ( ; count > 0; count-- )
      {
        if ( FT_STREAM_READ_FIELDS( name_record_fields, entry ) )
          continue;

        /* check that the name is not empty */
        if ( entry->stringLength == 0 )
          continue;

        /* check that the name string is within the table */
        entry->stringOffset += table_pos + table->storageOffset;
        if ( entry->stringOffset                       < storage_start ||
             entry->stringOffset + entry->stringLength > storage_limit )
        {
          /* invalid entry - ignore it */
          entry->stringOffset = 0;
          entry->stringLength = 0;
          continue;
        }

        entry++;
      }

      table->numNameRecords = (FT_UInt)( entry - table->names );
    }

    FT_FRAME_EXIT();

    FT_TRACE2(( "loaded\n" ));

    /* everything went well, update face->num_names */
    face->num_names = (FT_UShort) table->numNameRecords;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_free_names                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees the name records.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_face_free_names( TT_Face  face )
  {
    FT_Memory     memory = face->root.driver->root.memory;
    TT_NameTable  table  = &face->name_table;
    TT_NameEntry  entry  = table->names;
    FT_UInt       count  = table->numNameRecords;


    if ( table->names )
    {
      for ( ; count > 0; count--, entry++ )
      {
        FT_FREE( entry->string );
        entry->stringLength = 0;
      }

      /* free strings table */
      FT_FREE( table->names );
    }

    table->numNameRecords = 0;
    table->format         = 0;
    table->storageOffset  = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_cmap                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the cmap directory in a face object.  The cmaps itselves are */
  /*    loaded on demand in the `ttcmap.c' module.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_cmap( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error  error;


    error = face->goto_table( face, TTAG_cmap, stream, &face->cmap_size );
    if ( error )
    {
      FT_TRACE2(( "No `cmap' table in font !\n" ));
      error = SFNT_Err_CMap_Table_Missing;
      goto Exit;
    }

    if ( !FT_FRAME_EXTRACT( face->cmap_size, face->cmap_table ) )
      FT_TRACE2(( "`cmap' table loaded\n" ));
    else
    {
      FT_ERROR(( "`cmap' table is too short!\n" ));
      face->cmap_size = 0;
    }

  Exit:
    return error;
  }



  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_os2                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the OS2 table.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_os2( TT_Face    face,
                    FT_Stream  stream )
  {
    FT_Error  error;
    TT_OS2*   os2;

    const FT_Frame_Field  os2_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_OS2

      FT_FRAME_START( 78 ),
        FT_FRAME_USHORT( version ),
        FT_FRAME_SHORT ( xAvgCharWidth ),
        FT_FRAME_USHORT( usWeightClass ),
        FT_FRAME_USHORT( usWidthClass ),
        FT_FRAME_SHORT ( fsType ),
        FT_FRAME_SHORT ( ySubscriptXSize ),
        FT_FRAME_SHORT ( ySubscriptYSize ),
        FT_FRAME_SHORT ( ySubscriptXOffset ),
        FT_FRAME_SHORT ( ySubscriptYOffset ),
        FT_FRAME_SHORT ( ySuperscriptXSize ),
        FT_FRAME_SHORT ( ySuperscriptYSize ),
        FT_FRAME_SHORT ( ySuperscriptXOffset ),
        FT_FRAME_SHORT ( ySuperscriptYOffset ),
        FT_FRAME_SHORT ( yStrikeoutSize ),
        FT_FRAME_SHORT ( yStrikeoutPosition ),
        FT_FRAME_SHORT ( sFamilyClass ),
        FT_FRAME_BYTE  ( panose[0] ),
        FT_FRAME_BYTE  ( panose[1] ),
        FT_FRAME_BYTE  ( panose[2] ),
        FT_FRAME_BYTE  ( panose[3] ),
        FT_FRAME_BYTE  ( panose[4] ),
        FT_FRAME_BYTE  ( panose[5] ),
        FT_FRAME_BYTE  ( panose[6] ),
        FT_FRAME_BYTE  ( panose[7] ),
        FT_FRAME_BYTE  ( panose[8] ),
        FT_FRAME_BYTE  ( panose[9] ),
        FT_FRAME_ULONG ( ulUnicodeRange1 ),
        FT_FRAME_ULONG ( ulUnicodeRange2 ),
        FT_FRAME_ULONG ( ulUnicodeRange3 ),
        FT_FRAME_ULONG ( ulUnicodeRange4 ),
        FT_FRAME_BYTE  ( achVendID[0] ),
        FT_FRAME_BYTE  ( achVendID[1] ),
        FT_FRAME_BYTE  ( achVendID[2] ),
        FT_FRAME_BYTE  ( achVendID[3] ),

        FT_FRAME_USHORT( fsSelection ),
        FT_FRAME_USHORT( usFirstCharIndex ),
        FT_FRAME_USHORT( usLastCharIndex ),
        FT_FRAME_SHORT ( sTypoAscender ),
        FT_FRAME_SHORT ( sTypoDescender ),
        FT_FRAME_SHORT ( sTypoLineGap ),
        FT_FRAME_USHORT( usWinAscent ),
        FT_FRAME_USHORT( usWinDescent ),
      FT_FRAME_END
    };

    const FT_Frame_Field  os2_fields_extra[] =
    {
      FT_FRAME_START( 8 ),
        FT_FRAME_ULONG( ulCodePageRange1 ),
        FT_FRAME_ULONG( ulCodePageRange2 ),
      FT_FRAME_END
    };

    const FT_Frame_Field  os2_fields_extra2[] =
    {
      FT_FRAME_START( 10 ),
        FT_FRAME_SHORT ( sxHeight ),
        FT_FRAME_SHORT ( sCapHeight ),
        FT_FRAME_USHORT( usDefaultChar ),
        FT_FRAME_USHORT( usBreakChar ),
        FT_FRAME_USHORT( usMaxContext ),
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
      face->os2.version = 0xFFFFU;
      error = SFNT_Err_Ok;
      goto Exit;
    }

    os2 = &face->os2;

    if ( FT_STREAM_READ_FIELDS( os2_fields, os2 ) )
      goto Exit;

    os2->ulCodePageRange1 = 0;
    os2->ulCodePageRange2 = 0;
    os2->sxHeight         = 0;
    os2->sCapHeight       = 0;
    os2->usDefaultChar    = 0;
    os2->usBreakChar      = 0;
    os2->usMaxContext     = 0;

    if ( os2->version >= 0x0001 )
    {
      /* only version 1 tables */
      if ( FT_STREAM_READ_FIELDS( os2_fields_extra, os2 ) )
        goto Exit;

      if ( os2->version >= 0x0002 )
      {
        /* only version 2 tables */
        if ( FT_STREAM_READ_FIELDS( os2_fields_extra2, os2 ) )
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
  /*    tt_face_load_postscript                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the Postscript table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_postscript( TT_Face    face,
                           FT_Stream  stream )
  {
    FT_Error        error;
    TT_Postscript*  post = &face->postscript;

    static const FT_Frame_Field  post_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_Postscript

      FT_FRAME_START( 32 ),
        FT_FRAME_ULONG( FormatType ),
        FT_FRAME_ULONG( italicAngle ),
        FT_FRAME_SHORT( underlinePosition ),
        FT_FRAME_SHORT( underlineThickness ),
        FT_FRAME_ULONG( isFixedPitch ),
        FT_FRAME_ULONG( minMemType42 ),
        FT_FRAME_ULONG( maxMemType42 ),
        FT_FRAME_ULONG( minMemType1 ),
        FT_FRAME_ULONG( maxMemType1 ),
      FT_FRAME_END
    };


    FT_TRACE2(( "PostScript " ));

    error = face->goto_table( face, TTAG_post, stream, 0 );
    if ( error )
      return SFNT_Err_Post_Table_Missing;

    if ( FT_STREAM_READ_FIELDS( post_fields, post ) )
      return error;

    /* we don't load the glyph names, we do that in another */
    /* module (ttpost).                                     */
    FT_TRACE2(( "loaded\n" ));

    return SFNT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_pclt                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the PCL 5 Table.                                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_pclt( TT_Face    face,
                     FT_Stream  stream )
  {
    static const FT_Frame_Field  pclt_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_PCLT

      FT_FRAME_START( 54 ),
        FT_FRAME_ULONG ( Version ),
        FT_FRAME_ULONG ( FontNumber ),
        FT_FRAME_USHORT( Pitch ),
        FT_FRAME_USHORT( xHeight ),
        FT_FRAME_USHORT( Style ),
        FT_FRAME_USHORT( TypeFamily ),
        FT_FRAME_USHORT( CapHeight ),
        FT_FRAME_BYTES ( TypeFace, 16 ),
        FT_FRAME_BYTES ( CharacterComplement, 8 ),
        FT_FRAME_BYTES ( FileName, 6 ),
        FT_FRAME_CHAR  ( StrokeWeight ),
        FT_FRAME_CHAR  ( WidthType ),
        FT_FRAME_BYTE  ( SerifStyle ),
        FT_FRAME_BYTE  ( Reserved ),
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
      return SFNT_Err_Ok;
    }

    if ( FT_STREAM_READ_FIELDS( pclt_fields, pclt ) )
      goto Exit;

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_gasp                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the `gasp' table into a face object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_gasp( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt        j,num_ranges;
    TT_GaspRange   gaspranges;


    FT_TRACE2(( "tt_face_load_gasp: %08p\n", face ));

    /* the gasp table is optional */
    error = face->goto_table( face, TTAG_gasp, stream, 0 );
    if ( error )
      return SFNT_Err_Ok;

    if ( FT_FRAME_ENTER( 4L ) )
      goto Exit;

    face->gasp.version   = FT_GET_USHORT();
    face->gasp.numRanges = FT_GET_USHORT();

    FT_FRAME_EXIT();

    num_ranges = face->gasp.numRanges;
    FT_TRACE3(( "number of ranges = %d\n", num_ranges ));

    if ( FT_QNEW_ARRAY( gaspranges, num_ranges ) ||
         FT_FRAME_ENTER( num_ranges * 4L )      )
      goto Exit;

    face->gasp.gaspRanges = gaspranges;

    for ( j = 0; j < num_ranges; j++ )
    {
      gaspranges[j].maxPPEM  = FT_GET_USHORT();
      gaspranges[j].gaspFlag = FT_GET_USHORT();

      FT_TRACE3(( " [max:%d flag:%d]",
                    gaspranges[j].maxPPEM,
                    gaspranges[j].gaspFlag ));
    }
    FT_TRACE3(( "\n" ));

    FT_FRAME_EXIT();
    FT_TRACE2(( "GASP loaded\n" ));

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( int )
  tt_kern_pair_compare( const void*  a,
                        const void*  b );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_kern                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the first kerning table with format 0 in the font.  Only     */
  /*    accepts the first horizontal kerning table.  Developers should use */
  /*    the `ftxkern' extension to access other kerning tables in the font */
  /*    file, if they really want to.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  
#undef  TT_KERN_INDEX
#define TT_KERN_INDEX( g1, g2 )  ( ( (FT_ULong)g1 << 16 ) | g2 )


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_kern( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt    n, num_tables;


    /* the kern table is optional; exit silently if it is missing */
    error = face->goto_table( face, TTAG_kern, stream, 0 );
    if ( error )
      return SFNT_Err_Ok;

    if ( FT_FRAME_ENTER( 4L ) )
      goto Exit;

    (void)FT_GET_USHORT();         /* version */
    num_tables = FT_GET_USHORT();

    FT_FRAME_EXIT();

    for ( n = 0; n < num_tables; n++ )
    {
      FT_UInt  coverage;
      FT_UInt  length;


      if ( FT_FRAME_ENTER( 6L ) )
        goto Exit;

      (void)FT_GET_USHORT();           /* version                 */
      length   = FT_GET_USHORT() - 6;  /* substract header length */
      coverage = FT_GET_USHORT();

      FT_FRAME_EXIT();

      if ( coverage == 0x0001 )
      {
        FT_UInt        num_pairs;
        TT_Kern0_Pair  pair;
        TT_Kern0_Pair  limit;


        /* found a horizontal format 0 kerning table! */
        if ( FT_FRAME_ENTER( 8L ) )
          goto Exit;

        num_pairs = FT_GET_USHORT();

        /* skip the rest */

        FT_FRAME_EXIT();

        /* allocate array of kerning pairs */
        if ( FT_QNEW_ARRAY( face->kern_pairs, num_pairs ) ||
             FT_FRAME_ENTER( 6L * num_pairs )             )
          goto Exit;

        pair  = face->kern_pairs;
        limit = pair + num_pairs;
        for ( ; pair < limit; pair++ )
        {
          pair->left  = FT_GET_USHORT();
          pair->right = FT_GET_USHORT();
          pair->value = FT_GET_USHORT();
        }

        FT_FRAME_EXIT();

        face->num_kern_pairs   = num_pairs;
        face->kern_table_index = n;

        /* ensure that the kerning pair table is sorted (yes, some */
        /* fonts have unsorted tables!)                            */

#if 1
        if ( num_pairs > 0 )     
        {
          TT_Kern0_Pair  pair0 = face->kern_pairs;
          FT_ULong       prev  = TT_KERN_INDEX( pair0->left, pair0->right );
          

          for ( pair0++; pair0 < limit; pair0++ )
          {
            FT_ULong  next = TT_KERN_INDEX( pair0->left, pair0->right );
            

            if ( next < prev )
              goto SortIt;
              
            prev = next;
          }
          goto Exit;
          
        SortIt:
          ft_qsort( (void*)face->kern_pairs, (int)num_pairs,
                    sizeof ( TT_Kern0_PairRec ), tt_kern_pair_compare );
        }
#else        
        {
          TT_Kern0_Pair  pair0    = face->kern_pairs;
          FT_UInt        i;
          

          for ( i = 1; i < num_pairs; i++, pair0++ )
          {
            if ( tt_kern_pair_compare( pair0, pair0 + 1 ) != -1 )
            {
              ft_qsort( (void*)face->kern_pairs, (int)num_pairs,
                        sizeof ( TT_Kern0_PairRec ), tt_kern_pair_compare );
              break;
            }
          }
        }
#endif

        goto Exit;
      }

      if ( FT_STREAM_SKIP( length ) )
        goto Exit;
    }

    /* no kern table found -- doesn't matter */
    face->kern_table_index = -1;
    face->num_kern_pairs   = 0;
    face->kern_pairs       = NULL;

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( int )
  tt_kern_pair_compare( const void*  a,
                        const void*  b )
  {
    TT_Kern0_Pair  pair1 = (TT_Kern0_Pair)a;
    TT_Kern0_Pair  pair2 = (TT_Kern0_Pair)b;

    FT_ULong  index1 = TT_KERN_INDEX( pair1->left, pair1->right );
    FT_ULong  index2 = TT_KERN_INDEX( pair2->left, pair2->right );


    return ( index1 < index2 ? -1 :
           ( index1 > index2 ?  1 : 0 ));
  }


#undef TT_KERN_INDEX
  


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hdmx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal device metrics table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hdmx( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    TT_Hdmx    hdmx = &face->hdmx;
    FT_Short   num_records;
    FT_Long    num_glyphs;
    FT_Long    record_size;


    hdmx->version     = 0;
    hdmx->num_records = 0;
    hdmx->records     = 0;

    /* this table is optional */
    error = face->goto_table( face, TTAG_hdmx, stream, 0 );
    if ( error )
      return SFNT_Err_Ok;

    if ( FT_FRAME_ENTER( 8L ) )
      goto Exit;

    hdmx->version     = FT_GET_USHORT();
    num_records       = FT_GET_SHORT();
    record_size       = FT_GET_LONG();

    FT_FRAME_EXIT();

    /* Only recognize format 0 */
    if ( hdmx->version != 0 )
      goto Exit;

    if ( FT_QNEW_ARRAY( hdmx->records, num_records ) )
      goto Exit;

    hdmx->num_records = num_records;
    num_glyphs   = face->root.num_glyphs;
    record_size -= num_glyphs + 2;

    {
      TT_HdmxEntry  cur   = hdmx->records;
      TT_HdmxEntry  limit = cur + hdmx->num_records;


      for ( ; cur < limit; cur++ )
      {
        /* read record */
        if ( FT_READ_BYTE( cur->ppem      ) ||
             FT_READ_BYTE( cur->max_width ) )
          goto Exit;

        if ( FT_QALLOC( cur->widths, num_glyphs )       ||
             FT_STREAM_READ( cur->widths, num_glyphs ) )
          goto Exit;

        /* skip padding bytes */
        if ( record_size > 0 && FT_STREAM_SKIP( record_size ) )
            goto Exit;
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_free_hdmx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees the horizontal device metrics table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  tt_face_free_hdmx( TT_Face  face )
  {
    if ( face )
    {
      FT_Int     n;
      FT_Memory  memory = face->root.driver->root.memory;


      for ( n = 0; n < face->hdmx.num_records; n++ )
        FT_FREE( face->hdmx.records[n].widths );

      FT_FREE( face->hdmx.records );
      face->hdmx.num_records = 0;
    }
  }


/* END */
