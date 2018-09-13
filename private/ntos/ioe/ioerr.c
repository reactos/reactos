/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioerr.c

Abstract:

    This module contains the code for the worker routines

Author:

    Michael Tsang (MikeTs) 2-Sep-1998

Environment:

    Kernel mode

Revision History:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoepHandleErrCase)
#pragma alloc_text(PAGE, IoepFindErrHandler)
#pragma alloc_text(PAGE, IoepMatchErrIDPath)
#pragma alloc_text(PAGE, IoepGetErrMessage)
#pragma alloc_text(PAGE, IoepCatMsgArg)
#pragma alloc_text(PAGE, IoepUnicodeStringCatN)
#pragma alloc_text(PAGE, IoepGetErrCaseDB)
#endif

HANDLE
IoepInitErrLog(
    IN ULONG KeyType,
    IN PVOID Key,
    IN ULONG ulFlags
    )
/*++

Routine Description:
    This routine initializes an error logging session with the specified key.

Arguments:
    KeyType - specifies the key type
    Key - points to the key
    ulFlags - log session flags

Return Value:
    Success - returns the newly created error log handle.
    Failure - returns NULL.

--*/
{
    PROCNAME("IoepInitErrLog");
    PERRLOG ErrLog;

    ASSERT(Key != NULL);
    ASSERT(ulFlags & ~LOGF_INITMASK == 0);
    ENTER(2, ("(KeyType=%x,Key=%p,ulFlags=%x)\n", KeyType, Key, ulFlags));

    ErrLog = ExAllocatePoolWithTag(NonPagedPool,
                                   sizeof(ERRLOG),
                                   IOETAG_ERRLOG);

    if (ErrLog != NULL)
    {
        PERRTHREAD ErrThread;

        ErrThread = IoepFindErrThread(KeyType, Key);
        if (ErrThread == NULL)
        {
            ErrThread = IoepNewErrThread(KeyType, Key);
        }

        if (ErrThread != NULL)
        {
            ErrLog->Signature = SIG_ERRLOG;
            ErrLog->ulFlags = ulFlags & LOGF_INITMASK;
            ErrLog->ErrThread = ErrThread;
            ErrLog->ErrStack.Next = NULL;
            ErrLog->ErrInfo = NULL;
            ExInterlockedInsertHeadList(&ErrThread->ErrLogListHead,
                                        &ErrLog->list,
                                        &IoepErrListLock);
        }
        else
        {
            ExFreePool(ErrLog);
            ErrLog = NULL;
        }
    }
    else
    {
        DBGPRINT(("failed to allocate new error log\n"));
    }

    EXIT(2, ("=%p\n", ErrLog));
    return ErrLog;
}       //IoepInitErrLog

PERRTHREAD
IoepFindErrThread(
    IN ULONG KeyType,
    IN PVOID Key
    )
/*++

Routine Description:
    This routine finds the error thread keyed by the given Key.

Arguments:
    KeyType - specifies the type of key.
    Key - points to the key that is used to locate the logging session.

Return Value:
    Success - returns the error thread handle found.
    Failure - returns NULL.

--*/
{
    PROCNAME("IoepFindErrThread");
    PERRTHREAD ErrThread = NULL;
    PLIST_ENTRY list;
    KIRQL Irql;

    ENTER(2, ("(KeyType=%x,Key=%p)\n", KeyType, Key));

    ExAcquireSpinLock(&IoepErrListLock, &Irql);
    for (list = IoepErrThreadListHead.Flink;
         list != &IoepErrThreadListHead;
         list = list->Flink)
    {
        ErrThread = CONTAINING_RECORD(list, ERRTHREAD, list);
        if (ErrThread->ThreadKeyType == KeyType)
        {
            switch (KeyType)
            {
                case THREADKEY_IRP:
                    if ((PIRP)Key == ErrThread->ThreadKey.IrpKey.Irp)
                    {
                        break;
                    }
                    else
                    {
                        PIO_STACK_LOCATION IrpSp;

                        IrpSp = IoGetCurrentIrpStackLocation((PIRP)Key);
                        if ((IrpSp->MajorFunction ==
                             ErrThread->ThreadKey.IrpKey.MajorFunction) &&
                            (IrpSp->MinorFunction ==
                             ErrThread->ThreadKey.IrpKey.MinorFunction) &&
                            RtlEqualMemory(&IrpSp->Parameters.Others,
                                           ErrThread->ThreadKey.IrpKey.Arguments,
                                           sizeof(PVOID)*4) &&
                            (IopDbgGetLowestDevice(IrpSp->DeviceObject) ==
                             ErrThread->ThreadKey.IrpKey.TargetDevice))
                        {
                            break;
                        }
                    }
                    break;

                case THREADKEY_THREADID:
                    if ((PKTHREAD)Key == ErrThread->ThreadKey.ThIDKey.ThreadID)
                    {
                        break;
                    }
                    break;
            }
        }
        ErrThread = NULL;
    }
    ExReleaseSpinLock(&IoepErrListLock, Irql);

    EXIT(2, ("=%p\n", ErrThread));
    return ErrThread;
}       //IoepFindErrThread

