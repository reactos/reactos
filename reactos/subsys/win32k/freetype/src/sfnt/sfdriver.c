/***************************************************************************/
/*                                                                         */
/*  sfdriver.c                                                             */
/*                                                                         */
/*    High-level SFNT driver interface (body).                             */
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


#include <freetype/internal/sfnt.h>
#include <freetype/internal/ftobjs.h>


#ifdef FT_FLAT_COMPILE

#include "sfdriver.h"
#include "ttload.h"
#include "ttsbit.h"
#include "ttpost.h"
#include "ttcmap.h"
#include "sfobjs.h"

#else

#include <freetype/src/sfnt/sfdriver.h>
#include <freetype/src/sfnt/ttload.h>
#include <freetype/src/sfnt/ttsbit.h>
#include <freetype/src/sfnt/ttpost.h>
#include <freetype/src/sfnt/ttcmap.h>
#include <freetype/src/sfnt/sfobjs.h>

#endif


#include <string.h>     /* for strcmp() */


  static
  void*  get_sfnt_table( TT_Face      face,
                         FT_Sfnt_Tag  tag )
  {
    void*  table;


    switch ( tag )
    {
    case ft_sfnt_head:
      table = &face->header;
      break;

    case ft_sfnt_hhea:
      table = &face->horizontal;
      break;

    case ft_sfnt_vhea:
      table = face->vertical_info ? &face->vertical : 0;
      break;

    case ft_sfnt_os2:
      table = face->os2.version == 0xFFFF ? 0 : &face->os2;
      break;

    case ft_sfnt_post:
      table = &face->postscript;
      break;

    case ft_sfnt_maxp:
      table = &face->max_profile;
      break;

    case ft_sfnt_pclt:
      table = face->pclt.Version ? &face->pclt : 0;
      break;

    default:
      table = 0;
    }

    return table;
  }


#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES


  static
  FT_Error  get_sfnt_glyph_name( TT_Face     face,
                                 FT_UInt     glyph_index,
                                 FT_Pointer  buffer,
                                 FT_UInt     buffer_max )
  {
    FT_String*  gname;
    FT_Error    error;
    

    error = TT_Get_PS_Name( face, glyph_index, &gname );
    if ( !error && buffer_max > 0 )
    {
      FT_UInt  len = strlen( gname );
      

      if ( len >= buffer_max )
        len = buffer_max - 1;
        
      MEM_Copy( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }
    
    return error;
  }                                  


#endif /* TT_CONFIG_OPTION_POSTSCRIPT_NAMES */


  static
  FT_Module_Interface  SFNT_Get_Interface( FT_Module    module,
                                           const char*  interface )
  {
    FT_UNUSED( module );

    if ( strcmp( interface, "get_sfnt" ) == 0 )
      return (FT_Module_Interface)get_sfnt_table;

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
    if ( strcmp( interface, "glyph_name" ) == 0 )
      return (FT_Module_Interface)get_sfnt_glyph_name;
#endif
    return 0;
  }


  static
  const SFNT_Interface  sfnt_interface =
  {
    TT_Goto_Table,

    SFNT_Init_Face,
    SFNT_Load_Face,
    SFNT_Done_Face,
    SFNT_Get_Interface,

    TT_Load_Any,
    TT_Load_SFNT_Header,
    TT_Load_Directory,

    TT_Load_Header,
    TT_Load_Metrics_Header,
    TT_Load_CMap,
    TT_Load_MaxProfile,
    TT_Load_OS2,
    TT_Load_PostScript,

    TT_Load_Names,
    TT_Free_Names,

    TT_Load_Hdmx,
    TT_Free_Hdmx,

    TT_Load_Kern,
    TT_Load_Gasp,
    TT_Load_PCLT,

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* see `ttsbit.h' */
    TT_Load_SBit_Strikes,
    TT_Load_SBit_Image,
    TT_Free_SBit_Strikes,

#else /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    0,
    0,
    0,

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES

    /* see `ttpost.h' */
    TT_Get_PS_Name,
    TT_Free_Post_Names,

#else /* TT_CONFIG_OPTION_POSTSCRIPT_NAMES */

    0,
    0,

#endif /* TT_CONFIG_OPTION_POSTSCRIPT_NAMES */

    /* see `ttcmap.h' */
    TT_CharMap_Load,
    TT_CharMap_Free,
  };


  const
  FT_Module_Class  sfnt_module_class =
  {
    0,  /* not a font driver or renderer */
    sizeof( FT_ModuleRec ),

    "sfnt",     /* driver name                            */
    0x10000L,   /* driver version 1.0                     */
    0x20000L,   /* driver requires FreeType 2.0 or higher */

    (const void*)&sfnt_interface,  /* module specific interface */

    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  SFNT_Get_Interface
  };


/* END */
