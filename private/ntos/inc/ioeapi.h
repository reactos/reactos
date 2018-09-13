/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioeapi.h

Abstract:

    This module contains the public structure and constant definitions.

Author:
    Michael Tsang (MikeTs) 1-Sep-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _IOEAPI_H
#define _IOEAPI_H

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis begin_ntminiport begin_srv

//
// Constants
//
#define STATUS_IOE_MESSAGE      0x11111111      //BUGBUG: shouldn't be here
#define STATUS_IOE_MODULE_ALREADY_REGISTERED 0x22222222
#define STATUS_IOE_DATABASE_NOT_READY 0x44444444

#define MAX_MSG_LEN             1023

//
// DataBlkType values in IoErrLogErrBy*
//
#define IOEDATA_NONE            0x00000000
#define IOEDATA_PRIVATE         0x00000001
#define IOEDATA_WSTRING         0x00000002
#define IOEDATA_MAX             IOEDATA_WSTRING
#define IOEDATA_TEXT            0x80000000

//
// ErrLog ErrLogKey flags
//
#define IOEDATATAG_TYPE_MASK    0x000000ff
#define IOEDATATAG_TYPE_DEVNODE 0x00000001
#define IOEDATATAG_BITS         IOEDATATAG_TYPE_MASK

//
// Type Definitions
//
typedef struct _errid {
    GUID  ComponentGuid;
    ULONG ErrCode;
} ERRID, *PERRID;

typedef struct _errdata {
    ERRID  ErrID;                       //globally unique error ID
    ULONG  TextDataOffset;              //offset to unicode string data
    ULONG  DataBlkType;                 //type of associated data
    ULONG  DataBlkLen;                  //length of associated data
    ULONG  DataBlkOffset;               //points to associated data
    GUID   MofGuid;                     //MOF Guid to describe data
} ERRDATA, *PERRDATA;

typedef struct _errinfo {
    ULONG   Signature;                  //signature of the structure
    ULONG   Version;                    //version of the structure
    ULONG   Size;                       //size of the structure
    PVOID   DataTag;                    //tag associated with the error data
    ULONG   TagFlags;                   //tag flags
    ULONG   NumErrEntries;              //number of error entries
    ERRDATA ErrEntries[1];              //array of error data
} ERRINFO, *PERRINFO;

#define SIG_ERRINFO             'IRRE'  //signature of ERRINFO "ERRI"

typedef struct _infoblk {
    ULONG   Signature;                  //signature of the structure
    ULONG   Version;                    //version of the structure
    ULONG   Size;                       //size of the structure
    ULONG   NumErrInfos;                //number of error info blocks
    ERRINFO ErrInfos[1];                //array of error info.
} INFOBLK, *PINFOBLK;

#define SIG_INFOBLK             'KLBI'  //signature of INFOBLK "IBLK"

typedef NTSTATUS (*PERRHANDLER)(PVOID, ULONG);

//
// Exported Error Logging APIs
//
HANDLE
IoErrInitErrLogByIrp(
    IN PIRP  Irp,
    IN ULONG ulFlags
    );

HANDLE
IoErrInitErrLogByThreadID(
    IN PKTHREAD ThreadID,
    IN ULONG    ulFlags
    );

VOID
IoErrLogErrByIrp(
    IN PIRP        Irp,
    IN CONST GUID *ComponentGuid,
    IN ULONG       ErrCode,
    IN PWSTR       TextData OPTIONAL,
    IN ULONG       DataBlkType,
    IN ULONG       DataBlkLen OPTIONAL,
    IN PVOID       DataBlock OPTIONAL,
    IN CONST GUID *MofGuid OPTIONAL
    );

VOID
IoErrLogErrByThreadID(
    IN PKTHREAD    ThreadID,
    IN CONST GUID *ComponentGuid,
    IN ULONG       ErrCode,
    IN PWSTR       TextData OPTIONAL,
    IN ULONG       DataBlkType,
    IN ULONG       DataBlkLen OPTIONAL,
    IN PVOID       DataBlock OPTIONAL,
    IN CONST GUID *MofGuid OPTIONAL
    );

VOID
IoErrPropagateErrLog(
    IN HANDLE ErrLogHandle
    );

VOID
IoErrTerminateErrLog(
    IN HANDLE ErrLogHandle
    );

//
// Exported Error Handling APIs
//
NTSTATUS
IoErrRegisterErrHandlers(
    IN CONST GUID  *ModuleGuid,
    IN ULONG        NumErrHandlers,
    IN PERRHANDLER *HandlerTable
    );

PERRINFO
IoErrGetErrData(
    IN HANDLE ErrLogHandle
    );

HANDLE
IoErrSaveErrData(
    IN HANDLE ErrLogHandle,
    IN PVOID  DataTag OPTIONAL,
    IN ULONG  TagFlags OPTIONAL
    );

PERRINFO
IoErrGetSavedData(
    IN HANDLE SaveDataHandle
    );

VOID
IoErrFreeSavedData(
    IN HANDLE SaveDataHandle
    );

NTSTATUS
IoErrMatchErrCase(
    IN  PERRINFO ErrInfo,
    OUT PULONG   ErrCaseID,
    OUT PHANDLE  ErrCaseHandle OPTIONAL
    );

NTSTATUS
IoErrFindErrCaseByID(
    IN  ULONG   ErrCaseID,
    OUT PHANDLE ErrCaseHandle
    );

NTSTATUS
IoErrHandleErrCase(
    IN PERRINFO ErrInfo,
    IN HANDLE   ErrCaseHandle
    );

NTSTATUS
IoErrGetLongErrMessage(
    IN  PERRINFO ErrInfo,
    IN  HANDLE   ErrCaseHandle,
    OUT PUNICODE_STRING unicodeMsg
    );

NTSTATUS
IoErrGetShortErrMessage(
    IN  PERRINFO ErrInfo,
    IN  HANDLE   ErrCaseHandle,
    OUT PUNICODE_STRING unicodeMsg
    );

// end_ntddk end_wdm end_nthal end_ntifs end_ntndis end_ntminiport end_ntsrv

BOOLEAN
IoErrInitSystem(
    VOID
    );

NTSTATUS
IoErrRetrieveSavedData(
    OUT PINFOBLK InfoBlk,
    IN  ULONG    BuffSize,
    OUT PULONG   DataSize OPTIONAL,
    IN  PVOID    DataTag OPTIONAL,
    IN  ULONG    TagFlags OPTIONAL
    );

#endif  //ifndef _IOEAPI_H
