/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * Wrapper functions for dlopen(), dlsym(), dlclose().
 * Note that the #ifdef tests for various environments should be expanded.
 */


#include "glheader.h"
#include "imports.h"
#include "dlopen.h"

#if defined(_GNU_SOURCE) && !defined(__MINGW32__)
#include <dlfcn.h>
#endif


/**
 * Wrapper for dlopen().
 * Note that 'flags' isn't used at this time.
 */
void *
_mesa_dlopen(const char *libname, int flags)
{
#if defined(_GNU_SOURCE)
   flags = RTLD_LAZY | RTLD_GLOBAL; /* Overriding flags at this time */
   return dlopen(libname, flags);
#elif defined(__MINGW32__)
   return LoadLibrary(libname);
#else
   return NULL;
#endif
}


/**
 * Wrapper for dlsym() that does a cast to a generic function type,
 * rather than a void *.  This reduces the number of warnings that are
 * generated.
 */
GenericFunc
_mesa_dlsym(void *handle, const char *fname)
{
#if defined(__DJGPP__)
   /* need '_' prefix on symbol names */
   char fname2[1000];
   fname2[0] = '_';
   _mesa_strncpy(fname2 + 1, fname, 998);
   fname2[999] = 0;
   return (GenericFunc) dlsym(handle, fname2);
#elif defined(_GNU_SOURCE)
   return (GenericFunc) dlsym(handle, fname);
#elif defined(__MINGW32__)
   return (GenericFunc) GetProcAddress(handle, fname);
#else
   return (GenericFunc) NULL;
#endif
}


/**
 * Wrapper for dlclose().
 */
void
_mesa_dlclose(void *handle)
{
#if defined(_GNU_SOURCE)
   dlclose(handle);
#elif defined(__MINGW32__)
   FreeLibrary(handle);
#else
   (void) handle;
#endif
}



