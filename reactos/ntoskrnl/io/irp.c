/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/irp.c
 * PURPOSE:         Handle IRPs
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_IRP      TAG('I', 'R', 'P', ' ')
#define TAG_SYS_BUF  TAG('I', 'o', ' ' , ' ')

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 *
 * FUNCTION: Allocates an IRP
 * ARGUMENTS:
 *          StackSize = the size of the stack required for the irp
 *          ChargeQuota = Charge allocation to current threads quota
 * RETURNS: Irp allocated
 */
PIRP 
STDCALL
IoAllocateIrp(CCHAR StackSize,
              BOOLEAN ChargeQuota)
{
    PIRP Irp;
    USHORT Size = IoSizeOfIrp(StackSize);
  
    /* Check if we shoudl charge quota */
    if (ChargeQuota)
    {
        /* Irp = ExAllocatePoolWithQuotaTag(NonPagedPool,IoSizeOfIrp(StackSize), TAG_IRP); */
        /* FIXME */
        Irp = ExAllocatePoolWithTag(NonPagedPool,
                                    Size,
                                    TAG_IRP);
    }
    else
    {
        /* Allocate the IRP With no Quota charge */
        Irp = ExAllocatePoolWithTag(NonPagedPool,
                                    Size,
                                    TAG_IRP);
    }

    /* Make sure it was sucessful */
    if (Irp==NULL) return(NULL);

    /* Now Initialize it */
    IoInitializeIrp(Irp, Size, StackSize);
    
    /* Return it */
    return Irp;
}

/*
 * @implemented
 *
 * FUNCTION: Allocates and sets up an IRP to be sent to lower level drivers
 * ARGUMENTS:
 *         MajorFunction = One of IRP_MJ_READ, IRP_MJ_WRITE, 
 *                         IRP_MJ_FLUSH_BUFFERS or IRP_MJ_SHUTDOWN
 *         DeviceObject = Device object to send the irp to
 *         Buffer = Buffer into which data will be read or written
 *         Length = Length in bytes of the irp to be allocated
 *         StartingOffset = Starting offset on the device
 *         IoStatusBlock (OUT) = Storage for the result of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
PIRP
STDCALL
IoBuildAsynchronousFsdRequest(ULONG MajorFunction,
                              PDEVICE_OBJECT DeviceObject,
                              PVOID Buffer,
                              ULONG Length,
                              PLARGE_INTEGER StartingOffset,
                              PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    LOCK_OPERATION AccessType;

    DPRINT("IoBuildAsynchronousFsdRequest(MajorFunction %x, DeviceObject %x, "
            "Buffer %x, Length %x, StartingOffset %x, "
            "IoStatusBlock %x\n",MajorFunction,DeviceObject,Buffer,Length,
            StartingOffset,IoStatusBlock);

    /* Allocate IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE))) return Irp;
   
    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    
    /* Write the Major function and then deal with it */
    StackPtr->MajorFunction = (UCHAR)MajorFunction; 
    
    /* Do not handle the following here */
    if (MajorFunction != IRP_MJ_FLUSH_BUFFERS &&
        MajorFunction != IRP_MJ_SHUTDOWN &&
        MajorFunction != IRP_MJ_PNP) 
    {
        /* Check if this is Buffered IO */
        if (DeviceObject->Flags & DO_BUFFERED_IO)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                                    Length,
                                                                    TAG_SYS_BUF);
            
            /* Set flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            
            /* Handle special IRP_MJ_WRITE Case */
            if (MajorFunction == IRP_MJ_WRITE)
            {
                /* Copy the buffer data */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
            }
            else
            {
                /* Set the Input Operation flag and set this as a User Buffer */
                Irp->Flags |= IRP_INPUT_OPERATION;
                Irp->UserBuffer = Buffer;
            }
        }
        else if (DeviceObject->Flags & DO_DIRECT_IO)
        {
            /* Use an MDL for Direct I/O */
            Irp->MdlAddress = MmCreateMdl(NULL, Buffer, Length);
            
            /* Use the right Access Type */
            if (MajorFunction == IRP_MJ_READ)
            {
                 AccessType = IoReadAccess;
            }
            else
            {
                 AccessType = IoWriteAccess;
            }
            
            /* Probe and Lock */
            _SEH_FILTER(FreeAndGoOn)
            {
                /* Free the IRP and its MDL */
                IoFreeMdl(Irp->MdlAddress);
                IoFreeIrp(Irp);
                return EXCEPTION_CONTINUE_SEARCH;
             } 
            _SEH_TRY_FILTER(FreeAndGoOn) 
            {
                /* Do the probe */
                MmProbeAndLockPages(Irp->MdlAddress, KernelMode, AccessType);
            }
            _SEH_HANDLE
            {
                return NULL;
            }
            _SEH_END;
        } 
        else
        {
            /* Neither, use the buffer */
            Irp->UserBuffer = Buffer;
        }

        if (MajorFunction == IRP_MJ_READ)
        {
            StackPtr->Parameters.Read.Length = Length;
            StackPtr->Parameters.Read.ByteOffset = *StartingOffset;
        }
        else if (MajorFunction == IRP_MJ_WRITE)
        {
            StackPtr->Parameters.Write.Length = Length;
            StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
        }
    }
    
    /* Set the Current Thread and IOSB */
    if (!IoStatusBlock) KEBUGCHECK(0); /* Temporary to catch illegal ROS Drivers */
    Irp->UserIosb = IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    
    /* Set the Status Block after all is done */
    return Irp;
}

