/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2005-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009 VMware, Inc.   All Rights Reserved.
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

#ifndef SLANG_PREPROCESS_H
#define SLANG_PREPROCESS_H

#include "slang_compile.h"
#include "slang_log.h"


extern GLboolean
_slang_preprocess_version (const char *, GLuint *, GLuint *, slang_info_log *);

extern GLboolean
_slang_preprocess_directives(slang_string *output, const char *input,
                             slang_info_log *,
                             const struct gl_extensions *extensions,
                             struct gl_sl_pragmas *pragmas);

#endif /* SLANG_PREPROCESS_H */
