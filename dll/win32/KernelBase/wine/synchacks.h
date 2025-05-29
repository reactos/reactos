#pragma once

#define CRITICAL_SECTION RTL_CRITICAL_SECTION
#define CRITICAL_SECTION_DEBUG RTL_CRITICAL_SECTION_DEBUG
#define IMAGE_FILE_MACHINE_TARGET_HOST       0x0001 
#define RTL_CONSTANT_STRING(s)  { sizeof(s)-sizeof((s)[0]), sizeof(s), s }


//def ndk
NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
  _In_ ULONG OSMajorVersion,
  _In_ ULONG OSMinorVersion,
  _In_ ULONG SpMajorVersion,
  _In_ ULONG SpMinorVersion,
  _Out_ PULONG ReturnedProductType);
NTSYSAPI NTSTATUS  WINAPI RtlQueryActivationContextApplicationSettings(DWORD,HANDLE,const WCHAR*,const WCHAR*,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI LdrSetDefaultDllDirectories(ULONG);
NTSYSAPI NTSTATUS  WINAPI LdrRemoveDllDirectory(void*);

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
#define SYMBOLIC_LINK_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | 0x1)

//kernel32
 BOOL WINAPI DECLSPEC_HOTPATCH GetThreadGroupAffinity( HANDLE thread, GROUP_AFFINITY *affinity );

typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

HANDLE WINAPI DECLSPEC_HOTPATCH CreateRemoteThreadEx( HANDLE process, SECURITY_ATTRIBUTES *sa,
                                                      SIZE_T stack, LPTHREAD_START_ROUTINE start,
                                                      LPVOID param, DWORD flags,
                                                      LPPROC_THREAD_ATTRIBUTE_LIST attributes, DWORD *id );


BOOL WINAPI GetVolumeInformationByHandleW( HANDLE handle, WCHAR *label, DWORD label_len,
                                           DWORD *serial, DWORD *filename_len, DWORD *flags,
                                           WCHAR *fsname, DWORD fsname_len );


HRESULT WINAPI PathMatchSpecExW(const WCHAR *path, const WCHAR *mask, DWORD flags);
INT WINAPI DECLSPEC_HOTPATCH CompareStringOrdinal( const WCHAR *str1, INT len1,
                                                   const WCHAR *str2, INT len2, BOOL ignore_case );

#define URL_UNESCAPE_AS_UTF8            URL_ESCAPE_AS_UTF8

//shlwapi
HRESULT WINAPI GetAcceptLanguagesW(WCHAR *langbuf, DWORD *buflen);

BOOL WINAPI GetWindowsAccountDomainSid( PSID sid, PSID domain_sid, DWORD *size );
//rtl types
//rtl
NTSTATUS
NTAPI
RtlNewSecurityObjectEx(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                       IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                       OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                       IN LPGUID ObjectType,
                       IN BOOLEAN IsDirectoryObject,
                       IN ULONG AutoInheritFlags,
                       IN HANDLE Token,
                       IN PGENERIC_MAPPING GenericMapping);

NTSTATUS
NTAPI
RtlNewSecurityObjectWithMultipleInheritance(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                            IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                            OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                            IN LPGUID *ObjectTypes,
                                            IN ULONG GuidCount,
                                            IN BOOLEAN IsDirectoryObject,
                                            IN ULONG AutoInheritFlags,
                                            IN HANDLE Token,
                                            IN PGENERIC_MAPPING GenericMapping);
NTSTATUS
NTAPI
RtlConvertToAutoInheritSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                      IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                      OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                      IN LPGUID ObjectType,
                                      IN BOOLEAN IsDirectoryObject,
                                      IN PGENERIC_MAPPING GenericMapping);

BOOL WINAPI DECLSPEC_HOTPATCH QueryFullProcessImageNameW( HANDLE process, DWORD flags,
                                                          WCHAR *name, DWORD *size );


typedef void *PUMS_CONTEXT;
typedef void *PUMS_COMPLETION_LIST;
typedef PVOID PUMS_SCHEDULER_ENTRY_POINT;
typedef struct _UMS_SCHEDULER_STARTUP_INFO
{
    ULONG UmsVersion;
    PUMS_COMPLETION_LIST CompletionList;
    PUMS_SCHEDULER_ENTRY_POINT SchedulerProc;
    PVOID SchedulerParam;
} UMS_SCHEDULER_STARTUP_INFO, *PUMS_SCHEDULER_STARTUP_INFO;

typedef enum _RTL_UMS_SCHEDULER_REASON UMS_SCHEDULER_REASON;
typedef enum _RTL_UMS_THREAD_INFO_CLASS UMS_THREAD_INFO_CLASS, *PUMS_THREAD_INFO_CLASS;

#ifndef _PSAPI_H
typedef struct _PSAPI_WS_WATCH_INFORMATION {
  LPVOID FaultingPc;
  LPVOID FaultingVa;
} PSAPI_WS_WATCH_INFORMATION, *PPSAPI_WS_WATCH_INFORMATION;

#endif

typedef struct _PSAPI_WS_WATCH_INFORMATION_EX {
  PSAPI_WS_WATCH_INFORMATION BasicInfo;
  ULONG_PTR FaultingThreadId;
  ULONG_PTR Flags;
} PSAPI_WS_WATCH_INFORMATION_EX, *PPSAPI_WS_WATCH_INFORMATION_EX;

#define PROCESSOR_ARCHITECTURE_INTEL            0
#define PROCESSOR_ARCHITECTURE_MIPS             1
#define PROCESSOR_ARCHITECTURE_ALPHA            2
#define PROCESSOR_ARCHITECTURE_PPC              3
#define PROCESSOR_ARCHITECTURE_SHX              4
#define PROCESSOR_ARCHITECTURE_ARM              5
#define PROCESSOR_ARCHITECTURE_IA64             6
#define PROCESSOR_ARCHITECTURE_ALPHA64          7
#define PROCESSOR_ARCHITECTURE_MSIL             8
#define PROCESSOR_ARCHITECTURE_AMD64            9
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11
#define PROCESSOR_ARCHITECTURE_ARM64            12
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64   13
#define PROCESSOR_ARCHITECTURE_IA32_ON_ARM64    14
