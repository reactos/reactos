/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/allocators.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#include <ntimage.h>
#include <ndk/ldrfuncs.h>

#define NDEBUG
#include <debug.h>

/*
    @implemented
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
    PVOID _SEH2_VOLATILE Result = NULL;

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
                Result = AllocateItem(PoolType, Size);
                if (Result)
                {
                    /* copy resource */
                    RtlMoveMemory(Result, Data, Size);
                    /* store result */
                    *Resource = Result;

                    if (ResourceSize)
                    {
                        /* resource size is optional */
                        *ResourceSize = Size;
                    }
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
            FreeItem(Result);
        }
    }
    /* done */
    return Status;
}


NTSTATUS
KspQueryRegValue(
    IN HANDLE KeyHandle,
    IN LPWSTR KeyName,
    IN PVOID Buffer,
    IN OUT PULONG BufferLength,
    OUT PULONG Type)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
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
    NTSTATUS Status;
    ULONG ImageLength;
    WCHAR ImagePath[] = {L"\\SystemRoot\\system32\\drivers\\"};

    /* first clear the provided ImageName */
    ImageName->Buffer = NULL;
    ImageName->Length = ImageName->MaximumLength = 0;

    ImageLength = 0;
    /* retrieve length of image name */
    Status = KspQueryRegValue(RegKey, L"Image", NULL, &ImageLength, NULL);

    if (Status != STATUS_BUFFER_OVERFLOW)
    {
        /* key value doesnt exist */
        return Status;
    }

    /* allocate image name buffer */
    ImageName->MaximumLength = (USHORT)(sizeof(ImagePath) + ImageLength);
    ImageName->Buffer = AllocateItem(PagedPool, ImageName->MaximumLength);

    /* check for success */
    if (!ImageName->Buffer)
    {
        /* insufficient memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy image name */
    RtlCopyMemory(ImageName->Buffer, ImagePath, sizeof(ImagePath));

    /* retrieve image name */
    Status = KspQueryRegValue(RegKey,
                              L"Image",
                              &ImageName->Buffer[sizeof(ImagePath) / sizeof(WCHAR)],
                              &ImageLength,
                              NULL);

    if (!NT_SUCCESS(Status))
    {
        /* unexpected error */
        FreeItem(ImageName->Buffer);
        return Status;
    }

    /* now query for resource id length*/
   ImageLength = 0;
   Status = KspQueryRegValue(RegKey, L"ResourceId", NULL, &ImageLength, ValueType);

    /* allocate resource id buffer*/
    *ResourceId = (ULONG_PTR)AllocateItem(PagedPool, ImageLength);

    /* check for success */
    if (!*ResourceId)
    {
        /* insufficient memory */
        FreeItem(ImageName->Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    /* now query for resource id */
    Status = KspQueryRegValue(RegKey, L"ResourceId", (PVOID)*ResourceId, &ImageLength, ValueType);

    if (!NT_SUCCESS(Status))
    {
        /* unexpected error */
        FreeItem(ImageName->Buffer);
        FreeItem((PVOID)*ResourceId);
    }

    /* return result */
    return Status;
}

/*
    @implemented
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
    NTSTATUS Status;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING Modules = RTL_CONSTANT_STRING(L"Modules\\");
    HANDLE hKey, hSubKey;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* first open device key */
    Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject, PLUGPLAY_REGKEY_DEVICE, GENERIC_READ, &hKey);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* initialize subkey buffer */
    SubKeyName.Length = 0;
    SubKeyName.MaximumLength = Modules.MaximumLength + ModuleName->MaximumLength;
    SubKeyName.Buffer = AllocateItem(PagedPool, SubKeyName.MaximumLength);

    /* check for success */
    if (!SubKeyName.Buffer)
    {
        /* not enough memory */
        ZwClose(hKey);
        return STATUS_NO_MEMORY;
    }

    /* build subkey string */
    RtlAppendUnicodeStringToString(&SubKeyName, &Modules);
    RtlAppendUnicodeStringToString(&SubKeyName, ModuleName);

    /* initialize subkey attributes */
    InitializeObjectAttributes(&ObjectAttributes, &SubKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hKey, NULL);

    /* now open the subkey */
    Status = ZwOpenKey(&hSubKey, GENERIC_READ, &ObjectAttributes);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        /* defer work */
        Status = KsGetImageNameAndResourceId(hSubKey, ImageName, ResourceId, ValueType);

        /* close subkey */
        ZwClose(hSubKey);
    }

    /* free subkey string */
    FreeItem(SubKeyName.Buffer);

    /* close device key */
    ZwClose(hKey);

    /* return status */
    return Status;
}
