/***************************************************************************/
/*                                                                         */
/*  sfobjs.c                                                               */
/*                                                                         */
/*    SFNT object management (base).                                       */
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
#include "sfobjs.h"
#include "ttload.h"
#include "ttcmap0.h"
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_sfobjs



  /* convert a UTF-16 name entry to ASCII */
  static FT_String*
  tt_name_entry_ascii_from_utf16( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;


    len = (FT_UInt)entry->stringLength / 2;

    if ( FT_MEM_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = FT_NEXT_USHORT( read );
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  /* convert a UCS-4 name entry to ASCII */
  static FT_String*
  tt_name_entry_ascii_from_ucs4( TT_NameEntry  entry,
                                 FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;


    len = (FT_UInt)entry->stringLength / 4;

    if ( FT_MEM_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = (FT_UInt)FT_NEXT_ULONG( read );
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  /* convert an Apple Roman or symbol name entry to ASCII */
  static FT_String*
  tt_name_entry_ascii_from_other( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;


    len = (FT_UInt)entry->stringLength;

    if ( FT_MEM_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = *read++;
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  typedef FT_String*  (*TT_NameEntry_ConvertFunc)( TT_NameEntry  entry,
                                                   FT_Memory     memory );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_get_name                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a given ENGLISH name record in ASCII.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the source face object.                      */
  /*                                                                       */
  /*    nameid :: The name id of the name record to return.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Character string.  NULL if no name is present.                     */
  /*                                                                       */
  static FT_String*
  tt_face_get_name( TT_Face    face,
                    FT_UShort  nameid )
  {
    FT_Memory         memory = face->root.memory;
    FT_String*        result = NULL;
    FT_UShort         n;
    TT_NameEntryRec*  rec;
    FT_Int            found_apple   = -1;
    FT_Int            found_win     = -1;
    FT_Int            found_unicode = -1;

    FT_Bool           is_english = 0;

    TT_NameEntry_ConvertFunc  convert;


    rec = face->name_table.names;
    for ( n = 0; n < face->num_names; n++, rec++ )
    {
      /* According to the OpenType 1.3 specification, only Microsoft or  */
      /* Apple platform IDs might be used in the `name' table.  The      */
      /* `Unicode' platform is reserved for the `cmap' table, and the    */
      /* `Iso' one is deprecated.                                        */
      /*                                                                 */
      /* However, the Apple TrueType specification doesn't say the same  */
      /* thing and goes to suggest that all Unicode `name' table entries */
      /* should be coded in UTF-16 (in big-endian format I suppose).     */
      /*                                                                 */
      if ( rec->nameID == nameid && rec->stringLength > 0 )
      {
        switch ( rec->platformID )
        {
        case TT_PLATFORM_APPLE_UNICODE:
        case TT_PLATFORM_ISO:
          /* there is `languageID' to check there.  We should use this */
          /* field only as a last solution when nothing else is        */
          /* available.                                                */
          /*                                                           */
          found_unicode = n;
          break;

        case TT_PLATFORM_MACINTOSH:
          if ( rec->languageID == TT_MAC_LANGID_ENGLISH )
            found_apple = n;

          break;

        case TT_PLATFORM_MICROSOFT:
          /* we only take a non-English name when there is nothing */
          /* else available in the font                            */
          /*                                                       */
          if ( found_win == -1 || ( rec->languageID & 0x3FF ) == 0x009 )
          {
            switch ( rec->encodingID )
            {
            case TT_MS_ID_SYMBOL_CS:
            case TT_MS_ID_UNICODE_CS:
            case TT_MS_ID_UCS_4:
              is_english = ( rec->languageID & 0x3FF ) == 0x009;
              found_win  = n;
              break;

            default:
              ;
            }
          }
          break;

        default:
          ;
        }
      }
    }

    /* some fonts contain invalid Unicode or Macintosh formatted entries; */
    /* we will thus favor names encoded in Windows formats if available   */
    /* (provided it is an English name)                                   */
    /*                                                                    */
    convert = NULL;
    if ( found_win >= 0 && !( found_apple >= 0 && !is_english ) )
    {
      rec = face->name_table.names + found_win;
      switch ( rec->encodingID )
      {
      case TT_MS_ID_UNICODE_CS:
      case TT_MS_ID_SYMBOL_CS:
        convert = tt_name_entry_ascii_from_utf16;
        break;

      case TT_MS_ID_UCS_4:
        convert = tt_name_entry_ascii_from_ucs4;
        break;

      default:
        ;
      }
    }
    else if ( found_apple >= 0 )
    {
      rec     = face->name_table.names + found_apple;
      convert = tt_name_entry_ascii_from_other;
    }
    else if ( found_unicode >= 0 )
    {
      rec     = face->name_table.names + found_unicode;
      convert = tt_name_entry_ascii_from_utf16;
    }

    if ( rec && convert )
    {
      if ( rec->string == NULL )
      {
        FT_Error   error  = SFNT_Err_Ok;
        FT_Stream  stream = face->name_table.stream;

        FT_UNUSED( error );


        if ( FT_QNEW_ARRAY ( rec->string, rec->stringLength ) ||
             FT_STREAM_SEEK( rec->stringOffset )              ||
             FT_STREAM_READ( rec->string, rec->stringLength ) )
        {
          FT_FREE( rec->string );
          rec->stringLength = 0;
          result            = NULL;
          goto Exit;
        }
      }

      result = convert( rec, memory );
    }

  Exit:
    return result;
  }


  static FT_Encoding
  sfnt_find_encoding( int  platform_id,
                      int  encoding_id )
  {
    typedef struct  TEncoding
    {
      int          platform_id;
      int          encoding_id;
      FT_Encoding  encoding;

    } TEncoding;

    static
    const TEncoding  tt_encodings[] =
    {
      { TT_PLATFORM_ISO,           -1,                  FT_ENCODING_UNICODE },

      { TT_PLATFORM_APPLE_UNICODE, -1,                  FT_ENCODING_UNICODE },

      { TT_PLATFORM_MACINTOSH,     TT_MAC_ID_ROMAN,     FT_ENCODING_APPLE_ROMAN },

      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SYMBOL_CS,  FT_ENCODING_MS_SYMBOL },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UCS_4,      FT_ENCODING_UNICODE },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UNICODE_CS, FT_ENCODING_UNICODE },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SJIS,       FT_ENCODING_SJIS },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_GB2312,     FT_ENCODING_GB2312 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_BIG_5,      FT_ENCODING_BIG5 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_WANSUNG,    FT_ENCODING_WANSUNG },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_JOHAB,      FT_ENCODING_JOHAB }
    };

    const TEncoding  *cur, *limit;


    cur   = tt_encodings;
    limit = cur + sizeof ( tt_encodings ) / sizeof ( tt_encodings[0] );

    for ( ; cur < limit; cur++ )
    {
      if ( cur->platform_id == platform_id )
      {
        if ( cur->encoding_id == encoding_id ||
             cur->encoding_id == -1          )
          return cur->encoding;
      }
    }

    return FT_ENCODING_NONE;
  }


  FT_LOCAL_DEF( FT_Error )
  sfnt_init_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error         error;
    FT_Library       library = face->root.driver->root.library;
    SFNT_Service     sfnt;
    SFNT_HeaderRec   sfnt_header;

    /* for now, parameters are unused */
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    sfnt = (SFNT_Service)face->sfnt;
    if ( !sfnt )
    {
      sfnt = (SFNT_Service)FT_Get_Module_Interface( library, "sfnt" );
      if ( !sfnt )
      {
        error = SFNT_Err_Invalid_File_Format;
        goto Exit;
      }

      face->sfnt       = sfnt;
      face->goto_table = sfnt->goto_table;
    }

    FT_FACE_FIND_GLOBAL_SERVICE( face, face->psnames, POSTSCRIPT_CMAPS );

    /* check that we have a valid TrueType file */
    error = sfnt->load_sfnt_header( face, stream, face_index, &sfnt_header );
    if ( error )
      goto Exit;

    face->format_tag = sfnt_header.format_tag;
    face->num_tables = sfnt_header.num_tables;

    /* Load font directory */
    error = sfnt->load_directory( face, stream, &sfnt_header );
    if ( error )
      goto Exit;

    face->root.num_faces = face->ttc_header.count;
    if ( face->root.num_faces < 1 )
      face->root.num_faces = 1;

  Exit:
    return error;
  }


