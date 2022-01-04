#pragma once

typedef struct
{
    KoCreateObjectHandler CreateObjectHandler;
}KO_DRIVER_EXTENSION, *PKO_DRIVER_EXTENSION;

typedef struct
{
    const KSDEVICE_DESCRIPTOR  *Descriptor;
}KS_DRIVER_EXTENSION, *PKS_DRIVER_EXTENSION;

typedef struct
{
    KSOBJECT_HEADER ObjectHeader;
    KSOBJECT_CREATE_ITEM CreateItem;
}KO_OBJECT_HEADER, *PKO_OBJECT_HEADER;


typedef struct
{
    KSDISPATCH_TABLE DispatchTable;
    KSOBJECTTYPE Type;

    LONG ItemListCount;
    LIST_ENTRY ItemList;

    UNICODE_STRING ObjectClass;
    PUNKNOWN Unknown;
    PVOID ObjectType;

    PDEVICE_OBJECT TargetDevice;
    LIST_ENTRY TargetDeviceListEntry;

    PDEVICE_OBJECT ParentDeviceObject;

    PFNKSCONTEXT_DISPATCH PowerDispatch;
    PVOID PowerContext;
    LIST_ENTRY PowerDispatchEntry;
    PKSOBJECT_CREATE_ITEM OriginalCreateItem;
    ACCESS_MASK AccessMask;

}KSIOBJECT_HEADER, *PKSIOBJECT_HEADER;

typedef struct
{
    LIST_ENTRY Entry;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PFNKSITEMFREECALLBACK ItemFreeCallback;
    LONG ReferenceCount;
    LIST_ENTRY ObjectItemList;
}CREATE_ITEM_ENTRY, *PCREATE_ITEM_ENTRY;

typedef struct
{
    KSOBJECTTYPE Type;
    PKSDEVICE KsDevice;
    PRKMUTEX ControlMutex;
    LIST_ENTRY EventList;
    KSPIN_LOCK EventListLock;
    PUNKNOWN ClientAggregate;
    PUNKNOWN OuterUnknown;
    union
    {
        PKSDEVICE KsDevice;
        PKSFILTERFACTORY KsFilterFactory;
        PKSFILTER KsFilter;
    }Parent;

    union
    {
        PKSFILTERFACTORY FilterFactory;
        PKSFILTER Filter;
        PKSPIN Pin;
    }Next;

    union
    {
        PKSFILTERFACTORY FilterFactory;
        PKSFILTER Filter;
    }FirstChild;

}KSBASIC_HEADER, *PKSBASIC_HEADER;

typedef struct
{
    KSBASIC_HEADER BasicHeader;
    KSDEVICE KsDevice;

    LONG ref;
    ERESOURCE SecurityLock;

    LONG ItemListCount;
    LIST_ENTRY ItemList;

    ULONG DeviceIndex;
    PDEVICE_OBJECT PnpDeviceObject;
    PDEVICE_OBJECT BaseDevice;

    KSTARGET_STATE TargetState;
    LIST_ENTRY TargetDeviceList;

    KMUTEX DeviceMutex;
    KMUTEX BagMutex;

    LIST_ENTRY PowerDispatchList;
    LIST_ENTRY ObjectBags;

    PADAPTER_OBJECT AdapterObject;
    ULONG MaxMappingsByteCount;
    ULONG MappingTableStride;

}KSIDEVICE_HEADER, *PKSIDEVICE_HEADER;

