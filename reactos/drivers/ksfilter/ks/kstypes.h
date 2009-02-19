#ifndef KSTYPES_H__
#define KSTYPES_H__

struct KSIDEVICE_HEADER;

typedef struct
{
    KSDISPATCH_TABLE DispatchTable;
    LPWSTR ObjectClass;
    ULONG ItemCount;
    PKSOBJECT_CREATE_ITEM CreateItem;

}KSIOBJECT_HEADER, *PKSIOBJECT_HEADER;

typedef struct
{
    BOOL bCreated;
    PKSIOBJECT_HEADER ObjectHeader;
    KSOBJECT_CREATE_ITEM CreateItem;
}DEVICE_ITEM, *PDEVICE_ITEM;


typedef struct
{
    USHORT MaxItems;
    DEVICE_ITEM *ItemList;

    ULONG DeviceIndex;
    KSPIN_LOCK ItemListLock;

    PDEVICE_OBJECT NextDeviceObject;
    ERESOURCE SecurityLock;
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


#endif
