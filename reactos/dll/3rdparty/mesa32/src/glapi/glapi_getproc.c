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
 * \file glapi_getproc.
 *
 * Code for implementing glXGetProcAddress(), etc.
 * This was originally in glapi.c but refactored out.
 */


#include <stdlib.h>
#include <string.h>
#include "main/glheader.h"
#include "glapi.h"
#include "glapioffsets.h"
#include "glapitable.h"


static void
fill_in_entrypoint_offset(_glapi_proc entrypoint, GLuint offset);


/**
 * strdup() is actually not a standard ANSI C or POSIX routine.
 * Irix will not define it if ANSI mode is in effect.
 */
static char *
str_dup(const char *str)
{
   char *copy;
   copy = (char*) malloc(strlen(str) + 1);
   if (!copy)
      return NULL;
   strcpy(copy, str);
   return copy;
}



#if defined(USE_X64_64_ASM) && defined(GLX_USE_TLS)
# define DISPATCH_FUNCTION_SIZE  16
#elif defined(USE_X86_ASM)
# if defined(THREADS) && !defined(GLX_USE_TLS)
#  define DISPATCH_FUNCTION_SIZE  32
# else
#  define DISPATCH_FUNCTION_SIZE  16
# endif
#endif

#if !defined(DISPATCH_FUNCTION_SIZE) && !defined(XFree86Server) && !defined(XGLServer)
# define NEED_FUNCTION_POINTER
#endif

/* The code in this file is auto-generated with Python */
#include "glprocs.h"


/**
 * Search the table of static entrypoint functions for the named function
 * and return the corresponding glprocs_table_t entry.
 */
static const glprocs_table_t *
find_entry( const char * n )
{
   GLuint i;
   for (i = 0; static_functions[i].Name_offset >= 0; i++) {
      const char *testName = gl_string_table + static_functions[i].Name_offset;
      if (strcmp(testName, n) == 0) {
	 return &static_functions[i];
      }
   }
   return NULL;
}


/**
 * Return dispatch table offset of the named static (built-in) function.
 * Return -1 if function not found.
 */
static GLint
get_static_proc_offset(const char *funcName)
{
   const glprocs_table_t * const f = find_entry( funcName );
   if (f) {
      return f->Offset;
   }
   return -1;
}


#if !defined(XFree86Server) && !defined(XGLServer)
#ifdef USE_X86_ASM

#if defined( GLX_USE_TLS )
extern       GLubyte gl_dispatch_functions_start[];
extern       GLubyte gl_dispatch_functions_end[];
#else
extern const GLubyte gl_dispatch_functions_start[];
#endif

#endif /* USE_X86_ASM */


/**
 * Return dispatch function address for the named static (built-in) function.
 * Return NULL if function not found.
 */
static _glapi_proc
get_static_proc_address(const char *funcName)
{
   const glprocs_table_t * const f = find_entry( funcName );
   if (f) {
#if defined(DISPATCH_FUNCTION_SIZE) && defined(GLX_INDIRECT_RENDERING)
      return (f->Address == NULL)
	 ? (_glapi_proc) (gl_dispatch_functions_start
			  + (DISPATCH_FUNCTION_SIZE * f->Offset))
         : f->Address;
#elif defined(DISPATCH_FUNCTION_SIZE)
      return (_glapi_proc) (gl_dispatch_functions_start 
                            + (DISPATCH_FUNCTION_SIZE * f->Offset));
#else
      return f->Address;
#endif
   }
   else {
      return NULL;
   }
}

#endif /* !defined(XFree86Server) && !defined(XGLServer) */



/**
 * Return the name of the function at the given offset in the dispatch
 * table.  For debugging only.
 */
static const char *
get_static_proc_name( GLuint offset )
{
   GLuint i;
   for (i = 0; static_functions[i].Name_offset >= 0; i++) {
      if (static_functions[i].Offset == offset) {
	 return gl_string_table + static_functions[i].Name_offset;
      }
   }
   return NULL;
}



/**********************************************************************
 * Extension function management.
 */


/**
 * Track information about a function added to the GL API.
 */
struct _glapi_function {
   /**
    * Name of the function.
    */
   const char * name;


   /**
    * Text string that describes the types of the parameters passed to the
    * named function.   Parameter types are converted to characters using the
    * following rules:
    *   - 'i' for \c GLint, \c GLuint, and \c GLenum
    *   - 'p' for any pointer type
    *   - 'f' for \c GLfloat and \c GLclampf
    *   - 'd' for \c GLdouble and \c GLclampd
    */
   const char * parameter_signature;


   /**
    * Offset in the dispatch table where the pointer to the real function is
    * located.  If the driver has not requested that the named function be
    * added to the dispatch table, this will have the value ~0.
    */
   unsigned dispatch_offset;


