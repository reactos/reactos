#ifndef _INCLUDE_DDK_IOFUNCS_H
#define _INCLUDE_DDK_IOFUNCS_H
/* $Id: iofuncs.h,v 1.31 2002/08/28 07:13:04 hbirr Exp $ */

/* --- EXPORTED BY NTOSKRNL --- */

/**********************************************************************
 * NAME							EXPORTED
 *	IoAcquireCancelSpinLock@4
 *	
 * DESCRIPTION
 * 	Synchronizes cancelable-state transistions for IRPs in a 
 *	multiprocessor-safe way.
 *	
 * ARGUMENTS
 *	Irpl
 *		Variable to store the current IRQ level.
 *
 * RETURN VALUE
 * 	None.
 *
 * REVISIONS
 *
 */
VOID
STDCALL
IoAcquireCancelSpinLock (
	PKIRQL	Irpl
	);
VOID
STDCALL
IoAcquireVpbSpinLock (
	PKIRQL	Irpl
	);
/**********************************************************************
 * NAME							EXPORTED
 *	IoAllocateAdapterChannel@
 *	
 * DESCRIPTION
 * 	Allocates an adaptor object for a DMA operation on the target
 *	device.
 *	
 * ARGUMENTS
 *	Adaptor
 *		Adapter channel or busmaster adapter to be allocated;
 *		
 *	DeviceObject
 *		Target device for DMA;
 *		
 *	NumberOfMapRegisters
 *		Number of map registers;
 *		
 *	ExecutionRoutine
 *		Routine to be called when the adaptor is available;
 *		
 *	Context
 *		Driver defined contex that will be passed to the
 *		execution routine.
 *		
 * RETURN VALUE
 * 	Success or failure code.
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
IoAllocateAdapterChannel (
	PADAPTER_OBJECT	AdaperObject,
	PDEVICE_OBJECT	DeviceObject,
	ULONG		NumberOfMapRegisters,
	PDRIVER_CONTROL	ExecutionRoutine,
	PVOID		Context
	);
/**********************************************************************
 * NAME							EXPORTED
 *	IoAllocateController@16
 *
 * DESCRIPTION
 *	Sets up a call to a driver supplied controller object as 
 *	soon as it is available.
 *
 * ARGUMENTS
 *	ControllerObject
 *		Driver created controller object;
 *
 *	DeviceObject
 *		Target device;
 *
 *	ExecutionObject
 *		Routine to be called;
 *
 *	Context
 *		Driver determined context to be based to the
 *		routine.
 *
 * RETURN VALUE
 *	None.
 *
 * REVISIONS
 *
 */
VOID
STDCALL
IoAllocateController (
	PCONTROLLER_OBJECT	ControllerObject,
	PDEVICE_OBJECT		DeviceObject,
	PDRIVER_CONTROL		ExecutionRoutine,
	PVOID			Context
	);
/**********************************************************************
 * NAME							EXPORTED
 *	IoAllocateErrorLogEntry@8
 *	
 * DESCRIPTION
 *	Allocates an error log packet.
 *
 * ARGUMENTS
 *	IoObject
 *		Object which found the error;
 *		
 *	EntrySize
 *		Size in bytes of the packet to be allocated.
 *
 * RETURN VALUE
 *	On success, a pointer to the allocated packet.
 *	On failure, it returns NULL.
 */
PVOID
STDCALL
IoAllocateErrorLogEntry (
	PVOID	IoObject,
	UCHAR	EntrySize
	);
/**********************************************************************
 * NAME							EXPORTED
 *	IoAllocateIrp@8
 *	
 * DESCRIPTION
 * 	Allocates an IRP.
 * 	
 * ARGUMENTS
 *	StackSize
 *		Number of stack locations to allocate;
 *		
 *	ChargeQuota
 *		Who knows.
 *		
 * RETURN VALUE
 * 	On success, the allocated IRP. On failure, NULL.
 */
PIRP
STDCALL
IoAllocateIrp (
	CCHAR	StackSize,
	BOOLEAN	ChargeQuota
	);
/**********************************************************************
 * NAME							EXPORTED
 *	IoAllocateMdl@20
 *
 * DESCRIPTION
 * 	Allocates an MDL large enough to map the supplied buffer.
 * 	
 * ARGUMENTS
 *	VirtualAddress
 *		Base virtual address of the buffer to be mapped;
 *		
 *	Length
 *		Length of the buffer to be mapped;
 *		
 *	SecondaryBuffer
 *		Whether the buffer is primary or secondary;
 *		
 *	ChargeQuota
 *		Charge non-paged pool quota to current thread;
 *		
 *	Irp
 *		Optional irp to be associated with the MDL.
 *	
 * RETURN VALUE
 * 	On success, the allocated MDL; on failure, NULL.
 */
