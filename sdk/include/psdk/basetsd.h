#ifndef _BASETSD_H_
#define _BASETSD_H_
#pragma once

#ifdef __GNUC__
#include <msvctarget.h>
#endif

#if (defined(_LP64) || defined(__LP64__)) && !defined(_M_AMD64)
#ifndef __ROS_LONG64__
#define __ROS_LONG64__
#endif
#endif

#ifdef __GNUC__
#ifndef __int64
#define __int64 long long
#endif
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#error Old MSVC compiler version.
#endif

#ifdef _MAC
#error Not supported.
#endif

#if defined(_MSC_VER) && !defined(MIDL_PASS) && !defined(RC_INVOKED)
 #define POINTER_64 __ptr64
 #if defined(_WIN64)
  #define POINTER_32 __ptr32
 #else
  #define POINTER_32
 #endif
#else
 #define POINTER_64
 #define POINTER_32
#endif /* defined(_MSC_VER) && !defined(MIDL_PASS) && !defined(RC_INVOKED) */

#if defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_IA64)
 typedef unsigned __int64 POINTER_64_INT;
#else
 typedef unsigned long POINTER_64_INT;
#endif

#if defined(_IA64_) || defined(_AMD64_)
 #define FIRMWARE_PTR
#else
 #define FIRMWARE_PTR POINTER_32
#endif

#if 0 /* Not supported yet */
#define POINTER_SIGNED __sptr
#define POINTER_UNSIGNED __uptr
#else
#define POINTER_SIGNED
#define POINTER_UNSIGNED
#endif

#define SPOINTER_32 POINTER_SIGNED POINTER_32
#define UPOINTER_32 POINTER_UNSIGNED POINTER_32

#if defined(_WIN64)
#define __int3264   __int64
typedef __int64 SHANDLE_PTR;
typedef unsigned __int64 HANDLE_PTR;
typedef unsigned int UHALF_PTR, *PUHALF_PTR;
typedef int HALF_PTR, *PHALF_PTR;
#define ADDRESS_TAG_BIT 0x40000000000UI64
#define Handle32ToHandle( h ) ((HANDLE)(LONG_PTR)(LONG)(h))
#else /*  !_WIN64 */
#define __int3264   __int32
#define ADDRESS_TAG_BIT 0x80000000UL
typedef unsigned short UHALF_PTR, *PUHALF_PTR;
typedef short HALF_PTR, *PHALF_PTR;
typedef long SHANDLE_PTR;
typedef unsigned long HANDLE_PTR;
#define HandleToUlong( h ) ((ULONG)(ULONG_PTR)(h) )
#define HandleToLong( h ) ((LONG)(LONG_PTR) (h) )
#define ULongToHandle( h) ((HANDLE)(ULONG_PTR) (h))
#define LongToHandle( h) ((HANDLE)(LONG_PTR) (h))
#define PtrToUlong( p ) ((ULONG)(ULONG_PTR) (p) )
#define PtrToLong( p ) ((LONG)(LONG_PTR) (p) )
#define PtrToUint( p ) ((UINT)(UINT_PTR) (p) )
#define PtrToInt( p ) ((INT)(INT_PTR) (p) )
#define PtrToUshort( p ) ((unsigned short)(ULONG_PTR)(p) )
#define PtrToShort( p ) ((short)(LONG_PTR)(p) )
#define IntToPtr( i )    ((VOID*)(INT_PTR)((int)i))
#define UIntToPtr( ui )  ((VOID*)(UINT_PTR)((unsigned int)ui))
#define LongToPtr( l )   ((VOID*)(LONG_PTR)((long)l))
#define ULongToPtr( ul )  ((VOID*)(ULONG_PTR)((unsigned long)ul))
#endif /* !_WIN64 */

#define HandleToULong(h) HandleToUlong(h)

