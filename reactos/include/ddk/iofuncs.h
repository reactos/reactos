/* IO MANAGER ***************************************************************/

/*
 * FUNCTION: Registers the driver with WMI
 * ARGUMENTS:
 *          DeviceObject = Device to register
 *          Action = Action to take
 * RETURNS: Status (?)
 */
//NTSTATUS IoWMIRegistrationControl(DeviceObject, WMIREGACTION Action);
 
/*
 * FUNCTION: Synchronizes cancelable-state transistions for IRPs in a 
 * multiprocessor-safe way
 * ARGUMENTS:
 *          Irpl = Variable to store the current IRQ level 
 */
VOID IoAcquireCancelSpinLock(PKIRQL Irpl);

typedef IO_ALLOCATION_ACTION (*PDRIVER_CONTROL)(PDEVICE_OBJECT DeviceObject,
					PIRP irp,
					PVOID MapRegisterBase,
					PVOID Context);

/*
 * FUNCTION: Allocates an adaptor object for a DMA operation on the target
 * device
 * ARGUMENTS:
 *         Adaptor = Adapter channel or busmaster adapter to be allocated
 *         DeviceObject = Target device for DMA
 *         NumberOfMapRegisters = Number of map registers
 *         ExecutionRoutine = Routine to be called when the adaptor is 
 *                            available
 *         Context = driver defined contex that will be passed to the
 *                   execution routine
 * RETURNS: Success or failure code
 */
NTSTATUS IoAllocateAdapterChannel(PADAPTER_OBJECT AdaperObject,
				  PDEVICE_OBJECT DeviceObject,
				  ULONG NumberOfMapRegisters,
				  PDRIVER_CONTROL ExecutionRoutine,
				  PVOID Context);

/*
 * FUNCTION: Sets up a call to a driver supplied controller object as 
 * soon as it is available
 * ARGUMENTS:
 *        ControllerObject = Driver created controller object
 *        DeviceObject = target device
 *        ExecutionObject = Routine to be called
 *        Context = Driver determined context to be based to the routine
 */
VOID IoAllocateController(PCONTROLLER_OBJECT ControllerObject,
			  PDEVICE_OBJECT DeviceObject,
			  PDRIVER_CONTROL ExecutionRoutine,
			  PVOID Context);

/*
 * FUNCTION: Allocates an error log packet
 * ARGUMENTS:
 *         IoObject = Object which found the error
 *         EntrySize = Size in bytes of the packet to be allocated
 * RETURNS: On success a pointer to the allocated packet
 *          On failure returns NULL
 */
PVOID IoAllocateErrorLogEntry(PVOID IoObject, UCHAR EntrySize);

/*
 * FUNCTION: Allocates an IRP
 * ARGUMENTS:
 *        StackSize = number of stack locations to allocate
 *        ChargeQuota = Who knows
 * RETURNS: On success the allocated IRP
 *          On failure NULL
 */
PIRP IoAllocateIrp(CCHAR StackSize, BOOLEAN ChargeQuota);

/*
 * FUNCTION: Allocates an MDL large enough to map the supplied buffer
 * ARGUMENTS:
 *        VirtualAddress = base virtual address of the buffer to be mapped
 *        Length = length of the buffer to be mapped
 *        SecondaryBuffer = Whether the buffer is primary or secondary
 *        ChargeQuota = Charge non-paged pool quota to current thread
 *        Irp = Optional irp to be associated with the MDL
 * RETURNS: On the success the allocated MDL
 *          On failure NULL
 */
PMDL IoAllocateMdl(PVOID VirtualAddress, ULONG Length, 
		   BOOLEAN SecondaryBuffer, BOOLEAN ChargeQuota,
		   PIRP Irp);

/*
 * FUNCTION: Creates a symbolic link between the ARC name of a physical
 * device and the name of the corresponding device object
 * ARGUMENTS:
 *        ArcName = ARC name of the device
 *        DeviceName = Name of the device object
 */
VOID IoAssignArcName(PUNICODE_STRING ArcName, PUNICODE_STRING DeviceName);

enum
{
   IO_NO_INCREMENT,
};
   
/*
 * FUNCTION: Takes a list of requested hardware resources and allocates them
 * ARGUMENTS:
 *        RegisterPath = 
 *        DriverClassName = 
 *        DriverObject = Driver object passed to the DriverEntry routine
 *        DeviceObject = 
 *        RequestedResources = List of resources
 * RETURNS: 
 */
NTSTATUS IoAssignResources(PUNICODE_STRING RegistryPath,
			   PUNICODE_STRING DriverClassName,
			   PDRIVER_OBJECT DriverObject,
			   PDEVICE_OBJECT DeviceObject,
			   PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
			   PCM_RESOURCE_LIST* AllocatedResources);

