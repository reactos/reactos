// TetiSoft: replaced vprintf() with KVPrintF() and commented out exit()
extern void __stdargs KVPrintF( const char *formatString, const void *values );

/***************************************************************************/
/*                                                                         */
/*  ftdebug.c                                                              */
/*                                                                         */
/*    Debugging and logging component (body).                              */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This component contains various macros and functions used to ease the */
  /* debugging of the FreeType engine.  Its main purpose is in assertion   */
  /* checking, tracing, and error detection.                               */
  /*                                                                       */
  /* There are now three debugging modes:                                  */
  /*                                                                       */
  /* - trace mode                                                          */
  /*                                                                       */
  /*   Error and trace messages are sent to the log file (which can be the */
  /*   standard error output).                                             */
  /*                                                                       */
  /* - error mode                                                          */
  /*                                                                       */
  /*   Only error messages are generated.                                  */
  /*                                                                       */
  /* - release mode:                                                       */
  /*                                                                       */
  /*   No error message is sent or generated.  The code is free from any   */
  /*   debugging parts.                                                    */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H


#ifdef FT_DEBUG_LEVEL_TRACE
  char  ft_trace_levels[trace_max];
#endif


#if defined( FT_DEBUG_LEVEL_ERROR ) || defined( FT_DEBUG_LEVEL_TRACE )


#include <stdarg.h>
#include <stdlib.h>


  FT_EXPORT_DEF( void )
  FT_Message( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
//  vprintf( fmt, ap );
    KVPrintF( fmt, ap );
    va_end( ap );
  }


  FT_EXPORT_DEF( void )
  FT_Panic( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
//  vprintf( fmt, ap );
    KVPrintF( fmt, ap );
    va_end( ap );

//  exit( EXIT_FAILURE );
  }



  /* since I don't know wether "getenv" is available on the Amiga */
  /* I prefer to simply disable this code for now in all builds   */
  /*                                                              */

/* #ifdef FT_DEBUG_LEVEL_TRACE */
#if 0

  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
    const char*  ft2_debug = getenv( "FT2_DEBUG" );


    if ( ft2_debug )
    {
      const char*  p = ft2_debug;
      const char*  q;


      for ( ; *p; p++ )
      {
        /* skip leading whitespace and separators */
        if ( *p == ' ' || *p == '\t' || *p == ',' || *p == ';' || *p == '=' )
          continue;

        /* read toggle name, followed by '=' */
        q = p;
        while ( *p && *p != ':' )
          p++;

        if ( *p == ':' && p > q )
        {
          int  n, i, len = p - q;
          int  level = -1, found = -1;


          for ( n = 0; n < trace_count; n++ )
          {
            const char*  toggle = ft_trace_toggles[n];


            for ( i = 0; i < len; i++ )
            {
              if ( toggle[i] != q[i] )
                break;
            }

            if ( i == len && toggle[i] == 0 )
            {
              found = n;
              break;
            }
          }

          /* read level */
          p++;
          if ( *p )
          {
            level = *p++ - '0';
            if ( level < 0 || level > 6 )
              level = -1;
          }

          if ( found >= 0 && level >= 0 )
          {
            if ( found == trace_any )
            {
              /* special case for "any" */
              for ( n = 0; n < trace_count; n++ )
                ft_trace_levels[n] = level;
            }
            else
              ft_trace_levels[found] = level;
          }
        }
      }
    }
  }


#else  /* !FT_DEBUG_LEVEL_TRACE */


  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
    /* nothing */
  }


#endif /* !FT_DEBUG_LEVEL_TRACE */


/* END */
