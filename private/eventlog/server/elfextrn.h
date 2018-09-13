/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    elfextrn.h

Abstract:

    This file contains all the externs for the global variables.

Author:

    Rajen Shah (rajens) 10-Jul-1991

Revision History:

--*/

#include <clussprt.h>

//
// DEFINITIONS
//

#define   EVENTLOG_SVC_NAMEW   L"EVENTLOG"


#if DBG

extern      DWORD  ElfDebug;

#endif  // DBG


extern      HANDLE ElfConnectionPortHandle;
extern      HANDLE ElfCommunicationPortHandle;

extern      PWSTR Computername;

extern      LIST_ENTRY  LogFilesHead;       // Log files
extern      LIST_ENTRY  LogModuleHead;      // Modules registered for logging
extern      LIST_ENTRY  LogHandleListHead;  // Context-handles for log handles
extern      LIST_ENTRY  QueuedEventListHead; // Deferred events
extern      LIST_ENTRY  QueuedMessageListHead; //Deferred Messagebox

extern      RTL_CRITICAL_SECTION    LogFileCritSec;
extern      RTL_CRITICAL_SECTION    LogModuleCritSec;
extern      RTL_CRITICAL_SECTION    LogHandleCritSec;
extern      RTL_CRITICAL_SECTION    QueuedEventCritSec;
extern      RTL_CRITICAL_SECTION    QueuedMessageCritSec;

extern      SERVICE_STATUS ElfServiceStatus;
extern      SERVICE_STATUS_HANDLE ElfServiceStatusHandle;

extern      RTL_RESOURCE        GlobalElfResource;

extern      PSID        AnonymousLogonSid;

extern      PVOID       ElfBackupPointer;
extern      HANDLE      ElfBackupEvent;

extern      HANDLE      LPCThreadHandle;

extern      HANDLE      MBThreadHandle;

extern      HANDLE      RegistryThreadHandle;
extern      DWORD       RegistryThreadId;

extern      ULONG       EventFlags;

extern      ELF_EOF_RECORD  EOFRecord;

extern      PLOGMODULE ElfDefaultLogModule;

extern      PLOGMODULE ElfModule;

extern      HANDLE      hEventLogNode;

extern      DWORD       BackupModuleNumber;

extern      PSVCS_GLOBAL_DATA   ElfGlobalData;    // WellKnownSids


extern      LPWSTR LocalComputerName;
extern      ULONG ComputerNameLength;

extern      BOOL EventlogShutdown;

extern      HANDLE ElfGlobalSvcRefHandle;

extern      WCHAR   DefaultMessageBoxTitle[];
extern      LPWSTR  GlobalAllocatedMsgTitle;
extern      LPWSTR  GlobalMessageBoxTitle;

extern      HANDLE  g_hTimestampEvent;

//changes to support clustering
extern      BOOL                    gbClustering; 
extern      PPACKEDEVENTINFO        gpClPackedEventInfo;
extern      RTL_CRITICAL_SECTION    gClPropCritSec;     
extern      HMODULE                 ghClusDll;
extern      PROPAGATEEVENTSPROC     gpfnPropagateEvents;
extern      BINDTOCLUSTERPROC       gpfnBindToCluster;
extern      UNBINDFROMCLUSTERPROC   gpfnUnbindFromCluster;
extern      HANDLE                  ghCluster;