/*
 * @implemented
 *
 * FUNCTION: Allocates and sets up an IRP to be sent to drivers
 * ARGUMENTS:
 *         IoControlCode = Device io control code
 *         DeviceObject = Device object to send the irp to
 *         InputBuffer = Buffer from which data will be read by the driver
 *         InputBufferLength = Length in bytes of the input buffer
 *         OutputBuffer = Buffer into which data will be written by the driver
 *         OutputBufferLength = Length in bytes of the output buffer
 *         InternalDeviceIoControl = Determines weather
 *                                   IRP_MJ_INTERNAL_DEVICE_CONTROL or
 *                                   IRP_MJ_DEVICE_CONTROL will be used
 *         Event = Event used to notify the caller of completion
 *         IoStatusBlock (OUT) = Storage for the result of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
PIRP 
STDCALL
IoBuildDeviceIoControlRequest(ULONG IoControlCode,
                              PDEVICE_OBJECT DeviceObject,
                              PVOID InputBuffer,
                              ULONG InputBufferLength,
                              PVOID OutputBuffer,
                              ULONG OutputBufferLength,
                              BOOLEAN InternalDeviceIoControl,
                              PKEVENT Event,
                              PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    ULONG BufferLength;
    LOCK_OPERATION AccessType;

    DPRINT("IoBuildDeviceIoRequest(IoControlCode %x, DeviceObject %x, "
           "InputBuffer %x, InputBufferLength %x, OutputBuffer %x, "
           "OutputBufferLength %x, InternalDeviceIoControl %x "
           "Event %x, IoStatusBlock %x\n",IoControlCode,DeviceObject,
           InputBuffer,InputBufferLength,OutputBuffer,OutputBufferLength,
           InternalDeviceIoControl,Event,IoStatusBlock);
   
    /* Allocate IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE))) return Irp;
   
    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    
    /* Set the DevCtl Type */
    StackPtr->MajorFunction = InternalDeviceIoControl ? 
                              IRP_MJ_INTERNAL_DEVICE_CONTROL : IRP_MJ_DEVICE_CONTROL;
   
    /* Set the IOCTL Data */
    StackPtr->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    StackPtr->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

    /* Handle the Methods */
    switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
    {
        case METHOD_BUFFERED:
        DPRINT("Using METHOD_BUFFERED!\n");
      
        /* Select the right Buffer Length */
        BufferLength = InputBufferLength > OutputBufferLength ? InputBufferLength : OutputBufferLength;
        
        /* Make sure there is one */
        if (BufferLength)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, 
                                                                    BufferLength, 
                                                                    TAG_SYS_BUF);

            /* Fail if we couldn't */
            if (Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                IoFreeIrp(Irp);
                return(NULL);
            }
          
            /* Check if we got a buffer */
            if (InputBuffer)
            {
                /* Copy into the System Buffer */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, 
                              InputBuffer, 
                              InputBufferLength);
            }
        
            /* Write the flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            if (OutputBuffer) Irp->Flags |= IRP_INPUT_OPERATION;
            
            /* Save the Buffer */
            Irp->UserBuffer = OutputBuffer;
        }
        else
        {
            /* Clear the Flags and Buffer */
            Irp->Flags = 0;
            Irp->UserBuffer = NULL;
        }
        break;

        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
        DPRINT("Using METHOD_IN/OUT DIRECT!\n");

        /* Check if we got an input buffer */
        if (InputBuffer)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, 
                                                                    InputBufferLength, 
                                                                    TAG_SYS_BUF);

            /* Fail if we couldn't */
            if (Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                IoFreeIrp(Irp);
                return(NULL);
            }
            
            /* Copy into the System Buffer */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, 
                          InputBuffer, 
                          InputBufferLength);
        
            /* Write the flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        }
        else
        {
            Irp->Flags = 0;
        }
        
        /* Check if we got an output buffer */
        if (OutputBuffer)
        {
            /* Allocate the System Buffer */
            Irp->MdlAddress = IoAllocateMdl(OutputBuffer,
                                            OutputBufferLength,
                                            FALSE,
                                            FALSE,
                                            Irp);

            /* Fail if we couldn't */
            if (Irp->MdlAddress == NULL)
            {
                IoFreeIrp(Irp);
                return NULL;
            }
            
            /* Probe and Lock */
            _SEH_FILTER(FreeAndGoOn) 
            {
                /* Free the MDL and IRP */
                IoFreeMdl(Irp->MdlAddress);
                IoFreeIrp(Irp);
                return EXCEPTION_CONTINUE_SEARCH;
            } 
            _SEH_TRY_FILTER(FreeAndGoOn) 
            {
                /* Use the right Access Type */
                if (IO_METHOD_FROM_CTL_CODE(IoControlCode) == METHOD_IN_DIRECT)
                {
                    AccessType = IoReadAccess;
                }
                else
                {
                    AccessType = IoWriteAccess;
                }
                
                /* Do the probe */
                MmProbeAndLockPages(Irp->MdlAddress, KernelMode, AccessType);
            }
            _SEH_HANDLE
            {
                return NULL;
            }
            _SEH_END;
        }
        break;

        case METHOD_NEITHER:

        /* Just save the Buffer */
        Irp->UserBuffer = OutputBuffer;
        StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
        break;
    }
    
    /* Now write the Event and IoSB */
    if (!IoStatusBlock) KEBUGCHECK(0); /* Temporary to catch illegal ROS Drivers */
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;

    /* Sync IRPs are queued to requestor thread's irp cancel/cleanup list */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp(Irp);
    
    /* Return the IRP */
    return Irp;
}

