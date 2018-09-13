/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    csrdll.h

Abstract:

    Main include file for Client side of the Client Server Runtime (CSR)

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

//
// Include definitions common between the Client and Server portions.
//

#include "csr.h"

//
// Include definitions specific to the Client portion.
//

#include "ntcsrdll.h"
#include "ntcsrsrv.h"

#if DBG
#define CSR_DEBUG_INIT              0x00000001
#define CSR_DEBUG_LPC               0x00000002
#define CSR_DEBUG_FLAG3             0x00000004
#define CSR_DEBUG_FLAG4             0x00000008
#define CSR_DEBUG_FLAG5             0x00000010
#define CSR_DEBUG_FLAG6             0x00000020
#define CSR_DEBUG_FLAG7             0x00000040
#define CSR_DEBUG_FLAG8             0x00000080
#define CSR_DEBUG_FLAG9             0x00000100
#define CSR_DEBUG_FLAG10            0x00000200
#define CSR_DEBUG_FLAG11            0x00000400
#define CSR_DEBUG_FLAG12            0x00000800
#define CSR_DEBUG_FLAG13            0x00001000
#define CSR_DEBUG_FLAG14            0x00002000
#define CSR_DEBUG_FLAG15            0x00004000
#define CSR_DEBUG_FLAG16            0x00008000
#define CSR_DEBUG_FLAG17            0x00010000
#define CSR_DEBUG_FLAG18            0x00020000
#define CSR_DEBUG_FLAG19            0x00040000
#define CSR_DEBUG_FLAG20            0x00080000
#define CSR_DEBUG_FLAG21            0x00100000
#define CSR_DEBUG_FLAG22            0x00200000
#define CSR_DEBUG_FLAG23            0x00400000
#define CSR_DEBUG_FLAG24            0x00800000
#define CSR_DEBUG_FLAG25            0x01000000
#define CSR_DEBUG_FLAG26            0x02000000
#define CSR_DEBUG_FLAG27            0x04000000
#define CSR_DEBUG_FLAG28            0x08000000
#define CSR_DEBUG_FLAG29            0x10000000
#define CSR_DEBUG_FLAG30            0x20000000
#define CSR_DEBUG_FLAG31            0x40000000
#define CSR_DEBUG_FLAG32            0x80000000

ULONG CsrDebug;
#define IF_CSR_DEBUG( ComponentFlag ) \
    if (CsrDebug & (CSR_DEBUG_ ## ComponentFlag))

#else
#define IF_CSR_DEBUG( ComponentFlag ) if (FALSE)
#endif

//
// Common Types and Definitions
//

//
// CSR_HEAP_MEMORY_SIZE defines how much address space should be
// reserved for the Client heap.  This heap is used to store all
// data structures maintained by the Client DLL.
//

#define CSR_HEAP_MEMORY_SIZE (64*1024)


//
// CSR_PORT_MEMORY_SIZE defines how much address space should be
// reserved for passing data to the Server.  The memory is visible
// to both the client and server processes.
//

#define CSR_PORT_MEMORY_SIZE 0x10000

//
// Global data accessed by Client DLL
//

BOOLEAN CsrInitOnceDone;

//
// This boolean is TRUE if the dll is attached to a server process.
//

BOOLEAN CsrServerProcess;

//
// This points to the server routine that dispatches APIs, if the dll is
// being called by a server process.
//

NTSTATUS (*CsrServerApiRoutine)(PCSR_API_MSG,PCSR_API_MSG);

//
// The CsrNtSysInfo global variable contains NT specific constants of
// interest, such as page size, allocation granularity, etc.  It is filled
// in once during process initialization.
//

SYSTEM_BASIC_INFORMATION CsrNtSysInfo;

#define ROUND_UP_TO_PAGES(SIZE) (((ULONG)(SIZE) + CsrNtSysInfo.PageSize - 1) & ~(CsrNtSysInfo.PageSize - 1))
#define ROUND_DOWN_TO_PAGES(SIZE) (((ULONG)(SIZE)) & ~(CsrNtSysInfo.PageSize - 1))

//
// The CsrDebugFlag is non-zero if the Client Application was
// invoked with the Debug option.
//

ULONG CsrDebugFlag;

//
// The CsrHeap global variable describes a single heap used by the Client
// DLL for process wide storage management.  Process private data maintained
// by the Client DLL is allocated out of this heap.
//

PVOID CsrHeap;


//
// The connection to the Server is described by the CsrPortHandle global
// variable.  The connection is established when the CsrConnectToServer
// function is called.
//

UNICODE_STRING CsrPortName;
HANDLE CsrPortHandle;


//
// In order to pass large arguments to the Server (e.g. path name
// arguments) the CsrPortHeap global variable describes a heap that
// is visible to both the Windows Client process and the Server
// process.
//

PVOID CsrPortHeap;
ULONG_PTR CsrPortMemoryRemoteDelta;

ULONG CsrPortBaseTag;

#define MAKE_CSRPORT_TAG( t ) (RTL_HEAP_MAKE_TAG( CsrPortBaseTag, t ))

#define CAPTURE_TAG 0

//
// The CsrDllHandle global variable contains the DLL handle for the WINDLL
// client stubs executable.
//

HANDLE CsrDllHandle;


//
// The CsrObjectDirecotory global variable contains the handle to the
// object directory that is the name of the server.
//

HANDLE CsrObjectDirectory;


//
// The CsrCallbackInfo structure has the information passed to
// CsrClientConnectToServer if it was present, otherwise it is initialized
// to all zeros.
//

#define CSR_MAX_CLIENT_DLL 16

PCSR_CALLBACK_INFO CsrLoadedClientDll[ CSR_MAX_CLIENT_DLL ];

//
// Routines defined in dllinit.c
//

BOOLEAN
CsrDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );

NTSTATUS
CsrpConnectToServer(
    IN PWSTR ObjectDirectory
    );


//
// Routines defined in dllutil.c
//
