/***************************************************************************/
/*                                                                         */
/*  t2objs.c                                                               */
/*                                                                         */
/*    OpenType objects manager (body).                                     */
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
#include <freetype/internal/ftcalc.h>
#include <freetype/internal/ftstream.h>
#include <freetype/fterrors.h>
#include <freetype/ttnameid.h>
#include <freetype/tttags.h>

#include <freetype/internal/sfnt.h>
#include <freetype/internal/psnames.h>


#ifdef FT_FLAT_COMPILE

#include "t2objs.h"
#include "t2load.h"

#else

#include <cff/t2objs.h>
#include <cff/t2load.h>

#endif


#include <freetype/internal/t2errors.h>

#include <string.h>         /* for strlen() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t2objs


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

  static
  FT_String*  T2_StrCopy( FT_Memory         memory,
                          const FT_String*  source )
  {
    FT_Error    error;
    FT_String*  result = 0;
    FT_Int      len = (FT_Int)strlen( source );


    if ( !ALLOC( result, len + 1 ) )
    {
      MEM_Copy( result, source, len );
      result[len] = 0;
    }
    return result;
  }


#if 0

  /* this function is used to build a Unicode charmap from the glyph names */
  /* in a file                                                             */
  static
  FT_Error  CFF_Build_Unicode_Charmap( T2_Face             face,
                                       FT_ULong            base_offset,
                                       PSNames_Interface*  psnames )
  {
    CFF_Font*       font = (CFF_Font*)face->extra.data;
    FT_Memory       memory = FT_FACE_MEMORY(face);
    FT_UInt         n, num_glyphs = face->root.num_glyphs;
    const char**    glyph_names;
    FT_Error        error;
    CFF_Font_Dict*  dict = &font->top_font.font_dict;
    FT_ULong        charset_offset;
    FT_Byte         format;
    FT_Stream       stream = face->root.stream;


    charset_offset = dict->charset_offset;
    if ( !charset_offset )
    {
      FT_ERROR(( "CFF.Build_Unicode_Charmap: charset table is missing\n" ));
      error = T2_Err_Invalid_File_Format;
      goto Exit;
    }

    /* allocate the charmap */
    if ( ALLOC( face->charmap, ...

    /* seek to charset table and allocate glyph names table */
    if ( FILE_Seek( base_offset + charset_offset )           ||
         ALLOC_ARRAY( glyph_names, num_glyphs, const char* ) )
      goto Exit;

    /* now, read each glyph name and store it in the glyph name table */
    if ( READ_Byte( format ) )
      goto Fail;

    switch ( format )
    {
      case 0:  /* format 0 - one SID per glyph */
        {
          const char**  gname = glyph_names;
          const char**  limit = gname + num_glyphs;

          if ( ACCESS_Frame( num_glyphs*2 ) )
            goto Fail;

          for ( ; gname < limit; gname++ )
            gname[0] = T2_Get_String( &font->string_index,
                                      GET_UShort(),
                                      psnames );
          FORGET_Frame();
          break;
        }

      case 1:  /* format 1 - sequential ranges                    */
      case 2:  /* format 2 - sequential ranges with 16-bit counts */
        {
          const char**  gname = glyph_names;
          const char**  limit = gname + num_glyphs;
          FT_UInt       len = 3;

          if (format == 2)
            len++;

          while (gname < limit)
          {
            FT_UInt   first;
            FT_UInt   count;

            if ( ACCESS_Frame( len ) )
              goto Fail;

            first = GET_UShort();
            if (format == 3)
              count = GET_UShort();
            else
              count = GET_Byte();

            FORGET_Frame();

            for ( ; count > 0; count-- )
            {
              gname[0] = T2_Get_String( &font->string_index,
                                        first,
                                        psnames );
              gname++;
              first++;
            }
          }
          break;
        }

      default:   /* unknown charset format! */
        FT_ERROR(( "CFF: unknown charset format!\n" ));
        error = T2_Err_Invalid_File_Format;
        goto Fail;
    }

    /* all right, the glyph names were loaded, we now need to create */
    /* the corresponding unicode charmap..                           */

  Fail:
    for ( n = 0; n < num_glyphs; n++ )
      FREE( glyph_names[n] );

    FREE( glyph_names );

  Exit:
    return error;
  }

#endif /* 0 */


  static
  FT_Encoding  find_encoding( int  platform_id,
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
      { TT_PLATFORM_ISO,           -1,                  ft_encoding_unicode },

      { TT_PLATFORM_APPLE_UNICODE, -1,                  ft_encoding_unicode },

      { TT_PLATFORM_MACINTOSH,     TT_MAC_ID_ROMAN,     ft_encoding_apple_roman },

      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UNICODE_CS, ft_encoding_unicode },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SJIS,       ft_encoding_sjis },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_GB2312,     ft_encoding_gb2312 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_BIG_5,      ft_encoding_big5 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_WANSUNG,    ft_encoding_wansung },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_JOHAB,      ft_encoding_johab }
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

    return ft_encoding_none;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T2_Init_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given OpenType face object.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The source font stream.                              */
  /*                                                                       */
  /*    face_index :: The index of the font face in the resource.          */
  /*                                                                       */
  /*    num_params :: Number of additional generic parameters.  Ignored.   */
  /*                                                                       */
  /*    params     :: Additional generic parameters.  Ignored.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The newly built face object.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_DEF
  FT_Error  T2_Init_Face( FT_Stream      stream,
                          T2_Face        face,
                          FT_Int         face_index,
                          FT_Int         num_params,
                          FT_Parameter*  params )
  {
    FT_Error            error;
    SFNT_Interface*     sfnt;
    PSNames_Interface*  psnames;
    FT_Bool             pure_cff    = 1;
    FT_Bool             sfnt_format = 0;


    sfnt = (SFNT_Interface*)FT_Get_Module_Interface(
             face->root.driver->root.library, "sfnt" );
    if ( !sfnt )
      goto Bad_Format;

    psnames = (PSNames_Interface*)FT_Get_Module_Interface(
                face->root.driver->root.library, "psnames" );

    /* create input stream from resource */
    if ( FILE_Seek( 0 ) )
      goto Exit;

    /* check that we have a valid OpenType file */
    error = sfnt->init_face( stream, face, face_index, num_params, params );
    if ( !error )
    {
      if ( face->format_tag != 0x4F54544FL )  /* `OTTO'; OpenType/CFF font */
      {
        FT_TRACE2(( "[not a valid OpenType/CFF font]\n" ));
        goto Bad_Format;
      }

      /* If we are performing a simple font format check, exit immediately */
      if ( face_index < 0 )
        return T2_Err_Ok;

      sfnt_format = 1;

      /* now, the font can be either an OpenType/CFF font, or a SVG CEF */
      /* font in the later case; it doesn't have a `head' table         */
      error = face->goto_table( face, TTAG_head, stream, 0 );
      if ( !error )
      {
        pure_cff = 0;

        /* Load font directory */
        error = sfnt->load_face( stream, face,
                                 face_index, num_params, params );
        if ( error )
          goto Exit;
      }
      else
      {
        /* load the `cmap' table by hand */
        error = sfnt->load_charmaps( face, stream );
        if ( error )
          goto Exit;

        /* XXX: for now, we don't load the GPOS table, as OpenType Layout */
        /* support will be added later to FreeType 2 as a separate module */
      }

      /* now, load the CFF part of the file */
      error = face->goto_table( face, TTAG_CFF, stream, 0 );
      if ( error )
        goto Exit;
    }
    else
    {
      /* rewind to start of file; we are going to load a pure-CFF font */
      (void)FILE_Seek( 0 );
      error = FT_Err_Ok;
    }

    /* now load and parse the CFF table in the file */
    {
      CFF_Font*  cff;
      FT_Memory  memory = face->root.memory;
      FT_Face    root;
      FT_UInt    flags;
      FT_ULong   base_offset;


      if ( ALLOC( cff, sizeof ( *cff ) ) )
        goto Exit;

      base_offset = FILE_Pos();

      face->extra.data = cff;
      error = T2_Load_CFF_Font( stream, face_index, cff );
      if ( error )
        goto Exit;

      /* Complement the root flags with some interesting information. */
      /* Note that this is only necessary for pure CFF and CEF fonts  */

      root = &face->root;
      if ( pure_cff )
      {
        CFF_Font_Dict*  dict = &cff->top_font.font_dict;


        /* we need the `PSNames' module for pure-CFF and CEF formats */
        if ( !psnames )
        {
          FT_ERROR(( "T2_Init_Face:" ));
          FT_ERROR(( " cannot open CFF & CEF fonts\n" ));
          FT_ERROR(( "             " ));
          FT_ERROR(( " without the `PSNames' module\n" ));
          goto Bad_Format;
        }

        /* compute number of glyphs */
        if ( dict->cid_registry )
          root->num_glyphs = dict->cid_count;
        else
          root->num_glyphs = cff->charstrings_index.count;

        /* set global bbox, as well as EM size */
        root->units_per_EM = (FT_UInt)FT_DivFix( 1000L << 16,
                                                 dict->font_matrix.yy ) >> 16;
        root->bbox      = dict->font_bbox;
        root->ascender  = (FT_Short)root->bbox.yMax;
        root->descender = (FT_Short)root->bbox.yMin;

        /* retrieve font family & style name */
        root->family_name = T2_Get_Name( &cff->name_index, face_index );
        if ( dict->cid_registry )
        {
          root->style_name = T2_StrCopy( memory, "Regular" );  /* XXXX */
        }
        else
        {
          root->style_name = T2_Get_String( &cff->string_index,
                                            dict->weight,
                                            psnames );
        }

        /*******************************************************************/
        /*                                                                 */
        /* Compute face flags.                                             */
        /*                                                                 */
        flags = FT_FACE_FLAG_SCALABLE  |    /* scalable outlines */
                FT_FACE_FLAG_HORIZONTAL;    /* horizontal data   */

        if ( sfnt_format )
          flags |= FT_FACE_FLAG_SFNT;

        /* fixed width font? */
        if ( dict->is_fixed_pitch )
          flags |= FT_FACE_FLAG_FIXED_WIDTH;

/* XXXX: WE DO NOT SUPPORT KERNING METRICS IN THE GPOS TABLE FOR NOW */
#if 0
        /* kerning available? */
        if ( face->kern_pairs )
          flags |= FT_FACE_FLAG_KERNING;
#endif

        root->face_flags = flags;

        /*******************************************************************/
        /*                                                                 */
        /* Compute style flags.                                            */
        /*                                                                 */
        flags = 0;

        if ( dict->italic_angle )
          flags |= FT_STYLE_FLAG_ITALIC;

        /* XXX: may not be correct */
        if ( cff->top_font.private_dict.force_bold )
          flags |= FT_STYLE_FLAG_BOLD;

        root->style_flags = flags;

        /* set the charmaps if any */
        if ( sfnt_format )
        {
          /*****************************************************************/
          /*                                                               */
          /* Polish the charmaps.                                          */
          /*                                                               */
          /*   Try to set the charmap encoding according to the platform & */
          /*   encoding ID of each charmap.                                */
          /*                                                               */
          TT_CharMap  charmap;
          FT_Int      n;


          charmap            = face->charmaps;
          root->num_charmaps = face->num_charmaps;

          /* allocate table of pointers */
          if ( ALLOC_ARRAY( root->charmaps, root->num_charmaps, FT_CharMap ) )
            goto Exit;

          for ( n = 0; n < root->num_charmaps; n++, charmap++ )
          {
            FT_Int  platform = charmap->cmap.platformID;
            FT_Int  encoding = charmap->cmap.platformEncodingID;


            charmap->root.face        = (FT_Face)face;
            charmap->root.platform_id = platform;
            charmap->root.encoding_id = encoding;
            charmap->root.encoding    = find_encoding( platform, encoding );

            /* now, set root->charmap with a unicode charmap */
            /* wherever available                            */
            if ( !root->charmap                                &&
                 charmap->root.encoding == ft_encoding_unicode )
              root->charmap = (FT_CharMap)charmap;

            root->charmaps[n] = (FT_CharMap)charmap;
          }
        }
      }
    }

  Exit:
    return error;

  Bad_Format:
    error = FT_Err_Unknown_File_Format;
    goto Exit;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T2_Done_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given face object.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A pointer to the face object to destroy.                   */
  /*                                                                       */
  LOCAL_DEF
  void  T2_Done_Face( T2_Face  face )
  {
    FT_Memory        memory = face->root.memory;
    SFNT_Interface*  sfnt   = (SFNT_Interface*)face->sfnt;


    if ( sfnt )
      sfnt->done_face( face );

    {
      CFF_Font*  cff = (CFF_Font*)face->extra.data;


      if ( cff )
      {
        T2_Done_CFF_Font( cff );
        FREE( face->extra.data );
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T2_Init_Driver                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given OpenType driver object.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T2_Init_Driver( T2_Driver  driver )
  {
    /* init extension registry if needed */

#ifdef TT_CONFIG_OPTION_EXTEND_ENGINE

    return TT_Init_Extensions( driver );

#else

    FT_UNUSED( driver );

    return T2_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T2_Done_Driver                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given OpenType driver.                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target OpenType driver.                  */
  /*                                                                       */
  LOCAL_FUNC
  void  T2_Done_Driver( T2_Driver  driver )
  {
    /* destroy extensions registry if needed */

#ifdef TT_CONFIG_OPTION_EXTEND_ENGINE

    TT_Done_Extensions( driver );

#else

    FT_UNUSED( driver );

#endif
  }


/* END */
