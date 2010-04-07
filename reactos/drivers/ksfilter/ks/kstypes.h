#pragma once

#include <ntddk.h>
#include <ks.h>

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
    IKsDeviceVtbl *lpVtblIKsDevice;

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
    PIRP Irp;
    KEVENT Event;
}KSREMOVE_BUS_INTERFACE_CTX, *PKSREMOVE_BUS_INTERFACE_CTX;

typedef struct
{
    PLIST_ENTRY List;
    PFILE_OBJECT FileObject;
    PKSEVENT_ENTRY EventEntry;
    PIRP Irp;
}KSEVENT_CTX, *PKSEVENT_CTX;

typedef BOOLEAN (NTAPI *PKSEVENT_SYNCHRONIZED_ROUTINE)(PKSEVENT_CTX Context);

typedef struct
{
    BOOLEAN Enabled;

    PDEVICE_OBJECT PnpDeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT BusDeviceObject;

    UNICODE_STRING ServicePath;
    UNICODE_STRING SymbolicLinkName;

    WCHAR BusIdentifier[1];
}BUS_ENUM_DEVICE_EXTENSION, *PBUS_ENUM_DEVICE_EXTENSION;

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

