/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "entry.h"
#include "u_current.h"
#include "u_macros.h"

/* define macros for use by assembly dispatchers */
#define ENTRY_CURRENT_TABLE U_STRINGIFY(u_current_table)

/* in bridge mode, mapi is a user of glapi */
#ifdef MAPI_MODE_BRIDGE
#define ENTRY_CURRENT_TABLE_GET "_glapi_get_dispatch"
#else
#define ENTRY_CURRENT_TABLE_GET U_STRINGIFY(u_current_get_internal)
#endif

#if defined(USE_X86_ASM) && defined(__GNUC__)
#   ifdef GLX_USE_TLS
#      include "entry_x86_tls.h"
#   else                 
#      include "entry_x86_tsd.h"
#   endif
#elif defined(USE_X86_64_ASM) && defined(__GNUC__) && defined(GLX_USE_TLS)
#   include "entry_x86-64_tls.h"
#else

#include <stdlib.h>

static INLINE const struct mapi_table *
entry_current_get(void)
{
#ifdef MAPI_MODE_BRIDGE
   return GET_DISPATCH();
#else
   return u_current_get();
#endif
}

/* C version of the public entries */
#define MAPI_TMP_DEFINES
#define MAPI_TMP_PUBLIC_DECLARES
#define MAPI_TMP_PUBLIC_ENTRIES
#include "mapi_tmp.h"

#ifndef MAPI_MODE_BRIDGE

void
entry_patch_public(void)
{
}

mapi_func
entry_get_public(int slot)
{
   /* pubic_entries are defined by MAPI_TMP_PUBLIC_ENTRIES */
   return public_entries[slot];
}

mapi_func
entry_generate(int slot)
{
   return NULL;
}

void
entry_patch(mapi_func entry, int slot)
{
}

#endif /* MAPI_MODE_BRIDGE */

#endif /* asm */