   /**
    * Pointer to the dispatch stub for the named function.
    * 
    * \todo
    * The semantic of this field should be changed slightly.  Currently, it
    * is always expected to be non-\c NULL.  However, it would be better to
    * only allocate the entry-point stub when the application requests the
    * function via \c glXGetProcAddress.  This would save memory for all the
    * functions that the driver exports but that the application never wants
    * to call.
    */
   _glapi_proc dispatch_stub;
};


static struct _glapi_function ExtEntryTable[MAX_EXTENSION_FUNCS];
static GLuint NumExtEntryPoints = 0;

#ifdef USE_SPARC_ASM
extern void __glapi_sparc_icache_flush(unsigned int *);
#endif

/**
 * Generate a dispatch function (entrypoint) which jumps through
 * the given slot number (offset) in the current dispatch table.
 * We need assembly language in order to accomplish this.
 */
static _glapi_proc
generate_entrypoint(GLuint functionOffset)
{
#if defined(USE_X86_ASM)
   /* 32 is chosen as something of a magic offset.  For x86, the dispatch
    * at offset 32 is the first one where the offset in the
    * "jmp OFFSET*4(%eax)" can't be encoded in a single byte.
    */
   const GLubyte * const template_func = gl_dispatch_functions_start 
     + (DISPATCH_FUNCTION_SIZE * 32);
   GLubyte * const code = (GLubyte *) malloc(DISPATCH_FUNCTION_SIZE);


   if ( code != NULL ) {
      (void) memcpy(code, template_func, DISPATCH_FUNCTION_SIZE);
      fill_in_entrypoint_offset( (_glapi_proc) code, functionOffset );
   }

   return (_glapi_proc) code;
#elif defined(USE_SPARC_ASM)

#ifdef __arch64__
   static const unsigned int insn_template[] = {
	   0x05000000,	/* sethi	%uhi(_glapi_Dispatch), %g2	*/
	   0x03000000,	/* sethi	%hi(_glapi_Dispatch), %g1	*/
	   0x8410a000,	/* or		%g2, %ulo(_glapi_Dispatch), %g2	*/
	   0x82106000,	/* or		%g1, %lo(_glapi_Dispatch), %g1	*/
	   0x8528b020,	/* sllx		%g2, 32, %g2			*/
	   0xc2584002,	/* ldx		[%g1 + %g2], %g1		*/
	   0x05000000,	/* sethi	%hi(8 * glapioffset), %g2	*/
	   0x8410a000,	/* or		%g2, %lo(8 * glapioffset), %g2	*/
	   0xc6584002,	/* ldx		[%g1 + %g2], %g3		*/
	   0x81c0c000,	/* jmpl		%g3, %g0			*/
	   0x01000000	/*  nop						*/
   };
#else
   static const unsigned int insn_template[] = {
	   0x03000000,	/* sethi	%hi(_glapi_Dispatch), %g1	  */
	   0xc2006000,	/* ld		[%g1 + %lo(_glapi_Dispatch)], %g1 */
	   0xc6006000,	/* ld		[%g1 + %lo(4*glapioffset)], %g3	  */
	   0x81c0c000,	/* jmpl		%g3, %g0			  */
	   0x01000000	/*  nop						  */
   };
#endif /* __arch64__ */
   unsigned int *code = (unsigned int *) malloc(sizeof(insn_template));
   unsigned long glapi_addr = (unsigned long) &_glapi_Dispatch;
   if (code) {
      memcpy(code, insn_template, sizeof(insn_template));

#ifdef __arch64__
      code[0] |= (glapi_addr >> (32 + 10));
      code[1] |= ((glapi_addr & 0xffffffff) >> 10);
      __glapi_sparc_icache_flush(&code[0]);
      code[2] |= ((glapi_addr >> 32) & ((1 << 10) - 1));
      code[3] |= (glapi_addr & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&code[2]);
      code[6] |= ((functionOffset * 8) >> 10);
      code[7] |= ((functionOffset * 8) & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&code[6]);
#else
      code[0] |= (glapi_addr >> 10);
      code[1] |= (glapi_addr & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&code[0]);
      code[2] |= (functionOffset * 4);
      __glapi_sparc_icache_flush(&code[2]);
#endif /* __arch64__ */
   }
   return (_glapi_proc) code;
#else
   (void) functionOffset;
   return NULL;
#endif /* USE_*_ASM */
}


/**
 * This function inserts a new dispatch offset into the assembly language
 * stub that was generated with the preceeding function.
 */