#define UlongToHandle(ul) ULongToHandle(ul)
#define UlongToPtr(ul) ULongToPtr(ul)
#define UintToPtr(ui) UIntToPtr(ui)
#define MAXUINT_PTR  (~((UINT_PTR)0))
#define MAXINT_PTR   ((INT_PTR)(MAXUINT_PTR >> 1))
#define MININT_PTR   (~MAXINT_PTR)
#define MAXULONG_PTR (~((ULONG_PTR)0))
#define MAXLONG_PTR  ((LONG_PTR)(MAXULONG_PTR >> 1))
#define MINLONG_PTR  (~MAXLONG_PTR)
#define MAXUHALF_PTR ((UHALF_PTR)~0)
#define MAXHALF_PTR  ((HALF_PTR)(MAXUHALF_PTR >> 1))
#define MINHALF_PTR  (~MAXHALF_PTR)

#if _WIN32_WINNT >= 0x0600

#define MAXUINT      ((UINT)~((UINT)0))
#define MAXULONGLONG ((ULONGLONG)~((ULONGLONG)0))

#endif

#ifndef RC_INVOKED
#ifdef __cplusplus
extern "C" {
#endif
typedef int LONG32, *PLONG32;
typedef int INT32, *PINT32;
typedef unsigned int ULONG32, *PULONG32;
typedef unsigned int DWORD32, *PDWORD32;
typedef unsigned int UINT32, *PUINT32;

#if defined(_WIN64)
typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;
typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#if !defined(__midl) && !defined(__WIDL__)
static inline unsigned long HandleToUlong(const void* h )
    { return((unsigned long)(ULONG_PTR) h ); }
static inline long HandleToLong( const void* h )
    { return((long)(LONG_PTR) h ); }
static inline void* ULongToHandle( const long h )
    { return((void*) (UINT_PTR) h ); }
static inline void* LongToHandle( const long h )
    { return((void*) (INT_PTR) h ); }
static inline unsigned long PtrToUlong( const void* p)
    { return((unsigned long)(ULONG_PTR) p ); }
static inline unsigned int PtrToUint( const void* p )
    { return((unsigned int)(UINT_PTR) p ); }
static inline unsigned short PtrToUshort( const void* p )
    { return((unsigned short)(ULONG_PTR) p ); }
static inline long PtrToLong( const void* p )
    { return((long)(LONG_PTR) p ); }
static inline int PtrToInt( const void* p )
    { return((int)(INT_PTR) p ); }
static inline short PtrToShort( const void* p )
    { return((short)(INT_PTR) p ); }
static inline void* IntToPtr( const int i )
    { return( (void*)(INT_PTR)i ); }
static inline void* UIntToPtr(const unsigned int ui)
    { return( (void*)(UINT_PTR)ui ); }
static inline void* LongToPtr( const long l )
    { return( (void*)(LONG_PTR)l ); }
static inline void* ULongToPtr( const unsigned long ul )
    { return( (void*)(ULONG_PTR)ul ); }
#endif /* !__midl */
#else /*  !_WIN64 */
#ifndef __ROS_LONG64__
typedef int INT_PTR, *PINT_PTR;
typedef unsigned int UINT_PTR, *PUINT_PTR;
#else
typedef long INT_PTR, *PINT_PTR;
typedef unsigned long UINT_PTR, *PUINT_PTR;
#endif

#ifndef LONG_PTR_DEFINED
#define LONG_PTR_DEFINED
	typedef long LONG_PTR, *PLONG_PTR;
	typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif

#endif /* !_WIN64 */

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef __int64 LONG64, *PLONG64;
typedef __int64 INT64,  *PINT64;
typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;
typedef unsigned __int64 UINT64,  *PUINT64;

typedef signed char INT8, *PINT8;
typedef unsigned char UINT8, *PUINT8;
typedef signed short INT16, *PINT16;
typedef unsigned short UINT16, *PUINT16;

typedef ULONG_PTR KAFFINITY;
typedef KAFFINITY *PKAFFINITY;

#ifdef __cplusplus
}
#endif
#endif /* !RC_INVOKED */

#endif /* _BASETSD_H_ */
