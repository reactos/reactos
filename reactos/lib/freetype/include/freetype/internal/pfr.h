#ifndef __FT_INTERNAL_PFR_H__
#define __FT_INTERNAL_PFR_H__

#include <ft2build.h>
#include FT_FREETYPE_H

FT_BEGIN_HEADER

  typedef FT_Error  (*FT_PFR_GetMetricsFunc)( FT_Face    face,
                                              FT_UInt   *aoutline,
                                              FT_UInt   *ametrics,
                                              FT_Fixed  *ax_scale,
                                              FT_Fixed  *ay_scale );

  typedef FT_Error  (*FT_PFR_GetKerningFunc)( FT_Face     face,
                                              FT_UInt     left,
                                              FT_UInt     right,
                                              FT_Vector  *avector );

  typedef FT_Error  (*FT_PFR_GetAdvanceFunc)( FT_Face   face,
                                              FT_UInt   gindex,
                                              FT_Pos   *aadvance );

  typedef struct FT_PFR_ServiceRec_
  {
    FT_PFR_GetMetricsFunc    get_metrics;
    FT_PFR_GetKerningFunc    get_kerning;
    FT_PFR_GetAdvanceFunc    get_advance;

  } FT_PFR_ServiceRec, *FT_PFR_Service;

#define  FT_PFR_SERVICE_NAME  "pfr"

FT_END_HEADER

#endif /* __FT_INTERNAL_PFR_H__ */
