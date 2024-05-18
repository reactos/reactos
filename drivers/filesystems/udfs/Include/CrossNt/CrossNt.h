#ifndef __CROSS_VERSION_LIB_NT__H__
#define __CROSS_VERSION_LIB_NT__H__

extern "C" {

#pragma pack(push, 8)

#if !defined(NT_INCLUDED)
#include <ntddk.h>                  // various NT definitions
#endif

#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "ntddk_ex.h"

#include "rwlock.h"

#ifdef CROSS_NT_INTERNAL
#include "ilock.h"
#endif //CROSS_NT_INTERNAL

#include "misc.h"
#include "tools.h"

#pragma pack(pop)

extern "C"
NTSTATUS
CrNtInit(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

extern "C"
ULONG
CrNtGetCPUGen();

extern "C"
PVOID
CrNtGetModuleBase(
    IN PCHAR  pModuleName
    );

extern "C"
PVOID
CrNtFindModuleBaseByPtr(
    IN PVOID  ptrInSection,
    IN PCHAR  ptrExportedName
    );

extern "C"
PVOID
CrNtGetProcAddress(
    PVOID ModuleBase,
    PCHAR pFunctionName
    );

typedef BOOLEAN (__stdcall *ptrCrNtPsGetVersion)(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );

extern "C"
ptrCrNtPsGetVersion  CrNtPsGetVersion;

typedef NTSTATUS (__stdcall *ptrCrNtNtQuerySystemInformation)(
    IN SYSTEM_INFORMATION_CLASS SystemInfoClass,
    OUT PVOID                   SystemInfoBuffer,
    IN ULONG                    SystemInfoBufferSize,
    OUT PULONG                  BytesReturned OPTIONAL
    );

extern "C"
ptrCrNtNtQuerySystemInformation  CrNtNtQuerySystemInformation;

extern "C"
PVOID
CrNtSkipImportStub(
    PVOID p
    );

extern "C" {

extern ULONG  MajorVersion;
extern ULONG  MinorVersion;
extern ULONG  BuildNumber;
extern ULONG  SPVersion;

extern HANDLE g_hNtosKrnl;
extern HANDLE g_hHal;

extern PCHAR  g_KeNumberProcessors;

};

#define WinVer_Is351   (MajorVersion==0x03)
#define WinVer_IsNT    (MajorVersion==0x04)
#define WinVer_Is2k    (MajorVersion==0x05 && MinorVersion==0x00)
#define WinVer_IsXP    (MajorVersion==0x05 && MinorVersion==0x01)
#define WinVer_IsXPp   (MajorVersion==0x05 && MinorVersion>=0x01)
#define WinVer_IsdNET  (MajorVersion==0x05 && MinorVersion==0x02)
#define WinVer_IsdNETp ((MajorVersion==0x05 && MinorVersion>=0x02) || (MajorVersion>0x05))
#define WinVer_IsVista (MajorVersion==0x06 && MinorVersion==0x00)

#define WinVer_Id()   ((MajorVersion << 8) | MinorVersion)

#define WinVer_351    (0x0351)
#define WinVer_NT     (0x0400)
#define WinVer_ROS    (0x0401)
#define WinVer_2k     (0x0500)
#define WinVer_XP     (0x0501)
#define WinVer_dNET   (0x0502)
#define WinVer_Vista  (0x0600)

#ifdef _DEBUG

// NT3.51 doesn't export strlen() and strcmp()
// The same time, Release build doesn't depend no these functions since they are inlined

size_t __cdecl CrNtstrlen (
        const char * str
        );

int __cdecl CrNtstrcmp (
        const char * src,
        const char * dst
        );

#define strlen CrNtstrlen
#define strcmp CrNtstrcmp

#endif //_DEBUG

#define CROSSNT_DECL_API

#include "CrNtDecl.h"
#include "CrNtStubs.h"

#undef CROSSNT_DECL_API

}; // end extern "C"

#endif //__CROSS_VERSION_LIB_NT__H__
