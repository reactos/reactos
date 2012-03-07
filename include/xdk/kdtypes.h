/******************************************************************************
 *                          Kernel Debugger Types                             *
 ******************************************************************************/
$if (_NTDDK_)
typedef struct _DEBUG_DEVICE_ADDRESS {
  UCHAR Type;
  BOOLEAN Valid;
  UCHAR Reserved[2];
  PUCHAR TranslatedAddress;
  ULONG Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct _DEBUG_MEMORY_REQUIREMENTS {
  PHYSICAL_ADDRESS Start;
  PHYSICAL_ADDRESS MaxEnd;
  PVOID VirtualAddress;
  ULONG Length;
  BOOLEAN Cached;
  BOOLEAN Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef struct _DEBUG_DEVICE_DESCRIPTOR {
  ULONG Bus;
  ULONG Slot;
  USHORT Segment;
  USHORT VendorID;
  USHORT DeviceID;
  UCHAR BaseClass;
  UCHAR SubClass;
  UCHAR ProgIf;
  BOOLEAN Initialized;
  BOOLEAN Configured;
  DEBUG_DEVICE_ADDRESS BaseAddress[6];
  DEBUG_MEMORY_REQUIREMENTS Memory;
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;

typedef NTSTATUS
(NTAPI *pKdSetupPciDeviceForDebugging)(
  _In_opt_ PVOID LoaderBlock,
  _Inout_ PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef NTSTATUS
(NTAPI *pKdReleasePciDeviceForDebugging)(
  _Inout_ PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef PVOID
(NTAPI *pKdGetAcpiTablePhase0)(
  _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  _In_ ULONG Signature);

typedef VOID
(NTAPI *pKdCheckPowerButton)(VOID);

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ ULONG NumberPages,
  _In_ BOOLEAN FlushCurrentTLB);

typedef VOID
(NTAPI *pKdUnmapVirtualAddress)(
  _In_ PVOID VirtualAddress,
  _In_ ULONG NumberPages,
  _In_ BOOLEAN FlushCurrentTLB);
#else
typedef PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ ULONG NumberPages);

typedef VOID
(NTAPI *pKdUnmapVirtualAddress)(
  _In_ PVOID VirtualAddress,
  _In_ ULONG NumberPages);
#endif

typedef ULONG
(NTAPI *pKdGetPciDataByOffset)(
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);

typedef ULONG
(NTAPI *pKdSetPciDataByOffset)(
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);
$endif (_NTDDK_)
