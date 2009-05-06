#ifndef _WDMDDK_
#define _WDMDDK_

//
// Dependencies
//
#define NT_INCLUDED
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif /* GUID_DEFINED */

#ifdef __GNUC__
#include "intrin.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _NTOSKRNL_
/* HACKHACKHACK!!! We shouldn't include this header from ntoskrnl! */
#define NTKERNELAPI
#else
#define NTKERNELAPI DECLSPEC_IMPORT
#endif

/* Simple types */
typedef UCHAR KPROCESSOR_MODE;
typedef LONG KPRIORITY;
typedef PVOID PSECURITY_DESCRIPTOR;
typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;

/* Structures not exposed to drivers */
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _HAL_DISPATCH_TABLE *PHAL_DISPATCH_TABLE;
typedef struct _HAL_PRIVATE_DISPATCH_TABLE *PHAL_PRIVATE_DISPATCH_TABLE;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;
typedef struct _BUS_HANDLER *PBUS_HANDLER;

typedef struct _ADAPTER_OBJECT *PADAPTER_OBJECT; 
typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;
typedef struct _ETHREAD *PETHREAD;
typedef struct _EPROCESS *PEPROCESS;
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _KINTERRUPT *PKINTERRUPT;
typedef struct _KPROCESS *PKPROCESS;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;


typedef struct _CONTEXT *PCONTEXT;

//
// Resource list definitions
//
typedef int CM_RESOURCE_TYPE;

#define CmResourceTypeNull              0
#define CmResourceTypePort              1
#define CmResourceTypeInterrupt         2
#define CmResourceTypeMemory            3
#define CmResourceTypeDma               4
#define CmResourceTypeDeviceSpecific    5
#define CmResourceTypeBusNumber         6
#define CmResourceTypeNonArbitrated	  128
#define CmResourceTypeConfigData	  128
#define CmResourceTypeDevicePrivate	  129
#define CmResourceTypePcCardConfig	  130
#define CmResourceTypeMfCardConfig	  131

typedef enum _INTERFACE_TYPE {
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
  PNPBus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

/* IO_RESOURCE_DESCRIPTOR.Option */

#define IO_RESOURCE_PREFERRED             0x01
#define IO_RESOURCE_DEFAULT               0x02
#define IO_RESOURCE_ALTERNATIVE           0x08

typedef struct _IO_RESOURCE_DESCRIPTOR {
  UCHAR  Option;
  UCHAR  Type;
  UCHAR  ShareDisposition;
  UCHAR  Spare1;
  USHORT  Flags;
  USHORT  Spare2;
  union {
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Port;
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Memory;
    struct {
      ULONG  MinimumVector;
      ULONG  MaximumVector;
    } Interrupt;
    struct {
      ULONG  MinimumChannel;
      ULONG  MaximumChannel;
    } Dma;
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Generic;
    struct {
      ULONG  Data[3];
    } DevicePrivate;
    struct {
      ULONG  Length;
      ULONG  MinBusNumber;
      ULONG  MaxBusNumber;
      ULONG  Reserved;
    } BusNumber;
    struct {
      ULONG  Priority;
      ULONG  Reserved1;
      ULONG  Reserved2;
    } ConfigData;
  } u;
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST {
  USHORT  Version;
  USHORT  Revision;
  ULONG  Count;
  IO_RESOURCE_DESCRIPTOR  Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
  ULONG  ListSize;
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  ULONG  SlotNumber;
  ULONG  Reserved[3];
  ULONG  AlternativeLists;
  IO_RESOURCE_LIST  List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

//
// Global debug flag
//
extern ULONG NtGlobalFlag;


#include <pshpack4.h>
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
  UCHAR Type;
  UCHAR ShareDisposition;
  USHORT Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Generic;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Port;
    struct {
      ULONG Level;
      ULONG Vector;
      KAFFINITY Affinity;
    } Interrupt;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      union {
        struct {
          USHORT Reserved;
          USHORT MessageCount;
          ULONG Vector;
          KAFFINITY Affinity;
        } Raw;
        struct {
          ULONG Level;
          ULONG Vector;
          KAFFINITY Affinity;
        } Translated;
      };
    } MessageInterrupt;
#endif
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Memory;
    struct {
      ULONG Channel;
      ULONG Port;
      ULONG Reserved1;
    } Dma;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Start;
      ULONG Length;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG DataSize;
      ULONG Reserved1;
      ULONG Reserved2;
    } DeviceSpecificData;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length40;
    } Memory40;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length48;
    } Memory48;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length64;
    } Memory64;
#endif
  } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;
#include <poppack.h>

//
// Section map options
//
typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

//
// Section access rights
//
#define SECTION_QUERY                0x0001
#define SECTION_MAP_WRITE            0x0002
#define SECTION_MAP_READ             0x0004
#define SECTION_MAP_EXECUTE          0x0008
#define SECTION_EXTEND_SIZE          0x0010
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|\
                            SECTION_MAP_WRITE |      \
                            SECTION_MAP_READ |       \
                            SECTION_MAP_EXECUTE |    \
                            SECTION_EXTEND_SIZE)

#define SESSION_QUERY_ACCESS  0x0001
#define SESSION_MODIFY_ACCESS 0x0002

#define SESSION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |  \
                            SESSION_QUERY_ACCESS |             \
                            SESSION_MODIFY_ACCESS)



#define SEGMENT_ALL_ACCESS SECTION_ALL_ACCESS

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD            0x100
#define PAGE_NOCACHE          0x200
#define PAGE_WRITECOMBINE     0x400

#define MEM_COMMIT           0x1000
#define MEM_RESERVE          0x2000
#define MEM_DECOMMIT         0x4000
#define MEM_RELEASE          0x8000
#define MEM_FREE            0x10000
#define MEM_PRIVATE         0x20000
#define MEM_MAPPED          0x40000
#define MEM_RESET           0x80000
#define MEM_TOP_DOWN       0x100000
#define MEM_LARGE_PAGES  0x20000000
#define MEM_4MB_PAGES    0x80000000

#define SEC_RESERVE       0x4000000     
#define SEC_LARGE_PAGES  0x80000000

#define PROCESS_DUP_HANDLE                 (0x0040)

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define PROCESS_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFFF)
#else
#define PROCESS_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFF)
#endif



//
// Processor features
//
#define PF_FLOATING_POINT_PRECISION_ERRATA  0   
#define PF_FLOATING_POINT_EMULATED          1   
#define PF_COMPARE_EXCHANGE_DOUBLE          2   
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3   
#define PF_PPC_MOVEMEM_64BIT_OK             4   
#define PF_ALPHA_BYTE_INSTRUCTIONS          5   
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6   
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7   
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8   
#define PF_PAE_ENABLED                      9   
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10   
#define PF_SSE_DAZ_MODE_AVAILABLE          11   
#define PF_NX_ENABLED                      12   
#define PF_SSE3_INSTRUCTIONS_AVAILABLE     13   
#define PF_COMPARE_EXCHANGE128             14   
#define PF_COMPARE64_EXCHANGE128           15   
#define PF_CHANNELS_ENABLED                16   



//
// Intrinsics (note: taken from our winnt.h)
// FIXME: 64-bit
//
#if defined(__GNUC__)

static __inline__ BOOLEAN
InterlockedBitTestAndSet(IN LONG volatile *Base,
                         IN LONG Bit)
{
#if defined(_M_IX86)
	LONG OldBit;
	__asm__ __volatile__("lock "
	                     "btsl %2,%1\n\t"
	                     "sbbl %0,%0\n\t"
	                     :"=r" (OldBit),"+m" (*Base)
	                     :"Ir" (Bit)
	                     : "memory");
	return OldBit;
#else
	return (_InterlockedOr(Base, 1 << Bit) >> Bit) & 1;
#endif
}

