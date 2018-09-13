#ifndef _pch_h_
#define _pch_h_

#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shsemip.h>
#include <stdlib.h>


#include "propsext.h"
#include "clssfact.h"
#include "addon.h"
#include "rc.h"
#include "regutils.h"

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(unsigned int size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus


#endif
