#ifndef _NTDEF_H
#define _NTDEF_H

//
// Dependencies
//
#include <ctype.h>
#include <basetsd.h>
#include <excpt.h>
#include <sdkddkver.h>

// FIXME: Should we include these here?
#include <stdarg.h>
#include <string.h> 


typedef unsigned long POINTER_64; // FIXME! HACK!!!



//
// Pseudo Modifiers for Input Parameters
//
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef NOTHING
#define NOTHING
#endif

#ifndef CRITICAL
#define CRITICAL
#endif

#ifndef FAR
#define FAR
#endif


//
// Defines the "size" of an any-size array
//
#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 1
#endif

//
// Constant modifier
//
#ifndef CONST
#define CONST const
#endif


//
// FIXME: Non standard, should be deprecated
//
#ifndef STATIC
#define STATIC static
#endif

//
// TRUE/FALSE
//
#define FALSE   0
#define TRUE    1

//
// NULL/NULL64
//
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#define NULL64  0
#else
#define NULL    ((void *)0)
#define NULL64  ((void * POINTER_64)0)
#endif
#endif // NULL


//
// FIXME
// We should use the -fms-extensions compiler flag for gcc,
// and clean up the mess.
//
#ifdef __GNUC__
#ifndef NONAMELESSUNION
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define _ANONYMOUS_UNION __extension__
#define _ANONYMOUS_STRUCT __extension__
#else
#if defined(__cplusplus)
#define _ANONYMOUS_UNION __extension__
#endif /* __cplusplus */
#endif /* __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95) */
#endif /* NONAMELESSUNION */
#elif defined(__WATCOMC__) || defined(_MSC_VER)
#define _ANONYMOUS_UNION
#define _ANONYMOUS_STRUCT
#endif /* __GNUC__/__WATCOMC__ */

#ifndef _ANONYMOUS_UNION
#define _ANONYMOUS_UNION
#define _UNION_NAME(x) x
#define DUMMYUNIONNAME  u
#define DUMMYUNIONNAME2 u2
#define DUMMYUNIONNAME3 u3
#define DUMMYUNIONNAME4 u4
#define DUMMYUNIONNAME5 u5
#define DUMMYUNIONNAME6 u6
#define DUMMYUNIONNAME7 u7
#define DUMMYUNIONNAME8 u8
#else
#define _UNION_NAME(x)
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#define DUMMYUNIONNAME6
#define DUMMYUNIONNAME7
#define DUMMYUNIONNAME8
#endif

#ifndef _ANONYMOUS_STRUCT
#define _ANONYMOUS_STRUCT
#define _STRUCT_NAME(x) x
#define DUMMYSTRUCTNAME s
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#else
#define _STRUCT_NAME(x)
#define DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME2
#define DUMMYSTRUCTNAME3
#endif



// FIXME
#define UNALIGNED
#define RESTRICTED_POINTER

//
// Returns the base address of a structure from a structure member
//
#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
  ((type *)(((ULONG_PTR)address) - (ULONG_PTR)(&(((type *)0)->field))))
#endif

//
// Returns the byte offset of the specified structure's member
//
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(Type, Field) \
  ((LONG) (&(((Type *) 0)->Field)))
#endif

//
// Returns the type's alignment
//
#define TYPE_ALIGNMENT(t) FIELD_OFFSET( struct { char x; t test; }, test )


//
// Calling Conventions
//
#if defined(_M_IX86)
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

#define NTAPI __stdcall

//
// Used by the DDK exclusively , don't put in drivers
//
#define DDKAPI __stdcall // Use NTAPI instead
#define DDKCDECLAPI __cdecl // Just use __cdecl

//
// Import and Export Specifiers
//
#ifndef DECL_IMPORT
#define DECL_IMPORT DECLSPEC_IMPORT //FIXME: DECL_IMPORT -> DECLSPEC_IMPORT
#endif
#ifndef DECL_EXPORT
#define DECL_EXPORT DECLSPEC_EXPORT //FIXME: DECL_EXPORT -> DECLSPEC_EXPORT
#endif

