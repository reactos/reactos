/*
 * Mesa 3-D graphics library
 * Version:  6.0.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
#include <signal.h>
#endif
#if defined(USE_SSE_ASM) && defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
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

static void message( const char *msg )
{
   GLboolean debug;
#ifdef DEBUG
   debug = GL_TRUE;
#else
   if ( _mesa_getenv( "MESA_DEBUG" ) ) {
      debug = GL_TRUE;
   } else {
      debug = GL_FALSE;
   }
#endif
   if ( debug ) {
      fprintf( stderr, "%s", msg );
   }
}

#if defined(USE_SSE_ASM)
/*
 * We must verify that the Streaming SIMD Extensions are truly supported
 * on this processor before we go ahead and hook out the optimized code.
 * Unfortunately, the CPUID bit isn't enough, as the OS must set the
 * OSFXSR bit in CR4 if it supports the extended FPU save and restore
 * required to use SSE.  Unfortunately, we can't just go ahead and read
 * this register, as only the kernel can do that.  Similarly, we must
 * verify that the OSXMMEXCPT bit in CR4 has been set by the OS,
 * signifying that it supports unmasked SIMD FPU exceptions.  If we take
 * an unmasked exception and the OS doesn't correctly support them, the
 * best we'll get is a SIGILL and the worst we'll get is an infinite
 * loop in the signal delivery from the kernel as we can't interact with
 * the SIMD FPU state to clear the exception bits.  Either way, this is
 * not good.
 *
 * However, I have been told by Alan Cox that all 2.4 (and later) Linux
 * kernels provide full SSE support on all processors that expose SSE via
 * the CPUID mechanism.  It just so happens that this is the exact set of
 * kernels supported DRI.  Therefore, when building for DRI the funky SSE
 * exception test is omitted.
 */

extern void _mesa_test_os_sse_support( void );
extern void _mesa_test_os_sse_exception_support( void );

#if defined(__linux__) && defined(_POSIX_SOURCE) && defined(X86_FXSR_MAGIC) \
   && !defined(IN_DRI_DRIVER)
static void sigill_handler( int signal, struct sigcontext sc )
{
   message( "SIGILL, " );

   /* Both the "xorps %%xmm0,%%xmm0" and "divps %xmm0,%%xmm1"
    * instructions are 3 bytes long.  We must increment the instruction
    * pointer manually to avoid repeated execution of the offending
    * instruction.
    *
    * If the SIGILL is caused by a divide-by-zero when unmasked
    * exceptions aren't supported, the SIMD FPU status and control
    * word will be restored at the end of the test, so we don't need
    * to worry about doing it here.  Besides, we may not be able to...
    */
   sc.eip += 3;

   _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
}

static void sigfpe_handler( int signal, struct sigcontext sc )
{
   message( "SIGFPE, " );

   if ( sc.fpstate->magic != 0xffff ) {
      /* Our signal context has the extended FPU state, so reset the
       * divide-by-zero exception mask and clear the divide-by-zero
       * exception bit.
       */
      sc.fpstate->mxcsr |= 0x00000200;
      sc.fpstate->mxcsr &= 0xfffffffb;
   } else {
      /* If we ever get here, we're completely hosed.
       */
      message( "\n\n" );
      _mesa_problem( NULL, "SSE enabling test failed badly!" );
   }
}
#endif /* __linux__ && _POSIX_SOURCE && X86_FXSR_MAGIC */

#if defined(WIN32)
#ifndef STATUS_FLOAT_MULTIPLE_TRAPS
# define STATUS_FLOAT_MULTIPLE_TRAPS (0xC00002B5L)
#endif
static LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS exp)
{
   PEXCEPTION_RECORD rec = exp->ExceptionRecord;
   PCONTEXT ctx = exp->ContextRecord;

   if ( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION ) {
      message( "EXCEPTION_ILLEGAL_INSTRUCTION, " );
      _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
   } else if ( rec->ExceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS ) {
      message( "STATUS_FLOAT_MULTIPLE_TRAPS, " );
      /* Windows seems to clear the exception flag itself, we just have to increment Eip */
   } else {
      message( "UNEXPECTED EXCEPTION (0x%08x), terminating!" );
      return EXCEPTION_EXECUTE_HANDLER;
   }

   if ( (ctx->ContextFlags & CONTEXT_CONTROL) != CONTEXT_CONTROL ) {
      message( "Context does not contain control registers, terminating!" );
      return EXCEPTION_EXECUTE_HANDLER;
   }
   ctx->Eip += 3;

   return EXCEPTION_CONTINUE_EXECUTION;
}
#endif /* WIN32 */


/* If we're running on a processor that can do SSE, let's see if we
 * are allowed to or not.  This will catch 2.4.0 or later kernels that
 * haven't been configured for a Pentium III but are running on one,
 * and RedHat patched 2.2 kernels that have broken exception handling
 * support for user space apps that do SSE.
 *
 * GH: Isn't this just awful?
 */
