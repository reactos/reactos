#ifndef __WINE_DEBUG_H
#define __WINE_DEBUG_H

#include "../roscfg.h"
#include <stdarg.h>
#include <wchar.h>

#ifndef __GNUC__
#define	__FUNCTION__ ""
#define	inline __inline
#endif

unsigned long DbgPrint(char *Format,...);

#if DBG
#define DPRINT1 DbgPrint("(%s:%d:%s) ",__FILE__,__LINE__,__FUNCTION__), DbgPrint
#else
#define DPRINT1(args...)
#endif

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
extern const char *wine_dbgstr_wn( const wchar_t *s, int n );
extern const char *wine_dbgstr_guid( const struct _GUID *id );
extern const char *wine_dbgstr_longlong( unsigned long long ll );
extern const char *wine_dbg_sprintf( const char *format, ... );

inline static const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
inline static const char *debugstr_wn( const wchar_t *s, int n ) { return wine_dbgstr_wn( s, n ); }
inline static const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }
inline static const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugstr_w( const wchar_t *s ) { return wine_dbgstr_wn( s, 80 ); }
inline static const char *debugres_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugres_w( const wchar_t *s ) { return wine_dbgstr_wn( s, 80 ); }

#define TRACE        DPRINT
#define TRACE_(ch)   DPRINT
#ifdef NDEBUG
#define TRACE_ON(ch) 0
#else
#define TRACE_ON(ch) 1
#endif

#define WINE_TRACE        DPRINT
#define WINE_TRACE_(ch)   DPRINT
#ifdef NDEBUG
#define WINE_TRACE_ON(ch) 0
#else
#define WINE_TRACE_ON(ch) 1
#endif

#define WARN         DPRINT
#define WARN_(ch)    DPRINT
#ifdef NDEBUG
#define WARN_ON(ch)  0
#else
#define WARN_ON(ch)  1
#endif

#ifdef FIXME
#undef FIXME
#endif
#define FIXME        DPRINT1
#define FIXME_(ch)   DPRINT1
#ifdef NDEBUG
#define FIXME_ON(ch) 0
#else
#define FIXME_ON(ch) 1
#endif

#ifdef WINE_FIXME
#undef WINE_FIXME
#endif
#define WINE_FIXME        DPRINT1
#define WINE_FIXME_(ch)   DPRINT1
#ifdef NDEBUG
#define WINE_FIXME_ON(ch) 0
#else
#define WINE_FIXME_ON(ch) 1
#endif

#define ERR          DPRINT1
#define ERR_(ch)     DPRINT1
#ifdef NDEBUG
#define ERR_ON(ch)   0
#else
#define ERR_ON(ch)   1
#endif

#define WINE_ERR          DPRINT1
#define WINE_ERR_(ch)     DPRINT1
#ifdef NDEBUG
#define WINE_ERR_ON(ch)   0
#else
#define WINE_ERR_ON(ch)   1
#endif

#define DECLARE_DEBUG_CHANNEL(ch)
#define DEFAULT_DEBUG_CHANNEL(ch)

#define WINE_DECLARE_DEBUG_CHANNEL(ch) DECLARE_DEBUG_CHANNEL(ch)
#define WINE_DEFAULT_DEBUG_CHANNEL(ch) DEFAULT_DEBUG_CHANNEL(ch)

#define DPRINTF DPRINT
#define MESSAGE DPRINT

#endif  /* __WINE_DEBUG_H */
