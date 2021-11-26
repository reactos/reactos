/* Defines the "size" of an any-size array */
#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 1
#endif

/* Helper macro to enable gcc's extension.  */
#ifndef __GNU_EXTENSION
 #ifdef __GNUC__
  #define __GNU_EXTENSION __extension__
 #else
  #define __GNU_EXTENSION
 #endif
#endif /* __GNU_EXTENSION */

#ifndef DUMMYUNIONNAME
 #if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
  #define _ANONYMOUS_UNION
  #define _UNION_NAME(x) x
  #define DUMMYUNIONNAME  u
  #define DUMMYUNIONNAME1 u1
  #define DUMMYUNIONNAME2 u2
  #define DUMMYUNIONNAME3 u3
  #define DUMMYUNIONNAME4 u4
  #define DUMMYUNIONNAME5 u5
  #define DUMMYUNIONNAME6 u6
  #define DUMMYUNIONNAME7 u7
  #define DUMMYUNIONNAME8 u8
  #define DUMMYUNIONNAME9  u9
 #else
  #define _ANONYMOUS_UNION __GNU_EXTENSION
  #define _UNION_NAME(x)
  #define DUMMYUNIONNAME
  #define DUMMYUNIONNAME1
  #define DUMMYUNIONNAME2
  #define DUMMYUNIONNAME3
  #define DUMMYUNIONNAME4
  #define DUMMYUNIONNAME5
  #define DUMMYUNIONNAME6
  #define DUMMYUNIONNAME7
  #define DUMMYUNIONNAME8
  #define DUMMYUNIONNAME9
 #endif /* NONAMELESSUNION */
#endif /* !DUMMYUNIONNAME */

#ifndef DUMMYSTRUCTNAME
 #if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
  #define _ANONYMOUS_STRUCT
  #define _STRUCT_NAME(x) x
  #define DUMMYSTRUCTNAME s
  #define DUMMYSTRUCTNAME1 s1
  #define DUMMYSTRUCTNAME2 s2
  #define DUMMYSTRUCTNAME3 s3
  #define DUMMYSTRUCTNAME4 s4
  #define DUMMYSTRUCTNAME5 s5
 #else
  #define _ANONYMOUS_STRUCT __GNU_EXTENSION
  #define _STRUCT_NAME(x)
  #define DUMMYSTRUCTNAME
  #define DUMMYSTRUCTNAME1
  #define DUMMYSTRUCTNAME2
  #define DUMMYSTRUCTNAME3
  #define DUMMYSTRUCTNAME4
  #define DUMMYSTRUCTNAME5
 #endif /* NONAMELESSUNION */
#endif /* DUMMYSTRUCTNAME */

#if defined(STRICT_GS_ENABLED)
 #pragma strict_gs_check(push, on)
#endif

#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM)
 #define ALIGNMENT_MACHINE
 #define UNALIGNED __unaligned
 #if defined(_WIN64)
  #define UNALIGNED64 __unaligned
 #else
  #define UNALIGNED64
 #endif
#else
 #undef ALIGNMENT_MACHINE
 #define UNALIGNED
 #define UNALIGNED64
#endif

#if defined(_WIN64) || defined(_M_ALPHA)
 #define MAX_NATURAL_ALIGNMENT sizeof(ULONGLONG)
 #define MEMORY_ALLOCATION_ALIGNMENT 16
#else
 #define MAX_NATURAL_ALIGNMENT sizeof($ULONG)
 #define MEMORY_ALLOCATION_ALIGNMENT 8
#endif

/* C99 restrict support */
#if defined(ENABLE_RESTRICTED) && defined(_M_MRX000) && !defined(MIDL_PASS) && !defined(RC_INVOKED)
 #define RESTRICTED_POINTER __restrict
#else
 #define RESTRICTED_POINTER
#endif

/* Returns the base address of a structure from a structure member */
#ifndef CONTAINING_RECORD
 #define CONTAINING_RECORD(address, type, field) \
   ((type *)(((ULONG_PTR)address) - (ULONG_PTR)(&(((type *)0)->field))))
#endif

/* Returns the byte offset of the specified structure's member */
#if !defined(__GNUC__) && !defined(__clang__)
 #define FIELD_OFFSET(Type, Field) ((LONG)(LONG_PTR)&(((Type*) 0)->Field))
#else
 #define FIELD_OFFSET(Type, Field) ((LONG)__builtin_offsetof(Type, Field))
#endif /* __GNUC__ */

/* Returns the type's alignment */
#if defined(_MSC_VER)
 #define TYPE_ALIGNMENT(t) __alignof(t)
#else
 #define TYPE_ALIGNMENT(t) FIELD_OFFSET(struct { char x; t test; }, test)
#endif /* _MSC_VER */

#if defined(_AMD64_) || defined(_X86_)
 #define PROBE_ALIGNMENT(_s) TYPE_ALIGNMENT($ULONG)
#elif defined(_IA64_) || defined(_ARM_) || defined(_ARM64_)
 #define PROBE_ALIGNMENT(_s) max((TYPE_ALIGNMENT(_s), TYPE_ALIGNMENT($ULONG))
#elif !defined(RC_INVOKED)
 #error "Unknown architecture"
#endif

#if defined(_WIN64)
 #define PROBE_ALIGNMENT32(_s) TYPE_ALIGNMENT($ULONG)
#endif /* _WIN64 */

#ifdef __cplusplus
 #define EXTERN_C extern "C"
#else
 #define EXTERN_C extern
#endif /* __cplusplus */

#define NTAPI __stdcall

#ifndef STDMETHODCALLTYPE
 #define STDMETHODCALLTYPE  __stdcall
 #define STDMETHODVCALLTYPE __cdecl
 #define STDAPICALLTYPE     __stdcall
 #define STDAPIVCALLTYPE    __cdecl
 #define STDAPI             EXTERN_C HRESULT STDAPICALLTYPE
 #define STDAPI_(t)         EXTERN_C t STDAPICALLTYPE
 #define STDMETHODIMP       HRESULT STDMETHODCALLTYPE
 #define STDMETHODIMP_(t)   t STDMETHODCALLTYPE
 #define STDAPIV            EXTERN_C HRESULT STDAPIVCALLTYPE
 #define STDAPIV_(t)        EXTERN_C t STDAPIVCALLTYPE
 #define STDMETHODIMPV      HRESULT STDMETHODVCALLTYPE
 #define STDMETHODIMPV_(t)  t STDMETHODVCALLTYPE
#endif /* !STDMETHODCALLTYPE */

#define STDOVERRIDEMETHODIMP      __override STDMETHODIMP
#define STDOVERRIDEMETHODIMP_(t)  __override STDMETHODIMP_(t)
#define IFACEMETHODIMP            __override STDMETHODIMP
#define IFACEMETHODIMP_(t)        __override STDMETHODIMP_(t)
#define STDOVERRIDEMETHODIMPV     __override STDMETHODIMPV
#define STDOVERRIDEMETHODIMPV_(t) __override STDMETHODIMPV_(t)
#define IFACEMETHODIMPV           __override STDMETHODIMPV
#define IFACEMETHODIMPV_(t)       __override STDMETHODIMPV_(t)

/* Import and Export Specifiers */

#ifndef DECLSPEC_IMPORT
 #define DECLSPEC_IMPORT __declspec(dllimport) // MIDL?
#endif /* DECLSPEC_IMPORT */

#ifndef DECLSPEC_EXPORT
 #if defined(__REACTOS__) || defined(__WINESRC__)
  #define DECLSPEC_EXPORT __declspec(dllexport)
 #endif
#endif /* DECLSPEC_EXPORT */

#define DECLSPEC_NORETURN __declspec(noreturn)

#ifndef DECLSPEC_ADDRSAFE
 #if defined(_MSC_VER) && (defined(_M_ALPHA) || defined(_M_AXP64))
  #define DECLSPEC_ADDRSAFE __declspec(address_safe)
 #else
  #define DECLSPEC_ADDRSAFE
 #endif
#endif /* DECLSPEC_ADDRSAFE */

#ifndef DECLSPEC_NOTHROW
 #if !defined(MIDL_PASS)
  #define DECLSPEC_NOTHROW __declspec(nothrow)
 #else
  #define DECLSPEC_NOTHROW
 #endif
#endif /* DECLSPEC_NOTHROW */

#ifndef NOP_FUNCTION
 #if defined(_MSC_VER)
  #define NOP_FUNCTION __noop
 #else
  #define NOP_FUNCTION (void)0
 #endif
#endif /* NOP_FUNCTION */