/*
 * FUNCTION: Attaches the callers device object to a named target device
 * ARGUMENTS:
 *        SourceDevice = caller's device
 *        TargetDevice = Name of the target device
 *        AttachedDevice = Caller allocated storage. On return contains
 *                         a pointer to the target device
 * RETURNS: Success or failure code
 */
NTSTATUS IoAttachDevice(PDEVICE_OBJECT SourceDevice,
			PUNICODE_STRING TargetDevice,
			PDEVICE_OBJECT* AttachedDevice);

/*
 * FUNCTION: Obsolete
 * ARGUMENTS:
 *       SourceDevice = device to attach
 *       TargetDevice = device to be attached to
 * RETURNS: Success or failure code
 */
NTSTATUS IoAttachDeviceByPointer(PDEVICE_OBJECT SourceDevice,
				 PDEVICE_OBJECT TargetDevice);

/*
 * FUNCTION: Attaches the callers device to the highest device in the chain
 * ARGUMENTS:
 *       SourceDevice = caller's device
 *       TargetDevice = Device to attach
 * RETURNS: On success the previously highest device
 *          On failure NULL
 */
PDEVICE_OBJECT IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice,
					   PDEVICE_OBJECT TargetDevice);

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
PIRP IoBuildAsynchronousFsdRequest(ULONG MajorFunction,
				   PDEVICE_OBJECT DeviceObject,
				   PVOID Buffer,
				   ULONG Length,
				   PLARGE_INTEGER StartingOffset,
				   PIO_STATUS_BLOCK IoStatusBlock);

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
PIRP IoBuildDeviceIoControlRequest(ULONG IoControlCode,
				   PDEVICE_OBJECT DeviceObject,
				   PVOID InputBuffer,
				   ULONG InputBufferLength,
				   PVOID OutputBuffer,
				   ULONG OutputBufferLength,
				   BOOLEAN InternalDeviceIoControl,
				   PKEVENT Event,
				   PIO_STATUS_BLOCK IoStatusBlock);
   

VOID IoBuildPartialMdl(PMDL SourceMdl,
		       PMDL TargetMdl,
		       PVOID VirtualAddress,
		       ULONG Length);

PIRP IoBuildSynchronousFsdRequest(ULONG MajorFunction,
				  PDEVICE_OBJECT DeviceObject,
				  PVOID Buffer,
				  ULONG Length,
				  PLARGE_INTEGER StartingOffset,
				  PKEVENT Event,
				  PIO_STATUS_BLOCK IoStatusBlock);

/*
 * FUNCTION: Sends an irp to the next lower driver
 */
NTSTATUS IoCallDriver(PDEVICE_OBJECT DeviceObject, PIRP irp);

BOOLEAN IoCancelIrp(PIRP Irp);

NTSTATUS IoCheckShareAccess(ACCESS_MASK DesiredAccess,
			    ULONG DesiredShareAccess,
			    PFILE_OBJECT FileObject,
			    PSHARE_ACCESS ShareAccess,
			    BOOLEAN Update);

/*
 * FUNCTION: Indicates the caller has finished all processing for a given
 * I/O request and is returning the given IRP to the I/O manager
 * ARGUMENTS:
 *         Irp = Irp to be cancelled
 *         PriorityBoost = Increment by which to boost the priority of the
 *                         thread making the request
 */
VOID IoCompleteRequest(PIRP Irp, CCHAR PriorityBoost);

NTSTATUS IoConnectInterrupt(PKINTERRUPT* InterruptObject,
			    PKSERVICE_ROUTINE ServiceRoutine,
			    PVOID ServiceContext,
			    PKSPIN_LOCK SpinLock,
			    ULONG Vector,
			    KIRQL Irql,
			    KIRQL SynchronizeIrql,
			    KINTERRUPT_MODE InterruptMode,
			    BOOLEAN ShareVector,
			    KAFFINITY ProcessorEnableMask,
			    BOOLEAN FloatingSave);

PCONTROLLER_OBJECT IoCreateController(ULONG Size);

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
NTSTATUS IoCreateDevice(PDRIVER_OBJECT DriverObject,
			ULONG DeviceExtensionSize,
			PUNICODE_STRING DeviceName,
			DEVICE_TYPE DeviceType,
			ULONG DeviceCharacteristics,
			BOOLEAN Exclusive,
                        PDEVICE_OBJECT* DeviceObject);


PKEVENT IoCreateNotificationEvent(PUNICODE_STRING EventName,
				  PHANDLE EventHandle);

NTSTATUS IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
			      PUNICODE_STRING DeviceName);

PKEVENT IoCreateSynchronizationEvent(PUNICODE_STRING EventName,
				     PHANDLE EventHandle);

NTSTATUS IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
					 PUNICODE_STRING DeviceName);


VOID IoDeassignArcName(PUNICODE_STRING ArcName);

