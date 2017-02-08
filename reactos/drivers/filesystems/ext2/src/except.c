/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             except.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

NTSTATUS
Ext2ExceptionFilter (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXCEPTION_POINTERS  ExceptionPointer
)
{
    NTSTATUS Status = EXCEPTION_EXECUTE_HANDLER;
    NTSTATUS ExceptionCode;
    PEXCEPTION_RECORD ExceptRecord;

    ExceptRecord = ExceptionPointer->ExceptionRecord;
    ExceptionCode = ExceptRecord->ExceptionCode;

    DbgPrint("-------------------------------------------------------------\n");
    DbgPrint("Exception happends in Ext2Fsd (code %xh):\n", ExceptionCode);
    DbgPrint(".exr %p;.cxr %p;\n", ExceptionPointer->ExceptionRecord,
             ExceptionPointer->ContextRecord);
    DbgPrint("-------------------------------------------------------------\n");

    DbgBreak();

    //
    // Check IrpContext is valid or not
    //

    if (IrpContext) {
        if ((IrpContext->Identifier.Type != EXT2ICX) ||
            (IrpContext->Identifier.Size != sizeof(EXT2_IRP_CONTEXT))) {
            DbgBreak();
            IrpContext = NULL;
        } else if (IrpContext->DeviceObject) {
            PEXT2_VCB Vcb = NULL;
            Vcb = (PEXT2_VCB) IrpContext->DeviceObject->DeviceExtension;
            if (NULL == Vcb) {
                Status = EXCEPTION_EXECUTE_HANDLER;
            } else {
                if (Vcb->Identifier.Type == EXT2VCB && !IsMounted(Vcb)) {
                    Status = EXCEPTION_EXECUTE_HANDLER;
                }
            }
        }
    } else {
        if (FsRtlIsNtstatusExpected(ExceptionCode)) {
            return EXCEPTION_EXECUTE_HANDLER;
        } else {
            Ext2BugCheck( EXT2_BUGCHK_EXCEPT, (ULONG_PTR)ExceptRecord,
                          (ULONG_PTR)ExceptionPointer->ContextRecord,
                          (ULONG_PTR)ExceptRecord->ExceptionAddress );
        }
    }

    if (IrpContext) {
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    if ( Status == EXCEPTION_EXECUTE_HANDLER ||
         FsRtlIsNtstatusExpected(ExceptionCode)) {
        //
        // If the exception is expected execute our handler
        //

        DEBUG(DL_ERR, ( "Ext2ExceptionFilter: Catching exception %xh\n",
                         ExceptionCode));

        Status = EXCEPTION_EXECUTE_HANDLER;

        if (IrpContext) {
            IrpContext->ExceptionInProgress = TRUE;
            IrpContext->ExceptionCode = ExceptionCode;
        }

    } else  {

        //
        // Continue search for an higher level exception handler
        //

        DEBUG(DL_ERR, ( "Ext2ExceptionFilter: Passing on exception %#x\n",
                        ExceptionCode));

        Status = EXCEPTION_CONTINUE_SEARCH;

        if (IrpContext) {
            Ext2FreeIrpContext(IrpContext);
        }
    }

    return Status;
}


NTSTATUS
Ext2ExceptionHandler (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    if (IrpContext) {

        if ( (IrpContext->Identifier.Type != EXT2ICX) ||
                (IrpContext->Identifier.Size != sizeof(EXT2_IRP_CONTEXT))) {
            DbgBreak();
            return STATUS_UNSUCCESSFUL;
        }

        Status = IrpContext->ExceptionCode;

        if (IrpContext->Irp) {

            //
            // Check if this error is a result of user actions
            //

            PEXT2_VCB  Vcb = NULL;
            PIRP Irp = IrpContext->Irp;
            PIO_STACK_LOCATION IrpSp;

            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            Vcb = (PEXT2_VCB) IrpContext->DeviceObject->DeviceExtension;

            if (NULL == Vcb) {
                Status = STATUS_INVALID_PARAMETER;
            } else if (Vcb->Identifier.Type != EXT2VCB) {
                Status = STATUS_INVALID_PARAMETER;
            } else if (!IsMounted(Vcb)) {
                if (IsFlagOn(Vcb->Flags, VCB_DEVICE_REMOVED)) {
                    Status = STATUS_NO_SUCH_DEVICE;
                } else {
                    Status = STATUS_VOLUME_DISMOUNTED;
                }
            } else {

                /* queue it again if our request is at top level */
                if (IrpContext->IsTopLevel &&
                        ((Status == STATUS_CANT_WAIT) ||
                         ((Status == STATUS_VERIFY_REQUIRED) &&
                          (KeGetCurrentIrql() >= APC_LEVEL)))) {

                    Status = Ext2QueueRequest(IrpContext);
                }
            }

            if (Status == STATUS_PENDING) {
                goto errorout;
            }

            Irp->IoStatus.Status = Status;

            if (IoIsErrorUserInduced(Status)) {

                //
                //  Now we will generate a pop-up to user
                //

                PDEVICE_OBJECT RealDevice;
                PVPB           Vpb = NULL;
                PETHREAD       Thread;

                if (IrpSp->FileObject != NULL) {
                    Vpb = IrpSp->FileObject->Vpb;
                }

                //
                // Get the initial thread
                //

                Thread = Irp->Tail.Overlay.Thread;
                RealDevice = IoGetDeviceToVerify( Thread );

                if (RealDevice == NULL) {
                    //
                    // Get current thread
                    //

                    Thread = PsGetCurrentThread();
                    RealDevice = IoGetDeviceToVerify( Thread );

                    ASSERT( RealDevice != NULL );
                }

                Status =  IrpContext->ExceptionCode;

                if (RealDevice != NULL) {

                    if (IrpContext->ExceptionCode == STATUS_VERIFY_REQUIRED) {

                        Status = IoVerifyVolume (RealDevice, FALSE);

                        ExAcquireResourceSharedLite(&Vcb->MainResource, TRUE);
                        if (NT_SUCCESS(Status) && (!IsMounted(Vcb) ||
                                                   IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))) {
                            Status = STATUS_WRONG_VOLUME;
                        }
                        ExReleaseResourceLite(&Vcb->MainResource);

                        if (Ext2CheckDismount(IrpContext, Vcb, FALSE)) {
                            Ext2CompleteIrpContext( IrpContext, STATUS_VOLUME_DISMOUNTED);
                            Status = STATUS_VOLUME_DISMOUNTED;
                            Irp = NULL;
                            goto errorout;
                        }

                        if ( (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                                (IrpContext->FileObject->RelatedFileObject == NULL) &&
                                ((Status == STATUS_SUCCESS) || (Status == STATUS_WRONG_VOLUME))) {

                            Irp->IoStatus.Information = IO_REMOUNT;

                            Ext2CompleteIrpContext( IrpContext, STATUS_REPARSE);
                            Status = STATUS_REPARSE;
                            Irp = NULL;
                        }

                        if (Irp) {

                            if (!NT_SUCCESS(Status)) {
                                IoSetHardErrorOrVerifyDevice(Irp, RealDevice);
                                ASSERT (STATUS_VERIFY_REQUIRED != Status);
                                Ext2NormalizeAndRaiseStatus(IrpContext, Status);
                            }

                            Status = Ext2QueueRequest(IrpContext);
                        }

                        goto errorout;

                    } else {

                        Status = STATUS_PENDING;

                        IoMarkIrpPending( Irp );
                        IoRaiseHardError( Irp, Vpb, RealDevice );
                        IoSetDeviceToVerify( Thread, NULL );
                        goto release_context;
                    }
                }
            }

            Ext2CompleteRequest(Irp, FALSE, IO_NO_INCREMENT);
        }

release_context:

        Ext2FreeIrpContext(IrpContext);

    } else {

        Status = STATUS_INVALID_PARAMETER;
    }

errorout:

    return Status;
}

