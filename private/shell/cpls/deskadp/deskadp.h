/******************************************************************************

  Source File:  deskadp.h

  General include file

  Copyright (c) 1997-1998 by Microsoft Corporation

  Change History:

  12-01-97 AndreVa - Created It

******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shsemip.h>
#include <stdlib.h>
#include <shlobjp.h>
#include <shellp.h>
//#include <commctrl.h>
#include <string.h>

#include <initguid.h>
#include <help.h>
#include "..\..\common\deskcplext.h"
#include "..\..\common\propsext.h"
#include "resource.h"


//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus


typedef struct _NEW_DESKTOP_PARAM {
    LPDEVMODEW lpdevmode;
    LPWSTR pwszDevice;
} NEW_DESKTOP_PARAM, *PNEW_DESKTOP_PARAM;




VOID DrawBmp(HDC hDC);
