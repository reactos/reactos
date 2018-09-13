/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioeapi.c

Abstract:

    This module contains the code for the exported IoErr APIs

Author:

    Michael Tsang (MikeTs) 2-Sep-1998

Environment:

    Kernel mode

Revision History:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IoErrInitSystem)
#pragma alloc_text(PAGE, IoErrMatchErrCase)
#pragma alloc_text(PAGE, IoErrFindErrCaseByID)
#pragma alloc_text(PAGE, IoErrHandleErrCase)
#pragma alloc_text(PAGE, IoErrGetLongErrMessage)
#pragma alloc_text(PAGE, IoErrGetShortErrMessage)
#endif

BOOLEAN
IoErrInitSystem(
    VOID
    )
/*++

Routine Description:
    This routine initializes the whole error logging/handling system.

Arguments:
    None

Return Value:
    Success - returns TRUE
    Failure - returns FALSE

--*/
{
    PROCNAME("IoErrInitSystem");
    BOOLEAN rc = TRUE;

    ENTER(1, ("()\n"));

    KeInitializeSpinLock(&IoepErrListLock);
    InitializeListHead(&IoepErrThreadListHead);
    InitializeListHead(&IoepErrModuleListHead);
    InitializeListHead(&IoepSaveDataListHead);
    RtlInitUnicodeString(
        &IoepRegKeyStrIoErr,
        L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES\\IOERR");

    EXIT(1, ("=%x\n", rc));
    return rc;
}       //IoErrInitSystem

HANDLE
IoErrInitErrLogByIrp(
    IN PIRP  Irp,
    IN ULONG ulFlags
    )
/*++

Routine Description:
    This routine initializes an error logging session that is keyed by an Irp.

Arguments:
    Irp - points to the Irp that is used as the key to the logging session.
    ulFlags - log session flags

Return Value:
    Success - returns the newly created error log handle.
    Failure - returns NULL.

--*/
{
    PROCNAME("IoErrInitErrLogByIrp");
    PERRLOG ErrLog;

    ENTER(1, ("(Irp=%p,ulFlags=%x)\n", Irp, ulFlags));

    ErrLog = IoepInitErrLog(THREADKEY_IRP, Irp, ulFlags);

    EXIT(1, ("=%p\n", ErrLog));
    return ErrLog;
}       //IoErrInitErrLogByIrp

HANDLE
IoErrInitErrLogByThreadID(
    IN PKTHREAD ThreadID,
    IN ULONG    ulFlags
    )
/*++

Routine Description:
    This routine initializes an error logging session that is keyed by thread.

Arguments:
    ThreadID - thread ID
    ulFlags - log session flags

Return Value:
    Success - returns the newly created error log handle.
    Failure - returns NULL.

--*/
{
    PROCNAME("IoErrInitErrLogByThreadID");
    PERRLOG ErrLog;

    ENTER(1, ("(ThreadID=%p,ulFlags=%x)\n", ThreadID, ulFlags));

    ErrLog = IoepInitErrLog(THREADKEY_THREADID, ThreadID, ulFlags);

    EXIT(1, ("=%p\n", ErrLog));
    return ErrLog;
}       //IoErrInitErrLogByThreadID

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
    )
