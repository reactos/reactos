/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/types.h
 * PURPOSE:      Types used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * DEFINES:      _WIN64: 64-bit architecture
 *               _WIN32: 32-bit architecture (default)
 * UPDATE HISTORY:
 *               27/06/00: Created
 *               01/05/01: Portabillity changes
 */

#ifndef __INCLUDE_TYPES_H
#define __INCLUDE_TYPES_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef AS_INVOKED

#include <windef.h>
#include <winnt.h>
#include <windows.h>
#include <winreg.h>

#define PAGE_ROUND_UP(x) ( (((ULONG_PTR)x)%PAGE_SIZE) ? ((((ULONG_PTR)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG_PTR)x) )
#define PAGE_ROUND_DOWN(x) (((ULONG_PTR)x)&(~(PAGE_SIZE-1)))

typedef DWORD (STDCALL *PTHREAD_START_ROUTINE)(PVOID);

#define FILE_ATTRIBUTE_VALID_FLAGS 0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS 0x000031a7

#define IOCTL_CDROM_GET_DRIVE_GEOMETRY   CTL_CODE(FILE_DEVICE_CD_ROM, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)

#define THREAD_READ			(0x020048L)
#define THREAD_WRITE			(0x020037L)
#define THREAD_EXECUTE			(0x120000L)

enum
{
   DIRECTORY_QUERY,
   DIRECTORY_TRAVERSE,
   DIRECTORY_CREATE_OBJECT,
   DIRECTORY_CREATE_SUBDIRECTORY,
   DIRECTORY_ALL_ACCESS,
};

