/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 *                  Hermes Belusca-Maito
 */

#ifndef __CLIENT_SERVER_H__
#define __CLIENT_SERVER_H__

#pragma once

#define UNICODE
#define _UNICODE

#include <apitest.h>

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strsafe.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
// #define __USE_W32_SOCKETS
#include <windows.h>
// #include <winsock.h>
#include <winsock2.h>

#define SECURITY_WIN32  // Needed by sspi.h (SECURITY_WIN32 or SECURITY_KERNEL or SECURITY_MAC)
#define _NO_KSECDD_IMPORT_
#include <sspi.h>
/** Our broken psdk sspi.h misses this definition. But it's present in xdk sspi.h. I guess the psdk one needs some sync... **/
#ifndef SECPKG_ID_NONE
#define SECPKG_ID_NONE 0xFFFF
#endif

#include <ntsecapi.h>
#include <security.h>   // Security.h must come *before* secext.h
#include <secext.h>

#define SEC_SUCCESS(Status) ((Status) >= 0)

#define printerr(errnum)    \
do { \
    LPWSTR buffer;  \
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
                   NULL,    \
                   errnum,  \
                   LANG_USER_DEFAULT,   \
                   (LPWSTR)&buffer,     \
                   0,       \
                   NULL);   \
    sync_err("%S", buffer); \
    LocalFree(buffer);      \
} while (0)

void sync_msg(char* msg, ...);

#ifdef __MSVC__
#define sync_ok(cond, msg, ...) \
do {   \
    char buf[512];  \
    \
    StringCbPrintfA(buf, sizeof(buf), msg, __VA_ARGS__); \
    sync_msg_enter();   \
    ok(cond, "[%.4ld] %s", GetCurrentThreadId(), buf); \
    sync_msg_leave();   \
} while (0)
#else
#define sync_ok(cond, msg, ...) \
do {   \
    char buf[512];  \
    \
    StringCbPrintfA(buf, sizeof(buf), msg, ##__VA_ARGS__); \
    sync_msg_enter();   \
    ok(cond, "[%.4ld] %s", GetCurrentThreadId(), buf); \
    sync_msg_leave();   \
} while (0)
#endif

#ifdef __MSVC__
#define sync_trace(msg, ...)  \
do {   \
    char buf[512]; \
    \
    StringCbPrintfA(buf, sizeof(buf), msg, __VA_ARGS__);   \
    sync_msg_enter();   \
    trace("[%.4ld] %s", GetCurrentThreadId(), buf);    \
    sync_msg_leave();   \
} while (0)
#else
#define sync_trace(msg, ...)  \
do {   \
    char buf[512]; \
    \
    StringCbPrintfA(buf, sizeof(buf), msg, ##__VA_ARGS__);   \
    sync_msg_enter();   \
    trace("[%.4ld] %s", GetCurrentThreadId(), buf);    \
    sync_msg_leave();   \
} while (0)
#endif

#ifdef __MSVC__
#define sync_msg(msg, ...) \
{ \
    char buf[512]; \
 \
    StringCbPrintfA(buf, sizeof(buf), msg, __VA_ARGS__); \
    sync_msg_enter();   \
    printf("[%.4ld] %s", GetCurrentThreadId(), buf);  \
    sync_msg_leave();   \
}
#else
#define sync_msg(msg, ...) \
{ \
    char buf[512]; \
 \
    StringCbPrintfA(buf, sizeof(buf), msg, ##__VA_ARGS__); \
    \
    sync_msg_enter();   \
    printf("[%.4ld] %s", GetCurrentThreadId(), buf); \
    sync_msg_leave();   \
}
#endif


#ifdef __MSVC__
#define sync_err(msg, ...) sync_ok(FALSE, msg, __VA_ARGS__)
#else
#define sync_err(msg, ...) sync_ok(FALSE, msg, ##__VA_ARGS__)
#endif

void sync_msg_enter();
void sync_msg_leave();

extern PSecurityFunctionTable client1_SecFuncTable;
extern PSecurityFunctionTable server1_SecFuncTable;

void PrintErrorString(int errnum);
void wserr( int rc, LPCWSTR  const funcname );

void initSecLib(
    HINSTANCE* phSec,
    PSecurityFunctionTable* pSecFuncTable);

BOOL SendMsg(SOCKET s, PBYTE pBuf, DWORD cbBuf);
BOOL ReceiveMsg(SOCKET s,PBYTE pBuf,DWORD cbBuf,DWORD *pcbRead);
BOOL SendBytes(SOCKET s, PBYTE pBuf, DWORD cbBuf);
BOOL ReceiveBytes(SOCKET s, PBYTE pBuf, DWORD cbBuf, DWORD *pcbRead);
DWORD inet_addr_w(const WCHAR *pszAddr);

void PrintHexDump(DWORD length, PBYTE buffer);

#endif // __CLIENT_SERVER_H__