static __inline__ BOOLEAN
InterlockedBitTestAndReset(IN LONG volatile *Base,
                           IN LONG Bit)
{
#if defined(_M_IX86)
	LONG OldBit;
	__asm__ __volatile__("lock "
	                     "btrl %2,%1\n\t"
	                     "sbbl %0,%0\n\t"
	                     :"=r" (OldBit),"+m" (*Base)
	                     :"Ir" (Bit)
	                     : "memory");
	return OldBit;
#else
	return (_InterlockedAnd(Base, ~(1 << Bit)) >> Bit) & 1;
#endif
}

#endif

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse

#define BitTest _bittest
#define BitTestAndComplement _bittestandcomplement
#define BitTestAndSet _bittestandset
#define BitTestAndReset _bittestandreset
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndReset _interlockedbittestandreset


/** INTERLOCKED FUNCTIONS *****************************************************/

#if !defined(__INTERLOCKED_DECLARED)
#define __INTERLOCKED_DECLARED

#if defined (_X86_)
#if defined(NO_INTERLOCKED_INTRINSICS)
NTKERNELAPI
LONG
FASTCALL
InterlockedIncrement(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
LONG
FASTCALL
InterlockedDecrement(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
LONG
FASTCALL
InterlockedCompareExchange(
  IN OUT LONG volatile *Destination,
  IN LONG  Exchange,
  IN LONG  Comparand);

NTKERNELAPI
LONG
FASTCALL
InterlockedExchange(
  IN OUT LONG volatile *Destination,
  IN LONG Value);

NTKERNELAPI
LONG
FASTCALL
InterlockedExchangeAdd(
  IN OUT LONG volatile *Addend,
  IN LONG  Value);

#else // !defined(NO_INTERLOCKED_INTRINSICS)

#define InterlockedExchange _InterlockedExchange
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedOr _InterlockedOr
#define InterlockedAnd _InterlockedAnd
#define InterlockedXor _InterlockedXor

#endif // !defined(NO_INTERLOCKED_INTRINSICS)

#endif // defined (_X86_)

#if !defined (_WIN64)
/*
 * PVOID
 * InterlockedExchangePointer(
 *   IN OUT PVOID VOLATILE  *Target,
 *   IN PVOID  Value)
 */
#define InterlockedExchangePointer(Target, Value) \
  ((PVOID) InterlockedExchange((PLONG) Target, (LONG) Value))

/*
 * PVOID
 * InterlockedCompareExchangePointer(
 *   IN OUT PVOID  *Destination,
 *   IN PVOID  Exchange,
 *   IN PVOID  Comparand)
 */
#define InterlockedCompareExchangePointer(Destination, Exchange, Comparand) \
  ((PVOID) InterlockedCompareExchange((PLONG) Destination, (LONG) Exchange, (LONG) Comparand))

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement((LONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement((LONG *)a)

#endif // !defined (_WIN64)

#if defined (_M_AMD64)

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd64((LONGLONG *)a, (LONGLONG)b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement64((LONGLONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement64((LONGLONG *)a)
#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedAdd _InterlockedAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndReset64 _interlockedbittestandreset64

#endif // _M_AMD64

#if defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)
//#if !defined(_X86AMD64_) // FIXME: what's _X86AMD64_ used for?
FORCEINLINE
LONG64
InterlockedAdd64(
    IN OUT LONG64 volatile *Addend,
    IN LONG64 Value)
{
    return InterlockedExchangeAdd64(Addend, Value) + Value;
}
//#endif
#endif

#endif /* !__INTERLOCKED_DECLARED */

#if defined(_M_IX86)
#define YieldProcessor _mm_pause
#elif defined (_M_AMD64)
#define YieldProcessor _mm_pause
#elif defined(_M_PPC)
#define YieldProcessor() __asm__ __volatile__("nop");
#elif defined(_M_MIPS)
#define YieldProcessor() __asm__ __volatile__("nop");
#elif defined(_M_ARM)
#define YieldProcessor()
#else
#error Unknown architecture
#endif



//
// Slist Header
//
#ifndef _SLIST_HEADER_
#define _SLIST_HEADER_

#define SLIST_ENTRY SINGLE_LIST_ENTRY
#define _SLIST_ENTRY _SINGLE_LIST_ENTRY
#define PSLIST_ENTRY PSINGLE_LIST_ENTRY

#if defined(_WIN64)
typedef union _SLIST_HEADER {
    struct {
        ULONGLONG Alignment;
        ULONGLONG Region;
    } DUMMYSTRUCTNAME;
    struct {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:9;
        ULONGLONG NextEntry:39;
        ULONGLONG HeaderType:1;
        ULONGLONG Init:1;
        ULONGLONG Reserved:59;
        ULONGLONG Region:3;
    } Header8;
        ULONGLONG Depth:16;
        ULONGLONG Sequence:48;
        ULONGLONG HeaderType:1;
        ULONGLONG Init:1;
        ULONGLONG Reserved:2;
        ULONGLONG NextEntry:60;
    struct {
    } Header16;
} SLIST_HEADER, *PSLIST_HEADER;
#else
typedef union _SLIST_HEADER {
    ULONGLONG Alignment;
    struct {
        SLIST_ENTRY Next;
        USHORT Depth;
        USHORT Sequence;
    } DUMMYSTRUCTNAME;
} SLIST_HEADER, *PSLIST_HEADER;
#endif

#endif /* _SLIST_HEADER_ */



//
// Power States/Levels
//
typedef enum _SYSTEM_POWER_STATE {
    PowerSystemUnspecified,
    PowerSystemWorking,
    PowerSystemSleeping1,
    PowerSystemSleeping2,
    PowerSystemSleeping3,
    PowerSystemHibernate,
    PowerSystemShutdown,
    PowerSystemMaximum
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

#define POWER_SYSTEM_MAXIMUM PowerSystemMaximum

typedef enum _POWER_INFORMATION_LEVEL {
    SystemPowerPolicyAc,
    SystemPowerPolicyDc,
    VerifySystemPolicyAc,
    VerifySystemPolicyDc,
    SystemPowerCapabilities,
    SystemBatteryState,
    SystemPowerStateHandler,
    ProcessorStateHandler,
    SystemPowerPolicyCurrent,
    AdministratorPowerPolicy,
    SystemReserveHiberFile,
    ProcessorInformation,
    SystemPowerInformation,
    ProcessorStateHandler2,
    LastWakeTime,
    LastSleepTime,
    SystemExecutionState,
    SystemPowerStateNotifyHandler,
    ProcessorPowerPolicyAc,
    ProcessorPowerPolicyDc,
    VerifyProcessorPowerPolicyAc,
    VerifyProcessorPowerPolicyDc,
    ProcessorPowerPolicyCurrent
} POWER_INFORMATION_LEVEL;

typedef enum {
    PowerActionNone,
    PowerActionReserved,
    PowerActionSleep,
    PowerActionHibernate,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionShutdownOff,
    PowerActionWarmEject
} POWER_ACTION, *PPOWER_ACTION;

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceUnspecified,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

#define ES_SYSTEM_REQUIRED                0x00000001
#define ES_DISPLAY_REQUIRED               0x00000002
#define ES_USER_PRESENT                   0x00000004
#define ES_CONTINUOUS                     0x80000000

typedef ULONG EXECUTION_STATE;

typedef enum {
    LT_DONT_CARE,
    LT_LOWEST_LATENCY
} LATENCY_TIME;



//
// Access/Security Stuff
//
typedef ULONG ACCESS_MASK, *PACCESS_MASK;
typedef PVOID PACCESS_TOKEN;

#define DELETE                           0x00010000L
#define READ_CONTROL                     0x00020000L
#define WRITE_DAC                        0x00040000L
#define WRITE_OWNER                      0x00080000L
#define SYNCHRONIZE                      0x00100000L
#define STANDARD_RIGHTS_REQUIRED         0x000F0000L
#define STANDARD_RIGHTS_READ             READ_CONTROL
#define STANDARD_RIGHTS_WRITE            READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE          READ_CONTROL
#define STANDARD_RIGHTS_ALL              0x001F0000L
#define SPECIFIC_RIGHTS_ALL              0x0000FFFFL
#define ACCESS_SYSTEM_SECURITY           0x01000000L
#define MAXIMUM_ALLOWED                  0x02000000L
#define GENERIC_READ                     0x80000000L
#define GENERIC_WRITE                    0x40000000L
#define GENERIC_EXECUTE                  0x20000000L
#define GENERIC_ALL                      0x10000000L

typedef struct _GENERIC_MAPPING {
    ACCESS_MASK GenericRead;
    ACCESS_MASK GenericWrite;
    ACCESS_MASK GenericExecute;
    ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

#define ACL_REVISION                      2
#define ACL_REVISION_DS                   4

#define ACL_REVISION1                     1
#define ACL_REVISION2                     2
#define ACL_REVISION3                     3
#define ACL_REVISION4                     4
#define MIN_ACL_REVISION                  ACL_REVISION2
#define MAX_ACL_REVISION                  ACL_REVISION4

typedef struct _ACL {
    UCHAR AclRevision;
    UCHAR Sbz1;
    USHORT AclSize;
    USHORT AceCount;
    USHORT Sbz2;
} ACL, *PACL;



//
// Current security descriptor revision value
//
#define SECURITY_DESCRIPTOR_REVISION     (1)
#define SECURITY_DESCRIPTOR_REVISION1    (1)

//
// Privilege attributes
//
#define SE_PRIVILEGE_ENABLED_BY_DEFAULT (0x00000001L)
#define SE_PRIVILEGE_ENABLED            (0x00000002L)
#define SE_PRIVILEGE_REMOVED            (0X00000004L)
#define SE_PRIVILEGE_USED_FOR_ACCESS    (0x80000000L)

#define SE_PRIVILEGE_VALID_ATTRIBUTES   (SE_PRIVILEGE_ENABLED_BY_DEFAULT | \
                                         SE_PRIVILEGE_ENABLED            | \
                                         SE_PRIVILEGE_REMOVED            | \
                                         SE_PRIVILEGE_USED_FOR_ACCESS)

#include <pshpack4.h>
typedef struct _LUID_AND_ATTRIBUTES {
    LUID Luid;
    ULONG Attributes;
} LUID_AND_ATTRIBUTES, *PLUID_AND_ATTRIBUTES;
#include <poppack.h>
typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;



//
// Privilege sets
//
#define PRIVILEGE_SET_ALL_NECESSARY (1)

typedef struct _PRIVILEGE_SET {
    ULONG PrivilegeCount;
    ULONG Control;
    LUID_AND_ATTRIBUTES Privilege[ANYSIZE_ARRAY];
} PRIVILEGE_SET,*PPRIVILEGE_SET;

typedef enum _SECURITY_IMPERSONATION_LEVEL {
    SecurityAnonymous,
    SecurityIdentification,
    SecurityImpersonation,
    SecurityDelegation
} SECURITY_IMPERSONATION_LEVEL, * PSECURITY_IMPERSONATION_LEVEL;

#define SECURITY_MAX_IMPERSONATION_LEVEL SecurityDelegation
#define SECURITY_MIN_IMPERSONATION_LEVEL SecurityAnonymous
#define DEFAULT_IMPERSONATION_LEVEL SecurityImpersonation
#define VALID_IMPERSONATION_LEVEL(Level) (((Level) >= SECURITY_MIN_IMPERSONATION_LEVEL) && ((Level) <= SECURITY_MAX_IMPERSONATION_LEVEL))

#define SECURITY_DYNAMIC_TRACKING (TRUE)
#define SECURITY_STATIC_TRACKING (FALSE)

typedef BOOLEAN SECURITY_CONTEXT_TRACKING_MODE, *PSECURITY_CONTEXT_TRACKING_MODE;

typedef struct _SECURITY_QUALITY_OF_SERVICE {
    ULONG Length;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode;
    BOOLEAN EffectiveOnly;
} SECURITY_QUALITY_OF_SERVICE, *PSECURITY_QUALITY_OF_SERVICE;

typedef struct _SE_IMPERSONATION_STATE {
    PACCESS_TOKEN Token;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL Level;
} SE_IMPERSONATION_STATE, *PSE_IMPERSONATION_STATE;

#define OWNER_SECURITY_INFORMATION       (0x00000001L)
#define GROUP_SECURITY_INFORMATION       (0x00000002L)
#define DACL_SECURITY_INFORMATION        (0x00000004L)
#define SACL_SECURITY_INFORMATION        (0x00000008L)
#define LABEL_SECURITY_INFORMATION       (0x00000010L)

#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#define PROTECTED_SACL_SECURITY_INFORMATION     (0x40000000L)
#define UNPROTECTED_DACL_SECURITY_INFORMATION   (0x20000000L)
#define UNPROTECTED_SACL_SECURITY_INFORMATION   (0x10000000L)



//
// Registry Access Rights
//
#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)
#define KEY_WOW64_RES           (0x0300)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))

//
// Registry Open/Create Options
//
#define REG_OPTION_RESERVED         (0x00000000L)
#define REG_OPTION_NON_VOLATILE     (0x00000000L)
#define REG_OPTION_VOLATILE         (0x00000001L)
#define REG_OPTION_CREATE_LINK      (0x00000002L)
#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)
#define REG_OPTION_OPEN_LINK        (0x00000008L)

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

//
// Key creation/open disposition
//
#define REG_CREATED_NEW_KEY         (0x00000001L)
#define REG_OPENED_EXISTING_KEY     (0x00000002L)

//
// Key restore & hive load flags
//
#define REG_WHOLE_HIVE_VOLATILE         (0x00000001L)
#define REG_REFRESH_HIVE                (0x00000002L)
#define REG_NO_LAZY_FLUSH               (0x00000004L)
#define REG_FORCE_RESTORE               (0x00000008L)
#define REG_APP_HIVE                    (0x00000010L)
#define REG_PROCESS_PRIVATE             (0x00000020L)
#define REG_START_JOURNAL               (0x00000040L)
#define REG_HIVE_EXACT_FILE_GROWTH      (0x00000080L)
#define REG_HIVE_NO_RM                  (0x00000100L)
#define REG_HIVE_SINGLE_LOG             (0x00000200L)

//
// Unload Flags
//
#define REG_FORCE_UNLOAD            1

//
// Notify Filter Values
//
#define REG_NOTIFY_CHANGE_NAME          (0x00000001L)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L)
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)

#define REG_LEGAL_CHANGE_FILTER                 \
                (REG_NOTIFY_CHANGE_NAME          |\
                 REG_NOTIFY_CHANGE_ATTRIBUTES    |\
                 REG_NOTIFY_CHANGE_LAST_SET      |\
                 REG_NOTIFY_CHANGE_SECURITY)



//
// Thread Access Rights
//
#define THREAD_TERMINATE                 (0x0001)  
#define THREAD_SUSPEND_RESUME            (0x0002)  
#define THREAD_ALERT                     (0x0004)
#define THREAD_GET_CONTEXT               (0x0008)  
#define THREAD_SET_CONTEXT               (0x0010)  
#define THREAD_SET_INFORMATION           (0x0020)  
#define THREAD_SET_LIMITED_INFORMATION   (0x0400)  
#define THREAD_QUERY_LIMITED_INFORMATION (0x0800)  
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define THREAD_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFFF)
#else
#define THREAD_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0x3FF)
#endif

//
// Service Start Types
//
#define SERVICE_BOOT_START             0x00000000
#define SERVICE_SYSTEM_START           0x00000001
#define SERVICE_AUTO_START             0x00000002
#define SERVICE_DEMAND_START           0x00000003
#define SERVICE_DISABLED               0x00000004

//
// Exception Records
//
#define EXCEPTION_NONCONTINUABLE 1
#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _EXCEPTION_RECORD32 {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    ULONG ExceptionRecord;
    ULONG ExceptionAddress;
    ULONG NumberParameters;
    ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD32, *PEXCEPTION_RECORD32;

typedef struct _EXCEPTION_RECORD64 {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    ULONG64 ExceptionRecord;
    ULONG64 ExceptionAddress;
    ULONG NumberParameters;
    ULONG __unusedAlignment;
    ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;



//
// Process Qoutas
//
typedef struct _QUOTA_LIMITS {
    SIZE_T PagedPoolLimit;
    SIZE_T NonPagedPoolLimit;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    SIZE_T PagefileLimit;
    LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010


/******************************************************************************
 *                             WINBASE Functions                              *
 ******************************************************************************/
#if !defined(_WINBASE_)

#if defined(_WIN64)

#define InterlockedPopEntrySList(Head) \
    ExpInterlockedPopEntrySList(Head)

#define InterlockedPushEntrySList(Head, Entry) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define InterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#else // !defined(_WIN64)

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead);

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry);

#define InterlockedFlushSList(ListHead) \
    ExInterlockedFlushSList(ListHead)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#endif // !defined(_WIN64)

#endif // !defined(_WINBASE_)


/******************************************************************************
 *                              Kernel Types                                  *
 ******************************************************************************/

typedef struct _DISPATCHER_HEADER
{
    union
    {
        struct
        {
            UCHAR Type;
            union
            {
                UCHAR Absolute;
                UCHAR NpxIrql;
            };
            union
            {
                UCHAR Size;
                UCHAR Hand;
            };
            union
            {
                UCHAR Inserted;
                BOOLEAN DebugActive;
            };
        };
        volatile LONG Lock;
    };
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KEVENT {
  DISPATCHER_HEADER  Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

/******************************************************************************
 *                                 RTL Types                                  *
 ******************************************************************************/

#define RTL_REGISTRY_ABSOLUTE             0
#define RTL_REGISTRY_SERVICES             1
#define RTL_REGISTRY_CONTROL              2
#define RTL_REGISTRY_WINDOWS_NT           3
#define RTL_REGISTRY_DEVICEMAP            4
#define RTL_REGISTRY_USER                 5
#define RTL_REGISTRY_MAXIMUM              6
#define RTL_REGISTRY_HANDLE               0x40000000
#define RTL_REGISTRY_OPTIONAL             0x80000000

/* RTL_QUERY_REGISTRY_TABLE.Flags */
#define RTL_QUERY_REGISTRY_SUBKEY         0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY         0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED       0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE        0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND       0x00000010
#define RTL_QUERY_REGISTRY_DIRECT         0x00000020
#define RTL_QUERY_REGISTRY_DELETE         0x00000040

typedef struct _RTL_BITMAP {
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN {
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

typedef NTSTATUS
(DDKAPI *PRTL_QUERY_REGISTRY_ROUTINE)(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE {
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PCWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _TIME_FIELDS {
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;


/******************************************************************************
 *                               RTL Functions                                *
 ******************************************************************************/

NTSYSAPI
VOID
NTAPI
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message);

/* VOID
 * RtlCopyMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN CONST VOID UNALIGNED *Source,
 *     IN SIZE_T Length)
 */
#define RtlCopyMemory(Destination, Source, Length) \
    memcpy(Destination, Source, Length)

#define RtlCopyBytes RtlCopyMemory

#if defined(_M_AMD64)
NTSYSAPI
VOID
NTAPI
RtlCopyMemoryNonTemporal(
    VOID UNALIGNED *Destination,
    CONST VOID UNALIGNED *Source,
    SIZE_T Length);
#else
#define RtlCopyMemoryNonTemporal RtlCopyMemory
#endif

/* BOOLEAN
 * RtlEqualLuid(
 *     IN PLUID Luid1,
 *     IN PLUID Luid2)
 */
#define RtlEqualLuid(Luid1, Luid2) \
    (((Luid1)->LowPart == (Luid2)->LowPart) && ((Luid1)->HighPart == (Luid2)->HighPart))

/* ULONG
 * RtlEqualMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN CONST VOID UNALIGNED *Source,
 *     IN SIZE_T Length)
 */
#define RtlEqualMemory(Destination, Source, Length) \
    (!memcmp(Destination, Source, Length))

/* VOID
 * RtlFillMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN SIZE_T Length,
 *     IN UCHAR Fill)
 */
#define RtlFillMemory(Destination, Length, Fill) \
    memset(Destination, Fill, Length)

#define RtlFillBytes RtlFillMemory

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    IN PUNICODE_STRING UnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    IN PUNICODE_STRING GuidString,
    OUT GUID *Guid);

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString);

/* VOID
 * RtlMoveMemory(
 *    IN VOID UNALIGNED *Destination,
 *    IN CONST VOID UNALIGNED *Source,
 *    IN SIZE_T Length)
 */
#define RtlMoveMemory(Destination, Source, Length) \
    memmove(Destination, Source, Length)

NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
    IN REFGUID Guid,
    OUT PUNICODE_STRING GuidString);

/* VOID
 * RtlZeroMemory(
 *     IN VOID UNALIGNED *Destination,
 *     IN SIZE_T Length)
 */
#define RtlZeroMemory(Destination, Length) \
    memset(Destination, 0, Length)

#define RtlZeroBytes RtlZeroMemory


#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length);

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PANSI_STRING SourceString,
    IN BOOLEAN AllocateDestinationString);

NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
    IN PCANSI_STRING AnsiString);

#define RtlAnsiStringToUnicodeSize(String) (               \
  NLS_MB_CODE_PAGE_TAG ?                                   \
  RtlxAnsiStringToUnicodeSize(String) :                    \
  ((String)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)   \
)

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
    IN OUT PUNICODE_STRING Destination,
    IN PCUNICODE_STRING Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
    IN OUT PUNICODE_STRING Destination,
    IN PCWSTR Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
    IN ULONG RelativeTo,
    IN PWSTR Path);

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
    IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToClear);

NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory(
    IN CONST VOID *Source1,
    IN CONST VOID *Source2,
    IN SIZE_T Length);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
    IN PCWCH String1,
    IN SIZE_T String1Length,
    IN PCWCH String2,
    IN SIZE_T String2Length,
    IN BOOLEAN CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PCUNICODE_STRING  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
    IN ULONG RelativeTo,
    IN PWSTR Path);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Revision);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRegistryValue(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PCWSTR ValueName);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    IN CONST UNICODE_STRING *String1,
    IN CONST UNICODE_STRING *String2,
    IN BOOLEAN CaseInSensitive);

#if !defined(_AMD64_) && !defined(_IA64_)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply(
    IN LARGE_INTEGER Multiplicand,
    IN LONG Multiplier);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN ULONG Divisor,
  IN OUT PULONG Remainder);
