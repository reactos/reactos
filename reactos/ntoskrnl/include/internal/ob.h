/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

typedef struct _OBP_SET_HANDLE_ATTRIBUTES_CONTEXT
{
    KPROCESSOR_MODE PreviousMode;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION Information;
} OBP_SET_HANDLE_ATTRIBUTES_CONTEXT, *POBP_SET_HANDLE_ATTRIBUTES_CONTEXT;

typedef struct _OBP_CLOSE_HANDLE_CONTEXT
{
    PHANDLE_TABLE HandleTable;
    KPROCESSOR_MODE AccessMode;
} OBP_CLOSE_HANDLE_CONTEXT, *POBP_CLOSE_HANDLE_CONTEXT;

#define GENERIC_ACCESS (GENERIC_READ |      \
                        GENERIC_WRITE |     \
                        GENERIC_EXECUTE |   \
                        GENERIC_ALL)

#define KERNEL_HANDLE_FLAG (1 << ((sizeof(HANDLE) * 8) - 1))
#define ObIsKernelHandle(Handle, ProcessorMode)                                \
  (((ULONG_PTR)(Handle) & KERNEL_HANDLE_FLAG) &&                               \
   ((ProcessorMode) == KernelMode))
#define ObKernelHandleToHandle(Handle)                                         \
  (HANDLE)((ULONG_PTR)(Handle) & ~KERNEL_HANDLE_FLAG)
#define ObMarkHandleAsKernelHandle(Handle)                                     \
  (HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_FLAG)
#define ObpGetHandleCountByHandleTable(HandleTable)                             \
    ((PHANDLE_TABLE)HandleTable)->HandleCount

extern KEVENT ObpDefaultObject;
extern POBJECT_TYPE ObpTypeObjectType;
extern POBJECT_TYPE ObSymbolicLinkType;
extern POBJECT_TYPE ObTypeObjectType;
extern POBJECT_DIRECTORY NameSpaceRoot;
extern POBJECT_DIRECTORY ObpTypeDirectoryObject;
extern PHANDLE_TABLE ObpKernelHandleTable;
extern WORK_QUEUE_ITEM ObpReaperWorkItem;
extern volatile PVOID ObpReaperList;
extern NPAGED_LOOKASIDE_LIST ObpNmLookasideList, ObpCiLookasideList;

BOOLEAN
NTAPI
ObpDeleteEntryDirectory(
    IN POBP_LOOKUP_CONTEXT Context
);

BOOLEAN
NTAPI
ObpInsertEntryDirectory(
    IN POBJECT_DIRECTORY Parent,
    IN POBP_LOOKUP_CONTEXT Context,
    IN POBJECT_HEADER ObjectHeader
);

PVOID
NTAPI
ObpLookupEntryDirectory(
    IN POBJECT_DIRECTORY Directory,
    IN PUNICODE_STRING Name,
    IN ULONG Attributes,
    IN UCHAR SearchShadow,
    IN POBP_LOOKUP_CONTEXT Context
);

VOID
NTAPI
ObInitSymbolicLinkImplementation(
    VOID
);

NTSTATUS
NTAPI
ObpCreateHandleTable(
    IN PEPROCESS Parent,
    IN PEPROCESS Process
);

VOID
NTAPI
ObKillProcess(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
ObFindObject(
    IN HANDLE RootHandle,
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID *ReturnedObject,
    IN POBJECT_TYPE ObjectType,
    IN POBP_LOOKUP_CONTEXT Context,
    IN PACCESS_STATE AccessState,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PVOID ParseContext,
    IN PVOID Insert
);

BOOLEAN
NTAPI
ObpSetHandleAttributes(
    IN PHANDLE_TABLE HandleTable,
    IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN PVOID Context
);

VOID
NTAPI
ObpDeleteNameCheck(
    IN PVOID Object
);

NTSTATUS
NTAPI
ObDuplicateObject(
    PEPROCESS SourceProcess,
    PEPROCESS TargetProcess,
    HANDLE SourceHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG HandleAttributes,
    ULONG Options
);

VOID
NTAPI
ObQueryDeviceMapInformation(
    IN PEPROCESS Process,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo
);

VOID
FASTCALL
ObpSetPermanentObject(
    IN PVOID ObjectBody,
    IN BOOLEAN Permanent
);

VOID
FASTCALL
ObpDeleteObject(
    IN PVOID Object
);

VOID
NTAPI
ObpReapObject(
    IN PVOID Unused
);

/* Security descriptor cache functions */

NTSTATUS
NTAPI
ObpInitSdCache(VOID);

NTSTATUS
NTAPI
ObpAddSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SourceSD,
    OUT PSECURITY_DESCRIPTOR *DestinationSD
);