PERRTHREAD
IoepNewErrThread(
    IN ULONG KeyType,
    IN PVOID Key
    )
/*++

Routine Description:
    This routine creates a new error thread with a given key.

Arguments:
    KeyType - specifies the type of key.
    Key - points to the key that is used to create the new logging session.

Return Value:
    Success - returns the error thread handle created.
    Failure - returns NULL.

--*/
{
    PROCNAME("IoepNewErrThread");
    PERRTHREAD ErrThread;

    ENTER(2, ("(KeyType=%x,Key=%p)\n", KeyType, Key));

    ErrThread = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(ERRTHREAD),
                                      IOETAG_ERRTHREAD);

    if (ErrThread != NULL)
    {
        PIO_STACK_LOCATION IrpSp;

        InitializeListHead(&ErrThread->ErrLogListHead);
        ErrThread->ThreadKeyType = KeyType;

        switch (KeyType)
        {
            case THREADKEY_IRP:
                IrpSp = IoGetCurrentIrpStackLocation((PIRP)Key);
                ErrThread->ThreadKey.IrpKey.Irp = (PIRP)Key;
                ErrThread->ThreadKey.IrpKey.MajorFunction =
                    IrpSp->MajorFunction;
                ErrThread->ThreadKey.IrpKey.MinorFunction =
                    IrpSp->MinorFunction;
                RtlCopyMemory(ErrThread->ThreadKey.IrpKey.Arguments,
                              &IrpSp->Parameters.Others,
                              sizeof(PVOID)*4);
                ErrThread->ThreadKey.IrpKey.TargetDevice =
                    IopDbgGetLowestDevice(IrpSp->DeviceObject);
                break;

            case THREADKEY_THREADID:
                ErrThread->ThreadKey.ThIDKey.ThreadID = (PKTHREAD)Key;
                break;
        }
        ExInterlockedInsertHeadList(&IoepErrThreadListHead,
                                    &ErrThread->list,
                                    &IoepErrListLock);
    }
    else
    {
        DBGPRINT(("failed to allocate error thread\n"));
    }

    EXIT(2, ("=%p\n", ErrThread));
    return ErrThread;
}       //IoepNewErrThread

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
    )
/*++

Routine Description:
    This routine logs the error data to the error log session identified by
    the given key.

Arguments:
    KeyType - specifies the type of key.
    Key - points to the key that is used to locate the logging session.
    ComponentGuid - points to the component GUID of the caller
    ErrCode - unique error code
    TextData - points to an optional WSTR of text data
    DataBlkType - data type of the data block
    DataBlkLen - length of the data block
    DataBlock - points to the data block
    MofGuid - points to the MOF GUID of the data block if applicable

Return Value:
    None

--*/
{
    PROCNAME("IoepLogErr");
    PERRTHREAD ErrThread;

    ASSERT(Key != NULL);
    ASSERT(ComponentGuid != NULL);
    ASSERT((DataBlkType == IOEDATA_NONE) ||
           (DataBlkLen > 0) && (DataBlock != NULL));
    ASSERT(DataBlkType <= IOEDATA_MAX);
    ENTER(2, ("(KeyType=%x,Key=%p,pGuid=%p,ErrCode=%x,Text=%S,Type=%x,Len=%d,DataBlk=%p,MofGuid=%p)\n",
              KeyType, Key, ComponentGuid, ErrCode, TextData? TextData: L"",
              DataBlkType, DataBlkLen, DataBlock, MofGuid));

    ErrThread = IoepFindErrThread(KeyType, Key);
    if (ErrThread != NULL)
    {
        PERRENTRY Err;

        ASSERT(!IsListEmpty(&ErrThread->ErrLogListHead));
        Err = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(ERRENTRY),
                                    IOETAG_ERRENTRY);

        if (Err != NULL)
        {
            BOOLEAN fOK = TRUE;
            PVOID Data = NULL;
            PWSTR pwstr = NULL;

            RtlZeroMemory(Err, sizeof(ERRENTRY));
            if (DataBlkType != IOEDATA_NONE)
            {
                Data = ExAllocatePoolWithTag(NonPagedPool,
                                             DataBlkLen,
                                             IOETAG_DATABLOCK);

                if (Data != NULL)
                {
                    RtlCopyMemory(Data, DataBlock, DataBlkLen);
                }
                else
                {
                    fOK = FALSE;
                    DBGPRINT(("failed to allocate data block (len=%d)\n",
                              DataBlkLen));
                }
            }
            else
            {
                //
                // Make sure DataLen does not have garbage.
                //
                DataBlkLen = 0;
            }

            if (TextData != NULL)
            {
                ULONG len;

                len = wcslen(TextData)*sizeof(WCHAR) + sizeof(UNICODE_NULL);
                pwstr = ExAllocatePoolWithTag(NonPagedPool,
                                              len,
                                              IOETAG_DATATEXT);

                if (pwstr != NULL)
                {
                    RtlCopyMemory(pwstr, TextData, len);
                    RtlInitUnicodeString(&Err->unicodeStr, pwstr);
                }
                else
                {
                    fOK = FALSE;
                    DBGPRINT(("failed to allocate text data (len=%d)\n",
                              len));
                }
            }

            if (fOK)
            {
                PERRLOG ErrLog;
                KIRQL Irql;

                Err->ErrID.ComponentGuid = *ComponentGuid;
                Err->ErrID.ErrCode = ErrCode;
                Err->DataBlkType = DataBlkType;
                Err->DataBlkLen = DataBlkLen;
                Err->DataBlk = Data;
                if (MofGuid != NULL)
                {
                    Err->MofGuid = *MofGuid;
                }

                ExAcquireSpinLock(&IoepErrListLock, &Irql);
                ErrLog = CONTAINING_RECORD(ErrThread->ErrLogListHead.Flink,
                                           ERRLOG,
                                           list);
                PushEntryList(&ErrLog->ErrStack, &Err->slist);
                ExReleaseSpinLock(&IoepErrListLock, Irql);
            }
            else
            {
                if (Data != NULL)
                {
                    ExFreePool(Data);
                }
                if (pwstr != NULL)
                {
                    ExFreePool(pwstr);
                }
                ExFreePool(Err);
            }
        }
        else
        {
            DBGPRINT(("failed to allocate error entry\n"));
        }
    }
    else
    {
        DBGPRINT(("failed to find associated error thread (Type=%x,Key=%p)\n",
                  KeyType, Key));
    }

    EXIT(2, ("!\n"));
}       //IoepLogErr

