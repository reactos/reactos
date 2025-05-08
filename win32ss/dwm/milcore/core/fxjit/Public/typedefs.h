// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/****************************************************************************
*                                                                           *
* windef.h -- Basic Windows Type Definitions                                *
*                                                                           *

*                                                                           *
****************************************************************************/
#define GetComputerNameExW XcpGetComputerNameExW

#ifdef __MACINTOSH__
    // The following hack is to make the macros think they are in an old version
    //  of the MS C compiler when all the latest and greatest features aren't
    //  available.
    #undef _MSC_VER
    #define _MSC_VER 900
    #define _WCHAR_T_DEFINED
    #ifndef _MAC
        #define _MAC
    #endif // _MAC

    #define PASCAL
#endif

#ifndef _WINDEF_
#define _WINDEF_

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

// For use by limits.h
#ifndef _INTEGRAL_MAX_BITS
#define _INTEGRAL_MAX_BITS 64
#endif 

#define STRSAFE_MAX_CCH  2147483647 // max # of characters we support (same as INT_MAX)

// Win32 defines _WIN32 automatically,
// but Macintosh doesn't, so if we are using
// Win32 Functions, we must do it here

#ifdef _MAC
#ifndef _WIN32
#define _WIN32
#endif
#endif //_MAC

#ifndef WIN32
#define WIN32
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif /* WINVER */

#ifndef CONST
#define CONST               const
#endif

#ifndef NO_ERROR
#define NO_ERROR 0L
#endif

#ifndef ERROR_NOT_ENOUGH_MEMORY
#define ERROR_NOT_ENOUGH_MEMORY          8L    // dderror
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/*
 * BASETYPES is defined in ntdef.h if these types are already defined
 */

#ifndef BASETYPES
#define BASETYPES

typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef short CSHORT;
typedef CSHORT *PCSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char CHAR;
typedef CHAR *PCHAR, *LPCH, *PCH;
typedef wchar_t WCHAR;
typedef WCHAR *PWCHAR, *LPWCH, *PWCH;
typedef __nullterminated WCHAR *NWPSTR, *LPWSTR, *PWSTR;
typedef __nullterminated CONST WCHAR *LPCWSTR, *PCWSTR;
typedef char *PSZ;
#endif  /* !BASETYPES */

#ifndef MAXLONG
#define MAXLONG 0x7fffffffL
#endif


#define MAX_PATH          260

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#undef far
#undef near
#undef pascal

#define far
#define near
#if (!defined(_MAC)) && ((_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED))
#define pascal __stdcall
#else
#define pascal
#endif

#ifdef _MAC
    #ifndef CALLBACK
        #define CALLBACK    PASCAL
        #define WINAPI      CDECL
        #define WINAPIV     CDECL
        #define APIENTRY    WINAPI
        #define APIPRIVATE  CDECL
        #ifdef _68K_
        #define PASCAL      __pascal
        #else
        #define PASCAL
        #endif
    #endif // CALLBACK
#elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINAPIV     __cdecl
#define APIENTRY    WINAPI
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#else
// assume none of these have been defined if CALLBACK isn't
#define CALLBACK
#define WINAPI
#define WINAPIV
#define APIENTRY    WINAPI
#define APIPRIVATE
#define PASCAL      pascal
#endif

#ifdef __MACINTOSH__
     #define __forceinline
     #define __declspec(a)
#endif

#ifdef _M_CEE_PURE
#define WINAPI_INLINE  __clrcall
#else
#define WINAPI_INLINE  WINAPI
#endif

#undef FAR
#undef  NEAR
#define FAR                 far
#define NEAR                near

#ifndef DWORD_DEFINED
typedef unsigned long        DWORD;
#endif

typedef int                 BOOL;
typedef unsigned char       BOOLEAN;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL near           *PBOOL;
typedef BOOL far            *LPBOOL;
typedef BYTE near           *PBYTE;
typedef BYTE far            *LPBYTE;
typedef int near            *PINT;
typedef int far             *LPINT;
typedef WORD near           *PWORD;
typedef WORD far            *LPWORD;
typedef long far            *LPLONG;
typedef DWORD near          *PDWORD;
typedef DWORD far           *LPDWORD;
typedef void far            *LPVOID;
typedef CONST void far      *LPCVOID;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;
typedef long                LONG;