/*++

Routine Description:
    This routine logs the error data to the error log session identified by
    the given Irp.

Arguments:
    Irp - points to the Irp that is used as the key to the logging session.
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
    PROCNAME("IoErrLogErrByIrp");

    ENTER(1, ("(Irp=%p,pGuid=%p,ErrCode=%x,Text=%S,Type=%x,Len=%d,DataBlk=%p,MofGuid=%p)\n",
              Irp, ComponentGuid, ErrCode, TextData? TextData: L"", DataBlkType,
              DataBlkLen, DataBlock, MofGuid));

    IoepLogErr(THREADKEY_IRP,
               Irp,
               ComponentGuid,
               ErrCode,
               TextData,
               DataBlkType,
               DataBlkLen,
               DataBlock,
               MofGuid);

    EXIT(1, ("!\n"));
}       //IoErrLogErrByIrp

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
    )
/*++

Routine Description:
    This routine logs the error data to the error log session identified by
    the given ThreadID.

Arguments:
    ThreadID - points to the ThreadID that is used as the key to the logging
               session.
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
    PROCNAME("IoErrLogErrByThreadID");

    ENTER(1, ("(ThreadID=%p,pGuid=%p,ErrCode=%x,Text=%S,Type=%x,Len=%d,DataBlk=%p,MofGuid=%p)\n",
              ThreadID, ComponentGuid, ErrCode, TextData? TextData: L"",
              DataBlkType, DataBlkLen, DataBlock, MofGuid));

    IoepLogErr(THREADKEY_THREADID,
               ThreadID,
               ComponentGuid,
               ErrCode,
               TextData,
               DataBlkType,
               DataBlkLen,
               DataBlock,
               MofGuid);

    EXIT(1, ("!\n"));
}       //IoErrLogErrByThreadID

VOID
IoErrPropagateErrLog(
    IN HANDLE ErrLogHandle
    )
/*++

Routine Description:
    This routine propagates the error log stack from the current error log
    session to the next nested error log session.

Arguments:
    ErrLogHandle - points to the error log session

Return Value:
    None

--*/
{
    PROCNAME("IoErrPropagateErrLog");
    PERRLOG ErrLog = (PERRLOG)ErrLogHandle;

    ASSERT(ErrLogHandle != NULL);
    ASSERT(((PERRLOG)ErrLogHandle)->Signature == SIG_ERRLOG);
    ENTER(1, ("(ErrLog=%p)\n", ErrLogHandle));

    if ((ErrLog != NULL) && (ErrLog->Signature == SIG_ERRLOG))
    {
        PERRENTRY ErrStack;
        KIRQL Irql;

        ExAcquireSpinLock(&IoepErrListLock, &Irql);
        ErrStack = IoepGetErrStack((PERRLOG)ErrLogHandle);
        if (ErrStack != NULL)
        {
            PERRLOG ErrLogNext;

            ErrLogNext = CONTAINING_RECORD(ErrLog->list.Flink, ERRLOG, list);
            if (&ErrLogNext->list != &ErrLog->ErrThread->ErrLogListHead)
            {
                PSINGLE_LIST_ENTRY ErrTail;

                for (ErrTail = &ErrStack->slist;
                     ErrTail->Next != NULL;
                     ErrTail = ErrTail->Next)
                    ;

                ErrTail->Next = ErrLogNext->ErrStack.Next;
                ErrLogNext->ErrStack.Next = ErrStack->slist.Next;
                ErrLog->ErrStack.Next = NULL;
                if (ErrLog->ErrInfo != NULL)
                {
                    ErrLog->ErrInfo->Signature = 0;
                    ExFreePool(ErrLog->ErrInfo);
                    ErrLog->ErrInfo = NULL;
                }
            }
        }
        ExReleaseSpinLock(&IoepErrListLock, Irql);
    }
    else
    {
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("!\n"));
}       //IoErrPropagateErrLog

VOID
IoErrTerminateErrLog(
    IN HANDLE ErrLogHandle
    )
/*++

Routine Description:
    This routine terminates an error log session.

Arguments:
    ErrLogHandle - points to the error log session

Return Value:
    None

--*/
{
    PROCNAME("IoErrTerminateErrLog");
    PERRLOG ErrLog = (PERRLOG)ErrLogHandle;

    ASSERT(ErrLogHandle != NULL);
    ASSERT(((PERRLOG)ErrLogHandle)->Signature == SIG_ERRLOG);
    ENTER(1, ("(ErrLog=%p)\n", ErrLogHandle));

    if ((ErrLog != NULL) && (ErrLog->Signature == SIG_ERRLOG))
    {
        PERRENTRY ErrStack;
        KIRQL Irql;

        ErrStack = IoepGetErrStack(ErrLog);
        if (ErrStack != NULL)
        {
            //
            // If we are at the top level log session by Irp and we have an
            // error, we fire a WMI event.
            //
            if ((ErrLog->ErrThread->ThreadKeyType == THREADKEY_IRP) &&
                (ErrLog->list.Flink == &ErrLog->ErrThread->ErrLogListHead))
            {
                PDEVICE_NODE DevNode;

                ASSERT_PDO(ErrLog->ErrThread->ThreadKey.IrpKey.TargetDevice);
                DevNode = ErrLog->ErrThread->ThreadKey.IrpKey.TargetDevice->DeviceObjectExtension->DeviceNode;
                IoepFireWMIEvent(IoErrGetErrData(ErrLogHandle),
                                 DevNode->InstancePath.Buffer);
            }

            ErrLog->ErrStack.Next = NULL;
            IoepFreeErrStack(ErrStack);
        }

        ExAcquireSpinLock(&IoepErrListLock, &Irql);
        if (ErrLog->ErrInfo != NULL)
        {
            ErrLog->ErrInfo->Signature = 0;
            ExFreePool(ErrLog->ErrInfo);
            ErrLog->ErrInfo = NULL;
        }

        RemoveEntryList(&ErrLog->list);
        if (IsListEmpty(&ErrLog->ErrThread->ErrLogListHead))
        {
            RemoveEntryList(&ErrLog->ErrThread->list);
            ExFreePool(ErrLog->ErrThread);
            ErrLog->ErrThread = NULL;
        }
        ErrLog->Signature = 0;
        ExFreePool(ErrLog);
        ExReleaseSpinLock(&IoepErrListLock, Irql);
    }
    else
    {
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("!\n"));
}       //IoErrTerminateErrLog

