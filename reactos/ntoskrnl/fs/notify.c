/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/notify.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     Gunnar Dalsnes
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


PAGED_LOOKASIDE_LIST    NotifyEntryLookaside;

typedef struct _NOTIFY_ENTRY
{
   LIST_ENTRY ListEntry;
   PSTRING FullDirectoryName;
   BOOLEAN WatchTree;
   BOOLEAN IgnoreBuffer;
   ULONG CompletionFilter;
   LIST_ENTRY IrpQueue;
   PVOID Fcb;
   PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback;
   PSECURITY_SUBJECT_CONTEXT SubjectContext;
   PVOID FsContext;
   LIST_ENTRY BufferedChangesList;
} NOTIFY_ENTRY, *PNOTIFY_ENTRY;

typedef struct _BUFFERED_CHANGE
{
   LIST_ENTRY ListEntry;
   ULONG Action;
   USHORT NameLen;
   WCHAR RelativeName[1];
   
} BUFFERED_CHANGE, *PBUFFERED_CHANGE;


/**********************************************************************
 * NAME                    PRIVATE
 * FsRtlpInitNotifyImplementation
 *
 */
VOID
STDCALL INIT_FUNCTION
FsRtlpInitNotifyImplementation(VOID)
{
   ExInitializePagedLookasideList( &NotifyEntryLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(NOTIFY_ENTRY),
                                    0 /* FSRTL_NOTIFY_TAG*/,
                                    0
                                    );

   
}





/**********************************************************************
 * NAME                    PRIVATE
 * FsRtlpNotifyCancelRoutine
 *
 */
static
VOID
STDCALL 
FsRtlpNotifyCancelRoutine(
   IN PDEVICE_OBJECT DeviceObject, 
   IN PIRP Irp
   )
{
   PFAST_MUTEX Lock;

   //don't need this since we have our own sync. protecting irp cancellation
   IoReleaseCancelSpinLock(Irp->CancelIrql); 

   Lock = (PFAST_MUTEX)Irp->Tail.Overlay.DriverContext[3];

   ExAcquireFastMutex(Lock );
   
   RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
   
   ExReleaseFastMutex(Lock);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
}



static
PNOTIFY_ENTRY
FASTCALL
FsRtlpFindNotifyEntry(
   PLIST_ENTRY NotifyList,
   PVOID FsContext
   )
{
   PLIST_ENTRY EnumEntry;
   PNOTIFY_ENTRY NotifyEntry;
   
   LIST_FOR_EACH(EnumEntry, NotifyList)
   {
      NotifyEntry = CONTAINING_RECORD(EnumEntry, NOTIFY_ENTRY, ListEntry);
      
      if (NotifyEntry->FsContext == FsContext)
      {
         return NotifyEntry;
      }
   }

   return NULL;   
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyChangeDirectory@28
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
VOID
STDCALL
FsRtlNotifyChangeDirectory (
    	IN	PNOTIFY_SYNC NotifySync,
    	IN	PVOID        FsContext,
    	IN	PSTRING      FullDirectoryName,
    	IN	PLIST_ENTRY  NotifyList,
    	IN	BOOLEAN      WatchTree,
    	IN	ULONG        CompletionFilter,
    	IN	PIRP         NotifyIrp
	)
{
	FsRtlNotifyFullChangeDirectory (
		NotifySync,
		NotifyList,
		FsContext,
		FullDirectoryName,
		WatchTree,
		TRUE, /* IgnoreBuffer */
		CompletionFilter,
		NotifyIrp,
		NULL,
		NULL
		);
}



/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyCleanup@12
 *
 * DESCRIPTION
 *	 Called by FSD when all handles to FileObject (identified by FsContext) are closed
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
VOID
STDCALL
FsRtlNotifyCleanup (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PVOID		FsContext
	)
{
   PNOTIFY_ENTRY NotifyEntry;
   LIST_ENTRY CompletedListHead;
   PLIST_ENTRY TmpEntry;
   PBUFFERED_CHANGE BufferedChange;
   PIRP Irp;
   
   InitializeListHead(&CompletedListHead);
   
   ExAcquireFastMutex((PFAST_MUTEX)NotifySync);   
   
   NotifyEntry = FsRtlpFindNotifyEntry(NotifyList, FsContext);
   
   if (NotifyEntry)
   {
      /* free buffered changes */
      while (!IsListEmpty(&NotifyEntry->BufferedChangesList))
      {
         TmpEntry = RemoveHeadList(&NotifyEntry->BufferedChangesList);
         BufferedChange = CONTAINING_RECORD(TmpEntry , BUFFERED_CHANGE, ListEntry);
         ExFreePool(BufferedChange);
      }

      /* cancel(?) pending irps */
      while (!IsListEmpty(&NotifyEntry->IrpQueue))
      {
         TmpEntry = RemoveHeadList(&NotifyEntry->IrpQueue);
         Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);

         /* irp cancelation bolilerplate */
         if (!IoSetCancelRoutine(Irp, NULL))
         {  
            //The cancel routine will be called. When we release the lock it will complete the irp.
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
            continue;
         }

         Irp->IoStatus.Status = STATUS_NOTIFY_CLEANUP; /* FIXME: correct status? */
         Irp->IoStatus.Information = 0;

         /* avoid holding lock while completing irp */
         InsertTailList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
      }
         
      /* Unlink and free the NotifyStruct */
      RemoveEntryList(&NotifyEntry->ListEntry);
      ExFreeToPagedLookasideList(&NotifyEntryLookaside, NotifyEntry);
   }
   
   ExReleaseFastMutex((PFAST_MUTEX)NotifySync);
   
   /* complete defered irps */
   while (!IsListEmpty(&CompletedListHead)) 
   {
      TmpEntry = RemoveHeadList(&CompletedListHead);
      Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);     
   }
   
}