#endif

#if defined(_X86_) || defined(_IA64_)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedMagicDivide(
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER MagicDivisor,
    IN CCHAR  ShiftCount);
#endif

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    IN PANSI_STRING AnsiString);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
    IN PRTL_BITMAP BitMapHeader,
    OUT PULONG StartingIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
    IN PRTL_BITMAP BitMapHeader,
    OUT PRTL_BITMAP_RUN RunArray,
    IN ULONG SizeOfRunArray,
    IN BOOLEAN LocateLongestRuns);

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    OUT PULONG StartingRunIndex);

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
    IN ULONGLONG Set);

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
    IN PRTL_BITMAP BitMapHeader,
    OUT PULONG StartingIndex);

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
    IN ULONGLONG Set);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    OUT PULONG StartingRunIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex);

NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
    IN CONST UNICODE_STRING *String,
    IN BOOLEAN CaseInSensitive,
    IN ULONG HashAlgorithm,
    OUT PULONG HashValue);

NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    IN OUT PANSI_STRING DestinationString,
    IN PCSZ SourceString);

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG BitMapBuffer,
    IN ULONG SizeOfBitMap);

NTSYSAPI
VOID
NTAPI
RtlInitString(
    IN OUT PSTRING DestinationString,
    IN PCSZ SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String);

NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString(
    IN ULONGLONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String);

#ifdef _WIN64
#define RtlIntPtrToUnicodeString(Value, Base, String) \
    RtlInt64ToUnicodeString(Value, Base, String)
#else
#define RtlIntPtrToUnicodeString(Value, Base, String) \
    RtlIntegerToUnicodeString(Value, Base, String)
#endif

/* BOOLEAN
 * RtlIsZeroLuid(
 *     IN PLUID L1);
 */
#define RtlIsZeroLuid(_L1) \
    ((BOOLEAN) ((!(_L1)->LowPart) && (!(_L1)->HighPart)))

NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
    IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
    IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PVOID Context,
    IN PVOID Environment OPTIONAL);

#define LONG_SIZE (sizeof(LONG))
#define LONG_MASK (LONG_SIZE - 1)

/* VOID
 * RtlRetrieveUlong(
 *	  PULONG DestinationAddress,
 *	  PULONG SourceAddress);
 */
#if defined(_AMD64_)
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    *(ULONG UNALIGNED *)(DestAddress) = *(PULONG)(SrcAddress)
#else
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
    { \
        ((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
        ((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
        ((PUCHAR)(DestAddress))[2]=((PUCHAR)(SrcAddress))[2]; \
        ((PUCHAR)(DestAddress))[3]=((PUCHAR)(SrcAddress))[3]; \
    } \
    else \
    { \
        *((PULONG)(DestAddress))=*((PULONG)(SrcAddress)); \
    }
#endif

/* VOID
 * RtlRetrieveUshort(
 *	  PUSHORT DestinationAddress,
 *	  PUSHORT SourceAddress);
 */
#if defined(_AMD64_)
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    *(USHORT UNALIGNED *)(DestAddress) = *(USHORT)(SrcAddress)
#else
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
    { \
        ((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
        ((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
    } \
    else \
    { \
        *((PUSHORT)(DestAddress))=*((PUSHORT)(SrcAddress)); \
    }
#endif

NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
    IN PRTL_BITMAP BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToSet);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN DaclPresent,
    IN PACL Dacl OPTIONAL,
    IN BOOLEAN DaclDefaulted OPTIONAL);

/* VOID
 * RtlStoreUlong(
 *     IN PULONG Address,
 *     IN ULONG Value);
 */
#if defined(_AMD64_)
#define RtlStoreUlong(Address,Value) \
    *(ULONG UNALIGNED *)(Address) = (Value)
#else
#define RtlStoreUlong(Address,Value)                      \
    if ((ULONG_PTR)(Address) & LONG_MASK) { \
        ((PUCHAR) (Address))[LONG_LEAST_SIGNIFICANT_BIT]    = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_3RD_MOST_SIGNIFICANT_BIT] = (UCHAR)(SECONDBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_2ND_MOST_SIGNIFICANT_BIT] = (UCHAR)(THIRDBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_MOST_SIGNIFICANT_BIT]     = (UCHAR)(FOURTHBYTE(Value)); \
    } \
    else { \
        *((PULONG)(Address)) = (ULONG) (Value); \
    }
#endif

/* VOID
 * RtlStoreUlonglong(
 *     IN OUT PULONGLONG Address,
 *     ULONGLONG Value);
 */
#if defined(_AMD64_)
#define RtlStoreUlonglong(Address,Value) \
    *(ULONGLONG UNALIGNED *)(Address) = (Value)
#else
#define RtlStoreUlonglong(Address,Value) \
    if ((ULONG_PTR)(Address) & LONGLONG_MASK) { \
        RtlStoreUlong((ULONG_PTR)(Address), \
                      (ULONGLONG)(Value) & 0xFFFFFFFF); \
        RtlStoreUlong((ULONG_PTR)(Address)+sizeof(ULONG), \
                      (ULONGLONG)(Value) >> 32); \
    } else { \
        *((PULONGLONG)(Address)) = (ULONGLONG)(Value); \
    }
#endif

/* VOID
 * RtlStoreUlongPtr(
 *     IN OUT PULONG_PTR Address,
 *     IN ULONG_PTR Value);
 */
#ifdef _WIN64
#define RtlStoreUlongPtr(Address,Value)                         \
    RtlStoreUlonglong(Address,Value)
#else
#define RtlStoreUlongPtr(Address,Value)                         \
    RtlStoreUlong(Address,Value)
#endif

/* VOID
 * RtlStoreUshort(
 *     IN PUSHORT Address,
 *     IN USHORT Value);
 */
#if defined(_AMD64_)
#define RtlStoreUshort(Address,Value) \
    *(USHORT UNALIGNED *)(Address) = (Value)
#else
#define RtlStoreUshort(Address,Value) \
    if ((ULONG_PTR)(Address) & SHORT_MASK) { \
        ((PUCHAR) (Address))[SHORT_LEAST_SIGNIFICANT_BIT] = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[SHORT_MOST_SIGNIFICANT_BIT ] = (UCHAR)(SECONDBYTE(Value)); \
    } \
    else { \
        *((PUSHORT) (Address)) = (USHORT)Value; \
    }
#endif

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
    IN PTIME_FIELDS TimeFields,
    IN PLARGE_INTEGER Time);

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
    IN PLARGE_INTEGER Time,
    IN PTIME_FIELDS TimeFields);

NTSYSAPI
ULONG
FASTCALL
RtlUlongByteSwap(
    IN ULONG Source);

NTSYSAPI
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    IN ULONGLONG Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    IN OUT PANSI_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString);

NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(
    IN PCUNICODE_STRING UnicodeString);

#define RtlUnicodeStringToAnsiSize(String) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(String) :                     \
    ((String)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
    IN PCUNICODE_STRING String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value);

NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
    IN WCHAR SourceCharacter);

NTSYSAPI
USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PCWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength);