typedef enum _NT_PRODUCT_TYPE
{
   NtProductWinNt = 1,
   NtProductLanManNt,
   NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

// ReactOS specific
#define HEAP_NO_VALLOC (0x800)

#define EVENT_QUERY_STATE	(1)
#define EVENT_PAIR_ALL_ACCESS	(0x1f0000L)
#define SEMAPHORE_QUERY_STATE	(1)

typedef ULARGE_INTEGER TIME, *PTIME;

typedef struct
{
  ACE_HEADER Header;
} ACE, *PACE;

/* our own invention */
#define FLAG_TRACE_BIT 0x100
#define CONTEXT_DEBUGGER (CONTEXT_FULL | CONTEXT_FLOATING_POINT)

// ReactOS specific
#define EX_MAXIMUM_WAIT_OBJECTS (64)

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define CONSOLE_OUTPUT_MODE_VALID	0x03
#define CONSOLE_INPUT_MODE_VALID 	0x0f

typedef ULONG ACCESS_MODE, *PACCESS_MODE;

#include "except.h"



/* ReactOS specific */

#define MB_FLAGS_MEM_INFO         (0x1)
#define MB_FLAGS_BOOT_DEVICE      (0x2)
#define MB_FLAGS_COMMAND_LINE     (0x4)
#define MB_FLAGS_MODULE_INFO      (0x8)
#define MB_FLAGS_AOUT_SYMS        (0x10)
#define MB_FLAGS_ELF_SYMS         (0x20)
#define MB_FLAGS_MMAP_INFO        (0x40)
#define MB_FLAGS_DRIVES_INFO      (0x80)
#define MB_FLAGS_CONFIG_TABLE     (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME (0x200)
#define MB_FLAGS_APM_TABLE        (0x400)
#define MB_FLAGS_GRAPHICS_TABLE   (0x800)

typedef struct _LOADER_MODULE
{
   ULONG ModStart;
   ULONG ModEnd;
   ULONG String;
   ULONG Reserved;
} LOADER_MODULE, *PLOADER_MODULE;

typedef struct _ADDRESS_RANGE
{
   ULONG BaseAddrLow;
   ULONG BaseAddrHigh;
   ULONG LengthLow;
   ULONG LengthHigh;
   ULONG Type;
} ADDRESS_RANGE, *PADDRESS_RANGE;

typedef struct _LOADER_PARAMETER_BLOCK {
	ULONG Flags;
	ULONG MemLower;
	ULONG MemHigher;
	ULONG BootDevice;
	ULONG CommandLine;
	ULONG ModsCount;
	ULONG ModsAddr;
	UCHAR Syms[12];
	ULONG MmapLength;
	ULONG MmapAddr;
	ULONG DrivesCount;
	ULONG DrivesAddr;
	ULONG ConfigTable;
	ULONG BootLoaderName;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;


typedef struct _KTSS
{
  USHORT PreviousTask;
  USHORT Reserved1;
  ULONG  Esp0;
  USHORT Ss0;
  USHORT Reserved2;
  ULONG  Esp1;
  USHORT Ss1;
  USHORT Reserved3;
  ULONG  Esp2;
  USHORT Ss2;
  USHORT Reserved4;
  ULONG  Cr3;
  ULONG  Eip;
  ULONG  Eflags;
  ULONG  Eax;
  ULONG  Ecx;
  ULONG  Edx;
  ULONG  Ebx;
  ULONG  Esp;
  ULONG  Ebp;
  ULONG  Esi;
  ULONG  Edi;
  USHORT Es;
  USHORT Reserved5;
  USHORT Cs;
  USHORT Reserved6;
  USHORT Ss;
  USHORT Reserved7;
  USHORT Ds;
  USHORT Reserved8;
  USHORT Fs;
  USHORT Reserved9;
  USHORT Gs;
  USHORT Reserved10;
  USHORT Ldt;
  USHORT Reserved11;
  USHORT Trap;
  USHORT IoMapBase;
  UCHAR  IoBitmap[1];
} KTSS __attribute__((packed));

typedef struct _IACCESS_TOKEN
{
  TOKEN_SOURCE			TokenSource;               // 0x00
  LUID				TokenId;                   // 0x10
  LUID				AuthenticationId;          // 0x18
  LARGE_INTEGER			ExpirationTime;            // 0x20
  LUID				ModifiedId;                // 0x28
  ULONG				UserAndGroupCount;         // 0x30
  ULONG				PrivilegeCount;            // 0x34
  ULONG				VariableLength;            // 0x38
  ULONG				DynamicCharged;            // 0x3C
  ULONG				DynamicAvailable;          // 0x40
  ULONG				DefaultOwnerIndex;         // 0x44
  PSID_AND_ATTRIBUTES_ARRAY		UserAndGroups;             // 0x48
  PSID				PrimaryGroup;              // 0x4C
  PLUID_AND_ATTRIBUTES_ARRAY		Privileges;                // 0x50
  ULONG				Unknown1;                  // 0x54
  PACL				DefaultDacl;               // 0x58
  TOKEN_TYPE			TokenType;                 // 0x5C
  SECURITY_IMPERSONATION_LEVEL	ImpersonationLevel;        // 0x60
  UCHAR				TokenFlags;                // 0x64
  UCHAR				TokenInUse;                // 0x65
  UCHAR				Unused[2];                 // 0x66
  PVOID				ProxyData;                 // 0x68
  PVOID				AuditData;                 // 0x6c
  UCHAR				VariablePart[0];           // 0x70
} IACCESS_TOKEN, *PIACCESS_TOKEN;

struct _DIRECTORY_OBJECT;
struct _OBJECT_ATTRIBUTES;

typedef struct _OBJECT_TYPE
{
  /*
   * PURPOSE: Tag to be used when allocating objects of this type
   */
  ULONG Tag;

  /*
   * PURPOSE: Name of the type
   */
  UNICODE_STRING TypeName;
  
  /*
   * PURPOSE: Total number of objects of this type
   */
  ULONG TotalObjects;
  
  /*
   * PURPOSE: Total number of handles of this type
   */
  ULONG TotalHandles;
  
  /*
   * PURPOSE: Maximum objects of this type
   */
  ULONG MaxObjects;
  
   /*
    * PURPOSE: Maximum handles of this type
    */
  ULONG MaxHandles;
  
  /*
   * PURPOSE: Paged pool charge
   */
   ULONG PagedPoolCharge;
  
  /*
   * PURPOSE: Nonpaged pool charge
   */
  ULONG NonpagedPoolCharge;
  
  /*
   * PURPOSE: Mapping of generic access rights
   */
  PGENERIC_MAPPING Mapping;
  
  /*
   * PURPOSE: Dumps the object
   * NOTE: To be defined
   */
  VOID STDCALL (*Dump)(VOID);
  
  /*
   * PURPOSE: Opens the object
   * NOTE: To be defined
   */
  VOID STDCALL (*Open)(VOID);
  
   /*
    * PURPOSE: Called to close an object if OkayToClose returns true
    */
  VOID STDCALL (*Close)(PVOID ObjectBody,
			ULONG HandleCount);
  
  /*
   * PURPOSE: Called to delete an object when the last reference is removed
   */
  VOID STDCALL (*Delete)(PVOID ObjectBody);
  
  /*
   * PURPOSE: Called when an open attempts to open a file apparently
   * residing within the object
   * RETURNS
   *     STATUS_SUCCESS       NextObject was found
   *     STATUS_UNSUCCESSFUL  NextObject not found
   *     STATUS_REPARSE       Path changed, restart parsing the path
   */
   NTSTATUS STDCALL (*Parse)(PVOID ParsedObject,
			     PVOID *NextObject,
			     PUNICODE_STRING FullPath,
			     PWSTR *Path,
			     ULONG Attributes);
  
   /*
    */
  NTSTATUS STDCALL (*Security)(PVOID Object,
			       ULONG InfoClass,
			       PVOID Info,
			       PULONG InfoLength);
  
  /*
   */
  VOID STDCALL (*QueryName)(VOID);
   
  /*
   * PURPOSE: Called when a process asks to close the object
   */
  VOID STDCALL (*OkayToClose)(VOID);
  
  NTSTATUS STDCALL (*Create)(PVOID ObjectBody,
			     PVOID Parent,
			     PWSTR RemainingPath,
			     struct _OBJECT_ATTRIBUTES* ObjectAttributes);

  VOID STDCALL (*DuplicationNotify)(PEPROCESS DuplicateTo,
				    PEPROCESS DuplicateFrom,
				    PVOID Object);
} OBJECT_TYPE, *POBJECT_TYPE;


typedef struct _OBJECT_HEADER
/*
 * PURPOSE: Header for every object managed by the object manager
 */
{
   UNICODE_STRING Name;
   LIST_ENTRY Entry;
   LONG RefCount;
   LONG HandleCount;
   BOOLEAN CloseInProcess;
   BOOLEAN Permanent;
   struct _DIRECTORY_OBJECT* Parent;
   POBJECT_TYPE ObjectType;
   
   /*
    * PURPOSE: Object type
    * NOTE: This overlaps the first member of the object body
    */
   CSHORT Type;
   
   /*
    * PURPOSE: Object size
    * NOTE: This overlaps the second member of the object body
    */
   CSHORT Size;
   
   
} OBJECT_HEADER, *POBJECT_HEADER;

typedef struct _ROS_HANDLE_TABLE
{
   LIST_ENTRY ListHead;
   KSPIN_LOCK ListLock;
} ROS_HANDLE_TABLE, *PROS_HANDLE_TABLE;

extern POBJECT_TYPE ObDirectoryType;

typedef struct _KINTERRUPT
{
   ULONG Vector;
   KAFFINITY ProcessorEnableMask;
   PKSPIN_LOCK IrqLock;
   BOOLEAN Shareable;
   BOOLEAN FloatingSave;
   PKSERVICE_ROUTINE ServiceRoutine;
   PVOID ServiceContext;
   LIST_ENTRY Entry;
   KIRQL SynchLevel;
} KINTERRUPT, *PKINTERRUPT;

/* number of entries in the service descriptor tables */
#define SSDT_MAX_ENTRIES 4


#pragma pack(1)

typedef struct t_KeServiceDescriptorTableEntry {
                PSSDT               SSDT;
                PULONG              ServiceCounterTable;
                unsigned int        NumberOfServices;
                PSSPT               SSPT;

} KE_SERVICE_DESCRIPTOR_TABLE_ENTRY, *PKE_SERVICE_DESCRIPTOR_TABLE_ENTRY;

#pragma pack()


NTOSAPI KE_SERVICE_DESCRIPTOR_TABLE_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES];

extern
KE_SERVICE_DESCRIPTOR_TABLE_ENTRY
KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];