VOID
IoepFreeErrStack(
    IN PERRENTRY ErrStack
    )
/*++

Routine Description:
    This routine frees an error stack and the associated data blocks.

Arguments:
    ErrStack - points to the error stack to be freed

Return Value:
    None

--*/
{
    PROCNAME("IoepFreeErrStack");
    PERRENTRY Err, ErrNext;
    KIRQL Irql;

    ASSERT(ErrStack != NULL);
    ENTER(2, ("(ErrStack=%p)\n", ErrStack));

    ExAcquireSpinLock(&IoepErrListLock, &Irql);
    for (Err = ErrStack; Err != NULL; Err = ErrNext)
    {
        if (Err->DataBlk != NULL)
        {
            ExFreePool(Err->DataBlk);
            Err->DataBlk = NULL;
        }
        RtlFreeUnicodeString(&Err->unicodeStr);
        ErrNext = IoepGetNextErrEntry(Err);
        Err->slist.Next = NULL;
        ExFreePool(Err);
    }
    ExReleaseSpinLock(&IoepErrListLock, Irql);

    EXIT(2, ("!\n"));
}       //IoepFreeErrStack

PERRMODULE
IoepFindErrModule(
    IN CONST GUID *ComponentGuid
    )
/*++

Routine Description:
    This routine finds a registered error module with a given module GUID.

Arguments:
    ComponentGuid - points to the GUID of the error module

Return Value:
    Success - returns pointer to error module found
    Failure - returns NULL

--*/
{
    PROCNAME("IoepFindErrModule");
    PERRMODULE ErrModule = NULL;
    PLIST_ENTRY list;
    KIRQL Irql;

    ENTER(2, ("(pGuid=%p)\n", ComponentGuid));

    ExAcquireSpinLock(&IoepErrListLock, &Irql);
    for (list = IoepErrModuleListHead.Flink;
         list != &IoepErrModuleListHead;
         list = list->Flink)
    {
        ErrModule = CONTAINING_RECORD(list, ERRMODULE, list);
        if (RtlEqualMemory(&ErrModule->ComponentGuid,
                           ComponentGuid,
                           sizeof(GUID)))
        {
            break;
        }
        ErrModule = NULL;
    }
    ExReleaseSpinLock(&IoepErrListLock, Irql);

    EXIT(2, ("=%p\n", ErrModule));
    return ErrModule;
}       //IoepFindErrModule

NTSTATUS
IoepExtractErrData(
    IN PERRENTRY ErrStack,
    OUT PVOID    Buffer,
    IN  ULONG    BuffSize,
    OUT PULONG   DataSize OPTIONAL
    )
