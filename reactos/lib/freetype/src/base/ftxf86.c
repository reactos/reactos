/***************************************************************************/
/*                                                                         */
/*  ftxf86.c                                                               */
/*                                                                         */
/*    FreeType utility file for X11 support (body).                        */
/*                                                                         */
/*  Copyright 2002 by                                                      */
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
#include FT_XFREE86_H
#include FT_INTERNAL_OBJECTS_H

  /* XXX: This really is a sad hack, but I didn't want to change every     */
  /*      driver just to support this at the moment, since other important */
  /*      changes are coming anyway.                                       */

  typedef struct  FT_FontFormatRec_
  {
    const char*  driver_name;
    const char*  format_name;

  } FT_FontFormatRec;


  FT_EXPORT_DEF( const char* )
  FT_Get_X11_Font_Format( FT_Face  face )
  {
    static const FT_FontFormatRec  font_formats[] =
    {
      { "type1",    "Type 1" },
      { "truetype", "TrueType" },
      { "bdf",      "BDF" },
      { "pcf",      "PCF" },
      { "type42",   "Type 42" },
      { "cidtype1", "CID Type 1" },
      { "cff",      "CFF" },
      { "pfr",      "PFR" },
      { "winfonts", "Windows FNT" }
    };

    const char*  result = NULL;


    if ( face && face->driver )
    {
      FT_Module  driver = (FT_Module)face->driver;


      if ( driver->clazz && driver->clazz->module_name )
      {
        FT_Int  n;
        FT_Int  count = sizeof( font_formats ) / sizeof ( font_formats[0] );


        result = driver->clazz->module_name;

        for ( n = 0; n < count; n++ )
          if ( ft_strcmp( result, font_formats[n].driver_name ) == 0 )
          {
            result = font_formats[n].format_name;
            break;
          }
      }
    }

    return result;
  }


/* END */