#if !defined(_NTSYSTEM_)
 #define NTSYSAPI     DECLSPEC_IMPORT
 #define NTSYSCALLAPI DECLSPEC_IMPORT
#else
 #define NTSYSAPI
 #if defined(_NTDLLBUILD_)
  #define NTSYSCALLAPI
 #else
  #define NTSYSCALLAPI DECLSPEC_ADDRSAFE
 #endif
#endif /* _NTSYSTEM_ */

/* Inlines */
#ifndef FORCEINLINE
 #define FORCEINLINE __forceinline
#endif /* FORCEINLINE */

#ifndef DECLSPEC_NOINLINE
 #if (_MSC_VER >= 1300)
  #define DECLSPEC_NOINLINE  __declspec(noinline)
 #elif defined(__GNUC__)
  #define DECLSPEC_NOINLINE __attribute__((noinline))
 #else
  #define DECLSPEC_NOINLINE
 #endif
#endif /* DECLSPEC_NOINLINE */

#if !defined(_M_CEE_PURE)
 #define NTAPI_INLINE NTAPI
#else
 #define NTAPI_INLINE
#endif /* _M_CEE_PURE */

/* Use to specify structure alignment. Note: VS and GCC behave slightly
   different. Therefore it is important to stick to the following rules:
   - If you want a struct to be aligned, put DECLSPEC_ALIGN after "struct":
     "typedef struct DECLSPEC_ALIGN(16) _FOO { ... } FOO, *PFOO;"
     _alignof(PFOO) is sizeof(void*) here as usual.
   - If you don't want the struct, but only the typedef to be aligned,
     use an extra typedef.
     struct _BAR { ... };
     typedef DECLSPEC_ALIGN(16) struct _BAR BAR, *ALIGNEDPBAR;
     _alignof(ALIGNEDPBAR) is 16 now! */
#ifndef DECLSPEC_ALIGN
 #if defined(_MSC_VER) && !defined(MIDL_PASS)
  #define DECLSPEC_ALIGN(x) __declspec(align(x))
 #elif defined(__GNUC__)
  #define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
 #else
  #define DECLSPEC_ALIGN(x)
 #endif
#endif /* DECLSPEC_ALIGN */

#ifndef SYSTEM_CACHE_ALIGNMENT_SIZE
 #if defined(_AMD64_) || defined(_X86_)
  #define SYSTEM_CACHE_ALIGNMENT_SIZE 64
 #else
  #define SYSTEM_CACHE_ALIGNMENT_SIZE 128
 #endif
#endif /* SYSTEM_CACHE_ALIGNMENT_SIZE */

#ifndef DECLSPEC_CACHEALIGN
 #define DECLSPEC_CACHEALIGN DECLSPEC_ALIGN(SYSTEM_CACHE_ALIGNMENT_SIZE)
#endif /* DECLSPEC_CACHEALIGN */

#ifndef DECLSPEC_UUID
 #if defined(_MSC_VER) && defined(__cplusplus)
  #define DECLSPEC_UUID(x) __declspec(uuid(x))
 #else
  #define DECLSPEC_UUID(x)
 #endif
#endif /* DECLSPEC_UUID */

#ifndef DECLSPEC_NOVTABLE
 #if defined(_MSC_VER) && defined(__cplusplus)
  #define DECLSPEC_NOVTABLE __declspec(novtable)
 #else
  #define DECLSPEC_NOVTABLE
 #endif
#endif /* DECLSPEC_NOVTABLE */

#ifndef DECLSPEC_SELECTANY
 #if defined(_MSC_VER) || defined(__GNUC__)
  #define DECLSPEC_SELECTANY __declspec(selectany)
 #else
  #define DECLSPEC_SELECTANY
 #endif
#endif /* DECLSPEC_SELECTANY */

#ifndef DECLSPEC_DEPRECATED
 #if (defined(_MSC_VER) || defined(__GNUC__)) && !defined(MIDL_PASS)
  #define DECLSPEC_DEPRECATED __declspec(deprecated)
  #define DEPRECATE_SUPPORTED
 #else
  #define DECLSPEC_DEPRECATED
  #undef  DEPRECATE_SUPPORTED
 #endif
#endif /* DECLSPEC_DEPRECATED */

#ifdef DEPRECATE_DDK_FUNCTIONS
 #ifdef _NTDDK_
  #define DECLSPEC_DEPRECATED_DDK DECLSPEC_DEPRECATED
  #ifdef DEPRECATE_SUPPORTED
   #define PRAGMA_DEPRECATED_DDK 1
  #endif
 #else
  #define DECLSPEC_DEPRECATED_DDK
  #define PRAGMA_DEPRECATED_DDK 1
 #endif
#else
 #define DECLSPEC_DEPRECATED_DDK
 #define PRAGMA_DEPRECATED_DDK 0
#endif /* DEPRECATE_DDK_FUNCTIONS */

/* Use to silence unused variable warnings when it is intentional */
#define UNREFERENCED_PARAMETER(P) ((void)(P))
#define DBG_UNREFERENCED_PARAMETER(P) ((void)(P))
#define DBG_UNREFERENCED_LOCAL_VARIABLE(L) ((void)(L))

/* Void Pointers */
typedef void *PVOID;
typedef void * POINTER_64 PVOID64;

/* Handle Type */
typedef void *HANDLE, **PHANDLE;
#ifdef STRICT
 #define DECLARE_HANDLE(n) typedef struct n##__{int unused;} *n
#else
 #define DECLARE_HANDLE(n) typedef HANDLE n
#endif

/* Upper-Case Versions of Some Standard C Types */
#ifndef VOID
 #define VOID void
 typedef char CHAR;
 typedef short SHORT;

 #if defined(__ROS_LONG64__)
  typedef int LONG;
 #else
  typedef long LONG;
 #endif

 #if !defined(MIDL_PASS)
 typedef int INT;
 #endif /* !MIDL_PASS */
#endif /* VOID */

$if(_NTDEF_)
/* Unsigned Types */
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned long ULONG, *PULONG;

typedef double DOUBLE;
$endif(_NTDEF_)

/* Signed Types */
typedef SHORT *PSHORT;
typedef LONG *PLONG;

/* Flag types */
typedef unsigned char FCHAR;
typedef unsigned short FSHORT;
typedef unsigned long FLONG;

typedef unsigned char BOOLEAN, *PBOOLEAN;
$if(_NTDEF_)
typedef ULONG LOGICAL, *PLOGICAL;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS, *PNTSTATUS;
typedef signed char SCHAR, *PSCHAR;
$endif(_NTDEF_)

#ifndef _HRESULT_DEFINED
 #define _HRESULT_DEFINED
 typedef _Return_type_success_(return >= 0) LONG HRESULT;
#endif /* _HRESULT_DEFINED */

/* 64-bit types */
#define _ULONGLONG_
__GNU_EXTENSION typedef __int64 LONGLONG, *PLONGLONG;
__GNU_EXTENSION typedef unsigned __int64 ULONGLONG, *PULONGLONG;
#define _DWORDLONG_
typedef ULONGLONG DWORDLONG, *PDWORDLONG;

/* Update Sequence Number */
typedef LONGLONG USN;

/* ANSI (Multi-byte Character) types */
typedef CHAR *PCHAR, *LPCH, *PCH, *PNZCH;
typedef CONST CHAR *LPCCH, *PCCH, *PCNZCH;
typedef _Null_terminated_ CHAR *NPSTR, *LPSTR, *PSTR;
typedef _Null_terminated_ PSTR *PZPSTR;
typedef _Null_terminated_ CONST PSTR *PCZPSTR;
typedef _Null_terminated_ CONST CHAR *LPCSTR, *PCSTR;
typedef _Null_terminated_ PCSTR *PZPCSTR;

typedef _NullNull_terminated_ CHAR *PZZSTR;
typedef _NullNull_terminated_ CONST CHAR *PCZZSTR;

$if(_NTDEF_)
/* Pointer to an Asciiz string */
typedef _Null_terminated_ CHAR *PSZ;
typedef _Null_terminated_ CONST char *PCSZ;
$endif(_NTDEF_)

/* UNICODE (Wide Character) types */
typedef wchar_t WCHAR;
typedef WCHAR *PWCHAR, *LPWCH, *PWCH;
typedef CONST WCHAR *LPCWCH, *PCWCH;
typedef _Null_terminated_ WCHAR *NWPSTR, *LPWSTR, *PWSTR;
typedef _Null_terminated_ PWSTR *PZPWSTR;
typedef _Null_terminated_ CONST PWSTR *PCZPWSTR;
typedef _Null_terminated_ WCHAR UNALIGNED *LPUWSTR, *PUWSTR;
typedef _Null_terminated_ CONST WCHAR *LPCWSTR, *PCWSTR;
typedef _Null_terminated_ PCWSTR *PZPCWSTR;
typedef _Null_terminated_ CONST WCHAR UNALIGNED *LPCUWSTR, *PCUWSTR;

