
#ifndef __WINE_DEBUGTOOLS_H
#define __WINE_DEBUGTOOLS_H

#ifndef __NTDLL__

#include <stdarg.h>
#include "config.h"
#include "windef.h"

#else

#include <windows.h>

#endif __NTDLL__

struct _GUID;

/* Internal definitions (do not use these directly) */

enum __DEBUG_CLASS { __DBCL_FIXME, __DBCL_ERR, __DBCL_WARN, __DBCL_TRACE, __DBCL_COUNT };

#ifndef NO_TRACE_MSGS
# define __GET_DEBUGGING_TRACE(dbch) ((dbch)[0] & (1 << __DBCL_TRACE))
#else
# define __GET_DEBUGGING_TRACE(dbch) 0
#endif

#ifndef NO_DEBUG_MSGS
# define __GET_DEBUGGING_WARN(dbch)  ((dbch)[0] & (1 << __DBCL_WARN))
# define __GET_DEBUGGING_FIXME(dbch) ((dbch)[0] & (1 << __DBCL_FIXME))
#else
# define __GET_DEBUGGING_WARN(dbch)  0
# define __GET_DEBUGGING_FIXME(dbch) 0
#endif

/* define error macro regardless of what is configured */
#define __GET_DEBUGGING_ERR(dbch)  ((dbch)[0] & (1 << __DBCL_ERR))

#define __GET_DEBUGGING(dbcl,dbch)  __GET_DEBUGGING##dbcl(dbch)
#define __SET_DEBUGGING(dbcl,dbch,on) \
    ((on) ? ((dbch)[0] |= 1 << (dbcl)) : ((dbch)[0] &= ~(1 << (dbcl))))

#ifdef __GNUC__

#define __DPRINTF(dbcl,dbch) \
  do { if(__GET_DEBUGGING(dbcl,(dbch))) { \
       const char * const __dbch = (dbch); \
       const enum __DEBUG_CLASS __dbcl = __DBCL##dbcl; \
       __WINE_DBG_LOG

#define __WINE_DBG_LOG(args...) \
    wine_dbg_log( __dbcl, __dbch, __FUNCTION__, args); } } while(0)

#define __PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))

#else  /* __GNUC__ */

#define __DPRINTF(dbcl,dbch) \
    (!__GET_DEBUGGING(dbcl,(dbch)) || \
     (wine_dbg_log(__DBCL##dbcl,(dbch),__FILE__,"%d: ",__LINE__),0)) ? \
     (void)0 : (void)wine_dbg_printf

#define __PRINTF_ATTR(fmt, args)

#endif  /* __GNUC__ */

/* Exported definitions and macros */

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern const char *wine_dbgstr_an( const char * s, int n );
extern const char *wine_dbgstr_wn( const WCHAR *s, int n );
extern const char *wine_dbgstr_guid( const struct _GUID *id );

extern int wine_dbg_vprintf( const char *format, va_list args ) __PRINTF_ATTR(1,0);
extern int wine_dbg_printf( const char *format, ... ) __PRINTF_ATTR(1,2);
extern int wine_dbg_log( enum __DEBUG_CLASS cls, const char *ch,
                         const char *func, const char *format, ... ) __PRINTF_ATTR(4,5);

inline static const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
inline static const char *debugstr_wn( const WCHAR *s, int n ) { return wine_dbgstr_wn( s, n ); }
inline static const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }
inline static const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, 80 ); }
inline static const char *debugres_a( const char *s )  { return wine_dbgstr_an( s, 80 ); }
inline static const char *debugres_w( const WCHAR *s ) { return wine_dbgstr_wn( s, 80 ); }

#define TRACE        __DPRINTF(_TRACE,__wine_dbch___default)
#define TRACE_(ch)   __DPRINTF(_TRACE,__wine_dbch_##ch)
#define TRACE_ON(ch) __GET_DEBUGGING(_TRACE,__wine_dbch_##ch)

#define WARN         __DPRINTF(_WARN,__wine_dbch___default)
#define WARN_(ch)    __DPRINTF(_WARN,__wine_dbch_##ch)
#define WARN_ON(ch)  __GET_DEBUGGING(_WARN,__wine_dbch_##ch)

#define FIXME        __DPRINTF(_FIXME,__wine_dbch___default)
#define FIXME_(ch)   __DPRINTF(_FIXME,__wine_dbch_##ch)
#define FIXME_ON(ch) __GET_DEBUGGING(_FIXME,__wine_dbch_##ch)

#undef ERR  /* Solaris got an 'ERR' define in <sys/reg.h> */
#define ERR          __DPRINTF(_ERR,__wine_dbch___default)
#define ERR_(ch)     __DPRINTF(_ERR,__wine_dbch_##ch)
#define ERR_ON(ch)   __GET_DEBUGGING(_ERR,__wine_dbch_##ch)

#define DECLARE_DEBUG_CHANNEL(ch) \
    char __wine_dbch_##ch[1];
#define DEFAULT_DEBUG_CHANNEL(ch) \
    char __wine_dbch_##ch[1]; static char * const __wine_dbch___default = __wine_dbch_##ch

#define DPRINTF wine_dbg_printf
#define MESSAGE wine_dbg_printf

#endif  /* __WINE_DEBUGTOOLS_H */
