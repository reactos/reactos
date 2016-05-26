/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

#include <precomp.h>

/**
 * This function should be called before the various "cpu_has_foo" macros
 * are used.
 */
void
_mesa_get_cpu_features(void)
{
#ifdef USE_X86_ASM
   _mesa_get_x86_features();
#endif
}


/**
 * Return a string describing the CPU architexture and extensions that
 * Mesa is using (such as SSE or Altivec).
 * \return information string, free it with free()
 */
char *
_mesa_get_cpu_string(void)
{
#define MAX_STRING 50
   char *buffer;

   buffer = (char *) malloc(MAX_STRING);
   if (!buffer)
      return NULL;

   buffer[0] = 0;

#ifdef USE_X86_ASM

   if (_mesa_x86_cpu_features) {
      strcat(buffer, "x86");
   }

# ifdef USE_MMX_ASM
   if (cpu_has_mmx) {
      strcat(buffer, (cpu_has_mmxext) ? "/MMX+" : "/MMX");
   }
# endif
# ifdef USE_3DNOW_ASM
   if (cpu_has_3dnow) {
      strcat(buffer, (cpu_has_3dnowext) ? "/3DNow!+" : "/3DNow!");
   }
# endif
# ifdef USE_SSE_ASM
   if (cpu_has_xmm) {
      strcat(buffer, (cpu_has_xmm2) ? "/SSE2" : "/SSE");
   }
# endif

#elif defined(USE_SPARC_ASM)

   strcat(buffer, "SPARC");

#elif defined(USE_PPC_ASM)

   if (_mesa_ppc_cpu_features) {
      strcat(buffer, (cpu_has_64) ? "PowerPC 64" : "PowerPC");
   }

# ifdef USE_VMX_ASM

   if (cpu_has_vmx) {
      strcat(buffer, "/Altivec");
   }

# endif

   if (! cpu_has_fpu) {
      strcat(buffer, "/No FPU");
   }

#endif

   assert(strlen(buffer) < MAX_STRING);

   return buffer;
}
