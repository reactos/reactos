/***************************************************************************/
/*                                                                         */
/*  ftdebug.h                                                              */
/*                                                                         */
/*    Debugging and logging component (specification).                     */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTDEBUG_H
#define FTDEBUG_H

#include <freetype/config/ftconfig.h>   /* for FT_DEBUG_LEVEL_TRACE, */
                                        /* FT_DEBUG_LEVEL_ERROR      */

#ifdef __cplusplus
  extern "C" {
#endif


  /* A very stupid pre-processor trick.  See K&R version 2 */
  /* section A12.3 for details...                          */
  /*                                                       */
  /* It is also described in the section `Separate         */
  /* Expansion of Macro Arguments' in the info file        */
  /* `cpp.info', describing GNU cpp.                       */
  /*                                                       */
#define FT_CAT( x, y )   x ## y
#define FT_XCAT( x, y )  FT_CAT( x, y )


#ifdef FT_DEBUG_LEVEL_TRACE


  /* note that not all levels are used currently */

  typedef enum  FT_Trace_
  {
    /* the first level must always be `trace_any' */
    trace_any = 0,

    /* base components */
    trace_aaraster,  /* anti-aliasing raster    (ftgrays.c)  */
    trace_calc,      /* calculations            (ftcalc.c)   */
    trace_extend,    /* extension manager       (ftextend.c) */
    trace_glyph,     /* glyph manager           (ftglyph.c)  */
    trace_io,        /* i/o monitoring          (ftsystem.c) */
    trace_init,      /* initialization          (ftinit.c)   */
    trace_list,      /* list manager            (ftlist.c)   */
    trace_memory,    /* memory manager          (ftobjs.c)   */
    trace_mm,        /* MM interface            (ftmm.c)     */
    trace_objs,      /* base objects            (ftobjs.c)   */
    trace_outline,   /* outline management      (ftoutln.c)  */
    trace_raster,    /* rasterizer              (ftraster.c) */
    trace_stream,    /* stream manager          (ftstream.c) */

    /* SFNT driver components */
    trace_sfobjs,    /* SFNT object handler     (sfobjs.c)   */
    trace_ttcmap,    /* charmap handler         (ttcmap.c)   */
    trace_ttload,    /* basic TrueType tables   (ttload.c)   */
    trace_ttpost,    /* PS table processing     (ttpost.c)   */
    trace_ttsbit,    /* TrueType sbit handling  (ttsbit.c)   */

    /* TrueType driver components */
    trace_ttdriver,  /* TT font driver          (ttdriver.c) */
    trace_ttgload,   /* TT glyph loader         (ttgload.c)  */
    trace_ttinterp,  /* bytecode interpreter    (ttinterp.c) */
    trace_ttobjs,    /* TT objects manager      (ttobjs.c)   */
    trace_ttpload,   /* TT data/program loader  (ttpload.c)  */

    /* Type 1 driver components */
    trace_t1driver,
    trace_t1gload,
    trace_t1hint,
    trace_t1load,
    trace_t1objs,

    /* experimental Type 1 driver components */
    trace_z1driver,
    trace_z1gload,
    trace_z1hint,
    trace_z1load,
    trace_z1objs,
    trace_z1parse,

    /* Type 2 driver components */
    trace_t2driver,
    trace_t2gload,
    trace_t2load,
    trace_t2objs,
    trace_t2parse,

    /* CID driver components */
    trace_cidafm,
    trace_ciddriver,
    trace_cidgload,
    trace_cidload,
    trace_cidobjs,
    trace_cidparse,

    /* Windows fonts component */
    trace_winfnt,

    /* the last level must always be `trace_max' */
    trace_max

  } FT_Trace;


  /* declared in ftdebug.c */
  extern char  ft_trace_levels[trace_max];


  /*************************************************************************/
  /*                                                                       */
  /* IMPORTANT!                                                            */
  /*                                                                       */
  /* Each component must define the macro FT_COMPONENT to a valid FT_Trace */
  /* value before using any TRACE macro.                                   */
  /*                                                                       */
  /*************************************************************************/


#define FT_TRACE( level, varformat )                      \
          do                                              \
          {                                               \
            if ( ft_trace_levels[FT_COMPONENT] >= level ) \
              FT_XCAT( FT_Message, varformat );           \
          } while ( 0 )


  FT_EXPORT_DEF( void )  FT_SetTraceLevel( FT_Trace  component,
                                           char      level );


#elif defined( FT_DEBUG_LEVEL_ERROR )


#define FT_TRACE( level, varformat )  do ; while ( 0 )      /* nothing */


#else  /* release mode */


#define FT_Assert( condition )        do ; while ( 0 )      /* nothing */

#define FT_TRACE( level, varformat )  do ; while ( 0 )      /* nothing */
#define FT_ERROR( varformat )         do ; while ( 0 )      /* nothing */


#endif /* FT_DEBUG_LEVEL_TRACE, FT_DEBUG_LEVEL_ERROR */


  /*************************************************************************/
  /*                                                                       */
  /* Define macros and functions that are common to the debug and trace    */
  /* modes.                                                                */
  /*                                                                       */
  /* You need vprintf() to be able to compile ftdebug.c.                   */
  /*                                                                       */
  /*************************************************************************/


#if defined( FT_DEBUG_LEVEL_TRACE ) || defined( FT_DEBUG_LEVEL_ERROR )


#include "stdio.h"  /* for vprintf() */


#define FT_Assert( condition )                                      \
          do                                                        \
          {                                                         \
            if ( !( condition ) )                                   \
              FT_Panic( "assertion failed on line %d of file %s\n", \
                        __LINE__, __FILE__ );                       \
          } while ( 0 )

  /* print a message */
  FT_EXPORT_DEF( void )  FT_Message( const char*  fmt, ... );

  /* print a message and exit */
  FT_EXPORT_DEF( void )  FT_Panic( const char*  fmt, ... );

#define FT_ERROR( varformat )  FT_XCAT( FT_Message, varformat )


#endif /* FT_DEBUG_LEVEL_TRACE || FT_DEBUG_LEVEL_ERROR */


  /*************************************************************************/
  /*                                                                       */
  /* You need two opening resp. closing parentheses!                       */
  /*                                                                       */
  /* Example: FT_TRACE0(( "Value is %i", foo ))                            */
  /*                                                                       */
  /*************************************************************************/

#define FT_TRACE0( varformat )  FT_TRACE( 0, varformat )
#define FT_TRACE1( varformat )  FT_TRACE( 1, varformat )
#define FT_TRACE2( varformat )  FT_TRACE( 2, varformat )
#define FT_TRACE3( varformat )  FT_TRACE( 3, varformat )
#define FT_TRACE4( varformat )  FT_TRACE( 4, varformat )
#define FT_TRACE5( varformat )  FT_TRACE( 5, varformat )
#define FT_TRACE6( varformat )  FT_TRACE( 6, varformat )
#define FT_TRACE7( varformat )  FT_TRACE( 7, varformat )


#ifdef __cplusplus
  }
#endif


#endif /* FTDEBUG_H */


/* END */