/*++

Routine Description:
    This routine stores the error stack data into a flat buffer.

Arguments:
    ErrStack - points to the error stack
    Buffer - points to the buffer for the data
    BuffSize - specifies the buffer size
    DataSize - points to the variable to receive the actual data size

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoepExtractErrData");
    NTSTATUS status;
    PERRENTRY Err;
    ULONG len, i;

    ASSERT(ErrStack != NULL);
    ASSERT((Buffer != NULL) && (BuffSize > 0) || (DataSize != NULL));
    ENTER(2, ("(ErrStack=%p,Buff=%p,BuffSize=%d,pDataSize=%p)\n",
              ErrStack, Buffer, BuffSize, DataSize));

    for (Err = ErrStack, len = sizeof(ERRINFO) - sizeof(ERRDATA), i = 0;
         Err != NULL;
         Err = IoepGetNextErrEntry(Err), i++)
    {
        len += sizeof(ERRDATA) +
               Err->DataBlkLen +
               Err->unicodeStr.MaximumLength;
    }

    if (len <= BuffSize)
    {
        if ((Buffer != NULL) && (len > 0))
        {
            PERRINFO ErrInfo = (PERRINFO)Buffer;
            PUCHAR pBuff;

            ErrInfo->Signature = SIG_ERRINFO;
            ErrInfo->Version = IOE_ERRINFO_VERSION;
            ErrInfo->Size = len;
            ErrInfo->DataTag = NULL;
            ErrInfo->TagFlags = 0;
            ErrInfo->NumErrEntries = i;
            pBuff = (PUCHAR)ErrInfo + sizeof(ERRINFO) + sizeof(ERRDATA)*(i - 1);
            for (Err = ErrStack, i = 0;
                 Err != NULL;
                 Err = IoepGetNextErrEntry(Err), i++)
            {
                ErrInfo->ErrEntries[i].ErrID = Err->ErrID;
                ErrInfo->ErrEntries[i].DataBlkType = Err->DataBlkType;
                ErrInfo->ErrEntries[i].DataBlkLen = Err->DataBlkLen;
                ErrInfo->ErrEntries[i].MofGuid = Err->MofGuid;

                if (Err->unicodeStr.MaximumLength > 0)
                {
                    ErrInfo->ErrEntries[i].TextDataOffset = pBuff -
                                                            (PUCHAR)ErrInfo;
                    RtlCopyMemory(pBuff,
                                  Err->unicodeStr.Buffer,
                                  Err->unicodeStr.MaximumLength);
                    pBuff += Err->unicodeStr.MaximumLength;
                }
                else
                {
                    ErrInfo->ErrEntries[i].TextDataOffset = 0;
                }

                if (Err->DataBlk != NULL)
                {
                    ErrInfo->ErrEntries[i].DataBlkOffset = pBuff -
                                                           (PUCHAR)ErrInfo;
                    RtlCopyMemory(pBuff, Err->DataBlk, Err->DataBlkLen);
                    pBuff += Err->DataBlkLen;
                }
                else
                {
                    ErrInfo->ErrEntries[i].DataBlkOffset = 0;
                }
            }
        }

        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
        DBGPRINT(("buffer too small (size=%d,need=%d)\n", BuffSize, len));
    }

    if (DataSize != NULL)
    {
        *DataSize = len;
    }

    EXIT(2, ("=%x(len=%d)\n", status, len));
    return status;
}       //IoepExtractErrData

NTSTATUS
IoepFireWMIEvent(
    IN PERRINFO ErrInfo,
    IN PWSTR    InstanceName
    )
/*++

Routine Description:
    This routine fires a WMI event notifying the availability of an error info.

Arguments:
    ErrInfo - points to the error info.
    InstanceName - points to the instance name string

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoepFireWMIEvent");
    NTSTATUS status;

    ENTER(2, ("(ErrInfo=%p,InstanceName=%S)\n", ErrInfo, InstanceName));

    if (ErrInfo != NULL)
    {
        ULONG NameLen, EventSize;
        PWNODE_SINGLE_INSTANCE event;

        NameLen = wcslen(InstanceName)*sizeof(WCHAR) + sizeof(UNICODE_NULL);
        EventSize = sizeof(WNODE_SINGLE_INSTANCE) + NameLen + ErrInfo->Size;
        event = ExAllocatePoolWithTag(NonPagedPool,
                                      EventSize,
                                      IOETAG_WMIEVENT);
        if (event != NULL)
        {
            event->WnodeHeader.BufferSize = EventSize;
            event->WnodeHeader.ProviderId = 0;
            event->WnodeHeader.Guid = IoepGuid;
            event->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE |
                                       WNODE_FLAG_EVENT_ITEM;
            KeQuerySystemTime(&event->WnodeHeader.TimeStamp);
            event->OffsetInstanceName = sizeof(WNODE_SINGLE_INSTANCE);
            event->DataBlockOffset = sizeof(WNODE_SINGLE_INSTANCE) + NameLen;
            event->SizeDataBlock = ErrInfo->Size;
            RtlCopyMemory(&event->VariableData, InstanceName, NameLen);
            RtlCopyMemory(event->VariableData + NameLen,
                          ErrInfo,
                          ErrInfo->Size);
            status = IoWMIWriteEvent(event);
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DBGPRINT(("failed to allocate WMI event node (len=%d)\n",
                      EventSize));
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //IoepFireWMIEvent

NTSTATUS
IoepHandleErrCase(
    IN PERRINFO ErrInfo,
    IN PERRCASE ErrCase,
    IN ULONG    Method,
    OUT PUNICODE_STRING unicodeMsg OPTIONAL
    )
/*++

Routine Description:
    This routine handles an error case by executing the resolution method.

Arguments:
    ErrInfo - points to the error info
    ErrCase - points to the error case
    Method - specifying what method to use to handle the error case
    unicodeMsg - point to unicodeStr to receive the error message

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoepHandleErrCase");
    NTSTATUS status;
    PERRCASEDB ErrCaseDB;

    PAGED_CODE();
    ENTER(2, ("(ErrInfo=%p,ErrCase=%p,Method=%x,unicodeMsg=%p)\n",
              ErrInfo, ErrCase, Method, unicodeMsg));

    ErrCaseDB = IoepGetErrCaseDB();
    if (ErrCaseDB != NULL)
    {
        PMETHOD MethodTable;
        ULONG i;
        PVOID MethodData;
        UNICODE_STRING unicodeStr;
        PERRHANDLER ErrHandler;
        PWSTR MsgTemplate;

        MethodTable = (PMETHOD)((ULONG_PTR)ErrCaseDB +
                                ErrCaseDB->DataBlkOffset +
                                ErrCase->MethodOffset);

        for (i = 0, status = STATUS_NOT_FOUND;
             (status == STATUS_NOT_FOUND) && (i < ErrCase->NumMethods);
             ++i)
        {
            if ((Method != IOEMETHOD_ANY) &&
                (Method != MethodTable[i].MethodType))
            {
                continue;
            }

            MethodData = (PVOID)((ULONG_PTR)ErrCaseDB +
                                 ErrCaseDB->DataBlkOffset +
                                 MethodTable[i].MethodDataOffset);

            switch (MethodTable[i].MethodType)
            {
                case IOEMETHOD_LONGMSG:
                    status = IoepGetErrMessage((PMSGDATA)MethodData,
                                               ErrInfo,
                                               unicodeMsg? unicodeMsg:
                                                           &unicodeStr);

                    if (NT_SUCCESS(status) && (unicodeMsg == NULL))
                    {
                        IoRaiseInformationalHardError(STATUS_IOE_MESSAGE,
                                                      &unicodeStr,
                                                      NULL);
                        RtlFreeUnicodeString(&unicodeStr);
                    }
                    break;

                case IOEMETHOD_SHORTMSG:
                    MsgTemplate =
                        (PWSTR)((ULONG_PTR)ErrCaseDB +
                                ErrCaseDB->DataBlkOffset +
                                ((PMSGDATA)MethodData)->MsgTemplateOffset);

                    if (unicodeMsg == NULL)
                    {
                        RtlInitUnicodeString(&unicodeStr, MsgTemplate);
                        IoRaiseInformationalHardError(STATUS_IOE_MESSAGE,
                                                      &unicodeStr,
                                                      NULL);
                        status = STATUS_SUCCESS;
                    }
                    else
                    {
                        ULONG len;
                        PWSTR pwstr;

                        len = wcslen(MsgTemplate)*sizeof(WCHAR) +
                              sizeof(UNICODE_NULL);
                        pwstr = ExAllocatePoolWithTag(PagedPool,
                                                      len,
                                                      IOETAG_DATAWSTR);
                        if (pwstr != NULL)
                        {
                            RtlCopyMemory(pwstr, MsgTemplate, len);
                            RtlInitUnicodeString(unicodeMsg, pwstr);
                            status = STATUS_SUCCESS;
                        }
                        else
                        {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            DBGPRINT(("failed to allocate short message buffer (len=%d)\n",
                                      len));
                        }
                    }
                    break;

                case IOEMETHOD_HANDLER:
                    ErrHandler = IoepFindErrHandler(
                                    &((PHANDLERDATA)MethodData)->ComponentGuid,
                                    ((PHANDLERDATA)MethodData)->HandlerIndex);

                    if (ErrHandler != NULL)
                    {
                        status = ErrHandler(ErrInfo,
                                            ((PHANDLERDATA)MethodData)->Param);
                    }
                    break;

                default:
                    DBGPRINT(("invalid method type %d\n",
                              MethodTable[i].MethodType));
            }
        }
    }
    else
    {
        status = STATUS_IOE_DATABASE_NOT_READY;
        DBGPRINT(("error case database not ready\n"));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //IoepHandleErrCase

PERRHANDLER
IoepFindErrHandler(
    IN CONST GUID *ComponentGuid,
    IN ULONG       HandlerIndex
    )
/*++

Routine Description:
    This routine find the error handler with the given module GUID and
    handler index.

Arguments:
    ComponentGuid - points to the GUID of the handler module
    HandlerIndex - index into handler table

Return Value:
    Success - returns pointer to error handler found
    Failure - returns NULL

--*/
{
    PROCNAME("IoepFindErrHandler");
    PERRHANDLER ErrHandler = NULL;
    PERRMODULE ErrModule;

    ENTER(2, ("(pGuid=%p,Index=%x)\n", ComponentGuid, HandlerIndex));

    ErrModule = IoepFindErrModule(ComponentGuid);

    if ((ErrModule != NULL) && (HandlerIndex < ErrModule->NumErrHandlers))
    {
        ErrHandler = ErrModule->HandlerTable[HandlerIndex];
    }

    EXIT(2, ("=%p\n", ErrHandler));
    return ErrHandler;
}       //IoepFindErrHandler