/*
 * @unimplemented
 */
VOID
STDCALL
FsRtlNotifyFilterChangeDirectory (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PVOID FsContext,
    IN PSTRING FullDirectoryName,
    IN BOOLEAN WatchTree,
    IN BOOLEAN IgnoreBuffer,
    IN ULONG CompletionFilter,
    IN PIRP NotifyIrp,
    IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL,
    IN PFILTER_REPORT_CHANGE FilterCallback OPTIONAL
    )
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
FsRtlNotifyFilterReportChange (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PSTRING FullTargetName,
    IN USHORT TargetNameOffset,
    IN PSTRING StreamName OPTIONAL,
    IN PSTRING NormalizedParentName OPTIONAL,
    IN ULONG FilterMatch,
    IN ULONG Action,
    IN PVOID TargetContext,
    IN PVOID FilterContext
    )
{
	UNIMPLEMENTED;
}



static
VOID
FASTCALL
FsRtlpWatchedDirectoryWasDeleted(
   IN PNOTIFY_SYNC         NotifySync,
   IN PLIST_ENTRY       NotifyList,
   IN PVOID          Fcb
   )
{
   LIST_ENTRY CompletedListHead;
   PLIST_ENTRY EnumEntry, TmpEntry;
   PNOTIFY_ENTRY NotifyEntry;
   PBUFFERED_CHANGE BufferedChange;
   PIRP Irp;
   
   InitializeListHead(&CompletedListHead);
   
   ExAcquireFastMutex((PFAST_MUTEX)NotifySync);

   LIST_FOR_EACH_SAFE(EnumEntry, NotifyList, NotifyEntry, NOTIFY_ENTRY, ListEntry )
   {
      if (NotifyEntry->Fcb == Fcb)
      {
         RemoveEntryList(&NotifyEntry->ListEntry);         
         
         /* free buffered changes */
         while (!IsListEmpty(&NotifyEntry->BufferedChangesList))
         {
            TmpEntry = RemoveHeadList(&NotifyEntry->BufferedChangesList);
            BufferedChange = CONTAINING_RECORD(TmpEntry , BUFFERED_CHANGE, ListEntry);
            ExFreePool(BufferedChange);
         }
         
         /* cancel(?) pending irps */
         while (!IsListEmpty(&NotifyEntry->IrpQueue))
         {
            TmpEntry = RemoveHeadList(&NotifyEntry->IrpQueue);
            Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);

            /* irp cancelation bolilerplate */
            if (!IoSetCancelRoutine(Irp, NULL))
            {  
               //The cancel routine will be called. When we release the lock it will complete the irp.
               InitializeListHead(&Irp->Tail.Overlay.ListEntry);
               continue;
            }
            
            Irp->IoStatus.Status = STATUS_NOTIFY_CLEANUP; /* FIXME: correct status? */
            Irp->IoStatus.Information = 0;
         
            /* avoid holding lock while completing irp */
            InsertTailList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
         }
      }
   }
   
   ExReleaseFastMutex((PFAST_MUTEX)NotifySync);   
   
   /* complete defered irps */
   while (!IsListEmpty(&CompletedListHead)) 
   {
      TmpEntry = RemoveHeadList(&CompletedListHead);
      Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);     
   }
   
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyFullChangeDirectory@40
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
VOID
STDCALL
FsRtlNotifyFullChangeDirectory (
	IN	PNOTIFY_SYNC			NotifySync,
	IN	PLIST_ENTRY			NotifyList,
	IN	PVOID				FsContext,
   IN PSTRING           FullDirectoryName,
   IN BOOLEAN           WatchTree,
   IN BOOLEAN           IgnoreBuffer,
   IN ULONG          CompletionFilter,
   IN PIRP           NotifyIrp,
   IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback  OPTIONAL,
	IN	PSECURITY_SUBJECT_CONTEXT	SubjectContext		OPTIONAL
	)
{
   PIO_STACK_LOCATION IrpStack;   
   PNOTIFY_ENTRY NotifyEntry;
   PBUFFERED_CHANGE BufferedChange; 
   PLIST_ENTRY TmpEntry;
   
   if (!NotifyIrp)
   {
      /* all other params are ignored if NotifyIrp == NULL */
      FsRtlpWatchedDirectoryWasDeleted(NotifySync, NotifyList, FsContext);
      return;
   }
   
   ExAcquireFastMutex((PFAST_MUTEX)NotifySync);
   
   IrpStack = IoGetCurrentIrpStackLocation(NotifyIrp);
   if (IrpStack->FileObject->Flags & FO_CLEANUP_COMPLETE)
   {
      ExReleaseFastMutex((PFAST_MUTEX)NotifySync);
      
      NotifyIrp->IoStatus.Information = 0;
      NotifyIrp->IoStatus.Status = STATUS_NOTIFY_CLEANUP;
      IoCompleteRequest(NotifyIrp, IO_NO_INCREMENT);
      return;       
   }
   
   NotifyEntry = FsRtlpFindNotifyEntry(NotifyList, FsContext);

   if (!NotifyEntry)
   {
      /* No NotifyStruct for this FileObject existed */
      
      NotifyEntry = ExAllocateFromPagedLookasideList(&NotifyEntryLookaside);
      
      NotifyEntry->FsContext = FsContext;
      NotifyEntry->FullDirectoryName = FullDirectoryName;
      NotifyEntry->WatchTree = WatchTree;
      NotifyEntry->IgnoreBuffer = IgnoreBuffer;
      NotifyEntry->CompletionFilter = CompletionFilter;
      NotifyEntry->TraverseCallback = TraverseCallback;
      NotifyEntry->SubjectContext = SubjectContext;
      NotifyEntry->Fcb = IrpStack->FileObject->FsContext;
      InitializeListHead(&NotifyEntry->IrpQueue);
      InitializeListHead(&NotifyEntry->BufferedChangesList);
      
      InsertTailList(NotifyList, &NotifyEntry->ListEntry);
   }
   
   /*
    * FIXME: this NotifyStruct allready have values for WatchTree, CompletionFilter etc.
    * What if the WatchTree, CompletionFilter etc. params are different from
    * those in the NotifyStruct? Should the params be ignored or should the params overwrite
    * the "old" values in the NotifyStruct??
    * STATUS: Currently we ignore these params for subsequesnt request. 
    *
    * -Gunnar
    */

   if (IsListEmpty(&NotifyEntry->BufferedChangesList))
   {
      /* No changes are pending. Queue the irp */

      /* Irp cancelation boilerplate */
      IoSetCancelRoutine(NotifyIrp, FsRtlpNotifyCancelRoutine);
      if (NotifyIrp->Cancel && IoSetCancelRoutine(NotifyIrp, NULL))
      {              
         //irp was canceled
         ExReleaseFastMutex((PFAST_MUTEX)NotifySync);

         NotifyIrp->IoStatus.Status = STATUS_CANCELLED;
         NotifyIrp->IoStatus.Information = 0;

         IoCompleteRequest(NotifyIrp, IO_NO_INCREMENT);
         return;
      }

      IoMarkIrpPending(NotifyIrp);

      //FIXME: any point in setting irp status/information before queueing?
      
      /* save NotifySych for use in the cancel routine */
      NotifyIrp->Tail.Overlay.DriverContext[3] = NotifySync;
      InsertTailList(&NotifyEntry->IrpQueue, &NotifyIrp->Tail.Overlay.ListEntry);

      ExReleaseFastMutex((PFAST_MUTEX)NotifySync);
      return;
   }
    

    

   /*
   typedef struct _FILE_NOTIFY_INFORMATION {
   ULONG NextEntryOffset;
   ULONG Action;
   ULONG NameLength;
   WCHAR Name[1];
   } FILE_NOTIFY_INFORMATION, *PFILE_NOTIFY_INFORMATION;
   */
   
   /* Buffered changes exist */
   
   
   /* Copy as much buffered data as available/the buffer can hold */
   while (!IsListEmpty(&NotifyEntry->BufferedChangesList))
   {
      TmpEntry = RemoveHeadList(&NotifyEntry->BufferedChangesList);
      BufferedChange = CONTAINING_RECORD(TmpEntry, BUFFERED_CHANGE, ListEntry);

       /* FIXME:
       Fill user-buffer with recorded events until full. If user buffer is too small to hold even
       a single record or can only hold some of the events, what should we do????????????
       */
       
       /* FIXME: implement this (copy data to user) */

//             BufferedChange->Action = Action;
//             RecordedChange->Name
//             RecordedChange->NameLength

      ExFreePool(BufferedChange);


   }

   ExReleaseFastMutex((PFAST_MUTEX)NotifySync);
   
   NotifyIrp->IoStatus.Information = 0; //<- FIXME
   NotifyIrp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest(NotifyIrp, IO_NO_INCREMENT);
}



