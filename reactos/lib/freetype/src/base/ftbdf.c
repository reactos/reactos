/***************************************************************************/
/*                                                                         */
/*  ftbdf.c                                                                */
/*                                                                         */
/*    FreeType API for accessing BDF-specific strings (body).              */
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
#include FT_INTERNAL_BDF_TYPES_H
#include FT_INTERNAL_OBJECTS_H


  FT_EXPORT_DEF( FT_Error )
  FT_Get_BDF_Charset_ID( FT_Face       face,
                         const char*  *acharset_encoding,
                         const char*  *acharset_registry )
  {
    FT_Error     error;
    const char*  encoding = NULL;
    const char*  registry = NULL;
    

    error = FT_Err_Invalid_Argument;
    
    if ( face != NULL && face->driver != NULL )
    {
      FT_Module  driver = (FT_Module) face->driver;
      

      if ( driver->clazz && driver->clazz->module_name         &&
           ft_strcmp( driver->clazz->module_name, "bdf" ) == 0 )
      {
        BDF_Public_Face  bdf_face = (BDF_Public_Face)face;
        

        encoding = (const char*) bdf_face->charset_encoding;
        registry = (const char*) bdf_face->charset_registry;
        error    = 0;
      }           
    }
  
    if ( acharset_encoding )
      *acharset_encoding = encoding;
    
    if ( acharset_registry )
      *acharset_registry = registry;
    
    return error;
  }                         


/* END */
