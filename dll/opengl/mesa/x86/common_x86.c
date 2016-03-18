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

#include "main/imports.h"
#include "common_x86_asm.h"

#include <pseh/pseh2.h>


/** Bitmask of X86_FEATURE_x bits */
int _mesa_x86_cpu_features = 0x0;

static int detection_debug = GL_FALSE;

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

/* These are assembly functions: */
extern void _mesa_test_os_sse_support( void );
extern void _mesa_test_os_sse_exception_support( void );


/**
 * Check if SSE is supported.
 * If not, turn off the X86_FEATURE_XMM flag in _mesa_x86_cpu_features.
 */
void _mesa_check_os_sse_support( void )
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
    if ( cpu_has_xmm ) {
        _mesa_debug(NULL, "Testing OS support for SSE...\n");

        _SEH2_TRY
        {
            _mesa_test_os_sse_support();
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if(_SEH2_GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION)
                _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
        }
        _SEH2_END;

        if ( cpu_has_xmm ) {
            _mesa_debug(NULL, "Yes.\n");
        } else {
            _mesa_debug(NULL, "No!\n");
        }
    }

    if ( cpu_has_xmm ) {
        _mesa_debug(NULL, "Testing OS support for SSE unmasked exceptions...\n");

        _SEH2_TRY
        {
            _mesa_test_os_sse_exception_support();
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if(_SEH2_GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION)
                _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
        }
        _SEH2_END;

        if ( cpu_has_xmm ) {
            _mesa_debug(NULL, "Yes.\n");
        } else {
            _mesa_debug(NULL, "No!\n");
        }
    }

    if ( cpu_has_xmm ) {
        _mesa_debug(NULL, "Tests of OS support for SSE passed.\n");
    } else {
        _mesa_debug(NULL, "Tests of OS support for SSE failed!\n");
    }
#else
   /* Do nothing on other platforms for now.
    */
   if (detection_debug)
      _mesa_debug(NULL, "Not testing OS support for SSE, leaving enabled.\n");
#endif /* __FreeBSD__ */
}

#endif /* USE_SSE_ASM */


/**
 * Initialize the _mesa_x86_cpu_features bitfield.
 * This is a no-op if called more than once.
 */
void
_mesa_get_x86_features(void)
{
   static int called = 0;

   if (called)
      return;

   called = 1;

#ifdef USE_X86_ASM
   _mesa_x86_cpu_features = 0x0;

   if (_mesa_getenv( "MESA_NO_ASM")) {
      return;
   }

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

       if (detection_debug)
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

	       if (detection_debug)
		  _mesa_debug(NULL, "CPU name: %s\n", cpu_name);
	   }
       }

   }

#ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      if ( _mesa_getenv( "MESA_NO_MMX" ) == 0 ) {
	 if (detection_debug)
	    _mesa_debug(NULL, "MMX cpu detected.\n");
      } else {
         _mesa_x86_cpu_features &= ~(X86_FEATURE_MMX);
      }
   }
#endif

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      if ( _mesa_getenv( "MESA_NO_3DNOW" ) == 0 ) {
	 if (detection_debug)
	    _mesa_debug(NULL, "3DNow! cpu detected.\n");
      } else {
         _mesa_x86_cpu_features &= ~(X86_FEATURE_3DNOW);
      }
   }
#endif

#ifdef USE_SSE_ASM
   if ( cpu_has_xmm ) {
      if ( _mesa_getenv( "MESA_NO_SSE" ) == 0 ) {
	 if (detection_debug)
	    _mesa_debug(NULL, "SSE cpu detected.\n");
         if ( _mesa_getenv( "MESA_FORCE_SSE" ) == 0 ) {
            _mesa_check_os_sse_support();
         }
      } else {
         _mesa_debug(NULL, "SSE cpu detected, but switched off by user.\n");
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
      }
   }
#endif

#endif /* USE_X86_ASM */

   (void) detection_debug;
}
