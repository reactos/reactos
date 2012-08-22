/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Platform independent functions for string manipulation.
 *
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef U_STRING_H_
#define U_STRING_H_

#if !defined(WIN32) && !defined(XF86_LIBC_H)
#include <stdio.h>
#endif
#include <stddef.h>
#include <stdarg.h>

#include "pipe/p_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32

int util_vsnprintf(char *, size_t, const char *, va_list);
int util_snprintf(char *str, size_t size, const char *format, ...);

static INLINE void
util_vsprintf(char *str, const char *format, va_list ap)
{
   util_vsnprintf(str, (size_t)-1, format, ap);
}

static INLINE void
util_sprintf(char *str, const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   util_vsnprintf(str, (size_t)-1, format, ap);
   va_end(ap);
}

static INLINE char *
util_strchr(const char *s, char c)
{
   while(*s) {
      if(*s == c)
	 return (char *)s;
      ++s;
   }
   return NULL;
}

static INLINE char*
util_strncat(char *dst, const char *src, size_t n)
{
   char *p = dst + strlen(dst);
   const char *q = src;
   size_t i;

   for (i = 0; i < n && *q != '\0'; ++i)
       *p++ = *q++;
   *p = '\0';

   return dst;
}

static INLINE int
util_strcmp(const char *s1, const char *s2)
{
   unsigned char u1, u2;

   while (1) {
      u1 = (unsigned char) *s1++;
      u2 = (unsigned char) *s2++;
      if (u1 != u2)
	 return u1 - u2;
      if (u1 == '\0')
	 return 0;
   }
   return 0;
}

static INLINE int
util_strncmp(const char *s1, const char *s2, size_t n)
{
   unsigned char u1, u2;

   while (n-- > 0) {
      u1 = (unsigned char) *s1++;
      u2 = (unsigned char) *s2++;
      if (u1 != u2)
	 return u1 - u2;
      if (u1 == '\0')
	 return 0;
   }
   return 0;
}

static INLINE char *
util_strstr(const char *haystack, const char *needle)
{
   const char *p = haystack;
   size_t len = strlen(needle);

   for (; (p = util_strchr(p, *needle)) != 0; p++) {
      if (util_strncmp(p, needle, len) == 0) {
	 return (char *)p;
      }
   }
   return NULL;
}

static INLINE void *
util_memmove(void *dest, const void *src, size_t n)
{
   char *p = (char *)dest;
   const char *q = (const char *)src;
   if (dest < src) {
      while (n--)
	 *p++ = *q++;
   }
   else
   {
      p += n;
      q += n;
      while (n--)
	 *--p = *--q;
   }
   return dest;
}


#else

#define util_vsnprintf vsnprintf
#define util_snprintf snprintf
#define util_vsprintf vsprintf
#define util_sprintf sprintf
#define util_strchr strchr
#define util_strcmp strcmp
#define util_strncmp strncmp
#define util_strncat strncat
#define util_strstr strstr
#define util_memmove memmove

#endif


/**
 * Printable string buffer
 */
struct util_strbuf
{
   char *str;
   char *ptr;
   size_t left;
};


static INLINE void
util_strbuf_init(struct util_strbuf *sbuf, char *str, size_t size) 
{
   sbuf->str = str;
   sbuf->str[0] = 0;
   sbuf->ptr = sbuf->str;
   sbuf->left = size;
}


static INLINE void
util_strbuf_printf(struct util_strbuf *sbuf, const char *format, ...)
{
   if(sbuf->left > 1) {
      size_t written;
      va_list ap;
      va_start(ap, format);
      written = util_vsnprintf(sbuf->ptr, sbuf->left, format, ap);
      va_end(ap);
      sbuf->ptr += written;
      sbuf->left -= written;
   }
}



#ifdef __cplusplus
}
#endif

#endif /* U_STRING_H_ */