PMDL
STDCALL
IoAllocateMdl (
	PVOID	VirtualAddress,
	ULONG	Length,
	BOOLEAN	SecondaryBuffer,
	BOOLEAN	ChargeQuota,
	PIRP	Irp
	);

/**********************************************************************
 * NAME							MACRO
 *	IoAssignArcName
 *
 * DESCRIPTION
 *	Creates a symbolic link between the ARC name of a physical
 *	device and the name of the corresponding device object
 *
 * ARGUMENTS
 *	ArcName
 *		ARC name of the device
 *
 *	DeviceName
 *		Name of the device object
 *
 * NOTES
 *	VOID
 *	IoAssignArcName (
 *		PUNICODE_STRING	ArcName,
 *		PUNICODE_STRING	DeviceName
 *		);
 */
#define IoAssignArcName(ArcName,DeviceName) \
	(IoCreateSymbolicLink((ArcName),(DeviceName)))

/**********************************************************************
 * NAME							EXPORTED
 *	IoAssignResources@24
 *
 * DESCRIPTION
 *	Takes a list of requested hardware resources and allocates
 *	them.
 *
 * ARGUMENTS
 *	RegisterPath
 *		?
 *
 *	DriverClassName
 *		?
 *
 *	DriverObject
 *		Driver object passed to the DriverEntry routine;
 *
 *	DeviceObject
 *		?
 *
 *	RequestedResources
 *		List of resources.
 *
 * RETURN VALUE
 */
NTSTATUS
STDCALL
IoAssignResources (
	PUNICODE_STRING			RegistryPath,
	PUNICODE_STRING			DriverClassName,
	PDRIVER_OBJECT			DriverObject,
	PDEVICE_OBJECT			DeviceObject,
	PIO_RESOURCE_REQUIREMENTS_LIST	RequestedResources,
	PCM_RESOURCE_LIST		* AllocatedResources
	);
/*
 * FUNCTION: Attaches the callers device object to a named target device
 * ARGUMENTS:
 *        SourceDevice = caller's device
 *        TargetDevice = Name of the target device
 *        AttachedDevice = Caller allocated storage. On return contains
 *                         a pointer to the target device
 * RETURNS: Success or failure code
 */
NTSTATUS
STDCALL
IoAttachDevice (
	PDEVICE_OBJECT	SourceDevice,
	PUNICODE_STRING	TargetDevice,
	PDEVICE_OBJECT	* AttachedDevice
	);
/*
 * FUNCTION: Obsolete
 * ARGUMENTS:
 *       SourceDevice = device to attach
 *       TargetDevice = device to be attached to
 * RETURNS: Success or failure code
 */
NTSTATUS
STDCALL
IoAttachDeviceByPointer (
	PDEVICE_OBJECT	SourceDevice,
	PDEVICE_OBJECT	TargetDevice
	);
/*
 * FUNCTION: Attaches the callers device to the highest device in the chain
 * ARGUMENTS:
 *       SourceDevice = caller's device
 *       TargetDevice = Device to attach
 * RETURNS: On success the previously highest device
 *          On failure NULL
 */
PDEVICE_OBJECT
STDCALL
IoAttachDeviceToDeviceStack (
	PDEVICE_OBJECT	SourceDevice,
	PDEVICE_OBJECT	TargetDevice
	);
/*
 * FUNCTION: Builds a irp to be sent to lower level drivers
 * ARGUMENTS:
 *       MajorFunction = Major function code to be set in the IRP
 *       DeviceObject = Next lower device object
 *       Buffer = Buffer (only required for some major function codes)
 *       Length = Length in bytes of the buffer 
 *       StartingOffset = Starting offset on the target device
 *       IoStatusBlock = Storage for status about the operation (optional)
 * RETURNS: On success the IRP allocated
 *          On failure NULL
 */
PIRP
STDCALL
IoBuildAsynchronousFsdRequest (
	ULONG			MajorFunction,
	PDEVICE_OBJECT		DeviceObject,
	PVOID			Buffer,
	ULONG			Length,
	PLARGE_INTEGER		StartingOffset,
	PIO_STATUS_BLOCK	IoStatusBlock
	);
