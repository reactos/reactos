/***************************************************************************/
/*                                                                         */
/*  ahmodule.c                                                             */
/*                                                                         */
/*    Auto-hinting module implementation (declaration).                    */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004 Catharon Productions Inc.        */
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


#include <ft2build.h>
#include FT_MODULE_H
#include "ahhint.h"


#ifdef  DEBUG_HINTER
   AH_Hinter  ah_debug_hinter       = NULL;
   FT_Bool    ah_debug_disable_horz = 0;
   FT_Bool    ah_debug_disable_vert = 0;
#endif

  typedef struct  FT_AutoHinterRec_
  {
    FT_ModuleRec  root;
    AH_Hinter     hinter;

  } FT_AutoHinterRec;


  FT_CALLBACK_DEF( FT_Error )
  ft_autohinter_init( FT_Module  module )       /* FT_AutoHinter */
  {
    FT_AutoHinter  autohinter = (FT_AutoHinter)module;
    FT_Error       error;


    error = ah_hinter_new( module->library, &autohinter->hinter );

#ifdef DEBUG_HINTER
    if ( !error )
      ah_debug_hinter = autohinter->hinter;
#endif

    return error;
  }


  FT_CALLBACK_DEF( void )
  ft_autohinter_done( FT_Module  module )
  {
    FT_AutoHinter  autohinter = (FT_AutoHinter)module;


    ah_hinter_done( autohinter->hinter );

#ifdef DEBUG_HINTER
    ah_debug_hinter = NULL;
#endif
  }


  FT_CALLBACK_DEF( FT_Error )
  ft_autohinter_load_glyph( FT_AutoHinter  module,
                            FT_GlyphSlot   slot,
                            FT_Size        size,
                            FT_UInt        glyph_index,
                            FT_Int32       load_flags )
  {
    return ah_hinter_load_glyph( module->hinter,
                                 slot, size, glyph_index, load_flags );
  }


  FT_CALLBACK_DEF( void )
  ft_autohinter_reset_globals( FT_AutoHinter  module,
                               FT_Face        face )
  {
    FT_UNUSED( module );

    if ( face->autohint.data )
      ah_hinter_done_face_globals( (AH_Face_Globals)(face->autohint.data) );
  }


  FT_CALLBACK_DEF( void )
  ft_autohinter_get_globals( FT_AutoHinter  module,
                             FT_Face        face,
                             void**         global_hints,
                             long*          global_len )
  {
    ah_hinter_get_global_hints( module->hinter, face,
                                global_hints, global_len );
  }


  FT_CALLBACK_DEF( void )
  ft_autohinter_done_globals( FT_AutoHinter  module,
                              void*          global_hints )
  {
    ah_hinter_done_global_hints( module->hinter, global_hints );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_AutoHinter_ServiceRec  ft_autohinter_service =
  {
    ft_autohinter_reset_globals,
    ft_autohinter_get_globals,
    ft_autohinter_done_globals,
    ft_autohinter_load_glyph
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  autohint_module_class =
  {
    FT_MODULE_HINTER,
    sizeof ( FT_AutoHinterRec ),

    "autohinter",
    0x10000L,   /* version 1.0 of the autohinter  */
    0x20000L,   /* requires FreeType 2.0 or above */

    (const void*) &ft_autohinter_service,

    ft_autohinter_init,
    ft_autohinter_done,
    0                       /* FT_Module_Requester */
  };


/* END */
