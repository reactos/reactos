#ifndef __WINE_DEBUG_H
#define __WINE_DEBUG_H

#include "../roscfg.h"
#include <stdarg.h>
#include <windef.h>
#if !defined(_MSC_VER)
#include <winnt.h>
#endif

/* Add ROS Master debug functions if not added yet */
#ifndef __INTERNAL_DEBUG
#ifdef YDEBUG
#undef NDEBUG
#else
#define NDEBUG
#endif
#include <reactos/debug.h>
#endif

#ifndef __GNUC__
#if !defined(_MSC_VER) || _MSC_VER < 8
#define	__FUNCTION__ ""
#endif//_MSC_VER
#define	inline __inline
#endif

unsigned long DbgPrint(const char *Format,...);

struct _GUID;

/* Exported definitions and macros */

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern const char *wine_dbgstr_w( const WCHAR *s );
extern const char *wine_dbgstr_an( const char * s, int n );
extern const char *wine_dbgstr_wn( const wchar_t *s, int n );
extern const char *wine_dbgstr_longlong( unsigned long long ll );
extern const char *wine_dbg_sprintf( const char *format, ... );

inline static const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
inline static const char *debugstr_wn( const wchar_t *s, int n ) { return wine_dbgstr_wn( s, n ); }
inline static const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugstr_w( const wchar_t *s ) { return wine_dbgstr_wn( s, 80 ); }
inline static const char *debugres_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugres_w( const wchar_t *s ) { return wine_dbgstr_wn( s, 80 ); }

static inline const char *wine_dbgstr_point( const POINT *pt )
{
    if (!pt) return "(null)";
    return wine_dbg_sprintf( "(%ld,%ld)", pt->x, pt->y );
}

static inline const char *wine_dbgstr_size( const SIZE *size )
{
    if (!size) return "(null)";
    return wine_dbg_sprintf( "(%ld,%ld)", size->cx, size->cy );
}

static inline const char *wine_dbgstr_rect( const RECT *rect )
{
    if (!rect) return "(null)";
    return wine_dbg_sprintf( "(%ld,%ld)-(%ld,%ld)", rect->left, rect->top, rect->right, rect->bottom );
}

static inline const char *wine_dbgstr_guid( const GUID *id )
{
    if (!id) return "(null)";
    if (!((INT_PTR)id >> 16)) return wine_dbg_sprintf( "<guid-0x%04x>", (INT_PTR)id & 0xffff );
    return wine_dbg_sprintf( "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                             id->Data1, id->Data2, id->Data3,
                             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
}

static inline const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }

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
