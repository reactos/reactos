/*
 * Wine debugging interface
 *
 * Copyright 1999 Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DEBUG_H
#define __WINE_DEBUG_H

#include <stdarg.h>
#include <windef.h>
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#include <builddir.h>

#ifdef __WINE_WINE_TEST_H
#error This file should not be used in Wine tests
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _GUID;

/*
 * Internal definitions (do not use these directly)
 */

enum __wine_debug_class
{
    __WINE_DBCL_FIXME = 0, /* 0x1 */
    __WINE_DBCL_ERR   = 1, /* 0x2 */
    __WINE_DBCL_WARN  = 2, /* 0x4 */
    __WINE_DBCL_TRACE = 3, /* 0x8 */

    /* lazy init flag */
    __WINE_DBCL_INIT = 0x7
};

struct __wine_debug_channel
{
    unsigned char flags;
    char name[15];
};

#define UNIMPLEMENTED WINE_FIXME("%s is UNIMPLEMENTED!\n", __FUNCTION__)

#ifndef WINE_NO_TRACE_MSGS
# define __WINE_GET_DEBUGGING_TRACE(dbch) ((dbch)->flags & (1 << __WINE_DBCL_TRACE))
#else
# define __WINE_GET_DEBUGGING_TRACE(dbch) 0
#endif

#ifndef WINE_NO_DEBUG_MSGS
# define __WINE_GET_DEBUGGING_WARN(dbch)  ((dbch)->flags & (1 << __WINE_DBCL_WARN))
# define __WINE_GET_DEBUGGING_FIXME(dbch) ((dbch)->flags & (1 << __WINE_DBCL_FIXME))
#else
# define __WINE_GET_DEBUGGING_WARN(dbch)  0
# define __WINE_GET_DEBUGGING_FIXME(dbch) 0
#endif

/* define error macro regardless of what is configured */
#define __WINE_GET_DEBUGGING_ERR(dbch)  ((dbch)->flags & (1 << __WINE_DBCL_ERR))

#define __WINE_GET_DEBUGGING(dbcl,dbch)  __WINE_GET_DEBUGGING##dbcl(dbch)