BOOLEAN
STDCALL
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	);

extern struct _EPROCESS* PsInitialSystemProcess;
extern POBJECT_TYPE PsProcessType;
extern POBJECT_TYPE PsThreadType;

BOOLEAN STDCALL
KdPollBreakIn(VOID);


typedef struct _REACTOS_COMMON_FCB_HEADER
{
  CSHORT NodeTypeCode;
  CSHORT NodeByteSize;
  struct _ROS_BCB* Bcb;
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
  LARGE_INTEGER ValidDataLength;
} REACTOS_COMMON_FCB_HEADER, *PREACTOS_COMMON_FCB_HEADER;

NTSTATUS STDCALL
NtCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class OPTIONAL,
            IN ULONG CreateOptions,
            IN PULONG Disposition OPTIONAL);

#define FILE_NON_DIRECTORY_FILE 0x40
#define FILE_CREATED 0x2


typedef enum _TRAVERSE_METHOD {
  TraverseMethodPreorder,
  TraverseMethodInorder,
  TraverseMethodPostorder
} TRAVERSE_METHOD;

typedef LONG STDCALL
(*PKEY_COMPARATOR)(IN PVOID  Key1,
  IN PVOID  Key2);

typedef BOOLEAN STDCALL
(*PTRAVERSE_ROUTINE)(IN PVOID  Context,
  IN PVOID  Key,
  IN PVOID  Value);

