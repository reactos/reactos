
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 * This file manages the OpenGL API dispatch layer.
 * The dispatch table (struct _glapi_table) is basically just a list
 * of function pointers.
 * There are functions to set/get the current dispatch table for the
 * current thread and to manage registration/dispatch of dynamically
 * added extension functions.
 *
 * It's intended that this file and the other glapi*.[ch] files are
 * flexible enough to be reused in several places:  XFree86, DRI-
 * based libGL.so, and perhaps the SGI SI.
 *
 * NOTE: There are no dependencies on Mesa in this code.
 *
 * Versions (API changes):
 *   2000/02/23  - original version for Mesa 3.3 and XFree86 4.0
 *   2001/01/16  - added dispatch override feature for Mesa 3.5
 *   2002/06/28  - added _glapi_set_warning_func(), Mesa 4.1.
 *   2002/10/01  - _glapi_get_proc_address() will now generate new entrypoints
 *                 itself (using offset ~0).  _glapi_add_entrypoint() can be
 *                 called afterward and it'll fill in the correct dispatch
 *                 offset.  This allows DRI libGL to avoid probing for DRI
 *                 drivers!  No changes to the public glapi interface.
 */



#include "glheader.h"
#include "glapi.h"
#include "glapioffsets.h"
#include "glapitable.h"
#include "glthread.h"

/***** BEGIN NO-OP DISPATCH *****/

static GLboolean WarnFlag = GL_FALSE;
static _glapi_warning_func warning_func;


/*
 * Enable/disable printing of warning messages.
 */
void
_glapi_noop_enable_warnings(GLboolean enable)
{
   WarnFlag = enable;
}

/*
 * Register a callback function for reporting errors.
 */
void
_glapi_set_warning_func( _glapi_warning_func func )
{
   warning_func = func;
}

