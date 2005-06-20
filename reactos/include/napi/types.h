#ifndef __INCLUDE_NAPI_TYPES_H
#define __INCLUDE_NAPI_TYPES_H

/* these should be moved to a file like ntdef.h */

/*
 * General type for status information
 */

typedef enum _NT_PRODUCT_TYPE
{
   NtProductWinNt = 1,
   NtProductLanManNt,
   NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;
#endif
typedef ULARGE_INTEGER TIME, *PTIME;

#endif /* __INCLUDE_NAPI_TYPES_H */