BOOLEAN
IoepMatchErrIDPath(
    IN PERRINFO ErrInfo,
    IN PERRID   ErrIDPath,
    IN ULONG    NumErrIDs
    )
/*++

Routine Description:
    This routine compares the error ID paths for a match.

Arguments:
    ErrInfo - points to the error info buffer
    ErrIDPath - points to the error ID path of the error case
    NumErrIDs - number of error IDs in the path

Return Value:
    Success - returns TRUE: matched
    Failure - returns FALSE: no match

--*/
{
    PROCNAME("IoepMatchErrIDPath");
    BOOLEAN rc = TRUE;
    BOOLEAN fWildCard = FALSE;
    ULONG i, j;

    PAGED_CODE();
    ENTER(2, ("(ErrInfo=%p,pErrID=%p,NumIDs=%d)\n",
              ErrInfo, ErrIDPath, NumErrIDs));

    for (i = j = 0; (i < NumErrIDs) && (j < ErrInfo->NumErrEntries);)
    {
        if (ErrIDPath[i].ErrCode == 0)
        {
            fWildCard = TRUE;
            i++;
        }
        else if (RtlEqualMemory(&ErrIDPath[i].ComponentGuid,
                                &ErrInfo->ErrEntries[j].ErrID.ComponentGuid,
                                sizeof(GUID)) &&
                 (ErrIDPath[i].ErrCode == ErrInfo->ErrEntries[j].ErrID.ErrCode))
        {
            i++;
            j++;
            fWildCard = FALSE;
        }
        else if (fWildCard)
        {
            j++;
        }
        else
        {
            rc = FALSE;
            break;
        }
    }

    if ((rc == TRUE) && (i < NumErrIDs))
    {
        //
        // The database ID path is longer than the error stack ID path.
        // This is not a good match, so let's move on to find another match.
        // However, the reverse is OK though (i.e. error stack ID path is
        // longer than the database ID path).  In this case, the database
        // ID path has an implicit wild card at the end.
        //
        rc = FALSE;
    }

    EXIT(2, ("=%x\n", rc));
    return rc;
}       //IoepMatchErrIDPath

