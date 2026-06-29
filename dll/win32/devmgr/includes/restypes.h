#pragma once


/* Physical Addresses are always treated as 64-bit wide */
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

#include <pshpack4.h>
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
  UCHAR Type;
  UCHAR ShareDisposition;
  USHORT Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Generic;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Port;
    struct {
#if defined(NT_PROCESSOR_GROUPS)
      USHORT Level;
      USHORT Group;
#else
      ULONG Level;
#endif
      ULONG Vector;
      KAFFINITY Affinity;
    } Interrupt;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      union {
        struct {
#if defined(NT_PROCESSOR_GROUPS)
          USHORT Group;
#else
          USHORT Reserved;
#endif
          USHORT MessageCount;
          ULONG Vector;
          KAFFINITY Affinity;
        } Raw;
        struct {
#if defined(NT_PROCESSOR_GROUPS)
          USHORT Level;
          USHORT Group;
#else
          ULONG Level;
#endif
          ULONG Vector;
          KAFFINITY Affinity;
        } Translated;
      } DUMMYUNIONNAME;
    } MessageInterrupt;
#endif
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Memory;
    struct {
      ULONG Channel;
      ULONG Port;
      ULONG Reserved1;
    } Dma;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Start;
      ULONG Length;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG DataSize;
      ULONG Reserved1;
      ULONG Reserved2;
    } DeviceSpecificData;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length40;
    } Memory40;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length48;
    } Memory48;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length64;
    } Memory64;
#endif
  } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;
#include <poppack.h>
typedef struct _CM_PARTIAL_RESOURCE_LIST {
  USHORT Version;
  USHORT Revision;
  ULONG Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

#define CmResourceTypeNull              0
#define CmResourceTypePort              1
#define CmResourceTypeInterrupt         2
#define CmResourceTypeMemory            3
#define CmResourceTypeDma               4
#define CmResourceTypeDeviceSpecific    5
#define CmResourceTypeBusNumber         6
#define CmResourceTypeNonArbitrated     128
#define CmResourceTypeConfigData        128
#define CmResourceTypeDevicePrivate     129
#define CmResourceTypePcCardConfig      130
#define CmResourceTypeMfCardConfig      131

#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0x0000
#define CM_RESOURCE_INTERRUPT_LATCHED         0x0001

typedef enum _INTERFACE_TYPE {
  InterfaceTypeUndefined = -1,
  Internal,
  Isa,
  Eisa,
  MicroChannel,
  TurboChannel,
  PCIBus,
  VMEBus,
  NuBus,
  PCMCIABus,
  CBus,
  MPIBus,
  MPSABus,
  ProcessorInternal,
  InternalPowerBus,
  PNPISABus,
  PNPBus,
  Vmcs,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
  INTERFACE_TYPE InterfaceType;
  ULONG BusNumber;
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST {
  ULONG                       Count;
  CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

PVOID
GetResourceList(
    LPWSTR pszDeviceID);
