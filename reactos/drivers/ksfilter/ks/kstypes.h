#ifndef KSTYPES_H__
#define KSTYPES_H__

typedef struct
{
    KSDISPATCH_TABLE DispatchTable;
    KSOBJECTTYPE Type;
    ULONG ItemCount;
    PKSOBJECT_CREATE_ITEM CreateItem;

    UNICODE_STRING ObjectClass;
    PUNKNOWN Unknown;

    PDEVICE_OBJECT TargetDevice;
    LIST_ENTRY TargetDeviceListEntry;

    PDEVICE_OBJECT ParentDeviceObject;

    PFNKSCONTEXT_DISPATCH PowerDispatch;
    PVOID PowerContext;
    LIST_ENTRY PowerDispatchEntry;

}KSIOBJECT_HEADER, *PKSIOBJECT_HEADER;

typedef struct
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    PFNKSITEMFREECALLBACK ItemFreeCallback;
    LONG ReferenceCount;
}DEVICE_ITEM, *PDEVICE_ITEM;



typedef struct
{
    IKsDeviceVtbl *lpVtblIKsDevice;
    LONG ref;
    ERESOURCE SecurityLock;

    USHORT MaxItems;
    DEVICE_ITEM *ItemList;

    ULONG DeviceIndex;
    KSPIN_LOCK ItemListLock;

    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT NextDeviceObject;

    KSTARGET_STATE TargetState;
    LIST_ENTRY TargetDeviceList;

    KSDEVICE KsDevice;
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
    PIRP Irp;
}QUEUE_ENTRY, *PQUEUE_ENTRY;

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