static void
fill_in_entrypoint_offset(_glapi_proc entrypoint, GLuint offset)
{
#if defined(USE_X86_ASM)
   GLubyte * const code = (GLubyte *) entrypoint;

#if DISPATCH_FUNCTION_SIZE == 32
   *((unsigned int *)(code + 11)) = 4 * offset;
   *((unsigned int *)(code + 22)) = 4 * offset;
#elif DISPATCH_FUNCTION_SIZE == 16 && defined( GLX_USE_TLS )
   *((unsigned int *)(code +  8)) = 4 * offset;
#elif DISPATCH_FUNCTION_SIZE == 16
   *((unsigned int *)(code +  7)) = 4 * offset;
#else
# error Invalid DISPATCH_FUNCTION_SIZE!
#endif

#elif defined(USE_SPARC_ASM)

   /* XXX this hasn't been tested! */
   unsigned int *code = (unsigned int *) entrypoint;
#ifdef __arch64__
   code[6] = 0x05000000;  /* sethi	%hi(8 * glapioffset), %g2	*/
   code[7] = 0x8410a000;  /* or		%g2, %lo(8 * glapioffset), %g2	*/
   code[6] |= ((offset * 8) >> 10);
   code[7] |= ((offset * 8) & ((1 << 10) - 1));
   __glapi_sparc_icache_flush(&code[6]);
#else /* __arch64__ */
   code[2] = 0xc6006000;  /* ld		[%g1 + %lo(4*glapioffset)], %g3	  */
   code[2] |= (offset * 4);
   __glapi_sparc_icache_flush(&code[2]);
#endif /* __arch64__ */

#else

   /* an unimplemented architecture */
   (void) entrypoint;
   (void) offset;

#endif /* USE_*_ASM */
}


/**
 * Generate new entrypoint
 *
 * Use a temporary dispatch offset of ~0 (i.e. -1).  Later, when the driver
 * calls \c _glapi_add_dispatch we'll put in the proper offset.  If that
 * never happens, and the user calls this function, he'll segfault.  That's
 * what you get when you try calling a GL function that doesn't really exist.
 * 
 * \param funcName  Name of the function to create an entry-point for.
 * 
 * \sa _glapi_add_entrypoint
 */

static struct _glapi_function *
add_function_name( const char * funcName )
{
   struct _glapi_function * entry = NULL;
   
   if (NumExtEntryPoints < MAX_EXTENSION_FUNCS) {
      _glapi_proc entrypoint = generate_entrypoint(~0);
      if (entrypoint != NULL) {
	 entry = & ExtEntryTable[NumExtEntryPoints];

	 ExtEntryTable[NumExtEntryPoints].name = str_dup(funcName);
	 ExtEntryTable[NumExtEntryPoints].parameter_signature = NULL;
	 ExtEntryTable[NumExtEntryPoints].dispatch_offset = ~0;
	 ExtEntryTable[NumExtEntryPoints].dispatch_stub = entrypoint;
	 NumExtEntryPoints++;
      }
   }

   return entry;
}


/**
 * Fill-in the dispatch stub for the named function.
 * 
 * This function is intended to be called by a hardware driver.  When called,
 * a dispatch stub may be created created for the function.  A pointer to this
 * dispatch function will be returned by glXGetProcAddress.
 *
 * \param function_names       Array of pointers to function names that should
 *                             share a common dispatch offset.
 * \param parameter_signature  String representing the types of the parameters
 *                             passed to the named function.  Parameter types
 *                             are converted to characters using the following
 *                             rules:
 *                               - 'i' for \c GLint, \c GLuint, and \c GLenum
 *                               - 'p' for any pointer type
 *                               - 'f' for \c GLfloat and \c GLclampf
 *                               - 'd' for \c GLdouble and \c GLclampd
 *
 * \returns
 * The offset in the dispatch table of the named function.  A pointer to the
 * driver's implementation of the named function should be stored at
 * \c dispatch_table[\c offset].  Return -1 if error/problem.
 *
 * \sa glXGetProcAddress
 *
 * \warning
 * This function can only handle up to 8 names at a time.  As far as I know,
 * the maximum number of names ever associated with an existing GL function is
 * 4 (\c glPointParameterfSGIS, \c glPointParameterfEXT,
 * \c glPointParameterfARB, and \c glPointParameterf), so this should not be
 * too painful of a limitation.
 *
 * \todo
 * Determine whether or not \c parameter_signature should be allowed to be
 * \c NULL.  It doesn't seem like much of a hardship for drivers to have to
 * pass in an empty string.
 *
 * \todo
 * Determine if code should be added to reject function names that start with
 * 'glX'.
 * 
 * \bug
 * Add code to compare \c parameter_signature with the parameter signature of
 * a static function.  In order to do that, we need to find a way to \b get
 * the parameter signature of a static function.
 */