/*
 * FUNCTION: Allocates and sets up an IRP for a device control request
 * ARGUMENTS:
 *        IoControlCode = Type of request
 *        DeviceObject = Target device
 *        InputBuffer = Optional input buffer to the driver
 *        InputBufferLength = Length of the input buffer
 *        OutputBuffer = Optional output buffer
 *        OutputBufferLength = Length of the output buffer
 *        InternalDeviceIoControl = TRUE if the request is internal
 *        Event = Initialized event for the caller to wait for the request
 *                to be completed
 *        IoStatusBlock = I/O status block to be set when the request is
 *                        completed
 * RETURNS: Returns the IRP created
 */
PIRP
STDCALL
IoBuildDeviceIoControlRequest (
	ULONG			IoControlCode,
	PDEVICE_OBJECT		DeviceObject,
	PVOID			InputBuffer,
	ULONG			InputBufferLength,
	PVOID			OutputBuffer,
	ULONG			OutputBufferLength,
	BOOLEAN			InternalDeviceIoControl,
	PKEVENT			Event,
	PIO_STATUS_BLOCK	IoStatusBlock
	);
VOID
STDCALL
IoBuildPartialMdl (
	PMDL	SourceMdl,
	PMDL	TargetMdl,
	PVOID	VirtualAddress,
	ULONG	Length
	);
PIRP
STDCALL
IoBuildSynchronousFsdRequest (
	ULONG			MajorFunction,
	PDEVICE_OBJECT		DeviceObject,
	PVOID			Buffer,
	ULONG			Length,
	PLARGE_INTEGER		StartingOffset,
	PKEVENT			Event,
	PIO_STATUS_BLOCK	IoStatusBlock
	);
NTSTATUS
STDCALL
IoCallDriver (
	PDEVICE_OBJECT	DeviceObject,
	PIRP		Irp
	);
BOOLEAN
STDCALL
IoCancelIrp (
	PIRP	Irp
	);

NTSTATUS STDCALL
IoCheckDesiredAccess(IN OUT PACCESS_MASK DesiredAccess,
		     IN ACCESS_MASK GrantedAccess);

NTSTATUS STDCALL
IoCheckEaBufferValidity(IN PFILE_FULL_EA_INFORMATION EaBuffer,
			IN ULONG EaLength,
			OUT PULONG ErrorOffset);

NTSTATUS STDCALL
IoCheckFunctionAccess(IN ACCESS_MASK GrantedAccess,
		      IN UCHAR MajorFunction,
		      IN UCHAR MinorFunction,
		      IN ULONG IoControlCode,
		      IN PFILE_INFORMATION_CLASS FileInformationClass OPTIONAL,
		      IN PFS_INFORMATION_CLASS FsInformationClass OPTIONAL);

NTSTATUS
STDCALL
IoCheckShareAccess (
	ACCESS_MASK	DesiredAccess,
	ULONG		DesiredShareAccess,
	PFILE_OBJECT	FileObject,
	PSHARE_ACCESS	ShareAccess,
	BOOLEAN		Update
	);
VOID
STDCALL
IoCompleteRequest (
	PIRP	Irp,
	CCHAR	PriorityBoost
	);
NTSTATUS
STDCALL
IoConnectInterrupt (
	PKINTERRUPT		* InterruptObject,
	PKSERVICE_ROUTINE	ServiceRoutine,
	PVOID			ServiceContext,
	PKSPIN_LOCK		SpinLock,
	ULONG			Vector,
	KIRQL			Irql,
	KIRQL			SynchronizeIrql,
	KINTERRUPT_MODE		InterruptMode,
	BOOLEAN			ShareVector,
	KAFFINITY		ProcessorEnableMask,
	BOOLEAN			FloatingSave
	);

PCONTROLLER_OBJECT
STDCALL
IoCreateController (
	ULONG	Size
	);
/*
 * FUNCTION: Allocates memory for and intializes a device object for use for
 * a driver
 * ARGUMENTS:
 *         DriverObject : Driver object passed by iomgr when the driver was
 *                        loaded
 *         DeviceExtensionSize : Number of bytes for the device extension
 *         DeviceName : Unicode name of device
 *         DeviceType : Device type
 *         DeviceCharacteristics : Bit mask of device characteristics
 *         Exclusive : True if only one thread can access the device at a
 *                     time
 * RETURNS:
 *         Success or failure
 *         DeviceObject : Contains a pointer to allocated device object
 *                        if the call succeeded
 * NOTES: See the DDK documentation for more information        
 */
