/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        include/net/miniport.h
 * PURPOSE:     Type definitions available only to NDIS miniport drivers
 */
#ifndef __MINIPORT_H
#define __MINIPORT_H

#ifdef _MSC_VER
#include <ntstatus.h>
#else /* _MSC_VER */
#include <ddk/status.h>
#endif /* _MSC_VER */

/* Base types */

#define IN
#define OUT
#define OPTIONAL

#define ANYSIZE_ARRAY   1

#define CONST const

#ifdef _MSC_VER

#ifdef i386
#define STDCALL _stdcall
#define CDECL   _cdecl
#else /* i386 */
#define STDCALL
#define CDECL
#endif /* i386 */

#else /* _MSC_VER */

#ifdef i386
#define STDCALL __attribute__ ((stdcall))
#define CDECL   __attribute__ ((cdecl))
#else /* i386 */
#define STDCALL
#define CDECL
#endif /* i386 */

#endif /* _MSC_VER */

typedef void VOID, *PVOID;
typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef short SHORT, *PSHORT;
typedef unsigned short USHORT, *PUSHORT;
typedef long LONG,*PLONG;
typedef unsigned long ULONG,*PULONG;
typedef double DOUBLE, *PDOUBLE;
typedef struct _QUAD {
    double  DoNotUseThisField;
} QUAD,*PQUAD;
typedef QUAD UQUAD, *PUQUAD;
typedef signed int INT, *PINT;
typedef unsigned int UINT, *PUINT;
typedef double LONGLONG, *PLONGLONG;
typedef double ULONGLONG, *PULONGLONG;

typedef UCHAR BOOLEAN, *PBOOLEAN;
typedef ULONG DWORD, *PDWORD;

/* Cardinal types */
typedef CHAR CCHAR, *PCCHAR;
typedef SHORT CSHORT, *PCSHORT;
typedef ULONG CLONG, *PCLONG;

/* UNICODE (wide character) types */
#ifndef __cplusplus
typedef short unsigned int wchar_t;
#endif	/* __cplusplus */
typedef wchar_t WCHAR;
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef CONST WCHAR *LPCWCH, *PCWCH;
typedef WCHAR *NWPSTR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef CONST WCHAR *LPCWSTR, *PCWSTR;

/* ANSI (multi-byte character) types */
typedef CHAR *LPCH, *PCH;
typedef CONST CHAR *LPCCH, *PCCH;
typedef CHAR *NPSTR;
typedef CHAR *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;

/* Neutral ANSI/UNICODE types */
#ifdef UNICODE

typedef WCHAR TCHAR, *PTCHAR;
typedef WCHAR TUCHAR, *PTUCHAR;

typedef LPWSTR LPTCH, PTCH;
typedef LPWSTR PTSTR, LPTSTR;
typedef LPCWSTR LPCTSTR;
typedef LPWSTR LP;
#define _TEXT(string) L##string
#define _T(string) L##string

#else /* UNICODE */

typedef CHAR TCHAR, *PTCHAR;
typedef UCHAR TUCHAR, *PTUCHAR;

typedef LPSTR LPTCH, PTCH;
typedef LPSTR PTSTR, LPTSTR;
typedef LPCSTR LPCTSTR;
#define _TEXT(string) string
#define _T(string) string

#endif /* UNICODE */

#define TEXT(string) __TEXT(string)


typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    struct {
        ULONG LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef LARGE_INTEGER *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        ULONG LowPart;
        ULONG HighPart;
    };
    struct {
        ULONG LowPart;
        ULONG HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef ULARGE_INTEGER *PULARGE_INTEGER;


typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;



/* NT status type and macros for checking status */
typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#ifdef _MSC_VER
#define NT_SUCCESS(Status)      ((ULONG)(Status) >= 0)
#endif /* _MSC_VER */
#define NT_INFORMATION(Status)  ((ULONG)(Status) >> 30 == 1)
#define NT_WARNING(Status)      ((ULONG)(Status) >> 30 == 2)
#define NT_ERROR(Status)        ((ULONG)(Status) >> 30 == 3)

#define APPLICATION_ERROR_MASK       0x20000000
#define ERROR_SEVERITY_SUCCESS       0x00000000
#define ERROR_SEVERITY_INFORMATIONAL 0x40000000
#define ERROR_SEVERITY_WARNING       0x80000000
#define ERROR_SEVERITY_ERROR         0xC0000000



/* Basic constants */

#define FALSE   0
#define TRUE    1

#define NULL    ((PVOID)0)



/* Counted strings */

typedef struct _STRING
{
    USHORT  Length;
    USHORT  MaximumLength;
    PCHAR   Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING, *PANSI_STRING;
typedef STRING OEM_STRING, *POEM_STRING;

#define ANSI_NULL   ((CHAR)0)

typedef struct _CSTRING
{
    USHORT      Length;
    USHORT      MaximumLength;
    CONST CHAR  *Buffer;
} CSTRING, *PCSTRING;

typedef CSTRING CANSI_STRING, *PCANSI_STRING;


typedef struct _UNICODE_STRING
{
    USHORT  Length;
    USHORT  MaximumLength;
    PWSTR   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _CUNICODE_STRING
{
    USHORT      Length;
    USHORT      MaximumLength;
    CONST PWSTR Buffer;
} CUNICODE_STRING, *PCUNICODE_STRING;

#define UNICODE_NULL    ((WCHAR)0)



/* Linked lists */

typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _SINGLE_LIST_ENTRY
{
    struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;


/*
 * VOID InitializeListHead(
 *     PLIST_ENTRY ListHead);
 */
#define InitializeListHead(ListHead) \
{                                    \
    (ListHead)->Flink = (ListHead);  \
    (ListHead)->Blink = (ListHead);  \
}


/*
 * VOID InsertHeadList(
 *     LIST_ENTRY ListHead,
 *     PLIST_ENTRY Entry);
 */
#define InsertHeadList(ListHead, ListEntry) \
{                                           \
    PLIST_ENTRY OldFlink;                   \
    OldFlink           = (ListHead)->Flink; \
    (ListEntry)->Flink = OldFlink;          \
    (ListEntry)->Blink = (ListHead);        \
    OldFlink->Blink    = (ListEntry);       \
    (ListHead)->Flink  = (ListEntry);       \
}


/*
 * VOID InsertTailList(
 *     PLIST_ENTRY ListHead,
 *	   PLIST_ENTRY Entry);
 */
#define InsertTailList(ListHead, ListEntry) \
{                                           \
    PLIST_ENTRY OldBlink;                   \
	OldBlink           = (ListHead)->Blink; \
	(ListEntry)->Flink = (ListHead);        \
	(ListEntry)->Blink = OldBlink;          \
	OldBlink->Flink    = (ListEntry);       \
	(ListHead)->Blink  = (ListEntry);       \
}

/*
 * BOOLEAN IsListEmpty(
 *     PLIST_ENTRY ListHead);
 */
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))


/*
 * PSINGLE_LIST_ENTRY PopEntryList(
 *     PSINGLE_LIST_ENTRY ListHead);
 */
#define PopEntryList(ListHead)               \
    (ListHead)->Next;                        \
{                                            \
    PSINGLE_LIST_ENTRY FirstEntry;           \
                                             \
    FirstEntry = (ListHead)->Next;           \
    if (FirstEntry != NULL)                  \
    {                                        \
        (ListHead)->Next = FirstEntry->Next; \
    }                                        \
}


/*
 * VOID PushEntryList(
 *     PSINGLE_LIST_ENTRY ListHead,
 *     PSINGLE_LIST_ENTRY Entry);
 */
#define PushEntryList(ListHead, Entry)   \
    (Entry)->Next    = (ListHead)->Next; \
    (ListHead)->Next = (Entry)


/*
 * VOID RemoveEntryList(
 *     PLIST_ENTRY Entry);
 */
#define RemoveEntryList(ListEntry)        \
{                                         \
    PLIST_ENTRY OldFlink;                 \
    PLIST_ENTRY OldBlink;                 \
    OldFlink        = (ListEntry)->Flink; \
    OldBlink        = (ListEntry)->Blink; \
    OldFlink->Blink = OldBlink;           \
    OldBlink->Flink = OldFlink;           \
}


/*
 * PLIST_ENTRY RemoveHeadList(
 *     PLIST_ENTRY ListHead);
 */
#define RemoveHeadList(ListHead)       \
	(ListHead)->Flink;                 \
{                                      \
    RemoveEntryList((ListHead)->Flink) \
}


/*
 * PLIST_ENTRY RemoveTailList(
 *     PLIST_ENTRY ListHead);
 */
#define RemoveTailList(ListHead)       \
    (ListHead)->Blink;                 \
{                                      \
    RemoveEntryList((ListHead)->Blink) \
}


/* Hardware */

typedef UCHAR KIRQL, *PKIRQL;

typedef struct _KSPIN_LOCK
{
    ULONG    Lock;
} KSPIN_LOCK, *PKSPIN_LOCK;


typedef struct _ADAPTER_OBJECT  *PADAPTER_OBJECT;
typedef struct _DEVICE_OBJECT   *PDEVICE_OBJECT;
typedef struct _DRIVER_OBJECT   *PDRIVER_OBJECT;
typedef struct _FILE_OBJECT     *PFILE_OBJECT;

typedef struct _KINTERRUPT      *PKINTERRUPT;


/* Memory Descriptor List */
typedef struct _MDL {
    struct _MDL *Next;
    CSHORT Size;
    CSHORT MdlFlags;
    struct _EPROCESS *Process;
    PVOID MappedSystemVa;
    PVOID StartVa;
    ULONG ByteCount;
    ULONG ByteOffset;
} MDL, *PMDL;

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_LOCK_HELD               0x0200
#define MDL_PHYSICAL_VIEW           0x0400
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000


#define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
                           MDL_PAGES_LOCKED            | \
                           MDL_SOURCE_IS_NONPAGED_POOL | \
                           MDL_PARTIAL_HAS_BEEN_MAPPED | \
                           MDL_PARENT_MAPPED_SYSTEM_VA | \
                           MDL_LOCK_HELD               | \
                           MDL_SYSTEM_VA               | \
                           MDL_IO_SPACE )


typedef struct _DISPATCHER_HEADER
{
    UCHAR       Type;
    UCHAR       Absolute;
    UCHAR       Size;
    UCHAR       Inserted;
    LONG        SignalState;
    LIST_ENTRY  WaitListHead;
} DISPATCHER_HEADER;

typedef struct _KEVENT
{
    DISPATCHER_HEADER   Header;
} KEVENT, *PKEVENT;


typedef struct _KTIMER
{
    DISPATCHER_HEADER   Header;
    ULARGE_INTEGER      DueTime;
    LIST_ENTRY          TimerListEntry;
    struct _KDPC        *Dpc;
    LONG                Period;
} KTIMER, *PKTIMER;


typedef enum _KDPC_IMPORTANCE
{
    LowImportance,
    MediumImportance,
    HighImportance
} KDPC_IMPORTANCE;

/* Forward declaration */
struct _KDPC;

typedef VOID (*PKDEFERRED_ROUTINE)(
    IN  struct _KDPC    *Dpc,
    IN  PVOID           DeferredContext,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2);

/* Deferred Procedure Call */

typedef struct _KDPC {
    CSHORT              Type;
    UCHAR               Number;
    UCHAR               Importance;
    LIST_ENTRY          DpcListEntry;
    PKDEFERRED_ROUTINE  DeferredRoutine;
    PVOID               DeferredContext;
    PVOID               SystemArgument1;
    PVOID               SystemArgument2;
    PULONG              Lock;
} KDPC, *PKDPC;


typedef enum _INTERFACE_TYPE
{
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;


typedef enum _DMA_WIDTH
{
    Width8Bits,
    Width16Bits,
    Width32Bits,
    MaximumDmaWidth
} DMA_WIDTH, *PDMA_WIDTH;

typedef enum _DMA_SPEED
{
    Compatible,
    TypeA,
    TypeB,
    TypeC,
    TypeF,
    MaximumDmaSpeed
} DMA_SPEED, *PDMA_SPEED;


typedef enum _KINTERRUPT_MODE
{
    LevelSensitive,
    Latched
} KINTERRUPT_MODE;


typedef struct _DMA_CONFIGURATION_BYTE0
{
    UCHAR   Channel:3;
    UCHAR   Reserved:3;
    UCHAR   Shared:1;
    UCHAR   MoreEntries:1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1
{
    UCHAR   Reserved0:2;
    UCHAR   TransferSize:2;
    UCHAR   Timing:2;
    UCHAR   Reserved1:2;
} DMA_CONFIGURATION_BYTE1;


typedef struct _CM_MCA_POS_DATA
{
    USHORT  AdapterId;
    UCHAR   PosData1;
    UCHAR   PosData2;
    UCHAR   PosData3;
    UCHAR   PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;

typedef struct _EISA_MEMORY_TYPE
{
    UCHAR   ReadWrite:1;
    UCHAR   Cached:1;
    UCHAR   Reserved0:1;
    UCHAR   Type:2;
    UCHAR   Shared:1;
    UCHAR   Reserved1:1;
    UCHAR   MoreEntries:1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

typedef struct _EISA_MEMORY_CONFIGURATION
{
    EISA_MEMORY_TYPE    ConfigurationByte;
    UCHAR   DataSize;
    USHORT  AddressLowWord;
    UCHAR   AddressHighByte;
    USHORT  MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;


typedef struct _EISA_IRQ_DESCRIPTOR
{
    UCHAR   Interrupt:4;
    UCHAR   Reserved:1;
    UCHAR   LevelTriggered:1;
    UCHAR   Shared:1;
    UCHAR   MoreEntries:1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION
{
    EISA_IRQ_DESCRIPTOR ConfigurationByte;
    UCHAR   Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;

typedef struct _EISA_DMA_CONFIGURATION
{
    DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
    DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;


typedef struct _EISA_PORT_DESCRIPTOR
{
    UCHAR   NumberPorts:5;
    UCHAR   Reserved:1;
    UCHAR   Shared:1;
    UCHAR   MoreEntries:1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION
{
    EISA_PORT_DESCRIPTOR    Configuration;
    USHORT  PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;

typedef struct _CM_EISA_SLOT_INFORMATION
{
    UCHAR   ReturnCode;
    UCHAR   ReturnFlags;
    UCHAR   MajorRevision;
    UCHAR   MinorRevision;
    USHORT  Checksum;
    UCHAR   NumberFunctions;
    UCHAR   FunctionInformation;
    ULONG   CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;

typedef struct _CM_EISA_FUNCTION_INFORMATION
{
    ULONG   CompressedId;
    UCHAR   IdSlotFlags1;
    UCHAR   IdSlotFlags2;
    UCHAR   MinorRevision;
    UCHAR   MajorRevision;
    UCHAR   Selections[26];
    UCHAR   FunctionFlags;
    UCHAR   TypeString[80];
    EISA_MEMORY_CONFIGURATION   EisaMemory[9];
    EISA_IRQ_CONFIGURATION      EisaIrq[7];
    EISA_DMA_CONFIGURATION      EisaDma[4];
    EISA_PORT_CONFIGURATION     EisaPort[20];
    UCHAR   InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;


typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR
{
    UCHAR   Type;
    UCHAR   ShareDisposition;
    USHORT  Flags;
    union {
        struct {
            PHYSICAL_ADDRESS    Start;
            ULONG               Length;
        } Generic;

        struct {
            PHYSICAL_ADDRESS    Start;
            ULONG               Length;
        } Port;

        struct {
            ULONG   Level;
            ULONG   Vector;
            ULONG   Affinity;
        } Interrupt;

        struct {
            PHYSICAL_ADDRESS    Start;
            ULONG               Length;
        } Memory;

        struct {
            ULONG   Channel;
            ULONG   Port;
            ULONG   Reserved1;
        } Dma;

        struct {
            ULONG   Data[3];
        } DevicePrivate;

        struct {
            ULONG   Start;
            ULONG   Length;
            ULONG   Reserved;
        } BusNumber;

        struct {
            ULONG   DataSize;
            ULONG   Reserved1;
            ULONG   Reserved2;
        } DeviceSpecificData;
    } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct _CM_PARTIAL_RESOURCE_LIST
{
    USHORT  Version;
    USHORT  Revision;
    ULONG   Count;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR
{
    INTERFACE_TYPE  InterfaceType;
    ULONG           BusNumber;
    CM_PARTIAL_RESOURCE_LIST    PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST
{
    ULONG   Count;
    CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;



/* Operating System services */

/* Debugging */

ULONG CDECL DbgPrint(
    PCH Format,
    ...);

#ifdef DBG

VOID
STDCALL
RtlAssert(
    PVOID   FailedAssertion,
    PVOID   FileName,
    ULONG   LineNumber,
    PCHAR   Message);

#define ASSERT(exp) \
    if (!(exp)) \
        RtlAssert(#exp, __FILE__, __LINE__, NULL)

#define ASSERTMSG(msg, exp) \
    if (!(exp)) \
        RtlAssert(#exp, __FILE__, __LINE__, msg)

#else /* DBG */

#define ASSERT(exp)
#define ASSERTMSG(msg, exp)

#endif /* DBG */


/* Runtime library */

#define RtlEqualMemory(Destination,Source,Length)   (!memcmp((Destination), (Source), (Length)))
#define RtlMoveMemory(Destination,Source,Length)    memmove((Destination), (Source), (Length))
#define RtlCopyMemory(Destination,Source,Length)    memcpy((Destination), (Source), (Length))
#define RtlFillMemory(Destination,Length,Fill)      memset((Destination), (Fill),(Length))
#define RtlZeroMemory(Destination,Length)           memset((Destination), 0, (Length))


/* Core kernel functions */

VOID
STDCALL
KeStallExecutionProcessor(
    ULONG   MicroSeconds);


/* I/O functions */

VOID
STDCALL
READ_PORT_BUFFER_UCHAR(
    PUCHAR  Port,
    PUCHAR  Value,
    ULONG   Count);

VOID
STDCALL
READ_PORT_BUFFER_ULONG(
    PULONG  Port,
    PULONG  Value,
    ULONG   Count);

VOID
STDCALL
READ_PORT_BUFFER_USHORT(
    PUSHORT Port,
    PUSHORT Value,
    ULONG   Count);

UCHAR
STDCALL
READ_PORT_UCHAR(
    PUCHAR  Port);

ULONG
STDCALL
READ_PORT_ULONG(
    PULONG  Port);

USHORT
STDCALL
READ_PORT_USHORT(
    PUSHORT Port);

VOID
STDCALL
WRITE_PORT_BUFFER_UCHAR(
    PUCHAR  Port,
    PUCHAR  Value,
    ULONG   Count);

VOID
STDCALL
WRITE_PORT_BUFFER_ULONG(
    PULONG  Port,
    PULONG  Value,
    ULONG   Count);

VOID
STDCALL
WRITE_PORT_BUFFER_USHORT(
    PUSHORT Port,
    PUSHORT Value,
    ULONG   Count);

VOID
STDCALL
WRITE_PORT_UCHAR(
    PUCHAR  Port,
    UCHAR   Value);

VOID
STDCALL
WRITE_PORT_ULONG(
    PULONG  Port,
    ULONG   Value);

VOID
STDCALL
WRITE_PORT_USHORT(
    PUSHORT Port,
    USHORT  Value);

VOID
STDCALL
WRITE_REGISTER_UCHAR(
    PUCHAR  Register,
    UCHAR   Value);

VOID
STDCALL
WRITE_REGISTER_ULONG(
    PULONG  Register,
    ULONG   Value);

VOID
STDCALL
WRITE_REGISTER_USHORT(
    PUSHORT Register,
    USHORT  Value);

#endif /* __MINIPORT_H */

/* EOF */