static void check_os_sse_support( void )
{
#if defined(__linux__) && !defined(IN_DRI_DRIVER)
#if defined(_POSIX_SOURCE) && defined(X86_FXSR_MAGIC)
   struct sigaction saved_sigill;
   struct sigaction saved_sigfpe;

   /* Save the original signal handlers.
    */
   sigaction( SIGILL, NULL, &saved_sigill );
   sigaction( SIGFPE, NULL, &saved_sigfpe );

   signal( SIGILL, (void (*)(int))sigill_handler );
   signal( SIGFPE, (void (*)(int))sigfpe_handler );

   /* Emulate test for OSFXSR in CR4.  The OS will set this bit if it
    * supports the extended FPU save and restore required for SSE.  If
    * we execute an SSE instruction on a PIII and get a SIGILL, the OS
    * doesn't support Streaming SIMD Exceptions, even if the processor
    * does.
    */
   if ( cpu_has_xmm ) {
      message( "Testing OS support for SSE... " );

      _mesa_test_os_sse_support();

      if ( cpu_has_xmm ) {
	 message( "yes.\n" );
      } else {
	 message( "no!\n" );
      }
   }

   /* Emulate test for OSXMMEXCPT in CR4.  The OS will set this bit if
    * it supports unmasked SIMD FPU exceptions.  If we unmask the
    * exceptions, do a SIMD divide-by-zero and get a SIGILL, the OS
    * doesn't support unmasked SIMD FPU exceptions.  If we get a SIGFPE
    * as expected, we're okay but we need to clean up after it.
    *
    * Are we being too stringent in our requirement that the OS support
    * unmasked exceptions?  Certain RedHat 2.2 kernels enable SSE by
    * setting CR4.OSFXSR but don't support unmasked exceptions.  Win98
    * doesn't even support them.  We at least know the user-space SSE
    * support is good in kernels that do support unmasked exceptions,
    * and therefore to be safe I'm going to leave this test in here.
    */
   if ( cpu_has_xmm ) {
      message( "Testing OS support for SSE unmasked exceptions... " );

      _mesa_test_os_sse_exception_support();

      if ( cpu_has_xmm ) {
	 message( "yes.\n" );
      } else {
	 message( "no!\n" );
      }
   }

   /* Restore the original signal handlers.
    */
   sigaction( SIGILL, &saved_sigill, NULL );
   sigaction( SIGFPE, &saved_sigfpe, NULL );

   /* If we've gotten to here and the XMM CPUID bit is still set, we're
    * safe to go ahead and hook out the SSE code throughout Mesa.
    */
   if ( cpu_has_xmm ) {
      message( "Tests of OS support for SSE passed.\n" );
   } else {
      message( "Tests of OS support for SSE failed!\n" );
   }
#else
   /* We can't use POSIX signal handling to test the availability of
    * SSE, so we disable it by default.
    */
   message( "Cannot test OS support for SSE, disabling to be safe.\n" );
   _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
#endif /* _POSIX_SOURCE && X86_FXSR_MAGIC */
#elif defined(__FreeBSD__)
   {
      int ret, enabled;
      unsigned int len;
      len = sizeof(enabled);
      ret = sysctlbyname("hw.instruction_sse", &enabled, &len, NULL, 0);
      if (ret || !enabled)
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
   }
#elif defined(WIN32)
   LPTOP_LEVEL_EXCEPTION_FILTER oldFilter;
   
   /* Install our ExceptionFilter */
   oldFilter = SetUnhandledExceptionFilter( ExceptionFilter );
   
   if ( cpu_has_xmm ) {
      message( "Testing OS support for SSE... " );

      _mesa_test_os_sse_support();

      if ( cpu_has_xmm ) {
	 message( "yes.\n" );
      } else {
	 message( "no!\n" );
      }
   }

   if ( cpu_has_xmm ) {
      message( "Testing OS support for SSE unmasked exceptions... " );

      _mesa_test_os_sse_exception_support();

      if ( cpu_has_xmm ) {
	 message( "yes.\n" );
      } else {
	 message( "no!\n" );
      }
   }

   /* Restore previous exception filter */
   SetUnhandledExceptionFilter( oldFilter );

   if ( cpu_has_xmm ) {
      message( "Tests of OS support for SSE passed.\n" );
   } else {
      message( "Tests of OS support for SSE failed!\n" );
   }
#else
   /* Do nothing on other platforms for now.
    */
   message( "Not testing OS support for SSE, leaving enabled.\n" );
#endif /* __linux__ */
}

#endif /* USE_SSE_ASM */


void _mesa_init_all_x86_transform_asm( void )
{
   (void) message; /* silence warning */
#ifdef USE_X86_ASM
   _mesa_x86_cpu_features = 0;

   if (!_mesa_x86_has_cpuid()) {
       message("CPUID not detected");
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

       message("cpu vendor: ");
       message(cpu_vendor);
       message("\n");

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

	       message("cpu name: ");
	       message(cpu_name);
	       message("\n");
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
         message( "MMX cpu detected.\n" );
      } else {
         _mesa_x86_cpu_features &= ~(X86_FEATURE_MMX);
      }
   }
#endif

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      if ( _mesa_getenv( "MESA_NO_3DNOW" ) == 0 ) {
         message( "3DNow! cpu detected.\n" );
         _mesa_init_3dnow_transform_asm();
      } else {
         _mesa_x86_cpu_features &= ~(X86_FEATURE_3DNOW);
      }
   }
#endif

#ifdef USE_SSE_ASM
   if ( cpu_has_xmm ) {
      if ( _mesa_getenv( "MESA_NO_SSE" ) == 0 ) {
         message( "SSE cpu detected.\n" );
         if ( _mesa_getenv( "MESA_FORCE_SSE" ) == 0 ) {
            check_os_sse_support();
         }
         if ( cpu_has_xmm ) {
            _mesa_init_sse_transform_asm();
         }
      } else {
         message( "SSE cpu detected, but switched off by user.\n" );
         _mesa_x86_cpu_features &= ~(X86_FEATURE_XMM);
      }
   }
#endif
#endif
}

