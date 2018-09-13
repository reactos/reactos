/*****************************************************************************\
*                                                                             *
* thunks.h -  thunking functions, types, and definitions                      *
*                                                                             *
* Version 1.0								      *
*                                                                             *
* NOTE: windows.h must be #included first				      *
*                                                                             *
* Copyright (c) 1994, Microsoft Corp.	All rights reserved.	              *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_THUNKS
#define _INC_THUNKS

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifndef _INC_WINDOWS    /* Must include windows.h first */
#error windows.h must be included before thunks.h
#endif  /* _INC_WINDOWS */

DWORD  WINAPI  MapSL(DWORD);
DWORD  WINAPI  MapLS(DWORD);
VOID   WINAPI  UnMapLS(LPVOID);

#endif  /* _INC_THUNKS */
