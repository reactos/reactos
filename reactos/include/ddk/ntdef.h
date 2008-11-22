#ifndef _NTDEF_H
#define _NTDEF_H

#include <stdarg.h>
#include <windef.h>
#include <excpt.h>

/* TODO: some compilers support this */
#define RESTRICTED_POINTER

#define NTAPI __stdcall
#if !defined(_M_CEE_PURE)
#define NTAPI_INLINE    NTAPI
#else
#define NTAPI_INLINE
#endif

#ifndef DECLSPEC_NOINLINE
#define DECLSPEC_NOINLINE  __declspec(noinline)
#endif

#define OBJ_INHERIT             0x00000002
#define OBJ_PERMANENT           0x00000010
#define OBJ_EXCLUSIVE           0x00000020
#define OBJ_CASE_INSENSITIVE    0x00000040
#define OBJ_OPENIF              0x00000080
#define OBJ_OPENLINK            0x00000100
#define OBJ_KERNEL_HANDLE       0x00000200
#define OBJ_FORCE_ACCESS_CHECK  0x00000400
#define OBJ_VALID_ATTRIBUTES    0x000007F2
#define InitializeObjectAttributes(p,n,a,r,s) { \
  (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
  (p)->RootDirectory = (r); \
  (p)->Attributes = (a); \
  (p)->ObjectName = (n); \
  (p)->SecurityDescriptor = (s); \
  (p)->SecurityQualityOfService = NULL; \
}
#ifndef NT_SUCCESS
#define NT_SUCCESS(x) ((x)>=0)
#endif
#define NT_WARNING(x) ((ULONG)(x)>>30==2)
#define NT_ERROR(x) ((ULONG)(x)>>30==3)
#if !defined(_NTSECAPI_H) && !defined(_SUBAUTH_H)
typedef LONG NTSTATUS, *PNTSTATUS;
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;
typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR  Buffer;
} STRING, *PSTRING;
typedef struct _CSTRING {
  USHORT Length;
  USHORT MaximumLength;
  CONST CHAR *Buffer;
} CSTRING, *PCSTRING;
#endif
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef CONST STRING* PCOEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef CONST CHAR *LPCCH, *PCCH;
typedef signed char SCHAR;
typedef SCHAR *PSCHAR;
typedef enum _SECTION_INHERIT {
  ViewShare = 1,
  ViewUnmap = 2
} SECTION_INHERIT;
typedef enum _NT_PRODUCT_TYPE {
	NtProductWinNt = 1,
	NtProductLanManNt,
	NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;
#if !defined(_NTSECAPI_H)
typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif

typedef struct LIST_ENTRY32
{
    ULONG Flink;
    ULONG Blink;
} LIST_ENTRY32;
typedef LIST_ENTRY32 *PLIST_ENTRY32;

typedef struct LIST_ENTRY64
{
    ULONGLONG Flink;
    ULONGLONG Blink;
} LIST_ENTRY64;
typedef LIST_ENTRY64 *PLIST_ENTRY64;

#define NOTHING
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#define TYPE_ALIGNMENT( t ) FIELD_OFFSET( struct { char x; t test; }, test )
#define RTL_FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#define RTL_NUMBER_OF_V1(A) (sizeof(A)/sizeof((A)[0]))
#define RTL_NUMBER_OF_V2(A) RTL_NUMBER_OF_V1(A)
#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif
#define ARRAYSIZE(A)    RTL_NUMBER_OF_V2(A)
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

#define __C_ASSERT_JOIN(X, Y) __C_ASSERT_DO_JOIN(X, Y)
#define __C_ASSERT_DO_JOIN(X, Y) __C_ASSERT_DO_JOIN2(X, Y)
#define __C_ASSERT_DO_JOIN2(X, Y) X##Y

#define C_ASSERT(e) typedef char __C_ASSERT_JOIN(__C_ASSERT__, __LINE__)[(e) ? 1 : -1]

#endif /* _NTDEF_H */