NTSTATUS
STDCALL
IoCreateDevice (
	PDRIVER_OBJECT	DriverObject,
	ULONG		DeviceExtensionSize,
	PUNICODE_STRING	DeviceName,
	DEVICE_TYPE	DeviceType,
	ULONG		DeviceCharacteristics,
	BOOLEAN		Exclusive,
	PDEVICE_OBJECT	* DeviceObject
	);
NTSTATUS
STDCALL
IoCreateFile (
	OUT	PHANDLE			FileHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	PLARGE_INTEGER		AllocationSize		OPTIONAL,
	IN	ULONG			FileAttributes,
	IN	ULONG			ShareAccess,
	IN	ULONG			CreateDisposition,
	IN	ULONG			CreateOptions,
	IN	PVOID			EaBuffer		OPTIONAL,
	IN	ULONG			EaLength,
	IN	CREATE_FILE_TYPE	CreateFileType,
	IN	PVOID			ExtraCreateParameters	OPTIONAL,
	IN	ULONG			Options
	);
PKEVENT
STDCALL
IoCreateNotificationEvent (
	PUNICODE_STRING	EventName,
	PHANDLE	EventHandle
	);
PFILE_OBJECT
STDCALL
IoCreateStreamFileObject (
	PFILE_OBJECT	FileObject,
	PDEVICE_OBJECT	DeviceObject
	);
NTSTATUS
STDCALL
IoCreateSymbolicLink (
	PUNICODE_STRING	SymbolicLinkName,
	PUNICODE_STRING	DeviceName
	);
PKEVENT
STDCALL
IoCreateSynchronizationEvent (
	PUNICODE_STRING	EventName,
	PHANDLE	EventHandle
	);
NTSTATUS
STDCALL
IoCreateUnprotectedSymbolicLink (
	PUNICODE_STRING	SymbolicLinkName,
	PUNICODE_STRING	DeviceName
	);

/*
 * FUNCTION:
 *	Deletes a symbolic link between the ARC name of a physical
 *	device and the name of the corresponding device object
 *
 * ARGUMENTS:
 *	ArcName = ARC name of the device
 *
 * NOTES:
 *	VOID
 *	IoDeassignArcName (
 *		PUNICODE_STRING	ArcName
 *		);
 */
#define IoDeassignArcName(ArcName) \
	(IoDeleteSymbolicLink((ArcName)))

VOID
STDCALL
IoDeleteController (
	PCONTROLLER_OBJECT	ControllerObject
	);
VOID
STDCALL
IoDeleteDevice (
	PDEVICE_OBJECT	DeviceObject
	);
NTSTATUS
STDCALL
IoDeleteSymbolicLink (
	PUNICODE_STRING	SymbolicLinkName
	);
VOID
STDCALL
IoDetachDevice (
	PDEVICE_OBJECT	TargetDevice
	);
VOID
STDCALL
IoDisconnectInterrupt (
	PKINTERRUPT	InterruptObject
	);
VOID
STDCALL
IoEnqueueIrp (
	PIRP	Irp
	);

BOOLEAN STDCALL
IoFastQueryNetworkAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
			     IN ACCESS_MASK DesiredAccess,
			     IN ULONG OpenOptions,
			     OUT PIO_STATUS_BLOCK IoStatus,
			     OUT PFILE_NETWORK_OPEN_INFORMATION Buffer);

VOID
STDCALL
IoFreeController (
	PCONTROLLER_OBJECT	ControllerObject
	);
VOID
STDCALL
IoFreeIrp (
	PIRP	Irp
	);
VOID
STDCALL
IoFreeMdl (
	PMDL	Mdl
	);
PDEVICE_OBJECT
STDCALL
IoGetAttachedDevice (
	PDEVICE_OBJECT	DeviceObject
	);
PDEVICE_OBJECT
STDCALL
IoGetAttachedDeviceReference (
	PDEVICE_OBJECT	DeviceObject
	);
PDEVICE_OBJECT
STDCALL
IoGetBaseFileSystemDeviceObject (
	IN	PFILE_OBJECT	FileObject
	);
PCONFIGURATION_INFORMATION
STDCALL
IoGetConfigurationInformation (
	VOID
	);

/*
 * FUNCTION: Gets a pointer to the callers location in the I/O stack in
 * the given IRP
 * ARGUMENTS:
 *         Irp = Points to the IRP
 * RETURNS: A pointer to the stack location
 *
 * NOTES:
 *      PIO_STACK_LOCATION
 *      IoGetCurrentIrpStackLocation (PIRP Irp)
 */
