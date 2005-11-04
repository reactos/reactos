/* $Id: debug.h,v 1.5 2003/09/22 15:18:51 brianp Exp $
******************************************************************************

   GGIMesa debugging macros

   Copyright (C) 1998-1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 1999-2000 Jon Taylor		[taylorj@ggi-project.org]
  
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#ifndef _GGI_MESA_INTERNAL_DEBUG_H
#define _GGI_MESA_INTERNAL_DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <ggi/types.h>
#include <ggi/gg.h>

#ifndef DEBUG
#define DEBUG
#endif

__BEGIN_DECLS

/* Exported variables */
#ifdef BUILDING_GGIMESA
extern uint32     _ggimesaDebugState;
extern int        _ggimesaDebugSync;
#else
IMPORTVAR uint32  _ggimesaDebugState;
IMPORTVAR int     _ggimesaDebugSync;
#endif

__END_DECLS


/* Debugging types
 * bit 0 is reserved! */

#define GGIMESADEBUG_CORE		(1<<1)	/*   2 */
#define GGIMESADEBUG_MODE		(1<<2)	/*   4 */
#define GGIMESADEBUG_COLOR		(1<<3)	/*   8 */
#define GGIMESADEBUG_DRAW		(1<<4)	/*  16 */
#define GGIMESADEBUG_MISC		(1<<5)	/*  32 */
#define GGIMESADEBUG_LIBS		(1<<6)	/*  64 */
#define GGIMESADEBUG_EVENTS		(1<<7)	/* 128 */

#define GGIMESADEBUG_ALL	0xffffffff

#ifdef __GNUC__

#ifdef DEBUG
#define GGIMESADPRINT(args...)		if (_ggimesaDebugState) { ggDPrintf(_ggimesaDebugSync, "GGIMesa",args); }
#define GGIMESADPRINT_CORE(args...)	if (_ggimesaDebugState & GGIMESADEBUG_CORE) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#define GGIMESADPRINT_MODE(args...)	if (_ggimesaDebugState & GGIMESADEBUG_MODE) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#define GGIMESADPRINT_COLOR(args...)	if (_ggimesaDebugState & GGIMESADEBUG_COLOR) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#define GGIMESADPRINT_DRAW(args...)	if (_ggimesaDebugState & GGIMESADEBUG_DRAW) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#define GGIMESADPRINT_MISC(args...)	if (_ggimesaDebugState & GGIMESADEBUG_MISC) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#define GGIMESADPRINT_LIBS(args...)	if (_ggimesaDebugState & GGIMESADEBUG_LIBS) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#define GGIMESADPRINT_EVENTS(args...)	if (_ggimesaDebugState & GGIMESADEBUG_EVENTS) { ggDPrintf(_ggimesaDebugSync,"GGIMesa",args); }
#else /* DEBUG */
#define GGIMESADPRINT(args...)		do{}while(0)
#define GGIMESADPRINT_CORE(args...)	do{}while(0)
#define GGIMESADPRINT_MODE(args...)	do{}while(0)
#define GGIMESADPRINT_COLOR(args...)	do{}while(0)
#define GGIMESADPRINT_DRAW(args...)	do{}while(0)
#define GGIMESADPRINT_MISC(args...)	do{}while(0)
#define GGIMESADPRINT_LIBS(args...)	do{}while(0)
#define GGIMESADPRINT_EVENTS(args...)	do{}while(0)
#endif /* DEBUG */

#else /* __GNUC__ */

__BEGIN_DECLS

static inline void GGIMESADPRINT(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_CORE(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_CORE) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_MODE(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_MODE) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_COLOR(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_COLOR) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_DRAW(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_DRAW) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_MISC(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_MISC) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_LIBS(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_LIBS) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIMESADPRINT_EVENTS(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_EVENTS) {
		va_list args;

		fprintf(stderr, "GGIMesa: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggimesaDebugSync) fflush(stderr);
	}
#endif
}

__END_DECLS

#endif /* __GNUC__ */

#ifdef DEBUG
#define GGIMESA_ASSERT(x,str) \
{ if (!(x)) { \
	fprintf(stderr,"GGIMESA:%s:%d: INTERNAL ERROR: %s\n",__FILE__,__LINE__,str); \
	exit(1); \
} }
#define GGIMESA_APPASSERT(x,str) \
{ if (!(x)) { \
	fprintf(stderr,"GGIMESA:%s:%d: APPLICATION ERROR: %s\n",__FILE__,__LINE__,str); \
	exit(1); \
} }
#else /* DEBUG */
#define GGIMESA_ASSERT(x,str)	do{}while(0)
#define GGIMESA_APPASSERT(x,str)	do{}while(0)
#endif /* DEBUG */

#ifdef DEBUG
# define GGIMESAD0(x)	x
#else
# define GGIMESAD0(x)	/* empty */
#endif

#ifdef GGIMESADLEV
# if GGIMESADLEV == 1
#  define GGIMESAD1(x)	x
#  define GGIMESAD2(x)	/* empty */
#  define GGIMESAD3(x)	/* empty */
# elif GGIMESADLEV == 2
#  define GGIMESAD1(x)	x
#  define GGIMESAD2(x)	x
#  define GGIMESAD3(x)	/* empty */
# elif GGIMESADLEV > 2
#  define GGIMESAD1(x)	x
#  define GGIMESAD2(x)	x
#  define GGIMESAD3(x)	x
# endif
#else
# define GGIMESAD1(x)	/* empty */
# define GGIMESAD2(x)	/* empty */
# define GGIMESAD3(x)	/* empty */
#endif

#endif /* _GGI_MESA_INTERNAL_DEBUG_H */
