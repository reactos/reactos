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
#ifndef NTOS_MODE_USER
extern POBJECT_TYPE NTOSAPI ExIoCompletionType;
extern NTOSAPI POBJECT_TYPE ExMutantObjectType;
extern NTOSAPI POBJECT_TYPE ExTimerType;
#endif

/* CONSTANTS *****************************************************************/
#ifndef NTOS_MODE_USER
#define INVALID_HANDLE_VALUE (HANDLE)-1
#endif

/* Increments */
#define MUTANT_INCREMENT 1

/* Executive Object Access Rights */
#define CALLBACK_ALL_ACCESS     (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x0001)
#define CALLBACK_EXECUTE        (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE|0x0001)
#define CALLBACK_WRITE          (STANDARD_RIGHTS_WRITE|SYNCHRONIZE|0x0001)
#define CALLBACK_READ           (STANDARD_RIGHTS_READ|SYNCHRONIZE|0x0001)
#ifdef NTOS_MODE_USER
#define EVENT_QUERY_STATE       0x0001
#define SEMAPHORE_QUERY_STATE   0x0001
#endif

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

#ifndef NTOS_MODE_USER
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

typedef struct _EX_FAST_REF
{
    union
    {
        PVOID Object;
        ULONG RefCnt:3;
        ULONG Value;
    };
} EX_FAST_REF, *PEX_FAST_REF;

typedef struct _HANDLE_TABLE_ENTRY_INFO
{
    ULONG AuditMask;
} HANDLE_TABLE_ENTRY_INFO, *PHANDLE_TABLE_ENTRY_INFO;

typedef struct _RUNDOWN_DESCRIPTOR
{
    ULONG_PTR References;
    KEVENT RundownEvent;
} RUNDOWN_DESCRIPTOR, *PRUNDOWN_DESCRIPTOR;

typedef struct _CALLBACK_OBJECT
{
    ULONG Name;
    KSPIN_LOCK Lock;
    LIST_ENTRY RegisteredCallbacks;
    ULONG AllowMultipleCallbacks;
} CALLBACK_OBJECT , *PCALLBACK_OBJECT;

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
} HANDLE_TABLE, *PHANDLE_TABLE;

#endif
#endif

