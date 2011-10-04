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
  IN PVOID LoaderBlock OPTIONAL,
  IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef NTSTATUS
(NTAPI *pKdReleasePciDeviceForDebugging)(
  IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef PVOID
(NTAPI *pKdGetAcpiTablePhase0)(
  IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  IN ULONG Signature);

typedef VOID
(NTAPI *pKdCheckPowerButton)(
  VOID);

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG NumberPages,
  IN BOOLEAN FlushCurrentTLB);

typedef VOID
(NTAPI *pKdUnmapVirtualAddress)(
  IN PVOID VirtualAddress,
  IN ULONG NumberPages,
  IN BOOLEAN FlushCurrentTLB);
#else
typedef PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG NumberPages);

typedef VOID
(NTAPI *pKdUnmapVirtualAddress)(
  IN PVOID VirtualAddress,
  IN ULONG NumberPages);
#endif

typedef ULONG
(NTAPI *pKdGetPciDataByOffset)(
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

typedef ULONG
(NTAPI *pKdSetPciDataByOffset)(
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);
$endif
