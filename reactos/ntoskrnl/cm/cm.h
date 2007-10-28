#ifndef __INCLUDE_CM_H
#define __INCLUDE_CM_H

#include "ntoskrnl/config/cm.h"

#ifdef DBG
#define CHECKED 1
#else
#define CHECKED 0
#endif

#define  REG_ROOT_KEY_NAME		L"\\Registry"
#define  REG_MACHINE_KEY_NAME		L"\\Registry\\Machine"
#define  REG_HARDWARE_KEY_NAME		L"\\Registry\\Machine\\HARDWARE"
#define  REG_DESCRIPTION_KEY_NAME	L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION"
#define  REG_DEVICEMAP_KEY_NAME		L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP"
#define  REG_RESOURCEMAP_KEY_NAME	L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP"
#define  REG_CLASSES_KEY_NAME		L"\\Registry\\Machine\\Software\\Classes"
#define  REG_SYSTEM_KEY_NAME		L"\\Registry\\Machine\\SYSTEM"
#define  REG_SOFTWARE_KEY_NAME		L"\\Registry\\Machine\\SOFTWARE"
#define  REG_SAM_KEY_NAME		L"\\Registry\\Machine\\SAM"
#define  REG_SEC_KEY_NAME		L"\\Registry\\Machine\\SECURITY"
#define  REG_USER_KEY_NAME		L"\\Registry\\User"
#define  REG_DEFAULT_USER_KEY_NAME	L"\\Registry\\User\\.Default"
#define  REG_CURRENT_USER_KEY_NAME	L"\\Registry\\User\\CurrentUser"

#define  SYSTEM_REG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SYSTEM_LOG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM.log"
#define  SOFTWARE_REG_FILE		L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  DEFAULT_USER_REG_FILE		L"\\SystemRoot\\System32\\Config\\DEFAULT"
#define  SAM_REG_FILE			L"\\SystemRoot\\System32\\Config\\SAM"
#define  SEC_REG_FILE			L"\\SystemRoot\\System32\\Config\\SECURITY"

#define  REG_SYSTEM_FILE_NAME		L"\\system"
#define  REG_SOFTWARE_FILE_NAME		L"\\software"
#define  REG_DEFAULT_USER_FILE_NAME	L"\\default"
#define  REG_SAM_FILE_NAME		L"\\sam"
#define  REG_SEC_FILE_NAME		L"\\security"

/* When set, the hive is not backed by a file.
   Therefore, it can not be flushed to disk. */
#define HIVE_NO_FILE    0x00000002

/* When set, a modified (dirty) hive is not synchronized automatically.
   Explicit synchronization (save/flush) works. */
#define HIVE_NO_SYNCH   0x00000004

#define IsNoFileHive(Hive)  ((Hive)->Flags & HIVE_NO_FILE)
#define IsNoSynchHive(Hive)  ((Hive)->Flags & HIVE_NO_SYNCH)


/* KEY_OBJECT.Flags */

/* When set, the key is scheduled for deletion, and all
   attempts to access the key must not succeed */
#define KO_MARKED_FOR_DELETE              0x00000001

/* Bits 31-22 (top 10 bits) of the cell index is the directory index */
#define CmiDirectoryIndex(CellIndex)(CellIndex & 0xffc000000)
/* Bits 21-12 (middle 10 bits) of the cell index is the table index */
#define CmiTableIndex(Cellndex)(CellIndex & 0x003ff000)
/* Bits 11-0 (bottom 12 bits) of the cell index is the byte offset */
#define CmiByteOffset(Cellndex)(CellIndex & 0x00000fff)


extern POBJECT_TYPE CmpKeyObjectType;
extern KSPIN_LOCK CmiKeyListLock;

extern ERESOURCE CmpRegistryLock;
extern EX_PUSH_LOCK CmpHiveListHeadLock;

/* Registry Callback Function */
typedef struct _REGISTRY_CALLBACK
{
    LIST_ENTRY ListEntry;
    EX_RUNDOWN_REF RundownRef;
    PEX_CALLBACK_FUNCTION Function;
    PVOID Context;
    LARGE_INTEGER Cookie;
    BOOLEAN PendingDelete;
} REGISTRY_CALLBACK, *PREGISTRY_CALLBACK;

NTSTATUS
CmiCallRegisteredCallbacks(IN REG_NOTIFY_CLASS Argument1,
                           IN PVOID Argument2);

#define VERIFY_BIN_HEADER(x) ASSERT(x->HeaderId == REG_BIN_ID)
#define VERIFY_KEY_CELL(x) ASSERT(x->Signature == CM_KEY_NODE_SIGNATURE)
#define VERIFY_ROOT_KEY_CELL(x) ASSERT(x->Signature == CM_KEY_NODE_SIGNATURE)
#define VERIFY_VALUE_CELL(x) ASSERT(x->Signature == CM_KEY_VALUE_SIGNATURE)
#define VERIFY_VALUE_LIST_CELL(x)
#define VERIFY_KEY_OBJECT(x)
#define VERIFY_REGISTRY_HIVE(x)

NTSTATUS STDCALL
CmRegisterCallback(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID                 Context,
                   IN OUT PLARGE_INTEGER    Cookie
                    );

NTSTATUS STDCALL
CmUnRegisterCallback(IN LARGE_INTEGER    Cookie);

VOID
CmiAddKeyToList(IN PKEY_OBJECT ParentKey,
		IN PKEY_OBJECT NewKey);

NTSTATUS
CmiScanKeyList(IN PKEY_OBJECT Parent,
	       IN PCUNICODE_STRING KeyName,
	       IN ULONG Attributes,
	       PKEY_OBJECT* ReturnedObject);

NTSTATUS
CmiLoadHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	    PCUNICODE_STRING FileName,
	    ULONG Flags);

NTSTATUS
CmiFlushRegistryHive(PCMHIVE RegistryHive);

NTSTATUS
CmiScanForSubKey(IN PCMHIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 OUT PCM_KEY_NODE *SubKeyCell,
		 OUT HCELL_INDEX *BlockOffset,
		 IN PCUNICODE_STRING KeyName,
		 IN ACCESS_MASK DesiredAccess,
		 IN ULONG Attributes);

NTSTATUS
CmiScanKeyForValue(IN PCMHIVE RegistryHive,
		   IN PCM_KEY_NODE KeyCell,
		   IN PUNICODE_STRING ValueName,
		   OUT PCM_KEY_VALUE *ValueCell,
		   OUT HCELL_INDEX *VBOffset);


NTSTATUS
CmiConnectHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	       PCMHIVE RegistryHive);

NTSTATUS
CmiInitHives(BOOLEAN SetupBoot);

VOID
CmiSyncHives(VOID);

NTSTATUS
NTAPI
CmFindObject(
    POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    PUNICODE_STRING ObjectName,
    PVOID* ReturnedObject,
    PUNICODE_STRING RemainingPath,
    POBJECT_TYPE ObjectType,
    IN PACCESS_STATE AccessState,
    IN PVOID ParseContext
);

// Some Ob definitions for debug messages in Cm
#define ObGetObjectPointerCount(x) OBJECT_TO_OBJECT_HEADER(x)->PointerCount
#define ObGetObjectHandleCount(x) OBJECT_TO_OBJECT_HEADER(x)->HandleCount

#endif /*__INCLUDE_CM_H*/
