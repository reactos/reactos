#ifndef __WINE_DEBUG_H
#define __WINE_DEBUG_H

#include "../roscfg.h"
#include <stdarg.h>
#include <wchar.h>

ULONG DbgPrint(PCH Format,...);

#define DPRINT1 DbgPrint("(%s:%d:%s) ",__FILE__,__LINE__,__FUNCTION__), DbgPrint

#if !defined(DBG) || !defined(YDEBUG)
#ifdef __GNUC__
#define DPRINT(args...)
#else
#define DPRINT
#endif
#else
#define DPRINT DbgPrint("(%s:%d:%s) ",__FILE__,__LINE__,__FUNCTION__), DbgPrint
#endif

#define UNIMPLEMENTED   DbgPrint("WARNING:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__);


struct _GUID;

/* Exported definitions and macros */

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern const char *wine_dbgstr_an( const char * s, int n );
extern const char *wine_dbgstr_wn( const WCHAR *s, int n );
extern const char *wine_dbgstr_guid( const struct _GUID *id );

inline static const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
inline static const char *debugstr_wn( const WCHAR *s, int n ) { return wine_dbgstr_wn( s, n ); }
inline static const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }
inline static const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, 80 ); }
inline static const char *debugres_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugres_w( const WCHAR *s ) { return wine_dbgstr_wn( s, 80 ); }

#define TRACE        DPRINT
#define TRACE_(ch)   DPRINT
#ifdef NDEBUG
#define TRACE_ON(ch) 0
#else
#define TRACE_ON(ch) 1
#endif

#define WARN         DPRINT
#define WARN_(ch)    DPRINT
#ifdef NDEBUG
#define WARN_ON(ch)  0
#else
#define WARN_ON(ch)  1
#endif

#define FIXME        DPRINT1
#define FIXME_(ch)   DPRINT1
#ifdef NDEBUG
#define FIXME_ON(ch) 0
#else
#define FIXME_ON(ch) 1
#endif

#define ERR          DPRINT
#define ERR_(ch)     DPRINT
#ifdef NDEBUG
#define ERR_ON(ch)   0
#else
#define ERR_ON(ch)   1
#endif

#define DECLARE_DEBUG_CHANNEL(ch)
#define DEFAULT_DEBUG_CHANNEL(ch)

#define WINE_DECLARE_DEBUG_CHANNEL(ch) DECLARE_DEBUG_CHANNEL(ch)
#define WINE_DEFAULT_DEBUG_CHANNEL(ch) DEFAULT_DEBUG_CHANNEL(ch)

#define DPRINTF DPRINT
#define MESSAGE DPRINT

#endif  /* __WINE_DEBUG_H */