#endif // (NTDDI_VERSION >= NTDDI_WIN2K)

#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTSYSAPI
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
    IN PVOID Source,
    IN SIZE_T Length);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
VOID
NTAPI
RtlClearBit(
    PRTL_BITMAP BitMapHeader,
    ULONG BitNumber);

NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(
    IN WCHAR SourceCharacter);

NTSYSAPI
VOID
NTAPI
RtlSetBit(
    PRTL_BITMAP BitMapHeader,
    ULONG BitNumber);

NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG BitNumber);

#endif // (NTDDI_VERSION >= NTDDI_WINXP)

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsUlongPtr(
    IN ULONG_PTR Target);

NTSYSAPI
ULONGLONG
NTAPI
RtlIoDecodeMemIoResource (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Alignment OPTIONAL,
    OUT PULONGLONG MinimumAddress OPTIONAL,
    OUT PULONGLONG MaximumAddress OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlIoEncodeMemIoResource(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    IN UCHAR Type,
    IN ULONGLONG Length,
    IN ULONGLONG Alignment,
    IN ULONGLONG MinimumAddress,
    IN ULONGLONG MaximumAddress);

NTSYSAPI
ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlFindClosestEncodableLength(
    IN ULONGLONG SourceLength,
    OUT PULONGLONG TargetLength);

#endif

#if !defined(MIDL_PASS)
/* inline funftions */
//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertLongToLargeInteger(LONG SignedInteger)
{
    LARGE_INTEGER ret;
    ret.QuadPart = SignedInteger;
    return ret;
}

//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertUlongToLargeInteger(
  ULONG UnsignedInteger)
{
    LARGE_INTEGER ret;
    ret.QuadPart = UnsignedInteger;
    return ret;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
ULONG
NTAPI_INLINE
RtlEnlargedUnsignedDivide(
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN OUT PULONG Remainder)
{
    if (Remainder)
        *Remainder = Dividend.QuadPart % Divisor;
    return Dividend.QuadPart / Divisor;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedUnsignedMultiply(
    IN ULONG Multiplicand,
    IN ULONG Multiplier)
{
    LARGE_INTEGER ret;
    ret.QuadPart = (ULONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return ret;
}

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedIntegerMultiply(
    IN LONG Multiplicand,
    IN LONG Multiplier)
{
    LARGE_INTEGER ret;
    ret.QuadPart = (LONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return ret;
}

FORCEINLINE
VOID
RtlInitEmptyAnsiString(OUT PANSI_STRING AnsiString,
                       IN PCHAR Buffer,
                       IN USHORT BufferSize)
{
    AnsiString->Length = 0;
    AnsiString->MaximumLength = BufferSize;
    AnsiString->Buffer = Buffer;
}

FORCEINLINE
VOID
RtlInitEmptyUnicodeString(
    OUT PUNICODE_STRING UnicodeString,
    IN PWSTR Buffer,
    IN USHORT BufferSize)
{
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = BufferSize;
    UnicodeString->Buffer = Buffer;
}

#if defined(_AMD64_) || defined(_IA64_)
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedIntegerMultiply(
    LARGE_INTEGER Multiplicand,
    LONG Multiplier)
{
    LARGE_INTEGER ret;
    ret.QuadPart = Multiplicand.QuadPart * Multiplier;
    return ret;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedLargeIntegerDivide(
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder)
{
    LARGE_INTEGER ret;
    ret.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
    if (Remainder)
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    return ret;
}
#endif

#if defined(_AMD64_)

#define MultiplyHigh __mulh
#define UnsignedMultiplyHigh __umulh

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedMagicDivide(
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER MagicDivisor,
    IN CCHAR ShiftCount)
{
    LARGE_INTEGER ret;
    ULONG64 ret64;
    BOOLEAN Pos;
    Pos = (Dividend.QuadPart >= 0);
    ret64 = UnsignedMultiplyHigh(Pos ? Dividend.QuadPart : -Dividend.QuadPart,
                                 MagicDivisor.QuadPart);
    ret64 >>= ShiftCount;
    ret.QuadPart = Pos ? ret64 : -ret64;
    return ret;
}
#endif

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerAdd(
    IN LARGE_INTEGER Addend1,
    IN LARGE_INTEGER Addend2)
{
    LARGE_INTEGER ret;
    ret.QuadPart = Addend1.QuadPart + Addend2.QuadPart;
    return ret;
}

/* VOID
 * RtlLargeIntegerAnd(
 *     IN OUT LARGE_INTEGER Result,
 *     IN LARGE_INTEGER Source,
 *     IN LARGE_INTEGER Mask);
 */
#define RtlLargeIntegerAnd(Result, Source, Mask) \
    Result.QuadPart = Source.QuadPart & Mask.QuadPart

//DECLSPEC_DEPRECATED_DDK
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerArithmeticShift(
    IN LARGE_INTEGER LargeInteger,
    IN CCHAR ShiftCount)
{
    LARGE_INTEGER ret;
    ret.QuadPart = LargeInteger.QuadPart >> ShiftCount;
    return ret;
}

/* BOOLEAN
 * RtlLargeIntegerEqualTo(
 *     IN LARGE_INTEGER  Operand1,
 *     IN LARGE_INTEGER  Operand2);
 */
#define RtlLargeIntegerEqualTo(X,Y) \
    (!(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))

FORCEINLINE
PVOID
RtlSecureZeroMemory(
    OUT PVOID Pointer,
    IN SIZE_T Size)
{
    volatile char* vptr = (volatile char*)Pointer;
#if defined(_M_AMD64)
    __stosb((PUCHAR)vptr, 0, Size);
#else
    char * endptr = (char *)vptr + Size;
    while (vptr < endptr)
    {
        *vptr = 0; vptr++;
    }
#endif
     return Pointer;
}

#if defined(_M_AMD64)
FORCEINLINE
ULONG
RtlCheckBit(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG BitPosition)
{
    return BitTest((LONG CONST*)BitMapHeader->Buffer, BitPosition);
}
#else
#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP)/32]) >> ((BP)%32)) & 0x1)
#endif // defined(_M_AMD64)

#endif // !defined(MIDL_PASS)

//
// Byte Swap Functions
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037 || defined(__GNUC__))) || \
    ((defined(_M_AMD64) || defined(_M_IA64)) \
        && (_MSC_FULL_VER > 13009175 || defined(__GNUC__)))

#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))

#endif

/******************************************************************************
 *                         Memory manager Types                               *
 ******************************************************************************/

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


/******************************************************************************
 *                       Memory manager Functions                             *
 ******************************************************************************/

/* PVOID MmGetSystemAddressForMdl(
 *     IN PMDL Mdl);
 */
#define MmGetSystemAddressForMdl(Mdl) \
  (((Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | \
    MDL_SOURCE_IS_NONPAGED_POOL)) ? \
      ((Mdl)->MappedSystemVa) : \
      (MmMapLockedPages((Mdl), KernelMode)))

/* PVOID
 * MmGetSystemAddressForMdlSafe(
 *     IN PMDL Mdl,
 *     IN MM_PAGE_PRIORITY Priority)
 */
#define MmGetSystemAddressForMdlSafe(_Mdl, _Priority) \
  (((_Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA \
    | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
    (_Mdl)->MappedSystemVa : \
    (PVOID) MmMapLockedPagesSpecifyCache((_Mdl), \
      KernelMode, MmCached, NULL, FALSE, (_Priority)))

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
PMDL
NTAPI
MmCreateMdl(
  IN PMDL  MemoryDescriptorList  OPTIONAL,
  IN PVOID  Base,
  IN SIZE_T  Length);

#endif


/******************************************************************************
 *                         I/O Manager Functions                              *
 ******************************************************************************/

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && \
   (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)

#define DMA_MACROS_DEFINED

FORCEINLINE
NTSTATUS
IoAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context)
{
    PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
    AllocateAdapterChannel =
        *(DmaAdapter)->DmaOperations->AllocateAdapterChannel;
    ASSERT(AllocateAdapterChannel);
    return AllocateAdapterChannel(DmaAdapter,
                                  DeviceObject,
                                  NumberOfMapRegisters,
                                  ExecutionRoutine,
                                  Context );
}

FORCEINLINE
BOOLEAN
IoFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice)
{
    PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
    FlushAdapterBuffers = *(DmaAdapter)->DmaOperations->FlushAdapterBuffers;
    ASSERT(FlushAdapterBuffers);
    return FlushAdapterBuffers(DmaAdapter,
                               Mdl,
                               MapRegisterBase,
                               CurrentVa,
                               Length,
                               WriteToDevice );
}

FORCEINLINE
VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject)
{
    PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
    FreeAdapterChannel = *(DmaAdapter)->DmaOperations->FreeAdapterChannel;
    ASSERT(FreeAdapterChannel);
    FreeAdapterChannel(DmaAdapter);
}

FORCEINLINE
VOID
IoFreeMapRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN PVOID MapRegisterBase,
    IN ULONG NumberOfMapRegisters)
{
    PFREE_MAP_REGISTERS FreeMapRegisters;
    FreeMapRegisters = *(DmaAdapter)->DmaOperations->FreeMapRegisters;
    ASSERT(FreeMapRegisters);
    FreeMapRegisters(DmaAdapter, MapRegisterBase, NumberOfMapRegisters);
}

FORCEINLINE
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice)
{
    PMAP_TRANSFER MapTransfer;

    MapTransfer = *(DmaAdapter)->DmaOperations->MapTransfer;
    ASSERT(MapTransfer);
    return MapTransfer(DmaAdapter,
                       Mdl,
                       MapRegisterBase,
                       CurrentVa,
                       Length,
                       WriteToDevice);
}
#endif


