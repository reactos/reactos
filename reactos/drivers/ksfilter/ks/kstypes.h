#ifndef KSTYPES_H__
#define KSTYPES_H__

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
}KSBASIC_HEADER, *PKSBASIC_HEADER;

typedef struct
{
    KSOBJECTTYPE Type;
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
    KSDEVICE_DESCRIPTOR* Descriptor;

    LIST_ENTRY PowerDispatchList;

}KSIDEVICE_HEADER, *PKSIDEVICE_HEADER;


typedef struct
{
    PKSIDEVICE_HEADER DeviceHeader;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct
{
    LIST_ENTRY Entry;
    UNICODE_STRING SymbolicLink;
}SYMBOLIC_LINK_ENTRY, *PSYMBOLIC_LINK_ENTRY;

typedef struct
{
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_WORKITEM WorkItem;
}PNP_POSTSTART_CONTEXT, *PPNP_POSTSTART_CONTEXT;


#endif