typedef short               SHORT;
typedef short               INT16;
typedef unsigned int        UINT32;
typedef signed int          INT32;

typedef void                VOID;

typedef void*               HANDLE;
typedef void**              PHANDLE;

typedef long long           LONGLONG;
typedef unsigned long long  ULONGLONG;

typedef long long           INT64;
typedef unsigned long long  UINT64;

#ifndef __QWORD_DEFINED
#define __QWORD_DEFINED
typedef ULONGLONG QWORD;
#endif  // __QWORD_DEFINED

typedef union _LARGE_INTEGER {
#if __BIG_ENDIAN__      // PowerPC
  struct {
    LONG HighPart;
    DWORD LowPart;
  };
  struct {
    LONG HighPart;
    DWORD LowPart;
  } u;
#elif __LITTLE_ENDIAN__ // Intel
  struct {
    DWORD LowPart;
    LONG HighPart;
  };
  struct {
    DWORD LowPart;
    LONG HighPart;
  } u;
#endif
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */


typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, *PRECT, NEAR *NPRECT, FAR *LPRECT;

typedef const RECT FAR* LPCRECT;

typedef struct _RECTL       /* rcl */
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECTL, *PRECTL, *LPRECTL;

typedef const RECTL FAR* LPCRECTL;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, NEAR *NPPOINT, FAR *LPPOINT;

typedef struct _POINTL      /* ptl  */
{
    LONG  x;
    LONG  y;
} POINTL, *PPOINTL;

typedef struct tagSIZE
{
    LONG        cx;
    LONG        cy;
} SIZE, *PSIZE, *LPSIZE;

typedef SIZE               SIZEL;
typedef SIZE               *PSIZEL, *LPSIZEL;

typedef struct tagPOINTS
{
#ifndef _MAC
    SHORT   x;
    SHORT   y;
#else
    SHORT   y;
    SHORT   x;
#endif
} POINTS, *PPOINTS, *LPPOINTS;

//
//  File System time stamps are represented with the following structure:
//

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#define _FILETIME_


/* mode selections for the device mode function */
#define DM_UPDATE           1
#define DM_COPY             2
#define DM_PROMPT           4
#define DM_MODIFY           8

#define DM_IN_BUFFER        DM_MODIFY
#define DM_IN_PROMPT        DM_PROMPT
#define DM_OUT_BUFFER       DM_COPY
#define DM_OUT_DEFAULT      DM_UPDATE

/* device capabilities indices */
#define DC_FIELDS           1
#define DC_PAPERS           2
#define DC_PAPERSIZE        3
#define DC_MINEXTENT        4
#define DC_MAXEXTENT        5
#define DC_BINS             6
#define DC_DUPLEX           7
#define DC_SIZE             8
#define DC_EXTRA            9
#define DC_VERSION          10
#define DC_DRIVER           11
#define DC_BINNAMES         12
#define DC_ENUMRESOLUTIONS  13
#define DC_FILEDEPENDENCIES 14
#define DC_TRUETYPE         15
#define DC_PAPERNAMES       16
#define DC_ORIENTATION      17
#define DC_COPIES           18

//
// BEGIN Thunder definitions.  Placeholder for stuff needed for core\media tree.
//
#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

typedef char CHAR;

typedef CHAR *LPSTR;

typedef CHAR *PSTR;

typedef const CHAR *LPCSTR;

#ifndef _WCHAR_DEFINED
#define _WCHAR_DEFINED
typedef wchar_t WCHAR;

typedef WCHAR TCHAR;

#endif // !_WCHAR_DEFINED

typedef WCHAR *LPWSTR;

typedef TCHAR *LPTSTR;

typedef const WCHAR *LPCWSTR;

typedef const TCHAR *LPCTSTR;

typedef double DOUBLE;

typedef char INT8;

typedef unsigned char       UINT8;

#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif

#ifdef _WIN32

// Win32 doesn't support __export

#ifndef STDMETHODCALLTYPE
    #define STDMETHODCALLTYPE       __stdcall
#endif

#define STDMETHODVCALLTYPE      __cdecl

#define STDAPICALLTYPE          __stdcall
#define STDAPIVCALLTYPE         __cdecl

#else

#define STDMETHODCALLTYPE       __export __stdcall
#define STDMETHODVCALLTYPE      __export __cdecl

#define STDAPICALLTYPE          __export __stdcall
#define STDAPIVCALLTYPE         __export __cdecl

#endif

#define STDAPI                  EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE

#define STDMETHODIMP            HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)     type STDMETHODCALLTYPE

#ifndef DECLSPEC_NOTHROW
#if (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#define DECLSPEC_NOTHROW   __declspec(nothrow)
#else
#define DECLSPEC_NOTHROW
#endif
#endif

#ifdef COM_STDMETHOD_CAN_THROW
#define COM_DECLSPEC_NOTHROW
#else
#define COM_DECLSPEC_NOTHROW DECLSPEC_NOTHROW
#endif

#define STDMETHOD(method)        virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method)  virtual COM_DECLSPEC_NOTHROW type STDMETHODCALLTYPE method

typedef void *HMODULE;

#if MF_JOLT
#define _INC_WINDOWS
typedef HANDLE *LPHANDLE;
#ifndef _MAC
#ifdef _WIN64
typedef INT_PTR (FAR WINAPI *FARPROC)();
typedef INT_PTR (NEAR WINAPI *NEARPROC)();
typedef INT_PTR (WINAPI *PROC)();
#else
typedef int (FAR WINAPI *FARPROC)();
typedef int (NEAR WINAPI *NEARPROC)();
typedef int (WINAPI *PROC)();
#endif  // _WIN64
#else
typedef int (CALLBACK *FARPROC)();
typedef int (CALLBACK *NEARPROC)();
typedef int (CALLBACK *PROC)();
#endif
#endif

//
// BUGBUG: Copied From stdarg.h.  This will probably all have to go away for XPlat build.
//
#ifndef __MACINTOSH__
#define va_start _crt_va_start
#define va_arg _crt_va_arg
#define va_end _crt_va_end
#endif

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

typedef unsigned char byte;
typedef unsigned long       ULONG;

#define EXTERN_GUID(itf,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8) EXTERN_C const IID itf

#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x) struct
#endif

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && (_MSC_VER >= 1300)
#define _W64 __w64
#else
#define _W64
#endif
#endif

#if !defined(_UINT_PTR_DEFINED)
    #if defined(_WIN64)
        typedef unsigned __int64 UINT_PTR;
    #else
        typedef _W64 unsigned int UINT_PTR;
    #endif

    #define _UINT_PTR_DEFINED
#endif

#if defined(_WIN64)
    typedef __int64 INT_PTR, *PINT_PTR;
    typedef unsigned __int64 *PUINT_PTR;

    typedef __int64 LONG_PTR, *PLONG_PTR;

    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

    #define __int3264   __int64

    __inline
    void *
    UlongToPtr(
        const unsigned long ul
        )
    // Caution: ULongToPtr() zero-extends the unsigned long value.
    {
        return( (void *)(ULONG_PTR)ul );
    }

    __inline
    unsigned long
    PtrToUlong(
        const void  *p
        )
    {
        return((unsigned long) (ULONG_PTR) p );
    }
#else
    #ifndef INT_PTR_DEFINED
    typedef _W64 int INT_PTR, *PINT_PTR;
    #endif // INT_PTR_DEFINED
    typedef _W64 unsigned int *PUINT_PTR;

    typedef _W64 long LONG_PTR, *PLONG_PTR;
    typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

    #define __int3264   __int32

    #define PtrToUlong( p ) ((ULONG)(ULONG_PTR) (p) )
    #define UlongToPtr( ul ) ((VOID *)(ULONG_PTR)((unsigned long)ul))
#endif

#ifdef __MACINTOSH__
    #define _UINTPTR_T_DEFINED
    #define __int64  long long
#endif // __MACINTOSH__

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

typedef LONG *PLONG;
typedef void *PVOID;

typedef XUINT16 UINT16;

typedef unsigned __int64    DWORDLONG;

#define ZeroMemory(pb,cb)           memset((pb),0,(cb))
#define FillMemory(pb,cb,b)         memset((pb),(b),(cb))
#define CopyMemory(pbDst,pbSrc,cb)  do                              \
                                    {                               \
                                        size_t _cb = (size_t)(cb);  \
                                        if (_cb)                    \
                                            memcpy((void*)(pbDst),(void*)(pbSrc),_cb);\
                                    } while (FALSE)
#define MoveMemory(pbDst,pbSrc,cb)  memmove((pbDst),(pbSrc),(cb))

//
//  File structures
//

#define RESTRICTED_POINTER

//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words.
//

typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

//
//  Singly linked list structure. Can be used as either a list head, or
//  as link words.
//

typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

//===========================================================================

#ifndef GUID_DEFINED
#define GUID_DEFINED
#if defined(__midl)
typedef struct {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    byte           Data4[ 8 ];
} GUID;
#else
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif
#endif

#define STATIC_GUID_NULL \
    0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

EXTERN_C const GUID GUID_NULL;
#define DEFINE_GUIDEX(name) EXTERN_C const CDECL XGUID name
//DEFINE_GUIDEX(GUID_NULL);

#define DEFINE_GUIDSTRUCT(g, n) DEFINE_GUIDEX(n)
#define DEFINE_GUIDNAMED(n) n


// Network error from error.h
#define ERROR_FILE_NOT_FOUND        2
#define ERROR_INVALID_PARAMETER     87L

/* */
/* New OEM network-related errors are 50-79 */
/* */
#define ERROR_NOT_SUPPORTED     50

//

//
// MessageId: ERROR_SHARING_VIOLATION
//
// MessageText:
//
// The process cannot access the file because it is being used by another process.
//
#define ERROR_SHARING_VIOLATION          32L

//
// MessageId: ERROR_NETNAME_DELETED
//
// MessageText:
//
// The specified network name is no longer available.
//
#define ERROR_NETNAME_DELETED            64L

//
// MessageId: ERROR_SEM_TIMEOUT
//
// MessageText:
//
// The semaphore timeout period has expired.
//
#define ERROR_SEM_TIMEOUT                121L

//
// MessageId: ERROR_INSUFFICIENT_BUFFER
//
// MessageText:
//
// The data area passed to a system call is too small.
//
#define ERROR_INSUFFICIENT_BUFFER        122L    // dderror

//
// MessageId: ERROR_MORE_DATA
//
// MessageText:
//
//  More data is available.
//
#define ERROR_MORE_DATA                  234L    // dderror

//
// MessageId: ERROR_ARITHMETIC_OVERFLOW
//
// MessageText:
//
// Arithmetic result exceeded 32 bits.
//
#define ERROR_ARITHMETIC_OVERFLOW        534L

// MessageId: ERROR_OPERATION_ABORTED
//
// MessageText:
//
// The I/O operation has been aborted because of either a thread exit or an application request.
//
#define ERROR_OPERATION_ABORTED          995L

//
// MessageId: ERROR_IO_PENDING
//
// MessageText:
//
//  Overlapped I/O operation is in progress.
//
#define ERROR_IO_PENDING                 997L    // dderror

//
// MessageId: ERROR_CANCELLED
//
// MessageText:
//
// The operation was canceled by the user.
//
#define ERROR_CANCELLED                  1223L

//
// MessageId: ERROR_CONNECTION_REFUSED
//
// MessageText:
//
// The remote computer refused the network connection.
//
#define ERROR_CONNECTION_REFUSED         1225L

//
// MessageId: ERROR_NETWORK_UNREACHABLE
//
// MessageText:
//
// The network location cannot be reached. For information about network troubleshooting, see Windows Help.
//
#define ERROR_NETWORK_UNREACHABLE        1231L

//
// MessageId: ERROR_HOST_UNREACHABLE
//
// MessageText:
//
// The network location cannot be reached. For information about network troubleshooting, see Windows Help.
//
#define ERROR_HOST_UNREACHABLE           1232L

//
// MessageId: ERROR_PROTOCOL_UNREACHABLE
//
// MessageText:
//
// The network location cannot be reached. For information about network troubleshooting, see Windows Help.
//
#define ERROR_PROTOCOL_UNREACHABLE       1233L

//
// MessageId: ERROR_CONNECTION_ABORTED
//
// MessageText:
//
// The network connection was aborted by the local system.
//
#define ERROR_CONNECTION_ABORTED         1236L

//
// MessageId: ERROR_CONNECTION_INVALID
//
// MessageText:
//
// An operation was attempted on a nonexistent network connection.
//
#define ERROR_CONNECTION_INVALID         1229L

//
// MessageId: ERROR_DISK_QUOTA_EXCEEDED
//
// MessageText:
//
// The requested file operation failed because the storage quota was exceeded.
// To free up disk space, move files to a different location or delete unnecessary files. For more information, contact your system administrator.
//
#define ERROR_DISK_QUOTA_EXCEEDED        1295L

//
// MessageId: ERROR_TIMEOUT
//
// MessageText:
//
// This operation returned because the timeout period expired.
//
#define ERROR_TIMEOUT                    1460L

#define OPEN_EXISTING                               3
#define GENERIC_READ                     (0x80000000L)

//
// From winbase.h - probably not necessary, since we don't use security atts
// when creating  any events in media layer.
//
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        };

        PVOID Pointer;
    };

    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;


