/***************************************************************************/
/*                                                                         */
/*  ahmodule.c                                                             */
/*                                                                         */
/*    Auto-hinting module implementation (declaration).                    */
/*                                                                         */
/*  Copyright 2000 Catharon Productions Inc.                               */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#include <freetype/ftmodule.h>


#ifdef FT_FLAT_COMPILE

#include "ahhint.h"

#else

#include <freetype/src/autohint/ahhint.h>

#endif


  typedef struct  FT_AutoHinterRec_
  {
    FT_ModuleRec  root;
    AH_Hinter*    hinter;

  } FT_AutoHinterRec;


  static
  FT_Error  ft_autohinter_init( FT_AutoHinter  module )
  {
    return ah_hinter_new( module->root.library, &module->hinter );
  }


  static
  void  ft_autohinter_done( FT_AutoHinter  module )
  {
    ah_hinter_done( module->hinter );
  }


  static
  FT_Error  ft_autohinter_load( FT_AutoHinter  module,
                                FT_GlyphSlot   slot,
                                FT_Size        size,
                                FT_UInt        glyph_index,
                                FT_ULong       load_flags )
  {
    return ah_hinter_load_glyph( module->hinter,
                                 slot, size, glyph_index, load_flags );
  }


  static
  void   ft_autohinter_reset( FT_AutoHinter  module,
                              FT_Face        face )
  {
    UNUSED( module );

    if ( face->autohint.data )
      ah_hinter_done_face_globals( (AH_Face_Globals*)(face->autohint.data) );
  }


  static
  void  ft_autohinter_get_globals( FT_AutoHinter  module,
                                   FT_Face        face,
                                   void**         global_hints,
                                   long*          global_len )
  {
    ah_hinter_get_global_hints( module->hinter, face,
                                global_hints, global_len );
  }


  static
  void  ft_autohinter_done_globals( FT_AutoHinter  module,
                                    void*          global_hints )
  {
    ah_hinter_done_global_hints( module->hinter, global_hints );
  }


  static
  const FT_AutoHinter_Interface  autohinter_interface =
  {
    ft_autohinter_reset,
    ft_autohinter_load,
    ft_autohinter_get_globals,
    ft_autohinter_done_globals
  };


  const FT_Module_Class  autohint_module_class =
  {
    ft_module_hinter,
    sizeof ( FT_AutoHinterRec ),

    "autohinter",
    0x10000L,   /* version 1.0 of the autohinter  */
    0x20000L,   /* requires FreeType 2.0 or above */

    (const void*)&autohinter_interface,

    (FT_Module_Constructor)ft_autohinter_init,
    (FT_Module_Destructor) ft_autohinter_done,
    (FT_Module_Requester)  0
  };


/* END */