typedef _NullNull_terminated_ WCHAR *PZZWSTR;
typedef _NullNull_terminated_ CONST WCHAR *PCZZWSTR;
typedef _NullNull_terminated_ WCHAR UNALIGNED *PUZZWSTR;
typedef _NullNull_terminated_ CONST WCHAR UNALIGNED *PCUZZWSTR;

typedef  WCHAR *PNZWCH;
typedef  CONST WCHAR *PCNZWCH;
typedef  WCHAR UNALIGNED *PUNZWCH;
typedef  CONST WCHAR UNALIGNED *PCUNZWCH;

#if (_WIN32_WINNT >= 0x0600) || (defined(__cplusplus) && defined(WINDOWS_ENABLE_CPLUSPLUS))
 typedef CONST WCHAR *LPCWCHAR, *PCWCHAR;
 typedef CONST WCHAR UNALIGNED *LPCUWCHAR, *PCUWCHAR;
 typedef unsigned long UCSCHAR, *PUCSCHAR, *PUCSSTR;
 typedef const UCSCHAR *PCUCSCHAR, *PCUCSSTR;
 typedef UCSCHAR UNALIGNED *PUUCSCHAR, *PUUCSSTR;
 typedef const UCSCHAR UNALIGNED *PCUUCSCHAR, *PCUUCSSTR;
 #define UCSCHAR_INVALID_CHARACTER (0xffffffff)
 #define MIN_UCSCHAR (0)
 #define MAX_UCSCHAR (0x0010FFFF)
#endif /* _WIN32_WINNT >= 0x0600 */

#ifdef  UNICODE

 #ifndef _TCHAR_DEFINED
  typedef WCHAR TCHAR, *PTCHAR;
$if(_NTDEF_)
  typedef WCHAR TUCHAR, *PTUCHAR;
$endif(_NTDEF_)
$if(_WINNT_)
  typedef WCHAR TBYTE, *PTBYTE;
$endif(_WINNT_)
  #define _TCHAR_DEFINED
 #endif /* !_TCHAR_DEFINED */

 typedef LPWCH LPTCH, PTCH;
 typedef LPCWCH LPCTCH, PCTCH;
 typedef LPWSTR PTSTR, LPTSTR;
 typedef LPCWSTR PCTSTR, LPCTSTR;
 typedef LPUWSTR PUTSTR, LPUTSTR;
 typedef LPCUWSTR PCUTSTR, LPCUTSTR;
 typedef LPWSTR LP;
 typedef PZZWSTR PZZTSTR;
 typedef PCZZWSTR PCZZTSTR;
 typedef PUZZWSTR PUZZTSTR;
 typedef PCUZZWSTR PCUZZTSTR;
 typedef PZPWSTR PZPTSTR;
 typedef PNZWCH PNZTCH;
 typedef PCNZWCH PCNZTCH;
 typedef PUNZWCH PUNZTCH;
 typedef PCUNZWCH PCUNZTCH;
 #define __TEXT(quote) L##quote

#else /* UNICODE */

 #ifndef _TCHAR_DEFINED
  typedef char TCHAR, *PTCHAR;
$if(_NTDEF_)
  typedef unsigned char TUCHAR, *PTUCHAR;
$endif(_NTDEF_)
$if(_WINNT_)
  typedef unsigned char TBYTE, *PTBYTE;
$endif(_WINNT_)
  #define _TCHAR_DEFINED
 #endif /* !_TCHAR_DEFINED */
 typedef LPCH LPTCH, PTCH;
 typedef LPCCH LPCTCH, PCTCH;
 typedef LPSTR PTSTR, LPTSTR, PUTSTR, LPUTSTR;
 typedef LPCSTR PCTSTR, LPCTSTR, PCUTSTR, LPCUTSTR;
 typedef PZZSTR PZZTSTR, PUZZTSTR;
 typedef PCZZSTR PCZZTSTR, PCUZZTSTR;
 typedef PZPSTR PZPTSTR;
 typedef PNZCH PNZTCH, PUNZTCH;
 typedef PCNZCH PCNZTCH, PCUNZTCH;
 #define __TEXT(quote) quote

#endif /* UNICODE */

#define TEXT(quote) __TEXT(quote)

/* Cardinal Data Types */
typedef char CCHAR;
$if(_NTDEF_)
typedef CCHAR *PCCHAR;
typedef short CSHORT, *PCSHORT;
typedef ULONG CLONG, *PCLONG;
$endif(_NTDEF_)

/* NLS basics (Locale and Language Ids) */
typedef $ULONG LCID, *PLCID;
typedef $USHORT LANGID;

#ifndef __COMPARTMENT_ID_DEFINED__
#define __COMPARTMENT_ID_DEFINED__
typedef enum
{
    UNSPECIFIED_COMPARTMENT_ID = 0,
    DEFAULT_COMPARTMENT_ID
} COMPARTMENT_ID, *PCOMPARTMENT_ID;
#endif /* __COMPARTMENT_ID_DEFINED__ */

#ifndef __OBJECTID_DEFINED
#define __OBJECTID_DEFINED
typedef struct  _OBJECTID {
    GUID Lineage;
    $ULONG Uniquifier;
} OBJECTID;
#endif /* __OBJECTID_DEFINED */

#ifdef _MSC_VER
 #pragma warning(push)
 #pragma warning(disable:4201) // nameless struct / union
#endif

typedef struct
#if defined(_M_IA64)
DECLSPEC_ALIGN(16)
#endif
_FLOAT128 {
    __int64 LowPart;
    __int64 HighPart;
} FLOAT128;
typedef FLOAT128 *PFLOAT128;

