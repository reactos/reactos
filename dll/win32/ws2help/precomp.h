/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 Helper DLL
 * FILE:        lib/ws2help/precomp.h
 * PURPOSE:     WinSock 2 Helper DLL
 */

#ifndef __PRECOMP_H
#define __PRECOMP_H

/* Winsock Provider Headers */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define _WIN32_WINNT 0x502
#define NTOS_MODE_USER
#define INCL_WINSOCK_API_TYPEDEFS 1

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <ws2spi.h>

/* NDK Headers */
#include <ndk/rtlfuncs.h>

/* Missing definition */
#define SO_OPENTYPE 0x20

/* Global data */
extern HANDLE GlobalHeap;
extern PSECURITY_DESCRIPTOR pSDPipe;
extern HANDLE ghWriterEvent;
extern BOOL Ws2helpInitialized;
extern DWORD gdwSpinCount;
extern DWORD gHandleToIndexMask;

/* Functions */
DWORD
WINAPI
Ws2helpInitialize(VOID);

/* Initialization macro */
#define WS2HELP_PROLOG() \
    (Ws2helpInitialized? ERROR_SUCCESS : Ws2helpInitialize())

#endif /* __PRECOMP_H */
