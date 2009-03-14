/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <oscalls.h>
#include <internal.h>
#include <process.h>
#include <math.h>
#include <stdlib.h>
#include <tchar.h>
#include <sect_attribs.h>
#include <locale.h>
#if 0
#include "../revstamp.h"
#endif

const PIMAGE_TLS_CALLBACK __dyn_tls_init_callback;

void * __cdecl
_decode_pointer (void *codedptr)
{
  return (void *) codedptr;
}

void * __cdecl
_encode_pointer (void *ptr)
{
  return ptr;
}

/* 0:console, 1:windows.  */
int mingw_app_type = 0;

#if 0
const char *__mingw_get_crt_info (void)
{
  return "MinGW-W64 Runtime " __MINGW64_VERSION " ("
         __MINGW64_VERSION_STATE " - "
	 "rev. " __MINGW_W64_REV ") " __MINGW_W64_REV_STAMP;
}
#endif