#define IoGetCurrentIrpStackLocation(Irp) \
	((Irp)->Tail.Overlay.CurrentStackLocation)

#define IoSetNextIrpStackLocation(Irp) { \
  (Irp)->CurrentLocation--; \
  (Irp)->Tail.Overlay.CurrentStackLocation--; }

#define IoCopyCurrentIrpStackLocationToNext(Irp) { \
  PIO_STACK_LOCATION IrpSp; \
  PIO_STACK_LOCATION NextIrpSp; \
  IrpSp = IoGetCurrentIrpStackLocation((Irp)); \
  NextIrpSp = IoGetNextIrpStackLocation((Irp)); \
  RtlCopyMemory(NextIrpSp, IrpSp, \
    FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); \
  NextIrpSp->Control = 0; }

#define IoSkipCurrentIrpStackLocation(Irp) \
  (Irp)->CurrentLocation++; \
  (Irp)->Tail.Overlay.CurrentStackLocation++;

struct _EPROCESS*
STDCALL
IoGetCurrentProcess (
	VOID
	);
NTSTATUS
STDCALL
IoGetDeviceObjectPointer (
	PUNICODE_STRING	ObjectName,
	ACCESS_MASK	DesiredAccess,
	PFILE_OBJECT	* FileObject,
	PDEVICE_OBJECT	* DeviceObject
	);
PDEVICE_OBJECT
STDCALL
IoGetDeviceToVerify (
	struct _ETHREAD*	Thread
	);
PGENERIC_MAPPING
STDCALL
IoGetFileObjectGenericMapping (
	VOID
	);

#define IoGetFunctionCodeFromCtlCode(ControlCode) \
	((ControlCode >> 2) & 0x00000FFF)

PVOID
STDCALL
IoGetInitialStack (
	VOID
	);

/*
 * FUNCTION: Gives a higher level driver access to the next lower driver's 
 * I/O stack location
 * ARGUMENTS: 
 *           Irp = points to the irp
 * RETURNS: A pointer to the stack location
 *
 * NOTES:
 *      PIO_STACK_LOCATION
 *      IoGetNextIrpStackLocation (PIRP Irp)
 */
#define IoGetNextIrpStackLocation(Irp) \
	((Irp)->Tail.Overlay.CurrentStackLocation-1)


PDEVICE_OBJECT
STDCALL
IoGetRelatedDeviceObject (
	PFILE_OBJECT	FileObject
	);
struct _EPROCESS*
STDCALL
IoGetRequestorProcess (
	IN	PIRP	Irp
	);

VOID
STDCALL
IoGetStackLimits (
	PULONG	LowLimit,
	PULONG	HighLimit
	);

PIRP
STDCALL
IoGetTopLevelIrp (
	VOID
	);

#define IoInitializeDpcRequest(DeviceObject,DpcRoutine) \
	(KeInitializeDpc(&(DeviceObject)->Dpc, \
			 (PKDEFERRED_ROUTINE)(DpcRoutine), \
			 (DeviceObject)))

/*
 * FUNCTION: Initalizes an irp allocated by the caller
 * ARGUMENTS:
 *          Irp = IRP to initalize
 *          PacketSize = Size in bytes of the IRP
 *          StackSize = Number of stack locations in the IRP
 */
VOID
STDCALL
IoInitializeIrp (
	PIRP	Irp,
	USHORT	PacketSize,
	CCHAR	StackSize
	);
NTSTATUS
STDCALL
IoInitializeTimer (
	PDEVICE_OBJECT		DeviceObject,
	PIO_TIMER_ROUTINE	TimerRoutine,
	PVOID			Context
	);

/*
 * NOTES:
 *	BOOLEAN
 *	IsErrorUserInduced (NTSTATUS Status)
 */
#define IoIsErrorUserInduced(Status) \
	((BOOLEAN)(((Status) == STATUS_DEVICE_NOT_READY) || \
		   ((Status) == STATUS_IO_TIMEOUT) || \
		   ((Status) == STATUS_MEDIA_WRITE_PROTECTED) || \
		   ((Status) == STATUS_NO_MEDIA_IN_DRIVE) || \
		   ((Status) == STATUS_VERIFY_REQUIRED) || \
		   ((Status) == STATUS_UNRECOGNIZED_MEDIA) || \
		   ((Status) == STATUS_WRONG_VOLUME)))