#undef  LOAD_
#define LOAD_( x )  ( ( error = sfnt->load_##x( face, stream ) ) \
                      != SFNT_Err_Ok )


  FT_LOCAL_DEF( FT_Error )
  sfnt_load_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error      error, psnames_error;
    FT_Bool       has_outline;
    FT_Bool       is_apple_sbit;

    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;

    FT_UNUSED( face_index );
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    /* Load tables */

    /* We now support two SFNT-based bitmapped font formats.  They */
    /* are recognized easily as they do not include a `glyf'       */
    /* table.                                                      */
    /*                                                             */
    /* The first format comes from Apple, and uses a table named   */
    /* `bhed' instead of `head' to store the font header (using    */
    /* the same format).  It also doesn't include horizontal and   */
    /* vertical metrics tables (i.e. `hhea' and `vhea' tables are  */
    /* missing).                                                   */
    /*                                                             */
    /* The other format comes from Microsoft, and is used with     */
    /* WinCE/PocketPC.  It looks like a standard TTF, except that  */
    /* it doesn't contain outlines.                                */
    /*                                                             */

    /* do we have outlines in there? */
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    has_outline   = FT_BOOL( face->root.internal->incremental_interface != 0 ||
                             tt_face_lookup_table( face, TTAG_glyf )    != 0 ||
                             tt_face_lookup_table( face, TTAG_CFF )     != 0 );