__inline BOOL IsRectEmpty(_In_ LPCRECT pRect)
{
    return(((pRect->right-pRect->left<=0)||(pRect->bottom-pRect->top<=0)) ? TRUE : FALSE);
}

#if MF_JOLT
#if DBG
#define DEBUGBREAK  gps->DebugBreak()
#else
#define DEBUGBREAK
#endif
#else
#define DEBUGBREAK  DebugBreak()
#endif

#define __missing 0
#ifdef __MACINTOSH__
   // #include "xstrutil.h"

    #define __noop

    #define          GetLastError() __missing
    #define          SetLastError(...) __missing

    #ifndef assert
        // only define if we didn't include the CFFoundation version
        #define         assert(e)
    #endif // assert

#else // ! __MACINTOSH__
    #define GetLastError() __missing
    #define SetLastError(...) __missing

#endif // ! __MACINTOSH__

#define DWordMult(ui1,ui2,pui3) UInt32Mul(ui1, ui2, (XUINT32*)pui3)
#define DWordAdd(ui1, ui2, pui3) UInt32Add(ui1, ui2, (XUINT32*)pui3)
#define ULongAdd(a, b, pResult) UInt32Add(a, b, (XUINT32*)(pResult))
#define ULongSub(a, b, pResult) UInt32Sub(a, b, (XUINT32*)(pResult))
#define ULongMult(a, b, pResult) UInt32Mul(a, b, (XUINT32*)(pResult))
#define ULongToUShort UInt32ToUInt16
#define UShortAdd UInt16Add
#define DWord64Add  UInt64Add
#define ULongLongAdd UInt64Add
#define ULongLongSub UInt64Sub

