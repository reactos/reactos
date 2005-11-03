#ifndef __OTLAYOUT_UTILS_H__
#define __OTLAYOUT_UTILS_H__

#include "otlayout.h"

OTL_BEGIN_HEADER

  OTL_LOCAL( OTL_Error )
  otl_mem_alloc( OTL_Pointer*  pblock,
                 OTL_ULong     size,
                 OTL_Memory    memory );

  OTL_LOCAL( void )
  otl_mem_free( OTL_Pointer*  pblock,
                OTL_Memory    memory );

  OTL_LOCAL( OTL_Error )
  otl_mem_realloc( OTL_Pointer  *pblock,
                   OTL_ULong     cur_size,
                   OTL_ULong     new_size,
                   OTL_Memory    memory );

#define  OTL_MEM_ALLOC(p,s)       otl_mem_alloc( (void**)&(p), (s), memory )
#define  OTL_MEM_FREE(p)          otl_mem_free( (void**)&(p), memory )
#define  OTL_MEM_REALLOC(p,c,n)   otl_mem_realloc( (void**)&(p), (c), (s), memory )

#define  OTL_MEM_NEW(p)   OTL_MEM_ALLOC(p,sizeof(*(p)))
#define  OTL_MEM_NEW_ARRAY(p,c)  OTL_MEM_ALLOC(p,(c)*sizeof(*(p)))
#define  OTL_MEM_RENEW_ARRAY(p,c,n)  OTL_MEM_REALLOC(p,(c)*sizeof(*(p)),(n)*sizeof(*(p)))

OTL_END_HEADER

#endif /* __OTLAYOUT_UTILS_H__ */