#define __WINE_IS_DEBUG_ON(dbcl,dbch) \
  (__WINE_GET_DEBUGGING##dbcl(dbch) && (__wine_dbg_get_channel_flags(dbch) & (1 << __WINE_DBCL##dbcl)))

#ifdef __GNUC__

#define __WINE_DPRINTF(dbcl,dbch) \
  do { if(__WINE_GET_DEBUGGING(dbcl,(dbch))) { \
       struct __wine_debug_channel * const __dbch = (dbch); \
       const enum __wine_debug_class __dbcl = __WINE_DBCL##dbcl; \
       __WINE_DBG_LOG

#define __WINE_DBG_LOG(args...) \
    ros_dbg_log( __dbcl, __dbch, __RELFILE__, __FUNCTION__, __LINE__, args); } } while(0)

#define __WINE_PRINTF_ATTR(fmt,args) /*__attribute__((format (printf,fmt,args)))*/


#ifdef WINE_NO_TRACE_MSGS
#define WINE_TRACE(args...) do { } while(0)
#define WINE_TRACE_(ch) WINE_TRACE
#endif

#ifdef WINE_NO_DEBUG_MSGS
#define WINE_WARN(args...) do { } while(0)
#define WINE_WARN_(ch) WINE_WARN
#define WINE_FIXME(args...) do { } while(0)
#define WINE_FIXME_(ch) WINE_FIXME
#endif

#elif defined(__SUNPRO_C)

#define __WINE_DPRINTF(dbcl,dbch) \
  do { if(__WINE_GET_DEBUGGING(dbcl,(dbch))) { \
       struct __wine_debug_channel * const __dbch = (dbch); \
       const enum __WINE_DEBUG_CLASS __dbcl = __WINE_DBCL##dbcl; \
       __WINE_DBG_LOG

#define __WINE_DBG_LOG(...) \
   wine_dbg_log( __dbcl, __dbch, __func__, __VA_ARGS__); } } while(0)

#define __WINE_PRINTF_ATTR(fmt,args)

#ifdef WINE_NO_TRACE_MSGS
#define WINE_TRACE(...) do { } while(0)
#define WINE_TRACE_(ch) WINE_TRACE
#endif

#ifdef WINE_NO_DEBUG_MSGS
#define WINE_WARN(...) do { } while(0)
#define WINE_WARN_(ch) WINE_WARN
#define WINE_FIXME(...) do { } while(0)
#define WINE_FIXME_(ch) WINE_FIXME
#endif

#else  /* !__GNUC__ && !__SUNPRO_C */

#define __WINE_DPRINTF(dbcl,dbch) \
    (!__WINE_GET_DEBUGGING(dbcl,(dbch)) || \
     (ros_dbg_log(__WINE_DBCL##dbcl,(dbch),__FILE__,"",__LINE__,"") == -1)) ? \
     (void)0 : (void)wine_dbg_printf

#define __WINE_PRINTF_ATTR(fmt, args)

#endif  /* !__GNUC__ && !__SUNPRO_C */

struct __wine_debug_functions
{
    char * (*get_temp_buffer)( size_t n );
    void   (*release_temp_buffer)( char *buffer, size_t n );
    const char * (*dbgstr_an)( const char * s, int n );
    const char * (*dbgstr_wn)( const WCHAR *s, int n );
    int (*dbg_vprintf)( const char *format, va_list args );
    int (*dbg_vlog)( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                     const char *file, const char *function, const int line, const char *format, va_list args );
};

extern unsigned char __wine_dbg_get_channel_flags( struct __wine_debug_channel *channel );
extern int __wine_dbg_set_channel_flags( struct __wine_debug_channel *channel,
                                         unsigned char set, unsigned char clear );
extern void __wine_dbg_set_functions( const struct __wine_debug_functions *new_funcs,
                                      struct __wine_debug_functions *old_funcs, size_t size );

/*
 * Exported definitions and macros
 */

/* These functions return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern const char *wine_dbgstr_an( const char * s, int n );
extern const char *wine_dbgstr_wn( const WCHAR *s, int n );
extern const char *wine_dbg_sprintf( const char *format, ... ) __WINE_PRINTF_ATTR(1,2);

extern int wine_dbg_printf( const char *format, ... ) __WINE_PRINTF_ATTR(1,2);
extern int wine_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *ch, const char *func,
                         const char *format, ... ) __WINE_PRINTF_ATTR(4,5);
/* ReactOS compliant debug format */
extern int ros_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *ch, const char *file,
                         const char *func, const int line, const char *format, ... ) __WINE_PRINTF_ATTR(6,7);

static __inline const char *wine_dbgstr_a( const char *s )
{
    return wine_dbgstr_an( s, -1 );
}

static __inline const char *wine_dbgstr_w( const WCHAR *s )
{
    return wine_dbgstr_wn( s, -1 );
}

static __inline const char *wine_dbgstr_guid( const GUID *id )
{
    if (!id) return "(null)";
    if (!((ULONG_PTR)id >> 16)) return wine_dbg_sprintf( "<guid-0x%04lx>", (ULONG_PTR)id & 0xffff );
    return wine_dbg_sprintf( "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                             id->Data1, id->Data2, id->Data3,
                             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
}

static __inline const char *wine_dbgstr_point( const POINT *pt )
{
    if (!pt) return "(null)";
    return wine_dbg_sprintf( "(%ld,%ld)", pt->x, pt->y );
}

static __inline const char *wine_dbgstr_size( const SIZE *size )
{
    if (!size) return "(null)";
    return wine_dbg_sprintf( "(%ld,%ld)", size->cx, size->cy );
}

static __inline const char *wine_dbgstr_rect( const RECT *rect )
{
    if (!rect) return "(null)";
    return wine_dbg_sprintf( "(%ld,%ld)-(%ld,%ld)", rect->left, rect->top,
                             rect->right, rect->bottom );
}

static __inline const char *wine_dbgstr_longlong( ULONGLONG ll )
{
    if (/*sizeof(ll) > sizeof(unsigned long) &&*/ ll >> 32) /* ULONGLONG is always > long in ReactOS */
        return wine_dbg_sprintf( "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll );
    else return wine_dbg_sprintf( "%lx", (unsigned long)ll );
}

#if defined(__oaidl_h__) && defined(V_VT)

static inline const char *wine_dbgstr_vt( VARTYPE vt )
{
    static const char *const variant_types[] =
    {
        "VT_EMPTY","VT_NULL","VT_I2","VT_I4","VT_R4","VT_R8","VT_CY","VT_DATE",
        "VT_BSTR","VT_DISPATCH","VT_ERROR","VT_BOOL","VT_VARIANT","VT_UNKNOWN",
        "VT_DECIMAL","15","VT_I1","VT_UI1","VT_UI2","VT_UI4","VT_I8","VT_UI8",
        "VT_INT","VT_UINT","VT_VOID","VT_HRESULT","VT_PTR","VT_SAFEARRAY",
        "VT_CARRAY","VT_USERDEFINED","VT_LPSTR","VT_LPWSTR","32","33","34","35",
        "VT_RECORD","VT_INT_PTR","VT_UINT_PTR","39","40","41","42","43","44","45",
        "46","47","48","49","50","51","52","53","54","55","56","57","58","59","60",
        "61","62","63","VT_FILETIME","VT_BLOB","VT_STREAM","VT_STORAGE",
        "VT_STREAMED_OBJECT","VT_STORED_OBJECT","VT_BLOB_OBJECT","VT_CF","VT_CLSID",
        "VT_VERSIONED_STREAM"
    };

    static const char *const variant_flags[16] =
    {
        "",
        "|VT_VECTOR",
        "|VT_ARRAY",
        "|VT_VECTOR|VT_ARRAY",
        "|VT_BYREF",
        "|VT_VECTOR|VT_BYREF",
        "|VT_ARRAY|VT_BYREF",
        "|VT_VECTOR|VT_ARRAY|VT_BYREF",
        "|VT_RESERVED",
        "|VT_VECTOR|VT_RESERVED",
        "|VT_ARRAY|VT_RESERVED",
        "|VT_VECTOR|VT_ARRAY|VT_RESERVED",
        "|VT_BYREF|VT_RESERVED",
        "|VT_VECTOR|VT_BYREF|VT_RESERVED",
        "|VT_ARRAY|VT_BYREF|VT_RESERVED",
        "|VT_VECTOR|VT_ARRAY|VT_BYREF|VT_RESERVED",
    };

    if (vt & ~VT_TYPEMASK)
        return wine_dbg_sprintf( "%s%s", wine_dbgstr_vt(vt&VT_TYPEMASK), variant_flags[vt>>12] );

    if (vt < sizeof(variant_types)/sizeof(*variant_types))
        return variant_types[vt];

    if (vt == VT_BSTR_BLOB)
        return "VT_BSTR_BLOB";

    return wine_dbg_sprintf( "vt(invalid %x)", vt );
}

static inline const char *wine_dbgstr_variant( const VARIANT *v )
{
    if (!v)
        return "(null)";

    if (V_VT(v) & VT_BYREF) {
        if (V_VT(v) == (VT_VARIANT|VT_BYREF))
            return wine_dbg_sprintf( "%p {VT_VARIANT|VT_BYREF: %s}", v, wine_dbgstr_variant(V_VARIANTREF(v)) );
        if (V_VT(v) == (VT_BSTR|VT_BYREF))
            return wine_dbg_sprintf( "%p {VT_BSTR|VT_BYREF: %s}", v, V_BSTRREF(v) ? wine_dbgstr_w(*V_BSTRREF(v)) : "(none)" );
        return wine_dbg_sprintf( "%p {%s %p}", v, wine_dbgstr_vt(V_VT(v)), V_BYREF(v) );
    }

    if (V_ISARRAY(v) || V_ISVECTOR(v))
        return wine_dbg_sprintf( "%p {%s %p}", v, wine_dbgstr_vt(V_VT(v)), V_ARRAY(v) );

    switch(V_VT(v)) {
    case VT_EMPTY:
        return wine_dbg_sprintf( "%p {VT_EMPTY}", v );
    case VT_NULL:
        return wine_dbg_sprintf( "%p {VT_NULL}", v );
    case VT_I2:
        return wine_dbg_sprintf( "%p {VT_I2: %d}", v, V_I2(v) );
    case VT_I4:
        return wine_dbg_sprintf( "%p {VT_I4: %d}", v, V_I4(v) );
    case VT_R4:
        return wine_dbg_sprintf( "%p {VT_R4: %f}", v, V_R4(v) );
    case VT_R8:
        return wine_dbg_sprintf( "%p {VT_R8: %lf}", v, V_R8(v) );
    case VT_CY:
        return wine_dbg_sprintf( "%p {VT_CY: %s}", v, wine_dbgstr_longlong(V_CY(v).int64) );
    case VT_DATE:
        return wine_dbg_sprintf( "%p {VT_DATE: %lf}", v, V_DATE(v) );
    case VT_BSTR:
        return wine_dbg_sprintf( "%p {VT_BSTR: %s}", v, wine_dbgstr_w(V_BSTR(v)) );
    case VT_DISPATCH:
        return wine_dbg_sprintf( "%p {VT_DISPATCH: %p}", v, V_DISPATCH(v) );
    case VT_ERROR:
        return wine_dbg_sprintf( "%p {VT_ERROR: %08x}", v, V_ERROR(v) );
    case VT_BOOL:
        return wine_dbg_sprintf( "%p {VT_BOOL: %x}", v, V_BOOL(v) );
    case VT_UNKNOWN:
        return wine_dbg_sprintf( "%p {VT_UNKNOWN: %p}", v, V_UNKNOWN(v) );
    case VT_I1:
        return wine_dbg_sprintf( "%p {VT_I1: %d}", v, V_I1(v) );
    case VT_UI1:
        return wine_dbg_sprintf( "%p {VT_UI1: %u}", v, V_UI1(v) );
    case VT_UI2:
        return wine_dbg_sprintf( "%p {VT_UI2: %d}", v, V_UI2(v) );
    case VT_UI4:
        return wine_dbg_sprintf( "%p {VT_UI4: %d}", v, V_UI4(v) );
    case VT_I8:
        return wine_dbg_sprintf( "%p {VT_I8: %s}", v, wine_dbgstr_longlong(V_I8(v)) );
    case VT_UI8:
        return wine_dbg_sprintf( "%p {VT_UI8: %s}", v, wine_dbgstr_longlong(V_UI8(v)) );
    case VT_INT:
        return wine_dbg_sprintf( "%p {VT_INT: %d}", v, V_INT(v) );
    case VT_UINT:
        return wine_dbg_sprintf( "%p {VT_UINT: %u}", v, V_UINT(v) );
    case VT_VOID:
        return wine_dbg_sprintf( "%p {VT_VOID}", v );
    case VT_RECORD:
        return wine_dbg_sprintf( "%p {VT_RECORD: %p %p}", v, V_RECORD(v), V_RECORDINFO(v) );
    default:
        return wine_dbg_sprintf( "%p {vt %s}", v, wine_dbgstr_vt(V_VT(v)) );
    }
}

#endif /* defined(__oaidl_h__) && defined(V_VT) */

#ifndef WINE_TRACE
#define WINE_TRACE                 __WINE_DPRINTF(_TRACE,__wine_dbch___default)
#define WINE_TRACE_(ch)            __WINE_DPRINTF(_TRACE,&__wine_dbch_##ch)
#endif
#define WINE_TRACE_ON(ch)          __WINE_IS_DEBUG_ON(_TRACE,&__wine_dbch_##ch)

#ifndef WINE_WARN
#define WINE_WARN                  __WINE_DPRINTF(_WARN,__wine_dbch___default)
#define WINE_WARN_(ch)             __WINE_DPRINTF(_WARN,&__wine_dbch_##ch)
#endif
#define WINE_WARN_ON(ch)           __WINE_IS_DEBUG_ON(_WARN,&__wine_dbch_##ch)

#ifndef WINE_FIXME
#define WINE_FIXME                 __WINE_DPRINTF(_FIXME,__wine_dbch___default)
#define WINE_FIXME_(ch)            __WINE_DPRINTF(_FIXME,&__wine_dbch_##ch)
#endif
#define WINE_FIXME_ON(ch)          __WINE_IS_DEBUG_ON(_FIXME,&__wine_dbch_##ch)

#define WINE_ERR                   __WINE_DPRINTF(_ERR,__wine_dbch___default)
#define WINE_ERR_(ch)              __WINE_DPRINTF(_ERR,&__wine_dbch_##ch)
#define WINE_ERR_ON(ch)            __WINE_IS_DEBUG_ON(_ERR,&__wine_dbch_##ch)

#define WINE_DECLARE_DEBUG_CHANNEL(ch) \
    static struct __wine_debug_channel __wine_dbch_##ch = { (unsigned char)~0, #ch }
#define WINE_DEFAULT_DEBUG_CHANNEL(ch) \
    static struct __wine_debug_channel __wine_dbch_##ch = { (unsigned char)~0, #ch }; \
    static struct __wine_debug_channel * const __wine_dbch___default = &__wine_dbch_##ch

#define WINE_DPRINTF               wine_dbg_printf
#define WINE_MESSAGE               wine_dbg_printf

/* Wine uses shorter names that are very likely to conflict with other software */

static __inline const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
static __inline const char *debugstr_wn( const WCHAR *s, int n ) { return wine_dbgstr_wn( s, n ); }
static __inline const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }
static __inline const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, -1 ); }
static __inline const char *debugstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, -1 ); }

#if defined(__oaidl_h__) && defined(V_VT)
static __inline const char *debugstr_vt( VARTYPE vt ) { return wine_dbgstr_vt( vt ); }
static __inline const char *debugstr_variant( const VARIANT *v ) { return wine_dbgstr_variant( v ); }
#endif

#define TRACE                      WINE_TRACE
#define TRACE_(ch)                 WINE_TRACE_(ch)
#define TRACE_ON(ch)               WINE_TRACE_ON(ch)

#define WARN                       WINE_WARN
#define WARN_(ch)                  WINE_WARN_(ch)
#define WARN_ON(ch)                WINE_WARN_ON(ch)

#define FIXME                      WINE_FIXME
#define FIXME_(ch)                 WINE_FIXME_(ch)
#define FIXME_ON(ch)               WINE_FIXME_ON(ch)

#undef ERR  /* Solaris got an 'ERR' define in <sys/reg.h> */
#define ERR                        WINE_ERR
#define ERR_(ch)                   WINE_ERR_(ch)
#define ERR_ON(ch)                 WINE_ERR_ON(ch)

#define DPRINTF                    WINE_DPRINTF
#define MESSAGE                    WINE_MESSAGE

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_DEBUG_H */
