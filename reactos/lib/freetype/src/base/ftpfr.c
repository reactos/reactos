/***************************************************************************/
/*                                                                         */
/*  ftpfr.c                                                                */
/*                                                                         */
/*    FreeType API for accessing PFR-specific data                         */
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
#include FT_INTERNAL_PFR_H
#include FT_INTERNAL_OBJECTS_H


 /* check the format */
  static FT_Error
  ft_pfr_check( FT_Face           face,
                FT_PFR_Service   *aservice )
  {
    FT_Error  error = FT_Err_Bad_Argument;

    if ( face && face->driver )
    {
      FT_Module    module = (FT_Module) face->driver;
      const char*  name   = module->clazz->module_name;

      if ( name[0] == 'p' &&
           name[1] == 'f' &&
           name[2] == 'r' &&
           name[4] == 0   )
      {
        *aservice = (FT_PFR_Service) module->clazz->module_interface;
        error = 0;
      }
    }
    return error;
  }



  FT_EXPORT_DEF( FT_Error )
  FT_Get_PFR_Metrics( FT_Face     face,
                      FT_UInt    *aoutline_resolution,
                      FT_UInt    *ametrics_resolution,
                      FT_Fixed   *ametrics_x_scale,
                      FT_Fixed   *ametrics_y_scale )
  {
    FT_Error        error;
    FT_PFR_Service  service;

    error = ft_pfr_check( face, &service );
    if ( !error )
    {
      error = service->get_metrics( face,
                                    aoutline_resolution,
                                    ametrics_resolution,
                                    ametrics_x_scale,
                                    ametrics_y_scale );
    }
    return error;
  }

  FT_EXPORT_DEF( FT_Error )
  FT_Get_PFR_Kerning( FT_Face     face,
                      FT_UInt     left,
                      FT_UInt     right,
                      FT_Vector  *avector )
  {
    FT_Error        error;
    FT_PFR_Service  service;

    error = ft_pfr_check( face, &service );
    if ( !error )
    {
      error = service->get_kerning( face, left, right, avector );
    }
    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Get_PFR_Advance( FT_Face    face,
                      FT_UInt    gindex,
                      FT_Pos    *aadvance )
  {
    FT_Error        error;
    FT_PFR_Service  service;

    error = ft_pfr_check( face, &service );
    if ( !error )
    {
      error = service->get_advance( face, gindex, aadvance );
    }
    return error;
  }

/* END */
