/***************************************************************************/
/*                                                                         */
/*  t1driver.c                                                             */
/*                                                                         */
/*    Type 1 driver interface (body).                                      */
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
#include "t1driver.h"
#include "t1gload.h"
#include "t1load.h"

#include "t1errors.h"

#ifndef T1_CONFIG_OPTION_NO_AFM
#include "t1afm.h"
#endif

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1driver


  static FT_Error
  t1_get_glyph_name( T1_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max )
  {
    FT_String*  gname;


    gname = face->type1.glyph_names[glyph_index];

    if ( buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)( ft_strlen( gname ) );


      if (len >= buffer_max)
        len = buffer_max - 1;

      FT_MEM_COPY( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_get_name_index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Uses the Type 1 font's `glyph_names' table to find a given glyph   */
  /*    name's glyph index.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the source face object.                  */
  /*                                                                       */
  /*    glyph_name :: The glyph name.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index.  0 means `undefined character code'.                  */
  /*                                                                       */
  static FT_UInt
  t1_get_name_index( T1_Face     face,
                     FT_String*  glyph_name )
  {
    FT_Int      i;
    FT_String*  gname;


    for ( i = 0; i < face->type1.num_glyphs; i++ )
    {
      gname = face->type1.glyph_names[i];

      if ( !ft_strcmp( glyph_name, gname ) )
        return (FT_UInt)i;
    }

    return 0;
  }


  static const char*
  t1_get_ps_name( T1_Face  face )
  {
    return (const char*) face->type1.font_name;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Interface                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Each driver can provide one or more extensions to the base         */
  /*    FreeType API.  These can be used to access format specific         */
  /*    features (e.g., all TrueType/OpenType resources share a common     */
  /*    file structure and common tables which can be accessed through the */
  /*    `sfnt' interface), or more simply generic ones (e.g., the          */
  /*    `postscript names' interface which can be used to retrieve the     */
  /*     PostScript name of a given glyph index).                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    driver       :: A handle to a driver object.                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    t1_interface :: A string designing the interface.  Examples are    */
  /*                    `sfnt', `post_names', `charmaps', etc.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A typeless pointer to the extension's interface (normally a table  */
  /*    of function pointers).  Returns NULL if the requested extension    */
  /*    isn't available (i.e., wasn't compiled in the driver at build      */
  /*    time).                                                             */
  /*                                                                       */
  static FT_Module_Interface
  Get_Interface( FT_Driver         driver,
                 const FT_String*  t1_interface )
  {
    FT_UNUSED( driver );
    FT_UNUSED( t1_interface );

    if ( ft_strcmp( (const char*)t1_interface, "glyph_name" ) == 0 )
      return (FT_Module_Interface)t1_get_glyph_name;

    if ( ft_strcmp( (const char*)t1_interface, "name_index" ) == 0 )
      return (FT_Module_Interface)t1_get_name_index;

    if ( ft_strcmp( (const char*)t1_interface, "postscript_name" ) == 0 )
      return (FT_Module_Interface)t1_get_ps_name;

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT
    if ( ft_strcmp( (const char*)t1_interface, "get_mm" ) == 0 )
      return (FT_Module_Interface)T1_Get_Multi_Master;

    if ( ft_strcmp( (const char*)t1_interface, "set_mm_design") == 0 )
      return (FT_Module_Interface)T1_Set_MM_Design;

    if ( ft_strcmp( (const char*)t1_interface, "set_mm_blend") == 0 )
      return (FT_Module_Interface)T1_Set_MM_Blend;
#endif
    return 0;
  }


#ifndef T1_CONFIG_OPTION_NO_AFM

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
  /*    kernings are out of scope of this method (the basic driver         */
  /*    interface is meant to be simple).                                  */
  /*                                                                       */
  /*    They can be implemented by format-specific interfaces.             */
  /*                                                                       */
  static FT_Error
  Get_Kerning( T1_Face     face,
               FT_UInt     left_glyph,
               FT_UInt     right_glyph,
               FT_Vector*  kerning )
  {
    T1_AFM*  afm;


    kerning->x = 0;
    kerning->y = 0;

    afm = (T1_AFM*)face->afm_data;
    if ( afm )
      T1_Get_Kerning( afm, left_glyph, right_glyph, kerning );

    return T1_Err_Ok;
  }


#endif /* T1_CONFIG_OPTION_NO_AFM */




  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  t1_driver_class =
  {
    {
      ft_module_font_driver      |
      ft_module_driver_scalable  |
      ft_module_driver_has_hinter,

      sizeof( FT_DriverRec ),

      "type1",
      0x10000L,
      0x20000L,

      0,   /* format interface */

      (FT_Module_Constructor)T1_Driver_Init,
      (FT_Module_Destructor) T1_Driver_Done,
      (FT_Module_Requester)  Get_Interface,
    },

    sizeof( T1_FaceRec ),
    sizeof( T1_SizeRec ),
    sizeof( T1_GlyphSlotRec ),

    (FT_Face_InitFunc)        T1_Face_Init,
    (FT_Face_DoneFunc)        T1_Face_Done,
    (FT_Size_InitFunc)        T1_Size_Init,
    (FT_Size_DoneFunc)        T1_Size_Done,
    (FT_Slot_InitFunc)        T1_GlyphSlot_Init,
    (FT_Slot_DoneFunc)        T1_GlyphSlot_Done,

    (FT_Size_ResetPointsFunc) T1_Size_Reset,
    (FT_Size_ResetPixelsFunc) T1_Size_Reset,
    (FT_Slot_LoadFunc)        T1_Load_Glyph,

#ifdef T1_CONFIG_OPTION_NO_AFM
    (FT_Face_GetKerningFunc)  0,
    (FT_Face_AttachFunc)      0,
#else
    (FT_Face_GetKerningFunc)  Get_Kerning,
    (FT_Face_AttachFunc)      T1_Read_AFM,
#endif
    (FT_Face_GetAdvancesFunc) 0
  };


/* END */
