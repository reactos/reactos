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
#include "ketypes.h"

/* EXPORTED DATA *************************************************************/
extern POBJECT_TYPE NTOSAPI ExIoCompletionType;
extern NTOSAPI POBJECT_TYPE ExMutantObjectType;
extern NTOSAPI POBJECT_TYPE ExTimerType;

/* CONSTANTS *****************************************************************/

#define INVALID_HANDLE_VALUE (HANDLE)-1

/* Callback Object Access Rights */
#define CALLBACK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x0001)
#define CALLBACK_EXECUTE    (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE|0x0001)
#define CALLBACK_WRITE      (STANDARD_RIGHTS_WRITE|SYNCHRONIZE|0x0001)
#define CALLBACK_READ       (STANDARD_RIGHTS_READ|SYNCHRONIZE|0x0001)

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

/* You'll need the IFS for this, so use an equivalent version */
#ifndef _NTIFS_
typedef PVOID EX_RUNDOWN_REF;
#endif

/* You'll need the IFS for these, so let's not force everyone to have it */
#ifdef _NTIFS_
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
#endif

typedef struct _EX_FAST_REF
{
    union
    {
        PVOID Object;
        ULONG RefCnt:3;
        ULONG Value;
    };
} EX_FAST_REF, *PEX_FAST_REF;

typedef struct _EX_PUSH_LOCK
{
    union
    {
        struct
        {
            ULONG Waiting:1;
            ULONG Exclusive:1;
            ULONG Shared:30;
        };
        ULONG Value;
        PVOID Ptr;
    };
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

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

