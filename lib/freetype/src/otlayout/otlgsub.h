#ifndef __OTL_GSUB_H__
#define __OTL_GSUB_H__

#include "otlayout.h"

OTL_BEGIN_HEADER

  typedef OTL_UInt  (*OTL_GSUB_AlternateFunc)( OTL_UInt     gindex,
                                               OTL_UInt     count,
                                               OTL_Bytes    alternates,
                                               OTL_Pointer  data );

  typedef struct OTL_GSUB_AlternateRec_
  {
    OTL_GSUB_AlternateFunc  handler_func;
    OTL_Pointer             handler_data;

  } OTL_GSUB_AlternateRec, *OTL_GSUB_Alternate;

  OTL_LOCAL( void )
  otl_gsub_validate( OTL_Bytes      table,
                     OTL_Validator  valid );

OTL_END_HEADER

#endif /* __OTL_GSUB_H__ */
