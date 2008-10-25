/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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

/**
 * \file common_x86.c
 *
 * Check CPU capabilities & initialize optimized funtions for this particular
 * processor.
 *
 * Changed by Andre Werthmann for using the new SSE functions.
 *
 * \author Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 * \author Andre Werthmann <wertmann@cs.uni-potsdam.de>
 */

/* XXX these includes should probably go into imports.h or glheader.h */
#if defined(USE_SSE_ASM) && defined(__linux__)
#include <linux/version.h>
#endif
#if defined(USE_SSE_ASM) && defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#if defined(USE_SSE_ASM) && defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#include "common_x86_asm.h"
#include "imports.h"


int _mesa_x86_cpu_features = 0;

/* No reason for this to be public.
 */
extern GLuint	_ASMAPI _mesa_x86_has_cpuid(void);
extern void	_ASMAPI _mesa_x86_cpuid(GLuint op, GLuint *reg_eax, GLuint *reg_ebx, GLuint *reg_ecx, GLuint *reg_edx);
extern GLuint	_ASMAPI _mesa_x86_cpuid_eax(GLuint op);
extern GLuint	_ASMAPI _mesa_x86_cpuid_ebx(GLuint op);
extern GLuint	_ASMAPI _mesa_x86_cpuid_ecx(GLuint op);
extern GLuint	_ASMAPI _mesa_x86_cpuid_edx(GLuint op);


#if defined(USE_SSE_ASM)
/*
 * We must verify that the Streaming SIMD Extensions are truly supported
 * on this processor before we go ahead and hook out the optimized code.
 *
 * However, I have been told by Alan Cox that all 2.4 (and later) Linux
 * kernels provide full SSE support on all processors that expose SSE via
 * the CPUID mechanism.
 */
extern void _mesa_test_os_sse_support( void );
extern void _mesa_test_os_sse_exception_support( void );

#if defined(WIN32)
#ifndef STATUS_FLOAT_MULTIPLE_TRAPS
# define STATUS_FLOAT_MULTIPLE_TRAPS (0xC00002B5L)
#endif
static LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS exp)
{
   PEXCEPTION_RECORD rec = exp->ExceptionRecord;
   PCONTEXT ctx = exp->ContextRecord;

   if ( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION ) {
      _mesa_debug(NULL, "EXCEPTION_ILLEGAL_INSTRUCTION\n" );
      _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
   } else if ( rec->ExceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS ) {
      _mesa_debug(NULL, "STATUS_FLOAT_MULTIPLE_TRAPS\n");
      /* Windows seems to clear the exception flag itself, we just have to increment Eip */
   } else {
      _mesa_debug(NULL, "UNEXPECTED EXCEPTION (0x%08x), terminating!\n" );
      return EXCEPTION_EXECUTE_HANDLER;
   }

   if ( (ctx->ContextFlags & CONTEXT_CONTROL) != CONTEXT_CONTROL ) {
      _mesa_debug(NULL, "Context does not contain control registers, terminating!\n");
      return EXCEPTION_EXECUTE_HANDLER;
   }
   ctx->Eip += 3;

   return EXCEPTION_CONTINUE_EXECUTION;
}
#endif /* WIN32 */


static void check_os_sse_support( void )
{
#if defined(__FreeBSD__)
   {
      int ret, enabled;
      unsigned int len;
      len = sizeof(enabled);
      ret = sysctlbyname("hw.instruction_sse", &enabled, &len, NULL, 0);
      if (ret || !enabled)
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
   }
#elif defined (__NetBSD__)
   {
      int ret, enabled;
      size_t len = sizeof(enabled);
      ret = sysctlbyname("machdep.sse", &enabled, &len, (void *)NULL, 0);
      if (ret || !enabled)
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
   }
#elif defined(__OpenBSD__)
   {
      int mib[2];
      int ret, enabled;
      size_t len = sizeof(enabled);

      mib[0] = CTL_MACHDEP;
      mib[1] = CPU_SSE;

      ret = sysctl(mib, 2, &enabled, &len, NULL, 0);
      if (ret || !enabled)
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
   }
#elif defined(WIN32)
   LPTOP_LEVEL_EXCEPTION_FILTER oldFilter;
   
   /* Install our ExceptionFilter */
   oldFilter = SetUnhandledExceptionFilter( ExceptionFilter );
   
   if ( cpu_has_xmm ) {
      _mesa_debug(NULL, "Testing OS support for SSE...\n");

      _mesa_test_os_sse_support();

      if ( cpu_has_xmm ) {
	 _mesa_debug(NULL, "Yes.\n");
      } else {
	 _mesa_debug(NULL, "No!\n");
      }
   }

   if ( cpu_has_xmm ) {
      _mesa_debug(NULL, "Testing OS support for SSE unmasked exceptions...\n");

      _mesa_test_os_sse_exception_support();

      if ( cpu_has_xmm ) {
	 _mesa_debug(NULL, "Yes.\n");
      } else {
	 _mesa_debug(NULL, "No!\n");
      }
   }

   /* Restore previous exception filter */
   SetUnhandledExceptionFilter( oldFilter );

   if ( cpu_has_xmm ) {
      _mesa_debug(NULL, "Tests of OS support for SSE passed.\n");
   } else {
      _mesa_debug(NULL, "Tests of OS support for SSE failed!\n");
   }
#else
   /* Do nothing on other platforms for now.
    */
   _mesa_debug(NULL, "Not testing OS support for SSE, leaving enabled.\n");
#endif /* __FreeBSD__ */
}

