/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    DATA.C

Abstract:

    This file contains all the global data elements of the eventlog service.

Author:

    Rajen Shah  (rajens)    10-Jul-1991

[Environment:]

    User Mode - Win32, except for NTSTATUS returned by some functions.

Revision History:

    10-Jul-1991     RajenS
        created

--*/

//
// INCLUDES
//

#include <eventp.h>
#include <elfcfg.h>


#if DBG

//
// Debug flag used to control ElfDbgPrint
//
DWORD ElfDebug = 0;

#endif  // DBG


//
// Handles used for the LPC port.
//
HANDLE ElfConnectionPortHandle;
HANDLE ElfCommunicationPortHandle;

// The heads of various linked lists
//
LIST_ENTRY      LogFilesHead;               // Log files

RTL_CRITICAL_SECTION    LogFileCritSec;     // Accessing log files

LIST_ENTRY      LogModuleHead;              // Modules registered for logging

RTL_CRITICAL_SECTION    LogModuleCritSec;   // Accessing log files

LIST_ENTRY      LogHandleListHead;          // Context-handles for log handles

RTL_CRITICAL_SECTION    LogHandleCritSec;   // Accessing log handles

LIST_ENTRY      QueuedEventListHead;        // Deferred events to write

RTL_CRITICAL_SECTION QueuedEventCritSec;    // Accessing the deferred events

LIST_ENTRY      QueuedMessageListHead;      // Deferred messagebox

RTL_CRITICAL_SECTION QueuedMessageCritSec;  // Accessing the deferred mb's

//
// Service-related global data
//

SERVICE_STATUS_HANDLE ElfServiceStatusHandle;

//
// The following resource is used to serialize access to the resources
// of the Eventlog service at the highest level. It is used to make sure
// that the threads that write/read/clear the log file(s) do not step over
// the threads that monitor the registry and deal with service control
// operations.
//
// The threads that operate on the log file(s) have Shared access to the
// resource, since they are further serialized on the file that they are
// working on.
//
// The threads that will modify the internal data structures, or the state
// of the service, need Exclusive access to the resource so that we can
// control access to the data structures and log files.
//

RTL_RESOURCE        GlobalElfResource;

//
// This is used by the Backup API to signify which 4K block of the log it's
// currently reading.  This is used to prevent a writer from overwriting this
// block while it is reading it.  The event is used to let a writer block if
// it was going to overwrite the current backup block, and get pulsed when
// the backup thread moves to the next block.

PVOID               ElfBackupPointer;
HANDLE              ElfBackupEvent;

//
// Handle for the LPC thread
//
HANDLE      LPCThreadHandle;

//
// Handle for the MessageBox thread
//
HANDLE      MBThreadHandle;

//
// Handle and ID for the registry monitor thread
//
HANDLE      RegistryThreadHandle;
DWORD       RegistryThreadId;

//
// Bitmask of things that have been allocated and/or started by the
// service. When the service terminates, this is what needs to be
// cleaned.
//
ULONG       EventFlags;     // Keep track of what is allocated

//
// Record used to indicate the end of the event records in the file.
//
ELF_EOF_RECORD  EOFRecord = {  ELFEOFRECORDSIZE,
                               0x11111111,
                               0x22222222,
                               0x33333333,
                               0x44444444,
                               FILEHEADERBUFSIZE,
                               FILEHEADERBUFSIZE,
                               1,
                               1,
                               ELFEOFRECORDSIZE
                            };

//
// Default module to use if no match is found, APPLICATION
//

PLOGMODULE ElfDefaultLogModule;

//
// Module for the eventlog service itself
//

PLOGMODULE ElfModule;

//
// Handle (key) to the event log node in the registry.
// This is set up by the service main function.
//

HANDLE      hEventLogNode;

//
// Used to create a unigue module name for backup logs
//

DWORD BackupModuleNumber;

//
// NT well-known SIDs
//
PSVCS_GLOBAL_DATA       ElfGlobalData;

//
// Global anonymous logon sid - used in log ACL's. The only SID allocated
// specifically by the eventlog service, all others are passed in from
// the service controller in ElfGlobalData.
//

PSID AnonymousLogonSid;

//
// The local computer name.  Used when we generate events ourself.
//

LPWSTR LocalComputerName;
ULONG  ComputerNameLength;

//
// Shutdown Flag
//
BOOL    EventlogShutdown;

HANDLE  ElfGlobalSvcRefHandle;

//
// This is the string used in the title bar of the Message Box
// used to display log full messages.
// GlobalMessageBoxTitle will either point to the default string, or
// to the string allocated in the format Message function.
//
WCHAR   DefaultMessageBoxTitle[] = L"Eventlog Service";
LPWSTR  GlobalAllocatedMsgTitle;
LPWSTR  GlobalMessageBoxTitle    = DefaultMessageBoxTitle;


//SS:start of changes for clustering
BOOL                    gbClustering=FALSE; //the cluster service has registered for replication of events
PPACKEDEVENTINFO        gpClPackedEventInfo=NULL; //pointer to preallocated memory for event propagation
RTL_CRITICAL_SECTION    gClPropCritSec;     // for using the global glClPackedEventInfo structure
HMODULE                 ghClusDll=NULL;
PROPAGATEEVENTSPROC     gpfnPropagateEvents=NULL;
BINDTOCLUSTERPROC       gpfnBindToCluster=NULL;
UNBINDFROMCLUSTERPROC   gpfnUnbindFromCluster=NULL;
HANDLE                  ghCluster=NULL;
//SS: end of changes for clustering