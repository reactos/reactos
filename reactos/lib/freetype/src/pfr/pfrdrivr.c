/***************************************************************************/
/*                                                                         */
/*  pfrdrivr.c                                                             */
/*                                                                         */
/*    FreeType PFR driver interface (body).                                */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_PFR_H
#include "pfrdrivr.h"
#include "pfrobjs.h"


  static FT_Error
  pfr_get_kerning( PFR_Face    face,
                   FT_UInt     left,
                   FT_UInt     right,
                   FT_Vector  *avector )
  {
    FT_Error  error;

    error = pfr_face_get_kerning( face, left, right, avector );
    if ( !error )
    {
      PFR_PhyFont  phys = &face->phy_font;

      /* convert from metrics to outline units when necessary */
      if ( phys->outline_resolution != phys->metrics_resolution )
      {
        if ( avector->x != 0 )
          avector->x = FT_MulDiv( avector->x, phys->outline_resolution,
                                              phys->metrics_resolution );

        if ( avector->y != 0 )
          avector->y = FT_MulDiv( avector->x, phys->outline_resolution,
                                              phys->metrics_resolution );
      }
    }
    return error;
  }


  static FT_Error
  pfr_get_advance( PFR_Face   face,
                   FT_UInt    gindex,
                   FT_Pos    *aadvance )
  {
    FT_Error     error = FT_Err_Bad_Argument;

    *aadvance = 0;
    if ( face )
    {
      PFR_PhyFont  phys  = &face->phy_font;

      if ( gindex < phys->num_chars )
      {
        *aadvance = phys->chars[ gindex ].advance;
        error = 0;
      }
    }

    return error;
  }


  static FT_Error
  pfr_get_metrics( PFR_Face   face,
                   FT_UInt   *aoutline_resolution,
                   FT_UInt   *ametrics_resolution,
                   FT_Fixed  *ametrics_x_scale,
                   FT_Fixed  *ametrics_y_scale )
  {
    PFR_PhyFont  phys  = &face->phy_font;
    FT_Fixed     x_scale, y_scale;
    FT_Size      size = face->root.size;

    if ( aoutline_resolution )
      *aoutline_resolution = phys->outline_resolution;

    if ( ametrics_resolution )
      *ametrics_resolution = phys->metrics_resolution;

    x_scale = 0x10000L;
    y_scale = 0x10000L;

    if ( size )
    {
      x_scale = FT_DivFix( size->metrics.x_ppem << 6,
                           phys->metrics_resolution );

      y_scale = FT_DivFix( size->metrics.y_ppem << 6,
                           phys->metrics_resolution );
    }

    if ( ametrics_x_scale )
      *ametrics_x_scale = x_scale;

    if ( ametrics_y_scale )
      *ametrics_y_scale = y_scale;

    return 0;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_PFR_ServiceRec  pfr_service_rec =
  {
    (FT_PFR_GetMetricsFunc)  pfr_get_metrics,
    (FT_PFR_GetKerningFunc)  pfr_get_kerning,
    (FT_PFR_GetAdvanceFunc)  pfr_get_advance
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  pfr_driver_class =
  {
    {
      ft_module_font_driver      |
      ft_module_driver_scalable,

      sizeof( FT_DriverRec ),

      "pfr",
      0x10000L,
      0x20000L,

      (FT_PFR_Service)  &pfr_service_rec,   /* format interface */

      (FT_Module_Constructor)NULL,
      (FT_Module_Destructor) NULL,
      (FT_Module_Requester)  NULL
    },

    sizeof( PFR_FaceRec ),
    sizeof( PFR_SizeRec ),
    sizeof( PFR_SlotRec ),

    (FT_Face_InitFunc)        pfr_face_init,
    (FT_Face_DoneFunc)        pfr_face_done,
    (FT_Size_InitFunc)        NULL,
    (FT_Size_DoneFunc)        NULL,
    (FT_Slot_InitFunc)        pfr_slot_init,
    (FT_Slot_DoneFunc)        pfr_slot_done,

    (FT_Size_ResetPointsFunc) NULL,
    (FT_Size_ResetPixelsFunc) NULL,
    (FT_Slot_LoadFunc)        pfr_slot_load,

    (FT_Face_GetKerningFunc)  pfr_get_kerning,
    (FT_Face_AttachFunc)      0,
    (FT_Face_GetAdvancesFunc) 0
  };


/* END */
