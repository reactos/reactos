/***************************************************************************/
/*                                                                         */
/*  ttcmap.c                                                               */
/*                                                                         */
/*    TrueType character mapping table (cmap) support (body).              */
/*                                                                         */
/*  Copyright 1996-2001, 2002 by                                           */
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
#include "ttload.h"
#include "ttcmap.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttcmap


  FT_CALLBACK_DEF( FT_UInt )
  code_to_index0( TT_CMapTable  charmap,
                  FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_ULong )
  code_to_next0( TT_CMapTable  charmap,
                 FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index2( TT_CMapTable  charmap,
                  FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_ULong )
  code_to_next2( TT_CMapTable  charmap,
                 FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index4( TT_CMapTable  charmap,
                  FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_ULong )
  code_to_next4( TT_CMapTable  charmap,
                 FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index6( TT_CMapTable  charmap,
                  FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_ULong )
  code_to_next6( TT_CMapTable  charmap,
                 FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index8_12( TT_CMapTable  charmap,
                     FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_ULong )
  code_to_next8_12( TT_CMapTable  charmap,
                    FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index10( TT_CMapTable  charmap,
                   FT_ULong      char_code );

  FT_CALLBACK_DEF( FT_ULong )
  code_to_next10( TT_CMapTable  charmap,
                  FT_ULong      char_code );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_charmap                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given TrueType character map into memory.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the parent face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the current stream object.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: A pointer to a cmap object.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function assumes that the stream is already in use (i.e.,      */
  /*    opened).  In case of error, all partially allocated tables are     */
  /*    released.                                                          */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_charmap( TT_Face       face,
                        TT_CMapTable  cmap,
                        FT_Stream     stream )
  {
    FT_Error     error;
    FT_Memory    memory;
    FT_UShort    num_SH, num_Seg, i;
    FT_ULong     j, n;

    FT_UShort    u, l;

    TT_CMap0     cmap0;
    TT_CMap2     cmap2;
    TT_CMap4     cmap4;
    TT_CMap6     cmap6;
    TT_CMap8_12  cmap8_12;
    TT_CMap10    cmap10;

    TT_CMap2SubHeader  cmap2sub;
    TT_CMap4Segment    segments;
    TT_CMapGroup       groups;


    if ( cmap->loaded )
      return SFNT_Err_Ok;

    memory = stream->memory;

    if ( FT_STREAM_SEEK( cmap->offset ) )
      return error;

    switch ( cmap->format )
    {
    case 0:
      cmap0 = &cmap->c.cmap0;

      if ( FT_READ_USHORT( cmap0->language )           ||
           FT_ALLOC( cmap0->glyphIdArray, 256L )       ||
           FT_STREAM_READ( cmap0->glyphIdArray, 256L ) )
        goto Fail;

      cmap->get_index     = code_to_index0;
      cmap->get_next_char = code_to_next0;
      break;

    case 2:
      num_SH = 0;
      cmap2  = &cmap->c.cmap2;

      /* allocate subheader keys */

      if ( FT_NEW_ARRAY( cmap2->subHeaderKeys, 256 ) ||
           FT_FRAME_ENTER( 2L + 512L )               )
        goto Fail;

      cmap2->language = FT_GET_USHORT();

      for ( i = 0; i < 256; i++ )
      {
        u = (FT_UShort)( FT_GET_USHORT() / 8 );
        cmap2->subHeaderKeys[i] = u;

        if ( num_SH < u )
          num_SH = u;
      }

      FT_FRAME_EXIT();

      /* load subheaders */

      cmap2->numGlyphId = l = (FT_UShort)(
        ( ( cmap->length - 2L * ( 256 + 3 ) - num_SH * 8L ) & 0xFFFFU ) / 2 );

      if ( FT_NEW_ARRAY( cmap2->subHeaders, num_SH + 1 ) ||
           FT_FRAME_ENTER( ( num_SH + 1 ) * 8L )         )
      {
        FT_FREE( cmap2->subHeaderKeys );
        goto Fail;
      }

      cmap2sub = cmap2->subHeaders;

      for ( i = 0; i <= num_SH; i++ )
      {
        cmap2sub->firstCode     = FT_GET_USHORT();
        cmap2sub->entryCount    = FT_GET_USHORT();
        cmap2sub->idDelta       = FT_GET_SHORT();
        /* we apply the location offset immediately */
        cmap2sub->idRangeOffset = (FT_UShort)(
          FT_GET_USHORT() - ( num_SH - i ) * 8 - 2 );

        cmap2sub++;
      }

      FT_FRAME_EXIT();

      /* load glyph IDs */

      if ( FT_NEW_ARRAY( cmap2->glyphIdArray, l ) ||
           FT_FRAME_ENTER( l * 2L )               )
      {
        FT_FREE( cmap2->subHeaders );
        FT_FREE( cmap2->subHeaderKeys );
        goto Fail;
      }

      for ( i = 0; i < l; i++ )
        cmap2->glyphIdArray[i] = FT_GET_USHORT();

      FT_FRAME_EXIT();

      cmap->get_index = code_to_index2;
      cmap->get_next_char = code_to_next2;
      break;

    case 4:
      cmap4 = &cmap->c.cmap4;

      /* load header */

      if ( FT_FRAME_ENTER( 10L ) )
        goto Fail;

      cmap4->language      = FT_GET_USHORT();
      cmap4->segCountX2    = FT_GET_USHORT();
      cmap4->searchRange   = FT_GET_USHORT();
      cmap4->entrySelector = FT_GET_USHORT();
      cmap4->rangeShift    = FT_GET_USHORT();

      num_Seg = (FT_UShort)( cmap4->segCountX2 / 2 );

      FT_FRAME_EXIT();

      /* load segments */

      if ( FT_NEW_ARRAY( cmap4->segments, num_Seg )   ||
           FT_FRAME_ENTER( ( num_Seg * 4 + 1 ) * 2L ) )
        goto Fail;

      segments = cmap4->segments;

      for ( i = 0; i < num_Seg; i++ )
        segments[i].endCount = FT_GET_USHORT();

      (void)FT_GET_USHORT();

      for ( i = 0; i < num_Seg; i++ )
        segments[i].startCount = FT_GET_USHORT();

      for ( i = 0; i < num_Seg; i++ )
        segments[i].idDelta = FT_GET_SHORT();

      for ( i = 0; i < num_Seg; i++ )
        segments[i].idRangeOffset = FT_GET_USHORT();

      FT_FRAME_EXIT();

      cmap4->numGlyphId = l = (FT_UShort)(
        ( ( cmap->length - ( 16L + 8L * num_Seg ) ) & 0xFFFFU ) / 2 );

      /* load IDs */

      if ( FT_NEW_ARRAY( cmap4->glyphIdArray, l ) ||
           FT_FRAME_ENTER( l * 2L )               )
      {
        FT_FREE( cmap4->segments );
        goto Fail;
      }

      for ( i = 0; i < l; i++ )
        cmap4->glyphIdArray[i] = FT_GET_USHORT();

      FT_FRAME_EXIT();

      cmap4->last_segment = cmap4->segments;

      cmap->get_index     = code_to_index4;
      cmap->get_next_char = code_to_next4;
      break;

    case 6:
      cmap6 = &cmap->c.cmap6;

      if ( FT_FRAME_ENTER( 6L ) )
        goto Fail;

      cmap6->language   = FT_GET_USHORT();
      cmap6->firstCode  = FT_GET_USHORT();
      cmap6->entryCount = FT_GET_USHORT();

      FT_FRAME_EXIT();

      l = cmap6->entryCount;

      if ( FT_NEW_ARRAY( cmap6->glyphIdArray, l ) ||
           FT_FRAME_ENTER( l * 2L )               )
        goto Fail;

      for ( i = 0; i < l; i++ )
        cmap6->glyphIdArray[i] = FT_GET_USHORT();

      FT_FRAME_EXIT();
      cmap->get_index     = code_to_index6;
      cmap->get_next_char = code_to_next6;
      break;

    case 8:
    case 12:
      cmap8_12 = &cmap->c.cmap8_12;

      if ( FT_FRAME_ENTER( 8L ) )
        goto Fail;

      cmap->length       = FT_GET_ULONG();
      cmap8_12->language = FT_GET_ULONG();

      FT_FRAME_EXIT();

      if ( cmap->format == 8 )
        if ( FT_STREAM_SKIP( 8192L ) )
          goto Fail;

      if ( FT_READ_ULONG( cmap8_12->nGroups ) )
        goto Fail;

      n = cmap8_12->nGroups;

      if ( FT_NEW_ARRAY( cmap8_12->groups, n ) ||
           FT_FRAME_ENTER( n * 3 * 4L )        )
        goto Fail;

      groups = cmap8_12->groups;

      for ( j = 0; j < n; j++ )
      {
        groups[j].startCharCode = FT_GET_ULONG();
        groups[j].endCharCode   = FT_GET_ULONG();
        groups[j].startGlyphID  = FT_GET_ULONG();
      }

      FT_FRAME_EXIT();

      cmap8_12->last_group = cmap8_12->groups;

      cmap->get_index     = code_to_index8_12;
      cmap->get_next_char = code_to_next8_12;
      break;

    case 10:
      cmap10 = &cmap->c.cmap10;

      if ( FT_FRAME_ENTER( 16L ) )
        goto Fail;

      cmap->length          = FT_GET_ULONG();
      cmap10->language      = FT_GET_ULONG();
      cmap10->startCharCode = FT_GET_ULONG();
      cmap10->numChars      = FT_GET_ULONG();

      FT_FRAME_EXIT();

      n = cmap10->numChars;

      if ( FT_NEW_ARRAY( cmap10->glyphs, n ) ||
           FT_FRAME_ENTER( n * 2L )          )
        goto Fail;

      for ( j = 0; j < n; j++ )
        cmap10->glyphs[j] = FT_GET_USHORT();

      FT_FRAME_EXIT();
      cmap->get_index     = code_to_index10;
      cmap->get_next_char = code_to_next10;
      break;

    default:   /* corrupt character mapping table */
      return SFNT_Err_Invalid_CharMap_Format;

    }

    return SFNT_Err_Ok;

  Fail:
    tt_face_free_charmap( face, cmap );
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_free_charmap                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a character mapping table.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the parent face object.                        */
  /*                                                                       */
  /*    cmap :: A handle to a cmap object.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_free_charmap( TT_Face       face,
                        TT_CMapTable  cmap )
  {
    FT_Memory  memory;


    if ( !cmap )
      return SFNT_Err_Ok;

    memory = face->root.driver->root.memory;

    switch ( cmap->format )
    {
    case 0:
      FT_FREE( cmap->c.cmap0.glyphIdArray );
      break;

    case 2:
      FT_FREE( cmap->c.cmap2.subHeaderKeys );
      FT_FREE( cmap->c.cmap2.subHeaders );
      FT_FREE( cmap->c.cmap2.glyphIdArray );
      break;

    case 4:
      FT_FREE( cmap->c.cmap4.segments );
      FT_FREE( cmap->c.cmap4.glyphIdArray );
      cmap->c.cmap4.segCountX2 = 0;
      break;

    case 6:
      FT_FREE( cmap->c.cmap6.glyphIdArray );
      cmap->c.cmap6.entryCount = 0;
      break;

    case 8:
    case 12:
      FT_FREE( cmap->c.cmap8_12.groups );
      cmap->c.cmap8_12.nGroups = 0;
      break;

    case 10:
      FT_FREE( cmap->c.cmap10.glyphs );
      cmap->c.cmap10.numChars = 0;
      break;

    default:
      /* invalid table format, do nothing */
      ;
    }

    cmap->loaded = FALSE;
    return SFNT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index0                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 0.    */
  /*    `charCode' must be in the range 0x00-0xFF (otherwise 0 is          */
  /*    returned).                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap0    :: A pointer to a cmap table in format 0.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index0( TT_CMapTable  cmap,
                  FT_ULong      charCode )
  {
    TT_CMap0  cmap0 = &cmap->c.cmap0;


    return ( charCode <= 0xFF ? cmap0->glyphIdArray[charCode] : 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_next0                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finds the next encoded character after the given one.  Uses        */
  /*    format 0. `charCode' must be in the range 0x00-0xFF (otherwise 0   */
  /*    is returned).                                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap0    :: A pointer to a cmap table in format 0.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Next char code.  0 if no higher one is encoded.                    */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_ULong )
  code_to_next0( TT_CMapTable  cmap,
                 FT_ULong      charCode )
  {
    TT_CMap0  cmap0 = &cmap->c.cmap0;


    while ( ++charCode <= 0xFF )
      if ( cmap0->glyphIdArray[charCode] )
        return ( charCode );
    return ( 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index2                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 2.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap2    :: A pointer to a cmap table in format 2.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index2( TT_CMapTable  cmap,
                  FT_ULong      charCode )
  {
    FT_UInt            result, index1, offset;
    FT_UInt            char_lo;
    FT_ULong           char_hi;
    TT_CMap2SubHeader  sh2;
    TT_CMap2           cmap2;


    cmap2   = &cmap->c.cmap2;
    result  = 0;
    char_lo = (FT_UInt)( charCode & 0xFF );
    char_hi = charCode >> 8;

    if ( char_hi == 0 )
    {
      /* an 8-bit character code -- we use the subHeader 0 in this case */
      /* to test whether the character code is in the charmap           */
      index1 = cmap2->subHeaderKeys[char_lo];
      if ( index1 != 0 )
        return 0;
    }
    else
    {
      /* a 16-bit character code */
      index1 = cmap2->subHeaderKeys[char_hi & 0xFF];
      if ( index1 == 0 )
        return 0;
    }

    sh2      = cmap2->subHeaders + index1;
    char_lo -= sh2->firstCode;

    if ( char_lo < (FT_UInt)sh2->entryCount )
    {
      offset = sh2->idRangeOffset / 2 + char_lo;
      if ( offset < (FT_UInt)cmap2->numGlyphId )
      {
        result = cmap2->glyphIdArray[offset];
        if ( result )
          result = ( result + sh2->idDelta ) & 0xFFFFU;
      }
    }

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_next2                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the next encoded character.  Uses format 2.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap2    :: A pointer to a cmap table in format 2.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Next encoded character.  0 if none exists.                         */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_ULong )
  code_to_next2( TT_CMapTable  cmap,
                 FT_ULong      charCode )
  {
    FT_UInt            index1, offset;
    FT_UInt            char_lo;
    FT_ULong           char_hi;
    TT_CMap2SubHeader  sh2;
    TT_CMap2           cmap2;


    cmap2 = &cmap->c.cmap2;
    charCode++;

    /*
     * This is relatively simplistic -- look for a subHeader containing
     * glyphs and then walk to the first glyph in that subHeader.
     */
    while ( charCode < 0x10000L )
    {
      char_lo = (FT_UInt)( charCode & 0xFF );
      char_hi = charCode >> 8;

      if ( char_hi == 0 )
      {
        /* an 8-bit character code -- we use the subHeader 0 in this case */
        /* to test whether the character code is in the charmap           */
        index1 = cmap2->subHeaderKeys[char_lo];
        if ( index1 != 0 )
        {
          charCode++;
          continue;
        }
      }
      else
      {
        /* a 16-bit character code */
        index1 = cmap2->subHeaderKeys[char_hi & 0xFF];
        if ( index1 == 0 )
        {
          charCode = ( char_hi + 1 ) << 8;
          continue;
        }
      }

      sh2      = cmap2->subHeaders + index1;
      char_lo -= sh2->firstCode;

      if ( char_lo > (FT_UInt)sh2->entryCount )
      {
        charCode = ( char_hi + 1 ) << 8;
        continue;
      }

      offset = sh2->idRangeOffset / 2 + char_lo;
      if ( offset >= (FT_UInt)cmap2->numGlyphId ||
           cmap2->glyphIdArray[offset] == 0     )
      {
        charCode++;
        continue;
      }

      return charCode;
    }
    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index4                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 4.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap4    :: A pointer to a cmap table in format 4.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index4( TT_CMapTable  cmap,
                  FT_ULong      charCode )
  {
    FT_UInt             result, index1, segCount;
    TT_CMap4            cmap4;
    TT_CMap4SegmentRec  *seg4, *limit;


    cmap4    = &cmap->c.cmap4;
    result   = 0;
    segCount = cmap4->segCountX2 / 2;
    limit    = cmap4->segments + segCount;

    /* first, check against the last used segment */

    seg4 = cmap4->last_segment;

    /* the following is equivalent to performing two tests, as in         */
    /*                                                                    */
    /*  if ( charCode >= seg4->startCount && charCode <= seg4->endCount ) */
    /*                                                                    */
    /* This is a bit strange, but it is faster, and the idea behind the   */
    /* cache is to significantly speed up charcode to glyph index         */
    /* conversion.                                                        */

    if ( (FT_ULong)( charCode       - seg4->startCount ) <
         (FT_ULong)( seg4->endCount - seg4->startCount ) )
      goto Found1;

    for ( seg4 = cmap4->segments; seg4 < limit; seg4++ )
    {
      /* the ranges are sorted in increasing order.  If we are out of */
      /* the range here, the char code isn't in the charmap, so exit. */

      if ( charCode > (FT_UInt)seg4->endCount )
        continue;

      if ( charCode >= (FT_UInt)seg4->startCount )
        goto Found;
    }
    return 0;

  Found:
    cmap4->last_segment = seg4;

  Found1:
    /* if the idRangeOffset is 0, we can compute the glyph index */
    /* directly                                                  */

    if ( seg4->idRangeOffset == 0 )
      result = (FT_UInt)( charCode + seg4->idDelta ) & 0xFFFFU;
    else
    {
      /* otherwise, we must use the glyphIdArray to do it */
      index1 = (FT_UInt)( seg4->idRangeOffset / 2
                          + ( charCode - seg4->startCount )
                          + ( seg4 - cmap4->segments )
                          - segCount );

      if ( index1 < (FT_UInt)cmap4->numGlyphId &&
           cmap4->glyphIdArray[index1] != 0    )
        result = ( cmap4->glyphIdArray[index1] + seg4->idDelta ) & 0xFFFFU;
    }

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_next4                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the next encoded character.  Uses format 4.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap     :: A pointer to a cmap table in format 4.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Next encoded character.  0 if none exists.                         */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_ULong )
  code_to_next4( TT_CMapTable  cmap,
                 FT_ULong      charCode )
  {
    FT_UInt             index1, segCount;
    TT_CMap4            cmap4;
    TT_CMap4SegmentRec  *seg4, *limit;


    cmap4    = &cmap->c.cmap4;
    segCount = cmap4->segCountX2 / 2;
    limit    = cmap4->segments + segCount;

    charCode++;

    for ( seg4 = cmap4->segments; seg4 < limit; seg4++ )
    {
      /* The ranges are sorted in increasing order.  If we are out of */
      /* the range here, the char code isn't in the charmap, so exit. */

      if ( charCode <= (FT_UInt)seg4->endCount )
        goto Found;
    }
    return 0;

  Found:
    if ( charCode < (FT_ULong) seg4->startCount )
      charCode = seg4->startCount;

    /* if the idRangeOffset is 0, all chars in the map exist */

    if ( seg4->idRangeOffset == 0 )
      return ( charCode );

    while ( charCode <= (FT_UInt) seg4->endCount )
    {
      /* otherwise, we must use the glyphIdArray to do it */
      index1 = (FT_UInt)( seg4->idRangeOffset / 2
                          + ( charCode - seg4->startCount )
                          + ( seg4 - cmap4->segments )
                          - segCount );

      if ( index1 < (FT_UInt)cmap4->numGlyphId &&
           cmap4->glyphIdArray[index1] != 0    )
        return ( charCode );
      charCode++;
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index6                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 6.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap6    :: A pointer to a cmap table in format 6.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index6( TT_CMapTable  cmap,
                  FT_ULong      charCode )
  {
    TT_CMap6  cmap6;
    FT_UInt   result = 0;


    cmap6     = &cmap->c.cmap6;
    charCode -= cmap6->firstCode;

    if ( charCode < (FT_UInt)cmap6->entryCount )
      result = cmap6->glyphIdArray[charCode];

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_next6                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the next encoded character.  Uses format 6.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap     :: A pointer to a cmap table in format 6.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Next encoded character.  0 if none exists.                         */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_ULong )
  code_to_next6( TT_CMapTable  cmap,
                 FT_ULong      charCode )
  {
    TT_CMap6  cmap6;


    charCode++;

    cmap6 = &cmap->c.cmap6;

    if ( charCode < (FT_ULong) cmap6->firstCode )
      charCode = cmap6->firstCode;

    charCode -= cmap6->firstCode;

    while ( charCode < (FT_UInt)cmap6->entryCount )
    {
      if ( cmap6->glyphIdArray[charCode] != 0 )
        return charCode + cmap6->firstCode;
      charCode++;
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index8_12                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the (possibly 32bit) character code into a glyph index.   */
  /*    Uses format 8 or 12.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap8_12 :: A pointer to a cmap table in format 8 or 12.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index8_12( TT_CMapTable  cmap,
                     FT_ULong      charCode )
  {
    TT_CMap8_12      cmap8_12;
    TT_CMapGroupRec  *group, *limit;


    cmap8_12 = &cmap->c.cmap8_12;
    limit    = cmap8_12->groups + cmap8_12->nGroups;

    /* first, check against the last used group */

    group = cmap8_12->last_group;

    /* the following is equivalent to performing two tests, as in       */
    /*                                                                  */
    /*  if ( charCode >= group->startCharCode &&                        */
    /*       charCode <= group->endCharCode   )                         */
    /*                                                                  */
    /* This is a bit strange, but it is faster, and the idea behind the */
    /* cache is to significantly speed up charcode to glyph index       */
    /* conversion.                                                      */

    if ( (FT_ULong)( charCode           - group->startCharCode ) <
         (FT_ULong)( group->endCharCode - group->startCharCode ) )
      goto Found1;

    for ( group = cmap8_12->groups; group < limit; group++ )
    {
      /* the ranges are sorted in increasing order.  If we are out of */
      /* the range here, the char code isn't in the charmap, so exit. */

      if ( charCode > group->endCharCode )
        continue;

      if ( charCode >= group->startCharCode )
        goto Found;
    }
    return 0;

  Found:
    cmap8_12->last_group = group;

  Found1:
    return (FT_UInt)( group->startGlyphID +
                      ( charCode - group->startCharCode ) );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_next8_12                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the next encoded character.  Uses format 8 or 12.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap     :: A pointer to a cmap table in format 8 or 12.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Next encoded character.  0 if none exists.                         */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_ULong )
  code_to_next8_12( TT_CMapTable  cmap,
                    FT_ULong      charCode )
  {
    TT_CMap8_12      cmap8_12;
    TT_CMapGroupRec  *group, *limit;


    charCode++;
    cmap8_12 = &cmap->c.cmap8_12;
    limit    = cmap8_12->groups + cmap8_12->nGroups;

    for ( group = cmap8_12->groups; group < limit; group++ )
    {
      /* the ranges are sorted in increasing order.  If we are out of */
      /* the range here, the char code isn't in the charmap, so exit. */

      if ( charCode <= group->endCharCode )
        goto Found;
    }
    return 0;

  Found:
    if ( charCode < group->startCharCode )
      charCode = group->startCharCode;

    return charCode;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index10                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the (possibly 32bit) character code into a glyph index.   */
  /*    Uses format 10.                                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap10   :: A pointer to a cmap table in format 10.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index10( TT_CMapTable  cmap,
                   FT_ULong      charCode )
  {
    TT_CMap10  cmap10;
    FT_UInt    result = 0;


    cmap10    = &cmap->c.cmap10;
    charCode -= cmap10->startCharCode;

    /* the overflow trick for comparison works here also since the number */
    /* of glyphs (even if numChars is specified as ULong in the specs) in */
    /* an OpenType font is limited to 64k                                 */

    if ( charCode < cmap10->numChars )
      result = cmap10->glyphs[charCode];

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_next10                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the next encoded character.  Uses format 10.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*                                                                       */
  /*    cmap     :: A pointer to a cmap table in format 10.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Next encoded character.  0 if none exists.                         */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_ULong )
  code_to_next10( TT_CMapTable  cmap,
                  FT_ULong      charCode )
  {
    TT_CMap10  cmap10;


    charCode++;
    cmap10 = &cmap->c.cmap10;

    if ( charCode < cmap10->startCharCode )
      charCode = cmap10->startCharCode;

    charCode -= cmap10->startCharCode;

    /* the overflow trick for comparison works here also since the number */
    /* of glyphs (even if numChars is specified as ULong in the specs) in */
    /* an OpenType font is limited to 64k                                 */

    while ( charCode < cmap10->numChars )
    {
      if ( cmap10->glyphs[charCode] )
        return ( charCode + cmap10->startCharCode );
      charCode++;
    }

    return 0;
  }


/* END */
