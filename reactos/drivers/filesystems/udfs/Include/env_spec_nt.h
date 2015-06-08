////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __ENV_SPEC_NT_NATIVE__H__
#define __ENV_SPEC_NT_NATIVE__H__

#ifdef NT_NATIVE_MODE

#include "zw_2_nt.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifndef MAX_PATH
#define MAX_PATH   260
#endif //MAX_PATH

BOOLEAN
GetOsVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );

#define PsGetVersion(a,b,c,d)  GetOsVersion(a,b,c,d)

#define InterlockedIncrement(addr) \
    ((*addr)++)
#define InterlockedDecrement(addr) \
    ((*addr)--)
int
__inline
InterlockedExchangeAdd(PLONG addr, LONG i) {
    LONG Old = (*addr);
    (*addr) += i;
    return Old;
}

#define DeviceIoControl(h, ctlc, ib, is, ob, os, r, ov)  MyDeviceIoControl(h, ctlc, ib, is, ob, os, r, ov)  

BOOLEAN
MyDeviceIoControl(
    HANDLE hDevice,
    DWORD  dwIoControlCode,
    PVOID  lpInBuffer,
    DWORD  nInBufferSize,
    PVOID  lpOutBuffer,
    DWORD  nOutBufferSize,
    DWORD* lpBytesReturned,
    PVOID  lpOverlapped
    );

#define OemToCharW(ansi_s, uni_s)    (swprintf(uni_s, L"%S", ansi_s))
#define MultiByteToWideChar(cp, f, ansi_s, a_sz, uni_s, u_sz)  (swprintf(uni_s, L"%S", ansi_s))

VOID
Sleep(
    ULONG t
    );

#define GlobalAlloc(foo, size)  MyGlobalAlloc( size );
#define GlobalFree(ptr)         MyGlobalFree( ptr );

extern "C"
PVOID MyGlobalAlloc(ULONG Size);

extern "C"
VOID  MyGlobalFree(PVOID Addr);

#define ExitProcess(Status)    NtTerminateProcess( NtCurrentProcess(), Status );

extern "C"
VOID
PrintNtConsole(
    PCHAR DebugMessage,
    ...
    );

extern "C"
NTSTATUS
EnvFileOpenW(
    PWCHAR Name,
    HANDLE* ph
    );

extern "C"
NTSTATUS
EnvFileOpenA(
    PCHAR Name,
    HANDLE* ph
    );

extern "C"
NTSTATUS
EnvFileClose(
    HANDLE hFile
    );

extern "C"
NTSTATUS
EnvFileGetSizeByHandle(
    HANDLE hFile,
    PLONGLONG lpFileSize
    );

extern "C"
NTSTATUS
EnvFileGetSizeA(
    PCHAR Name,
    PLONGLONG lpFileSize
    );

extern "C"
NTSTATUS
EnvFileGetSizeW(
    PWCHAR Name,
    PLONGLONG lpFileSize
    );

extern "C"
BOOLEAN
EnvFileExistsA(PCHAR Name);

extern "C"
BOOLEAN
EnvFileExistsW(PWCHAR Name);

extern "C"
NTSTATUS
EnvFileWrite(
    HANDLE h,
    PVOID ioBuffer,
    ULONG Length,
    PULONG bytesWritten
    );

extern "C"
NTSTATUS
EnvFileRead(
    HANDLE h,
    PVOID ioBuffer,
    ULONG Length,
    PULONG bytesRead
    );

#define ENV_FILE_CURRENT  1
#define ENV_FILE_END      2
#define ENV_FILE_BEGIN    3

extern "C"
NTSTATUS
EnvFileSetPointer(
    HANDLE hFile,
    LONGLONG lDistanceToMove,
    LONGLONG* lResultPointer,
    DWORD dwMoveMethod
    );

extern "C"
NTSTATUS 
EnvFileDeleteW(
    PWCHAR fName
    );

#define PrintDbgConsole    PrintNtConsole

#ifdef __cplusplus
};
#endif //__cplusplus

#endif //NT_NATIVE_MODE

#endif //__ENV_SPEC_NT_NATIVE__H__
