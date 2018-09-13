/*
 -  L O G I T . H
 -
 *  Purpose:
 *      Function and Macro definitions for logging module activity.
 *
 *  Author: Glenn A. Curtis
 *
 *  Comments:
 *      10/28/93    glennc     original file.
 *
 */

#ifndef LOGIT_H
#define LOGIT_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#if DBG
   void  LogInit(void);
   void  CDECL LogIt( char *, ... );
   DWORD LogIn( char * );
   void  LogOut( char *, DWORD );
#else
#undef ENABLE_DEBUG_LOGGING
#endif // DBG

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_INIT() LogInit()
#else
#define WS2LOG_INIT() 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_F1( a ) LogIt( a )
#else
#define WS2LOG_F1( a )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_F2( a, b ) LogIt( a, b )
#else
#define WS2LOG_F2( a, b )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_F3( a, b, c ) LogIt( a, b, c )
#else
#define WS2LOG_F3( a, b, c )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_F4( a, b, c, d ) LogIt( a, b, c, d )
#else
#define WS2LOG_F4( a, b, c, d ) 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_F5( a, b, c, d, e ) LogIt( a, b, c, d, e )
#else
#define WS2LOG_F5( a, b, c, d, e ) 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define WS2LOG_F6( a, b, c, d, e, f ) LogIt( a, b, c, d, e, f )
#else
#define WS2LOG_F6( a, b, c, d, e, f )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define LOG_IN( a ) LogIn( a )
#else
#define LOG_IN( a )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define LOG_OUT( a, b ) LogOut( a, b )
#else
#define LOG_OUT( a, b )
#endif                 

#endif