VOID IoDeleteController(PCONTROLLER_OBJECT ControllerObject);

VOID IoDeleteDevice(PDEVICE_OBJECT DeviceObject);

NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName);

VOID IoDetachDevice(PDEVICE_OBJECT TargetDevice);

VOID IoDisconnectInterrupt(PKINTERRUPT InterruptObject);

BOOLEAN IoFlushAdapterBuffers(PADAPTER_OBJECT AdapterObject,
			      PMDL Mdl,
			      PVOID MapRegisterBase,
			      PVOID CurrentVa,
			      ULONG Length,
			      BOOLEAN WriteToDevice);

VOID IoFreeAdapterChannel(PADAPTER_OBJECT AdapterObject);
VOID IoFreeController(PCONTROLLER_OBJECT ControllerObject);
VOID IoFreeIrp(PIRP Irp);
VOID IoFreeMapRegisters(PADAPTER_OBJECT AdapterObject,
			PVOID MapRegisterBase,
			ULONG NumberOfMapRegisters);
VOID IoFreeMdl(PMDL Mdl);
PCONFIGURATION_INFORMATION IoGetConfigurationInformation(VOID);


/*
 * FUNCTION: Returns a pointer to the callers stack location in the irp 
 */
PIO_STACK_LOCATION IoGetCurrentIrpStackLocation(IRP* irp);

struct _EPROCESS* IoGetCurrentProcess(VOID);

NTSTATUS IoGetDeviceObjectPointer(PUNICODE_STRING ObjectName,
				  ACCESS_MASK DesiredAccess,
				  PFILE_OBJECT* FileObject,
				  PDEVICE_OBJECT* DeviceObject);

PDEVICE_OBJECT IoGetDeviceToVerify(PETHREAD Thread);

PGENERIC_MAPPING IoGetFileObjectGenericMapping(VOID);

ULONG IoGetFunctionCodeFromCtlCode(ULONG ControlCode);

PVOID IoGetInitialStack(VOID);

/*
 * FUNCTION:  
 */
PIO_STACK_LOCATION IoGetNextIrpStackLocation(IRP* irp);

PDEVICE_OBJECT IoGetRelatedDeviceObject(PFILE_OBJECT FileObject);

VOID IoInitializeDpcRequest(PDEVICE_OBJECT DeviceObject,
			    PIO_DPC_ROUTINE DpcRoutine);

/*
 * FUNCTION: Initalizes an irp allocated by the caller
 * ARGUMENTS:
 *          Irp = IRP to initalize
 *          PacketSize = Size in bytes of the IRP
 *          StackSize = Number of stack locations in the IRP
 */
VOID IoInitializeIrp(PIRP Irp, USHORT PacketSize, CCHAR StackSize);

NTSTATUS IoInitializeTimer(PDEVICE_OBJECT DeviceObject,
			   PIO_TIMER_ROUTINE TimerRoutine,
			   PVOID Context);

BOOLEAN IoIsErrorUserInduced(NTSTATUS Status);

BOOLEAN IoIsTotalDeviceFailure(NTSTATUS Status);

PIRP IoMakeAssociatedIrp(PIRP Irp, CCHAR StackSize);

PHYSICAL_ADDRESS IoMapTransfer(PADAPTER_OBJECT AdapterObject,
			       PMDL Mdl,
			       PVOID MapRegisterBase,
			       PVOID CurrentVa,
			       PULONG Length,
			       BOOLEAN WriteToDevice);

/*
 * FUNCTION: Marks an IRP as pending
 * ARGUMENTS:
 *         Irp = Irp to mark
 * NOTE: If a driver doesn't complete the irp in its dispatch routine it
 * must mark it pending otherwise the I/O manager will complete it on
 * return from the dispatch routine.
 */
VOID IoMarkIrpPending(PIRP Irp);

NTSTATUS IoQueryDeviceDescription(PINTERFACE_TYPE BusType,
				  PULONG BusNumber,
				  PCONFIGURATION_TYPE ControllerType,
				  PULONG ControllerNumber,
				  PCONFIGURATION_TYPE PeripheralType,
				  PULONG PeripheralNumber,
				  PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
				  PVOID Context);

VOID IoRaiseHardError(PIRP Irp, PVPB Vpb, PDEVICE_OBJECT RealDeviceObject);

BOOLEAN IoRaiseHardInformationalError(NTSTATUS ErrorStatus,
				      PUNICODE_STRING String,
				      PKTHREAD Thread);

NTSTATUS IoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
			      ULONG SectorSize,
			      BOOLEAN ReturnedRecognizedPartitions,
			      struct _DRIVE_LAYOUT_INFORMATION** PBuffer);

VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
				      PDRIVER_REINITIALIZE ReinitRoutine,
				      PVOID Context);

