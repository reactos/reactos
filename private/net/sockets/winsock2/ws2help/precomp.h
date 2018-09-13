/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file for the WinSock 2.0 helper DLL.

Author:

    Keith Moore (keithmo)       08-Nov-1995

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdio.h>
#include <malloc.h>

#include <windows.h>
#include <tchar.h>
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <winsock2.h>
#include <ws2spi.h>
#include <ws2help.h>


#include <tdi.h>
#include <afd.h>

#if DBG
#define DBG_FAILURES        0x80000000
#define DBG_PROCESS         0x40000000
#define DBG_SOCKET          0x20000000
#define DBG_COMPLETE        0x10000000
#define DBG_APC_THREAD      0x08000000
#define DBG_WINSOCK_APC     0x04000000
#define DBG_REQUEST			0x02000000
#define DBG_CANCEL          0x01000000
#define DBG_DRIVER_READ		0x00800000
#define DBG_DRIVER_WRITE	0x00400000
#define DBG_DRIVER_SEND		0x00200000
#define DBG_DRIVER_RECV		0x00100000
#define DBG_SERVICE         0x00080000
#define DBG_NOTIFY          0x00040000

#define WS2IFSL_DEBUG_KEY   "System\\CurrentControlSet\\Services\\Ws2IFSL\\Parameters\\ProcessDebug"
extern DWORD       PID;
extern ULONG       DbgLevel;
extern VOID        ReadDbgInfo (VOID);

#define WshPrint(COMPONENT,ARGS)	\
	do {						    \
		if (DbgLevel&COMPONENT){    \
			DbgPrint ARGS;		    \
		}						    \
	} while (0)
//
// Define an assert that actually works even on a free build.
//

#define WS_ASSERT(exp)                                              \
        ((exp)                                                      \
            ? 0                                                     \
            : (DbgPrint( "\n*** Assertion failed: %s\n"             \
                        "***   Source File: %s, line %ld\n\n",      \
                    #exp,__FILE__, __LINE__), DbgBreakPoint()))     \



#else
#define WshPrint(COMPONENT,ARGS) do {NOTHING;} while (0)
#define WS_ASSERT(exp)
#endif

#undef ASSERT
#define ASSERT WS_ASSERT

extern HINSTANCE            LibraryHdl;
extern PSECURITY_DESCRIPTOR pSDPipe;
extern CRITICAL_SECTION     StartupSyncronization;
extern BOOL                 Ws2helpInitialized;
extern HANDLE               ghWriterEvent;
DWORD
Ws2helpInitialize (
    VOID
    );

#define ENTER_WS2HELP_API()                                         \
    (Ws2helpInitialized ? 0 : Ws2helpInitialize())

VOID
NewCtxInit (
    VOID
    );

#ifdef NT351

#define ALLOC_MEM(cb)       RtlAllocateHeap(                        \
                                RtlProcessHeap(),                   \
                                0,                                  \
                                (cb)                                \
                                )

#define REALLOC_MEM(p,cb)   RtlReAllocateHeap(                      \
                                RtlProcessHeap(),                   \
                                0,                                  \
                                (p),                                \
                                (cb)                                \
                                )

#define FREE_MEM(p)         RtlFreeHeap(                            \
                                RtlProcessHeap(),                   \
                                0,                                  \
                                (p)                                 \
                                )

#else   // !NT351

#include <ws2ifsl.h>

#define ALLOC_MEM(cb)       (LPVOID)GlobalAlloc(                    \
                                GPTR,                               \
                                (cb)                                \
                                )

#define REALLOC_MEM(p,cb)   (LPVOID)GlobalReAlloc(                  \
                                (HGLOBAL)(p),                       \
                                (cb),                               \
                                (GMEM_MOVEABLE | GMEM_ZEROINIT)     \
                                )

#define FREE_MEM(p)         (VOID)GlobalFree(                       \
                                (HGLOBAL)(p)                        \
                                )

#endif  // NT351


#endif  // _PRECOMP_H_

