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


#define fatal_error_0(fmt, ...) \
do { \
    printf("(%s:%d) fatal error: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    return; /* exit(0); */ \
} while (0)

#define fatal_error(fmt, ...) \
do { \
    printf("(%s:%d) fatal error: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    return 0; /* exit(0); */ \
} while (0)

#define err(fmt, ...) printf("(%s:%d) " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

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
    err("%S",buffer );      \
    LocalFree(buffer);      \
} while (0)


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
