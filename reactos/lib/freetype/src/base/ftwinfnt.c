/***************************************************************************/
/*                                                                         */
/*  ftwinfnt.c                                                             */
/*                                                                         */
/*    FreeType API for accessing Windows FNT specific info (body).         */
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
#include FT_WINFONTS_H
#include FT_INTERNAL_FNT_TYPES_H
#include FT_INTERNAL_OBJECTS_H


  FT_EXPORT_DEF( FT_Error )
  FT_Get_WinFNT_Header( FT_Face              face,
                        FT_WinFNT_HeaderRec *header )
  {
    FT_Error     error;

    error = FT_Err_Invalid_Argument;

    if ( face != NULL && face->driver != NULL )
    {
      FT_Module  driver = (FT_Module) face->driver;


      if ( driver->clazz && driver->clazz->module_name              &&
           ft_strcmp( driver->clazz->module_name, "winfonts" ) == 0 )
      {
        FNT_Size  size = (FNT_Size)face->size;
        FNT_Font  font = size->font;

        if (font)
        {
          FT_MEM_COPY( header, &font->header, sizeof(*header) );
          error    = 0;
        }
      }
    }
    return error;
  }


/* END */