struct _BINARY_TREE_NODE;

typedef struct _BINARY_TREE
{
  struct _BINARY_TREE_NODE  * RootNode;
  PKEY_COMPARATOR  Compare;
  BOOLEAN  UseNonPagedPool;
  union {
    NPAGED_LOOKASIDE_LIST  NonPaged;
    PAGED_LOOKASIDE_LIST  Paged;
  } List;
  union {
    KSPIN_LOCK  NonPaged;
    FAST_MUTEX  Paged;
  } Lock;
} BINARY_TREE, *PBINARY_TREE;


struct _SPLAY_TREE_NODE;

typedef struct _SPLAY_TREE
{
  struct _SPLAY_TREE_NODE  * RootNode;
  PKEY_COMPARATOR  Compare;
  BOOLEAN  Weighted;
  BOOLEAN  UseNonPagedPool;
  union {
    NPAGED_LOOKASIDE_LIST  NonPaged;
    PAGED_LOOKASIDE_LIST  Paged;
  } List;
  union {
    KSPIN_LOCK  NonPaged;
    FAST_MUTEX  Paged;
  } Lock;
  PVOID  Reserved[4];
} SPLAY_TREE, *PSPLAY_TREE;


typedef struct _HASH_TABLE
{
  // Size of hash table in number of bits
  ULONG  HashTableSize;

  // Use non-paged pool memory?
  BOOLEAN  UseNonPagedPool;

  // Lock for this structure
  union {
    KSPIN_LOCK  NonPaged;
    FAST_MUTEX  Paged;
  } Lock;

  // Pointer to array of hash buckets with splay trees
  PSPLAY_TREE  HashTrees;
} HASH_TABLE, *PHASH_TABLE;


BOOLEAN STDCALL
ExInitializeBinaryTree(IN PBINARY_TREE  Tree,
  IN PKEY_COMPARATOR  Compare,
  IN BOOLEAN  UseNonPagedPool);

VOID STDCALL
ExDeleteBinaryTree(IN PBINARY_TREE  Tree);

VOID STDCALL
ExInsertBinaryTree(IN PBINARY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  Value);