/******************************************************************************
 *                            Executive Types                                 *
 ******************************************************************************/

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType,
    NonPagedPoolSession = 32,
    PagedPoolSession,
    NonPagedPoolMustSucceedSession,
    DontUseThisTypeSession,
    NonPagedPoolCacheAlignedSession,
    PagedPoolCacheAlignedSession,
    NonPagedPoolCacheAlignedMustSSession
} POOL_TYPE;

typedef enum _SUITE_TYPE {
    SmallBusiness,
    Enterprise,
    BackOffice,
    CommunicationServer,
    TerminalServer,
    SmallBusinessRestricted,
    EmbeddedNT,
    DataCenter,
    SingleUserTS,
    Personal,
    Blade,
    MaxSuiteType
} SUITE_TYPE;

typedef enum _EX_POOL_PRIORITY {
    LowPoolPriority,
    LowPoolPrioritySpecialPoolOverrun = 8,
    LowPoolPrioritySpecialPoolUnderrun = 9,
    NormalPoolPriority = 16,
    NormalPoolPrioritySpecialPoolOverrun = 24,
    NormalPoolPrioritySpecialPoolUnderrun = 25,
    HighPoolPriority = 32,
    HighPoolPrioritySpecialPoolOverrun = 40,
    HighPoolPrioritySpecialPoolUnderrun = 41
} EX_POOL_PRIORITY;