NTSTATUS
IoepGetErrMessage(
    IN  PMSGDATA        MsgData,
    IN  PERRINFO        ErrInfo,
    OUT PUNICODE_STRING unicodeMsg
    )
/*++

Routine Description:
    This routine constructs the error message from the message method data.

Arguments:
    MsgData - points to the message method data
    ErrInfo - points to the error info
    unicodeMsg - points to the uninitialized unicode string message buffer

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

Note:
    This routine will allocate the actual string buffer of the unicode message.
    Therefore, it is the caller's responsibility to free the message buffer
    via RtlFreeUnicodeString.

--*/
{
    PROCNAME("IoepGetErrMessage");
    NTSTATUS status = STATUS_SUCCESS;
    PERRCASEDB ErrCaseDB;

    PAGED_CODE();
    ENTER(2, ("(pMsgData=%p,ErrInfo=%p,pMsg=%p)\n",
              MsgData, ErrInfo, unicodeMsg));

    ErrCaseDB = IoepGetErrCaseDB();
    if (ErrCaseDB != NULL)
    {
        PWSTR MsgTemplate, MsgBuff, pwstr;

        MsgTemplate = (PWSTR)((ULONG_PTR)ErrCaseDB +
                              ErrCaseDB->DataBlkOffset +
                              MsgData->MsgTemplateOffset);

        MsgBuff = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(WCHAR)*MAX_MSG_LEN +
                                        sizeof(UNICODE_NULL),
                                        IOETAG_MSGBUFF);

        if (MsgBuff != NULL)
        {
            MsgBuff[0] = L'\0';
            RtlInitUnicodeString(unicodeMsg, MsgBuff);
            unicodeMsg->MaximumLength = sizeof(WCHAR)*MAX_MSG_LEN +
                                        sizeof(UNICODE_NULL);

            while (NT_SUCCESS(status))
            {
                pwstr = wcschr(MsgTemplate, L'%');
                if (pwstr == NULL)
                {
                    status = RtlAppendUnicodeToString(unicodeMsg, MsgTemplate);
                    if (!NT_SUCCESS(status))
                    {
                        DBGPRINT(("failed to append unicode string (rc=%x)\n",
                                  status));
                    }
                    break;
                }
                else if ((pwstr[1] >= L'0') && (pwstr[1] <= L'9'))
                {
                    ULONG ArgIndex;
                    PWSTR pwstr2;

                    ArgIndex = wcstoul(&pwstr[1], &pwstr2, 10);
                    status = IoepUnicodeStringCatN(unicodeMsg,
                                                   MsgTemplate,
                                                   pwstr - MsgTemplate);
                    if (NT_SUCCESS(status))
                    {
                        MsgTemplate = pwstr2;
                        status = IoepCatMsgArg(unicodeMsg,
                                               &MsgData->Args[ArgIndex],
                                               ErrInfo);
                    }
                }
                else
                {
                    pwstr++;
                    status = IoepUnicodeStringCatN(unicodeMsg,
                                                   MsgTemplate,
                                                   pwstr - MsgTemplate);
                    if (NT_SUCCESS(status))
                    {
                        MsgTemplate = pwstr;
                    }
                }
            }
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DBGPRINT(("failed to allocate message buffer\n"));
        }
    }
    else
    {
        status = STATUS_IOE_DATABASE_NOT_READY;
        DBGPRINT(("error case database not ready\n"));
    }

    EXIT(2, ("=%x(Msg=%S)\n",
             status, unicodeMsg->Buffer? unicodeMsg->Buffer: L""));
    return status;
}       //IoepGetErrMessage

