/***************************************************************************/
/*                                                                         */
/*  cffcmap.c                                                              */
/*                                                                         */
/*    CFF character mapping table (cmap) support (body).                   */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "cffcmap.h"
#include "cffload.h"

#include "cfferrs.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****           CFF STANDARD (AND EXPERT) ENCODING CMAPS            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Error )
  cff_cmap_encoding_init( CFF_CMapStd  cmap )
  {
    TT_Face       face     = (TT_Face)FT_CMAP_FACE( cmap );
    CFF_Font      cff      = (CFF_Font)face->extra.data;
    CFF_Encoding  encoding = &cff->encoding;


    cmap->gids  = encoding->codes;

    return 0;
  }


  FT_CALLBACK_DEF( void )
  cff_cmap_encoding_done( CFF_CMapStd  cmap )
  {
    cmap->gids  = NULL;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_encoding_char_index( CFF_CMapStd  cmap,
                                FT_UInt32    char_code )
  {
    FT_UInt  result = 0;


    if ( char_code < 256 )
      result = cmap->gids[char_code];

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_encoding_char_next( CFF_CMapStd   cmap,
                               FT_UInt32    *pchar_code )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *pchar_code;


    *pchar_code = 0;

    if ( char_code < 255 )
    {
      FT_UInt  code = (FT_UInt)(char_code + 1);


      for (;;)
      {
        if ( code >= 256 )
          break;

        result = cmap->gids[code];
        if ( result != 0 )
        {
          *pchar_code = code;
          break;
        }

        code++;
      }
    }
    return result;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  cff_cmap_encoding_class_rec =
  {
    sizeof ( CFF_CMapStdRec ),

    (FT_CMap_InitFunc)     cff_cmap_encoding_init,
    (FT_CMap_DoneFunc)     cff_cmap_encoding_done,
    (FT_CMap_CharIndexFunc)cff_cmap_encoding_char_index,
    (FT_CMap_CharNextFunc) cff_cmap_encoding_char_next
  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              CFF SYNTHETIC UNICODE ENCODING CMAP              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Int )
  cff_cmap_uni_pair_compare( const void*  pair1,
                             const void*  pair2 )
  {
    FT_UInt32  u1 = ((CFF_CMapUniPair)pair1)->unicode;
    FT_UInt32  u2 = ((CFF_CMapUniPair)pair2)->unicode;


    if ( u1 < u2 )
      return -1;

    if ( u1 > u2 )
      return +1;

    return 0;
  }


  FT_CALLBACK_DEF( FT_Error )
  cff_cmap_unicode_init( CFF_CMapUnicode  cmap )
  {
    FT_Error            error;
    FT_UInt             count;
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( cmap );
    FT_Memory           memory  = FT_FACE_MEMORY( face );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    CFF_Charset         charset = &cff->charset;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


    /* can't build Unicode map for CID-keyed font */
    if ( !charset->sids )
    {
      error = CFF_Err_Invalid_Argument;
      goto Exit;
    }

    cmap->num_pairs = 0;
    cmap->pairs     = NULL;

    count = cff->num_glyphs;

    if ( !FT_NEW_ARRAY( cmap->pairs, count ) )
    {
      FT_UInt          n, new_count;
      CFF_CMapUniPair  pair;
      FT_UInt32        uni_code;


      pair = cmap->pairs;
      for ( n = 0; n < count; n++ )
      {
        FT_UInt      sid = charset->sids[n];
        const char*  gname;


        gname = cff_index_get_sid_string( &cff->string_index, sid, psnames );

        /* build unsorted pair table by matching glyph names */
        if ( gname )
        {
          uni_code = psnames->unicode_value( gname );

          if ( uni_code != 0 )
          {
            pair->unicode = uni_code;
            pair->gindex  = n;
            pair++;
          }

          FT_FREE( gname );
        }
      }

      new_count = (FT_UInt)( pair - cmap->pairs );
      if ( new_count == 0 )
      {
        /* there are no unicode characters in here! */
        FT_FREE( cmap->pairs );
        error = CFF_Err_Invalid_Argument;
      }
      else
      {
        /* re-allocate if the new array is much smaller than the original */
        /* one                                                            */
        if ( new_count != count && new_count < count / 2 )
        {
          (void)FT_RENEW_ARRAY( cmap->pairs, count, new_count );
          error = CFF_Err_Ok;
        }

        /* sort the pairs table to allow efficient binary searches */
        ft_qsort( cmap->pairs,
                  new_count,
                  sizeof ( CFF_CMapUniPairRec ),
                  cff_cmap_uni_pair_compare );

        cmap->num_pairs = new_count;
      }
    }

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( void )
  cff_cmap_unicode_done( CFF_CMapUnicode  cmap )
  {
    FT_Face    face   = FT_CMAP_FACE( cmap );
    FT_Memory  memory = FT_FACE_MEMORY( face );


    FT_FREE( cmap->pairs );
    cmap->num_pairs = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_unicode_char_index( CFF_CMapUnicode  cmap,
                               FT_UInt32        char_code )
  {
    FT_UInt          min = 0;
    FT_UInt          max = cmap->num_pairs;
    FT_UInt          mid;
    CFF_CMapUniPair  pair;


    while ( min < max )
    {
      mid  = min + ( max - min ) / 2;
      pair = cmap->pairs + mid;

      if ( pair->unicode == char_code )
        return pair->gindex;

      if ( pair->unicode < char_code )
        min = mid + 1;
      else
        max = mid;
    }
    return 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  cff_cmap_unicode_char_next( CFF_CMapUnicode  cmap,
                              FT_UInt32       *pchar_code )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *pchar_code + 1;


  Restart:
    {
      FT_UInt          min = 0;
      FT_UInt          max = cmap->num_pairs;
      FT_UInt          mid;
      CFF_CMapUniPair  pair;


      while ( min < max )
      {
        mid  = min + ( ( max - min ) >> 1 );
        pair = cmap->pairs + mid;

        if ( pair->unicode == char_code )
        {
          result = pair->gindex;
          if ( result != 0 )
            goto Exit;

          char_code++;
          goto Restart;
        }

        if ( pair->unicode < char_code )
          min = mid+1;
        else
          max = mid;
      }

      /* we didn't find it, but we have a pair just above it */
      char_code = 0;

      if ( min < cmap->num_pairs )
      {
        pair   = cmap->pairs + min;
        result = pair->gindex;
        if ( result != 0 )
          char_code = pair->unicode;
      }
    }

  Exit:
    *pchar_code = char_code;
    return result;
  }


  FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec
  cff_cmap_unicode_class_rec =
  {
    sizeof ( CFF_CMapUnicodeRec ),

    (FT_CMap_InitFunc)     cff_cmap_unicode_init,
    (FT_CMap_DoneFunc)     cff_cmap_unicode_done,
    (FT_CMap_CharIndexFunc)cff_cmap_unicode_char_index,
    (FT_CMap_CharNextFunc) cff_cmap_unicode_char_next
  };


/* END */
