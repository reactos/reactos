/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        include/ws2_32.h
 * PURPOSE:     WinSock 2 DLL header
 */
#ifndef __WS2_32_H
#define __WS2_32_H

#include <ddk/ntddk.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <windows.h>
#include <debug.h>

/* Exported by ntdll.dll, but where is the prototype? */
unsigned long strtoul(const char *nptr, char **endptr, int base);


#define EXPORT STDCALL

extern HANDLE GlobalHeap;
extern WSPUPCALLTABLE UpcallTable;


typedef struct _WINSOCK_THREAD_BLOCK {
    INT LastErrorValue;     /* Error value from last function that failed */
    BOOL Initialized;       /* TRUE if WSAStartup() has been successfully called */
} WINSOCK_THREAD_BLOCK, *PWINSOCK_THREAD_BLOCK;


/* Macros */

#define WSAINITIALIZED (((PWINSOCK_THREAD_BLOCK)NtCurrentTeb()->WinSockData)->Initialized)

#define WSASETINITIALIZED (((PWINSOCK_THREAD_BLOCK)NtCurrentTeb()->WinSockData)->Initialized = TRUE)

#endif /* __WS2_32_H */

/* EOF */