// Done the same way as in windef.h (for now *cough*)
#define DECLSPEC_IMPORT __declspec(dllimport)
#define DECLSPEC_EXPORT __declspec(dllexport)
#define DECLSPEC_NORETURN __declspec(noreturn)


#ifndef DECLSPEC_ADDRSAFE
#if (_MSC_VER >= 1200) && (defined(_M_ALPHA) || defined(_M_AXP64))
#define DECLSPEC_ADDRSAFE  __declspec(address_safe)
#else
#define DECLSPEC_ADDRSAFE
#endif
#endif

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
#endif

//
// Inlines
//
#ifndef FORCEINLINE
#if (_MSC_VER >= 1200)
#define FORCEINLINE __forceinline
#elif (_MSC_VER)
#define FORCEINLINE __inline
#else
#define FORCEINLINE static __inline__ __attribute__((always_inline))
#endif
#endif

#if (_MSC_VER >= 1300)
#define DECLSPEC_NOINLINE  __declspec(noinline)
#elif defined(__GNUC__)
#define DECLSPEC_NOINLINE __attribute__((noinline))
#else
#define DECLSPEC_NOINLINE
#endif

#if !defined(_M_CEE_PURE)
#define NTAPI_INLINE    NTAPI
#else
#define NTAPI_INLINE
#endif

//
// Use to specify structure alignment
//
#ifndef DECLSPEC_ALIGN
#if defined(_MSC_VER) && (_MSC_VER >= 1300) && !defined(MIDL_PASS)
#define DECLSPEC_ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
#define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
#else
#define DECLSPEC_ALIGN(x)
#endif
#endif



//
// Use to silence unused variable warnings when it is intentional 
//
#define UNREFERENCED_PARAMETER(P) {(P)=(P);}
#define UNREFERENCED_LOCAL_VARIABLE(L) {(L)=(L);}
#define DBG_UNREFERENCED_PARAMETER(P)
#define DBG_UNREFERENCED_LOCAL_VARIABLE(L)



//
// min/max helper macros
//
#ifndef NOMINMAX

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif // NOMINMAX



//
// Tell windef.h that we have defined some basic types
//
#define BASETYPES

//
// Void Pointers
//
typedef void *PVOID;
//typedef void * POINTER_64 PVOID64; 
typedef PVOID PVOID64; // FIXME!

//
// Handle Type
//
#ifdef STRICT
typedef void *HANDLE;
#define DECLARE_HANDLE(n) typedef struct n##__{int i;}*n
#else
typedef PVOID HANDLE;
#define DECLARE_HANDLE(n) typedef HANDLE n
#endif
typedef HANDLE *PHANDLE;

//
// Upper-Case Versions of Some Standard C Types
//
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#if !defined(MIDL_PASS)
typedef int INT;
#endif
#endif
typedef double DOUBLE;

//
// Used to store a non-float 8 byte aligned structure
//
typedef struct _QUAD
{
    _ANONYMOUS_UNION union
    {
        __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    };
} QUAD, *PQUAD, UQUAD, *PUQUAD;


//
// Unsigned Types
//
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned long ULONG, *PULONG;
typedef CONST UCHAR *PCUCHAR;
typedef CONST USHORT *PCUSHORT;
typedef CONST ULONG *PCULONG;

typedef UCHAR FCHAR;
typedef USHORT FSHORT;
typedef ULONG FLONG;
typedef UCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;

//
// Signed Types
//
typedef SHORT *PSHORT;
typedef LONG *PLONG;

typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

typedef signed char SCHAR;
typedef SCHAR *PSCHAR;

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;
#endif

//
// 64-bit types
//
typedef __int64 LONGLONG, *PLONGLONG;
typedef unsigned __int64 ULONGLONG, *PULONGLONG;
typedef ULONGLONG DWORDLONG, *PDWORDLONG;

//
// Update Sequence Number
//
typedef LONGLONG USN;


//
// ANSI (Multi-byte Character) types
//
typedef CHAR *PCHAR, *LPCH, *PCH;
typedef CONST CHAR *LPCCH, *PCCH;
typedef CHAR *NPSTR, *LPSTR, *PSTR;
typedef PSTR *PZPSTR;
typedef CONST PSTR *PCZPSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;
typedef PCSTR *PZPCSTR;