/* Large Integer Unions */
#if defined(MIDL_PASS)
typedef struct _LARGE_INTEGER {
#else
typedef union _LARGE_INTEGER {
    _ANONYMOUS_STRUCT struct
    {
        $ULONG LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;
    struct
    {
        $ULONG LowPart;
        LONG HighPart;
    } u;
#endif /* MIDL_PASS */
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#if defined(MIDL_PASS)
typedef struct _ULARGE_INTEGER {
#else
typedef union _ULARGE_INTEGER {
    _ANONYMOUS_STRUCT struct
    {
        $ULONG LowPart;
        $ULONG HighPart;
    } DUMMYSTRUCTNAME;
    struct
    {
        $ULONG LowPart;
        $ULONG HighPart;
    } u;
#endif /* MIDL_PASS */
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#ifdef _MSC_VER
#pragma warning(pop) /* disable:4201 */
#endif

/* Locally Unique Identifier */
typedef struct _LUID
{
    $ULONG LowPart;
    LONG HighPart;
} LUID, *PLUID;

#define APPLICATION_ERROR_MASK       0x20000000
#define ERROR_SEVERITY_SUCCESS       0x00000000
#define ERROR_SEVERITY_INFORMATIONAL 0x40000000
#define ERROR_SEVERITY_WARNING       0x80000000
#define ERROR_SEVERITY_ERROR         0xC0000000

$if(_NTDEF_)
/* Native API Return Value Macros */
#define NT_SUCCESS(Status)              (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status)          ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status)              ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status)                ((((ULONG)(Status)) >> 30) == 3)
$endif(_NTDEF_)

#define ANSI_NULL ((CHAR)0)
#define UNICODE_NULL ((WCHAR)0)
#define UNICODE_STRING_MAX_BYTES ((USHORT) 65534)
#define UNICODE_STRING_MAX_CHARS (32767)

/* Doubly Linked Lists */
typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

typedef struct LIST_ENTRY32 {
  $ULONG Flink;
  $ULONG Blink;
} LIST_ENTRY32, *PLIST_ENTRY32;

typedef struct LIST_ENTRY64 {
  ULONGLONG Flink;
  ULONGLONG Blink;
} LIST_ENTRY64, *PLIST_ENTRY64;

/* Singly Linked Lists */
typedef struct _SINGLE_LIST_ENTRY {
  struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

$if(_NTDEF_)
typedef struct _SINGLE_LIST_ENTRY32 {
    ULONG Next;
} SINGLE_LIST_ENTRY32, *PSINGLE_LIST_ENTRY32;
$endif(_NTDEF_)

typedef struct _PROCESSOR_NUMBER {
  $USHORT Group;
  $UCHAR Number;
  $UCHAR Reserved;
} PROCESSOR_NUMBER, *PPROCESSOR_NUMBER;

#define ALL_PROCESSOR_GROUPS 0xffff

typedef
_IRQL_requires_same_
_Function_class_(EXCEPTION_ROUTINE)
EXCEPTION_DISPOSITION
NTAPI
EXCEPTION_ROUTINE(
    _Inout_ struct _EXCEPTION_RECORD *ExceptionRecord,
    _In_ PVOID EstablisherFrame,
    _Inout_ struct _CONTEXT *ContextRecord,
    _In_ PVOID DispatcherContext);

typedef EXCEPTION_ROUTINE *PEXCEPTION_ROUTINE;

typedef struct _GROUP_AFFINITY {
  KAFFINITY Mask;
  $USHORT Group;
  $USHORT Reserved[3];
} GROUP_AFFINITY, *PGROUP_AFFINITY;

/* Helper Macros */

#define RTL_FIELD_TYPE(type, field)    (((type*)0)->field)
#define RTL_BITS_OF(sizeOfArg)         (sizeof(sizeOfArg) * 8)
#define RTL_BITS_OF_FIELD(type, field) (RTL_BITS_OF(RTL_FIELD_TYPE(type, field)))
#define RTL_FIELD_SIZE(type, field) (sizeof(((type *)0)->field))

#define RTL_SIZEOF_THROUGH_FIELD(type, field) \
    (FIELD_OFFSET(type, field) + RTL_FIELD_SIZE(type, field))

#define RTL_CONTAINS_FIELD(Struct, Size, Field) \
    ( (((PCHAR)(&(Struct)->Field)) + sizeof((Struct)->Field)) <= (((PCHAR)(Struct))+(Size)) )

#define RTL_NUMBER_OF_V1(A) (sizeof(A)/sizeof((A)[0]))

#ifdef __GNUC__
 #define RTL_NUMBER_OF_V2(A) \
     (({ int _check_array_type[__builtin_types_compatible_p(typeof(A), typeof(&A[0])) ? -1 : 1]; (void)_check_array_type; }), \
     RTL_NUMBER_OF_V1(A))
#elif defined(__cplusplus)
extern "C++" {
 template <typename T, size_t N>
 static char (& SAFE_RTL_NUMBER_OF(T (&)[N]))[N];
}
 #define RTL_NUMBER_OF_V2(A) sizeof(SAFE_RTL_NUMBER_OF(A))
#else
 #define RTL_NUMBER_OF_V2(A) RTL_NUMBER_OF_V1(A)
#endif

#ifdef ENABLE_RTL_NUMBER_OF_V2
 #define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
 #define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif

#define ARRAYSIZE(A)    RTL_NUMBER_OF_V2(A)
#define _ARRAYSIZE(A)   RTL_NUMBER_OF_V1(A)

#define RTL_NUMBER_OF_FIELD(type, field) \
    (RTL_NUMBER_OF(RTL_FIELD_TYPE(type, field)))

#define RTL_PADDING_BETWEEN_FIELDS(type, field1, field2) \
    ((FIELD_OFFSET(type, field2) > FIELD_OFFSET(type, field1)) \
        ? (FIELD_OFFSET(type, field2) - FIELD_OFFSET(type, field1) - RTL_FIELD_SIZE(type, field1)) \
        : (FIELD_OFFSET(type, field1) - FIELD_OFFSET(type, field2) - RTL_FIELD_SIZE(type, field2)))

#if defined(__cplusplus)
 #define RTL_CONST_CAST(type) const_cast<type>
#else
 #define RTL_CONST_CAST(type) (type)
#endif

#ifdef __cplusplus
#define DEFINE_ENUM_FLAG_OPERATORS(_ENUMTYPE) \
extern "C++" { \
  inline _ENUMTYPE operator|(_ENUMTYPE a, _ENUMTYPE b) { return _ENUMTYPE(((int)a) | ((int)b)); } \
  inline _ENUMTYPE &operator|=(_ENUMTYPE &a, _ENUMTYPE b) { return (_ENUMTYPE &)(((int &)a) |= ((int)b)); } \
  inline _ENUMTYPE operator&(_ENUMTYPE a, _ENUMTYPE b) { return _ENUMTYPE(((int)a) & ((int)b)); } \
  inline _ENUMTYPE &operator&=(_ENUMTYPE &a, _ENUMTYPE b) { return (_ENUMTYPE &)(((int &)a) &= ((int)b)); } \
  inline _ENUMTYPE operator~(_ENUMTYPE a) { return _ENUMTYPE(~((int)a)); } \
  inline _ENUMTYPE operator^(_ENUMTYPE a, _ENUMTYPE b) { return _ENUMTYPE(((int)a) ^ ((int)b)); } \
  inline _ENUMTYPE &operator^=(_ENUMTYPE &a, _ENUMTYPE b) { return (_ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}
#else
#define DEFINE_ENUM_FLAG_OPERATORS(_ENUMTYPE)
#endif

#define COMPILETIME_OR_2FLAGS(a,b)          ((UINT)(a)|(UINT)(b))
#define COMPILETIME_OR_3FLAGS(a,b,c)        ((UINT)(a)|(UINT)(b)|(UINT)(c))
#define COMPILETIME_OR_4FLAGS(a,b,c,d)      ((UINT)(a)|(UINT)(b)|(UINT)(c)|(UINT)(d))
#define COMPILETIME_OR_5FLAGS(a,b,c,d,e)    ((UINT)(a)|(UINT)(b)|(UINT)(c)|(UINT)(d)|(UINT)(e))

/* Type Limits */
#define MINCHAR   0x80
#define MAXCHAR   0x7f
#define MINSHORT  0x8000
#define MAXSHORT  0x7fff
#define MINLONG   0x80000000
#define MAXLONG   0x7fffffff
$if(_NTDEF_)
#define MAXUCHAR  0xff
#define MAXUSHORT 0xffff
#define MAXULONG  0xffffffff
$endif(_NTDEF_)
$if(_WINNT_)
#define MAXBYTE   0xff
#define MAXWORD   0xffff
#define MAXDWORD  0xffffffff
$endif(_WINNT_)
#define MAXLONGLONG (0x7fffffffffffffffLL)

/* 32 to 64 bit multiplication. GCC is really bad at optimizing the native math */
#if defined(_M_IX86) && !defined(_M_ARM) && !defined(_M_ARM64) && \
    !defined(MIDL_PASS)&& !defined(RC_INVOKED) && !defined(_M_CEE_PURE)
 #define Int32x32To64(a,b) __emul(a,b)
 #define UInt32x32To64(a,b) __emulu(a,b)
#else
 #define Int32x32To64(a,b) (((__int64)(long)(a))*((__int64)(long)(b)))
 #define UInt32x32To64(a,b) ((unsigned __int64)(unsigned int)(a)*(unsigned __int64)(unsigned int)(b))
#endif

#if defined(MIDL_PASS)|| defined(RC_INVOKED) || defined(_M_CEE_PURE) || defined(_M_ARM)
/* Use native math */
 #define Int64ShllMod32(a,b) ((unsigned __int64)(a)<<(b))
 #define Int64ShraMod32(a,b) (((__int64)(a))>>(b))
 #define Int64ShrlMod32(a,b) (((unsigned __int64)(a))>>(b))
#else
/* Use intrinsics */
 #define Int64ShllMod32(a,b) __ll_lshift(a,b)
 #define Int64ShraMod32(a,b) __ll_rshift(a,b)
 #define Int64ShrlMod32(a,b) __ull_rshift(a,b)
#endif

#define RotateLeft32 _rotl
#define RotateLeft64 _rotl64
#define RotateRight32 _rotr
#define RotateRight64 _rotr64

#if defined(_M_AMD64)
 #define RotateLeft8 _rotl8
 #define RotateLeft16 _rotl16
 #define RotateRight8 _rotr8
 #define RotateRight16 _rotr16
#endif /* _M_AMD64 */

/* C_ASSERT Definition */
#define C_ASSERT(expr) extern char (*c_assert(void)) [(expr) ? 1 : -1]

/* Eliminate Microsoft C/C++ compiler warning 4715 */
#if defined(_MSC_VER)
 #define DEFAULT_UNREACHABLE default: __assume(0)
#elif defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))))
 #define DEFAULT_UNREACHABLE default: __builtin_unreachable()
#else
 #define DEFAULT_UNREACHABLE default: break
#endif

#if defined(__GNUC__) || defined(__clang__)
 #define UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
 #define UNREACHABLE __assume(0)
#else
 #define UNREACHABLE
#endif

#define VER_WORKSTATION_NT                  0x40000000
#define VER_SERVER_NT                       0x80000000
#define VER_SUITE_SMALLBUSINESS             0x00000001
#define VER_SUITE_ENTERPRISE                0x00000002
#define VER_SUITE_BACKOFFICE                0x00000004
#define VER_SUITE_COMMUNICATIONS            0x00000008
#define VER_SUITE_TERMINAL                  0x00000010
#define VER_SUITE_SMALLBUSINESS_RESTRICTED  0x00000020
#define VER_SUITE_EMBEDDEDNT                0x00000040
#define VER_SUITE_DATACENTER                0x00000080
#define VER_SUITE_SINGLEUSERTS              0x00000100
#define VER_SUITE_PERSONAL                  0x00000200
#define VER_SUITE_BLADE                     0x00000400
#define VER_SUITE_EMBEDDED_RESTRICTED       0x00000800
#define VER_SUITE_SECURITY_APPLIANCE        0x00001000
#define VER_SUITE_STORAGE_SERVER            0x00002000
#define VER_SUITE_COMPUTE_SERVER            0x00004000
#define VER_SUITE_WH_SERVER                 0x00008000

/* Product types */
#define PRODUCT_UNDEFINED                           0x00000000
#define PRODUCT_ULTIMATE                            0x00000001
#define PRODUCT_HOME_BASIC                          0x00000002
#define PRODUCT_HOME_PREMIUM                        0x00000003
#define PRODUCT_ENTERPRISE                          0x00000004
#define PRODUCT_HOME_BASIC_N                        0x00000005
#define PRODUCT_BUSINESS                            0x00000006
#define PRODUCT_STANDARD_SERVER                     0x00000007
#define PRODUCT_DATACENTER_SERVER                   0x00000008
#define PRODUCT_SMALLBUSINESS_SERVER                0x00000009
#define PRODUCT_ENTERPRISE_SERVER                   0x0000000A
#define PRODUCT_STARTER                             0x0000000B
#define PRODUCT_DATACENTER_SERVER_CORE              0x0000000C
#define PRODUCT_STANDARD_SERVER_CORE                0x0000000D
#define PRODUCT_ENTERPRISE_SERVER_CORE              0x0000000E
#define PRODUCT_ENTERPRISE_SERVER_IA64              0x0000000F
#define PRODUCT_BUSINESS_N                          0x00000010
#define PRODUCT_WEB_SERVER                          0x00000011
#define PRODUCT_CLUSTER_SERVER                      0x00000012
#define PRODUCT_HOME_SERVER                         0x00000013
#define PRODUCT_STORAGE_EXPRESS_SERVER              0x00000014
#define PRODUCT_STORAGE_STANDARD_SERVER             0x00000015
#define PRODUCT_STORAGE_WORKGROUP_SERVER            0x00000016
#define PRODUCT_STORAGE_ENTERPRISE_SERVER           0x00000017
#define PRODUCT_SERVER_FOR_SMALLBUSINESS            0x00000018
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM        0x00000019
#define PRODUCT_HOME_PREMIUM_N                      0x0000001A
#define PRODUCT_ENTERPRISE_N                        0x0000001B
#define PRODUCT_ULTIMATE_N                          0x0000001C
#define PRODUCT_WEB_SERVER_CORE                     0x0000001D
#define PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT    0x0000001E
#define PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY      0x0000001F
#define PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING     0x00000020
#define PRODUCT_SERVER_FOUNDATION                   0x00000021
#define PRODUCT_HOME_PREMIUM_SERVER                 0x00000022
#define PRODUCT_SERVER_FOR_SMALLBUSINESS_V          0x00000023
#define PRODUCT_STANDARD_SERVER_V                   0x00000024
#define PRODUCT_DATACENTER_SERVER_V                 0x00000025
#define PRODUCT_ENTERPRISE_SERVER_V                 0x00000026
#define PRODUCT_DATACENTER_SERVER_CORE_V            0x00000027
#define PRODUCT_STANDARD_SERVER_CORE_V              0x00000028
#define PRODUCT_ENTERPRISE_SERVER_CORE_V            0x00000029
#define PRODUCT_HYPERV                              0x0000002A
#define PRODUCT_STORAGE_EXPRESS_SERVER_CORE         0x0000002B
#define PRODUCT_STORAGE_STANDARD_SERVER_CORE        0x0000002C
#define PRODUCT_STORAGE_WORKGROUP_SERVER_CORE       0x0000002D
#define PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE      0x0000002E
#define PRODUCT_STARTER_N                           0x0000002F
#define PRODUCT_PROFESSIONAL                        0x00000030
#define PRODUCT_PROFESSIONAL_N                      0x00000031
#define PRODUCT_SB_SOLUTION_SERVER                  0x00000032
#define PRODUCT_SERVER_FOR_SB_SOLUTIONS             0x00000033
#define PRODUCT_STANDARD_SERVER_SOLUTIONS           0x00000034
#define PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE      0x00000035
#define PRODUCT_SB_SOLUTION_SERVER_EM               0x00000036
#define PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM          0x00000037
#define PRODUCT_SOLUTION_EMBEDDEDSERVER             0x00000038
#define PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE        0x00000039
#define PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT       0x0000003B
#define PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL       0x0000003C
#define PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC    0x0000003D
#define PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC    0x0000003E
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE   0x0000003F
#define PRODUCT_CLUSTER_SERVER_V                    0x00000040
#define PRODUCT_EMBEDDED                            0x00000041
#define PRODUCT_STARTER_E                           0x00000042
#define PRODUCT_HOME_BASIC_E                        0x00000043
#define PRODUCT_HOME_PREMIUM_E                      0x00000044
#define PRODUCT_PROFESSIONAL_E                      0x00000045
#define PRODUCT_ENTERPRISE_E                        0x00000046
#define PRODUCT_ULTIMATE_E                          0x00000047
#define PRODUCT_ENTERPRISE_EVALUATION               0x00000048
#define PRODUCT_MULTIPOINT_STANDARD_SERVER          0x0000004C
#define PRODUCT_MULTIPOINT_PREMIUM_SERVER           0x0000004D
#define PRODUCT_STANDARD_EVALUATION_SERVER          0x0000004F
#define PRODUCT_DATACENTER_EVALUATION_SERVER        0x00000050
#define PRODUCT_ENTERPRISE_N_EVALUATION             0x00000054
#define PRODUCT_EMBEDDED_AUTOMOTIVE                 0x00000055
#define PRODUCT_EMBEDDED_INDUSTRY_A                 0x00000056
#define PRODUCT_THINPC                              0x00000057
#define PRODUCT_EMBEDDED_A                          0x00000058
#define PRODUCT_EMBEDDED_INDUSTRY                   0x00000059
#define PRODUCT_EMBEDDED_E                          0x0000005A
#define PRODUCT_EMBEDDED_INDUSTRY_E                 0x0000005B
#define PRODUCT_EMBEDDED_INDUSTRY_A_E               0x0000005C
#define PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER 0x0000005F
#define PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER  0x00000060
#define PRODUCT_CORE_ARM                            0x00000061
#define PRODUCT_CORE_N                              0x00000062
#define PRODUCT_CORE_COUNTRYSPECIFIC                0x00000063
#define PRODUCT_CORE_SINGLELANGUAGE                 0x00000064
#define PRODUCT_CORE                                0x00000065
#define PRODUCT_PROFESSIONAL_WMC                    0x00000067
#define PRODUCT_ENTERPRISE_S_N_EVALUATION           0x00000082
#define PRODUCT_UNLICENSED                          0xABCDABCD

/* LangID and NLS */
#define MAKELANGID(p, s)       ((((USHORT)(s)) << 10) | (USHORT)(p))
#define PRIMARYLANGID(lgid)    ((USHORT)(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((USHORT)(lgid) >> 10)
#define MAKELCID(lgid, srtid)  (($ULONG)(((($ULONG)((USHORT)(srtid))) << 16) |  \
                                          (($ULONG)((USHORT)(lgid)))))
#define MAKESORTLCID(lgid, srtid, ver)                                        \
                               (($ULONG)((MAKELCID(lgid, srtid)) |             \
                                    ((($ULONG)((USHORT)(ver))) << 20)))
#define LANGIDFROMLCID(lcid)   ((USHORT)(lcid))
#define SORTIDFROMLCID(lcid)   ((USHORT)(((($ULONG)(lcid)) >> 16) & 0xf))
#define SORTVERSIONFROMLCID(lcid)  ((USHORT)(((($ULONG)(lcid)) >> 20) & 0xf))

#define NLS_VALID_LOCALE_MASK  0x000fffff
#define LOCALE_NAME_MAX_LENGTH   85

/*  Primary language IDs. */
#define LANG_NEUTRAL                              0x00
#define LANG_INVARIANT                            0x7f
#define LANG_AFRIKAANS                            0x36
#define LANG_ALBANIAN                             0x1c
#define LANG_ALSATIAN                             0x84
#define LANG_AMHARIC                              0x5e
#define LANG_ARABIC                               0x01
#define LANG_ARMENIAN                             0x2b
#define LANG_ASSAMESE                             0x4d
#define LANG_AZERI                                0x2c
#define LANG_AZERBAIJANI                          0x2c
#define LANG_BANGLA                               0x45
#define LANG_BASHKIR                              0x6d
#define LANG_BASQUE                               0x2d
#define LANG_BELARUSIAN                           0x23
#define LANG_BENGALI                              0x45
#define LANG_BOSNIAN                              0x1a
#define LANG_BOSNIAN_NEUTRAL                    0x781a
#define LANG_BRETON                               0x7e
#define LANG_BULGARIAN                            0x02
#define LANG_CATALAN                              0x03
#define LANG_CENTRAL_KURDISH                      0x92
#define LANG_CHEROKEE                             0x5c
#define LANG_CHINESE                              0x04
#define LANG_CHINESE_SIMPLIFIED                   0x04
#define LANG_CHINESE_TRADITIONAL                0x7c04
#define LANG_CORSICAN                             0x83
#define LANG_CROATIAN                             0x1a
#define LANG_CZECH                                0x05
#define LANG_DANISH                               0x06
#define LANG_DARI                                 0x8c
#define LANG_DIVEHI                               0x65
#define LANG_DUTCH                                0x13
#define LANG_ENGLISH                              0x09
#define LANG_ESTONIAN                             0x25
#define LANG_FAEROESE                             0x38
#define LANG_FARSI                                0x29
#define LANG_FILIPINO                             0x64
#define LANG_FINNISH                              0x0b
#define LANG_FRENCH                               0x0c
#define LANG_FRISIAN                              0x62
#define LANG_FULAH                                0x67
#define LANG_GALICIAN                             0x56
#define LANG_GEORGIAN                             0x37
#define LANG_GERMAN                               0x07
#define LANG_GREEK                                0x08
#define LANG_GREENLANDIC                          0x6f
#define LANG_GUJARATI                             0x47
#define LANG_HAUSA                                0x68
#define LANG_HAWAIIAN                             0x75
#define LANG_HEBREW                               0x0d
#define LANG_HINDI                                0x39
#define LANG_HUNGARIAN                            0x0e
#define LANG_ICELANDIC                            0x0f
#define LANG_IGBO                                 0x70
#define LANG_INDONESIAN                           0x21
#define LANG_INUKTITUT                            0x5d
#define LANG_IRISH                                0x3c
#define LANG_ITALIAN                              0x10
#define LANG_JAPANESE                             0x11
#define LANG_KANNADA                              0x4b
#define LANG_KASHMIRI                             0x60
#define LANG_KAZAK                                0x3f
#define LANG_KHMER                                0x53
#define LANG_KICHE                                0x86
#define LANG_KINYARWANDA                          0x87
#define LANG_KONKANI                              0x57
#define LANG_KOREAN                               0x12
#define LANG_KYRGYZ                               0x40
#define LANG_LAO                                  0x54
#define LANG_LATVIAN                              0x26
#define LANG_LITHUANIAN                           0x27
#define LANG_LOWER_SORBIAN                        0x2e
#define LANG_LUXEMBOURGISH                        0x6e
#define LANG_MACEDONIAN                           0x2f
#define LANG_MALAY                                0x3e
#define LANG_MALAYALAM                            0x4c
#define LANG_MALTESE                              0x3a
#define LANG_MANIPURI                             0x58
#define LANG_MAORI                                0x81
#define LANG_MAPUDUNGUN                           0x7a
#define LANG_MARATHI                              0x4e
#define LANG_MOHAWK                               0x7c
#define LANG_MONGOLIAN                            0x50
#define LANG_NEPALI                               0x61
#define LANG_NORWEGIAN                            0x14
#define LANG_OCCITAN                              0x82
#define LANG_ODIA                                 0x48
#define LANG_ORIYA                                0x48
#define LANG_PASHTO                               0x63
#define LANG_PERSIAN                              0x29
#define LANG_POLISH                               0x15
#define LANG_PORTUGUESE                           0x16
#define LANG_PULAR                                0x67
#define LANG_PUNJABI                              0x46
#define LANG_QUECHUA                              0x6b
#define LANG_ROMANIAN                             0x18
#define LANG_ROMANSH                              0x17
#define LANG_RUSSIAN                              0x19
#define LANG_SAKHA                                0x85
#define LANG_SAMI                                 0x3b
#define LANG_SANSKRIT                             0x4f
#define LANG_SCOTTISH_GAELIC                      0x91
#define LANG_SERBIAN                              0x1a
#define LANG_SERBIAN_NEUTRAL                    0x7c1a
#define LANG_SINDHI                               0x59
#define LANG_SINHALESE                            0x5b
#define LANG_SLOVAK                               0x1b
#define LANG_SLOVENIAN                            0x24
#define LANG_SOTHO                                0x6c
#define LANG_SPANISH                              0x0a
#define LANG_SWAHILI                              0x41
#define LANG_SWEDISH                              0x1d
#define LANG_SYRIAC                               0x5a
#define LANG_TAJIK                                0x28
#define LANG_TAMAZIGHT                            0x5f
#define LANG_TAMIL                                0x49
#define LANG_TATAR                                0x44
#define LANG_TELUGU                               0x4a
#define LANG_THAI                                 0x1e
#define LANG_TIBETAN                              0x51
#define LANG_TIGRIGNA                             0x73
#define LANG_TIGRINYA                             0x73
#define LANG_TSWANA                               0x32
#define LANG_TURKISH                              0x1f
#define LANG_TURKMEN                              0x42
#define LANG_UIGHUR                               0x80
#define LANG_UKRAINIAN                            0x22
#define LANG_UPPER_SORBIAN                        0x2e
#define LANG_URDU                                 0x20
#define LANG_UZBEK                                0x43
#define LANG_VALENCIAN                            0x03
#define LANG_VIETNAMESE                           0x2a
#define LANG_WELSH                                0x52
#define LANG_WOLOF                                0x88
#define LANG_XHOSA                                0x34
#define LANG_YAKUT                                0x85
#define LANG_YI                                   0x78
#define LANG_YORUBA                               0x6a
#define LANG_ZULU                                 0x35

#ifdef __REACTOS__
/* WINE extensions */
/* These are documented by the MSDN but are missing from the Windows header */
#define LANG_MALAGASY       0x8d

/* FIXME: these are not defined anywhere */
#define LANG_SUTU           0x30
#define LANG_TSONGA         0x31
#define LANG_VENDA          0x33

/* non standard; keep the number high enough (but < 0xff) */
#define LANG_ASTURIAN                    0xa5
#define LANG_ESPERANTO                   0x8f
#define LANG_WALON                       0x90
#define LANG_CORNISH                     0x92
#define LANG_MANX_GAELIC                 0x94
#endif

#define SUBLANG_NEUTRAL                             0x00
#define SUBLANG_DEFAULT                             0x01
#define SUBLANG_SYS_DEFAULT                         0x02
#define SUBLANG_CUSTOM_DEFAULT                      0x03
#define SUBLANG_CUSTOM_UNSPECIFIED                  0x04
#define SUBLANG_UI_CUSTOM_DEFAULT                   0x05
#define SUBLANG_AFRIKAANS_SOUTH_AFRICA              0x01
#define SUBLANG_ALBANIAN_ALBANIA                    0x01
#define SUBLANG_ALSATIAN_FRANCE                     0x01
#define SUBLANG_AMHARIC_ETHIOPIA                    0x01
#define SUBLANG_ARABIC_SAUDI_ARABIA                 0x01
#define SUBLANG_ARABIC_IRAQ                         0x02
#define SUBLANG_ARABIC_EGYPT                        0x03
#define SUBLANG_ARABIC_LIBYA                        0x04
#define SUBLANG_ARABIC_ALGERIA                      0x05
#define SUBLANG_ARABIC_MOROCCO                      0x06
#define SUBLANG_ARABIC_TUNISIA                      0x07
#define SUBLANG_ARABIC_OMAN                         0x08
#define SUBLANG_ARABIC_YEMEN                        0x09
#define SUBLANG_ARABIC_SYRIA                        0x0a
#define SUBLANG_ARABIC_JORDAN                       0x0b
#define SUBLANG_ARABIC_LEBANON                      0x0c
#define SUBLANG_ARABIC_KUWAIT                       0x0d
#define SUBLANG_ARABIC_UAE                          0x0e
#define SUBLANG_ARABIC_BAHRAIN                      0x0f
#define SUBLANG_ARABIC_QATAR                        0x10
#define SUBLANG_ARMENIAN_ARMENIA                    0x01
#define SUBLANG_ASSAMESE_INDIA                      0x01
#define SUBLANG_AZERI_LATIN                         0x01
#define SUBLANG_AZERI_CYRILLIC                      0x02
#define SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN        0x01
#define SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC     0x02
#define SUBLANG_BANGLA_INDIA                        0x01
#define SUBLANG_BANGLA_BANGLADESH                   0x02
#define SUBLANG_BASHKIR_RUSSIA                      0x01
#define SUBLANG_BASQUE_BASQUE                       0x01
#define SUBLANG_BELARUSIAN_BELARUS                  0x01
#define SUBLANG_BENGALI_INDIA                       0x01
#define SUBLANG_BENGALI_BANGLADESH                  0x02
#define SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN    0x05
#define SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC 0x08
#define SUBLANG_BRETON_FRANCE                       0x01
#define SUBLANG_BULGARIAN_BULGARIA                  0x01
#define SUBLANG_CATALAN_CATALAN                     0x01
#define SUBLANG_CENTRAL_KURDISH_IRAQ                0x01
#define SUBLANG_CHEROKEE_CHEROKEE                   0x01
#define SUBLANG_CHINESE_TRADITIONAL                 0x01
#define SUBLANG_CHINESE_SIMPLIFIED                  0x02
#define SUBLANG_CHINESE_HONGKONG                    0x03
#define SUBLANG_CHINESE_SINGAPORE                   0x04
#define SUBLANG_CHINESE_MACAU                       0x05
#define SUBLANG_CORSICAN_FRANCE                     0x01
#define SUBLANG_CZECH_CZECH_REPUBLIC                0x01
#define SUBLANG_CROATIAN_CROATIA                    0x01
#define SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN   0x04
#define SUBLANG_DANISH_DENMARK                      0x01
#define SUBLANG_DARI_AFGHANISTAN                    0x01
#define SUBLANG_DIVEHI_MALDIVES                     0x01
#define SUBLANG_DUTCH                               0x01
#define SUBLANG_DUTCH_BELGIAN                       0x02
#define SUBLANG_ENGLISH_US                          0x01
#define SUBLANG_ENGLISH_UK                          0x02
#define SUBLANG_ENGLISH_AUS                         0x03
#define SUBLANG_ENGLISH_CAN                         0x04
#define SUBLANG_ENGLISH_NZ                          0x05
#define SUBLANG_ENGLISH_EIRE                        0x06
#define SUBLANG_ENGLISH_SOUTH_AFRICA                0x07
#define SUBLANG_ENGLISH_JAMAICA                     0x08
#define SUBLANG_ENGLISH_CARIBBEAN                   0x09
#define SUBLANG_ENGLISH_BELIZE                      0x0a
#define SUBLANG_ENGLISH_TRINIDAD                    0x0b
#define SUBLANG_ENGLISH_ZIMBABWE                    0x0c
#define SUBLANG_ENGLISH_PHILIPPINES                 0x0d
#define SUBLANG_ENGLISH_INDIA                       0x10
#define SUBLANG_ENGLISH_MALAYSIA                    0x11
#define SUBLANG_ENGLISH_SINGAPORE                   0x12
#define SUBLANG_ESTONIAN_ESTONIA                    0x01
#define SUBLANG_FAEROESE_FAROE_ISLANDS              0x01
#define SUBLANG_FILIPINO_PHILIPPINES                0x01
#define SUBLANG_FINNISH_FINLAND                     0x01
#define SUBLANG_FRENCH                              0x01
#define SUBLANG_FRENCH_BELGIAN                      0x02
#define SUBLANG_FRENCH_CANADIAN                     0x03
#define SUBLANG_FRENCH_SWISS                        0x04
#define SUBLANG_FRENCH_LUXEMBOURG                   0x05
#define SUBLANG_FRENCH_MONACO                       0x06
#define SUBLANG_FRISIAN_NETHERLANDS                 0x01
#define SUBLANG_FULAH_SENEGAL                       0x02
#define SUBLANG_GALICIAN_GALICIAN                   0x01
#define SUBLANG_GEORGIAN_GEORGIA                    0x01
#define SUBLANG_GERMAN                              0x01
#define SUBLANG_GERMAN_SWISS                        0x02
#define SUBLANG_GERMAN_AUSTRIAN                     0x03
#define SUBLANG_GERMAN_LUXEMBOURG                   0x04
#define SUBLANG_GERMAN_LIECHTENSTEIN                0x05
#define SUBLANG_GREEK_GREECE                        0x01
#define SUBLANG_GREENLANDIC_GREENLAND               0x01
#define SUBLANG_GUJARATI_INDIA                      0x01
#define SUBLANG_HAUSA_NIGERIA_LATIN                 0x01
#define SUBLANG_HAWAIIAN_US                         0x01
#define SUBLANG_HEBREW_ISRAEL                       0x01
#define SUBLANG_HINDI_INDIA                         0x01
#define SUBLANG_HUNGARIAN_HUNGARY                   0x01
#define SUBLANG_ICELANDIC_ICELAND                   0x01
#define SUBLANG_IGBO_NIGERIA                        0x01
#define SUBLANG_INDONESIAN_INDONESIA                0x01
#define SUBLANG_INUKTITUT_CANADA                    0x01
#define SUBLANG_INUKTITUT_CANADA_LATIN              0x02
#define SUBLANG_IRISH_IRELAND                       0x02
#define SUBLANG_ITALIAN                             0x01
#define SUBLANG_ITALIAN_SWISS                       0x02
#define SUBLANG_JAPANESE_JAPAN                      0x01
#define SUBLANG_KANNADA_INDIA                       0x01
#define SUBLANG_KASHMIRI_SASIA                      0x02
#define SUBLANG_KASHMIRI_INDIA                      0x02
#define SUBLANG_KAZAK_KAZAKHSTAN                    0x01
#define SUBLANG_KHMER_CAMBODIA                      0x01
#define SUBLANG_KICHE_GUATEMALA                     0x01
#define SUBLANG_KINYARWANDA_RWANDA                  0x01
#define SUBLANG_KONKANI_INDIA                       0x01
#define SUBLANG_KOREAN                              0x01
#define SUBLANG_KYRGYZ_KYRGYZSTAN                   0x01
#define SUBLANG_LAO_LAO                             0x01
#define SUBLANG_LATVIAN_LATVIA                      0x01
#define SUBLANG_LITHUANIAN                          0x01
#define SUBLANG_LOWER_SORBIAN_GERMANY               0x02
#define SUBLANG_LUXEMBOURGISH_LUXEMBOURG            0x01
#define SUBLANG_MACEDONIAN_MACEDONIA                0x01
#define SUBLANG_MALAY_MALAYSIA                      0x01
#define SUBLANG_MALAY_BRUNEI_DARUSSALAM             0x02
#define SUBLANG_MALAYALAM_INDIA                     0x01
#define SUBLANG_MALTESE_MALTA                       0x01
#define SUBLANG_MAORI_NEW_ZEALAND                   0x01
#define SUBLANG_MAPUDUNGUN_CHILE                    0x01
#define SUBLANG_MARATHI_INDIA                       0x01
#define SUBLANG_MOHAWK_MOHAWK                       0x01
#define SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA         0x01
#define SUBLANG_MONGOLIAN_PRC                       0x02
#define SUBLANG_NEPALI_INDIA                        0x02
#define SUBLANG_NEPALI_NEPAL                        0x01
#define SUBLANG_NORWEGIAN_BOKMAL                    0x01
#define SUBLANG_NORWEGIAN_NYNORSK                   0x02
#define SUBLANG_OCCITAN_FRANCE                      0x01
#define SUBLANG_ODIA_INDIA                          0x01
#define SUBLANG_ORIYA_INDIA                         0x01
#define SUBLANG_PASHTO_AFGHANISTAN                  0x01
#define SUBLANG_PERSIAN_IRAN                        0x01
#define SUBLANG_POLISH_POLAND                       0x01
#define SUBLANG_PORTUGUESE                          0x02
#define SUBLANG_PORTUGUESE_BRAZILIAN                0x01
#define SUBLANG_PULAR_SENEGAL                       0x02
#define SUBLANG_PUNJABI_INDIA                       0x01
#define SUBLANG_PUNJABI_PAKISTAN                    0x02
#define SUBLANG_QUECHUA_BOLIVIA                     0x01
#define SUBLANG_QUECHUA_ECUADOR                     0x02
#define SUBLANG_QUECHUA_PERU                        0x03
#define SUBLANG_ROMANIAN_ROMANIA                    0x01
#define SUBLANG_ROMANSH_SWITZERLAND                 0x01
#define SUBLANG_RUSSIAN_RUSSIA                      0x01
#define SUBLANG_SAKHA_RUSSIA                        0x01
#define SUBLANG_SAMI_NORTHERN_NORWAY                0x01
#define SUBLANG_SAMI_NORTHERN_SWEDEN                0x02
#define SUBLANG_SAMI_NORTHERN_FINLAND               0x03
#define SUBLANG_SAMI_LULE_NORWAY                    0x04
#define SUBLANG_SAMI_LULE_SWEDEN                    0x05
#define SUBLANG_SAMI_SOUTHERN_NORWAY                0x06
#define SUBLANG_SAMI_SOUTHERN_SWEDEN                0x07
#define SUBLANG_SAMI_SKOLT_FINLAND                  0x08
#define SUBLANG_SAMI_INARI_FINLAND                  0x09
#define SUBLANG_SANSKRIT_INDIA                      0x01
#define SUBLANG_SCOTTISH_GAELIC                     0x01
#define SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN    0x06
#define SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC 0x07
#define SUBLANG_SERBIAN_MONTENEGRO_LATIN            0x0b
#define SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC         0x0c
#define SUBLANG_SERBIAN_SERBIA_LATIN                0x09
#define SUBLANG_SERBIAN_SERBIA_CYRILLIC             0x0a
#define SUBLANG_SERBIAN_CROATIA                     0x01
#define SUBLANG_SERBIAN_LATIN                       0x02
#define SUBLANG_SERBIAN_CYRILLIC                    0x03
#define SUBLANG_SINDHI_INDIA                        0x01
#define SUBLANG_SINDHI_PAKISTAN                     0x02
#define SUBLANG_SINDHI_AFGHANISTAN                  0x02
#define SUBLANG_SINHALESE_SRI_LANKA                 0x01
#define SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA         0x01
#define SUBLANG_SLOVAK_SLOVAKIA                     0x01
#define SUBLANG_SLOVENIAN_SLOVENIA                  0x01
#define SUBLANG_SPANISH                             0x01
#define SUBLANG_SPANISH_MEXICAN                     0x02
#define SUBLANG_SPANISH_MODERN                      0x03
#define SUBLANG_SPANISH_GUATEMALA                   0x04
#define SUBLANG_SPANISH_COSTA_RICA                  0x05
#define SUBLANG_SPANISH_PANAMA                      0x06
#define SUBLANG_SPANISH_DOMINICAN_REPUBLIC          0x07
#define SUBLANG_SPANISH_VENEZUELA                   0x08
#define SUBLANG_SPANISH_COLOMBIA                    0x09
#define SUBLANG_SPANISH_PERU                        0x0a
#define SUBLANG_SPANISH_ARGENTINA                   0x0b
#define SUBLANG_SPANISH_ECUADOR                     0x0c
#define SUBLANG_SPANISH_CHILE                       0x0d
#define SUBLANG_SPANISH_URUGUAY                     0x0e
#define SUBLANG_SPANISH_PARAGUAY                    0x0f
#define SUBLANG_SPANISH_BOLIVIA                     0x10
#define SUBLANG_SPANISH_EL_SALVADOR                 0x11
#define SUBLANG_SPANISH_HONDURAS                    0x12
#define SUBLANG_SPANISH_NICARAGUA                   0x13
#define SUBLANG_SPANISH_PUERTO_RICO                 0x14
#define SUBLANG_SPANISH_US                          0x15
#define SUBLANG_SWAHILI_KENYA                       0x01
#define SUBLANG_SWEDISH                             0x01
#define SUBLANG_SWEDISH_FINLAND                     0x02
#define SUBLANG_SYRIAC_SYRIA                        0x01
#define SUBLANG_TAJIK_TAJIKISTAN                    0x01
#define SUBLANG_TAMAZIGHT_ALGERIA_LATIN             0x02
#define SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH          0x04
#define SUBLANG_TAMIL_INDIA                         0x01
#define SUBLANG_TAMIL_SRI_LANKA                     0x02
#define SUBLANG_TATAR_RUSSIA                        0x01
#define SUBLANG_TELUGU_INDIA                        0x01
#define SUBLANG_THAI_THAILAND                       0x01
#define SUBLANG_TIBETAN_PRC                         0x01
#define SUBLANG_TIGRIGNA_ERITREA                    0x02
#define SUBLANG_TIGRINYA_ERITREA                    0x02
#define SUBLANG_TIGRINYA_ETHIOPIA                   0x01
#define SUBLANG_TSWANA_BOTSWANA                     0x02
#define SUBLANG_TSWANA_SOUTH_AFRICA                 0x01
#define SUBLANG_TURKISH_TURKEY                      0x01
#define SUBLANG_TURKMEN_TURKMENISTAN                0x01
#define SUBLANG_UIGHUR_PRC                          0x01
#define SUBLANG_UKRAINIAN_UKRAINE                   0x01
#define SUBLANG_UPPER_SORBIAN_GERMANY               0x01
#define SUBLANG_URDU_PAKISTAN                       0x01
#define SUBLANG_URDU_INDIA                          0x02
#define SUBLANG_UZBEK_LATIN                         0x01
#define SUBLANG_UZBEK_CYRILLIC                      0x02
#define SUBLANG_VALENCIAN_VALENCIA                  0x02
#define SUBLANG_VIETNAMESE_VIETNAM                  0x01
#define SUBLANG_WELSH_UNITED_KINGDOM                0x01
#define SUBLANG_WOLOF_SENEGAL                       0x01
#define SUBLANG_XHOSA_SOUTH_AFRICA                  0x01
#define SUBLANG_YAKUT_RUSSIA                        0x01
#define SUBLANG_YI_PRC                              0x01
#define SUBLANG_YORUBA_NIGERIA                      0x01
#define SUBLANG_ZULU_SOUTH_AFRICA                   0x01

#ifdef __REACTOS__
/* WINE extensions */
#define SUBLANG_DUTCH_SURINAM              0x03
#define SUBLANG_ROMANIAN_MOLDAVIA          0x02
#define SUBLANG_RUSSIAN_MOLDAVIA           0x02
#define SUBLANG_LITHUANIAN_CLASSIC         0x02
#define SUBLANG_MANX_GAELIC                0x01
#endif

#define SORT_DEFAULT                     0x0
#define SORT_INVARIANT_MATH              0x1
#define SORT_JAPANESE_XJIS               0x0
#define SORT_JAPANESE_UNICODE            0x1
#define SORT_JAPANESE_RADICALSTROKE      0x4
#define SORT_CHINESE_BIG5                0x0
#define SORT_CHINESE_PRCP                0x0
#define SORT_CHINESE_UNICODE             0x1
#define SORT_CHINESE_PRC                 0x2
#define SORT_CHINESE_BOPOMOFO            0x3
#define SORT_CHINESE_RADICALSTROKE       0x4
#define SORT_KOREAN_KSC                  0x0
#define SORT_KOREAN_UNICODE              0x1
#define SORT_GERMAN_PHONE_BOOK           0x1
#define SORT_HUNGARIAN_DEFAULT           0x0
#define SORT_HUNGARIAN_TECHNICAL         0x1
#define SORT_GEORGIAN_TRADITIONAL        0x0
#define SORT_GEORGIAN_MODERN             0x1

#define LANG_SYSTEM_DEFAULT       MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)
#define LANG_USER_DEFAULT         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)

#define LOCALE_SYSTEM_DEFAULT     MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT)
#define LOCALE_USER_DEFAULT       MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT)
#define LOCALE_CUSTOM_DEFAULT     MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_CUSTOM_DEFAULT), SORT_DEFAULT)
#define LOCALE_CUSTOM_UNSPECIFIED MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_CUSTOM_UNSPECIFIED), SORT_DEFAULT)
#define LOCALE_CUSTOM_UI_DEFAULT  MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_UI_CUSTOM_DEFAULT), SORT_DEFAULT)
#define LOCALE_NEUTRAL            MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT)
#define LOCALE_INVARIANT          MAKELCID(MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL), SORT_DEFAULT)
