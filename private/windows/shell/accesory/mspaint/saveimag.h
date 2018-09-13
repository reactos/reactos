//
//  SaveImage.c
//
//  routines to save and compress a graphics file using a MS Office
//  graphic export filter.
//
#include "image.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
//  SaveDIBToFile
//
//  save an image file using an installed image export filter.
//
BOOL SaveDIBToFileA( LPCSTR  szFileName,
                     IFLTYPE iflType,
                     CBitmapObj * pBitmap );
BOOL SaveDIBToFileW( LPCWSTR  szFileName,
                     IFLTYPE iflType,
                     CBitmapObj * pBitmap );
#ifdef UNICODE
#define SaveDIBToFile SaveDIBToFileW
#else
#define SaveDIBToFile SaveDIBToFileA
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */
