/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file glapi_entrypoint.c
 *
 * Arch-specific code for manipulating GL API entrypoints (dispatch stubs).
 */


#include "glapi/glapi_priv.h"
#include "mapi/u_execmem.h"


#ifdef USE_X86_ASM

#if defined( GLX_USE_TLS )
extern       GLubyte gl_dispatch_functions_start[];
extern       GLubyte gl_dispatch_functions_end[];
#else
extern const GLubyte gl_dispatch_functions_start[];
#endif

#endif /* USE_X86_ASM */


#if defined(DISPATCH_FUNCTION_SIZE)

_glapi_proc
get_entrypoint_address(unsigned int functionOffset)
{
   return (_glapi_proc) (gl_dispatch_functions_start
                         + (DISPATCH_FUNCTION_SIZE * functionOffset));
}

#endif


#if defined(USE_X86_ASM)

/**
 * Perform platform-specific GL API entry-point fixups.
 */
static void
init_glapi_relocs( void )
{
#if defined(GLX_USE_TLS) && !defined(GLX_X86_READONLY_TEXT)
    extern unsigned long _x86_get_dispatch(void);
    char run_time_patch[] = {
       0x65, 0xa1, 0, 0, 0, 0 /* movl %gs:0,%eax */
    };
    GLuint *offset = (GLuint *) &run_time_patch[2]; /* 32-bits for x86/32 */
    const GLubyte * const get_disp = (const GLubyte *) run_time_patch;
    GLubyte * curr_func = (GLubyte *) gl_dispatch_functions_start;

    *offset = _x86_get_dispatch();
    while ( curr_func != (GLubyte *) gl_dispatch_functions_end ) {
	(void) memcpy( curr_func, get_disp, sizeof(run_time_patch));
	curr_func += DISPATCH_FUNCTION_SIZE;
    }
#endif
}


/**
 * Generate a dispatch function (entrypoint) which jumps through
 * the given slot number (offset) in the current dispatch table.
 * We need assembly language in order to accomplish this.
 */
_glapi_proc
generate_entrypoint(unsigned int functionOffset)
{
   /* 32 is chosen as something of a magic offset.  For x86, the dispatch
    * at offset 32 is the first one where the offset in the
    * "jmp OFFSET*4(%eax)" can't be encoded in a single byte.
    */
   const GLubyte * const template_func = gl_dispatch_functions_start 
     + (DISPATCH_FUNCTION_SIZE * 32);
   GLubyte * const code = (GLubyte *) u_execmem_alloc(DISPATCH_FUNCTION_SIZE);


   if ( code != NULL ) {
      (void) memcpy(code, template_func, DISPATCH_FUNCTION_SIZE);
      fill_in_entrypoint_offset( (_glapi_proc) code, functionOffset );
   }

   return (_glapi_proc) code;
}


/**
 * This function inserts a new dispatch offset into the assembly language
 * stub that was generated with the preceeding function.
 */
void
fill_in_entrypoint_offset(_glapi_proc entrypoint, unsigned int offset)
{
   GLubyte * const code = (GLubyte *) entrypoint;

#if defined(GLX_USE_TLS)
   *((unsigned int *)(code +  8)) = 4 * offset;
#elif defined(THREADS)
   *((unsigned int *)(code + 11)) = 4 * offset;
   *((unsigned int *)(code + 22)) = 4 * offset;
#else
   *((unsigned int *)(code +  7)) = 4 * offset;
#endif
}


#elif defined(USE_SPARC_ASM)

extern void __glapi_sparc_icache_flush(unsigned int *);

