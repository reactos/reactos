/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             except.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, RfsdExceptionFilter)
//#pragma alloc_text(PAGE, RfsdExceptionHandler)
#endif

NTSTATUS
RfsdExceptionFilter (
             IN PRFSD_IRP_CONTEXT    IrpContext,
             IN PEXCEPTION_POINTERS ExceptionPointer)
{
    NTSTATUS Status, ExceptionCode;
    PEXCEPTION_RECORD ExceptRecord;

    ExceptRecord = ExceptionPointer->ExceptionRecord;

    ExceptionCode = ExceptRecord->ExceptionCode;

    DbgBreak();

    //
    // Check IrpContext is valid or not
    //

    if (IrpContext) {
        if ((IrpContext->Identifier.Type != RFSDICX) ||
            (IrpContext->Identifier.Size != sizeof(RFSD_IRP_CONTEXT))) {
            IrpContext = NULL;
        }
    } else {
        if (FsRtlIsNtstatusExpected(ExceptionCode)) {
            return EXCEPTION_EXECUTE_HANDLER;
        } else {
            RfsdBugCheck( RFSD_BUGCHK_EXCEPT, (ULONG_PTR)ExceptRecord,
                          (ULONG_PTR)ExceptionPointer->ContextRecord,
                          (ULONG_PTR)ExceptRecord->ExceptionAddress );
        }
    }

    //
    //  For the purposes of processing this exception, let's mark this
    //  request as being able to wait, and neither write through nor on
    //  removable media if we aren't posting it.
    //

	if (IrpContext)
		{ SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT); }

    if (FsRtlIsNtstatusExpected(ExceptionCode)) {
        //
        // If the exception is expected execute our handler
        //

        RfsdPrint((DBG_ERROR, "RfsdExceptionFilter: Catching exception %xh\n",
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

        RfsdPrint((DBG_ERROR, "RfsdExceptionFilter: Passing on exception %#x\n",
            ExceptionCode));
        
        Status = EXCEPTION_CONTINUE_SEARCH;
        
        if (IrpContext) {
            RfsdFreeIrpContext(IrpContext);
        }
    }
    
    return Status;
}

NTSTATUS
RfsdExceptionHandler (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;
    
    if (IrpContext) {
        if ( (IrpContext->Identifier.Type != RFSDICX) || 
             (IrpContext->Identifier.Size != sizeof(RFSD_IRP_CONTEXT))) {
            DbgBreak();
            return STATUS_UNSUCCESSFUL;
        }

        Status = IrpContext->ExceptionCode;

        if (IrpContext->Irp) {

            //
            // Check if this error is a result of user actions
            //

            PIRP Irp = IrpContext->Irp;
        

            if (IoIsErrorUserInduced(Status)) {

                //
                //  Now we will generate a pop-up to user
                //

                PDEVICE_OBJECT RealDevice;
                PVPB           Vpb = NULL;
                PETHREAD       Thread;

                if (IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL) {
                    Vpb = IoGetCurrentIrpStackLocation(Irp)->FileObject->Vpb;
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

                if (RealDevice != NULL) {

                    //
                    //  Now we pop-up the error dialog ...
                    //

                    IoMarkIrpPending( Irp );
                    IoRaiseHardError( Irp, Vpb, RealDevice );

                    IoSetDeviceToVerify( Thread, NULL );

                    Status =  STATUS_PENDING;
                    goto errorout;
                }
            }

            IrpContext->Irp->IoStatus.Status = Status;
            
            RfsdCompleteRequest(IrpContext->Irp, FALSE, IO_NO_INCREMENT);
        }

errorout:
        
        RfsdFreeIrpContext(IrpContext);

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return Status;
}