#ifdef  _WIN64
#define SizeTMult UInt64Mul
#define SizeTAdd  UInt64Add
#else
#define SizeTMult UInt32Mul
#define SizeTAdd  UInt32Add
#endif // _WIN64


#define INFINITE            0xFFFFFFFF  // Infinite timeout
#define WAIT_FAILED ((DWORD)0xFFFFFFFF)

/* Define offsetof macro */

#ifdef  _WIN64
typedef __int64             ptrdiff_t;
#define offsetof(s,m)   (size_t)( (ptrdiff_t)&(((s *)0)->m) )
#else
    #ifdef __MACINTOSH__
        #ifndef offsetof
            // don't define if already defined by CoreFoundation.h
            #define offsetof(s, m)    __builtin_offsetof(s, m)
        #endif
    #else // __MACINTOSH__
        #define offsetof(s,m)   (size_t)&(((s *)0)->m)
    #endif
#endif

#define HRESULT XRESULT

//#define size_t XUINT32

//
// Return the code
//

#define HRESULT_CODE(hr)    ((hr) & 0xFFFF)
#define SCODE_CODE(sc)      ((sc) & 0xFFFF)

//
//  Return the facility
//

#define HRESULT_FACILITY(hr)  (((hr) >> 16) & 0x1fff)
#define SCODE_FACILITY(sc)    (((sc) >> 16) & 0x1fff)