//
// Pointer to an Asciiz string
//
typedef CHAR *PSZ;
typedef CONST char *PCSZ;

//
// UNICODE (Wide Character) types
//
typedef wchar_t WCHAR;
typedef WCHAR *PWCHAR, *LPWCH, *PWCH;
typedef CONST WCHAR *LPCWCH, *PCWCH;
typedef WCHAR *NWPSTR, *LPWSTR, *PWSTR;
typedef PWSTR *PZPWSTR;
typedef CONST PWSTR *PCZPWSTR;
typedef WCHAR UNALIGNED *LPUWSTR, *PUWSTR;
typedef CONST WCHAR *LPCWSTR, *PCWSTR;
typedef PCWSTR *PZPCWSTR;
typedef CONST WCHAR UNALIGNED *LPCUWSTR, *PCUWSTR;

//
// Cardinal Data Types
//
typedef char CCHAR, *PCCHAR;
typedef short CSHORT, *PCSHORT;
typedef ULONG CLONG, *PCLONG;

//
// NLS basics (Locale and Language Ids)
//
typedef ULONG LCID;
typedef PULONG PLCID;
typedef USHORT LANGID;



//
// Large Integer Unions
//
#if defined(MIDL_PASS)
typedef struct _LARGE_INTEGER {
#else
typedef union _LARGE_INTEGER {
    _ANONYMOUS_STRUCT struct
    {
        ULONG LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;
    struct
    {
        ULONG LowPart;
        LONG HighPart;
    } u;
#endif //MIDL_PASS
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#if defined(MIDL_PASS)
typedef struct _ULARGE_INTEGER {
#else
typedef union _ULARGE_INTEGER {
    _ANONYMOUS_STRUCT struct
    {
        ULONG LowPart;
        ULONG HighPart;
    } DUMMYSTRUCTNAME;
    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    } u;
#endif //MIDL_PASS
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

//
// Physical Addresses are always treated as 64-bit wide
//
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;



//
// Locally Unique Identifier
//
typedef struct _LUID {
    ULONG LowPart;
    LONG HighPart;
} LUID, *PLUID;



//
// Native API Return Value Macros
//
#define NT_SUCCESS(Status)              (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status)          ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status)              ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status)                ((((ULONG)(Status)) >> 30) == 3)



//
// String Types
//
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0)

typedef struct _CSTRING {
  USHORT Length;
  USHORT MaximumLength;
  CONST CHAR *Buffer;
} CSTRING, *PCSTRING;
#define ANSI_NULL ((CHAR)0)

typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR  Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef CONST STRING* PCOEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;



//
// LangID and NLS
//
#define MAKELANGID(p, s)       ((((USHORT)(s)) << 10) | (USHORT)(p))
#define PRIMARYLANGID(lgid)    ((USHORT)(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((USHORT)(lgid) >> 10)

#define NLS_VALID_LOCALE_MASK  0x000fffff

#define MAKELCID(lgid, srtid)  ((ULONG)((((ULONG)((USHORT)(srtid))) << 16) |  \
                                         ((ULONG)((USHORT)(lgid)))))
#define MAKESORTLCID(lgid, srtid, ver)                                            \
                               ((ULONG)((MAKELCID(lgid, srtid)) |             \
                                    (((ULONG)((USHORT)(ver))) << 20)))
#define LANGIDFROMLCID(lcid)   ((USHORT)(lcid))
#define SORTIDFROMLCID(lcid)   ((USHORT)((((ULONG)(lcid)) >> 16) & 0xf))
#define SORTVERSIONFROMLCID(lcid)  ((USHORT)((((ULONG)(lcid)) >> 20) & 0xf))



//
// Object Attributes
//
typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

//
// Values for the Attributes member
//
#define OBJ_INHERIT             0x00000002
#define OBJ_PERMANENT           0x00000010
#define OBJ_EXCLUSIVE           0x00000020
#define OBJ_CASE_INSENSITIVE    0x00000040
#define OBJ_OPENIF              0x00000080
#define OBJ_OPENLINK            0x00000100
#define OBJ_KERNEL_HANDLE       0x00000200
#define OBJ_FORCE_ACCESS_CHECK  0x00000400
#define OBJ_VALID_ATTRIBUTES    0x000007F2

//
// Helper Macro
//
#define InitializeObjectAttributes(p,n,a,r,s) { \
  (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
  (p)->RootDirectory = (r); \
  (p)->Attributes = (a); \
  (p)->ObjectName = (n); \
  (p)->SecurityDescriptor = (s); \
  (p)->SecurityQualityOfService = NULL; \
}



//
// Product Types
//
typedef enum _NT_PRODUCT_TYPE {
	NtProductWinNt = 1,
	NtProductLanManNt,
	NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;



//
// Doubly Linked Lists
//
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

typedef struct LIST_ENTRY32
{
    ULONG Flink;
    ULONG Blink;
} LIST_ENTRY32, *PLIST_ENTRY32;

typedef struct LIST_ENTRY64
{
    ULONGLONG Flink;
    ULONGLONG Blink;
} LIST_ENTRY64, *PLIST_ENTRY64;

//
// Singly Linked Lists
//
typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;



//
// Helper Macros
//
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }

#define RTL_FIELD_SIZE(type, field) (sizeof(((type *)0)->field))

#define RTL_NUMBER_OF_V1(A) (sizeof(A)/sizeof((A)[0]))
#define RTL_NUMBER_OF_V2(A) RTL_NUMBER_OF_V1(A)
#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif
#define ARRAYSIZE(A)    RTL_NUMBER_OF_V2(A)



//
// Type Limits
//
#define MINCHAR   0x80
#define MAXCHAR   0x7f
#define MINSHORT  0x8000
#define MAXSHORT  0x7fff
#define MINLONG   0x80000000
#define MAXLONG   0x7fffffff
#define MAXUCHAR  0xff
#define MAXUSHORT 0xffff
#define MAXULONG  0xffffffff
#define MAXLONGLONG (0x7fffffffffffffffLL)



//
// Multiplication and Shift Operations
//
#define Int32x32To64(a,b) ((LONGLONG)(a)*(LONGLONG)(b))
#define UInt32x32To64(a,b) ((DWORDLONG)(a)*(DWORDLONG)(b))
#define Int64ShllMod32(a,b) ((DWORDLONG)(a)<<(b))
#define Int64ShraMod32(a,b) ((LONGLONG)(a)>>(b))
#define Int64ShrlMod32(a,b) ((DWORDLONG)(a)>>(b))



//
// C_ASSERT Definition
//
#define __C_ASSERT_JOIN(X, Y) __C_ASSERT_DO_JOIN(X, Y)
#define __C_ASSERT_DO_JOIN(X, Y) __C_ASSERT_DO_JOIN2(X, Y)
#define __C_ASSERT_DO_JOIN2(X, Y) X##Y
#define C_ASSERT(e) typedef char __C_ASSERT_JOIN(__C_ASSERT__, __LINE__)[(e) ? 1 : -1]



//
//  Primary language IDs.
//
#define LANG_NEUTRAL                     0x00
#define LANG_INVARIANT                   0x7f

#define LANG_AFRIKAANS                   0x36
#define LANG_ALBANIAN                    0x1c
#define LANG_ALSATIAN                    0x84
#define LANG_AMHARIC                     0x5e
#define LANG_ARABIC                      0x01
#define LANG_ARMENIAN                    0x2b
#define LANG_ASSAMESE                    0x4d
#define LANG_AZERI                       0x2c
#define LANG_BASHKIR                     0x6d
#define LANG_BASQUE                      0x2d
#define LANG_BELARUSIAN                  0x23
#define LANG_BENGALI                     0x45
#define LANG_BRETON                      0x7e
#define LANG_BOSNIAN                     0x1a
#define LANG_BOSNIAN_NEUTRAL           0x781a
#define LANG_BULGARIAN                   0x02
#define LANG_CATALAN                     0x03
#define LANG_CHINESE                     0x04
#define LANG_CHINESE_SIMPLIFIED          0x04
#define LANG_CHINESE_TRADITIONAL       0x7c04
#define LANG_CORSICAN                    0x83
#define LANG_CROATIAN                    0x1a
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DARI                        0x8c
#define LANG_DIVEHI                      0x65
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_ESTONIAN                    0x25
#define LANG_FAEROESE                    0x38
#define LANG_FARSI                       0x29
#define LANG_FILIPINO                    0x64
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_FRISIAN                     0x62
#define LANG_GALICIAN                    0x56
#define LANG_GEORGIAN                    0x37
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_GREENLANDIC                 0x6f
#define LANG_GUJARATI                    0x47
#define LANG_HAUSA                       0x68
#define LANG_HEBREW                      0x0d
#define LANG_HINDI                       0x39
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_IGBO                        0x70
#define LANG_INDONESIAN                  0x21
#define LANG_INUKTITUT                   0x5d
#define LANG_IRISH                       0x3c
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KANNADA                     0x4b
#define LANG_KASHMIRI                    0x60
#define LANG_KAZAK                       0x3f
#define LANG_KHMER                       0x53
#define LANG_KICHE                       0x86
#define LANG_KINYARWANDA                 0x87
#define LANG_KONKANI                     0x57
#define LANG_KOREAN                      0x12
#define LANG_KYRGYZ                      0x40
#define LANG_LAO                         0x54
#define LANG_LATVIAN                     0x26
#define LANG_LITHUANIAN                  0x27
#define LANG_LOWER_SORBIAN               0x2e
#define LANG_LUXEMBOURGISH               0x6e
#define LANG_MACEDONIAN                  0x2f
#define LANG_MALAY                       0x3e
#define LANG_MALAYALAM                   0x4c
#define LANG_MALTESE                     0x3a
#define LANG_MANIPURI                    0x58
#define LANG_MAORI                       0x81
#define LANG_MAPUDUNGUN                  0x7a
#define LANG_MARATHI                     0x4e
#define LANG_MOHAWK                      0x7c
#define LANG_MONGOLIAN                   0x50
#define LANG_NEPALI                      0x61
#define LANG_NORWEGIAN                   0x14
#define LANG_OCCITAN                     0x82
#define LANG_ORIYA                       0x48
#define LANG_PASHTO                      0x63
#define LANG_PERSIAN                     0x29
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_PUNJABI                     0x46
#define LANG_QUECHUA                     0x6b
#define LANG_ROMANIAN                    0x18
#define LANG_ROMANSH                     0x17
#define LANG_RUSSIAN                     0x19
#define LANG_SAMI                        0x3b
#define LANG_SANSKRIT                    0x4f
#define LANG_SERBIAN                     0x1a
#define LANG_SERBIAN_NEUTRAL           0x7c1a
#define LANG_SINDHI                      0x59
#define LANG_SINHALESE                   0x5b
#define LANG_SLOVAK                      0x1b
#define LANG_SLOVENIAN                   0x24
#define LANG_SOTHO                       0x6c
#define LANG_SPANISH                     0x0a
#define LANG_SWAHILI                     0x41
#define LANG_SWEDISH                     0x1d
#define LANG_SYRIAC                      0x5a
#define LANG_TAJIK                       0x28
#define LANG_TAMAZIGHT                   0x5f
#define LANG_TAMIL                       0x49
#define LANG_TATAR                       0x44
#define LANG_TELUGU                      0x4a
#define LANG_THAI                        0x1e
#define LANG_TIBETAN                     0x51
#define LANG_TIGRIGNA                    0x73
#define LANG_TSWANA                      0x32
#define LANG_TURKISH                     0x1f
#define LANG_TURKMEN                     0x42
#define LANG_UIGHUR                      0x80
#define LANG_UKRAINIAN                   0x22
#define LANG_UPPER_SORBIAN               0x2e
#define LANG_URDU                        0x20
#define LANG_UZBEK                       0x43
#define LANG_VIETNAMESE                  0x2a
#define LANG_WELSH                       0x52
#define LANG_WOLOF                       0x88
#define LANG_XHOSA                       0x34
#define LANG_YAKUT                       0x85
#define LANG_YI                          0x78
#define LANG_YORUBA                      0x6a
#define LANG_ZULU                        0x35



#endif /* _NTDEF_H */