BOOLEAN STDCALL
ExSearchBinaryTree(IN PBINARY_TREE  Tree,
  IN PVOID  Key,
  OUT PVOID  * Value);

BOOLEAN STDCALL
ExRemoveBinaryTree(IN PBINARY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  * Value);

BOOLEAN STDCALL
ExTraverseBinaryTree(IN PBINARY_TREE  Tree,
  IN TRAVERSE_METHOD  Method,
  IN PTRAVERSE_ROUTINE  Routine,
  IN PVOID  Context);

BOOLEAN STDCALL
ExInitializeSplayTree(IN PSPLAY_TREE  Tree,
  IN PKEY_COMPARATOR  Compare,
  IN BOOLEAN  Weighted,
  IN BOOLEAN  UseNonPagedPool);

VOID STDCALL
ExDeleteSplayTree(IN PSPLAY_TREE  Tree);

VOID STDCALL
ExInsertSplayTree(IN PSPLAY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  Value);

BOOLEAN STDCALL
ExSearchSplayTree(IN PSPLAY_TREE  Tree,
  IN PVOID  Key,
  OUT PVOID  * Value);

BOOLEAN STDCALL
ExRemoveSplayTree(IN PSPLAY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  * Value);

BOOLEAN STDCALL
ExWeightOfSplayTree(IN PSPLAY_TREE  Tree,
  OUT PULONG  Weight);

BOOLEAN STDCALL
ExTraverseSplayTree(IN PSPLAY_TREE  Tree,
  IN TRAVERSE_METHOD  Method,
  IN PTRAVERSE_ROUTINE  Routine,
  IN PVOID  Context);

BOOLEAN STDCALL
ExInitializeHashTable(IN PHASH_TABLE  HashTable,
  IN ULONG  HashTableSize,
  IN PKEY_COMPARATOR  Compare  OPTIONAL,
  IN BOOLEAN  UseNonPagedPool);

VOID STDCALL
ExDeleteHashTable(IN PHASH_TABLE  HashTable);

VOID STDCALL
ExInsertHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  IN PVOID  Value);

BOOLEAN STDCALL
ExSearchHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  OUT PVOID  * Value);

BOOLEAN STDCALL
ExRemoveHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  IN PVOID  * Value);



NTSTATUS STDCALL
NtAccessCheckAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
			   IN PHANDLE ObjectHandle,
			   IN POBJECT_ATTRIBUTES ObjectAttributes,
			   IN ACCESS_MASK DesiredAccess,
			   IN PGENERIC_MAPPING GenericMapping,
			   IN BOOLEAN ObjectCreation,
			   OUT PULONG GrantedAccess,
			   OUT PBOOLEAN AccessStatus,
			   OUT PBOOLEAN GenerateOnClose
	);

NTSTATUS
STDCALL
NtSetInformationObject (
	IN	HANDLE	ObjectHandle,
	IN	CINT	ObjectInformationClass,
	IN	PVOID	ObjectInformation,
	IN	ULONG	Length
	);

#define LPC_CONNECTION_REFUSED (LPC_TYPE)(LPC_MAXIMUM + 1)

//#define DEVICE_TYPE_FROM_CTL_CODE(ctlCode) (((ULONG)(ctlCode&0xffff0000))>>16)
#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

/*
 * PURPOSE: Irp flags
 */
enum
{
   IRP_NOCACHE = 0x1,
   IRP_PAGING_IO = 0x2,
   IRP_MOUNT_COMPLETION = 0x2,
   IRP_SYNCHRONOUS_API = 0x4,
   IRP_ASSOCIATED_IRP = 0x8,
   IRP_BUFFERED_IO = 0x10,
   IRP_DEALLOCATE_BUFFER = 0x20,
   IRP_INPUT_OPERATION = 0x40,
   IRP_SYNCHRONOUS_PAGING_IO = 0x40,
   IRP_CREATE_OPERATION = 0x80,
   IRP_READ_OPERATION = 0x100,
   IRP_WRITE_OPERATION = 0x200,
   IRP_CLOSE_OPERATION = 0x400,
   IRP_DEFER_IO_COMPLETION = 0x800,
   IRP_OB_QUERY_NAME = 0x1000,
   IRP_HOLD_DEVICE_QUEUE = 0x2000,
   IRP_RETRY_IO_COMPLETION = 0x4000
};

