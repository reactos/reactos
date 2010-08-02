#ifndef _WMISTR_
#define _WMISTR_

typedef struct _WNODE_HEADER
{
    ULONG BufferSize;
    ULONG ProviderId;
    union
    {
        ULONG64 HistoricalContext;
        struct
        {
            ULONG Version;
            ULONG Linkage;
        };
    };
    union
    {
        ULONG CountLost;
        HANDLE KernelHandle;
        LARGE_INTEGER TimeStamp;
    };
    GUID Guid;
    ULONG ClientContext;
    ULONG Flags;
} WNODE_HEADER, *PWNODE_HEADER;

typedef enum
{
    WMI_GET_ALL_DATA = 0,
    WMI_GET_SINGLE_INSTANCE,
    WMI_SET_SINGLE_INSTANCE,
    WMI_SET_SINGLE_ITEM,
    WMI_ENABLE_EVENTS,
    WMI_DISABLE_EVENTS,
    WMI_ENABLE_COLLECTION,
    WMI_DISABLE_COLLECTION,
    WMI_REGINFO,
    WMI_EXECUTE_METHOD
} WMIDPREQUESTCODE;

#endif /* _WMISTR_ */