static GLboolean
warn(void)
{
   if ((WarnFlag || getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
       && warning_func) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


#define KEYWORD1 static
#define KEYWORD2 GLAPIENTRY
#define NAME(func)  NoOp##func

#define F NULL

#define DISPATCH(func, args, msg)					\
   if (warn()) {							\
      warning_func(NULL, "GL User Error: called without context:");	\
      warning_func msg;							\
   }

#define RETURN_DISPATCH(func, args, msg)				\
   if (warn()) {							\
      warning_func(NULL, "GL User Error: called without context:");	\
      warning_func msg;							\
   }									\
   return 0

#define DISPATCH_TABLE_NAME __glapi_noop_table
#define UNUSED_TABLE_NAME __usused_noop_functions

#define TABLE_ENTRY(name) (void *) (int)NoOp##name

static int NoOpUnused(void)
{
   if (warn()) {
      warning_func(NULL, "GL User Error: calling extension function without a current context\n");
   }
   return 0;
}

#include "glapitemp.h"

/***** END NO-OP DISPATCH *****/



/***** BEGIN THREAD-SAFE DISPATCH *****/

#if defined(THREADS)

/**
 * \name Multi-threaded control support variables
 *
 * If thread-safety is supported, there are two potential mechanisms that can
 * be used.  The old-style mechanism would set \c _glapi_Dispatch to a special
 * thread-safe dispatch table.  These dispatch routines would call
 * \c _glapi_get_dispatch to get the actual dispatch pointer.  In this
 * setup \c _glapi_Dispatch could never be \c NULL.  This dual layered
 * dispatch setup performed great for single-threaded apps, but didn't
 * perform well for multithreaded apps.
 *
 * In the new mechansim, there are two variables.  The first is
 * \c _glapi_DispatchTSD.  In the single-threaded case, this variable points
 * to the dispatch table.  In the multi-threaded case, this variable is
 * \c NULL, and thread-specific variable \c _gl_DispatchTSD points to the
 * actual dispatch table.  \c _glapi_DispatchTSD is used to signal to the
 * static dispatch functions to call \c _glapi_get_dispatch to get the real
 * dispatch table.
 * 
 * There is a race condition in setting \c _glapi_DispatchTSD to \c NULL.
 * It is possible for the original thread to be setting it at the same instant
 * a new thread, perhaps running on a different processor, is clearing it.
 * Because of that, \c ThreadSafe, which can only ever be changed to
 * \c GL_TRUE, is used to determine whether or not the application is
 * multithreaded.
 */
/*@{*/
static GLboolean ThreadSafe = GL_FALSE;  /**< In thread-safe mode? */
_glthread_TSD _gl_DispatchTSD;           /**< Per-thread dispatch pointer */
static _glthread_TSD RealDispatchTSD;    /**< only when using override */
static _glthread_TSD ContextTSD;         /**< Per-thread context pointer */
/*@}*/


#define DISPATCH_TABLE_NAME __glapi_threadsafe_table
#define UNUSED_TABLE_NAME __usused_threadsafe_functions

#define TABLE_ENTRY(name) (void *) gl##name

static int glUnused(void)
{
   return 0;
}

#include "glapitemp.h"

#endif

/***** END THREAD-SAFE DISPATCH *****/



struct _glapi_table *_glapi_Dispatch = (struct _glapi_table *) __glapi_noop_table;
#if defined( THREADS )
struct _glapi_table *_glapi_DispatchTSD = (struct _glapi_table *) __glapi_noop_table;
#endif
struct _glapi_table *_glapi_RealDispatch = (struct _glapi_table *) __glapi_noop_table;


/* Used when thread safety disabled */
void *_glapi_Context = NULL;


static GLboolean DispatchOverride = GL_FALSE;



/* strdup() is actually not a standard ANSI C or POSIX routine.
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



/*
 * We should call this periodically from a function such as glXMakeCurrent
 * in order to test if multiple threads are being used.
 */
void
_glapi_check_multithread(void)
{
#if defined(THREADS)
   if (!ThreadSafe) {
      static unsigned long knownID;
      static GLboolean firstCall = GL_TRUE;
      if (firstCall) {
         knownID = _glthread_GetID();
         firstCall = GL_FALSE;
      }
      else if (knownID != _glthread_GetID()) {
         ThreadSafe = GL_TRUE;
         _glapi_set_dispatch(NULL);
      }
   }
   else if (!_glapi_get_dispatch()) {
      /* make sure that this thread's dispatch pointer isn't null */
      _glapi_set_dispatch(NULL);
   }
#endif
}



/*
 * Set the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast to
 * void from the real context pointer type.
 */
void
_glapi_set_context(void *context)
{
#if defined(THREADS)
   _glthread_SetTSD(&ContextTSD, context);
   _glapi_Context = (ThreadSafe) ? NULL : context;
#else
   _glapi_Context = context;
#endif
}



/*
 * Get the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast from
 * void to the real context pointer type.
 */
void *
_glapi_get_context(void)
{
#if defined(THREADS)
   if (ThreadSafe) {
      return _glthread_GetTSD(&ContextTSD);
   }
   else {
      return _glapi_Context;
   }
#else
   return _glapi_Context;
#endif
}



/*
 * Set the global or per-thread dispatch table pointer.
 */
void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
   if (!dispatch) {
      /* use the no-op functions */
      dispatch = (struct _glapi_table *) __glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(THREADS)
   if (DispatchOverride) {
      _glthread_SetTSD(&RealDispatchTSD, (void *) dispatch);
      if (ThreadSafe)
         _glapi_RealDispatch = (struct _glapi_table*) __glapi_threadsafe_table;
      else
         _glapi_RealDispatch = dispatch;
   }
   else {
      /* normal operation */
      _glthread_SetTSD(&_gl_DispatchTSD, (void *) dispatch);
      if (ThreadSafe) {
	 _glapi_Dispatch = (struct _glapi_table *) __glapi_threadsafe_table;
	 _glapi_DispatchTSD = NULL;
      }
      else {
	 _glapi_Dispatch = dispatch;
	 _glapi_DispatchTSD = dispatch;
      }
   }
#else /*THREADS*/
   if (DispatchOverride) {
      _glapi_RealDispatch = dispatch;
   }
   else {
      _glapi_Dispatch = dispatch;
   }
#endif /*THREADS*/
}



/*
 * Return pointer to current dispatch table for calling thread.
 */
struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(THREADS)
   if (ThreadSafe) {
      if (DispatchOverride) {
         return (struct _glapi_table *) _glthread_GetTSD(&RealDispatchTSD);
      }
      else {
         return (struct _glapi_table *) _glthread_GetTSD(&_gl_DispatchTSD);
      }
   }
   else {
      if (DispatchOverride) {
         assert(_glapi_RealDispatch);
         return _glapi_RealDispatch;
      }
      else {
         assert(_glapi_DispatchTSD);
         return _glapi_DispatchTSD;
      }
   }
#else
   return _glapi_Dispatch;
#endif
}


/*
 * Notes on dispatch overrride:
 *
 * Dispatch override allows an external agent to hook into the GL dispatch
 * mechanism before execution goes into the core rendering library.  For
 * example, a trace mechanism would insert itself as an overrider, print
 * logging info for each GL function, then dispatch to the real GL function.
 *
 * libGLS (GL Stream library) is another agent that might use override.
 *
 * We don't allow more than one layer of overriding at this time.
 * In the future we may allow nested/layered override.  In that case
 * _glapi_begin_dispatch_override() will return an override layer,
 * _glapi_end_dispatch_override(layer) will remove an override layer
 * and _glapi_get_override_dispatch(layer) will return the dispatch
 * table for a given override layer.  layer = 0 will be the "real"
 * dispatch table.
 */

/*
 * Return: dispatch override layer number.
 */
int
_glapi_begin_dispatch_override(struct _glapi_table *override)
{
   struct _glapi_table *real = _glapi_get_dispatch();

   assert(!DispatchOverride);  /* can't nest at this time */
   DispatchOverride = GL_TRUE;

   _glapi_set_dispatch(real);

#if defined(THREADS)
   _glthread_SetTSD(&_gl_DispatchTSD, (void *) override);
   if ( ThreadSafe ) {
      _glapi_Dispatch = (struct _glapi_table *) __glapi_threadsafe_table;
      _glapi_DispatchTSD = NULL;
   }
   else {
      _glapi_Dispatch = override;
      _glapi_DispatchTSD = override;
   }
#else
   _glapi_Dispatch = override;
#endif
   return 1;
}


void
_glapi_end_dispatch_override(int layer)
{
   struct _glapi_table *real = _glapi_get_dispatch();
   (void) layer;
   DispatchOverride = GL_FALSE;
   _glapi_set_dispatch(real);
   /* the rest of this isn't needed, just play it safe */
#if defined(THREADS)
   _glthread_SetTSD(&RealDispatchTSD, NULL);
#endif
   _glapi_RealDispatch = NULL;
}


struct _glapi_table *
_glapi_get_override_dispatch(int layer)
{
   if (layer == 0) {
      return _glapi_get_dispatch();
   }
   else {
      if (DispatchOverride) {
#if defined(THREADS)
         return (struct _glapi_table *) _glthread_GetTSD(&_gl_DispatchTSD);
#else
         return _glapi_Dispatch;
#endif
      }
      else {
         return NULL;
      }
   }
}


struct name_address_offset {
   const char *Name;
   GLvoid *Address;
   GLuint Offset;
};


#if !defined( USE_X86_ASM )
#define NEED_FUNCTION_POINTER
#endif

/* The code in this file is auto-generated with Python */
#include "glprocs.h"


static const glprocs_table_t *
find_entry( const char * n )
{
   unsigned   i;

   for ( i = 0 ; static_functions[i].Name_offset >= 0 ; i++ ) {
      const char * test_name;

      test_name = gl_string_table + static_functions[i].Name_offset;
      if (strcmp(test_name, n) == 0) {
	 return & static_functions[i];
      }
   }
   return NULL;
}

/*
 * Return dispatch table offset of the named static (built-in) function.
 * Return -1 if function not found.
 */
static GLint
get_static_proc_offset(const char *funcName)
{
   const glprocs_table_t * const f = find_entry( funcName );

   if ( f != NULL ) {
      return f->Offset;
   }
   return -1;
}


#ifdef USE_X86_ASM
extern const GLubyte gl_dispatch_functions_start[];

# if defined(THREADS)
#  define X86_DISPATCH_FUNCTION_SIZE  32
# else
#  define X86_DISPATCH_FUNCTION_SIZE  16
# endif


/*
 * Return dispatch function address the named static (built-in) function.
 * Return NULL if function not found.
 */
static const GLvoid *
get_static_proc_address(const char *funcName)
{
   const glprocs_table_t * const f = find_entry( funcName );

   if ( f != NULL ) {
      return gl_dispatch_functions_start 
	   + (X86_DISPATCH_FUNCTION_SIZE * f->Offset);
   }
   else {
      return NULL;
   }
}

#else


/*
 * Return dispatch function address the named static (built-in) function.
 * Return NULL if function not found.
 */
static const GLvoid *
get_static_proc_address(const char *funcName)
{
   const glprocs_table_t * const f = find_entry( funcName );
   return ( f != NULL ) ? f->Address : NULL;
}

#endif /* USE_X86_ASM */


static const char *
get_static_proc_name( GLuint offset )
{
   unsigned   i;

   for ( i = 0 ; static_functions[i].Name_offset >= 0 ; i++ ) {
      if (static_functions[i].Offset == offset) {
	 return gl_string_table + static_functions[i].Name_offset;
      }
   }
   return NULL;
}



/**********************************************************************
 * Extension function management.
 */

/*
 * Number of extension functions which we can dynamically add at runtime.
 */
#define MAX_EXTENSION_FUNCS 300


/*
 * The disptach table size (number of entries) is the sizeof the
 * _glapi_table struct plus the number of dynamic entries we can add.
 * The extra slots can be filled in by DRI drivers that register new extension
 * functions.
 */
#define DISPATCH_TABLE_SIZE (sizeof(struct _glapi_table) / sizeof(void *) + MAX_EXTENSION_FUNCS)


static struct name_address_offset ExtEntryTable[MAX_EXTENSION_FUNCS];
static GLuint NumExtEntryPoints = 0;

#ifdef USE_SPARC_ASM
extern void __glapi_sparc_icache_flush(unsigned int *);
#endif

/*
 * Generate a dispatch function (entrypoint) which jumps through
 * the given slot number (offset) in the current dispatch table.
 * We need assembly language in order to accomplish this.
 */
static void *
generate_entrypoint(GLuint functionOffset)
{
#if defined(USE_X86_ASM)
   /*
    * This x86 code contributed by Josh Vanderhoof.
    *
    *  0:   a1 10 32 54 76          movl   __glapi_Dispatch,%eax
    *       00 01 02 03 04
    *  5:   85 c0                   testl  %eax,%eax
    *       05 06
    *  7:   74 06                   je     f <entrypoint+0xf>
    *       07 08
    *  9:   ff a0 10 32 54 76       jmp    *0x76543210(%eax)
    *       09 0a 0b 0c 0d 0e
    *  f:   e8 fc ff ff ff          call   __glapi_get_dispatch
    *       0f 10 11 12 13
    * 14:   ff a0 10 32 54 76       jmp    *0x76543210(%eax)
    *       14 15 16 17 18 19
    */
   static const unsigned char insn_template[] = {
      0xa1, 0x00, 0x00, 0x00, 0x00,
      0x85, 0xc0,
      0x74, 0x06,
      0xff, 0xa0, 0x00, 0x00, 0x00, 0x00,
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xff, 0xa0, 0x00, 0x00, 0x00, 0x00
   };
   unsigned char *code = (unsigned char *) malloc(sizeof(insn_template));
   unsigned int next_insn;
   if (code) {
      memcpy(code, insn_template, sizeof(insn_template));

#if defined( THREADS )
      *(unsigned int *)(code + 0x01) = (unsigned int)&_glapi_DispatchTSD;
#else
      *(unsigned int *)(code + 0x01) = (unsigned int)&_glapi_Dispatch;
#endif
      *(unsigned int *)(code + 0x0b) = (unsigned int)functionOffset * 4;
      next_insn = (unsigned int)(code + 0x14);
      *(unsigned int *)(code + 0x10) = (unsigned int)_glapi_get_dispatch - next_insn;
      *(unsigned int *)(code + 0x16) = (unsigned int)functionOffset * 4;
   }
   return code;
#elif defined(USE_SPARC_ASM)

#if defined(__sparc_v9__) && !defined(__linux__)
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
#endif
   unsigned int *code = (unsigned int *) malloc(sizeof(insn_template));
   unsigned long glapi_addr = (unsigned long) &_glapi_Dispatch;
   if (code) {
      memcpy(code, insn_template, sizeof(insn_template));

#if defined(__sparc_v9__) && !defined(__linux__)
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
#endif
   }
   return code;
#else
   (void) functionOffset;
   return NULL;
#endif /* USE_*_ASM */
}


/*
 * This function inserts a new dispatch offset into the assembly language
 * stub that was generated with the preceeding function.
 */
static void
fill_in_entrypoint_offset(void *entrypoint, GLuint offset)
{
#if defined(USE_X86_ASM)

   unsigned char *code = (unsigned char *) entrypoint;
   *(unsigned int *)(code + 0x0b) = offset * 4;
   *(unsigned int *)(code + 0x16) = offset * 4;

#elif defined(USE_SPARC_ASM)

   /* XXX this hasn't been tested! */
   unsigned int *code = (unsigned int *) entrypoint;
#if defined(__sparc_v9__) && !defined(__linux__)
   code[6] = 0x05000000;  /* sethi	%hi(8 * glapioffset), %g2	*/
   code[7] = 0x8410a000;  /* or		%g2, %lo(8 * glapioffset), %g2	*/
   code[6] |= ((offset * 8) >> 10);
   code[7] |= ((offset * 8) & ((1 << 10) - 1));
   __glapi_sparc_icache_flush(&code[6]);
#else /* __sparc_v9__ && !linux */
   code[2] = 0xc6006000;  /* ld		[%g1 + %lo(4*glapioffset)], %g3	  */
   code[2] |= (offset * 4);
   __glapi_sparc_icache_flush(&code[2]);
#endif /* __sparc_v9__ && !linux */

#else
   (void) entrypoint; (void) offset;
#endif /* USE_*_ASM */
}


/*
 * Add a new extension function entrypoint.
 * Return: GL_TRUE = success or GL_FALSE = failure
 */
GLboolean
_glapi_add_entrypoint(const char *funcName, GLuint offset)
{
   /* trivial rejection test */
#ifdef MANGLE
   if (!funcName || funcName[0] != 'm' || funcName[1] != 'g' || funcName[2] != 'l')
      return GL_FALSE;
#else
   if (!funcName || funcName[0] != 'g' || funcName[1] != 'l')
      return GL_FALSE;
#endif

   /* first check if the named function is already statically present */
   {
      GLint index = get_static_proc_offset(funcName);
      if (index >= 0) {
         return (GLboolean) ((GLuint) index == offset);  /* bad offset! */
      }
   }

   /* See if this function has already been dynamically added */
   {
      GLuint i;
      for (i = 0; i < NumExtEntryPoints; i++) {
         if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
            /* function already registered */
            if (ExtEntryTable[i].Offset == offset) {
               return GL_TRUE;  /* offsets match */
            }
            else if (ExtEntryTable[i].Offset == (GLuint) ~0
                     && offset < DISPATCH_TABLE_SIZE) {
               /* need to patch-up the dispatch code */
               if (offset != (GLuint) ~0) {
                  fill_in_entrypoint_offset(ExtEntryTable[i].Address, offset);
                  ExtEntryTable[i].Offset = offset;
               }
               return GL_TRUE;
            }
            else {
               return GL_FALSE;  /* bad offset! */
            }
         }
      }
   }

   /* This is a new function, try to add it.  */
   if (NumExtEntryPoints >= MAX_EXTENSION_FUNCS ||
       offset >= DISPATCH_TABLE_SIZE) {
      /* No space left */
      return GL_FALSE;
   }
   else {
      void *entrypoint = generate_entrypoint(offset);
      if (!entrypoint)
         return GL_FALSE; /* couldn't generate assembly */

      /* OK! */
      ExtEntryTable[NumExtEntryPoints].Name = str_dup(funcName);
      ExtEntryTable[NumExtEntryPoints].Offset = offset;
      ExtEntryTable[NumExtEntryPoints].Address = entrypoint;
      NumExtEntryPoints++;

      return GL_TRUE;  /* success */
   }

   /* should never get here, silence compiler warnings */
   return GL_FALSE;
}