#endif /* USE_SSE_ASM */


void _mesa_init_all_x86_transform_asm( void )
{
#ifdef USE_X86_ASM
   _mesa_x86_cpu_features = 0;

   if (!_mesa_x86_has_cpuid()) {
       _mesa_debug(NULL, "CPUID not detected\n");
   }
   else {
       GLuint cpu_features;
       GLuint cpu_ext_features;
       GLuint cpu_ext_info;
       char cpu_vendor[13];
       GLuint result;

       /* get vendor name */
       _mesa_x86_cpuid(0, &result, (GLuint *)(cpu_vendor + 0), (GLuint *)(cpu_vendor + 8), (GLuint *)(cpu_vendor + 4));
       cpu_vendor[12] = '\0';

       _mesa_debug(NULL, "CPU vendor: %s\n", cpu_vendor);

       /* get cpu features */
       cpu_features = _mesa_x86_cpuid_edx(1);

       if (cpu_features & X86_CPU_FPU)
	   _mesa_x86_cpu_features |= X86_FEATURE_FPU;
       if (cpu_features & X86_CPU_CMOV)
	   _mesa_x86_cpu_features |= X86_FEATURE_CMOV;

#ifdef USE_MMX_ASM
       if (cpu_features & X86_CPU_MMX)
	   _mesa_x86_cpu_features |= X86_FEATURE_MMX;
#endif

#ifdef USE_SSE_ASM
       if (cpu_features & X86_CPU_XMM)
	   _mesa_x86_cpu_features |= X86_FEATURE_XMM;
       if (cpu_features & X86_CPU_XMM2)
	   _mesa_x86_cpu_features |= X86_FEATURE_XMM2;
#endif

       /* query extended cpu features */
       if ((cpu_ext_info = _mesa_x86_cpuid_eax(0x80000000)) > 0x80000000) {
	   if (cpu_ext_info >= 0x80000001) {

	       cpu_ext_features = _mesa_x86_cpuid_edx(0x80000001);

	       if (cpu_features & X86_CPU_MMX) {

#ifdef USE_3DNOW_ASM
		   if (cpu_ext_features & X86_CPUEXT_3DNOW)
		       _mesa_x86_cpu_features |= X86_FEATURE_3DNOW;
		   if (cpu_ext_features & X86_CPUEXT_3DNOW_EXT)
		       _mesa_x86_cpu_features |= X86_FEATURE_3DNOWEXT;
#endif

#ifdef USE_MMX_ASM
		   if (cpu_ext_features & X86_CPUEXT_MMX_EXT)
		       _mesa_x86_cpu_features |= X86_FEATURE_MMXEXT;
#endif
	       }
	   }

	   /* query cpu name */
	   if (cpu_ext_info >= 0x80000002) {
	       GLuint ofs;
	       char cpu_name[49];
	       for (ofs = 0; ofs < 3; ofs++)
		   _mesa_x86_cpuid(0x80000002+ofs, (GLuint *)(cpu_name + (16*ofs)+0), (GLuint *)(cpu_name + (16*ofs)+4), (GLuint *)(cpu_name + (16*ofs)+8), (GLuint *)(cpu_name + (16*ofs)+12));
	       cpu_name[48] = '\0'; /* the name should be NULL terminated, but just to be sure */

	       _mesa_debug(NULL, "CPU name: %s\n", cpu_name);
	   }
       }

   }
   
   if ( _mesa_getenv( "MESA_NO_ASM" ) ) {
      _mesa_x86_cpu_features = 0;
   }

   if ( _mesa_x86_cpu_features ) {
      _mesa_init_x86_transform_asm();
   }

#ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      if ( _mesa_getenv( "MESA_NO_MMX" ) == 0 ) {
         _mesa_debug(NULL, "MMX cpu detected.\n");
      } else {
         _mesa_x86_cpu_features &= ~(X86_FEATURE_MMX);
      }
   }
#endif

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      if ( _mesa_getenv( "MESA_NO_3DNOW" ) == 0 ) {
         _mesa_debug(NULL, "3DNow! cpu detected.\n");
         _mesa_init_3dnow_transform_asm();
      } else {
         _mesa_x86_cpu_features &= ~(X86_FEATURE_3DNOW);
      }
   }
#endif

#ifdef USE_SSE_ASM
   if ( cpu_has_xmm ) {
      if ( _mesa_getenv( "MESA_NO_SSE" ) == 0 ) {
         _mesa_debug(NULL, "SSE cpu detected.\n");
         if ( _mesa_getenv( "MESA_FORCE_SSE" ) == 0 ) {
            check_os_sse_support();
         }
         if ( cpu_has_xmm ) {
            _mesa_init_sse_transform_asm();
         }
      } else {
         _mesa_debug(NULL, "SSE cpu detected, but switched off by user.\n");
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
      }
   }
#endif
#endif
}