/*
 * @implemented
 *
 * FUNCTION: Allocates and builds an IRP to be sent synchronously to lower
 * level driver(s)
 * ARGUMENTS:
 *        MajorFunction = Major function code, one of IRP_MJ_READ,
 *                        IRP_MJ_WRITE, IRP_MJ_FLUSH_BUFFERS, IRP_MJ_SHUTDOWN
 *        DeviceObject = Target device object
 *        Buffer = Buffer containing data for a read or write
 *        Length = Length in bytes of the information to be transferred
 *        StartingOffset = Offset to begin the read/write from
 *        Event (OUT) = Will be set when the operation is complete
 *        IoStatusBlock (OUT) = Set to the status of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
PIRP 
STDCALL
IoBuildSynchronousFsdRequest(ULONG MajorFunction,
                             PDEVICE_OBJECT DeviceObject,
                             PVOID Buffer,
                             ULONG Length,
                             PLARGE_INTEGER StartingOffset,
                             PKEVENT Event,
                             PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP Irp;
   
    DPRINT("IoBuildSynchronousFsdRequest(MajorFunction %x, DeviceObject %x, "
           "Buffer %x, Length %x, StartingOffset %x, Event %x, "
           "IoStatusBlock %x\n",MajorFunction,DeviceObject,Buffer,Length,
            StartingOffset,Event,IoStatusBlock);
   
    /* Do the big work to set up the IRP */
    Irp = IoBuildAsynchronousFsdRequest(MajorFunction,
                                        DeviceObject,
                                        Buffer,
                                        Length,
                                        StartingOffset,
                                        IoStatusBlock );
    
    /* Set the Event which makes it Syncronous */
    Irp->UserEvent = Event;

    /* Sync IRPs are queued to requestor thread's irp cancel/cleanup list */
    IoQueueThreadIrp(Irp);
    return(Irp);
}

/*
 * @implemented
 */
BOOLEAN 
STDCALL 
IoCancelIrp(PIRP Irp)
{
   KIRQL oldlvl;
   PDRIVER_CANCEL CancelRoutine;
   
   DPRINT("IoCancelIrp(Irp %x)\n",Irp);
   
   IoAcquireCancelSpinLock(&oldlvl);
   
   Irp->Cancel = TRUE;
  
   CancelRoutine = IoSetCancelRoutine(Irp, NULL);
   if (CancelRoutine == NULL)
   {
      IoReleaseCancelSpinLock(oldlvl);
      return(FALSE);
   }
   
   Irp->CancelIrql = oldlvl;
   CancelRoutine(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
   return(TRUE);
}

/**
 * @name IoCancelThreadIo
 *
 * Cancel all pending I/O request associated with specified thread.
 *
 * @param Thread
 *        Thread to cancel requests for.
 */

VOID 
STDCALL
IoCancelThreadIo(PETHREAD Thread)
{
   PLIST_ENTRY IrpEntry;
   PIRP Irp;
   KIRQL OldIrql;
   ULONG Retries = 3000;
   LARGE_INTEGER Interval;

   OldIrql = KfRaiseIrql(APC_LEVEL);

   /*
    * Start by cancelling all the IRPs in the current thread queue.
    */

   for (IrpEntry = Thread->IrpList.Flink;
        IrpEntry != &Thread->IrpList;
        IrpEntry = IrpEntry->Flink)
   {
      Irp = CONTAINING_RECORD(IrpEntry, IRP, ThreadListEntry);
      IoCancelIrp(Irp);
   }

   /*
    * Wait till all the IRPs are completed or cancelled.
    */

   while (!IsListEmpty(&Thread->IrpList))
   {
      KfLowerIrql(OldIrql);

      /* Wait a short while and then look if all our IRPs were completed. */
      Interval.QuadPart = -1000000; /* 100 milliseconds */
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);

      /*
       * Don't stay here forever if some broken driver doesn't complete
       * the IRP.
       */

      if (Retries-- == 0)
      {
         /* FIXME: Handle this gracefully. */
         DPRINT1("Thread with dead IRPs!");
         ASSERT(FALSE);
      }
      
      OldIrql = KfRaiseIrql(APC_LEVEL);
   }

   KfLowerIrql(OldIrql);
}