PUBLIC int
_glapi_add_dispatch( const char * const * function_names,
		     const char * parameter_signature )
{
   static int next_dynamic_offset = _gloffset_FIRST_DYNAMIC;
   const char * const real_sig = (parameter_signature != NULL)
     ? parameter_signature : "";
   struct _glapi_function * entry[8];
   GLboolean is_static[8];
   unsigned i;
   unsigned j;
   int offset = ~0;
   int new_offset;


   (void) memset( is_static, 0, sizeof( is_static ) );
   (void) memset( entry, 0, sizeof( entry ) );

   for ( i = 0 ; function_names[i] != NULL ; i++ ) {
      /* Do some trivial validation on the name of the function.
       */

      if (!function_names[i] || function_names[i][0] != 'g' || function_names[i][1] != 'l')
         return -1;
   
      /* Determine if the named function already exists.  If the function does
       * exist, it must have the same parameter signature as the function
       * being added.
       */

      new_offset = get_static_proc_offset(function_names[i]);
      if (new_offset >= 0) {
	 /* FIXME: Make sure the parameter signatures match!  How do we get
	  * FIXME: the parameter signature for static functions?
	  */

	 if ( (offset != ~0) && (new_offset != offset) ) {
	    return -1;
	 }

	 is_static[i] = GL_TRUE;
	 offset = new_offset;
      }
   
   
      for ( j = 0 ; j < NumExtEntryPoints ; j++ ) {
	 if (strcmp(ExtEntryTable[j].name, function_names[i]) == 0) {
	    /* The offset may be ~0 if the function name was added by
	     * glXGetProcAddress but never filled in by the driver.
	     */

	    if (ExtEntryTable[j].dispatch_offset != ~0) {
	       if (strcmp(real_sig, ExtEntryTable[j].parameter_signature) 
		   != 0) {
		  return -1;
	       }

	       if ( (offset != ~0) && (ExtEntryTable[j].dispatch_offset != offset) ) {
		  return -1;
	       }

	       offset = ExtEntryTable[j].dispatch_offset;
	    }
	    
	    entry[i] = & ExtEntryTable[j];
	    break;
	 }
      }
   }

   if (offset == ~0) {
      offset = next_dynamic_offset;
      next_dynamic_offset++;
   }

   for ( i = 0 ; function_names[i] != NULL ; i++ ) {
      if (! is_static[i] ) {
	 if (entry[i] == NULL) {
	    entry[i] = add_function_name( function_names[i] );
	    if (entry[i] == NULL) {
	       /* FIXME: Possible memory leak here.
		*/
	       return -1;
	    }
	 }

	 entry[i]->parameter_signature = str_dup(real_sig);
	 fill_in_entrypoint_offset(entry[i]->dispatch_stub, offset);
	 entry[i]->dispatch_offset = offset;
      }
   }
   
   return offset;
}


/**
 * Return offset of entrypoint for named function within dispatch table.
 */
PUBLIC GLint
_glapi_get_proc_offset(const char *funcName)
{
   /* search extension functions first */
   GLuint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].name, funcName) == 0) {
         return ExtEntryTable[i].dispatch_offset;
      }
   }
   /* search static functions */
   return get_static_proc_offset(funcName);
}



/**
 * Return pointer to the named function.  If the function name isn't found
 * in the name of static functions, try generating a new API entrypoint on
 * the fly with assembly language.
 */
_glapi_proc
_glapi_get_proc_address(const char *funcName)
{
   struct _glapi_function * entry;
   GLuint i;

#ifdef MANGLE
   if (funcName[0] != 'm' || funcName[1] != 'g' || funcName[2] != 'l')
      return NULL;
#else
   if (funcName[0] != 'g' || funcName[1] != 'l')
      return NULL;
#endif

   /* search extension functions first */
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].name, funcName) == 0) {
         return ExtEntryTable[i].dispatch_stub;
      }
   }

#if !defined( XFree86Server ) && !defined( XGLServer )
   /* search static functions */
   {
      const _glapi_proc func = get_static_proc_address(funcName);
      if (func)
         return func;
   }
#endif /* !defined( XFree86Server ) */

   entry = add_function_name(funcName);
   return (entry == NULL) ? NULL : entry->dispatch_stub;
}



/**
 * Return the name of the function at the given dispatch offset.
 * This is only intended for debugging.
 */
const char *
_glapi_get_proc_name(GLuint offset)
{
   GLuint i;
   const char * n;

   /* search built-in functions */
   n = get_static_proc_name(offset);
   if ( n != NULL ) {
      return n;
   }

   /* search added extension functions */
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (ExtEntryTable[i].dispatch_offset == offset) {
         return ExtEntryTable[i].name;
      }
   }
   return NULL;
}
