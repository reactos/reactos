/***************************************************************************/
/*                                                                         */
/*  cidriver.c                                                             */
/*                                                                         */
/*    CID driver interface (body).                                         */
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


#ifdef FT_FLAT_COMPILE

#include "cidriver.h"
#include "cidgload.h"

#else

#include <freetype/src/cid/cidriver.h>
#include <freetype/src/cid/cidgload.h>

#endif


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/psnames.h>

#include <string.h>         /* for strcmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ciddriver


  static
  FT_Module_Interface  CID_Get_Interface( FT_Driver         driver,
                                          const FT_String*  interface )
  {
    FT_UNUSED( driver );
    FT_UNUSED( interface );

    return 0;
  }


#if 0 /* unimplemented yet */

  static
  FT_Error  cid_Get_Kerning( T1_Face     face,
                             FT_UInt     left_glyph,
                             FT_UInt     right_glyph,
                             FT_Vector*  kerning )
  {
    CID_AFM*  afm;


    kerning->x = 0;
    kerning->y = 0;

    afm = (CID_AFM*)face->afm_data;
    if ( afm )
      CID_Get_Kerning( afm, left_glyph, right_glyph, kerning );

    return T1_Err_Ok;
  }


#endif /* 0 */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Cid_Get_Char_Index                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Uses a charmap to return a given character code's glyph index.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charmap  :: A handle to the source charmap object.                 */
  /*                                                                       */
  /*    charcode :: The character code.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index.  0 means `undefined character code'.                  */
  /*                                                                       */
  static
  FT_UInt  CID_Get_Char_Index( FT_CharMap  charmap,
                               FT_Long     charcode )
  {
    T1_Face             face;
    FT_UInt             result = 0;
    PSNames_Interface*  psnames;


    face = (T1_Face)charmap->face;
    psnames = (PSNames_Interface*)face->psnames;
    if ( psnames )
      switch ( charmap->encoding )
      {
        /*******************************************************************/
        /*                                                                 */
        /* Unicode encoding support                                        */
        /*                                                                 */
      case ft_encoding_unicode:
        /* use the `PSNames' module to synthetize the Unicode charmap */
        result = psnames->lookup_unicode( &face->unicode_map,
                                          (FT_ULong)charcode );

        /* the function returns 0xFFFF if the Unicode charcode has */
        /* no corresponding glyph.                                 */
        if ( result == 0xFFFF )
          result = 0;
        goto Exit;

        /*******************************************************************/
        /*                                                                 */
        /* Custom Type 1 encoding                                          */
        /*                                                                 */
      case ft_encoding_adobe_custom:
        {
          T1_Encoding*  encoding = &face->type1.encoding;


          if ( charcode >= encoding->code_first &&
               charcode <= encoding->code_last  )
            result = encoding->char_index[charcode];
          goto Exit;
        }

        /*******************************************************************/
        /*                                                                 */
        /* Adobe Standard & Expert encoding support                        */
        /*                                                                 */
      default:
        if ( charcode < 256 )
        {
          FT_UInt      code;
          FT_Int       n;
          const char*  glyph_name;


          code = psnames->adobe_std_encoding[charcode];
          if ( charmap->encoding == ft_encoding_adobe_expert )
            code = psnames->adobe_expert_encoding[charcode];

          glyph_name = psnames->adobe_std_strings( code );
          if ( !glyph_name )
            break;

          for ( n = 0; n < face->type1.num_glyphs; n++ )
          {
            const char*  gname = face->type1.glyph_names[n];


            if ( gname && gname[0] == glyph_name[0] &&
                 strcmp( gname, glyph_name ) == 0   )
            {
              result = n;
              break;
            }
          }
        }
      }

  Exit:
    return result;
  }


  const FT_Driver_Class  t1cid_driver_class =
  {
    /* first of all, the FT_Module_Class fields */
    {
      ft_module_font_driver | ft_module_driver_scalable,
      sizeof( FT_DriverRec ),
      "t1cid",   /* module name           */
      0x10000L,  /* version 1.0 of driver */
      0x20000L,  /* requires FreeType 2.0 */

      0,

      (FT_Module_Constructor)CID_Init_Driver,
      (FT_Module_Destructor) CID_Done_Driver,
      (FT_Module_Requester)  CID_Get_Interface
    },

    /* then the other font drivers fields */
    sizeof( CID_FaceRec ),
    sizeof( CID_SizeRec ),
    sizeof( CID_GlyphSlotRec ),

    (FTDriver_initFace)     CID_Init_Face,
    (FTDriver_doneFace)     CID_Done_Face,

    (FTDriver_initSize)     0,
    (FTDriver_doneSize)     0,
    (FTDriver_initGlyphSlot)0,
    (FTDriver_doneGlyphSlot)0,

    (FTDriver_setCharSizes) 0,
    (FTDriver_setPixelSizes)0,

    (FTDriver_loadGlyph)    CID_Load_Glyph,
    (FTDriver_getCharIndex) CID_Get_Char_Index,

    (FTDriver_getKerning)   0,
    (FTDriver_attachFile)   0,

    (FTDriver_getAdvances)  0
  };


#ifdef FT_CONFIG_OPTION_DYNAMIC_DRIVERS


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    getDriverClass                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used when compiling the TrueType driver as a      */
  /*    shared library (`.DLL' or `.so').  It will be used by the          */
  /*    high-level library of FreeType to retrieve the address of the      */
  /*    driver's generic interface.                                        */
  /*                                                                       */
  /*    It shouldn't be implemented in a static build, as each driver must */
  /*    have the same function as an exported entry point.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of the TrueType's driver generic interface.  The       */
  /*    format-specific interface can then be retrieved through the method */
  /*    interface->get_format_interface.                                   */
  /*                                                                       */
  EXPORT_FUNC( FT_Driver_Class* )  getDriverClass( void )
  {
    return &t1cid_driver_class;
  }


#endif /* CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
