/***************************************************************************/
/*                                                                         */
/*  t2driver.c                                                             */
/*                                                                         */
/*    OpenType font driver implementation (body).                          */
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


#include <freetype/freetype.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/sfnt.h>
#include <freetype/ttnameid.h>

#include <freetype/internal/t2errors.h>


#ifdef FT_FLAT_COMPILE

#include "t2driver.h"
#include "t2gload.h"

#else

#include <cff/t2driver.h>
#include <cff/t2gload.h>

#endif


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t2driver


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                          F A C E S                              ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#undef  PAIR_TAG
#define PAIR_TAG( left, right )  ( ( (FT_ULong)left << 16 ) | \
                                     (FT_ULong)right        )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Kerning                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to return the kerning vector between two      */
  /*    glyphs of the same face.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to the source face object.                 */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    kerning     :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this function.  Other layouts, or more sophisticated  */
  /*    kernings, are out of scope of this method (the basic driver        */
  /*    interface is meant to be simple).                                  */
  /*                                                                       */
  /*    They can be implemented by format-specific interfaces.             */
  /*                                                                       */
  static
  FT_Error  Get_Kerning( TT_Face     face,
                         FT_UInt     left_glyph,
                         FT_UInt     right_glyph,
                         FT_Vector*  kerning )
  {
    TT_Kern_0_Pair*  pair;


    if ( !face )
      return T2_Err_Invalid_Face_Handle;

    kerning->x = 0;
    kerning->y = 0;

    if ( face->kern_pairs )
    {
      /* there are some kerning pairs in this font file! */
      FT_ULong  search_tag = PAIR_TAG( left_glyph, right_glyph );
      FT_Long   left, right;


      left  = 0;
      right = face->num_kern_pairs - 1;

      while ( left <= right )
      {
        FT_Int    middle = left + ( ( right - left ) >> 1 );
        FT_ULong  cur_pair;


        pair     = face->kern_pairs + middle;
        cur_pair = PAIR_TAG( pair->left, pair->right );

        if ( cur_pair == search_tag )
          goto Found;

        if ( cur_pair < search_tag )
          left = middle + 1;
        else
          right = middle - 1;
      }
    }

  Exit:
    return T2_Err_Ok;

  Found:
    kerning->x = pair->value;
    goto Exit;
  }


#undef PAIR_TAG


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Load_Glyph                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to load a glyph within a given glyph slot.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot        :: A handle to the target slot object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled, loaded, etc.                        */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FTLOAD_??? constants can be used to control the     */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Load_Glyph( T2_GlyphSlot  slot,
                        T2_Size       size,
                        FT_UShort     glyph_index,
                        FT_UInt       load_flags )
  {
    FT_Error  error;


    if ( !slot )
      return T2_Err_Invalid_Glyph_Handle;

    /* check whether we want a scaled outline or bitmap */
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    /* reset the size object if necessary */
    if ( size )
    {
      /* these two object must have the same parent */
      if ( size->face != slot->root.face )
        return T2_Err_Invalid_Face_Handle;
    }

    /* now load the glyph outline if necessary */
    error = T2_Load_Glyph( slot, size, glyph_index, load_flags );

    /* force drop-out mode to 2 - irrelevant now */
    /* slot->outline.dropout_mode = 2; */

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****             C H A R A C T E R   M A P P I N G S                 ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Char_Index                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Uses a charmap to return a given character code's glyph index.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charmap  :: A handle to the source charmap object.                 */
  /*    charcode :: The character code.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index.  0 means `undefined character code'.                  */
  /*                                                                       */
  static
  FT_UInt  t2_get_char_index( TT_CharMap  charmap,
                              FT_Long     charcode )
  {
    FT_Error       error;
    T2_Face        face;
    TT_CMapTable*  cmap;


    cmap = &charmap->cmap;
    face = (T2_Face)charmap->root.face;

    /* Load table if needed */
    if ( !cmap->loaded )
    {
      SFNT_Interface*  sfnt = (SFNT_Interface*)face->sfnt;


      error = sfnt->load_charmap( face, cmap, face->root.stream );
      if ( error )
        return 0;

      cmap->loaded = TRUE;
    }

    return ( cmap->get_index ? cmap->get_index( cmap, charcode ) : 0 );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                D R I V E R  I N T E R F A C E                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  static
  FT_Module_Interface  t2_get_interface( T2_Driver    driver,
                                         const char*  interface )
  {
    FT_Module  sfnt;


    /* we simply pass our request to the `sfnt' module */
    sfnt = FT_Get_Module( driver->root.root.library, "sfnt" );

    return sfnt ? sfnt->clazz->get_interface( sfnt, interface ) : 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  const FT_Driver_Class   cff_driver_class =
  {
    /* begin with the FT_Module_Class fields */
    {
      ft_module_font_driver | ft_module_driver_scalable,
      sizeof( T2_DriverRec ),
      "cff",
      0x10000L,
      0x20000L,

      0,   /* module-specific interface */

      (FT_Module_Constructor)T2_Init_Driver,
      (FT_Module_Destructor) T2_Done_Driver,
      (FT_Module_Requester)  t2_get_interface,
    },

    /* now the specific driver fields */
    sizeof( TT_FaceRec ),
    sizeof( FT_SizeRec ),
    sizeof( T2_GlyphSlotRec ),

    (FTDriver_initFace)     T2_Init_Face,
    (FTDriver_doneFace)     T2_Done_Face,
    (FTDriver_initSize)     0,
    (FTDriver_doneSize)     0,
    (FTDriver_initGlyphSlot)0,
    (FTDriver_doneGlyphSlot)0,

    (FTDriver_setCharSizes) 0,
    (FTDriver_setPixelSizes)0,

    (FTDriver_loadGlyph)    Load_Glyph,
    (FTDriver_getCharIndex) t2_get_char_index,

    (FTDriver_getKerning)   Get_Kerning,
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
    return &cff_driver_class;
  }


#endif /* CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
