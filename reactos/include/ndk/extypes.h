/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/extypes.h
 * PURPOSE:         Definitions for exported Executive Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#ifndef _EXTYPES_H
#define _EXTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/
extern POBJECT_TYPE NTOSAPI ExIoCompletionType;
extern NTOSAPI POBJECT_TYPE ExMutantObjectType;
extern NTOSAPI POBJECT_TYPE ExTimerType;

/* CONSTANTS *****************************************************************/

/* ENUMERATIONS **************************************************************/

typedef enum _HARDERROR_RESPONSE_OPTION 
{
    OptionAbortRetryIgnore,
    OptionOk,
    OptionOkCancel,
    OptionRetryCancel,
    OptionYesNo,
    OptionYesNoCancel,
    OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE 
{
    ResponseReturnToCaller,
    ResponseNotHandled,
    ResponseAbort,
    ResponseCancel,
    ResponseIgnore,
    ResponseNo,
    ResponseOk,
    ResponseRetry,
    ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

/* TYPES *********************************************************************/

typedef struct _EX_QUEUE_WORKER_INFO 
{
    UCHAR QueueDisabled:1;
    UCHAR MakeThreadsAsNecessary:1;
    UCHAR WaitMode:1;
    ULONG WorkerCount:29;
} EX_QUEUE_WORKER_INFO, *PEX_QUEUE_WORKER_INFO;

typedef struct _EX_WORK_QUEUE 
{
    KQUEUE WorkerQueue;
    ULONG DynamicThreadCount;
    ULONG WorkItemsProcessed;
    ULONG WorkItemsProcessedLastPass;
    ULONG QueueDepthLastPass;
    EX_QUEUE_WORKER_INFO Info;    
} EX_WORK_QUEUE, *PEX_WORK_QUEUE;

typedef struct _HANDLE_TABLE_ENTRY_INFO 
{
    ULONG AuditMask;
} HANDLE_TABLE_ENTRY_INFO, *PHANDLE_TABLE_ENTRY_INFO;

typedef struct _HANDLE_TABLE_ENTRY 
{
    union 
    {
        PVOID Object;
        ULONG_PTR ObAttributes;
        PHANDLE_TABLE_ENTRY_INFO InfoTable;
        ULONG_PTR Value;
    } u1;
    union 
    {
        ULONG GrantedAccess;
        USHORT GrantedAccessIndex;
        LONG NextFreeTableEntry;
    } u2;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE
{
    ULONG Flags;
    LONG HandleCount;
    PHANDLE_TABLE_ENTRY **Table;
    PEPROCESS QuotaProcess;
    HANDLE UniqueProcessId;
    LONG FirstFreeTableEntry;
    LONG NextIndexNeedingPool;
    ERESOURCE HandleTableLock;
    LIST_ENTRY HandleTableList;
    KEVENT HandleContentionEvent;
} HANDLE_TABLE;

#endif

