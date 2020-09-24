/******************************************************************************
 *                         I/O Manager Functions                              *
 ******************************************************************************/

$if (_WDMDDK_)
/*
 * NTSTATUS
 * IoAcquireRemoveLock(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN OPTIONAL PVOID  Tag)
 */
#if DBG
#define IoAcquireRemoveLock(RemoveLock, Tag) \
  IoAcquireRemoveLockEx(RemoveLock, Tag, __FILE__, __LINE__, sizeof (IO_REMOVE_LOCK))
#else
#define IoAcquireRemoveLock(RemoveLock, Tag) \
  IoAcquireRemoveLockEx(RemoveLock, Tag, "", 1, sizeof (IO_REMOVE_LOCK))
#endif

/*
 * VOID
 * IoAdjustPagingPathCount(
 *   IN PLONG  Count,
 *   IN BOOLEAN  Increment)
 */
#define IoAdjustPagingPathCount(_Count, \
                                _Increment) \
{ \
  if (_Increment) \
    { \
      InterlockedIncrement(_Count); \
    } \
  else \
    { \
      InterlockedDecrement(_Count); \
    } \
}

#if !defined(_M_AMD64)
NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_UCHAR(
  IN PUCHAR Port,
  IN PUCHAR Buffer,
  IN ULONG Count);

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_ULONG(
  IN PULONG Port,
  IN PULONG Buffer,
  IN ULONG Count);

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_USHORT(
  IN PUSHORT Port,
  IN PUSHORT Buffer,
  IN ULONG Count);

NTHALAPI
UCHAR
NTAPI
READ_PORT_UCHAR(
  IN PUCHAR Port);

NTHALAPI
ULONG
NTAPI
READ_PORT_ULONG(
  IN PULONG Port);

NTHALAPI
USHORT
NTAPI
READ_PORT_USHORT(
  IN PUSHORT Port);

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_UCHAR(
  IN PUCHAR Register,
  IN PUCHAR Buffer,
  IN ULONG Count);

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_ULONG(
  IN PULONG Register,
  IN PULONG Buffer,
  IN ULONG Count);

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_USHORT(
  IN PUSHORT Register,
  IN PUSHORT Buffer,
  IN ULONG Count);

NTKERNELAPI
UCHAR
NTAPI
READ_REGISTER_UCHAR(
  IN PUCHAR Register);

NTKERNELAPI
ULONG
NTAPI
READ_REGISTER_ULONG(
  IN PULONG Register);

NTKERNELAPI
USHORT
NTAPI
READ_REGISTER_USHORT(
  IN PUSHORT Register);

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_UCHAR(
  IN PUCHAR Port,
  IN PUCHAR Buffer,
  IN ULONG Count);

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_ULONG(
  IN PULONG Port,
  IN PULONG Buffer,
  IN ULONG Count);

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_USHORT(
  IN PUSHORT Port,
  IN PUSHORT Buffer,
  IN ULONG Count);

NTHALAPI
VOID
NTAPI
WRITE_PORT_UCHAR(
  IN PUCHAR Port,
  IN UCHAR Value);

NTHALAPI
VOID
NTAPI
WRITE_PORT_ULONG(
  IN PULONG Port,
  IN ULONG Value);

NTHALAPI
VOID
NTAPI
WRITE_PORT_USHORT(
  IN PUSHORT Port,
  IN USHORT Value);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_UCHAR(
  IN PUCHAR Register,
  IN PUCHAR Buffer,
  IN ULONG Count);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_ULONG(
  IN PULONG Register,
  IN PULONG Buffer,
  IN ULONG Count);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_USHORT(
  IN PUSHORT Register,
  IN PUSHORT Buffer,
  IN ULONG Count);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_UCHAR(
  IN PUCHAR Register,
  IN UCHAR Value);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_ULONG(
  IN PULONG Register,
  IN ULONG Value);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_USHORT(
  IN PUSHORT Register,
  IN USHORT Value);

#else

