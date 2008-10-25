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

#include "main/imports.h"
#include "slang_log.h"
#include "slang_utility.h"



static char *out_of_memory = "Error: Out of memory.\n";

void
slang_info_log_construct(slang_info_log * log)
{
   log->text = NULL;
   log->dont_free_text = GL_FALSE;
   log->error_flag = GL_FALSE;
}

void
slang_info_log_destruct(slang_info_log * log)
{
   if (!log->dont_free_text)
#if 0
      slang_alloc_free(log->text);
#else
      _mesa_free(log->text);
#endif
}

static int
slang_info_log_message(slang_info_log * log, const char *prefix,
                       const char *msg)
{
   GLuint size;

   if (log->dont_free_text)
      return 0;
   size = slang_string_length(msg) + 2;
   if (prefix != NULL)
      size += slang_string_length(prefix) + 2;
   if (log->text != NULL) {
      GLuint old_len = slang_string_length(log->text);
      log->text = (char *)
#if 0
	 slang_alloc_realloc(log->text, old_len + 1, old_len + size);
#else
	 _mesa_realloc(log->text, old_len + 1, old_len + size);
#endif
   }
   else {
#if 0
      log->text = (char *) (slang_alloc_malloc(size));
#else
      log->text = (char *) (_mesa_malloc(size));
#endif
      if (log->text != NULL)
         log->text[0] = '\0';
   }
   if (log->text == NULL)
      return 0;
   if (prefix != NULL) {
      slang_string_concat(log->text, prefix);
      slang_string_concat(log->text, ": ");
   }
   slang_string_concat(log->text, msg);
   slang_string_concat(log->text, "\n");
#if 0 /* debug */
   _mesa_printf("Mesa GLSL error/warning: %s\n", log->text);
#endif
   return 1;
}

int
slang_info_log_print(slang_info_log * log, const char *msg, ...)
{
   va_list va;
   char buf[1024];

   va_start(va, msg);
   _mesa_vsprintf(buf, msg, va);
   va_end(va);
   return slang_info_log_message(log, NULL, buf);
}

int
slang_info_log_error(slang_info_log * log, const char *msg, ...)
{
   va_list va;
   char buf[1024];

   va_start(va, msg);
   _mesa_vsprintf(buf, msg, va);
   va_end(va);
   log->error_flag = GL_TRUE;
   if (slang_info_log_message(log, "Error", buf))
      return 1;
   slang_info_log_memory(log);
   return 0;
}

int
slang_info_log_warning(slang_info_log * log, const char *msg, ...)
{
   va_list va;
   char buf[1024];

   va_start(va, msg);
   _mesa_vsprintf(buf, msg, va);
   va_end(va);
   if (slang_info_log_message(log, "Warning", buf))
      return 1;
   slang_info_log_memory(log);
   return 0;
}

void
slang_info_log_memory(slang_info_log * log)
{
   if (!slang_info_log_message(log, "Error", "Out of memory.")) {
      log->dont_free_text = GL_TRUE;
      log->error_flag = GL_TRUE;
      log->text = out_of_memory;
   }
}