NTSTATUS
IoErrRegisterErrHandlers(
    IN CONST GUID  *ComponentGuid,
    IN ULONG        NumErrHandlers,
    IN PERRHANDLER *HandlerTable
    )
/*++

Routine Description:
    This routine registers error handlers for the caller module.

Arguments:
    ComponentGuid - points to the GUID of the caller component
    NumErrHandlers - number of error handlers in the table
    HandlerTable - points to the error handler table

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoErrRegisterErrHandlers");
    NTSTATUS status;
    PERRMODULE ErrModule;

    ASSERT(ComponentGuid != NULL);
    ASSERT(NumErrHandlers > 0);
    ASSERT(HandlerTable != NULL);
    ENTER(1, ("(pGuid=%p,NumHandlers=%d,HandleTable=%p)\n",
              ComponentGuid, NumErrHandlers, HandlerTable));

    ErrModule = IoepFindErrModule(ComponentGuid);
    if (ErrModule == NULL)
    {
        ErrModule = ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(ERRMODULE) +
                        sizeof(PERRHANDLER)*(NumErrHandlers - 1),
                        IOETAG_ERRMODULE);

        if (ErrModule != NULL)
        {
            ErrModule->ComponentGuid = *ComponentGuid;
            ErrModule->NumErrHandlers = NumErrHandlers;
            RtlCopyMemory(ErrModule->HandlerTable,
                          HandlerTable,
                          sizeof(PERRHANDLER)*NumErrHandlers);
            ExInterlockedInsertTailList(&IoepErrModuleListHead,
                                        &ErrModule->list,
                                        &IoepErrListLock);
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DBGPRINT(("failed to allocate error module\n"));
        }
    }
    else
    {
        status = STATUS_IOE_MODULE_ALREADY_REGISTERED;
        DBGPRINT(("error module already registered\n"));
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //IoErrRegisterErrHandlers

PERRINFO
IoErrGetErrData(
    IN HANDLE ErrLogHandle
    )
/*++

Routine Description:
    This routine returns the error data from the error log session.

Arguments:
    ErrLogHandle - points to the error log session

Return Value:
    Success - returns pointer to the error info structure
    Failure - returns NULL

--*/
{
    PROCNAME("IoErrGetErrData");
    PERRINFO ErrInfo = NULL;
    PERRLOG ErrLog = (PERRLOG)ErrLogHandle;

    ASSERT(ErrLogHandle != NULL);
    ASSERT(((PERRLOG)ErrLogHandle)->Signature == SIG_ERRLOG);
    ENTER(1, ("(ErrLog=%p)\n", ErrLogHandle));

    if ((ErrLog != NULL) && (ErrLog->Signature == SIG_ERRLOG))
    {
        if (ErrLog->ErrInfo != NULL)
        {
            ErrInfo = ErrLog->ErrInfo;
        }
        else
        {
            NTSTATUS status;
            ULONG len;

            status = IoepExtractErrData(IoepGetErrStack(ErrLog),
                                        NULL,
                                        0,
                                        &len);
            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                ErrInfo = ExAllocatePoolWithTag(NonPagedPool,
                                                len,
                                                IOETAG_ERRINFO);
                if (ErrInfo != NULL)
                {
                    status = IoepExtractErrData(
                                    IoepGetErrStack(ErrLog),
                                    ErrInfo,
                                    len,
                                    NULL);
                    if (NT_SUCCESS(status))
                    {
                        ErrLog->ErrInfo = ErrInfo;
                    }
                    else
                    {
                        ExFreePool(ErrInfo);
                        ErrInfo = NULL;
                        DBGPRINT(("failed to get error data (rc=%x)\n",
                                  status));
                    }
                }
                else
                {
                    DBGPRINT(("failed to allocate error info buffer (len=%d)\n",
                              len));
                }
            }
            else
            {
                DBGPRINT(("failed to determine error data size (rc=%x)\n", status));
            }
        }
    }
    else
    {
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%p\n", ErrInfo));
    return ErrInfo;
}       //IoErrGetErrData

