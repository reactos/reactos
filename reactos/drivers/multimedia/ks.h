/*
	Kernel Streaming

	Part of the ReactOS project
	(See ReactOS licence for usage restrictions/permissions)

	This file created by Andrew Greenwood.

	Started September 12th, 2005.

	You may notice some structs are empty. These are just placeholders and
	should be fleshed-out when functions are implemented that require the
	use of such structs.
*/

#ifndef __INCLUDES_REACTOS_KS_H__
#define __INCLUDES_REACTOS_KS_H__

#include <ddk/ntddk.h>

/* What's this meant to be?! */
#define KSDDKAPI


/* Some unimplemented structs :) */

typedef struct _BUS_INTERFACE_REFERENCE
{
} BUS_INTERFACE_REFERENCE, *PBUS_INTERFACE_REFERENCE;

typedef struct _KSPIN_DESCRIPTOR
{
} KSPIN_DESCRIPTOR, *PKSPIN_DESCRIPTOR;

typedef struct _KSPIN_DESCRIPTOR_EX
{
} KSPIN_DESCRIPTOR_EX, *PKSPIN_DESCRIPTOR_EX;


/* This is just to shut the compiler up so DON'T USE IT! */
typedef void (*PFNKSINTERSECTHANDLER)(void);



typedef struct _KS_COMPRESSION
{
	ULONG RatioNumerator;
	ULONG RatioDenominator;
	ULONG RatioConstantMargin;
} KS_COMPRESSION, *PKS_COMPRESSION;

typedef struct _KS_FRAMING_RANGE
{
	ULONG MinFrameSize;
	ULONG MaxFrameSize;
	ULONG Stepping;
} KS_FRAMING_RANGE, *PKS_FRAMING_RANGE;

typedef struct _KS_FRAMING_RANGE_WEIGHTED
{
	/* Obsolete */
} KS_FRAMING_RANGE_WEIGHTED, *PKS_FRAMING_RANGE_WEIGHTED;

typedef struct _KS_FRAMING_ITEM
{
	GUID MemoryType;
	GUID BusType;
	ULONG MemoryFlags;
	ULONG BusFlags;
	ULONG Flags;
	ULONG Frames;
	ULONG FileAlignment;
	ULONG MemoryTypeWeight;
	KS_FRAMING_RANGE PhysicalRange;
	KS_FRAMING_RANGE_WEIGHTED FramingRange;
} KS_FRAMING_ITEM, *PKS_FRAMING_ITEM;



typedef struct _KSALLOCATOR_FRAMING
{
	union
	{
		ULONG OptionFlags;
		ULONG RequirementsFlags;
	};
	POOL_TYPE PoolType;
	ULONG Frames;
	ULONG FrameSize;
	ULONG FileAlignment;
	ULONG Reserved;
} KSALLOCATOR_FRAMING, *PKSALLOCATOR_FRAMING;

typedef struct _KSALLOCATOR_FRAMING_EX
{
	ULONG CountItems;
	ULONG PinFlags;
	KS_COMPRESSION OutputCompression;
	ULONG PinWeight;
	KS_FRAMING_ITEM FramingItem[1];
} KSALLOCATOR_FRAMING_EX, *PKSALLOCATOR_FRAMING_EX;



typedef struct _KSATTRIBUTE
{
	ULONG Size;
	ULONG Flags;
	GUID Attribute;
} KSATTRIBUTE, *PKSATTRIBUTE;


/*
typedef struct _KSBUFFER_ITEM
{
	KSDPC_ITEM DpcItem;
	LIST_ENTRY BufferList;
} KSBUFFER_ITEM, *PKSBUFFER_ITEM;
*/


typedef struct _KSIDENTIFIER
{
	GUID Set;
	ULONG Id;
	ULONG Flags;
} KSIDENTIFIER;

typedef KSIDENTIFIER KSPIN_MEDIUM, *PKSPIN_MEDIUM;
typedef KSIDENTIFIER KSPIN_INTERFACE, *PKSPIN_INTERFACE;
typedef KSIDENTIFIER KSPROPERTY, *PKSPROPERTY;

typedef struct _KSPRIORITY
{
	ULONG PriorityClass;
	ULONG PrioritySubClass;
} KSPRIORITY, *PKSPRIORITY;

typedef struct _KSPIN_CONNECT
{
	KSPIN_INTERFACE Interface;
	KSPIN_MEDIUM Medium;
	ULONG PinId;
	HANDLE PinToHandle;
	KSPRIORITY Priority;
} KSPIN_CONNECT, *PKSPIN_CONNECT;

typedef struct _KSP_PIN
{
	KSPROPERTY Property;
	ULONG PinId;
	ULONG Reserved;
} KSP_PIN, *PKSP_PIN;


typedef struct _KSDEVICE
{
	/* TODO */
} KSDEVICE, *PKSDEVICE;


/* Device dispatch routines */

typedef NTSTATUS
	(*PFNKSDEVICECREATE)
		(
			IN PKSDEVICE Device
		);

typedef NTSTATUS 
	(*PFNKSDEVICEPNPSTART)
		(
			IN PKSDEVICE Device,
			IN PIRP Irp,
			IN PCM_RESOURCE_LIST TranslatedResourceList OPTIONAL,
			IN PCM_RESOURCE_LIST UntranslatedResourceList OPTIONAL
		);