BOOLEAN
STDCALL
IoIsOperationSynchronous (
	IN	PIRP	Irp
	);
BOOLEAN
STDCALL
IoIsSystemThread (
	PVOID	Unknown0
	);
PIRP
STDCALL
IoMakeAssociatedIrp (
	PIRP	Irp,
	CCHAR	StackSize
	);

/*
 * FUNCTION: Marks the specified irp, indicating further processing will
 * be required by other driver routines
 * ARGUMENTS:
 *      Irp = Irp to mark
 * NOTES:
 *      VOID
 *      IoMarkIrpPending (PIRP Irp)
 */
#define IoMarkIrpPending(Irp) \
	(IoGetCurrentIrpStackLocation(Irp)->Control |= SL_PENDING_RETURNED)


NTSTATUS
STDCALL
IoOpenDeviceInstanceKey (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
NTSTATUS
STDCALL
IoQueryDeviceDescription (
	PINTERFACE_TYPE			BusType,
	PULONG				BusNumber,
	PCONFIGURATION_TYPE		ControllerType,
	PULONG				ControllerNumber,
	PCONFIGURATION_TYPE		PeripheralType,
	PULONG				PeripheralNumber,
	PIO_QUERY_DEVICE_ROUTINE	CalloutRoutine,
	PVOID				Context
	);
DWORD
STDCALL
IoQueryDeviceEnumInfo (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
// IoQueryFileInformation: confirmed - Undocumented because it does not require a valid file handle 
NTSTATUS 
STDCALL
IoQueryFileInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FileInformation,
	OUT	PULONG			ReturnedLength	
	);
NTSTATUS
STDCALL
IoQueryVolumeInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FS_INFORMATION_CLASS	FsInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FsInformation,
	OUT	PULONG			ReturnedLength
	);
VOID
STDCALL
IoQueueThreadIrp (
	IN	PIRP	Irp
	);
VOID
STDCALL
IoRaiseHardError (
	PIRP		Irp,
	PVPB		Vpb,
	PDEVICE_OBJECT	RealDeviceObject
	);
BOOLEAN
STDCALL
IoRaiseInformationalHardError (
	NTSTATUS	ErrorStatus,
	PUNICODE_STRING	String,
	struct _KTHREAD*	Thread
	);
VOID
STDCALL
IoRegisterDriverReinitialization (
	PDRIVER_OBJECT		DriverObject,
	PDRIVER_REINITIALIZE	ReinitRoutine,
	PVOID			Context
	);
VOID
STDCALL
IoRegisterFileSystem (
	PDEVICE_OBJECT	DeviceObject
	);
#if (_WIN32_WINNT >= 0x0400)
NTSTATUS
STDCALL
IoRegisterFsRegistrationChange (
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PFSDNOTIFICATIONPROC	FSDNotificationProc
	);
#endif // (_WIN32_WINNT >= 0x0400)
NTSTATUS
STDCALL
IoRegisterShutdownNotification (
	PDEVICE_OBJECT	DeviceObject
	);
VOID
STDCALL
IoReleaseCancelSpinLock (
	IN	KIRQL	Irql
	);
VOID
STDCALL
IoReleaseVpbSpinLock (
	IN	KIRQL	Irql
	);
VOID
STDCALL
IoRemoveShareAccess (
	PFILE_OBJECT	FileObject,
	PSHARE_ACCESS	ShareAccess
	);
NTSTATUS
STDCALL
IoReportHalResourceUsage (
	IN	PUNICODE_STRING		HalDescription,
	IN	PCM_RESOURCE_LIST	RawList,
	IN	PCM_RESOURCE_LIST	TranslatedList,
	IN	ULONG			ListSize
	);
NTSTATUS
STDCALL
IoReportResourceUsage (
	PUNICODE_STRING		DriverClassName,
	PDRIVER_OBJECT		DriverObject,
	PCM_RESOURCE_LIST	DriverList,
	ULONG			DriverListSize,
	PDEVICE_OBJECT		DeviceObject,
	PCM_RESOURCE_LIST	DeviceList,
	ULONG			DeviceListSize,
	BOOLEAN			OverrideConflict,
	PBOOLEAN		ConflictDetected
	);

#define IoRequestDpc(DeviceObject,Irp,Context) \
	(KeInsertQueueDpc(&(DeviceObject)->Dpc,(Irp),(Context)))

