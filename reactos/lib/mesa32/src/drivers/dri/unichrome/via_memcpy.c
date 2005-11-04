/*
 * Copyright (C) 2004 Thomas Hellstrom, All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE CODE SUPPLIER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Thomas' orginal gutted for mesa by Keith Whitwell
 */

#include "via_tex.h"

#if defined( USE_SSE_ASM )

#define SSE_PREFETCH "  prefetchnta "
#define FENCE __asm__ __volatile__ ("sfence":::"memory");


#define PREFETCH1(arch_prefetch,from)			\
    __asm__ __volatile__ (				\
			  "1:  " arch_prefetch "(%0)\n"	\
			  arch_prefetch "32(%0)\n"	\
			  arch_prefetch "64(%0)\n"	\
			  arch_prefetch "96(%0)\n"	\
			  arch_prefetch "128(%0)\n"	\
			  arch_prefetch "160(%0)\n"	\
			  arch_prefetch "192(%0)\n"	\
			  arch_prefetch "256(%0)\n"	\
			  arch_prefetch "288(%0)\n"	\
			  "2:\n"			\
			  : : "r" (from) );



#define small_memcpy(to,from,n)						\
    {									\
	__asm__ __volatile__(						\
			     "movl %2,%%ecx\n\t"			\
                             "sarl $2,%%ecx\n\t"			\
			     "rep ; movsl\n\t"				\
			     "testb $2,%b2\n\t"				\
			     "je 1f\n\t"				\
			     "movsw\n"					\
			     "1:\ttestb $1,%b2\n\t"			\
			     "je 2f\n\t"				\
			     "movsb\n"					\
			     "2:"					\
			     :"=&D" (to), "=&S" (from)			\
			     :"q" (n),"0" ((long) to),"1" ((long) from) \
			     : "%ecx","memory");			\
    }


#define SSE_CPY(prefetch,from,to,dummy,lcnt)				\
    if ((unsigned long) from & 15)			 {		\
	__asm__ __volatile__ (						\
			      "1:\n"					\
                              prefetch "320(%1)\n"			\
			      "  movups (%1), %%xmm0\n"			\
			      "  movups 16(%1), %%xmm1\n"		\
			      "  movntps %%xmm0, (%0)\n"		\
			      "  movntps %%xmm1, 16(%0)\n"		\
                              prefetch "352(%1)\n"			\
			      "  movups 32(%1), %%xmm2\n"		\
			      "  movups 48(%1), %%xmm3\n"		\
			      "  movntps %%xmm2, 32(%0)\n"		\
			      "  movntps %%xmm3, 48(%0)\n"		\
			      "  addl $64,%0\n"				\
			      "  addl $64,%1\n"				\
			      "  decl %2\n"				\
			      "  jne 1b\n"				\
			      :"=&D"(to), "=&S"(from), "=&r"(dummy)	\
			      :"0" (to), "1" (from), "2" (lcnt): "memory"); \
    } else {								\
	__asm__ __volatile__ (						\
			      "2:\n"					\
			      prefetch "320(%1)\n"			\
			      "  movaps (%1), %%xmm0\n"			\
			      "  movaps 16(%1), %%xmm1\n"		\
			      "  movntps %%xmm0, (%0)\n"		\
			      "  movntps %%xmm1, 16(%0)\n"		\
                              prefetch "352(%1)\n"			\
			      "  movaps 32(%1), %%xmm2\n"		\
			      "  movaps 48(%1), %%xmm3\n"		\
			      "  movntps %%xmm2, 32(%0)\n"		\
			      "  movntps %%xmm3, 48(%0)\n"		\
			      "  addl $64,%0\n"				\
			      "  addl $64,%1\n"				\
			      "  decl %2\n"				\
			      "  jne 2b\n"				\
			      :"=&D"(to), "=&S"(from), "=&r"(dummy)	\
			      :"0" (to), "1" (from), "2" (lcnt): "memory"); \
    }



/*
 */
void via_sse_memcpy(void *to,
		    const void *from,
		    size_t sz)

{
   int dummy;
   int lcnt = sz >> 6;
   int rest = sz & 63;

   PREFETCH1(SSE_PREFETCH,from);

   if (lcnt > 5) {
      lcnt -= 5;
      SSE_CPY(SSE_PREFETCH,from,to,dummy,lcnt);
      lcnt = 5;
   }
   if (lcnt) {
      SSE_CPY("#",from,to,dummy,lcnt);
   }
   if (rest) small_memcpy(to, from, rest);
   FENCE;
}

#endif /* defined( USE_SSE_ASM ) */
