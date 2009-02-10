#ifndef KSTYPES_H__
#define KSTYPES_H__

typedef struct
{
    KSDISPATCH_TABLE DispatchTable;
    LIST_ENTRY ListEntry;
    ACCESS_MASK AccessMask;
    PKSOBJECT_CREATE_ITEM CreateItem;
    BOOL Initialized;

}KSIOBJECT_HEADER, *PKSIOBJECT_HEADER;

typedef struct
{
    ULONG MaxItems;
    ULONG FreeIndex;
    PKSOBJECT_CREATE_ITEM ItemsList;
    PKSIOBJECT_HEADER ObjectList;
    UCHAR ItemsListProvided;
    PDEVICE_OBJECT LowerDeviceObject;

    ERESOURCE SecurityLock;
}KSIDEVICE_HEADER, *PKSIDEVICE_HEADER;


typedef struct
{
    PKSIDEVICE_HEADER DeviceHeader;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;



#endif
