/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * Copyright 1999-2008  Brian Paul
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#include "pipe/p_config.h"
#include "pipe/p_compiler.h"

#if defined(PIPE_OS_UNIX)
#include <dlfcn.h>
#endif
#if defined(PIPE_OS_WINDOWS)
#include <windows.h>
#endif

#include "u_dl.h"
#include "u_pointer.h"


struct util_dl_library *
util_dl_open(const char *filename)
{
#if defined(PIPE_OS_UNIX)
   return (struct util_dl_library *)dlopen(filename, RTLD_LAZY | RTLD_GLOBAL);
#elif defined(PIPE_OS_WINDOWS)
   return (struct util_dl_library *)LoadLibraryA(filename);
#else
   return NULL;
#endif
}


util_dl_proc
util_dl_get_proc_address(struct util_dl_library *library,
                         const char *procname)
{
#if defined(PIPE_OS_UNIX)
   return (util_dl_proc) pointer_to_func(dlsym((void *)library, procname));
#elif defined(PIPE_OS_WINDOWS)
   return (util_dl_proc)GetProcAddress((HMODULE)library, procname);
#else
   return (util_dl_proc)NULL;
#endif
}


void
util_dl_close(struct util_dl_library *library)
{
#if defined(PIPE_OS_UNIX)
   dlclose((void *)library);
#elif defined(PIPE_OS_WINDOWS)
   FreeLibrary((HMODULE)library);
#else
   (void)library;
#endif
}


const char *
util_dl_error(void)
{
#if defined(PIPE_OS_UNIX)
   return dlerror();
#elif defined(PIPE_OS_WINDOWS)
   return "unknown error";
#else
   return "unknown error";
#endif
}