static
PIRP
FASTCALL
FsRtlpGetNextIrp(PNOTIFY_ENTRY NotifyEntry)
{
   PIRP Irp;
   PLIST_ENTRY TmpEntry;   
   
   /* Loop to get a non-canceled irp */
   while (!IsListEmpty(&NotifyEntry->IrpQueue))
   {
      /* If we have queued irp(s) we can't possibly have buffered changes too */
      ASSERT(IsListEmpty(&NotifyEntry->BufferedChangesList));
         
      TmpEntry = RemoveHeadList(&NotifyEntry->IrpQueue);
      Irp = CONTAINING_RECORD(TmpEntry , IRP, Tail.Overlay.ListEntry);

      /* irp cancelation bolilerplate */
      if (!IoSetCancelRoutine(Irp, NULL))
      {  
         //The cancel routine will be called. When we release the lock it will complete the irp.
         InitializeListHead(&Irp->Tail.Overlay.ListEntry);
         continue;
      }

      /* Finally we got a non-canceled irp */
      return Irp;            
   }
   
   return NULL;
}
   
   
   
   


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyFullReportChange@36
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
VOID
STDCALL
FsRtlNotifyFullReportChange (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PSTRING		FullTargetName,
	IN	USHORT		TargetNameOffset, /* in bytes */
	IN	PSTRING		StreamName		OPTIONAL,
	IN	PSTRING		NormalizedParentName	OPTIONAL,
	IN	ULONG		FilterMatch,
	IN	ULONG		Action,
	IN	PVOID		TargetContext
	)
{
   UNICODE_STRING FullDirName;
   UNICODE_STRING TargetName;
   PLIST_ENTRY EnumEntry;
   PNOTIFY_ENTRY NotifyEntry;
   PBUFFERED_CHANGE BufferedChange;
   PIRP Irp;
   LIST_ENTRY CompletedListHead;
   
   InitializeListHead(&CompletedListHead);
   
   FullDirName.Buffer = (WCHAR*)FullTargetName->Buffer;
   FullDirName.MaximumLength = FullDirName.Length = TargetNameOffset - sizeof(WCHAR);
   
   TargetName.Buffer = (WCHAR*)(FullTargetName->Buffer + TargetNameOffset);
   TargetName.MaximumLength = TargetName.Length = FullTargetName->Length - TargetNameOffset;
   

   ExAcquireFastMutex((PFAST_MUTEX)NotifySync);   

   LIST_FOR_EACH_SAFE(EnumEntry, NotifyList, NotifyEntry, NOTIFY_ENTRY, ListEntry )
   {
      /* rule out some easy cases */
      /* FIXME: short vs. long names??? */
      if (FilterMatch != NotifyEntry->CompletionFilter) continue;
      
      if (FullDirName.Length < NotifyEntry->FullDirectoryName->Length) continue;
      
      if (!NotifyEntry->WatchTree && FullDirName.Length != NotifyEntry->FullDirectoryName->Length) continue;

      if (wcsncmp((WCHAR*)NotifyEntry->FullDirectoryName->Buffer, 
            FullDirName.Buffer, 
            NotifyEntry->FullDirectoryName->Length/sizeof(WCHAR)) != 0) continue;
      
      /* Found a valid change */
      
      if ((Irp = FsRtlpGetNextIrp(NotifyEntry)))
      {
         //FIXME: copy data to user
         
         Irp->IoStatus.Status = STATUS_SUCCESS;
         Irp->IoStatus.Information = 0;
         
         /* avoid holding lock while completing irp */
         InsertTailList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
      }
      else
      {
         /* No irp in queue. Buffer changes */
         /* FIXME: how much stuff should we buffer? 
            -Should we alloc with quotas? 
            -Should we use a hardcoded limit?
            -Should we use a time-out? (drop changes if they are not retrieved in x seconds?
         */
         BufferedChange = ExAllocatePool(PagedPool, FIELD_OFFSET(BUFFERED_CHANGE, RelativeName) + TargetName.Length);
         
         BufferedChange->Action = Action;
         BufferedChange->NameLen = TargetName.Length;
         memcpy(BufferedChange->RelativeName, TargetName.Buffer, TargetName.Length); 
         
         InsertTailList(&NotifyEntry->BufferedChangesList, &BufferedChange->ListEntry);
      }
   }

   ExReleaseFastMutex((PFAST_MUTEX)NotifySync);
   
   /* complete defered irps */
   while (!IsListEmpty(&CompletedListHead)) 
   {
      EnumEntry = RemoveHeadList(&CompletedListHead);
      Irp = CONTAINING_RECORD(EnumEntry, IRP, Tail.Overlay.ListEntry);
      
      IoCompleteRequest(Irp, IO_NO_INCREMENT);     
   }
   
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyInitializeSync@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
VOID
STDCALL
FsRtlNotifyInitializeSync (
    IN  PNOTIFY_SYNC *NotifySync
	)
{
   *NotifySync = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), 0/*FSRTL_NOTIFY_TAG*/ );
   ExInitializeFastMutex((PFAST_MUTEX)*NotifySync);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyReportChange@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