//
//  Return the severity
//

#define HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)
#define SCODE_SEVERITY(sc)    (((sc) >> 31) & 0x1)

// __HRESULT_FROM_WIN32 will always be a macro.
// The goal will be to enable INLINE_HRESULT_FROM_WIN32 all the time,
// but there's too much code to change to do that at this time.

#define __HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))

#define FACILITY_WIN32                   7
__inline HRESULT HRESULT_FROM_WIN32(unsigned long x) { return (HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);}

#ifndef     UNALIGNED
#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

#define STRSAFEAPI      __inline HRESULT __stdcall

#define CP_ACP      0       /* default to ANSI code page */
#define CP_OEMCP    1       /* default to OEM  code page */
#define CP_UTF8     65001   /* UTF-8 translation */

//
// These typedefs are used in places where the string is guaranteed to
// be null terminated.
//
typedef __nullterminated char* STRSAFE_LPSTR;
typedef __nullterminated const char* STRSAFE_LPCSTR;
typedef __nullterminated wchar_t* STRSAFE_LPWSTR;
typedef __nullterminated const wchar_t* STRSAFE_LPCWSTR;
typedef __nullterminated const wchar_t UNALIGNED* STRSAFE_LPCUWSTR;

// MAKEFOURCC definition

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
            ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
            ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif // MAKEFOURCC

