/***************************************************************************/
/*                                                                         */
/*  t42drivr.c                                                             */
/*                                                                         */
/*    High-level Type 42 driver interface (body).                          */
/*                                                                         */
/*  Copyright 2002, 2003 by Roberto Alameda.                               */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This driver implements Type42 fonts as described in the               */
  /* Technical Note #5012 from Adobe, with these limitations:              */
  /*                                                                       */
  /* 1) CID Fonts are not currently supported.                             */
  /* 2) Incremental fonts making use of the GlyphDirectory keyword         */
  /*    will be loaded, but the rendering will be using the TrueType       */
  /*    tables.                                                            */
  /* 3) The sfnts array is expected to be ASCII, not binary.               */
  /* 4) As for Type1 fonts, CDevProc is not supported.                     */
  /* 5) The Metrics dictionary is not supported.                           */
  /* 6) AFM metrics are not supported.                                     */
  /*                                                                       */
  /* In other words, this driver supports Type42 fonts derived from        */
  /* TrueType fonts in a non-CID manner, as done by usual conversion       */
  /* programs.                                                             */
  /*                                                                       */
  /*************************************************************************/


#include "t42drivr.h"
#include "t42objs.h"
#include "t42error.h"
#include FT_INTERNAL_DEBUG_H


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t42


  static FT_Error
  t42_get_glyph_name( T42_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    FT_String*  gname;


    gname = face->type1.glyph_names[glyph_index];

    if ( buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)( ft_strlen( gname ) );


      if ( len >= buffer_max )
        len = buffer_max - 1;

      FT_MEM_COPY( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    return T42_Err_Ok;
  }


  static const char*
  t42_get_ps_name( T42_Face  face )
  {
    return (const char*)face->type1.font_name;
  }


  static FT_UInt
  t42_get_name_index( T42_Face    face,
                      FT_String*  glyph_name )
  {
    FT_Int      i;
    FT_String*  gname;


    for ( i = 0; i < face->type1.num_glyphs; i++ )
    {
      gname = face->type1.glyph_names[i];

      if ( !ft_strcmp( glyph_name, gname ) )
        return ft_atoi( (const char *)face->type1.charstrings[i] );
    }

    return 0;
  }


  static FT_Module_Interface
  T42_Get_Interface( FT_Driver         driver,
                     const FT_String*  t42_interface )
  {
    FT_UNUSED( driver );

    /* Any additional interface are defined here */
    if (ft_strcmp( (const char*)t42_interface, "glyph_name" ) == 0 )
      return (FT_Module_Interface)t42_get_glyph_name;

    if ( ft_strcmp( (const char*)t42_interface, "name_index" ) == 0 )
      return (FT_Module_Interface)t42_get_name_index;

    if ( ft_strcmp( (const char*)t42_interface, "postscript_name" ) == 0 )
      return (FT_Module_Interface)t42_get_ps_name;

    return 0;
  }


  const FT_Driver_ClassRec  t42_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      FT_MODULE_DRIVER_HAS_HINTER,
#else
      0,
#endif

      sizeof ( T42_DriverRec ),

      "type42",
      0x10000L,
      0x20000L,

      0,    /* format interface */

      (FT_Module_Constructor)T42_Driver_Init,
      (FT_Module_Destructor) T42_Driver_Done,
      (FT_Module_Requester)  T42_Get_Interface,
    },

    sizeof ( T42_FaceRec ),
    sizeof ( T42_SizeRec ),
    sizeof ( T42_GlyphSlotRec ),

    (FT_Face_InitFunc)        T42_Face_Init,
    (FT_Face_DoneFunc)        T42_Face_Done,
    (FT_Size_InitFunc)        T42_Size_Init,
    (FT_Size_DoneFunc)        T42_Size_Done,
    (FT_Slot_InitFunc)        T42_GlyphSlot_Init,
    (FT_Slot_DoneFunc)        T42_GlyphSlot_Done,

    (FT_Size_ResetPointsFunc) T42_Size_SetChars,
    (FT_Size_ResetPixelsFunc) T42_Size_SetPixels,
    (FT_Slot_LoadFunc)        T42_GlyphSlot_Load,

    (FT_Face_GetKerningFunc)  0,
    (FT_Face_AttachFunc)      0,

    (FT_Face_GetAdvancesFunc) 0
  };


/* END */
