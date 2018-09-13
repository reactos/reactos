//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ntlmkrnl.h
//
// Contents:    global include file for kernel mode NTLM security package
//
//
// History:     16-April-1996       Created     MikeSw
//
//------------------------------------------------------------------------

#ifndef __NTLMKRNL_H__
#define __NTLMKRNL_H__

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

extern "C"
{
#include <ntos.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <winerror.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_KERNEL
#define SECURITY_PACKAGE
#define SECURITY_NTLM
#include <security.h>
#include <secint.h>
#include <..\ntlmssp.h>
}

//
// Global state variables
//

ULONG NtLmPackageId;
extern PSECPKG_KERNEL_FUNCTIONS KernelFunctions;
extern POOL_TYPE NtlmPoolType ;

//
// Useful macros
//

//
// Macro to return the type field of a SecBuffer
//

#define BUFFERTYPE(_x_) ((_x_).BufferType & ~SECBUFFER_ATTRMASK)

#define NtLmAllocate( _x_ ) ExAllocatePoolWithTag( NtlmPoolType, (_x_) ,  'CvsM')
#define NtLmFree( _x_ ) ExFreePool(_x_)


#if DBG


#define DEB_ERROR               0x00000001
#define DEB_WARN                0x00000002
#define DEB_TRACE               0x00000004
#define DEB_TRACE_LOCKS         0x00010000

extern "C"
{
void KsecDebugOut(ULONG, const char *, ...);
}

#define DebugLog(x) KsecDebugOut x

#else // DBG

#define DebugLog(x)

#endif // DBG

#endif // __NTLMKRNL_H__