typedef struct
{
    PKSIDEVICE_HEADER DeviceHeader;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct
{
    LIST_ENTRY Entry;
    UNICODE_STRING SymbolicLink;
    CLSID DeviceInterfaceClass;
}SYMBOLIC_LINK_ENTRY, *PSYMBOLIC_LINK_ENTRY;

typedef struct
{
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_WORKITEM WorkItem;
}PNP_POSTSTART_CONTEXT, *PPNP_POSTSTART_CONTEXT;

typedef struct
{
    PLIST_ENTRY List;
    PFILE_OBJECT FileObject;
    PKSEVENT_ENTRY EventEntry;
    PIRP Irp;
}KSEVENT_CTX, *PKSEVENT_CTX;

typedef BOOLEAN (NTAPI *PKSEVENT_SYNCHRONIZED_ROUTINE)(PKSEVENT_CTX Context);

struct __BUS_ENUM_DEVICE_EXTENSION__;
struct __BUS_DEVICE_ENTRY__;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG IsBus;
    union
    {
        PDEVICE_OBJECT DeviceObject;
        ULONG DeviceReferenceCount;
    };
    union
    {
        struct __BUS_DEVICE_ENTRY__* DeviceEntry;
        ULONG Dummy1;
    };
    struct __BUS_ENUM_DEVICE_EXTENSION__ *BusDeviceExtension;
    ULONG DeviceObjectReferenceCount;
}COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct
{
    PCOMMON_DEVICE_EXTENSION Ext;
}DEV_EXTENSION, *PDEV_EXTENSION;

typedef struct
{
    LIST_ENTRY Entry;
    GUID InterfaceGuid;
    UNICODE_STRING SymbolicLink;
}BUS_INSTANCE_ENTRY, *PBUS_INSTANCE_ENTRY;



typedef enum
{
    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted
}DEVICE_STATE;


typedef struct __BUS_DEVICE_ENTRY__
{
    LIST_ENTRY Entry;
    LIST_ENTRY DeviceInterfaceList;
    LIST_ENTRY IrpPendingList;
    PDEVICE_OBJECT PDO;
    DEVICE_STATE DeviceState;
    GUID DeviceGuid;
    LPWSTR PDODeviceName;
    LPWSTR DeviceName;
    LPWSTR BusId;
    LARGE_INTEGER TimeCreated;
    LARGE_INTEGER TimeExpired;
    LPWSTR Instance;
}BUS_DEVICE_ENTRY, *PBUS_DEVICE_ENTRY;

typedef struct __BUS_ENUM_DEVICE_EXTENSION__
{
    COMMON_DEVICE_EXTENSION Common;
    KSPIN_LOCK Lock;
    KEVENT Event;
    UNICODE_STRING DeviceInterfaceLink;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT PnpDeviceObject;
    PDEVICE_OBJECT BusDeviceObject;
    ULONG PdoCreated;
    KTIMER Timer;
    KDPC Dpc;
    WORK_QUEUE_ITEM WorkItem;
    ULONG DeviceAttached;
    UNICODE_STRING ServicePath;

    WCHAR BusIdentifier[1];
}BUS_ENUM_DEVICE_EXTENSION, *PBUS_ENUM_DEVICE_EXTENSION;

typedef struct
{
    PIRP Irp;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    KEVENT Event;
    NTSTATUS Status;
    WORK_QUEUE_ITEM WorkItem;
}BUS_INSTALL_ENUM_CONTEXT, *PBUS_INSTALL_ENUM_CONTEXT;

typedef struct
{
    PUCHAR FilterData;
    ULONG FilterLength;
    ULONG FilterOffset;

    PUCHAR DataCache;
    ULONG DataLength;
    ULONG DataOffset;

}KSPCACHE_DESCRIPTOR, *PKSPCACHE_DESCRIPTOR;

typedef struct
{
    DWORD dwVersion;
    DWORD dwMerit;
    DWORD dwPins;
    DWORD dwUnused;
}KSPCACHE_FILTER_HEADER, *PKSPCACHE_FILTER_HEADER;

typedef struct
{
    ULONG Signature;
    ULONG Flags;
    ULONG Instances;
    ULONG MediaTypes;
    ULONG Mediums;
    DWORD Category;
}KSPCACHE_PIN_HEADER, *PKSPCACHE_PIN_HEADER;


typedef struct
{
    ULONG Signature;
    ULONG dwUnused;
    ULONG OffsetMajor;
    ULONG OffsetMinor;
}KSPCACHE_DATARANGE, *PKSPCACHE_DATARANGE;


typedef struct
{
    CLSID Medium;
    ULONG dw1;
    ULONG dw2;
}KSPCACHE_MEDIUM;