typedef enum _D3DFORMAT
{
    D3DFMT_UNKNOWN              =  0,

    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_A8B8G8R8             = 32,
    D3DFMT_X8B8G8R8             = 33,
    D3DFMT_G16R16               = 34,
    D3DFMT_A2R10G10B10          = 35,
    D3DFMT_A16B16G16R16         = 36,

    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,

    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,

    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_A2W10V10U10          = 67,

    D3DFMT_UYVY                 = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DFMT_R8G8_B8G8            = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DFMT_YUY2                 = MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DFMT_G8R8_G8B8            = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DFMT_DXT1                 = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 = MAKEFOURCC('D', 'X', 'T', '5'),

    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,

    D3DFMT_D32F_LOCKABLE        = 82,
    D3DFMT_D24FS8               = 83,

    /* Z-Stencil formats valid for CPU access */
    D3DFMT_D32_LOCKABLE         = 84,
    D3DFMT_S8_LOCKABLE          = 85,



    D3DFMT_L16                  = 81,

    D3DFMT_VERTEXDATA           =100,
    D3DFMT_INDEX16              =101,
    D3DFMT_INDEX32              =102,

    D3DFMT_Q16W16V16U16         =110,

    D3DFMT_MULTI2_ARGB8         = MAKEFOURCC('M','E','T','1'),

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,

    // IEEE s23e8 formats (32-bits per channel)
    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,

    D3DFMT_CxV8U8               = 117,

    // Monochrome 1 bit per pixel format
    D3DFMT_A1                   = 118,


    // Binary format indicating that the data has no inherent type
    D3DFMT_BINARYBUFFER            = 199,


    D3DFMT_FORCE_DWORD          =0x7fffffff
} D3DFORMAT;
#define AMCONTROL_COLORINFO_PRESENT 0x00000080 // if set, indicates DXVA color info is present in the upper (24) bits of the dwControlFlags

/* Pool types */
typedef enum _D3DPOOL {
    D3DPOOL_DEFAULT                 = 0,
    D3DPOOL_MANAGED                 = 1,
    D3DPOOL_SYSTEMMEM               = 2,
    D3DPOOL_SCRATCH                 = 3,

    D3DPOOL_FORCE_DWORD             = 0x7fffffff
} D3DPOOL;

#define PF_MMX_INSTRUCTIONS_AVAILABLE       3   // winnt
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6   // winnt
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10   // winnt

#define CFloat16 WORD

/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

