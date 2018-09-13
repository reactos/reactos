/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioep.h

Abstract:

    This module contains private structures and definitions.

Author:
    Michael Tsang (MikeTs) 1-Sep-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _IOEP_H
#define _IOEP_H

//
// Constants
//
#define IOETAG_ERRLOG           'LeoI'
#define IOETAG_ERRTHREAD        'TeoI'
#define IOETAG_ERRENTRY         'EeoI'
#define IOETAG_DATABLOCK        'DeoI'
#define IOETAG_ERRMODULE        'MeoI'
#define IOETAG_MSGBUFF          'BeoI'
#define IOETAG_ERRCASEDB        'CeoI'
#define IOETAG_DATATEXT         'XeoI'
#define IOETAG_DATAWSTR         'SeoI'
#define IOETAG_ERRINFO          'IeoI'
#define IOETAG_SAVEDATA         'VeoI'
#define IOETAG_WMIEVENT         'WeoI'

#define THREADKEY_IRP           1
#define THREADKEY_THREADID      2

#define IOEMETHOD_ANY           0
#define IOEMETHOD_LONGMSG       1
#define IOEMETHOD_SHORTMSG      2
#define IOEMETHOD_HANDLER       3

#define IOE_ERRINFO_VERSION     1
#define IOE_INFOBLK_VERSION     1

//
// Macros
//
#if DBG
  #define PROCNAME(s)   static PSZ ProcName = s
  #define DBGPRINT(p)   {                                               \
                            KdPrint(("%s: ", ProcName));                \
                            KdPrint(p);                                 \
                        }
#else
  #define PROCNAME(s)
  #define DBGPRINT(p)
#endif

//
// Type Definitions
//
typedef struct _errentry {
    SINGLE_LIST_ENTRY slist;            //error log stack link
    ERRID  ErrID;                       //globally unique error ID
    UNICODE_STRING unicodeStr;          //unicode string data
    ULONG  DataBlkType;                 //type of associated data
    ULONG  DataBlkLen;                  //length of associated data
    PVOID  DataBlk;                     //points to associated data
    GUID   MofGuid;                     //MOF Guid to describe data
} ERRENTRY, *PERRENTRY;

typedef struct _errthread {
    LIST_ENTRY list;                    //error thread chain
    LIST_ENTRY ErrLogListHead;          //points to error log nesting chain
    ULONG   ThreadKeyType;              //key type of error thread
    union {
        struct {
            PIRP  Irp;
            UCHAR MajorFunction;
            UCHAR MinorFunction;
            PVOID Arguments[4];
            PDEVICE_OBJECT TargetDevice;
        } IrpKey;

        struct {
            PKTHREAD ThreadID;
        } ThIDKey;

    } ThreadKey;
} ERRTHREAD, *PERRTHREAD;

typedef struct _errlog {
    ULONG             Signature;        //error log structure signature
    ULONG             ulFlags;          //log session flags
    LIST_ENTRY        list;             //error log nesting link
    SINGLE_LIST_ENTRY ErrStack;         //points to the error stack
    PERRTHREAD        ErrThread;        //points to associated error thread
    PERRINFO          ErrInfo;          //points to error info.
} ERRLOG, *PERRLOG;

#define SIG_ERRLOG              'GOLE'  //error log signature "ELOG"
#define LOGF_INITMASK           0x00000000      //init flags mask

typedef struct _savedata {
    ULONG      Signature;               //savedata signature
    LIST_ENTRY list;                    //chain savedata list
    PERRINFO   ErrInfo;                 //points to error info.
} SAVEDATA, *PSAVEDATA;

#define SIG_SAVEDATA            'TADE'  //saved error data signature "EDAT"

typedef struct _errmodule {
    LIST_ENTRY  list;                   //error modules chain
    GUID        ComponentGuid;          //GUID of handler module
    ULONG       NumErrHandlers;         //number of handlers in the module
    PERRHANDLER HandlerTable[1];        //error handler table
} ERRMODULE, *PERRMODULE;

//
// Private Function Prototypes
//
#define IoepGetErrStack(ErrLog)                         \
            CONTAINING_RECORD((ErrLog)->ErrStack.Next, ERRENTRY, slist)

#define IoepGetNextErrEntry(ErrEntry)                   \
            (((ErrEntry)->slist.Next != NULL)?          \
                CONTAINING_RECORD((ErrEntry)->slist.Next, ERRENTRY, slist): \
                NULL)

HANDLE
IoepInitErrLog(
    IN ULONG KeyType,
    IN PVOID Key,
    IN ULONG ulFlags
    );

PERRTHREAD
IoepFindErrThread(
    IN ULONG KeyType,
    IN PVOID Key
    );

PERRTHREAD
IoepNewErrThread(
    IN ULONG KeyType,
    IN PVOID Key
    );

VOID
IoepLogErr(
    IN ULONG       KeyType,
    IN PVOID       Key,
    IN CONST GUID *ComponentGuid,
    IN ULONG       ErrCode,
    IN PWSTR       TextData OPTIONAL,
    IN ULONG       DataBlkType,
    IN ULONG       DataBlkLen OPTIONAL,
    IN PVOID       DataBlock OPTIONAL,
    IN CONST GUID *MofGuid OPTIONAL
    );

VOID
IoepFreeErrStack(
    IN PERRENTRY ErrStack
    );

PERRMODULE
IoepFindErrModule(
    IN CONST GUID *ComponentGuid
    );

NTSTATUS
IoepExtractErrData(
    IN PERRENTRY ErrStack,
    OUT PVOID    Buffer,
    IN  ULONG    BuffSize,
    OUT PULONG   DataSize OPTIONAL
    );

NTSTATUS
IoepFireWMIEvent(
    IN PERRINFO ErrInfo,
    IN PWSTR    InstanceName
    );

NTSTATUS
IoepHandleErrCase(
    IN PERRINFO ErrInfo,
    IN PERRCASE ErrCase,
    IN ULONG    Method,
    OUT PUNICODE_STRING unicodeMsg OPTIONAL
    );

PERRHANDLER
IoepFindErrHandler(
    IN CONST GUID *ComponentGuid,
    IN ULONG       HandlerIndex
    );

BOOLEAN
IoepMatchErrIDPath(
    IN PERRINFO ErrInfo,
    IN PERRID   ErrIDPath,
    IN ULONG    NumErrIDs
    );

NTSTATUS
IoepGetErrMessage(
    IN  PMSGDATA        MsgData,
    IN  PERRINFO        ErrInfo,
    OUT PUNICODE_STRING unicodeMsg
    );

NTSTATUS
IoepCatMsgArg(
    IN OUT PUNICODE_STRING unicodeMsg,
    IN PMSGARG MsgArg,
    IN PERRINFO ErrInfo
    );

NTSTATUS
IoepUnicodeStringCatN(
    IN OUT PUNICODE_STRING unicodeStr,
    IN PWSTR pwstr,
    IN ULONG len
    );

PERRCASEDB
IoepGetErrCaseDB(
    VOID
    );

//
// External prototypes
//
NTSTATUS
IopGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    );

#endif  //ifndef _IOEP_H