#define IoSetCancelRoutine(Irp,NewCancelRoutine) \
	((PDRIVER_CANCEL)InterlockedExchange((PULONG)&(Irp)->CancelRoutine, \
					     (ULONG)(NewCancelRoutine)));

#define IoSetCompletionRoutine(Irp,Routine,Context,Success,Error,Cancel) \
	{ \
		PIO_STACK_LOCATION param; \
		assert((Success)||(Error)||(Cancel)?(Routine)!=NULL:TRUE); \
		param = IoGetNextIrpStackLocation((Irp)); \
		param->CompletionRoutine=(Routine); \
		param->CompletionContext=(Context); \
		param->Control = 0; \
		if ((Success)) \
			param->Control = SL_INVOKE_ON_SUCCESS; \
		if ((Error)) \
			param->Control |= SL_INVOKE_ON_ERROR; \
		if ((Cancel)) \
			param->Control |= SL_INVOKE_ON_CANCEL; \
	} 

VOID STDCALL
IoSetDeviceToVerify (IN struct _ETHREAD* Thread,
		     IN PDEVICE_OBJECT DeviceObject);
VOID
STDCALL
IoSetHardErrorOrVerifyDevice (
	IN	PIRP		Irp,
	IN	PDEVICE_OBJECT	DeviceObject
	);
NTSTATUS
STDCALL
IoSetInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FileInformation
	);

#define IoSetNextIrpStackLocation(Irp) \
{ \
	(Irp)->CurrentLocation--; \
	(Irp)->Tail.Overlay.CurrentStackLocation--; \
} 

VOID
STDCALL
IoSetShareAccess (
	ACCESS_MASK	DesiredAccess,
	ULONG		DesiredShareAccess,
	PFILE_OBJECT	FileObject,
	PSHARE_ACCESS	ShareAccess
	);
BOOLEAN
STDCALL
IoSetThreadHardErrorMode (
	IN	BOOLEAN	HardErrorEnabled
	);
VOID
STDCALL
IoSetTopLevelIrp (
	IN	PIRP	Irp
	);

/*
 * FUNCTION:  Determines the size of an IRP
 * ARGUMENTS: 
 *           StackSize = number of stack locations in the IRP
 * RETURNS: The size of the IRP in bytes 
USHORT
IoSizeOfIrp (CCHAR StackSize)
 */
#define IoSizeOfIrp(StackSize) \
	((USHORT)(sizeof(IRP)+(((StackSize)-1)*sizeof(IO_STACK_LOCATION))))

/* original macro */
/*
#define IoSizeOfIrp(StackSize) \
	((USHORT)(sizeof(IRP)+((StackSize)*sizeof(IO_STACK_LOCATION))))
*/

/*
 * FUNCTION: Dequeues the next IRP from the device's associated queue and
 * calls its StartIo routine
 * ARGUMENTS:
 *          DeviceObject = Device object
 *          Cancelable = True if IRPs in the queue can be cancelled
 */
VOID
STDCALL
IoStartNextPacket (
	PDEVICE_OBJECT	DeviceObject,
	BOOLEAN		Cancelable
	);
VOID
STDCALL
IoStartNextPacketByKey (
	PDEVICE_OBJECT	DeviceObject,
	BOOLEAN		Cancelable,
	ULONG		Key
	);
/*
 * FUNCTION: Calls the drivers StartIO routine with the IRP or queues it if
 * the device is busy
 * ARGUMENTS:
 *         DeviceObject = Device to pass the IRP to 
 *         Irp = Irp to be processed
 *         Key = Optional value for where to insert the IRP
 *         CancelFunction = Entry point for a driver supplied cancel function
 */
VOID
STDCALL
IoStartPacket (
	PDEVICE_OBJECT	DeviceObject,
	PIRP		Irp,
	PULONG		Key,
	PDRIVER_CANCEL	CancelFunction
	);
VOID
STDCALL
IoStartTimer (
	PDEVICE_OBJECT	DeviceObject
	);
VOID
STDCALL
IoStopTimer (
	PDEVICE_OBJECT	DeviceObject
	);

NTSTATUS STDCALL
IoPageRead(PFILE_OBJECT		FileObject,
	   PMDL			Mdl,
	   PLARGE_INTEGER	Offset,
	   PKEVENT		Event,
	   PIO_STATUS_BLOCK	StatusBlock);

NTSTATUS STDCALL 
IoSynchronousPageWrite (PFILE_OBJECT	    FileObject,
			PMDL		    Mdl,
			PLARGE_INTEGER	    Offset,
			PKEVENT		    Event,
			PIO_STATUS_BLOCK    StatusBlock);