NTSTATUS
NTAPI
ObpRemoveSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

PSECURITY_DESCRIPTOR
NTAPI
ObpReferenceCachedSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

VOID
NTAPI
ObpDereferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
FASTCALL
ObInitializeFastReference(
    IN PEX_FAST_REF FastRef,
    PVOID Object
);

PVOID
FASTCALL
ObFastReplaceObject(
    IN PEX_FAST_REF FastRef,
    PVOID Object
);

PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef);

VOID
FASTCALL
ObFastDereferenceObject(
    IN PEX_FAST_REF FastRef,
    PVOID Object
);

/* Object Create and Object Name Capture Functions */

NTSTATUS
STDCALL
ObpCaptureObjectName(
    IN PUNICODE_STRING CapturedName,
    IN PUNICODE_STRING ObjectName,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN AllocateFromLookaside
);

NTSTATUS
STDCALL
ObpCaptureObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN AllocateFromLookaside,
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    OUT PUNICODE_STRING ObjectName
);

VOID
static __inline
ObpReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* Check if we have a security descriptor */
    if (ObjectCreateInfo->SecurityDescriptor)
    {
        /* Release it */
        SeReleaseSecurityDescriptor(ObjectCreateInfo->SecurityDescriptor,
                                    ObjectCreateInfo->ProbeMode,
                                    TRUE);
        ObjectCreateInfo->SecurityDescriptor = NULL;
    }
}

PVOID
static __inline
ObpAllocateCapturedAttributes(IN PP_NPAGED_LOOKASIDE_NUMBER Type)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PVOID Buffer;
    PNPAGED_LOOKASIDE_LIST List;

    /* Get the P list first */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].P;

    /* Attempt allocation */
    List->L.TotalAllocates++;
    Buffer = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
    if (!Buffer)
    {
        /* Let the balancer know that the P list failed */
        List->L.AllocateMisses++;

        /* Try the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].L;
        List->L.TotalAllocates++;
        Buffer = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
        if (!Buffer)
        {
            /* Let the balancer know the L list failed too */
            List->L.AllocateMisses++;

            /* Allocate it */
            Buffer = List->L.Allocate(List->L.Type, List->L.Size, List->L.Tag);
        }
    }

    /* Return buffer */
    return Buffer;
}

VOID
static __inline
ObpFreeCapturedAttributes(IN PVOID Buffer,
                          IN PP_NPAGED_LOOKASIDE_NUMBER Type)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PNPAGED_LOOKASIDE_LIST List;

    /* Use the P List */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].P;
    List->L.TotalFrees++;

    /* Check if the Free was within the Depth or not */
    if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
    {
        /* Let the balancer know */
        List->L.FreeMisses++;

        /* Use the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].L;
        List->L.TotalFrees++;

        /* Check if the Free was within the Depth or not */
        if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
        {
            /* All lists failed, use the pool */
            List->L.FreeMisses++;
            List->L.Free(Buffer);
        }
    }
    else
    {
        /* The free was within the Depth */
        InterlockedPushEntrySList(&List->L.ListHead,
                                  (PSINGLE_LIST_ENTRY)Buffer);
    }
}

VOID
static __inline
ObpReleaseCapturedName(IN PUNICODE_STRING Name)
{
    /* We know this is a pool-allocation if the size doesn't match */
    if (Name->MaximumLength != 248)
    {
        ExFreePool(Name->Buffer);
    }
    else
    {
        /* Otherwise, free from the lookaside */
        ObpFreeCapturedAttributes(Name, LookasideNameBufferList);
    }
}

VOID
static __inline
ObpFreeAndReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* First release the attributes, then free them from the lookaside list */
    ObpReleaseCapturedAttributes(ObjectCreateInfo);
    ObpFreeCapturedAttributes(ObjectCreateInfo, LookasideCreateInfoList);
}

#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
