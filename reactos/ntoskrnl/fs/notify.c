/* $Id: notify.c,v 1.10 2004/06/23 00:42:21 ion Exp $
 *
 * reactos/ntoskrnl/fs/notify.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>

#define NDEBUG
#include <internal/debug.h>

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
}


/*
 * @unimplemented
 */
STDCALL
VOID
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
STDCALL
VOID
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
	IN	PSTRING				FullDirectoryName,
	IN	BOOLEAN				WatchTree,
	IN	BOOLEAN				IgnoreBuffer,
	IN	ULONG				CompletionFilter,
	IN	PIRP				NotifyIrp,
	IN	PCHECK_FOR_TRAVERSE_ACCESS	TraverseCallback	OPTIONAL,
	IN	PSECURITY_SUBJECT_CONTEXT	SubjectContext		OPTIONAL
	)
{
#if defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER >= 1300)
	DbgPrint("%s()\n", __FUNCTION__);
#else
	DbgPrint("FsRtlNotifyFullChangeDirectory()\n");
#endif
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
	IN	USHORT		TargetNameOffset,
	IN	PSTRING		StreamName		OPTIONAL,
	IN	PSTRING		NormalizedParentName	OPTIONAL,
	IN	ULONG		FilterMatch,
	IN	ULONG		Action,
	IN	PVOID		TargetContext
	)
{
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
    IN  PNOTIFY_SYNC NotifySync
	)
{
#if 0
    *NotifySync = NULL;
    *NotifySync = ExAllocatePoolWithTag (
			0x10,			// PoolType???
			sizeof (NOTIFY_SYNC),	// NumberOfBytes = 0x28
			FSRTL_TAG
			);

    *NotifySync->Unknown0 = 1;
    *NotifySync->Unknown2 = 0;
    *NotifySync->Unknown3 = 1;
    *NotifySync->Unknown4 = 4;
    *NotifySync->Unknown5 = 0;
    *NotifySync->Unknown9 = 0;
    *NotifySync->Unknown10 = 0;
#endif
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
		(FullTargetName->Length - *FileNamePartLength), /*?*/
		NULL,
		NULL,
		FilterMatch,
		0, /* Action ? */
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
    IN OUT PNOTIFY_SYNC NotifySync
	)
{
#if 0
    if (NULL != *NotifySync) 
	{
        ExFreePool (*NotifySync);
        *NotifySync = NULL;
	}
#endif
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
STDCALL
NTSTATUS
FsRtlRegisterFileSystemFilterCallbacks (
    IN struct _DRIVER_OBJECT *FilterDriverObject,
    IN PFS_FILTER_CALLBACKS Callbacks
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
