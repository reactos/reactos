/*
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_INIT_H
#define U_INIT_H

/* Use UTIL_INIT(f) to have f called at program initialization.
   Note that it is only guaranteed to be called if any symbol in the
   .c file it is in sis referenced by the program.

   UTIL_INIT functions are called in arbitrary order.
*/

#ifdef __cplusplus
/* use a C++ global constructor */
#define UTIL_INIT(f) struct f##__gctor_t {f##__gctor_t() {x();}} f##__gctor;
#elif defined(_MSC_VER)
/* add a pointer to the section where MSVC stores global constructor pointers */
/* see http://blogs.msdn.com/vcblog/archive/2006/10/20/crt-initialization.aspx and
   http://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc */
#pragma section(".CRT$XCU",read)
#define UTIL_INIT(f) static void __cdecl f##__init(void) {f();}; __declspec(allocate(".CRT$XCU")) void (__cdecl* f##__xcu)(void) = f##__init;
#elif defined(__GNUC__)
#define UTIL_INIT(f) static void f##__init(void) __attribute__((constructor)); static void f##__init(void) {f();}
#else
#error Unsupported compiler: please find out how to implement global initializers in C on it
#endif

#endif