HANDLE
IoErrSaveErrData(
    IN HANDLE ErrLogHandle,
    IN PVOID  DataTag OPTIONAL,
    IN ULONG  TagFlags OPTIONAL
    )
/*++

Routine Description:
    This routine saves the error data and returns a handle to the saved data.

Arguments:
    ErrLogHandle - points to the error log session
    DataTag - tag to be associated with for the saved error data
    KeyFlags - flags about the DataTag:
        IOEDATATAG_TYPE_DEVNODE - DataTag is a DevNode pointer

Return Value:
    Success - returns the detached error log handle
    Failure - returns NULL

--*/
{
    PROCNAME("IoErrSaveErrData");
    PSAVEDATA SaveData = NULL;

    ASSERT(ErrLogHandle != NULL);
    ASSERT(((PERRLOG)ErrLogHandle)->Signature == SIG_ERRLOG);
    ASSERT((TagFlags & ~IOEDATATAG_BITS) == 0);
    ENTER(1, ("(ErrLog=%p,DataTag=%p,TagFlags=%x)\n",
              ErrLogHandle, DataTag, TagFlags));

    if ((ErrLogHandle != NULL) &&
        (((PERRLOG)ErrLogHandle)->Signature == SIG_ERRLOG))
    {
        PERRINFO ErrInfo;

        ErrInfo = IoErrGetErrData(ErrLogHandle);
        if (ErrInfo != NULL)
        {
            SaveData = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(SAVEDATA),
                                             IOETAG_SAVEDATA);
            if (SaveData != NULL)
            {
                ErrInfo->DataTag = DataTag;
                ErrInfo->TagFlags = TagFlags & IOEDATATAG_TYPE_MASK;
                SaveData->Signature = SIG_SAVEDATA;
                SaveData->ErrInfo = ErrInfo;
                ((PERRLOG)ErrLogHandle)->ErrInfo = NULL;
                ExInterlockedInsertHeadList(&IoepSaveDataListHead,
                                            &SaveData->list,
                                            &IoepErrListLock);
            }
            else
            {
                DBGPRINT(("failed to allocate save data block\n"));
            }
        }
    }
    else
    {
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%p\n", SaveData));
    return SaveData;
}       //IoErrSaveErrData

PERRINFO
IoErrGetSavedData(
    IN HANDLE SaveDataHandle
    )
