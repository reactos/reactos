/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/allocators.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsLoadResource(
    IN  PVOID ImageBase,
    IN  POOL_TYPE PoolType,
    IN  ULONG_PTR ResourceName,
    IN  ULONG ResourceType,
    OUT PVOID* Resource,
    OUT PULONG ResourceSize)
{
    NTSTATUS Status;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    PVOID Data;
    ULONG Size;
    PVOID Result = NULL;

    /* set up resource info */
    ResourceInfo.Type = ResourceType;
    ResourceInfo.Name = ResourceName;
    ResourceInfo.Language = 0;

    _SEH2_TRY
    {
        /* find the resource */
        Status = LdrFindResource_U(ImageBase, &ResourceInfo, RESOURCE_DATA_LEVEL, &ResourceDataEntry);
        if (NT_SUCCESS(Status))
        {
            /* try accessing it */
            Status = LdrAccessResource(ImageBase, ResourceDataEntry, &Data, &Size);
            if (NT_SUCCESS(Status))
            {
                /* allocate resource buffer */
                Result = ExAllocatePool(PoolType, Size);
                if (Result)
                {
                    /* copy resource */
                    RtlMoveMemory(Result, Data, Size);
                    /* store result */
                    *Resource = Result;
                    *ResourceSize = Size;
                }
                else
                {
                    /* not enough memory */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Exception, get the error code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        if (Result)
        {
            /* free resource buffer in case of a failure */
            ExFreePool(Result);
        }
    }
    /* done */
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGetImageNameAndResourceId(
    IN HANDLE RegKey,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG_PTR ResourceId,
    OUT PULONG ValueType)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsMapModuleName(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING ModuleName,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG_PTR ResourceId,
    OUT PULONG ValueType)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
