/* 
 * FFS File System Driver for Windows
 *
 * except.c
 *
 * 2004.5.6 ~
 *
 * Lee Jae-Hong, http://www.pyrasis.com
 *
 * See License.txt
 *
 */

#include "ntifs.h"
#include "ffsdrv.h"

/* Globals */

extern PFFS_GLOBAL FFSGlobal;


/* Definitions */

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, FFSExceptionFilter)
//#pragma alloc_text(PAGE, FFSExceptionHandler)
#endif


NTSTATUS
FFSExceptionFilter(
	IN PFFS_IRP_CONTEXT    IrpContext,
	IN PEXCEPTION_POINTERS ExceptionPointer)
{
	NTSTATUS Status, ExceptionCode;
	PEXCEPTION_RECORD ExceptRecord;

	ExceptRecord = ExceptionPointer->ExceptionRecord;

	ExceptionCode = ExceptRecord->ExceptionCode;

	FFSBreakPoint();

	//
	// Check IrpContext is valid or not
	//

	if (IrpContext)
	{
		if ((IrpContext->Identifier.Type != FFSICX) ||
				(IrpContext->Identifier.Size != sizeof(FFS_IRP_CONTEXT)))
		{
			IrpContext = NULL;
		}
	}
	else
	{
		if (FsRtlIsNtstatusExpected(ExceptionCode))
		{
			return EXCEPTION_EXECUTE_HANDLER;
		}
		else
		{
			FFSBugCheck(FFS_BUGCHK_EXCEPT, (ULONG_PTR)ExceptRecord,
					(ULONG_PTR)ExceptionPointer->ContextRecord,
					(ULONG_PTR)ExceptRecord->ExceptionAddress);
		}
	}

	//
	//  For the purposes of processing this exception, let's mark this
	//  request as being able to wait, and neither write through nor on
	//  removable media if we aren't posting it.
	//

	SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

	if (FsRtlIsNtstatusExpected(ExceptionCode))
	{
		//
		// If the exception is expected execute our handler
		//

		FFSPrint((DBG_ERROR, "FFSExceptionFilter: Catching exception %xh\n",
					ExceptionCode));

		Status = EXCEPTION_EXECUTE_HANDLER;

		if (IrpContext)
		{
			IrpContext->ExceptionInProgress = TRUE;
			IrpContext->ExceptionCode = ExceptionCode;
		}
	}
	else
	{
		//
		// Continue search for an higher level exception handler
		//

		FFSPrint((DBG_ERROR, "FFSExceptionFilter: Passing on exception %#x\n",
					ExceptionCode));

		Status = EXCEPTION_CONTINUE_SEARCH;

		if (IrpContext)
		{
			FFSFreeIrpContext(IrpContext);
		}
	}

	return Status;
}


NTSTATUS
FFSExceptionHandler(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS Status;

	FFSBreakPoint();

	if (IrpContext)
	{
		if ((IrpContext->Identifier.Type != FFSICX) || 
				(IrpContext->Identifier.Size != sizeof(FFS_IRP_CONTEXT)))
		{
			FFSBreakPoint();
			return STATUS_UNSUCCESSFUL;
		}

		Status = IrpContext->ExceptionCode;

		if (IrpContext->Irp)
		{
			//
			// Check if this error is a result of user actions
			//

			PIRP Irp = IrpContext->Irp;


			if (IoIsErrorUserInduced(Status))
			{
				//
				//  Now we will generate a pop-up to user
				//

				PDEVICE_OBJECT RealDevice;
				PVPB           Vpb = NULL;
				PETHREAD       Thread;

				if (IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL)
				{
					Vpb = IoGetCurrentIrpStackLocation(Irp)->FileObject->Vpb;
				}

				//
				// Get the initial thread
				//

				Thread = Irp->Tail.Overlay.Thread;
				RealDevice = IoGetDeviceToVerify(Thread);

				if (RealDevice == NULL)
				{
					//
					// Get current thread
					//

					Thread = PsGetCurrentThread();
					RealDevice = IoGetDeviceToVerify(Thread);

					ASSERT(RealDevice != NULL);
				}

				if (RealDevice != NULL)
				{
					//
					//  Now we pop-up the error dialog ...
					//

					IoMarkIrpPending(Irp);
					IoRaiseHardError(Irp, Vpb, RealDevice);

					IoSetDeviceToVerify(Thread, NULL);

					Status =  STATUS_PENDING;
					goto errorout;
				}
			}

			IrpContext->Irp->IoStatus.Status = Status;

			FFSCompleteRequest(IrpContext->Irp, FALSE, IO_NO_INCREMENT);
		}

errorout:

		FFSFreeIrpContext(IrpContext);
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
	}

	return Status;
}