typedef struct _FAST_MUTEX
{
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Gate;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef ULONG_PTR ERESOURCE_THREAD, *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
    ERESOURCE_THREAD OwnerThread;
    _ANONYMOUS_UNION union {
        LONG OwnerCount;
        ULONG TableSize;
    } DUMMYUNIONNAME;
} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE
{
    LIST_ENTRY SystemResourcesList;
    POWNER_ENTRY OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    volatile PKSEMAPHORE SharedWaiters;
    volatile PKEVENT ExclusiveWaiters;
    OWNER_ENTRY OwnerEntry;
    ULONG ActiveEntries;
    ULONG ContentionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
    union
    {
        PVOID Address;
        ULONG_PTR CreatorBackTraceIndex;
    };
    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NDIS_))
#define LOOKASIDE_ALIGN
#else
#define LOOKASIDE_ALIGN /* FIXME: DECLSPEC_CACHEALIGN */
#endif

typedef struct _LOOKASIDE_LIST_EX *PLOOKASIDE_LIST_EX;

typedef PVOID
(DDKAPI *PALLOCATE_FUNCTION)(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

typedef PVOID
(DDKAPI *PALLOCATE_FUNCTION_EX)(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN OUT PLOOKASIDE_LIST_EX Lookaside);

typedef VOID
(DDKAPI *PFREE_FUNCTION)(
    IN PVOID Buffer);

typedef VOID
(DDKAPI *PFREE_FUNCTION_EX)(
    IN PVOID Buffer,
    IN OUT PLOOKASIDE_LIST_EX Lookaside);

typedef VOID
(DDKAPI *PCALLBACK_FUNCTION)(
  IN PVOID  CallbackContext,
  IN PVOID  Argument1,
  IN PVOID  Argument2);

typedef struct LOOKASIDE_ALIGN _GENERAL_LOOKASIDE {
    union {
        SLIST_HEADER ListHead;
        SINGLE_LIST_ENTRY SingleListHead;
    } DUMMYUNIONNAME;
    USHORT Depth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    union {
        ULONG AllocateMisses;
        ULONG AllocateHits;
    } DUMMYUNIONNAME2;
    ULONG TotalFrees;
    union {
        ULONG FreeMisses;
        ULONG FreeHits;
    } DUMMYUNIONNAME3;
    POOL_TYPE Type;
    ULONG Tag;
    ULONG Size;
    union {
        PALLOCATE_FUNCTION_EX AllocateEx; 
        PALLOCATE_FUNCTION Allocate;
    } DUMMYUNIONNAME4;
    union {
        PFREE_FUNCTION_EX FreeEx;
        PFREE_FUNCTION Free;
    } DUMMYUNIONNAME5;
    LIST_ENTRY ListEntry;
    ULONG  LastTotalAllocates;
    union {
        ULONG  LastAllocateMisses;
        ULONG  LastAllocateHits;
    } DUMMYUNIONNAME6;
    ULONG Future[2];
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

typedef struct _PAGED_LOOKASIDE_LIST {
    GENERAL_LOOKASIDE L;
#if !defined(_AMD64_) && !defined(_IA64_)
    FAST_MUTEX Lock__ObsoleteButDoNotDelete;
#endif
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

typedef struct LOOKASIDE_ALIGN _NPAGED_LOOKASIDE_LIST {
    GENERAL_LOOKASIDE L;
#if !defined(_AMD64_) && !defined(_IA64_)
    KSPIN_LOCK Lock__ObsoleteButDoNotDelete;
#endif
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

//typedef struct _LOOKASIDE_LIST_EX {
//    GENERAL_LOOKASIDE_POOL L;
//} LOOKASIDE_LIST_EX, *PLOOKASIDE_LIST_EX;

typedef struct _EX_RUNDOWN_REF {
    union {
        volatile ULONG_PTR Count;
        volatile PVOID Ptr;
    };
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

typedef struct _EX_RUNDOWN_REF_CACHE_AWARE  *PEX_RUNDOWN_REF_CACHE_AWARE;

typedef enum _WORK_QUEUE_TYPE {
  CriticalWorkQueue,
  DelayedWorkQueue,
  HyperCriticalWorkQueue,
  MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef VOID
(DDKAPI *PWORKER_THREAD_ROUTINE)(
  IN PVOID Parameter);

typedef struct _WORK_QUEUE_ITEM {
  LIST_ENTRY  List;
  PWORKER_THREAD_ROUTINE  WorkerRoutine;
  volatile PVOID  Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;


/******************************************************************************
 *                          Executive Functions                               *
 ******************************************************************************/

#if defined(_X86_)
#if defined(_NTHAL_)
#define ExAcquireFastMutex ExiAcquireFastMutex
#define ExReleaseFastMutex ExiReleaseFastMutex
#define ExTryToAcquireFastMutex ExiTryToAcquireFastMutex
#endif
#define ExInterlockedAddUlong ExfInterlockedAddUlong
#define ExInterlockedInsertHeadList ExfInterlockedInsertHeadList
#define ExInterlockedInsertTailList ExfInterlockedInsertTailList
#define ExInterlockedRemoveHeadList ExfInterlockedRemoveHeadList
#define ExInterlockedPopEntryList ExfInterlockedPopEntryList
#define ExInterlockedPushEntryList ExfInterlockedPushEntryList
#endif

#if defined(_WIN64)

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || \
    defined(_NTHAL_) || defined(_NTOSP_)
NTKERNELAPI
USHORT
ExQueryDepthSList(IN PSLIST_HEADER ListHead);
#else
FORCEINLINE
USHORT
ExQueryDepthSList(IN PSLIST_HEADER ListHead)
{
    return (USHORT)(ListHead->Alignment & 0xffff);
}
#endif

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedFlushSList(
    PSLIST_HEADER ListHead);

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPopEntrySList(
    PSLIST_HEADER ListHead);

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPushEntrySList(
    PSLIST_HEADER ListHead,
    PSLIST_ENTRY ListEntry);

#define ExInterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)
#define ExInterlockedPopEntrySList(Head, Lock) \
    ExpInterlockedPopEntrySList(Head)
#define ExInterlockedPushEntrySList(Head, Entry, Lock) \
    ExpInterlockedPushEntrySList(Head, Entry)

#else // !defined(_WIN64)

#define ExQueryDepthSList(listhead) (listhead)->Depth

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList(
    IN PSLIST_HEADER ListHead);

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)
NTKERNELAPI
PSINGLE_LIST_ENTRY 
FASTCALL
ExInterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY 
FASTCALL
ExInterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);
#else
#define ExInterlockedPopEntrySList(_ListHead, _Lock) \
    InterlockedPopEntrySList(_ListHead)
#define ExInterlockedPushEntrySList(_ListHead, _ListEntry, _Lock) \
    InterlockedPushEntrySList(_ListHead, _ListEntry)
#endif // _WIN2K_COMPAT_SLIST_USAGE

#endif // !defined(_WIN64)

/* ERESOURCE_THREAD
 * ExGetCurrentResourceThread(
 *     VOID);
 */
#define ExGetCurrentResourceThread() ((ERESOURCE_THREAD)PsGetCurrentThread())

#define ExReleaseResource(R) (ExReleaseResourceLite(R))

/* VOID
 * ExInitializeWorkItem(
 *     IN PWORK_QUEUE_ITEM Item,
 *     IN PWORKER_THREAD_ROUTINE Routine,
 *     IN PVOID Context)
 */
#define ExInitializeWorkItem(Item, Routine, Context) \
{ \
  (Item)->WorkerRoutine = Routine; \
  (Item)->Parameter = Context; \
  (Item)->List.Flink = NULL; \
}

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex(
    IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex(
    IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(
    IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe(
    IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe(
    IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceExclusiveLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedStarveExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedWaitForExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

#ifdef POOL_TAGGING
#define ExAllocatePool(p,n) ExAllocatePoolWithTag(p,n,' kdD')
#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

#ifdef POOL_TAGGING
#define ExAllocatePoolWithQuota(p,n) ExAllocatePoolWithQuotaTag(p,n,' kdD')
#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuotaTag(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority);

NTKERNELAPI
VOID
NTAPI
ExConvertExclusiveToSharedLite(
    IN PERESOURCE Resource);

NTKERNELAPI
NTSTATUS
NTAPI
ExCreateCallback(
    OUT PCALLBACK_OBJECT *CallbackObject,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN Create,
    IN BOOLEAN AllowMultipleCallbacks);

NTKERNELAPI
VOID
NTAPI
ExDeleteNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
VOID
NTAPI
ExDeletePagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
NTSTATUS
NTAPI
ExDeleteResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExFreePool(
    IN PVOID P);

#ifdef POOL_TAGGING
#define ExFreePool(P) ExFreePoolWithTag(P, 0)
#endif

NTKERNELAPI
VOID
NTAPI
ExFreePoolWithTag(
    IN PVOID P,
    IN ULONG Tag);

NTKERNELAPI
ULONG
NTAPI
ExGetExclusiveWaiterCount(
    IN PERESOURCE Resource);

NTKERNELAPI
KPROCESSOR_MODE
NTAPI
ExGetPreviousMode(
    VOID);

NTKERNELAPI
ULONG
NTAPI
ExGetSharedWaiterCount(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExInitializeNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate OPTIONAL,
    IN PFREE_FUNCTION Free OPTIONAL,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth);

NTKERNELAPI
VOID
NTAPI
ExInitializePagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate OPTIONAL,
    IN PFREE_FUNCTION Free OPTIONAL,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PKSPIN_LOCK Lock);

#if defined(_WIN64)
#define ExInterlockedAddLargeStatistic(Addend, Increment) \
    (VOID)InterlockedAdd64(&(Addend)->QuadPart, Increment)
#else
#define ExInterlockedAddLargeStatistic(Addend, Increment) \
    _InterlockedAddLargeStatistic((PLONGLONG)&(Addend)->QuadPart, Increment)
#endif

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong(
    IN PULONG Addend,
    IN ULONG Increment,
    PKSPIN_LOCK Lock);

#if defined(_AMD64_) || defined(_IA64_)
#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))
#elif defined(_X86_)
NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comperand);
#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExfInterlockedCompareExchange64(Destination, Exchange, Comperand)
#else
NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comparand,
    IN PKSPIN_LOCK Lock);
#endif

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList(
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsProcessorFeaturePresent(
    IN ULONG ProcessorFeature);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsResourceAcquiredExclusiveLite(
    IN PERESOURCE Resource);

NTKERNELAPI
ULONG
NTAPI
ExIsResourceAcquiredSharedLite(
    IN PERESOURCE Resource);

#define ExIsResourceAcquiredLite ExIsResourceAcquiredSharedLite

NTKERNELAPI
VOID
NTAPI
ExLocalTimeToSystemTime(
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime);

NTKERNELAPI
VOID
NTAPI
ExNotifyCallback(
    IN PCALLBACK_OBJECT CallbackObject,
    IN PVOID Argument1,
    IN PVOID Argument2);

NTKERNELAPI
VOID
NTAPI
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseStatus(
    IN NTSTATUS Status);

NTKERNELAPI
PVOID
NTAPI
ExRegisterCallback(
    IN PCALLBACK_OBJECT CallbackObject,
    IN PCALLBACK_FUNCTION CallbackFunction,
    IN PVOID CallbackContext);

NTKERNELAPI
NTSTATUS
NTAPI
ExReinitializeResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExReleaseResourceForThreadLite(
    IN PERESOURCE Resource,
    IN ERESOURCE_THREAD ResourceThreadId);

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointer(
    IN PERESOURCE Resource,
    IN PVOID OwnerPointer);

NTKERNELAPI
ULONG
NTAPI
ExSetTimerResolution(
    IN ULONG DesiredTime,
    IN BOOLEAN SetResolution);

NTKERNELAPI
VOID
NTAPI
ExSystemTimeToLocalTime(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime);

NTKERNELAPI
VOID
NTAPI
ExUnregisterCallback(
    IN PVOID CbRegistration);


#endif // (NTDDI_VERSION >= NTDDI_WIN2K)

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtection(
    IN OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExInitializeRundownProtection(
    OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtection(
    OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtection(
    IN OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExRundownCompleted(
    OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
BOOLEAN
NTAPI
ExVerifySuite(
    IN SUITE_TYPE SuiteType);

NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionRelease(
    IN OUT PEX_RUNDOWN_REF RunRef);

#endif // (NTDDI_VERSION >= NTDDI_WINXP)

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx(
    IN OUT PEX_RUNDOWN_REF RunRef,
    IN ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionEx(
    IN OUT PEX_RUNDOWN_REF RunRef,
    IN ULONG Count);
#endif // (NTDDI_VERSION >= NTDDI_WINXPSP2)

#if (NTDDI_VERSION >= NTDDI_WS03)

#endif // (NTDDI_VERSION >= NTDDI_WS03)

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
NTKERNELAPI
PEX_RUNDOWN_REF_CACHE_AWARE
ExAllocateCacheAwareRundownProtection(
    IN POOL_TYPE PoolType,
    IN ULONG PoolTag);

NTKERNELAPI
SIZE_T
ExSizeOfRundownProtectionCacheAware(VOID);

#endif // (NTDDI_VERSION >= NTDDI_WS03SP1)

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
NTSTATUS
ExInitializeLookasideListEx(
    OUT PLOOKASIDE_LIST_EX Lookaside,
    IN PALLOCATE_FUNCTION_EX Allocate OPTIONAL,
    IN PFREE_FUNCTION_EX Free OPTIONAL,
    IN POOL_TYPE PoolType,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth);
#endif


#if !defined(MIDL_PASS)

static __inline PVOID
ExAllocateFromNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside)
{
    PVOID Entry;

    Lookaside->L.TotalAllocates++;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses++;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }
    return Entry;
}

static __inline PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside)
{
    PVOID Entry;

    Lookaside->L.TotalAllocates++;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses++;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }
    return Entry;
}

static __inline VOID
ExFreeToNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID  Entry)
{
    Lookaside->L.TotalFrees++;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses++;
        (Lookaside->L.Free)(Entry);
    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
    }
}

static __inline VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID  Entry)
{
    Lookaside->L.TotalFrees++;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses++;
        (Lookaside->L.Free)(Entry);
    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
    }
}


#endif // !defined(MIDL_PASS)


#ifdef __cplusplus
}
#endif

#endif // _WDMDDK_
