#ifndef _WDMDDK_
#define _WDMDDK_

//
// Dependencies
//
#define NT_INCLUDED
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

#ifdef __GNUC__
#include "intrin.h"
#endif

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



//
// Global debug flag
//
extern ULONG NtGlobalFlag;



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

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse

#endif


#if defined(_M_IX86)
#define YieldProcessor() __asm__ __volatile__("pause");
#elif defined (_M_AMD64)
#define YieldProcessor() __asm__ __volatile__("pause");
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

typedef union _SLIST_HEADER {
    ULONGLONG Alignment;
    struct {
        SLIST_ENTRY Next;
        USHORT Depth;
        USHORT Sequence;
    } DUMMYSTRUCTNAME;
} SLIST_HEADER, *PSLIST_HEADER;

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
// Architecture Defined Contexts
//
#if defined(_M_IX86)
#define SIZE_OF_80387_REGISTERS	80
#define CONTEXT_i386	0x10000
#define CONTEXT_i486	0x10000
#define CONTEXT_CONTROL	(CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER	(CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	(CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	(CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define MAXIMUM_SUPPORTED_EXTENSION  512

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

typedef struct _FLOATING_SAVE_AREA {
    ULONG ControlWord;
    ULONG StatusWord;
    ULONG TagWord;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    UCHAR RegisterArea[SIZE_OF_80387_REGISTERS];
    ULONG Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

typedef struct _CONTEXT {
    ULONG ContextFlags;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;
    FLOATING_SAVE_AREA FloatSave;
    ULONG SegGs;
    ULONG SegFs;
    ULONG SegEs;
    ULONG SegDs;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;
    ULONG Ebp;
    ULONG Eip;
    ULONG SegCs;
    ULONG EFlags;
    ULONG Esp;
    ULONG SegSs;
    UCHAR ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;

#elif defined(_M_AMD64)

#define BitTest _bittest
#define BitTest64 _bittest64
#define BitTestAndComplement _bittestandcomplement
#define BitTestAndComplement64 _bittestandcomplement64
#define BitTestAndSet _bittestandset
#define BitTestAndSet64 _bittestandset64
#define BitTestAndReset _bittestandreset
#define BitTestAndReset64 _bittestandreset64
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndReset _interlockedbittestandreset
#define InterlockedBitTestAndReset64 _interlockedbittestandreset64

#elif defined(_M_ARM)

//
// FIXME: Move to armddk.h?
//

/* The following flags control the contents of the CONTEXT structure. */

#define CONTEXT_ARM    0x0000040
#define CONTEXT_CONTROL         (CONTEXT_ARM | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_ARM | 0x00000002L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER)

typedef struct _CONTEXT {
	/* The flags values within this flag control the contents of
	   a CONTEXT record.

	   If the context record is used as an input parameter, then
	   for each portion of the context record controlled by a flag
	   whose value is set, it is assumed that that portion of the
	   context record contains valid context. If the context record
	   is being used to modify a thread's context, then only that
	   portion of the threads context will be modified.

	   If the context record is used as an IN OUT parameter to capture
	   the context of a thread, then only those portions of the thread's
	   context corresponding to set flags will be returned.

	   The context record is never used as an OUT only parameter. */

	ULONG ContextFlags;

	/* This section is specified/returned if the ContextFlags word contains
	   the flag CONTEXT_INTEGER. */
	ULONG R0;
	ULONG R1;
	ULONG R2;
	ULONG R3;
	ULONG R4;
	ULONG R5;
	ULONG R6;
	ULONG R7;
	ULONG R8;
	ULONG R9;
	ULONG R10;
	ULONG R11;
	ULONG R12;

	ULONG Sp;
	ULONG Lr;
	ULONG Pc;
	ULONG Psr;
} CONTEXT;

#else
#error "Undefined processor architecture"
#endif



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



#endif // _WDMDDK_
