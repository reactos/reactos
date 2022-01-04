#pragma once

#define TAG_KSDEVICE 'DESK'
#define TAG_KSOBJECT_TAG 'HOSK'

VOID
CompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost);



NTSTATUS
NTAPI
KspCreateObjectType(
    IN HANDLE ParentHandle,
    IN LPWSTR ObjectType,
    PVOID CreateParameters,
    UINT CreateParametersSize,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle);

NTSTATUS
NTAPI
KspCreateFilterFactory(
    IN PDEVICE_OBJECT  DeviceObject,
    IN const KSFILTER_DESCRIPTOR  *Descriptor,
    IN PWSTR  RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
    IN ULONG  CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY *FilterFactory OPTIONAL);

NTSTATUS
NTAPI
IKsDevice_Create(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
IKsDevice_Pnp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
IKsDevice_Power(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
KspCreateFilter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN IKsFilterFactory *iface);

NTSTATUS
KspSetDeviceInterfacesState(
    IN PLIST_ENTRY ListHead,
    IN BOOL Enable);

NTSTATUS
KspFreeDeviceInterfaces(
    IN PLIST_ENTRY ListHead);

NTSTATUS
KspRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG CategoriesCount,
    IN GUID const*Categories,
    IN PUNICODE_STRING ReferenceString,
    OUT PLIST_ENTRY SymbolicLinkList);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

VOID
FreeItem(
    IN PVOID Item);

KSDDKAPI
NTSTATUS
NTAPI
KspPinPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptors,
    IN  ULONG DescriptorSize);


NTSTATUS
FindMatchingCreateItem(
    PLIST_ENTRY ListHead,
    PUNICODE_STRING String,
    OUT PCREATE_ITEM_ENTRY *OutCreateItem);

NTSTATUS
KspCopyCreateRequest(
    IN PIRP Irp,
    IN LPWSTR ObjectClass,
    IN OUT PULONG Size,
    OUT PVOID * Result);

NTSTATUS
KspCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKSDEVICE KsDevice,
    IN IKsFilterFactory * FilterFactory,
    IN IKsFilter* Filter,
    IN PKSPIN_CONNECT Connect,
    IN KSPIN_DESCRIPTOR_EX* Descriptor);

NTSTATUS
KspAddCreateItemToList(
    OUT PLIST_ENTRY ListHead,
    IN ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList);

VOID
KspFreeCreateItems(
    IN PLIST_ENTRY ListHead);

NTSTATUS
KspPropertyHandler(
    IN PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG PropertyItemSize OPTIONAL);

NTSTATUS
NTAPI
IKsFilterFactory_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
KspSetFilterFactoriesState(
    IN PKSIDEVICE_HEADER DeviceHeader,
    IN BOOLEAN NewState);

NTSTATUS
NTAPI
KspMethodHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG MethodSetsCount,
    IN  const KSMETHOD_SET *MethodSet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG MethodItemSize OPTIONAL);

VOID
IKsFilter_AddPin(
    PKSFILTER Filter,
    PKSPIN Pin);

VOID
IKsFilter_RemovePin(
    PKSFILTER Filter,
    PKSPIN Pin);

NTSTATUS
KspEnableEvent(
    IN  PIRP Irp,
    IN  ULONG EventSetsCount,
    IN  const KSEVENT_SET* EventSet,
    IN  OUT PLIST_ENTRY EventsList OPTIONAL,
    IN  KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN  PVOID EventsLock OPTIONAL,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG EventItemSize OPTIONAL);

NTSTATUS
KspValidateConnectRequest(
    IN PIRP Irp,
    IN ULONG DescriptorsCount,
    IN PVOID Descriptors,
    IN ULONG DescriptorSize,
    OUT PKSPIN_CONNECT* Connect);

NTSTATUS
KspReadMediaCategory(
    IN LPGUID Category,
    PKEY_VALUE_PARTIAL_INFORMATION *OutInformation);

