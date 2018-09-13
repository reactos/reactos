/*****************************************************************************
 *
 *	util.h - Shared stuff that operates on all classes
 *
 *****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

#include "dllload.h"


extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

#ifdef  UNICODE

   typedef WCHAR TUCHAR, *PTUCHAR;

#else   /* UNICODE */

   typedef unsigned char TUCHAR, *PTUCHAR;

#endif /* UNICODE */

#ifdef UNICODE
#define AllocBStrFromString(psz)    SysAllocString(psz)
#define TCharSysAllocString(psz)    SysAllocString(psz)
#else
extern BSTR AllocBStrFromString(LPTSTR);
#define TCharSysAllocString(psz)    AllocBStrFromString(psz)
#endif


#endif // _UTIL_H
