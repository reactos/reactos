#ifndef _NTDEF_H
#define _NTDEF_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#define NTAPI __stdcall
#define OBJ_INHERIT 2L
#define OBJ_PERMANENT 16L
#define OBJ_EXCLUSIVE 32L
#define OBJ_CASE_INSENSITIVE 64L
#define OBJ_OPENIF 128L
#define OBJ_OPENLINK 256L
#define OBJ_VALID_ATTRIBUTES 498L
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
#define STATUS_SUCCESS ((NTSTATUS)0)
#endif
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
#endif
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef enum _SECTION_INHERIT {
  ViewShare = 1,
  ViewUnmap = 2
} SECTION_INHERIT;
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
#endif /* _NTDEF_H */
