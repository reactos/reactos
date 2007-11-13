/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>

#include "internal.h"


#ifndef MAX
#define MAX(x, y) (((x) < (y)) ? (y) : (x))
#endif


/* _create_linear_mapping:
 *  Maps a physical address range into linear memory.
 */
int
_create_linear_mapping (unsigned long *linear, unsigned long physaddr, int size)
{
   __dpmi_meminfo meminfo;

   if (physaddr >= 0x100000) {
      /* map into linear memory */
      meminfo.address = physaddr;
      meminfo.size = size;
      if (__dpmi_physical_address_mapping(&meminfo) != 0) {
         return -1;
      }

      *linear = meminfo.address;
   } else {
      /* exploit 1 -> 1 physical to linear mapping in low megabyte */
      *linear = physaddr;
   }

   return 0;
}


/* _remove_linear_mapping:
 *  Frees the DPMI resources being used to map a linear address range.
 */
void
_remove_linear_mapping (unsigned long *linear)
{
   __dpmi_meminfo meminfo;

   if (*linear) {
      if (*linear >= 0x100000) {
         meminfo.address = *linear;
         __dpmi_free_physical_address_mapping(&meminfo);
      }

      *linear = 0;
   }
}


/* _create_selector:
 *  Allocates a selector to access a region of linear memory.
 */
int
_create_selector (int *segment, unsigned long base, int size)
{
   /* allocate an ldt descriptor */
   if ((*segment=__dpmi_allocate_ldt_descriptors(1)) < 0) {
      *segment = 0;
      return -1;
   }

   /* create the linear mapping */
   if (_create_linear_mapping(&base, base, size)) {
      __dpmi_free_ldt_descriptor(*segment);
      *segment = 0;
      return -1;
   }

   /* set the descriptor base and limit */
   __dpmi_set_segment_base_address(*segment, base);
   __dpmi_set_segment_limit(*segment, MAX(size-1, 0xFFFF));

   return 0;
}


/* _remove_selector:
 *  Frees a DPMI segment selector.
 */
void
_remove_selector (int *segment)
{
   if (*segment) {
      unsigned long base;
      __dpmi_get_segment_base_address(*segment, &base);
      _remove_linear_mapping(&base);
      __dpmi_free_ldt_descriptor(*segment);
      *segment = 0;
   }
}


/* Desc: retrieve CPU MMX capability
 *
 * In  : -
 * Out : FALSE if CPU cannot do MMX
 *
 * Note: -
 */
int
_can_mmx (void)
{
#ifdef USE_MMX_ASM
   int x86_cpu_features = 0;
   __asm("\n\
		pushfl			\n\
		popl	%%eax		\n\
		movl	%%eax, %%ecx	\n\
		xorl	$0x200000, %%eax\n\
		pushl	%%eax		\n\
		popfl			\n\
		pushfl			\n\
		popl	%%eax		\n\
		pushl	%%ecx		\n\
		popfl			\n\
		xorl	%%ecx, %%eax	\n\
		jz 0f			\n\
		movl	$1, %%eax	\n\
		cpuid			\n\
		movl	%%edx, %0	\n\
	0:				\n\
   ":"=g"(x86_cpu_features)::"%eax", "%ebx", "%ecx", "%edx");
   return (x86_cpu_features & 0x00800000);
#else
   return 0;
#endif
}
