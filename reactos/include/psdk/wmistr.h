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

#define WNODE_FLAG_ALL_DATA             0x00000001
#define WNODE_FLAG_SINGLE_INSTANCE      0x00000002
#define WNODE_FLAG_SINGLE_ITEM          0x00000004
#define WNODE_FLAG_EVENT_ITEM           0x00000008
#define WNODE_FLAG_FIXED_INSTANCE_SIZE  0x00000010
#define WNODE_FLAG_TOO_SMALL            0x00000020
#define WNODE_FLAG_INSTANCES_SAME       0x00000040
#define WNODE_FLAG_STATIC_INSTANCE_NAMES 0x00000080
#define WNODE_FLAG_INTERNAL             0x00000100
#define WNODE_FLAG_USE_TIMESTAMP        0x00000200
#define WNODE_FLAG_PERSIST_EVENT        0x00000400
#define WNODE_FLAG_EVENT_REFERENCE      0x00002000
#define WNODE_FLAG_ANSI_INSTANCENAMES   0x00004000
#define WNODE_FLAG_METHOD_ITEM          0x00008000
#define WNODE_FLAG_PDO_INSTANCE_NAMES   0x00010000
#define WNODE_FLAG_TRACED_GUID          0x00020000
#define WNODE_FLAG_LOG_WNODE            0x00040000
#define WNODE_FLAG_USE_GUID_PTR         0x00080000
#define WNODE_FLAG_USE_MOF_PTR          0x00100000
#define WNODE_FLAG_NO_HEADER            0x00200000
#define WNODE_FLAG_SEVERITY_MASK        0xff000000

typedef struct tagWNODE_TOO_SMALL
{
    struct _WNODE_HEADER WnodeHeader;
    ULONG SizeNeeded;
} WNODE_TOO_SMALL, *PWNODE_TOO_SMALL;

typedef struct tagWNODE_SINGLE_INSTANCE
{
    struct _WNODE_HEADER WnodeHeader;
    ULONG OffsetInstanceName;
    ULONG InstanceIndex;
    ULONG DataBlockOffset;
    ULONG SizeDataBlock;
    UCHAR VariableData[];
} WNODE_SINGLE_INSTANCE, *PWNODE_SINGLE_INSTANCE;

#define WMIREG_FLAG_EXPENSIVE           0x00000001
#define WMIREG_FLAG_INSTANCE_LIST       0x00000004
#define WMIREG_FLAG_INSTANCE_BASENAME   0x00000008
#define WMIREG_FLAG_INSTANCE_PDO        0x00000020
#define WMIREG_FLAG_REMOVE_GUID         0x00010000
#define WMIREG_FLAG_RESERVED1           0x00020000
#define WMIREG_FLAG_RESERVED2           0x00040000
#define WMIREG_FLAG_TRACED_GUID         0x00080000
#define WMIREG_FLAG_TRACE_CONTROL_GUID  0x00001000
#define WMIREG_FLAG_EVENT_ONLY_GUID     0x00000040

typedef struct
{
    GUID Guid;
    ULONG Flags;
    ULONG InstanceCount;
    union
    {
        ULONG InstanceNameList;
        ULONG BaseNameOffset;
        ULONG_PTR Pdo;
        ULONG_PTR InstanceInfo;
    };
} WMIREGGUIDW, *PWMIREGGUIDW;

typedef struct
{
    ULONG BufferSize;
    ULONG NextWmiRegInfo;
    ULONG RegistryPath;
    ULONG MofResourceName;
    ULONG GuidCount;
    WMIREGGUIDW WmiRegGuid[];
} WMIREGINFOW, *PWMIREGINFOW;

typedef WMIREGINFOW WMIREGINFO;
typedef PWMIREGINFOW PWMIREGINFO;

typedef struct
{
    ULONG OffsetInstanceData;
    ULONG LengthInstanceData;
} OFFSETINSTANCEDATAANDLENGTH, *POFFSETINSTANCEDATAANDLENGTH;

typedef struct tagWNODE_ALL_DATA
{
    struct _WNODE_HEADER WnodeHeader;
    ULONG DataBlockOffset;
    ULONG InstanceCount;
    ULONG OffsetInstanceNameOffsets;
    union
    {
        ULONG FixedInstanceSize;
        OFFSETINSTANCEDATAANDLENGTH OffsetInstanceDataAndLength[0];
    };
} WNODE_ALL_DATA, *PWNODE_ALL_DATA;

typedef struct tagWNODE_SINGLE_ITEM
{
    struct _WNODE_HEADER WnodeHeader;
    ULONG OffsetInstanceName;
    ULONG InstanceIndex;
    ULONG ItemId;
    ULONG DataBlockOffset;
    ULONG SizeDataItem;
    UCHAR VariableData[];
} WNODE_SINGLE_ITEM, *PWNODE_SINGLE_ITEM;

typedef struct tagWNODE_METHOD_ITEM
{
    struct _WNODE_HEADER WnodeHeader;
    ULONG OffsetInstanceName;
    ULONG InstanceIndex;
    ULONG MethodId;
    ULONG DataBlockOffset;
    ULONG SizeDataBlock;
    UCHAR VariableData[];
} WNODE_METHOD_ITEM, *PWNODE_METHOD_ITEM;

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

#define WMIGUID_QUERY                 0x0001
#define WMIGUID_SET                   0x0002
#define WMIGUID_NOTIFICATION          0x0004
#define WMIGUID_READ_DESCRIPTION      0x0008
#define WMIGUID_EXECUTE               0x0010
#define TRACELOG_CREATE_REALTIME      0x0020
#define TRACELOG_CREATE_ONDISK        0x0040
#define TRACELOG_GUID_ENABLE          0x0080
#define TRACELOG_ACCESS_KERNEL_LOGGER 0x0100
#define TRACELOG_LOG_EVENT            0x0200
#define TRACELOG_CREATE_INPROC        0x0200
#define TRACELOG_ACCESS_REALTIME      0x0400
#define TRACELOG_REGISTER_GUIDS       0x0800

#endif /* _WMISTR_ */