static void
init_glapi_relocs( void )
{
#if defined(PTHREADS) || defined(GLX_USE_TLS)
    static const unsigned int template[] = {
#ifdef GLX_USE_TLS
	0x05000000, /* sethi %hi(_glapi_tls_Dispatch), %g2 */
	0x8730e00a, /* srl %g3, 10, %g3 */
	0x8410a000, /* or %g2, %lo(_glapi_tls_Dispatch), %g2 */
#ifdef __arch64__
	0xc259c002, /* ldx [%g7 + %g2], %g1 */
	0xc2584003, /* ldx [%g1 + %g3], %g1 */
#else
	0xc201c002, /* ld [%g7 + %g2], %g1 */
	0xc2004003, /* ld [%g1 + %g3], %g1 */
#endif
	0x81c04000, /* jmp %g1 */
	0x01000000, /* nop  */
#else
#ifdef __arch64__
	0x03000000, /* 64-bit 0x00 --> sethi %hh(_glapi_Dispatch), %g1 */
	0x05000000, /* 64-bit 0x04 --> sethi %lm(_glapi_Dispatch), %g2 */
	0x82106000, /* 64-bit 0x08 --> or %g1, %hm(_glapi_Dispatch), %g1 */
	0x8730e00a, /* 64-bit 0x0c --> srl %g3, 10, %g3 */
	0x83287020, /* 64-bit 0x10 --> sllx %g1, 32, %g1 */
	0x82004002, /* 64-bit 0x14 --> add %g1, %g2, %g1 */
	0xc2586000, /* 64-bit 0x18 --> ldx [%g1 + %lo(_glapi_Dispatch)], %g1 */
#else
	0x03000000, /* 32-bit 0x00 --> sethi %hi(_glapi_Dispatch), %g1 */
	0x8730e00a, /* 32-bit 0x04 --> srl %g3, 10, %g3 */
	0xc2006000, /* 32-bit 0x08 --> ld [%g1 + %lo(_glapi_Dispatch)], %g1 */
#endif
	0x80a06000, /*             --> cmp %g1, 0 */
	0x02800005, /*             --> be +4*5 */
	0x01000000, /*             -->  nop  */
#ifdef __arch64__
	0xc2584003, /* 64-bit      --> ldx [%g1 + %g3], %g1 */
#else
	0xc2004003, /* 32-bit      --> ld [%g1 + %g3], %g1 */
#endif
	0x81c04000, /*             --> jmp %g1 */
	0x01000000, /*             --> nop  */
#ifdef __arch64__
	0x9de3bf80, /* 64-bit      --> save  %sp, -128, %sp */
#else
	0x9de3bfc0, /* 32-bit      --> save  %sp, -64, %sp */
#endif
	0xa0100003, /*             --> mov  %g3, %l0 */
	0x40000000, /*             --> call _glapi_get_dispatch */
	0x01000000, /*             -->  nop */
	0x82100008, /*             --> mov %o0, %g1 */
	0x86100010, /*             --> mov %l0, %g3 */
	0x10bffff7, /*             --> ba -4*9 */
	0x81e80000, /*             -->  restore  */
#endif
    };
#ifdef GLX_USE_TLS
    extern unsigned int __glapi_sparc_tls_stub;
    extern unsigned long __glapi_sparc_get_dispatch(void);
    unsigned int *code = &__glapi_sparc_tls_stub;
    unsigned long dispatch = __glapi_sparc_get_dispatch();
#else
    extern unsigned int __glapi_sparc_pthread_stub;
    unsigned int *code = &__glapi_sparc_pthread_stub;
    unsigned long dispatch = (unsigned long) &_glapi_Dispatch;
    unsigned long call_dest = (unsigned long ) &_glapi_get_dispatch;
    int idx;
#endif

#ifdef GLX_USE_TLS
    code[0] = template[0] | (dispatch >> 10);
    code[1] = template[1];
    __glapi_sparc_icache_flush(&code[0]);
    code[2] = template[2] | (dispatch & 0x3ff);
    code[3] = template[3];
    __glapi_sparc_icache_flush(&code[2]);
    code[4] = template[4];
    code[5] = template[5];
    __glapi_sparc_icache_flush(&code[4]);
    code[6] = template[6];
    __glapi_sparc_icache_flush(&code[6]);
#else
#if defined(__arch64__)
    code[0] = template[0] | (dispatch >> (32 + 10));
    code[1] = template[1] | ((dispatch & 0xffffffff) >> 10);
    __glapi_sparc_icache_flush(&code[0]);
    code[2] = template[2] | ((dispatch >> 32) & 0x3ff);
    code[3] = template[3];
    __glapi_sparc_icache_flush(&code[2]);
    code[4] = template[4];
    code[5] = template[5];
    __glapi_sparc_icache_flush(&code[4]);
    code[6] = template[6] | (dispatch & 0x3ff);
    idx = 7;
#else
    code[0] = template[0] | (dispatch >> 10);
    code[1] = template[1];
    __glapi_sparc_icache_flush(&code[0]);
    code[2] = template[2] | (dispatch & 0x3ff);
    idx = 3;
#endif
    code[idx + 0] = template[idx + 0];
    __glapi_sparc_icache_flush(&code[idx - 1]);
    code[idx + 1] = template[idx + 1];
    code[idx + 2] = template[idx + 2];
    __glapi_sparc_icache_flush(&code[idx + 1]);
    code[idx + 3] = template[idx + 3];
    code[idx + 4] = template[idx + 4];
    __glapi_sparc_icache_flush(&code[idx + 3]);
    code[idx + 5] = template[idx + 5];
    code[idx + 6] = template[idx + 6];
    __glapi_sparc_icache_flush(&code[idx + 5]);
    code[idx + 7] = template[idx + 7];
    code[idx + 8] = template[idx + 8] |
	    (((call_dest - ((unsigned long) &code[idx + 8]))
	      >> 2) & 0x3fffffff);
    __glapi_sparc_icache_flush(&code[idx + 7]);
    code[idx + 9] = template[idx + 9];
    code[idx + 10] = template[idx + 10];
    __glapi_sparc_icache_flush(&code[idx + 9]);
    code[idx + 11] = template[idx + 11];
    code[idx + 12] = template[idx + 12];
    __glapi_sparc_icache_flush(&code[idx + 11]);
    code[idx + 13] = template[idx + 13];
    __glapi_sparc_icache_flush(&code[idx + 13]);
#endif
#endif
}


