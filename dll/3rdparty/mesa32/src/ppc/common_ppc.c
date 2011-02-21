/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file common_ppc.c
 * Check CPU capabilities & initialize optimized funtions for this particular
 * processor.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef USE_PPC_ASM
#include <elf.h>
#endif

#include "common_ppc_features.h"


unsigned long _mesa_ppc_cpu_features = 0;

/**
 * Detect CPU features and install optimized transform and lighting routines.
 * Currently, CPU features are only detected.  The optimized routines have
 * yet to be written.
 * 
 * \bug
 * This routine is highly specific to Linux kernel 2.6.  I'm still waiting
 * to hear back from the glibc folk on how to do this "right".
 */

void _mesa_init_all_ppc_transform_asm( void )
{
#ifdef USE_PPC_ASM
   const pid_t  my_pid = getpid();
   char file_name[32];
   FILE * f;
#ifdef __powerpc64__
   Elf64_auxv_t  v;
#else
   Elf32_auxv_t  v;
#endif

   sprintf( file_name, "/proc/%u/auxv", (unsigned) my_pid );
   f = fopen( file_name, "rb" );
   if ( f != NULL ) {
      while( 1 ) {
	 ssize_t elem = fread( & v, sizeof( v ), 1, f );

	 if ( elem < 1 ) {
	    break;
	 }

	 if ( v.a_type == AT_HWCAP ) {
	    _mesa_ppc_cpu_features = v.a_un.a_val;
	    break;
	 }
      }

      fclose( f );
   }
   
# ifndef USE_VMX_ASM
   _mesa_ppc_cpu_features &= ~PPC_FEATURES_HAS_ALTIVEC;
# endif
#endif
}