struct _EPROCESS* STDCALL IoThreadToProcess (struct _ETHREAD*	Thread);
VOID
STDCALL
IoUnregisterFileSystem (
	IN	PDEVICE_OBJECT	DeviceObject
	);
#if (_WIN32_WINNT >= 0x0400)
VOID
STDCALL
IoUnregisterFsRegistrationChange (
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PFSDNOTIFICATIONPROC	FSDNotificationProc
	);
#endif // (_WIN32_WINNT >= 0x0400)
VOID
STDCALL
IoUnregisterShutdownNotification (
	IN	PDEVICE_OBJECT	DeviceObject
	);
VOID
STDCALL
IoUpdateShareAccess (
	IN	PFILE_OBJECT	FileObject,
	IN	PSHARE_ACCESS	ShareAccess
	);
NTSTATUS
STDCALL
IoVerifyVolume (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	BOOLEAN		AllowRawMount
	);
VOID
STDCALL
IoWriteErrorLogEntry (
	PVOID	ElEntry
	);
/*
 * FUNCTION: Sends an irp to the next lower driver
 */
NTSTATUS
FASTCALL
IofCallDriver (
	PDEVICE_OBJECT	DeviceObject,
	PIRP		Irp
	);
/*
 * FUNCTION: Indicates the caller has finished all processing for a given
 * I/O request and is returning the given IRP to the I/O manager
 * ARGUMENTS:
 *         Irp = Irp to be cancelled
 *         PriorityBoost = Increment by which to boost the priority of the
 *                         thread making the request
 */
VOID
FASTCALL
IofCompleteRequest (
	PIRP	Irp,
	CCHAR	PriorityBoost
	);

/* --- EXPORTED BY HAL --- */

VOID
STDCALL
IoAssignDriveLetters (
	IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
	IN	PSTRING			NtDeviceName,
	OUT	PUCHAR			NtSystemPath,
	OUT	PSTRING			NtSystemPathString
	);

BOOLEAN
STDCALL
IoFlushAdapterBuffers (
	PADAPTER_OBJECT	AdapterObject,
	PMDL		Mdl,
	PVOID		MapRegisterBase,
	PVOID		CurrentVa,
	ULONG		Length,
	BOOLEAN		WriteToDevice
	);

VOID
STDCALL
IoFreeAdapterChannel (
	PADAPTER_OBJECT	AdapterObject
	);

VOID
STDCALL
IoFreeMapRegisters (
	PADAPTER_OBJECT	AdapterObject,
	PVOID		MapRegisterBase,
	ULONG		NumberOfMapRegisters
	);

PHYSICAL_ADDRESS
STDCALL
IoMapTransfer (
	PADAPTER_OBJECT	AdapterObject,
	PMDL		Mdl,
	PVOID		MapRegisterBase,
	PVOID		CurrentVa,
	PULONG		Length,
	BOOLEAN		WriteToDevice
	);

NTSTATUS
STDCALL
IoReadPartitionTable (
	PDEVICE_OBJECT			DeviceObject,
	ULONG				SectorSize,
	BOOLEAN				ReturnedRecognizedPartitions,
	PDRIVE_LAYOUT_INFORMATION	* PartitionBuffer
	);

NTSTATUS
STDCALL
IoSetPartitionInformation (
	PDEVICE_OBJECT	DeviceObject,
	ULONG		SectorSize,
	ULONG		PartitionNumber,
	ULONG		PartitionType
	);

NTSTATUS
STDCALL
IoWritePartitionTable (
	PDEVICE_OBJECT			DeviceObject,
	ULONG				SectorSize,
	ULONG				SectorsPerTrack,
	ULONG				NumberOfHeads,
	PDRIVE_LAYOUT_INFORMATION	PartitionBuffer
	);


/* --- --- --- INTERNAL or REACTOS ONLY --- --- --- */

/*
 * FUNCTION: Registers the driver with WMI
 * ARGUMENTS:
 *          DeviceObject = Device to register
 *          Action = Action to take
 * RETURNS: Status (?)
 */
/*
NTSTATUS
IoWMIRegistrationControl (
	PDEVICE_OBJECT DeviceObject,
	WMIREGACTION Action);
*/

BOOLEAN
IoIsTotalDeviceFailure (
	NTSTATUS	Status
	);


#endif /* ndef _INCLUDE_DDK_IOFUNCS_H */