#else
    has_outline   = FT_BOOL( tt_face_lookup_table( face, TTAG_glyf ) != 0 ||
                             tt_face_lookup_table( face, TTAG_CFF )  != 0 );
#endif

    is_apple_sbit = 0;

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* if this font doesn't contain outlines, we try to load */
    /* a `bhed' table                                        */
    if ( !has_outline )
      is_apple_sbit = FT_BOOL( !LOAD_( bitmap_header ) );

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* load the font header (`head' table) if this isn't an Apple */
    /* sbit font file                                             */
    if ( !is_apple_sbit && LOAD_( header ) )
      goto Exit;

    /* the following tables are often not present in embedded TrueType */
    /* fonts within PDF documents, so don't check for them.            */
    (void)LOAD_( max_profile );
    (void)LOAD_( charmaps );

    /* the following tables are optional in PCL fonts -- */
    /* don't check for errors                            */
    (void)LOAD_( names );
    psnames_error = LOAD_( psnames );

    /* do not load the metrics headers and tables if this is an Apple */
    /* sbit font file                                                 */
    if ( !is_apple_sbit )
    {
      /* load the `hhea' and `hmtx' tables at once */
      error = sfnt->load_metrics( face, stream, 0 );
      if ( error )
        goto Exit;

      /* try to load the `vhea' and `vmtx' tables at once */
      error = sfnt->load_metrics( face, stream, 1 );
      if ( error )
        goto Exit;

      if ( LOAD_( os2 ) )
        goto Exit;
    }

    /* the optional tables */

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* embedded bitmap support. */
    if ( sfnt->load_sbits && LOAD_( sbits ) )
    {
      /* return an error if this font file has no outlines */
      if ( error == SFNT_Err_Table_Missing && has_outline )
        error = SFNT_Err_Ok;
      else
        goto Exit;
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    if ( LOAD_( hdmx )    ||
         LOAD_( gasp )    ||
         LOAD_( kerning ) ||
         LOAD_( pclt )    )
      goto Exit;

    face->root.family_name = tt_face_get_name( face,
                                               TT_NAME_ID_FONT_FAMILY );
    face->root.style_name  = tt_face_get_name( face,
                                               TT_NAME_ID_FONT_SUBFAMILY );

    /* now set up root fields */
    {
      FT_Face    root = &face->root;
      FT_Int32   flags = 0;
      FT_Memory  memory;


      memory = root->memory;

      /*********************************************************************/
      /*                                                                   */
      /* Compute face flags.                                               */
      /*                                                                   */
      if ( has_outline == TRUE )
        flags = FT_FACE_FLAG_SCALABLE;    /* scalable outlines */

      flags |= FT_FACE_FLAG_SFNT      |   /* SFNT file format  */
               FT_FACE_FLAG_HORIZONTAL;   /* horizontal data   */

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
      if ( psnames_error == SFNT_Err_Ok &&
           face->postscript.FormatType != 0x00030000L )
        flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

      /* fixed width font? */
      if ( face->postscript.isFixedPitch )
        flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* vertical information? */
      if ( face->vertical_info )
        flags |= FT_FACE_FLAG_VERTICAL;

      /* kerning available ? */
      if ( face->kern_pairs )
        flags |= FT_FACE_FLAG_KERNING;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
      /* Don't bother to load the tables unless somebody asks for them. */
      /* No need to do work which will (probably) not be used.          */
      if ( tt_face_lookup_table( face, TTAG_glyf ) != 0 &&
           tt_face_lookup_table( face, TTAG_fvar ) != 0 &&
           tt_face_lookup_table( face, TTAG_gvar ) != 0 )
        flags |= FT_FACE_FLAG_MULTIPLE_MASTERS;
#endif

      root->face_flags = flags;

      /*********************************************************************/
      /*                                                                   */
      /* Compute style flags.                                              */
      /*                                                                   */
      flags = 0;
      if ( has_outline == TRUE && face->os2.version != 0xFFFFU )
      {
        /* we have an OS/2 table; use the `fsSelection' field */
        if ( face->os2.fsSelection & 1 )
          flags |= FT_STYLE_FLAG_ITALIC;

        if ( face->os2.fsSelection & 32 )
          flags |= FT_STYLE_FLAG_BOLD;
      }
      else
      {
        /* this is an old Mac font, use the header field */
        if ( face->header.Mac_Style & 1 )
          flags |= FT_STYLE_FLAG_BOLD;

        if ( face->header.Mac_Style & 2 )
          flags |= FT_STYLE_FLAG_ITALIC;
      }

      root->style_flags = flags;

      /*********************************************************************/
      /*                                                                   */
      /* Polish the charmaps.                                              */
      /*                                                                   */
      /*   Try to set the charmap encoding according to the platform &     */
      /*   encoding ID of each charmap.                                    */
      /*                                                                   */

      tt_face_build_cmaps( face );  /* ignore errors */


      /* set the encoding fields */
      {
        FT_Int  m;


        for ( m = 0; m < root->num_charmaps; m++ )
        {
          FT_CharMap  charmap = root->charmaps[m];


          charmap->encoding = sfnt_find_encoding( charmap->platform_id,
                                                  charmap->encoding_id );

#if 0
          if ( root->charmap     == NULL &&
               charmap->encoding == FT_ENCODING_UNICODE )
          {
            /* set 'root->charmap' to the first Unicode encoding we find */
            root->charmap = charmap;
          }
#endif
        }
      }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

      if ( face->num_sbit_strikes )
      {
        FT_ULong  n;


        root->face_flags |= FT_FACE_FLAG_FIXED_SIZES;

#if 0
        /* XXX: I don't know criteria whether layout is horizontal */
        /*      or vertical.                                       */
        if ( has_outline.... )
        {
          ...
          root->face_flags |= FT_FACE_FLAG_VERTICAL;
        }
#endif
        root->num_fixed_sizes = (FT_Int)face->num_sbit_strikes;

        if ( FT_NEW_ARRAY( root->available_sizes, face->num_sbit_strikes ) )
          goto Exit;

        for ( n = 0 ; n < face->num_sbit_strikes ; n++ )
        {
          FT_Bitmap_Size*  bsize  = root->available_sizes + n;
          TT_SBit_Strike   strike = face->sbit_strikes + n;
          FT_UShort        fupem  = face->header.Units_Per_EM;
          FT_Short         height = (FT_Short)( face->horizontal.Ascender -
                                                face->horizontal.Descender +
                                                face->horizontal.Line_Gap );
          FT_Short         avg    = face->os2.xAvgCharWidth;


          /* assume 72dpi */
          bsize->height =
            (FT_Short)( ( height * strike->y_ppem + fupem/2 ) / fupem );
          bsize->width  =
            (FT_Short)( ( avg * strike->y_ppem + fupem/2 ) / fupem );
          bsize->size   = strike->y_ppem << 6;
          bsize->x_ppem = strike->x_ppem << 6;
          bsize->y_ppem = strike->y_ppem << 6;
        }
      }
      else

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

      {
        root->num_fixed_sizes = 0;
        root->available_sizes = 0;
      }

      /*********************************************************************/
      /*                                                                   */
      /*  Set up metrics.                                                  */
      /*                                                                   */
      if ( has_outline == TRUE )
      {
        /* XXX What about if outline header is missing */
        /*     (e.g. sfnt wrapped bitmap)?             */
        root->bbox.xMin    = face->header.xMin;
        root->bbox.yMin    = face->header.yMin;
        root->bbox.xMax    = face->header.xMax;
        root->bbox.yMax    = face->header.yMax;
        root->units_per_EM = face->header.Units_Per_EM;


        /* XXX: Computing the ascender/descender/height is very different */
        /*      from what the specification tells you.  Apparently, we    */
        /*      must be careful because                                   */
        /*                                                                */
        /*      - not all fonts have an OS/2 table; in this case, we take */
        /*        the values in the horizontal header.  However, these    */
        /*        values very often are not reliable.                     */
        /*                                                                */
        /*      - otherwise, the correct typographic values are in the    */
        /*        sTypoAscender, sTypoDescender & sTypoLineGap fields.    */
        /*                                                                */
        /*        However, certains fonts have these fields set to 0.     */
        /*        Rather, they have usWinAscent & usWinDescent correctly  */
        /*        set (but with different values).                        */
        /*                                                                */
        /*      As an example, Arial Narrow is implemented through four   */
        /*      files ARIALN.TTF, ARIALNI.TTF, ARIALNB.TTF & ARIALNBI.TTF */
        /*                                                                */
        /*      Strangely, all fonts have the same values in their        */
        /*      sTypoXXX fields, except ARIALNB which sets them to 0.     */
        /*                                                                */
        /*      On the other hand, they all have different                */
        /*      usWinAscent/Descent values -- as a conclusion, the OS/2   */
        /*      table cannot be used to compute the text height reliably! */
        /*                                                                */

        /* The ascender/descender/height are computed from the OS/2 table */
        /* when found.  Otherwise, they're taken from the horizontal      */
        /* header.                                                        */
        /*                                                                */

        root->ascender  = face->horizontal.Ascender;
        root->descender = face->horizontal.Descender;

        root->height    = (FT_Short)( root->ascender - root->descender +
                                      face->horizontal.Line_Gap );

#if 0
        /* if the line_gap is 0, we add an extra 15% to the text height --  */
        /* this computation is based on various versions of Times New Roman */
        if ( face->horizontal.Line_Gap == 0 )
          root->height = (FT_Short)( ( root->height * 115 + 50 ) / 100 );
#endif

#if 0

        /* some fonts have the OS/2 "sTypoAscender", "sTypoDescender" & */
        /* "sTypoLineGap" fields set to 0, like ARIALNB.TTF             */
        if ( face->os2.version != 0xFFFFU && root->ascender )
        {
          FT_Int  height;


          root->ascender  =  face->os2.sTypoAscender;
          root->descender = -face->os2.sTypoDescender;

          height = root->ascender + root->descender + face->os2.sTypoLineGap;
          if ( height > root->height )
            root->height = height;
        }

#endif /* 0 */

        root->max_advance_width   = face->horizontal.advance_Width_Max;

        root->max_advance_height  = (FT_Short)( face->vertical_info
                                      ? face->vertical.advance_Height_Max
                                      : root->height );

        root->underline_position  = face->postscript.underlinePosition;
        root->underline_thickness = face->postscript.underlineThickness;

        /* root->max_points   -- already set up */
        /* root->max_contours -- already set up */
      }
    }

  Exit:
    return error;
  }