NTSTATUS IoRegisterShutdownNotification(PDEVICE_OBJECT DeviceObject);

VOID IoReleaseCancelSpinLock(KIRQL Irql);

VOID IoRemoveShareAccess(PFILE_OBJECT FileObject,
			 PSHARE_ACCESS ShareAccess);

NTSTATUS IoReportResourceUsage(PUNICODE_STRING DriverClassName,
			       PDRIVER_OBJECT DriverObject,
			       PCM_RESOURCE_LIST DriverList,
			       ULONG DriverListSize,
			       PDEVICE_OBJECT DeviceObject,
			       PCM_RESOURCE_LIST DeviceList,
			       ULONG DeviceListSize,
			       BOOLEAN OverrideConflict,
			       PBOOLEAN ConflictDetected);

VOID IoRequestDpc(PDEVICE_OBJECT DeviceObject,
		  PIRP Irp,
		  PVOID Context);

PDRIVER_CANCEL IoSetCancelRoutine(PIRP Irp, PDRIVER_CANCEL CancelRoutine);

VOID IoSetCompletionRoutine(PIRP Irp,
			    PIO_COMPLETION_ROUTINE CompletionRoutine,
			    PVOID Context,
			    BOOLEAN InvokeOnSuccess,
			    BOOLEAN InvokeOnError,
			    BOOLEAN InvokeOnCancel);

VOID IoSetHardErrorOrVerifyDevice(PIRP Irp, PDEVICE_OBJECT DeviceObject);

VOID IoSetNextIrpStackLocation(PIRP Irp);

NTSTATUS IoSetPartitionInformation(PDEVICE_OBJECT DeviceObject,
				   ULONG SectorSize,
				   ULONG PartitionNumber,
				   ULONG PartitionType);

VOID IoSetShareAccess(ACCESS_MASK DesiredAccess,
		      ULONG DesiredShareAccess,
		      PFILE_OBJECT FileObject,
		      PSHARE_ACCESS ShareAccess);

/*
 * FUNCTION:  Determines the size of an IRP
 * ARGUMENTS: 
 *           StackSize = number of stack locations in the IRP
 * RETURNS: The size of the IRP in bytes 
 */
USHORT IoSizeOfIrp(CCHAR StackSize);

/*
 * FUNCTION: Dequeues the next IRP from the device's associated queue and
 * calls its StartIo routine
 * ARGUMENTS:
 *          DeviceObject = Device object
 *          Cancelable = True if IRPs in the queue can be cancelled
 */
VOID IoStartNextPacket(PDEVICE_OBJECT DeviceObject, BOOLEAN Cancelable);
   
VOID IoStartNextPacketByKey(PDEVICE_OBJECT DeviceObject, 
			    BOOLEAN Cancelable,
			    ULONG Key);

/*
 * FUNCTION: Calls the drivers StartIO routine with the IRP or queues it if
 * the device is busy
 * ARGUMENTS:
 *         DeviceObject = Device to pass the IRP to 
 *         Irp = Irp to be processed
 *         Key = Optional value for where to insert the IRP
 *         CancelFunction = Entry point for a driver supplied cancel function
 */
VOID IoStartPacket(PDEVICE_OBJECT DeviceObject, PIRP Irp, PULONG Key,
		   PDRIVER_CANCEL CancelFunction);

VOID IoStartTimer(PDEVICE_OBJECT DeviceObject);

VOID IoStopTimer(PDEVICE_OBJECT DeviceObject);

VOID IoUnregisterShutdownNotification(PDEVICE_OBJECT DeviceObject);

VOID IoUpdateShareAccess(PFILE_OBJECT FileObject, PSHARE_ACCESS ShareAccess);

VOID IoWriteErrorLogEntry(PVOID ElEntry);

NTSTATUS IoWritePartitionTable(PDEVICE_OBJECT DeviceObject,
			       ULONG SectorSize,
			       ULONG SectorsPerTrack,
			       ULONG NumberOfHeads,
			       struct _DRIVE_LAYOUT_INFORMATION* PBuffer);

typedef ULONG FS_INFORMATION_CLASS;

// Preliminary guess
NTKERNELAPI NTSTATUS IoQueryFileVolumeInformation(IN PFILE_OBJECT FileObject, 
				  IN FS_INFORMATION_CLASS FsInformationClass, 
						  IN ULONG Length, 
						  OUT PVOID FsInformation, 
						  OUT PULONG ReturnedLength);

NTKERNELAPI // confirmed - Undocumented because it does not require a valid file handle 
NTSTATUS 
IoQueryFileInformation(
IN PFILE_OBJECT FileObject,
IN FILE_INFORMATION_CLASS FileInformationClass,
IN ULONG Length,
OUT PVOID FileInformation,
OUT PULONG ReturnedLength
);