typedef NTSTATUS
	(*PFNKSDEVICE)
		(
			IN PKSDEVICE Device
		);

typedef NTSTATUS
	(*PFNKSDEVICEIRP)
		(
			IN PKSDEVICE Device,
			IN PIRP Irp
		);

typedef VOID
	(*PFNKSDEVICEIRPVOID)
		(
			IN PKSDEVICE Device,
			IN PIRP Irp
		);

typedef NTSTATUS
	(*PFNKSDEVICEQUERYCAPABILITIES)
		(
			IN PKSDEVICE Device,
			IN PIRP Irp,
			IN OUT PDEVICE_CAPABILITIES Capabilities
		);

typedef NTSTATUS
	(*PFNKSDEVICEQUERYPOWER)
		(
			IN PKSDEVICE Device,
			IN PIRP Irp,
			IN DEVICE_POWER_STATE DeviceTo,
			IN DEVICE_POWER_STATE DeviceFrom,
			IN SYSTEM_POWER_STATE SystemTo,
			IN SYSTEM_POWER_STATE SystemFrom,
			IN POWER_ACTION Action
		);

typedef VOID
	(*PFNKSDEVICESETPOWER)
		(
			IN PKSDEVICE Device,
			IN PIRP Irp,
			IN DEVICE_POWER_STATE To,
			IN DEVICE_POWER_STATE From
		);

typedef struct _KSDEVICE_DISPATCH
{
	PFNKSDEVICECREATE Add;
	PFNKSDEVICEPNPSTART Start;
	PFNKSDEVICE PostStart;
	PFNKSDEVICEIRP QueryStop;
	PFNKSDEVICEIRPVOID CancelStop;
	PFNKSDEVICEIRPVOID Stop;
	PFNKSDEVICEIRP QueryRemove;
	PFNKSDEVICEIRPVOID CancelRemove;
	PFNKSDEVICEIRPVOID Remove;
	PFNKSDEVICEQUERYCAPABILITIES QueryCapabilities;
	PFNKSDEVICEIRPVOID SurpriseRemoval;
	PFNKSDEVICEQUERYPOWER Querypower;
	PFNKSDEVICESETPOWER SetPower;
} KSDEVICE_DISPATCH, *PKSDEVICE_DISPATCH;



/* Some more unimplemented stuff */

typedef struct _KSFILTER
{
} KSFILTER, *PKSFILTER;

typedef struct _KSPROCESSPIN_INDEXENTRY
{
} KSPROCESSPIN_INDEXENTRY, *PKSPROCESSPIN_INDEXENTRY;


/* Filter dispatch routines */

typedef NTSTATUS
	(*PFNKSFILTERIRP)
		(
			IN PKSFILTER Filter,
			IN PIRP Irp
		);

typedef NTSTATUS
	(*PFNKSFILTERPROCESS)
		(
			IN PKSFILTER FIlter,
			IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
		);

typedef NTSTATUS
	(*PFNKSFILTERVOID)
		(
			IN PKSFILTER Filter
		);

typedef struct _KSFILTER_DISPATCH
{
	PFNKSFILTERIRP Create;
	PFNKSFILTERIRP Close;
	PFNKSFILTERPROCESS Process;
	PFNKSFILTERVOID Reset;
} KSFILTER_DISPATCH, *PKSFILTER_DISPATCH;



/* TODO! */

typedef struct _KSAUTOMATION_TABLE
{
} KSAUTOMATION_TABLE, *PKSAUTOMATION_TABLE;

typedef struct _KSNODE_DESCRIPTOR
{
} KSNODE_DESCRIPTOR, *PKSNODE_DESCRIPTOR;

typedef struct _KSTOPOLOGY_CONNECTION
{
} KSTOPOLOGY_CONNECTION, *PKSTOPOLOGY_CONNECTION;

typedef struct _KSCOMPONENTID
{
} KSCOMPONENTID, *PKSCOMPONENTID;


/* Descriptors (filter, device, ...) */

typedef struct _KSFILTER_DESCRIPTOR
{
	const KSFILTER_DISPATCH* Dispatch;
	const KSAUTOMATION_TABLE* AutomationTable;
	ULONG Version;
	ULONG Flags;
	const GUID* ReferenceGuid;
	ULONG PinDescriptorsCount;
	ULONG PinDescriptorSize;
	const KSPIN_DESCRIPTOR_EX* PinDescriptors;
	ULONG CategoriesCount;
	const GUID* Categories;
	ULONG NodeDescriptorsCount;
	ULONG NodeDescriptorSize;
	const KSNODE_DESCRIPTOR* NodeDescriptors;
	ULONG ConnectionsCount;
	const KSTOPOLOGY_CONNECTION* Connections;
	const KSCOMPONENTID* ComponentId;
} KSFILTER_DESCRIPTOR, *PKSFILTER_DESCRIPTOR;

typedef struct _KSDEVICE_DESCRIPTOR
{
	const KSDEVICE_DISPATCH* Dispatch;
	ULONG FilterDescriptorsCount;
	const KSFILTER_DESCRIPTOR* const* FilterDescriptors;
	ULONG Version; /* Doesn't appear to be in the documentation */
} KSDEVICE_DESCRIPTOR, *PKSDEVICE_DESCRIPTOR;




/*
	API functions
*/

NTSTATUS NTAPI
KsInitializeDriver(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath,
	IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL);

#endif