#undef LOAD_


  FT_LOCAL_DEF( void )
  sfnt_done_face( TT_Face  face )
  {
    FT_Memory     memory = face->root.memory;
    SFNT_Service  sfnt   = (SFNT_Service)face->sfnt;


    if ( sfnt )
    {
      /* destroy the postscript names table if it is loaded */
      if ( sfnt->free_psnames )
        sfnt->free_psnames( face );

      /* destroy the embedded bitmaps table if it is loaded */
      if ( sfnt->free_sbits )
        sfnt->free_sbits( face );
    }

    /* freeing the kerning table */
    FT_FREE( face->kern_pairs );
    face->num_kern_pairs = 0;

    /* freeing the collection table */
    FT_FREE( face->ttc_header.offsets );
    face->ttc_header.count = 0;

    /* freeing table directory */
    FT_FREE( face->dir_tables );
    face->num_tables = 0;

    {
      FT_Stream  stream = FT_FACE_STREAM( face );


      /* simply release the 'cmap' table frame */
      FT_FRAME_RELEASE( face->cmap_table );
      face->cmap_size = 0;
    }

    /* freeing the horizontal metrics */
    FT_FREE( face->horizontal.long_metrics );
    FT_FREE( face->horizontal.short_metrics );

    /* freeing the vertical ones, if any */
    if ( face->vertical_info )
    {
      FT_FREE( face->vertical.long_metrics  );
      FT_FREE( face->vertical.short_metrics );
      face->vertical_info = 0;
    }

    /* freeing the gasp table */
    FT_FREE( face->gasp.gaspRanges );
    face->gasp.numRanges = 0;

    /* freeing the name table */
    sfnt->free_names( face );

    /* freeing the hdmx table */
    sfnt->free_hdmx( face );

    /* freeing family and style name */
    FT_FREE( face->root.family_name );
    FT_FREE( face->root.style_name );

    /* freeing sbit size table */
    FT_FREE( face->root.available_sizes );
    face->root.num_fixed_sizes = 0;

    FT_FREE( face->postscript_name );

    face->sfnt = 0;
  }


/* END */
