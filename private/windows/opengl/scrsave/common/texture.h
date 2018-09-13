/******************************Module*Header*******************************\
* Module Name: texture.h
*
* Local texture processing functions
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __texture_h__
#define __texture_h__

#include "sscommon.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL bVerifyDIB(LPTSTR pszFileName, ISIZE *pSize );
extern BOOL bVerifyRGB(LPTSTR pszFileName, ISIZE *pSize );

#ifdef __cplusplus
}
#endif

#endif // __texture_h__