_glapi_proc
generate_entrypoint(GLuint functionOffset)
{
#if defined(PTHREADS) || defined(GLX_USE_TLS)
   static const unsigned int template[] = {
      0x07000000, /* sethi %hi(0), %g3 */
      0x8210000f, /* mov  %o7, %g1 */
      0x40000000, /* call */
      0x9e100001, /* mov  %g1, %o7 */
   };
#ifdef GLX_USE_TLS
   extern unsigned int __glapi_sparc_tls_stub;
   unsigned long call_dest = (unsigned long ) &__glapi_sparc_tls_stub;
#else
   extern unsigned int __glapi_sparc_pthread_stub;
   unsigned long call_dest = (unsigned long ) &__glapi_sparc_pthread_stub;
#endif
   unsigned int *code = (unsigned int *) u_execmem_alloc(sizeof(template));
   if (code) {
      code[0] = template[0] | (functionOffset & 0x3fffff);
      code[1] = template[1];
      __glapi_sparc_icache_flush(&code[0]);
      code[2] = template[2] |
         (((call_dest - ((unsigned long) &code[2]))
	   >> 2) & 0x3fffffff);
      code[3] = template[3];
      __glapi_sparc_icache_flush(&code[2]);
   }
   return (_glapi_proc) code;
#endif
}


void
fill_in_entrypoint_offset(_glapi_proc entrypoint, GLuint offset)
{
   unsigned int *code = (unsigned int *) entrypoint;

   code[0] &= ~0x3fffff;
   code[0] |= (offset * sizeof(void *)) & 0x3fffff;
   __glapi_sparc_icache_flush(&code[0]);
}


#else /* USE_*_ASM */

static void
init_glapi_relocs( void )
{
}


_glapi_proc
generate_entrypoint(GLuint functionOffset)
{
   (void) functionOffset;
   return NULL;
}


void
fill_in_entrypoint_offset(_glapi_proc entrypoint, GLuint offset)
{
   /* an unimplemented architecture */
   (void) entrypoint;
   (void) offset;
}

#endif /* USE_*_ASM */


void
init_glapi_relocs_once( void )
{
#if defined(PTHREADS) || defined(GLX_USE_TLS)
   static pthread_once_t once_control = PTHREAD_ONCE_INIT;
   pthread_once( & once_control, init_glapi_relocs );
#endif
}
