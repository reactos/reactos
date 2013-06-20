#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdio.h>
#include <winternl.h>

#define DRIVER_NAME L"TestDriver"

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_PRIVILEGE_NOT_HELD ((NTSTATUS)0xC0000061L)

typedef LONG NTSTATUS;

//
// umode methods
//
BOOL RegisterDriver(LPCWSTR lpDriverName, LPCWSTR lpPathName);
BOOL StartDriver(LPCWSTR lpDriverName);
BOOL StopDriver(LPCWSTR lpDriverName);
BOOL UnregisterDriver(LPCWSTR lpDriverName);

//
// undoc methods
//
BOOL ConvertPath(LPCWSTR lpPath, LPWSTR lpDevice);
BOOL LoadVia_SystemLoadGdiDriverInformation(LPWSTR lpDriverPath);
BOOL LoadVia_SystemExtendServiceTableInformation(LPWSTR lpDriverPath);
BOOL NtStartDriver(LPCWSTR lpService);
BOOL NtStopDriver(LPCWSTR lpService);


//
// undocumented stuff
//
#define SystemLoadGdiDriverInformation 26
#define SystemExtendServiceTableInformation 38
NTSYSAPI NTSTATUS NTAPI
NtSetSystemInformation(IN INT SystemInformationClass,
                       IN PVOID SystemInformation,
                       IN ULONG SystemInformationLength );
NTSTATUS
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName);

typedef struct _SYSTEM_GDI_DRIVER_INFORMATION
{
    UNICODE_STRING DriverName;
    PVOID ImageAddress;
    PVOID SectionPointer;
    PVOID EntryPoint;
    PIMAGE_EXPORT_DIRECTORY ExportSectionPointer;
    ULONG ImageLength;
} SYSTEM_GDI_DRIVER_INFORMATION, *PSYSTEM_GDI_DRIVER_INFORMATION;

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation
} OBJECT_INFO_CLASS;

NTSTATUS NtQueryObject(IN HANDLE Handle,
                       IN OBJECT_INFO_CLASS ObjectInformationClass,
                       OUT PVOID ObjectInformation,
                       IN ULONG ObjectInformationLength,
                       OUT PULONG ReturnLength);


typedef struct _OBJECT_NAME_INFORMATION {
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;