/*
 * PIO_STACK_LOCATION
 * IoGetFirstIrpStackLocation(
 *   IN PIRP  Irp)
 */
#define IoGetFirstIrpStackLocation(_Irp) \
  ((PIO_STACK_LOCATION)((_Irp) + 1))

/*
 * PIO_STACK_LOCATION
 * IoGetSpecificIrpStackLocation(
 *   IN PIRP  Irp,
 *   IN ULONG  Number)
 */
#define IoGetSpecificIrpStackLocation(_Irp, _Number) \
  (((PIO_STACK_LOCATION)((_Irp) + 1)) + (_Number))

/*
 * file creation flags 
 */
#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
//#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200

#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_TRANSACTED_MODE                    0x00200000
#define FILE_OPEN_OFFLINE_FILE                  0x00400000

#define FILE_VALID_OPTION_FLAGS                 0x007fffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00001036

typedef struct _IO_PIPE_CREATE_BUFFER
{
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   ULONG MaxInstances;
   ULONG InBufferSize;
   ULONG OutBufferSize;
   LARGE_INTEGER TimeOut;
} IO_PIPE_CREATE_BUFFER, *PIO_PIPE_CREATE_BUFFER;

typedef struct _IO_MAILSLOT_CREATE_BUFFER {
	ULONG Param; /* ?? */
	ULONG MaxMessageSize;
	LARGE_INTEGER TimeOut;
} IO_MAILSLOT_CREATE_BUFFER, *PIO_MAILSLOT_CREATE_BUFFER;

/*
 * PURPOSE: Special timer associated with each device
 * NOTES: This is a guess
 */
typedef struct _IO_TIMER
{
   KTIMER timer;
   KDPC dpc;
} IO_TIMER, *PIO_TIMER;


#include "ntos/hal.h"

extern ULONG KdDebugState;
extern KD_PORT_INFORMATION GdbPortInfo;
extern KD_PORT_INFORMATION LogPortInfo;