/*++

Routine Description:
    This routine returns the error info. from the saved error data.

Arguments:
    SaveDataHandle - points to the saved error data

Return Value:
    Success - returns a pointer to the saved error info.
    Failure - returns NULL

--*/
{
    PROCNAME("IoErrGetSavedData");
    PERRINFO ErrInfo;

    ASSERT(SaveDataHandle != NULL);
    ASSERT(((PSAVEDATA)SaveDataHandle)->Signature == SIG_SAVEDATA);
    ENTER(1, ("(SaveData=%p)\n", SaveDataHandle));

    if ((SaveDataHandle != NULL) &&
        (((PSAVEDATA)SaveDataHandle)->Signature == SIG_SAVEDATA))
    {
        ErrInfo = ((PSAVEDATA)SaveDataHandle)->ErrInfo;
    }
    else
    {
        ErrInfo = NULL;
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%p\n", ErrInfo));
    return ErrInfo;
}       //IoErrGetSavedData

VOID
IoErrFreeSavedData(
    IN HANDLE SaveDataHandle
    )
/*++

Routine Description:
    This routine frees the storage associated with the saved error data.
    This function should only be call if the error data has been previously
    saved via IoErrSaveErrData.

Arguments:
    SaveDataHandle - points to the save data

Return Value:
    None

--*/
{
    PROCNAME("IoErrFreeSavedData");
    PSAVEDATA SaveData = (PSAVEDATA)SaveDataHandle;

    ASSERT(SaveDataHandle != NULL);
    ASSERT(((PSAVEDATA)SaveDataHandle)->Signature == SIG_SAVEDATA);
    ENTER(1, ("(SaveData=%p)\n", SaveDataHandle));

    if ((SaveDataHandle != NULL) &&
        (((PSAVEDATA)SaveDataHandle)->Signature == SIG_SAVEDATA))
    {
        KIRQL Irql;

        ExAcquireSpinLock(&IoepErrListLock, &Irql);
        RemoveEntryList(&SaveData->list);
        if (SaveData->ErrInfo != NULL)
        {
            SaveData->ErrInfo->Signature = 0;
            ExFreePool(SaveData->ErrInfo);
            SaveData->ErrInfo = NULL;
        }
        SaveData->Signature = 0;
        ExFreePool(SaveData);
        ExReleaseSpinLock(&IoepErrListLock, Irql);
    }
    else
    {
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("!\n"));
}       //IoErrFreeSavedData

NTSTATUS
IoErrRetrieveSavedData(
    OUT PINFOBLK InfoBlk,
    IN  ULONG    BuffSize,
    OUT PULONG   DataSize OPTIONAL,
    IN  PVOID    DataTag OPTIONAL,
    IN  ULONG    TagFlags OPTIONAL
    )
/*++

Routine Description:
    This routine returns the saved error info. in the given buffer.  If no
    DataTag is given, data of all saved error info. are returned.  Otherwise,
    only the data of the error info. which matches the data tag is returned.

Arguments:
    InfoBlk - points to the buffer to receive the data
    BuffSize - specifies the size of the buffer
    DataSize - points to the variable to receive the actual data size
    DataTag - tag associated with the data info.
    TagFlags - tag flags

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoErrRetrieveSavedData");
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY list;
    ULONG len, i;
    PSAVEDATA SaveData;

    ASSERT((InfoBlk != NULL) && (BuffSize > 0) || (DataSize != NULL));
    ENTER(1, ("(Buff=%p,BuffSize=%d,pDataSize=%p,DataTag=%p,TagFlags=%x)\n",
              InfoBlk, BuffSize, DataSize, DataTag, TagFlags));

    for (list = IoepSaveDataListHead.Flink,
         len = sizeof(INFOBLK) - sizeof(ERRINFO),
         i = 0;
         list != &IoepSaveDataListHead;
         list = list->Flink)
    {
        SaveData = CONTAINING_RECORD(list, SAVEDATA, list);
        if ((DataTag == NULL) ||
            (DataTag == SaveData->ErrInfo->DataTag) &&
            (TagFlags == SaveData->ErrInfo->TagFlags & IOEDATATAG_TYPE_MASK))
        {
            i++;
            len += SaveData->ErrInfo->Size;
        }
    }

    if (len <= BuffSize)
    {
        if ((InfoBlk != NULL) && (BuffSize > 0))
        {
            PERRINFO ErrInfo;

            InfoBlk->Signature = SIG_INFOBLK;
            InfoBlk->Version = IOE_INFOBLK_VERSION;
            InfoBlk->Size = len;
            InfoBlk->NumErrInfos = i;

            for (list = IoepSaveDataListHead.Flink,
                 ErrInfo = &InfoBlk->ErrInfos[0];
                 list != &IoepSaveDataListHead;
                 list = list->Flink)
            {
                SaveData = CONTAINING_RECORD(list, SAVEDATA, list);
                if ((DataTag == NULL) ||
                    (DataTag == SaveData->ErrInfo->DataTag) &&
                    (TagFlags == SaveData->ErrInfo->TagFlags &
                     IOEDATATAG_TYPE_MASK))
                {
                    RtlCopyMemory(ErrInfo,
                                  SaveData->ErrInfo,
                                  SaveData->ErrInfo->Size);
                    ErrInfo = (PERRINFO)((PUCHAR)ErrInfo +
                                         SaveData->ErrInfo->Size);
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

    EXIT(1, ("=%x(len=%d)\n", status, len));
    return status;
}       //IoErrRetrievedSavedData

NTSTATUS
IoErrMatchErrCase(
    IN  PERRINFO ErrInfo,
    OUT PULONG   ErrCaseID,
    OUT PHANDLE  ErrCaseHandle OPTIONAL
    )
/*++

Routine Description:
    This routine gets the indexed error entry from the error stack.

Arguments:
    ErrInfo - points to the error info.
    ErrCaseID - points to the variable to receive the error case ID
    ErrCaseHandle - points to the variable to receive the error case handle

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoErrMatchErrCase");
    NTSTATUS status;

    ASSERT(ErrInfo != NULL);
    ASSERT(ErrInfo->Signature == SIG_ERRINFO);
    ASSERT(ErrCaseID != NULL);
    PAGED_CODE();
    ENTER(1, ("(ErrInfo=%p,pErrCaseID=%p,pErrCaseHandle=%p)\n",
              ErrInfo, ErrCaseID, ErrCaseHandle));

    if ((ErrInfo != NULL) && (ErrInfo->Signature == SIG_ERRINFO))
    {
        PERRCASEDB ErrCaseDB = IoepGetErrCaseDB();

        if (ErrCaseDB != NULL)
        {
            PERRCASE ErrCaseTable;
            PERRID ErrIDPath;
            ULONG i;

            ErrCaseTable = (PERRCASE)((ULONG_PTR)ErrCaseDB +
                                      ErrCaseDB->ErrCaseOffset);

            for (i = 0, status = STATUS_NOT_FOUND; i < ErrCaseDB->NumErrCases; ++i)
            {
                ErrIDPath = (PERRID)((ULONG_PTR)ErrCaseDB +
                                     ErrCaseDB->ErrIDPathBlkOffset +
                                     ErrCaseTable[i].ErrIDPathOffset);

                if (IoepMatchErrIDPath(ErrInfo,
                                       ErrIDPath,
                                       ErrCaseTable[i].NumErrIDs))
                {
                    *ErrCaseID = ErrCaseTable[i].ErrCaseID;
                    if (ErrCaseHandle != NULL)
                    {
                        *ErrCaseHandle = &ErrCaseTable[i];
                    }
                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }
        else
        {
            status = STATUS_IOE_DATABASE_NOT_READY;
            DBGPRINT(("error case database not ready\n"));
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%x(ErrCaseID=%x,ErrCase=%p)\n",
             status, *ErrCaseID, ErrCaseHandle? *ErrCaseHandle: 0));
    return status;
}       //IoErrMatchErrCase

NTSTATUS
IoErrFindErrCaseByID(
    IN  ULONG   ErrCaseID,
    OUT PHANDLE ErrCaseHandle
    )
/*++

Routine Description:
    This routine finds an error case by the error case ID and returns the
    handle to the error case.

Arguments:
    ErrCaseID - unique error case ID
    ErrCaseHandle - points to the variable to receive the error case handle

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoErrFindErrCaseByID");
    NTSTATUS status;
    PERRCASEDB ErrCaseDB;

    PAGED_CODE();
    ENTER(1, ("(ErrCaseID=%x,pErrCaseHandle=%p)\n", ErrCaseID, ErrCaseHandle));

    ErrCaseDB = IoepGetErrCaseDB();
    if (ErrCaseDB != NULL)
    {
        PERRCASE ErrCaseTable;
        ULONG i;

        ErrCaseTable = (PERRCASE)((ULONG_PTR)ErrCaseDB +
                                  ErrCaseDB->ErrCaseOffset);

        for (i = 0, status = STATUS_NOT_FOUND; i < ErrCaseDB->NumErrCases; ++i)
        {
            if (ErrCaseTable[i].ErrCaseID == ErrCaseID)
            {
                *ErrCaseHandle = &ErrCaseTable[i];
                status = STATUS_SUCCESS;
                break;
            }
        }
    }
    else
    {
        status = STATUS_IOE_DATABASE_NOT_READY;
        DBGPRINT(("error case database not ready\n"));
    }

    EXIT(1, ("=%x(ErrCase=%p)\n", status, *ErrCaseHandle));
    return status;
}       //IoErrFindErrCaseByID

NTSTATUS
IoErrHandleErrCase(
    IN PERRINFO ErrInfo,
    IN HANDLE   ErrCaseHandle
    )
/*++

Routine Description:
    This routine handles an error case by executing the resolution method.

Arguments:
    ErrInfo - points to the error info.
    ErrCaseHandle - points to the error case handle

Return Value:
    Success - returns STATUS_SUCCESS
    Failure - returns NT status code

--*/
{
    PROCNAME("IoErrHandleErrCase");
    NTSTATUS status;

    ASSERT(ErrInfo != NULL);
    ASSERT(ErrInfo->Signature == SIG_ERRINFO);
    ASSERT(ErrCaseHandle != NULL);
    PAGED_CODE();
    ENTER(1, ("(ErrInfo=%p,ErrCase=%p)\n", ErrInfo, ErrCaseHandle));

    if ((ErrInfo != NULL) && (ErrInfo->Signature == SIG_ERRINFO))
    {
        status = IoepHandleErrCase(ErrInfo,
                                   (PERRCASE)ErrCaseHandle,
                                   IOEMETHOD_ANY,
                                   NULL);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //IoErrHandleErrCase

NTSTATUS
IoErrGetLongErrMessage(
    IN  PERRINFO ErrInfo,
    IN  HANDLE   ErrCaseHandle,
    OUT PUNICODE_STRING unicodeMsg
    )
/*++

Routine Description:
    This routine handles the error case by executing the long message method
    and returns the resulting message.

Arguments:
    ErrInfo - points to the error info.
    ErrCaseHandle - points to the error case handle
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
    PROCNAME("IoErrGetLongErrMessage");
    NTSTATUS status;

    ASSERT(ErrInfo != NULL);
    ASSERT(ErrInfo->Signature == SIG_ERRINFO);
    ASSERT(ErrCaseHandle != NULL);
    ASSERT(unicodeMsg != NULL);
    PAGED_CODE();
    ENTER(1, ("(ErrInfo=%p,ErrCase=%p,pMsg=%p)\n",
              ErrInfo, ErrCaseHandle, unicodeMsg));

    if ((ErrInfo != NULL) && (ErrInfo->Signature == SIG_ERRINFO))
    {
        status = IoepHandleErrCase(ErrInfo,
                                   (PERRCASE)ErrCaseHandle,
                                   IOEMETHOD_LONGMSG,
                                   unicodeMsg);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%x(Msg=%S)\n", status, unicodeMsg->Buffer));
    return status;
}       //IoErrGetLongErrMessage

NTSTATUS
IoErrGetShortErrMessage(
    IN  PERRINFO ErrInfo,
    IN  HANDLE   ErrCaseHandle,
    OUT PUNICODE_STRING unicodeMsg
    )
/*++

Routine Description:
    This routine handles the error case by executing the short message method
    and returns the resulting message.

Arguments:
    ErrInfo - points to the error info.
    ErrCaseHandle - points to the error case handle
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
    PROCNAME("IoErrGetShortErrMessage");
    NTSTATUS status;

    ASSERT(ErrInfo != NULL);
    ASSERT(ErrInfo->Signature == SIG_ERRINFO);
    ASSERT(ErrCaseHandle != NULL);
    ASSERT(unicodeMsg != NULL);
    PAGED_CODE();
    ENTER(1, ("(ErrInfo=%p,ErrCase=%p,pMsg=%p)\n",
              ErrInfo, ErrCaseHandle, unicodeMsg));

    if ((ErrInfo != NULL) && (ErrInfo->Signature == SIG_ERRINFO))
    {
        status = IoepHandleErrCase(ErrInfo,
                                   (PERRCASE)ErrCaseHandle,
                                   IOEMETHOD_SHORTMSG,
                                   unicodeMsg);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        DBGPRINT(("invalid handle\n"))
    }

    EXIT(1, ("=%x(Msg=%S)\n", status, unicodeMsg->Buffer));
    return status;
}       //IoErrGetShortErrMessage