//
// BUGBUG: CoCreateInstance calls in xcptopobuilder will go away completely when
// we have codecs integrated into Thunder. Remove this enumeration at that
// time!
//
typedef
enum tagCLSCTX
    {   CLSCTX_INPROC_SERVER    = 0x1,
    CLSCTX_INPROC_HANDLER   = 0x2,
    CLSCTX_LOCAL_SERVER = 0x4,
    CLSCTX_INPROC_SERVER16  = 0x8,
    CLSCTX_REMOTE_SERVER    = 0x10,
    CLSCTX_INPROC_HANDLER16 = 0x20,
    CLSCTX_RESERVED1    = 0x40,
    CLSCTX_RESERVED2    = 0x80,
    CLSCTX_RESERVED3    = 0x100,
    CLSCTX_RESERVED4    = 0x200,
    CLSCTX_NO_CODE_DOWNLOAD = 0x400,
    CLSCTX_RESERVED5    = 0x800,
    CLSCTX_NO_CUSTOM_MARSHAL    = 0x1000,
    CLSCTX_ENABLE_CODE_DOWNLOAD = 0x2000,
    CLSCTX_NO_FAILURE_LOG   = 0x4000,
    CLSCTX_DISABLE_AAA  = 0x8000,
    CLSCTX_ENABLE_AAA   = 0x10000,
    CLSCTX_FROM_DEFAULT_CONTEXT = 0x20000,
    CLSCTX_ACTIVATE_32_BIT_SERVER   = 0x40000,
    CLSCTX_ACTIVATE_64_BIT_SERVER   = 0x80000,
    CLSCTX_ENABLE_CLOAKING  = 0x100000,
    CLSCTX_PS_DLL   = 0x80000000
    }   CLSCTX;

#ifdef __MACINTOSH__
    #define CLSCTX_ALL  0xFFFFFFFF
#endif

#define REGDB_E_CLASSNOTREG              _HRESULT_TYPEDEF_(0x80040154L)

#if !defined(_KERNEL32_)
#define WINBASEAPI DECLSPEC_IMPORT
#else
#define WINBASEAPI
#endif

typedef __success(return >= 0) LONG NTSTATUS;

//
// MessageId: STATUS_WAIT_0
//
// MessageText:
//
//  STATUS_WAIT_0
//
#define STATUS_WAIT_0                    ((NTSTATUS)0x00000000L)    // winnt

//
// MessageId: STATUS_STACK_OVERFLOW
//
// MessageText:
//
// A new guard page for the stack cannot be created.
//
#define STATUS_STACK_OVERFLOW            ((NTSTATUS)0xC00000FDL)    // winnt

#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )

// COM initialization flags; passed to CoInitialize.
typedef enum tagCOINIT
{
  COINIT_APARTMENTTHREADED  = 0x2,      // Apartment model

#if  (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
  // These constants are only valid on Windows NT 4.0
  COINIT_MULTITHREADED      = 0x0,      // OLE calls objects on any thread.
  COINIT_DISABLE_OLE1DDE    = 0x4,      // Don't use DDE for Ole1 support.
  COINIT_SPEED_OVER_MEMORY  = 0x8,      // Trade memory for speed.
#endif // DCOM
} COINIT;

typedef void *HKEY;

#ifndef _MAC
#define MAX_COMPUTERNAME_LENGTH 15
#else
#define MAX_COMPUTERNAME_LENGTH 31
#endif

#define FILE_ATTRIBUTE_NORMAL               0x00000080  // winnt
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100

#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_WRITE_THROUGH         0x80000000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5


#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))

#define GENERIC_WRITE                    (0x40000000L)

#define FILE_SHARE_READ                 0x00000001  // winnt
#define FILE_SHARE_WRITE                0x00000002
#define FILE_SHARE_DELETE               0x00000004

#define CREATE_ALWAYS       2

typedef struct tagVS_FIXEDFILEINFO
{
    DWORD   dwSignature;            /* e.g. 0xfeef04bd */
    DWORD   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
    DWORD   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
    DWORD   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
    DWORD   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
    DWORD   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
    DWORD   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
    DWORD   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    DWORD   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
    DWORD   dwFileType;             /* e.g. VFT_DRIVER */
    DWORD   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
    DWORD   dwFileDateMS;           /* e.g. 0 */
    DWORD   dwFileDateLS;           /* e.g. 0 */
} VS_FIXEDFILEINFO;


typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;

_Success_(return != FALSE)
BOOL
WINAPI
GetComputerNameExA (
    _In_    COMPUTER_NAME_FORMAT NameType,
    _Out_writes_to_opt_(*nSize, *nSize + 1) LPSTR lpBuffer,
    _Inout_ LPDWORD nSize
    );

__success(return!=0)
BOOL
WINAPI
GetComputerNameExW (
    __in    COMPUTER_NAME_FORMAT NameType,
    __out_ecount_part(*nSize, (*nSize + 1)) LPWSTR lpBuffer,
    __inout LPDWORD nSize
    );
#ifdef UNICODE
#define GetComputerNameEx  GetComputerNameExW
#else
#define GetComputerNameEx  GetComputerNameExA
#endif // !UNICODE

#define NTAPI WINAPI

#define WT_EXECUTEDEFAULT       0x00000000
#define WT_EXECUTEINIOTHREAD    0x00000001
#define WT_EXECUTEINUITHREAD    0x00000002
#define WT_EXECUTEINWAITTHREAD  0x00000004
#define WT_EXECUTEONLYONCE      0x00000008
#define WT_EXECUTEINTIMERTHREAD 0x00000020
#define WT_EXECUTELONGFUNCTION  0x00000010
#define WT_EXECUTEINPERSISTENTIOTHREAD  0x00000040
#define WT_EXECUTEINPERSISTENTTHREAD 0x00000080
#define WT_TRANSFER_IMPERSONATION 0x00000100
#define WT_SET_MAX_THREADPOOL_THREADS(Flags, Limit)  ((Flags) |= (Limit)<<16)
typedef VOID (NTAPI * WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK ;

#define GetTickCount() (gps->GetCPUTime() / 1000000)
#if 0
DWORD
WINAPI
GetTickCount(
    VOID
    );
#endif //0

BOOL
WINAPI
RegisterWaitForSingleObject(
    _Outptr_ PHANDLE phNewWaitObject,
    _In_        HANDLE hObject,
    _In_        WAITORTIMERCALLBACK Callback,
    _In_opt_    PVOID Context,
    _In_        ULONG dwMilliseconds,
    _In_        ULONG dwFlags
    );

_Must_inspect_result_
BOOL
WINAPI
UnregisterWait(_In_ HANDLE hObject);

BOOL
WINAPI
QueryPerformanceCounter(
    _Out_ LARGE_INTEGER *lpPerformanceCount
    );

//typedef XGUID IID;
//typedef XGUID REFIID;
//typedef XGUID REFGUID;
//typedef XGUID GUID;
//typedef XGUID REFCLSID;
//typedef XGUID CLSID;

#define NOERROR                 0
#define ERROR_ALREADY_EXISTS    183

//
// END Thunder definitions.
//
typedef double DATE;
typedef WCHAR OLECHAR;
typedef OLECHAR *BSTR;

typedef const OLECHAR *LPCOLESTR;

// Definitions for XMLLite
#define __assume(p)
#define AssertSz(x,y)
#define SIZE_T size_t
#define HINSTANCE void *
#define EXCEPTION_POINTERS int
#define DISP_E_OVERFLOW 0x8002000AL
#define MB_PRECOMPOSED 0x00000001L

// IUnknown Interface
#ifndef __INC_XCPUNKNWN__
#define __INC_XCPUNKNWN__
EXTERN_C const IID IID_IUnknown;

    struct IUnknown
    {
    public:
        virtual __checkReturn HRESULT STDMETHODCALLTYPE QueryInterface(
            __in REFIID riid,
            __deref_out void **ppvObject) = 0;

        virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;

        virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
    };

typedef IUnknown *LPUNKNOWN;
#endif __INC_XCPUNKNWN__

// END Definitions for XMLLite

#ifdef __cplusplus
}
#endif

#endif /* _WINDEF_ */