typedef struct _LDR_RESOURCE_INFO
{
    ULONG Type;
    ULONG Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

#define RESOURCE_TYPE_LEVEL      0
#define RESOURCE_NAME_LEVEL      1
#define RESOURCE_LANGUAGE_LEVEL  2
#define RESOURCE_DATA_LEVEL      3



#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
#define FO_ALERTABLE_IO                 0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FO_WRITE_THROUGH                0x00000010
#define FO_SEQUENTIAL_ONLY              0x00000020
#define FO_CACHE_SUPPORTED              0x00000040
#define FO_NAMED_PIPE                   0x00000080
#define FO_STREAM_FILE                  0x00000100
#define FO_MAILSLOT                     0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
#define FO_DIRECT_DEVICE_OPEN           0x00000800
#define FO_FILE_MODIFIED                0x00001000
#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
#define FO_TEMPORARY_FILE               0x00008000
#define FO_DELETE_ON_CLOSE              0x00010000
#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000


/*
 * ReactOS specific flags
 */
#define FO_DIRECT_CACHE_READ            0x72000001
#define FO_DIRECT_CACHE_WRITE           0x72000002
#define FO_DIRECT_CACHE_PAGING_READ     0x72000004
#define FO_DIRECT_CACHE_PAGING_WRITE    0x72000008
#define FO_FCB_IS_VALID                 0x72000010


extern LOADER_PARAMETER_BLOCK KeLoaderBlock;


#define MUTANT_INCREMENT                  1

NTSTATUS STDCALL
NtCreateMutant(OUT PHANDLE MutantHandle,
	       IN ACCESS_MASK DesiredAccess,
	       IN POBJECT_ATTRIBUTES ObjectAttributes,
	       IN BOOLEAN InitialOwner);

NTSTATUS STDCALL
NtOpenMutant(OUT PHANDLE MutantHandle,
	     IN ACCESS_MASK DesiredAccess,
	     IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS STDCALL
NtQueryMutant(IN HANDLE MutantHandle,
	      IN CINT MutantInformationClass,
	      OUT PVOID MutantInformation,
	      IN ULONG Length,
	      OUT PULONG ResultLength);

NTSTATUS STDCALL
NtReleaseMutant(IN HANDLE MutantHandle,
		IN PULONG ReleaseCount OPTIONAL);

VOID STDCALL
KeInitializeMutant(IN PKMUTANT Mutant,
		   IN BOOLEAN InitialOwner);

LONG STDCALL
KeReadStateMutant(IN PKMUTANT Mutant);

LONG STDCALL
KeReleaseMutant(IN PKMUTANT Mutant,
		IN KPRIORITY Increment,
		IN BOOLEAN Abandon,
		IN BOOLEAN Wait);


typedef struct _ROS_OBJECT_TYPE_INFORMATION
{
	UNICODE_STRING	Name;
	UNICODE_STRING Type;
	ULONG TotalHandles;
	ULONG ReferenceCount;
} ROS_OBJECT_TYPE_INFORMATION, *PROS_OBJECT_TYPE_INFORMATION;


VOID STDCALL
PsImpersonateClient(PETHREAD Thread,
		    PACCESS_TOKEN Token,
		    UCHAR b,
		    UCHAR c,
		    SECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS STDCALL
PsAssignImpersonationToken(PETHREAD Thread,
			   HANDLE TokenHandle);

VOID STDCALL 
KiDispatchInterrupt(VOID);


NTSTATUS STDCALL
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID *Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL);

NTSTATUS STDCALL
LdrFindResource_U(PVOID BaseAddress,
                  PLDR_RESOURCE_INFO ResourceInfo,
                  ULONG Level,
                  PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry);

typedef TIME_ZONE_INFORMATION *PTIME_ZONE_INFORMATION;

PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
				  IN ULONG Alignment);

/*
 * PVOID
 * MmGetSystemAddressForMdl (
 *	PMDL Mdl
 *	);
 *
 * FUNCTION:
 *	Maps the physical pages described by an MDL into system space
 *
 * ARGUMENTS:
 *	Mdl = mdl
 *
 * RETURNS:
 *	The base system address for the mapped buffer
 */
#define MmGetSystemAddressForMdl(Mdl) \
	(((Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
		((Mdl)->MappedSystemVa):(MmMapLockedPages((Mdl),KernelMode)))

VOID STDCALL
KeEnterKernelDebugger(VOID);

#ifdef _NTOSKRNL_
extern DECL_EXPORT ULONG DpcQueueSize;
#else
extern DECL_IMPORT ULONG DpcQueueSize;
#endif

VOID STDCALL 
KiDeliverApc(ULONG Unknown1,
	     ULONG Unknown2,
	     ULONG Unknown3);

VOID STDCALL
KeFlushWriteBuffer(VOID);

#if 0
typedef struct _HANDLE_TABLE {
  // Not used by ReactOS
} HANDLE_TABLE, *PHANDLE_TABLE;
#endif

typedef struct _ROS_BCB
{
  LIST_ENTRY BcbSegmentListHead;
  PFILE_OBJECT FileObject;
  ULONG CacheSegmentSize;
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
  KSPIN_LOCK BcbLock;
  ULONG RefCount;
} ROS_BCB, *PROS_BCB;


NTSTATUS STDCALL 
CcRosInitializeFileCache(PFILE_OBJECT FileObject,
		      PROS_BCB* Bcb,
		      ULONG CacheSegmentSize);

NTSTATUS STDCALL 
CcRosReleaseFileCache(PFILE_OBJECT FileObject, PROS_BCB Bcb);

#define DebugDbgLoadSymbols (DEBUG_CONTROL_CODE)(DebugMaximum + 1)

#define WM_DROPOBJECT 544
#define WM_QUERYDROPOBJECT 555

typedef WINDOWPOS *PWINDOWPOS;

#define SECURITY_CREATOR_OWNER_SERVER_RID 0x2
#define SECURITY_CREATOR_GROUP_SERVER_RID 0x3
#define SECURITY_LOGON_IDS_RID_COUNT 0x3
#define SECURITY_ANONYMOUS_LOGON_RID 0x7
#define SECURITY_PROXY_RID 0x8
#define SECURITY_ENTERPRISE_CONTROLLERS_RID 0x9
#define SECURITY_SERVER_LOGON_RID SECURITY_ENTERPRISE_CONTROLLERS_RID
#define SECURITY_AUTHENTICATED_USER_RID 0xB
#define SECURITY_RESTRICTED_CODE_RID 0xC
#define SECURITY_NT_NON_UNIQUE_RID 0x15
#define SE_MIN_WELL_KNOWN_PRIVILEGE 2
#define SE_CREATE_TOKEN_PRIVILEGE 2
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE 3
#define SE_LOCK_MEMORY_PRIVILEGE 4
#define SE_INCREASE_QUOTA_PRIVILEGE 5
#define SE_UNSOLICITED_INPUT_PRIVILEGE 6
#define SE_MACHINE_ACCOUNT_PRIVILEGE 6
#define SE_TCB_PRIVILEGE 7
#define SE_SECURITY_PRIVILEGE 8
#define SE_TAKE_OWNERSHIP_PRIVILEGE 9
#define SE_LOAD_DRIVER_PRIVILEGE 10
#define SE_SYSTEM_PROFILE_PRIVILEGE 11
#define SE_SYSTEMTIME_PRIVILEGE 12
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 13
#define SE_INC_BASE_PRIORITY_PRIVILEGE 14
#define SE_CREATE_PAGEFILE_PRIVILEGE 15
#define SE_CREATE_PERMANENT_PRIVILEGE 16
#define SE_BACKUP_PRIVILEGE 17
#define SE_RESTORE_PRIVILEGE 18
#define SE_SHUTDOWN_PRIVILEGE 19
#define SE_DEBUG_PRIVILEGE 20
#define SE_AUDIT_PRIVILEGE 21
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE 22
#define SE_CHANGE_NOTIFY_PRIVILEGE 23
#define SE_REMOTE_SHUTDOWN_PRIVILEGE 24
#define SE_MAX_WELL_KNOWN_PRIVILEGE SE_REMOTE_SHUTDOWN_PRIVILEGE

typedef short INT16;

BOOL STDCALL GdiDllInitialize(HANDLE, DWORD, LPVOID);

#include <ddk/winddi.h>

typedef struct _ROS_PALOBJ
{
  XLATEOBJ *logicalToSystem;
  int *mapping;
  PLOGPALETTE logpalette; // _MUST_ be the last field
} ROS_PALOBJ, *PROS_PALOBJ;

typedef struct _ROS_BRUSHOBJ 
{
  ULONG  iSolidColor;
  PVOID  pvRbrush;

  /*  remainder of fields are for GDI internal use  */
  LOGBRUSH  logbrush;
} ROS_BRUSHOBJ, *PROS_BRUSHOBJ;

HBRUSH STDCALL W32kCreateDIBPatternBrushPt(CONST VOID  *PackedDIB, UINT  Usage);

#define GA_PARENT 	1

#define DCX_USESTYLE	0x00010000L

#define HELP_SETWINPOS	0x0203L

typedef ENUMRECTS *PENUMRECTS;

#define FSRTL_TAG 	TAG('F','S','r','t')

NTSTATUS STDCALL
ObRosCreateObject(OUT PHANDLE Handle,
	       IN ACCESS_MASK DesiredAccess,
	       IN POBJECT_ATTRIBUTES ObjectAttributes,
	       IN POBJECT_TYPE Type,
	       OUT PVOID *Object);

#endif /* !AS_INVOKED */

#endif /* __INCLUDE_TYPES_H */
