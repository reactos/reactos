/***************************************************************************/
/*                                                                         */
/*  ftinit.c                                                               */
/*                                                                         */
/*    FreeType initialization layer (body).                                */
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

  /*************************************************************************/
  /*                                                                       */
  /*  The purpose of this file is to implement the following two           */
  /*  functions:                                                           */
  /*                                                                       */
  /*  FT_Add_Default_Modules():                                            */
  /*     This function is used to add the set of default modules to a      */
  /*     fresh new library object.  The set is taken from the header file  */
  /*     `freetype/config/ftmodule.h'.  See the document `FreeType 2.0     */
  /*     Build System' for more information.                               */
  /*                                                                       */
  /*  FT_Init_FreeType():                                                  */
  /*     This function creates a system object for the current platform,   */
  /*     builds a library out of it, then calls FT_Default_Drivers().      */
  /*                                                                       */
  /*  Note that even if FT_Init_FreeType() uses the implementation of the  */
  /*  system object defined at build time, client applications are still   */
  /*  able to provide their own `ftsystem.c'.                              */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/config/ftconfig.h>
#include <freetype/internal/ftobjs.h>

#include <freetype/internal/ftdebug.h>
#include <freetype/ftmodule.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_init

#undef  FT_USE_MODULE
#define FT_USE_MODULE( x )  extern const FT_Module_Class*  x;

#ifdef macintosh
    FT_USE_MODULE(fond_driver_class)
#endif
#include <freetype/config/ftmodule.h>

#undef  FT_USE_MODULE
#define FT_USE_MODULE( x )  (const FT_Module_Class*)&x,

static
const FT_Module_Class*  ft_default_modules[] =
  {
#ifdef macintosh
    FT_USE_MODULE(fond_driver_class)
#endif
#include <freetype/config/ftmodule.h>
    0
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Default_Modules                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds the set of default drivers to a given library object.         */
  /*    This is only useful when you create a library object with          */
  /*    FT_New_Library() (usually to plug a custom memory manager).        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to a new library object.                       */
  /*                                                                       */
  FT_EXPORT_FUNC( void )  FT_Add_Default_Modules( FT_Library  library )
  {
    FT_Error                 error;
    const FT_Module_Class**  cur;


    /* test for valid `library' delayed to FT_Add_Module() */

    cur = ft_default_modules;
    while ( *cur )
    {
      error = FT_Add_Module( library, *cur );
      /* notify errors, but don't stop */
      if ( error )
      {
        FT_ERROR(( "FT_Add_Default_Module: Cannot install `%s', error = %x\n",
                   (*cur)->module_name, error ));
      }
      cur++;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Init_FreeType                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a new FreeType library object.  The set of drivers     */
  /*    that are registered by this function is determined at build time.  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    library :: A handle to a new library object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_Init_FreeType( FT_Library*  library )
  {
    FT_Error   error;
    FT_Memory  memory;


    /* First of all, allocate a new system object -- this function is part */
    /* of the system-specific component, i.e. `ftsystem.c'.                */

    memory = FT_New_Memory();
    if ( !memory )
    {
      FT_ERROR(( "FT_Init_FreeType: cannot find memory manager\n" ));
      return FT_Err_Unimplemented_Feature;
    }

    /* build a library out of it, then fill it with the set of */
    /* default drivers.                                        */

    error = FT_New_Library( memory, library );
    if ( !error )
      FT_Add_Default_Modules( *library );

    return error;
  }


/* END */