#ifdef IoCallDriver
#undef IoCallDriver
#endif
/*
 * @implemented
 */
NTSTATUS
STDCALL
IoCallDriver (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    /* Call fast call */
    return IofCallDriver(DeviceObject, Irp);
}

/*
 * @implemented
 */
VOID 
STDCALL
IoCompleteRequest(PIRP Irp,
                  CCHAR PriorityBoost)
{
    /* Call the fastcall */
    IofCompleteRequest(Irp, PriorityBoost);
}

/*
 * @implemented
 */
VOID 
STDCALL
IoEnqueueIrp(IN PIRP Irp)
{
    IoQueueThreadIrp(Irp);
}

/*
 * @implemented
 *
 * FUNCTION: Sends an IRP to the next lower driver
 */
NTSTATUS 
FASTCALL
IofCallDriver(PDEVICE_OBJECT DeviceObject,
              PIRP Irp)

{
    PDRIVER_OBJECT DriverObject;
    PIO_STACK_LOCATION Param;

    DPRINT("IofCallDriver(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);

    /* Get the Driver Object */
    DriverObject = DeviceObject->DriverObject;
 
    /* Set the Stack Location */
    IoSetNextIrpStackLocation(Irp);
    
    /* Get the current one */
    Param = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("IrpSp 0x%X\n", Param);

    /* Get the Device Object */
    Param->DeviceObject = DeviceObject;
    
    /* Call it */
    return DriverObject->MajorFunction[Param->MajorFunction](DeviceObject, Irp);
}

#ifdef IoCompleteRequest
#undef IoCompleteRequest
#endif
/*
 * @implemented
 */
VOID FASTCALL
IofCompleteRequest(PIRP Irp,
         CCHAR PriorityBoost)
/*
 * FUNCTION: Indicates the caller has finished all processing for a given
 * I/O request and is returning the given IRP to the I/O manager
 * ARGUMENTS:
 *         Irp = Irp to be cancelled
 *         PriorityBoost = Increment by which to boost the priority of the
 *                         thread making the request
 */
{
   ULONG             i;
   NTSTATUS          Status;
   PFILE_OBJECT      OriginalFileObject;
   PDEVICE_OBJECT    DeviceObject;
   KIRQL             oldIrql;
   PMDL              Mdl;
   PIO_STACK_LOCATION Stack = (PIO_STACK_LOCATION)(Irp + 1);

   DPRINT("IoCompleteRequest(Irp %x, PriorityBoost %d) Event %x THread %x\n",
      Irp,PriorityBoost, Irp->UserEvent, PsGetCurrentThread());

   ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
   ASSERT(Irp->CancelRoutine == NULL);
   ASSERT(Irp->IoStatus.Status != STATUS_PENDING);

   Irp->PendingReturned = IoGetCurrentIrpStackLocation(Irp)->Control & SL_PENDING_RETURNED;
   
   /*
    * Run the completion routines.
    */

   for (i=Irp->CurrentLocation;i<(ULONG)Irp->StackCount;i++)
   {
      /*
      Completion routines expect the current irp stack location to be the same as when
      IoSetCompletionRoutine was called to set them. A side effect is that completion
      routines set by highest level drivers without their own stack location will receive
      an invalid current stack location (at least it should be considered as invalid).
      Since the DeviceObject argument passed is taken from the current stack, this value
      is also invalid (NULL).
      */
      if (Irp->CurrentLocation < Irp->StackCount - 1)
      {
         IoSkipCurrentIrpStackLocation(Irp);
         DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;
      }
      else
      {
         DeviceObject = NULL;
      }

      if (Stack[i].CompletionRoutine != NULL &&
         ((NT_SUCCESS(Irp->IoStatus.Status) && (Stack[i].Control & SL_INVOKE_ON_SUCCESS)) ||
         (!NT_SUCCESS(Irp->IoStatus.Status) && (Stack[i].Control & SL_INVOKE_ON_ERROR)) ||
         (Irp->Cancel && (Stack[i].Control & SL_INVOKE_ON_CANCEL))))
      {
         Status = Stack[i].CompletionRoutine(DeviceObject,
                                                  Irp,
                                                  Stack[i].Context);

         if (Status == STATUS_MORE_PROCESSING_REQUIRED)
         {
            return;
         }
      }
   
      if (IoGetCurrentIrpStackLocation(Irp)->Control & SL_PENDING_RETURNED)
      {
         Irp->PendingReturned = TRUE;
      }
   }

   /* Windows NT File System Internals, page 165 */
   if (Irp->Flags & IRP_ASSOCIATED_IRP)
   {
      ULONG MasterIrpCount;
      PIRP MasterIrp = Irp->AssociatedIrp.MasterIrp;

      /* This should never happen! */
      ASSERT(IsListEmpty(&Irp->ThreadListEntry));

      MasterIrpCount = InterlockedDecrement(&MasterIrp->AssociatedIrp.IrpCount);
      while ((Mdl = Irp->MdlAddress))
      {
         Irp->MdlAddress = Mdl->Next;
         IoFreeMdl(Mdl);
      }
      IoFreeIrp(Irp);
      if (MasterIrpCount == 0)
      {
         IofCompleteRequest(MasterIrp, IO_NO_INCREMENT);
      }
      return;
   }

   /*
    * Were done calling completion routines. Now do any cleanup that can be 
    * done in an arbitrarily context.
    */

   /* Windows NT File System Internals, page 165 */
   if (Irp->Flags & (IRP_PAGING_IO|IRP_CLOSE_OPERATION))
   {
      /* This should never happen! */
      ASSERT(IsListEmpty(&Irp->ThreadListEntry));

      /* 
       * If MDL_IO_PAGE_READ is set, then the caller is responsible 
       * for deallocating of the mdl. 
       */
      if (Irp->Flags & IRP_PAGING_IO &&
          Irp->MdlAddress &&
          !(Irp->MdlAddress->MdlFlags & MDL_IO_PAGE_READ))
      {

         if (Irp->MdlAddress->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
         {
            MmUnmapLockedPages(Irp->MdlAddress->MappedSystemVa, Irp->MdlAddress);
         }

         ExFreePool(Irp->MdlAddress);
      }

      if (Irp->UserIosb)
      {
         _SEH_TRY
         {
            *Irp->UserIosb = Irp->IoStatus;
         }
         _SEH_HANDLE
         {
           DPRINT1("Unable to set UserIosb (at 0x%x) to 0x%x, Error: 0x%x\n",
                   Irp->UserIosb, Irp->IoStatus, _SEH_GetExceptionCode());
         }
         _SEH_END;
      }

      if (Irp->UserEvent)
      {
         KeSetEvent(Irp->UserEvent, PriorityBoost, FALSE);
      }
      
      /* io manager frees the irp for close operations */
//      if (Irp->Flags & IRP_PAGING_IO)
//      {
         IoFreeIrp(Irp);
//      }
      
      return;
   }
   
   
   /*
   Hi Dave,
   
    I went through my old notes. You are correct and in most cases
   IoCompleteRequest() will issue an MmUnlockPages() for each MDL in the IRP
   chain. There are however few exceptions: one is MDLs for associated IRPs,
   it's expected that those MDLs have been initialized with
   IoBuildPartialMdl(). Another exception is PAGING_IO irps, the i/o completion
   code doesn't do anything to MDLs of those IRPs.
   
   sara
   
*/
   

   for (Mdl = Irp->MdlAddress; Mdl; Mdl = Mdl->Next)
   {
      /* 
       * Undo the MmProbeAndLockPages. If MmGetSystemAddressForMdl was called
       * on this mdl, this mapping (if any) is also undone by MmUnlockPages.
       */
      MmUnlockPages(Mdl);
   }
    
   //Windows NT File System Internals, page 154
   OriginalFileObject = Irp->Tail.Overlay.OriginalFileObject;

   if (Irp->PendingReturned || KeGetCurrentIrql() == DISPATCH_LEVEL)
   {
      BOOLEAN bStatus;
      
      DPRINT("Dispatching APC\n");

      KeInitializeApc(  &Irp->Tail.Apc,
                             &Irp->Tail.Overlay.Thread->Tcb,
                             Irp->ApcEnvironment,
                             IoSecondStageCompletion,//kernel routine
                             NULL,
                             (PKNORMAL_ROUTINE) NULL,
                             KernelMode,
                             NULL);
      
      bStatus = KeInsertQueueApc(&Irp->Tail.Apc,
                                      (PVOID)OriginalFileObject,
                                      NULL, // This is used for REPARSE stuff
                                      PriorityBoost);

      if (bStatus == FALSE)
      {
         DPRINT1("Error queueing APC for thread. Thread has probably exited.\n");
      }

      DPRINT("Finished dispatching APC\n");
   }
   else
   {
      DPRINT("Calling IoSecondStageCompletion routine directly\n");
      KeRaiseIrql(APC_LEVEL, &oldIrql);
      IoSecondStageCompletion(&Irp->Tail.Apc,NULL,NULL,(PVOID)&OriginalFileObject, NULL);
      KeLowerIrql(oldIrql);
      DPRINT("Finished completition routine\n");
   }
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
IoForwardIrpSynchronously(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 *
 * FUNCTION: Releases a caller allocated irp
 * ARGUMENTS:
 *      Irp = Irp to free
 */
VOID 
STDCALL
IoFreeIrp(PIRP Irp)
{
    /* Free the pool memory associated with it */
    ExFreePool(Irp);
}

/*
 * @implemented
 */
PEPROCESS STDCALL
IoGetRequestorProcess(IN PIRP Irp)
{
    return(Irp->Tail.Overlay.Thread->ThreadsProcess);
}

/*
 * @implemented
 */
ULONG
STDCALL
IoGetRequestorProcessId(IN PIRP Irp)
{
    return (ULONG)(IoGetRequestorProcess(Irp)->UniqueProcessId);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoGetRequestorSessionId(IN PIRP Irp,
                        OUT PULONG pSessionId)
{
    *pSessionId = IoGetRequestorProcess(Irp)->SessionId;
    
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PIRP 
STDCALL
IoGetTopLevelIrp(VOID)
{
    return(PsGetCurrentThread()->TopLevelIrp);
}

/*
 * @implemented
 *
 * FUNCTION: Initalizes an irp allocated by the caller
 * ARGUMENTS:
 *          Irp = IRP to initalize
 *          PacketSize = Size in bytes of the IRP
 *          StackSize = Number of stack locations in the IRP
 */
VOID 
STDCALL
IoInitializeIrp(PIRP Irp,
                USHORT PacketSize,
                CCHAR StackSize)
{
    ASSERT(Irp != NULL);

    DPRINT("IoInitializeIrp(StackSize %x, Irp %x)\n",StackSize, Irp);
    
    /* Clear it */
    RtlZeroMemory(Irp, PacketSize);
    
    /* Set the Header and other data */
    Irp->Type = IO_TYPE_IRP;
    Irp->Size = PacketSize;
    Irp->StackCount = StackSize;
    Irp->CurrentLocation = StackSize;
    Irp->ApcEnvironment =  KeGetCurrentThread()->ApcStateIndex;
    Irp->Tail.Overlay.CurrentStackLocation = (PIO_STACK_LOCATION)(Irp + 1) + StackSize;
    
    /* Initialize the Thread List */
    InitializeListHead(&Irp->ThreadListEntry);
  
    DPRINT("Irp->Tail.Overlay.CurrentStackLocation %x\n", Irp->Tail.Overlay.CurrentStackLocation);    
}

/*
 * NAME							EXPORTED
 * 	IoIsOperationSynchronous@4
 *
 * DESCRIPTION
 *	Check if the I/O operation associated with the given IRP
 *	is synchronous.
 *
 * ARGUMENTS
 * 	Irp 	Packet to check.
 *
 * RETURN VALUE
 * 	TRUE if Irp's operation is synchronous; otherwise FALSE.
 *
 * @implemented
 */
BOOLEAN 
STDCALL
IoIsOperationSynchronous(IN PIRP Irp)
{
    /* Check the flags */
    if ((Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) ||
        (Irp->Flags & IRP_SYNCHRONOUS_API) ||  
        (IoGetCurrentIrpStackLocation(Irp)->FileObject->Flags & 
        FO_SYNCHRONOUS_IO))
    {
        /* Synch API or Paging I/O is OK, as is Sync File I/O */
        return TRUE;
    }

    /* Otherwise, it is an asynchronous operation. */
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
IoIsValidNameGraftingBuffer(IN PIRP Irp,
                            IN PREPARSE_DATA_BUFFER ReparseBuffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 *
 * FUNCTION: Allocates and initializes an irp to associated with a master irp
 * ARGUMENTS:
 *       Irp = Master irp
 *       StackSize = Number of stack locations to be allocated in the irp
 * RETURNS: The irp allocated
 * NOTE: The caller is responsible for incrementing
 *       Irp->AssociatedIrp.IrpCount.
 */
PIRP 
STDCALL
IoMakeAssociatedIrp(PIRP Irp,
                    CCHAR StackSize)
{
   PIRP AssocIrp;

   /* Allocate the IRP */
   AssocIrp = IoAllocateIrp(StackSize,FALSE);
   if (AssocIrp == NULL) return NULL;

   /* Set the Flags */
   AssocIrp->Flags |= IRP_ASSOCIATED_IRP;

   /* Set the Thread */
   AssocIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;

   /* Associate them */
   AssocIrp->AssociatedIrp.MasterIrp = Irp;
 
   return AssocIrp;
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL 
IoPageRead(PFILE_OBJECT FileObject,
           PMDL Mdl,
           PLARGE_INTEGER Offset,
           PKEVENT Event,
           PIO_STATUS_BLOCK StatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
   
    DPRINT("IoPageRead(FileObject %x, Mdl %x)\n",
           FileObject, Mdl);
           
    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    
    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    
    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    
    /* Create the IRP Settings */
    Irp->MdlAddress = Mdl;
    Irp->UserBuffer = MmGetMdlVirtualAddress(Mdl);
    Irp->UserIosb = StatusBlock;
    Irp->UserEvent = Event;
    Irp->RequestorMode = KernelMode;
    Irp->Flags = IRP_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_PAGING_IO | IRP_INPUT_OPERATION;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    
    /* Set the Stack Settings */
    StackPtr->Parameters.Read.Length = MmGetMdlByteCount(Mdl);
    StackPtr->Parameters.Read.ByteOffset = *Offset;
    StackPtr->MajorFunction = IRP_MJ_READ;
    StackPtr->FileObject = FileObject;
    
    /* Call the Driver */
    return IofCallDriver(DeviceObject, Irp);
}

/*
 * @implemented
 */
VOID 
STDCALL
IoQueueThreadIrp(IN PIRP Irp)
{
    KIRQL OldIrql;
  
    /* Raise to APC */
    OldIrql = KfRaiseIrql(APC_LEVEL);
  
   /*
    * Synchronous irp's are queued to requestor thread. If they are not
    * completed when the thread exits, they are canceled (cleaned up).
    * - Gunnar
    */
    InsertTailList(&Irp->Tail.Overlay.Thread->IrpList, &Irp->ThreadListEntry);
    
    /* Lower back */
    KfLowerIrql(OldIrql);
}

/*
 * @implemented
 * Reference: Chris Cant's "Writing WDM Device Drivers" 
 */
VOID 
STDCALL
IoReuseIrp(IN OUT PIRP Irp,
           IN NTSTATUS Status)
{
    UCHAR AllocationFlags;
       
    /* Get the old flags */
    AllocationFlags = Irp->AllocationFlags;
    
    /* Reinitialize the IRP */
    IoInitializeIrp(Irp, Irp->Size, Irp->StackCount);
    
    /* Duplicate the data */
    Irp->IoStatus.Status = Status;
    Irp->AllocationFlags = AllocationFlags;
}

/*
 * @implemented
 */
VOID
STDCALL
IoSetTopLevelIrp(IN PIRP Irp)
{
    PsGetCurrentThread()->TopLevelIrp = Irp;
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL 
IoSynchronousPageWrite(PFILE_OBJECT FileObject,
                       PMDL Mdl,
                       PLARGE_INTEGER Offset,
                       PKEVENT Event,
                       PIO_STATUS_BLOCK StatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
   
    DPRINT("IoSynchronousPageWrite(FileObject %x, Mdl %x)\n",
            FileObject, Mdl);
           
    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    
    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    
    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    
    /* Create the IRP Settings */
    Irp->MdlAddress = Mdl;
    Irp->UserBuffer = MmGetMdlVirtualAddress(Mdl);
    Irp->UserIosb = StatusBlock;
    Irp->UserEvent = Event;
    Irp->RequestorMode = KernelMode;
    Irp->Flags = IRP_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_PAGING_IO;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    
    /* Set the Stack Settings */
    StackPtr->Parameters.Write.Length = MmGetMdlByteCount(Mdl);
    StackPtr->Parameters.Write.ByteOffset = *Offset;
    StackPtr->MajorFunction = IRP_MJ_WRITE;
    StackPtr->FileObject = FileObject;
    
    /* Call the Driver */
    return IofCallDriver(DeviceObject, Irp);
}

VOID 
STDCALL
IoSecondStageCompletion_KernelApcRoutine(PKAPC Apc,
                                         PKNORMAL_ROUTINE *NormalRoutine,
                                         PVOID *NormalContext,
                                         PVOID *SystemArgument1,
                                         PVOID *SystemArgument2)
{
    /* Free the IRP */
    IoFreeIrp(CONTAINING_RECORD(Apc, IRP, Tail.Apc));
}

VOID 
STDCALL
IoSecondStageCompletion_RundownApcRoutine(PKAPC Apc)
{
    /* Free the IRP */
    IoFreeIrp(CONTAINING_RECORD(Apc, IRP, Tail.Apc));
}

/*
 * FUNCTION: Performs the second stage of irp completion for read/write irps
 * 
 * Called as a special kernel APC kernel-routine or directly from IofCompleteRequest()
 *
 * Note that we'll never see irp's flagged IRP_PAGING_IO (IRP_MOUNT_OPERATION)
 * or IRP_CLOSE_OPERATION (IRP_MJ_CLOSE and IRP_MJ_CLEANUP) here since their
 * cleanup/completion is fully taken care of in IoCompleteRequest.
 * -Gunnar
 */
VOID 
STDCALL
IoSecondStageCompletion(PKAPC Apc,
                        PKNORMAL_ROUTINE* NormalRoutine,
                        PVOID* NormalContext,
                        PVOID* SystemArgument1,
                        PVOID* SystemArgument2)
{
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PMDL Mdl, NextMdl;

    if (Apc) DPRINT("IoSecondStageCompletition with APC: %x\n", Apc);
   
    /* Get data from the APC */
    FileObject = (PFILE_OBJECT)(*SystemArgument1);
    Irp = CONTAINING_RECORD(Apc, IRP, Tail.Apc);
    DPRINT("IoSecondStageCompletition, %x\n", Irp);
     
    /* Handle Buffered case first */
    if (Irp->Flags & IRP_BUFFERED_IO)
    {
        /* Check if we have an input buffer and if we suceeded */
        if (Irp->Flags & IRP_INPUT_OPERATION && NT_SUCCESS(Irp->IoStatus.Status))
        {
            /* Copy the buffer back to the user */
            RtlCopyMemory(Irp->UserBuffer,
                          Irp->AssociatedIrp.SystemBuffer,
                          Irp->IoStatus.Information);
        }
        
        /* Also check if we should de-allocate it */
        if (Irp->Flags & IRP_DEALLOCATE_BUFFER)
        {
            ExFreePool(Irp->AssociatedIrp.SystemBuffer);
        }
    }
    
    /* Now we got rid of these two... */
    Irp->Flags &= ~(IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
    
    /* Check if there's an MDL */
    if ((Mdl = Irp->MdlAddress))
    {
        /* Clear all of them */
        do
        {
            NextMdl = Mdl->Next;
            IoFreeMdl(Mdl);
            Mdl = NextMdl;
        } while (Mdl);
    }
    Irp->MdlAddress = NULL;
    
    /* Check for Success but allow failure for Async IRPs */
    if (NT_SUCCESS(Irp->IoStatus.Status) || 
        (Irp->PendingReturned &&
        !(Irp->Flags & IRP_SYNCHRONOUS_API) &&
        (FileObject == NULL || FileObject->Flags & FO_SYNCHRONOUS_IO)))
    {    
        /*  Save the IOSB Information */
        *Irp->UserIosb = Irp->IoStatus;
    
        /* Check if there's an event */
        if (Irp->UserEvent)
        {
            /* Signal the Event */
            KeSetEvent(Irp->UserEvent, 0, FALSE);
    
        }
        else if (FileObject)
        {
            /* Signal the File Object Instead */
            KeSetEvent(&FileObject->Event, 0, FALSE);
        
            /* Set the Status */
            FileObject->FinalStatus = Irp->IoStatus.Status;
        }
    
        /* Check if there's a File Object */
        if (FileObject)
        {
            /* Dereference the Event if this is an ASYNC IRP */
            if (!Irp->Flags & IRP_SYNCHRONOUS_API)
            {
                if (Irp->UserEvent != &FileObject->Event)
                {
                    ObDereferenceObject(Irp->UserEvent);
                }
            }
            
            /* If the File Object is SYNC, then we need to signal its event too */
            if (FileObject->Flags & FO_SYNCHRONOUS_IO)
            {
                /* Signal Event */

                KeSetEvent(&FileObject->Event, 0, FALSE);
                
                /* Set the Status */
                FileObject->FinalStatus = Irp->IoStatus.Status;
            }
        }
    
        /* Remove the IRP from the list of Thread Pending IRPs */
        RemoveEntryList(&Irp->ThreadListEntry);
        InitializeListHead(&Irp->ThreadListEntry);  
    
        /* Now call the User APC if one was requested */
        if (Irp->Overlay.AsynchronousParameters.UserApcRoutine != NULL)
        {
            KeInitializeApc(&Irp->Tail.Apc,
                            KeGetCurrentThread(),
                            CurrentApcEnvironment,
                            IoSecondStageCompletion_KernelApcRoutine,
                            IoSecondStageCompletion_RundownApcRoutine,
                            (PKNORMAL_ROUTINE)Irp->Overlay.AsynchronousParameters.UserApcRoutine,
                            Irp->RequestorMode,
                            Irp->Overlay.AsynchronousParameters.UserApcContext);

            KeInsertQueueApc(&Irp->Tail.Apc,
                             Irp->UserIosb,
                             NULL,
                             2);
        }
        else if (FileObject && FileObject->CompletionContext)
        {
            /* Call the IO Completion Port if we have one, instead */
            IoSetIoCompletion(FileObject->CompletionContext->Port,
                              FileObject->CompletionContext->Key,
                              Irp->Overlay.AsynchronousParameters.UserApcContext,
                              Irp->IoStatus.Status,
                              Irp->IoStatus.Information,
                              FALSE);
        }
        else
        {
            /* Don't have anything, free it */
            IoFreeIrp(Irp);
        }
        
        /* Dereference the File Object */
        if (FileObject) ObDereferenceObject(FileObject);
    }
    else
    {   
        /* Remove the IRP from the list of Thread Pending IRPs */
        RemoveEntryList(&Irp->ThreadListEntry);
        InitializeListHead(&Irp->ThreadListEntry);  
        
        /* Signal the Events only if PendingReturned and we have a File Object */
        if (FileObject && Irp->PendingReturned)
        {
            /* Check for SYNC IRP */
            if (Irp->Flags & IRP_SYNCHRONOUS_API)
            {
                /* Set the status in this case only */
                *Irp->UserIosb = Irp->IoStatus;
            
                /* Signal our event if we have one */
                if (Irp->UserEvent)
                {
                    KeSetEvent(Irp->UserEvent, 0, FALSE);
                }
                else
                {
                    /* Signal the File's Event instead */
                    KeSetEvent(&FileObject->Event, 0, FALSE);
                }
            }
            else
            {
                /* We'll report the Status to the File Object, not the IRP */
                FileObject->FinalStatus = Irp->IoStatus.Status;
                
                /* Signal the File Object ONLY if this was Async */
                KeSetEvent(&FileObject->Event, 0, FALSE);
            }
        }
        
        /* Dereference the Event if it's an ASYNC IRP on a File Object */
        if (Irp->UserEvent && !(Irp->Flags & IRP_SYNCHRONOUS_API) && FileObject)
        {
            if (Irp->UserEvent != &FileObject->Event)
            {
                ObDereferenceObject(Irp->UserEvent);
            }   
        }
        
        /* Dereference the File Object */
        if (FileObject) ObDereferenceObject(FileObject);
        
        /* Free the IRP */
        IoFreeIrp(Irp);
    }
}

/* EOF */