VOID
STDCALL
FsRtlNotifyReportChange (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PSTRING		FullTargetName,
	IN	PUSHORT		FileNamePartLength,
	IN	ULONG		FilterMatch
	)
{
	FsRtlNotifyFullReportChange (
		NotifySync,
		NotifyList,
		FullTargetName,
		(FullTargetName->Length - *FileNamePartLength),
		NULL,
		NULL,
		FilterMatch,
		0,
		NULL
		);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyUninitializeSync@4
 *
 * DESCRIPTION
 *	Uninitialize a NOTIFY_SYNC object.
 *
 * ARGUMENTS
 *	NotifySync is the address of a pointer
 *      to a PNOTIFY_SYNC object previously initialized by
 *      FsRtlNotifyInitializeSync().
 *
 * RETURN VALUE
 *	None.
 *
 * @implemented
 */
VOID
STDCALL
FsRtlNotifyUninitializeSync (
    IN PNOTIFY_SYNC NotifySync
	)
{
   ExFreePool (NotifySync);
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyVolumeEvent@8
 *
 * DESCRIPTION
 *	NOTE: Only present in NT 5+.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
FsRtlNotifyVolumeEvent (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		EventCode
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

/*
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
FsRtlRegisterFileSystemFilterCallbacks(IN PDRIVER_OBJECT FilterDriverObject,
                                       IN PFS_FILTER_CALLBACKS Callbacks)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
