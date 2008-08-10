/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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


#ifndef SLANG_LOG_H
#define SLANG_LOG_H


typedef struct slang_info_log_
{
   char *text;
   GLboolean dont_free_text;
   GLboolean error_flag;
} slang_info_log;


extern void
slang_info_log_construct(slang_info_log *);

extern void
slang_info_log_destruct(slang_info_log *);

extern int
slang_info_log_print(slang_info_log *, const char *, ...);

extern int
slang_info_log_error(slang_info_log *, const char *, ...);

extern int
slang_info_log_warning(slang_info_log *, const char *, ...);

extern void
slang_info_log_memory(slang_info_log *);


#endif /* SLANG_LOG_H */