FORCEINLINE
VOID
READ_PORT_BUFFER_UCHAR(
  IN PUCHAR Port,
  IN PUCHAR Buffer,
  IN ULONG Count)
{
  __inbytestring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
READ_PORT_BUFFER_ULONG(
  IN PULONG Port,
  IN PULONG Buffer,
  IN ULONG Count)
{
  __indwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
READ_PORT_BUFFER_USHORT(
  IN PUSHORT Port,
  IN PUSHORT Buffer,
  IN ULONG Count)
{
  __inwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
UCHAR
READ_PORT_UCHAR(
  IN PUCHAR Port)
{
  return __inbyte((USHORT)(ULONG_PTR)Port);
}

FORCEINLINE
ULONG
READ_PORT_ULONG(
  IN PULONG Port)
{
  return __indword((USHORT)(ULONG_PTR)Port);
}

FORCEINLINE
USHORT
READ_PORT_USHORT(
  IN PUSHORT Port)
{
  return __inword((USHORT)(ULONG_PTR)Port);
}

FORCEINLINE
VOID
READ_REGISTER_BUFFER_UCHAR(
  IN PUCHAR Register,
  IN PUCHAR Buffer,
  IN ULONG Count)
{
  __movsb(Register, Buffer, Count);
}

FORCEINLINE
VOID
READ_REGISTER_BUFFER_ULONG(
  IN PULONG Register,
  IN PULONG Buffer,
  IN ULONG Count)
{
  __movsd(Register, Buffer, Count);
}

FORCEINLINE
VOID
READ_REGISTER_BUFFER_USHORT(
  IN PUSHORT Register,
  IN PUSHORT Buffer,
  IN ULONG Count)
{
  __movsw(Register, Buffer, Count);
}

FORCEINLINE
UCHAR
READ_REGISTER_UCHAR(
  IN volatile UCHAR *Register)
{
  return *Register;
}

FORCEINLINE
ULONG
READ_REGISTER_ULONG(
  IN volatile ULONG *Register)
{
  return *Register;
}

FORCEINLINE
USHORT
READ_REGISTER_USHORT(
  IN volatile USHORT *Register)
{
  return *Register;
}

FORCEINLINE
VOID
WRITE_PORT_BUFFER_UCHAR(
  IN PUCHAR Port,
  IN PUCHAR Buffer,
  IN ULONG Count)
{
  __outbytestring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
WRITE_PORT_BUFFER_ULONG(
  IN PULONG Port,
  IN PULONG Buffer,
  IN ULONG Count)
{
  __outdwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
WRITE_PORT_BUFFER_USHORT(
  IN PUSHORT Port,
  IN PUSHORT Buffer,
  IN ULONG Count)
{
  __outwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
WRITE_PORT_UCHAR(
  IN PUCHAR Port,
  IN UCHAR Value)
{
  __outbyte((USHORT)(ULONG_PTR)Port, Value);
}

FORCEINLINE
VOID
WRITE_PORT_ULONG(
  IN PULONG Port,
  IN ULONG Value)
{
  __outdword((USHORT)(ULONG_PTR)Port, Value);
}

FORCEINLINE
VOID
WRITE_PORT_USHORT(
  IN PUSHORT Port,
  IN USHORT Value)
{
  __outword((USHORT)(ULONG_PTR)Port, Value);
}

FORCEINLINE
VOID
WRITE_REGISTER_BUFFER_UCHAR(
  IN PUCHAR Register,
  IN PUCHAR Buffer,
  IN ULONG Count)
{
  LONG Synch;
  __movsb(Register, Buffer, Count);
  InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_BUFFER_ULONG(
  IN PULONG Register,
  IN PULONG Buffer,
  IN ULONG Count)
{
  LONG Synch;
  __movsd(Register, Buffer, Count);
  InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_BUFFER_USHORT(
  IN PUSHORT Register,
  IN PUSHORT Buffer,
  IN ULONG Count)
{
  LONG Synch;
  __movsw(Register, Buffer, Count);
  InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_UCHAR(
  IN volatile UCHAR *Register,
  IN UCHAR Value)
{
  LONG Synch;
  *Register = Value;
  InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_ULONG(
  IN volatile ULONG *Register,
  IN ULONG Value)
{
  LONG Synch;
  *Register = Value;
  InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_USHORT(
  IN volatile USHORT *Register,
  IN USHORT Value)
{
  LONG Sync;
  *Register = Value;
  InterlockedOr(&Sync, 1);
}
#endif

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && \
   (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)

#define DMA_MACROS_DEFINED

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
FORCEINLINE
NTSTATUS
IoAllocateAdapterChannel(
  _In_ PDMA_ADAPTER DmaAdapter,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG NumberOfMapRegisters,
  _In_ PDRIVER_CONTROL ExecutionRoutine,
  _In_ PVOID Context)
{
  PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
  AllocateAdapterChannel =
      *(DmaAdapter)->DmaOperations->AllocateAdapterChannel;
  ASSERT(AllocateAdapterChannel);
  return AllocateAdapterChannel(DmaAdapter,
                                DeviceObject,
                                NumberOfMapRegisters,
                                ExecutionRoutine,
                                Context);
}

FORCEINLINE
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  _In_ PDMA_ADAPTER DmaAdapter,
  _In_ PMDL Mdl,
  _In_ PVOID MapRegisterBase,
  _In_ PVOID CurrentVa,
  _In_ ULONG Length,
  _In_ BOOLEAN WriteToDevice)
{
  PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
  FlushAdapterBuffers = *(DmaAdapter)->DmaOperations->FlushAdapterBuffers;
  ASSERT(FlushAdapterBuffers);
  return FlushAdapterBuffers(DmaAdapter,
                             Mdl,
                             MapRegisterBase,
                             CurrentVa,
                             Length,
                             WriteToDevice);
}

FORCEINLINE
VOID
NTAPI
IoFreeAdapterChannel(
  _In_ PDMA_ADAPTER DmaAdapter)
{
  PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
  FreeAdapterChannel = *(DmaAdapter)->DmaOperations->FreeAdapterChannel;
  ASSERT(FreeAdapterChannel);
  FreeAdapterChannel(DmaAdapter);
}

FORCEINLINE
VOID
NTAPI
IoFreeMapRegisters(
  _In_ PDMA_ADAPTER DmaAdapter,
  _In_ PVOID MapRegisterBase,
  _In_ ULONG NumberOfMapRegisters)
{
  PFREE_MAP_REGISTERS FreeMapRegisters;
  FreeMapRegisters = *(DmaAdapter)->DmaOperations->FreeMapRegisters;
  ASSERT(FreeMapRegisters);
  FreeMapRegisters(DmaAdapter, MapRegisterBase, NumberOfMapRegisters);
}

FORCEINLINE
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  _In_ PDMA_ADAPTER DmaAdapter,
  _In_ PMDL Mdl,
  _In_ PVOID MapRegisterBase,
  _In_ PVOID CurrentVa,
  _Inout_ PULONG Length,
  _In_ BOOLEAN WriteToDevice)
{
  PMAP_TRANSFER MapTransfer;

  MapTransfer = *(DmaAdapter)->DmaOperations->MapTransfer;
  ASSERT(MapTransfer);
  return MapTransfer(DmaAdapter,
                     Mdl,
                     MapRegisterBase,
                     CurrentVa,
                     Length,
                     WriteToDevice);
}
#endif

$endif (_WDMDDK_)
$if (_NTDDK_)
/*
 * VOID IoAssignArcName(
 *   IN PUNICODE_STRING  ArcName,
 *   IN PUNICODE_STRING  DeviceName);
 */
#define IoAssignArcName(_ArcName, _DeviceName) ( \
  IoCreateSymbolicLink((_ArcName), (_DeviceName)))

/*
 * VOID
 * IoDeassignArcName(
 *   IN PUNICODE_STRING  ArcName)
 */
#define IoDeassignArcName IoDeleteSymbolicLink

FORCEINLINE
VOID
NTAPI
IoInitializeDriverCreateContext(
  PIO_DRIVER_CREATE_CONTEXT DriverContext)
{
  RtlZeroMemory(DriverContext, sizeof(IO_DRIVER_CREATE_CONTEXT));
  DriverContext->Size = sizeof(IO_DRIVER_CREATE_CONTEXT);
}

$endif (_NTDDK_)
$if (_NTIFS_)
#define IoIsFileOpenedExclusively(FileObject) ( \
    (BOOLEAN) !(                                \
    (FileObject)->SharedRead ||                 \
    (FileObject)->SharedWrite ||                \
    (FileObject)->SharedDelete                  \
    )                                           \
)

#if (NTDDI_VERSION == NTDDI_WIN2K)
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChangeEx(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine);
#endif
$endif (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_WDMDDK_)
_Acquires_lock_(_Global_cancel_spin_lock_)
_Requires_lock_not_held_(_Global_cancel_spin_lock_)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_raises_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoAcquireCancelSpinLock(
  _Out_ _At_(*Irql, _IRQL_saves_) PKIRQL Irql);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoAcquireRemoveLockEx(
  _Inout_ PIO_REMOVE_LOCK RemoveLock,
  _In_opt_ PVOID Tag,
  _In_ PCSTR File,
  _In_ ULONG Line,
  _In_ ULONG RemlockSize);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Ret_range_(<=, 0)
NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateDriverObjectExtension(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PVOID ClientIdentificationAddress,
  _In_ ULONG DriverObjectExtensionSize,
  _Post_ _At_(*DriverObjectExtension, _When_(return==0,
    __drv_aliasesMem __drv_allocatesMem(Mem) _Post_notnull_))
  _When_(return == 0, _Outptr_result_bytebuffer_(DriverObjectExtensionSize))
    PVOID *DriverObjectExtension);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PVOID
NTAPI
IoAllocateErrorLogEntry(
  _In_ PVOID IoObject,
  _In_ UCHAR EntrySize);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PIRP
NTAPI
IoAllocateIrp(
  _In_ CCHAR StackSize,
  _In_ BOOLEAN ChargeQuota);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PMDL
NTAPI
IoAllocateMdl(
  _In_opt_ __drv_aliasesMem PVOID VirtualAddress,
  _In_ ULONG Length,
  _In_ BOOLEAN SecondaryBuffer,
  _In_ BOOLEAN ChargeQuota,
  _Inout_opt_ PIRP Irp);

__drv_allocatesMem(Mem)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PIO_WORKITEM
NTAPI
IoAllocateWorkItem(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(<=, 0)
NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDevice(
  _In_ _Kernel_requires_resource_held_(Memory) _When_(return==0, __drv_aliasesMem)
    PDEVICE_OBJECT SourceDevice,
  _In_ PUNICODE_STRING TargetDevice,
  _Out_ PDEVICE_OBJECT *AttachedDevice);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoAttachDeviceToDeviceStack(
  _In_ _Kernel_requires_resource_held_(Memory) _When_(return!=0, __drv_aliasesMem)
    PDEVICE_OBJECT SourceDevice,
  _In_ PDEVICE_OBJECT TargetDevice);

_Must_inspect_result_
__drv_aliasesMem
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PIRP
NTAPI
IoBuildAsynchronousFsdRequest(
  _In_ ULONG MajorFunction,
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_opt_ PVOID Buffer,
  _In_opt_ ULONG Length,
  _In_opt_ PLARGE_INTEGER StartingOffset,
  _In_opt_ PIO_STATUS_BLOCK IoStatusBlock);

_Must_inspect_result_
__drv_aliasesMem
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PIRP
NTAPI
IoBuildDeviceIoControlRequest(
  _In_ ULONG IoControlCode,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_opt_ PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_opt_ PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength,
  _In_ BOOLEAN InternalDeviceIoControl,
  _In_opt_ PKEVENT Event,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoBuildPartialMdl(
  _In_ PMDL SourceMdl,
  _Inout_ PMDL TargetMdl,
  _In_ PVOID VirtualAddress,
  _In_ ULONG Length);

_Must_inspect_result_
__drv_aliasesMem
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PIRP
NTAPI
IoBuildSynchronousFsdRequest(
  _In_ ULONG MajorFunction,
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_opt_ PVOID Buffer,
  _In_opt_ ULONG Length,
  _In_opt_ PLARGE_INTEGER StartingOffset,
  _In_ PKEVENT Event,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Success_(TRUE)
NTKERNELAPI
NTSTATUS
FASTCALL
IofCallDriver(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ __drv_aliasesMem PIRP Irp);
#define IoCallDriver IofCallDriver

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
IofCompleteRequest(
  _In_ PIRP Irp,
  _In_ CCHAR PriorityBoost);
#define IoCompleteRequest IofCompleteRequest

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
IoCancelIrp(
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoCheckShareAccess(
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG DesiredShareAccess,
  _Inout_ PFILE_OBJECT FileObject,
  _Inout_ PSHARE_ACCESS ShareAccess,
  _In_ BOOLEAN Update);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
IofCompleteRequest(
  _In_ PIRP Irp,
  _In_ CCHAR PriorityBoost);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoConnectInterrupt(
  _Out_ PKINTERRUPT *InterruptObject,
  _In_ PKSERVICE_ROUTINE ServiceRoutine,
  _In_opt_ PVOID ServiceContext,
  _In_opt_ PKSPIN_LOCK SpinLock,
  _In_ ULONG Vector,
  _In_ KIRQL Irql,
  _In_ KIRQL SynchronizeIrql,
  _In_ KINTERRUPT_MODE InterruptMode,
  _In_ BOOLEAN ShareVector,
  _In_ KAFFINITY ProcessorEnableMask,
  _In_ BOOLEAN FloatingSave);

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(<=, 0)
NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDevice(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ ULONG DeviceExtensionSize,
  _In_opt_ PUNICODE_STRING DeviceName,
  _In_ DEVICE_TYPE DeviceType,
  _In_ ULONG DeviceCharacteristics,
  _In_ BOOLEAN Exclusive,
  _Outptr_result_nullonfailure_
  _At_(*DeviceObject,
    __drv_allocatesMem(Mem)
    _When_(((_In_function_class_(DRIVER_INITIALIZE))
      ||(_In_function_class_(DRIVER_DISPATCH))),
      __drv_aliasesMem))
    PDEVICE_OBJECT *DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoCreateFile(
  _Out_ PHANDLE FileHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_opt_ PLARGE_INTEGER AllocationSize,
  _In_ ULONG FileAttributes,
  _In_ ULONG ShareAccess,
  _In_ ULONG Disposition,
  _In_ ULONG CreateOptions,
  _In_opt_ PVOID EaBuffer,
  _In_ ULONG EaLength,
  _In_ CREATE_FILE_TYPE CreateFileType,
  _In_opt_ PVOID InternalParameters,
  _In_ ULONG Options);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PKEVENT
NTAPI
IoCreateNotificationEvent(
  _In_ PUNICODE_STRING EventName,
  _Out_ PHANDLE EventHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoCreateSymbolicLink(
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ PUNICODE_STRING DeviceName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PKEVENT
NTAPI
IoCreateSynchronizationEvent(
  _In_ PUNICODE_STRING EventName,
  _Out_ PHANDLE EventHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoCreateUnprotectedSymbolicLink(
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ PUNICODE_STRING DeviceName);

_IRQL_requires_max_(APC_LEVEL)
_Kernel_clear_do_init_(__yes)
NTKERNELAPI
VOID
NTAPI
IoDeleteDevice(
  _In_ _Kernel_requires_resource_held_(Memory) __drv_freesMem(Mem)
    PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoDeleteSymbolicLink(
  _In_ PUNICODE_STRING SymbolicLinkName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoDetachDevice(
  _Inout_ PDEVICE_OBJECT TargetDevice);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoDisconnectInterrupt(
  _In_ PKINTERRUPT InterruptObject);

__drv_freesMem(Mem)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoFreeIrp(
  _In_ PIRP Irp);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoFreeMdl(
  PMDL Mdl);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoFreeWorkItem(
  _In_ __drv_freesMem(Mem) PIO_WORKITEM IoWorkItem);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetAttachedDevice(
  IN PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetAttachedDeviceReference(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetBootDiskInformation(
  _Inout_ PBOOTDISK_INFORMATION BootDiskInformation,
  _In_ ULONG Size);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceInterfaceAlias(
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ CONST GUID *AliasInterfaceClassGuid,
  _Out_
  _When_(return==0, _At_(AliasSymbolicLinkName->Buffer, __drv_allocatesMem(Mem)))
    PUNICODE_STRING AliasSymbolicLinkName);

NTKERNELAPI
PEPROCESS
NTAPI
IoGetCurrentProcess(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceInterfaces(
  _In_ CONST GUID *InterfaceClassGuid,
  _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_ ULONG Flags,
  _Outptr_result_nullonfailure_
  _At_(*SymbolicLinkList, _When_(return==0, __drv_allocatesMem(Mem)))
    PZZWSTR *SymbolicLinkList);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceObjectPointer(
  _In_ PUNICODE_STRING ObjectName,
  _In_ ACCESS_MASK DesiredAccess,
  _Out_ PFILE_OBJECT *FileObject,
  _Out_ PDEVICE_OBJECT *DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_((DeviceProperty & __string_type),
  _At_(PropertyBuffer, _Post_z_))
_When_((DeviceProperty & __multiString_type),
  _At_(PropertyBuffer, _Post_ _NullNull_terminated_))
NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceProperty(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
  _In_ ULONG BufferLength,
  _Out_writes_bytes_opt_(BufferLength) PVOID PropertyBuffer,
  _Deref_out_range_(<=, BufferLength) PULONG ResultLength);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PDMA_ADAPTER
NTAPI
IoGetDmaAdapter(
  _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_ PDEVICE_DESCRIPTION DeviceDescription,
  _Out_ _When_(return!=0, _Kernel_IoGetDmaAdapter_ _At_(*NumberOfMapRegisters, _Must_inspect_result_))
    PULONG NumberOfMapRegisters);

__drv_aliasesMem
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PVOID
NTAPI
IoGetDriverObjectExtension(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PVOID ClientIdentificationAddress);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
IoGetInitialStack(VOID);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetRelatedDeviceObject(
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoQueueWorkItem(
  _Inout_ PIO_WORKITEM IoWorkItem,
  _In_ PIO_WORKITEM_ROUTINE WorkerRoutine,
  _In_ WORK_QUEUE_TYPE QueueType,
  _In_opt_ __drv_aliasesMem PVOID Context);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoInitializeIrp(
  _Inout_ PIRP Irp,
  _In_ USHORT PacketSize,
  _In_ CCHAR StackSize);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoInitializeRemoveLockEx(
  _Out_ PIO_REMOVE_LOCK Lock,
  _In_ ULONG AllocateTag,
  _In_ ULONG MaxLockedMinutes,
  _In_ ULONG HighWatermark,
  _In_ ULONG RemlockSize);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoInitializeTimer(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIO_TIMER_ROUTINE TimerRoutine,
  _In_opt_ __drv_aliasesMem PVOID Context);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoInvalidateDeviceRelations(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ DEVICE_RELATION_TYPE Type);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoInvalidateDeviceState(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
IoIsWdmVersionAvailable(
  _When_(MajorVersion!=1&&MajorVersion!=6, _In_ __drv_reportError("MajorVersion must be 1 or 6"))
    UCHAR MajorVersion,
  _In_ _When_(MinorVersion!=0 && MinorVersion!=5 &&
              MinorVersion!=16 && MinorVersion!=32 &&
              MinorVersion!=48, __drv_reportError("MinorVersion must be 0, 0x5, 0x10, 0x20, or 0x30"))
    UCHAR MinorVersion);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoOpenDeviceInterfaceRegistryKey(
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ ACCESS_MASK DesiredAccess,
  _Out_ PHANDLE DeviceInterfaceKey);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoOpenDeviceRegistryKey(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG DevInstKeyType,
  _In_ ACCESS_MASK DesiredAccess,
  _Out_ PHANDLE DevInstRegKey);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterDeviceInterface(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_ CONST GUID *InterfaceClassGuid,
  _In_opt_ PUNICODE_STRING ReferenceString,
  _Out_ _When_(return==0, _At_(SymbolicLinkName->Buffer, __drv_allocatesMem(Mem)))
    PUNICODE_STRING SymbolicLinkName);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterPlugPlayNotification(
  _In_ IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
  _In_ ULONG EventCategoryFlags,
  _In_opt_ PVOID EventCategoryData,
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
  _Inout_opt_ __drv_aliasesMem PVOID Context,
  _Outptr_result_nullonfailure_
  _At_(*NotificationEntry, _When_(return==0, __drv_allocatesMem(Mem)))
    PVOID *NotificationEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterShutdownNotification(
  _In_ PDEVICE_OBJECT DeviceObject);

_Requires_lock_held_(_Global_cancel_spin_lock_)
_Releases_lock_(_Global_cancel_spin_lock_)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoReleaseCancelSpinLock(
  _In_ _IRQL_restores_ _IRQL_uses_cancel_ KIRQL Irql);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoReleaseRemoveLockAndWaitEx(
  _Inout_ PIO_REMOVE_LOCK RemoveLock,
  _In_opt_ PVOID Tag,
  _In_ ULONG RemlockSize);

NTKERNELAPI
VOID
NTAPI
IoReleaseRemoveLockEx(
  _Inout_ PIO_REMOVE_LOCK RemoveLock,
  _In_opt_ PVOID Tag,
  _In_ ULONG RemlockSize);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoRemoveShareAccess(
  _In_ PFILE_OBJECT FileObject,
  _Inout_ PSHARE_ACCESS ShareAccess);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoReportTargetDeviceChange(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_ PVOID NotificationStructure);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoReportTargetDeviceChangeAsynchronous(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_ PVOID NotificationStructure,
  _In_opt_ PDEVICE_CHANGE_COMPLETE_CALLBACK Callback,
  _In_opt_ PVOID Context);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoRequestDeviceEject(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoReuseIrp(
  _Inout_ PIRP Irp,
  _In_ NTSTATUS Status);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoSetDeviceInterfaceState(
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ BOOLEAN Enable);

NTKERNELAPI
VOID
NTAPI
IoSetShareAccess(
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG DesiredShareAccess,
  _Inout_ PFILE_OBJECT FileObject,
  _Out_ PSHARE_ACCESS ShareAccess);

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoStartNextPacket(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN Cancelable);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoStartNextPacketByKey(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN Cancelable,
  _In_ ULONG Key);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoStartPacket(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_opt_ PULONG Key,
  _In_opt_ PDRIVER_CANCEL CancelFunction);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoStartTimer(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoStopTimer(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_freesMem(Pool)
NTKERNELAPI
NTSTATUS
NTAPI
IoUnregisterPlugPlayNotification(
  _In_ PVOID NotificationEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoUnregisterShutdownNotification(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoUpdateShareAccess(
  _In_ PFILE_OBJECT FileObject,
  _Inout_ PSHARE_ACCESS ShareAccess);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoWMIAllocateInstanceIds(
  _In_ GUID *Guid,
  _In_ ULONG InstanceCount,
  _Out_ ULONG *FirstInstanceId);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQuerySingleInstanceMultiple(
  _In_reads_(ObjectCount) PVOID *DataBlockObjectList,
  _In_reads_(ObjectCount) PUNICODE_STRING InstanceNames,
  _In_ ULONG ObjectCount,
  _Inout_ ULONG *InOutBufferSize,
  _Out_writes_bytes_opt_(*InOutBufferSize) PVOID OutBuffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoWMIRegistrationControl(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG Action);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISuggestInstanceName(
  _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_opt_ PUNICODE_STRING SymbolicLinkName,
  _In_ BOOLEAN CombineNames,
  _Out_ PUNICODE_STRING SuggestedInstanceName);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_Ret_range_(<=, 0)
NTKERNELAPI
NTSTATUS
NTAPI
IoWMIWriteEvent(
  _Inout_ _When_(return==0, __drv_aliasesMem) PVOID WnodeEventItem);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoWriteErrorLogEntry(
  _In_ PVOID ElEntry);

NTKERNELAPI
PIRP
NTAPI
IoGetTopLevelIrp(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterLastChanceShutdownNotification(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
VOID
NTAPI
IoSetTopLevelIrp(
  _In_opt_ PIRP Irp);

$endif (_WDMDDK_)
$if (_NTDDK_)
#if !(defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_))
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateAdapterChannel(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG NumberOfMapRegisters,
  _In_ PDRIVER_CONTROL ExecutionRoutine,
  _In_ PVOID Context);
#endif

#if !defined(DMA_MACROS_DEFINED)
//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  _In_ PADAPTER_OBJECT AdapterObject,
  _In_ PMDL Mdl,
  _In_ PVOID MapRegisterBase,
  _In_ PVOID CurrentVa,
  _Inout_ PULONG Length,
  _In_ BOOLEAN WriteToDevice);
#endif

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoAllocateController(
  _In_ PCONTROLLER_OBJECT ControllerObject,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PDRIVER_CONTROL ExecutionRoutine,
  _In_opt_ PVOID Context);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PCONTROLLER_OBJECT
NTAPI
IoCreateController(
  _In_ ULONG Size);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoDeleteController(
  _In_ PCONTROLLER_OBJECT ControllerObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoFreeController(
  _In_ PCONTROLLER_OBJECT ControllerObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceToVerify(
  _In_ PETHREAD Thread);

NTKERNELAPI
VOID
NTAPI
IoCancelFileOpen(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PGENERIC_MAPPING
NTAPI
IoGetFileObjectGenericMapping(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PIRP
NTAPI
IoMakeAssociatedIrp(
  _In_ PIRP Irp,
  _In_ CCHAR StackSize);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryDeviceDescription(
  _In_opt_ PINTERFACE_TYPE BusType,
  _In_opt_ PULONG BusNumber,
  _In_opt_ PCONFIGURATION_TYPE ControllerType,
  _In_opt_ PULONG ControllerNumber,
  _In_opt_ PCONFIGURATION_TYPE PeripheralType,
  _In_opt_ PULONG PeripheralNumber,
  _In_ PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
  _Inout_opt_ PVOID Context);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoRaiseHardError(
  _In_ PIRP Irp,
  _In_opt_ PVPB Vpb,
  _In_ PDEVICE_OBJECT RealDeviceObject);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
IoRaiseInformationalHardError(
  _In_ NTSTATUS ErrorStatus,
  _In_opt_ PUNICODE_STRING String,
  _In_opt_ PKTHREAD Thread);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoRegisterBootDriverReinitialization(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_REINITIALIZE DriverReinitializationRoutine,
  _In_opt_ PVOID Context);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoRegisterDriverReinitialization(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_REINITIALIZE DriverReinitializationRoutine,
  _In_opt_ PVOID Context);

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDeviceByPointer(
  _In_ PDEVICE_OBJECT SourceDevice,
  _In_ PDEVICE_OBJECT TargetDevice);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoReportDetectedDevice(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ INTERFACE_TYPE LegacyBusType,
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _In_opt_ PCM_RESOURCE_LIST ResourceList,
  _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
  _In_ BOOLEAN ResourceAssigned,
  _Inout_ PDEVICE_OBJECT *DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceForDetection(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_reads_bytes_opt_(DriverListSize) PCM_RESOURCE_LIST DriverList,
  _In_opt_ ULONG DriverListSize,
  _In_opt_ PDEVICE_OBJECT DeviceObject,
  _In_reads_bytes_opt_(DeviceListSize) PCM_RESOURCE_LIST DeviceList,
  _In_opt_ ULONG DeviceListSize,
  _Out_ PBOOLEAN ConflictDetected);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceUsage(
  _In_opt_ PUNICODE_STRING DriverClassName,
  _In_ PDRIVER_OBJECT DriverObject,
  _In_reads_bytes_opt_(DriverListSize) PCM_RESOURCE_LIST DriverList,
  _In_opt_ ULONG DriverListSize,
  _In_opt_ PDEVICE_OBJECT DeviceObject,
  _In_reads_bytes_opt_(DeviceListSize) PCM_RESOURCE_LIST DeviceList,
  _In_opt_ ULONG DeviceListSize,
  _In_ BOOLEAN OverrideConflict,
  _Out_ PBOOLEAN ConflictDetected);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoSetHardErrorOrVerifyDevice(
  _In_ PIRP Irp,
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoAssignResources(
  _In_ PUNICODE_STRING RegistryPath,
  _In_opt_ PUNICODE_STRING DriverClassName,
  _In_ PDRIVER_OBJECT DriverObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject,
  _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
  _Inout_ PCM_RESOURCE_LIST *AllocatedResources);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(
  _In_ BOOLEAN EnableHardErrors);

$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
IoAcquireVpbSpinLock(
  _Out_ PKIRQL Irql);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckDesiredAccess(
  _Inout_ PACCESS_MASK DesiredAccess,
  _In_ ACCESS_MASK GrantedAccess);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckEaBufferValidity(
  _In_ PFILE_FULL_EA_INFORMATION EaBuffer,
  _In_ ULONG EaLength,
  _Out_ PULONG ErrorOffset);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckFunctionAccess(
  _In_ ACCESS_MASK GrantedAccess,
  _In_ UCHAR MajorFunction,
  _In_ UCHAR MinorFunction,
  _In_ ULONG IoControlCode,
  _In_opt_ PVOID Argument1,
  _In_opt_ PVOID Argument2);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuerySetFileInformation(
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ ULONG Length,
  _In_ BOOLEAN SetOperation);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuerySetVolumeInformation(
  _In_ FS_INFORMATION_CLASS FsInformationClass,
  _In_ ULONG Length,
  _In_ BOOLEAN SetOperation);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuotaBufferValidity(
  _In_ PFILE_QUOTA_INFORMATION QuotaBuffer,
  _In_ ULONG QuotaLength,
  _Out_ PULONG ErrorOffset);

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObject(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObjectLite(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoFastQueryNetworkAttributes(
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG OpenOptions,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoPageRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL Mdl,
  _In_ PLARGE_INTEGER Offset,
  _In_ PKEVENT Event,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetBaseFileSystemDeviceObject(
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(VOID);

NTKERNELAPI
ULONG
NTAPI
IoGetRequestorProcessId(
  _In_ PIRP Irp);

NTKERNELAPI
PEPROCESS
NTAPI
IoGetRequestorProcess(
  _In_ PIRP Irp);

NTKERNELAPI
PIRP
NTAPI
IoGetTopLevelIrp(VOID);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsOperationSynchronous(
  _In_ PIRP Irp);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsSystemThread(
  _In_ PETHREAD Thread);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsValidNameGraftingBuffer(
  _In_ PIRP Irp,
  _In_ PREPARSE_DATA_BUFFER ReparseBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryFileInformation(
  _In_ PFILE_OBJECT FileObject,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ ULONG Length,
  _Out_ PVOID FileInformation,
  _Out_ PULONG ReturnedLength);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryVolumeInformation(
  _In_ PFILE_OBJECT FileObject,
  _In_ FS_INFORMATION_CLASS FsInformationClass,
  _In_ ULONG Length,
  _Out_ PVOID FsInformation,
  _Out_ PULONG ReturnedLength);

NTKERNELAPI
VOID
NTAPI
IoQueueThreadIrp(
  _In_ PIRP Irp);

NTKERNELAPI
VOID
NTAPI
IoRegisterFileSystem(
  _In_ __drv_aliasesMem PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChange(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine);

NTKERNELAPI
VOID
NTAPI
IoReleaseVpbSpinLock(
  _In_ KIRQL Irql);

NTKERNELAPI
VOID
NTAPI
IoSetDeviceToVerify(
  _In_ PETHREAD Thread,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetInformation(
  _In_ PFILE_OBJECT FileObject,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ ULONG Length,
  _In_ PVOID FileInformation);

NTKERNELAPI
VOID
NTAPI
IoSetTopLevelIrp(
  _In_opt_ PIRP Irp);

NTKERNELAPI
NTSTATUS
NTAPI
IoSynchronousPageWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL Mdl,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PKEVENT Event,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

NTKERNELAPI
PEPROCESS
NTAPI
IoThreadToProcess(
  _In_ PETHREAD Thread);

NTKERNELAPI
VOID
NTAPI
IoUnregisterFileSystem(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
VOID
NTAPI
IoUnregisterFsRegistrationChange(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyVolume(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN AllowRawMount);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetRequestorSessionId(
  _In_ PIRP Irp,
  _Out_ PULONG pSessionId);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)

NTKERNELAPI
BOOLEAN
NTAPI
IoIsFileOriginRemote(
  _In_ PFILE_OBJECT FileObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetFileOrigin(
  _In_ PFILE_OBJECT FileObject,
  _In_ BOOLEAN Remote);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2KSP3) */
$endif (_NTDDK_)

#if (NTDDI_VERSION >= NTDDI_WINXP)

$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
IoCsqInitialize(
  _Out_ PIO_CSQ Csq,
  _In_ PIO_CSQ_INSERT_IRP CsqInsertIrp,
  _In_ PIO_CSQ_REMOVE_IRP CsqRemoveIrp,
  _In_ PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
  _In_ PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock,
  _In_ PIO_CSQ_RELEASE_LOCK CsqReleaseLock,
  _In_ PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp);

NTKERNELAPI
VOID
NTAPI
IoCsqInsertIrp(
  _Inout_ PIO_CSQ Csq,
  _Inout_ PIRP Irp,
  _Out_opt_ PIO_CSQ_IRP_CONTEXT Context);

NTKERNELAPI
PIRP
NTAPI
IoCsqRemoveIrp(
  _Inout_ PIO_CSQ Csq,
  _Inout_ PIO_CSQ_IRP_CONTEXT Context);

NTKERNELAPI
PIRP
NTAPI
IoCsqRemoveNextIrp(
  _Inout_ PIO_CSQ Csq,
  _In_opt_ PVOID PeekContext);

NTKERNELAPI
BOOLEAN
NTAPI
IoForwardIrpSynchronously(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

#define IoForwardAndCatchIrp IoForwardIrpSynchronously

NTKERNELAPI
VOID
NTAPI
IoFreeErrorLogEntry(
  _In_ PVOID ElEntry);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoSetCompletionRoutineEx(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ PIO_COMPLETION_ROUTINE CompletionRoutine,
  _In_opt_ PVOID Context,
  _In_ BOOLEAN InvokeOnSuccess,
  _In_ BOOLEAN InvokeOnError,
  _In_ BOOLEAN InvokeOnCancel);

VOID
NTAPI
IoSetStartIoAttributes(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN DeferredStartIo,
  _In_ BOOLEAN NonCancelable);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIDeviceObjectToInstanceName(
  _In_ PVOID DataBlockObject,
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ PUNICODE_STRING InstanceName);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIExecuteMethod(
  _In_ PVOID DataBlockObject,
  _In_ PUNICODE_STRING InstanceName,
  _In_ ULONG MethodId,
  _In_ ULONG InBufferSize,
  _Inout_ PULONG OutBufferSize,
  _Inout_updates_bytes_to_opt_(*OutBufferSize, InBufferSize) PUCHAR InOutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIHandleToInstanceName(
  _In_ PVOID DataBlockObject,
  _In_ HANDLE FileHandle,
  _Out_ PUNICODE_STRING InstanceName);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIOpenBlock(
  _In_ LPCGUID DataBlockGuid,
  _In_ ULONG DesiredAccess,
  _Out_ PVOID *DataBlockObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQueryAllData(
  _In_ PVOID DataBlockObject,
  _Inout_ ULONG *InOutBufferSize,
  _Out_writes_bytes_opt_(*InOutBufferSize) PVOID OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQueryAllDataMultiple(
  _In_reads_(ObjectCount) PVOID *DataBlockObjectList,
  _In_ ULONG ObjectCount,
  _Inout_ ULONG *InOutBufferSize,
  _Out_writes_bytes_opt_(*InOutBufferSize) PVOID OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQuerySingleInstance(
  _In_ PVOID DataBlockObject,
  _In_ PUNICODE_STRING InstanceName,
  _Inout_ ULONG *InOutBufferSize,
  _Out_writes_bytes_opt_(*InOutBufferSize) PVOID OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISetNotificationCallback(
  _Inout_ PVOID Object,
  _In_ WMI_NOTIFICATION_CALLBACK Callback,
  _In_opt_ PVOID Context);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISetSingleInstance(
  _In_ PVOID DataBlockObject,
  _In_ PUNICODE_STRING InstanceName,
  _In_ ULONG Version,
  _In_ ULONG ValueBufferSize,
  _In_reads_bytes_(ValueBufferSize) PVOID ValueBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISetSingleItem(
  _In_ PVOID DataBlockObject,
  _In_ PUNICODE_STRING InstanceName,
  _In_ ULONG DataItemId,
  _In_ ULONG Version,
  _In_ ULONG ValueBufferSize,
  _In_reads_bytes_(ValueBufferSize) PVOID ValueBuffer);
$endif (_WDMDDK_)
$if (_NTDDK_)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
FASTCALL
IoReadPartitionTable(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ BOOLEAN ReturnRecognizedPartitions,
  _Out_ struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
FASTCALL
IoSetPartitionInformation(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ ULONG PartitionNumber,
  _In_ ULONG PartitionType);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG SectorSize,
  _In_ ULONG SectorsPerTrack,
  _In_ ULONG NumberOfHeads,
  _In_ struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDisk(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_opt_ struct _CREATE_DISK* Disk);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadDiskSignature(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG BytesPerSector,
  _Out_ PDISK_SIGNATURE Signature);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoReadPartitionTableEx(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ struct _DRIVE_LAYOUT_INFORMATION_EX **PartitionBuffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoSetPartitionInformationEx(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG PartitionNumber,
  _In_ struct _SET_PARTITION_INFORMATION_EX *PartitionInfo);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetSystemPartition(
  _In_ PUNICODE_STRING VolumeNameString);

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyPartitionTable(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN FixErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVolumeDeviceToDosName(
  _In_ PVOID VolumeDeviceObject,
  _Out_ _When_(return==0,
    _At_(DosName->Buffer, __drv_allocatesMem(Mem)))
    PUNICODE_STRING DosName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoWritePartitionTableEx(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_reads_(_Inexpressible_(FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0])))
    struct _DRIVE_LAYOUT_INFORMATION_EX *DriveLayout);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateFileSpecifyDeviceObjectHint(
  _Out_ PHANDLE FileHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_opt_ PLARGE_INTEGER AllocationSize,
  _In_ ULONG FileAttributes,
  _In_ ULONG ShareAccess,
  _In_ ULONG Disposition,
  _In_ ULONG CreateOptions,
  _In_opt_ PVOID EaBuffer,
  _In_ ULONG EaLength,
  _In_ CREATE_FILE_TYPE CreateFileType,
  _In_opt_ PVOID InternalParameters,
  _In_ ULONG Options,
  _In_opt_ PVOID DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDeviceToDeviceStackSafe(
  _In_ PDEVICE_OBJECT SourceDevice,
  _In_ PDEVICE_OBJECT TargetDevice,
  _Outptr_ PDEVICE_OBJECT *AttachedToDeviceObject);

$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObjectEx(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject,
  _Out_opt_ PHANDLE FileObjectHandle);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryFileDosDeviceName(
  _In_ PFILE_OBJECT FileObject,
  _Out_ POBJECT_NAME_INFORMATION *ObjectNameInformation);

NTKERNELAPI
NTSTATUS
NTAPI
IoEnumerateDeviceObjectList(
  _In_ PDRIVER_OBJECT DriverObject,
  _Out_writes_bytes_to_opt_(DeviceObjectListSize,(*ActualNumberDeviceObjects)*sizeof(PDEVICE_OBJECT))
    PDEVICE_OBJECT *DeviceObjectList,
  _In_ ULONG DeviceObjectListSize,
  _Out_ PULONG ActualNumberDeviceObjects);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetLowerDeviceObject(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceAttachmentBaseRef(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDiskDeviceObject(
  _In_ PDEVICE_OBJECT FileSystemDeviceObject,
  _Out_ PDEVICE_OBJECT *DiskDeviceObject);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXPSP1)
NTKERNELAPI
NTSTATUS
NTAPI
IoValidateDeviceIoControlAccess(
  _In_ PIRP Irp,
  _In_ ULONG RequiredAccess);
#endif

$endif (_WDMDDK_)
$if (_WDMDDK_ || _NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03)
$endif (_WDMDDK_ || _NTDDK_)
$if (_NTDDK_)
NTKERNELAPI
IO_PAGING_PRIORITY
FASTCALL
IoGetPagingIoPriority(
  _In_ PIRP Irp);

$endif (_NTDDK_)
$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
IoCsqInitializeEx(
  _Out_ PIO_CSQ Csq,
  _In_ PIO_CSQ_INSERT_IRP_EX CsqInsertIrp,
  _In_ PIO_CSQ_REMOVE_IRP CsqRemoveIrp,
  _In_ PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
  _In_ PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock,
  _In_ PIO_CSQ_RELEASE_LOCK CsqReleaseLock,
  _In_ PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp);

NTKERNELAPI
NTSTATUS
NTAPI
IoCsqInsertIrpEx(
  _Inout_ PIO_CSQ Csq,
  _Inout_ PIRP Irp,
  _Out_opt_ PIO_CSQ_IRP_CONTEXT Context,
  _In_opt_ PVOID InsertContext);
$endif (_WDMDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* (NTDDI_VERSION >= NTDDI_WS03) */
$endif (_WDMDDK_ || _NTDDK_)
$if (_NTDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WS03SP1)
$endif (_NTDDK_ || _NTIFS_)

$if (_NTDDK_)
BOOLEAN
NTAPI
IoTranslateBusAddress(
  _In_ INTERFACE_TYPE InterfaceType,
  _In_ ULONG BusNumber,
  _In_ PHYSICAL_ADDRESS BusAddress,
  _Inout_ PULONG AddressSpace,
  _Out_ PPHYSICAL_ADDRESS TranslatedAddress);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
NTSTATUS
NTAPI
IoEnumerateRegisteredFiltersList(
  _Out_writes_bytes_to_opt_(DriverObjectListSize,(*ActualNumberDriverObjects)*sizeof(PDRIVER_OBJECT))
    PDRIVER_OBJECT *DriverObjectList,
  _In_ ULONG DriverObjectListSize,
  _Out_ PULONG ActualNumberDriverObjects);
$endif (_NTIFS_)
$if (_NTDDK_ || _NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */
$endif (_NTDDK_ || _NTIFS_)

#if (NTDDI_VERSION >= NTDDI_VISTA)
$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
IoGetBootDiskInformationLite(
  _Outptr_ PBOOTDISK_INFORMATION_LITE *BootDiskInformation);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckShareAccessEx(
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG DesiredShareAccess,
  _Inout_ PFILE_OBJECT FileObject,
  _Inout_ PSHARE_ACCESS ShareAccess,
  _In_ BOOLEAN Update,
  _In_ PBOOLEAN WritePermission);

NTKERNELAPI
NTSTATUS
NTAPI
IoConnectInterruptEx(
  _Inout_ PIO_CONNECT_INTERRUPT_PARAMETERS Parameters);

NTKERNELAPI
VOID
NTAPI
IoDisconnectInterruptEx(
  _In_ PIO_DISCONNECT_INTERRUPT_PARAMETERS Parameters);

LOGICAL
NTAPI
IoWithinStackLimits(
  _In_ ULONG_PTR RegionStart,
  _In_ SIZE_T RegionSize);

NTKERNELAPI
VOID
NTAPI
IoSetShareAccessEx(
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG DesiredShareAccess,
  _Inout_ PFILE_OBJECT FileObject,
  _Out_ PSHARE_ACCESS ShareAccess,
  _In_ PBOOLEAN WritePermission);

ULONG
NTAPI
IoSizeofWorkItem(VOID);

VOID
NTAPI
IoInitializeWorkItem(
  _In_ PVOID IoObject,
  _Out_ PIO_WORKITEM IoWorkItem);

VOID
NTAPI
IoUninitializeWorkItem(
  _Inout_ PIO_WORKITEM IoWorkItem);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
VOID
NTAPI
IoQueueWorkItemEx(
  _Inout_ PIO_WORKITEM IoWorkItem,
  _In_ PIO_WORKITEM_ROUTINE_EX WorkerRoutine,
  _In_ WORK_QUEUE_TYPE QueueType,
  _In_opt_ __drv_aliasesMem PVOID Context);

NTKRNLVISTAAPI
IO_PRIORITY_HINT
NTAPI
IoGetIoPriorityHint(
  _In_ PIRP Irp);

NTSTATUS
NTAPI
IoSetIoPriorityHint(
  _In_ PIRP Irp,
  _In_ IO_PRIORITY_HINT PriorityHint);

NTSTATUS
NTAPI
IoAllocateSfioStreamIdentifier(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG Length,
  _In_ PVOID Signature,
  _Out_ PVOID *StreamIdentifier);

PVOID
NTAPI
IoGetSfioStreamIdentifier(
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID Signature);

NTSTATUS
NTAPI
IoFreeSfioStreamIdentifier(
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID Signature);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoRequestDeviceEjectEx(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_opt_ PIO_DEVICE_EJECT_CALLBACK Callback,
  _In_opt_ PVOID Context,
  _In_opt_ PDRIVER_OBJECT DriverObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoSetDevicePropertyData(
  _In_ PDEVICE_OBJECT Pdo,
  _In_ CONST DEVPROPKEY *PropertyKey,
  _In_ LCID Lcid,
  _In_ ULONG Flags,
  _In_ DEVPROPTYPE Type,
  _In_ ULONG Size,
  _In_opt_ PVOID Data);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoGetDevicePropertyData(
  _In_ PDEVICE_OBJECT Pdo,
  _In_ CONST DEVPROPKEY *PropertyKey,
  _In_ LCID Lcid,
  _Reserved_ ULONG Flags,
  _In_ ULONG Size,
  _Out_ PVOID Data,
  _Out_ PULONG RequiredSize,
  _Out_ PDEVPROPTYPE Type);

$endif (_WDMDDK_)
$if (_NTDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
IoUpdateDiskGeometry(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ struct _DISK_GEOMETRY_EX* OldDiskGeometry,
  _In_ struct _DISK_GEOMETRY_EX* NewDiskGeometry);

PTXN_PARAMETER_BLOCK
NTAPI
IoGetTransactionParameterBlock(
  _In_ PFILE_OBJECT FileObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateFileEx(
  _Out_ PHANDLE FileHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_opt_ PLARGE_INTEGER AllocationSize,
  _In_ ULONG FileAttributes,
  _In_ ULONG ShareAccess,
  _In_ ULONG Disposition,
  _In_ ULONG CreateOptions,
  _In_opt_ PVOID EaBuffer,
  _In_ ULONG EaLength,
  _In_ CREATE_FILE_TYPE CreateFileType,
  _In_opt_ PVOID InternalParameters,
  _In_ ULONG Options,
  _In_opt_ PIO_DRIVER_CREATE_CONTEXT DriverContext);

NTSTATUS
NTAPI
IoSetIrpExtraCreateParameter(
  _Inout_ PIRP Irp,
  _In_ struct _ECP_LIST *ExtraCreateParameter);

VOID
NTAPI
IoClearIrpExtraCreateParameter(
  _Inout_ PIRP Irp);

NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoGetIrpExtraCreateParameter(
  _In_ PIRP Irp,
  _Outptr_result_maybenull_ struct _ECP_LIST **ExtraCreateParameter);

BOOLEAN
NTAPI
IoIsFileObjectIgnoringSharing(
  _In_ PFILE_OBJECT FileObject);

$endif (_NTDDK_)
$if (_NTIFS_)

FORCEINLINE
VOID
NTAPI
IoInitializePriorityInfo(
    _In_ PIO_PRIORITY_INFO PriorityInfo)
{
    PriorityInfo->Size = sizeof(IO_PRIORITY_INFO);
    PriorityInfo->ThreadPriority = 0xffff;
    PriorityInfo->IoPriority = IoPriorityNormal;
    PriorityInfo->PagePriority = 0;
}
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

$if (_WDMDDK_)
#define IoCallDriverStackSafeDefault(a, b) IoCallDriver(a, b)

#if (NTDDI_VERSION >= NTDDI_WS08)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoReplacePartitionUnit(
  _In_ PDEVICE_OBJECT TargetPdo,
  _In_ PDEVICE_OBJECT SparePdo,
  _In_ ULONG Flags);
#endif

$endif (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WIN7)

$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
IoGetAffinityInterrupt(
  _In_ PKINTERRUPT InterruptObject,
  _Out_ PGROUP_AFFINITY GroupAffinity);

NTSTATUS
NTAPI
IoGetContainerInformation(
  _In_ IO_CONTAINER_INFORMATION_CLASS InformationClass,
  _In_opt_ PVOID ContainerObject,
  _Inout_updates_bytes_opt_(BufferLength) PVOID Buffer,
  _In_ ULONG BufferLength);

NTSTATUS
NTAPI
IoRegisterContainerNotification(
  _In_ IO_CONTAINER_NOTIFICATION_CLASS NotificationClass,
  _In_ PIO_CONTAINER_NOTIFICATION_FUNCTION CallbackFunction,
  _In_reads_bytes_opt_(NotificationInformationLength) PVOID NotificationInformation,
  _In_ ULONG NotificationInformationLength,
  _Out_ PVOID CallbackRegistration);

VOID
NTAPI
IoUnregisterContainerNotification(
  _In_ PVOID CallbackRegistration);

_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_freesMem(Pool)
NTKERNELAPI
NTSTATUS
NTAPI
IoUnregisterPlugPlayNotificationEx(
  _In_ PVOID NotificationEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceNumaNode(
  _In_ PDEVICE_OBJECT Pdo,
  _Out_ PUSHORT NodeNumber);

$endif (_WDMDDK_)
$if (_NTDDK_)
NTSTATUS
NTAPI
IoSetFileObjectIgnoreSharing(
  _In_ PFILE_OBJECT FileObject);

$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChangeMountAware(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine,
  _In_ BOOLEAN SynchronizeWithMounts);

NTKERNELAPI
NTSTATUS
NTAPI
IoReplaceFileObjectName(
  _In_ PFILE_OBJECT FileObject,
  _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
  _In_ USHORT FileNameLength);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#if (NTDDI_VERSION >= NTDDI_WIN8)

$if (_WDMDDK_)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKRNLVISTAAPI
NTSTATUS
IoSetDeviceInterfacePropertyData(
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ CONST DEVPROPKEY *PropertyKey,
  _In_ LCID Lcid,
  _In_ ULONG Flags,
  _In_ DEVPROPTYPE Type,
  _In_ ULONG Size,
  _In_reads_bytes_opt_(Size) PVOID Data);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
IoGetDeviceInterfacePropertyData (
  _In_ PUNICODE_STRING SymbolicLinkName,
  _In_ CONST DEVPROPKEY *PropertyKey,
  _In_ LCID Lcid,
  _Reserved_ ULONG Flags,
  _In_ ULONG Size,
  _Out_writes_bytes_to_(Size, *RequiredSize) PVOID Data,
  _Out_ PULONG RequiredSize,
  _Out_ PDEVPROPTYPE Type);
$endif (_WDMDDK_)
$if (_NTDDK_)

NTKRNLVISTAAPI
VOID
IoSetMasterIrpStatus(
  _Inout_ PIRP MasterIrp,
  _In_ NTSTATUS Status);
$endif (_NTDDK_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

$if (_WDMDDK_)
#if defined(_WIN64)
NTKERNELAPI
ULONG
NTAPI
IoWMIDeviceObjectToProviderId(
  _In_ PDEVICE_OBJECT DeviceObject);
#else
#define IoWMIDeviceObjectToProviderId(DeviceObject) ((ULONG)(DeviceObject))
#endif

/*
 * USHORT
 * IoSizeOfIrp(
 *   IN CCHAR  StackSize)
 */
#define IoSizeOfIrp(_StackSize) \
  ((USHORT) (sizeof(IRP) + ((_StackSize) * (sizeof(IO_STACK_LOCATION)))))

FORCEINLINE
VOID
IoSkipCurrentIrpStackLocation(
  _Inout_ PIRP Irp)
{
  ASSERT(Irp->CurrentLocation <= Irp->StackCount);
  Irp->CurrentLocation++;
#ifdef NONAMELESSUNION
  Irp->Tail.Overlay.s.u.CurrentStackLocation++;
#else
  Irp->Tail.Overlay.CurrentStackLocation++;
#endif
}

FORCEINLINE
VOID
IoSetNextIrpStackLocation(
  _Inout_ PIRP Irp)
{
  ASSERT(Irp->CurrentLocation > 0);
  Irp->CurrentLocation--;
#ifdef NONAMELESSUNION
  Irp->Tail.Overlay.s.u.CurrentStackLocation--;
#else
  Irp->Tail.Overlay.CurrentStackLocation--;
#endif
}

__drv_aliasesMem
FORCEINLINE
PIO_STACK_LOCATION
IoGetNextIrpStackLocation(
  _In_ PIRP Irp)
{
  ASSERT(Irp->CurrentLocation > 0);
#ifdef NONAMELESSUNION
  return ((Irp)->Tail.Overlay.s.u.CurrentStackLocation - 1 );
#else
  return ((Irp)->Tail.Overlay.CurrentStackLocation - 1 );
#endif
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
IoSetCompletionRoutine(
  _In_ PIRP Irp,
  _In_opt_ PIO_COMPLETION_ROUTINE CompletionRoutine,
  _In_opt_ __drv_aliasesMem PVOID Context,
  _In_ BOOLEAN InvokeOnSuccess,
  _In_ BOOLEAN InvokeOnError,
  _In_ BOOLEAN InvokeOnCancel)
{
  PIO_STACK_LOCATION irpSp;
  ASSERT( (InvokeOnSuccess || InvokeOnError || InvokeOnCancel) ? (CompletionRoutine != NULL) : TRUE );
  irpSp = IoGetNextIrpStackLocation(Irp);
  irpSp->CompletionRoutine = CompletionRoutine;
  irpSp->Context = Context;
  irpSp->Control = 0;

  if (InvokeOnSuccess) {
    irpSp->Control = SL_INVOKE_ON_SUCCESS;
  }

  if (InvokeOnError) {
    irpSp->Control |= SL_INVOKE_ON_ERROR;
  }

  if (InvokeOnCancel) {
    irpSp->Control |= SL_INVOKE_ON_CANCEL;
  }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_Ret_maybenull_
FORCEINLINE
PDRIVER_CANCEL
IoSetCancelRoutine(
    _Inout_ PIRP Irp,
    _In_opt_ PDRIVER_CANCEL CancelRoutine)
{
    return (PDRIVER_CANCEL)(ULONG_PTR) InterlockedExchangePointer((PVOID *)&(Irp)->CancelRoutine, (PVOID)(ULONG_PTR)(CancelRoutine));
}

FORCEINLINE
VOID
IoRequestDpc(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIRP Irp,
    _In_opt_ __drv_aliasesMem PVOID Context)
{
    KeInsertQueueDpc(&DeviceObject->Dpc, Irp, Context);
}

/*
 * VOID
 * IoReleaseRemoveLock(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN PVOID  Tag)
 */
#define IoReleaseRemoveLock(_RemoveLock, \
                            _Tag) \
  IoReleaseRemoveLockEx(_RemoveLock, _Tag, sizeof(IO_REMOVE_LOCK))

/*
 * VOID
 * IoReleaseRemoveLockAndWait(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN PVOID  Tag)
 */
#define IoReleaseRemoveLockAndWait(_RemoveLock, \
                                   _Tag) \
  IoReleaseRemoveLockAndWaitEx(_RemoveLock, _Tag, sizeof(IO_REMOVE_LOCK))

#if defined(_WIN64)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
IoIs32bitProcess(
  _In_opt_ PIRP Irp);
#endif

#define PLUGPLAY_REGKEY_DEVICE                            1
#define PLUGPLAY_REGKEY_DRIVER                            2
#define PLUGPLAY_REGKEY_CURRENT_HWPROFILE                 4

__drv_aliasesMem
FORCEINLINE
PIO_STACK_LOCATION
IoGetCurrentIrpStackLocation(
  _In_ PIRP Irp)
{
  ASSERT(Irp->CurrentLocation <= Irp->StackCount + 1);
#ifdef NONAMELESSUNION
  return Irp->Tail.Overlay.s.u.CurrentStackLocation;
#else
  return Irp->Tail.Overlay.CurrentStackLocation;
#endif
}

FORCEINLINE
VOID
IoMarkIrpPending(
  _Inout_ PIRP Irp)
{
    IoGetCurrentIrpStackLocation((Irp))->Control |= SL_PENDING_RETURNED;
}

/*
 * BOOLEAN
 * IoIsErrorUserInduced(
 *   IN NTSTATUS  Status);
 */
#define IoIsErrorUserInduced(Status) \
   ((BOOLEAN)(((Status) == STATUS_DEVICE_NOT_READY) || \
   ((Status) == STATUS_IO_TIMEOUT) || \
   ((Status) == STATUS_MEDIA_WRITE_PROTECTED) || \
   ((Status) == STATUS_NO_MEDIA_IN_DEVICE) || \
   ((Status) == STATUS_VERIFY_REQUIRED) || \
   ((Status) == STATUS_UNRECOGNIZED_MEDIA) || \
   ((Status) == STATUS_WRONG_VOLUME)))

/* VOID
 * IoInitializeRemoveLock(
 *   IN PIO_REMOVE_LOCK  Lock,
 *   IN ULONG  AllocateTag,
 *   IN ULONG  MaxLockedMinutes,
 *   IN ULONG  HighWatermark)
 */
#define IoInitializeRemoveLock( \
  Lock, AllocateTag, MaxLockedMinutes, HighWatermark) \
  IoInitializeRemoveLockEx(Lock, AllocateTag, MaxLockedMinutes, \
    HighWatermark, sizeof(IO_REMOVE_LOCK))

FORCEINLINE
VOID
IoInitializeDpcRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIO_DPC_ROUTINE DpcRoutine)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:28024)
#endif
  KeInitializeDpc(&DeviceObject->Dpc,
                  (PKDEFERRED_ROUTINE) DpcRoutine,
                  DeviceObject);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

#define DEVICE_INTERFACE_INCLUDE_NONACTIVE 0x00000001

/*
 * ULONG
 * IoGetFunctionCodeFromCtlCode(
 *   IN ULONG  ControlCode)
 */
#define IoGetFunctionCodeFromCtlCode(_ControlCode) \
  (((_ControlCode) >> 2) & 0x00000FFF)

FORCEINLINE
VOID
IoCopyCurrentIrpStackLocationToNext(
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION irpSp;
    PIO_STACK_LOCATION nextIrpSp;
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);
    RtlCopyMemory(nextIrpSp, irpSp, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
    nextIrpSp->Control = 0;
}

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
IoGetStackLimits(
  _Out_ PULONG_PTR LowLimit,
  _Out_ PULONG_PTR HighLimit);

_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
ULONG_PTR
IoGetRemainingStackSize(VOID)
{
    ULONG_PTR End, Begin;
    ULONG_PTR Result;

    IoGetStackLimits(&Begin, &End);
    Result = (ULONG_PTR)(&End) - Begin;
    return Result;
}

#if (NTDDI_VERSION >= NTDDI_WS03)
FORCEINLINE
VOID
IoInitializeThreadedDpcRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIO_DPC_ROUTINE DpcRoutine)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:28024)
#pragma warning(disable:28128)
#endif
    KeInitializeThreadedDpc(&DeviceObject->Dpc,
                            (PKDEFERRED_ROUTINE) DpcRoutine,
                            DeviceObject );
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}
#endif

$endif (_WDMDDK_)