/*
 * Return offset of entrypoint for named function within dispatch table.
 */
GLint
_glapi_get_proc_offset(const char *funcName)
{
   /* search extension functions first */
   GLuint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
         return ExtEntryTable[i].Offset;
      }
   }

   /* search static functions */
   return get_static_proc_offset(funcName);
}



/*
 * Return entrypoint for named function.
 */
const GLvoid *
_glapi_get_proc_address(const char *funcName)
{
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
      if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
         return ExtEntryTable[i].Address;
      }
   }

   /* search static functions */
   {
      const GLvoid *func = get_static_proc_address(funcName);
      if (func)
         return func;
   }

   /* generate new entrypoint - use a temporary dispatch offset of
    * ~0 (i.e. -1).  Later, when the driver calls _glapi_add_entrypoint()
    * we'll put in the proper offset.  If that never happens, and the
    * user calls this function, he'll segfault.  That's what you get
    * when you try calling a GL function that doesn't really exist.
    */
   if (NumExtEntryPoints < MAX_EXTENSION_FUNCS) {
      GLvoid *entrypoint = generate_entrypoint(~0);
      if (!entrypoint)
         return GL_FALSE;

      ExtEntryTable[NumExtEntryPoints].Name = str_dup(funcName);
      ExtEntryTable[NumExtEntryPoints].Offset = ~0;
      ExtEntryTable[NumExtEntryPoints].Address = entrypoint;
      NumExtEntryPoints++;

      return entrypoint;
   }
   else {
      /* no space for new functions! */
      return NULL;
   }
}