NTSTATUS
IoepCatMsgArg(
    IN OUT PUNICODE_STRING unicodeMsg,
    IN PMSGARG MsgArg,
    IN PERRINFO ErrInfo
    )
/*++

Routine Description:
    This routine appends the substituted message argument to the unicode
    string if there is enough space.  Otherwise, STATUS_BUFFER_TOO_SMALL is
    returned.

Arguments:
    unicodeMsg - points to the target unicode message
    MsgArg - points to the message argument structure
    ErrInfo - points to the error info.

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoepCatMsgArg");
    NTSTATUS status = STATUS_SUCCESS;
    PERRCASEDB ErrCaseDB;

    PAGED_CODE();
    ENTER(2, ("(Msg=%S,pMsgArg=%p,ErrInfo=%p)\n",
              unicodeMsg->Buffer, MsgArg, ErrInfo));

    ErrCaseDB = IoepGetErrCaseDB();
    if (ErrCaseDB != NULL)
    {
        if (MsgArg->ErrIDIndex < ErrInfo->NumErrEntries)
        {
            PERRDATA ErrData;
            ANSI_STRING ansiStr;
            UNICODE_STRING unicodeStr;
            PHANDLERDATA HandlerData;
            PERRHANDLER ErrHandler;

            ErrData = &ErrInfo->ErrEntries[MsgArg->ErrIDIndex];
            switch (MsgArg->ArgType)
            {
                case IOEDATA_TEXT:
                    status = RtlAppendUnicodeToString(
                                unicodeMsg,
                                (PWSTR)((PUCHAR)ErrInfo +
                                        ErrData->TextDataOffset));
                    if (!NT_SUCCESS(status))
                    {
                        DBGPRINT(("failed to append unicode string (rc=%x)\n",
                                  status));
                    }
                    break;

                case IOEDATA_WSTRING:
                    status = RtlAppendUnicodeToString(
                                unicodeMsg,
                                (PWSTR)((PUCHAR)ErrInfo +
                                        ErrData->DataBlkOffset));
                    if (!NT_SUCCESS(status))
                    {
                        DBGPRINT(("failed to append unicode string argument (rc=%x)\n",
                                  status));
                    }
                    break;

                case IOEDATA_PRIVATE:
                    HandlerData = (PHANDLERDATA)((ULONG_PTR)ErrCaseDB +
                                                 ErrCaseDB->DataBlkOffset +
                                                 MsgArg->ArgDataOffset);
                    ErrHandler = IoepFindErrHandler(&HandlerData->ComponentGuid,
                                                    HandlerData->HandlerIndex);
                    if (ErrHandler != NULL)
                    {
                        status = ErrHandler(ErrData, HandlerData->Param);
                    }
                    break;

                default:
                    DBGPRINT(("invalid message argument type %d\n",
                              MsgArg->ArgType));
            }
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
            DBGPRINT(("failed to find error entry\n"));
        }
    }
    else
    {
        status = STATUS_IOE_DATABASE_NOT_READY;
        DBGPRINT(("error case database not ready\n"));
    }

    EXIT(2, ("=%x(Msg=%S)\n", status, unicodeMsg->Buffer));
    return status;
}       //IoepCatMsgArg

NTSTATUS
IoepUnicodeStringCatN(
    IN OUT PUNICODE_STRING unicodeStr,
    IN PWSTR pwstr,
    IN ULONG len
    )
/*++

Routine Description:
    This routine appends the wchar string to the unicode string if there is
    enough space.  Otherwise, STATUS_BUFFER_TOO_SMALL is returned.

Arguments:
    unicodeStr - points to the target unicode string
    pwstr - points to the wchar string
    len - length of wstr in bytes to append

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoepUnicodeStringCatN");
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    ENTER(2, ("(Msg=%S,Str=%S,Len=%d)\n", unicodeStr->Buffer, pwstr, len));

    if (len < (ULONG)(unicodeStr->MaximumLength - unicodeStr->Length))
    {
        wcsncpy(unicodeStr->Buffer, pwstr, len/sizeof(WCHAR));
        unicodeStr->Length += (USHORT)len;
        unicodeStr->Buffer[len/sizeof(WCHAR)] = L'\0';
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
        DBGPRINT(("unicode string too small\n"));
    }

    EXIT(2, ("=%x(Msg=%S)\n", status, unicodeStr->Buffer));
    return status;
}       //IoepUnicodeStringCatN

PERRCASEDB
IoepGetErrCaseDB(
    VOID
    )
/*++

Routine Description:
    This routine checks if the ErrCase database has been read into memory.
    If it has, it returns the pointer to the error case database.  Otherwise,
    it tries to read the database in.

Arguments:
    None

Return Value:
    Success - returns pointer to the error case database
    Failure - returns NULL

--*/
{
    PROCNAME("IoepGetErrCaseDB");

    PAGED_CODE();
    ENTER(2, ("()\n"));

    if (IoepErrCaseDB == NULL)
    {
        OBJECT_ATTRIBUTES ObjAttr;
        NTSTATUS status;
        HANDLE hReg, hFile;

        InitializeObjectAttributes(&ObjAttr,
                                   &IoepRegKeyStrIoErr,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        status = ZwOpenKey(&hReg, KEY_READ, &ObjAttr);
        if (NT_SUCCESS(status))
        {
            PKEY_VALUE_FULL_INFORMATION KeyValue;

            status = IopGetRegistryValue(hReg, L"ErrCaseDB", &KeyValue);
            if (NT_SUCCESS(status))
            {
                UNICODE_STRING unicodeStr;
                IO_STATUS_BLOCK IoStatusBlk;

                RtlInitUnicodeString(&unicodeStr,
                                     (PWSTR)((PUCHAR)KeyValue +
                                             KeyValue->DataOffset));
                InitializeObjectAttributes(&ObjAttr,
                                           &unicodeStr,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);
                status = ZwCreateFile(&hFile,
                                      FILE_READ_DATA,
                                      &ObjAttr,
                                      &IoStatusBlk,
                                      NULL,
                                      FILE_ATTRIBUTE_NORMAL,
                                      FILE_SHARE_READ,
                                      FILE_OPEN,
                                      FILE_NON_DIRECTORY_FILE |
                                      FILE_SEQUENTIAL_ONLY |
                                      FILE_SYNCHRONOUS_IO_NONALERT,
                                      NULL,
                                      0);

                if (NT_SUCCESS(status) &&
                    (IoStatusBlk.Information == FILE_OPENED))
                {
                    ERRCASEDB DBHeader;

                    status = ZwReadFile(hFile,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatusBlk,
                                        &DBHeader,
                                        sizeof(DBHeader),
                                        NULL,
                                        NULL);

                    if (NT_SUCCESS(status))
                    {
                        IoepErrCaseDB = ExAllocatePoolWithTag(PagedPool,
                                                              DBHeader.Length,
                                                              IOETAG_ERRCASEDB);
                        if (IoepErrCaseDB != NULL)
                        {
                            RtlCopyMemory(IoepErrCaseDB,
                                          &DBHeader,
                                          sizeof(DBHeader));
                            status = ZwReadFile(hFile,
                                                NULL,
                                                NULL,
                                                NULL,
                                                &IoStatusBlk,
                                                (PUCHAR)IoepErrCaseDB +
                                                sizeof(DBHeader),
                                                DBHeader.Length -
                                                sizeof(DBHeader),
                                                NULL,
                                                NULL);

                            if (!NT_SUCCESS(status))
                            {
                                ExFreePool(IoepErrCaseDB);
                                IoepErrCaseDB = NULL;
                                DBGPRINT(("failed to read error case database (rc=%x)\n",
                                          status));
                            }
                        }
                        else
                        {
                            DBGPRINT(("failed to allocate storage for error case database (len=%d)\n",
                                      DBHeader.Length));
                        }
                    }
                    else
                    {
                        DBGPRINT(("failed to read database header (rc=%x)\n",
                                  status));
                    }
                    ZwClose(hFile);
                }
                else
                {
                    DBGPRINT(("failed to open error case database (rc=%x)\n",
                              status));
                }
                ExFreePool(KeyValue);
            }
            else
            {
                DBGPRINT(("failed to read registry value (rc=%x)\n", status));
            }
            ZwClose(hReg);
        }
        else
        {
            DBGPRINT(("failed to open registry key (rc=%x)\n", status));
        }
    }

    EXIT(2, ("=%p\n", IoepErrCaseDB));
    return IoepErrCaseDB;
}       //IoepGetErrCaseDB
