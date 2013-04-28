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

#ifndef MAPI_ABI_HEADER
#error "MAPI_ABI_HEADER must be defined"
#endif

/* does not need hidden entries in bridge mode */
#ifdef MAPI_MODE_BRIDGE

#ifdef MAPI_TMP_PUBLIC_ENTRIES
#undef MAPI_TMP_PUBLIC_ENTRIES
#define MAPI_TMP_PUBLIC_ENTRIES_NO_HIDDEN
#endif

#ifdef MAPI_TMP_STUB_ASM_GCC
#undef MAPI_TMP_STUB_ASM_GCC
#define MAPI_TMP_STUB_ASM_GCC_NO_HIDDEN
#endif

#endif /* MAPI_MODE_BRIDGE */

#include MAPI_ABI_HEADER