/*
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
      if (ExtEntryTable[i].Offset == offset) {
         return ExtEntryTable[i].Name;
      }
   }
   return NULL;
}



/*
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
GLuint
_glapi_get_dispatch_table_size(void)
{
   return DISPATCH_TABLE_SIZE;
}



/*
 * Get API dispatcher version string.
 */
const char *
_glapi_get_version(void)
{
   return "20021001";  /* YYYYMMDD */
}



/*
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intended for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
#ifdef DEBUG
   const GLuint entries = _glapi_get_dispatch_table_size();
   const void **tab = (const void **) table;
   GLuint i;
   for (i = 1; i < entries; i++) {
      assert(tab[i]);
   }

   /* Do some spot checks to be sure that the dispatch table
    * slots are assigned correctly.
    */
   {
      GLuint BeginOffset = _glapi_get_proc_offset("glBegin");
      char *BeginFunc = (char*) &table->Begin;
      GLuint offset = (BeginFunc - (char *) table) / sizeof(void *);
      assert(BeginOffset == _gloffset_Begin);
      assert(BeginOffset == offset);
   }
   {
      GLuint viewportOffset = _glapi_get_proc_offset("glViewport");
      char *viewportFunc = (char*) &table->Viewport;
      GLuint offset = (viewportFunc - (char *) table) / sizeof(void *);
      assert(viewportOffset == _gloffset_Viewport);
      assert(viewportOffset == offset);
   }
   {
      GLuint VertexPointerOffset = _glapi_get_proc_offset("glVertexPointer");
      char *VertexPointerFunc = (char*) &table->VertexPointer;
      GLuint offset = (VertexPointerFunc - (char *) table) / sizeof(void *);
      assert(VertexPointerOffset == _gloffset_VertexPointer);
      assert(VertexPointerOffset == offset);
   }
   {
      GLuint ResetMinMaxOffset = _glapi_get_proc_offset("glResetMinmax");
      char *ResetMinMaxFunc = (char*) &table->ResetMinmax;
      GLuint offset = (ResetMinMaxFunc - (char *) table) / sizeof(void *);
      assert(ResetMinMaxOffset == _gloffset_ResetMinmax);
      assert(ResetMinMaxOffset == offset);
   }
   {
      GLuint blendColorOffset = _glapi_get_proc_offset("glBlendColor");
      char *blendColorFunc = (char*) &table->BlendColor;
      GLuint offset = (blendColorFunc - (char *) table) / sizeof(void *);
      assert(blendColorOffset == _gloffset_BlendColor);
      assert(blendColorOffset == offset);
   }
   {
      GLuint istextureOffset = _glapi_get_proc_offset("glIsTextureEXT");
      char *istextureFunc = (char*) &table->IsTextureEXT;
      GLuint offset = (istextureFunc - (char *) table) / sizeof(void *);
      assert(istextureOffset == _gloffset_IsTextureEXT);
      assert(istextureOffset == offset);
   }
   {
      GLuint secondaryColor3fOffset = _glapi_get_proc_offset("glSecondaryColor3fEXT");
      char *secondaryColor3fFunc = (char*) &table->SecondaryColor3fEXT;
      GLuint offset = (secondaryColor3fFunc - (char *) table) / sizeof(void *);
      assert(secondaryColor3fOffset == _gloffset_SecondaryColor3fEXT);
      assert(secondaryColor3fOffset == offset);
      assert(_glapi_get_proc_address("glSecondaryColor3fEXT") == (void *) &glSecondaryColor3fEXT);
   }
   {
      GLuint pointParameterivOffset = _glapi_get_proc_offset("glPointParameterivNV");
      char *pointParameterivFunc = (char*) &table->PointParameterivNV;
      GLuint offset = (pointParameterivFunc - (char *) table) / sizeof(void *);
      assert(pointParameterivOffset == _gloffset_PointParameterivNV);
      assert(pointParameterivOffset == offset);
      assert(_glapi_get_proc_address("glPointParameterivNV") == (void *) &glPointParameterivNV);
   }
   {
      GLuint setFenceOffset = _glapi_get_proc_offset("glSetFenceNV");
      char *setFenceFunc = (char*) &table->SetFenceNV;
      GLuint offset = (setFenceFunc - (char *) table) / sizeof(void *);
      assert(setFenceOffset == _gloffset_SetFenceNV);
      assert(setFenceOffset == offset);
      assert(_glapi_get_proc_address("glSetFenceNV") == (void *) &glSetFenceNV);
   }
#else
   (void) table;
#endif
}
