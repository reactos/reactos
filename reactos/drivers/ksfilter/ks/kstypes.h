#ifndef KSTYPES_H__
#define KSTYPES_H__

typedef struct
{
    KSDISPATCH_TABLE DispatchTable;
    LIST_ENTRY ListEntry;
    ACCESS_MASK AccessMask;
    PKSOBJECT_CREATE_ITEM CreateItem;

}KSIOBJECT_HEADER, *PKSIOBJECT_HEADER;






#endif
